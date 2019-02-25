/**
* @file       Math.cpp
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

#include "Math.h"

// Todo multi-exp optimization
CBigNum MultiExp(std::vector<MultiexpData> multiexp_data, const libzerocoin::IntegerGroupParams* params)
{
    CBigNum result = 1;

    for (MultiexpData& it: multiexp_data)
    {
        //std::cout << it.base.ToString(16).substr(0,8) << "^" << it.exp.ToString(16).substr(0,8) << std::endl;
        result = result.mul_mod(it.base.pow_mod(it.exp % params->groupOrder, params->modulus), params->modulus);
    }

    return result;
}

CBigNum VectorPowerSum(const CBigNum &x, unsigned int n)
{
    if (n < 0)
        throw std::runtime_error("VectorPowerSum(): n can't be negative");

    if (n == 0)
        return 0;

    if (n == 1)
        return 1;

    CBigNum prev = x;
    CBigNum out = 1;

    for (size_t i = 1; i < n; ++i)
    {
        if (i > 1)
            prev = prev * x;
        out = out + prev;
    }

    return out;
}


std::vector<CBigNum> VectorExponent(const std::vector<CBigNum>& a, const CBigNum& a_exp, CBigNum mod)
{
    std::vector<CBigNum> ret(a.size());

    for (unsigned int i = 0; i < a.size(); i++)
    {
        ret[i] = a[i].pow_mod(a_exp, mod);
    }

    return ret;
}

/* Compute a custom vector-scalar commitment mod */
CBigNum VectorExponent2Mod(const std::vector<CBigNum>& a, const std::vector<CBigNum>& a_exp, const std::vector<CBigNum>& b, const std::vector<CBigNum>& b_exp, CBigNum mod)
{
    if (!(a.size() == b.size() && b.size() >= b_exp.size() && a.size() >= a_exp.size()))
        throw std::runtime_error("VectorExponent2Mod(): a, b, a_exp and b_exp should be of the same size");

    CBigNum ret = 1;

    for (unsigned int i = 0; i < a_exp.size(); i++)
    {
        ret = ret.mul_mod(a[i].pow_mod(a_exp[i], mod), mod);
        ret = ret.mul_mod(b[i].pow_mod(b_exp[i], mod), mod);
    }
    return ret;
}

/* Given a scalar, construct a vector of powers */
std::vector<CBigNum> VectorPowers(CBigNum x, unsigned int size)
{
    std::vector<CBigNum> temp(size);

    for (unsigned int i = 0; i < size; i++)
    {
        temp[i] = x.pow(i);
    }

    return temp;
}

/* Given two scalar arrays, construct the inner product */
CBigNum InnerProduct(const std::vector<CBigNum>& a, const std::vector<CBigNum>& b, CBigNum mod)
{
    if (a.size() != b.size())
        throw std::runtime_error("InnerProduct(): a and b should be of the same size");

    CBigNum ret;

    for (unsigned int i = 0; i < a.size(); i++)
    {
        ret = (ret + a[i].mul_mod(b[i], mod)) % mod;
    }

    return ret;
}

CBigNum InnerProduct(const std::vector<CBigNum>& a, const std::vector<CBigNum>& b)
{
    if (a.size() != b.size())
        throw std::runtime_error("InnerProduct(): a and b should be of the same size");

    CBigNum ret;

    for (unsigned int i = 0; i < a.size(); i++)
    {
        ret = ret + a[i] * b[i];
    }

    return ret;
}

/* Given two scalar arrays, construct the Hadamard product */
std::vector<CBigNum> Hadamard(const std::vector<CBigNum>& a, const std::vector<CBigNum>& b, CBigNum mod)
{
    if (a.size() != b.size())
        throw std::runtime_error("Hadamard(): a and b should be of the same size");

    std::vector<CBigNum> ret(a.size());

    for (unsigned int i = 0; i < a.size(); i++)
    {
        ret[i] = a[i].mul_mod(b[i], mod);
    }

    return ret;
}

/* Add two vectors */
std::vector<CBigNum> VectorAdd(const std::vector<CBigNum>& a, const std::vector<CBigNum>& b, CBigNum mod)
{
    if (a.size() != b.size())
        throw std::runtime_error("VectorAdd(): a and b should be of the same size");

    std::vector<CBigNum> ret(a.size());

    for (unsigned int i = 0; i < a.size(); i++)
    {
        ret[i] = (a[i] + b[i]) % mod;
    }

    return ret;
}

std::vector<CBigNum> VectorAdd(const std::vector<CBigNum>& a, const CBigNum& b)
{
    std::vector<CBigNum> ret(a.size());

    for (unsigned int i = 0; i < a.size(); i++)
    {
        ret[i] = a[i] + b;
    }

    return ret;
}

/* Subtract two vectors */
std::vector<CBigNum> VectorSubtract(const std::vector<CBigNum>& a, const CBigNum& b)
{

    std::vector<CBigNum> ret(a.size());

    for (unsigned int i = 0; i < a.size(); i++)
    {
        ret[i] = a[i] - b;
    }

    return ret;
}

/* Multiply a scalar and a vector */
std::vector<CBigNum> VectorScalar(const std::vector<CBigNum>& a, const CBigNum& x, CBigNum mod)
{
    std::vector<CBigNum> ret(a.size());

    for (unsigned int i = 0; i < a.size(); i++)
    {
        ret[i] = a[i].mul_mod(x, mod);
    }

    return ret;
}

/* Compute the slice of a scalar vector */
std::vector<CBigNum> VectorSlice(const std::vector<CBigNum>& a, unsigned int start, unsigned int stop)
{
    if (start > a.size() || stop > a.size())
        throw std::runtime_error("VectorSlice(): wrong start or stop point");

    std::vector<CBigNum> ret(stop-start);

    for (unsigned int i = start; i < stop; i++)
    {
        ret[i-start] = a[i];
    }

    return ret;
}

std::vector<CBigNum> HadamardFold(std::vector<CBigNum> &vec, std::vector<CBigNum> *scale, const CBigNum &a, const CBigNum &b, const CBigNum& mod, const CBigNum& order)
{
    if(!((vec.size() & 1) == 0))
        throw std::runtime_error("HadamardFold(): vector argument size is not even");

    const size_t sz = vec.size() / 2;
    std::vector<CBigNum> out(sz);

    for (size_t n = 0; n < sz; ++n)
    {
        CBigNum c0 = vec[n];
        CBigNum c1 = vec[sz + n];
        CBigNum sa, sb;
        if (scale) sa = a.mul_mod((*scale)[n], order); else sa = a;
        if (scale) sb = b.mul_mod((*scale)[sz + n], order); else sb = b;
        out[n] = c0.pow_mod(sa, mod).mul_mod(c1.pow_mod(sb, mod), mod);
    }

    return out;
}

CBN_vector VectorInvert(CBN_vector x, CBigNum modulus)
{
    CBN_vector scratch;
    scratch.reserve(x.size());

    CBigNum acc = CBigNum(1);
    for (size_t n = 0; n < x.size(); ++n)
    {
        scratch.push_back(acc);
        if (n == 0)
            acc = x[0];
        else
            acc = acc.mul_mod(x[n], modulus);
    }

    acc = acc.inverse(modulus);

    CBigNum tmp;
    for (int i = x.size(); i-- > 0; )
    {
        tmp = acc.mul_mod(x[i], modulus);
        x[i] = acc.mul_mod(scratch[i], modulus);
        acc = tmp;
    }

    return x;
}
