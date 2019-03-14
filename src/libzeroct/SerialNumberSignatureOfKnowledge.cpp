/**
* @file       SerialNumberSignatureOfKnowledge.cpp
*
* @brief      SerialNumberSignatureOfKnowledge class for the Zerocoin library.
*
* @author     Ian Miers, Christina Garman and Matthew Green
* @date       June 2013
*
* @copyright  Copyright 2013 Ian Miers, Christina Garman and Matthew Green
* @license    This project is released under the MIT license.
**/
// Copyright (c) 2017 The PIVX developers
// Copyright (c) 2018-2019 The NavCoin Core developers

#include <streams.h>
#include "SerialNumberSignatureOfKnowledge.h"

namespace libzeroct {

SerialNumberSignatureOfKnowledge::SerialNumberSignatureOfKnowledge(const ZeroCTParams* p): params(p) { }

// Use one 256 bit seed and concatenate 4 unique 256 bit hashes to make a 1024 bit hash
CBigNum SeedTo1024(uint256 hashSeed) {
    CHashWriter hasher(0,0);
    hasher << hashSeed;

    vector<unsigned char> vResult;
    vector<unsigned char> vHash = CBigNum(hasher.GetHash()).getvch();
    vResult.insert(vResult.end(), vHash.begin(), vHash.end());
    for (int i = 0; i < 3; i ++) {
        hasher << vResult;
        vHash = CBigNum(hasher.GetHash()).getvch();
        vResult.insert(vResult.end(), vHash.begin(), vHash.end());
    }

    CBigNum bnResult;
    bnResult.setvch(vResult);
    return bnResult;
}

SerialNumberSignatureOfKnowledge::SerialNumberSignatureOfKnowledge(const
                                                                   ZeroCTParams* p, const PrivateCoin& coin, const Commitment& commitmentToCoin, const Commitment& coinValuePublic,
                                                                   CBigNum obfuscatedRandomness, Commitment amountCommitment, Commitment commitmentToValue, uint256 msghash):params(p),
    xi(p->zkp_iterations),
    iota(p->zkp_iterations),
    delta(p->zkp_iterations),
    psi(p->zkp_iterations),
    nu(p->zkp_iterations),
    Omega(p->zkp_iterations),
    eta(p->zkp_iterations) {

    // Sanity check: verify that the order of the "accumulatedValueCommitmentGroup" is
    // equal to the modulus of "coinCommitmentGroup". Otherwise we will produce invalid
    // proofs.
    if (params->coinCommitmentGroup.modulus != params->serialNumberSoKCommitmentGroup.groupOrder) {
        throw std::runtime_error("Groups are not structured correctly.");
    }

    const CBigNum& a = params->coinCommitmentGroup.g;
    const CBigNum& b = params->coinCommitmentGroup.h;
    const CBigNum& c = params->coinCommitmentGroup.g2;
    const CBigNum& g = params->serialNumberSoKCommitmentGroup.g;
    const CBigNum& h = params->serialNumberSoKCommitmentGroup.h;

    const CBigNum& q = params->serialNumberSoKCommitmentGroup.groupOrder;

    CHashWriter hasher(0,0);
    hasher << *params << commitmentToCoin.getCommitmentValue() << coinValuePublic.getCommitmentValue() << amountCommitment.getCommitmentValue() << commitmentToValue.getCommitmentValue() << msghash;

    vector<CBigNum> rho(params->zkp_iterations);
    vector<CBigNum> tau(params->zkp_iterations);
    vector<CBigNum> alpha(params->zkp_iterations);
    vector<CBigNum> gamma(params->zkp_iterations);
    vector<CBigNum> zeta_seed(params->zkp_iterations);
    vector<CBigNum> zeta_expanded(params->zkp_iterations);
    vector<CBigNum> varpi_seed(params->zkp_iterations);
    vector<CBigNum> varpi_expanded(params->zkp_iterations);
    vector<CBigNum> varphi_seed(params->zkp_iterations);
    vector<CBigNum> varphi_expanded(params->zkp_iterations);

    vector<CBigNum> t(params->zkp_iterations);
    vector<CBigNum> upsilon(params->zkp_iterations);
    vector<CBigNum> mu(params->zkp_iterations);
    vector<CBigNum> kappa(params->zkp_iterations);

    for(uint32_t i=0; i < params->zkp_iterations; i++) {
        rho[i] = CBigNum::randBignum(params->coinCommitmentGroup.groupOrder);
        tau[i] = CBigNum::randBignum(params->coinCommitmentGroup.groupOrder);
        alpha[i] = CBigNum::randBignum(params->coinCommitmentGroup.groupOrder);
        gamma[i] = CBigNum::randBignum(params->coinCommitmentGroup.groupOrder);

        //use a random 256 bit seed that expands to 1024 bit for v[i]
        uint256 hashRand = CBigNum::randBignum(CBigNum(~uint256(0))).getuint256();
        CBigNum bnExpanded = SeedTo1024(hashRand) % params->serialNumberSoKCommitmentGroup.groupOrder;

        zeta_seed[i] = CBigNum(hashRand);
        zeta_expanded[i] = bnExpanded;

        hashRand = CBigNum::randBignum(CBigNum(~uint256(0))).getuint256();
        bnExpanded = SeedTo1024(hashRand) % params->serialNumberSoKCommitmentGroup.groupOrder;

        varpi_seed[i] = CBigNum(hashRand);
        varpi_expanded[i] = bnExpanded;

        hashRand = CBigNum::randBignum(CBigNum(~uint256(0))).getuint256();
        bnExpanded = SeedTo1024(hashRand) % params->serialNumberSoKCommitmentGroup.groupOrder;

        varphi_seed[i] = CBigNum(hashRand);
        varphi_expanded[i] = bnExpanded;
    }

    for(uint32_t i=0; i < params->zkp_iterations; i++)
    {
        // compute g^{ {S b^r} h^v} mod p2
        t[i]    = challengeCalculation(g, a, coinValuePublic.getContents(), b, rho[i], h, zeta_expanded[i]);

        // compute g^{ {a^alpha b^gamma c^tau} h^varpi} mod p2
        upsilon[i]
                = challengeCalculation(g, a, alpha[i], b, gamma[i], c, tau[i], h, varpi_expanded[i]);

        // compute a^tau c^rho mod p1
        mu[i]   = challengeCalculation(a, rho[i], 1, 1, c, tau[i]);

        // compute g^gamma h^varpji mod p2
        kappa[i]
                = challengeCalculation(g, gamma[i], 1, 1, 1, h, varphi_expanded[i]);
    }

    // We can't hash data in parallel either
    // because OPENMP cannot not guarantee loops
    // execute in order.
    for(uint32_t i=0; i < params->zkp_iterations; i++) {
        hasher << t[i];
        hasher << upsilon[i];
        hasher << mu[i];
        hasher << kappa[i];
    }
    this->omega = hasher.GetHash();
    unsigned char *hashbytes =  (unsigned char*) &omega;

    for(uint32_t i = 0; i < params->zkp_iterations; i++) {
        int bit = i % 8;
        int byte = i / 8;

        bool challenge_bit = ((hashbytes[byte] >> bit) & 0x01);
        if (challenge_bit) {
            xi[i]   = rho[i];
            iota[i] = tau[i];
            delta[i]
                    = alpha[i];
            psi[i]  = zeta_seed[i];
            nu[i]   = gamma[i];
            Omega[i]
                    = varpi_seed[i];
            eta[i]  = varphi_seed[i];
        } else {
            xi[i]   = rho[i] - obfuscatedRandomness;
            iota[i] = tau[i] - coin.getAmount();
            delta[i]
                    = alpha[i] - coin.getRandomness();
            psi[i]  = zeta_expanded[i] - (commitmentToValue.getRandomness() *
                                  b.pow_mod(xi[i], q));
            nu[i]   = gamma[i] - coin.getPublicCoin().getCoinValue();
            Omega[i]
                    = varpi_expanded[i] - (commitmentToCoin.getRandomness().mul_mod(
                                           a.pow_mod(delta[i], q), q).mul_mod(
                                           b.pow_mod(nu[i], q), q).mul_mod(
                                           c.pow_mod(iota[i], q), q));
            eta[i]  = varphi_expanded[i] - commitmentToValue.getRandomness();
        }
    }
}

inline CBigNum SerialNumberSignatureOfKnowledge::challengeCalculation(const CBigNum& g, const CBigNum& a,const CBigNum& a_exp,const CBigNum& b,const CBigNum& b_exp,
                                                                      const CBigNum& h, const CBigNum& h_exp) const
{
    return g.pow_mod(challengeCalculation(a,a_exp,b,b_exp), params->serialNumberSoKCommitmentGroup.modulus)
            .mul_mod(h.pow_mod(h_exp, params->serialNumberSoKCommitmentGroup.modulus), params->serialNumberSoKCommitmentGroup.modulus);
}

inline CBigNum SerialNumberSignatureOfKnowledge::challengeCalculation(const CBigNum& g, const CBigNum& a,const CBigNum& a_exp,const CBigNum& b,const CBigNum& b_exp,
                                                                      const CBigNum& c, const CBigNum& c_exp,const CBigNum& h, const CBigNum& h_exp) const
{
    return g.pow_mod(challengeCalculation(a,a_exp,b,b_exp,c,c_exp), params->serialNumberSoKCommitmentGroup.modulus)
            .mul_mod(h.pow_mod(h_exp, params->serialNumberSoKCommitmentGroup.modulus), params->serialNumberSoKCommitmentGroup.modulus);
}

inline CBigNum SerialNumberSignatureOfKnowledge::challengeCalculation(const CBigNum& a,const CBigNum& a_exp, const CBigNum& b, const CBigNum& b_exp, const CBigNum& c,
                                                                      const CBigNum& c_exp) const
{
    return a.pow_mod(a_exp, params->serialNumberSoKCommitmentGroup.groupOrder)
            .mul_mod(b.pow_mod(b_exp, params->serialNumberSoKCommitmentGroup.groupOrder), params->serialNumberSoKCommitmentGroup.groupOrder)
            .mul_mod(c.pow_mod(c_exp, params->serialNumberSoKCommitmentGroup.groupOrder), params->serialNumberSoKCommitmentGroup.groupOrder);
}

inline CBigNum SerialNumberSignatureOfKnowledge::challengeCalculation(const CBigNum& a,const CBigNum& a_exp, const CBigNum& b, const CBigNum& b_exp) const
{
    return a.pow_mod(a_exp, params->serialNumberSoKCommitmentGroup.groupOrder)
            .mul_mod(b.pow_mod(b_exp, params->serialNumberSoKCommitmentGroup.groupOrder), params->serialNumberSoKCommitmentGroup.groupOrder);
}

bool SerialNumberSignatureOfKnowledge::Verify(const CBigNum& valueOfCommitmentToCoin, const CBigNum& valueOfPublicCoin,
                                              CBigNum amountCommitment, CBigNum valueCommitment, const uint256 msghash) const {
    const CBigNum& a = params->coinCommitmentGroup.g;
    const CBigNum& b = params->coinCommitmentGroup.h;
    const CBigNum& c = params->coinCommitmentGroup.g2;
    const CBigNum& g = params->serialNumberSoKCommitmentGroup.g;
    const CBigNum& h = params->serialNumberSoKCommitmentGroup.h;

    const CBigNum& q = params->serialNumberSoKCommitmentGroup.groupOrder;
    const CBigNum& p = params->serialNumberSoKCommitmentGroup.modulus;

    CHashWriter hasher(0,0);
    hasher << *params << valueOfCommitmentToCoin << valueOfPublicCoin << amountCommitment << valueCommitment << msghash;

    vector<CBigNum> t(params->zkp_iterations);
    vector<CBigNum> upsilon(params->zkp_iterations);
    vector<CBigNum> mu(params->zkp_iterations);
    vector<CBigNum> kappa(params->zkp_iterations);

    unsigned char *hashbytes = (unsigned char*) &this->omega;

    for(uint32_t i = 0; i < params->zkp_iterations; i++)
    {
        int bit = i % 8;
        int byte = i / 8;
        bool challenge_bit = ((hashbytes[byte] >> bit) & 0x01);

        if(challenge_bit)
        {
            CBigNum zeta_expanded
                         = SeedTo1024(psi[i].getuint256()) % q;
            t[i]         = challengeCalculation(g, valueOfPublicCoin, 1, b, xi[i], h, zeta_expanded);

            CBigNum varpi_expanded
                         = SeedTo1024(Omega[i].getuint256()) % q;
            upsilon[i]   = challengeCalculation(g, a, delta[i], b, nu[i], c, iota[i], h, varpi_expanded);

            mu[i]        = challengeCalculation(a, xi[i], 1, 1, c, iota[i]);

            CBigNum varphi_expanded
                         = SeedTo1024(eta[i].getuint256()) % q;
            kappa[i]     = challengeCalculation(g, nu[i], 1, 1, 1, h, varphi_expanded);
        }
        else
        {
            CBigNum exp  = b.pow_mod(xi[i], q);
            t[i]         = valueCommitment.pow_mod(exp, p).mul_mod(h.pow_mod(psi[i], p), p);

            CBigNum exp2 = a.pow_mod(delta[i], q).mul_mod(b.pow_mod(nu[i], q), q).mul_mod(c.pow_mod(iota[i], q), q);
            upsilon[i]   = valueOfCommitmentToCoin.pow_mod(exp2, p).mul_mod(h.pow_mod(Omega[i], p), p);

            CBigNum exp3 = a.pow_mod(xi[i], q).mul_mod(c.pow_mod(iota[i], q), q);
            mu[i]        = amountCommitment.mul_mod(exp3, q);

            CBigNum exp4 = g.pow_mod(nu[i], p).mul_mod(h.pow_mod(eta[i], p), p);
            kappa[i]     = valueCommitment.mul_mod(exp4, p);

        }

    }

    for(uint32_t i = 0; i < params->zkp_iterations; i++)
    {
        hasher << t[i];
        hasher << upsilon[i];
        hasher << mu[i];
        hasher << kappa[i];
    }

    return hasher.GetHash() == omega;
}

} /* namespace libzeroct */

