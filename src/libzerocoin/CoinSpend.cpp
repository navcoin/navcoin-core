/**
 * @file       CoinSpend.cpp
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

#include "Keys.h"
#include "CoinSpend.h"
#include <iostream>
#include <sstream>

namespace libzerocoin
{
CoinSpend::CoinSpend(const ZerocoinParams* params, const PrivateCoin& coin, const Accumulator& a, const uint256& checksum,
                     const AccumulatorWitness& witness, const uint256& ptxHash, const SpendType& spendType,
                     const libzerocoin::ObfuscationValue obfuscationJ, const libzerocoin::ObfuscationValue obfuscationK,
                     bool fUseBulletproofs) : accChecksum(checksum),
    ptxHash(ptxHash),
    accumulatorPoK(&params->accumulatorParams),
    serialNumberSoK(params),
    serialNumberPoK(&params->coinCommitmentGroup),
    commitmentPoK(&params->serialNumberSoKCommitmentGroup,
                  &params->accumulatorParams.accumulatorPoKCommitmentGroup),
    spendType(spendType)
{
    denomination = coin.getPublicCoin().getDenomination();
    version = coin.getVersion();

    if (!static_cast<int>(version)) //todo: figure out why version does not make it here
        version = 1;

    // Sanity check: let's verify that the Witness is valid with respect to
    // the coin and Accumulator provided.
    if (!(witness.VerifyWitness(a, coin.getPublicCoin()))) {
        throw std::runtime_error("Accumulator witness does not verify");
    }

    // 1: Generate two separate commitments to the public coin (C), each under
    // a different set of public parameters. We do this because the RSA accumulator
    // has specific requirements for the commitment parameters that are not
    // compatible with the group we use for the serial number proof.
    // Specifically, our serial number proof requires the order of the commitment group
    // to be the same as the modulus of the upper group. The Accumulator proof requires a
    // group with a significantly larger order.
    const Commitment fullCommitmentToCoinUnderSerialParams(&params->serialNumberSoKCommitmentGroup, coin.getPublicCoin().getValue());
    this->serialCommitmentToCoinValue = fullCommitmentToCoinUnderSerialParams.getCommitmentValue();
    const Commitment fullCommitmentToCoinUnderAccParams(&params->accumulatorParams.accumulatorPoKCommitmentGroup, coin.getPublicCoin().getValue());
    this->accCommitmentToCoinValue = fullCommitmentToCoinUnderAccParams.getCommitmentValue();

    // 2. Generate a ZK proof that the two commitments contain the same public coin.
    this->commitmentPoK = CommitmentProofOfKnowledge(&params->serialNumberSoKCommitmentGroup, &params->accumulatorParams.accumulatorPoKCommitmentGroup, fullCommitmentToCoinUnderSerialParams, fullCommitmentToCoinUnderAccParams);

    // Now generate the two core ZK proofs:
    // 3. Proves that the committed public coin is in the Accumulator (PoK of "witness")
    this->accumulatorPoK = AccumulatorProofOfKnowledge(&params->accumulatorParams, fullCommitmentToCoinUnderAccParams, witness, a);

    // We generate the Coin Value Image and its generator
    CBigNum obfuscatedSerial = (obfuscationJ.first*coin.getObfuscationValue())+obfuscationJ.second;
    CBigNum obfuscatedRandomness = ((obfuscationK.first*coin.getObfuscationValue())+obfuscationK.second)%params->coinCommitmentGroup.groupOrder;
    Commitment publicSerialNumber(&params->coinCommitmentGroup, obfuscatedSerial, 0);
    this->coinValuePublic = publicSerialNumber.getCommitmentValue();

    // 4. Proves that the coin is correct w.r.t. serial number and hidden coin secret and the serial number image
    // (This proof is bound to the coin 'metadata', i.e., transaction hash)
    uint256 hashSig = signatureHash();
    if (!fUseBulletproofs) {
        this->serialNumberSoK = SerialNumberSignatureOfKnowledge(params, coin, fullCommitmentToCoinUnderSerialParams, publicSerialNumber, hashSig, obfuscatedRandomness);
    } else {
        this->serialNumberSoK_small = SerialNumberSoK_small(params, obfuscatedSerial, obfuscatedRandomness, fullCommitmentToCoinUnderSerialParams, hashSig);
    }

    //5. Zero knowledge proof of the serial number
    this->serialNumberPoK = SerialNumberProofOfKnowledge(&params->coinCommitmentGroup, obfuscatedSerial, hashSig);
}

bool CoinSpend::Verify(const Accumulator& a, bool fUseBulletproofs) const
{
    if (a.getDenomination() != this->denomination) {
        return error("CoinsSpend::Verify: failed, denominations do not match");
    }

    // Verify both of the sub-proofs using the given meta-data
    if (!commitmentPoK.Verify(serialCommitmentToCoinValue, accCommitmentToCoinValue)) {
        return error("CoinsSpend::Verify: commitmentPoK failed");
    }

    if (!accumulatorPoK.Verify(a, accCommitmentToCoinValue)) {
        return error("CoinsSpend::Verify: accumulatorPoK failed");
    }

    if (!fUseBulletproofs) {
        if (!serialNumberSoK.Verify(serialCommitmentToCoinValue, coinValuePublic, signatureHash())) {
            return error("CoinsSpend::Verify: serialNumberSoK failed.");
        }
    } else {
        if (!serialNumberSoK_small.Verify(coinValuePublic, serialCommitmentToCoinValue, signatureHash())) {
            return error("CoinsSpend::Verify: serialNumberSoK failed.");
        }
    }

    if (!serialNumberPoK.Verify(coinValuePublic, signatureHash())) {
        return error("CoinsSpend::Verify: serialNumberPoK failed.");
    }

    return true;
}

const uint256 CoinSpend::signatureHash() const
{
    CHashWriter h(0, 0);
    h << serialCommitmentToCoinValue << accCommitmentToCoinValue << commitmentPoK << accumulatorPoK << ptxHash
      << coinValuePublic << accChecksum << denomination << spendType;

    return h.GetHash();
}

std::string CoinSpend::ToString() const
{
    std::stringstream ss;
    ss << "CoinSpend(" << coinValuePublic.ToString(16) << ", " << getDenomination() << ")";
    return ss.str();
}

uint256 CoinSpend::GetHash() const {
    CHashWriter h(0, 0);
    h << *this;
    return h.GetHash();
}

bool CoinSpend::HasValidPublicSerial(ZerocoinParams* params) const
{
    return IsValidPublicSerial(params, coinValuePublic);
}

CBigNum CoinSpend::CalculateValidPublicSerial(ZerocoinParams* params)
{
    CBigNum bnSerial = coinValuePublic;
    bnSerial = bnSerial.mul_mod(CBigNum(1),params->coinCommitmentGroup.modulus);
    return bnSerial;
}

} /* namespace libzerocoin */
