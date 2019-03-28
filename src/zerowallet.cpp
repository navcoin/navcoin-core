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
    } else {
        CRecipient recipient = {scriptPubKey, nValue, fSubtractFeeFromAmount, "", 0};
        vecSend.push_back(recipient);
    }

    random_shuffle(vecSend.begin(), vecSend.end(), GetRandInt);

    if (fShowDialog)
        uiInterface.ShowProgress(_("Constructing transaction..."), 100);

    return true;
}

bool PrepareAndSignCoinSpend(const BaseSignatureCreator& creator, const std::map<CBigNum, PublicMintWitnessData>& mapWitness, const CCoinsViewCache& view, const CScript& scriptPubKey, const CAmount& amount,
                             CScript& sigdata, CBigNum& r, bool fStake)
{
    if (!scriptPubKey.IsZeroCTMint())
        return error(strprintf("PrepareAndSignCoinSpend(): Transaction output script is not a ZeroCT mint."));

    string strError = "";
    const libzeroct::ZeroCTParams* params = &Params().GetConsensus().ZeroCT_Params;
    libzeroct::Accumulator a(params);
    libzeroct::AccumulatorWitness aw(params, a, libzeroct::PublicCoin(params));
    uint256 bah;
    CBigNum ac;

    CTxOut txout(amount, scriptPubKey);

    CBigNum pubCoinValue;

    if (!TxOutToPublicCoinValue(params, txout, pubCoinValue, NULL))
        return error(strprintf("PrepareAndSignCoinSpend(): Could not convert transaction otuput to public coin"));

    bool fFoundWitness = false;
    int nEntropy = rand() % WITNESS_ADDED_ENTROPY;

    libzeroct::PublicCoin pubCoin(params);

    if (mapWitness.count(pubCoinValue))
    {
        PublicMintWitnessData witnessData = pwalletMain->mapWitness.at(pubCoinValue);
        pubCoin = witnessData.GetPublicCoin();

        bah = witnessData.GetBlockAccumulatorHash();
        aw = witnessData.GetAccumulatorWitness();
        a = witnessData.GetAccumulator();

        if (witnessData.Verify()) {
            if (witnessData.GetCount() > (MIN_MINT_SECURITY + nEntropy))
                fFoundWitness = true;
        } else {
            return error("PrepareAndSignCoinSpend(): Something is wrong with the witness map. One of the precomputed witnesses did not verify.\n");
        }

        if (mapBlockIndex.count(bah) == 0) {
            LogPrintf("PrepareAndSignCoinSpend(): block hash %s is not known", bah.ToString());
            fFoundWitness = false;
        }
    }
    else
    {
        return error("PrepareAndSignCoinSpend(): Could not find public coin in the witness map");
    }

    if (!fFoundWitness && !CalculateWitnessForMint(txout, view, pubCoin, a, aw, ac, bah, strError, MIN_MINT_SECURITY + nEntropy,
                                                   chainActive.Tip()->nHeight - (fStake ? COINBASE_MATURITY : 0)))
        return error(strprintf("PrepareAndSignCoinSpend(): Error calculating witness for mint: %s", strError));

    if (!creator.CreateCoinSpendScript(params, pubCoin, a, bah, aw, scriptPubKey, sigdata, r, fStake, strError))
        return error(strprintf("PrepareAndSignCoinSpend(): Error creating coin spend: %s", strError));

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
