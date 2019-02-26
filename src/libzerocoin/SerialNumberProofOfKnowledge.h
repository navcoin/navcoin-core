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

#define BASE_BLOCK_SIGNATURE "9a7fd6508dfa79258e50019ab6cb59b4f91b2823dcd9250fb3ccf9fd8263b29a15b005c429915cec63e7d3eba1da337f45dd713246c41e39ac671cf2f87adfc6d45c842ae7ad21ed291e3a48b2a6e5d39381f6d4a9ab83d5aaa5031d17554df70cf5ecfe10096cf1a565d0f826b71eb4d105a3016afc445613f04ffbd0dd4162"

using namespace std;

namespace libzerocoin {

class SerialNumberProofOfKnowledge
{
public:
    SerialNumberProofOfKnowledge(const IntegerGroupParams* p);
    SerialNumberProofOfKnowledge(const IntegerGroupParams* p, const CBigNum serialNumber, const uint256 signatureHash, CBigNum base=0);

    bool Verify(const CBigNum& coinSerialNumberPubKey, const uint256 signatureHash, CBigNum base=0) const;
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
