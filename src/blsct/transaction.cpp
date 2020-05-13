// Copyright (c) 2020 The NavCoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "transaction.h"

bool CreateBLSCTOutput(const bls::PrivateKey& blindingKey, CTxOut& newTxOut, const blsctDoublePublicKey& destKey, const CAmount& nAmount, std::string sMemo,
                  Scalar& gammaAcc, std::string &strFailReason, const bool& fBLSSign, std::vector<bls::PrependSignature>& vBLSSignatures)
{

    newTxOut = CTxOut(0, CScript(OP_TRUE));

    std::vector<Scalar> value;
    value.push_back(nAmount);

    bls::PublicKey secret1 = bls::BLS::DHKeyExchange(blindingKey, destKey.GetViewKey());
    Point nonce = secret1;
    std::vector<Point> nonces;
    nonces.push_back(nonce);

    Scalar gamma = Scalar::Rand();
    std::vector<Scalar> gammas;
    gammaAcc = gammaAcc + gamma;
    gammas.push_back(gamma);

    Scalar nonceHash = nonce.Hash(0);
    bls::PublicKey spendingKey = bls::BLS::DHKeyExchange(bls::PrivateKey::FromBN(nonceHash.bn), destKey.GetSpendKey());

    BulletproofsRangeproof bprp;
    std::vector<unsigned char> vMemo(sMemo.length());
    std::copy(sMemo.begin(), sMemo.end(), vMemo.begin());
    bprp.Prove(value, gammas, nonce, vMemo);

    std::vector<std::pair<int, BulletproofsRangeproof>> proofs;
    proofs.push_back(std::make_pair(0,bprp));
    std::vector<RangeproofEncodedData> data;
    if (!VerifyBulletproof(proofs, data, nonces))
    {
        strFailReason = "Range proof failed";
        return false;
    }
    newTxOut.blindingKey = blindingKey.GetPublicKey().Serialize();
    newTxOut.spendingKey = spendingKey.Serialize();
    newTxOut.bp = bprp;
    if (fBLSSign)
    {
        SignBLSOutput(blindingKey, newTxOut, vBLSSignatures);
    }

    return true;
}

bool SignBLSOutput(const bls::PrivateKey& blindingKey, CTxOut& newTxOut, std::vector<bls::PrependSignature>& vBLSSignatures)
{
    newTxOut.blindingKey = blindingKey.GetPublicKey().Serialize();
    uint256 txOutHash = newTxOut.GetHash();
    bls::PrependSignature sig = blindingKey.SignPrependPrehashed((unsigned char*)(&txOutHash));
    vBLSSignatures.push_back(sig);

    return true;
}

CandidateTransaction::CandidateTransaction() {

}
