// Copyright (c) 2019 The NavCoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "zerowallet.h"
#include "zerotx.h"

bool DestinationToVecRecipients(CAmount nValue, const std::string &strAddress, vector<CRecipient> &vecSend, bool fSubtractFeeFromAmount,
                                bool fDonate, bool fShowDialog)
{
    CNavCoinAddress a(strAddress);

    if(!a.IsValid())
        return false;

    CTxDestination address = a.Get();

    return DestinationToVecRecipients(nValue, address, vecSend, fSubtractFeeFromAmount, fDonate, fShowDialog);
}

bool DestinationToVecRecipients(CAmount nValue, const CTxDestination &address, vector<CRecipient> &vecSend, bool fSubtractFeeFromAmount,
                                bool fDonate, bool fShowDialog)
{
    vecSend.clear();

    if (fShowDialog)
        uiInterface.ShowProgress(_("Constructing transaction..."), 0);

    CScript scriptPubKey = GetScriptForDestination(address);

    if (fShowDialog)
        uiInterface.ShowProgress(_("Constructing transaction..."), 50);

    if (fDonate)
        CFund::SetScriptForCommunityFundContribution(scriptPubKey);

    if (scriptPubKey.IsZeroCTMint()) {
        CRecipient recipient = {scriptPubKey, nValue, false, "",
                                boost::get<libzeroct::CPrivateAddress>(address).GetGamma()};
        vecSend.push_back(recipient);
        if (fShowDialog)
            uiInterface.ShowProgress(_("Constructing transaction..."), 100);
    } else {
        CRecipient recipient = {scriptPubKey, nValue, fSubtractFeeFromAmount, "", 0};
        vecSend.push_back(recipient);
    }

    random_shuffle(vecSend.begin(), vecSend.end(), GetRandInt);

    return true;
}

bool PrepareAndSignCoinSpend(const BaseSignatureCreator& creator, const std::map<CBigNum, PublicMintWitnessData>& mapWitness, const CCoinsViewCache& view, const CScript& scriptPubKey, const CAmount& amount,
                             CScript& sigdata, CBigNum& r, bool fStake)
{
    if (!scriptPubKey.IsZeroCTMint())
        return error(strprintf("Transaction output script is not a ZeroCT mint."));

    string strError = "";
    libzeroct::Accumulator a(&Params().GetConsensus().ZeroCT_Params);
    libzeroct::AccumulatorWitness aw(&Params().GetConsensus().ZeroCT_Params, a,
                                       libzeroct::PublicCoin(&Params().GetConsensus().ZeroCT_Params));
    uint256 bah;
    CBigNum ac;

    CTxOut txout(amount, scriptPubKey);

    libzeroct::PublicCoin pubCoin(&Params().GetConsensus().ZeroCT_Params);

    if (!TxOutToPublicCoin(&Params().GetConsensus().ZeroCT_Params, txout, pubCoin, NULL))
        return error(strprintf("Could not convert transaction otuput to public coin"));

    bool fFoundWitness = false;
    int nEntropy = rand() % WITNESS_ADDED_ENTROPY;

    if (mapWitness.count(pubCoin.getValue()))
    {
        PublicMintWitnessData witnessData = pwalletMain->mapWitness.at(pubCoin.getValue());

        int nCalculatedBlocksAgo = std::numeric_limits<unsigned int>::max();

        bah = witnessData.GetBlockAccumulatorHash();
        aw = witnessData.GetAccumulatorWitness();
        a = witnessData.GetAccumulator();

        if (mapBlockIndex.count(bah))
        {
            LOCK(cs_main);
            CBlockIndex* pindex = mapBlockIndex[bah];
            if (chainActive.Contains(pindex))
                nCalculatedBlocksAgo = chainActive.Height() - pindex->nHeight;
        }

        if (witnessData.Verify() &&
                (witnessData.GetCount() > (MIN_MINT_SECURITY + nEntropy) || nCalculatedBlocksAgo < (MIN_MINT_SECURITY/2)))
            fFoundWitness = true;

        if (bah == uint256())
            fFoundWitness = false;
    }

    if (!fFoundWitness && !CalculateWitnessForMint(txout, view, pubCoin, a, aw, ac, bah, strError, MIN_MINT_SECURITY + nEntropy,
                                                   chainActive.Tip()->nHeight - (fStake ? COINBASE_MATURITY : 0)))
        return error(strprintf("Error calculating witness for mint: %s", strError));

    if (!creator.CreateCoinSpendScript(&Params().GetConsensus().ZeroCT_Params, pubCoin, a, bah, aw, scriptPubKey, sigdata, r, fStake, strError))
        return error(strprintf("Error creating coin spend: %s", strError));

    return true;
}

bool ProduceCoinSpend(const BaseSignatureCreator& creator, const std::map<CBigNum, PublicMintWitnessData>& mapWitness, const CCoinsViewCache& view, const CScript& fromPubKey, SignatureData& sigdata, bool fCoinStake, CAmount amount)
{
    CScript script = fromPubKey;
    bool solved = true;
    CScript result;
    CBigNum r;
    solved = PrepareAndSignCoinSpend(creator, mapWitness, view, script, amount, result, r, fCoinStake);
    sigdata.scriptWitness.stack.clear();
    sigdata.scriptSig = result;
    sigdata.r = r;

    // Test solution
    return solved;
}
