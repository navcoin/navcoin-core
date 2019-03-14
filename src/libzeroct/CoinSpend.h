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
// Copyright (c) 2018-2019 The NavCoin Core developers

#ifndef COINSPEND_H_
#define COINSPEND_H_

#include "Accumulator.h"
#include "AccumulatorProofOfKnowledge.h"
#include "Coin.h"
#include "Commitment.h"
#include "Params.h"
#include "SerialNumberSignatureOfKnowledge.h"
#include "SerialNumberProofOfKnowledge.h"
#include "SpendType.h"

#include "bignum.h"
#include "pubkey.h"
#include "serialize.h"
#include "streams.h"

namespace libzeroct
{
/** The complete proof needed to spend a zerocoin.
 * Composes together a proof that a coin is accumulated
 * and that it has a given serial number.
 */
class CoinSpend
{
public:
    CoinSpend(const ZeroCTParams* params) :
        p(params),
        accumulatorPoK(&params->accumulatorParams),
        serialNumberSoK(params),
        serialNumberPoK(&params->coinCommitmentGroup),
        commitmentPoK(&params->serialNumberSoKCommitmentGroup,
                      &params->accumulatorParams.accumulatorPoKCommitmentGroup,
                      params->serialNumberSoKCommitmentGroup.g,
                      params->serialNumberSoKCommitmentGroup.h,
                      params->accumulatorParams.accumulatorPoKCommitmentGroup.g,
                      params->accumulatorParams.accumulatorPoKCommitmentGroup.h) {}

    //! \param param - params
    //! \param strm - a serialized CoinSpend
    template <typename Stream>
    CoinSpend(const ZeroCTParams* params, Stream& strm) :
        p(params),
        accumulatorPoK(&params->accumulatorParams),
        serialNumberSoK(params),
        serialNumberPoK(&params->coinCommitmentGroup),
        commitmentPoK(&params->serialNumberSoKCommitmentGroup,
                      &params->accumulatorParams.accumulatorPoKCommitmentGroup,
                      params->serialNumberSoKCommitmentGroup.g,
                      params->serialNumberSoKCommitmentGroup.h,
                      params->accumulatorParams.accumulatorPoKCommitmentGroup.g,
                      params->accumulatorParams.accumulatorPoKCommitmentGroup.h)
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
    CoinSpend(const ZeroCTParams* params, const PrivateCoin& coin, const Accumulator& a, const uint256& checksum,
              const AccumulatorWitness& witness, const uint256& ptxHash, const SpendType& spendType,
              const libzeroct::ObfuscationValue obfuscationJ, const libzeroct::ObfuscationValue obfuscationK,
              CBigNum& r);

    /** Returns the serial number of the coin spend by this proof.
   *
   * @return the coin's serial number
   */
    const CBigNum& getCoinSerialNumber() const { return this->coinValuePublic; }

    /**Gets the checksum of the accumulator used in this proof.
   *
   * @return the checksum
   */
    uint256 getBlockAccumulatorHash() const { return blockAccumulatorHash; }

    /**Gets the txout hash used in this proof.
   *
   * @return the txout hash
   */
    uint256 getTxOutHash() const { return ptxHash; }
    CBigNum getAccCommitment() const { return accCommitmentToCoinValue; }
    CBigNum getAmountCommitment() const { return amountCommitment; }
    CBigNum getSerialComm() const { return serialCommitmentToCoinValue; }
    uint8_t getVersion() const { return version; }
    SpendType getSpendType() const { return spendType; }

    bool Verify(const Accumulator& a) const;
    bool HasValidPublicSerial(const ZeroCTParams* params) const;
    bool HasValidSignature() const;
    CBigNum CalculateValidPublicSerial(ZeroCTParams* params);
    std::string ToString() const;

    uint256 GetHash() const;

    ADD_SERIALIZE_METHODS;
    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion)
    {
        READWRITE(version);
        READWRITE(ptxHash);
        READWRITE(blockAccumulatorHash);
        READWRITE(accCommitmentToCoinValue);
        READWRITE(serialCommitmentToCoinValue);
        READWRITE(commitmentToCoinValue);
        READWRITE(coinValuePublic);
        READWRITE(amountCommitment);
        READWRITE(accumulatorPoK);
        READWRITE(serialNumberSoK);
        READWRITE(serialNumberPoK);
        READWRITE(commitmentPoK);
        READWRITE(spendType);
    }

private:
    const uint256 signatureHash() const;
    const ZeroCTParams* p;
    uint256 blockAccumulatorHash;
    uint256 ptxHash;
    CBigNum accCommitmentToCoinValue;
    CBigNum serialCommitmentToCoinValue;
    CBigNum commitmentToCoinValue;
    CBigNum coinValuePublic;
    CBigNum amountCommitment;
    AccumulatorProofOfKnowledge accumulatorPoK;
    SerialNumberSignatureOfKnowledge serialNumberSoK;
    SerialNumberProofOfKnowledge serialNumberPoK;
    CommitmentProofOfKnowledge commitmentPoK;
    uint8_t version;
    SpendType spendType;
};

} /* namespace libzeroct */
#endif /* COINSPEND_H_ */
