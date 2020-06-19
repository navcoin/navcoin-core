// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Copyright (c) 2018 The NavCoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chain.h>
#include <main.h>
#include <pos.h>
#include <primitives/block.h>

double GetDifficulty(const CBlockIndex* blockindex)
{
  // Floating point number that is a multiple of the minimum difficulty,
  // minimum difficulty = 1.0.
  if (blockindex == nullptr)
  {
      if (pindexBestHeader == nullptr)
          return 1.0;
      else
          blockindex = GetLastBlockIndex(pindexBestHeader, false);
  }

  int nShift = (blockindex->nBits >> 24) & 0xff;

  double dDiff =
      (double)0x0000ffff / (double)(blockindex->nBits & 0x00ffffff);

  while (nShift < 29)
  {
      dDiff *= 256.0;
      nShift++;
  }
  while (nShift > 29)
  {
      dDiff /= 256.0;
      nShift--;
  }

  return dDiff;
}

double GetPoWMHashPS()
{
    return 0;
}

double GetPoSKernelPS()
{
    int nTargetSpacing = GetTargetSpacing(pindexBestHeader->nHeight); // time per block
    int nPoSInterval = 1440 * 60 / nTargetSpacing; // 24 hours worth of blocks
    double dStakeKernelsTriedAvg = 0;
    int nStakesHandled = 0, nStakesTime = 0;

    CBlockIndex* pindex = pindexBestHeader;
    CBlockIndex* pindexPrevStake = nullptr;

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

    result *= nTargetSpacing;

    return result;
}
