/**
* @file       Math.h
*
* @brief      Bulletproofs Rangeproof
*             Inspired by https://github.com/b-g-goodell/research-lab/blob/master/source-code/StringCT-java/src/how/monero/hodl/bulletproof/Bulletproof.java
*
* @author     alex v
* @date       December 2018
*
* @copyright  Copyright 2018 alex v
* @license    This project is released under the MIT license.
**/
// Copyright (c) 2018 The NavCoin Core developers

#include "bulletproof_rangeproof.h"

namespace Bulletproof
{

bool Rangeproof::Prove(std::vector<CBigNum> v, std::vector<CBigNum> gamma, unsigned int logM)
{
    if (v.size() != gamma.size())
        return false;

    unsigned int M = v.size();

    int logMN = logM + this->nexp;

    if (pow(2, logM) < v.size())
        return false;

    this->V.resize(M);

    this->V[0] = p->h.pow_mod(v[0], p->modulus).mul_mod(p->g.pow_mod(gamma[0], p->modulus), p->modulus);

    // This hash is updated for Fiat-Shamir throughout the proof
    CHashWriter hasher(0,0);

    hasher << this->V[0];

    for (unsigned int j = 1; j < M; j++)
    {
        this->V[j] = p->h.pow_mod(v[j], p->modulus).mul_mod(p->g.pow_mod(gamma[j], p->modulus), p->modulus);
        hasher << this->V[j];
    }

    // PAPER LINES 36-37
    std::vector<CBigNum> aL(M*this->n);
    std::vector<CBigNum> aR(M*this->n);

    for (unsigned int j = 0; j < M; j++)
    {
        CBigNum tempV = v[j];

        for (int i = this->n-1; i>=0; i--)
        {
            CBigNum basePow = CBigNum(2).pow(i);

            if(tempV.div(basePow) == CBigNum())
            {
                aL[j*this->n+i] = CBigNum();
            }
            else
            {
                aL[j*this->n+i] = CBigNum(1);
                tempV = tempV - basePow;
            }

            aR[j*this->n+i] = aL[j*this->n+i] - CBigNum(1);
        }
    }

    // PAPER LINES 38-39
    CBigNum alpha = CBigNum::RandKBitBigum(256);

    if(!VectorVectorExponentMod(p->gi, aL, p->hi, aR, p->modulus, A))
        return false;

    this->A = this->A.mul_mod(p->g.pow_mod(alpha, p->modulus), p->modulus);

    // PAPER LINES 40-42
    std::vector<CBigNum> sL(M*this->n);
    std::vector<CBigNum> sR(M*this->n);

    for (unsigned int i = 0; i < M*this->n; i++)
    {
        sL[i] = CBigNum::RandKBitBigum(256);
        sR[i] = CBigNum::RandKBitBigum(256);
    }

    CBigNum rho = CBigNum::RandKBitBigum(256);

    if(!VectorVectorExponentMod(p->gi, sL, p->hi, sR, p->modulus, S))
        return false;

    this->S = this->S.mul_mod(p->g.pow_mod(rho, p->modulus), p->modulus);

    // PAPER LINES 43-45
    hasher << this->A;
    hasher << this->S;

    CBigNum y = CBigNum(hasher.GetHash());

    hasher << y;

    CBigNum z = CBigNum(hasher.GetHash());

    // Polynomial construction by coefficients
    std::vector<CBigNum> l0;
    std::vector<CBigNum> l1;
    std::vector<CBigNum> r0;
    std::vector<CBigNum> r1;

    std::vector<CBigNum> vTempBN;

    if(!VectorPowers(CBigNum(1),M*this->n, vTempBN))
        return false;

    if(!VectorScalarExp(vTempBN, z, vTempBN))
        return false;

    if(!VectorSubtract(aL, vTempBN, l0))
        return false;

    l1 = sL;

    // This computes the ugly sum/concatenation from PAPER LINE 65
    std::vector<CBigNum> zerosTwos(M*this->n);
    for(unsigned int i = 0; i<M*this->n; i++)
    {
        zerosTwos[i] = CBigNum();

        for (unsigned int j = 1; j<=M; j++) // note this starts from 1
        {
            CBigNum tempBN = 0;

            if (i >= (j-1)*this->n)
                tempBN = CBigNum(2).pow(i-(j-1)*this->n); // exponent ranges from 0..N-1

            zerosTwos[i] = (zerosTwos[i] + z.pow(1+j))*(tempBN);
        }
    }

    if (!VectorPowers(CBigNum(1),M*this->n,vTempBN))
        return false;

    if (!VectorScalar(vTempBN,z,vTempBN))
        return false;

    if (!VectorAdd(aR, vTempBN, r0))
        return false;

    if (!VectorPowers(y, M*this->n, vTempBN))
        return false;

    if (!Hadamard(r0, vTempBN, r0))
        return false;

    if (!VectorAdd(r0, zerosTwos, r0))
        return false;

    if (!VectorPowers(y, M*this->n, vTempBN))
        return false;

    if (!Hadamard(vTempBN, sR, r1))
        return false;

    // Polynomial construction before PAPER LINE 46
    CBigNum t0;
    CBigNum t1;
    CBigNum t2;
    CBigNum tempBN;
    CBigNum tempBN2;

    if (!InnerProduct(l0, r0, t0))
        return false;

    if (!InnerProduct(l0, r1, tempBN))
        return false;

    if (!InnerProduct(l1, r0, tempBN2))
        return false;

    t1 = tempBN + tempBN2;

    if (!InnerProduct(l1, r1, t2))
        return false;

    // PAPER LINES 47-48
    CBigNum tau1 = CBigNum::RandKBitBigum(256);
    CBigNum tau2 = CBigNum::RandKBitBigum(256);

    this->T1 = p->h.pow_mod(t1, p->modulus).mul_mod(p->g.pow_mod(tau1, p->modulus), p->modulus);
    this->T2 = p->h.pow_mod(t2, p->modulus).mul_mod(p->g.pow_mod(tau2, p->modulus), p->modulus);

    // PAPER LINES 49-51
    hasher << z;
    hasher << this->T1;
    hasher << this->T2;

    CBigNum x = CBigNum(hasher.GetHash());

    // PAPER LINES 52-53
    this->taux = tau1 * x;
    this->taux = this->taux + (tau2 * x.pow(2));

    for (unsigned int j = 1; j <= M; j++) // note this starts from 1
    {
        this->taux = this->taux + (z.pow(1+j)*(gamma[j-1]));
    }

    this->mu = (x * rho) + alpha;

    // PAPER LINES 54-57
    std::vector<CBigNum> l = l0;

    if (!VectorScalar(l1, x, vTempBN))
        return false;

    if (!VectorAdd(l, vTempBN, l))
        return false;

    std::vector<CBigNum> r = r0;

    if (!VectorScalar(r1, x, vTempBN))
        return false;

    if (!VectorAdd(r, vTempBN, r))
        return false;

    if (!InnerProduct(l,r,this->t))
        return false;

    // PAPER LINES 32-33
    hasher << x;
    hasher << this->taux;
    hasher << this->mu;
    hasher << this->t;

    CBigNum x_ip = CBigNum(hasher.GetHash());

    // These are used in the inner product rounds
    unsigned int nprime = M*this->n;

    std::vector<CBigNum> gprime(nprime);
    std::vector<CBigNum> hprime(nprime);
    std::vector<CBigNum> aprime(nprime);
    std::vector<CBigNum> bprime(nprime);

    for (unsigned int i = 0; i < nprime; i++)
    {
        gprime[i] = p->gi[i];
        hprime[i] = p->hi[i].pow_mod(y.inverse(p->groupOrder).pow(i), p->modulus);
        aprime[i] = l[i];
        bprime[i] = r[i];
    }

    this->L.resize(logMN);
    this->R.resize(logMN);

    unsigned int round = 0;

    std::vector<CBigNum> w(logMN);

    while (nprime > 1)
    {
        // PAPER LINE 15
        nprime /= 2;

        std::vector<CBigNum> vTempBN;
        std::vector<CBigNum> vTempBN2;
        std::vector<CBigNum> vTempBN3;
        std::vector<CBigNum> vTempBN4;

        // PAPER LINES 16-17
        if (!VectorSlice(aprime,0,nprime,vTempBN))
            return false;

        if (!VectorSlice(bprime,nprime,bprime.size(),vTempBN2))
            return false;

        CBigNum cL;

        if (!InnerProduct(vTempBN,vTempBN2,cL))
            return false;

        if (!VectorSlice(aprime,nprime,aprime.size(),vTempBN))
            return false;
        if (!VectorSlice(bprime,0,nprime,vTempBN2))
            return false;

        CBigNum cR;

        if (!InnerProduct(vTempBN,vTempBN2,cR))
            return false;

        // PAPER LINES 18-19
        if (!VectorSlice(gprime, nprime, gprime.size(), vTempBN))
            return false;

        if (!VectorSlice(hprime, 0, nprime, vTempBN2))
            return false;

        if (!VectorSlice(aprime, 0, nprime, vTempBN3))
            return false;

        if (!VectorSlice(bprime, nprime, bprime.size(), vTempBN4))
            return false;

        if (!VectorVectorExponentMod(vTempBN, vTempBN3, vTempBN2, vTempBN4, p->modulus, this->L[round]))
            return false;

        this->L[round] = this->L[round] * p->h.pow_mod(cL*x_ip,p->modulus);

        if (!VectorSlice(gprime, 0, nprime, vTempBN))
            return false;

        if (!VectorSlice(hprime, nprime, hprime.size(), vTempBN2))
            return false;

        if (!VectorSlice(aprime, nprime, aprime.size(), vTempBN3))
            return false;

        if (!VectorSlice(bprime, 0, nprime, vTempBN4))
            return false;

        if (!VectorVectorExponentMod(vTempBN, vTempBN3, vTempBN2, vTempBN4, p->modulus, this->R[round]))
            return false;

        this->R[round] = this->R[round] * p->h.pow_mod(cR*x_ip,p->modulus);

        // PAPER LINES 21-22
        hasher << this->L[round];
        hasher << this->R[round];

        w[round] = CBigNum(hasher.GetHash());

        // PAPER LINES 24-25
        if (!VectorSlice(gprime, 0, nprime, vTempBN))
            return false;

        if (!VectorScalar(vTempBN, w[round].inverse(p->groupOrder), vTempBN2))
            return false;

        if (!VectorSlice(gprime, nprime, gprime.size(), vTempBN3))
            return false;

        if (!VectorScalar(vTempBN3, w[round], vTempBN4))
            return false;

        if (!Hadamard(vTempBN2, vTempBN4, gprime))
            return false;

        if (!VectorSlice(hprime, 0, nprime, vTempBN))
            return false;

        if (!VectorScalar(vTempBN, w[round], vTempBN2))
            return false;

        if (!VectorSlice(hprime, nprime, hprime.size(), vTempBN3))
            return false;

        if (!VectorScalar(vTempBN3, w[round].inverse(p->groupOrder), vTempBN4))
            return false;

        if (!Hadamard(vTempBN2, vTempBN4, hprime))
            return false;

        // PAPER LINES 28-29
        if (!VectorSlice(aprime, 0, nprime, vTempBN))
            return false;

        if (!VectorScalar(vTempBN, w[round], vTempBN2))
            return false;

        if (!VectorSlice(aprime, nprime, aprime.size(), vTempBN3))
            return false;

        if (!VectorScalar(vTempBN3, w[round].inverse(p->groupOrder), vTempBN4))
            return false;

        if (!VectorAdd(vTempBN2, vTempBN4, aprime))
            return false;

        if (!VectorSlice(bprime, 0, nprime, vTempBN))
            return false;

        if (!VectorScalar(vTempBN, w[round].inverse(p->groupOrder), vTempBN2))
            return false;

        if (!VectorSlice(bprime, nprime, bprime.size(), vTempBN3))
            return false;

        if (!VectorScalar(vTempBN3, w[round], vTempBN4))
            return false;

        if (!VectorAdd(vTempBN2, vTempBN4, bprime))
            return false;

        round += 1;
    }

    this->a = aprime[0];
    this->b = bprime[0];

    return true;

}

}


