// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef NAVCOIN_WALLET_WALLETDB_H
#define NAVCOIN_WALLET_WALLETDB_H

#include <amount.h>
#include <blsct/key.h>
#include <blsct/transaction.h>
#include <primitives/transaction.h>
#include <wallet/db.h>
#include <key.h>

#include <list>
#include <stdint.h>
#include <string>
#include <utility>
#include <vector>

static const bool DEFAULT_FLUSHWALLET = true;

class CandidateTransaction;
class CAccount;
class CAccountingEntry;
struct CBlockLocator;
class CBLSCTBlindingKeyPool;
class CBLSCTSubAddressKeyPool;
class CKeyPool;
class CMasterKey;
class CScript;
class CWallet;
class CWalletTx;
class uint160;
class uint256;

/** Error statuses for the wallet database */
enum DBErrors
{
    DB_LOAD_OK,
    DB_CORRUPT,
    DB_NONCRITICAL_ERROR,
    DB_TOO_NEW,
    DB_LOAD_FAIL,
    DB_NEED_REWRITE
};

/* simple HD chain data model */
class CHDChain
{
public:
    uint32_t nExternalChainCounter;
    uint32_t nExternalBLSCTChainCounter;
    uint32_t nExternalBLSCTTokenCounter;
    std::map<uint64_t, uint64_t> nExternalBLSCTSubAddressCounter;
    CKeyID masterKeyID; //!< master key hash160

    static const int CURRENT_VERSION = 1;
    int nVersion;

    CHDChain() { SetNull(); }
    ADD_SERIALIZE_METHODS;
    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion)
    {
        READWRITE(this->nVersion);
        nVersion = this->nVersion;
        if (ser_action.ForRead())
        {
            READWRITE(nExternalChainCounter);
            if (nExternalChainCounter == ~(uint32_t)0)
            {
                READWRITE(nExternalChainCounter);
                READWRITE(nExternalBLSCTChainCounter);
                READWRITE(nExternalBLSCTSubAddressCounter);
            }
            READWRITE(masterKeyID);
        }
        else
        {
            if (nExternalBLSCTChainCounter > 0 || nExternalBLSCTSubAddressCounter.size() > 0)
            {
                uint32_t nMarker = ~(uint32_t)0;
                READWRITE(nMarker);
                READWRITE(nExternalChainCounter);
                READWRITE(nExternalBLSCTChainCounter);
                READWRITE(nExternalBLSCTSubAddressCounter);
            }
            else
            {
                READWRITE(nExternalChainCounter);
            }
            READWRITE(masterKeyID);
        }
    }

    void SetNull()
    {
        nVersion = CHDChain::CURRENT_VERSION;
        nExternalChainCounter = 0;
        nExternalBLSCTChainCounter = 0;
        nExternalBLSCTSubAddressCounter.clear();
        masterKeyID.SetNull();
    }
};

class CKeyMetadata
{
public:
    static const int VERSION_BASIC=1;
    static const int VERSION_WITH_HDDATA=10;
    static const int CURRENT_VERSION=VERSION_WITH_HDDATA;
    int nVersion;
    int64_t nCreateTime; // 0 means unknown
    std::string hdKeypath; //optional HD/bip32 keypath
    CKeyID hdMasterKeyID; //id of the HD masterkey used to derive this key

    CKeyMetadata()
    {
        SetNull();
    }
    CKeyMetadata(int64_t nCreateTime_)
    {
        SetNull();
        nCreateTime = nCreateTime_;
    }

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(this->nVersion);
        nVersion = this->nVersion;
        READWRITE(nCreateTime);
        if (this->nVersion >= VERSION_WITH_HDDATA)
        {
            READWRITE(hdKeypath);
            READWRITE(hdMasterKeyID);
        }
    }

    void SetNull()
    {
        nVersion = CKeyMetadata::CURRENT_VERSION;
        nCreateTime = 0;
        hdKeypath.clear();
        hdMasterKeyID.SetNull();
    }
};

class CBLSCTBlindingKeyMetadata
{
public:
    static const int CURRENT_VERSION=1;
    int nVersion;
    int64_t nCreateTime; // 0 means unknown
    std::string hdKeypath; //optional HD/bip32 keypath

    CBLSCTBlindingKeyMetadata()
    {
        SetNull();
    }
    CBLSCTBlindingKeyMetadata(int64_t nCreateTime_)
    {
        SetNull();
        nCreateTime = nCreateTime_;
    }

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(this->nVersion);
        nVersion = this->nVersion;
        READWRITE(nCreateTime);
        READWRITE(hdKeypath);
    }

    void SetNull()
    {
        nVersion = CBLSCTBlindingKeyMetadata::CURRENT_VERSION;
        nCreateTime = 0;
        hdKeypath.clear();
    }
};

class CBLSCTTokenKeyMetadata
{
public:
    static const int CURRENT_VERSION=1;
    int nVersion;
    int64_t nCreateTime; // 0 means unknown
    std::string hdKeypath; //optional HD/bip32 keypath

    CBLSCTTokenKeyMetadata()
    {
        SetNull();
    }
    CBLSCTTokenKeyMetadata(int64_t nCreateTime_)
    {
        SetNull();
        nCreateTime = nCreateTime_;
    }

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(this->nVersion);
        nVersion = this->nVersion;
        READWRITE(nCreateTime);
        READWRITE(hdKeypath);
    }

    void SetNull()
    {
        nVersion = CBLSCTTokenKeyMetadata::CURRENT_VERSION;
        nCreateTime = 0;
        hdKeypath.clear();
    }
};

/** Access to the wallet database */
class CWalletDB : public CDB
{
public:
    CWalletDB(const std::string& strFilename, const char* pszMode = "r+", bool fFlushOnClose = true) : CDB(strFilename, pszMode, fFlushOnClose)
    {
    }

    bool WriteName(const std::string& strAddress, const std::string& strName);
    bool EraseName(const std::string& strAddress);

    bool WritePurpose(const std::string& strAddress, const std::string& purpose);
    bool ErasePurpose(const std::string& strAddress);

    bool WritePrivateName(const std::string& strAddress, const std::string& strName);
    bool ErasePrivateName(const std::string& strAddress);

    bool WritePrivatePurpose(const std::string& strAddress, const std::string& purpose);
    bool ErasePrivatePurpose(const std::string& strAddress);


    bool WriteTx(const CWalletTx& wtx);
    bool EraseTx(uint256 hash);

    bool WriteKey(const CPubKey& vchPubKey, const CPrivKey& vchPrivKey, const CKeyMetadata &keyMeta);
    bool WriteCryptedKey(const CPubKey& vchPubKey, const std::vector<unsigned char>& vchCryptedSecret, const CKeyMetadata &keyMeta);
    bool WriteMasterKey(unsigned int nID, const CMasterKey& kMasterKey);

    bool WriteCScript(const uint160& hash, const CScript& redeemScript);

    bool WriteWatchOnly(const CScript &script);
    bool EraseWatchOnly(const CScript &script);

    bool WriteBestBlock(const CBlockLocator& locator);
    bool ReadBestBlock(CBlockLocator& locator);

    bool WriteOrderPosNext(int64_t nOrderPosNext);

    bool WriteDefaultKey(const CPubKey& vchPubKey);

    bool ReadPool(int64_t nPool, CKeyPool& keypool);
    bool WritePool(int64_t nPool, const CKeyPool& keypool);
    bool ErasePool(int64_t nPool);

    bool ReadBLSCTBlindingPool(int64_t nPool, CBLSCTBlindingKeyPool& keypool);
    bool WriteBLSCTBlindingPool(int64_t nPool, const CBLSCTBlindingKeyPool& keypool);
    bool EraseBLSCTBlindingPool(int64_t nPool);

    bool ReadBLSCTSubAddressPool(uint64_t account, uint64_t nPool, CBLSCTSubAddressKeyPool& keypool);
    bool WriteBLSCTSubAddressPool(uint64_t account, uint64_t nPool, const CBLSCTSubAddressKeyPool& keypool);
    bool EraseBLSCTSubAddressPool(uint64_t account, uint64_t nPool);

    bool WriteMinVersion(int nVersion);

    /// This writes directly to the database, and will not update the CWallet's cached accounting entries!
    /// Use wallet.AddAccountingEntry instead, to write *and* update its caches.
    bool WriteAccountingEntry_Backend(const CAccountingEntry& acentry);
    bool ReadAccount(const std::string& strAccount, CAccount& account);
    bool WriteAccount(const std::string& strAccount, const CAccount& account);

    bool WriteBLSCTSpendKey(const blsctKey& key);
    bool WriteBLSCTViewKey(const blsctKey& key);
    bool WriteBLSCTDoublePublicKey(const blsctDoublePublicKey& key);
    bool WriteBLSCTBlindingMasterKey(const blsctKey& key);
    bool WriteBLSCTTokenMasterKey(const blsctKey& key);
    bool WriteBLSCTCryptedKey(const std::vector<unsigned char>& ck);
    bool WriteBLSCTKey(const CWallet* pwallet);
    bool WriteBLSCTBlindingKey(const blsctPublicKey& vchPubKey, const blsctKey& vchPrivKey, const CBLSCTBlindingKeyMetadata& keyMeta);
    bool WriteBLSCTTokenKey(const blsctPublicKey& vchPubKey, const blsctKey& vchPrivKey, const CBLSCTTokenKeyMetadata& keyMeta);
    bool WriteBLSCTSubAddress(const CKeyID &hashId, const std::pair<uint64_t, uint64_t>& index);
    bool WriteCandidateTransactions(const std::vector<CandidateTransaction>& candidates);
    bool WriteOutputNonce(const uint256& hash, const std::vector<unsigned char>& nonce);

    /// Write destination data key,value tuple to database
    bool WriteDestData(const std::string &address, const std::string &key, const std::string &value);
    /// Erase destination data tuple from wallet database
    bool EraseDestData(const std::string &address, const std::string &key);

    CAmount GetAccountCreditDebit(const std::string& strAccount);
    void ListAccountCreditDebit(const std::string& strAccount, std::list<CAccountingEntry>& acentries);

    DBErrors ReorderTransactions(CWallet* pwallet);
    DBErrors LoadWallet(CWallet* pwallet);
    DBErrors FindWalletTx(CWallet* pwallet, std::vector<uint256>& vTxHash, std::vector<CWalletTx>& vWtx);
    DBErrors ZapWalletTx(CWallet* pwallet, std::vector<CWalletTx>& vWtx);
    DBErrors ZapSelectTx(CWallet* pwallet, std::vector<uint256>& vHashIn, std::vector<uint256>& vHashOut);
    static bool Recover(CDBEnv& dbenv, const std::string& filename, bool fOnlyKeys);
    static bool Recover(CDBEnv& dbenv, const std::string& filename);

    //! write the hdchain model (external chain child index counter)
    bool WriteHDChain(const CHDChain& chain);

private:
    CWalletDB(const CWalletDB&);
    void operator=(const CWalletDB&);

    bool WriteAccountingEntry(const uint64_t nAccEntryNum, const CAccountingEntry& acentry);
};

void ThreadFlushWalletDB(const std::string& strFile);

#endif // NAVCOIN_WALLET_WALLETDB_H
