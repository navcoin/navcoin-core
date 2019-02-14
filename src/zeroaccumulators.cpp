// Copyright (c) 2017-2018 The PIVX developers
// Copyright (c) 2018 The NavCoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "chainparams.h"
#include "zeroaccumulators.h"
#include "main.h"
#include "txdb.h"
#include "libzerocoin/Denominations.h"
#include "zerochain.h"
#include "zerotx.h"

using namespace libzerocoin;
using namespace std;

std::map<uint256, std::pair<std::map<int, uint256>,std::vector<std::pair<libzerocoin::CoinDenomination,CBigNum>>>> mapCacheAccumulatorMap;

uint32_t GetChecksumFromBn(const CBigNum &bnValue)
{
    CDataStream ss(SER_GETHASH, 0);
    ss << bnValue;
    uint256 hash = Hash(ss.begin(), ss.end());

    return hash.Get32();
}

//Construct accumulators for all denominations
AccumulatorMap::AccumulatorMap(const libzerocoin::ZerocoinParams* params)
{
    this->params = params;
    for (auto& denom : zerocoinDenomList) {
        unique_ptr<Accumulator> uptr(new Accumulator(params, denom));
        mapAccumulators.insert(make_pair(denom, std::move(uptr)));
    }
}

//Reset each accumulator to its default state
void AccumulatorMap::Reset()
{
    Reset(params);
}

void AccumulatorMap::Reset(const libzerocoin::ZerocoinParams* params2)
{
    this->params = params2;
    mapAccumulators.clear();
    for (auto& denom : zerocoinDenomList) {
        unique_ptr<Accumulator> uptr(new Accumulator(params, denom));
        mapAccumulators.insert(make_pair(denom, std::move(uptr)));
    }
}

//Add a zerocoin to the accumulator of its denomination.
bool AccumulatorMap::Accumulate(const PublicCoin& pubCoin, bool fSkipValidation)
{
    CoinDenomination denom = pubCoin.getDenomination();
    if (denom == CoinDenomination::ZQ_ERROR)
        return false;

    if (fSkipValidation)
        mapAccumulators.at(denom)->increment(pubCoin.getValue());
    else
        mapAccumulators.at(denom)->accumulate(pubCoin);

    return true;
}

//Add a zerocoin to the accumulator of its denomination using directly its value
bool AccumulatorMap::Increment(const CoinDenomination denom, const CBigNum& bnValue)
{
    mapAccumulators.at(denom)->increment(bnValue);
    return true;
}

//Get the value of a specific accumulator
CBigNum AccumulatorMap::GetValue(CoinDenomination denom)
{
    if (denom == CoinDenomination::ZQ_ERROR)
        return CBigNum(0);
    return mapAccumulators.at(denom)->getValue();
}

//Returns a specific accumulator
bool AccumulatorMap::Get(CoinDenomination denom, libzerocoin::Accumulator& accumulator)
{
    if (denom == CoinDenomination::ZQ_ERROR)
        return false;
    accumulator.setValue(mapAccumulators.at(denom)->getValue());
    return true;
}

//Calculate a 32bit checksum of each accumulator value. Concatenate checksums into uint256
uint256 AccumulatorMap::GetChecksum()
{
    uint256 nCombinedChecksum;

    //Prevent possible overflows from future changes to the list and forgetting to update this code
    assert(zerocoinDenomList.size() == 8);
    for (auto& denom : zerocoinDenomList) {
        if(mapAccumulators.at(denom)) {
            CBigNum bnValue = mapAccumulators.at(denom)->getValue();
            uint32_t nCheckSum = GetChecksumFromBn(bnValue);
            nCombinedChecksum = nCombinedChecksum << 32 | nCheckSum;
        }
    }

    return nCombinedChecksum;
}

//Load a checkpoint containing 8 32bit checksums of accumulator values.
bool AccumulatorMap::Load(uint256 nChecksum)
{
    mapBlockHashes.clear();

    std::pair<std::map<int,uint256>,std::vector<std::pair<libzerocoin::CoinDenomination,CBigNum>>> toRead;

    if (mapCacheAccumulatorMap.count(nChecksum)) {
        toRead = mapCacheAccumulatorMap[nChecksum];
    } else if (!pblocktree->ReadZerocoinAccumulator(nChecksum, toRead))
        return error("%s : cannot read zerocoin accumulator checksum %s", __func__, nChecksum.ToString());

    for(auto& it : toRead.second)
    {
        mapAccumulators.at(it.first)->setValue(it.second);
    }

    mapBlockHashes = toRead.first;

    return true;
}

bool AccumulatorMap::Save(const std::pair<int, uint256>& blockIn)
{
    std::vector<std::pair<libzerocoin::CoinDenomination,CBigNum>> vAcc;

    for(auto& it : mapAccumulators)
        vAcc.push_back(std::make_pair(it.first, it.second->getValue()));

    mapBlockHashes.insert(blockIn);

    mapCacheAccumulatorMap[GetChecksum()] = std::make_pair(mapBlockHashes, vAcc);

    if (!pblocktree->WriteZerocoinAccumulator(GetChecksum(), std::make_pair(mapBlockHashes, vAcc)))
        return error("%s : cannot write zerocoin accumulator checksum %s", __func__, GetChecksum().ToString());

    return true;
}

bool AccumulatorMap::Disconnect(const std::pair<int, uint256>& blockIn)
{
    if (mapBlockHashes.size() == 0 || !mapBlockHashes.count(blockIn.first))
        return false;

    if (mapBlockHashes.size() == 1 && mapBlockHashes.at(blockIn.first) == blockIn.second)
    {
        if (!pblocktree->EraseZerocoinAccumulator(GetChecksum()))
            return error("%s : cannot erase zerocoin accumulator checksum %s", __func__, GetChecksum().ToString());
        else
            return true;
    }

    std::vector<std::pair<libzerocoin::CoinDenomination,CBigNum>> vAcc;

    vAcc.clear();

    for(auto& it : mapAccumulators)
    {
        vAcc.push_back(std::make_pair(it.first, it.second->getValue()));
    }

    mapBlockHashes.erase(blockIn.first);
    mapCacheAccumulatorMap.erase(GetChecksum());

    if (!pblocktree->WriteZerocoinAccumulator(GetChecksum(), std::make_pair(mapBlockHashes, vAcc)))
        return error("%s : cannot write zerocoin accumulator checksum %s", __func__, GetChecksum().ToString());

    return true;
}

bool CalculateAccumulatorChecksum(const CBlock* block, AccumulatorMap& accumulatorMap, std::vector<std::pair<CBigNum, uint256>>& vPubCoins)
{
    for (auto& tx : block->vtx) {
        for (auto& out : tx.vout) {
            if (!out.IsZerocoinMint())
                continue;

            libzerocoin::PublicCoin pubCoin(&Params().GetConsensus().Zerocoin_Params);

            if (!TxOutToPublicCoin(&Params().GetConsensus().Zerocoin_Params, out, pubCoin))
                return false;

            if (!accumulatorMap.Increment(pubCoin.getDenomination(), pubCoin.getValue()))
                return false;

            vPubCoins.push_back(make_pair(pubCoin.getValue(), block->GetHash()));
        }
    }

    return true;
}
