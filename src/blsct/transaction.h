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

bool CreateBLSCTOutput(const bls::PrivateKey& blindingKey, CTxOut& newTxOut, const blsctDoublePublicKey& destKey, const CAmount& nAmount, std::string sMemo,
                  Scalar& gammaAcc, std::string &strFailReason, const bool& fBLSSign, std::vector<bls::PrependSignature>& vBLSSignatures);
bool SignBLSOutput(const bls::PrivateKey& blindingKey, CTxOut& newTxOut, std::vector<bls::PrependSignature>& vBLSSignatures);

#endif // BLSCT_TRANSACTION_H
