/**
* @file       Math.h
*
* @brief      Bulletproofs Rangeproof
*             inspired by https://github.com/b-g-goodell/research-lab/blob/master/source-code/StringCT-java/src/how/monero/hodl/bulletproof/Bulletproof.java
*             and https://github.com/monero-project/monero/blob/master/src/ringct/bulletproofs.cc
*
* @author     alex v
* @date       February 2019
*
* @license    This project is released under the MIT license.
**/
// Copyright (c) 2018-2019 The NavCoin Core developers

#ifndef BULLETPROOFS_RANGEPROOF_H
#define BULLETPROOFS_RANGEPROOF_H

#include "Params.h"
#include "Math.h"

#include "hash.h"

#include <cmath>

class BulletproofsRangeproof
{
public:
    static const size_t maxN = 64;
    static const size_t maxM = 16;

    BulletproofsRangeproof(const libzerocoin::IntegerGroupParams* in_p) : params(in_p) {}

    void Prove(std::vector<CBigNum> v, std::vector<CBigNum> gamma);

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

private:
    const libzerocoin::IntegerGroupParams* params;
};

bool VerifyBulletproof(const libzerocoin::IntegerGroupParams* p, const std::vector<BulletproofsRangeproof>& proof);
CBigNum CrossVectorExponent(size_t size, const std::vector<CBigNum> &A, size_t Ao, const std::vector<CBigNum> &B, size_t Bo, const std::vector<CBigNum> &a, size_t ao, const std::vector<CBigNum> &b, size_t bo, const std::vector<CBigNum> *scale, const CBigNum *extra_point, const CBigNum *extra_scalar, const libzerocoin::IntegerGroupParams* p);

#endif // BULLETPROOFS_RANGEPROOF_H
