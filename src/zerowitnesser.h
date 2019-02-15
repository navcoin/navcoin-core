// Copyright (c) 2019 The Navcoin Core developers
// Copyright (c) 2019 The DeVault Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef ZEROWITNESSER_H
#define ZEROWITNESSER_H

#include "wallet/wallet.h"

#define DEFAULT_BLOCKS_PER_ROUND 2
#define DEFAULT_BLOCK_SNAPSHOT 20

class CBlockIndex;
class CChainParams;

namespace Consensus { struct Params; };

// DeVault - Zero witnesser thread
void DeVaultWitnesser(const CChainParams& chainparams);

#endif // ZEROWITNESSER_H
