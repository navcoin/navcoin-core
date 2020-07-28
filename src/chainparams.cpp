// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Copyright (c) 2017-2018 The NavCoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainparams.h>
#include <consensus/dao.h>
#include <consensus/merkle.h>

#include <streams.h>
#include <tinyformat.h>
#include <util.h>
#include <utilstrencodings.h>

#include <assert.h>

#include <boost/assign/list_of.hpp>

#include <chainparamsseeds.h>

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
    txNew.strDZeel = "NavBS genesis block";

    CBlock genesis;
    genesis.nTime = nTime;
    genesis.nBits = nBits;
    genesis.nNonce = nNonce;
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

class CMainParams : public CChainParams
{
public:
    CMainParams()
    {
        strNetworkID = "main";
        consensus.nSubsidyHalvingInterval = 420000; // 210000 => 420000
        consensus.nMajorityEnforceBlockUpgrade = 750;
        consensus.nMajorityRejectBlockOutdated = 950;
        consensus.nMajorityWindow = 1000;
        consensus.BIP34Height = 900000;
        consensus.BIP34Hash = uint256S("0xecb7444214d068028ec1fa4561662433452c1cbbd6b0f8eeb6452bcfa1d0a7d6");
        consensus.powLimit = ArithToUint256(~arith_uint256(0) >> 16);
        consensus.nPowTargetTimespan = 10; // время, для перепроверки 30 => 10
        consensus.nPowTargetSpacing = 10;  // время блока
        consensus.fPowAllowMinDifficultyBlocks = false;
        consensus.fPowNoRetargeting = false;
        consensus.nRuleChangeActivationThreshold = 15120; // 75% of 20160
        consensus.nMinerConfirmationWindow = 20160;
        consensus.nStakeMinAge = 10;   // minimum for coin age: 10 sec
        consensus.nTargetSpacing = 30; // Blocktime: 30 sec
        consensus.nStakeCombineThreshold = 1000 * COIN;
        consensus.nStakeSplitThreshold = 2 * consensus.nStakeCombineThreshold;
        consensus.nDailyBlockCount = (24 * 60 * 60) / consensus.nTargetSpacing;
        consensus.nModifierInterval = 10 * 60; // time to elapse before new modifier is computed
        consensus.nTargetTimespan = 25 * 30;
        consensus.nLastPOWBlock = 140; // last PoW then PoS 
        consensus.nCoinbaseTimeActivationHeight = 140;
        consensus.nMinimumQuorumFirstHalf = 0.5;
        consensus.nMinimumQuorumSecondHalf = 0.4;
        consensus.nCommunityFundMinAge = 50;
        consensus.sigActivationTime = 1512990000;
        consensus.nCommunityFundAmount = 0.25 * COIN;
        consensus.nPaymentRequestMaxVersion = CPaymentRequest::ALL_VERSION;
        consensus.nProposalMaxVersion = CProposal::ALL_VERSION;
        consensus.nConsultationMaxVersion = CConsultation::ALL_VERSION;
        consensus.nConsultationAnswerMaxVersion = CConsultationAnswer::ALL_VERSION;
        consensus.nMaxFutureDrift = 60;
        consensus.nHeightv451Fork = 2722100;
        consensus.nHeightv452Fork = 2882875;
        consensus.fDaoClientActivated = true;

        /** Coinbase transaction outputs can only be spent after this number of new blocks (network rule) */
        consensus.nCoinbaseMaturity = 50;
        consensus.nConsensusChangeMinAccept = 7500;

        consensus.vParameters[Consensus::CONSENSUS_PARAM_VOTING_CYCLE_LENGTH].value = 2880 * 7; // 7 Days
        consensus.vParameters[Consensus::CONSENSUS_PARAM_PROPOSAL_MIN_QUORUM].value = 5000;
        consensus.vParameters[Consensus::CONSENSUS_PARAM_PAYMENT_REQUEST_MIN_QUORUM].value = 5000;
        consensus.vParameters[Consensus::CONSENSUS_PARAM_CONSULTATION_MIN_SUPPORT].value = 150;
        consensus.vParameters[Consensus::CONSENSUS_PARAM_CONSULTATION_ANSWER_MIN_SUPPORT].value = 150;
        consensus.vParameters[Consensus::CONSENSUS_PARAM_CONSULTATION_MIN_CYCLES].value = 2;
        consensus.vParameters[Consensus::CONSENSUS_PARAM_PROPOSAL_MIN_ACCEPT].value = 7000;
        consensus.vParameters[Consensus::CONSENSUS_PARAM_PROPOSAL_MIN_REJECT].value = 7000;
        consensus.vParameters[Consensus::CONSENSUS_PARAM_PAYMENT_REQUEST_MIN_ACCEPT].value = 7000;
        consensus.vParameters[Consensus::CONSENSUS_PARAM_PAYMENT_REQUEST_MIN_REJECT].value = 7000;
        consensus.vParameters[Consensus::CONSENSUS_PARAM_PROPOSAL_MIN_FEE].value = 5000000000;
        consensus.vParameters[Consensus::CONSENSUS_PARAM_PAYMENT_REQUEST_MIN_FEE].value = 0;
        consensus.vParameters[Consensus::CONSENSUS_PARAM_CONSULTATION_MIN_FEE].value = 10000000000;
        consensus.vParameters[Consensus::CONSENSUS_PARAM_CONSULTATION_ANSWER_MIN_FEE].value = 5000000000;
        consensus.vParameters[Consensus::CONSENSUS_PARAM_FUND_SPREAD_ACCUMULATION].value = 500;
        consensus.vParameters[Consensus::CONSENSUS_PARAM_FUND_PERCENT_PER_BLOCK].value = 2000;
        consensus.vParameters[Consensus::CONSENSUS_PARAM_PROPOSAL_MAX_VOTING_CYCLES].value = 6;
        consensus.vParameters[Consensus::CONSENSUS_PARAM_PAYMENT_REQUEST_MAX_VOTING_CYCLES].value = 8;
        consensus.vParameters[Consensus::CONSENSUS_PARAM_CONSULTATION_MAX_VOTING_CYCLES].value = 4;
        consensus.vParameters[Consensus::CONSENSUS_PARAM_CONSULTATION_MAX_SUPPORT_CYCLES].value = 4;
        consensus.vParameters[Consensus::CONSENSUS_PARAM_CONSULTATION_REFLECTION_LENGTH].value = 1;
        consensus.vParameters[Consensus::CONSENSUS_PARAM_GENERATION_PER_BLOCK].value = 2.5 * COIN;
        consensus.vParameters[Consensus::CONSENSUS_PARAM_NAVNS_FEE].value = 100 * COIN;
        consensus.vParameters[Consensus::CONSENSUS_PARAMS_DAO_VOTE_LIGHT_MIN_FEE].value = 0.1 * COIN;

        

        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = 1199145601; // January 1, 2008
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = 1230767999;   // December 31, 2008

        // Deployment of BIP68, BIP112, and BIP113.
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].bit = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nStartTime = 1462060800; // May 1st, 2016
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nTimeout = 1525132800;   // May 1st, 2017

        // Deployment of SegWit (BIP141 and BIP143)
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT_LEGACY].bit = 1;
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT_LEGACY].nStartTime = 1493424000; // May 1st, 2017
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT_LEGACY].nTimeout = 1525132800;   // May 1st, 2018

        consensus.vDeployments[Consensus::DEPLOYMENT_CSV_LEGACY].bit = 2;
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV_LEGACY].nStartTime = 1462060800; // May 1st, 2016
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV_LEGACY].nTimeout = 1525132800;   // May 1st, 2018

        // Deployment of Cold Staking
        consensus.vDeployments[Consensus::DEPLOYMENT_COLDSTAKING].bit = 3;
        consensus.vDeployments[Consensus::DEPLOYMENT_COLDSTAKING].nStartTime = 1525132800; // May 1st, 2018
        consensus.vDeployments[Consensus::DEPLOYMENT_COLDSTAKING].nTimeout = 1556712000;   // May 1st, 2019

        // Deployment of SegWit (BIP141 and BIP143)
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].bit = 4;
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].nStartTime = 1493424000; // May 1st, 2017
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].nTimeout = 1525132800;   // May 1st, 2018

        // Deployment of Community Fund
        consensus.vDeployments[Consensus::DEPLOYMENT_COMMUNITYFUND].bit = 6;
        consensus.vDeployments[Consensus::DEPLOYMENT_COMMUNITYFUND].nStartTime = 1493424000; // May 1st, 2017
        consensus.vDeployments[Consensus::DEPLOYMENT_COMMUNITYFUND].nTimeout = 1556668800;   // May 1st, 2019

        // Deployment of Community Fund Accumulation
        consensus.vDeployments[Consensus::DEPLOYMENT_COMMUNITYFUND_ACCUMULATION].bit = 7;
        consensus.vDeployments[Consensus::DEPLOYMENT_COMMUNITYFUND_ACCUMULATION].nStartTime = 1525132800; // May 1st, 2018
        consensus.vDeployments[Consensus::DEPLOYMENT_COMMUNITYFUND_ACCUMULATION].nTimeout = 1556712000;   // May 1st, 2019

        // Deployment of NTP Sync
        consensus.vDeployments[Consensus::DEPLOYMENT_NTPSYNC].bit = 8;
        consensus.vDeployments[Consensus::DEPLOYMENT_NTPSYNC].nStartTime = 1525132800; // May 1st, 2018
        consensus.vDeployments[Consensus::DEPLOYMENT_NTPSYNC].nTimeout = 1556712000;   // May 1st, 2019

        // Deployment of Community Fund Accumulation Spread(NPIP-0003)
        consensus.vDeployments[Consensus::DEPLOYMENT_COMMUNITYFUND_ACCUMULATION_SPREAD].bit = 14;
        consensus.vDeployments[Consensus::DEPLOYMENT_COMMUNITYFUND_ACCUMULATION_SPREAD].nStartTime = 1525132800; // May 1st, 2018
        consensus.vDeployments[Consensus::DEPLOYMENT_COMMUNITYFUND_ACCUMULATION_SPREAD].nTimeout = 1556712000;   // May 1st, 2019

        // Increate in Community Fund Accumulation Ammonut (NPIP-0004)
        consensus.vDeployments[Consensus::DEPLOYMENT_COMMUNITYFUND_AMOUNT_V2].bit = 16;
        consensus.vDeployments[Consensus::DEPLOYMENT_COMMUNITYFUND_AMOUNT_V2].nStartTime = 1533081600; // Aug 1st, 2018
        consensus.vDeployments[Consensus::DEPLOYMENT_COMMUNITYFUND_AMOUNT_V2].nTimeout = 1564617600;   // Aug 1st, 2019

        // Deployment of Static Reward
        consensus.vDeployments[Consensus::DEPLOYMENT_STATIC_REWARD].bit = 15;
        consensus.vDeployments[Consensus::DEPLOYMENT_STATIC_REWARD].nStartTime = 1533081600; // August 1st, 2018
        consensus.vDeployments[Consensus::DEPLOYMENT_STATIC_REWARD].nTimeout = 1564617600;   // August 1st, 2019

        // Deployment of Quorum reduction for the Community Fund
        consensus.vDeployments[Consensus::DEPLOYMENT_QUORUM_CFUND].bit = 17;
        consensus.vDeployments[Consensus::DEPLOYMENT_QUORUM_CFUND].nStartTime = 1543622400; // Dec 1st, 2018
        consensus.vDeployments[Consensus::DEPLOYMENT_QUORUM_CFUND].nTimeout = 1575158400;   // Dec 1st, 2019

        // Deployment of VOTING STATE CACHE for the Community Fund
        consensus.vDeployments[Consensus::DEPLOYMENT_VOTE_STATE_CACHE].bit = 22;
        consensus.vDeployments[Consensus::DEPLOYMENT_VOTE_STATE_CACHE].nStartTime = 1559390400; // Jun 1st, 2019
        consensus.vDeployments[Consensus::DEPLOYMENT_VOTE_STATE_CACHE].nTimeout = 1622548800;   // Jun 1st, 2021

        // Deployment of CONSULTATIONS for the Community Fund
        consensus.vDeployments[Consensus::DEPLOYMENT_CONSULTATIONS].bit = 23;
        consensus.vDeployments[Consensus::DEPLOYMENT_CONSULTATIONS].nStartTime = 1559390400; // Jun 1st, 2019
        consensus.vDeployments[Consensus::DEPLOYMENT_CONSULTATIONS].nTimeout = 1622548800;   // Jun 1st, 2021

        // Deployment of DAO CONSENSUS
        consensus.vDeployments[Consensus::DEPLOYMENT_DAO_CONSENSUS].bit = 25;
        consensus.vDeployments[Consensus::DEPLOYMENT_DAO_CONSENSUS].nStartTime = 1559390400; // Jun 1st, 2019
        consensus.vDeployments[Consensus::DEPLOYMENT_DAO_CONSENSUS].nTimeout = 1622548800;   // Jun 1st, 2021

        // Deployment of Cold Staking
        consensus.vDeployments[Consensus::DEPLOYMENT_COLDSTAKING_V2].bit = 27;
        consensus.vDeployments[Consensus::DEPLOYMENT_COLDSTAKING_V2].nStartTime = 1559390400; // Jun 1st, 2019
        consensus.vDeployments[Consensus::DEPLOYMENT_COLDSTAKING_V2].nTimeout = 1622548800;   // Jun 1st, 2021

        // Deployment of Cold Staking Pool Fee
        consensus.vDeployments[Consensus::DEPLOYMENT_POOL_FEE].bit = 18;
        consensus.vDeployments[Consensus::DEPLOYMENT_POOL_FEE].nStartTime = 1559390400; // Jun 1st, 2018
        consensus.vDeployments[Consensus::DEPLOYMENT_POOL_FEE].nTimeout = 1622548800;   // Jun 1st, 2021


        /**
         * The message start string is designed to be unlikely to occur in normal data.
         * The characters are rarely used upper ASCII, not valid as UTF-8, and produce
         * a large 32-bit integer with any alignment.
         */
        pchMessageStart[0] = 0x7f;
        pchMessageStart[1] = 0x98;
        pchMessageStart[2] = 0xaf;
        pchMessageStart[3] = 0xbb;
        nDefaultPort = 8434;
        nPruneAfterHeight = 100000;
        bnProofOfWorkLimit = arith_uint256(~arith_uint256() >> 16);

        genesis = CreateGenesisBlock(1460561040, 6945, 0x1f00ffff, 1, 0);

        consensus.hashGenesisBlock = genesis.GetHash();

        assert(consensus.hashGenesisBlock == uint256S("0x00006a4e3e18c71c6d48ad6c261e2254fa764cf29607a4357c99b712dfbb8e6a"));
        assert(genesis.hashMerkleRoot == uint256S("0xc507eec6ccabfd5432d764afceafba42d2d946594b8a60570cb2358a7392c61a"));

        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1, 53);
        base58Prefixes[COLDSTAKING_ADDRESS] = std::vector<unsigned char>(1, 21); // cold staking addresses start with 'X'
        base58Prefixes[COLDSTAKING_ADDRESS_V2] = std::vector<unsigned char>(1, 36);
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1, 85);
        base58Prefixes[RAW_SCRIPT_ADDRESS] = std::vector<unsigned char>(1, 60);
        base58Prefixes[SECRET_KEY] = std::vector<unsigned char>(1, 150);
        base58Prefixes[EXT_PUBLIC_KEY] = boost::assign::list_of(0x04)(0x88)(0xB2)(0x1E).convert_to_container<std::vector<unsigned char> >();
        base58Prefixes[EXT_SECRET_KEY] = boost::assign::list_of(0x04)(0x88)(0xAD)(0xE4).convert_to_container<std::vector<unsigned char> >();

        // vSeeds.push_back(CDNSSeedData("seed 1 nav.community", "seed.nav.community"));
        // vSeeds.push_back(CDNSSeedData("seed 2 nav.community", "seed2.nav.community"));

        vFixedSeeds = std::vector<SeedSpec6>(pnSeed6_main, pnSeed6_main + ARRAYLEN(pnSeed6_main));

        fMiningRequiresPeers = true;
        fDefaultConsistencyChecks = false;
        fRequireStandard = true;
        fMineBlocksOnDemand = false;
        fTestnetToBeDeprecatedFieldRPC = false;

        checkpointData = {};
    }
};
static CMainParams mainParams;

/**
 * Testnet (v3)
 */
class CTestNetParams : public CChainParams
{
public:
    CTestNetParams()
    {
        strNetworkID = "test";
        consensus.nSubsidyHalvingInterval = 210000;
        consensus.nMajorityEnforceBlockUpgrade = 750;
        consensus.nMajorityRejectBlockOutdated = 950;
        consensus.nMajorityWindow = 1000;
        consensus.BIP34Height = 900000;
        consensus.BIP34Hash = uint256S("0xecb7444214d068028ec1fa4561662433452c1cbbd6b0f8eeb6452bcfa1d0a7d6");
        consensus.powLimit = ArithToUint256(~arith_uint256(0) >> 16);
        consensus.nPowTargetTimespan = 30; // 10 sec; //
        consensus.nPowTargetSpacing = 30;
        consensus.fPowAllowMinDifficultyBlocks = false;
        consensus.fPowNoRetargeting = false;
        consensus.nRuleChangeActivationThreshold = 300; // 75% of 400
        consensus.nMinerConfirmationWindow = 400;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = 1199145601; // January 1, 2008
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = 1230767999;   // December 31, 2008
        consensus.nStakeMinAge = 2;                                                      // minimum for coin age: 2 seconds
        consensus.nTargetSpacing = 30;                                                   // Blocktime: 30 secs
        consensus.nStakeCombineThreshold = 1000 * COIN;
        consensus.nStakeSplitThreshold = 2 * consensus.nStakeCombineThreshold;
        consensus.nDailyBlockCount = (24 * 60 * 60) / consensus.nTargetSpacing;
        consensus.nModifierInterval = 10 * 60; // time to elapse before new modifier is computed
        consensus.nTargetTimespan = 25 * 30;
        consensus.nLastPOWBlock = 1000000;
        consensus.nMinimumQuorumFirstHalf = 0.5;
        consensus.nMinimumQuorumSecondHalf = 0.4;
        consensus.nCommunityFundMinAge = 5;
        consensus.sigActivationTime = 1512826692;
        consensus.nCoinbaseTimeActivationHeight = 30000;
        consensus.nCommunityFundAmount = 0.25 * COIN;
        consensus.nPaymentRequestMaxVersion = CPaymentRequest::ALL_VERSION;
        consensus.nProposalMaxVersion = CProposal::ALL_VERSION;
        consensus.nConsultationMaxVersion = CConsultation::ALL_VERSION;
        consensus.nConsultationAnswerMaxVersion = CConsultationAnswer::ALL_VERSION;
        consensus.nMaxFutureDrift = 60;
        consensus.nHeightv451Fork = 100000;
        consensus.nHeightv452Fork = 100000;
        consensus.fDaoClientActivated = true;

        consensus.nConsensusChangeMinAccept = 7500;

        consensus.vParameters[Consensus::CONSENSUS_PARAM_VOTING_CYCLE_LENGTH].value = 180;
        consensus.vParameters[Consensus::CONSENSUS_PARAM_PROPOSAL_MIN_QUORUM].value = 5000;
        consensus.vParameters[Consensus::CONSENSUS_PARAM_PAYMENT_REQUEST_MIN_QUORUM].value = 5000;
        consensus.vParameters[Consensus::CONSENSUS_PARAM_CONSULTATION_MIN_SUPPORT].value = 150;
        consensus.vParameters[Consensus::CONSENSUS_PARAM_CONSULTATION_ANSWER_MIN_SUPPORT].value = 150;
        consensus.vParameters[Consensus::CONSENSUS_PARAM_CONSULTATION_MIN_CYCLES].value = 2;
        consensus.vParameters[Consensus::CONSENSUS_PARAM_PROPOSAL_MIN_ACCEPT].value = 7000;
        consensus.vParameters[Consensus::CONSENSUS_PARAM_PROPOSAL_MIN_REJECT].value = 7000;
        consensus.vParameters[Consensus::CONSENSUS_PARAM_PAYMENT_REQUEST_MIN_ACCEPT].value = 7000;
        consensus.vParameters[Consensus::CONSENSUS_PARAM_PAYMENT_REQUEST_MIN_REJECT].value = 7000;
        consensus.vParameters[Consensus::CONSENSUS_PARAM_PROPOSAL_MIN_FEE].value = 10000;
        consensus.vParameters[Consensus::CONSENSUS_PARAM_PAYMENT_REQUEST_MIN_FEE].value = 0;
        consensus.vParameters[Consensus::CONSENSUS_PARAM_CONSULTATION_MIN_FEE].value = 10000000000;
        consensus.vParameters[Consensus::CONSENSUS_PARAM_CONSULTATION_ANSWER_MIN_FEE].value = 5000000000;
        consensus.vParameters[Consensus::CONSENSUS_PARAM_FUND_SPREAD_ACCUMULATION].value = 500;
        consensus.vParameters[Consensus::CONSENSUS_PARAM_FUND_PERCENT_PER_BLOCK].value = 2000;
        consensus.vParameters[Consensus::CONSENSUS_PARAM_PROPOSAL_MAX_VOTING_CYCLES].value = 6;
        consensus.vParameters[Consensus::CONSENSUS_PARAM_PAYMENT_REQUEST_MAX_VOTING_CYCLES].value = 8;
        consensus.vParameters[Consensus::CONSENSUS_PARAM_CONSULTATION_MAX_VOTING_CYCLES].value = 4;
        consensus.vParameters[Consensus::CONSENSUS_PARAM_CONSULTATION_MAX_SUPPORT_CYCLES].value = 4;
        consensus.vParameters[Consensus::CONSENSUS_PARAM_CONSULTATION_REFLECTION_LENGTH].value = 1;
        consensus.vParameters[Consensus::CONSENSUS_PARAM_GENERATION_PER_BLOCK].value = 2.5 * COIN;
        consensus.vParameters[Consensus::CONSENSUS_PARAM_NAVNS_FEE].value = 100 * COIN;
        consensus.vParameters[Consensus::CONSENSUS_PARAMS_DAO_VOTE_LIGHT_MIN_FEE].value = 0.1 * COIN;

        /** Coinbase transaction outputs can only be spent after this number of new blocks (network rule) */
        consensus.nCoinbaseMaturity = 50;

        // Deployment of BIP68, BIP112, and BIP113.
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].bit = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nStartTime = 1462060800; // May 1st, 2016
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nTimeout = 1622548800;   // Jun 1st, 2021

        // Deployment of Cold Staking
        consensus.vDeployments[Consensus::DEPLOYMENT_COLDSTAKING].bit = 6;
        consensus.vDeployments[Consensus::DEPLOYMENT_COLDSTAKING].nStartTime = 1525132800; // May 1st, 2018
        consensus.vDeployments[Consensus::DEPLOYMENT_COLDSTAKING].nTimeout = 1622548800;   // Jun 1st, 2021

        // Deployment of SegWit (BIP141 and BIP143)
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].bit = 4;
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].nStartTime = 1493424000; // May 1st, 2017
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].nTimeout = 1622548800;   // Jun 1st, 2021

        // Deployment of Community Fund
        consensus.vDeployments[Consensus::DEPLOYMENT_COMMUNITYFUND].bit = 6;
        consensus.vDeployments[Consensus::DEPLOYMENT_COMMUNITYFUND].nStartTime = 1493424000; // May 1st, 2017
        consensus.vDeployments[Consensus::DEPLOYMENT_COMMUNITYFUND].nTimeout = 1622548800;   // Jun 1st, 2021

        // Deployment of Community Fund Accumulation
        consensus.vDeployments[Consensus::DEPLOYMENT_COMMUNITYFUND_ACCUMULATION].bit = 7;
        consensus.vDeployments[Consensus::DEPLOYMENT_COMMUNITYFUND_ACCUMULATION].nStartTime = 1525132800; // May 1st, 2018
        consensus.vDeployments[Consensus::DEPLOYMENT_COMMUNITYFUND_ACCUMULATION].nTimeout = 1622548800;   // Jun 1st, 2021

        // Deployment of NTP Sync
        consensus.vDeployments[Consensus::DEPLOYMENT_NTPSYNC].bit = 8;
        consensus.vDeployments[Consensus::DEPLOYMENT_NTPSYNC].nStartTime = 1525132800; // May 1st, 2018
        consensus.vDeployments[Consensus::DEPLOYMENT_NTPSYNC].nTimeout = 1556712000;   // 1622548800; // Jun 1st, 2021

        // Deployment of Community Fund Accumulation Spread(NPIP-0003)
        consensus.vDeployments[Consensus::DEPLOYMENT_COMMUNITYFUND_ACCUMULATION_SPREAD].bit = 14;
        consensus.vDeployments[Consensus::DEPLOYMENT_COMMUNITYFUND_ACCUMULATION_SPREAD].nStartTime = 1525132800; // May 1st, 2018
        consensus.vDeployments[Consensus::DEPLOYMENT_COMMUNITYFUND_ACCUMULATION_SPREAD].nTimeout = 1622548800;   // Jun 1st, 2021

        // Increate in Community Fund Accumulation Ammonut (NPIP-0004)
        consensus.vDeployments[Consensus::DEPLOYMENT_COMMUNITYFUND_AMOUNT_V2].bit = 16;
        consensus.vDeployments[Consensus::DEPLOYMENT_COMMUNITYFUND_AMOUNT_V2].nStartTime = 1533081600; // Aug 1st, 2018
        consensus.vDeployments[Consensus::DEPLOYMENT_COMMUNITYFUND_AMOUNT_V2].nTimeout = 1622548800;   // Jun 1st, 2021

        // Deployment of Static Reward
        consensus.vDeployments[Consensus::DEPLOYMENT_STATIC_REWARD].bit = 15;
        consensus.vDeployments[Consensus::DEPLOYMENT_STATIC_REWARD].nStartTime = 1533081600; // August 1st, 2018
        consensus.vDeployments[Consensus::DEPLOYMENT_STATIC_REWARD].nTimeout = 1622548800;   // Jun 1st, 2021

        // Deployment of Quorum reduction for the Community Fund
        consensus.vDeployments[Consensus::DEPLOYMENT_QUORUM_CFUND].bit = 17;
        consensus.vDeployments[Consensus::DEPLOYMENT_QUORUM_CFUND].nStartTime = 1543622400; // Dec 1st, 2018
        consensus.vDeployments[Consensus::DEPLOYMENT_QUORUM_CFUND].nTimeout = 1622548800;   // Jun 1st, 2021

        // Deployment of VOTING STATE CACHE for the Community Fund
        consensus.vDeployments[Consensus::DEPLOYMENT_VOTE_STATE_CACHE].bit = 22;
        consensus.vDeployments[Consensus::DEPLOYMENT_VOTE_STATE_CACHE].nStartTime = 1559390400; // Jun 1st, 2019
        consensus.vDeployments[Consensus::DEPLOYMENT_VOTE_STATE_CACHE].nTimeout = 1622548800;   // Jun 1st, 2021

        // Deployment of CONSULTATIONS for the Community Fund
        consensus.vDeployments[Consensus::DEPLOYMENT_CONSULTATIONS].bit = 23;
        consensus.vDeployments[Consensus::DEPLOYMENT_CONSULTATIONS].nStartTime = 1559390400; // Jun 1st, 2019
        consensus.vDeployments[Consensus::DEPLOYMENT_CONSULTATIONS].nTimeout = 1622548800;   // Jun 1st, 2021

        // Deployment of DAO CONSENSUS
        consensus.vDeployments[Consensus::DEPLOYMENT_DAO_CONSENSUS].bit = 25;
        consensus.vDeployments[Consensus::DEPLOYMENT_DAO_CONSENSUS].nStartTime = 1559390400; // Jun 1st, 2019
        consensus.vDeployments[Consensus::DEPLOYMENT_DAO_CONSENSUS].nTimeout = 1622548800;   // Jun 1st, 2021

        // Deployment of Cold Staking
        consensus.vDeployments[Consensus::DEPLOYMENT_COLDSTAKING_V2].bit = 27;
        consensus.vDeployments[Consensus::DEPLOYMENT_COLDSTAKING_V2].nStartTime = 1559390400; // Jun 1st, 2019
        consensus.vDeployments[Consensus::DEPLOYMENT_COLDSTAKING_V2].nTimeout = 1622548800;   // Jun 1st, 2021

        // Deployment of Cold Staking Pool Fee
        consensus.vDeployments[Consensus::DEPLOYMENT_POOL_FEE].bit = 18;
        consensus.vDeployments[Consensus::DEPLOYMENT_POOL_FEE].nStartTime = 1559390400; // Jun 1st, 2018
        consensus.vDeployments[Consensus::DEPLOYMENT_POOL_FEE].nTimeout = 1622548800;   // Jun 1st, 2021

        /**
         * The message start string is designed to be unlikely to occur in normal data.
         * The characters are rarely used upper ASCII, not valid as UTF-8, and produce
         * a large 32-bit integer with any alignment.
         */
        pchMessageStart[0] = 0x87;
        pchMessageStart[1] = 0xcd;
        pchMessageStart[2] = 0xf8;
        pchMessageStart[3] = 0xca;
        nDefaultPort = 15556;
        nPruneAfterHeight = 1000;
        bnProofOfWorkLimit = arith_uint256(~arith_uint256() >> 16);

        uint32_t nTimestamp = 1588962030;
        uint256 hashGenesisBlock = uint256S("0x0000ca20010727d0fdf6343bc233ac5a5eb44631c89c1722b27c15017dbc2902");
        uint256 hashMerkleRoot = uint256S("0x413d5d2f43cdb6375980973a56036d4a9c67408fe91240934e269c904677b88a");
        uint32_t nNonce = 2043761685;

        genesis = CreateGenesisBlockTestnet(nTimestamp, nNonce, 0x1d00ffff, 1, 0);
        consensus.hashGenesisBlock = genesis.GetHash();

        if (true && (genesis.GetHash() != hashGenesisBlock || genesis.hashMerkleRoot != hashMerkleRoot)) {
            printf("recalculating params for testnet.\n");
            printf("old testnet genesis nonce: %d\n", genesis.nNonce);
            printf("old testnet genesis hash:  %s\n", hashGenesisBlock.ToString().c_str());
            // deliberately empty for loop finds nonce value.
            for (; genesis.GetHash() > consensus.powLimit; genesis.nNonce++) {
            }
            printf("new testnet genesis merkle root: %s\n", genesis.hashMerkleRoot.ToString().c_str());
            printf("new testnet genesis nonce: %d\n", genesis.nNonce);
            printf("new testnet genesis hash: %s\n", genesis.GetHash().ToString().c_str());
        }

        // vSeeds.push_back(CDNSSeedData("testnav.community", "testseed.nav.community"));
        // vSeeds.push_back(CDNSSeedData("testnavcoin.org", "testseed.navcoin.org"));

        assert(consensus.hashGenesisBlock == hashGenesisBlock);
        assert(genesis.hashMerkleRoot == hashMerkleRoot);

        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1, 111);
        base58Prefixes[COLDSTAKING_ADDRESS] = std::vector<unsigned char>(1, 8); // cold staking addresses start with 'C/D'
        base58Prefixes[COLDSTAKING_ADDRESS_V2] = std::vector<unsigned char>(1, 32);
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1, 196);
        base58Prefixes[RAW_SCRIPT_ADDRESS] = std::vector<unsigned char>(1, 60);
        base58Prefixes[SECRET_KEY] = std::vector<unsigned char>(1, 239);
        base58Prefixes[EXT_PUBLIC_KEY] = boost::assign::list_of(0x40)(0x88)(0x2B)(0xE1).convert_to_container<std::vector<unsigned char> >();
        base58Prefixes[EXT_SECRET_KEY] = boost::assign::list_of(0x40)(0x88)(0xDA)(0x4E).convert_to_container<std::vector<unsigned char> >();

        vFixedSeeds = std::vector<SeedSpec6>(pnSeed6_test, pnSeed6_test + ARRAYLEN(pnSeed6_test));

        fMiningRequiresPeers = true;
        fDefaultConsistencyChecks = false;
        fRequireStandard = false;
        fMineBlocksOnDemand = false;
        fTestnetToBeDeprecatedFieldRPC = true;

        checkpointData = (CCheckpointData){
            boost::assign::map_list_of(0, hashGenesisBlock),
            1525248575, // * UNIX timestamp of last checkpoint block
            0,          // * total number of transactions between genesis and last checkpoint
                        //   (the tx=... number in the SetBestChain debug.log lines)
            7000        // * estimated number of transactions per day after checkpoint
        };
    }
};
static CTestNetParams testNetParams;

/**
 * Devnet (v3)
 */
class CDevNetParams : public CChainParams
{
public:
    CDevNetParams()
    {
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
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = 1230767999;   // December 31, 2008
        consensus.nStakeMinAge = 2;                                                      // minimum for coin age: 2 seconds
        consensus.nTargetSpacing = 5;                                                    // Blocktime: 5 secs
        consensus.nStakeCombineThreshold = 1000 * COIN;
        consensus.nStakeSplitThreshold = 2 * consensus.nStakeCombineThreshold;
        consensus.nDailyBlockCount = (24 * 60 * 60) / consensus.nTargetSpacing;
        consensus.nModifierInterval = 10 * 60; // time to elapse before new modifier is computed
        consensus.nTargetTimespan = 25 * 30;
        consensus.nLastPOWBlock = 100000;
        consensus.nMinimumQuorumFirstHalf = 0.5;
        consensus.nMinimumQuorumSecondHalf = 0.4;
        consensus.nCommunityFundMinAge = 5;
        consensus.sigActivationTime = 1512826692;
        consensus.nCoinbaseTimeActivationHeight = 0;
        consensus.nCommunityFundAmount = 0.25 * COIN;
        consensus.nPaymentRequestMaxVersion = CPaymentRequest::ALL_VERSION;
        consensus.nProposalMaxVersion = CProposal::ALL_VERSION;
        consensus.nConsultationMaxVersion = CConsultation::ALL_VERSION;
        consensus.nConsultationAnswerMaxVersion = CConsultationAnswer::ALL_VERSION;
        consensus.nMaxFutureDrift = 60000;
        consensus.nHeightv451Fork = 1000;
        consensus.nHeightv452Fork = 1000;
        consensus.fDaoClientActivated = true;

        consensus.nConsensusChangeMinAccept = 7500;

        consensus.vParameters[Consensus::CONSENSUS_PARAM_VOTING_CYCLE_LENGTH].value = 10;
        consensus.vParameters[Consensus::CONSENSUS_PARAM_PROPOSAL_MIN_QUORUM].value = 5000;
        consensus.vParameters[Consensus::CONSENSUS_PARAM_PAYMENT_REQUEST_MIN_QUORUM].value = 5000;
        consensus.vParameters[Consensus::CONSENSUS_PARAM_CONSULTATION_MIN_SUPPORT].value = 150;
        consensus.vParameters[Consensus::CONSENSUS_PARAM_CONSULTATION_ANSWER_MIN_SUPPORT].value = 150;
        consensus.vParameters[Consensus::CONSENSUS_PARAM_CONSULTATION_MIN_CYCLES].value = 2;
        consensus.vParameters[Consensus::CONSENSUS_PARAM_PROPOSAL_MIN_ACCEPT].value = 7000;
        consensus.vParameters[Consensus::CONSENSUS_PARAM_PROPOSAL_MIN_REJECT].value = 7000;
        consensus.vParameters[Consensus::CONSENSUS_PARAM_PAYMENT_REQUEST_MIN_ACCEPT].value = 7000;
        consensus.vParameters[Consensus::CONSENSUS_PARAM_PAYMENT_REQUEST_MIN_REJECT].value = 7000;
        consensus.vParameters[Consensus::CONSENSUS_PARAM_PROPOSAL_MIN_FEE].value = 5000000000;
        consensus.vParameters[Consensus::CONSENSUS_PARAM_CONSULTATION_MIN_FEE].value = 10000000000;
        consensus.vParameters[Consensus::CONSENSUS_PARAM_PAYMENT_REQUEST_MIN_FEE].value = 0;
        consensus.vParameters[Consensus::CONSENSUS_PARAM_CONSULTATION_ANSWER_MIN_FEE].value = 5000000000;
        consensus.vParameters[Consensus::CONSENSUS_PARAM_FUND_SPREAD_ACCUMULATION].value = 500;
        consensus.vParameters[Consensus::CONSENSUS_PARAM_FUND_PERCENT_PER_BLOCK].value = 2000;
        consensus.vParameters[Consensus::CONSENSUS_PARAM_PROPOSAL_MAX_VOTING_CYCLES].value = 6;
        consensus.vParameters[Consensus::CONSENSUS_PARAM_PAYMENT_REQUEST_MAX_VOTING_CYCLES].value = 8;
        consensus.vParameters[Consensus::CONSENSUS_PARAM_CONSULTATION_MAX_VOTING_CYCLES].value = 4;
        consensus.vParameters[Consensus::CONSENSUS_PARAM_CONSULTATION_MAX_SUPPORT_CYCLES].value = 4;
        consensus.vParameters[Consensus::CONSENSUS_PARAM_CONSULTATION_REFLECTION_LENGTH].value = 1;
        consensus.vParameters[Consensus::CONSENSUS_PARAM_GENERATION_PER_BLOCK].value = 2.5 * COIN;
        consensus.vParameters[Consensus::CONSENSUS_PARAM_NAVNS_FEE].value = 100 * COIN;
        consensus.vParameters[Consensus::CONSENSUS_PARAMS_DAO_VOTE_LIGHT_MIN_FEE].value = 0.1 * COIN;

        /** Coinbase transaction outputs can only be spent after this number of new blocks (network rule) */
        consensus.nCoinbaseMaturity = 5;

        // Deployment of BIP68, BIP112, and BIP113.
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].bit = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nStartTime = 1462060800; // May 1st, 2016
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nTimeout = 1651363200;   // May 1st, 2022

        // Deployment of Cold Staking
        consensus.vDeployments[Consensus::DEPLOYMENT_COLDSTAKING].bit = 3;
        consensus.vDeployments[Consensus::DEPLOYMENT_COLDSTAKING].nStartTime = 1525132800; // May 1st, 2018
        consensus.vDeployments[Consensus::DEPLOYMENT_COLDSTAKING].nTimeout = 1651363200;   // May 1st, 2022

        // Deployment of SegWit (BIP141 and BIP143)
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].bit = 5;
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].nStartTime = 1493424000; // May 1st, 2017
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].nTimeout = 1651363200;   // May 1st, 2022

        // Deployment of Community Fund
        consensus.vDeployments[Consensus::DEPLOYMENT_COMMUNITYFUND].bit = 6;
        consensus.vDeployments[Consensus::DEPLOYMENT_COMMUNITYFUND].nStartTime = 1493424000; // May 1st, 2017
        consensus.vDeployments[Consensus::DEPLOYMENT_COMMUNITYFUND].nTimeout = 1651363200;   // May 1st, 2022

        // Deployment of NTP Sync
        consensus.vDeployments[Consensus::DEPLOYMENT_NTPSYNC].bit = 8;
        consensus.vDeployments[Consensus::DEPLOYMENT_NTPSYNC].nStartTime = 1525132800; // May 1st, 2018
        consensus.vDeployments[Consensus::DEPLOYMENT_NTPSYNC].nTimeout = 1651363200;   // May 1st, 2022

        // Deployment of Community Fund Accumulation
        consensus.vDeployments[Consensus::DEPLOYMENT_COMMUNITYFUND_ACCUMULATION].bit = 7;
        consensus.vDeployments[Consensus::DEPLOYMENT_COMMUNITYFUND_ACCUMULATION].nStartTime = 1525132800; // May 1st, 2018
        consensus.vDeployments[Consensus::DEPLOYMENT_COMMUNITYFUND_ACCUMULATION].nTimeout = 1651363200;   // May 1st, 2022

        // Deployment of Community Fund Accumulation Spread(NPIP-0003)
        consensus.vDeployments[Consensus::DEPLOYMENT_COMMUNITYFUND_ACCUMULATION_SPREAD].bit = 14;
        consensus.vDeployments[Consensus::DEPLOYMENT_COMMUNITYFUND_ACCUMULATION_SPREAD].nStartTime = 1525132800; // May 1st, 2018
        consensus.vDeployments[Consensus::DEPLOYMENT_COMMUNITYFUND_ACCUMULATION_SPREAD].nTimeout = 1651363200;   // May 1st, 2022

        // Increate in Community Fund Accumulation Ammonut (NPIP-0004)
        consensus.vDeployments[Consensus::DEPLOYMENT_COMMUNITYFUND_AMOUNT_V2].bit = 16;
        consensus.vDeployments[Consensus::DEPLOYMENT_COMMUNITYFUND_AMOUNT_V2].nStartTime = 1533081600; // Aug 1st, 2018
        consensus.vDeployments[Consensus::DEPLOYMENT_COMMUNITYFUND_AMOUNT_V2].nTimeout = 1651363200;   // May 1st, 2022

        // Deployment of Static Reward
        consensus.vDeployments[Consensus::DEPLOYMENT_STATIC_REWARD].bit = 15;
        consensus.vDeployments[Consensus::DEPLOYMENT_STATIC_REWARD].nStartTime = 1533081600; // August 1st, 2018
        consensus.vDeployments[Consensus::DEPLOYMENT_STATIC_REWARD].nTimeout = 1651363200;   // May 1st, 2022

        // Deployment of Quorum reduction for the Community Fund
        consensus.vDeployments[Consensus::DEPLOYMENT_QUORUM_CFUND].bit = 17;
        consensus.vDeployments[Consensus::DEPLOYMENT_QUORUM_CFUND].nStartTime = 1543622400; // Dec 1st, 2018
        consensus.vDeployments[Consensus::DEPLOYMENT_QUORUM_CFUND].nTimeout = 1651363200;   // May 1st, 2022

        // Deployment of VOTING STATE CACHE for the Community Fund
        consensus.vDeployments[Consensus::DEPLOYMENT_VOTE_STATE_CACHE].bit = 22;
        consensus.vDeployments[Consensus::DEPLOYMENT_VOTE_STATE_CACHE].nStartTime = 1559390400; // Jun 1st, 2019
        consensus.vDeployments[Consensus::DEPLOYMENT_VOTE_STATE_CACHE].nTimeout = 1622548800;   // Jun 1st, 2021

        // Deployment of CONSULTATIONS for the Community Fund
        consensus.vDeployments[Consensus::DEPLOYMENT_CONSULTATIONS].bit = 23;
        consensus.vDeployments[Consensus::DEPLOYMENT_CONSULTATIONS].nStartTime = 1559390400; // Jun 1st, 2019
        consensus.vDeployments[Consensus::DEPLOYMENT_CONSULTATIONS].nTimeout = 1622548800;   // Jun 1st, 2021

        // Deployment of DAO CONSENSUS
        consensus.vDeployments[Consensus::DEPLOYMENT_DAO_CONSENSUS].bit = 25;
        consensus.vDeployments[Consensus::DEPLOYMENT_DAO_CONSENSUS].nStartTime = 1559390400; // Jun 1st, 2019
        consensus.vDeployments[Consensus::DEPLOYMENT_DAO_CONSENSUS].nTimeout = 1622548800;   // Jun 1st, 2021

        // Deployment of Cold Staking
        consensus.vDeployments[Consensus::DEPLOYMENT_COLDSTAKING_V2].bit = 27;
        consensus.vDeployments[Consensus::DEPLOYMENT_COLDSTAKING_V2].nStartTime = 1559390400; // Jun 1st, 2019
        consensus.vDeployments[Consensus::DEPLOYMENT_COLDSTAKING_V2].nTimeout = 1622548800;   // Jun 1st, 2021

        // Deployment of Cold Staking Pool Fee
        consensus.vDeployments[Consensus::DEPLOYMENT_POOL_FEE].bit = 18;
        consensus.vDeployments[Consensus::DEPLOYMENT_POOL_FEE].nStartTime = 1559390400; // Jun 1st, 2018
        consensus.vDeployments[Consensus::DEPLOYMENT_POOL_FEE].nTimeout = 1622548800;   // Jun 1st, 2021

        /**
         * The message start string is designed to be unlikely to occur in normal data.
         * The characters are rarely used upper ASCII, not valid as UTF-8, and produce
         * a large 32-bit integer with any alignment.
         */
        pchMessageStart[0] = 0xe4;
        pchMessageStart[1] = 0xf1;
        pchMessageStart[2] = 0xd0;
        pchMessageStart[3] = 0xc0;
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
        uint256 hashGenesisBlock = uint256S("0x0000971c241a5a1b8462c3cb8d455f0493043eb37c7163f88c658c70aa689929");
        uint256 hashMerkleRoot = uint256S("0x2d9101b87fe7b9deaea41849c1f3bed71e060739147802a238fe968f75ad0fd9");
        uint32_t nNonce = 2043198879;
        // 5) Rebuild. Launch daemon.
        // 6) Generate first block using RPC command "./navcoin-cli generate 1"

        genesis = CreateGenesisBlockTestnet(nTimestamp, nNonce, 0xffffffff, 1, 0);
        consensus.hashGenesisBlock = genesis.GetHash();

        if (true && (genesis.GetHash() != hashGenesisBlock || genesis.hashMerkleRoot != hashMerkleRoot)) {
            printf("recalculating params for devnet.\n");
            printf("old devnet genesis merkle root:  0x%s\n\n", hashMerkleRoot.ToString().c_str());
            printf("old devnet genesis nonce: %d\n", genesis.nNonce);
            printf("old devnet genesis hash:  0x%s\n\n", hashGenesisBlock.ToString().c_str());
            // deliberately empty for loop finds nonce value.
            for (; genesis.GetHash() > consensus.powLimit; genesis.nNonce++) {
            }
            printf("new devnet genesis merkle root: 0x%s\n", genesis.hashMerkleRoot.ToString().c_str());
            printf("new devnet genesis nonce: %d\n", genesis.nNonce);
            printf("new devnet genesis hash: 0x%s\n", genesis.GetHash().ToString().c_str());
            printf("use the new values to update CDevNetParams class in src/chainparams.cpp\n");
        }

        // vSeeds.push_back(CDNSSeedData("devnav.community", "devseed.nav.community"));
        // vSeeds.push_back(CDNSSeedData("devnet.navcoin.org", "devseed.navcoin.org"));

        assert(consensus.hashGenesisBlock == hashGenesisBlock);
        assert(genesis.hashMerkleRoot == hashMerkleRoot);

        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1, 111);
        base58Prefixes[COLDSTAKING_ADDRESS] = std::vector<unsigned char>(1, 63); // cold staking addresses start with 'S'
        base58Prefixes[COLDSTAKING_ADDRESS_V2] = std::vector<unsigned char>(1, 40);
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1, 196);
        base58Prefixes[RAW_SCRIPT_ADDRESS] = std::vector<unsigned char>(1, 60);
        base58Prefixes[SECRET_KEY] = std::vector<unsigned char>(1, 239);
        base58Prefixes[EXT_PUBLIC_KEY] = boost::assign::list_of(0x40)(0x88)(0x2B)(0xE1).convert_to_container<std::vector<unsigned char> >();
        base58Prefixes[EXT_SECRET_KEY] = boost::assign::list_of(0x40)(0x88)(0xDA)(0x4E).convert_to_container<std::vector<unsigned char> >();

        vFixedSeeds = std::vector<SeedSpec6>(pnSeed6_dev, pnSeed6_dev + ARRAYLEN(pnSeed6_dev));

        fMiningRequiresPeers = false;
        fDefaultConsistencyChecks = false;
        fRequireStandard = false;
        fMineBlocksOnDemand = true;
        fTestnetToBeDeprecatedFieldRPC = true;

        checkpointData = (CCheckpointData){
            boost::assign::map_list_of(0, hashGenesisBlock),
            1515437594, // * UNIX timestamp of last checkpoint block
            0,          // * total number of transactions between genesis and last checkpoint
                        //   (the tx=... number in the SetBestChain debug.log lines)
            7000        // * estimated number of transactions per day after checkpoint
        };
    }
};
static CDevNetParams devNetParams;

/**
 * Regression test
 */
class CRegTestParams : public CChainParams
{
public:
    CRegTestParams()
    {
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
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = 1230767999;   // December 31, 2008
        consensus.nStakeMinAge = 2;                                                      // minimum for coin age: 2 seconds
        consensus.nTargetSpacing = 30;                                                   // Blocktime: 30 secs
        consensus.nStakeCombineThreshold = 1000 * COIN;
        consensus.nStakeSplitThreshold = 2 * consensus.nStakeCombineThreshold;
        consensus.nDailyBlockCount = (24 * 60 * 60) / consensus.nTargetSpacing;
        consensus.nModifierInterval = 10 * 60; // time to elapse before new modifier is computed
        consensus.nTargetTimespan = 25 * 30;
        consensus.nLastPOWBlock = 100000;
        consensus.nMinimumQuorumFirstHalf = 0.5;
        consensus.nMinimumQuorumSecondHalf = 0.4;
        consensus.nCommunityFundMinAge = 5;
        consensus.sigActivationTime = 0;
        consensus.nCoinbaseTimeActivationHeight = 0;
        consensus.nCommunityFundAmount = 0.25 * COIN;
        consensus.nPaymentRequestMaxVersion = CPaymentRequest::ALL_VERSION;
        consensus.nProposalMaxVersion = CProposal::ALL_VERSION;
        consensus.nConsultationMaxVersion = CConsultation::ALL_VERSION;
        consensus.nConsultationAnswerMaxVersion = CConsultationAnswer::ALL_VERSION;
        consensus.nMaxFutureDrift = 60000;
        consensus.nHeightv451Fork = 1000;
        consensus.nHeightv452Fork = 1000;
        consensus.fDaoClientActivated = true;

        consensus.nConsensusChangeMinAccept = 7500;

        consensus.vParameters[Consensus::CONSENSUS_PARAM_VOTING_CYCLE_LENGTH].value = 180;
        consensus.vParameters[Consensus::CONSENSUS_PARAM_PROPOSAL_MIN_QUORUM].value = 5000;
        consensus.vParameters[Consensus::CONSENSUS_PARAM_PAYMENT_REQUEST_MIN_QUORUM].value = 5000;
        consensus.vParameters[Consensus::CONSENSUS_PARAM_CONSULTATION_MIN_SUPPORT].value = 150;
        consensus.vParameters[Consensus::CONSENSUS_PARAM_CONSULTATION_ANSWER_MIN_SUPPORT].value = 150;
        consensus.vParameters[Consensus::CONSENSUS_PARAM_CONSULTATION_MIN_CYCLES].value = 2;
        consensus.vParameters[Consensus::CONSENSUS_PARAM_PROPOSAL_MIN_ACCEPT].value = 7000;
        consensus.vParameters[Consensus::CONSENSUS_PARAM_PROPOSAL_MIN_REJECT].value = 7000;
        consensus.vParameters[Consensus::CONSENSUS_PARAM_PAYMENT_REQUEST_MIN_ACCEPT].value = 7000;
        consensus.vParameters[Consensus::CONSENSUS_PARAM_PAYMENT_REQUEST_MIN_REJECT].value = 7000;
        consensus.vParameters[Consensus::CONSENSUS_PARAM_PROPOSAL_MIN_FEE].value = 5000000000;
        consensus.vParameters[Consensus::CONSENSUS_PARAM_PAYMENT_REQUEST_MIN_FEE].value = 0;
        consensus.vParameters[Consensus::CONSENSUS_PARAM_CONSULTATION_MIN_FEE].value = 10000000000;
        consensus.vParameters[Consensus::CONSENSUS_PARAM_CONSULTATION_ANSWER_MIN_FEE].value = 5000000000;
        consensus.vParameters[Consensus::CONSENSUS_PARAM_FUND_SPREAD_ACCUMULATION].value = 500;
        consensus.vParameters[Consensus::CONSENSUS_PARAM_FUND_PERCENT_PER_BLOCK].value = 2000;
        consensus.vParameters[Consensus::CONSENSUS_PARAM_PROPOSAL_MAX_VOTING_CYCLES].value = 6;
        consensus.vParameters[Consensus::CONSENSUS_PARAM_PAYMENT_REQUEST_MAX_VOTING_CYCLES].value = 8;
        consensus.vParameters[Consensus::CONSENSUS_PARAM_CONSULTATION_MAX_VOTING_CYCLES].value = 4;
        consensus.vParameters[Consensus::CONSENSUS_PARAM_CONSULTATION_MAX_SUPPORT_CYCLES].value = 4;
        consensus.vParameters[Consensus::CONSENSUS_PARAM_CONSULTATION_REFLECTION_LENGTH].value = 1;
        consensus.vParameters[Consensus::CONSENSUS_PARAM_GENERATION_PER_BLOCK].value = 2.5 * COIN;
        consensus.vParameters[Consensus::CONSENSUS_PARAM_NAVNS_FEE].value = 100 * COIN;
        consensus.vParameters[Consensus::CONSENSUS_PARAMS_DAO_VOTE_LIGHT_MIN_FEE].value = 0.1 * COIN;

        /** Coinbase transaction outputs can only be spent after this number of new blocks (network rule) */
        consensus.nCoinbaseMaturity = 50;

        // Deployment of BIP68, BIP112, and BIP113.
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].bit = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nStartTime = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nTimeout = 2556712000;

        // Deployment of Cold Staking
        consensus.vDeployments[Consensus::DEPLOYMENT_COLDSTAKING].bit = 3;
        consensus.vDeployments[Consensus::DEPLOYMENT_COLDSTAKING].nStartTime = 1525132800; // May 1st, 2018
        consensus.vDeployments[Consensus::DEPLOYMENT_COLDSTAKING].nTimeout = 1556712000;   // May 1st, 2019

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
        consensus.vDeployments[Consensus::DEPLOYMENT_COMMUNITYFUND_ACCUMULATION_SPREAD].nTimeout = 1556712000;   // May 1st, 2019

        // Increate in Community Fund Accumulation Ammonut (NPIP-0004)
        consensus.vDeployments[Consensus::DEPLOYMENT_COMMUNITYFUND_AMOUNT_V2].bit = 16;
        consensus.vDeployments[Consensus::DEPLOYMENT_COMMUNITYFUND_AMOUNT_V2].nStartTime = 1533081600; // Aug 1st, 2018
        consensus.vDeployments[Consensus::DEPLOYMENT_COMMUNITYFUND_AMOUNT_V2].nTimeout = 1564617600;   // Aug 1st, 2019

        // Deployment of Static Reward
        consensus.vDeployments[Consensus::DEPLOYMENT_STATIC_REWARD].bit = 15;
        consensus.vDeployments[Consensus::DEPLOYMENT_STATIC_REWARD].nStartTime = 1533081600; // August 1st, 2018
        consensus.vDeployments[Consensus::DEPLOYMENT_STATIC_REWARD].nTimeout = 1564617600;   // August 1st, 2019

        // Deployment of Quorum reduction for the Community Fund
        consensus.vDeployments[Consensus::DEPLOYMENT_QUORUM_CFUND].bit = 17;
        consensus.vDeployments[Consensus::DEPLOYMENT_QUORUM_CFUND].nStartTime = 1543622400; // Dec 1st, 2018
        consensus.vDeployments[Consensus::DEPLOYMENT_QUORUM_CFUND].nTimeout = 1575158400;   // Dec 1st, 2019

        // Deployment of VOTING STATE CACHE for the Community Fund
        consensus.vDeployments[Consensus::DEPLOYMENT_VOTE_STATE_CACHE].bit = 22;
        consensus.vDeployments[Consensus::DEPLOYMENT_VOTE_STATE_CACHE].nStartTime = 1559390400; // Jun 1st, 2019
        consensus.vDeployments[Consensus::DEPLOYMENT_VOTE_STATE_CACHE].nTimeout = 1622548800;   // Jun 1st, 2021

        // Deployment of CONSULTATIONS for the Community Fund
        consensus.vDeployments[Consensus::DEPLOYMENT_CONSULTATIONS].bit = 23;
        consensus.vDeployments[Consensus::DEPLOYMENT_CONSULTATIONS].nStartTime = 1559390400; // Jun 1st, 2019
        consensus.vDeployments[Consensus::DEPLOYMENT_CONSULTATIONS].nTimeout = 1622548800;   // Jun 1st, 2021

        // Deployment of DAO CONSENSUS
        consensus.vDeployments[Consensus::DEPLOYMENT_DAO_CONSENSUS].bit = 25;
        consensus.vDeployments[Consensus::DEPLOYMENT_DAO_CONSENSUS].nStartTime = 1559390400; // Jun 1st, 2019
        consensus.vDeployments[Consensus::DEPLOYMENT_DAO_CONSENSUS].nTimeout = 1622548800;   // Jun 1st, 2021

        // Deployment of Cold Staking
        consensus.vDeployments[Consensus::DEPLOYMENT_COLDSTAKING_V2].bit = 27;
        consensus.vDeployments[Consensus::DEPLOYMENT_COLDSTAKING_V2].nStartTime = 1559390400; // Jun 1st, 2019
        consensus.vDeployments[Consensus::DEPLOYMENT_COLDSTAKING_V2].nTimeout = 1622548800;   // Jun 1st, 2021

        // Deployment of Cold Staking Pool Fee
        consensus.vDeployments[Consensus::DEPLOYMENT_POOL_FEE].bit = 18;
        consensus.vDeployments[Consensus::DEPLOYMENT_POOL_FEE].nStartTime = 1559390400; // Jun 1st, 2018
        consensus.vDeployments[Consensus::DEPLOYMENT_POOL_FEE].nTimeout = 1622548800;   // Jun 1st, 2021

        /**
         * The message start string is designed to be unlikely to occur in normal data.
         * The characters are rarely used upper ASCII, not valid as UTF-8, and produce
         * a large 32-bit integer with any alignment.
         */
        pchMessageStart[0] = 0xa7;
        pchMessageStart[1] = 0xb4;
        pchMessageStart[2] = 0xec;
        pchMessageStart[3] = 0xbb;
        nDefaultPort = 18886;
        nPruneAfterHeight = 1000;
        bnProofOfWorkLimit = arith_uint256(~arith_uint256() >> 16);

        uint32_t nTimestamp = GetTimeNow();
        uint256 hashGenesisBlock = uint256S("0x0000e01b12644af6917e5aada637a609dd9590ad6bdc4828cd8df95258d85c02");
        uint256 hashMerkleRoot = uint256S("0x2d9101b87fe7b9deaea41849c1f3bed71e060739147802a238fe968f75ad0fd9");
        uint32_t nNonce = 2043184832;

        genesis = CreateGenesisBlockTestnet(nTimestamp, nNonce, 0x1d00ffff, 1, 0);
        consensus.hashGenesisBlock = genesis.GetHash();

        if ((genesis.GetHash() != hashGenesisBlock || genesis.hashMerkleRoot != hashMerkleRoot)) {
            nTimestamp = GetTimeNow();
            // deliberately empty for loop finds nonce value.
            for (; genesis.GetHash() > consensus.powLimit; genesis.nNonce++) {
            }
            hashGenesisBlock = genesis.GetHash();
            nNonce = genesis.nNonce;
            hashMerkleRoot = genesis.hashMerkleRoot;
        }

        // vSeeds.push_back(CDNSSeedData("testnav.community", "testseed.nav.community"));
        // vSeeds.push_back(CDNSSeedData("testnavcoin.org", "testseed.navcoin.org"));

        consensus.hashGenesisBlock = genesis.GetHash();

        assert(consensus.hashGenesisBlock == hashGenesisBlock);
        assert(genesis.hashMerkleRoot == hashMerkleRoot);

        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1, 111);
        base58Prefixes[COLDSTAKING_ADDRESS] = std::vector<unsigned char>(1, 63); // cold staking addresses start with 'S'
        base58Prefixes[COLDSTAKING_ADDRESS_V2] = std::vector<unsigned char>(1, 44);
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1, 196);
        base58Prefixes[RAW_SCRIPT_ADDRESS] = std::vector<unsigned char>(1, 60);
        base58Prefixes[SECRET_KEY] = std::vector<unsigned char>(1, 239);
        base58Prefixes[EXT_PUBLIC_KEY] = boost::assign::list_of(0x40)(0x88)(0x2B)(0xE1).convert_to_container<std::vector<unsigned char> >();
        base58Prefixes[EXT_SECRET_KEY] = boost::assign::list_of(0x40)(0x88)(0xDA)(0x4E).convert_to_container<std::vector<unsigned char> >();

        vFixedSeeds = std::vector<SeedSpec6>(pnSeed6_test, pnSeed6_test + ARRAYLEN(pnSeed6_test));

        fMiningRequiresPeers = false;
        fDefaultConsistencyChecks = false;
        fRequireStandard = false;
        fMineBlocksOnDemand = true;
        fTestnetToBeDeprecatedFieldRPC = true;

        checkpointData = (CCheckpointData){
            boost::assign::map_list_of(0, hashGenesisBlock),
            nTimestamp, // * UNIX timestamp of last checkpoint block
            0,          // * total number of transactions between genesis and last checkpoint
                        //   (the tx=... number in the SetBestChain debug.log lines)
            7000        // * estimated number of transactions per day after checkpoint
        };
    }
};
static CRegTestParams regTestParams;

static CChainParams* pCurrentParams = 0;

const CChainParams& Params()
{
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
