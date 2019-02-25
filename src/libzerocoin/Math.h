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
CBigNum MultiExp(std::vector<MultiexpData> multiexp_data, const libzerocoin::IntegerGroupParams* params);
CBigNum VectorPowerSum(const CBigNum &x, unsigned int n);
std::vector<CBigNum> VectorExponent(const std::vector<CBigNum>& a, const CBigNum& a_exp, CBigNum mod);
CBigNum VectorExponent2Mod(const std::vector<CBigNum>& a, const std::vector<CBigNum>& a_exp, const std::vector<CBigNum>& b, const std::vector<CBigNum>& b_exp, CBigNum mod);
std::vector<CBigNum> VectorPowers(CBigNum x, unsigned int size);
CBigNum InnerProduct(const std::vector<CBigNum>& a, const std::vector<CBigNum>& b, CBigNum mod);
CBigNum InnerProduct(const std::vector<CBigNum>& a, const std::vector<CBigNum>& b);
std::vector<CBigNum> Hadamard(const std::vector<CBigNum>& a, const std::vector<CBigNum>& b, CBigNum mod);
std::vector<CBigNum> VectorAdd(const std::vector<CBigNum>& a, const std::vector<CBigNum>& b, CBigNum mod);
std::vector<CBigNum> VectorAdd(const std::vector<CBigNum>& a, const CBigNum& b);
std::vector<CBigNum> VectorSubtract(const std::vector<CBigNum>& a, const CBigNum& b);
std::vector<CBigNum> VectorScalar(const std::vector<CBigNum>& a, const CBigNum& x, CBigNum mod);
std::vector<CBigNum> VectorSlice(const std::vector<CBigNum>& a, unsigned int start, unsigned int stop);
std::vector<CBigNum> HadamardFold(std::vector<CBigNum> &vec, std::vector<CBigNum> *scale, const CBigNum &a, const CBigNum &b, const CBigNum& mod, const CBigNum& order);
CBN_vector VectorInvert(CBN_vector x, CBigNum modulus);

#endif // VECTORMATH_H
