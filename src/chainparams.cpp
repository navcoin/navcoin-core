// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "chainparams.h"
#include "consensus/merkle.h"

#include "tinyformat.h"
#include "util.h"
#include "utilstrencodings.h"
#include "streams.h"

#include <assert.h>

#include <boost/assign/list_of.hpp>

#include "chainparamsseeds.h"

static CBlock CreateGenesisBlock(const char* pszTimestamp, const CScript& genesisOutputScript, uint32_t nTime, uint32_t nNonce, uint32_t nBits, int32_t nVersion, const CAmount& genesisReward)
{
    CMutableTransaction txNew;
    txNew.nVersion = 1;
    txNew.vin.resize(1);
    txNew.vout.resize(1);
    txNew.vin[0].scriptSig = CScript() << CScriptNum(0) << CScriptNum(42) << std::vector<unsigned char>((const unsigned char*)pszTimestamp, (const unsigned char*)pszTimestamp + strlen(pszTimestamp));
    // txNew.vout[0].scriptPubKey.clear();
    txNew.vout[0].nValue = 0;
    txNew.vout[0].scriptPubKey.clear();
    txNew.strDZeel = "NavCoin genesis block";

    CBlock genesis;
    genesis.nTime    = 1460561040;
    genesis.nBits    = 0x1f00ffff;
    genesis.nNonce   = 6945;
    genesis.nVersion = nVersion;
    genesis.vtx.push_back(txNew);
    genesis.vtx[0].nTime = 1460561040;
    genesis.vtx[0].UpdateHash();
    genesis.hashPrevBlock.SetNull();
    genesis.hashMerkleRoot = BlockMerkleRoot(genesis);
    return genesis;
}

/**
 * Build the genesis block. Note that the output of its generation
 * transaction cannot be spent since it did not originally exist in the
 * database.
 *
 * CBlock(hash=000000000019d6, ver=1, hashPrevBlock=00000000000000, hashMerkleRoot=4a5e1e, nTime=1231006505, nBits=1d00ffff, nNonce=2083236893, vtx=1)
 *   CTransaction(hash=4a5e1e, ver=1, vin.size=1, vout.size=1, nLockTime=0)
 *     CTxIn(COutPoint(000000, -1), coinbase 04ffff001d0104455468652054696d65732030332f4a616e2f32303039204368616e63656c6c6f72206f6e206272696e6b206f66207365636f6e64206261696c6f757420666f722062616e6b73)
 *     CTxOut(nValue=50.00000000, scriptPubKey=0x5F1DF16B2B704C8A578D0B)
 *   vMerkleTree: 4a5e1e
 */
static CBlock CreateGenesisBlock(uint32_t nTime, uint32_t nNonce, uint32_t nBits, int32_t nVersion, const CAmount& genesisReward)
{
    const char* pszTimestamp = "Game is afoot!";
    const CScript genesisOutputScript = CScript() << ParseHex("04bf5608f13e9b2781b839ea78adbd1cb90d8fc17dcc67028e93e65223ea77f8bc8d8eed1191f37dd0ad20f371912d86e1c2e7369251cb06d2a3fdc5e26262d6df") << OP_CHECKSIG;
    return CreateGenesisBlock(pszTimestamp, genesisOutputScript, nTime, nNonce, nBits, nVersion, genesisReward);
}

/**
 * Main network
 */
/**
 * What makes a good checkpoint block?
 * + Is surrounded by blocks with reasonable timestamps
 *   (no blocks before with a timestamp after, none after with
 *    timestamp before)
 * + Contains no strange transactions
 */

class CMainParams : public CChainParams {
public:
    CMainParams() {
        strNetworkID = "main";
        consensus.nSubsidyHalvingInterval = 210000;
        consensus.nMajorityEnforceBlockUpgrade = 750;
        consensus.nMajorityRejectBlockOutdated = 950;
        consensus.nMajorityWindow = 1000;
        consensus.BIP34Height = 900000;
        consensus.BIP34Hash = uint256S("0x000000000000024b89b42a942fe0d9fea3bb44ab7bd1b19115dd6a759c0808b8");
        consensus.powLimit = ArithToUint256(~arith_uint256(0) >> 16);
        consensus.nPowTargetTimespan = 30; // two weeks
        consensus.nPowTargetSpacing = 30;
        consensus.fPowAllowMinDifficultyBlocks = false;
        consensus.fPowNoRetargeting = false;
        consensus.nRuleChangeActivationThreshold = 1916; // 95% of 2016
        consensus.nMinerConfirmationWindow = 1; // nPowTargetTimespan / nPowTargetSpacing
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = 1199145601; // January 1, 2008
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = 1230767999; // December 31, 2008

        // Deployment of BIP68, BIP112, and BIP113.
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].bit = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nStartTime = 1462060800; // May 1st, 2016
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nTimeout = 1493596800; // May 1st, 2017

        // Deployment of SegWit (BIP141 and BIP143)
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].bit = 1;
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].nStartTime = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].nTimeout = 0; // Never / undefined

        /**
         * The message start string is designed to be unlikely to occur in normal data.
         * The characters are rarely used upper ASCII, not valid as UTF-8, and produce
         * a large 32-bit integer with any alignment.
         */
        pchMessageStart[0] = 0x80;
        pchMessageStart[1] = 0x50;
        pchMessageStart[2] = 0x34;
        pchMessageStart[3] = 0x20;
        nDefaultPort = 44440;
        nPruneAfterHeight = 100000;
        bnProofOfWorkLimit = arith_uint256(~arith_uint256() >> 16);

        genesis = CreateGenesisBlock(1460161040, 6945, 0, 1, 50 * COIN);

	      uint256 hashGenesisBlock = uint256S("0x00006a4e3e18c71c6d48ad6c261e2254fa764cf29607a4357c99b712dfbb8e6a");

	      // Change to true to enable genesis block creation

	     if (false && genesis.GetHash() != hashGenesisBlock)
        {
            printf("recalculating params for mainnet.\n");
            printf("old mainnet genesis nonce: %d\n", genesis.nNonce);
            printf("old mainnet genesis hash:  %s\n", hashGenesisBlock.ToString().c_str());
            // deliberately empty for loop finds nonce value.
            for(; genesis.GetHash() < consensus.powLimit; genesis.nNonce++){ }
            LogPrintf("new mainnet genesis merkle root: %s\n", genesis.hashMerkleRoot.ToString().c_str());
            LogPrintf("new mainnet genesis nonce: %d\n", genesis.nNonce);
            LogPrintf("new mainnet genesis hash: %s\n", genesis.GetHash().ToString().c_str());
        }

	      consensus.hashGenesisBlock = genesis.GetHash();

        assert(consensus.hashGenesisBlock == uint256S("0x00006a4e3e18c71c6d48ad6c261e2254fa764cf29607a4357c99b712dfbb8e6a"));
        assert(genesis.hashMerkleRoot == uint256S("0xc507eec6ccabfd5432d764afceafba42d2d946594b8a60570cb2358a7392c61a"));

        /*vSeeds.push_back(CDNSSeedData("navcoin.sipa.be", "seed.navcoin.sipa.be", true)); // Pieter Wuille
        vSeeds.push_back(CDNSSeedData("bluematt.me", "dnsseed.bluematt.me")); // Matt Corallo
        vSeeds.push_back(CDNSSeedData("dashjr.org", "dnsseed.navcoin.dashjr.org")); // Luke Dashjr
        vSeeds.push_back(CDNSSeedData("navcoinstats.com", "seed.navcoinstats.com")); // Christian Decker
        vSeeds.push_back(CDNSSeedData("xf2.org", "bitseed.xf2.org")); // Jeff Garzik
        vSeeds.push_back(CDNSSeedData("navcoin.jonasschnelli.ch", "seed.navcoin.jonasschnelli.ch")); // Jonas Schnelli*/

        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,53);
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,85);
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,150);
        base58Prefixes[EXT_PUBLIC_KEY] = boost::assign::list_of(0x04)(0x88)(0xB2)(0x1E).convert_to_container<std::vector<unsigned char> >();
        base58Prefixes[EXT_SECRET_KEY] = boost::assign::list_of(0x04)(0x88)(0xAD)(0xE4).convert_to_container<std::vector<unsigned char> >();

        vFixedSeeds.clear();
      	vSeeds.clear();

        vSeeds.push_back(CDNSSeedData("supernode.navcoin.org", "95.183.51.56"));

        vFixedSeeds = std::vector<SeedSpec6>(pnSeed6_main, pnSeed6_main + ARRAYLEN(pnSeed6_main));

        fMiningRequiresPeers = true;
        fDefaultConsistencyChecks = false;
        fRequireStandard = true;
        fMineBlocksOnDemand = false;
        fTestnetToBeDeprecatedFieldRPC = false;

        checkpointData = (CCheckpointData) {
            boost::assign::map_list_of
            ( 0, uint256S("0x00006a4e3e18c71c6d48ad6c261e2254fa764cf29607a4357c99b712dfbb8e6a"))
            (10000, uint256S("0x844f1eab31e8773328ba21970362b4fcff19622f13787cbbe164649ad2393b7a"))
            (10000, uint256S("0x844f1eab31e8773328ba21970362b4fcff19622f13787cbbe164649ad2393b7a"))
            (20000, uint256S("0xfea6d227117db665c5cff2fca0b29d850c5e7f064463d001f5228e80a7e21624"))
            (30000, uint256S("0x5e6212b3b23ed3e5092d7765f7ae36512ecdc254f84c9988161e955d94c91a48"))
            (40000, uint256S("0x3ae62cc62888db77de952d5855fb59f24a46f008177badc5f4f78ab12734985d"))
            (50000, uint256S("0xb0df7fbaa66f0844a607bd3d4b8d25d68a63c57fb34fdae7212c689165edcb8d"))
            (60000, uint256S("0x65504c021a5657321f070a11dd3973cc2dbc56a1f4a0c0d5f1a4d35e887e8182"))
            (70000, uint256S("0x22e5cbb2fbc635e031e424157c49ec55907ba5198ef3aee9595b30238666824f"))
            (80000, uint256S("0x64ce42ada8708c0b30b800403811275edd54054fc95f770f8dc49be1dad3a0e7"))
            (100000, uint256S("0x85e33b3e583fba18d1fc2227702ea97b9d5a441a1a3fa2633c28d6e5d3837218"))
            (120000, uint256S("0x4a11ab4cc4774ebc1dd602472fab4759c2d19ea29d8bb71073cb64474e70da89"))
            (140000, uint256S("0xe6c750c5ce99932b86ca000139909f37abbf829cc39cd09fe4eb7ec88cc50238"))
            (160000, uint256S("0xb855f143c2ebec37b0b9a9e2a2fc9e3d3d2437440c2101fb57ad11407e3bb147"))
            (180000, uint256S("0x774851c8ac775e671bed326be4fec73f5663aa4b1e84e89b20d0cea529fb5c06"))
            (200000, uint256S("0x9aa7fff01e07e800774b4ef7e11d55afffa8a1c6fdb0cd19762418cb8b901b32"))
            (211160, uint256S("0x7de94b058dc9fb6c183c5d3b493c88d13b597e473b86454c923e029d9c6a67b0"))
            (575981, uint256S("0xdeb9ff859b5263edcf1968cb43626264c9b92e84a9805e2af6463776eca51137"))
            (750000, uint256S("0x7c163d8dc6320bdc3b1b726bf7be13fa3a44c621efcb0f8f3bcd7f2ad374b5ef")),
            1485685696, // * UNIX timestamp of last checkpoint block
            0,          // * total number of transactions between genesis and last checkpoint
                        //   (the tx=... number in the SetBestChain debug.log lines)
            7000         // * estimated number of transactions per day after checkpoint
        };
    }
};
static CMainParams mainParams;

/**
 * Testnet (v3)
 */
class CTestNetParams : public CChainParams {
public:
    CTestNetParams() {
        strNetworkID = "test";
        consensus.nSubsidyHalvingInterval = 210000;
        consensus.nMajorityEnforceBlockUpgrade = 51;
        consensus.nMajorityRejectBlockOutdated = 75;
        consensus.nMajorityWindow = 100;
        consensus.BIP34Height = 21111;
        consensus.BIP34Hash = uint256S("0x0000000023b3a96d3484e5abb3755c413e7d41500f8e2a5c3f0dd01299cd8ef8");
        consensus.powLimit = uint256S("00000000ffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
        consensus.nPowTargetTimespan = 14 * 24 * 60 * 60; // two weeks
        consensus.nPowTargetSpacing = 10 * 60;
        consensus.fPowAllowMinDifficultyBlocks = true;
        consensus.fPowNoRetargeting = false;
        consensus.nRuleChangeActivationThreshold = 1512; // 75% for testchains
        consensus.nMinerConfirmationWindow = 2016; // nPowTargetTimespan / nPowTargetSpacing
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = 1199145601; // January 1, 2008
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = 1230767999; // December 31, 2008

        // Deployment of BIP68, BIP112, and BIP113.
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].bit = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nStartTime = 1456790400; // March 1st, 2016
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nTimeout = 1493596800; // May 1st, 2017

        // Deployment of SegWit (BIP141 and BIP143)
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].bit = 1;
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].nStartTime = 1462060800; // May 1st 2016
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].nTimeout = 1493596800; // May 1st 2017

        pchMessageStart[0] = 0x1b;
        pchMessageStart[1] = 0x1a;
        pchMessageStart[2] = 0x49;
        pchMessageStart[3] = 0x02;
        nDefaultPort = 15556;
        nPruneAfterHeight = 1000;

        genesis = CreateGenesisBlock(1484131714, 2042883868, 0x1d00ffff, 1, 50 * COIN);
        consensus.hashGenesisBlock = genesis.GetHash();

        uint256 hashGenesisBlock = uint256S("0x0000000013cb675cc890cf8c7a22f1f3948684b297ccd2553d6e203e00198ae0");

        // Change to true to enable genesis block creation

        if (false && genesis.GetHash() != hashGenesisBlock)
        {
            printf("recalculating params for testnet.\n");
            printf("old testnet genesis nonce: %d\n", genesis.nNonce);
            printf("old testnet genesis hash:  %s\n", hashGenesisBlock.ToString().c_str());
            // deliberately empty for loop finds nonce value.
            for(; genesis.GetHash() > consensus.powLimit; genesis.nNonce++){ }
            printf("new testnet genesis merkle root: %s\n", genesis.hashMerkleRoot.ToString().c_str());
            printf("new testnet genesis nonce: %d\n", genesis.nNonce);
            printf("new testnet genesis hash: %s\n", genesis.GetHash().ToString().c_str());
        }


	// assert(consensus.hashGenesisBlock == uint256S("0x0000000013cb675cc890cf8c7a22f1f3948684b297ccd2553d6e203e00198ae0"));
  //       assert(genesis.hashMerkleRoot == uint256S("0x3c71675ea78d84a0aafd2ec31a82745cd7c2844b6f77616a7f36134f0fd3e30b"));

        vFixedSeeds.clear();
        vSeeds.clear();
        // nodes with support for servicebits filtering should be at the top
        /*vSeeds.push_back(CDNSSeedData("testnetnavcoin.jonasschnelli.ch", "testnet-seed.navcoin.jonasschnelli.ch", true));
        vSeeds.push_back(CDNSSeedData("petertodd.org", "seed.tNAV.petertodd.org", true));
        vSeeds.push_back(CDNSSeedData("bluematt.me", "testnet-seed.bluematt.me"));
        vSeeds.push_back(CDNSSeedData("navcoin.schildbach.de", "testnet-seed.navcoin.schildbach.de"));*/

        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,24);
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,16);
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,29);
        base58Prefixes[EXT_PUBLIC_KEY] = boost::assign::list_of(0x14)(0x45)(0x97)(0xAB).convert_to_container<std::vector<unsigned char> >();
        base58Prefixes[EXT_SECRET_KEY] = boost::assign::list_of(0x24)(0x55)(0x63)(0x92).convert_to_container<std::vector<unsigned char> >();

        vFixedSeeds = std::vector<SeedSpec6>(pnSeed6_test, pnSeed6_test + ARRAYLEN(pnSeed6_test));

        fMiningRequiresPeers = true;
        fDefaultConsistencyChecks = false;
        fRequireStandard = false;
        fMineBlocksOnDemand = false;
        fTestnetToBeDeprecatedFieldRPC = true;

        checkpointData = (CCheckpointData) {
            boost::assign::map_list_of
            ( 0, uint256S("0x0000000013cb675cc890cf8c7a22f1f3948684b297ccd2553d6e203e00198ae0")),
            1484131714,
            0,
            100
        };

    }
};
static CTestNetParams testNetParams;

/**
 * Regression test
 */
class CRegTestParams : public CChainParams {
public:
    CRegTestParams() {
        strNetworkID = "regtest";
        consensus.nSubsidyHalvingInterval = 150;
        consensus.nMajorityEnforceBlockUpgrade = 750;
        consensus.nMajorityRejectBlockOutdated = 950;
        consensus.nMajorityWindow = 1000;
        consensus.BIP34Height = -1; // BIP34 has not necessarily activated on regtest
        consensus.BIP34Hash = uint256();
        consensus.powLimit = uint256S("7fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
        consensus.nPowTargetTimespan = 14 * 24 * 60 * 60; // two weeks
        consensus.nPowTargetSpacing = 10 * 60;
        consensus.fPowAllowMinDifficultyBlocks = true;
        consensus.fPowNoRetargeting = true;
        consensus.nRuleChangeActivationThreshold = 108; // 75% for testchains
        consensus.nMinerConfirmationWindow = 144; // Faster than normal for regtest (144 instead of 2016)
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = 999999999999ULL;
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].bit = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nStartTime = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nTimeout = 999999999999ULL;
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].bit = 1;
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].nStartTime = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].nTimeout = 999999999999ULL;

        pchMessageStart[0] = 0xf9;
        pchMessageStart[1] = 0xef;
        pchMessageStart[2] = 0xa5;
        pchMessageStart[3] = 0x93;
        nDefaultPort = 18444;
        nPruneAfterHeight = 1000;

        genesis = CreateGenesisBlock(1484131714, 2042883868, 0x207fffff, 1, 50 * COIN);
        consensus.hashGenesisBlock = genesis.GetHash();

	// Need to be reviewed

        //assert(consensus.hashGenesisBlock == uint256S("0x0000000013cb675cc890cf8c7a22f1f3948684b297ccd2553d6e203e00198ae0"));
        //assert(genesis.hashMerkleRoot == uint256S("0x3c71675ea78d84a0aafd2ec31a82745cd7c2844b6f77616a7f36134f0fd3e30b"));

        vFixedSeeds.clear(); //!< Regtest mode doesn't have any fixed seeds.
        vSeeds.clear();      //!< Regtest mode doesn't have any DNS seeds.

        fMiningRequiresPeers = false;
        fDefaultConsistencyChecks = true;
        fRequireStandard = false;
        fMineBlocksOnDemand = true;
        fTestnetToBeDeprecatedFieldRPC = false;

        checkpointData = (CCheckpointData){
            boost::assign::map_list_of
            ( 0, uint256S("0x0000000013cb675cc890cf8c7a22f1f3948684b297ccd2553d6e203e00198ae0")),
            0,
            0,
            0
        };
        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,20);
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,96);
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,39);
        base58Prefixes[EXT_PUBLIC_KEY] = boost::assign::list_of(0x05)(0x38)(0x34)(0x76).convert_to_container<std::vector<unsigned char> >();
        base58Prefixes[EXT_SECRET_KEY] = boost::assign::list_of(0x06)(0x37)(0x64)(0x13).convert_to_container<std::vector<unsigned char> >();
    }
};
static CRegTestParams regTestParams;

static CChainParams *pCurrentParams = 0;

const CChainParams &Params() {
    assert(pCurrentParams);
    return *pCurrentParams;
}

CChainParams& Params(const std::string& chain)
{
    if (chain == CBaseChainParams::MAIN)
            return mainParams;
    else if (chain == CBaseChainParams::TESTNET)
            return testNetParams;
    else if (chain == CBaseChainParams::REGTEST)
            return regTestParams;
    else
        throw std::runtime_error(strprintf("%s: Unknown chain %s.", __func__, chain));
}

void SelectParams(const std::string& network)
{
    SelectBaseParams(network);
    pCurrentParams = &Params(network);
}
