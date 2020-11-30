// Copyright (c) 2020 The NavCoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "transaction.h"

bool CreateBLSCTOutput(bls::PrivateKey blindingKey, bls::G1Element& nonce, CTxOut& newTxOut, const blsctDoublePublicKey& destKey, const CAmount& nAmount, std::string sMemo,
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

    nonce = blindingKey*vk;
    nonces.push_back(nonce);

    // Masking key - Used for bulletproof
    Scalar gamma = HashG1Element(nonce, 100);
    std::vector<Scalar> gammas;
    gammaAcc = gammaAcc + gamma;
    gammas.push_back(gamma);

    BulletproofsRangeproof bprp;
    std::vector<unsigned char> vMemo(sMemo.length());
    std::copy(sMemo.begin(), sMemo.end(), vMemo.begin());

    try
    {
        bprp.Prove(value, nonces[0], vMemo);
    }
    catch(...)
    {
        strFailReason = "Range proof failed with exception";
        return false;
    }

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
    bls::G2Element sig = bls::AugSchemeMPL::Sign(blindingKey, std::vector<unsigned char>(txOutHash.begin(), txOutHash.end()));
    vBLSSignatures.push_back(sig);

    return true;
}

bool SignBLSInput(const bls::PrivateKey& signingKey, CTxIn& newTxIn, std::vector<bls::G2Element>& vBLSSignatures)
{
    CHashWriter ss(SER_GETHASH, 0);
    ss << newTxIn;
    uint256 txInHash = ss.GetHash();
    bls::G2Element sig = bls::AugSchemeMPL::Sign(signingKey, std::vector<unsigned char>(txInHash.begin(), txInHash.end()));
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

bool CandidateTransaction::Validate(const CStateViewCache* inputs) {
    if (!(tx.IsBLSInput() && tx.IsCTOutput() && tx.vin.size() == 1 && tx.vout.size() == 1 && minAmountProofs.V.size() == tx.vout.size()))
        return error("CandidateTransaction::%s: Received transaction is not BLSCT mix compliant", __func__);

    std::vector<std::pair<int, BulletproofsRangeproof>> proofs;
    std::vector<bls::G1Element> nonces;
    std::vector<RangeproofEncodedData> blsctData;

    if (!inputs->HaveInputs(tx))
        return error ("CandidateTransaction::%s: Received spent inputs", __func__);

    for (unsigned int i = 0; i < tx.vout.size(); i++)
    {
        Scalar s = minAmount;
        bls::G1Element l = (BulletproofsRangeproof::H*s.bn).Inverse();
        bls::G1Element r = tx.vout[i].bp.V[0];
        l = l + r;
        if (!(l == minAmountProofs.V[i]))
            return error ("CandidateTransaction::%s: Failed verification from output's amount %d", __func__, i);
        proofs.push_back(std::make_pair(i, minAmountProofs));
    }

    if (!VerifyBulletproof(proofs, blsctData, nonces))
        return error("CandidateTransaction::%s: Failed verification of min amount range proofs", __func__);

    if (!MoneyRange(fee))
        return error("CandidateTransaction::%s: Received transaction with incorrect fee %d", __func__, fee);

    if (fee > GetArg("-aggregationmaxfee", DEFAULT_MAX_MIX_FEE))
        return error("CandidateTransaction::%s: Received transaction with too high fee %d", __func__, fee);

    CValidationState state;

    try
    {
        if(!VerifyBLSCT(tx, bls::PrivateKey::FromBN(Scalar::Rand().bn), blsctData, *inputs, state, false, fee))
        {
            return error("CandidateTransaction::%s: Failed validation of transaction candidate %s", __func__, state.GetRejectReason());
        }
    }
    catch(...)
    {
        return error("CandidateTransaction::%s: Failed validation of transaction candidate %s. Catched exception.", __func__, state.GetRejectReason());
    }

    return true;
}

EncryptedCandidateTransaction::EncryptedCandidateTransaction(const bls::G1Element &pubKey, const CandidateTransaction &tx)
{
    CPrivKey rand;
    rand.reserve(32);
    GetRandBytes(rand.data(), 32);
    bls::PrivateKey key = bls::PrivateKey::FromSeed(rand.data(), 32);

    vPublicKey  = key.GetG1Element().Serialize();

    bls::G1Element sharedKey = key * pubKey;

    uint256 hashedSharedKey = SerializeHash(sharedKey.Serialize());

    bls::G2Element signature = bls::AugSchemeMPL::Sign(key, pubKey.Serialize());

    DecryptedCandidateTransaction dct(tx, signature);

    CDataStream ss(SER_NETWORK, PROTOCOL_VERSION);
    ss << dct;

    std::vector<unsigned char> vss(ss.begin(), ss.end());

    CKeyingMaterial vEncryptionKey;
    vEncryptionKey.resize(bls::PrivateKey::PRIVATE_KEY_SIZE);
    memcpy(vEncryptionKey.data(), hashedSharedKey.begin(), vEncryptionKey.size());

    CKeyingMaterial vchSecret;
    vchSecret.resize(vss.size());
    memcpy(vchSecret.data(), &vss[0], vchSecret.size());

    if (!EncryptSecret(vEncryptionKey, vchSecret, SerializeHash(pubKey.Serialize()), vData))
        throw std::runtime_error("EncryptSecret returned false");
}

bool EncryptedCandidateTransaction::Decrypt(const bls::PrivateKey &key, const CStateViewCache* inputs, CandidateTransaction& tx) const
{
    if (vPublicKey.size() == 0)
        return false;

    bls::G1Element publicKey = bls::G1Element::FromByteVector(vPublicKey);

    if (vData.size() == 0)
        return false;

    bls::G1Element sharedKey = key * publicKey;
    uint256 hashedSharedKey = SerializeHash(sharedKey.Serialize());

    CKeyingMaterial vEncryptionKey;
    vEncryptionKey.resize(bls::PrivateKey::PRIVATE_KEY_SIZE);
    memcpy(vEncryptionKey.data(), hashedSharedKey.begin(), vEncryptionKey.size());

    CKeyingMaterial vchSecret;
    if(!DecryptSecret(vEncryptionKey, vData, SerializeHash(key.GetG1Element().Serialize()), vchSecret))
        return false;

    DecryptedCandidateTransaction dct;

    std::vector<unsigned char> vchDecrptedSecret;
    vchDecrptedSecret.resize(vchSecret.size());
    memcpy(vchDecrptedSecret.data(), &vchSecret[0], vchDecrptedSecret.size());

    CDataStream ss(vchDecrptedSecret, SER_NETWORK, PROTOCOL_VERSION);
    ss >> dct;

    bls::G2Element sig = bls::G2Element::FromByteVector(dct.vSig);

    if (!bls::AugSchemeMPL::Verify(publicKey, key.GetG1Element().Serialize(), sig))
        return false;

    if (!dct.tx.Validate(inputs))
        return false;

    tx = dct.tx;

    return true;
}
