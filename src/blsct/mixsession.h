// Copyright (c) 2020 The NavCoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef MIXSESSION_H
#define MIXSESSION_H

#include <blsct/ephemeralserver.h>
#include <blsct/key.h>
#include <blsct/transaction.h>
#include <net.h>
#include <random.h>
#include <util.h>
#include <utilstrencodings.h>
#include <utiltime.h>
#include <wallet/wallet.h>

#define DEFAULT_MIX_FEE 50000000
#define DEFAULT_MIX true

class MixSession
{
public:
    MixSession(const CCoinsViewCache* inputs);

    bool Start();
    void Stop();

    static CAmount GetDefaultFee();
    static bool IsKnown(const MixSession& ms);

    bool GetState() const;

    void AnnounceHiddenService();

    bool AddCandidateTransaction(const std::vector<unsigned char>& v);

    uint256 GetHash() const
    {
        return SerializeHash(*this);
    }

    std::string GetHiddenService() const
    {
        return sHiddenService;
    }

    set<CandidateTransaction> GetTransactionCandidates() const
    {
        return setTransactionCandidates;
    }

    bool Join() const;

    friend inline  bool operator==(const MixSession& a, const MixSession& b) { return a.GetHiddenService() == b.GetHiddenService(); }
    friend inline  bool operator<(const MixSession& a, const MixSession& b) { return a.GetHiddenService() < b.GetHiddenService(); }

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(sHiddenService);
    }

private:
    EphemeralServer *es;
    const CCoinsViewCache* inputs;
    std::string sHiddenService;
    int fState;
    bool lock;

    set<CandidateTransaction> setTransactionCandidates;
};

#endif // MIXSESSION_H
