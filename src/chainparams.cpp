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
    txNew.vout[0].nValue = genesisReward;
    txNew.vout[0].scriptPubKey.clear();
    txNew.strDZeel = "NavCoin genesis block";

    CBlock genesis;
    genesis.nTime    = nTime;
    genesis.nBits    = nBits;
    genesis.nNonce   = nNonce;
    genesis.nVersion = nVersion;
    genesis.vtx.push_back(txNew);
    genesis.vtx[0].nTime = nTime;
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

static CBlock CreateGenesisBlockTestnet(uint32_t nTime, uint32_t nNonce, uint32_t nBits, int32_t nVersion, const CAmount& genesisReward)
{
    const char* pszTimestamp = "Time does not exist";
    const CScript genesisOutputScript = CScript() << ParseHex("047c3ec9cb8ea3148cfdb2107383209fc0883585b25e355ded65970bde3a49c5c79dfac1210dc8a4876f6bb68431b273b6d5347955135502726b0743948cee36d1") << OP_CHECKSIG;
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
        consensus.BIP34Hash = uint256S("0xecb7444214d068028ec1fa4561662433452c1cbbd6b0f8eeb6452bcfa1d0a7d6");
        consensus.powLimit = ArithToUint256(~arith_uint256(0) >> 16);
        consensus.nPowTargetTimespan = 30;
        consensus.nPowTargetSpacing = 30;
        consensus.fPowAllowMinDifficultyBlocks = false;
        consensus.fPowNoRetargeting = false;
        consensus.nRuleChangeActivationThreshold = 15120; // 75% of 20160
        consensus.nMinerConfirmationWindow = 20160;
        consensus.nStakeMinAge = 60 * 60 * 2;	// minimum for coin age: 2 hours
        consensus.nTargetSpacing = 30; // Blocktime: 30 secs
        consensus.nStakeCombineThreshold = 1000 * COIN;
        consensus.nStakeSplitThreshold = 2 * consensus.nStakeCombineThreshold;
        consensus.nDailyBlockCount =  (24 * 60 * 60) / consensus.nTargetSpacing;
        consensus.nModifierInterval = 10 * 60; // time to elapse before new modifier is computed
        consensus.nTargetTimespan = 25 * 30;
        consensus.nLastPOWBlock = 20000;
        consensus.nBlocksPerVotingCycle = 2880 * 7; // 7 Days
        consensus.nMinimumQuorum = 0.5;
        consensus.nMinimumQuorumFirstHalf = 0.5;
        consensus.nMinimumQuorumSecondHalf = 0.4;
        consensus.nVotesAcceptProposal = 0.7;
        consensus.nVotesRejectProposal = 0.7;
        consensus.nVotesAcceptPaymentRequest = 0.7;
        consensus.nVotesRejectPaymentRequest = 0.7;
        consensus.nCommunityFundMinAge = 50;
        consensus.nProposalMinimalFee = 5000000000;
        consensus.sigActivationTime = 1512990000;
        consensus.nCoinbaseTimeActivationHeight = 20000;
        consensus.nBlockSpreadCFundAccumulation = 500;
        consensus.nCommunityFundAmount = 0.25 * COIN;
        consensus.nCommunityFundAmountV2 = 0.5 * COIN;
        consensus.nCyclesProposalVoting = 6;
        consensus.nCyclesPaymentRequestVoting = 8;
        consensus.nPaymentRequestMaxVersion = 3;
        consensus.nProposalMaxVersion = 3;
        consensus.nMaxFutureDrift = 60;
        consensus.nStaticReward = 2 * COIN;
        consensus.nHeightv451Fork = 2722100;

        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = 1199145601; // January 1, 2008
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = 1230767999; // December 31, 2008

        // Deployment of BIP68, BIP112, and BIP113.
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].bit = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nStartTime = 1462060800; // May 1st, 2016
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nTimeout = 1525132800; // May 1st, 2017

        // Deployment of SegWit (BIP141 and BIP143)
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT_LEGACY].bit = 1;
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT_LEGACY].nStartTime = 1493424000; // May 1st, 2017
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT_LEGACY].nTimeout = 1525132800; // May 1st, 2018

        consensus.vDeployments[Consensus::DEPLOYMENT_CSV_LEGACY].bit = 2;
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV_LEGACY].nStartTime = 1462060800; // May 1st, 2016
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV_LEGACY].nTimeout = 1525132800; // May 1st, 2018

        // Deployment of Cold Staking
        consensus.vDeployments[Consensus::DEPLOYMENT_COLDSTAKING].bit = 3;
        consensus.vDeployments[Consensus::DEPLOYMENT_COLDSTAKING].nStartTime = 1525132800; // May 1st, 2018
        consensus.vDeployments[Consensus::DEPLOYMENT_COLDSTAKING].nTimeout = 1556712000; // May 1st, 2019

        // Deployment of SegWit (BIP141 and BIP143)
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].bit = 4;
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].nStartTime = 1493424000; // May 1st, 2017
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].nTimeout = 1525132800; // May 1st, 2018

        // Deployment of Community Fund
        consensus.vDeployments[Consensus::DEPLOYMENT_COMMUNITYFUND].bit = 6;
        consensus.vDeployments[Consensus::DEPLOYMENT_COMMUNITYFUND].nStartTime = 1493424000; // May 1st, 2017
        consensus.vDeployments[Consensus::DEPLOYMENT_COMMUNITYFUND].nTimeout = 1556668800; // May 1st, 2019

        // Deployment of Community Fund Accumulation
        consensus.vDeployments[Consensus::DEPLOYMENT_COMMUNITYFUND_ACCUMULATION].bit = 7;
        consensus.vDeployments[Consensus::DEPLOYMENT_COMMUNITYFUND_ACCUMULATION].nStartTime = 1525132800; // May 1st, 2018
        consensus.vDeployments[Consensus::DEPLOYMENT_COMMUNITYFUND_ACCUMULATION].nTimeout = 1556712000; // May 1st, 2019

        // Deployment of NTP Sync
        consensus.vDeployments[Consensus::DEPLOYMENT_NTPSYNC].bit = 8;
        consensus.vDeployments[Consensus::DEPLOYMENT_NTPSYNC].nStartTime = 1525132800; // May 1st, 2018
        consensus.vDeployments[Consensus::DEPLOYMENT_NTPSYNC].nTimeout = 1556712000; // May 1st, 2019

        // Deployment of Community Fund Accumulation Spread(NPIP-0003)
        consensus.vDeployments[Consensus::DEPLOYMENT_COMMUNITYFUND_ACCUMULATION_SPREAD].bit = 14;
        consensus.vDeployments[Consensus::DEPLOYMENT_COMMUNITYFUND_ACCUMULATION_SPREAD].nStartTime = 1525132800; // May 1st, 2018
        consensus.vDeployments[Consensus::DEPLOYMENT_COMMUNITYFUND_ACCUMULATION_SPREAD].nTimeout = 1556712000; // May 1st, 2019

        // Increate in Community Fund Accumulation Ammonut (NPIP-0004)
        consensus.vDeployments[Consensus::DEPLOYMENT_COMMUNITYFUND_AMOUNT_V2].bit = 16;
        consensus.vDeployments[Consensus::DEPLOYMENT_COMMUNITYFUND_AMOUNT_V2].nStartTime = 1533081600; // Aug 1st, 2018
        consensus.vDeployments[Consensus::DEPLOYMENT_COMMUNITYFUND_AMOUNT_V2].nTimeout = 1564617600; // Aug 1st, 2019

        // Deployment of Static Reward
        consensus.vDeployments[Consensus::DEPLOYMENT_STATIC_REWARD].bit = 15;
        consensus.vDeployments[Consensus::DEPLOYMENT_STATIC_REWARD].nStartTime = 1533081600; // August 1st, 2018
        consensus.vDeployments[Consensus::DEPLOYMENT_STATIC_REWARD].nTimeout = 1564617600; // August 1st, 2019

        // Deployment of Quorum reduction for the Community Fund
        consensus.vDeployments[Consensus::DEPLOYMENT_QUORUM_CFUND].bit = 17;
        consensus.vDeployments[Consensus::DEPLOYMENT_QUORUM_CFUND].nStartTime = 1543622400; // Dec 1st, 2018
        consensus.vDeployments[Consensus::DEPLOYMENT_QUORUM_CFUND].nTimeout = 1575158400; // Dec 1st, 2019

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

        genesis = CreateGenesisBlock(1460561040, 6945, 0x1f00ffff, 1, 0);

	      consensus.hashGenesisBlock = genesis.GetHash();

        assert(consensus.hashGenesisBlock == uint256S("0x00006a4e3e18c71c6d48ad6c261e2254fa764cf29607a4357c99b712dfbb8e6a"));
        assert(genesis.hashMerkleRoot == uint256S("0xc507eec6ccabfd5432d764afceafba42d2d946594b8a60570cb2358a7392c61a"));

        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,53);
        base58Prefixes[COLDSTAKING_ADDRESS] = std::vector<unsigned char>(1,21); // cold staking addresses start with 'X'
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,85);
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,150);
        base58Prefixes[EXT_PUBLIC_KEY] = boost::assign::list_of(0x04)(0x88)(0xB2)(0x1E).convert_to_container<std::vector<unsigned char> >();
        base58Prefixes[EXT_SECRET_KEY] = boost::assign::list_of(0x04)(0x88)(0xAD)(0xE4).convert_to_container<std::vector<unsigned char> >();

        vSeeds.push_back(CDNSSeedData("nav.community", "seed.nav.community"));
        vSeeds.push_back(CDNSSeedData("navcoin.org", "seed.navcoin.org"));

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
            (500000, uint256S("0xee97c591a67c19ed74e6162f18eaa73c2a566d3e2754df8fa70a5662e3ed30df"))
            (750000, uint256S("0x7c163d8dc6320bdc3b1b726bf7be13fa3a44c621efcb0f8f3bcd7f2ad374b5ef"))
            (1000000,uint256S("0x12befed86624731beb4f53a523ece1342f68832710a2933f189f18ddaf5af713"))
            (1250000,uint256S("0x43a9f08444cf05b1193dbdcfffebd558c0d406e478606bdc8755de75c513f620"))
            (1500000,uint256S("0xdfc75b7d1be540bbcabea1323f6d06d92e96b73c93f386b57f82cd62a1c94ce1"))
            (1750000,uint256S("0x713cbac2df077bacb25dedf4acd60745f102cbe53ede019b738a6864fc0b12c6"))
            (2000000,uint256S("0x41b723e003cab30d326a0fae995521554ab59e550e1e0013187b3267d09dd42c"))
            (2250000,uint256S("0x1f7d459f6dcb3752868395819ac676adccfb0d0ec904a60b9fcb15879bcc5228"))
            (2500000,uint256S("0x38000f4f433d47f171c7cccc001a2e7c9292620b37170d347b5a21ef19243dda")),
            1535607904, // * UNIX timestamp of last checkpoint block
            5067164,    // * total number of transactions between genesis and last checkpoint
                        //   (the tx=... number in the SetBestChain debug.log lines)
            7000        // * estimated number of transactions per day after checkpoint
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
        consensus.nMajorityEnforceBlockUpgrade = 750;
        consensus.nMajorityRejectBlockOutdated = 950;
        consensus.nMajorityWindow = 1000;
        consensus.BIP34Height = 900000;
        consensus.BIP34Hash = uint256S("0xecb7444214d068028ec1fa4561662433452c1cbbd6b0f8eeb6452bcfa1d0a7d6");
        consensus.powLimit = ArithToUint256(~arith_uint256(0) >> 16);
        consensus.nPowTargetTimespan = 30;
        consensus.nPowTargetSpacing = 30;
        consensus.fPowAllowMinDifficultyBlocks = false;
        consensus.fPowNoRetargeting = false;
        consensus.nRuleChangeActivationThreshold = 300; // 75% of 400
        consensus.nMinerConfirmationWindow = 400;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = 1199145601; // January 1, 2008
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = 1230767999; // December 31, 2008
        consensus.nStakeMinAge = 2;	// minimum for coin age: 2 seconds
        consensus.nTargetSpacing = 30; // Blocktime: 30 secs
        consensus.nStakeCombineThreshold = 1000 * COIN;
        consensus.nStakeSplitThreshold = 2 * consensus.nStakeCombineThreshold;
        consensus.nDailyBlockCount =  (24 * 60 * 60) / consensus.nTargetSpacing;
        consensus.nModifierInterval = 10 * 60; // time to elapse before new modifier is computed
        consensus.nTargetTimespan = 25 * 30;
        consensus.nLastPOWBlock = 1000000;
        consensus.nBlocksPerVotingCycle = 180; // 1.5 hours
        consensus.nMinimumQuorum = 0.5;
        consensus.nMinimumQuorumFirstHalf = 0.5;
        consensus.nMinimumQuorumSecondHalf = 0.4;
        consensus.nVotesAcceptProposal = 0.7;
        consensus.nVotesRejectProposal = 0.7;
        consensus.nVotesAcceptPaymentRequest = 0.7;
        consensus.nVotesRejectPaymentRequest = 0.7;
        consensus.nCommunityFundMinAge = 5;
        consensus.nProposalMinimalFee = 10000;
        consensus.sigActivationTime = 1512826692;
        consensus.nCoinbaseTimeActivationHeight = 30000;
        consensus.nBlockSpreadCFundAccumulation = 500;
        consensus.nCommunityFundAmount = 0.25 * COIN;
        consensus.nCommunityFundAmountV2 = 0.5 * COIN;
        consensus.nCyclesProposalVoting = 4;
        consensus.nCyclesPaymentRequestVoting = 4;
        consensus.nPaymentRequestMaxVersion = 3;
        consensus.nProposalMaxVersion = 3;
        consensus.nMaxFutureDrift = 60;
        consensus.nStaticReward = 2 * COIN;
        consensus.nHeightv451Fork = 100000;

        // Deployment of BIP68, BIP112, and BIP113.
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].bit = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nStartTime = 1462060800; // May 1st, 2016
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nTimeout = 1556712000; // May 1st, 2019

        // Deployment of Cold Staking
        consensus.vDeployments[Consensus::DEPLOYMENT_COLDSTAKING].bit = 6;
        consensus.vDeployments[Consensus::DEPLOYMENT_COLDSTAKING].nStartTime = 1525132800; // May 1st, 2018
        consensus.vDeployments[Consensus::DEPLOYMENT_COLDSTAKING].nTimeout = 1556712000; // May 1st, 2019

        // Deployment of SegWit (BIP141 and BIP143)
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].bit = 4;
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].nStartTime = 1493424000; // May 1st, 2017
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].nTimeout = 1556712000; // May 1st, 2019

        // Deployment of Community Fund
        consensus.vDeployments[Consensus::DEPLOYMENT_COMMUNITYFUND].bit = 6;
        consensus.vDeployments[Consensus::DEPLOYMENT_COMMUNITYFUND].nStartTime = 1493424000; // May 1st, 2017
        consensus.vDeployments[Consensus::DEPLOYMENT_COMMUNITYFUND].nTimeout = 1556712000; // May 1st, 2019

        // Deployment of Community Fund Accumulation
        consensus.vDeployments[Consensus::DEPLOYMENT_COMMUNITYFUND_ACCUMULATION].bit = 7;
        consensus.vDeployments[Consensus::DEPLOYMENT_COMMUNITYFUND_ACCUMULATION].nStartTime = 1525132800; // May 1st, 2018
        consensus.vDeployments[Consensus::DEPLOYMENT_COMMUNITYFUND_ACCUMULATION].nTimeout = 1556712000; // May 1st, 2019

        // Deployment of NTP Sync
        consensus.vDeployments[Consensus::DEPLOYMENT_NTPSYNC].bit = 8;
        consensus.vDeployments[Consensus::DEPLOYMENT_NTPSYNC].nStartTime = 1525132800; // May 1st, 2018
        consensus.vDeployments[Consensus::DEPLOYMENT_NTPSYNC].nTimeout = 1556712000; // May 1st, 2019

        // Deployment of Community Fund Accumulation Spread(NPIP-0003)
        consensus.vDeployments[Consensus::DEPLOYMENT_COMMUNITYFUND_ACCUMULATION_SPREAD].bit = 14;
        consensus.vDeployments[Consensus::DEPLOYMENT_COMMUNITYFUND_ACCUMULATION_SPREAD].nStartTime = 1525132800; // May 1st, 2018
        consensus.vDeployments[Consensus::DEPLOYMENT_COMMUNITYFUND_ACCUMULATION_SPREAD].nTimeout = 1556712000; // May 1st, 2019

        // Increate in Community Fund Accumulation Ammonut (NPIP-0004)
        consensus.vDeployments[Consensus::DEPLOYMENT_COMMUNITYFUND_AMOUNT_V2].bit = 16;
        consensus.vDeployments[Consensus::DEPLOYMENT_COMMUNITYFUND_AMOUNT_V2].nStartTime = 1533081600; // Aug 1st, 2018
        consensus.vDeployments[Consensus::DEPLOYMENT_COMMUNITYFUND_AMOUNT_V2].nTimeout = 1564617600; // Aug 1st, 2019

        // Deployment of Static Reward
        consensus.vDeployments[Consensus::DEPLOYMENT_STATIC_REWARD].bit = 15;
        consensus.vDeployments[Consensus::DEPLOYMENT_STATIC_REWARD].nStartTime = 1533081600; // August 1st, 2018
        consensus.vDeployments[Consensus::DEPLOYMENT_STATIC_REWARD].nTimeout = 1564617600; // August 1st, 2019

        // Deployment of Quorum reduction for the Community Fund
        consensus.vDeployments[Consensus::DEPLOYMENT_QUORUM_CFUND].bit = 17;
        consensus.vDeployments[Consensus::DEPLOYMENT_QUORUM_CFUND].nStartTime = 1543622400; // Dec 1st, 2018
        consensus.vDeployments[Consensus::DEPLOYMENT_QUORUM_CFUND].nTimeout = 1575158400; // Dec 1st, 2019

        /**
         * The message start string is designed to be unlikely to occur in normal data.
         * The characters are rarely used upper ASCII, not valid as UTF-8, and produce
         * a large 32-bit integer with any alignment.
         */
        pchMessageStart[0] = 0x3f;
        pchMessageStart[1] = 0xa4;
        pchMessageStart[2] = 0x52;
        pchMessageStart[3] = 0x22;
        nDefaultPort = 15556;
        nPruneAfterHeight = 1000;
        bnProofOfWorkLimit = arith_uint256(~arith_uint256() >> 16);
    
        uint32_t nTimestamp = 1541978905;
        uint256 hashGenesisBlock = uint256S("0x0000c493d0f928a9f0a0eb0e0d88d871f4f5ab8bacc308f77bb0e8c7adee1307");
        uint256 hashMerkleRoot = uint256S("0xf3f3286d731fc322d34f6a7c4df3eecfc1abaa6032350acf6cd363eb34edf574");
        uint32_t nNonce = 2043537527;
	    
        genesis = CreateGenesisBlockTestnet(nTimestamp, nNonce, 0x1d00ffff, 1, 0);
        consensus.hashGenesisBlock = genesis.GetHash();
	    
        if (true && (genesis.GetHash() != hashGenesisBlock || genesis.hashMerkleRoot != hashMerkleRoot))
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

        vSeeds.push_back(CDNSSeedData("testnav.community", "testseed.nav.community"));
        vSeeds.push_back(CDNSSeedData("testnavcoin.org", "testseed.navcoin.org"));

        assert(consensus.hashGenesisBlock == hashGenesisBlock);
        assert(genesis.hashMerkleRoot == hashMerkleRoot);

        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,111);
        base58Prefixes[COLDSTAKING_ADDRESS] = std::vector<unsigned char>(1,8); // cold staking addresses start with 'C/D'
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,196);
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,239);
        base58Prefixes[EXT_PUBLIC_KEY] = boost::assign::list_of(0x40)(0x88)(0x2B)(0xE1).convert_to_container<std::vector<unsigned char> >();
        base58Prefixes[EXT_SECRET_KEY] = boost::assign::list_of(0x40)(0x88)(0xDA)(0x4E).convert_to_container<std::vector<unsigned char> >();

        vFixedSeeds = std::vector<SeedSpec6>(pnSeed6_test, pnSeed6_test + ARRAYLEN(pnSeed6_test));

        fMiningRequiresPeers = true;
        fDefaultConsistencyChecks = false;
        fRequireStandard = false;
        fMineBlocksOnDemand = false;
        fTestnetToBeDeprecatedFieldRPC = true;

        checkpointData = (CCheckpointData) {
            boost::assign::map_list_of
            ( 0,     hashGenesisBlock),
            1525248575, // * UNIX timestamp of last checkpoint block
            0,          // * total number of transactions between genesis and last checkpoint
                        //   (the tx=... number in the SetBestChain debug.log lines)
            7000         // * estimated number of transactions per day after checkpoint
        };

    }
};
static CTestNetParams testNetParams;

/**
 * Devnet (v3)
 */
class CDevNetParams : public CChainParams {
public:
    CDevNetParams() {
        strNetworkID = "dev";
        consensus.nSubsidyHalvingInterval = 210000;
        consensus.nMajorityEnforceBlockUpgrade = 750;
        consensus.nMajorityRejectBlockOutdated = 950;
        consensus.nMajorityWindow = 1000;
        consensus.BIP34Height = 900000;
        consensus.BIP34Hash = uint256S("0x0");
        consensus.powLimit = ArithToUint256(~arith_uint256(0) >> 16);
        consensus.nPowTargetTimespan = 30;
        consensus.nPowTargetSpacing = 30;
        consensus.fPowAllowMinDifficultyBlocks = false;
        consensus.fPowNoRetargeting = false;
        consensus.nRuleChangeActivationThreshold = 75; // 75% of 400
        consensus.nMinerConfirmationWindow = 100;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = 1199145601; // January 1, 2008
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = 1230767999; // December 31, 2008
        consensus.nStakeMinAge = 2;	// minimum for coin age: 2 seconds
        consensus.nTargetSpacing = 30; // Blocktime: 30 secs
        consensus.nStakeCombineThreshold = 1000 * COIN;
        consensus.nStakeSplitThreshold = 2 * consensus.nStakeCombineThreshold;
        consensus.nDailyBlockCount =  (24 * 60 * 60) / consensus.nTargetSpacing;
        consensus.nModifierInterval = 10 * 60; // time to elapse before new modifier is computed
        consensus.nTargetTimespan = 25 * 30;
        consensus.nLastPOWBlock = 100000;
        consensus.nBlocksPerVotingCycle = 180; // 1.5 hours
        consensus.nMinimumQuorum = 0.5;
        consensus.nMinimumQuorumFirstHalf = 0.5;
        consensus.nMinimumQuorumSecondHalf = 0.4;
        consensus.nVotesAcceptProposal = 0.7;
        consensus.nVotesRejectProposal = 0.7;
        consensus.nVotesAcceptPaymentRequest = 0.7;
        consensus.nVotesRejectPaymentRequest = 0.7;
        consensus.nCommunityFundMinAge = 5;
        consensus.nProposalMinimalFee = 10000;
        consensus.sigActivationTime = 1512826692;
        consensus.nCoinbaseTimeActivationHeight = 0;
        consensus.nBlockSpreadCFundAccumulation = 500;
        consensus.nCommunityFundAmount = 0.25 * COIN;
        consensus.nCommunityFundAmountV2 = 0.5 * COIN;
        consensus.nCyclesProposalVoting = 4;
        consensus.nCyclesPaymentRequestVoting = 4;
        consensus.nPaymentRequestMaxVersion = 3;
        consensus.nProposalMaxVersion = 3;
        consensus.nMaxFutureDrift = 60000;
        consensus.nStaticReward = 2 * COIN;
        consensus.nHeightv451Fork = 1000;

        // Deployment of BIP68, BIP112, and BIP113.
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].bit = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nStartTime = 1462060800; // May 1st, 2016
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nTimeout = 1556668800; // May 1st, 2019

        // Deployment of Cold Staking
        consensus.vDeployments[Consensus::DEPLOYMENT_COLDSTAKING].bit = 3;
        consensus.vDeployments[Consensus::DEPLOYMENT_COLDSTAKING].nStartTime = 1525132800; // May 1st, 2018
        consensus.vDeployments[Consensus::DEPLOYMENT_COLDSTAKING].nTimeout = 1556712000; // May 1st, 2019

        // Deployment of SegWit (BIP141 and BIP143)
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].bit = 5;
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].nStartTime = 1493424000; // May 1st, 2017
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].nTimeout = 1556668800; // May 1st, 2019

        // Deployment of Community Fund
        consensus.vDeployments[Consensus::DEPLOYMENT_COMMUNITYFUND].bit = 6;
        consensus.vDeployments[Consensus::DEPLOYMENT_COMMUNITYFUND].nStartTime = 1493424000; // May 1st, 2017
        consensus.vDeployments[Consensus::DEPLOYMENT_COMMUNITYFUND].nTimeout = 1556668800; // May 1st, 2019
	    
        // Deployment of NTP Sync
        consensus.vDeployments[Consensus::DEPLOYMENT_NTPSYNC].bit = 8;
        consensus.vDeployments[Consensus::DEPLOYMENT_NTPSYNC].nStartTime = 1525132800; // May 1st, 2018
        consensus.vDeployments[Consensus::DEPLOYMENT_NTPSYNC].nTimeout = 1556712000; // May 1st, 2019

        // Deployment of Community Fund Accumulation
        consensus.vDeployments[Consensus::DEPLOYMENT_COMMUNITYFUND_ACCUMULATION].bit = 7;
        consensus.vDeployments[Consensus::DEPLOYMENT_COMMUNITYFUND_ACCUMULATION].nStartTime = 1525132800; // May 1st, 2018
        consensus.vDeployments[Consensus::DEPLOYMENT_COMMUNITYFUND_ACCUMULATION].nTimeout = 1556712000; // May 1st, 2019

        // Deployment of Community Fund Accumulation Spread(NPIP-0003)
        consensus.vDeployments[Consensus::DEPLOYMENT_COMMUNITYFUND_ACCUMULATION_SPREAD].bit = 14;
        consensus.vDeployments[Consensus::DEPLOYMENT_COMMUNITYFUND_ACCUMULATION_SPREAD].nStartTime = 1525132800; // May 1st, 2018
        consensus.vDeployments[Consensus::DEPLOYMENT_COMMUNITYFUND_ACCUMULATION_SPREAD].nTimeout = 1556712000; // May 1st, 2019

        // Increate in Community Fund Accumulation Ammonut (NPIP-0004)
        consensus.vDeployments[Consensus::DEPLOYMENT_COMMUNITYFUND_AMOUNT_V2].bit = 16;
        consensus.vDeployments[Consensus::DEPLOYMENT_COMMUNITYFUND_AMOUNT_V2].nStartTime = 1533081600; // Aug 1st, 2018
        consensus.vDeployments[Consensus::DEPLOYMENT_COMMUNITYFUND_AMOUNT_V2].nTimeout = 1564617600; // Aug 1st, 2019

        // Deployment of Static Reward
        consensus.vDeployments[Consensus::DEPLOYMENT_STATIC_REWARD].bit = 15;
        consensus.vDeployments[Consensus::DEPLOYMENT_STATIC_REWARD].nStartTime = 1533081600; // August 1st, 2018
        consensus.vDeployments[Consensus::DEPLOYMENT_STATIC_REWARD].nTimeout = 1564617600; // August 1st, 2019

        // Deployment of Quorum reduction for the Community Fund
        consensus.vDeployments[Consensus::DEPLOYMENT_QUORUM_CFUND].bit = 17;
        consensus.vDeployments[Consensus::DEPLOYMENT_QUORUM_CFUND].nStartTime = 1543622400; // Dec 1st, 2018
        consensus.vDeployments[Consensus::DEPLOYMENT_QUORUM_CFUND].nTimeout = 1575158400; // Dec 1st, 2019

        /**
         * The message start string is designed to be unlikely to occur in normal data.
         * The characters are rarely used upper ASCII, not valid as UTF-8, and produce
         * a large 32-bit integer with any alignment.
         */
        pchMessageStart[0] = 0xa8;
        pchMessageStart[1] = 0xb3;
        pchMessageStart[2] = 0x89;
        pchMessageStart[3] = 0xfa;
        nDefaultPort = 18886;
        nPruneAfterHeight = 1000;
        bnProofOfWorkLimit = arith_uint256(~arith_uint256() >> 16);

        // To create a new devnet:
        //
        // 1) Replace nTimestamp with current timestamp.
        uint32_t nTimestamp = 1525248575;
        // 2) Rebuild
        // 3) Launch daemon. It'll calculate the new parameters.
        // 4) Update the following variables with the new values:
        uint256 hashGenesisBlock = uint256S("0x0000e01b12644af6917e5aada637a609dd9590ad6bdc4828cd8df95258d85c02");
        uint256 hashMerkleRoot = uint256S("0x2d9101b87fe7b9deaea41849c1f3bed71e060739147802a238fe968f75ad0fd9");
        uint32_t nNonce = 2043184832;
        // 5) Rebuild. Launch daemon.
        // 6) Generate first block using RPC command "./navcoin-cli generate 1"

        genesis = CreateGenesisBlockTestnet(nTimestamp, nNonce, 0x1d00ffff, 1, 0);
        consensus.hashGenesisBlock = genesis.GetHash();

        if (true && (genesis.GetHash() != hashGenesisBlock || genesis.hashMerkleRoot != hashMerkleRoot))
        {
            printf("recalculating params for devnet.\n");
            printf("old devnet genesis merkle root:  0x%s\n\n", hashMerkleRoot.ToString().c_str());
            printf("old devnet genesis nonce: %d\n", genesis.nNonce);
            printf("old devnet genesis hash:  0x%s\n\n", hashGenesisBlock.ToString().c_str());
            // deliberately empty for loop finds nonce value.
            for(; genesis.GetHash() > consensus.powLimit; genesis.nNonce++){ }
            printf("new devnet genesis merkle root: 0x%s\n", genesis.hashMerkleRoot.ToString().c_str());
            printf("new devnet genesis nonce: %d\n", genesis.nNonce);
            printf("new devnet genesis hash: 0x%s\n", genesis.GetHash().ToString().c_str());
            printf("use the new values to update CDevNetParams class in src/chainparams.cpp\n");
        }

        vSeeds.push_back(CDNSSeedData("devnav.community", "devseed.nav.community"));
        vSeeds.push_back(CDNSSeedData("devnet.navcoin.org", "devseed.navcoin.org"));

        assert(consensus.hashGenesisBlock == hashGenesisBlock);
        assert(genesis.hashMerkleRoot == hashMerkleRoot);

        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,111);
        base58Prefixes[COLDSTAKING_ADDRESS] = std::vector<unsigned char>(1,63); // cold staking addresses start with 'S'
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,196);
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,239);
        base58Prefixes[EXT_PUBLIC_KEY] = boost::assign::list_of(0x40)(0x88)(0x2B)(0xE1).convert_to_container<std::vector<unsigned char> >();
        base58Prefixes[EXT_SECRET_KEY] = boost::assign::list_of(0x40)(0x88)(0xDA)(0x4E).convert_to_container<std::vector<unsigned char> >();

        vFixedSeeds = std::vector<SeedSpec6>(pnSeed6_dev, pnSeed6_dev + ARRAYLEN(pnSeed6_dev));

        fMiningRequiresPeers = false;
        fDefaultConsistencyChecks = false;
        fRequireStandard = false;
        fMineBlocksOnDemand = true;
        fTestnetToBeDeprecatedFieldRPC = true;

        checkpointData = (CCheckpointData) {
            boost::assign::map_list_of
            ( 0,     hashGenesisBlock),
            1515437594, // * UNIX timestamp of last checkpoint block
            0,          // * total number of transactions between genesis and last checkpoint
                        //   (the tx=... number in the SetBestChain debug.log lines)
            7000         // * estimated number of transactions per day after checkpoint
        };

    }
};
static CDevNetParams devNetParams;

/**
 * Regression test
 */
class CRegTestParams : public CChainParams {
public:
    CRegTestParams() {
        strNetworkID = "regtest";
        consensus.nSubsidyHalvingInterval = 210000;
        consensus.nMajorityEnforceBlockUpgrade = 750;
        consensus.nMajorityRejectBlockOutdated = 950;
        consensus.nMajorityWindow = 1000;
        consensus.BIP34Height = 900000;
        consensus.BIP34Hash = uint256S("0xecb7444214d068028ec1fa4561662433452c1cbbd6b0f8eeb6452bcfa1d0a7d6");
        consensus.powLimit = ArithToUint256(~arith_uint256(0) >> 1);
        consensus.nPowTargetTimespan = 30;
        consensus.nPowTargetSpacing = 30;
        consensus.fPowAllowMinDifficultyBlocks = false;
        consensus.fPowNoRetargeting = false;
        consensus.nRuleChangeActivationThreshold = 75; // 75% of 100
        consensus.nMinerConfirmationWindow = 100;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = 1199145601; // January 1, 2008
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = 1230767999; // December 31, 2008
        consensus.nStakeMinAge = 2;	// minimum for coin age: 2 seconds
        consensus.nTargetSpacing = 30; // Blocktime: 30 secs
        consensus.nStakeCombineThreshold = 1000 * COIN;
        consensus.nStakeSplitThreshold = 2 * consensus.nStakeCombineThreshold;
        consensus.nDailyBlockCount =  (24 * 60 * 60) / consensus.nTargetSpacing;
        consensus.nModifierInterval = 10 * 60; // time to elapse before new modifier is computed
        consensus.nTargetTimespan = 25 * 30;
        consensus.nLastPOWBlock = 100000;
        consensus.nBlocksPerVotingCycle = 180; // 1.5 hours
        consensus.nMinimumQuorum = 0.5;
        consensus.nMinimumQuorumFirstHalf = 0.5;
        consensus.nMinimumQuorumSecondHalf = 0.4;
        consensus.nVotesAcceptProposal = 0.7;
        consensus.nVotesRejectProposal = 0.7;
        consensus.nVotesAcceptPaymentRequest = 0.7;
        consensus.nVotesRejectPaymentRequest = 0.7;
        consensus.nCommunityFundMinAge = 5;
        consensus.nProposalMinimalFee = 10000;
        consensus.sigActivationTime = 0;
        consensus.nCoinbaseTimeActivationHeight = 0;
        consensus.nBlockSpreadCFundAccumulation = 10;
        consensus.nCommunityFundAmount = 0.25 * COIN;
        consensus.nCommunityFundAmountV2 = 0.5 * COIN;
        consensus.nCyclesProposalVoting = 4;
        consensus.nCyclesPaymentRequestVoting = 4;
        consensus.nPaymentRequestMaxVersion = 3;
        consensus.nProposalMaxVersion = 3;
        consensus.nMaxFutureDrift = 60000;
        consensus.nStaticReward = 2 * COIN;
        consensus.nHeightv451Fork = 1000;

        // Deployment of BIP68, BIP112, and BIP113.
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].bit = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nStartTime = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nTimeout = 2556712000;

        // Deployment of Cold Staking
        consensus.vDeployments[Consensus::DEPLOYMENT_COLDSTAKING].bit = 3;
        consensus.vDeployments[Consensus::DEPLOYMENT_COLDSTAKING].nStartTime = 1525132800; // May 1st, 2018
        consensus.vDeployments[Consensus::DEPLOYMENT_COLDSTAKING].nTimeout = 1556712000; // May 1st, 2019

        // Deployment of SegWit (BIP141 and BIP143)
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].bit = 5;
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].nStartTime = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].nTimeout = 2556712000;

        // Deployment of Community Fund
        consensus.vDeployments[Consensus::DEPLOYMENT_COMMUNITYFUND].bit = 6;
        consensus.vDeployments[Consensus::DEPLOYMENT_COMMUNITYFUND].nStartTime = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_COMMUNITYFUND].nTimeout = 2556712000;

        // Deployment of Community Fund Accumulation
        consensus.vDeployments[Consensus::DEPLOYMENT_COMMUNITYFUND_ACCUMULATION].bit = 7;
        consensus.vDeployments[Consensus::DEPLOYMENT_COMMUNITYFUND_ACCUMULATION].nStartTime = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_COMMUNITYFUND_ACCUMULATION].nTimeout = 2556712000;

        // Deployment of NTP Sync
        consensus.vDeployments[Consensus::DEPLOYMENT_NTPSYNC].bit = 8;
        consensus.vDeployments[Consensus::DEPLOYMENT_NTPSYNC].nStartTime = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_NTPSYNC].nTimeout = 2556712000;

        // Deployment of Community Fund Accumulation Spread(NPIP-0003)
        consensus.vDeployments[Consensus::DEPLOYMENT_COMMUNITYFUND_ACCUMULATION_SPREAD].bit = 14;
        consensus.vDeployments[Consensus::DEPLOYMENT_COMMUNITYFUND_ACCUMULATION_SPREAD].nStartTime = 1525132800; // May 1st, 2018
        consensus.vDeployments[Consensus::DEPLOYMENT_COMMUNITYFUND_ACCUMULATION_SPREAD].nTimeout = 1556712000; // May 1st, 2019

        // Increate in Community Fund Accumulation Ammonut (NPIP-0004)
        consensus.vDeployments[Consensus::DEPLOYMENT_COMMUNITYFUND_AMOUNT_V2].bit = 16;
        consensus.vDeployments[Consensus::DEPLOYMENT_COMMUNITYFUND_AMOUNT_V2].nStartTime = 1533081600; // Aug 1st, 2018
        consensus.vDeployments[Consensus::DEPLOYMENT_COMMUNITYFUND_AMOUNT_V2].nTimeout = 1564617600; // Aug 1st, 2019

        // Deployment of Static Reward
        consensus.vDeployments[Consensus::DEPLOYMENT_STATIC_REWARD].bit = 15;
        consensus.vDeployments[Consensus::DEPLOYMENT_STATIC_REWARD].nStartTime = 1533081600; // August 1st, 2018
        consensus.vDeployments[Consensus::DEPLOYMENT_STATIC_REWARD].nTimeout = 1564617600; // August 1st, 2019

        // Deployment of Quorum reduction for the Community Fund
        consensus.vDeployments[Consensus::DEPLOYMENT_QUORUM_CFUND].bit = 17;
        consensus.vDeployments[Consensus::DEPLOYMENT_QUORUM_CFUND].nStartTime = 1543622400; // Dec 1st, 2018
        consensus.vDeployments[Consensus::DEPLOYMENT_QUORUM_CFUND].nTimeout = 1575158400; // Dec 1st, 2019

        /**
         * The message start string is designed to be unlikely to occur in normal data.
         * The characters are rarely used upper ASCII, not valid as UTF-8, and produce
         * a large 32-bit integer with any alignment.
         */
        pchMessageStart[0] = 0x7d;
        pchMessageStart[1] = 0x11;
        pchMessageStart[2] = 0xb7;
        pchMessageStart[3] = 0x89;
        nDefaultPort = 18886;
        nPruneAfterHeight = 1000;
        bnProofOfWorkLimit = arith_uint256(~arith_uint256() >> 16);

        uint32_t nTimestamp = GetTimeNow();
        uint256 hashGenesisBlock = uint256S("0x0000e01b12644af6917e5aada637a609dd9590ad6bdc4828cd8df95258d85c02");
        uint256 hashMerkleRoot = uint256S("0x2d9101b87fe7b9deaea41849c1f3bed71e060739147802a238fe968f75ad0fd9");
        uint32_t nNonce = 2043184832;

        genesis = CreateGenesisBlockTestnet(nTimestamp, nNonce, 0x1d00ffff, 1, 0);
        consensus.hashGenesisBlock = genesis.GetHash();

        if ((genesis.GetHash() != hashGenesisBlock || genesis.hashMerkleRoot != hashMerkleRoot))
        {
            nTimestamp = GetTimeNow();
            // deliberately empty for loop finds nonce value.
            for(; genesis.GetHash() > consensus.powLimit; genesis.nNonce++){ }
            hashGenesisBlock = genesis.GetHash();
            nNonce = genesis.nNonce;
            hashMerkleRoot = genesis.hashMerkleRoot;
        }

        vSeeds.push_back(CDNSSeedData("testnav.community", "testseed.nav.community"));
        vSeeds.push_back(CDNSSeedData("testnavcoin.org", "testseed.navcoin.org"));

        consensus.hashGenesisBlock = genesis.GetHash();

        assert(consensus.hashGenesisBlock == hashGenesisBlock);
        assert(genesis.hashMerkleRoot == hashMerkleRoot);

        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,111);
        base58Prefixes[COLDSTAKING_ADDRESS] = std::vector<unsigned char>(1,63); // cold staking addresses start with 'S'
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,196);
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,239);
        base58Prefixes[EXT_PUBLIC_KEY] = boost::assign::list_of(0x40)(0x88)(0x2B)(0xE1).convert_to_container<std::vector<unsigned char> >();
        base58Prefixes[EXT_SECRET_KEY] = boost::assign::list_of(0x40)(0x88)(0xDA)(0x4E).convert_to_container<std::vector<unsigned char> >();

        vFixedSeeds = std::vector<SeedSpec6>(pnSeed6_test, pnSeed6_test + ARRAYLEN(pnSeed6_test));

        fMiningRequiresPeers = false;
        fDefaultConsistencyChecks = false;
        fRequireStandard = false;
        fMineBlocksOnDemand = true;
        fTestnetToBeDeprecatedFieldRPC = true;

        checkpointData = (CCheckpointData) {
            boost::assign::map_list_of
            ( 0,         hashGenesisBlock),
            nTimestamp, // * UNIX timestamp of last checkpoint block
            0,          // * total number of transactions between genesis and last checkpoint
                        //   (the tx=... number in the SetBestChain debug.log lines)
            7000        // * estimated number of transactions per day after checkpoint
        };
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
    else if (chain == CBaseChainParams::DEVNET)
            return devNetParams;
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
