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

#include "SerialNumberProofOfKnowledge.h"
#include <iostream>
#include <sstream>

namespace libzerocoin {

SerialNumberProofOfKnowledge::SerialNumberProofOfKnowledge(const IntegerGroupParams* p): params(p) { }

SerialNumberProofOfKnowledge::SerialNumberProofOfKnowledge(const IntegerGroupParams* p, const CBigNum serialNumber, const uint256 signatureHash) : params(p)
{
    CBigNum y = params->g.pow_mod(serialNumber, params->modulus);
    CBigNum v = CBigNum::randBignum(params->groupOrder);
    CBigNum t = params->g.pow_mod(v, params->modulus);
    CHashWriter hasher(0,0);
    hasher << *params << y << t << signatureHash;
    CBigNum c = CBigNum(hasher.GetHash());
    CBigNum r = v - (c * serialNumber);
    this->t = t;
    this->r = r;
}
bool SerialNumberProofOfKnowledge::Verify(const CBigNum& coinSerialNumberPubKey, const uint256 signatureHash) const
{
    CHashWriter hasher(0,0);
    hasher << *params << coinSerialNumberPubKey << t << signatureHash;
    CBigNum c = CBigNum(hasher.GetHash());
    CBigNum u = params->g.pow_mod(r, params->modulus).mul_mod(coinSerialNumberPubKey.pow_mod(c, params->modulus), params->modulus);
    return (t == u);
}
}
