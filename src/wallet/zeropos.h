// Copyright (c) 2019 The NavCoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef ZEROPOS_H
#define ZEROPOS_H

#include "chainparams.h"
#include "main.h"
#include "wallet.h"
#include "zerochain.h"

#include "libzerocoin/Coin.h"

bool CheckZeroKernel(CBlockIndex* pindexPrev, const CCoinsViewCache& view, unsigned int nBits, int64_t nTime, const COutPoint& prevout, int64_t* pBlockTime, CAmount& nValue);
bool CheckZeroStakeKernelHash(CBlockIndex* pindexPrev, unsigned int nBits, int64_t nTime, const CBigNum& serialPublicNumber, CAmount nAmount, uint256& hashProofOfStake, uint256& targetProofOfStake);

#endif // ZEROPOS_H
