/**
* @file       Math.h
*
* @brief      Diverse vector operations
*             Inspired by https://github.com/b-g-goodell/research-lab/blob/master/source-code/StringCT-java/src/how/monero/hodl/bulletproof/Bulletproof.java
*
* @author     alex v
* @date       December 2018
*
* @copyright  Copyright 2018 alex v
* @license    This project is released under the MIT license.
**/
// Copyright (c) 2018 The NavCoin Core developers

#ifndef VECTORMATH_H
#define VECTORMATH_H

#include "bignum.h"

bool VectorExponent(CBigNum a, std::vector<CBigNum> a_exp, CBigNum b, std::vector<CBigNum> b_exp, CBigNum &out)
{
    if (a_exp.size() != b_exp.size())
        return false;

    out.Nullify();
    for (unsigned int i = 0; i < a_exp.size(); i++)
    {
        out *= a.pow(a_exp[i]);
        out *= b.pow(b_exp[i]);
    }
    return true;
}

/* Compute a custom vector-scalar commitment */
bool VectorVectorExponent(std::vector<CBigNum> a, std::vector<CBigNum> a_exp, std::vector<CBigNum> b, std::vector<CBigNum> b_exp, CBigNum &out)
{
    if (!(a.size() == b.size() && a_exp.size() == b_exp.size() && a.size() == b_exp.size()))
        return false;

    out.Nullify();
    for (unsigned int i = 0; i < a.size(); i++)
    {
        out *= a[i].pow(a_exp[i]);
        out *= b[i].pow(b_exp[i]);
    }
    return true;
}

/* Compute a custom vector-scalar commitment mod */
bool VectorVectorExponentMod(std::vector<CBigNum> a, std::vector<CBigNum> a_exp, std::vector<CBigNum> b, std::vector<CBigNum> b_exp, CBigNum mod, CBigNum &out)
{
    if (!(a.size() == b.size() && a_exp.size() == b_exp.size() && a.size() == b_exp.size()))
        return false;

    out.Nullify();
    for (unsigned int i = 0; i < a.size(); i++)
    {
        out *= a[i].pow_mod(a_exp[i], mod);
        out *= b[i].pow_mod(b_exp[i], mod);
    }
    return true;
}


/* Given a scalar, construct a vector of powers */
bool VectorPowers(CBigNum x, unsigned int size, std::vector<CBigNum>& out)
{
    out.clear();
    out.resize(size);

    std::vector<CBigNum> temp(size);

    for (unsigned int i = 0; i < size; i++)
    {
        temp[i] = x.pow(i);
    }

    out = temp;

    return true;
}

/* Given two scalar arrays, construct the inner product */
bool InnerProduct(std::vector<CBigNum> a, std::vector<CBigNum> b, CBigNum& out)
{
    if (a.size() != b.size())
        return false;

    out.Nullify();
    for (unsigned int i = 0; i < a.size(); i++)
    {
        out += a[i] * b[i];
    }
    return true;
}

/* Given two scalar arrays, construct the Hadamard product */
bool Hadamard(std::vector<CBigNum> a, std::vector<CBigNum> b, std::vector<CBigNum>& out)
{
    if (a.size() != b.size())
        return false;

    out.clear();
    out.resize(a.size());

    std::vector<CBigNum> temp(a.size());

    for (unsigned int i = 0; i < a.size(); i++)
    {
        temp[i] = a[i] * b[i];
    }

    out = temp;

    return true;
}

/* Add two vectors */
bool VectorAdd(std::vector<CBigNum> a, std::vector<CBigNum> b, std::vector<CBigNum>& out)
{
    if (a.size() != b.size())
        return false;

    out.clear();
    out.resize(a.size());

    std::vector<CBigNum> temp(a.size());

    for (unsigned int i = 0; i < a.size(); i++)
    {
        temp[i] = a[i] + b[i];
    }

    out = temp;

    return true;
}

/* Subtract two vectors */
bool VectorSubtract(std::vector<CBigNum> a, std::vector<CBigNum> b, std::vector<CBigNum>& out)
{
    if (a.size() != b.size())
        return false;

    out.clear();
    out.resize(a.size());

    std::vector<CBigNum> temp(a.size());

    for (unsigned int i = 0; i < a.size(); i++)
    {
        temp[i] = a[i] - b[i];
    }

    out = temp;

    return true;
}

/* Multiply a scalar and a vector */
bool VectorScalar(std::vector<CBigNum> a, CBigNum x, std::vector<CBigNum>& out)
{
    std::vector<CBigNum> temp(out.size());

    for (unsigned int i = 0; i < a.size(); i++)
    {
        temp[i] = a[i] * x;
    }

    out = temp;

    return true;
}

/* Exponentiate a vector by a scalar */
bool VectorScalarExp(std::vector<CBigNum> a, CBigNum x, std::vector<CBigNum>& out)
{
    std::vector<CBigNum> temp(a.size());

    for (unsigned int i = 0; i < a.size(); i++)
    {
        temp[i] = a[i].pow(x);
    }

    out = temp;

    return true;
}

/* Compute the slice of a scalar vector */
bool VectorSlice(std::vector<CBigNum> a, unsigned int start, unsigned int stop, std::vector<CBigNum>& out)
{
    if (start > a.size() || stop > a.size())
        return false;

    out.clear();
    out.resize(stop-start);

    std::vector<CBigNum> temp(a.size());

    for (unsigned int i = start; i < stop; i++)
    {
        out[i-start] = a[i];
    }

    out = temp;

    return true;
}
#endif // VECTORMATH_H
