// Copyright (c) 2019 The NavCoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "libzeroct/Coin.h"
#include "libzeroct/Keys.h"
#include "key.h"
#include "script/sign.h"
#include "zerowallet.h"
#include "zerotx.h"
#include "test/test_navcoin.h"

#include <iostream>
#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>

using namespace std;
using namespace libzeroct;

BOOST_FIXTURE_TEST_SUITE(zeroct_transaction_amount_signature, BasicTestingSetup)

#define TUTORIAL_TEST_MODULUS   "a8852ebf7c49f01cd196e35394f3b74dd86283a07f57e0a262928e7493d4a3961d93d93c90ea3369719641d626d28b9cddc6d9307b9aabdbffc40b6d6da2e329d079b4187ff784b2893d9f53e9ab913a04ff02668114695b07d8ce877c4c8cac1b12b9beff3c51294ebe349eca41c24cd32a6d09dd1579d3947e5c4dcc30b2090b0454edb98c6336e7571db09e0fdafbd68d8f0470223836e90666a5b143b73b9cd71547c917bf24c0efc86af2eba046ed781d9acb05c80f007ef5a0a5dfca23236f37e698e8728def12554bc80f294f71c040a88eff144d130b24211016a97ce0f5fe520f477e555c9997683d762aff8bd1402ae6938dd5c994780b1bf6aa7239e9d8101630ecfeaa730d2bbc97d39beb057f016db2e28bf12fab4989c0170c2593383fd04660b5229adcd8486ba78f6cc1b558bcd92f344100dff239a8c00dbc4c2825277f24bdd04475bcc9a8c39fd895eff97c1967e434effcb9bd394e0577f4cf98c30d9e6b54cd47d6e447dcf34d67e48e4421691dbe4a7d9bd503abb9"

ZeroCTParams* params;
CBigNum testModulus;

CKey zk;
ObfuscationValue oj;
ObfuscationValue ok;
BlindingCommitment bc;
std::vector<unsigned char> seed;

CBigNum gamma_sum;
CBigNum r_sum;

// Helper: create two dummy transactions, each with
// two outputs.  The first has 11 and 50 CENT outputs
// paid to a TX_PUBKEY, the second 21 and 22 CENT outputs
// paid to a ZeroCT anonymous identity.

static std::vector<CMutableTransaction>
SetupDummyInputs(CBasicKeyStore& keystoreRet, CCoinsViewCache& coinsRet)
{
    std::vector<CMutableTransaction> dummyTransactions;
    dummyTransactions.resize(2);

    // Add some keys to the keystore:
    CKey key[4];
    for (int i = 0; i < 2; i++)
    {
        key[i].MakeNewKey(i % 2);
        keystoreRet.AddKey(key[i]);
    }

    seed = testModulus.getvch();

    GenerateParameters(params, seed, oj, ok, bc, zk);

    keystoreRet.SetZeroKey(zk);
    keystoreRet.SetBlindingCommitment(bc);
    keystoreRet.SetObfuscationJ(oj);
    keystoreRet.SetObfuscationK(ok);

    // Create some dummy input transactions
    dummyTransactions[0].vout.resize(2);
    dummyTransactions[0].vout[0].nValue = 11*CENT;
    dummyTransactions[0].vout[0].scriptPubKey << ToByteVector(key[0].GetPubKey()) << OP_CHECKSIG;
    dummyTransactions[0].vout[1].nValue = 50*CENT;
    dummyTransactions[0].vout[1].scriptPubKey << ToByteVector(key[1].GetPubKey()) << OP_CHECKSIG;
    coinsRet.ModifyCoins(dummyTransactions[0].GetHash())->FromTx(dummyTransactions[0], 0);

    CPrivateAddress ad1(params, bc, zk, "TEST", 21*CENT);
    CPrivateAddress ad2(params, bc, zk, "TEST", 22*CENT);

    dummyTransactions[1].vout.resize(2);
    dummyTransactions[1].vout[0].nValue = 0;
    dummyTransactions[1].vout[0].scriptPubKey = GetScriptForDestination(ad1);
    dummyTransactions[1].vout[1].nValue = 0;
    dummyTransactions[1].vout[1].scriptPubKey = GetScriptForDestination(ad2);
    coinsRet.ModifyCoins(dummyTransactions[1].GetHash())->FromTx(dummyTransactions[1], 0);

    return dummyTransactions;
}

CMutableTransaction ConstructTransaction(CBasicKeyStore& keystoreRet, CCoinsViewCache& coinsRet, std::vector<COutPoint> prevOuts, std::vector<CAmount> valuesForPublicOutputs, std::vector<CAmount> valuesForPrivateOutputs)
{
    CMutableTransaction t;
    t.nVersion |= ZEROCT_TX_VERSION_FLAG;

    t.vin.resize(prevOuts.size());

    CCoins coin;

    r_sum = 0;

    for (unsigned int i = 0; i < prevOuts.size(); i++)
    {
        BOOST_CHECK(coinsRet.GetCoins(prevOuts[i].hash, coin));

        const CScript& scriptPubKey = coin.vout[prevOuts[i].n].scriptPubKey;

        if (scriptPubKey.IsZeroCTMint())
        {
            SignatureData sigdata;
            CScript scriptOut;
            CTransaction tx(t);
            CBigNum r;
            std::string strError = "";

            libzeroct::PublicCoin pubCoin(params);

            assert(TxOutToPublicCoin(params, coin.vout[prevOuts[i].n], pubCoin));

            TransactionSignatureCreator creator(&keystoreRet, &tx, i,
                                coin.vout[prevOuts[i].n].nValue, SIGHASH_ALL);

            libzeroct::Accumulator a(params);
            libzeroct::AccumulatorWitness aw(params, a, pubCoin);

            a.accumulate(pubCoin);

            assert(creator.CreateCoinSpendScript(params, pubCoin, a, uint256(1), aw, scriptPubKey, scriptOut, r, false, strError));

            sigdata.r = r;
            sigdata.scriptSig = scriptOut;

            t.vin[i].scriptSig = sigdata.scriptSig;

            r_sum = (r_sum + sigdata.r) % params->coinCommitmentGroup.groupOrder;
        }
        else
        {
            t.vin[i].prevout.hash = prevOuts[i].hash;
            t.vin[i].prevout.n = prevOuts[i].n;
            t.vin[i].scriptSig << std::vector<unsigned char>(65, 0);
        }
    }

    t.vout.clear();
    gamma_sum = 0;

    for (unsigned int i = 0; i < valuesForPublicOutputs.size(); i++)
    {
        t.vout.push_back(CTxOut(valuesForPublicOutputs[i], CScript(OP_RETURN)));
    }

    for (unsigned int i = 0; i < valuesForPrivateOutputs.size(); i++)
    {
        CPrivateAddress ad(params, bc, zk, "TEST", valuesForPrivateOutputs[i]);
        CTxDestination dest(ad);
        t.vout.push_back(CTxOut(0, GetScriptForDestination(dest)));
        gamma_sum = (gamma_sum + boost::get<libzeroct::CPrivateAddress>(dest).GetGamma()) % params->coinCommitmentGroup.groupOrder;
    }

    CBigNum r_minus_gamma = (r_sum - gamma_sum) % params->coinCommitmentGroup.groupOrder;

    SerialNumberProofOfKnowledge txAmountSig = SerialNumberProofOfKnowledge(&params->coinCommitmentGroup,
                                                   r_minus_gamma, CTransaction(t).GetHashAmountSig());

    CDataStream ss(SER_NETWORK, PROTOCOL_VERSION);
    ss << txAmountSig;
    t.vchTxSig = std::vector<unsigned char>(ss.begin(), ss.end());

    return t;
}

BOOST_AUTO_TEST_CASE(zeroct_transaction_amount_signature)
{
    // Load a test modulus from our hardcoded string (above)
    testModulus.SetHex(std::string(TUTORIAL_TEST_MODULUS));
    params = new ZeroCTParams(testModulus);

    CBasicKeyStore keystore;
    CCoinsView coinsDummy;
    CCoinsViewCache coins(&coinsDummy);
    std::vector<CMutableTransaction> dummyTransactions = SetupDummyInputs(keystore, coins);

    COutPoint outPublic11Cent(dummyTransactions[0].GetHash(), 0);
    COutPoint outPublic50Cent(dummyTransactions[0].GetHash(), 1);
    COutPoint outPrivate21Cent(dummyTransactions[1].GetHash(), 0);
    COutPoint outPrivate22Cent(dummyTransactions[1].GetHash(), 1);

    BOOST_CHECK(VerifyZeroCTBalance(params, ConstructTransaction(keystore, coins, {outPublic11Cent}, {10*CENT}, {1*CENT}), coins));
    BOOST_CHECK(VerifyZeroCTBalance(params, ConstructTransaction(keystore, coins, {outPublic11Cent}, {10*CENT}, {CENT/2,CENT/2}), coins));
    BOOST_CHECK(VerifyZeroCTBalance(params, ConstructTransaction(keystore, coins, {outPublic11Cent}, {0*CENT}, {3*CENT,6*CENT,2*CENT}), coins));
    BOOST_CHECK(VerifyZeroCTBalance(params, ConstructTransaction(keystore, coins, {outPublic11Cent, outPublic50Cent}, {2*CENT,2*CENT}, {50*CENT,7*CENT}), coins));
    BOOST_CHECK(VerifyZeroCTBalance(params, ConstructTransaction(keystore, coins, {outPublic11Cent, outPublic50Cent}, {60*CENT}, {1*CENT}), coins));
    BOOST_CHECK(VerifyZeroCTBalance(params, ConstructTransaction(keystore, coins, {outPublic11Cent, outPrivate21Cent}, {10*CENT}, {11*CENT,10*CENT,1*CENT}), coins));
    BOOST_CHECK(VerifyZeroCTBalance(params, ConstructTransaction(keystore, coins, {outPrivate21Cent, outPrivate22Cent}, {10*CENT}, {11*CENT,10*CENT,1*CENT,11*CENT}), coins));
    BOOST_CHECK(VerifyZeroCTBalance(params, ConstructTransaction(keystore, coins, {outPrivate22Cent}, {10*CENT}, {12*CENT}), coins));
    BOOST_CHECK(VerifyZeroCTBalance(params, ConstructTransaction(keystore, coins, {outPrivate22Cent}, {10*CENT,12*CENT}, {0*CENT}), coins));

    BOOST_CHECK(!VerifyZeroCTBalance(params, ConstructTransaction(keystore, coins, {outPublic11Cent}, {10*CENT}, {1}), coins));
    BOOST_CHECK(!VerifyZeroCTBalance(params, ConstructTransaction(keystore, coins, {outPublic11Cent}, {10*CENT}, {CENT,CENT/2,CENT/2}), coins));
    BOOST_CHECK(!VerifyZeroCTBalance(params, ConstructTransaction(keystore, coins, {outPublic11Cent}, {1*CENT}, {3*CENT,6*CENT,2*CENT}), coins));
    BOOST_CHECK(!VerifyZeroCTBalance(params, ConstructTransaction(keystore, coins, {outPublic11Cent, outPublic50Cent}, {2,2*CENT}, {50*CENT,7*CENT}), coins));
    BOOST_CHECK(!VerifyZeroCTBalance(params, ConstructTransaction(keystore, coins, {outPublic11Cent, outPublic50Cent}, {60*CENT}, {1}), coins));
    BOOST_CHECK(!VerifyZeroCTBalance(params, ConstructTransaction(keystore, coins, {outPublic11Cent, outPrivate21Cent}, {0*CENT}, {11*CENT,1*CENT}), coins));
    BOOST_CHECK(!VerifyZeroCTBalance(params, ConstructTransaction(keystore, coins, {outPrivate21Cent, outPrivate22Cent}, {10*CENT}, {11*CENT,1*CENT,1*CENT,11*CENT}), coins));
    BOOST_CHECK(!VerifyZeroCTBalance(params, ConstructTransaction(keystore, coins, {outPrivate22Cent}, {1*CENT}, {12*CENT}), coins));
    BOOST_CHECK(!VerifyZeroCTBalance(params, ConstructTransaction(keystore, coins, {outPrivate22Cent}, {10*CENT}, {0*CENT}), coins));

}

BOOST_AUTO_TEST_SUITE_END()
