// Copyright (c) 2018-2019 The NavCoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef NAVCOIN_DAO_H
#define NAVCOIN_DAO_H

#include "amount.h"
#include "script/script.h"
#include "serialize.h"
#include "tinyformat.h"
#include "univalue/include/univalue.h"

#include "dao/flags.h"

using namespace std;
using namespace DAOFlags;

class CTransaction;
class CStateViewCache;
class CBlockIndex;
class CChainParams;
class CValidationState;

extern std::map<uint256, int64_t> mapAddedVotes;
extern std::map<uint256, bool> mapSupported;

void SetScriptForCommunityFundContribution(CScript &script);
void SetScriptForProposalVote(CScript &script, uint256 proposalhash, int64_t vote);
void SetScriptForPaymentRequestVote(CScript &script, uint256 prequest, int64_t vote);
void SetScriptForConsultationSupport(CScript &script, uint256 hash);
void SetScriptForConsultationSupportRemove(CScript &script, uint256 hash);
void SetScriptForConsultationVote(CScript &script, uint256 hash, int64_t vote);
void SetScriptForConsultationVoteRemove(CScript &script, uint256 hash);

bool Vote(uint256 hash, int64_t vote, bool &duplicate);
bool VoteValue(uint256 hash, int64_t vote, bool &duplicate);
bool RemoveVote(string str);
bool RemoveVoteValue(string str);
bool RemoveVote(uint256 hash);

bool Support(uint256 hash, bool &duplicate);
bool RemoveSupport(string str);

void GetVersionMask(uint64_t& nProposalMask, uint64_t& nPaymentRequestMask, uint64_t& nConsultationMask, uint64_t& nConsultatioAnswernMask);

class CVote
{
public:
    CVote() { SetNull(); }

    void SetNull()
    {
        fNull = true;
    }

    bool IsNull() const
    {
        return (fNull == true);
    }

    bool GetValue(int64_t& nVal) const
    {
        if (fNull)
            return false;
        nVal = nValue;
        return true;
    }

    void SetValue(const int64_t& nVal)
    {
        fNull = false;
        nValue = nVal;
    }

    void swap(CVote &to) {
        std::swap(to.fNull, fNull);
        std::swap(to.nValue, nValue);
    }

    std::string ToString()
    {
        return strprintf("CVote(nValue=%d, fNull=%b)", nValue, fNull);
    }

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(nValue);
        READWRITE(fNull);
    }

private:
    int64_t nValue;
    bool fNull;
};

class CVoteList
{
public:
    bool fDirty;

    CVoteList() { SetNull(); }

    void SetNull()
    {
        list.clear();
        fDirty = true;
    }

    bool IsNull() const
    {
        for (auto& it: list)
        {
            if (!it.second.IsNull())
                return false;
        }

        return true;
    }

    CVote* Get(const uint256& hash)
    {
        CVote* ret = nullptr;
        int nHeight = 0;
        for (auto& it: list)
        {
            if (it.first.first > nHeight && it.first.second == hash && !it.second.IsNull())
            {
                ret = &it.second;
                nHeight = it.first.first;
            }
        }
        return ret;
    }

    bool Set(const int& height, const uint256& hash, int64_t vote)
    {
        if (list.count(std::make_pair(height, hash)) == 0)
            list[std::make_pair(height, hash)] = CVote();
        list[std::make_pair(height, hash)].SetValue(vote);
        fDirty = true;
        return true;
    }

    bool Set(const int& height, const uint256& hash, CVote vote)
    {
        list[std::make_pair(height, hash)] = vote;
        fDirty = true;
        return true;
    }

    bool Clear(const int& height, const uint256& hash)
    {
        if (list.count(std::make_pair(height, hash)) == 0)
            list[std::make_pair(height, hash)] = CVote();
        list[std::make_pair(height, hash)].SetNull();
        fDirty = true;
        return true;
    }

    bool Clear(const int& height)
    {
        for (auto &it: list)
        {
            if (it.first.first == height)
                it.second.SetNull();
        }
        return true;
    }

    std::map<uint256, CVote> GetList()
    {
        std::map<uint256, CVote> ret;
        std::map<uint256, int> mapCacheHeight;
        for (auto &it: list)
        {
            if (!it.second.IsNull())
            {
                if (mapCacheHeight.count(it.first.second) == 0)
                    mapCacheHeight[it.first.second] = 0;
                if (it.first.first > mapCacheHeight[it.first.second])
                {
                    ret[it.first.second] = it.second;
                    mapCacheHeight[it.first.second] = it.first.first;
                }
            }
        }
        return ret;
    }


    std::map<std::pair<int, uint256>, CVote> GetFullList()
    {
        std::map<std::pair<int, uint256>, CVote> ret;
        for (auto &it: list)
        {
            if (!it.second.IsNull())
                ret[it.first] = it.second;
        }
        return ret;
    }

    std::string ToString()
    {
        std::string sList;
        for (auto& it:list)
        {
            sList += strprintf("{%s at height %d => %s}, ", it.first.second.ToString(), it.first.first, it.second.ToString());
        }
        return strprintf("CVoteList([%s])", sList);
    }

    void swap(CVoteList &to) {
        std::swap(to.fDirty, fDirty);
        std::swap(to.list, list);
    }

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(list);
    }

private:
    std::map<std::pair<int, uint256>, CVote> list;
};

bool IsBeginningCycle(const CBlockIndex* pindex, CChainParams params);
bool IsEndCycle(const CBlockIndex* pindex, CChainParams params);
bool VoteStep(const CValidationState& state, CBlockIndex *pindexNew, const bool fUndo, CStateViewCache& coins);

bool IsValidPaymentRequest(CTransaction tx, CStateViewCache& coins, uint64_t nMaxVersion);
bool IsValidProposal(CTransaction tx, uint64_t nMaxVersion);
bool IsValidConsultation(CTransaction tx, CStateViewCache& coins, uint64_t nMaskVersion, CBlockIndex* pindex);
bool IsValidConsultationAnswer(CTransaction tx, CStateViewCache& coins, uint64_t nMaskVersion, CBlockIndex* pindex);
bool IsValidConsensusParameterProposal(Consensus::ConsensusParamsPos pos, std::string proposal, CBlockIndex *pindex);
bool IsValidDaoTxVote(const CTransaction& tx, CBlockIndex* pindex);

std::string FormatConsensusParameter(Consensus::ConsensusParamsPos pos, std::string string);
std::string RemoveFormatConsensusParameter(Consensus::ConsensusParamsPos pos, std::string string);

// CFUND

class CPaymentRequest
{
public:
    static const uint64_t BASE_VERSION=1<<1;
    static const uint64_t REDUCED_QUORUM_VERSION=1<<2;
    static const uint64_t ABSTAIN_VOTE_VERSION=1<<3;
    static const uint64_t ALL_VERSION = 1 | BASE_VERSION | REDUCED_QUORUM_VERSION | ABSTAIN_VOTE_VERSION;

    CAmount nAmount;
    flags fState;
    uint256 hash;
    uint256 proposalhash;
    uint256 txblockhash;
    uint256 blockhash;
    uint256 paymenthash;
    int nVotesYes;
    int nVotesNo;
    int nVotesAbs;
    string strDZeel;
    int nVersion;
    unsigned int nVotingCycle;
    bool fDirty;

    CPaymentRequest() { SetNull(); }

    void SetNull() {
        nAmount = 0;
        fState = DAOFlags::NIL;
        nVotesYes = 0;
        nVotesNo = 0;
        nVotesAbs = 0;
        hash = uint256();
        blockhash = uint256();
        txblockhash = uint256();
        proposalhash = uint256();
        paymenthash = uint256();
        strDZeel = "";
        nVersion = 0;
        nVotingCycle = 0;
        fDirty = false;
    }

    void swap(CPaymentRequest &to) {
        std::swap(to.nAmount, nAmount);
        std::swap(to.fState, fState);
        std::swap(to.hash, hash);
        std::swap(to.proposalhash, proposalhash);
        std::swap(to.txblockhash, txblockhash);
        std::swap(to.blockhash, blockhash);
        std::swap(to.paymenthash, paymenthash);
        std::swap(to.nVotesYes, nVotesYes);
        std::swap(to.nVotesNo, nVotesNo);
        std::swap(to.nVotesAbs, nVotesAbs);
        std::swap(to.strDZeel, strDZeel);
        std::swap(to.nVersion, nVersion);
        std::swap(to.nVotingCycle, nVotingCycle);
        std::swap(to.fDirty, fDirty);
    }

    bool IsNull() const {
        return (nAmount == 0 && fState == DAOFlags::NIL && nVotesYes == 0  && nVotesYes == 0 && nVotesAbs == 0 && strDZeel == "");
    }

    std::string GetState() const {
        std::string sFlags = "pending";
        if(IsAccepted()) {
            sFlags = "accepted";
            if(fState != DAOFlags::ACCEPTED)
                sFlags += " waiting for end of voting period";
        }
        if(IsRejected()) {
            sFlags = "rejected";
            if(fState != DAOFlags::REJECTED)
                sFlags += " waiting for end of voting period";
        }
        if(IsExpired())
            sFlags = "expired";
        return sFlags;
    }

    std::string ToString() const {
        return strprintf("CPaymentRequest(hash=%s, nVersion=%d, nAmount=%f, fState=%s, nVotesYes=%u, nVotesNo=%u, nVotesAbs=%u, nVotingCycle=%u, "
                         " proposalhash=%s, blockhash=%s, paymenthash=%s, strDZeel=%s)",
                         hash.ToString(), nVersion, (float)nAmount/COIN, GetState(), nVotesYes, nVotesNo, nVotesAbs,
                         nVotingCycle, proposalhash.ToString(), blockhash.ToString().substr(0,10),
                         paymenthash.ToString().substr(0,10), strDZeel);
    }

    void ToJson(UniValue& ret) const;

    bool IsAccepted() const;

    bool IsRejected() const;

    bool IsExpired() const;

    bool ExceededMaxVotingCycles() const;

    bool CanVote(CStateViewCache& coins) const;

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        if(ser_action.ForRead())
        {
            READWRITE(nAmount);
            // Payment Requests with versioning are signalled by a negative amount followed by the real amount
            if(nAmount < 0)
            {
                READWRITE(nAmount);
                READWRITE(this->nVersion);
            }
            else
            {
                nVersion = 1;
            }
        }
        else
        {
            CAmount nSignalVersion = -1;
            READWRITE(nSignalVersion);
            READWRITE(nAmount);
            READWRITE(this->nVersion);
        }
        READWRITE(fState);
        READWRITE(nVotesYes);
        READWRITE(nVotesNo);
        READWRITE(hash);
        READWRITE(proposalhash);
        READWRITE(blockhash);
        READWRITE(paymenthash);
        READWRITE(strDZeel);
        READWRITE(txblockhash);

        // Version-based read/write
        if(nVersion & BASE_VERSION)
           READWRITE(nVotingCycle);

        if(nVersion & ABSTAIN_VOTE_VERSION)
           READWRITE(nVotesAbs);

        if (ser_action.ForRead())
            fDirty = false;
    }

};

class  CProposal
{
public:
    static const uint64_t BASE_VERSION=1<<1;
    static const uint64_t REDUCED_QUORUM_VERSION=1<<2;
    static const uint64_t ABSTAIN_VOTE_VERSION=1<<3;
    static const uint64_t ALL_VERSION = 1 | BASE_VERSION | REDUCED_QUORUM_VERSION | ABSTAIN_VOTE_VERSION;

    CAmount nAmount;
    CAmount nFee;
    std::string Address;
    uint32_t nDeadline;
    flags fState;
    int nVotesYes;
    int nVotesNo;
    int nVotesAbs;
    std::vector<uint256> vPayments;
    std::string strDZeel;
    uint256 hash;
    uint256 blockhash;
    uint256 txblockhash;
    int nVersion;
    unsigned int nVotingCycle;
    bool fDirty;

    CProposal() { SetNull(); }

    void SetNull() {
        nAmount = 0;
        nFee = 0;
        Address = "";
        fState = DAOFlags::NIL;
        nVotesYes = 0;
        nVotesNo = 0;
        nVotesAbs = 0;
        nDeadline = 0;
        vPayments.clear();
        strDZeel = "";
        hash = uint256();
        blockhash = uint256();
        nVersion = 0;
        nVotingCycle = 0;
        fDirty = false;
    }

    void swap(CProposal &to) {
        std::swap(to.nAmount, nAmount);
        std::swap(to.nFee, nFee);
        std::swap(to.Address, Address);
        std::swap(to.nDeadline, nDeadline);
        std::swap(to.fState, fState);
        std::swap(to.nVotesYes, nVotesYes);
        std::swap(to.nVotesNo, nVotesNo);
        std::swap(to.nVotesAbs, nVotesAbs);
        std::swap(to.vPayments, vPayments);
        std::swap(to.strDZeel, strDZeel);
        std::swap(to.hash, hash);
        std::swap(to.txblockhash, txblockhash);
        std::swap(to.blockhash, blockhash);
        std::swap(to.nVersion, nVersion);
        std::swap(to.nVotingCycle, nVotingCycle);
        std::swap(to.fDirty, fDirty);
    }

    bool IsNull() const {
        return (nAmount == 0 && nFee == 0 && Address == "" && nVotesYes == 0 && fState == DAOFlags::NIL
                && nVotesYes == 0 && nVotesNo == 0 && nVotesAbs == 0 && nDeadline == 0 && strDZeel == "");
    }

    std::string ToString(CStateViewCache& coins, uint32_t currentTime = 0) const;
    std::string GetState(uint32_t currentTime) const;

    void ToJson(UniValue& ret, CStateViewCache& coins) const;

    bool IsAccepted() const;

    bool IsRejected() const;

    bool IsExpired(uint32_t currentTime) const;

    bool ExceededMaxVotingCycles() const;

    uint64_t getTimeTillExpired(uint32_t currentTime) const;

    bool CanVote() const;

    bool CanRequestPayments() const {
        return fState == DAOFlags::ACCEPTED;
    }

    bool HasPendingPaymentRequests(CStateViewCache& coins) const;

    CAmount GetAvailable(CStateViewCache& coins, bool fIncludeRequests = false) const;

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        if (ser_action.ForRead()) {
            const_cast<std::vector<uint256>*>(&vPayments)->clear();
        }

        READWRITE(nAmount);

        if(ser_action.ForRead())
        {
            READWRITE(nFee);
            // Proposals with versioning are signalled by a negative fee followed by the real fee
            if(nFee < 0)
            {
                READWRITE(nFee);
                READWRITE(this->nVersion);
            }
            else
            {
                nVersion = 1;
            }
        }
        else
        {
            CAmount nSignalVersion = -1;
            READWRITE(nSignalVersion);
            READWRITE(nFee);
            READWRITE(this->nVersion);
        }

        READWRITE(Address);
        READWRITE(nDeadline);
        READWRITE(fState);
        READWRITE(nVotesYes);
        READWRITE(nVotesNo);
        READWRITE(*const_cast<std::vector<uint256>*>(&vPayments));
        READWRITE(strDZeel);
        READWRITE(hash);
        READWRITE(blockhash);
        READWRITE(txblockhash);

        // Version-based read/write
        if(nVersion & BASE_VERSION) {
            READWRITE(nVotingCycle);
        }

        if(nVersion & ABSTAIN_VOTE_VERSION) {
           READWRITE(nVotesAbs);
        }

        if (ser_action.ForRead())
            fDirty = false;

    }
};


// CONSULTATIONS

class CConsultationAnswer
{
public:
    static const uint64_t BASE_VERSION=1;
    static const uint64_t ALL_VERSION =BASE_VERSION;

    int nVersion;
    std::string sAnswer;
    int nVotes;
    int nSupport;
    int fState;
    uint256 hash;
    uint256 parent;
    uint256 txblockhash;
    uint256 blockhash;
    bool fDirty;

    CConsultationAnswer() { SetNull(); }

    void swap(CConsultationAnswer &to) {
        std::swap(to.nVersion, nVersion);
        std::swap(to.sAnswer, sAnswer);
        std::swap(to.nVotes, nVotes);
        std::swap(to.nSupport, nSupport);
        std::swap(to.fState, fState);
        std::swap(to.fDirty, fDirty);
        std::swap(to.hash, hash);
        std::swap(to.txblockhash, txblockhash);
        std::swap(to.blockhash, blockhash);
        std::swap(to.parent, parent);
    }

    bool IsNull() const
    {
        return (sAnswer == "" && nVotes == 0 && nSupport == 0 && fState == 0 && nVersion == 0 &&
                hash == uint256() && blockhash == uint256() && parent == uint256() && txblockhash == uint256());
    };

    void SetNull()
    {
        sAnswer = "";
        nVotes = 0;
        fState = 0;
        nVersion = 0;
        nSupport = 0;
        hash = uint256();
        parent = uint256();
        txblockhash = uint256();
        blockhash = uint256();
        fDirty = false;
    };

    void Vote();
    void DecVote();
    void ClearVotes();
    int GetVotes() const;
    std::string GetState() const;
    std::string GetText() const;
    bool CanBeVoted(CStateViewCache& view) const;
    bool CanBeSupported(CStateViewCache& view) const;
    bool IsSupported() const;
    bool IsConsensusAccepted() const;
    std::string ToString() const;
    void ToJson(UniValue& ret) const;

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion_) {
        READWRITE(nVersion);
        READWRITE(sAnswer);
        READWRITE(nVotes);
        READWRITE(nSupport);
        READWRITE(fState);
        READWRITE(hash);
        READWRITE(parent);
        READWRITE(txblockhash);
        READWRITE(blockhash);
        if (ser_action.ForRead())
            fDirty = false;
    }
};

class CConsultation
{
public:
    static const uint64_t BASE_VERSION = 1;
    static const uint64_t ANSWER_IS_A_RANGE_VERSION = 1<<1;
    static const uint64_t MORE_ANSWERS_VERSION  = 1<<2;
    static const uint64_t CONSENSUS_PARAMETER_VERSION  = 1<<3;
    static const uint64_t ALL_VERSION = BASE_VERSION | ANSWER_IS_A_RANGE_VERSION | MORE_ANSWERS_VERSION | CONSENSUS_PARAMETER_VERSION;

    flags fState;
    uint256 hash;
    uint256 txblockhash;
    uint256 blockhash;
    uint64_t nVersion;
    unsigned int nVotingCycle;
    bool fDirty;
    std::string strDZeel;
    int nSupport;
    uint64_t nMin;
    uint64_t nMax;
    map<uint64_t, uint64_t> mapVotes;

    CConsultation() { SetNull(); }

    void swap(CConsultation &to) {
        std::swap(to.fState, fState);
        std::swap(to.hash, hash);
        std::swap(to.txblockhash, txblockhash);
        std::swap(to.blockhash, blockhash);
        std::swap(to.nVersion, nVersion);
        std::swap(to.nVotingCycle, nVotingCycle);
        std::swap(to.fDirty, fDirty);
        std::swap(to.strDZeel, strDZeel);
        std::swap(to.nSupport, nSupport);
        std::swap(to.nMin, nMin);
        std::swap(to.nMax, nMax);
        std::swap(to.mapVotes, mapVotes);
    };

    bool IsNull() const {
        return (hash == uint256() && fState == DAOFlags::NIL && txblockhash == uint256() && blockhash == uint256()
                && nVersion == 0 && nVotingCycle == 0 && strDZeel == "" && nSupport == 0 && mapVotes.size() == 0
                && nMin == 0 && nMax == 0);
    };

    void SetNull() {
        fState = DAOFlags::NIL;
        hash = uint256();
        txblockhash = uint256();
        blockhash = uint256();
        nVersion = 0;
        nVotingCycle = 0;
        fDirty = false;
        strDZeel = "";
        nSupport = 0;
        nMin = 0;
        nMax = 0;
        mapVotes.clear();
    };

    std::string GetState(CBlockIndex* pindex) const;
    std::string ToString(CBlockIndex* pindex) const;
    void ToJson(UniValue& ret, CStateViewCache& view) const;
    bool CanBeSupported() const;
    bool CanBeVoted() const;
    bool IsSupported(CStateViewCache& view) const;
    bool IsExpired(CBlockIndex* pindex) const;
    bool IsValidVote(int64_t vote) const;
    bool ExceededMaxVotingCycles() const;
    bool IsRange() const;
    bool CanHaveNewAnswers() const;
    bool CanHaveAnswers() const;
    bool HaveEnoughAnswers() const;
    bool IsAboutConsensusParameter() const;

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(this->nVersion);
        READWRITE(fState);
        READWRITE(nSupport);
        READWRITE(nMin);
        READWRITE(nMax);
        READWRITE(nVotingCycle);
        READWRITE(hash);
        READWRITE(blockhash);
        READWRITE(strDZeel);
        READWRITE(txblockhash);
        READWRITE(mapVotes);
        if (ser_action.ForRead())
            fDirty = false;
    }
};

#endif // NAVCOIN_DAO_H
