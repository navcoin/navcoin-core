// Copyright (c) 2019 The NavCoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "zerotx.h"

using namespace libzerocoin;

bool TxOutToPublicCoin(const ZerocoinParams *params, const CTxOut& txout, PublicCoin& pubCoin, CValidationState* state, bool fCheck)
{
    if (!txout.scriptPubKey.IsZerocoinMint())
        return false;

    CoinDenomination denomination = AmountToZerocoinDenomination(txout.nValue);
    if (denomination == ZQ_ERROR)
        return state != NULL ? state->DoS(100, error("TxOutToPublicCoin(): txout.nValue is not a valid denomination value")) : false;

    std::vector<unsigned char> c; CPubKey p; std::vector<unsigned char> i;
    if(!txout.scriptPubKey.ExtractZerocoinMintData(p, c, i))
        return state != NULL ? state->DoS(100, error("TxOutToPublicCoin(): could not read mint data from txout.scriptPubKey")) : false;

    CBigNum pid;
    pid.setvch(i);

    PublicCoin checkPubCoin(params, denomination, CBigNum(c), p, pid, fCheck);
    pubCoin = checkPubCoin;

    return true;
}

bool TxInToCoinSpend(const ZerocoinParams *params, const CTxIn& txin, CoinSpend& coinSpend)
{
    return ScriptToCoinSpend(params, txin.scriptSig, coinSpend);
}

bool ScriptToCoinSpend(const ZerocoinParams *params, const CScript& scriptSig, CoinSpend& coinSpend)
{
    if (!scriptSig.IsZerocoinSpend())
        return false;

    std::vector<char, zero_after_free_allocator<char> > dataTxIn;
    dataTxIn.insert(dataTxIn.end(), scriptSig.begin() + BIGNUM_SIZE, scriptSig.end());
    CDataStream serializedCoinSpend(dataTxIn, SER_NETWORK, PROTOCOL_VERSION);

    libzerocoin::CoinSpend spend(params, serializedCoinSpend);

    coinSpend = spend;

    return true;
}
