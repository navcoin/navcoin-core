// Copyright (c) 2019 The NavCoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "chainparams.h"
#include "consensus/validation.h"
#include "libzerocoin/Coin.h"
#include "libzerocoin/CoinSpend.h"

#ifndef ZEROTX_H
#define ZEROTX_H

#define BIGNUM_SIZE   4

using namespace libzerocoin;

bool TxOutToPublicCoin(const ZerocoinParams *params, const CTxOut& txout, PublicCoin& pubCoin, CValidationState* state = NULL, bool fCheck = false);
bool TxInToCoinSpend(const ZerocoinParams *params, const CTxIn& txin, CoinSpend& coinSpend);
bool ScriptToCoinSpend(const ZerocoinParams *params, const CScript& scriptSig, CoinSpend& coinSpend);

#endif // ZEROTX_H
