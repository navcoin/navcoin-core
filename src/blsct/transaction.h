// Copyright (c) 2020 The NavCoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BLSCT_TRANSACTION_H
#define BLSCT_TRANSACTION_H

#include <bls.hpp>
#include <blsct/key.h>
#include <blsct/types.h>
#include <blsct/verification.h>
#include <primitives/transaction.h>

#define BLSCT_TX_INPUT_FEE 200000
#define BLSCT_TX_OUTPUT_FEE 200000

bool CreateBLSCTOutput(const bls::PrivateKey& blindingKey, CTxOut& newTxOut, const blsctDoublePublicKey& destKey, const CAmount& nAmount, std::string sMemo,
                  Scalar& gammaAcc, std::string &strFailReason, const bool& fBLSSign, std::vector<bls::PrependSignature>& vBLSSignatures);
bool SignBLSOutput(const bls::PrivateKey& blindingKey, CTxOut& newTxOut, std::vector<bls::PrependSignature>& vBLSSignatures);

class CandidateTransaction
{
public:
    CandidateTransaction();
    CandidateTransaction(CTransaction txIn, CAmount feeIn): tx(txIn), fee(feeIn) {}

    friend inline  bool operator==(const CandidateTransaction& a, const CandidateTransaction& b) { return a.tx == b.tx; }
    friend inline  bool operator<(const CandidateTransaction& a, const CandidateTransaction& b) { return a.fee < b.fee; }

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(tx);
        READWRITE(fee);
    }

    CTransaction tx;
    CAmount fee;
};

#endif // BLSCT_TRANSACTION_H
