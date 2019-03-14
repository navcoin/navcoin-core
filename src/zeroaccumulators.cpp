// Copyright (c) 2018-2019 The NavCoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "chainparams.h"
#include "zeroaccumulators.h"
#include "main.h"
#include "txdb.h"
#include "zerochain.h"
#include "zerotx.h"

using namespace libzeroct;
using namespace std;

bool CalculateAccumulatorChecksum(const CBlock* block, Accumulator& accumulator, std::vector<std::pair<CBigNum, uint256>>& vPubCoins)
{
    for (auto& tx : block->vtx) {
        for (auto& out : tx.vout) {
            if (!out.IsZeroCTMint())
                continue;

            libzeroct::PublicCoin pubCoin(&Params().GetConsensus().ZeroCT_Params);

            if (!TxOutToPublicCoin(&Params().GetConsensus().ZeroCT_Params, out, pubCoin))
                return false;

            CBigNum bnValue = pubCoin.getValue();

            accumulator.increment(bnValue);

            vPubCoins.push_back(make_pair(pubCoin.getValue(), block->GetHash()));
        }
    }

    return true;
}
