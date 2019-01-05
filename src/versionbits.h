// Copyright (c) 2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef NAVCOIN_CONSENSUS_VERSIONBITS
#define NAVCOIN_CONSENSUS_VERSIONBITS

#include "chain.h"
#include <map>

/** What block version to use for new blocks (pre versionbits) */
static const int32_t VERSIONBITS_LAST_OLD_BLOCK_VERSION = 7;
/** What bits to set in version for versionbits blocks */
static const int32_t VERSIONBITS_TOP_BITS = 0x70000000UL;
static const int32_t VERSIONBITS_TOP_BITS_SIG = 0x71000000UL;
/** What bitmask determines whether versionbits is in use */
static const int32_t VERSIONBITS_TOP_MASK = 0xF0000000UL;
/** Total bits available for versionbits */
static const int32_t VERSIONBITS_NUM_BITS = 29;

/***
*
* NavCoin Version Bits
*
* SIGNAL
*
* Bit 0 -> CSV
* Bit 1 -> RESERVED LEGACY
* Bit 2 -> RESERVED LEGACY
* Bit 3 -> COLD STAKING
* Bit 4 -> SEGWIT
* Bit 6 -> C FUND
* Bit 7 -> C FUND ACCUMULATION
* Bit 8 -> NTP SYNC
* Bit 14 -> C FUND ACCUMULATION SPREAD
* Bit 17 -< C FUND REDUCED QUORUM
*
* ACTIVATION
*
* Bit 5 -> SEGWIT
* Bit 6 -> C FUND
* Bit 7 -> NTP SYNC
* Bit 12 -> CFUND ACCUMULATION
* Bit 13 -> COLD STAKING
* Bit 8 -> CFUND ACCUMULATION
* Bit 13 -> COLD STAKING
* Bit 14 -> C FUND ACCUMULATION SPREAD
* Bit 16 -< C FUND ACCUMULATION AMOUNT V2
* Bit 17 -< C FUND REDUCED QUORUM
* Bit 20 -< V451 FORK
*
***/

static const int32_t nSegWitVersionMask = 0x00000020;
static const int32_t nCFundVersionMask = 0x00000040;
static const int32_t nNSyncVersionMask = 0x00000080;
static const int32_t nCFundAccVersionMask = 0x00000100;
static const int32_t nColdStakingVersionMask = 0x00002000;
static const int32_t nCFundAccSpreadVersionMask = 0x00004000;
static const int32_t nCFundAmountV2Mask = 0x00010000;
static const int32_t nCFundReducedQuorumMask = 0x00020000;
static const int32_t nStaticRewardVersionMask = 0x00008000;
static const int32_t nV451ForkMask = 0x00100000;

static const std::vector<int> rejectedVersionBitsByDefault = {17};

enum ThresholdState {
    THRESHOLD_DEFINED,
    THRESHOLD_STARTED,
    THRESHOLD_LOCKED_IN,
    THRESHOLD_ACTIVE,
    THRESHOLD_FAILED,
};

// A map that gives the state for blocks whose height is a multiple of Period().
// The map is indexed by the block's parent, however, so all keys in the map
// will either be NULL or a block with (height + 1) % Period() == 0.
typedef std::map<const CBlockIndex*, ThresholdState> ThresholdConditionCache;

struct BIP9DeploymentInfo {
    /** Deployment name */
    const char *name;
    /** Whether GBT clients can safely ignore this rule in simplified usage */
    bool gbt_force;
};

extern const struct BIP9DeploymentInfo VersionBitsDeploymentInfo[];

/**
 * Abstract class that implements BIP9-style threshold logic, and caches results.
 */
class AbstractThresholdConditionChecker {
protected:
    virtual bool Condition(const CBlockIndex* pindex, const Consensus::Params& params) const =0;
    virtual int64_t BeginTime(const Consensus::Params& params) const =0;
    virtual int64_t EndTime(const Consensus::Params& params) const =0;
    virtual int Period(const Consensus::Params& params) const =0;
    virtual int Threshold(const Consensus::Params& params) const =0;

public:
    // Note that the function below takes a pindexPrev as input: they compute information for block B based on its parent.
    ThresholdState GetStateFor(const CBlockIndex* pindexPrev, const Consensus::Params& params, ThresholdConditionCache& cache) const;

};

struct VersionBitsCache
{
    ThresholdConditionCache caches[Consensus::MAX_VERSION_BITS_DEPLOYMENTS];

    void Clear();
};

ThresholdState VersionBitsState(const CBlockIndex* pindexPrev, const Consensus::Params& params, Consensus::DeploymentPos pos, VersionBitsCache& cache);
uint32_t VersionBitsMask(const Consensus::Params& params, Consensus::DeploymentPos pos);
bool IsVersionBitRejected(const Consensus::Params& params, Consensus::DeploymentPos pos);


#endif
