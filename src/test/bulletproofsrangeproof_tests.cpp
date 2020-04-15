// Copyright (c) 2020 The NavCoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "blsct/bulletproofs.h"
#include "test/test_navcoin.h"

#include <map>

#include <boost/test/unit_test.hpp>
#include "boost/assign.hpp"

BOOST_FIXTURE_TEST_SUITE(bulletproofsrangeproof, BasicTestingSetup)

using namespace BLSCT;

bool TestRange(std::vector<Scalar> values)
{
    std::vector<Scalar> gamma;

    for (unsigned int i = 0; i < values.size(); i++)
    {
        gamma.push_back(Scalar::Rand());
    }

    BulletproofsRangeproof bprp;
    bprp.Prove(values, gamma);

    std::vector<BulletproofsRangeproof> proofs;
    proofs.push_back(bprp);

    return VerifyBulletproof(proofs);
}

bool TestRangeBatch(std::vector<Scalar> values)
{
    std::vector<BulletproofsRangeproof> proofs;

    for (unsigned int i = 0; i < values.size(); i++)
    {
        std::vector<Scalar> v;
        v.push_back(values[i]);

        std::vector<Scalar> gamma;
        gamma.push_back(Scalar::Rand());

        BulletproofsRangeproof bprp;
        bprp.Prove(v, gamma);

        proofs.push_back(bprp);
    }

    return VerifyBulletproof(proofs);
}

BOOST_AUTO_TEST_CASE(RangeProofTest)
{
    std::vector<Scalar> vInRange;
    std::vector<Scalar> vOutOfRange;

    // Admited range is (0, 2**64)
    Scalar one;
    one = 1;
    Scalar two;
    two = 2;
    Scalar twoPow64;
    twoPow64.SetPow2(64);
    Scalar bnLowerBound = 0;
    Scalar bnUpperBound = twoPow64-one;

    vInRange.push_back(bnLowerBound);
    vInRange.push_back(bnLowerBound+one);
    vInRange.push_back(bnUpperBound-one);
    vInRange.push_back(bnUpperBound);
    vOutOfRange.push_back(bnUpperBound+one);
    vOutOfRange.push_back(bnUpperBound+one+one);
    vOutOfRange.push_back(bnUpperBound*two);
    vOutOfRange.push_back(one.Negate());

    for (Scalar v: vInRange)
    {
        std::vector<Scalar> values;
        bn_print(v.bn);
        values.push_back(v);
        BOOST_CHECK(TestRange(values));
    }

    for (Scalar v: vOutOfRange)
    {
        std::vector<Scalar> values;
        values.push_back(v);
        BOOST_CHECK(!TestRange(values));
    }

    BOOST_CHECK(TestRangeBatch(vInRange));
    BOOST_CHECK(TestRange(vInRange));
    BOOST_CHECK(!TestRangeBatch(vOutOfRange));
    BOOST_CHECK(!TestRange(vOutOfRange));
}

BOOST_AUTO_TEST_SUITE_END()
