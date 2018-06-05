// Copyright (c) 2017 The NavCoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef NAVCOIN_CFUND_H
#define NAVCOIN_CFUND_H

#include "amount.h"
#include "script/script.h"
#include "serialize.h"
#include "tinyformat.h"
#include "univalue/include/univalue.h"
#include "uint256.h"

using namespace std;

class CTransaction;

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

void SetScriptForCommunityFundContribution(CScript &script);
void SetScriptForProposalVote(CScript &script, uint256 proposalhash, bool vote);
void SetScriptForPaymentRequestVote(CScript &script, uint256 prequest, bool vote);
bool FindProposal(string propstr, CFund::CProposal &proposal);
bool FindProposal(uint256 prophash, CFund::CProposal &proposal);
bool FindPaymentRequest(uint256 preqhash, CFund::CPaymentRequest &prequest);
bool FindPaymentRequest(string preqstr, CFund::CPaymentRequest &prequest);
void UpdateMapPaymentRequest(uint256 preqhash, CFund::CPaymentRequest prequest);
void UpdateMapPaymentRequest(uint256 preqhash);
void UpdateMapProposal(uint256 preqhash, CFund::CProposal prequest);
void UpdateMapProposal(uint256 preqhash);
bool VoteProposal(string strProp, bool vote, bool &duplicate);
bool VoteProposal(uint256 proposalHash, bool vote, bool &duplicate);
bool RemoveVoteProposal(string strProp);
bool RemoveVoteProposal(uint256 proposalHash);
bool VotePaymentRequest(string strProp, bool vote, bool &duplicate);
bool VotePaymentRequest(uint256 proposalHash, bool vote, bool &duplicate);
bool RemoveVotePaymentRequest(string strProp);
bool RemoveVotePaymentRequest(uint256 proposalHash);
bool IsValidPaymentRequest(CTransaction tx);
bool IsValidProposal(CTransaction tx);

class CPaymentRequest
{
public:
    CAmount nAmount;
    flags fState;
    uint256 hash;
    uint256 proposalhash;
    uint256 blockhash;
    uint256 paymenthash;
    int nVotesYes;
    int nVotesNo;
    string strDZeel;

    CPaymentRequest() { SetNull(); }

    void SetNull() {
        nAmount = 0;
        fState = NIL;
        nVotesYes = 0;
        nVotesNo = 0;
        hash = uint256();
        proposalhash = uint256();
        paymenthash = uint256();
        strDZeel = "";
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
        return sFlags;
    }

    std::string ToString() const {
        return strprintf("CPaymentRequest(hash=%s, nAmount=%f, fState=%s, nVotesYes=%u, nVotesNo=%u, proposalhash=%s, "
                         "blockhash=%s, paymenthash=%s, strDZeel=%s)",
                         hash.ToString().substr(0,10), (float)nAmount/COIN, GetState(), nVotesYes, nVotesNo, proposalhash.ToString().substr(0,10),
                         blockhash.ToString().substr(0,10), paymenthash.ToString().substr(0,10), strDZeel);
    }

    void ToJson(UniValue& ret) const;

    bool IsAccepted() const;

    bool IsRejected() const;

    bool CanVote() const;

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(nAmount);
        READWRITE(fState);
        READWRITE(nVotesYes);
        READWRITE(nVotesNo);
        READWRITE(hash);
        READWRITE(proposalhash);
        READWRITE(blockhash);
        READWRITE(paymenthash);
        READWRITE(strDZeel);
    }

};

class CProposal
{
public:
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
    }

    bool IsNull() const {
        return (nAmount == 0 && nFee == 0 && Address == "" && nVotesYes == 0 && fState == NIL
                && nVotesNo == 0 && nDeadline == 0 && strDZeel == "");
    }

    std::string ToString(uint32_t currentTime = 0) const {
        std::string str;
        str += strprintf("CProposal(hash=%s, nAmount=%f, available=%f, nFee=%f, address=%s, nDeadline=%u, nVotesYes=%u, "
                         "nVotesNo=%u, fState=%s, strDZeel=%s, blockhash=%s)",
                         hash.ToString(), (float)nAmount/COIN, (float)GetAvailable()/COIN, (float)nFee/COIN, Address, nDeadline,
                         nVotesYes, nVotesNo, GetState(currentTime), strDZeel, blockhash.ToString().substr(0,10));
        for (unsigned int i = 0; i < vPayments.size(); i++) {
            CFund::CPaymentRequest prequest;
            if(FindPaymentRequest(vPayments[i], prequest))
                str += "\n    " + prequest.ToString();
        }
        return str + "\n";
    }

    std::string GetState(uint32_t currentTime) const {
        std::string sFlags = "pending";
        if(IsAccepted()) {
            sFlags = "accepted";
            if(fState == PENDING_FUNDS)
                sFlags += " waiting for enough coins in fund";
            else if(fState != ACCEPTED)
                sFlags += " waiting for end of voting period";
        }
        if(IsRejected()) {
            sFlags = "rejected";
            if(fState != REJECTED)
                sFlags += " waiting for end of voting period";
        }
        if(currentTime > 0 && IsExpired(currentTime)) {
            sFlags = "expired";
            if(fState != EXPIRED)
                sFlags += " waiting for end of voting period";
        }
        return sFlags;
    }

    void ToJson(UniValue& ret) const;

    bool IsAccepted() const;

    bool IsRejected() const;

    bool IsExpired(uint32_t currentTime) const {
        return (nDeadline < currentTime);
    }

    bool CanVote() const {
        return fState == NIL;
    }

    bool CanRequestPayments() const {
        return fState == ACCEPTED;
    }

    CAmount GetAvailable(bool fIncludeRequests = false) const
    {
        CAmount initial = nAmount;
        for (unsigned int i = 0; i < vPayments.size(); i++)
        {
            CFund::CPaymentRequest prequest;
            if(FindPaymentRequest(vPayments[i], prequest))
                if(fIncludeRequests || (!fIncludeRequests && prequest.fState == ACCEPTED))
                    initial -= prequest.nAmount;
        }
        return initial;
    }

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        if (ser_action.ForRead()) {
            const_cast<std::vector<uint256>*>(&vPayments)->clear();
        }
        READWRITE(nAmount);
        READWRITE(nFee);
        READWRITE(Address);
        READWRITE(nDeadline);
        READWRITE(fState);
        READWRITE(nVotesYes);
        READWRITE(nVotesNo);
        READWRITE(*const_cast<std::vector<uint256>*>(&vPayments));
        READWRITE(strDZeel);
        READWRITE(hash);
        READWRITE(blockhash);
    }

};

}

#endif // NAVCOIN_CFUND_H
