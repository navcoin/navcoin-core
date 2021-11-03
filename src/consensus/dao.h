// Copyright (c) 2018-2020 The Navcoin Core developers
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

using namespace DAOFlags;

class CTransaction;
class CStateViewCache;
class CBlockIndex;
class CChainParams;
class CValidationState;

void SetScriptForCommunityFundContribution(CScript &script);
void SetScriptForProposalVote(CScript &script, uint256 proposalhash, int64_t vote);
void SetScriptForPaymentRequestVote(CScript &script, uint256 prequest, int64_t vote);
void SetScriptForConsultationSupport(CScript &script, uint256 hash);
void SetScriptForConsultationSupportRemove(CScript &script, uint256 hash);
void SetScriptForConsultationVote(CScript &script, uint256 hash, int64_t vote);
void SetScriptForConsultationVoteRemove(CScript &script, uint256 hash);

bool Vote(uint256 hash, int64_t vote, bool &duplicate);
bool VoteValue(uint256 hash, int64_t vote, bool &duplicate);
bool RemoveVote(std::string str);
bool RemoveVoteValue(std::string str);
bool RemoveVote(uint256 hash);

bool Support(uint256 hash, bool &duplicate);
bool RemoveSupport(std::string str);

uint256 GetDAOStateHash(CStateViewCache& view, const CAmount& nCFLocked, const CAmount& nCFSupply);
uint256 GetConsensusStateHash(CStateViewCache& view);

void GetVersionMask(uint64_t& nProposalMask, uint64_t& nPaymentRequestMask, uint64_t& nConsultationMask, uint64_t& nConsultatioAnswernMask, CBlockIndex* pindex);

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
            if (it.second.size() > 0)
                return false;
        }

        return true;
    }

    bool Get(const uint256& hash, int64_t& val);

    std::string diff(const CVoteList& b) const {
        std::string ret = "";
        if (list != b.list)
        {
            std::string sList;
            for (auto& it:list)
            {
                if (b.list.count(it.first) == 0)
                {
                    sList += strprintf("removed height %d => {", it.first);
                    for (auto&it2: it.second)
                    {
                        sList += strprintf("%s => %d, ", it2.first.ToString(), it2.second);
                    }
                    sList += strprintf("}, ");
                }
                else if (b.list.at(it.first) != it.second)
                {
                    std::string sBuf = "";
                    for (auto&it2: it.second)
                    {
                        if (b.list.at(it.first).count(it2.first) == 0)
                        {
                            sBuf += strprintf("removed %s => %d, ", it2.first.ToString(), it2.second);
                        }
                        else if (b.list.at(it.first).at(it2.first) != it2.second)
                        {
                            sBuf += strprintf("%s => %d, ", it2.first.ToString(), it2.second);
                        }
                    }
                    if (sBuf != "")
                        sList += strprintf("modified height %d => {%s}, ", it.first, sBuf);
                }
            }
            for (auto& it:b.list)
            {
                if (list.count(it.first) == 0)
                {
                    sList += strprintf("added height %d => {", it.first);
                    for (auto&it2: it.second)
                    {
                        sList += strprintf("%s => %d, ", it2.first.ToString(), it2.second);
                    }
                    sList += strprintf("}, ");
                }
                else if (list.at(it.first) != it.second)
                {
                    std::string sBuf = "";
                    for (auto&it2: it.second)
                    {
                        if (list.at(it.first).count(it2.first) == 0)
                        {
                            sBuf += strprintf("added %s => %d, ", it2.first.ToString(), it2.second);
                        }
                    }
                    if (sBuf != "")
                        sList += strprintf("modified height %d => {%s}, ", it.first, sBuf);
                }
            }
            ret = strprintf("list: %s", sList);
        }
        return ret;
    }

    bool operator==(const CVoteList &b) const {
        return list == b.list;
    }

    bool operator!=(const CVoteList& b) const {
        return !(*this == b);
    }

    bool Set(const int& height, const uint256& hash, int64_t vote);

    bool Clear(const int& height, const uint256& hash);

    bool Clear(const int& height);

    std::map<uint256, int64_t> GetList();

    std::map<int, std::map<uint256, int64_t>>* GetFullList();

    std::string ToString() const;

    void swap(CVoteList &to) {
        std::swap(to.fDirty, fDirty);
        std::swap(to.list, list);
    }

    int GetLastVoteHeight() {
        if (list.empty())
            return 0;
        return list.rbegin()->first;
    }

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(list);
    }

private:
    std::map<int, std::map<uint256, int64_t>> list;
};

class CConsensusParameter
{
public:
    bool fDirty;

    CConsensusParameter() { SetNull(); }

    void SetNull()
    {
        list.clear();
        fDirty = true;
    }

    bool IsNull() const
    {
        return list.size() == 0;
    }

    bool Get(uint64_t& val);

    int GetHeight() const;

    std::string diff(const CConsensusParameter& b) const {
        std::string ret = "";
        if (list != b.list)
        {
            std::string sList;
            for (auto& it:list)
            {
                sList += strprintf("{height %d => %d},", it.first, it.second);
            }
            std::string sListb;
            for (auto& it:b.list)
            {
                sListb += strprintf("{height %d => %d},", it.first, it.second);
            }
            ret = strprintf("list: %s => %s", sList, sListb);
        }
        return ret;
    }

    bool operator==(const CConsensusParameter &b) const {
        return list == b.list;
    }

    bool operator!=(const CConsensusParameter& b) const {
        return !(*this == b);
    }

    bool Set(const int& height, const uint64_t& val);

    bool Clear(const int& height);

    std::map<int, uint64_t>* GetFullList();

    std::string ToString() const;

    void swap(CConsensusParameter &to) {
        std::swap(to.fDirty, fDirty);
        std::swap(to.list, list);
    }

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(list);
    }

private:
    std::map<int, uint64_t> list;
};

bool IsBeginningCycle(const CBlockIndex* pindex, const CStateViewCache& coins);
bool IsEndCycle(const CBlockIndex* pindex, CChainParams params);
bool VoteStep(const CValidationState& state, CBlockIndex *pindexNew, const bool fUndo, CStateViewCache& coins);

bool IsValidPaymentRequest(CTransaction tx, CStateViewCache& coins, uint64_t nMaxVersion);
bool IsValidProposal(CTransaction tx, const CStateViewCache& view, uint64_t nMaxVersion);
bool IsValidConsultation(CTransaction tx, CStateViewCache& coins, uint64_t nMaskVersion, CBlockIndex* pindex);
bool IsValidConsultationAnswer(CTransaction tx, CStateViewCache& coins, uint64_t nMaskVersion, CBlockIndex* pindex);
bool IsValidConsensusParameterProposal(std::vector<uint64_t> vPos, std::vector<std::string> vProposal, CBlockIndex *pindex, CStateViewCache& coins);
bool IsValidConsensusParameterProposal(Consensus::ConsensusParamsPos pos, std::string proposal, CBlockIndex *pindex, CStateViewCache& coins);
bool IsValidConsensusParameterProposal(std::vector<Consensus::ConsensusParamsPos> pos, std::vector<std::string> proposal, CBlockIndex *pindex, CStateViewCache& coins);
bool IsValidDaoTxVote(const CTransaction& tx, const CStateViewCache& view);

std::string FormatConsensusParameter(Consensus::ConsensusParamsPos pos, std::string string);
std::string RemoveFormatConsensusParameter(Consensus::ConsensusParamsPos pos, std::string string);

// CFUND

class CPaymentRequest
{
public:
    static const uint64_t BASE_VERSION=1<<1;
    static const uint64_t REDUCED_QUORUM_VERSION=1<<2;
    static const uint64_t ABSTAIN_VOTE_VERSION=1<<3;
    static const uint64_t EXCLUDE_VERSION=1<<4;
    static const uint64_t SUPER_VERSION=1<<5;
    static const uint64_t ALL_VERSION = 1 | BASE_VERSION | REDUCED_QUORUM_VERSION | ABSTAIN_VOTE_VERSION | EXCLUDE_VERSION | SUPER_VERSION;

    CAmount nAmount;
    std::map<uint256, flags> mapState;
    uint256 hash;
    uint256 proposalhash;
    uint256 txblockhash;
    int nVotesYes;
    int nVotesNo;
    int nVotesAbs;
    int nExclude;
    std::string strDZeel;
    int nVersion;
    unsigned int nVotingCycle;
    bool fDirty;

    CPaymentRequest() { SetNull(); }

    void SetNull() {
        nAmount = 0;
        mapState.clear();
        nVotesYes = 0;
        nVotesNo = 0;
        nVotesAbs = 0;
        nExclude = 0;
        hash = uint256();
        txblockhash = uint256();
        proposalhash = uint256();
        strDZeel = "";
        nVersion = 0;
        nVotingCycle = 0;
        fDirty = true;
    }

    void swap(CPaymentRequest &to) {
        std::swap(to.nAmount, nAmount);
        std::swap(to.mapState, mapState);
        std::swap(to.hash, hash);
        std::swap(to.proposalhash, proposalhash);
        std::swap(to.txblockhash, txblockhash);
        std::swap(to.nVotesYes, nVotesYes);
        std::swap(to.nVotesNo, nVotesNo);
        std::swap(to.nVotesAbs, nVotesAbs);
        std::swap(to.nExclude, nExclude);
        std::swap(to.strDZeel, strDZeel);
        std::swap(to.nVersion, nVersion);
        std::swap(to.nVotingCycle, nVotingCycle);
        std::swap(to.fDirty, fDirty);
    }

    bool operator==(const CPaymentRequest &b) const {
        std::string thisMapState = "";
        std::string bMapState = "";

        for (auto &it:mapState) thisMapState += it.first.ToString()+":"+std::to_string(it.second)+",";
        for (auto &it:b.mapState) bMapState += it.first.ToString()+":"+std::to_string(it.second)+",";

        return nAmount == b.nAmount
                && thisMapState == bMapState
                && hash == b.hash
                && proposalhash == b.proposalhash
                && txblockhash == b.txblockhash
                && nVotesYes == b.nVotesYes
                && nVotesNo == b.nVotesNo
                && nVotesAbs == b.nVotesAbs
                && nExclude == b.nExclude
                && strDZeel == b.strDZeel
                && nVersion == b.nVersion
                && nVotingCycle == b.nVotingCycle;
    }

    bool operator!=(const CPaymentRequest& b) const {
        return !(*this == b);
    }

    std::string diff(const CPaymentRequest& b) const {
        std::string ret = "";
        if (nAmount != b.nAmount) ret += strprintf("nAmount: %d => %d, ", nAmount, b.nAmount);
        if (mapState != b.mapState)
        {
            std::string sAdded = "";
            std::string sRemoved = "";
            for (auto &it:mapState) if (b.mapState.count(it.first) == 0) sRemoved += it.first.ToString()+":"+std::to_string(it.second)+",";
            for (auto &it:b.mapState) if (mapState.count(it.first) == 0) sAdded += it.first.ToString()+":"+std::to_string(it.second)+",";
            if (sRemoved.size() > 0) sRemoved.pop_back();
            if (sAdded.size() > 0) sAdded.pop_back();
            ret += strprintf("mapState: added: %s - removed: %s, ", sAdded, sRemoved);
        }
        if (nVotesYes != b.nVotesYes) ret += strprintf("nVotesYes: %d => %d, ", nVotesYes, b.nVotesYes);
        if (nVotesNo != b.nVotesNo) ret += strprintf("nVotesNo: %d => %d, ", nVotesNo, b.nVotesNo);
        if (nVotesAbs != b.nVotesAbs) ret += strprintf("nVotesAbs: %d => %d, ", nVotesAbs, b.nVotesAbs);
        if (strDZeel != b.strDZeel) ret += strprintf("strDZeel: %s => %s, ", strDZeel, b.strDZeel);
        if (hash != b.hash) ret += strprintf("hash: %s => %s, ", hash.ToString(), b.hash.ToString());
        if (proposalhash != b.proposalhash) ret += strprintf("proposalhash: %s => %s, ", proposalhash.ToString(), b.proposalhash.ToString());
        if (txblockhash != b.txblockhash) ret += strprintf("txblockhash: %s => %s, ", txblockhash.ToString(), b.txblockhash.ToString());
        if (nVersion != b.nVersion) ret += strprintf("nVersion: %d => %d, ", nVersion, b.nVersion);
        if (nVotingCycle != b.nVotingCycle) ret += strprintf("nVotingCycle: %d => %d, ", nVotingCycle, b.nVotingCycle);
        if (ret != "")
        {
            ret.pop_back();
            ret.pop_back();
        }
        return ret;
    }

    bool IsNull() const {
        return (nAmount == 0 && nVotesYes == 0 && nVotesNo == 0 && nExclude == 0 && strDZeel == "" && mapState.size() == 0);
    }

    std::string GetState(const CStateViewCache& view) const {
        flags fState = GetLastState();
        std::string sFlags = "pending";
        if(IsAccepted(view)) {
            sFlags = "accepted";
            if(fState != DAOFlags::ACCEPTED)
                sFlags = "pending";
        }
        if(IsRejected(view)) {
            sFlags = "rejected";
            if(fState != DAOFlags::REJECTED)
                sFlags = "pending";
        }
        if(IsExpired(view)) {
            sFlags = "expired";
            if(fState != DAOFlags::EXPIRED)
                sFlags = "pending";
        }
        if (fState == PAID)
            sFlags = "paid";
        return sFlags;
    }

    flags GetLastState() const;
    CBlockIndex* GetLastStateBlockIndex() const;
    CBlockIndex* GetLastStateBlockIndexForState(flags state) const;
    bool SetState(const CBlockIndex* pindex, flags state);
    bool ClearState(const CBlockIndex* pindex);
    std::string ToString(const CStateViewCache& view) const;

    void ToJson(UniValue& ret, const CStateViewCache& view) const;

    bool IsAccepted(const CStateViewCache& view) const;

    bool IsRejected(const CStateViewCache& view) const;

    bool IsExpired(const CStateViewCache& view) const;

    bool ExceededMaxVotingCycles(const CStateViewCache& view) const;

    bool CanVote(CStateViewCache& coins) const;

    bool IsSuper() const {
        return nVersion & SUPER_VERSION;
    }

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
                this->nVersion = 1;
            }
        }
        else
        {
            CAmount nSignalVersion = -1;
            READWRITE(nSignalVersion);
            READWRITE(nAmount);
            READWRITE(this->nVersion);
        }
        READWRITE(mapState);
        READWRITE(nVotesYes);
        READWRITE(nVotesNo);
        READWRITE(hash);
        READWRITE(proposalhash);
        READWRITE(txblockhash);
        READWRITE(strDZeel);

        // Version-based read/write
        if(this->nVersion & BASE_VERSION)
            READWRITE(nVotingCycle);

        if(this->nVersion & ABSTAIN_VOTE_VERSION)
            READWRITE(nVotesAbs);

        if(this->nVersion & EXCLUDE_VERSION)
            READWRITE(nExclude);
    }

};

class  CProposal
{
public:
    static const uint64_t BASE_VERSION=1<<1;
    static const uint64_t REDUCED_QUORUM_VERSION=1<<2;
    static const uint64_t ABSTAIN_VOTE_VERSION=1<<3;
    static const uint64_t PAYMENT_ADDRESS_VERSION=1<<4;
    static const uint64_t EXCLUDE_VERSION=1<<5;
    static const uint64_t SUPER_VERSION=1<<6;
    static const uint64_t ALL_VERSION = 1 | BASE_VERSION | REDUCED_QUORUM_VERSION | ABSTAIN_VOTE_VERSION | PAYMENT_ADDRESS_VERSION | EXCLUDE_VERSION | SUPER_VERSION;

    CAmount nAmount;
    CAmount nFee;
    std::string ownerAddress;
    std::string paymentAddress;
    uint32_t nDeadline;
    std::map<uint256, flags> mapState;
    int nVotesYes;
    int nVotesNo;
    int nVotesAbs;
    int nExclude;
    std::string strDZeel;
    uint256 hash;
    uint256 txblockhash;
    int nVersion;
    unsigned int nVotingCycle;
    bool fDirty;

    CProposal() { SetNull(); }

    void SetNull() {
        nAmount = 0;
        nFee = 0;
        ownerAddress = "";
        paymentAddress = "";
        mapState.clear();
        nVotesYes = 0;
        nVotesNo = 0;
        nVotesAbs = 0;
        nExclude = 0;
        nDeadline = 0;
        strDZeel = "";
        hash = uint256();
        nVersion = 0;
        nVotingCycle = 0;
        fDirty = true;
    }

    void swap(CProposal &to) {
        std::swap(to.nAmount, nAmount);
        std::swap(to.nFee, nFee);
        std::swap(to.ownerAddress, ownerAddress);
        std::swap(to.paymentAddress, paymentAddress);
        std::swap(to.nDeadline, nDeadline);
        std::swap(to.mapState, mapState);
        std::swap(to.nVotesYes, nVotesYes);
        std::swap(to.nVotesNo, nVotesNo);
        std::swap(to.nVotesAbs, nVotesAbs);
        std::swap(to.nExclude, nExclude);
        std::swap(to.strDZeel, strDZeel);
        std::swap(to.hash, hash);
        std::swap(to.txblockhash, txblockhash);
        std::swap(to.nVersion, nVersion);
        std::swap(to.nVotingCycle, nVotingCycle);
        std::swap(to.fDirty, fDirty);
    }

    bool operator==(const CProposal& b) const {
        std::string thisMapState = "";
        std::string bMapState = "";

        for (auto &it:mapState) thisMapState += it.first.ToString()+":"+std::to_string(it.second)+",";
        for (auto &it:b.mapState) bMapState += it.first.ToString()+":"+std::to_string(it.second)+",";

        return nAmount == b.nAmount
                && nFee == b.nFee
                && ownerAddress == b.ownerAddress
                && ((nVersion & PAYMENT_ADDRESS_VERSION) ? paymentAddress : ownerAddress) == ((b.nVersion & PAYMENT_ADDRESS_VERSION) ? b.paymentAddress : b.ownerAddress)
                && nDeadline == b.nDeadline
                && thisMapState == bMapState
                && nVotesYes == b.nVotesYes
                && nVotesNo == b.nVotesNo
                && nVotesAbs == b.nVotesAbs
                && nExclude == b.nExclude
                && strDZeel == b.strDZeel
                && hash == b.hash
                && txblockhash == b.txblockhash
                && nVersion == b.nVersion
                && nVotingCycle == b.nVotingCycle;
    }

    std::string diff(const CProposal& b) const {
        std::string ret = "";
        if (nAmount != b.nAmount) ret += strprintf("nAmount: %d => %d, ", nAmount, b.nAmount);
        if (nFee != b.nFee) ret += strprintf("nFee: %d => %d, ", nFee, b.nFee);
        if (ownerAddress != b.ownerAddress) ret += strprintf("ownerAddress: %s => %s, ", ownerAddress, b.ownerAddress);
        if (paymentAddress != b.paymentAddress) ret += strprintf("paymentAddress: %s => %s, ", paymentAddress, b.paymentAddress);
        if (nDeadline != b.nDeadline) ret += strprintf("nDeadline: %d => %d, ", nDeadline, b.nDeadline);
        if (mapState != b.mapState)
        {
            std::string sAdded = "";
            std::string sRemoved = "";
            for (auto &it:mapState) if (b.mapState.count(it.first) == 0) sRemoved += it.first.ToString()+":"+std::to_string(it.second)+",";
            for (auto &it:b.mapState) if (mapState.count(it.first) == 0) sAdded += it.first.ToString()+":"+std::to_string(it.second)+",";
            if (sRemoved.size() > 0) sRemoved.pop_back();
            if (sAdded.size() > 0) sAdded.pop_back();
            ret += strprintf("mapState: added: %s - removed: %s, ", sAdded, sRemoved);
        }
        if (nVotesYes != b.nVotesYes) ret += strprintf("nVotesYes: %d => %d, ", nVotesYes, b.nVotesYes);
        if (nVotesNo != b.nVotesNo) ret += strprintf("nVotesNo: %d => %d, ", nVotesNo, b.nVotesNo);
        if (nVotesAbs != b.nVotesAbs) ret += strprintf("nVotesAbs: %d => %d, ", nVotesAbs, b.nVotesAbs);
        if (strDZeel != b.strDZeel) ret += strprintf("strDZeel: %s => %s, ", strDZeel, b.strDZeel);
        if (hash != b.hash) ret += strprintf("hash: %s => %s, ", hash.ToString(), b.hash.ToString());
        if (txblockhash != b.txblockhash) ret += strprintf("txblockhash: %s => %s, ", txblockhash.ToString(), b.txblockhash.ToString());
        if (nVersion != b.nVersion) ret += strprintf("nVersion: %d => %d, ", nVersion, b.nVersion);
        if (nVotingCycle != b.nVotingCycle) ret += strprintf("nVotingCycle: %s => %s, ", nVotingCycle, b.nVotingCycle);
        if (ret != "")
        {
            ret.pop_back();
            ret.pop_back();
        }
        return ret;
    }

    bool operator!=(const CProposal& b) const {
        return !(*this == b);
    }

    flags GetLastState() const;

    CBlockIndex* GetLastStateBlockIndex() const;

    CBlockIndex* GetLastStateBlockIndexForState(flags state) const;

    bool SetState(const CBlockIndex* pindex, flags state);

    bool ClearState(const CBlockIndex* pindex);

    bool IsNull() const {
        return (nAmount == 0 && nFee == 0 && ownerAddress == "" && paymentAddress == "" &&
                nVotesYes == 0 && nVotesNo == 0 && nExclude == 0 && nDeadline == 0 && strDZeel == "" && mapState.size() == 0);
    }

    std::string ToString(CStateViewCache& coins, uint32_t currentTime = 0) const;
    std::string GetState(uint32_t currentTime, const CStateViewCache& view) const;

    void ToJson(UniValue& ret, CStateViewCache& coins) const;

    bool IsAccepted(const CStateViewCache& view) const;

    bool IsRejected(const CStateViewCache& view) const;

    bool IsExpired(uint32_t currentTime, const CStateViewCache& view) const;

    bool ExceededMaxVotingCycles(const CStateViewCache& view) const;

    uint64_t getTimeTillExpired(uint32_t currentTime) const;

    bool CanVote(const CStateViewCache& view) const;

    bool CanRequestPayments() const {
        return GetLastState() == DAOFlags::ACCEPTED;
    }

    bool IsSuper() const {
        return nVersion & SUPER_VERSION;
    }

    std::string GetOwnerAddress() const;

    std::string GetPaymentAddress() const;

    bool HasPendingPaymentRequests(CStateViewCache& coins) const;

    CAmount GetAvailable(CStateViewCache& coins, bool fIncludeRequests = false) const;

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
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
                this->nVersion = 1;
            }
        }
        else
        {
            CAmount nSignalVersion = -1;
            READWRITE(nSignalVersion);
            READWRITE(nFee);
            READWRITE(this->nVersion);
        }

        READWRITE(ownerAddress);
        READWRITE(nDeadline);
        READWRITE(mapState);
        READWRITE(nVotesYes);
        READWRITE(nVotesNo);
        READWRITE(strDZeel);
        READWRITE(hash);
        READWRITE(txblockhash);

        // Version-based read/write
        if(this->nVersion & BASE_VERSION) {
            READWRITE(nVotingCycle);
        }

        if(this->nVersion & ABSTAIN_VOTE_VERSION) {
            READWRITE(nVotesAbs);
        }

        if(this->nVersion & PAYMENT_ADDRESS_VERSION) {
            READWRITE(paymentAddress);
        }

        if (this->nVersion & EXCLUDE_VERSION) {
            READWRITE(nExclude);
        }
    }
};


// CONSULTATIONS

class CConsultationAnswer
{
public:
    static const uint64_t BASE_VERSION=1;
    static const uint64_t EXCLUDE_VERSION=1<<1;
    static const uint64_t SUPER_VERSION=1<<2;
    static const uint64_t ALL_VERSION = BASE_VERSION|EXCLUDE_VERSION|SUPER_VERSION;

    int nVersion;
    std::string sAnswer;
    std::vector<std::string> vAnswer;
    int nVotes;
    int nSupport;
    int nExclude;
    std::map<uint256, flags> mapState;
    uint256 hash;
    uint256 parent;
    uint256 txblockhash;
    uint256 txhash;
    bool fDirty;

    CConsultationAnswer() { SetNull(); }

    void swap(CConsultationAnswer &to) {
        std::swap(to.nVersion, nVersion);
        std::swap(to.sAnswer, sAnswer);
        std::swap(to.nVotes, nVotes);
        std::swap(to.nExclude, nExclude);
        std::swap(to.nSupport, nSupport);
        std::swap(to.mapState, mapState);
        std::swap(to.fDirty, fDirty);
        std::swap(to.hash, hash);
        std::swap(to.txblockhash, txblockhash);
        std::swap(to.parent, parent);
        std::swap(to.txhash, txhash);
        std::swap(to.vAnswer, vAnswer);
    }

    std::string diff(const CConsultationAnswer& b) const {
        std::string ret = "";
        if (nVersion != b.nVersion) ret += strprintf("nVersion: %d => %d, ", nVersion, b.nVersion);
        if (sAnswer != b.sAnswer) ret += strprintf("sAnswer: %d => %d, ", sAnswer, b.sAnswer);
        if (nVotes != b.nVotes) ret += strprintf("nVotes: %d => %d, ", nVotes, b.nVotes);
        if (nSupport != b.nSupport) ret += strprintf("nSupport: %d => %d, ", nSupport, b.nSupport);
        if (mapState != b.mapState)
        {
            std::string sAdded = "";
            std::string sRemoved = "";
            for (auto &it:mapState) if (b.mapState.count(it.first) == 0) sRemoved += it.first.ToString()+":"+std::to_string(it.second)+",";
            for (auto &it:b.mapState) if (mapState.count(it.first) == 0) sAdded += it.first.ToString()+":"+std::to_string(it.second)+",";
            if (sRemoved.size() > 0) sRemoved.pop_back();
            if (sAdded.size() > 0) sAdded.pop_back();
            ret += strprintf("mapState: added: %s - removed: %s, ", sAdded, sRemoved);
        }
        if (hash != b.hash) ret += strprintf("hash: %s => %s, ", hash.ToString(), b.hash.ToString());
        if (txblockhash != b.txblockhash) ret += strprintf("txblockhash: %s => %s, ", txblockhash.ToString(), b.txblockhash.ToString());
        if (parent != b.parent) ret += strprintf("parent: %d => %d, ", parent.ToString(), b.parent.ToString());
        if (txhash != b.txhash) ret += strprintf("txhash: %d => %d, ", txhash.ToString(), b.txhash.ToString());
        if (ret != "")
        {
            ret.pop_back();
            ret.pop_back();
        }
        return ret;
    }


    bool operator==(const CConsultationAnswer& b) const {
        return nVersion == b.nVersion
                && sAnswer == b.sAnswer
                && vAnswer == b.vAnswer
                && nVotes == b.nVotes
                && nExclude == b.nExclude
                && nSupport == b.nSupport
                && mapState == b.mapState
                && hash == b.hash
                && txblockhash == b.txblockhash
                && parent == b.parent
                && txhash == b.txhash;
    }

    bool operator!=(const CConsultationAnswer& b) const {
        return !(*this == b);
    }

    std::vector<std::string> GetAnswers() const {
        std::vector<std::string> ret;
        if (nVersion & SUPER_VERSION) {
            for (auto&it: vAnswer) {
                ret.push_back(it);
            }
            std::sort(ret.begin(), ret.end());
        } else {
            ret.push_back(sAnswer);
        }
        return ret;
    }

    bool IsNull() const
    {
        return (sAnswer == "" && nVotes == 0 && nSupport == 0 && nVersion == 0 && mapState.size() == 0 && nExclude == 0 &&
                hash == uint256() && txhash == uint256() && parent == uint256() && txblockhash == uint256() &&
                vAnswer.size() == 0);
    };

    void SetNull()
    {
        sAnswer = "";
        nVotes = 0;
        nExclude = 0;
        mapState.clear();
        vAnswer.clear();
        nVersion = 0;
        nSupport = 0;
        hash = uint256();
        parent = uint256();
        txblockhash = uint256();
        txhash = uint256();
        fDirty = true;
    };

    flags GetLastState() const;

    CBlockIndex* GetLastStateBlockIndex() const;

    CBlockIndex* GetLastStateBlockIndexForState(flags state) const;

    bool SetState(const CBlockIndex* pindex, flags state);

    bool ClearState(const CBlockIndex* pindex);

    void Vote();
    void DecVote();
    void ClearVotes();
    int GetVotes() const;
    std::string GetState(const CStateViewCache& view) const;
    std::string GetText() const;
    bool CanBeVoted(const CStateViewCache& view) const;
    bool CanBeSupported(const CStateViewCache& view) const;
    bool IsSupported(const CStateViewCache& view) const;
    bool IsConsensusAccepted(const CStateViewCache& view) const;
    std::string ToString() const;
    void ToJson(UniValue& ret, const CStateViewCache& view) const;

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(this->nVersion);
        if (this->nVersion & SUPER_VERSION)
            READWRITE(vAnswer);
        else
            READWRITE(sAnswer);
        READWRITE(nVotes);
        READWRITE(nSupport);
        READWRITE(mapState);
        READWRITE(hash);
        READWRITE(parent);
        READWRITE(txblockhash);
        READWRITE(txhash);
        if (this->nVersion & EXCLUDE_VERSION)
        {
            READWRITE(nExclude);
        }
    }
};

class CConsultation
{
public:
    static const uint64_t BASE_VERSION = 1;
    static const uint64_t ANSWER_IS_A_RANGE_VERSION = 1<<1;
    static const uint64_t MORE_ANSWERS_VERSION  = 1<<2;
    static const uint64_t CONSENSUS_PARAMETER_VERSION  = 1<<3;
    static const uint64_t EXCLUDE_VERSION  = 1<<4;
    static const uint64_t SUPER_VERSION=1<<5;
    static const uint64_t ALL_VERSION = BASE_VERSION | ANSWER_IS_A_RANGE_VERSION | MORE_ANSWERS_VERSION | CONSENSUS_PARAMETER_VERSION | EXCLUDE_VERSION | SUPER_VERSION;

    std::map<uint256, flags> mapState;
    uint256 hash;
    uint256 txblockhash;
    uint64_t nVersion;
    unsigned int nVotingCycle;
    bool fDirty;
    std::string strDZeel;
    int nSupport;
    uint64_t nMin;
    std::vector<uint64_t> vParameters;
    uint64_t nMax;
    std::map<uint64_t, uint64_t> mapVotes;
    std::vector<uint256> vAnswers;
    int nExclude;

    CConsultation() { SetNull(); }

    void swap(CConsultation &to) {
        std::swap(to.mapState, mapState);
        std::swap(to.hash, hash);
        std::swap(to.txblockhash, txblockhash);
        std::swap(to.nVersion, nVersion);
        std::swap(to.nVotingCycle, nVotingCycle);
        std::swap(to.fDirty, fDirty);
        std::swap(to.strDZeel, strDZeel);
        std::swap(to.nSupport, nSupport);
        std::swap(to.nMin, nMin);
        std::swap(to.nMax, nMax);
        std::swap(to.mapVotes, mapVotes);
        std::swap(to.vAnswers, vAnswers);
        std::swap(to.nExclude, nExclude);
        std::swap(to.vParameters, vParameters);
    };

    bool IsNull() const {
        return (hash == uint256() && txblockhash == uint256() && mapState.size() == 0
                && nVersion == 0 && nVotingCycle == 0 && strDZeel == "" && nSupport == 0
                && nMin == 0 && nMax == 0 && vAnswers.size() == 0 && nExclude == 0
                && vParameters.size() == 0);
    };

    std::vector<uint64_t> GetParameters() {
        if (IsAboutConsensusParameter()){
            std::vector<uint64_t> ret;
            if (nVersion & SUPER_VERSION) {
                for (auto&it: vParameters) {
                    ret.push_back(it);
                }
                std::sort(ret.begin(), ret.end());
            } else {
                ret.push_back(nMin);
            }
            return ret;
        }
        return std::vector<uint64_t>();
    }

    void SetNull() {
        hash = uint256();
        txblockhash = uint256();
        mapState.clear();
        nVersion = 0;
        nVotingCycle = 0;
        fDirty = true;
        strDZeel = "";
        nSupport = 0;
        nMin = 0;
        nMax = 0;
        nExclude = 0;
        mapVotes.clear();
        vAnswers.clear();
        vParameters.clear();
    };

    std::string diff(const CConsultation& b) const {
        std::string ret = "";
        if (mapState != b.mapState)
        {
            std::string sAdded = "";
            std::string sRemoved = "";
            for (auto &it:mapState) if (b.mapState.count(it.first) == 0) sRemoved += it.first.ToString()+":"+std::to_string(it.second)+",";
            for (auto &it:b.mapState) if (mapState.count(it.first) == 0) sAdded += it.first.ToString()+":"+std::to_string(it.second)+",";
            if (sRemoved.size() > 0) sRemoved.pop_back();
            if (sAdded.size() > 0) sAdded.pop_back();
            ret += strprintf("mapState: added: %s - removed: %s, ", sAdded, sRemoved);
        }
        if (hash != b.hash) ret += strprintf("hash: %s => %s, ", hash.ToString(), b.hash.ToString());
        if (txblockhash != b.txblockhash) ret += strprintf("txblockhash: %s => %s, ", txblockhash.ToString(), b.txblockhash.ToString());
        if (nVersion != b.nVersion) ret += strprintf("nVersion: %d => %d, ", nVersion, b.nVersion);
        if (nVotingCycle != b.nVotingCycle) ret += strprintf("nVotingCycle: %d => %d, ", nVotingCycle, b.nVotingCycle);
        if (strDZeel != b.strDZeel) ret += strprintf("strDZeel: %d => %d, ", strDZeel, b.strDZeel);
        if (nSupport != b.nSupport) ret += strprintf("nSupport: %d => %d, ", nSupport, b.nSupport);
        if (nMin != b.nMin) ret += strprintf("nMin: %s => %s, ", nMin, b.nMin);
        if (nMax != b.nMax) ret += strprintf("nMax: %d => %d, ", nMax, b.nMax);
        if (mapVotes != b.mapVotes)
        {
            std::string sList;
            for (auto& it:mapVotes)
            {
                if (b.mapVotes.count(it.first) == 0)
                {
                    sList += strprintf("removed vote %d, ", it.first);
                }
                else if (b.mapVotes.at(it.first) != it.second)
                {
                    sList += strprintf("modified vote %d => %d, ", it.first, b.mapVotes.at(it.first));
                }
            }
            for (auto& it:b.mapVotes)
            {
                if (mapVotes.count(it.first) == 0)
                {
                    sList += strprintf("added vote %d, ", it.first);
                }
            }
            ret = strprintf("mapVotes: %s", sList);
        }
        if (vAnswers != b.vAnswers)
        {
            std::string sAdded = "";
            std::string sRemoved = "";
            for (auto &it:vAnswers)  if (std::find(b.vAnswers.begin(), b.vAnswers.end(), it) == b.vAnswers.end()) sRemoved += it.ToString()+",";
            for (auto &it:b.vAnswers) if (std::find(vAnswers.begin(), vAnswers.end(), it) == vAnswers.end()) sAdded += it.ToString()+",";
            if (sRemoved.size() > 0) sRemoved.pop_back();
            if (sAdded.size() > 0) sAdded.pop_back();
            ret += strprintf("vAnswers: added: %s - removed: %s, ", sAdded, sRemoved);
        }
        if (ret != "")
        {
            ret.pop_back();
            ret.pop_back();
        }
        return ret;
    }


    bool operator==(const CConsultation& b) const {
        return mapState == b.mapState
                && hash == b.hash
                && txblockhash == b.txblockhash
                && mapVotes == b.mapVotes
                && nVersion == b.nVersion
                && nVotingCycle == b.nVotingCycle
                && strDZeel == b.strDZeel
                && nSupport == b.nSupport
                && nVotingCycle == b.nVotingCycle
                && nMin == b.nMin
                && nMax == b.nMax
                && nExclude == b.nExclude
                && vAnswers == vAnswers
                && vParameters == vParameters;
    }

    bool operator!=(const CConsultation& b) const {
        return !(*this == b);
    }

    flags GetLastState() const;

    CBlockIndex* GetLastStateBlockIndex() const;

    CBlockIndex* GetLastStateBlockIndexForState(flags state) const;

    bool SetState(const CBlockIndex* pindex, flags state);

    bool ClearState(const CBlockIndex* pindex);

    std::string GetState(const CBlockIndex* pindex, const CStateViewCache& view) const;
    std::string ToString(const CBlockIndex* pindex, const CStateViewCache& view) const;
    void ToJson(UniValue& ret, const CStateViewCache& view) const;
    bool CanBeSupported() const;
    bool CanBeVoted(int64_t vote = VoteFlags::VOTE_NO) const;
    bool IsSupported(const CStateViewCache& view) const;
    bool CanMoveInReflection(const CStateViewCache& view) const;
    bool IsExpired(const CBlockIndex* pindex, const CStateViewCache& view) const;
    bool IsReflectionOver(const CBlockIndex* pindex, const CStateViewCache& view) const;
    bool IsValidVote(int64_t vote) const;
    bool ExceededMaxVotingCycles(const CStateViewCache& view) const;
    bool IsRange() const;
    bool IsSuper() const;
    bool CanHaveNewAnswers() const;
    bool CanHaveAnswers() const;
    bool HaveEnoughAnswers(const CStateViewCache& view) const;
    bool IsAboutConsensusParameter() const;
    bool IsFinished() const;

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(this->nVersion);
        READWRITE(mapState);
        READWRITE(nSupport);
        if (this->nVersion & SUPER_VERSION)
            READWRITE(vParameters);
        else
            READWRITE(nMin);
        READWRITE(nMax);
        READWRITE(nVotingCycle);
        READWRITE(hash);
        READWRITE(strDZeel);
        READWRITE(txblockhash);
        READWRITE(mapVotes);
        READWRITE(vAnswers);

        if (this->nVersion & EXCLUDE_VERSION)
        {
            READWRITE(nExclude);
        }
    }
};

#endif // NAVCOIN_DAO_H
