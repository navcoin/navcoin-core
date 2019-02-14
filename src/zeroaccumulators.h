// Copyright (c) 2017-2018 The PIVX developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef NAV_ACCUMULATORMAP_H
#define NAV_ACCUMULATORMAP_H

#include "primitives/block.h"
#include "libzerocoin/Accumulator.h"
#include "libzerocoin/Coin.h"

class CBlock;
class CBlockIndex;
class CChain;

//A map with an accumulator for each denomination
class AccumulatorMap
{
private:
    const libzerocoin::ZerocoinParams* params;
    std::map<libzerocoin::CoinDenomination, std::unique_ptr<libzerocoin::Accumulator> > mapAccumulators;
    std::map<int, uint256> mapBlockHashes;
public:
    AccumulatorMap(const libzerocoin::ZerocoinParams* params);
    bool Accumulate(const libzerocoin::PublicCoin& pubCoin, bool fSkipValidation = false);
    bool Increment(const libzerocoin::CoinDenomination denom, const CBigNum& bnValue);
    CBigNum GetValue(libzerocoin::CoinDenomination denom);
    bool Get(libzerocoin::CoinDenomination denom, libzerocoin::Accumulator& accumulator);
    uint256 GetChecksum();
    uint256 GetBlockHash() { return mapBlockHashes.size() == 0 ? uint256() : mapBlockHashes.rbegin()->second; }
    uint256 GetFirstBlockHash() { return mapBlockHashes.size() == 0 ? uint256() : mapBlockHashes.begin()->second; }
    void Reset();
    void Reset(const libzerocoin::ZerocoinParams* params2);
    bool Load(uint256 nCheckpoint);
    bool Save(const std::pair<int, uint256>& blockIn);
    bool Disconnect(const std::pair<int, uint256>& blockIn);
};

bool CalculateAccumulatorChecksum(const CBlock* block, AccumulatorMap& accumulatorMap, std::vector<std::pair<CBigNum, uint256>>& vPubCoins);
#endif //NAV_ACCUMULATORMAP_H
