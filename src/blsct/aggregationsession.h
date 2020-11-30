// Copyright (c) 2020 The NavCoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef AGGREGATIONSESSION_H
#define AGGREGATIONSESSION_H

#include <blsct/ephemeralserver.h>
#include <blsct/key.h>
#include <blsct/transaction.h>
#include <net.h>
#include <random.h>
#include <util.h>
#include <utilstrencodings.h>
#include <utiltime.h>
#include <wallet/wallet.h>

extern CCriticalSection cs_aggregation;

class CandidateTransaction;
class COutput;
class CWalletDB;

class AggregationSession
{
public:
    AggregationSession(const CStateViewCache* inputs);

    bool Start();
    void Stop();

    static CAmount GetDefaultFee();
    static CAmount GetMaxFee();

    static bool IsKnown(const AggregationSession& ms);
    static bool IsKnown(const EncryptedCandidateTransaction& ec);

    bool GetState() const;

    void AnnounceHiddenService();

    bool AddCandidateTransaction(const std::vector<unsigned char>& v);

    bool NewEncryptedCandidateTransaction(const EncryptedCandidateTransaction& v);

    bool AddEncryptedCandidateTransaction(const EncryptedCandidateTransaction& v);

    bool SelectCandidates(CandidateTransaction& ret);

    void SetCandidateTransactions(std::vector<CandidateTransaction> candidates);

    bool UpdateCandidateTransactions(const CTransaction &tx);

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
        else if (nVersion == 2)
            return HexStr(vPublicKey);
        return "";
    }

    std::vector<CandidateTransaction> GetTransactionCandidates() const
    {
        return vTransactionCandidates;
    }

    bool Join();
    static bool JoinSingle(int index, const std::string &hiddenService, const std::vector<COutput> &vAvailableCoins, const CStateViewCache* inputs);
    static bool JoinThread(const std::string &hiddenService, const std::vector<COutput> &vAvailableCoins, const CStateViewCache* inputs);
    static bool JoinSingleV2(int index, std::vector<unsigned char> &vPublicKey, const std::vector<COutput> &vAvailableCoins, const CStateViewCache* inputs);
    static bool JoinThreadV2(const std::vector<unsigned char> &vPublicKey, const std::vector<COutput> &vAvailableCoins, const CStateViewCache* inputs);

    static bool BuildCandidateTransaction(const CWalletTx *prevcoin, const int &prevout, const CStateViewCache* inputs, CandidateTransaction& tx);

    friend inline  bool operator==(const AggregationSession& a, const AggregationSession& b) { return a.GetHiddenService() == b.GetHiddenService(); }
    friend inline  bool operator<(const AggregationSession& a, const AggregationSession& b) { return a.GetHiddenService() < b.GetHiddenService(); }

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(this->nVersion);
        if (this->nVersion == 1)
        {
            READWRITE(sHiddenService);
        }
        else if (this->nVersion == 2)
        {
            READWRITE(vPublicKey);
        }
    }

private:
    EphemeralServer *es;
    const CStateViewCache* inputs;
    std::string sHiddenService;
    std::vector<unsigned char> vPublicKey;
    int fState;
    bool lock;
    static bool fJoining;

    std::vector<std::pair<uint256, bls::PrivateKey>> vKeys;

    boost::thread joinThread;
    boost::thread_group candidateVerificationThreadGroup;

    int nVersion;

    std::vector<CandidateTransaction> vTransactionCandidates;
};

void AggregationSessionThread();

#endif // AGGREGATIONSESSION_H
