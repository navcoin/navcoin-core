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
#include "streams.h"
#include "univalue/include/univalue.h"

#include <cmath>

class BulletproofsRangeproof
{
public:
    static const size_t maxN = 512;
    static const size_t maxM = 16;

    template <typename Stream>
    BulletproofsRangeproof(const libzeroct::IntegerGroupParams* in_p, Stream& strm) : params(in_p)
    {
        Stream strmCopy = strm;
        strmCopy >> *this;
    }

    BulletproofsRangeproof(const libzeroct::IntegerGroupParams* in_p) : params(in_p) {}

    void Prove(CBN_vector v, CBN_vector gamma, const size_t logN = 6);

    ADD_SERIALIZE_METHODS;
    template <typename Stream, typename Operation>  inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(L);
        READWRITE(R);
        READWRITE(A);
        READWRITE(S);
        READWRITE(T1);
        READWRITE(T2);
        READWRITE(taux);
        READWRITE(mu);
        READWRITE(a);
        READWRITE(b);
        READWRITE(t);
    }

    CBN_vector GetValueCommitments() const { return V; }

    void ToJson(UniValue& ret) const;

    CBN_vector V;
    CBN_vector L;
    CBN_vector R;
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
    const libzeroct::IntegerGroupParams* params;
};

bool VerifyBulletproof(const libzeroct::IntegerGroupParams* p, const std::vector<BulletproofsRangeproof>& proof, CBN_matrix v, const unsigned int logN = 6);
CBigNum CrossVectorExponent(size_t size, const CBN_vector &A, size_t Ao, const CBN_vector &B, size_t Bo, const CBN_vector &a, size_t ao, const CBN_vector &b, size_t bo, const CBN_vector *scale, const CBigNum *extra_point, const CBigNum *extra_scalar, const libzeroct::IntegerGroupParams* p);

#endif // BULLETPROOFS_RANGEPROOF_H
