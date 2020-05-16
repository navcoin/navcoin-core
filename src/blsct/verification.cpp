// Copyright (c) 2020 The NavCoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "verification.h"

bool VerifyBLSCT(const CTransaction &tx, const bls::PrivateKey& viewKey, std::vector<RangeproofEncodedData> &vData, const CCoinsViewCache& view, CValidationState& state, bool fOnlyRecover, CAmount nMixFee)
{
    std::vector<std::pair<int, BulletproofsRangeproof>> proofs;
    std::vector<Point> nonces;

    Point balKey;
    bool fPointZero = true;

    bool fCheckRange = tx.IsCTOutput();
    bool fCheckBLSSignature = tx.IsBLSInput();
    bool fCheckBalance = fCheckBLSSignature || fCheckRange;

    if (fOnlyRecover)
    {
        fCheckBLSSignature = false;
        fCheckBalance = false;
    }

    if (!(fCheckRange || fCheckBalance || fCheckBLSSignature))
        return true;

    std::vector<bls::PublicKey> txSigningKeys;
    std::vector<const uint8_t*> vMessages;

    CAmount valIn = 0;
    CAmount valOut = 0;

    if (nMixFee != 0)
    {
        Scalar sMixFee = Scalar(nMixFee);

        if (nMixFee < 0)
            sMixFee = sMixFee.Negate();

        balKey = fPointZero ? (BulletproofsRangeproof::H*sMixFee) : balKey + (BulletproofsRangeproof::H*sMixFee);
        valIn += nMixFee;
        fPointZero = false;
    }

    for (size_t j = 0; j < tx.vin.size(); j++)
    {
        if (fCheckBalance || fCheckBLSSignature)
        {
            const CTxOut &prevOut = view.GetOutputFor(tx.vin[j]);

            if (fCheckBalance)
            {
                if (prevOut.HasRangeProof())
                {
                    balKey = fPointZero ? prevOut.bp.GetValueCommitments()[0] : balKey + prevOut.bp.GetValueCommitments()[0];
                    fPointZero = false;
                }
                else
                {
                    balKey = fPointZero ? (BulletproofsRangeproof::H*Scalar(prevOut.nValue)) : balKey + (BulletproofsRangeproof::H*Scalar(prevOut.nValue));
                    valIn += prevOut.nValue;
                    fPointZero = false;
                }
            }

            if (prevOut.spendingKey.size() > 0 && fCheckBLSSignature)
            {
                txSigningKeys.push_back(prevOut.GetSpendingKey());
                CHashWriter hasher(0,0);
                hasher << tx.vin[j];
                uint256 hash = hasher.GetHash();
                unsigned char *h;
                h = new unsigned char [32];
                memcpy(h, hash.begin(), 32);
                vMessages.push_back(h);
            }
        }
    }

    for (size_t j = 0; j < tx.vout.size(); j++)
    {
        if (tx.vout[j].HasRangeProof())
        {
            if (fCheckRange)
            {
                proofs.push_back(std::make_pair(j, tx.vout[j].bp));
                nonces.push_back(bls::BLS::DHKeyExchange(viewKey, tx.vout[j].GetBlindingKey()));
            }
            if (fCheckBalance)
            {
                balKey = fPointZero ? tx.vout[j].bp.GetValueCommitments()[0] : balKey - tx.vout[j].bp.GetValueCommitments()[0];
                fPointZero = false;
            }
        }
        else if (fCheckBalance)
        {
            balKey = fPointZero ? (BulletproofsRangeproof::H*Scalar(tx.vout[j].nValue)) : balKey - (BulletproofsRangeproof::H*Scalar(tx.vout[j].nValue));
            valOut += tx.vout[j].nValue;
            fPointZero = false;
        }

        if (fCheckBLSSignature && !tx.vout[j].scriptPubKey.IsFee())
        {
            txSigningKeys.push_back(tx.vout[j].GetBlindingKey());
            uint256 hash = tx.vout[j].GetHash();
            unsigned char *h;
            h = new unsigned char [32];
            memcpy(h, hash.begin(), 32);
            vMessages.push_back(h);
        }
    }

    if (fCheckRange && proofs.size() > 0)
    {
        if (!VerifyBulletproof(proofs, vData, nonces, fOnlyRecover))
        {
            return state.DoS(100, false, REJECT_INVALID, "invalid-rangeproof");
        }
    }

    if (fCheckBalance)
    {
        bls::PublicKey balanceKey = bls::PublicKey::FromG1(&(balKey.g1));
        bls::Signature sig = tx.GetBalanceSignature();
        sig.SetAggregationInfo(bls::AggregationInfo::FromMsg(balanceKey, balanceMsg, sizeof(balanceMsg)));

        if (!sig.Verify())
            return state.DoS(100, false, REJECT_INVALID, strprintf("invalid-balanceproof"));
    }

    if (fCheckBLSSignature)
    {
        bls::PrependSignature txsig = tx.GetTxSignature();
        if (!txsig.Verify(vMessages, txSigningKeys))
            return state.DoS(100, false, REJECT_INVALID, "invalid-bls-signature");
    }

    return true;
}

bool CombineBLSCTTransactions(std::vector<CTransaction> &vTx, CTransaction& outTx, const CCoinsViewCache& inputs, CValidationState& state, CAmount nMixFee)
{
    std::set<CTxIn> setInputs;
    std::set<CTxOut> setOutputs;

    std::vector<bls::InsecureSignature> balanceSigs;
    std::vector<bls::PrependSignature> txSigs;

    if (vTx.size() == 0)
        return state.DoS(100, false, REJECT_INVALID, strprintf("empty-vector-combine-blsct"));

    bool fAnyCTOutput = false;
    CAmount nFee = 0;

    for (auto& tx: vTx)
    {
        if (!tx.IsBLSInput())
            return state.DoS(100, false, REJECT_INVALID, strprintf("cant-combine-non-blsct"));

        for (auto& in: tx.vin)
        {
            for (auto &cin: setInputs)
            {
                if (cin == in)
                    return state.DoS(100, false, REJECT_INVALID, strprintf("duplicate-input"));
            }

            setInputs.insert(in);
        }

        for (auto& out: tx.vout)
        {
            if (out.HasRangeProof())
            {
                fAnyCTOutput = true;
            }

            if (out.scriptPubKey.IsFee())
            {
                nFee += out.nValue;
                continue;
            }

            for (auto &cout: setOutputs)
            {
                if (cout == out)
                    return state.DoS(100, false, REJECT_INVALID, strprintf("duplicate-output"));
            }

            setOutputs.insert(out);
        }

        balanceSigs.push_back(tx.GetBalanceSignature().GetInsecureSig());
        txSigs.push_back(tx.GetTxSignature());
    }

    CMutableTransaction mutOutTx;
    mutOutTx.nVersion = TX_BLS_INPUT_FLAG;
    if (fAnyCTOutput)
        mutOutTx.nVersion |= TX_BLS_CT_FLAG;
    mutOutTx.nTime = GetTime();
    mutOutTx.vin.clear();
    mutOutTx.vout.clear();

    for (auto& in: setInputs)
    {
        mutOutTx.vin.push_back(in);
    }

    for (auto& out: setOutputs)
    {
        mutOutTx.vout.push_back(out);
    }

    mutOutTx.vout.push_back(CTxOut(nFee, CScript(OP_RETURN)));
    mutOutTx.SetBalanceSignature(bls::Signature::FromInsecureSig(bls::InsecureSignature::Aggregate(balanceSigs)));
    mutOutTx.SetTxSignature(bls::PrependSignature::Aggregate(txSigs));

    outTx = mutOutTx;

    std::vector<RangeproofEncodedData> blsctData;

    if (!MoneyRange(nMixFee))
        return state.DoS(100, false, REJECT_INVALID, strprintf("%s: Can't combine transactions with incorrect fee %d\n", __func__, nMixFee));

    return VerifyBLSCT(outTx, bls::PrivateKey::FromBN(Scalar::Rand().bn), blsctData, inputs, state, false, nMixFee);
}
