// Copyright (c) 2018 The NavCoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef NAVCOIN_CFUND_H
#define NAVCOIN_CFUND_H

#include <amount.h>
#include <script/script.h>
#include <serialize.h>
#include <tinyformat.h>
#include <univalue/include/univalue.h>
#include <uint256.h>

using namespace std;

class CTransaction;
class CCoinsViewCache;
class CBlockIndex;
class CChainParams;
class CValidationState;

extern std::vector<std::pair<std::string, bool>> vAddedProposalVotes;
extern std::vector<std::pair<std::string, bool>> vAddedPaymentRequestVotes;

namespace CFund {

class CProposal;
class CPaymentRequest;

typedef unsigned int flags;

static const flags NIL = 0x0;
static const flags ACCEPTED = 0x1;
static const flags REJECTED = 0x2;
static const flags EXPIRED = 0x3;
static const flags PENDING_FUNDS = 0x4;
static const flags PENDING_VOTING_PREQ = 0x5;

void SetScriptForCommunityFundContribution(CScript &script);
void SetScriptForProposalVote(CScript &script, uint256 proposalhash, bool vote);
void SetScriptForPaymentRequestVote(CScript &script, uint256 prequest, bool vote);
bool VoteProposal(CProposal proposal, bool vote, bool &duplicate);
bool RemoveVoteProposal(string strProp);
bool RemoveVoteProposal(uint256 proposalHash);
bool VotePaymentRequest(CPaymentRequest prequest, bool vote, bool &duplicate);
bool RemoveVotePaymentRequest(string strProp);
bool RemoveVotePaymentRequest(uint256 proposalHash);
bool IsValidPaymentRequest(CTransaction tx, CCoinsViewCache& coins, int nMaxVersion);
bool IsValidProposal(CTransaction tx, int nMaxVersion);

class CPaymentRequest
{
public:
    static const int32_t CURRENT_VERSION=3;

    CAmount nAmount;
    flags fState;
    uint256 hash;
    uint256 proposalhash;
    uint256 txblockhash;
    uint256 blockhash;
    uint256 paymenthash;
    int nVotesYes;
    int nVotesNo;
    string strDZeel;
    int nVersion;
    unsigned int nVotingCycle;
    bool fDirty;

    CPaymentRequest() { SetNull(); }

    void SetNull() {
        nAmount = 0;
        fState = NIL;
        nVotesYes = 0;
        nVotesNo = 0;
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
        std::swap(to.strDZeel, strDZeel);
        std::swap(to.nVersion, nVersion);
        std::swap(to.nVotingCycle, nVotingCycle);
        std::swap(to.fDirty, fDirty);
    }

    bool operator==(const CPaymentRequest& b) const {
        return nAmount == b.nAmount
                && fState == b.fState
                && hash == b.hash
                && proposalhash == b.proposalhash
                && txblockhash == b.txblockhash
                && blockhash == b.blockhash
                && paymenthash == b.paymenthash
                && nVotesYes == b.nVotesYes
                && nVotesNo == b.nVotesNo
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
        if (fState != b.fState) ret += strprintf("fState: %d => %d, ", fState, b.fState);
        if (nVotesYes != b.nVotesYes) ret += strprintf("nVotesYes: %d => %d, ", nVotesYes, b.nVotesYes);
        if (nVotesNo != b.nVotesNo) ret += strprintf("nVotesNo: %d => %d, ", nVotesNo, b.nVotesNo);
        if (strDZeel != b.strDZeel) ret += strprintf("strDZeel: %s => %s, ", strDZeel, b.strDZeel);
        if (hash != b.hash) ret += strprintf("hash: %s => %s, ", hash, b.hash);
        if (proposalhash != b.proposalhash) ret += strprintf("proposalhash: %s => %s, ", proposalhash.ToString(), b.proposalhash.ToString());
        if (paymenthash != b.paymenthash) ret += strprintf("paymenthash: %s => %s, ", paymenthash.ToString(), b.paymenthash.ToString());
        if (txblockhash != b.txblockhash) ret += strprintf("txblockhash: %s => %s, ", txblockhash.ToString(), b.txblockhash.ToString());
        if (blockhash != b.blockhash) ret += strprintf("blockhash: %s => %s, ", blockhash.ToString(), b.blockhash.ToString());
        if (nVersion != b.nVersion) ret += strprintf("nVersion: %d => %d, ", nVersion, b.nVersion);
        if (nVotingCycle != b.nVotingCycle) ret += strprintf("nVotingCycle: %s => %s, ", nVotingCycle, b.nVotingCycle);
        if (ret != "")
        {
            ret.pop_back();
            ret.pop_back();
        }
        return ret;
    }

    bool IsNull() const {
        return (nAmount == 0 && fState == NIL && nVotesYes == 0 && nVotesNo == 0 && strDZeel == "");
    }

    std::string GetState() const {
        std::string sFlags = "pending";
        if(IsAccepted()) {
            sFlags = "accepted";
            if(fState != ACCEPTED)
                sFlags += " waiting for end of voting period";
        }
        if(IsRejected()) {
            sFlags = "rejected";
            if(fState != REJECTED)
                sFlags += " waiting for end of voting period";
        }
        if(IsExpired())
            sFlags = "expired";
        return sFlags;
    }

    std::string ToString() const {
        return strprintf("CPaymentRequest(hash=%s, nVersion=%d, nAmount=%f, fState=%s, nVotesYes=%u, nVotesNo=%u, nVotingCycle=%u, "
                         " proposalhash=%s, blockhash=%s, paymenthash=%s, strDZeel=%s)",
                         hash.ToString(), nVersion, (float)nAmount/COIN, GetState(), nVotesYes, nVotesNo,
                         nVotingCycle, proposalhash.ToString(), blockhash.ToString().substr(0,10),
                         paymenthash.ToString().substr(0,10), strDZeel);
    }

    void ToJson(UniValue& ret) const;

    bool IsAccepted() const;

    bool IsRejected() const;

    bool IsExpired() const;

    bool ExceededMaxVotingCycles() const;

    bool CanVote(CCoinsViewCache& coins) const;

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
        if(nVersion >= 2)
           READWRITE(nVotingCycle);
    }

};

class CProposal
{
public:
    static const int32_t CURRENT_VERSION=3;

    CAmount nAmount;
    CAmount nFee;
    std::string Address;
    uint32_t nDeadline;
    flags fState;
    int nVotesYes;
    int nVotesNo;
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
        fState = NIL;
        nVotesYes = 0;
        nVotesNo = 0;
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
        std::swap(to.vPayments, vPayments);
        std::swap(to.strDZeel, strDZeel);
        std::swap(to.hash, hash);
        std::swap(to.txblockhash, txblockhash);
        std::swap(to.blockhash, blockhash);
        std::swap(to.nVersion, nVersion);
        std::swap(to.nVotingCycle, nVotingCycle);
        std::swap(to.fDirty, fDirty);
    }

    bool operator==(const CProposal& b) const {
        return nAmount == b.nAmount
                && nFee == b.nFee
                && Address == b.Address
                && nDeadline == b.nDeadline
                && fState == b.fState
                && nVotesYes == b.nVotesYes
                && nVotesNo == b.nVotesNo
                && vPayments == b.vPayments
                && strDZeel == b.strDZeel
                && hash == b.hash
                && txblockhash == b.txblockhash
                && blockhash == b.blockhash
                && nVersion == b.nVersion
                && nVotingCycle == b.nVotingCycle;
    }

    std::string diff(const CProposal& b) const {
        std::string ret = "";
        if (nAmount != b.nAmount) ret += strprintf("nAmount: %d => %d, ", nAmount, b.nAmount);
        if (nFee != b.nFee) ret += strprintf("nFee: %d => %d, ", nFee, b.nFee);
        if (Address != b.Address) ret += strprintf("Address: %s => %s, ", Address, b.Address);
        if (nDeadline != b.nDeadline) ret += strprintf("nDeadline: %d => %d, ", nDeadline, b.nDeadline);
        if (fState != b.fState) ret += strprintf("fState: %d => %d, ", fState, b.fState);
        if (nVotesYes != b.nVotesYes) ret += strprintf("nVotesYes: %d => %d, ", nVotesYes, b.nVotesYes);
        if (nVotesNo != b.nVotesNo) ret += strprintf("nVotesNo: %d => %d, ", nVotesNo, b.nVotesNo);
        if (vPayments != b.vPayments)
        {
            std::string thisStrPayments = "";
            std::string bStrPayments = "";
            for (auto &it:vPayments) thisStrPayments += it.ToString()+",";
            for (auto &it:b.vPayments) bStrPayments += it.ToString()+",";
            thisStrPayments.pop_back();
            bStrPayments.pop_back();
            ret += strprintf("vPayments: %s => %s, ", thisStrPayments, bStrPayments);
        }
        if (strDZeel != b.strDZeel) ret += strprintf("strDZeel: %s => %s, ", strDZeel, b.strDZeel);
        if (hash != b.hash) ret += strprintf("hash: %s => %s, ", hash.ToString(), b.hash.ToString());
        if (txblockhash != b.txblockhash) ret += strprintf("txblockhash: %s => %s, ", txblockhash.ToString(), b.txblockhash.ToString());
        if (blockhash != b.blockhash) ret += strprintf("blockhash: %s => %s, ", blockhash.ToString(), b.blockhash.ToString());
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

    bool IsNull() const {
        return (nAmount == 0 && nFee == 0 && Address == "" && nVotesYes == 0 && fState == NIL
                && nVotesNo == 0 && nDeadline == 0 && strDZeel == "");
    }

    std::string ToString(CCoinsViewCache& coins, uint32_t currentTime = 0) const;
    std::string GetState(uint32_t currentTime) const;

    void ToJson(UniValue& ret, CCoinsViewCache& coins) const;

    bool IsAccepted() const;

    bool IsRejected() const;

    bool IsExpired(uint32_t currentTime) const;

    bool ExceededMaxVotingCycles() const;

    uint64_t getTimeTillExpired(uint32_t currentTime) const;

    bool CanVote() const;

    bool CanRequestPayments() const {
        return fState == ACCEPTED;
    }

    bool HasPendingPaymentRequests(CCoinsViewCache& coins) const;

    CAmount GetAvailable(CCoinsViewCache& coins, bool fIncludeRequests = false) const;

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
        if(nVersion >= 2) {
           READWRITE(nVotingCycle);
        }

    }
};

bool IsBeginningCycle(const CBlockIndex* pindex, CChainParams params);
bool IsEndCycle(const CBlockIndex* pindex, CChainParams params);
void CFundStep(const CValidationState& state, CBlockIndex *pindexNew, const bool fUndo, CCoinsViewCache& coins);

}

#endif // NAVCOIN_CFUND_H
