// Copyright (c) 2019 The NavCoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "libzeroct/Keys.h"
#include "libzeroct/Coin.h"
#include "script/script.h"
#include "script/standard.h"
#include "key.h"
#include "zerotx.h"
#include "test/test_navcoin.h"

#include <iostream>
#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>

using namespace std;
using namespace libzeroct;

BOOST_FIXTURE_TEST_SUITE(zeroct_mint_script, BasicTestingSetup)

#define TUTORIAL_TEST_MODULUS   "a8852ebf7c49f01cd196e35394f3b74dd86283a07f57e0a262928e7493d4a3961d93d93c90ea3369719641d626d28b9cddc6d9307b9aabdbffc40b6d6da2e329d079b4187ff784b2893d9f53e9ab913a04ff02668114695b07d8ce877c4c8cac1b12b9beff3c51294ebe349eca41c24cd32a6d09dd1579d3947e5c4dcc30b2090b0454edb98c6336e7571db09e0fdafbd68d8f0470223836e90666a5b143b73b9cd71547c917bf24c0efc86af2eba046ed781d9acb05c80f007ef5a0a5dfca23236f37e698e8728def12554bc80f294f71c040a88eff144d130b24211016a97ce0f5fe520f477e555c9997683d762aff8bd1402ae6938dd5c994780b1bf6aa7239e9d8101630ecfeaa730d2bbc97d39beb057f016db2e28bf12fab4989c0170c2593383fd04660b5229adcd8486ba78f6cc1b558bcd92f344100dff239a8c00dbc4c2825277f24bdd04475bcc9a8c39fd895eff97c1967e434effcb9bd394e0577f4cf98c30d9e6b54cd47d6e447dcf34d67e48e4421691dbe4a7d9bd503abb9"

BOOST_AUTO_TEST_CASE(zeroct_mint_script)
{
    // Load a test modulus from our hardcoded string (above)
    CBigNum testModulus;
    testModulus.SetHex(std::string(TUTORIAL_TEST_MODULUS));

    ZeroCTParams* params = new ZeroCTParams(testModulus);

    CKey zk;
    ObfuscationValue oj;
    ObfuscationValue ok;
    BlindingCommitment bc;
    std::vector<unsigned char> seed;

    seed = testModulus.getvch();

    GenerateParameters(params, seed, oj, ok, bc, zk);

    CPrivateAddress ad(params, bc, zk, "TEST", COIN);
    CTxDestination dest(ad);

    CScript mintScript = GetScriptForDestination(dest);

    std::cout << "Mint script size is " << mintScript.size() << std::endl;

    CPubKey p;
    vector<unsigned char> c;
    vector<unsigned char> i;
    vector<unsigned char> a;
    vector<unsigned char> ac;

    BOOST_CHECK(mintScript.ExtractZeroCTMintData(p, c, i, a, ac));

    CBigNum cv;
    cv.setvch(c);

    CBigNum pid;
    pid.setvch(i);

    CBigNum oa;
    oa.setvch(a);

    CBigNum aco;
    aco.setvch(ac);

    PublicCoin publicCoin(params, cv, p, pid, oa, aco, true);
    PrivateCoin privateCoin(params, zk, publicCoin.getPubKey(), bc,
                            publicCoin.getValue(), publicCoin.getPaymentId(),
                            publicCoin.getAmount());

    BOOST_CHECK(privateCoin.getPublicCoin().getPubKey() == publicCoin.getPubKey());
    BOOST_CHECK(privateCoin.getPublicCoin().getPubKey() == p);
    BOOST_CHECK(privateCoin.getPublicCoin().getValue() == publicCoin.getValue());
    BOOST_CHECK(privateCoin.getPublicCoin().getCoinValue() == publicCoin.getCoinValue());
    BOOST_CHECK(privateCoin.getPublicCoin().getCoinValue() == cv);
    BOOST_CHECK(privateCoin.getPublicCoin().getAmountCommitment() == publicCoin.getAmountCommitment());
    BOOST_CHECK(privateCoin.getPublicCoin().getAmountCommitment() == aco);
    BOOST_CHECK(privateCoin.getPublicCoin().getPaymentId() == publicCoin.getPaymentId());
    BOOST_CHECK(privateCoin.getPublicCoin().getPaymentId() == pid);
    BOOST_CHECK(privateCoin.isValid());
    BOOST_CHECK(privateCoin.getAmount() == COIN);
    BOOST_CHECK(privateCoin.getPaymentId() == "TEST");
}

BOOST_AUTO_TEST_SUITE_END()
