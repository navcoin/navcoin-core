// Copyright (c) 2020 The NavCoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "types.h"
#include <utilstrencodings.h>

#include <iostream>

namespace BLSCT
{
static Scalar sm(const Scalar &y, int n, const Scalar &x)
{
    Scalar ret;
    ret = y;
    while (n--)
        ret = ret * ret;
    ret = ret * x;
    return ret;
}

Point::Point()
{
    g1_new(g1);
    g1_set_infty(g1);
    CheckRelicErrors();
}

Point Point::operator+(const Point &b) const
{
    Point ret;
    g1_add(ret.g1, this->g1, b.g1);
    CheckRelicErrors();
    return ret;
}

Point Point::operator-(const Point &b) const
{
    Point ret;
    g1_sub(ret.g1, this->g1, b.g1);
    CheckRelicErrors();
    return ret;
}

Point Point::operator*(const Scalar &b) const
{
    Point ret;
    g1_mul(ret.g1, this->g1, b.bn);
    CheckRelicErrors();
    return ret;
}

Point::Point(const Point& n)
{
    g1_new(g1);
    g1_set_infty(g1);
    g1_copy(this->g1, n.g1);
    CheckRelicErrors();
}

Point::Point(const g1_t& n)
{
    g1_new(g1);
    g1_set_infty(g1);
    g1_copy(this->g1, n);
    CheckRelicErrors();
}

Point::Point(const uint256 &b)
{
    g1_new(g1);
    g1_set_infty(g1);
    g1_map(this->g1, (unsigned char*)&b, 32);
    CheckRelicErrors();
}

bool Point::operator==(const Point &b) const
{
    return g1_cmp(this->g1, b.g1) == CMP_EQ;
}

bool Point::IsUnity() const
{
    return g1_is_infty(this->g1);
}

std::vector<uint8_t> Point::GetVch() const
{
    size_t size = g1_size_bin(this->g1, 1);
    uint8_t buffer[size];
    g1_write_bin(buffer, size, this->g1, 1);
    std::vector<uint8_t> ret(buffer, buffer+size);
    return ret;
}

void Point::SetVch(const std::vector<uint8_t> &b)
{
    g1_read_bin(this->g1, (unsigned char*)&b, b.size());
    CheckRelicErrors();
}

Scalar::Scalar()
{
    bn_new(bn);
    bn_zero(bn);
    CheckRelicErrors();
}

Scalar::Scalar(const std::vector<uint8_t> &v)
{
    bn_new(bn);
    bn_zero(bn);
    SetVch(v);
    CheckRelicErrors();
}

Scalar Scalar::operator+(const Scalar &b) const
{
    Scalar ret;
    bn_add(ret.bn, this->bn, b.bn);
    bn_t ord;
    bn_new(ord);
    g1_get_ord(ord);
    bn_mod(ret.bn, ret.bn, ord);
    CheckRelicErrors();
    return ret;
}

Scalar Scalar::operator-(const Scalar &b) const
{
    Scalar ret;
    bn_sub(ret.bn, this->bn, b.bn);
    bn_t ord;
    bn_new(ord);
    g1_get_ord(ord);
    bn_mod(ret.bn, ret.bn, ord);
    CheckRelicErrors();
    return ret;
}

Scalar Scalar::operator*(const Scalar &b) const
{
    Scalar ret;
    bn_mul(ret.bn, this->bn, b.bn);
    bn_t ord;
    bn_new(ord);
    g1_get_ord(ord);
    bn_mod(ret.bn, ret.bn, ord);
    CheckRelicErrors();
    return ret;
}

Point Scalar::operator*(const Point &b) const
{
    Point ret;
    g1_mul(ret.g1, b.g1, this->bn);
    CheckRelicErrors();
    return ret;
}

Scalar::Scalar(const uint64_t& n)
{
    bn_new(this->bn);
    bn_zero(this->bn);
    for (size_t i = 0; i < 64; i++)
    {
        bn_set_bit(this->bn, i, (n>>i)&1);
    }
    CheckRelicErrors();
}

Scalar::Scalar(const Scalar& n)
{
    bn_new(this->bn);
    bn_zero(this->bn);
    bn_copy(this->bn, n.bn);
    bn_t ord;
    bn_new(ord);
    g1_get_ord(ord);
    bn_mod(this->bn, this->bn, ord);
    CheckRelicErrors();
}

bool Scalar::operator==(const int &b) const
{
    Scalar temp;
    temp = b;
    return bn_cmp(this->bn, temp.bn) == CMP_EQ;
}

bool Scalar::operator==(const Scalar &b) const
{
    return bn_cmp(this->bn, b.bn) == CMP_EQ;
}

std::vector<uint8_t> Scalar::GetVch() const
{
    size_t size = bn_size_bin(this->bn);
    uint8_t buffer[size];
    bn_write_bin(buffer, size, this->bn);
    std::vector<uint8_t> ret(buffer, buffer+size);
    return ret;
}

Scalar Scalar::Invert() const
{
    bn_t ord;
    bn_new(ord);
    g1_get_ord(ord);
    Scalar inv;
    Scalar toinverse;
    toinverse = *this;
    fp_inv_exgcd_bn(inv.bn, toinverse.bn, ord);
    bn_mod(inv.bn, inv.bn, ord);

//    Scalar _1, _10, _100, _11, _101, _111, _1001, _1011, _1111;

//    _1 = *this;
//    _10 = _1 * _1;
//    _100 = _10 * _10;
//    _11 = _10 * _1;
//    _101 = _10 * _11;
//    _111 = _10 * _101;
//    _1001 = _10 * _111;
//    _1011 = _10 * _1001;
//    _1111 = _100 * _1011;

//    Scalar inv;
//    inv = _1111 * _1;

//    inv = sm(inv, 123 + 3, _101);
//    inv = sm(inv, 2 + 2, _11);
//    inv = sm(inv, 1 + 4, _1111);
//    inv = sm(inv, 1 + 4, _1111);
//    inv = sm(inv, 4, _1001);
//    inv = sm(inv, 2, _11);
//    inv = sm(inv, 1 + 4, _1111);
//    inv = sm(inv, 1 + 3, _101);
//    inv = sm(inv, 3 + 3, _101);
//    inv = sm(inv, 3, _111);
//    inv = sm(inv, 1 + 4, _1111);
//    inv = sm(inv, 2 + 3, _111);
//    inv = sm(inv, 2 + 2, _11);
//    inv = sm(inv, 1 + 4, _1011);
//    inv = sm(inv, 2 + 4, _1011);
//    inv = sm(inv, 6 + 4, _1001);
//    inv = sm(inv, 2 + 2, _11);
//    inv = sm(inv, 3 + 2, _11);
//    inv = sm(inv, 3 + 2, _11);
//    inv = sm(inv, 1 + 4, _1001);
//    inv = sm(inv, 1 + 3, _111);
//    inv = sm(inv, 2 + 4, _1111);
//    inv = sm(inv, 1 + 4, _1011);
//    inv = sm(inv, 3, _101);
//    inv = sm(inv, 2 + 4, _1111);
//    inv = sm(inv, 3, _101);
//    inv = sm(inv, 1 + 2, _11);

    CHECK_AND_ASSERT_THROW_MES((*this * inv) == 1, "Invert failed");

    return inv;
}

uint64_t Scalar::GetUint64() const
{
    uint64_t ret = 0;

    for (unsigned int i = 0; i < 64; i++)
    {
        ret |= bn_get_bit(this->bn, i) << i;
    }

    return ret;

}

bool Scalar::GetBit(size_t n) const
{
    return bn_get_bit(this->bn, n);
}

Scalar Scalar::Rand()
{
    bn_t ord;
    bn_new(ord);
    g1_get_ord(ord);
    bn_t ret;
    bn_rand_mod(ret, ord);
    Scalar r;
    r = ret;
    return r;
}

Scalar::Scalar(const bn_t& n)
{
    bn_new(this->bn);
    bn_zero(this->bn);
    bn_copy(this->bn, n);
    bn_t ord;
    bn_new(ord);
    g1_get_ord(ord);
    bn_mod(this->bn, this->bn, ord);
    CheckRelicErrors();
}

Scalar::Scalar(const uint256 &b)
{
    bn_new(this->bn);
    bn_zero(this->bn);
    bn_read_bin(this->bn, (unsigned char*)&b, 32);
    bn_t ord;
    bn_new(ord);
    g1_get_ord(ord);
    bn_mod(this->bn, this->bn, ord);
    CheckRelicErrors();
}

void Scalar::SetVch(const std::vector<uint8_t> &b)
{
    bn_read_bin(this->bn, (unsigned char*)&b, b.size());
    bn_t ord;
    bn_new(ord);
    g1_get_ord(ord);
    bn_mod(this->bn, this->bn, ord);
    CheckRelicErrors();
}

Scalar Scalar::Negate() const
{
    Scalar ret;
    bn_neg(ret.bn, this->bn);
    bn_t ord;
    bn_new(ord);
    g1_get_ord(ord);
    bn_mod(ret.bn, ret.bn, ord);
    return ret;
}

void Scalar::SetPow2(const int& n)
{
    bn_set_2b(this->bn, n);
}
}
