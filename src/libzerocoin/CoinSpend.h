/**
 * @file       CoinSpend.h
 *
 * @brief      CoinSpend class for the Zerocoin library.
 *
 * @author     Ian Miers, Christina Garman and Matthew Green
 * @date       June 2013
 *
 * @copyright  Copyright 2013 Ian Miers, Christina Garman and Matthew Green
 * @license    This project is released under the MIT license.
 **/
// Copyright (c) 2017-2018 The PIVX developers
// Copyright (c) 2018 The NavCoin Core developers

#ifndef COINSPEND_H_
#define COINSPEND_H_

#include "Accumulator.h"
#include "AccumulatorProofOfKnowledge.h"
#include "Coin.h"
#include "Commitment.h"
#include "Params.h"
#include "SerialNumberSignatureOfKnowledge.h"
#include "SerialNumberSoK_small.h"
#include "SerialNumberProofOfKnowledge.h"
#include "SpendType.h"

#include "bignum.h"
#include "pubkey.h"
#include "serialize.h"
#include "streams.h"

namespace libzerocoin
{
/** The complete proof needed to spend a zerocoin.
 * Composes together a proof that a coin is accumulated
 * and that it has a given serial number.
 */
class CoinSpend
{
public:
    CoinSpend(const ZerocoinParams* params) :
        accumulatorPoK(&params->accumulatorParams),
        serialNumberSoK(params),
        serialNumberSoK_small(params),
        serialNumberPoK(&params->coinCommitmentGroup),
        commitmentPoK(&params->serialNumberSoKCommitmentGroup, &params->accumulatorParams.accumulatorPoKCommitmentGroup) {}

    //! \param param - params
    //! \param strm - a serialized CoinSpend
    template <typename Stream>
    CoinSpend(const ZerocoinParams* params, Stream& strm) :
        accumulatorPoK(&params->accumulatorParams),
        serialNumberSoK(params),
        serialNumberSoK_small(params),
        serialNumberPoK(&params->coinCommitmentGroup),
        commitmentPoK(&params->serialNumberSoKCommitmentGroup, &params->accumulatorParams.accumulatorPoKCommitmentGroup)
    {
        Stream strmCopy = strm;
        strm >> *this;
    }
    /**Generates a proof spending a zerocoin.
   *
   * To use this, provide an unspent PrivateCoin, the latest Accumulator
   * (e.g from the most recent Bitcoin block) containing the public part
   * of the coin, a witness to that, and whatever metadata is needed.
   *
   * Once constructed, this proof can be serialized and sent.
   * It is validated simply be calling validate.
   * @warning Validation only checks that the proof is correct
   * @warning for the specified values in this class. These values must be validated
   *  Clients ought to check that
   * 1) params is the right params
   * 2) the accumulator actually is in some block
   * 3) that the serial number is unspent
   * 4) that the transaction
   *
   * @param p cryptographic parameters
   * @param coin The coin to be spend
   * @param a The current accumulator containing the coin
   * @param witness The witness showing that the accumulator contains the coin
   * @param a hash of the partial transaction that contains this coin spend
   * @throw ZerocoinException if the process fails
   */
    CoinSpend(const ZerocoinParams* params, const PrivateCoin& coin, const Accumulator& a, const uint256& checksum,
              const AccumulatorWitness& witness, const uint256& ptxHash, const SpendType& spendType,
              const libzerocoin::ObfuscationValue obfuscationJ, const libzerocoin::ObfuscationValue obfuscationK,
              bool fUseBulletproofs = false);

    /** Returns the serial number of the coin spend by this proof.
   *
   * @return the coin's serial number
   */
    const CBigNum& getCoinSerialNumber() const { return this->coinValuePublic; }

    /**Gets the denomination of the coin spent in this proof.
   *
   * @return the denomination
   */
    CoinDenomination getDenomination() const { return this->denomination; }

    /**Gets the checksum of the accumulator used in this proof.
   *
   * @return the checksum
   */
    uint256 getAccumulatorChecksum() const { return this->accChecksum; }

    /**Gets the txout hash used in this proof.
   *
   * @return the txout hash
   */
    uint256 getTxOutHash() const { return ptxHash; }
    CBigNum getAccCommitment() const { return accCommitmentToCoinValue; }
    CBigNum getSerialComm() const { return serialCommitmentToCoinValue; }
    uint8_t getVersion() const { return version; }
    SpendType getSpendType() const { return spendType; }

    bool Verify(const Accumulator& a, bool fUseBulletproofs = false) const;
    bool HasValidPublicSerial(ZerocoinParams* params) const;
    bool HasValidSignature() const;
    CBigNum CalculateValidPublicSerial(ZerocoinParams* params);
    std::string ToString() const;

    uint256 GetHash() const;

    ADD_SERIALIZE_METHODS;
    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion)
    {
        READWRITE(denomination);
        READWRITE(ptxHash);
        READWRITE(accChecksum);
        READWRITE(accCommitmentToCoinValue);
        READWRITE(serialCommitmentToCoinValue);
        READWRITE(coinValuePublic);
        READWRITE(accumulatorPoK);
        READWRITE(serialNumberSoK);
        READWRITE(serialNumberSoK_small);
        READWRITE(serialNumberPoK);
        READWRITE(commitmentPoK);
        READWRITE(version);
        READWRITE(spendType);
    }

private:
    const uint256 signatureHash() const;
    CoinDenomination denomination;
    uint256 accChecksum;
    uint256 ptxHash;
    CBigNum accCommitmentToCoinValue;
    CBigNum serialCommitmentToCoinValue;
    CBigNum coinValuePublic;
    AccumulatorProofOfKnowledge accumulatorPoK;
    SerialNumberSignatureOfKnowledge serialNumberSoK;
    SerialNumberSoK_small serialNumberSoK_small;
    SerialNumberProofOfKnowledge serialNumberPoK;
    CommitmentProofOfKnowledge commitmentPoK;
    uint8_t version;
    SpendType spendType;
};

} /* namespace libzerocoin */
#endif /* COINSPEND_H_ */
