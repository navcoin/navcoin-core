// Copyright (c) 2020 The Navcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef AGGREGATIONSESSION_H
#define AGGREGATIONSESSION_H

#include <blsct/ephemeralserver.h>
#include <blsct/key.h>
#include <blsct/transaction.h>
#include <chainparams.h>
#include <net.h>
#include <random.h>
#include <util.h>
#include <utilstrencodings.h>
#include <utiltime.h>
#include <wallet/wallet.h>

#include <queue>

extern CCriticalSection cs_aggregation;
extern CCriticalSection cs_sessionKeys;

class CandidateTransaction;
class COutput;
class CWalletDB;

class AggregationSession
{
public:
    AggregationSession(const CStateViewCache* inputs);
    AggregationSession(const UniValue& msg);

    int64_t nTime;
    std::vector<CandidateTransaction> vTransactionCandidates;
    const CStateViewCache* inputs;

    bool Start();
    void Stop();

    static CAmount GetDefaultFee();
    static CAmount GetMaxFee();

    bool GetState() const;

    void AnnounceHiddenService();

    bool AddCandidateTransaction(const std::vector<unsigned char>& v);

    bool NewEncryptedCandidateTransaction(EncryptedCandidateTransaction v);

    bool SelectCandidates(CandidateTransaction& ret);

    void SetCandidateTransactions(std::vector<CandidateTransaction> candidates);

    bool UpdateCandidateTransactions(const CTransaction& tx);

    bool CleanCandidateTransactions();

    int GetVersion() { return nVersion; }

    uint256 GetHash() const
    {
        return SerializeHash(*this);
    }

    std::string GetHiddenService() const
    {
        if (nVersion == 1)
            return sHiddenService;
        else if (nVersion == 2 || nVersion == 3)
            return HexStr(vPublicKey);
        return "";
    }

    std::vector<CandidateTransaction> GetTransactionCandidates() const
    {
        return vTransactionCandidates;
    }

    bool Join();
    static bool JoinSingle(int index, const std::string& hiddenService, const std::vector<COutput>& vAvailableCoins, const CStateViewCache* inputs);
    static bool JoinThread(const std::string& hiddenService, const std::vector<COutput>& vAvailableCoins, const CStateViewCache* inputs);
    static bool JoinSingleV2(int index, std::vector<unsigned char>& vPublicKey, const std::vector<COutput>& vAvailableCoins, const CStateViewCache* inputs);
    static bool JoinThreadV2(const std::vector<unsigned char>& vPublicKey, const std::vector<COutput>& vAvailableCoins, const CStateViewCache* inputs);
    static bool JoinSingleV3(int index, std::vector<unsigned char>& vPublicKey, std::vector<unsigned char>& vData, const std::vector<COutput>& vAvailableCoins, const CStateViewCache* inputs);
    static bool JoinThreadV3(const std::vector<unsigned char>& vPublicKey, const std::vector<unsigned char>& vData, const std::vector<COutput>& vAvailableCoins, const CStateViewCache* inputs);

    static bool BuildCandidateTransaction(const CWalletTx* prevcoin, const int& prevout, const CStateViewCache* inputs, CandidateTransaction& tx);

    friend inline bool operator==(const AggregationSession& a, const AggregationSession& b) { return a.GetHiddenService() == b.GetHiddenService(); }
    friend inline bool operator<(const AggregationSession& a, const AggregationSession& b) { return a.GetHiddenService() < b.GetHiddenService(); }

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion)
    {
        READWRITE(this->nVersion);
        if (this->nVersion == 1) {
            READWRITE(sHiddenService);
        } else if (this->nVersion == 2) {
            READWRITE(vPublicKey);
        } else if (this->nVersion == 3) {
            READWRITE(vPublicKey);
            READWRITE(vData);
        }
    }

    UniValue GetData() const
    {
        UniValue ret;

        if (nVersion != 3)
            return ret;

        try {
            ret.read(std::string(vData.begin(), vData.end()));
        } catch (...) {
        }

        return ret;
    }

private:
    EphemeralServer* es;
    std::string sHiddenService;
    std::vector<unsigned char> vPublicKey;
    std::vector<unsigned char> vData;
    int fState;
    bool lock;
    static bool fJoining;

    int nVersion;
};

void AggregationSessionThread();
void CandidateVerificationThread();

template <class T>
class SafeQueue
{
public:
    SafeQueue(void)
        : q(), m()
    {
    }

    ~SafeQueue(void)
    {
    }

    void push(T t)
    {
        std::lock_guard<std::mutex> lock(m);
        q.push(t);
    }

    bool pop(T& val)
    {
        while (q.empty()) {
            MilliSleep(50);
        }

        std::unique_lock<std::mutex> lock(m);
        val = q.front();
        q.pop();

        return true;
    }

private:
    std::queue<T> q;
    mutable std::mutex m;
};

#endif // AGGREGATIONSESSION_H
