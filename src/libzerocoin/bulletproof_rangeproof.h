/**
* @file       Math.h
*
* @brief      Bulletproofs Rangeproof
*             Inspired by https://github.com/b-g-goodell/research-lab/blob/master/source-code/StringCT-java/src/how/monero/hodl/bulletproof/Bulletproof.java
*
* @author     alex v
* @date       December 2018
*
* @copyright  Copyright 2018 alex v
* @license    This project is released under the MIT license.
**/
// Copyright (c) 2018 The NavCoin Core developers

#ifndef BULLETPROOF_RANGEPROOF_H
#define BULLETPROOF_RANGEPROOF_H

#include "Params.h"
#include "Math.h"

#include "hash.h"

#include <cmath>

namespace Bulletproof
{

class Rangeproof
{
public:
    Rangeproof(const libzerocoin::IntegerGroupParams* in_p, unsigned int in_nexp) : p(in_p), nexp(in_nexp) { n = pow(2,nexp); }

    bool Prove(std::vector<CBigNum> v, std::vector<CBigNum> gamma, unsigned int logM);

    ADD_SERIALIZE_METHODS;
    template <typename Stream, typename Operation>  inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(V);
        READWRITE(L);
        READWRITE(R);
        READWRITE(a);
        READWRITE(S);
        READWRITE(T1);
        READWRITE(T2);
        READWRITE(taux);
        READWRITE(mu);
        READWRITE(a);
        READWRITE(b);
        READWRITE(t);
    }

private:
    const libzerocoin::IntegerGroupParams* p;

    unsigned int nexp;
    unsigned int n;

    std::vector<CBigNum> V;
    std::vector<CBigNum> L;
    std::vector<CBigNum> R;
    CBigNum A;
    CBigNum S;
    CBigNum T1;
    CBigNum T2;
    CBigNum taux;
    CBigNum mu;
    CBigNum a;
    CBigNum b;
    CBigNum t;

};


}

#endif // BULLETPROOF_RANGEPROOF_H
