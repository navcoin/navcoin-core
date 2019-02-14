// Copyright (c) 2018-2019 The NavCoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef ZEROCHAIN_H
#define ZEROCHAIN_H

#include "chainparams.h"
#include "consensus/validation.h"
#include "libzerocoin/Coin.h"
#include "libzerocoin/CoinSpend.h"
#include "main.h"
#include "primitives/transaction.h"
#include "zeromint.h"

#define COINSPEND_CACHE_SIZE 255

using namespace libzerocoin;

// consider moving to unordered map
static std::map<uint256, bool> mapCacheValidCoinSpends;

extern CCriticalSection cs_dummy;

bool CheckZerocoinMint(const ZerocoinParams *params, const CTxOut& txout, const CCoinsViewCache& view, CValidationState& state, std::vector<std::pair<CBigNum, PublicMintChainData>> vSeen = std::vector<std::pair<CBigNum, PublicMintChainData>>(), PublicCoin* pPubCoin = NULL, bool fCheck = true, bool fFast = false);
bool BlockToZerocoinMints(const ZerocoinParams *params, const CBlock* block, std::vector<PublicCoin> &vPubCoins);
bool CountMintsFromHeight(unsigned int nInitialHeight, CoinDenomination denom, unsigned int& nRet);
bool CalculateWitnessForMint(const CTxOut& txout, const PublicCoin& pubCoin, Accumulator& accumulator, AccumulatorWitness& AccumulatorWitness, uint256& AccumulatorChecksum, std::string& strError, int nRequiredMints, int nMaxHeight);
bool CheckZerocoinSpend(const ZerocoinParams *params, const CTxIn& txin, const CCoinsViewCache& view, CValidationState& state, std::vector<std::pair<CBigNum, uint256>> vSeen = std::vector<std::pair<CBigNum, uint256>>(), CoinSpend* pCoinSpend = NULL, Accumulator* accumulator = NULL, bool fSpendCheck = true);
bool VerifyCoinSpend(const CoinSpend& coinSpend, const Accumulator &accumulator, bool fWriteToCache);
#endif // ZEROCHAIN_H
