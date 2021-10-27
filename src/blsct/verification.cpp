// Copyright (c) 2020 The Navcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "verification.h"
#include "utiltime.h"

bool VerifyBLSCT(const CTransaction &tx, bls::PrivateKey viewKey, std::vector<RangeproofEncodedData> &vData, const CStateViewCache& view, CValidationState& state, bool fOnlyRecover, CAmount nMixFee)
{
    std::map<uint256, std::vector<std::pair<int, BulletproofsRangeproof>>> proofs;
    std::map<uint256, std::vector<bls::G1Element>> nonces;


    bls::G1Element balKey, balKeyOut;
    bool fElementZero = true;
    bool fElementZeroOut = true;

    bool fCheckRange = tx.IsCTOutput();
    bool fCheckBLSSignature = tx.IsBLSInput();
    bool fCheckBalance = fCheckBLSSignature || fCheckRange;

    if (fOnlyRecover)
    {
        fCheckBLSSignature = false;
        fCheckBalance = false;
    }
    else if (!view.HaveInputs(tx)) {
        return state.DoS(100, false, REJECT_INVALID, strprintf("inputs-not-available"));
    }

    if (!(fCheckRange || fCheckBalance || fCheckBLSSignature))
        return true;

    std::vector<bls::G1Element> txSigningKeys;
    std::vector<std::vector<uint8_t>> vMessages;

    Generators gens = BulletproofsRangeproof::GetGenerators();

    CAmount valIn = 0;
    CAmount valOut = 0;

    if (nMixFee != 0)
    {
        Scalar sMixFee = Scalar(nMixFee);

        if (nMixFee < 0)
            sMixFee = sMixFee.Negate();

        Scalar s = Scalar(sMixFee).bn;
        bls::G1Element t = gens.H*s.bn;
        balKey = fElementZero ? t : balKey + t;
        valIn += nMixFee;
        fElementZero = false;
    }

    for (size_t j = 0; j < tx.vin.size(); j++)
    {
        if (fCheckBalance || fCheckBLSSignature)
        {
            const CTxOut &prevOut = view.GetOutputFor(tx.vin[j]);

            gens = BulletproofsRangeproof::GetGenerators(prevOut.tokenId);

            if (fCheckBalance)
            {
                if (prevOut.HasRangeProof())
                {
                    balKey = fElementZero ? prevOut.GetBulletproof().GetValueCommitments()[0] : balKey + prevOut.GetBulletproof().GetValueCommitments()[0];
                    fElementZero = false;
                }
                else
                {
                    Scalar s = Scalar(prevOut.nValue).bn;
                    bls::G1Element t = gens.H*s.bn;
                    balKey = fElementZero ? t : balKey + t;
                    valIn += prevOut.nValue;
                    fElementZero = false;
                }
            }

            if (fCheckBLSSignature)
            {
                try
                {
                    txSigningKeys.push_back(bls::G1Element::FromBytes(prevOut.spendingKey.data()));
                }
                catch(std::exception& e)
                {
                    return state.DoS(100, false, REJECT_INVALID, strprintf("caught-spendingkey-exception %s", e.what()));
                }

                CHashWriter hasher(0,0);
                hasher << tx.vin[j];
                uint256 hash = hasher.GetHash();
                vMessages.push_back(std::vector<unsigned char>(hash.begin(), hash.end()));
            }
        }
    }

    for (size_t j = 0; j < tx.vout.size(); j++)
    {
        gens = BulletproofsRangeproof::GetGenerators(tx.vout[j].tokenId);
        uint256 hash;

        if (tx.vout[j].vData.size() > 0)
        {
            try {
                Predicate program(tx.vout[j].vData);

                if (program.action == ERR) {
                    return state.DoS(100, false, REJECT_INVALID, "error-program-vdata");
                } else if (program.action == CREATE_TOKEN) {
                    txSigningKeys.push_back(program.kParameters[0]);
                    hash = tx.vout[j].GetHash();
                    vMessages.push_back(std::vector<unsigned char>(hash.begin(), hash.end()));
                } else if (program.action == MINT) {
                    txSigningKeys.push_back(program.kParameters[0]);
                    hash = tx.vout[j].GetHash();
                    vMessages.push_back(std::vector<unsigned char>(hash.begin(), hash.end()));

                    auto gensToken = BulletproofsRangeproof::GetGenerators(tx.vout[j].tokenId);

                    Scalar s = Scalar(program.nParameters[0]);

                    if (fElementZero)
                    {
                        balKey = gensToken.H*s.bn;
                    }
                    else
                    {
                        bls::G1Element t = gensToken.H*s.bn;
                        balKey = balKey + t;
                    }
                    fElementZero = false;
                } else if (program.action == STOP_MINT) {
                    txSigningKeys.push_back(program.kParameters[0]);
                    hash = tx.vout[j].GetHash();
                    vMessages.push_back(std::vector<unsigned char>(hash.begin(), hash.end()));
                }
            } catch(...) {
                return state.DoS(100, false, REJECT_INVALID, "error-program-vdata");
            }
        }

        if (tx.vout[j].HasRangeProof())
        {
            if (tx.vout[j].ephemeralKey.size() == 0)
            {
                return state.DoS(100, false, REJECT_INVALID, "empty-ephemeral-key");
            }

            if (fCheckRange)
            {
                proofs[tx.vout[j].tokenId].push_back(std::make_pair(j, tx.vout[j].GetBulletproof()));
                // Shared key v*R - Used as nonce for bulletproof
                try
                {
                    bls::G1Element t = bls::G1Element::FromBytes(tx.vout[j].outputKey.data());
                    t = t * viewKey;
                    nonces[tx.vout[j].tokenId].push_back(t);
                }
                catch(std::exception& e)
                {
                    return state.DoS(100, false, REJECT_INVALID, strprintf("caught-ephemeralkey-exception"));
                }
            }

            if (fCheckBalance)
            {
                if (fElementZeroOut)
                {
                    balKeyOut = tx.vout[j].GetBulletproof().GetValueCommitments()[0];
                }
                else
                {
                    bls::G1Element t = tx.vout[j].GetBulletproof().GetValueCommitments()[0];
                    balKeyOut = balKeyOut + t;
                }
                fElementZeroOut = false;
            }
        }
        else if (fCheckBalance)
        {
            gens = BulletproofsRangeproof::GetGenerators(tx.vout[j].tokenId);

            if (fElementZeroOut)
            {
                Scalar s = Scalar(tx.vout[j].nValue);
                balKeyOut = gens.H*s.bn;
            }
            else
            {
                Scalar s = Scalar(tx.vout[j].nValue);
                bls::G1Element t = gens.H*s.bn;
                balKeyOut = balKeyOut + t;
            }
            valOut += tx.vout[j].nValue;
            fElementZeroOut = false;
        }

        if (fCheckBLSSignature && !tx.vout[j].scriptPubKey.IsFee())
        {
            if (tx.vout[j].ephemeralKey.size() == 0)
            {
                return state.DoS(100, false, REJECT_INVALID, "empty-ephemeral-key");
            }
            try
            {
                txSigningKeys.push_back(bls::G1Element::FromBytes(tx.vout[j].ephemeralKey.data()));
            }
            catch(std::exception& e)
            {
                return state.DoS(100, false, REJECT_INVALID, strprintf("caught-ephemeralkey-exception"));
            }

            if (hash == uint256())
                hash = tx.vout[j].GetHash();

            vMessages.push_back(std::vector<unsigned char>(hash.begin(), hash.end()));
        }
    }

    if (fCheckRange)
    {
        for (auto& it: proofs){
            if (it.second.size() > 0)
            {
                if (!VerifyBulletproof(it.second, vData, nonces[it.first], fOnlyRecover, it.first))
                {
                    return state.DoS(100, false, REJECT_INVALID, "invalid-rangeproof");
                }
            }
        }
    }

    if (fCheckBalance)
    {
        if (tx.vchBalanceSig.size() == 0)
            return state.DoS(100, false, REJECT_INVALID, strprintf("could-not-read-balanceproof"));

        try
        {
            bls::G2Element sig = bls::G2Element::FromBytes(tx.vchBalanceSig.data());

            if (!bls::BasicSchemeMPL::Verify(balKey + balKeyOut.Inverse(), balanceMsg, sig))
                return state.DoS(100, false, REJECT_INVALID, strprintf("invalid-balanceproof"));
        }
        catch(std::exception& e)
        {
            return state.DoS(100, false, REJECT_INVALID, strprintf("caught-balanceproof-exception"));
        }
    }

    if (fCheckBLSSignature)
    {
        if (tx.vchTxSig.size() == 0)
            return state.DoS(100, false, REJECT_INVALID, strprintf("could-not-read-blstxsig"));

        try
        {
            bls::G2Element txsig = bls::G2Element::FromBytes(tx.vchTxSig.data());

            if (!bls::AugSchemeMPL::AggregateVerify(txSigningKeys, vMessages, txsig))
                return state.DoS(100, false, REJECT_INVALID, "invalid-bls-signature");
        }
        catch(std::exception& e)
        {
            return state.DoS(100, false, REJECT_INVALID, strprintf("caught-blstxsig-exception"));
        }
    }

    return true;
}


bool VerifyBLSCTBalanceOutputs(const CTransaction &tx, bls::PrivateKey viewKey, std::vector<RangeproofEncodedData> &vData, const CStateViewCache& view, CValidationState& state, bool fOnlyRecover, CAmount nMixFee)
{
    std::vector<std::pair<int, BulletproofsRangeproof>> proofs;
    std::vector<bls::G1Element> nonces;

    Generators gens = BulletproofsRangeproof::GetGenerators();

    bls::G1Element balKey;
    bool fElementZero = true;

    bool fCheckRange = true;
    bool fCheckBalance = true;

    CAmount valIn = 0;
    CAmount valOut = 0;

    if (nMixFee != 0)
    {
        Scalar sMixFee = Scalar(nMixFee);

        if (nMixFee < 0)
            sMixFee = sMixFee.Negate();

        Scalar s = Scalar(sMixFee).bn;
        bls::G1Element t = gens.H*s.bn;
        balKey = fElementZero ? t : balKey + t;
        valIn += nMixFee;
        fElementZero = false;
    }


    for (size_t j = 0; j < tx.vout.size(); j++)
    {
        if (tx.vout[j].HasRangeProof())
        {
            if (tx.vout[j].ephemeralKey.size() == 0)
            {
                return state.DoS(100, false, REJECT_INVALID, "empty-ephemeral-key");
            }
            if (fCheckRange)
            {
                proofs.push_back(std::make_pair(j, tx.vout[j].GetBulletproof()));
                // Shared key v*R - Used as nonce for bulletproof
                try
                {
                    bls::G1Element t = bls::G1Element::FromBytes(tx.vout[j].outputKey.data());
                    t = t * viewKey;
                    nonces.push_back(t);
                }
                catch(std::exception& e)
                {
                    return state.DoS(100, false, REJECT_INVALID, strprintf("caught-ephemeralkey-exception"));
                }
            }
            if (fCheckBalance)
            {
                if (fElementZero)
                {
                    balKey = tx.vout[j].GetBulletproof().GetValueCommitments()[0];
                }
                else
                {
                    bls::G1Element t = tx.vout[j].GetBulletproof().GetValueCommitments()[0].Inverse();
                    balKey = balKey + t;
                }
                fElementZero = false;
            }
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
        if (tx.vchBalanceSig.size() == 0)
            return state.DoS(100, false, REJECT_INVALID, strprintf("could-not-read-balanceproof"));

        try
        {
            bls::G2Element sig = bls::G2Element::FromBytes(tx.vchBalanceSig.data());

            if (!bls::BasicSchemeMPL::Verify(balKey, balanceMsg, sig))
                return state.DoS(100, false, REJECT_INVALID, strprintf("invalid-balanceproof"));
        }
        catch(std::exception& e)
        {
            return state.DoS(100, false, REJECT_INVALID, strprintf("caught-balanceproof-exception"));
        }
    }

    return true;
}


bool CombineBLSCTTransactions(std::set<CTransaction> &vTx, CTransaction& outTx, const CStateViewCache& inputs, CValidationState& state, CAmount nMixFee)
{
    std::set<CTxIn> setInputs;
    std::set<CTxOut> setOutputs;

    std::vector<bls::G2Element> balanceSigs;
    std::vector<bls::G2Element> txSigs;

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

        if (tx.vchBalanceSig.size() > 0)
        {
            try
            {
                bls::G2Element sig = bls::G2Element::FromByteVector(tx.vchBalanceSig);
                balanceSigs.push_back(sig);
            }
            catch(std::exception& e)
            {
                return state.DoS(100, false, REJECT_INVALID, strprintf("caught-balancesig-exception"));
            }
        }

        if (tx.vchTxSig.size() > 0)
        {
            try
            {
                bls::G2Element sig = bls::G2Element::FromByteVector(tx.vchTxSig);
                txSigs.push_back(sig);
            }
            catch(std::exception& e)
            {
                return state.DoS(100, false, REJECT_INVALID, strprintf("caught-txsig-exception"));
            }
        }
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

    std::random_shuffle(mutOutTx.vin.begin(), mutOutTx.vin.end(), GetRandInt);
    std::random_shuffle(mutOutTx.vout.begin(), mutOutTx.vout.end(), GetRandInt);

    mutOutTx.vout.push_back(CTxOut(nFee, CScript(OP_RETURN)));
    mutOutTx.SetBalanceSignature(bls::AugSchemeMPL::Aggregate(balanceSigs));
    mutOutTx.SetTxSignature(bls::AugSchemeMPL::Aggregate(txSigs));

    outTx = mutOutTx;

    std::vector<RangeproofEncodedData> blsctData;

    if (!MoneyRange(nMixFee))
        return state.DoS(100, false, REJECT_INVALID, strprintf("%s: Can't combine transactions with incorrect fee %d\n", __func__, nMixFee));

    try
    {
        return VerifyBLSCT(outTx, bls::PrivateKey::FromBN(Scalar::Rand().bn), blsctData, inputs, state, false, nMixFee);
    }
    catch(...)
    {
        return state.DoS(100, false, REJECT_INVALID, strprintf("%s: catched exception", __func__));
    }
}
