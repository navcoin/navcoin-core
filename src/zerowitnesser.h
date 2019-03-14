// Copyright (c) 2019 The NavCoin Core developers
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

class CacheWitnessToWrite {
public:
    CacheWitnessToWrite() { }

    void Add(const CBigNum witness, PublicMintWitnessData data)
    {
        std::pair<std::map<const CBigNum, PublicMintWitnessData>::iterator, bool> ret = map.insert(std::make_pair(witness, data));

        if (!ret.second) {
            map.erase(witness);
            ret = map.insert(std::make_pair(witness, data));
            assert(ret.second);
        }
    }

    const std::map<const CBigNum, PublicMintWitnessData> GetMap()
    {
        return map;
    }

private:
    std::map<const CBigNum, PublicMintWitnessData> map;
};

// NAVCoin - Zero witnesser thread
void NavCoinWitnesser(const CChainParams& chainparams);

#endif // ZEROWITNESSER_H
