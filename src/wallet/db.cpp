// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <wallet/db.h>

#include <init.h>
#include <addrman.h>
#include <hash.h>
#include <protocol.h>
#include <util.h>
#include <utilstrencodings.h>
#include <ui_interface.h>

#include <stdint.h>

#ifndef WIN32
#include <sys/stat.h>
#endif

#include <boost/filesystem.hpp>
#include <boost/thread.hpp>
#include <boost/version.hpp>

using namespace std;


unsigned int nWalletDBUpdated;


//
// CDB
//

CDBEnv bitdb;

void CDBEnv::EnvShutdown()
{
    if (!fDbEnvInit)
        return;

    fDbEnvInit = false;
    int ret = dbenv->close(0);
    if (ret != 0)
        LogPrintf("CDBEnv::EnvShutdown: Error %d shutting down database environment: %s\n", ret, DbEnv::strerror(ret));
    if (!fMockDb)
        DbEnv(0).remove(strPath.c_str(), 0);
}

void CDBEnv::Reset()
{
    delete dbenv;
    dbenv = new DbEnv(DB_CXX_NO_EXCEPTIONS);
    fDbEnvInit = false;
    fMockDb = false;
}

CDBEnv::CDBEnv() : dbenv(NULL)
{
    Reset();
}

CDBEnv::~CDBEnv()
{
    info("CDBEnv::~CDBEnv destroy");
    EnvShutdown();
    delete dbenv;
    dbenv = nullptr;
}

void CDBEnv::Close()
{
    EnvShutdown();
}

bool CDBEnv::Open(const boost::filesystem::path& pathIn, std::string pin)
{
    if (fDbEnvInit)
        return true;

    // Check if we are encrypted and save the state for later use
    if (!pin.empty())
        fCrypted = true;

    boost::this_thread::interruption_point();

    strPath = pathIn.string();
    boost::filesystem::path pathLogDir = pathIn / "database";
    TryCreateDirectory(pathLogDir);
    boost::filesystem::path pathErrorFile = pathIn / "db.log";
    LogPrintf("CDBEnv::Open: LogDir=%s ErrorFile=%s\n", pathLogDir.string(), pathErrorFile.string());

    dbenv->set_lg_dir(pathLogDir.string().c_str());
    dbenv->set_cachesize(0, 0x100000, 1); // 1 MiB should be enough for just the wallet
    dbenv->set_lg_bsize(0x10000);
    dbenv->set_lg_max(1048576);
    dbenv->set_lk_max_locks(40000);
    dbenv->set_lk_max_objects(40000);
    dbenv->set_errfile(fopen(pathErrorFile.string().c_str(), "a")); /// debug
    dbenv->set_flags(DB_AUTO_COMMIT, 1);
    dbenv->set_flags(DB_TXN_WRITE_NOSYNC, 1);
    dbenv->log_set_config(DB_LOG_AUTO_REMOVE, 1);

    // Check if we got a pin
    if (fCrypted)
    {
        info("CDBEnv::Open: Encryption Enabled");

        // Enable encryption for the envirnment
        int cryptRet = dbenv->set_encrypt(pin.c_str(), DB_ENCRYPT_AES);

        // Check if it worked
        if (cryptRet != 0)
            return error("CDBEnv::Open: Error %d enabling database encryption: %s", cryptRet, DbEnv::strerror(cryptRet));
    } else {
        info("CDBEnv::Open: Encryption Disabled");
    }

    info("CDBEnv::Open: CALL DBEnv->open");
    info("CDBEnv::Open: DIR %s", strPath);
    int ret = dbenv->open(strPath.c_str(),
                         DB_CREATE |
                             DB_INIT_LOCK |
                             DB_INIT_LOG |
                             DB_INIT_MPOOL |
                             DB_INIT_TXN |
                             DB_THREAD |
                             DB_RECOVER |
                             DB_PRIVATE,
                         S_IRUSR | S_IWUSR);
    if (ret != 0)
        return error("CDBEnv::Open: Error %d opening database environment: %s\n", ret, DbEnv::strerror(ret));

    fDbEnvInit = true;
    fMockDb = false;
    return true;
}

void CDBEnv::MakeMock()
{
    if (fDbEnvInit)
        throw runtime_error("CDBEnv::MakeMock: Already initialized");

    boost::this_thread::interruption_point();

    LogPrint("db", "CDBEnv::MakeMock\n");

    dbenv->set_cachesize(1, 0, 1);
    dbenv->set_lg_bsize(10485760 * 4);
    dbenv->set_lg_max(10485760);
    dbenv->set_lk_max_locks(10000);
    dbenv->set_lk_max_objects(10000);
    dbenv->set_flags(DB_AUTO_COMMIT, 1);
    dbenv->log_set_config(DB_LOG_IN_MEMORY, 1);

    info("CDBEnv::MakeMock: CALL DB->open");
    int ret = dbenv->open(NULL,
                         DB_CREATE |
                             DB_INIT_LOCK |
                             DB_INIT_LOG |
                             DB_INIT_MPOOL |
                             DB_INIT_TXN |
                             DB_THREAD |
                             DB_PRIVATE,
                         S_IRUSR | S_IWUSR);
    if (ret > 0)
        throw runtime_error(strprintf("CDBEnv::MakeMock: Error %d opening database environment.", ret));

    fDbEnvInit = true;
    fMockDb = true;
}

CDBEnv::VerifyResult CDBEnv::Verify(const std::string& strFile, bool (*recoverFunc)(CDBEnv& dbenv, const std::string& strFile))
{
    LOCK(cs_db);
    assert(mapFileUseCount.count(strFile) == 0);

    Db db(dbenv, 0);
    int result = db.verify(strFile.c_str(), nullptr, nullptr, 0);

    info("CDBEnv::Verify: CODE: %d MESSAGE: %s", result, DbEnv::strerror(result));

    if (result != DB_VERIFY_BAD)
        return VERIFY_OK;
    else if (recoverFunc == nullptr)
        return RECOVER_FAIL;

    // Try to recover:
    bool fRecovered = (*recoverFunc)(*this, strFile);
    return (fRecovered ? RECOVER_OK : RECOVER_FAIL);
}

/* End of headers, beginning of key/value data */
static const char *HEADER_END = "HEADER=END";
/* End of key/value data */
static const char *DATA_END = "DATA=END";

bool CDBEnv::Salvage(const std::string& strFile, bool fAggressive, std::vector<CDBEnv::KeyValPair>& vResult)
{
    LOCK(cs_db);
    assert(mapFileUseCount.count(strFile) == 0);

    u_int32_t flags = DB_SALVAGE;
    if (fAggressive)
        flags |= DB_AGGRESSIVE;

    stringstream strDump;

    Db db(dbenv, 0);
    int result = db.verify(strFile.c_str(), nullptr, &strDump, flags);
    if (result == DB_VERIFY_BAD) {
        LogPrintf("CDBEnv::Salvage: Database salvage found errors, all data may not be recoverable.\n");
        if (!fAggressive) {
            LogPrintf("CDBEnv::Salvage: Rerun with aggressive mode to ignore errors and continue.\n");
            return false;
        }
    }
    if (result != 0 && result != DB_VERIFY_BAD) {
        LogPrintf("CDBEnv::Salvage: Database salvage failed with result %d.\n", result);
        return false;
    }

    // Format of bdb dump is ascii lines:
    // header lines...
    // HEADER=END
    //  hexadecimal key
    //  hexadecimal value
    //  ... repeated
    // DATA=END

    string strLine;
    while (!strDump.eof() && strLine != HEADER_END)
        getline(strDump, strLine); // Skip past header

    std::string keyHex, valueHex;
    while (!strDump.eof() && keyHex != DATA_END) {
        getline(strDump, keyHex);
        if (keyHex != DATA_END) {
            if (strDump.eof())
                break;
            getline(strDump, valueHex);
            if (valueHex == DATA_END) {
                LogPrintf("CDBEnv::Salvage: WARNING: Number of keys in data does not match number of values.\n");
                break;
            }
            vResult.push_back(make_pair(ParseHex(keyHex), ParseHex(valueHex)));
        }
    }

    if (keyHex != DATA_END) {
        LogPrintf("CDBEnv::Salvage: WARNING: Unexpected end of file while reading salvage output.\n");
        return false;
    }

    return (result == 0);
}


void CDBEnv::CheckpointLSN(const std::string& strFile)
{
    dbenv->txn_checkpoint(0, 0, 0);
    if (fMockDb)
        return;
    dbenv->lsn_reset(strFile.c_str(), 0);
}

CDB::CDB(const std::string& strFilename, const char* pszMode, bool fFlushOnCloseIn) : pdb(nullptr), activeTxn(nullptr)
{
    info("CDB::CDB: START");
    int ret;
    fReadOnly = (!strchr(pszMode, '+') && !strchr(pszMode, 'w'));
    fFlushOnClose = fFlushOnCloseIn;
    if (strFilename.empty())
        return;

    bool fCreate = strchr(pszMode, 'c') != nullptr;
    unsigned int nFlags = DB_THREAD;
    if (fCreate)
        nFlags |= DB_CREATE;

    {
        LOCK(bitdb.cs_db);
        if (!bitdb.Open(GetDataDir()))
            throw runtime_error("CDB::CDB: Failed to open database environment.");

        strFile = strFilename;
        ++bitdb.mapFileUseCount[strFile];
        pdb = bitdb.mapDb[strFile];
        if (pdb == nullptr) {
            pdb = new Db(bitdb.dbenv, 0);

            bool fMockDb = bitdb.IsMock();
            if (fMockDb) {
                DbMpoolFile* mpf = pdb->get_mpf();
                ret = mpf->set_flags(DB_MPOOL_NOFILE, 1);
                if (ret != 0)
                    throw runtime_error(strprintf("CDB::CDB Failed to configure for no temp file backing for database %s", strFile));
            }

            // Check if bitdb is encrypted
            if (bitdb.IsCrypted())
            {
                info("CDB::CDB: DB_ENCRYPT enabled");
                // Enable encryption for the database
                int cryptRet = pdb->set_flags(DB_ENCRYPT);

                // Check if it worked
                if (cryptRet != 0)
                    throw runtime_error(strprintf("CDB::CDB: Error %d enabling database encryption: %s", cryptRet, DbEnv::strerror(cryptRet)));
            }

            info("CDB::CDB: CALL DB->open");
            ret = pdb->open(NULL,                               // Txn pointer
                            fMockDb ? NULL : strFile.c_str(),   // Filename
                            fMockDb ? strFile.c_str() : "main", // Logical db name
                            DB_BTREE,                           // Database type
                            nFlags,                             // Flags
                            0);

            if (ret != 0) {
                std::string err = strprintf("CDB::CDB Error %d, can't open database %s", ret, strFile);
                error(err);

                delete pdb;
                pdb = nullptr;
                --bitdb.mapFileUseCount[strFile];
                strFile = "";

                // Could not decrypt?
                if (ret == -30973) {
                    InitError(_("Could not decrypt the wallet database"));
                    StartShutdown();
                    return;
                }
                throw runtime_error(err);
            }

            if (fCreate && !Exists(string("version"))) {
                bool fTmp = fReadOnly;
                fReadOnly = false;
                WriteVersion(CLIENT_VERSION);
                fReadOnly = fTmp;
            }

            bitdb.mapDb[strFile] = pdb;
        }
    }
}

void CDB::Flush()
{
    if (activeTxn)
        return;

    // Flush database activity from memory pool to disk log
    unsigned int nMinutes = 0;
    if (fReadOnly)
        nMinutes = 1;

    bitdb.dbenv->txn_checkpoint(nMinutes ? GetArg("-dblogsize", DEFAULT_WALLET_DBLOGSIZE) * 1024 : 0, nMinutes, 0);
}

void CDB::Close()
{
    if (!pdb)
        return;
    if (activeTxn)
        activeTxn->abort();
    activeTxn = nullptr;
    pdb = nullptr;

    if (fFlushOnClose)
        Flush();

    {
        LOCK(bitdb.cs_db);
        --bitdb.mapFileUseCount[strFile];
    }
}

void CDBEnv::CloseDb(const string& strFile)
{
    {
        LOCK(cs_db);
        if (mapDb[strFile] != nullptr) {
            // Close the database handle
            Db* pdb = mapDb[strFile];
            pdb->close(0);
            delete pdb;
            mapDb[strFile] = nullptr;
        }
    }
}

bool CDBEnv::RemoveDb(const string& strFile)
{
    this->CloseDb(strFile);

    LOCK(cs_db);
    int rc = dbenv->dbremove(NULL, strFile.c_str(), nullptr, DB_AUTO_COMMIT);
    return (rc == 0);
}

bool CDB::Rewrite(const string& strFile, const char* pszSkip, std::string newPin)
{
    while (true)
    {
        {
            LOCK(bitdb.cs_db);
            if (!bitdb.mapFileUseCount.count(strFile) || bitdb.mapFileUseCount[strFile] == 0)
            {
                // Flush log data to the dat file
                bitdb.CloseDb(strFile);
                bitdb.CheckpointLSN(strFile);
                bitdb.mapFileUseCount.erase(strFile);

                // Just make a copy of the original
                CDBEnv* _bitdb(&bitdb);

                // Check if we wanna replace the pin
                if (!newPin.empty())
                {
                    // Replace with a new copy
                    _bitdb = new CDBEnv();

                    // Get the path
                    boost::filesystem::path _path = GetDataDir() / "tmp";

                    // Remove old tmp if we already have it
                    if (boost::filesystem::exists(_path))
                        boost::filesystem::remove_all(_path);

                    // Create the directory again
                    boost::filesystem::create_directory(_path);

                    // Make sure the bitdb is open
                    if (!_bitdb->Open(_path, newPin))
                        throw runtime_error("CDB::Rewrite: Failed to open database environment.");
                }

                bool fSuccess = true;
                LogPrintf("CDB::Rewrite: Rewriting %s...\n", strFile);
                string strFileRes = strFile + ".rewrite";
                { // surround usage of db with extra {}
                    CDB db(strFile.c_str(), "r");
                    Db* pdbCopy = new Db(_bitdb->dbenv, 0);

                    // Check if _bitdb is encrypted
                    if (_bitdb->IsCrypted())
                    {
                        info("CDB::Rewrite: DB_ENCRYPT enabled");
                        // Enable encryption for the database
                        int cryptRet = pdbCopy->set_flags(DB_ENCRYPT);

                        // Check if it worked
                        if (cryptRet != 0)
                            throw runtime_error(strprintf("CDB::Rewrite: Error %d enabling database encryption: %s", cryptRet, DbEnv::strerror(cryptRet)));
                    }

                    info("CDB::Rewrite: CALL DB->open");
                    int ret = pdbCopy->open(NULL,               // Txn pointer
                                            strFileRes.c_str(), // Filename
                                            "main",             // Logical db name
                                            DB_BTREE,           // Database type
                                            DB_CREATE,          // Flags
                                            0);
                    if (ret != 0)
                    {
                        LogPrintf("CDB::Rewrite: Error %d can't create database %s : %s\n", ret, strFileRes, DbEnv::strerror(ret));
                        fSuccess = false;
                    }

                    Dbc* pcursor = db.GetCursor();
                    if (pcursor)
                        while (fSuccess)
                        {
                            CDataStream ssKey(SER_DISK, CLIENT_VERSION);
                            CDataStream ssValue(SER_DISK, CLIENT_VERSION);
                            int ret = db.ReadAtCursor(pcursor, ssKey, ssValue, DB_NEXT);
                            if (ret == DB_NOTFOUND) {
                                pcursor->close();
                                break;
                            } else if (ret != 0) {
                                pcursor->close();
                                fSuccess = false;
                                break;
                            }
                            if (pszSkip &&
                                strncmp(&ssKey[0], pszSkip, std::min(ssKey.size(), strlen(pszSkip))) == 0)
                                continue;
                            if (strncmp(&ssKey[0], "\x07version", 8) == 0)
                            {
                                // Update version:
                                ssValue.clear();
                                ssValue << CLIENT_VERSION;
                            }
                            Dbt datKey(&ssKey[0], ssKey.size());
                            Dbt datValue(&ssValue[0], ssValue.size());
                            int ret2 = pdbCopy->put(NULL, &datKey, &datValue, DB_NOOVERWRITE);
                            if (ret2 > 0)
                                fSuccess = false;
                        }
                    if (fSuccess)
                    {
                        db.Close();

                        if (pdbCopy->close(0))
                            fSuccess = false;

                        if (!newPin.empty())
                            _bitdb->dbenv->lsn_reset(strFileRes.c_str(), 0); // DO NOT REMOVE

                        delete pdbCopy;

                        if (!newPin.empty())
                            delete _bitdb;
                    }
                }
                if (fSuccess)
                {
                    // Remove the old wallet file
                    bitdb.RemoveDb(strFile);

                    // Check if we have a new pin
                    if (!newPin.empty())
                    {
                        // Remove our old database dir
                        boost::filesystem::remove_all(GetDataDir() / "database");

                        // Reset bitdb
                        bitdb.Reset();

                        // Reopen it with the new pin
                        if (!bitdb.Open(GetDataDir(), newPin))
                            throw runtime_error("CDB::Rewrite: Failed to open database environment after moving new wallet over form tmp env.");

                        Db db(bitdb.dbenv, 0);
                        if (db.rename(("tmp/" + strFileRes).c_str(), nullptr, strFile.c_str(), 0))
                            fSuccess = false;

                        // Remove our tmp working dir
                        boost::filesystem::remove_all(GetDataDir() / "tmp");
                    } else
                    {
                        Db db(bitdb.dbenv, 0);
                        if (db.rename(strFileRes.c_str(), nullptr, strFile.c_str(), 0))
                            fSuccess = false;
                    }
                }
                if (!fSuccess)
                    LogPrintf("CDB::Rewrite: Failed to rewrite database file %s\n", strFileRes);
                return fSuccess;
            }
        }

        MilliSleep(100);
    }

    return false;
}


void CDBEnv::Flush(bool fShutdown)
{
    int64_t nStart = GetTimeMillis();
    // Flush log data to the actual data file on all files that are not in use
    LogPrint("db", "CDBEnv::Flush: Flush(%s)%s\n", fShutdown ? "true" : "false", fDbEnvInit ? "" : " database not started");
    if (!fDbEnvInit)
        return;
    {
        LOCK(cs_db);
        map<string, int>::iterator mi = mapFileUseCount.begin();
        while (mi != mapFileUseCount.end()) {
            string strFile = (*mi).first;
            int nRefCount = (*mi).second;
            LogPrint("db", "CDBEnv::Flush: Flushing %s (refcount = %d)...\n", strFile, nRefCount);
            if (nRefCount == 0) {
                // Move log data to the dat file
                CloseDb(strFile);
                LogPrint("db", "CDBEnv::Flush: %s checkpoint\n", strFile);
                dbenv->txn_checkpoint(0, 0, 0);
                LogPrint("db", "CDBEnv::Flush: %s detach\n", strFile);
                if (!fMockDb)
                    dbenv->lsn_reset(strFile.c_str(), 0);
                LogPrint("db", "CDBEnv::Flush: %s closed\n", strFile);
                mapFileUseCount.erase(mi++);
            } else
                mi++;
        }
        LogPrint("db", "CDBEnv::Flush: Flush(%s)%s took %15dms\n", fShutdown ? "true" : "false", fDbEnvInit ? "" : " database not started", GetTimeMillis() - nStart);
        if (fShutdown) {
            char** listp;
            if (mapFileUseCount.empty()) {
                dbenv->log_archive(&listp, DB_ARCH_REMOVE);
                Close();
                if (!fMockDb)
                    boost::filesystem::remove_all(boost::filesystem::path(strPath) / "database");
            }
        }
    }
}
