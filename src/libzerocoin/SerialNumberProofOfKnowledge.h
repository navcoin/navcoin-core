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

#define BASE_BLOCK_SIGNATURE "1A0878AC4739B61F3E3036875C286C0B9AA73B842457E708A1753E8BEAC48411011CFEC7080D257140353881FF7D0F6EBF9F01678EB0DB42D38BFD9E289CAFDD6EDD153FB7AD05DA18BDB3AC5AB3D2E341C5576FC2C1C8CB39D2421495EFB6F8D09E70EA3"

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
