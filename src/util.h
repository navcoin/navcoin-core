// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/**
 * Server/client environment: argument handling, config file parsing,
 * logging, thread wrappers
 */
#ifndef NAVCOIN_UTIL_H
#define NAVCOIN_UTIL_H

#if defined(HAVE_CONFIG_H)
#include <config/navcoin-config.h>
#endif

#include <compat.h>
#include <tinyformat.h>
#include <uint256.h>
#include <utiltime.h>

#include <atomic>
#include <exception>
#include <map>
#include <stdint.h>
#include <string>
#include <vector>

#include <boost/filesystem.hpp>
#include <boost/signals2/signal.hpp>
#include <boost/thread/exceptions.hpp>

static const bool DEFAULT_LOGTIMEMICROS = false;
static const bool DEFAULT_LOGIPS        = false;
static const bool DEFAULT_LOGTIMESTAMPS = true;

/** Signals for translation. */
class CTranslationInterface
{
public:
    /** Translate a message to the native language of the user. */
    boost::signals2::signal<std::string (const char* psz)> Translate;
};

extern std::map<std::string, std::string> mapArgs;
extern std::map<std::string, std::vector<std::string> > mapMultiArgs;
extern std::map<uint256, int64_t> mapAddedVotes;
extern std::map<uint256, bool> mapSupported;
extern bool fDebug;
extern bool fPrintToConsole;
extern bool fPrintToDebugLog;
extern bool fServer;
extern bool fTorServer;
extern std::string strMiscWarning;
extern bool fLogTimestamps;
extern bool fLogTimeMicros;
extern bool fLogIPs;
extern std::atomic<bool> fReopenLogFiles;
extern CTranslationInterface translationInterface;

extern const char * const NAVCOIN_CONF_FILENAME;
extern const char * const NAVCOIN_PID_FILENAME;
extern const char * const DEFAULT_WALLET_DAT;

/**
 * Translation function: Call Translate signal on UI interface, which returns a boost::optional result.
 * If no translation slot is registered, nothing is returned, and simply return the input.
 */
inline std::string _(const char* psz)
{
    boost::optional<std::string> rv = translationInterface.Translate(psz);
    return rv ? (*rv) : psz;
}

void SetupEnvironment();
bool SetupNetworking();

/** Return true if log accepts specified category */
bool LogAcceptCategory(const char* category);

/** Returns the path to the debug.log file */
boost::filesystem::path GetDebugLogPath();

/** Returns the path to the error.log file */
boost::filesystem::path GetErrorLogPath();

/** Send a string to the debug log output */
int DebugLogPrintStr(const std::string &str);

/** Send a string to the error log output */
int ErrorLogPrintStr(const std::string &str);

#define LogPrintf(...) LogPrint(NULL, __VA_ARGS__)

template<typename T1, typename... Args>
static inline int LogPrint(const char* category, const char* fmt, const T1& v1, const Args&... args)
{
    if(!LogAcceptCategory(category)) return 0;
    std::string _log_msg_;
    try {
        _log_msg_ = tfm::format(fmt, v1, args...);
    } catch (std::runtime_error &e) {
        _log_msg_ = "Error \"" + std::string(e.what()) + "\" while formatting log message: " + fmt;
    }

    return DebugLogPrintStr(_log_msg_);
}

template<typename T1, typename... Args>
bool error(const char* fmt, const T1& v1, const Args&... args)
{
    ErrorLogPrintStr("ERROR: " + tfm::format(fmt, v1, args...) + "\n");
    return false;
}

template<typename T1, typename... Args>
int info(const char* fmt, const T1& v1, const Args&... args)
{
    return DebugLogPrintStr("INFO: " + tfm::format(fmt, v1, args...) + "\n");
}

/**
 * Zero-arg versions of logging and error, these are not covered by
 * the variadic templates above (and don't take format arguments but
 * bare strings).
 */
static inline int LogPrint(const char* category, const char* s)
{
    if(!LogAcceptCategory(category)) return 0;
    return DebugLogPrintStr(s);
}
static inline bool error(const char* s)
{
    ErrorLogPrintStr(std::string("ERROR: ") + s + "\n");
    return false;
}
static inline bool error(std::string s)
{
    return error(s.c_str());
}
static inline int info(const char* s)
{
    return DebugLogPrintStr(std::string("INFO: ") + s + "\n");
}
static inline bool info(std::string s)
{
    return info(s.c_str());
}
void PrintExceptionContinue(const std::exception *pex, const char* pszThread);
void ParseParameters(int argc, const char*const argv[]);
void FileCommit(FILE *file);
bool TruncateFile(FILE *file, unsigned int length);
int RaiseFileDescriptorLimit(int nMinFD);
void AllocateFileRange(FILE *file, unsigned int offset, unsigned int length);
bool RenameOver(boost::filesystem::path src, boost::filesystem::path dest);
bool TryCreateDirectory(const boost::filesystem::path& p);
boost::filesystem::path GetDefaultDataDir();
bool CheckIfWalletDatExists(bool fNetSpecific = true);
const boost::filesystem::path &GetDataDir(bool fNetSpecific = true);
void ClearDatadirCache();
boost::filesystem::path GetConfigFile();
#ifndef WIN32
boost::filesystem::path GetPidFile();
void CreatePidFile(const boost::filesystem::path &path, pid_t pid);
#endif
void ReadConfigFile(std::map<std::string, std::string>& mapSettingsRet, std::map<std::string, std::vector<std::string> >& mapMultiSettingsRet);
void WriteConfigFile(std::string key, std::string value);
void RemoveConfigFile(std::string key, std::string value);
void RemoveConfigFilePair(std::string key, std::string value);
void RemoveConfigFile(std::string key);
bool ExistsKeyInConfigFile(std::string key);
#ifdef WIN32
boost::filesystem::path GetSpecialFolderPath(int nFolder, bool fCreate = true);
#endif
void OpenDebugLog();
void ShrinkDebugFile();
void ShrinkDebugFile(boost::filesystem::path pathLog, int maxSize);
void runCommand(const std::string& strCommand);

inline bool IsSwitchChar(char c)
{
#ifdef WIN32
    return c == '-' || c == '/';
#else
    return c == '-';
#endif
}

void SetThreadPriority(int nPriority);

/**
 * Return string argument or default value
 *
 * @param strArg Argument to get (e.g. "-foo")
 * @param default (e.g. "1")
 * @return command-line argument or default value
 */
std::string GetArg(const std::string& strArg, const std::string& strDefault);

/**
 * Return integer argument or default value
 *
 * @param strArg Argument to get (e.g. "-foo")
 * @param default (e.g. 1)
 * @return command-line argument (0 if invalid number) or default value
 */
int64_t GetArg(const std::string& strArg, int64_t nDefault);

/**
 * Return boolean argument or default value
 *
 * @param strArg Argument to get (e.g. "-foo")
 * @param default (true or false)
 * @return command-line argument or default value
 */
bool GetBoolArg(const std::string& strArg, bool fDefault);

/**
 * Set an argument if it doesn't already have a value
 *
 * @param strArg Argument to set (e.g. "-foo")
 * @param strValue Value (e.g. "1")
 * @return true if argument gets set, false if it already had a value
 */
bool SoftSetArg(const std::string& strArg, const std::string& strValue, bool force=false);

/**
 * Set a boolean argument if it doesn't already have a value
 *
 * @param strArg Argument to set (e.g. "-foo")
 * @param fValue Value (e.g. false)
 * @return true if argument gets set, false if it already had a value
 */
bool SoftSetBoolArg(const std::string& strArg, bool fValue);

/**
 * Format a string to be used as group of options in help messages
 *
 * @param message Group name (e.g. "RPC server options:")
 * @return the formatted string
 */
std::string HelpMessageGroup(const std::string& message);

/**
 * Format a string to be used as option description in help messages
 *
 * @param option Option message (e.g. "-rpcuser=<user>")
 * @param message Option description (e.g. "Username for JSON-RPC connections")
 * @return the formatted string
 */
std::string HelpMessageOpt(const std::string& option, const std::string& message);

/**
 * Return the number of physical cores available on the current system.
 * @note This does not count virtual cores, such as those provided by HyperThreading
 * when boost is newer than 1.56.
 */
int GetNumCores();

void RenameThread(const char* name);

/**
 * .. and a wrapper that just calls func once
 */
template <typename Callable> void TraceThread(const char* name,  Callable func)
{
    std::string s = strprintf("navcoin-%s", name);
    RenameThread(s.c_str());
    try
    {
        LogPrintf("%s thread start\n", name);
        func();
        LogPrintf("%s thread exit\n", name);
    }
    catch (const boost::thread_interrupted&)
    {
        LogPrintf("%s thread interrupt\n", name);
        throw;
    }
    catch (const std::exception& e) {
        PrintExceptionContinue(&e, name);
        throw;
    }
    catch (...) {
        PrintExceptionContinue(NULL, name);
        throw;
    }
}

template <typename Callable, typename Arg1> void TraceThread(const char* name,  Callable func, Arg1 arg)
{
    std::string s = strprintf("navcoin-%s", name);
    RenameThread(s.c_str());
    try
    {
        LogPrintf("%s thread start\n", name);
        func(arg);
        LogPrintf("%s thread exit\n", name);
    }
    catch (const boost::thread_interrupted&)
    {
        LogPrintf("%s thread interrupt\n", name);
        throw;
    }
    catch (const std::exception& e) {
        PrintExceptionContinue(&e, name);
        throw;
    }
    catch (...) {
        PrintExceptionContinue(NULL, name);
        throw;
    }
}

std::string CopyrightHolders(const std::string& strPrefix);

bool BdbEncrypted(boost::filesystem::path wallet);

/*
 * These are used to get password input from user on cli
 */
int __getch();
std::string __getpass(const std::string& prompt, bool show_asterisk = true);

#endif // NAVCOIN_UTIL_H
