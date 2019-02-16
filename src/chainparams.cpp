// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Copyright (c) 2018 The Navcoin Core developers
// Copyright (c) 2019 The DeVault Core developers
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
    txNew.strDZeel = "DeVault genesis block";

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
        consensus.powLimit = ~uint256(0) >> 16;
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
        consensus.nCyclesProposalVoting = 6;
        consensus.nCyclesPaymentRequestVoting = 8;
        consensus.nPaymentRequestMaxVersion = 3;
        consensus.nProposalMaxVersion = 3;
        consensus.nMaxFutureDrift = 60;
        consensus.nHeightv452Fork = 2882875;

        /** Zerocoin */
        consensus.zerocoinModulus = "25195908475657893494027183240048398571429282126204032027777137836043662020707595556264018525880784"
                                    "4069182906412495150821892985591491761845028084891200728449926873928072877767359714183472702618963750149718246911"
                                    "6507761337985909570009733045974880842840179742910064245869181719511874612151517265463228221686998754918242243363"
                                    "7259085141865462043576798423387184774447920739934236584823824281198163815010674810451660377306056201619676256133"
                                    "8441436038339044149526344321901146575444541784240209246165157233507787077498171257724679629263863563732899121548"
                                    "31438167899885040445364023527381951378636564391212010397122822120720357";

        CBigNum bnModulus;
        bnModulus.SetDec(consensus.zerocoinModulus);
        consensus.Zerocoin_Params = libzerocoin::ZerocoinParams(bnModulus);

        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = 1199145601; // January 1, 2008
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = 1230767999; // December 31, 2008

        // Deployment of BIP68, BIP112, and BIP113.
        // Deployment of SegWit (BIP141 and BIP143)
        // Deployment of Community Fund
        // Deployment of Cold Staking
        // Deployment of SegWit (BIP141 and BIP143)
        // Deployment of Community Fund Accumulation
        // Deployment of Community Fund Accumulation Spread(NPIP-0003)
        // Increate in Community Fund Accumulation Ammonut (NPIP-0004)
        // Deployment of NTP Sync

        // Deployment of Zerocoin
        consensus.vDeployments[Consensus::DEPLOYMENT_ZEROCOIN].bit = 18;
        consensus.vDeployments[Consensus::DEPLOYMENT_ZEROCOIN].nStartTime = 1538352000; // May 1st, 2018
        consensus.vDeployments[Consensus::DEPLOYMENT_ZEROCOIN].nTimeout = 1601510400; // May 1st, 2019

        // Deployment of Quorum reduction for the Community Fund
        /**
         * The message start string is designed to be unlikely to occur in normal data.
         * The characters are rarely used upper ASCII, not valid as UTF-8, and produce
         * a large 32-bit integer with any alignment.
         */
        pchMessageStart[0] = 0xc0;
        pchMessageStart[1] = 0xd2;
        pchMessageStart[2] = 0xe0;
        pchMessageStart[3] = 0xd1;
        nDefaultPort = 33039;
        nPruneAfterHeight = 100000;
        bnProofOfWorkLimit = ~uint256() >> 16;

        genesis = CreateGenesisBlock(1460561040, 6945, 0x1f00ffff, 1, 0);

	      consensus.hashGenesisBlock = genesis.GetHash();

        assert(consensus.hashGenesisBlock == uint256S("0x00006a4e3e18c71c6d48ad6c261e2254fa764cf29607a4357c99b712dfbb8e6a"));
        assert(genesis.hashMerkleRoot == uint256S("0xc507eec6ccabfd5432d764afceafba42d2d946594b8a60570cb2358a7392c61a"));

        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,31); // D standard address prefix
        base58Prefixes[COLDSTAKING_ADDRESS] = std::vector<unsigned char>(1,21); // cold staking addresses start with 'X'
        base58Prefixes[PRIVATE_ADDRESS] = boost::assign::list_of(74)(7).convert_to_container<std::vector<unsigned char> >(); // Starts with ZN
        base58Prefixes[PRIVATE_SPEND_KEY] = std::vector<unsigned char>(2,29); //
        base58Prefixes[PRIVATE_VIEW_KEY] = std::vector<unsigned char>(2,32); //
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,90); // d script address prefix
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,193); // V privkey prefix
        base58Prefixes[EXT_PUBLIC_KEY] = boost::assign::list_of(0x04)(0x88)(0xB2)(0x1E).convert_to_container<std::vector<unsigned char> >();
        base58Prefixes[EXT_SECRET_KEY] = boost::assign::list_of(0x04)(0x88)(0xAD)(0xE4).convert_to_container<std::vector<unsigned char> >();

        vSeeds.push_back(CDNSSeedData("nav.community", "seed.nav.community"));
        vSeeds.push_back(CDNSSeedData("devault.org", "seed.devault.org"));

        vFixedSeeds = std::vector<SeedSpec6>(pnSeed6_main, pnSeed6_main + ARRAYLEN(pnSeed6_main));

        fMiningRequiresPeers = true;
        fDefaultConsistencyChecks = false;
        fRequireStandard = true;
        fMineBlocksOnDemand = false;
        fTestnetToBeDeprecatedFieldRPC = false;

        checkpointData = (CCheckpointData) {
            boost::assign::map_list_of
            ( 0, uint256S("0x00006a4e3e18c71c6d48ad6c261e2254fa764cf29607a4357c99b712dfbb8e6a")),
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
        consensus.powLimit = ~uint256(0) >> 16;
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
        consensus.nCommunityFundAmount = 0.5 * COIN;
        consensus.nCyclesProposalVoting = 4;
        consensus.nCyclesPaymentRequestVoting = 4;
        consensus.nPaymentRequestMaxVersion = 3;
        consensus.nProposalMaxVersion = 3;
        consensus.nMaxFutureDrift = 60;

        /** Zerocoin */
        consensus.zerocoinModulus = "25195908475657893494027183240048398571429282126204032027777137836043662020707595556264018525880784"
                                    "4069182906412495150821892985591491761845028084891200728449926873928072877767359714183472702618963750149718246911"
                                    "6507761337985909570009733045974880842840179742910064245869181719511874612151517265463228221686998754918242243363"
                                    "7259085141865462043576798423387184774447920739934236584823824281198163815010674810451660377306056201619676256133"
                                    "8441436038339044149526344321901146575444541784240209246165157233507787077498171257724679629263863563732899121548"
                                    "31438167899885040445364023527381951378636564391212010397122822120720357";

        CBigNum bnModulus;
        bnModulus.SetDec(consensus.zerocoinModulus);
        consensus.Zerocoin_Params = libzerocoin::ZerocoinParams(bnModulus);

        consensus.nHeightv452Fork = 100000;

        // Deployment of BIP68, BIP112, and BIP113.
        // Deployment of Cold Staking
        // Deployment of SegWit (BIP141 and BIP143)
        // Deployment of Community Fund
        // Deployment of Community Fund Accumulation
        // Deployment of Community Fund Accumulation Spread(NPIP-0003)
        // Increate in Community Fund Accumulation Ammonut (NPIP-0004)
        // Deployment of NTP Sync

        // Deployment of Zerocoin
        consensus.vDeployments[Consensus::DEPLOYMENT_ZEROCOIN].bit = 18;
        consensus.vDeployments[Consensus::DEPLOYMENT_ZEROCOIN].nStartTime = 1538352000; // May 1st, 2018
        consensus.vDeployments[Consensus::DEPLOYMENT_ZEROCOIN].nTimeout = 1601510400; // May 1st, 2019

        // Deployment of Quorum reduction for the Community Fund
        /**
         * The message start string is designed to be unlikely to occur in normal data.
         * The characters are rarely used upper ASCII, not valid as UTF-8, and produce
         * a large 32-bit integer with any alignment.
         */
        pchMessageStart[0] = 0x0d;
        pchMessageStart[1] = 0x08;
        pchMessageStart[2] = 0x13;
        pchMessageStart[3] = 0x04;
        nDefaultPort = 39039;
        nPruneAfterHeight = 1000;
        bnProofOfWorkLimit = ~uint256() >> 16;
    
        uint32_t nTimestamp = 1545341312;
        uint256 hashGenesisBlock = uint256S("0x0000a2ed763c6efc24bbb3ac8d9f1ab9e8f1e7100d5221ad80815cd7b369dc2c");
        uint256 hashMerkleRoot = uint256S("0x02128838f2516796eb04f5b3fd143a7786001301dc5ffcfd2b2c687a2864aae9");
        uint32_t nNonce = 2043585747;
	    
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
        vSeeds.push_back(CDNSSeedData("testdevault.org", "testseed.devault.org"));

        assert(consensus.hashGenesisBlock == hashGenesisBlock);
        assert(genesis.hashMerkleRoot == hashMerkleRoot);

        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,66); // T address prefix
        base58Prefixes[COLDSTAKING_ADDRESS] = std::vector<unsigned char>(1,8); // cold staking addresses start with 'C/D'
        base58Prefixes[PRIVATE_ADDRESS] = boost::assign::list_of(74)(64).convert_to_container<std::vector<unsigned char> >(); // Starts with ZT
        base58Prefixes[PRIVATE_SPEND_KEY] = std::vector<unsigned char>(2,29); //
        base58Prefixes[PRIVATE_VIEW_KEY] = std::vector<unsigned char>(2,32); //
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,127); // t script address prefix
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,212); // Y privkey prefix
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
        consensus.powLimit = ~uint256(0) >> 16;
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
        consensus.nCyclesProposalVoting = 4;
        consensus.nCyclesPaymentRequestVoting = 4;
        consensus.nPaymentRequestMaxVersion = 3;
        consensus.nProposalMaxVersion = 3;
        consensus.nMaxFutureDrift = 60000;
        consensus.nHeightv452Fork = 1000;

        /** Zerocoin */
        consensus.zerocoinModulus = "25195908475657893494027183240048398571429282126204032027777137836043662020707595556264018525880784"
                                    "4069182906412495150821892985591491761845028084891200728449926873928072877767359714183472702618963750149718246911"
                                    "6507761337985909570009733045974880842840179742910064245869181719511874612151517265463228221686998754918242243363"
                                    "7259085141865462043576798423387184774447920739934236584823824281198163815010674810451660377306056201619676256133"
                                    "8441436038339044149526344321901146575444541784240209246165157233507787077498171257724679629263863563732899121548"
                                    "31438167899885040445364023527381951378636564391212010397122822120720357";

        CBigNum bnModulus;
        bnModulus.SetDec(consensus.zerocoinModulus);
        consensus.Zerocoin_Params = libzerocoin::ZerocoinParams(bnModulus);

        // Deployment of BIP68, BIP112, and BIP113.
        // Deployment of Cold Staking
        // Deployment of SegWit (BIP141 and BIP143)
        // Deployment of Community Fund
        // Deployment of NTP Sync
        // Deployment of Community Fund Accumulation
        // Deployment of Community Fund Accumulation Spread(NPIP-0003)
        // Increate in Community Fund Accumulation Ammonut (NPIP-0004)

         // Deployment of Zerocoin
        consensus.vDeployments[Consensus::DEPLOYMENT_ZEROCOIN].bit = 18;
        consensus.vDeployments[Consensus::DEPLOYMENT_ZEROCOIN].nStartTime = 1538352000; // May 1st, 2018
        consensus.vDeployments[Consensus::DEPLOYMENT_ZEROCOIN].nTimeout = 1601510400; // May 1st, 2019

        // Deployment of Quorum reduction for the Community Fund

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
        bnProofOfWorkLimit = ~uint256() >> 16;

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
        // 6) Generate first block using RPC command "./devault-cli generate 1"

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
        vSeeds.push_back(CDNSSeedData("devnet.devault.org", "devseed.devault.org"));

        assert(consensus.hashGenesisBlock == hashGenesisBlock);
        assert(genesis.hashMerkleRoot == hashMerkleRoot);

        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,111);
        base58Prefixes[COLDSTAKING_ADDRESS] = std::vector<unsigned char>(1,63); // cold staking addresses start with 'S'
        base58Prefixes[PRIVATE_ADDRESS] = boost::assign::list_of(74)(37).convert_to_container<std::vector<unsigned char> >(); // Starts with ZR
        base58Prefixes[PRIVATE_SPEND_KEY] = std::vector<unsigned char>(2,29); //
        base58Prefixes[PRIVATE_VIEW_KEY] = std::vector<unsigned char>(2,32); //
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
        consensus.powLimit = ~uint256(0) >> 1;
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
        consensus.nCyclesProposalVoting = 4;
        consensus.nCyclesPaymentRequestVoting = 4;
        consensus.nPaymentRequestMaxVersion = 3;
        consensus.nProposalMaxVersion = 3;
        consensus.nMaxFutureDrift = 60000;
        consensus.nHeightv452Fork = 1000;

        /** Zerocoin */
        consensus.zerocoinModulus = "25195908475657893494027183240048398571429282126204032027777137836043662020707595556264018525880784"
                                    "4069182906412495150821892985591491761845028084891200728449926873928072877767359714183472702618963750149718246911"
                                    "6507761337985909570009733045974880842840179742910064245869181719511874612151517265463228221686998754918242243363"
                                    "7259085141865462043576798423387184774447920739934236584823824281198163815010674810451660377306056201619676256133"
                                    "8441436038339044149526344321901146575444541784240209246165157233507787077498171257724679629263863563732899121548"
                                    "31438167899885040445364023527381951378636564391212010397122822120720357";

        CBigNum bnModulus;
        bnModulus.SetDec(consensus.zerocoinModulus);
        consensus.Zerocoin_Params = libzerocoin::ZerocoinParams(bnModulus);

        // Deployment of BIP68, BIP112, and BIP113.
        // Deployment of Cold Staking
        // Deployment of SegWit (BIP141 and BIP143)
        // Deployment of Community Fund
        // Deployment of Community Fund Accumulation
        // Deployment of NTP Sync
        // Deployment of Community Fund Accumulation Spread(NPIP-0003)
        // Increate in Community Fund Accumulation Ammonut (NPIP-0004)

        // Deployment of Zerocoin
        consensus.vDeployments[Consensus::DEPLOYMENT_ZEROCOIN].bit = 18;
        consensus.vDeployments[Consensus::DEPLOYMENT_ZEROCOIN].nStartTime = 1538352000; // May 1st, 2018
        consensus.vDeployments[Consensus::DEPLOYMENT_ZEROCOIN].nTimeout = 1601510400; // May 1st, 2019

        // Deployment of Quorum reduction for the Community Fund

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
        bnProofOfWorkLimit = ~uint256() >> 16;

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
        vSeeds.push_back(CDNSSeedData("testdevault.org", "testseed.devault.org"));

        consensus.hashGenesisBlock = genesis.GetHash();

        assert(consensus.hashGenesisBlock == hashGenesisBlock);
        assert(genesis.hashMerkleRoot == hashMerkleRoot);

        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,111);
        base58Prefixes[COLDSTAKING_ADDRESS] = std::vector<unsigned char>(1,63); // cold staking addresses start with 'S'
        base58Prefixes[PRIVATE_ADDRESS] = boost::assign::list_of(74)(37).convert_to_container<std::vector<unsigned char> >(); // Starts with ZR
        base58Prefixes[PRIVATE_SPEND_KEY] = std::vector<unsigned char>(2,29); //
        base58Prefixes[PRIVATE_VIEW_KEY] = std::vector<unsigned char>(2,32); //
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
