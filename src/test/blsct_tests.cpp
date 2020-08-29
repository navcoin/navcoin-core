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
#include <wallet/test/wallet_test_fixture.h>
#include <main.h>

#include <vector>
#include <map>

#include <boost/test/unit_test.hpp>

const uint32_t BIP32_HARDENED_KEY_LIMIT = 0x80000000;

BOOST_FIXTURE_TEST_SUITE(blsct_tests, WalletTestingSetup)

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

    blsctKey masterBLSKey = blsctKey(bls::BasicSchemeMPL::KeyGen(std::vector<uint8_t>(hash.begin(), hash.end())));
    blsctKey childBLSKey = blsctKey(masterBLSKey.PrivateChild(BIP32_HARDENED_KEY_LIMIT|130));
    blsctKey transactionBLSKey = blsctKey(childBLSKey.PrivateChild(BIP32_HARDENED_KEY_LIMIT));
    blsctKey blindingBLSKey = blsctKey(childBLSKey.PrivateChild(BIP32_HARDENED_KEY_LIMIT|1));
    bls::PrivateKey viewKey = blsctKey(transactionBLSKey).PrivateChild(BIP32_HARDENED_KEY_LIMIT);
    bls::PrivateKey spendKey = blsctKey(transactionBLSKey).PrivateChild(BIP32_HARDENED_KEY_LIMIT|1);
    bls::G1Element viewPublicKey = viewKey.GetG1Element();
    bls::G1Element spendPublicKey = spendKey.GetG1Element();
    blsctDoublePublicKey doubleKey(viewPublicKey, spendPublicKey);

    BOOST_CHECK(pwalletMain->SetBLSCTDoublePublicKey(doubleKey));
    BOOST_CHECK(pwalletMain->SetBLSCTViewKey(blsctKey(viewKey)));
    BOOST_CHECK(pwalletMain->SetBLSCTSpendKey(blsctKey(spendKey)));
    BOOST_CHECK(pwalletMain->SetBLSCTBlindingMasterKey(blindingBLSKey));

    BOOST_CHECK(pwalletMain->NewBLSCTBlindingKeyPool());
    BOOST_CHECK(pwalletMain->NewBLSCTSubAddressKeyPool(0));
    BOOST_CHECK(!pwalletMain->IsLocked());

    BOOST_CHECK(pwalletMain->TopUpBLSCTBlindingKeyPool());
    BOOST_CHECK(pwalletMain->TopUpBLSCTSubAddressKeyPool(0));

    CKeyID keyID;
    BOOST_CHECK(pwalletMain->GetBLSCTSubAddressKeyFromPool(0, keyID));

    blsctPublicKey bpk;
    BOOST_CHECK(pwalletMain->GetBLSCTBlindingKeyFromPool(bpk));

    blsctKey b;
    BOOST_CHECK(pwalletMain->GetBLSCTBlindingKey(bpk, b));

    blsctDoublePublicKey destKey;
    BOOST_CHECK(pwalletMain->GetBLSCTSubAddressPublicKeys(keyID, destKey));

    CMutableTransaction prevTx;

    prevTx.vout.resize(2);
    prevTx.vout[0].nValue = 10*COIN;
    prevTx.vout[0].scriptPubKey << ToByteVector(key.GetPubKey()) << OP_CHECKSIG;

    bls::PrivateKey bk = b.GetKey();
    Scalar gammaIns, gammaOuts, gammaPrevOut;
    std::string strFailReason;
    std::vector<bls::G2Element> vBLSSignatures;

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
    BOOST_CHECK(vData.size() == 1);
    BOOST_CHECK(vData[0].message == "test");
    BOOST_CHECK(vData[0].amount == 10);

    {
        Scalar diff = gammaIns-gammaOuts;
        bls::PrivateKey balanceSigningKey = bls::PrivateKey::FromByteVector(diff.GetVch());
        spendingTx.vchBalanceSig = bls::BasicSchemeMPL::Sign(balanceSigningKey, balanceMsg).Serialize();
    }

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

    {
        Scalar diff = gammaIns-gammaOuts;
        bls::PrivateKey balanceSigningKey = bls::PrivateKey::FromByteVector(diff.GetVch());
        spendingTx.vchBalanceSig = bls::BasicSchemeMPL::Sign(balanceSigningKey, balanceMsg).Serialize();
    }

    // Public to Private. Same amount. Balance signature correct. Tx signature empty.
    state = CValidationState();
    BOOST_CHECK(VerifyBLSCT(spendingTx, viewKey, vData, view, state));
    BOOST_CHECK(vData.size() == 1);
    BOOST_CHECK(vData[0].message == "testtesttesttesttest");
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

    {
        Scalar diff = gammaIns-gammaOuts;
        bls::PrivateKey balanceSigningKey = bls::PrivateKey::FromByteVector(diff.GetVch());
        spendingTx.vchBalanceSig = bls::BasicSchemeMPL::Sign(balanceSigningKey, balanceMsg).Serialize();
    }

    // Private to Private. Same amount. Balance signature correct. Tx signature empty.
    state = CValidationState();
    BOOST_CHECK(!VerifyBLSCT(spendingTx, viewKey, vData, view, state));
    BOOST_CHECK(state.GetRejectReason() == "could-not-read-blstxsig");

    spendingTx.vchTxSig = bls::BasicSchemeMPL::Aggregate(vBLSSignatures).Serialize();

    // Private to Private. Same amount. Balance signature correct. Tx signature incomplete.
    state = CValidationState();
    BOOST_CHECK(!VerifyBLSCT(spendingTx, viewKey, vData, view, state));
    BOOST_CHECK(state.GetRejectReason() == "invalid-bls-signature");

    blsctKey sk_;

    BOOST_CHECK(pwalletMain->GetBLSCTSubAddressSpendingKeyForOutput(keyID, prevTx.vout[1].outputKey, sk_));
    BOOST_CHECK(sk_.GetG1Element() == bls::G1Element::FromBytes(prevTx.vout[1].spendingKey.data()));

    SignBLSInput(sk_.GetKey(), spendingTx.vin[0], vBLSSignatures);

    spendingTx.vchTxSig = bls::BasicSchemeMPL::Aggregate(vBLSSignatures).Serialize();

    // Private to Private. Same amount. Balance signature correct. Tx signature complete.
    state = CValidationState();
    BOOST_CHECK(VerifyBLSCT(spendingTx, viewKey, vData, view, state));
    BOOST_CHECK(vData.size() == 1);
    BOOST_CHECK(vData[0].message == "test2test2test2test2tes");
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

    {
        Scalar diff = gammaIns-gammaOuts;
        bls::PrivateKey balanceSigningKey = bls::PrivateKey::FromByteVector(diff.GetVch());
        spendingTx.vchBalanceSig = bls::BasicSchemeMPL::Sign(balanceSigningKey, balanceMsg).Serialize();
    }

    // Private to Private. Different amount. Balance signature complete but incorrect due different amount. Tx signature empty.
    state = CValidationState();
    BOOST_CHECK(!VerifyBLSCT(spendingTx, viewKey, vData, view, state));
    BOOST_CHECK(state.GetRejectReason() == "invalid-balanceproof");
    spendingTx.vchTxSig = bls::BasicSchemeMPL::Aggregate(vBLSSignatures).Serialize();

    // Private to Private. Different amount. Balance signature complete but incorrect due different amount. Tx signature empty.
    state = CValidationState();
    BOOST_CHECK(!VerifyBLSCT(spendingTx, viewKey, vData, view, state));
    BOOST_CHECK(state.GetRejectReason() == "invalid-balanceproof");

    SignBLSInput(sk_.GetKey(), spendingTx.vin[0], vBLSSignatures);

    spendingTx.vchTxSig = bls::BasicSchemeMPL::Aggregate(vBLSSignatures).Serialize();

    // Private to Private. Different amount. Balance signature correct. Tx signature complete.
    state = CValidationState();
    BOOST_CHECK(!VerifyBLSCT(spendingTx, viewKey, vData, view, state));
    BOOST_CHECK(state.GetRejectReason() == "invalid-balanceproof");
    BOOST_CHECK(vData.size() == 1);
    BOOST_CHECK(vData[0].message == "test2");
    BOOST_CHECK(vData[0].amount == 10);

    spendingTx.nVersion &= ~TX_BLS_CT_FLAG;

    spendingTx.vout[0] = CTxOut();
    spendingTx.vout[0].nValue = 10*COIN;
    spendingTx.vout[0].scriptPubKey << ToByteVector(key.GetPubKey()) << OP_CHECKSIG;
    spendingTx.vout[0].ephemeralKey = bk.GetG1Element().Serialize();

    vBLSSignatures.clear();
    spendingTx.vchTxSig.clear();
    spendingTx.vchBalanceSig.clear();

    gammaIns = gammaPrevOut;
    gammaOuts = 0;

    // Private to Public. Same amount. Balance signature empty. Tx signature empty.
    state = CValidationState();
    BOOST_CHECK(!VerifyBLSCT(spendingTx, viewKey, vData, view, state));
    BOOST_CHECK(state.GetRejectReason() == "could-not-read-balanceproof");

    SignBLSInput(sk_.GetKey(), spendingTx.vin[0], vBLSSignatures);
    SignBLSOutput(bk, spendingTx.vout[0], vBLSSignatures);

    spendingTx.vchTxSig = bls::BasicSchemeMPL::Aggregate(vBLSSignatures).Serialize();

    // Private to Public. Same amount. Balance signature empty. Tx signature complete.
    state = CValidationState();
    BOOST_CHECK(!VerifyBLSCT(spendingTx, viewKey, vData, view, state));
    BOOST_CHECK(state.GetRejectReason() == "could-not-read-balanceproof");

    {
        Scalar diff = gammaIns-gammaOuts;
        bls::PrivateKey balanceSigningKey = bls::PrivateKey::FromByteVector(diff.GetVch());
        spendingTx.vchBalanceSig = bls::BasicSchemeMPL::Sign(balanceSigningKey, balanceMsg).Serialize();
    }

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
