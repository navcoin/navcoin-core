// Copyright (c) 2018-2019 The NavCoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef ZEROCHAIN_H
#define ZEROCHAIN_H

#include "chainparams.h"
#include "consensus/validation.h"
#include "libzeroct/Coin.h"
#include "libzeroct/CoinSpend.h"
#include "libzeroct/BulletproofsRangeproof.h"
#include "main.h"
#include "primitives/transaction.h"
#include "zeromint.h"

#define COINSPEND_CACHE_SIZE 255

using namespace libzeroct;

// consider moving to unordered map
static std::map<uint256, bool> mapCacheValidCoinSpends;

extern CCriticalSection cs_dummy;

bool CheckZeroCTMint(const ZeroCTParams *params, const CTxOut& txout, const CCoinsViewCache& view, CValidationState& state, PublicCoin* pPubCoin = NULL, bool fCheck = true, bool fFast = false);
bool BlockToZeroCTMints(const ZeroCTParams *params, const CBlock* block, std::vector<PublicCoin> &vPubCoins);
bool CountMintsFromHeight(unsigned int nInitialHeight, unsigned int& nRet);
bool CalculateWitnessForMint(const CTxOut& txout, const CCoinsViewCache& view, const PublicCoin& pubCoin, Accumulator& accumulator, AccumulatorWitness& AccumulatorWitness, CBigNum& accumulatorValue, uint256& blockAccumulatorHash, std::string& strError, int nRequiredMints, int nMaxHeight);
bool CheckZeroCTSpend(const ZeroCTParams *params, const CTxIn& txin, const CCoinsViewCache& view, CValidationState& state, CoinSpend* pCoinSpend = NULL, Accumulator* accumulator = NULL, bool fSpendCheck = true);
bool VerifyCoinSpend(const CoinSpend& coinSpend, const Accumulator &accumulator, bool fWriteToCache);
bool VerifyCoinSpendNoCache(const CoinSpend& coinSpend, const Accumulator &accumulator);
bool VerifyCoinSpendCache(const CoinSpend& coinSpend, const Accumulator &accumulator);
bool VerifyZeroCTBalance(const ZeroCTParams *params, const CTransaction& transaction, const CCoinsViewCache& view, const CAmount nReward = 0);

#endif // ZEROCHAIN_H
