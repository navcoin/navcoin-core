// Copyright (c) 2020 The Navcoin developers
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
#include <wallet/crypter.h>
#include <schemes.hpp>
#include <blsct/bulletproofs.h>
#include <blsct/key.h>
#include <blsct/verification.h>
#include <primitives/transaction.h>
#include <wallet/walletdb.h>

#define DEFAULT_MIX_FEE 10000000
#define DEFAULT_MIN_OUTPUT_AMOUNT 10000000000
#define DEFAULT_MAX_MIX_FEE 100000000
#define DEFAULT_TX_MIXCOINS 5
#define DEFAULT_MIX true

#define BLSCT_THREAD_SLEEP_AGG 5000
#define BLSCT_THREAD_SLEEP_VER 16000

#define BLSCT_TX_INPUT_FEE 200000
#define BLSCT_TX_OUTPUT_FEE 200000

class CWalletDB;

bool CreateBLSCTOutput(bls::PrivateKey ephemeralKey, bls::G1Element &nonce, CTxOut& newTxOut, const blsctDoublePublicKey& destKey, const CAmount& nAmount, std::string sMemo,
                  Scalar& gammaAcc, std::string &strFailReason, const bool& fBLSSign, std::vector<bls::G2Element>& vBLSSignatures, bool fVerify = true, const std::vector<unsigned char>& vData = std::vector<unsigned char>(),
                  const std::pair<uint256,uint64_t>& tokenId = std::make_pair(uint256(),-1), const bool& fIsBurn=false, const bool& fConfidentialAmount=true);
bool GenTxOutputKeys(bls::PrivateKey blindingKey, const blsctDoublePublicKey& destKey, std::vector<unsigned char>& spendingKey, std::vector<unsigned char>& outputKey, std::vector<unsigned char>& ephemeralKey);
bool SignBLSOutput(const bls::PrivateKey& ephemeralKey, CTxOut& newTxOut, std::vector<bls::G2Element>& vBLSSignatures);
bool SignBLSInput(const bls::PrivateKey& ephemeralKey, CTxIn& newTxOut, std::vector<bls::G2Element>& vBLSSignatures);

class CandidateTransaction
{
public:
    CandidateTransaction();
    CandidateTransaction(CTransaction txIn, CAmount feeIn, CAmount minAmountIn, BulletproofsRangeproof minAmountProofsIn) :
        tx(txIn), fee(feeIn), minAmount(minAmountIn), minAmountProofs(minAmountProofsIn) {}

    bool Validate(const CStateViewCache* inputs);

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

class EncryptedCandidateTransaction
{
public:
    EncryptedCandidateTransaction() {};
    EncryptedCandidateTransaction(const bls::G1Element &pubKey, const CandidateTransaction &tx, const bool& upgraded);

    bool Decrypt(const bls::PrivateKey &key, const CStateViewCache* inputs, CandidateTransaction& tx) const;

    friend inline  bool operator==(const EncryptedCandidateTransaction& a, const EncryptedCandidateTransaction& b) { return a.vData == b.vData && a.vPublicKey == b.vPublicKey; }
    friend inline  bool operator<(const EncryptedCandidateTransaction& a, const EncryptedCandidateTransaction& b) { return a.vData < b.vData; }

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        if (ser_action.ForRead()) {
            READWRITE(vPublicKey);
            READWRITE(sessionId);
            if (sessionId.size() > 48) {
                vData = sessionId;
                sessionId.clear();
            } else {
                READWRITE(vData);
            }
        } else {
            READWRITE(vPublicKey);
            if (sessionId.size() > 0)
                READWRITE(sessionId);
            READWRITE(vData);
        }
    }

    uint256 GetHash() {
        if (cachedHash == uint256())
            cachedHash = SerializeHash(*this);
        return cachedHash;
    }

    std::vector<unsigned char> vPublicKey;
    std::vector<unsigned char> vData;
    std::vector<unsigned char> sessionId;
    uint256 cachedHash;
    int64_t nTime;
};

class DecryptedCandidateTransaction
{
public:
    DecryptedCandidateTransaction() { }
    DecryptedCandidateTransaction(const CandidateTransaction &tx_, const bls::G2Element &sig_) : tx(tx_), vSig(sig_.Serialize()) { }

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(vSig);
        READWRITE(tx);
    }

    std::vector<unsigned char> vSig;
    CandidateTransaction tx;
};

#endif // BLSCT_TRANSACTION_H
