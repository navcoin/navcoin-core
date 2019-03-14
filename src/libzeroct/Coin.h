/**
 * @file       Coin.h
 *
 * @brief      PublicCoin and PrivateCoin classes for the Zerocoin library.
 *
 * @author     Ian Miers, Christina Garman and Matthew Green
 * @date       June 2013
 *
 * @copyright  Copyright 2013 Ian Miers, Christina Garman and Matthew Green
 * @license    This project is released under the MIT license.
 **/
// Copyright (c) 2017-2018 The PIVX developers
// Copyright (c) 2018-2019 The NavCoin Core developers

#ifndef COIN_H_
#define COIN_H_

#include "amount.h"
#include "bignum.h"
#include "Keys.h"
#include "Params.h"
#include "key.h"
#include "streams.h"
#include "util.h"
#include "version.h"

namespace libzeroct
{
bool IsValidPrivateSerial(const ZeroCTParams* params, const CBigNum& bnSerial);
bool IsValidPublicSerial(const ZeroCTParams* params, const CBigNum& bnSerial);

/** A Public coin is the part of a coin that
 * is published to the network and what is handled
 * by other clients.
 */
class PublicCoin
{
public:
    static int const CURRENT_VERSION = 1;

    template <typename Stream>
    PublicCoin(const ZeroCTParams* p, Stream& strm) : params(p)
    {
        strm >> *this;
    }

    PublicCoin(const ZeroCTParams* p);

    /**Generates a new Zerocoin by (a) selecting a random key pair,
   * (b) deriving a secret using DH with the destination pub key,
   * (c) using the secret as a seed for a PNRG from where obtain
   * s and r, (d) committing to the values and succeeding if
   * the resulting commitment is prime and in range. Repeats the
   * process for a limited number of rounds if necessary. Stores the
   * resulting commitment (coin) and the mint pub key.
   *
   * @brief Mint a new coin.
   * @param p cryptographic parameters
   * @param destPubKey the public key of the destination
   * @param blindingCommitment the bliding commitment from the destination
   * @throws ZerocoinException if the process takes too long
   **/

    PublicCoin(const ZeroCTParams* p, const CPubKey destPubKey, const BlindingCommitment blindingCommitment,
               const std::string pid, const CAmount amount, CBigNum* prpval = NULL);
    PublicCoin(const ZeroCTParams* p, const CBigNum value, const CPubKey pubKey, const CBigNum obfuscatedPid,
               const CBigNum obfuscatedAmount, const CBigNum amountCommitment, bool fCheck = true);

    const CBigNum getValue() const;
    const CBigNum getCoinValue() const { return this->value; }
    const CPubKey& getPubKey() const { return this->pubKey; }
    const uint8_t& getVersion() const { return this->version; }
    const CBigNum& getPaymentId() const { return this->paymentId; }
    const CBigNum& getAmount() const { return this->amount; }
    const CBigNum& getAmountCommitment() const { return this->amountcommitment; }

    bool operator==(const PublicCoin& rhs) const
    {
        return ((this->value == rhs.value) && (this->params == rhs.params) && (this->amountcommitment == rhs.amountcommitment) && (this->amount == rhs.amount) && (this->pubKey == rhs.pubKey) && (this->paymentId == rhs.paymentId));
    }
    bool operator!=(const PublicCoin& rhs) const { return !(*this == rhs); }

    bool isValid(bool fFast=false) const;

    ADD_SERIALIZE_METHODS;
    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion)
    {
        READWRITE(version);
        READWRITE(pubKey);
        READWRITE(value);
        READWRITE(amountcommitment);
        READWRITE(coincommitment);
        READWRITE(paymentId);
        READWRITE(amount);
    }

private:
    const ZeroCTParams* params;
    uint8_t version;
    CBigNum value;
    CPubKey pubKey;
    CBigNum paymentId;
    CBigNum amount;
    CBigNum amountcommitment;
    CBigNum coincommitment;
};

/**
 * A private coin. As the name implies, the content
 * of this should stay private except PublicCoin.
 *
 * Contains a coin's serial number, a commitment to it,
 * and opening randomness for the commitment.
 *
 * @warning Failure to keep this secret(or safe),
 * @warning will result in the theft of your coins
 * @warning and a TOTAL loss of anonymity.
 */
class PrivateCoin
{
public:
    static int const CURRENT_VERSION = 1;
    template <typename Stream>
    PrivateCoin(const ZeroCTParams* p, Stream& strm) : params(p), publicCoin(p), fValid(true)
    {
        strm >> *this;
    }

    /**Tries to obtain the private parameters of a Zerocoin by
   * (a) deriving a secret using DH with the mint pub key,
   * (b) using the secret as a seed for a PNRG from where obtain
   * s and r, (c) obfuscating s and r with j and k (d) commiting
   * to the values and succeeding if the resulting commitment is
   * equal to the provided commitment. Stores the
   * resulting serial number and randomness.
   *
   * @brief Calculate private parameters of a zerocoin.
   * @param p cryptographic parameters
   * @param privKey the private key part of a private address
   * @param mintPubKey the public key of the mint
   * @param obfuscationK the K obfuscation parameter
   * @param obfuscationJ the J obfuscation parameter
   * @param commitment_value the commitment value specified on the mint
   **/

    PrivateCoin(const ZeroCTParams* p, const CKey privKey, const CPubKey mintPubKey,
                const BlindingCommitment blindingCommitment, const CBigNum commitment_value, const CBigNum obfuscatedPid,
                const CBigNum obfuscatedAmount, bool fCheck = true);

    const PublicCoin& getPublicCoin() const { return this->publicCoin; }
    const CBigNum& getObfuscationValue() const { return this->serialNumber; }
    const CBigNum getPublicSerialNumber(const libzeroct::ObfuscationValue& bnObfuscationJ) const {
        return params->coinCommitmentGroup.g.pow_mod(
                    ((bnObfuscationJ.first*this->serialNumber)+bnObfuscationJ.second) % params->coinCommitmentGroup.groupOrder,
                    params->coinCommitmentGroup.modulus);
    }
    const CBigNum getPrivateSerialNumber(const libzeroct::ObfuscationValue& bnObfuscationJ) const {
        return ((bnObfuscationJ.first*this->serialNumber)+bnObfuscationJ.second) % params->coinCommitmentGroup.groupOrder;
    }
    const std::string getPaymentId() const;
    CAmount getAmount() const;
    CAmount getAmount(CBigNum obfValue, CBigNum obfAmount) const;
    const CBigNum& getRandomness() const { return this->randomness; }
    const uint8_t& getVersion() const { return this->version; }

    static bool QuickCheckIsMine(const ZeroCTParams* p, const CKey& privKey, const CPubKey& mintPubKey, const BlindingCommitment& blindingCommitment,
                                 const CBigNum& commitment_value, const CBigNum& obfuscationAmount, const CBigNum& amountCommitment);

    bool isValid();

    ADD_SERIALIZE_METHODS;
    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion)
    {
        READWRITE(version);
        READWRITE(publicCoin);
        READWRITE(randomness);
        READWRITE(serialNumber);
        READWRITE(obfuscationPid);
        READWRITE(obfuscationAmount);
    }

private:
    const ZeroCTParams* params;
    PublicCoin publicCoin;
    CBigNum randomness;
    CBigNum serialNumber;
    CBigNum obfuscationPid;
    CBigNum obfuscationAmount;
    uint8_t version = 1;
    bool fValid = false;

};

} /* namespace libzeroct */
#endif /* COIN_H_ */
