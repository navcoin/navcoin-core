// Copyright (c) 2017 The NavCoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef NAVCOIN_CFUND_H
#define NAVCOIN_CFUND_H

#include "amount.h"
#include "script/script.h"
#include "serialize.h"
#include "uint256.h"

#define FUND_MINIMAL_FEE 10000000000

namespace CFund {
void SetScriptForCommunityFundContribution(CScript &script);

class CProposal;

class CPaymentRequest
{
public:
    CAmount nValue;
    unsigned char fState;
    uint256 hash;
    unsigned int nout;

    CPaymentRequest() { SetNull(); };
    CPaymentRequest(CAmount& nValue, unsigned char fState, uint256 hash, unsigned int nout);

    void SetNull() {
        nValue = 0;
        fState = 0;
        hash = uint256();
        nout = 0;
    }

    bool IsNull() {
        return (nValue == 0 && fState == 0 && nout == 0);
    }

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(nValue);
        READWRITE(fState);
        READWRITE(hash);
        READWRITE(nout);
    }
};

class CProposal
{
public:
    const CAmount nValue;
    const std::string Address;
    const unsigned int nDeadline;
    const unsigned char fState;
    std::vector<CPaymentRequest> vPayments;
    const std::string strDZeel;

    CProposal(const CAmount& nValue, std::string Address, unsigned int nDeadline, unsigned char fState,
              std::vector<CPaymentRequest> vPayments, std::string strDZeel);

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(nValue);
        READWRITE(Address);
        READWRITE(nDeadline);
        READWRITE(fState);
        READWRITE(*const_cast<std::vector<CPaymentRequest>*>(&vPayments));
        READWRITE(strDZeel);
    }
};

}

#endif // NAVCOIN_CFUND_H
