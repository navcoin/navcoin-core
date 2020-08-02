// Copyright (c) 2020 The NavCoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <blsct/transaction.h>
#include <chainparams.h>
#include <coins.h>
#include <hash.h>
#include <key.h>
#include <random.h>
#include <uint256.h>
#include <test/test_navcoin.h>
#include <main.h>

#include <vector>
#include <map>

#include <boost/test/unit_test.hpp>

const uint32_t BIP32_HARDENED_KEY_LIMIT = 0x80000000;

BOOST_FIXTURE_TEST_SUITE(blsct_tests, TestingSetup)

BOOST_AUTO_TEST_CASE(blsct)
{
    LOCK(cs_main);

    CStateViewDB *pcoinsdbview = new CStateViewDB(1<<23, true);
    CStateViewCache *base = new CStateViewCache(pcoinsdbview);
    CStateViewCache view(pcoinsTip);

    CKey key;
    key.MakeNewKey(0);

    CHashWriter h(0, 0);
    std::vector<unsigned char> vKey(key.begin(), key.end());

    h << vKey;

    uint256 hash = h.GetHash();
    unsigned char h_[32];
    memcpy(h_, &hash, 32);

    bls::ExtendedPrivateKey masterBLSKey = bls::ExtendedPrivateKey::FromSeed(h_, 32);
    bls::ExtendedPrivateKey childBLSKey = masterBLSKey.PrivateChild(BIP32_HARDENED_KEY_LIMIT|130);
    bls::ExtendedPrivateKey transactionBLSKey = childBLSKey.PrivateChild(BIP32_HARDENED_KEY_LIMIT);
    bls::ExtendedPrivateKey blindingBLSKey = childBLSKey.PrivateChild(BIP32_HARDENED_KEY_LIMIT|1);
    bls::PrivateKey viewKey = transactionBLSKey.PrivateChild(BIP32_HARDENED_KEY_LIMIT).GetPrivateKey();
    bls::PrivateKey spendKey = transactionBLSKey.PrivateChild(BIP32_HARDENED_KEY_LIMIT|1).GetPrivateKey();

    blsctDoublePublicKey destKey = blsctDoublePublicKey(viewKey.GetPublicKey(), spendKey.GetPublicKey());

    CMutableTransaction prevTx;

    prevTx.vout.resize(2);
    prevTx.vout[0].nValue = 10*COIN;
    prevTx.vout[0].scriptPubKey << ToByteVector(key.GetPubKey()) << OP_CHECKSIG;

    bls::PrivateKey bk = blindingBLSKey.PrivateChild(BIP32_HARDENED_KEY_LIMIT).GetPrivateKey();
    Scalar gammaIns, gammaOuts, gammaPrevOut;
    std::string strFailReason;
    std::vector<bls::PrependSignature> vBLSSignatures;

    BOOST_CHECK(CreateBLSCTOutput(bk, prevTx.vout[1], destKey, 10*COIN, "", gammaPrevOut, strFailReason, false, vBLSSignatures));

    view.ModifyCoins(prevTx.GetHash())->FromTx(prevTx, 0);

    CMutableTransaction spendingTx;

    spendingTx.nVersion |= TX_BLS_CT_FLAG;

    spendingTx.vin.resize(1);
    spendingTx.vin[0].prevout = COutPoint(prevTx.GetHash(), 0);

    spendingTx.vout.resize(1);

    vBLSSignatures.clear();
    spendingTx.vchTxSig.clear();
    spendingTx.vchBalanceSig.clear();

    gammaIns = 0;
    gammaOuts = 0;

    BOOST_CHECK(CreateBLSCTOutput(bk, spendingTx.vout[0], destKey, 10, "test", gammaOuts, strFailReason, true, vBLSSignatures));

    std::vector<RangeproofEncodedData> vData;
    CValidationState state;

    // Public to Private. Different amount. Balance signature empty. Tx signature empty.
    BOOST_CHECK(!VerifyBLSCT(spendingTx, viewKey, vData, view, state));
    BOOST_CHECK(std::string(vData[0].message.begin(), vData[0].message.end()) == "test");
    BOOST_CHECK(vData[0].amount == 10);

    Scalar diff = gammaIns-gammaOuts;
    bls::PrivateKey balanceSigningKey = bls::PrivateKey::FromBN(diff.bn);

    spendingTx.vchBalanceSig = balanceSigningKey.Sign(balanceMsg, sizeof(balanceMsg)).Serialize();

    // Public to Private. Different amount. Balance signature correct. Tx signature empty.
    BOOST_CHECK(!VerifyBLSCT(spendingTx, viewKey, vData, view, state));

    vBLSSignatures.clear();
    spendingTx.vchTxSig.clear();
    spendingTx.vchBalanceSig.clear();

    gammaIns = 0;
    gammaOuts = 0;

    BOOST_CHECK(CreateBLSCTOutput(bk, spendingTx.vout[0], destKey, 10*COIN, "testtesttesttesttest", gammaOuts, strFailReason, true, vBLSSignatures));

    // Public to Private. Same amount. Balance signature empty. Tx signature empty.
    state = CValidationState();
    BOOST_CHECK(!VerifyBLSCT(spendingTx, viewKey, vData, view, state));

    diff = gammaIns-gammaOuts;
    balanceSigningKey = bls::PrivateKey::FromBN(diff.bn);

    spendingTx.vchBalanceSig = balanceSigningKey.Sign(balanceMsg, sizeof(balanceMsg)).Serialize();

    // Public to Private. Same amount. Balance signature correct. Tx signature empty.
    state = CValidationState();
    BOOST_CHECK(VerifyBLSCT(spendingTx, viewKey, vData, view, state));
    BOOST_CHECK(std::string(vData[0].message.begin(), vData[0].message.end()) == "testtesttesttesttest");
    BOOST_CHECK(vData[0].amount == 10*COIN);

    vBLSSignatures.clear();
    spendingTx.vchTxSig.clear();
    spendingTx.vchBalanceSig.clear();

    gammaIns = gammaPrevOut;
    gammaOuts = 0;

    spendingTx.vin[0].prevout = COutPoint(prevTx.GetHash(), 1);
    spendingTx.nVersion |= TX_BLS_INPUT_FLAG;

    bool fException = false;

    try {
        CreateBLSCTOutput(bk, spendingTx.vout[0], destKey, 10*COIN, "test2test2test2test2test2", gammaOuts, strFailReason, true, vBLSSignatures);
    }
    catch (...)
    {
        fException = true;
    }

    gammaIns = gammaPrevOut;
    gammaOuts = 0;

    BOOST_CHECK(fException);
    BOOST_CHECK(CreateBLSCTOutput(bk, spendingTx.vout[0], destKey, 10*COIN, "test2test2test2test2tes", gammaOuts, strFailReason, true, vBLSSignatures));
    // Private to Private. Same amount. Balance signature empty. Tx signature empty.
    state = CValidationState();
    BOOST_CHECK(!VerifyBLSCT(spendingTx, viewKey, vData, view, state));

    diff = gammaIns-gammaOuts;
    balanceSigningKey = bls::PrivateKey::FromBN(diff.bn);
    spendingTx.vchBalanceSig = balanceSigningKey.Sign(balanceMsg, sizeof(balanceMsg)).Serialize();

    // Private to Private. Same amount. Balance signature correct. Tx signature empty.
    state = CValidationState();
    BOOST_CHECK(!VerifyBLSCT(spendingTx, viewKey, vData, view, state));
    BOOST_CHECK(state.GetRejectReason() == "could-not-read-blstxsig");

    spendingTx.vchTxSig = bls::PrependSignature::Aggregate(vBLSSignatures).Serialize();

    // Private to Private. Same amount. Balance signature correct. Tx signature incomplete.
    state = CValidationState();
    BOOST_CHECK(!VerifyBLSCT(spendingTx, viewKey, vData, view, state));
    BOOST_CHECK(state.GetRejectReason() == "invalid-bls-signature");

    std::vector<bls::PrivateKey> keys;
    keys.push_back(spendKey);
    keys.push_back(bls::PrivateKey::FromBN(Scalar(Point(bls::BLS::DHKeyExchange(viewKey, bk.GetPublicKey())).Hash(0)).bn));
    bls::PrivateKey sk_  = bls::PrivateKey::AggregateInsecure(keys);

    BOOST_CHECK(sk_.GetPublicKey() == bls::PublicKey::FromBytes(prevTx.vout[1].spendingKey.data()));

    SignBLSInput(sk_, spendingTx.vin[0], vBLSSignatures);

    spendingTx.vchTxSig = bls::PrependSignature::Aggregate(vBLSSignatures).Serialize();

    // Private to Private. Same amount. Balance signature correct. Tx signature complete.
    state = CValidationState();
    BOOST_CHECK(VerifyBLSCT(spendingTx, viewKey, vData, view, state));
    BOOST_CHECK(std::string(vData[0].message.begin(), vData[0].message.end()) == "test2test2test2test2tes");
    BOOST_CHECK(vData[0].amount == 10*COIN);

    vBLSSignatures.clear();
    spendingTx.vchTxSig.clear();
    spendingTx.vchBalanceSig.clear();

    gammaIns = gammaPrevOut;
    gammaOuts = 0;

    spendingTx.vin[0].prevout = COutPoint(prevTx.GetHash(), 1);
    spendingTx.nVersion |= TX_BLS_INPUT_FLAG;

    BOOST_CHECK(CreateBLSCTOutput(bk, spendingTx.vout[0], destKey, 10, "test2", gammaOuts, strFailReason, true, vBLSSignatures));

    // Private to Private. Different amount. Balance signature empty. Tx signature empty.
    state = CValidationState();
    BOOST_CHECK(!VerifyBLSCT(spendingTx, viewKey, vData, view, state));
    BOOST_CHECK(state.GetRejectReason() == "could-not-read-balanceproof");

    diff = gammaIns-gammaOuts;
    balanceSigningKey = bls::PrivateKey::FromBN(diff.bn);
    spendingTx.vchBalanceSig = balanceSigningKey.Sign(balanceMsg, sizeof(balanceMsg)).Serialize();

    // Private to Private. Different amount. Balance signature complete but incorrect due different amount. Tx signature empty.
    state = CValidationState();
    BOOST_CHECK(!VerifyBLSCT(spendingTx, viewKey, vData, view, state));
    BOOST_CHECK(state.GetRejectReason() == "invalid-balanceproof");
    spendingTx.vchTxSig = bls::PrependSignature::Aggregate(vBLSSignatures).Serialize();

    // Private to Private. Different amount. Balance signature complete but incorrect due different amount. Tx signature empty.
    state = CValidationState();
    BOOST_CHECK(!VerifyBLSCT(spendingTx, viewKey, vData, view, state));
    BOOST_CHECK(state.GetRejectReason() == "invalid-balanceproof");

    SignBLSInput(sk_, spendingTx.vin[0], vBLSSignatures);

    spendingTx.vchTxSig = bls::PrependSignature::Aggregate(vBLSSignatures).Serialize();

    // Private to Private. Different amount. Balance signature correct. Tx signature complete.
    state = CValidationState();
    BOOST_CHECK(!VerifyBLSCT(spendingTx, viewKey, vData, view, state));
    BOOST_CHECK(state.GetRejectReason() == "invalid-balanceproof");
    BOOST_CHECK(std::string(vData[0].message.begin(), vData[0].message.end()) == "test2");
    BOOST_CHECK(vData[0].amount == 10);

    spendingTx.nVersion &= ~TX_BLS_CT_FLAG;

    spendingTx.vout[0] = CTxOut();
    spendingTx.vout[0].nValue = 10*COIN;
    spendingTx.vout[0].scriptPubKey << ToByteVector(key.GetPubKey()) << OP_CHECKSIG;
    spendingTx.vout[0].ephemeralKey = bk.GetPublicKey().Serialize();

    vBLSSignatures.clear();
    spendingTx.vchTxSig.clear();
    spendingTx.vchBalanceSig.clear();

    gammaIns = gammaPrevOut;
    gammaOuts = 0;

    // Private to Public. Same amount. Balance signature empty. Tx signature empty.
    state = CValidationState();
    BOOST_CHECK(!VerifyBLSCT(spendingTx, viewKey, vData, view, state));
    BOOST_CHECK(state.GetRejectReason() == "could-not-read-balanceproof");

    SignBLSInput(sk_, spendingTx.vin[0], vBLSSignatures);
    SignBLSOutput(bk, spendingTx.vout[0], vBLSSignatures);

    spendingTx.vchTxSig = bls::PrependSignature::Aggregate(vBLSSignatures).Serialize();

    // Private to Public. Same amount. Balance signature empty. Tx signature complete.
    state = CValidationState();
    BOOST_CHECK(!VerifyBLSCT(spendingTx, viewKey, vData, view, state));
    BOOST_CHECK(state.GetRejectReason() == "could-not-read-balanceproof");

    diff = gammaIns-gammaOuts;
    balanceSigningKey = bls::PrivateKey::FromBN(diff.bn);

    spendingTx.vchBalanceSig = balanceSigningKey.Sign(balanceMsg, sizeof(balanceMsg)).Serialize();

    // Private to Public. Same amount. Balance signature correct. Tx signature complete.
    state = CValidationState();

    BOOST_CHECK(VerifyBLSCT(spendingTx, viewKey, vData, view, state));
    spendingTx.vout[0].nValue = 10;

    // Private to Public. Different amount. Balance signature correct. Tx signature complete.
    state = CValidationState();
    BOOST_CHECK(!VerifyBLSCT(spendingTx, viewKey, vData, view, state));
    BOOST_CHECK(state.GetRejectReason() == "invalid-balanceproof");
}

BOOST_AUTO_TEST_SUITE_END()
