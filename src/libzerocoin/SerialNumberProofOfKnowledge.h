/**
* @file       SerialNumberProofOfKnowledge.cpp
*
* @brief      SerialNumberProofOfKnowledge class for the Zerocoin library.
*
* @author     alex v
* @date       September 2018
*
* @copyright  Copyright 2018 alex v
* @license    This project is released under the MIT license.
**/
// Copyright (c) 2018 The NavCoin Core developers

#ifndef SERIALNUMBERPROOFOFKNOWLEDGE_H
#define SERIALNUMBERPROOFOFKNOWLEDGE_H

#include "hash.h"
#include "bignum.h"
#include "Params.h"

#define BASE_BLOCK_SIGNATURE 108493142922526728666632567817665003170796919105635379209392198423511598366861400891077767613510689994661458247627782967131380742158803028619370850599707720731033316825396014640162292085141754403426615747531818926542832854769301325069842551396491200417945280389398187668412942712019408916579750269745817796962
#define BASE_SPEND_SIGNATURE 108493142922526728666632567817665003170796919105635379209392198423511598366861400891077767613510689994661458247627782967131380742158803028619370850599707720731033316825396014640162292085141754403426615747531818926542832854769301325069842551396491200417945280389398187668412942712019408916579750269745817796962

using namespace std;

namespace libzerocoin {

class SerialNumberProofOfKnowledge
{
public:
    SerialNumberProofOfKnowledge(const IntegerGroupParams* p);
    SerialNumberProofOfKnowledge(const IntegerGroupParams* p, const CBigNum serialNumber, const uint256 signatureHash);

    bool Verify(const CBigNum& coinSerialNumberPubKey, const uint256 signatureHash) const;
    ADD_SERIALIZE_METHODS;
    template <typename Stream, typename Operation>  inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(t);
        READWRITE(r);
    }
private:
    const IntegerGroupParams* params;
    CBigNum t;
    CBigNum r;
};
}

#endif // SERIALNUMBERPROOFOFKNOWLEDGE_H
