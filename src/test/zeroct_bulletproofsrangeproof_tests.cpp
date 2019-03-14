// Copyright (c) 2019 The NavCoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "libzeroct/BulletproofsRangeproof.h"
#include "test/test_navcoin.h"

#include <map>

#include <boost/test/unit_test.hpp>
#include "boost/assign.hpp"

BOOST_FIXTURE_TEST_SUITE(bulletproofsrangeproof, BasicTestingSetup)

#define TUTORIAL_TEST_MODULUS   "a8852ebf7c49f01cd196e35394f3b74dd86283a07f57e0a262928e7493d4a3961d93d93c90ea3369719641d626d28b9cddc6d9307b9aabdbffc40b6d6da2e329d079b4187ff784b2893d9f53e9ab913a04ff02668114695b07d8ce877c4c8cac1b12b9beff3c51294ebe349eca41c24cd32a6d09dd1579d3947e5c4dcc30b2090b0454edb98c6336e7571db09e0fdafbd68d8f0470223836e90666a5b143b73b9cd71547c917bf24c0efc86af2eba046ed781d9acb05c80f007ef5a0a5dfca23236f37e698e8728def12554bc80f294f71c040a88eff144d130b24211016a97ce0f5fe520f477e555c9997683d762aff8bd1402ae6938dd5c994780b1bf6aa7239e9d8101630ecfeaa730d2bbc97d39beb057f016db2e28bf12fab4989c0170c2593383fd04660b5229adcd8486ba78f6cc1b558bcd92f344100dff239a8c00dbc4c2825277f24bdd04475bcc9a8c39fd895eff97c1967e434effcb9bd394e0577f4cf98c30d9e6b54cd47d6e447dcf34d67e48e4421691dbe4a7d9bd503abb9"

bool TestRange(CBN_vector values, const libzeroct::IntegerGroupParams* group, unsigned int logN)
{
    CBN_vector gamma;
    for (unsigned int i = 0; i < values.size(); i++)
    {
        CBigNum bnRand = CBigNum::randBignum(group->groupOrder);
        gamma.push_back(bnRand);
    }

    BulletproofsRangeproof bprp(group);

    bprp.Prove(values, gamma, logN);

    std::vector<BulletproofsRangeproof> proofs;
    proofs.push_back(bprp);

    CBN_matrix mValueCommitments;
    CBN_vector valueCommitments = bprp.GetValueCommitments();
    mValueCommitments.push_back(valueCommitments);

    return VerifyBulletproof(group, proofs, mValueCommitments, logN);
}

bool TestRangeBatch(CBN_vector values, const libzeroct::IntegerGroupParams* group, unsigned int logN)
{
    std::vector<BulletproofsRangeproof> proofs;
    CBN_matrix mValueCommitments;

    for (unsigned int i = 0; i < values.size(); i++)
    {
        CBN_vector v;
        v.push_back(values[i]);

        CBN_vector gamma;
        CBigNum bnRand = CBigNum::randBignum(group->groupOrder);
        gamma.push_back(bnRand);

        BulletproofsRangeproof bprp(group);
        bprp.Prove(v, gamma, logN);

        proofs.push_back(bprp);

        CBN_vector valueCommitments = bprp.GetValueCommitments();
        mValueCommitments.push_back(valueCommitments);
    }

    return VerifyBulletproof(group, proofs, mValueCommitments, logN);
}

BOOST_AUTO_TEST_CASE(RangeProofTest)
{
    // Load a test modulus from our hardcoded string (above)
    CBigNum testModulus;
    testModulus.SetHex(std::string(TUTORIAL_TEST_MODULUS));

    std::cout << "Running bulletproofs rangeproof test..." << std::endl;

    libzeroct::ZeroCTParams* params = new libzeroct::ZeroCTParams(testModulus);

    for (unsigned int logN = 6; logN < 9; logN++)
    {
        libzeroct::IntegerGroupParams* group = logN == 6 ? &params->coinCommitmentGroup : &params->serialNumberSoKCommitmentGroup;

        CBN_vector vInRange;
        CBN_vector vOutOfRange;

        // Admited range is [0, 2**64)
        CBigNum bnLowerBound = CBigNum(0);
        CBigNum bnUpperBound = CBigNum(2).pow(pow(2,logN))-1;

        vInRange.push_back(bnLowerBound);
        vInRange.push_back(bnLowerBound+1);
        vInRange.push_back(bnUpperBound/2);
        vInRange.push_back(bnUpperBound);
        vOutOfRange.push_back(bnUpperBound+1);
        vOutOfRange.push_back(bnUpperBound*2);
        vOutOfRange.push_back(bnUpperBound.pow_mod(-1, group->groupOrder));
        vOutOfRange.push_back(bnLowerBound-1 % group->groupOrder);

        for (CBigNum v: vInRange)
        {
            CBN_vector values;
            values.push_back(v);
            BOOST_CHECK(TestRange(values, group, logN));
        }

        for (CBigNum v: vOutOfRange)
        {
            CBN_vector values;
            values.push_back(v);
            BOOST_CHECK(!TestRange(values, group, logN));
        }

        BOOST_CHECK(TestRangeBatch(vInRange, group, logN));
        BOOST_CHECK(TestRange(vInRange, group, logN));
        BOOST_CHECK(!TestRangeBatch(vOutOfRange, group, logN));
        BOOST_CHECK(!TestRange(vOutOfRange, group, logN));

        vInRange.insert(vInRange.end(), vOutOfRange.begin(), vOutOfRange.end());

        BOOST_CHECK(!TestRangeBatch(vInRange, group, logN));
        BOOST_CHECK(!TestRange(vInRange, group, logN));
    }
}

BOOST_AUTO_TEST_SUITE_END()
