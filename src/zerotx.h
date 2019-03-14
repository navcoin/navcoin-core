// Copyright (c) 2019 The NavCoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "chainparams.h"
#include "consensus/validation.h"
#include "libzeroct/Coin.h"
#include "libzeroct/CoinSpend.h"
#include "libzeroct/SerialNumberProofOfKnowledge.h"

#ifndef ZEROTX_H
#define ZEROTX_H

#define BIGNUM_SIZE   4

using namespace libzeroct;

bool TxOutToPublicCoin(const ZeroCTParams *params, const CTxOut& txout, PublicCoin& pubCoin, CValidationState* state = NULL, bool fCheck = false);
bool TxInToCoinSpend(const ZeroCTParams *params, const CTxIn& txin, CoinSpend& coinSpend);
bool ScriptToCoinSpend(const ZeroCTParams *params, const CScript& scriptSig, CoinSpend& coinSpend);

#endif // ZEROTX_H
