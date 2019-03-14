/**
 * @file       Accumulator.cpp
 *
 * @brief      Accumulator and AccumulatorWitness classes for the Zerocoin library.
 *
 * @author     Ian Miers, Christina Garman and Matthew Green
 * @date       June 2013
 *
 * @copyright  Copyright 2013 Ian Miers, Christina Garman and Matthew Green
 * @license    This project is released under the MIT license.
 **/
// Copyright (c) 2017-2018 The PIVX developers
// Copyright (c) 2018-2019 The NavCoin Core developers

#include <sstream>
#include <iostream>
#include "Accumulator.h"
#include "ZerocoinDefines.h"

namespace libzeroct {

Accumulator::Accumulator(const ZeroCTParams* p, const CBigNum bnValue) {
    this->params = &(p->accumulatorParams);

    if (!(params->initialized)) {
        throw std::runtime_error("Invalid parameters for accumulator");
    }

    if(bnValue != 0)
        this->value = bnValue;
    else
        this->value = this->params->accumulatorBase;
}

void Accumulator::increment(const CBigNum& bnValue) {
    // Compute new accumulator = "old accumulator"^{element} mod N
    this->value = this->value.pow_mod(bnValue, this->params->accumulatorModulus);
}

void Accumulator::accumulate(const PublicCoin& coin) {
    // Make sure we're initialized
    if(!(this->value)) {
        throw std::runtime_error("Accumulator is not initialized");
    }

    if(coin.isValid()) {
        increment(coin.getValue());
    } else {
        throw std::runtime_error("Coin is not valid");
    }
}

const CBigNum& Accumulator::getValue() const {
    return this->value;
}

//Manually set accumulator value
void Accumulator::setValue(CBigNum bnValue) {
    this->value = bnValue;
}

void Accumulator::setValue(uint256 value) {
    this->value = CBigNum(value);
}

Accumulator& Accumulator::operator += (const PublicCoin& c) {
    this->accumulate(c);
    return *this;
}

bool Accumulator::operator == (const Accumulator rhs) const {
    return this->value == rhs.value;
}

//AccumulatorWitness class
AccumulatorWitness::AccumulatorWitness(const ZeroCTParams* p,
                                       const Accumulator& checkpoint, const PublicCoin coin): witness(checkpoint), element(coin) {
}

void AccumulatorWitness::resetValue(const Accumulator& checkpoint, const PublicCoin coin) {
    this->witness.setValue(checkpoint.getValue());
    this->element = coin;
}

void AccumulatorWitness::AddElement(const PublicCoin& c) {
    if(element.getValue() != c.getValue())
        witness += c;
}

void AccumulatorWitness::AddElement(const CBigNum& bnValue) {
    if(element.getValue() != bnValue)
        witness.increment(bnValue);
}

//warning check pubcoin value & denom outside of this function!
void AccumulatorWitness::addRawValue(const CBigNum& bnValue) {
    witness.increment(bnValue);
}

const CBigNum& AccumulatorWitness::getValue() const {
    return this->witness.getValue();
}

bool AccumulatorWitness::VerifyWitness(const Accumulator& a, const PublicCoin &publicCoin) const {
    Accumulator temp(a);
    temp.setValue(witness.getValue());
    temp.increment(element.getValue());
    if (!(temp == a)) {
        return false;
    } else if (this->element != publicCoin) {
        return false;
    }

    return true;
}

AccumulatorWitness& AccumulatorWitness::operator +=(
        const PublicCoin& rhs) {
    this->AddElement(rhs);
    return *this;
}

} /* namespace libzeroct */
