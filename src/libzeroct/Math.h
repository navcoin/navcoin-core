/**
* @file       Math.h
*
* @brief      Diverse arithmetic operations
*             inspired by https://github.com/b-g-goodell/research-lab/blob/master/source-code/StringCT-java/src/how/monero/hodl/bulletproof/Bulletproof.java
*             and https://github.com/monero-project/monero/blob/master/src/ringct/bulletproofs.cc
*
* @author     alex v
* @date       February 2019
*
* @license    This project is released under the MIT license.
**/
// Copyright (c) 2018-2019 The NavCoin Core developers

#ifndef VECTORMATH_H
#define VECTORMATH_H

#include "bignum.h"
#include "Params.h"

struct MultiexpData {
    CBigNum base;
    CBigNum exp;

    MultiexpData() {}
    MultiexpData(const CBigNum &s, const CBigNum &p): base(s), exp(p) {}
};

// Todo multi-exp optimization
CBigNum MultiExp(std::vector<MultiexpData> multiexp_data, const libzeroct::IntegerGroupParams* params);
CBigNum VectorPowerSum(const CBigNum &x, unsigned int n);
CBN_vector VectorExponent(const CBN_vector& a, const CBigNum& a_exp, CBigNum mod);
CBigNum VectorExponent2Mod(const CBN_vector& a, const CBN_vector& a_exp, const CBN_vector& b, const CBN_vector& b_exp, CBigNum mod);
CBN_vector VectorPowers(CBigNum x, unsigned int size);
CBigNum InnerProduct(const CBN_vector& a, const CBN_vector& b, CBigNum mod);
CBigNum InnerProduct(const CBN_vector& a, const CBN_vector& b);
CBN_vector Hadamard(const CBN_vector& a, const CBN_vector& b, CBigNum mod);
CBN_vector VectorAdd(const CBN_vector& a, const CBN_vector& b, CBigNum mod);
CBN_vector VectorAdd(const CBN_vector& a, const CBigNum& b);
CBN_vector VectorSubtract(const CBN_vector& a, const CBigNum& b);
CBN_vector VectorScalar(const CBN_vector& a, const CBigNum& x, CBigNum mod);
CBN_vector VectorSlice(const CBN_vector& a, unsigned int start, unsigned int stop);
CBN_vector HadamardFold(CBN_vector &vec, CBN_vector *scale, const CBigNum &a, const CBigNum &b, const CBigNum& mod, const CBigNum& order);
CBN_vector VectorInvert(CBN_vector x, CBigNum modulus);
inline std::string toStringVector(const CBN_vector v, int base=16);

#endif // VECTORMATH_H
