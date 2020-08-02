// Copyright (c) 2020 The NavCoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BLSCT_TRANSACTION_H
#define BLSCT_TRANSACTION_H

#ifdef _WIN32
/* Avoid redefinition warning. */
#undef ERROR
#undef WSIZE
#undef DOUBLE
#endif

#include <bls.hpp>
#include <blsct/bulletproofs.h>
#include <blsct/key.h>
#include <blsct/types.h>
#include <blsct/verification.h>
#include <primitives/transaction.h>

#define BLSCT_TX_INPUT_FEE 200000
#define BLSCT_TX_OUTPUT_FEE 200000

bool CreateBLSCTOutput(const bls::PrivateKey& ephemeralKey, CTxOut& newTxOut, const blsctDoublePublicKey& destKey, const CAmount& nAmount, std::string sMemo,
                  Scalar& gammaAcc, std::string &strFailReason, const bool& fBLSSign, std::vector<bls::PrependSignature>& vBLSSignatures, bool fVerify = true);
bool GenTxOutputKeys(const bls::PrivateKey& blindingKey, const blsctDoublePublicKey& destKey, std::vector<unsigned char>& spendingKey, std::vector<unsigned char>& outputKey, std::vector<unsigned char>& ephemeralKey);
bool SignBLSOutput(const bls::PrivateKey& ephemeralKey, CTxOut& newTxOut, std::vector<bls::PrependSignature>& vBLSSignatures);
bool SignBLSInput(const bls::PrivateKey& ephemeralKey, CTxIn& newTxOut, std::vector<bls::PrependSignature>& vBLSSignatures);

class CandidateTransaction
{
public:
    CandidateTransaction();
    CandidateTransaction(CTransaction txIn, CAmount feeIn, CAmount minAmountIn, BulletproofsRangeproof minAmountProofsIn) :
        tx(txIn), fee(feeIn), minAmount(minAmountIn), minAmountProofs(minAmountProofsIn) {}

    friend inline  bool operator==(const CandidateTransaction& a, const CandidateTransaction& b) { return a.tx == b.tx; }
    friend inline  bool operator<(const CandidateTransaction& a, const CandidateTransaction& b) { return a.fee < b.fee; }

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(tx);
        READWRITE(fee);
        READWRITE(minAmount);
        READWRITE(minAmountProofs);
    }

    CTransaction tx;
    CAmount fee;
    CAmount minAmount;
    BulletproofsRangeproof minAmountProofs;
};

#endif // BLSCT_TRANSACTION_H
