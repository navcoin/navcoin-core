/**
 * @file       Coin.cpp
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

#include <stdexcept>
#include <iostream>
#include "Coin.h"
#include "Commitment.h"
#include "pubkey.h"
#include "utilstrencodings.h"

namespace libzeroct {

inline CBigNum XorObfuscate(std::string pid, CBigNum key, unsigned int len) {
    std::string truncatedPid = pid.substr(0,len);
    std::vector<unsigned char> vPid;

    vPid.push_back(truncatedPid.length());
    vPid.insert(vPid.end(), truncatedPid.begin(), truncatedPid.end());

    CBigNum bnRand = CBigNum::RandKBitBigum((len-vPid.size())*8);
    std::vector<unsigned char> vRand = bnRand.getvch();

    vRand.resize(32-vPid.size());
    vPid.insert(vPid.end(), vRand.begin(), vRand.end());

    CBigNum bnPid;

    bnPid.setvch(vPid);

    return bnPid.Xor(key);
}

PublicCoin::PublicCoin(const ZeroCTParams* p):
    params(p), value(0) {
    if (this->params->initialized == false) {
        throw std::runtime_error("Params are not initialized");
    }
};

PublicCoin::PublicCoin(const ZeroCTParams* p, const CBigNum valueIn, const CPubKey pubKeyIn, const CBigNum pid,
                       CBigNum obfuscatedAmount, CBigNum amountCommitment, bool fCheck) : params(p), value(valueIn),
                       pubKey(pubKeyIn), paymentId(pid), amount(obfuscatedAmount), amountcommitment(amountCommitment) {
    if (this->params->initialized == false) {
        throw std::runtime_error("Params are not initialized");
    }

    this->coincommitment = amountcommitment.mul_mod(
                this->params->coinCommitmentGroup.h.pow_mod(value, this->params->coinCommitmentGroup.modulus), this->params->coinCommitmentGroup.modulus);

    if (fCheck && !isValid())
        throw std::runtime_error("Commitment Value of Public Coin is invalid");
}

PublicCoin::PublicCoin(const ZeroCTParams* p, const CPubKey destPubKey, const BlindingCommitment blindingCommitment,
                       const std::string pid, CAmount amount, CBigNum* rpval): params(p) {
    // Verify that the parameters are valid
    if(this->params->initialized == false)
        throw std::runtime_error("Params are not initialized");

    CKey key;
    bool fFound = false;

    // Repeat this process up to MAX_COINMINT_ATTEMPTS times until
    // we obtain a valid commitment value.
    for (uint32_t attempt = 0; attempt < MAX_COINMINT_ATTEMPTS; attempt++) {
        // Generate a new Key Pair
        key.MakeNewKey(false);

        CPrivKey shared_secret;
        if(!key.ECDHSecret(destPubKey, shared_secret))
            throw std::runtime_error("PrivateCoin::PrivateCoin(): Could not calculate ECDH Secret");

        uint256 pre_chi(std::vector<unsigned char>(shared_secret.begin(), shared_secret.end()));
        uint256 pre_sigma(Hash(pre_chi.begin(), pre_chi.end()));

        CBigNum chi = CBigNum(pre_chi) % (this->params->coinCommitmentGroup.groupOrder);
        CBigNum sigma = CBigNum(pre_sigma) % (this->params->coinCommitmentGroup.groupOrder);

        // c = bc2 * (bc1 ^ z) mod p
        CBigNum c = blindingCommitment.first.pow_mod(chi, this->params->coinCommitmentGroup.modulus, false).mul_mod(
                                  blindingCommitment.second, this->params->coinCommitmentGroup.modulus);

        // epsilon = C(w,c,sigma)
        CBigNum ac = this->params->coinCommitmentGroup.g2.pow_mod(amount, this->params->coinCommitmentGroup.modulus).mul_mod(
                    this->params->coinCommitmentGroup.g.pow_mod(sigma, this->params->coinCommitmentGroup.modulus), this->params->coinCommitmentGroup.modulus);

        CBigNum epsilon = ac.mul_mod(
                    this->params->coinCommitmentGroup.h.pow_mod(c, this->params->coinCommitmentGroup.modulus), this->params->coinCommitmentGroup.modulus);

        // First verify that the commitment is a prime number
        // in the appropriate range. If not, we repeat the process.
        if (epsilon.isPrime(ZEROCOIN_MINT_PRIME_PARAM) &&
                epsilon >= params->accumulatorParams.minCoinValue &&
                epsilon <= params->accumulatorParams.maxCoinValue) {
            // Found a valid coin. Store it.
            this->pubKey = key.GetPubKey();
            this->value = c;
            this->amountcommitment = ac;

            this->coincommitment = epsilon;
            this->version = CURRENT_VERSION;

            uint256 varrho(Hash(pre_sigma.begin(), pre_sigma.end()));
            uint256 o(Hash(varrho.begin(), varrho.end()));

            this->amount = XorObfuscate(std::to_string(amount), CBigNum(varrho), 20);
            this->paymentId = XorObfuscate(pid, CBigNum(o), 32);

            if (rpval != NULL) {
                *rpval = sigma;
            }

            pre_chi = uint256();
            pre_sigma = uint256();
            chi.Nullify();
            varrho = uint256();
            o = uint256();

            // Success! We're done.
            fFound = true;
            break;
        }
    }

    // We only get here if we did not find a coin within
    // MAX_COINMINT_ATTEMPTS. Throw an exception.
    if(!fFound)
        throw std::runtime_error("Unable to mint a new Zerocoin (too many attempts)");
}

const CBigNum PublicCoin::getValue() const
{
    return this->coincommitment;
}


bool PublicCoin::isValid(bool fFast) const
{
    if (value > this->params->coinCommitmentGroup.modulus || amountcommitment > this->params->coinCommitmentGroup.modulus) {
        throw std::runtime_error("PublicCoin::isValid(): values out of range");
    }

    CBigNum acVal = getValue();

    if (this->params->accumulatorParams.minCoinValue >= acVal) {
        throw std::runtime_error("PublicCoin::isValid(): value is too low");
    }

    if (acVal > this->params->accumulatorParams.maxCoinValue) {
        throw std::runtime_error("PublicCoin::isValid(): value is too high");
    }

    if (!acVal.isPrime(params->zkp_iterations/(fFast?2:1))) {
        throw std::runtime_error("PublicCoin::isValid(): value is not prime");
    }

    return true;
}

PrivateCoin::PrivateCoin(const ZeroCTParams* p, const CKey privKey, const CPubKey mintPubKey,
                         const BlindingCommitment blindingCommitment, const CBigNum commitment_value, const CBigNum obfuscatedPid,
                         const CBigNum obfuscatedAmount, bool fCheck) : params(p), publicCoin(p), randomness(0), serialNumber(0), fValid(false)
{
    // Verify that the parameters are valid
    if(!this->params->initialized)
        throw std::runtime_error("PrivateCoin::PrivateCoin(): Params are not initialized");

    CPrivKey shared_secret;
    if(!privKey.ECDHSecret(mintPubKey, shared_secret))
        throw std::runtime_error("PrivateCoin::PrivateCoin(): Could not calculate ECDH Secret");

    uint256 pre_chi(std::vector<unsigned char>(shared_secret.begin(), shared_secret.end()));
    uint256 pre_sigma(Hash(pre_chi.begin(), pre_chi.end()));

    CBigNum chi = CBigNum(pre_chi) % (this->params->coinCommitmentGroup.groupOrder);
    CBigNum sigma = CBigNum(pre_sigma) % (this->params->coinCommitmentGroup.groupOrder);

    // c = bc2 * (bc1 ^ z) mod p
    CBigNum c = blindingCommitment.first.pow_mod(chi, this->params->coinCommitmentGroup.modulus, false).mul_mod(
                              blindingCommitment.second, this->params->coinCommitmentGroup.modulus);

    uint256 varrho(Hash(pre_sigma.begin(), pre_sigma.end()));

    CBigNum amount = getAmount(CBigNum(varrho), obfuscatedAmount);

    // epsilon = C(w,c,sigma)
    CBigNum ac = this->params->coinCommitmentGroup.g2.pow_mod(amount, this->params->coinCommitmentGroup.modulus).mul_mod(
                this->params->coinCommitmentGroup.g.pow_mod(sigma, this->params->coinCommitmentGroup.modulus), this->params->coinCommitmentGroup.modulus);

    CBigNum epsilon = ac.mul_mod(
                this->params->coinCommitmentGroup.h.pow_mod(c, this->params->coinCommitmentGroup.modulus), this->params->coinCommitmentGroup.modulus);

    if (epsilon.isPrime(ZEROCOIN_MINT_PRIME_PARAM) &&
            epsilon >= params->accumulatorParams.minCoinValue &&
            epsilon <= params->accumulatorParams.maxCoinValue &&
            epsilon == commitment_value)
    {
        uint256 o(Hash(varrho.begin(), varrho.end()));

        this->serialNumber = chi;
        this->randomness = sigma;
        this->obfuscationAmount = CBigNum(varrho);

        this->obfuscationPid = CBigNum(o);

        o = uint256();

        this->publicCoin = PublicCoin(p, c, mintPubKey, obfuscatedPid, obfuscatedAmount, ac, fCheck);
        fValid = true;
    }

    pre_chi = uint256();
    pre_sigma = uint256();
    chi.Nullify();
    sigma.Nullify();
    varrho = uint256();

    this->version = CURRENT_VERSION;
}

bool PrivateCoin::QuickCheckIsMine(const ZeroCTParams* p, const CKey& privKey, const CPubKey& mintPubKey, const BlindingCommitment& blindingCommitment,
                                   const CBigNum& commitment_value, const CBigNum& obfuscationAmount, const CBigNum& amountCommitment)
{
    // Verify that the parameters are valid
    if(!p->initialized)
        throw std::runtime_error("PrivateCoin::PrivateCoin(): Params are not initialized");

    CPrivKey shared_secret;
    if(!privKey.ECDHSecret(mintPubKey, shared_secret))
        throw std::runtime_error("PrivateCoin::PrivateCoin(): Could not calculate ECDH Secret");

    uint256 pre_chi(std::vector<unsigned char>(shared_secret.begin(), shared_secret.end()));
    uint256 pre_sigma(Hash(pre_chi.begin(), pre_chi.end()));

    CBigNum chi = CBigNum(pre_chi) % (p->coinCommitmentGroup.groupOrder);

    // c = bc2 * (bc1 ^ z) mod p
    CBigNum c = blindingCommitment.first.pow_mod(chi, p->coinCommitmentGroup.modulus, false).mul_mod(
                              blindingCommitment.second, p->coinCommitmentGroup.modulus);

    if (c != commitment_value)
        return false;

    CBigNum sigma = CBigNum(pre_sigma) % (p->coinCommitmentGroup.groupOrder);
    CBigNum varrho(CBigNum(Hash(pre_sigma.begin(), pre_sigma.end())));

    CBigNum bnPAm = obfuscationAmount.Xor(varrho);
    std::vector<unsigned char> vPAm = bnPAm.getvch();
    std::string strAmount(vPAm.begin()+1,vPAm.end());
    strAmount = strAmount.substr(0, (unsigned int)vPAm[0]);
    CAmount am = stoll(strAmount);

    CBigNum w = p->coinCommitmentGroup.g2.pow_mod(am, p->coinCommitmentGroup.modulus).mul_mod(
                p->coinCommitmentGroup.g.pow_mod(sigma, p->coinCommitmentGroup.modulus), p->coinCommitmentGroup.modulus);

    return (w == amountCommitment);
}

const std::string PrivateCoin::getPaymentId() const {
    CBigNum bnPid = obfuscationPid.Xor(getPublicCoin().getPaymentId());
    std::vector<unsigned char> vPid = bnPid.getvch();
    std::string paymentId(vPid.begin()+1,vPid.end());
    paymentId = paymentId.substr(0, (unsigned int)vPid[0]);

    return paymentId;
}

CAmount PrivateCoin::getAmount(CBigNum obfValue, CBigNum obfAmount) const {
    CBigNum bnPAm = obfValue.Xor(obfAmount);
    std::vector<unsigned char> vPAm = bnPAm.getvch();
    std::string strAmount(vPAm.begin()+1,vPAm.end());
    strAmount = strAmount.substr(0, (unsigned int)vPAm[0]);

    return stoll(strAmount);
}

CAmount PrivateCoin::getAmount() const {
    CBigNum bnPAm = obfuscationAmount.Xor(getPublicCoin().getAmount());
    std::vector<unsigned char> vPAm = bnPAm.getvch();
    std::string strAmount(vPAm.begin()+1,vPAm.end());
    strAmount = strAmount.substr(0, (unsigned int)vPAm[0]);

    return stoll(strAmount);
}

bool PrivateCoin::isValid()
{
    if (!fValid)
        return false;

    if (!IsValidPrivateSerial(params, serialNumber)) {
        throw std::runtime_error("PrivateCoin::isValid(): Serial not valid");
        return false;
    }

    return getPublicCoin().isValid();
}

bool IsValidPublicSerial(const ZeroCTParams* params, const CBigNum& bnSerial)
{
    return bnSerial > 0 && bnSerial < params->serialNumberSoKCommitmentGroup.groupOrder;
}

bool IsValidPrivateSerial(const ZeroCTParams* params, const CBigNum& bnSerial)
{
    return bnSerial > 0 && bnSerial < params->coinCommitmentGroup.groupOrder;
}

} /* namespace libzeroct */
