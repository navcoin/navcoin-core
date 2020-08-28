// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Copyright (c) 2018 The NavCoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <base58.h>
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

std::pair<CAmount, std::pair<CAmount, CAmount>> GetStakingCoins()
{
    int nTargetSpacing = GetTargetSpacing(pindexBestHeader->nHeight); // time per block
    int nPoSInterval = 1440 * 60 / nTargetSpacing; // 24 hours worth of blocks
    int nStakesHandled = 0;

    CBlockIndex* pindex = chainActive.Tip();

    std::set<CNavCoinAddress> stakers;

    while (pindex && nStakesHandled < nPoSInterval)
    {
        if (pindex->IsProofOfStake())
        {
            CBlock block;
            if(!ReadBlockFromDisk(block, pindex, Params().GetConsensus()))
                continue;
            if (block.vtx.size() < 2)
                continue;
            CTransaction coinstake = block.vtx[1];
            if (coinstake.IsCoinStake())
            {
                CScript script = coinstake.vout[1].scriptPubKey;
                CTxDestination destination;
                if (ExtractDestination(script, destination)) {
                    stakers.insert(CNavCoinAddress(destination));
                }
            }
            nStakesHandled++;
        }
        pindex = pindex->pprev;
    }

    CAmount hot = 0;
    CAmount cold = 0;
    CAmount coldv2 = 0;

    for (auto &address: stakers)
    {
        std::vector<std::pair<std::pair<uint160, uint160>, AddressHistoryFilter>> addresses;

        if (address.IsColdStakingv2Address(Params()))
        {
            CNavCoinAddress addressStaking;
            uint160 hashBytes, hashBytesStaking;
            int type = 0;

            address.GetStakingAddress(addressStaking);

            if (!address.GetIndexKey(hashBytes, type)) {
                continue;
            }

            if (!addressStaking.GetIndexKey(hashBytesStaking, type)) {
                continue;
            }
            addresses.push_back(std::make_pair(std::make_pair(hashBytes, hashBytesStaking), AddressHistoryFilter::STAKABLE));
        }
        else if (address.IsColdStakingAddress(Params()))
        {
            CNavCoinAddress addressStaking, addressSpending;
            uint160 hashBytes, hashBytesStaking, hashBytesSpending;
            int type = 0;

            if (!address.GetIndexKey(hashBytes, type)) {
                continue;
            }

            address.GetStakingAddress(addressStaking);

            if (!addressStaking.GetIndexKey(hashBytesStaking, type)) {
                continue;
            }
            addresses.push_back(std::make_pair(std::make_pair(hashBytes, hashBytesStaking), AddressHistoryFilter::STAKABLE_VOTING_WEIGHT));
        }
        else
        {
            uint160 hashBytes;
            int type = 0;
            if (!address.GetIndexKey(hashBytes, type)) {
                continue;
            }
            addresses.push_back(std::make_pair(std::make_pair(hashBytes, hashBytes), AddressHistoryFilter::ALL));
        }

        std::vector<std::pair<CAddressHistoryKey, CAddressHistoryValue> > addressHistory;

        for (std::vector<std::pair<std::pair<uint160, uint160>, AddressHistoryFilter>>::iterator it = addresses.begin(); it != addresses.end(); it++) {
            if (!GetAddressHistory((*it).first.first, (*it).first.second, addressHistory, (*it).second)) {
                continue;
            }
        }

        CAmount stakable = 0;

        for (std::vector<std::pair<CAddressHistoryKey, CAddressHistoryValue> >::const_iterator it=addressHistory.begin(); it!=addressHistory.end(); it++) {
            stakable += (*it).second.stakable;
        }

        if (address.IsColdStakingAddress(Params()))
            cold+=stakable;
        else if (address.IsColdStakingv2Address(Params()))
            coldv2+=stakable;
        else
            hot+=stakable;
    }

    return std::make_pair(hot, std::make_pair(cold, coldv2));
}
