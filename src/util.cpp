// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#if defined(HAVE_CONFIG_H)
#include <config/navcoin-config.h>
#endif

#include <util.h>
#include <fs.h>

#include <chainparamsbase.h>
#include <main.h>
#include <miner.h>
#include <net.h>
#include <random.h>
#include <serialize.h>
#include <sync.h>
#include <utilstrencodings.h>
#include <utiltime.h>

#include <boost/algorithm/string.hpp>

#include <stdarg.h>

#if (defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__DragonFly__))
#include <pthread.h>
#include <pthread_np.h>
#endif

#ifndef WIN32
// for posix_fallocate
#ifdef __linux__

#ifdef _POSIX_C_SOURCE
#undef _POSIX_C_SOURCE
#endif

#define _POSIX_C_SOURCE 200112L

#endif // __linux__

#include <algorithm>
#include <fcntl.h>
#include <sys/resource.h>
#include <sys/stat.h>

// We need this for __getch function
#include <termios.h>
#include <unistd.h>

#else

#ifdef _MSC_VER
#pragma warning(disable:4786)
#pragma warning(disable:4804)
#pragma warning(disable:4805)
#pragma warning(disable:4717)
#endif

#ifdef _WIN32_WINNT
#undef _WIN32_WINNT
#endif
#define _WIN32_WINNT 0x0501

#ifdef _WIN32_IE
#undef _WIN32_IE
#endif
#define _WIN32_IE 0x0501

#define WIN32_LEAN_AND_MEAN 1
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <io.h> /* for _commit */
#include <shlobj.h>

// We need this for ReadConsoleA
// and other windows cli functions
#include <windows.h>
#endif

#ifdef HAVE_SYS_PRCTL_H
#include <sys/prctl.h>
#endif

#include <boost/algorithm/string/case_conv.hpp> // for to_lower()
#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string/predicate.hpp> // for startswith() and endswith()
#include <boost/thread.hpp>
#include <openssl/crypto.h>
#include <openssl/rand.h>
#include <openssl/conf.h>

const char * const NAVCOIN_CONF_FILENAME = "navcoin.conf";
const char * const NAVCOIN_PID_FILENAME = "navcoin.pid";
const char * const DEFAULT_WALLET_DAT = "wallet.dat";

std::vector<std::pair<std::string, bool>> vAddedProposalVotes;
std::vector<std::pair<std::string, bool>> vAddedPaymentRequestVotes;

std::map<uint256, int64_t> mapAddedVotes;
std::map<uint256, bool> mapSupported;

std::map<std::string, std::string> mapArgs;
std::map<std::string, std::vector<std::string> > mapMultiArgs;
bool fDebug = false;
bool fPrintToConsole = false;
bool fPrintToDebugLog = true;
bool fDaemon = false;
bool fServer = false;
bool fTorServer = false;
std::string strMiscWarning;
bool fLogTimestamps = DEFAULT_LOGTIMESTAMPS;
bool fLogTimeMicros = DEFAULT_LOGTIMEMICROS;
bool fLogIPs = DEFAULT_LOGIPS;
std::atomic<bool> fReopenLogFiles(false);
CTranslationInterface translationInterface;

/** Init OpenSSL library multithreading support */
static CCriticalSection** ppmutexOpenSSL;
void locking_callback(int mode, int i, const char* file, int line) NO_THREAD_SAFETY_ANALYSIS
{
    if (mode & CRYPTO_LOCK) {
        ENTER_CRITICAL_SECTION(*ppmutexOpenSSL[i]);
    } else {
        LEAVE_CRITICAL_SECTION(*ppmutexOpenSSL[i]);
    }
}

// Init
class CInit
{
public:
    CInit()
    {
        // Init OpenSSL library multithreading support
        ppmutexOpenSSL = (CCriticalSection**)OPENSSL_malloc(CRYPTO_num_locks() * sizeof(CCriticalSection*));
        for (int i = 0; i < CRYPTO_num_locks(); i++)
            ppmutexOpenSSL[i] = new CCriticalSection();
        CRYPTO_set_locking_callback(locking_callback);

        // OpenSSL can optionally load a config file which lists optional loadable modules and engines.
        // We don't use them so we don't require the config. However some of our libs may call functions
        // which attempt to load the config file, possibly resulting in an exit() or crash if it is missing
        // or corrupt. Explicitly tell OpenSSL not to try to load the file. The result for our libs will be
        // that the config appears to have been loaded and there are no modules/engines available.
        OPENSSL_no_config();

#ifdef WIN32
        // Seed OpenSSL PRNG with current contents of the screen
        RAND_screen();
#endif

        // Seed OpenSSL PRNG with performance counter
        RandAddSeed();
    }
    ~CInit()
    {
        // Securely erase the memory used by the PRNG
        RAND_cleanup();
        // Shutdown OpenSSL library multithreading support
        CRYPTO_set_locking_callback(NULL);
        for (int i = 0; i < CRYPTO_num_locks(); i++)
            delete ppmutexOpenSSL[i];
        OPENSSL_free(ppmutexOpenSSL);
    }
}
instance_of_cinit;

/**
 * LogPrintf() has been broken a couple of times now
 * by well-meaning people adding mutexes in the most straightforward way.
 * It breaks because it may be called by global destructors during shutdown.
 * Since the order of destruction of static/global objects is undefined,
 * defining a mutex as a global object doesn't work (the mutex gets
 * destroyed, and then some later destructor calls OutputDebugStringF,
 * maybe indirectly, and you get a core dump at shutdown trying to lock
 * the mutex).
 */

static boost::once_flag debugPrintInitFlag = BOOST_ONCE_INIT;
static boost::once_flag errorPrintInitFlag = BOOST_ONCE_INIT;

/**
 * We use boost::call_once() to make sure mutexDebugLog and
 * vMsgsBeforeOpenDebugLog are initialized in a thread-safe manner.
 *
 * NOTE: fileoutDebugLog, fileoutErrorLog, mutexDebugLog and sometimes
 * vMsgsBeforeOpenDebugLog are leaked on exit. This is ugly, but will be cleaned
 * up by the OS/libc. When the shutdown sequence is fully audited and
 * tested, explicit destruction of these objects can be implemented.
 */
static FILE* fileoutDebugLog = nullptr;
static FILE* fileoutErrorLog = nullptr;
static boost::mutex* mutexDebugLog = nullptr;
static boost::mutex* mutexErrorLog = nullptr;
static std::list<std::string> *vMsgsBeforeOpenDebugLog;
static std::list<std::string> *vMsgsBeforeOpenErrorLog;

static int FileWriteStr(const std::string &str, FILE *fp)
{
    return fwrite(str.data(), 1, str.size(), fp);
}

static int DebugLogWriteStr(const std::string &str)
{
    // Return the int from the write size
    return FileWriteStr(str, fileoutDebugLog); // write to debug log
}

static int ErrorLogWriteStr(const std::string &str)
{
    // Return the int from the write size
    return FileWriteStr(str, fileoutDebugLog) + // write to debug log
        FileWriteStr(str, fileoutErrorLog); // write to error log
}

static void DebugPrintInit()
{
    assert(mutexDebugLog == nullptr);
    mutexDebugLog = new boost::mutex();
    vMsgsBeforeOpenDebugLog = new std::list<std::string>;
}

static void ErrorPrintInit()
{
    assert(mutexErrorLog == nullptr);
    mutexErrorLog = new boost::mutex();
    vMsgsBeforeOpenErrorLog = new std::list<std::string>;
}

void OpenDebugLog()
{
    boost::call_once(&DebugPrintInit, debugPrintInitFlag);
    boost::call_once(&ErrorPrintInit, errorPrintInitFlag);
    boost::mutex::scoped_lock scoped_lock_debug(*mutexDebugLog);
    boost::mutex::scoped_lock scoped_lock_error(*mutexErrorLog);

    assert(fileoutDebugLog == nullptr);
    assert(fileoutErrorLog == nullptr);
    assert(vMsgsBeforeOpenDebugLog);
    assert(vMsgsBeforeOpenErrorLog);

    // Open the debug log
    fileoutDebugLog = fopen(GetDebugLogPath().string().c_str(), "a");
    if (fileoutDebugLog) setbuf(fileoutDebugLog, nullptr); // unbuffered

    // Open the error log
    fileoutErrorLog = fopen(GetErrorLogPath().string().c_str(), "a");
    if (fileoutErrorLog) setbuf(fileoutErrorLog, nullptr); // unbuffered

    // dump buffered messages from before we opened the log
    while (!vMsgsBeforeOpenDebugLog->empty()) {
        DebugLogWriteStr(vMsgsBeforeOpenDebugLog->front()); // write to log
        vMsgsBeforeOpenDebugLog->pop_front();
    }

    // dump buffered messages from before we opened the log
    while (!vMsgsBeforeOpenErrorLog->empty()) {
        ErrorLogWriteStr(vMsgsBeforeOpenErrorLog->front()); // write to log
        vMsgsBeforeOpenErrorLog->pop_front();
    }

    delete vMsgsBeforeOpenDebugLog;
    delete vMsgsBeforeOpenErrorLog;
    vMsgsBeforeOpenDebugLog = nullptr;
    vMsgsBeforeOpenErrorLog = nullptr;
}

bool LogAcceptCategory(const char* category)
{
    if (category != nullptr)
    {
        if (!fDebug)
            return false;

        // Give each thread quick access to -debug settings.
        // This helps prevent issues debugging global destructors,
        // where mapMultiArgs might be deleted before another
        // global destructor calls LogPrint()
        static boost::thread_specific_ptr<std::set<std::string> > ptrCategory;
        if (ptrCategory.get() == nullptr)
        {
            const std::vector<std::string>& categories = mapMultiArgs["-debug"];
            ptrCategory.reset(new std::set<std::string>(categories.begin(), categories.end()));
            // thread_specific_ptr automatically deletes the set when the thread ends.
        }
        const std::set<std::string>& setCategories = *ptrCategory.get();

        // if not debugging everything and not debugging specific category, LogPrint does nothing.
        if (setCategories.count(std::string("")) == 0 &&
            setCategories.count(std::string("1")) == 0 &&
            setCategories.count(std::string(category)) == 0)
            return false;
    }

    return true;
}

/**
 * fStartedNewLine is a state variable held by the calling context that will
 * suppress printing of the timestamp when multiple calls are made that don't
 * end in a newline. Initialize it to true, and hold it, in the calling context.
 */
static std::string LogTimestampStr(const std::string &str, bool *fStartedNewLine)
{
    std::string strStamped;

    if (!fLogTimestamps)
        return str;

    if (*fStartedNewLine) {
        int64_t nTimeMicros = GetLogTimeMicros();
        strStamped = DateTimeStrFormat("%Y-%m-%d %H:%M:%S", nTimeMicros/1000000);
        if (fLogTimeMicros)
            strStamped += strprintf(".%06d", nTimeMicros%1000000);
        strStamped += ' ' + str;
    } else
        strStamped = str;

    if (!str.empty() && str[str.size()-1] == '\n')
        *fStartedNewLine = true;
    else
        *fStartedNewLine = false;

    return strStamped;
}

fs::path GetDebugLogPath()
{
    return GetDataDir() / "debug.log";
}

fs::path GetErrorLogPath()
{
    return GetDataDir() / "error.log";
}

int DebugLogPrintStr(const std::string &str)
{
    int ret = 0; // Returns total number of characters written
    static bool fStartedNewLine = true;

    std::string strTimestamped = LogTimestampStr(str, &fStartedNewLine);

    if (fPrintToConsole)
    {
        // print to console
        ret = fwrite(strTimestamped.data(), 1, strTimestamped.size(), stdout);
        fflush(stdout);
    }
    else if (fPrintToDebugLog)
    {
        boost::call_once(&DebugPrintInit, debugPrintInitFlag);
        boost::mutex::scoped_lock scoped_lock_debug(*mutexDebugLog);

        // buffer if we haven't opened the log yet
        if (fileoutDebugLog == nullptr) {
            assert(vMsgsBeforeOpenDebugLog);
            ret = strTimestamped.length();
            vMsgsBeforeOpenDebugLog->push_back(strTimestamped);
        }
        else
        {
            // reopen the log file, if requested
            if (fReopenLogFiles) {
                fReopenLogFiles = false;

                // Open the log files
                OpenDebugLog();
            }

            ret = DebugLogWriteStr(strTimestamped);
        }
    }

    return ret;
}

int ErrorLogPrintStr(const std::string &str)
{
    int ret = 0; // Returns total number of characters written
    static bool fStartedNewLine = true;

    std::string strTimestamped = LogTimestampStr(str, &fStartedNewLine);

    if (fPrintToConsole)
    {
        // print to console
        ret = fwrite(strTimestamped.data(), 1, strTimestamped.size(), stdout);
        fflush(stdout);
    }
    else if (fPrintToDebugLog)
    {
        boost::call_once(&ErrorPrintInit, errorPrintInitFlag);
        boost::mutex::scoped_lock scoped_lock_error(*mutexErrorLog);

        // buffer if we haven't opened the log yet
        if (fileoutErrorLog == nullptr) {
            assert(vMsgsBeforeOpenErrorLog);
            ret = strTimestamped.length();
            vMsgsBeforeOpenErrorLog->push_back(strTimestamped);
        }
        else
        {
            // reopen the log file, if requested
            if (fReopenLogFiles) {
                fReopenLogFiles = false;

                // Open the log files
                OpenDebugLog();
            }

            ret = ErrorLogWriteStr(strTimestamped);
        }
    }

    return ret;
}

/** Interpret string as boolean, for argument parsing */
static bool InterpretBool(const std::string& strValue)
{
    if (strValue.empty())
        return true;
    return (atoi(strValue) != 0);
}

/** Turn -noX into -X=0 */
static void InterpretNegativeSetting(std::string& strKey, std::string& strValue)
{
    if (strKey.length()>3 && strKey[0]=='-' && strKey[1]=='n' && strKey[2]=='o')
    {
        strKey = "-" + strKey.substr(3);
        strValue = InterpretBool(strValue) ? "0" : "1";
    }
}

void ParseParameters(int argc, const char* const argv[])
{
    mapArgs.clear();
    mapMultiArgs.clear();

    for (int i = 1; i < argc; i++)
    {
        std::string str(argv[i]);
        std::string strValue;
        size_t is_index = str.find('=');
        if (is_index != std::string::npos)
        {
            strValue = str.substr(is_index+1);
            str = str.substr(0, is_index);
        }
#ifdef WIN32
        boost::to_lower(str);
        if (boost::algorithm::starts_with(str, "/"))
            str = "-" + str.substr(1);
#endif

        if (str[0] != '-')
            break;

        // Interpret --foo as -foo.
        // If both --foo and -foo are set, the last takes effect.
        if (str.length() > 1 && str[1] == '-')
            str = str.substr(1);
        InterpretNegativeSetting(str, strValue);

        mapArgs[str] = strValue;
        mapMultiArgs[str].push_back(strValue);
    }
}

std::string GetArg(const std::string& strArg, const std::string& strDefault)
{
    if (mapArgs.count(strArg))
        return mapArgs[strArg];
    return strDefault;
}

int64_t GetArg(const std::string& strArg, int64_t nDefault)
{
    if (mapArgs.count(strArg))
        return atoi64(mapArgs[strArg]);
    return nDefault;
}

bool GetBoolArg(const std::string& strArg, bool fDefault)
{
    if (mapArgs.count(strArg))
        return InterpretBool(mapArgs[strArg]);
    return fDefault;
}

bool SoftSetArg(const std::string& strArg, const std::string& strValue, bool force)
{
    if (mapArgs.count(strArg) && !force)
        return false;
    mapArgs[strArg] = strValue;
    return true;
}

bool SoftSetBoolArg(const std::string& strArg, bool fValue)
{
    if (fValue)
        return SoftSetArg(strArg, std::string("1"));
    else
        return SoftSetArg(strArg, std::string("0"));
}

static const int screenWidth = 79;
static const int optIndent = 2;
static const int msgIndent = 7;

std::string HelpMessageGroup(const std::string &message) {
    return std::string(message) + std::string("\n\n");
}

std::string HelpMessageOpt(const std::string &option, const std::string &message) {
    return std::string(optIndent,' ') + std::string(option) +
           std::string("\n") + std::string(msgIndent,' ') +
           FormatParagraph(message, screenWidth - msgIndent, msgIndent) +
           std::string("\n\n");
}

static std::string FormatException(const std::exception* pex, const char* pszThread)
{
#ifdef WIN32
    char pszModule[MAX_PATH] = "";
    GetModuleFileNameA(NULL, pszModule, sizeof(pszModule));
#else
    const char* pszModule = "navcoin";
#endif
    if (pex)
        return strprintf(
            "EXCEPTION: %s       \n%s       \n%s in %s       \n", typeid(*pex).name(), pex->what(), pszModule, pszThread);
    else
        return strprintf(
            "UNKNOWN EXCEPTION       \n%s in %s       \n", pszModule, pszThread);
}

void PrintExceptionContinue(const std::exception* pex, const char* pszThread)
{
    std::string message = FormatException(pex, pszThread);
    LogPrintf("\n\n************************\n%s\n", message);
    fprintf(stderr, "\n\n************************\n%s\n", message.c_str());
}

fs::path GetDefaultDataDir()
{
    namespace fs = boost::filesystem;
    // Windows < Vista: C:\Documents and Settings\Username\Application Data\NavCoin4
    // Windows >= Vista: C:\Users\Username\AppData\Roaming\NavCoin4
    // Mac: ~/Library/Application Support/NavCoin4
    // Unix: ~/.navcoin4
#ifdef WIN32
    // Windows
    return GetSpecialFolderPath(CSIDL_APPDATA) / "NavCoin4";
#else
    fs::path pathRet;
    char* pszHome = getenv("HOME");
    if (pszHome == NULL || strlen(pszHome) == 0)
        pathRet = fs::path("/");
    else
        pathRet = fs::path(pszHome);
#ifdef MAC_OSX
    // Mac
    return pathRet / "Library/Application Support/NavCoin4";
#else
    // Unix
    return pathRet / ".navcoin4";
#endif
#endif
}

static fs::path pathCached;
static fs::path pathCachedNetSpecific;
static CCriticalSection csPathCached;

bool CheckIfWalletDatExists(bool fNetSpecific) {

    namespace fs = boost::filesystem;

    fs::path path(GetArg("-wallet", DEFAULT_WALLET_DAT));
    if (!path.is_complete())
        path = GetDataDir(fNetSpecific) / path;

    return fs::exists(path);
}

const fs::path &GetDataDir(bool fNetSpecific)
{
    namespace fs = boost::filesystem;

    LOCK(csPathCached);

    fs::path &path = fNetSpecific ? pathCachedNetSpecific : pathCached;

    // Cache the path to avoid calling fs::create_directories on every call of
    // this function
    if (!path.empty()) return path;

    std::string datadir = GetArg("-datadir", "");
    if (!datadir.empty()) {
        if (datadir[0] == '~') {
            char const* home = getenv("HOME");
            if (home or ((home = getenv("USERPROFILE")))) {
                datadir.replace(0, 1, home);
            }
        }

        path = fs::system_complete(datadir);

        if (!fs::is_directory(path)) {
            path = "";
            return path;
        }
    } else {
        path = GetDefaultDataDir();
    }
    if (fNetSpecific)
        path /= BaseParams().DataDir();

    fs::create_directories(path);

    return path;
}

void ClearDatadirCache()
{
    pathCached = fs::path();
    pathCachedNetSpecific = fs::path();
}

fs::path GetConfigFile()
{
    fs::path pathConfigFile(GetArg("-conf", NAVCOIN_CONF_FILENAME));
    if (!pathConfigFile.is_complete())
        pathConfigFile = GetDataDir(false) / pathConfigFile;

    return pathConfigFile;
}

static std::string TrimString(const std::string& str, const std::string& pattern)
{
    std::string::size_type front = str.find_first_not_of(pattern);
    if (front == std::string::npos) {
        return std::string();

    }
    std::string::size_type end = str.find_last_not_of(pattern);
    return str.substr(front, end - front + 1);

}

static std::vector<std::pair<std::string, std::string>> GetConfigOptions(std::istream& stream)
{
    std::vector<std::pair<std::string, std::string>> options;
    std::string str, prefix;
    std::string::size_type pos;
    while (std::getline(stream, str)) {
        if ((pos = str.find('#')) != std::string::npos) {
            str = str.substr(0, pos);

        }
        const static std::string pattern = " \t\r\n";
        str = TrimString(str, pattern);
        if (!str.empty()) {
            if (*str.begin() == '[' && *str.rbegin() == ']') {
                prefix = str.substr(1, str.size() - 2) + '.';

            } else if ((pos = str.find('=')) != std::string::npos) {
                std::string name = prefix + TrimString(str.substr(0, pos), pattern);
                std::string value = TrimString(str.substr(pos + 1), pattern);
                options.emplace_back(name, value);

            }

        }

    }
    return options;
}

void ReadConfigFile(std::map<std::string, std::string>& mapSettingsRet,
                    std::map<std::string, std::vector<std::string> >& mapMultiSettingsRet)
{
    fs::ifstream stream(GetConfigFile());
    if (!stream.good())
        return; // No navcoin.conf file is OK

    for (const std::pair<std::string, std::string>& option : GetConfigOptions(stream)) {
        std::string strKey = std::string("-") + option.first;
        std::string strValue = option.second;

        if(strKey == "-addproposalvoteyes" || strKey == "-addpaymentrequestvoteyes" || strKey == "-voteyes")
        {
            mapAddedVotes[uint256S(strValue)]=1;
            continue;
        }

        else if(strKey == "-addproposalvoteno" || strKey == "-addpaymentrequestvoteno" || strKey == "-voteno")
        {
            mapAddedVotes[uint256S(strValue)]=0;
            continue;
        }

        else if(strKey == "-addproposalvoteabs" || strKey == "-addpaymentrequestvotabs" || strKey == "-voteabs")
        {
            mapAddedVotes[uint256S(strValue)]=-1;
            continue;
        }

        else if(strKey == "-support")
        {
            mapSupported[uint256S(strValue)]=true;
        }

        else if(strKey == "-vote" && strValue.find("~") != std::string::npos)
        {
            std::string sHash = strValue.substr(0, strValue.find("~"));
            std::string sVote = strValue.substr(strValue.find("~")+1, strValue.size());
            mapAddedVotes[uint256S(sHash)]=stoull(sVote);
        }

        else if(strKey == "-stakervote")
        {
            mapArgs[strKey] = strValue;
        }

        InterpretNegativeSetting(strKey, strValue);
        if (mapSettingsRet.count(strKey) == 0)
            mapSettingsRet[strKey] = strValue;
        mapMultiSettingsRet[strKey].push_back(strValue);
    }
    // If datadir is changed in .conf file:
    ClearDatadirCache();
}

void WriteConfigFile(std::string key, std::string value)
{
    bool alreadyInConfigFile = false;
    fs::ifstream stream(GetConfigFile());

    if(stream.good())
    {

        std::set<std::string> setOptions;
        setOptions.insert("*");
        for (const std::pair<std::string, std::string>& option : GetConfigOptions(stream)) {
            std::string strKey = std::string("-") + option.first;
            std::string strValue = option.second;
            if(strKey == key && strValue == value)
                alreadyInConfigFile = true;
        }
    }

    if(!alreadyInConfigFile)
    {
        fs::ofstream outStream(GetConfigFile(), std::ios_base::app);
        outStream << std::endl << key + std::string("=") + value;
        outStream.close();
    }

}

bool ExistsKeyInConfigFile(std::string key)
{

    fs::ifstream stream(GetConfigFile());

    if(stream.good())
    {

        std::set<std::string> setOptions;
        setOptions.insert("*");

        for (const std::pair<std::string, std::string>& option : GetConfigOptions(stream)) {
            std::string strKey = std::string("-") + option.first;
            if(strKey == key)
                return true;
        }
    }

    return false;

}

void RemoveConfigFile(std::string key, std::string value)
{
    fs::ifstream stream(GetConfigFile());
    if (!stream.good())
        return; // Nothing to remove

    std::string configBuffer, line;
    std::set<std::string> setOptions;
    setOptions.insert("*");

    while (std::getline(stream, line))
    {
          if(line != key + "=" + value && line != "")
              configBuffer += line + "\n";
    }

    fs::ofstream outStream(GetConfigFile());
    outStream << configBuffer;
    outStream.close();
}

void RemoveConfigFilePair(std::string key, std::string value)
{
    fs::ifstream stream(GetConfigFile());
    if (!stream.good())
        return; // Nothing to remove

    std::string configBuffer, line;
    std::set<std::string> setOptions;
    setOptions.insert("*");

    while (std::getline(stream, line))
    {
          if(line.substr(0,key.length()+1+value.length()) != key + "=" + value && line != "")
              configBuffer += line + "\n";
    }

    fs::ofstream outStream(GetConfigFile());
    outStream << configBuffer;
    outStream.close();
}

void RemoveConfigFile(std::string key)
{
    fs::ifstream stream(GetConfigFile());
    if (!stream.good())
        return; // Nothing to remove

    std::string configBuffer, line;
    std::set<std::string> setOptions;
    setOptions.insert("*");

    while (std::getline(stream, line))
    {
          if(line.substr(0,key.length()+1) != key + "=" && line != "")
              configBuffer += line + "\n";
    }

    fs::ofstream outStream(GetConfigFile());
    outStream << configBuffer;
    outStream.close();
}
#ifndef WIN32
fs::path GetPidFile()
{
    fs::path pathPidFile(GetArg("-pid", NAVCOIN_PID_FILENAME));
    if (!pathPidFile.is_complete()) pathPidFile = GetDataDir() / pathPidFile;
    return pathPidFile;
}

void CreatePidFile(const fs::path &path, pid_t pid)
{
    FILE* file = fopen(path.string().c_str(), "w");
    if (file)
    {
        fprintf(file, "%d\n", pid);
        fclose(file);
    }
}
#endif

bool RenameOver(fs::path src, fs::path dest)
{
#ifdef WIN32
    return MoveFileExA(src.string().c_str(), dest.string().c_str(),
                       MOVEFILE_REPLACE_EXISTING) != 0;
#else
    int rc = std::rename(src.string().c_str(), dest.string().c_str());
    return (rc == 0);
#endif /* WIN32 */
}

/**
 * Ignores exceptions thrown by Boost's create_directory if the requested directory exists.
 * Specifically handles case where path p exists, but it wasn't possible for the user to
 * write to the parent directory.
 */
bool TryCreateDirectory(const fs::path& p)
{
    try
    {
        return fs::create_directory(p);
    } catch (const fs::filesystem_error&) {
        if (!fs::exists(p) || !fs::is_directory(p))
            throw;
    }

    // create_directory didn't create the directory, it had to have existed already
    return false;
}

void FileCommit(FILE *file)
{
    fflush(file); // harmless if redundantly called
#ifdef WIN32
    HANDLE hFile = (HANDLE)_get_osfhandle(_fileno(file));
    FlushFileBuffers(hFile);
#else
    #if defined(__linux__) || defined(__NetBSD__)
    fdatasync(fileno(file));
    #elif defined(__APPLE__) && defined(F_FULLFSYNC)
    fcntl(fileno(file), F_FULLFSYNC, 0);
    #else
    fsync(fileno(file));
    #endif
#endif
}

bool TruncateFile(FILE *file, unsigned int length) {
#if defined(WIN32)
    return _chsize(_fileno(file), length) == 0;
#else
    return ftruncate(fileno(file), length) == 0;
#endif
}

/**
 * this function tries to raise the file descriptor limit to the requested number.
 * It returns the actual file descriptor limit (which may be more or less than nMinFD)
 */
int RaiseFileDescriptorLimit(int nMinFD) {
#if defined(WIN32)
    return 2048;
#else
    struct rlimit limitFD;
    if (getrlimit(RLIMIT_NOFILE, &limitFD) != -1) {
        if (limitFD.rlim_cur < (rlim_t)nMinFD) {
            limitFD.rlim_cur = nMinFD;
            if (limitFD.rlim_cur > limitFD.rlim_max)
                limitFD.rlim_cur = limitFD.rlim_max;
            setrlimit(RLIMIT_NOFILE, &limitFD);
            getrlimit(RLIMIT_NOFILE, &limitFD);
        }
        return limitFD.rlim_cur;
    }
    return nMinFD; // getrlimit failed, assume it's fine
#endif
}

/**
 * this function tries to make a particular range of a file allocated (corresponding to disk space)
 * it is advisory, and the range specified in the arguments will never contain live data
 */
void AllocateFileRange(FILE *file, unsigned int offset, unsigned int length) {
#if defined(WIN32)
    // Windows-specific version
    HANDLE hFile = (HANDLE)_get_osfhandle(_fileno(file));
    LARGE_INTEGER nFileSize;
    int64_t nEndPos = (int64_t)offset + length;
    nFileSize.u.LowPart = nEndPos & 0xFFFFFFFF;
    nFileSize.u.HighPart = nEndPos >> 32;
    SetFilePointerEx(hFile, nFileSize, 0, FILE_BEGIN);
    SetEndOfFile(hFile);
#elif defined(MAC_OSX)
    // OSX specific version
    fstore_t fst;
    fst.fst_flags = F_ALLOCATECONTIG;
    fst.fst_posmode = F_PEOFPOSMODE;
    fst.fst_offset = 0;
    fst.fst_length = (off_t)offset + length;
    fst.fst_bytesalloc = 0;
    if (fcntl(fileno(file), F_PREALLOCATE, &fst) == -1) {
        fst.fst_flags = F_ALLOCATEALL;
        fcntl(fileno(file), F_PREALLOCATE, &fst);
    }
    ftruncate(fileno(file), fst.fst_length);
#elif defined(__linux__)
    // Version using posix_fallocate
    off_t nEndPos = (off_t)offset + length;
    posix_fallocate(fileno(file), 0, nEndPos);
#else
    // Fallback version
    // TODO: just write one byte per block
    static const char buf[65536] = {};
    fseek(file, offset, SEEK_SET);
    while (length > 0) {
        unsigned int now = 65536;
        if (length < now)
            now = length;
        fwrite(buf, 1, now, file); // allowed to fail; this function is advisory anyway
        length -= now;
    }
#endif
}

void ShrinkDebugFile()
{
    ShrinkDebugFile(GetDebugLogPath(), 10); // Shrink the debug log
    ShrinkDebugFile(GetErrorLogPath(),  2); // Shrink the error log
}

void ShrinkDebugFile(fs::path pathLog, int maxSize)
{
    // Scroll debug.log if it's getting too big
    FILE* file = fopen(pathLog.string().c_str(), "r");
    if (file && fs::file_size(pathLog) > maxSize * 1000000)
    {
        // Restart the file with some of the end
        std::vector <char> vch(200000,0);
        fseek(file, -((long)vch.size()), SEEK_END);
        int nBytes = fread(begin_ptr(vch), 1, vch.size(), file);
        fclose(file);

        file = fopen(pathLog.string().c_str(), "w");
        if (file)
        {
            fwrite(begin_ptr(vch), 1, nBytes, file);
            fclose(file);
        }
    }
    else if (file != nullptr)
        fclose(file);
}

#ifdef WIN32
fs::path GetSpecialFolderPath(int nFolder, bool fCreate)
{
    namespace fs = boost::filesystem;

    char pszPath[MAX_PATH] = "";

    if(SHGetSpecialFolderPathA(NULL, pszPath, nFolder, fCreate))
    {
        return fs::path(pszPath);
    }

    LogPrintf("SHGetSpecialFolderPathA() failed, could not obtain requested path.\n");
    return fs::path("");
}
#endif

void runCommand(const std::string& strCommand)
{
    int nErr = ::system(strCommand.c_str());
    if (nErr)
        LogPrintf("runCommand error: system(%s) returned %d\n", strCommand, nErr);
}

void RenameThread(const char* name)
{
#if defined(PR_SET_NAME)
    // Only the first 15 characters are used (16 - NUL terminator)
    ::prctl(PR_SET_NAME, name, 0, 0, 0);
#elif (defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__DragonFly__))
    pthread_set_name_np(pthread_self(), name);

#elif defined(MAC_OSX)
    pthread_setname_np(name);
#else
    // Prevent warnings for unused parameters...
    (void)name;
#endif
}

void SetupEnvironment()
{
    // On most POSIX systems (e.g. Linux, but not BSD) the environment's locale
    // may be invalid, in which case the "C" locale is used as fallback.
#if !defined(WIN32) && !defined(MAC_OSX) && !defined(__FreeBSD__) && !defined(__OpenBSD__)
    try {
        std::locale(""); // Raises a runtime error if current locale is invalid
    } catch (const std::runtime_error&) {
        setenv("LC_ALL", "C", 1);
    }
#endif
    // The path locale is lazy initialized and to avoid deinitialization errors
    // in multithreading environments, it is set explicitly by the main thread.
    // A dummy locale is used to extract the internal default locale, used by
    // fs::path, which is then used to explicitly imbue the path.
    std::locale loc = fs::path::imbue(std::locale::classic());
    fs::path::imbue(loc);
}

bool SetupNetworking()
{
#ifdef WIN32
    // Initialize Windows Sockets
    WSADATA wsadata;
    int ret = WSAStartup(MAKEWORD(2,2), &wsadata);
    if (ret != NO_ERROR || LOBYTE(wsadata.wVersion ) != 2 || HIBYTE(wsadata.wVersion) != 2)
        return false;
#endif
    return true;
}

int GetNumCores()
{
#if BOOST_VERSION >= 105600
    return boost::thread::physical_concurrency();
#else // Must fall back to hardware_concurrency, which unfortunately counts virtual cores
    return boost::thread::hardware_concurrency();
#endif
}

std::string CopyrightHolders(const std::string& strPrefix)
{
    std::string strCopyrightHolders = strPrefix + _(COPYRIGHT_HOLDERS);
    if (strCopyrightHolders.find("%s") != strCopyrightHolders.npos) {
        strCopyrightHolders = strprintf(strCopyrightHolders, _(COPYRIGHT_HOLDERS_SUBSTITUTION));
    }
    if (strCopyrightHolders.find("Bitcoin Core developers") == strCopyrightHolders.npos) {
        strCopyrightHolders += "\n" + strPrefix + "The Bitcoin Core developers";
    }
    return strCopyrightHolders;
}


void SetThreadPriority(int nPriority)
{
    // It's unclear if it's even possible to change thread priorities on Linux,
    // but we really and truly need it for the generation threads.
#ifdef WIN32
    SetThreadPriority(GetCurrentThread(), nPriority);
#else
#ifdef PRIO_THREAD
    setpriority(PRIO_THREAD, 0, nPriority);
#else
    setpriority(PRIO_PROCESS, 0, nPriority);
#endif
#endif
}

bool BdbEncrypted(fs::path wallet)
{
    // Open file
    std::ifstream file(wallet.string(), std::ifstream::binary);
    char* buffer = new char [5];

    // Get length of file
    file.seekg(0, std::ios::end);
    size_t length = file.tellg();

    // Loop the file contents
    for (int i = 0; i < length; i++) {
        // Reset our cursor
        file.seekg(i, file.beg);

        // Read data from the file
        file.read(buffer, 5);

        // Check if we have it
        if (std::string(buffer) == "main") {
            // Close the file
            file.close();

            // It's not encrypted
            return false;
        }
    }

    // Close the file
    file.close();

    // It's encrypted
    return true;
}

#ifndef WIN32
int __getch() {
    int ch;
    struct termios t_old, t_new;

    tcgetattr(STDIN_FILENO, &t_old);
    t_new = t_old;
    t_new.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &t_new);

    ch = getchar();

    tcsetattr(STDIN_FILENO, TCSANOW, &t_old);

    return ch;
}
#endif

std::string __getpass(const std::string& prompt, bool show_asterisk)
{
    std::string password;
    unsigned char ch = 0;

    std::cout << prompt;

#ifdef WIN32
    const char BACKSPACE = 8;
    const char RETURN = 13;

    DWORD con_mode;
    DWORD dwRead;

    HANDLE hIn=GetStdHandle(STD_INPUT_HANDLE);

    GetConsoleMode( hIn, &con_mode );
    SetConsoleMode( hIn, con_mode & ~(ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT) );
#else
    const char BACKSPACE = 127;
    const char RETURN = 10;
#endif

#ifndef WIN32
    while((ch = __getch()) != RETURN)
#else
    while(ReadConsoleA(hIn, &ch, 1, &dwRead, NULL) && ch != RETURN)
#endif
    {
        if(ch == BACKSPACE)
        {
            if(password.length() != 0)
            {
                if(show_asterisk)
                    std::cout << "\b \b";
                password.resize(password.length()-1);
            }
        }
        else
        {
            password+=ch;
            if(show_asterisk)
                std::cout << '*';
        }
    }

    std::cout << std::endl;

    return password;
}
