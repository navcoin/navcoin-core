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
// Copyright (c) 2018-2019 The NavCoin Core developers

#include "Keys.h"
#include "CoinSpend.h"
#include <iostream>
#include <sstream>

namespace libzeroct
{
CoinSpend::CoinSpend(const ZeroCTParams* params, const PrivateCoin& coin, const Accumulator& a, const uint256& blockHash,
                     const AccumulatorWitness& witness, const uint256& ptxHash, const SpendType& spendType,
                     const libzeroct::ObfuscationValue obfuscationJ, const libzeroct::ObfuscationValue obfuscationK,
                     CBigNum& r) :
    p(params),
    blockAccumulatorHash(blockHash),
    ptxHash(ptxHash),
    accumulatorPoK(&params->accumulatorParams),
    serialNumberSoK(params),
    serialNumberPoK(&params->coinCommitmentGroup),
    commitmentPoK(&params->serialNumberSoKCommitmentGroup,
                  &params->accumulatorParams.accumulatorPoKCommitmentGroup,
                  params->serialNumberSoKCommitmentGroup.g,
                  params->serialNumberSoKCommitmentGroup.h,
                  params->accumulatorParams.accumulatorPoKCommitmentGroup.g,
                  params->accumulatorParams.accumulatorPoKCommitmentGroup.h),
    spendType(spendType)
{
    version = coin.getVersion();

    if (!static_cast<int>(version)) //todo: figure out why version does not make it here
        version = 1;

    // Sanity check: let's verify that the Witness is valid with respect to
    // the coin and Accumulator provided.
    if (!(witness.VerifyWitness(a, coin.getPublicCoin()))) {
        throw std::runtime_error("Accumulator witness does not verify");
    }

    const libzeroct::IntegerGroupParams* group1 = &params->serialNumberSoKCommitmentGroup;
    const libzeroct::IntegerGroupParams* group2 = &params->accumulatorParams.accumulatorPoKCommitmentGroup;
    const libzeroct::IntegerGroupParams* group3 = &params->coinCommitmentGroup;

    CBigNum obfuscatedSerial = ((obfuscationJ.first*coin.getObfuscationValue())+obfuscationJ.second)%group3->groupOrder;
    CBigNum obfuscatedRandomness = ((obfuscationK.first*coin.getObfuscationValue())+obfuscationK.second)%group3->groupOrder;

    // 1: Generate two separate commitments to the public coin (C), each under
    // a different set of public parameters. We do this because the RSA accumulator
    // has specific requirements for the commitment parameters that are not
    // compatible with the group we use for the serial number proof.
    // Specifically, our serial number proof requires the order of the commitment group
    // to be the same as the modulus of the upper group. The Accumulator proof requires a
    // group with a significantly larger order.
    const Commitment fullCommitmentToCoinUnderSerialParams(group1, coin.getPublicCoin().getValue());
    this->serialCommitmentToCoinValue = fullCommitmentToCoinUnderSerialParams.getCommitmentValue();

    const Commitment fullCommitmentToCoinUnderAccParams(group2, coin.getPublicCoin().getValue());
    this->accCommitmentToCoinValue = fullCommitmentToCoinUnderAccParams.getCommitmentValue();

    // Generate a new amount commitment with a different randomness value
    const Commitment amountCommitment(group3, obfuscatedRandomness, 0, coin.getAmount());
    this->amountCommitment = amountCommitment.getCommitmentValue();

    const Commitment valueCommitment(group1, coin.getPublicCoin().getCoinValue());
    this->commitmentToCoinValue = valueCommitment.getCommitmentValue();

    // 2. Generate a ZK proof that the two commitments contain the same public coin.
    this->commitmentPoK = CommitmentProofOfKnowledge(group1, group2, fullCommitmentToCoinUnderSerialParams, fullCommitmentToCoinUnderAccParams, group1->g, group1->h, group2->g, group2->h);

    // Now generate the two core ZK proofs:
    // 3. Proves that the committed public coin is in the Accumulator (PoK of "witness")
    this->accumulatorPoK = AccumulatorProofOfKnowledge(&params->accumulatorParams, fullCommitmentToCoinUnderAccParams, witness, a);

    Commitment publicSerialNumber(group3, obfuscatedSerial, 0);
    this->coinValuePublic = publicSerialNumber.getCommitmentValue();

    // 4. Proves that the coin is correct w.r.t. serial number and hidden coin secret and the serial number image
    // (This proof is bound to the coin 'metadata', i.e., transaction hash)
    uint256 hashSig = signatureHash();
    this->serialNumberSoK = SerialNumberSignatureOfKnowledge(params, coin, fullCommitmentToCoinUnderSerialParams, publicSerialNumber, obfuscatedRandomness, amountCommitment, valueCommitment, hashSig);

    r = obfuscatedRandomness;

    //5. Zero knowledge proof of the serial number
    this->serialNumberPoK = SerialNumberProofOfKnowledge(group3, obfuscatedSerial, hashSig);
}

bool CoinSpend::Verify(const Accumulator& a) const
{

    if (!HasValidPublicSerial(p)) {
        return error("CoinsSpend::Verify: invalid Public Serial");
    }

    // Verify both of the sub-proofs using the given meta-data
    if (!commitmentPoK.Verify(serialCommitmentToCoinValue, accCommitmentToCoinValue)) {
        return error("CoinsSpend::Verify: commitmentPoK failed");
    }

    if (!accumulatorPoK.Verify(a, accCommitmentToCoinValue)) {
        return error("CoinsSpend::Verify: accumulatorPoK failed");
    }

    if (!serialNumberSoK.Verify(serialCommitmentToCoinValue, coinValuePublic, amountCommitment, commitmentToCoinValue, signatureHash())) {
        return error("CoinsSpend::Verify: serialNumberSoK failed.");
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
      << coinValuePublic << amountCommitment << commitmentToCoinValue << blockAccumulatorHash << spendType;

    return h.GetHash();
}

std::string CoinSpend::ToString() const
{
    std::stringstream ss;
    ss << "CoinSpend(" << coinValuePublic.ToString(16) << ")";
    return ss.str();
}

uint256 CoinSpend::GetHash() const {
    CHashWriter h(0, 0);
    h << *this;
    return h.GetHash();
}

bool CoinSpend::HasValidPublicSerial(const ZeroCTParams* params) const
{
    return IsValidPublicSerial(params, coinValuePublic);
}

CBigNum CoinSpend::CalculateValidPublicSerial(ZeroCTParams* params)
{
    CBigNum bnSerial = coinValuePublic;
    bnSerial = bnSerial.mul_mod(CBigNum(1),params->coinCommitmentGroup.modulus);
    return bnSerial;
}

} /* namespace libzeroct */
