// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef NAVCOIN_CONSENSUS_PARAMS_H
#define NAVCOIN_CONSENSUS_PARAMS_H

#include <amount.h>
#include <uint256.h>
#include <map>
#include <string>

namespace Consensus {

enum DeploymentPos
{
    DEPLOYMENT_TESTDUMMY,
    DEPLOYMENT_CSV, // Deployment of BIP68, BIP112, and BIP113.
    DEPLOYMENT_SEGWIT, // Deployment of BIP141 and BIP143
    // NOTE: Also add new deployments to VersionBitsDeploymentInfo in versionbits.cpp
    DEPLOYMENT_CSV_LEGACY,
    DEPLOYMENT_SEGWIT_LEGACY,
    DEPLOYMENT_COMMUNITYFUND,
    DEPLOYMENT_COMMUNITYFUND_ACCUMULATION,
    DEPLOYMENT_COLDSTAKING,
    DEPLOYMENT_COMMUNITYFUND_ACCUMULATION_SPREAD,
    DEPLOYMENT_COMMUNITYFUND_AMOUNT_V2,
    DEPLOYMENT_NTPSYNC,
    DEPLOYMENT_STATIC_REWARD,
    DEPLOYMENT_QUORUM_CFUND,
    DEPLOYMENT_VOTE_STATE_CACHE,
    DEPLOYMENT_CONSULTATIONS,
    DEPLOYMENT_DAO_CONSENSUS,
    DEPLOYMENT_COLDSTAKING_V2,
    DEPLOYMENT_POOL_FEE,
    DEPLOYMENT_BLSCT,
    DEPLOYMENT_EXCLUDE,
    DEPLOYMENT_DAO_SUPER,
    MAX_VERSION_BITS_DEPLOYMENTS
};

static std::string sDeploymentsDesc[Consensus::MAX_VERSION_BITS_DEPLOYMENTS] = {
    "Test",
    "CSV",
    "Segregated Witness",
    "CSV Legacy",
    "Segregated Witness Legacy",
    "Community Fund",
    "Starts the accumulation of coins on the Community Fund",
    "Cold Staking",
    "Spread the accumulation of coins on the Community Fund to save on block space",
    "Increase the contributed amount to the Community Fund to 0.5NAV per block",
    "NTP SYNC",
    "Upgrade to POSv3. Every staked block will have a fixed reward of 2 NAV",
    "Reduce the Quorum necessary for the last cycles of the Community Fund votings",
    "Upgrades the Community Fund with a state based cache which will save on block space as votes do not need to be broadcasted every block",
    "Enable DAO Consultations",
    "Enables remote DAO voting from light wallets",
    "Enables the decision over consensus parameters using distributed voting",
    "Allows staking pools to charge a fee",
    "Activates the privacy protocol blsCT and the private token xNAV",
    "Excludes inactive voters from the DAO quorums",
    "Enables Super proposals and allows combined vote of consensus parameters"
};

/**
 * Struct for each individual consensus rule change using BIP9.
 */
struct BIP9Deployment {
    /** Bit position to select the particular bit in nVersion. */
    int bit;
    /** Start MedianTime for version bits miner confirmation. Can be a date in the past */
    int64_t nStartTime;
    /** Timeout/expiry MedianTime for the deployment attempt. */
    int64_t nTimeout;
    double nYesCount;
};

struct ConsensusParameter {
    uint64_t value;
};

/**
 * Parameters that influence chain consensus.
 */
struct Params {
    uint256 hashGenesisBlock;
    int nSubsidyHalvingInterval;
    /** Used to check majorities for block version upgrade */
    int nMajorityEnforceBlockUpgrade;
    int nMajorityRejectBlockOutdated;
    int nMajorityWindow;
    /** Block height and hash at which BIP34 becomes active */
    int BIP34Height;
    uint256 BIP34Hash;
    /**
     * Minimum blocks including miner confirmation of the total of 2016 blocks in a retargetting period,
     * (nPowTargetTimespan / nPowTargetSpacing) which is also used for BIP9 deployments.
     * Examples: 1916 for 95%, 1512 for testchains.
     */
    uint32_t nRuleChangeActivationThreshold;
    uint32_t nMinerConfirmationWindow;
    BIP9Deployment vDeployments[MAX_VERSION_BITS_DEPLOYMENTS];
    ConsensusParameter vParameters[MAX_CONSENSUS_PARAMS];
    /** Proof of work parameters */
    uint256 powLimit;
    bool fPowAllowMinDifficultyBlocks;
    bool fPowNoRetargeting;
    int64_t nPowTargetSpacing;
    int64_t nPowTargetTimespan;
    int nLastPOWBlock;

    float nMinimumQuorumFirstHalf;
    float nMinimumQuorumSecondHalf;
    int nCommunityFundMinAge;
    uint64_t nCommunityFundAmount;
    int64_t nPaymentRequestMaxVersion;
    int64_t nProposalMaxVersion;
    int64_t nConsultationMaxVersion;
    int64_t nConsultationAnswerMaxVersion;

    int64_t nConsensusChangeMinAccept;

    /** Proof of stake parameters */
    unsigned int nStakeMinAge;
    int nTargetSpacing;
    unsigned int nTargetTimespan;
    int64_t nStakeCombineThreshold;
    int64_t nStakeSplitThreshold;
    int nDailyBlockCount;
    unsigned int nModifierInterval; // time to elapse before new modifier is computed
    int64_t sigActivationTime;
    int64_t nCoinbaseTimeActivationHeight;
    int64_t nMaxFutureDrift;
    int nHeightv451Fork;
    int nHeightv452Fork;

    bool fDaoClientActivated;

    /** Coinbase transaction outputs can only be spent after this number of new blocks (network rule) */
    int nCoinbaseMaturity;

    int64_t DifficultyAdjustmentInterval() const { return nPowTargetTimespan / nPowTargetSpacing; }
};
} // namespace Consensus

#endif // NAVCOIN_CONSENSUS_PARAMS_H
