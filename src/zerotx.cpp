// Copyright (c) 2019 The NavCoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "zerotx.h"

using namespace libzeroct;

bool TxOutToPublicCoin(const ZeroCTParams *params, const CTxOut& txout, PublicCoin& pubCoin, CValidationState* state, bool fCheck)
{
    if (!txout.scriptPubKey.IsZeroCTMint())
        return false;

    std::vector<unsigned char> c; CPubKey p; std::vector<unsigned char> i; std::vector<unsigned char> a;
    std::vector<unsigned char> ac;
    if(!txout.scriptPubKey.ExtractZeroCTMintData(p, c, i, a, ac))
        return state != NULL ? state->DoS(100, error("TxOutToPublicCoin(): could not read mint data from txout.scriptPubKey")) : false;

    CBigNum pid;
    pid.setvch(i);

    CBigNum oa;
    oa.setvch(a);

    CBigNum aco;
    aco.setvch(ac);

    PublicCoin checkPubCoin(params, CBigNum(c), p, pid, oa, aco, fCheck);
    pubCoin = checkPubCoin;

    return true;
}

bool TxInToCoinSpend(const ZeroCTParams *params, const CTxIn& txin, CoinSpend& coinSpend)
{
    return ScriptToCoinSpend(params, txin.scriptSig, coinSpend);
}

bool ScriptToCoinSpend(const ZeroCTParams *params, const CScript& scriptSig, CoinSpend& coinSpend)
{
    if (!scriptSig.IsZeroCTSpend())
        return false;

    std::vector<unsigned char> dataTxIn;

    dataTxIn.clear();
    dataTxIn.insert(dataTxIn.end(), scriptSig.begin() + 1, scriptSig.end());

    CDataStream serializedCoinSpend(dataTxIn, SER_NETWORK, PROTOCOL_VERSION);

    libzeroct::CoinSpend spend(params, serializedCoinSpend);

    coinSpend = spend;

    return true;
}
