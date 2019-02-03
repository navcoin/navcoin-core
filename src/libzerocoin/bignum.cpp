// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2012 The Bitcoin developers
// Copyright (c) 2017-2018 The PIVX developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "bignum.h"

#if defined(USE_NUM_GMP)
#include "bignum_gmp.cpp"
#endif

#if defined(USE_NUM_OPENSSL)
#include "bignum_openssl.cpp"
#endif

std::string CBigNum::GetHex() const
{
    return ToString(16);
}

std::string CBigNum::GetDec() const
{
    return ToString(10);
}

CBigNum CBigNum::pow(const int e) const
{
    return this->pow(CBigNum(e));
}

void CBigNum::SetHex(const std::string& str)
{
    SetHexBool(str);
}

CBigNum& CBigNum::operator/=(const CBigNum& b)
{
    *this = *this / b;
    return *this;
}

CBigNum& CBigNum::operator%=(const CBigNum& b)
{
    *this = *this % b;
    return *this;
}

const CBigNum CBigNum::operator++(int)
{
    // postfix operator
    const CBigNum ret = *this;
    ++(*this);
    return ret;
}

const CBigNum CBigNum::operator--(int)
{
    // postfix operator
    const CBigNum ret = *this;
    --(*this);
    return ret;
}
