// Copyright (c) 2020 The NavCoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "transaction.h"

bool CreateBLSCTOutput(bls::PrivateKey blindingKey, CTxOut& newTxOut, const blsctDoublePublicKey& destKey, const CAmount& nAmount, std::string sMemo,
                       Scalar& gammaAcc, std::string &strFailReason, const bool& fBLSSign, std::vector<bls::G2Element>& vBLSSignatures, bool fVerify)
{

    newTxOut = CTxOut(0, CScript(OP_TRUE));

    std::vector<Scalar> value;
    value.push_back(nAmount);

    // Shared key H(r*V) - Used as nonce for bulletproof
    std::vector<bls::G1Element> nonces;
    bls::G1Element vk;
    if (!destKey.GetViewKey(vk))
    {
        strFailReason = "Could not read view key from address";
        return false;
    }

    bls::G1Element nonce = blindingKey*vk;
    nonces.push_back(nonce);

    // Masking key - Used for bulletproof
    Scalar gamma = Scalar::Rand();
    std::vector<Scalar> gammas;
    gammaAcc = gammaAcc + gamma;
    gammas.push_back(gamma);

    BulletproofsRangeproof bprp;
    std::vector<unsigned char> vMemo(sMemo.length());
    std::copy(sMemo.begin(), sMemo.end(), vMemo.begin());

    bprp.Prove(value, gammas, nonces[0], vMemo);

    std::vector<std::pair<int, BulletproofsRangeproof>> proofs;
    proofs.push_back(std::make_pair(0,bprp));
    std::vector<RangeproofEncodedData> data;

    if (fVerify && !VerifyBulletproof(proofs, data, nonces))
    {
        strFailReason = "Range proof failed";
        return false;
    }

    newTxOut.bp = bprp;

    if (!GenTxOutputKeys(blindingKey, destKey, newTxOut.spendingKey, newTxOut.outputKey, newTxOut.ephemeralKey))
    {
        strFailReason = "Could not generate tx output keys";
        return false;
    }

    if (fBLSSign)
    {
        SignBLSOutput(blindingKey, newTxOut, vBLSSignatures);
    }

    return true;
}

bool SignBLSOutput(const bls::PrivateKey& blindingKey, CTxOut& newTxOut, std::vector<bls::G2Element>& vBLSSignatures)
{
    newTxOut.ephemeralKey = blindingKey.GetG1Element().Serialize();
    uint256 txOutHash = newTxOut.GetHash();
    bls::G2Element sig = bls::AugSchemeMPL::SignNative(blindingKey, std::vector<unsigned char>(txOutHash.begin(), txOutHash.end()));
    vBLSSignatures.push_back(sig);

    return true;
}

bool SignBLSInput(const bls::PrivateKey& signingKey, CTxIn& newTxIn, std::vector<bls::G2Element>& vBLSSignatures)
{
    CHashWriter ss(SER_GETHASH, 0);
    ss << newTxIn;
    uint256 txInHash = ss.GetHash();
    bls::G2Element sig = bls::AugSchemeMPL::SignNative(signingKey, std::vector<unsigned char>(txInHash.begin(), txInHash.end()));
    vBLSSignatures.push_back(sig);

    return true;
}

bool GenTxOutputKeys(bls::PrivateKey blindingKey, const blsctDoublePublicKey& destKey, std::vector<unsigned char>& spendingKey, std::vector<unsigned char>& outputKey, std::vector<unsigned char>& ephemeralKey)
{
    try
    {
        // E = r*G
        ephemeralKey = blindingKey.GetG1Element().Serialize();

        // R = r*S
        bls::G1Element S, R;

        if(!destKey.GetSpendKey(S))
        {
            return false;
        }
        R = blindingKey*S;

        outputKey = R.Serialize();

        // P = H(r*V)*G + S
        bls::G1Element V;
        if(!destKey.GetViewKey(V))
        {
            return false;
        }
        bls::G1Element rV = blindingKey*V;
        bls::G1Element P = bls::PrivateKey::FromBN(Scalar(HashG1Element(rV,0)).bn).GetG1Element();
        P = S + P;

        spendingKey = P.Serialize();
    }
    catch(...)
    {
        return false;
    }
    return true;
}

CandidateTransaction::CandidateTransaction() : fee(0) {
}
