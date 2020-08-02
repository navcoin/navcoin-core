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

    BOOST_CHECK(pwalletMain->SetBLSCTDoublePublicKey(blsctDoublePublicKey(viewKey.GetPublicKey(), spendKey.GetPublicKey())));
    BOOST_CHECK(pwalletMain->SetBLSCTViewKey(blsctKey(viewKey)));
    BOOST_CHECK(pwalletMain->SetBLSCTSpendKey(blsctKey(spendKey)));
    BOOST_CHECK(pwalletMain->SetBLSCTBlindingMasterKey(blsctExtendedKey(blindingBLSKey)));

    BOOST_CHECK(pwalletMain->NewBLSCTBlindingKeyPool());
    BOOST_CHECK(pwalletMain->NewBLSCTSubAddressKeyPool(0));
    BOOST_CHECK(!pwalletMain->IsLocked());
    BOOST_CHECK(pwalletMain->TopUpBLSCTBlindingKeyPool());
    BOOST_CHECK(pwalletMain->TopUpBLSCTSubAddressKeyPool(0));

    CKeyID keyID;
    BOOST_CHECK(pwalletMain->GetBLSCTSubAddressKeyFromPool(0, keyID));

    blsctPublicKey bpk;
    blsctKey b;
    BOOST_CHECK(pwalletMain->GetBLSCTBlindingKeyFromPool(bpk));
    BOOST_CHECK(pwalletMain->GetBLSCTBlindingKey(bpk, b));

    blsctDoublePublicKey destKey;
    BOOST_CHECK(pwalletMain->GetBLSCTSubAddressPublicKeys(keyID, destKey));

    Scalar gamma;
    std::string strFailReason;
    std::vector<bls::PrependSignature> vBLSSignatures;
    CTxOut txout;

    BOOST_CHECK(CreateBLSCTOutput(b.GetKey(), txout, destKey, 10*COIN, "", gamma, strFailReason, false, vBLSSignatures));

    BOOST_CHECK(pwalletMain->IsMine(txout));

}

BOOST_AUTO_TEST_SUITE_END()
