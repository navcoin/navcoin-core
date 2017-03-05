// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Copyright (c) 2009-2015 The NavCoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "chain.h"
#include "main.h"
#include "pos.h"
#include "primitives/block.h"

double GetDifficulty(const CBlockIndex* blockindex)
{
    return 1.0;
}

double GetPoWMHashPS()
{
    return 0;
}

double GetPoSKernelPS()
{
    int nPoSInterval = 72;
    double dStakeKernelsTriedAvg = 0;
    int nStakesHandled = 0, nStakesTime = 0;

    CBlockIndex* pindex = pindexBestHeader;
    CBlockIndex* pindexPrevStake = NULL;

    while (pindex && nStakesHandled < nPoSInterval)
    {
        if (pindex->IsProofOfStake())
        {
            if (pindexPrevStake)
            {
                dStakeKernelsTriedAvg += GetDifficulty(pindexPrevStake) * 4294967296.0;
                nStakesTime += pindexPrevStake->nTime - pindex->nTime;
                nStakesHandled++;
            }
            pindexPrevStake = pindex;
        }

        pindex = pindex->pprev;
    }

    double result = 0;

    if (nStakesTime)
        result = dStakeKernelsTriedAvg / nStakesTime;

    result *= STAKE_TIMESTAMP_MASK + 1;

    return result;
}
