// Copyright (c) 2020 The NavCoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <blsct/transaction.h>
#include <chainparams.h>
#include <coins.h>
#include <hash.h>
#include <key.h>
#include <keystore.h>
#include <random.h>
#include <uint256.h>
#include <wallet/test/wallet_test_fixture.h>
#include <main.h>

#include <vector>
#include <map>

#include <boost/test/unit_test.hpp>

const uint32_t BIP32_HARDENED_KEY_LIMIT = 0x80000000;

BOOST_FIXTURE_TEST_SUITE(blsct_wallet_tests, WalletTestingSetup)

BOOST_AUTO_TEST_CASE(blsct_wallet)
{
    LOCK(cs_main);

    CStateViewCache view(pcoinsTip);
    CBasicKeyStore keystore;

    blsctKey masterBLSKey = blsctKey(bls::BasicSchemeMPL::KeyGen(std::vector<uint8_t>(32, 0)));
    blsctKey childBLSKey = blsctKey(masterBLSKey.PrivateChild(BIP32_HARDENED_KEY_LIMIT|130));
    blsctKey transactionBLSKey = blsctKey(childBLSKey.PrivateChild(BIP32_HARDENED_KEY_LIMIT));
    blsctKey blindingBLSKey = blsctKey(childBLSKey.PrivateChild(BIP32_HARDENED_KEY_LIMIT|1));
    bls::PrivateKey viewKey = blsctKey(transactionBLSKey).PrivateChild(BIP32_HARDENED_KEY_LIMIT);
    bls::PrivateKey spendKey = blsctKey(transactionBLSKey).PrivateChild(BIP32_HARDENED_KEY_LIMIT|1);

    BOOST_CHECK(pwalletMain->SetBLSCTDoublePublicKey(blsctDoublePublicKey(bls::BasicSchemeMPL::SkToG1(viewKey), bls::BasicSchemeMPL::SkToG1(spendKey))));

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

    Scalar gamma;
    std::string strFailReason;
    std::vector<bls::G2Element> vBLSSignatures;
    CTxOut txout;
    BOOST_CHECK(CreateBLSCTOutput(b.GetKey(), txout, destKey, 10*COIN, "", gamma, strFailReason, false, vBLSSignatures));

    BOOST_CHECK(pwalletMain->IsMine(txout));
}

BOOST_AUTO_TEST_SUITE_END()
