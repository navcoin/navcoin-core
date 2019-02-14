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
// Copyright (c) 2018 The NavCoin Core developers

#include <stdexcept>
#include <iostream>
#include "Coin.h"
#include "Commitment.h"
#include "pubkey.h"
#include "utilstrencodings.h"

namespace libzerocoin {

PublicCoin::PublicCoin(const ZerocoinParams* p):
    params(p), value(0), denomination(ZQ_ERROR) {
    if (this->params->initialized == false) {
        throw std::runtime_error("Params are not initialized");
    }
};

PublicCoin::PublicCoin(const ZerocoinParams* p, const CoinDenomination d, const CBigNum value, const CPubKey pubKey, const CBigNum pid, bool fCheck) : params(p), value(value), denomination(d), pubKey(pubKey), paymentId(pid) {
    if (this->params->initialized == false) {
        throw std::runtime_error("Params are not initialized");
    }

    if (fCheck) {
        std::vector<CoinDenomination>::const_iterator it;

        it = find (zerocoinDenomList.begin(), zerocoinDenomList.end(), denomination);
        if(it == zerocoinDenomList.end()){
            throw std::runtime_error("Denomination does not exist");
        }

        if(!isValid())
            throw std::runtime_error("Commitment Value of Public Coin is invalid");
    }
}

PublicCoin::PublicCoin(const ZerocoinParams* p, const CoinDenomination d, const CPubKey destPubKey, const BlindingCommitment blindingCommitment, const std::string pid): params(p), denomination(d) {
    // Verify that the parameters are valid
    if(this->params->initialized == false)
        throw std::runtime_error("Params are not initialized");

    std::vector<CoinDenomination>::const_iterator it;

    it = find (zerocoinDenomList.begin(), zerocoinDenomList.end(), denomination);
    if(it == zerocoinDenomList.end()){
        throw std::runtime_error("Denomination does not exist");
    }

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

        // C = bc2 * (bc1 ^ z) mod p
        CBigNum commitmentValue = blindingCommitment.first.pow_mod(chi, this->params->coinCommitmentGroup.modulus, false).mul_mod(
                                  blindingCommitment.second, this->params->coinCommitmentGroup.modulus);

        // First verify that the commitment is a prime number
        // in the appropriate range. If not, we repeat the process.
        if (commitmentValue.isPrime(ZEROCOIN_MINT_PRIME_PARAM) &&
                commitmentValue >= params->accumulatorParams.minCoinValue &&
                commitmentValue <= params->accumulatorParams.maxCoinValue) {
            // Found a valid coin. Store it.
            uint256 varrho(Hash(pre_sigma.begin(), pre_sigma.end()));
            this->denomination = denomination;
            this->pubKey = key.GetPubKey();
            this->value = commitmentValue;
            this->version = CURRENT_VERSION;

            std::string truncatedPid = pid.substr(0,32);
            std::vector<unsigned char> vPid;

            vPid.push_back(truncatedPid.length());
            vPid.insert(vPid.end(), truncatedPid.begin(), truncatedPid.end());

            CBigNum bnRand = CBigNum::RandKBitBigum((32-vPid.size())*8);
            std::vector<unsigned char> vRand = bnRand.getvch();

            vRand.resize(32-vPid.size());
            vPid.insert(vPid.end(), vRand.begin(), vRand.end());

            CBigNum bnPid;

            bnPid.setvch(vPid);

            this->paymentId = bnPid.Xor(CBigNum(varrho));

            pre_chi = uint256();
            pre_sigma = uint256();
            chi.Nullify();
            sigma.Nullify();
            varrho = uint256();

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


bool PublicCoin::isValid(bool fFast) const
{
    if (this->params->accumulatorParams.minCoinValue >= value) {
        throw std::runtime_error("PublicCoin::isValid(): value is too low");
    }

    if (value > this->params->accumulatorParams.maxCoinValue) {
        throw std::runtime_error("PublicCoin::isValid(): value is too high");
    }

    if (!value.isPrime(params->zkp_iterations/(fFast?2:1))) {
        throw std::runtime_error("PublicCoin::isValid(): value is not prime");
    }

    std::vector<CoinDenomination>::const_iterator it;

    it = find (zerocoinDenomList.begin(), zerocoinDenomList.end(), denomination);
    if(it == zerocoinDenomList.end()){
        throw std::runtime_error("Denomination does not exist");
    }

    return true;
}

PrivateCoin::PrivateCoin(const ZerocoinParams* p, const CoinDenomination denomination, const CKey privKey, const CPubKey mintPubKey,
                         const BlindingCommitment blindingCommitment, const CBigNum commitment_value, const CBigNum obfuscatedPid,
                         bool fCheck) : params(p), publicCoin(p), randomness(0), serialNumber(0), fValid(false)
{
    // Verify that the parameters are valid
    if(!this->params->initialized)
        throw std::runtime_error("PrivateCoin::PrivateCoin(): Params are not initialized");

    std::vector<CoinDenomination>::const_iterator it;

    it = find (zerocoinDenomList.begin(), zerocoinDenomList.end(), denomination);
    if(it == zerocoinDenomList.end())
    {
        throw std::runtime_error("Denomination does not exist");
    }

    CPrivKey shared_secret;
    if(!privKey.ECDHSecret(mintPubKey, shared_secret))
        throw std::runtime_error("PrivateCoin::PrivateCoin(): Could not calculate ECDH Secret");

    uint256 pre_chi(std::vector<unsigned char>(shared_secret.begin(), shared_secret.end()));
    uint256 pre_sigma(Hash(pre_chi.begin(), pre_chi.end()));

    CBigNum chi = CBigNum(pre_chi) % (this->params->coinCommitmentGroup.groupOrder);
    CBigNum sigma = CBigNum(pre_sigma) % (this->params->coinCommitmentGroup.groupOrder);

    // C = bc2 * (bc1 ^ z) mod p
    CBigNum commitmentValue = blindingCommitment.first.pow_mod(chi, this->params->coinCommitmentGroup.modulus, false).mul_mod(
                              blindingCommitment.second, this->params->coinCommitmentGroup.modulus);

    if (commitmentValue.isPrime(ZEROCOIN_MINT_PRIME_PARAM) &&
            commitmentValue >= params->accumulatorParams.minCoinValue &&
            commitmentValue <= params->accumulatorParams.maxCoinValue)
    {
        uint256 varrho(Hash(pre_sigma.begin(), pre_sigma.end()));
        this->serialNumber = chi;
        this->randomness = sigma;
        this->obfuscationPid = CBigNum(varrho);

        if(commitmentValue == commitment_value)
        {
            this->publicCoin = PublicCoin(p, denomination, commitmentValue, mintPubKey, obfuscatedPid, fCheck);
            fValid = true;
        }

        pre_chi = uint256();
        pre_sigma = uint256();
        chi.Nullify();
        sigma.Nullify();
        varrho = uint256();
    }

    this->version = CURRENT_VERSION;
}

bool PrivateCoin::QuickCheckIsMine(const ZerocoinParams* p, const CKey privKey, const CPubKey mintPubKey,
                         const BlindingCommitment blindingCommitment, const CBigNum commitment_value)
{
    // Verify that the parameters are valid
    if(!p->initialized)
        throw std::runtime_error("PrivateCoin::PrivateCoin(): Params are not initialized");

    CPrivKey shared_secret;
    if(!privKey.ECDHSecret(mintPubKey, shared_secret))
        throw std::runtime_error("PrivateCoin::PrivateCoin(): Could not calculate ECDH Secret");

    uint256 pre_chi(std::vector<unsigned char>(shared_secret.begin(), shared_secret.end()));

    CBigNum chi = CBigNum(pre_chi) % (p->coinCommitmentGroup.groupOrder);

    // C = bc2 * (bc1 ^ z) mod p
    CBigNum commitmentValue = blindingCommitment.first.pow_mod(chi, p->coinCommitmentGroup.modulus, false).mul_mod(
                              blindingCommitment.second, p->coinCommitmentGroup.modulus);

    return (commitmentValue == commitment_value);
}

const std::string PrivateCoin::getPaymentId() const {
    CBigNum bnPid = obfuscationPid.Xor(getPublicCoin().getPaymentId());
    std::vector<unsigned char> vPid = bnPid.getvch();
    std::string paymentId(vPid.begin()+1,vPid.end());
    paymentId = paymentId.substr(0, (unsigned int)vPid[0]);

    return paymentId;
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

bool IsValidPublicSerial(const ZerocoinParams* params, const CBigNum& bnSerial)
{
    return bnSerial > 0 && bnSerial < params->serialNumberSoKCommitmentGroup.groupOrder;
}

bool IsValidPrivateSerial(const ZerocoinParams* params, const CBigNum& bnSerial)
{
    return bnSerial > 0 && bnSerial < params->coinCommitmentGroup.groupOrder;
}

void GetRandomnessBits(CBigNum randomness, std::vector<int> &randomness_bits)
{
    randomness_bits.resize(ZKP_SERIALSIZE);
    std::string bin_string = randomness.ToString(2);
    unsigned int len = bin_string.length();
    for(unsigned int i=0; i<len; i++)
        randomness_bits[len-1-i] = (int)bin_string[i]-'0';
    for(unsigned int i=len; i<ZKP_SERIALSIZE; i++)
        randomness_bits[i] = 0;
}

} /* namespace libzerocoin */
