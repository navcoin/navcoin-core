// Copyright (c) 2020 The NavCoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "transaction.h"

bool CreateBLSCTOutput(const bls::PrivateKey& blindingKey, CTxOut& newTxOut, const blsctDoublePublicKey& destKey, const CAmount& nAmount, std::string sMemo,
                  Scalar& gammaAcc, std::string &strFailReason, const bool& fBLSSign, std::vector<bls::PrependSignature>& vBLSSignatures, bool fVerify)
{

    newTxOut = CTxOut(0, CScript(OP_TRUE));

    std::vector<Scalar> value;
    value.push_back(nAmount);

    // Shared key H(r*V) - Used as nonce for bulletproof
    std::vector<Point> nonces;
    nonces.push_back(Point(bls::BLS::DHKeyExchange(blindingKey, destKey.GetViewKey())));

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

    std::cout << strprintf("%s: sk: %s\n", __func__, HexStr(newTxOut.spendingKey));
    std::cout << strprintf("%s: ok: %s\n", __func__, HexStr(newTxOut.outputKey));

    if (fBLSSign)
    {
        SignBLSOutput(blindingKey, newTxOut, vBLSSignatures);
    }

    std::cout << strprintf("%s: sk: %s\n", __func__, HexStr(newTxOut.spendingKey));
    std::cout << strprintf("%s: ok: %s\n", __func__, HexStr(newTxOut.outputKey));

    return true;
}

bool SignBLSOutput(const bls::PrivateKey& blindingKey, CTxOut& newTxOut, std::vector<bls::PrependSignature>& vBLSSignatures)
{
    newTxOut.ephemeralKey = blindingKey.GetPublicKey().Serialize();
    uint256 txOutHash = newTxOut.GetHash();
    bls::PrependSignature sig = blindingKey.SignPrependPrehashed((unsigned char*)(&txOutHash));
    vBLSSignatures.push_back(sig);

    return true;
}

bool SignBLSInput(const bls::PrivateKey& signingKey, CTxIn& newTxIn, std::vector<bls::PrependSignature>& vBLSSignatures)
{
    CHashWriter ss(SER_GETHASH, 0);
    ss << newTxIn;
    uint256 txInHash = ss.GetHash();
    bls::PrependSignature sig = signingKey.SignPrependPrehashed((unsigned char*)(&txInHash));
    vBLSSignatures.push_back(sig);

    return true;
}

bool GenTxOutputKeys(const bls::PrivateKey& blindingKey, const blsctDoublePublicKey& destKey, std::vector<unsigned char>& spendingKey, std::vector<unsigned char>& outputKey, std::vector<unsigned char>& ephemeralKey)
{
    try
    {
        // E = r*G
        ephemeralKey = blindingKey.GetPublicKey().Serialize();
        // R = r*S
        outputKey = bls::BLS::DHKeyExchange(blindingKey, destKey.GetSpendKey()).Serialize();
        // P = H(r*V)*G + S
        std::vector<bls::PublicKey> keys;
        keys.push_back(destKey.GetSpendKey());
        keys.push_back(bls::PrivateKey::FromBN(Scalar(Point(bls::BLS::DHKeyExchange(blindingKey, destKey.GetViewKey())).Hash(0)).bn).GetPublicKey());
        spendingKey = bls::PublicKey::AggregateInsecure(keys).Serialize();
        std::cout << strprintf("%s: dh: %s\n", __func__, HexStr(bls::BLS::DHKeyExchange(blindingKey,destKey.GetViewKey()).Serialize()));
        std::cout << strprintf("%s: sk: %s\n", __func__, HexStr(spendingKey));
        std::cout << strprintf("%s: ok: %s\n", __func__, HexStr(outputKey));
    }
    catch(...)
    {
        return false;
    }
    return true;
}

CandidateTransaction::CandidateTransaction() : fee(0) {
}
