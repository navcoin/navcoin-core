// Copyright (c) 2017 The NavCoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef NAVCOIN_CFUND_H
#define NAVCOIN_CFUND_H

#include "amount.h"
#include "script/script.h"
#include "serialize.h"
#include "tinyformat.h"
#include "uint256.h"

#define FUND_MINIMAL_FEE 10000000000

namespace CFund {
void SetScriptForCommunityFundContribution(CScript &script);

class CProposal;

class CPaymentRequest
{
public:
    CAmount nAmount;
    unsigned char fState;
    uint256 hash;
    unsigned int nout;

    CPaymentRequest() { SetNull(); }

    void SetNull() {
        nAmount = 0;
        fState = 0;
        hash = uint256();
        nout = 0;
    }

    bool IsNull() {
        return (nAmount == 0 && fState == 0 && nout == 0);
    }

    std::string ToString() const
    {
        return strprintf("CPaymentRequest(amount=%u, fState=%u, hash=%s, nout=%u)", nAmount, fState, hash.ToString().substr(0,10), nout);
    }

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(nAmount);
        READWRITE(fState);
        READWRITE(hash);
        READWRITE(nout);
    }
};

class CProposal
{
public:
    CAmount nAmount;
    CAmount nFee;
    std::string Address;
    uint32_t nDeadline;
    unsigned char fState;
    std::vector<CPaymentRequest> vPayments;
    std::string strDZeel;

    CProposal() { SetNull(); }

    void SetNull() {
        nAmount = 0;
        nFee = 0;
        Address = "";
        fState = 0;
        nDeadline = 0;
        vPayments.clear();
        strDZeel = "";
    }

    bool IsNull() const {
        return (nAmount == 0 && nFee == 0 && Address == "" && fState == 0 && nDeadline == 0 && strDZeel == "");
    }

    std::string ToString() const
    {
        std::string str;
        str += strprintf("CProposal(amount=%u, nFee=%u, address=%s, nDeadline=%u, fState=%u, strDZeel=%s)",
                         nAmount, nFee, Address, nDeadline, fState, strDZeel);
        for (unsigned int i = 0; i < vPayments.size(); i++)
            str += "    " + vPayments[i].ToString() + "\n";
        return str;
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
        READWRITE(*const_cast<std::vector<CPaymentRequest>*>(&vPayments));
        READWRITE(strDZeel);
    }
};

}

#endif // NAVCOIN_CFUND_H
