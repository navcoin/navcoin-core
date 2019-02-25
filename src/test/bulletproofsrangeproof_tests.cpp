// Copyright (c) 2019 The NavCoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "libzerocoin/BulletproofsRangeproof.h"
#include "test/test_navcoin.h"

#include <map>

#include <boost/test/unit_test.hpp>
#include "boost/assign.hpp"

BOOST_FIXTURE_TEST_SUITE(bulletproofsrangeproof, BasicTestingSetup)

#define TUTORIAL_TEST_MODULUS   "a8852ebf7c49f01cd196e35394f3b74dd86283a07f57e0a262928e7493d4a3961d93d93c90ea3369719641d626d28b9cddc6d9307b9aabdbffc40b6d6da2e329d079b4187ff784b2893d9f53e9ab913a04ff02668114695b07d8ce877c4c8cac1b12b9beff3c51294ebe349eca41c24cd32a6d09dd1579d3947e5c4dcc30b2090b0454edb98c6336e7571db09e0fdafbd68d8f0470223836e90666a5b143b73b9cd71547c917bf24c0efc86af2eba046ed781d9acb05c80f007ef5a0a5dfca23236f37e698e8728def12554bc80f294f71c040a88eff144d130b24211016a97ce0f5fe520f477e555c9997683d762aff8bd1402ae6938dd5c994780b1bf6aa7239e9d8101630ecfeaa730d2bbc97d39beb057f016db2e28bf12fab4989c0170c2593383fd04660b5229adcd8486ba78f6cc1b558bcd92f344100dff239a8c00dbc4c2825277f24bdd04475bcc9a8c39fd895eff97c1967e434effcb9bd394e0577f4cf98c30d9e6b54cd47d6e447dcf34d67e48e4421691dbe4a7d9bd503abb9"

bool TestRange(std::vector<CBigNum> values, const libzerocoin::ZerocoinParams* params)
{
    std::vector<CBigNum> gamma;

    for (unsigned int i = 0; i < values.size(); i++)
    {
        gamma.push_back(CBigNum::randBignum(params->coinCommitmentGroup.groupOrder));
    }

    BulletproofsRangeproof bprp(&params->coinCommitmentGroup);
    bprp.Prove(values, gamma);

    std::vector<BulletproofsRangeproof> proofs;
    proofs.push_back(bprp);

    return VerifyBulletproof(&params->coinCommitmentGroup, proofs);
}

bool TestRangeBatch(std::vector<CBigNum> values, const libzerocoin::ZerocoinParams* params)
{
    std::vector<BulletproofsRangeproof> proofs;

    for (unsigned int i = 0; i < values.size(); i++)
    {
        std::vector<CBigNum> v;
        v.push_back(values[i]);

        std::vector<CBigNum> gamma;
        gamma.push_back(CBigNum::randBignum(params->coinCommitmentGroup.groupOrder));

        BulletproofsRangeproof bprp(&params->coinCommitmentGroup);
        bprp.Prove(v, gamma);

        proofs.push_back(bprp);
    }

    return VerifyBulletproof(&params->coinCommitmentGroup, proofs);
}

BOOST_AUTO_TEST_CASE(RangeProofTest)
{
    // Load a test modulus from our hardcoded string (above)
    CBigNum testModulus;
    testModulus.SetHex(std::string(TUTORIAL_TEST_MODULUS));

    libzerocoin::ZerocoinParams* params = new libzerocoin::ZerocoinParams(testModulus);

    std::vector<CBigNum> vInRange;
    std::vector<CBigNum> vOutOfRange;

    // Admited range is (0, 2**64)
    CBigNum bnLowerBound = CBigNum(0);
    CBigNum bnUpperBound = CBigNum(2).pow(64)-1;

    vInRange.push_back(bnLowerBound);
    vInRange.push_back(bnLowerBound+1);
    vInRange.push_back(bnUpperBound/2);
    vInRange.push_back(bnUpperBound);
    vOutOfRange.push_back(bnUpperBound+1);
    vOutOfRange.push_back(bnUpperBound*2);
    vOutOfRange.push_back(bnLowerBound-1 % params->coinCommitmentGroup.groupOrder);

    for (CBigNum v: vInRange)
    {
        std::vector<CBigNum> values;
        values.push_back(v);
        BOOST_CHECK(TestRange(values, params));
    }

    for (CBigNum v: vOutOfRange)
    {
        std::vector<CBigNum> values;
        values.push_back(v);
        BOOST_CHECK(!TestRange(values, params));
    }

    BOOST_CHECK(TestRangeBatch(vInRange, params));
    BOOST_CHECK(TestRange(vInRange, params));
    BOOST_CHECK(!TestRangeBatch(vOutOfRange, params));
    BOOST_CHECK(!TestRange(vOutOfRange, params));
}

BOOST_AUTO_TEST_SUITE_END()
