// Copyright (c) 2018-2019 The NavCoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef NAV_ACCUMULATORMAP_H
#define NAV_ACCUMULATORMAP_H

#include "primitives/block.h"
#include "libzeroct/Accumulator.h"

class CBlock;

bool CalculateAccumulatorChecksum(const CBlock* block, libzeroct::Accumulator& ac, std::vector<std::pair<CBigNum, uint256>>& vPubCoins);
#endif //NAV_ACCUMULATORMAP_H
