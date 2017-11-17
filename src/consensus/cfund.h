// Copyright (c) 2017 The NavCoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef NAVCOIN_CFUND_H
#define NAVCOIN_CFUND_H

#include "amount.h"
#include "net.h"
#include "script/script.h"
#include "serialize.h"
#include "tinyformat.h"
#include "uint256.h"
#include "util.h"

#define FUND_MINIMAL_FEE 10000000000

using namespace std;

namespace CFund {

void SetScriptForCommunityFundContribution(CScript &script);
void VoteProposal(string strProp);
void VoteProposal(uint256 proposalHash);
void RemoveVoteProposal(string strProp);
void RemoveVoteProposal(uint256 proposalHash);
void VotePaymentRequest(string strProp);
void VotePaymentRequest(uint256 proposalHash);
void RemoveVotePaymentRequest(string strProp);
void RemoveVotePaymentRequest(uint256 proposalHash);

class CProposal;

class CPaymentRequest
{
public:
    CAmount nAmount;
    unsigned char fState;
    uint256 hash;
    uint256 proposalHash;
    uint256 paymentHash;
    int votes;

    CPaymentRequest() { SetNull(); }

    void SetNull() {
        nAmount = 0;
        fState = 0;
        votes = 0;
        hash = uint256();
        proposalHash = uint256();
        paymentHash = uint256();
    }

    bool IsNull() const {
        return (nAmount == 0 && fState == 0 && votes == 0);
    }

    void Accept() {
        fState = ACCEPTED;
    }

    void Reject() {
        fState = REJECTED;
    }

    std::string ToString() const
    {
        std::string sFlags;
        if(IsAccepted())
            sFlags = "accepted";
        if(IsRejected())
            sFlags = "rejected";
        return strprintf("CPaymentRequest(hash=%s, amount=%u, fState=%s, votes=%u, proposalHash=%s, paymentHash=%s)",
                         hash.ToString().substr(0,10), nAmount, sFlags, votes, proposalHash.ToString().substr(0,10), paymentHash.ToString().substr(0,10));
    }

    bool IsAccepted() const {
        return fState == ACCEPTED;
    }

    bool IsRejected() const {
        return fState == REJECTED;
    }

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(nAmount);
        READWRITE(fState);
        READWRITE(votes);
        READWRITE(hash);
        READWRITE(proposalHash);
        READWRITE(paymentHash);
    }

private: // TODO: move to enum
    const char ACCEPTED = 0x01;
    const char REJECTED = 0x02;

};

class CProposal
{
public:
    CAmount nAmount;
    CAmount nFee;
    std::string Address;
    uint32_t nDeadline;
    unsigned char fState;
    int votes;
    std::vector<CPaymentRequest> vPayments;
    std::string strDZeel;
    uint256 hash;
    uint256 blockhash;

    CProposal() { SetNull(); }

    void SetNull() {
        nAmount = 0;
        nFee = 0;
        Address = "";
        fState = 0;
        votes = 0;
        nDeadline = 0;
        vPayments.clear();
        strDZeel = "";
        hash = uint256();
        blockhash = uint256();
    }

    bool IsNull() const {
        return (nAmount == 0 && nFee == 0 && Address == "" && votes == 0 && fState == 0 && nDeadline == 0 && strDZeel == "");
    }

    void Accept() {
        fState = ACCEPTED;
    }

    void Reject() {
        fState = REJECTED;
    }

    std::string ToString(uint32_t currentTime = 0) const
    {
        std::string sFlags;
        if(IsAccepted())
            sFlags = "accepted";
        if(IsRejected())
            sFlags = "rejected";
        if(currentTime > 0 && IsExpired(currentTime))
            sFlags = "expired";
        std::string str;
        str += strprintf("CProposal(hash=%s, amount=%u, available=%d, nFee=%u, address=%s, nDeadline=%u, votes=%u, fState=%s, strDZeel=%s, blockhash=%s)",
                         hash.ToString(), nAmount, GetAvailable(), nFee, Address, nDeadline, votes, sFlags, strDZeel, blockhash.ToString().substr(0,10));
        for (unsigned int i = 0; i < vPayments.size(); i++)
            str += "    " + vPayments[i].ToString() + "\n";
        return str;
    }

    bool IsAccepted() const {
        return fState == ACCEPTED;
    }

    bool IsRejected() const {
        return fState == REJECTED;
    }

    bool IsExpired(uint32_t currentTime) const {
        return (nDeadline > currentTime);
    }

    CAmount GetAvailable(bool fIncludeRequests = false) const
    {
        CAmount initial = nAmount;
        for (unsigned int i = 0; i < vPayments.size(); i++)
        {
            if(fIncludeRequests || (!fIncludeRequests && vPayments[i].IsAccepted()))
                initial -= vPayments[i].nAmount;
        }
        return initial;
    }

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        if (ser_action.ForRead()) {
            const_cast<std::vector<CPaymentRequest>*>(&vPayments)->clear();
        }
        READWRITE(nAmount);
        READWRITE(nFee);
        READWRITE(Address);
        READWRITE(nDeadline);
        READWRITE(fState);
        READWRITE(votes);
        READWRITE(*const_cast<std::vector<CPaymentRequest>*>(&vPayments));
        READWRITE(strDZeel);
        READWRITE(hash);
        READWRITE(blockhash);
    }

private: // TODO: move to enum
    const char ACCEPTED = 0x01;
    const char REJECTED = 0x02;

};

}

#endif // NAVCOIN_CFUND_H
