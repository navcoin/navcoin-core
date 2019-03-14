// Copyright (c) 2019 The NavCoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef ZEROPOS_H
#define ZEROPOS_H

#include "chainparams.h"
#include "main.h"
#include "wallet.h"
#include "zerochain.h"

#include "libzeroct/Coin.h"

#define KERNEL_PROOF_BITS   7

bool CheckZeroKernel(CBlockIndex* pindexPrev, const CCoinsViewCache& view, unsigned int nBits, int64_t nTime, const COutPoint& prevout, int64_t* pBlockTime, CAmount& nValue, uint256& hashProofOfStake, uint256& targetProofOfStake);
bool CheckZeroStakeKernelHashCandidate(CBlockIndex* pindexPrev, unsigned int nBits, int64_t nTime, const CBigNum& serialPublicNumber, CAmount nAmount, uint256& hashProofOfStake, uint256& targetProofOfStake);
bool CheckZeroCTKernelHash(CBlockIndex* pindexPrev, unsigned int nBits, const CTransaction& CTransaction, const libzeroct::CoinSpend& cs, uint256& hashProofOfStake, uint256& targetProofOfStake);
uint256 GetZeroCTKernelHash(uint64_t nStakeModifier, int nStakeModifierHeight, int64_t nStakeModifierTime, CBigNum bnSerialNumber, uint64_t nTime, int nShift);

#endif // ZEROPOS_H
