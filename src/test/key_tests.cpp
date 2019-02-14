// Copyright (c) 2012-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "key.h"

#include "base58.h"
#include "script/script.h"
#include "uint256.h"
#include "util.h"
#include "utilstrencodings.h"
#include "test/test_navcoin.h"

#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>

using namespace std;

static const string strSecret1     ("PHSecPt927qh8bVhLhv4Rr66JMUYWRcaQfsdPnP8nucyhE4Lhc8b");
static const string strSecret2     ("PEc7Ctgscq6fJCCBAsmi9NQr2W2dLfpraVJgYqSEhacGP4YwqaDF");
static const CNavCoinAddress addr1 ("NfWVKf7BxmvNCm8e82eegeKeHyFM2Dy2Nv");
static const CNavCoinAddress addr2 ("NMxJRcqfcgfQvzhKy42zHVSzTfrnS2HLQo");

static const string strAddressBad("1HV9Lc3sNHZxwj4Zk6fB38tEmBryq2cBiF");

#ifdef KEY_TESTS_DUMPINFO
void dumpKeyInfo(uint256 privkey)
{
    CKey key;
    key.resize(32);
    memcpy(&secret[0], &privkey, 32);
    vector<unsigned char> sec;
    sec.resize(32);
    memcpy(&sec[0], &secret[0], 32);
    printf("  * secret (hex): %s\n", HexStr(sec).c_str());

    for (int nCompressed=0; nCompressed<2; nCompressed++)
    {
        bool fCompressed = nCompressed == 1;
        printf("  * %s:\n", fCompressed ? "compressed" : "uncompressed");
        CNavCoinSecret bsecret;
        bsecret.SetSecret(secret, fCompressed);
        printf("    * secret (base58): %s\n", bsecret.ToString().c_str());
        CKey key;
        key.SetSecret(secret, fCompressed);
        vector<unsigned char> vchPubKey = key.GetPubKey();
        printf("    * pubkey (hex): %s\n", HexStr(vchPubKey).c_str());
        printf("    * address (base58): %s\n", CNavCoinAddress(vchPubKey).ToString().c_str());
    }
}
#endif


BOOST_FIXTURE_TEST_SUITE(key_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(key_test1)
{
    CNavCoinSecret bsecret1, bsecret2, bsecret1C, bsecret2C, baddress1;


    BOOST_CHECK( bsecret1.SetString (strSecret1));
    BOOST_CHECK( bsecret2.SetString (strSecret2));
    BOOST_CHECK(!baddress1.SetString(strAddressBad));

    CKey key1  = bsecret1.GetKey();
    BOOST_CHECK(key1.IsCompressed() == true);
    CKey key2  = bsecret2.GetKey();
    BOOST_CHECK(key2.IsCompressed() == true);

    CPubKey pubkey1  = key1. GetPubKey();
    CPubKey pubkey2  = key2. GetPubKey();

    BOOST_CHECK(key1.VerifyPubKey(pubkey1));
    BOOST_CHECK(!key1.VerifyPubKey(pubkey2));

    BOOST_CHECK(!key2.VerifyPubKey(pubkey1));
    BOOST_CHECK(key2.VerifyPubKey(pubkey2));

    BOOST_CHECK(addr1.Get()  == CTxDestination(pubkey1.GetID()));
    BOOST_CHECK(addr2.Get()  == CTxDestination(pubkey2.GetID()));

    for (int n=0; n<16; n++)
    {
        string strMsg = strprintf("Very secret message %i: 11", n);
        uint256 hashMsg = Hash(strMsg.begin(), strMsg.end());

        // normal signatures

        vector<unsigned char> sign1, sign2;

        BOOST_CHECK(key1.Sign (hashMsg, sign1));
        BOOST_CHECK(key2.Sign (hashMsg, sign2));

        BOOST_CHECK( pubkey1.Verify(hashMsg, sign1));
        BOOST_CHECK(!pubkey1.Verify(hashMsg, sign2));

        BOOST_CHECK(!pubkey2.Verify(hashMsg, sign1));
        BOOST_CHECK( pubkey2.Verify(hashMsg, sign2));

        // compact signatures (with key recovery)

        vector<unsigned char> csign1, csign2;

        BOOST_CHECK(key1.SignCompact (hashMsg, csign1));
        BOOST_CHECK(key2.SignCompact (hashMsg, csign2));

        CPubKey rkey1, rkey2;

        BOOST_CHECK(rkey1.RecoverCompact (hashMsg, csign1));
        BOOST_CHECK(rkey2.RecoverCompact (hashMsg, csign2));

        BOOST_CHECK(rkey1  == pubkey1);
        BOOST_CHECK(rkey2  == pubkey2);
    }
    // test EC Diffie-Hellman secret computation
    CPrivKey secret1;
    CPrivKey secret2;
    BOOST_CHECK(key1.ECDHSecret(pubkey2, secret1));
    BOOST_CHECK(key2.ECDHSecret(pubkey1, secret2));
    BOOST_CHECK(secret1.size() == 32);
    BOOST_CHECK(secret1 == secret2);

    // invalid pubkey test
    std::vector<unsigned char> pubkeydata;
    pubkeydata.insert(pubkeydata.end(), pubkey1.begin(), pubkey1.end());
    pubkeydata[0] = 0xFF;
    CPubKey pubkey1_invalid(pubkeydata);
    BOOST_CHECK(key2.ECDHSecret(pubkey1_invalid, secret2) == false);
    pubkeydata[0] = 0x03;
    CPubKey pubkey2_invalid(pubkeydata);
    BOOST_CHECK(key2.ECDHSecret(pubkey2_invalid, secret2) == true);
    pubkeydata[9] = 0xFF;
    CPubKey pubkey3_invalid(pubkeydata);
    BOOST_CHECK(key2.ECDHSecret(pubkey3_invalid, secret2) == false);

}

BOOST_AUTO_TEST_SUITE_END()
