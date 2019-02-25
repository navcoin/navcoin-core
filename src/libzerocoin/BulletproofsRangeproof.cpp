/**
* @file       Math.h
*
* @brief      Bulletproofs Rangeproof
*             inspired by https://github.com/b-g-goodell/research-lab/blob/master/source-code/StringCT-java/src/how/monero/hodl/bulletproof/Bulletproof.java
*             and https://github.com/monero-project/monero/blob/master/src/ringct/bulletproofs.cc
*
* @author     alex v
* @date       February 2019
*
* @license    This project is released under the MIT license.
**/
// Copyright (c) 2018-2019 The NavCoin Core developers

#include "BulletproofsRangeproof.h"

CBN_vector oneN = VectorPowers(1, BulletproofsRangeproof::maxN);
CBN_vector twoN = VectorPowers(2, BulletproofsRangeproof::maxN);
CBigNum ip12 = InnerProduct(oneN, twoN);

void BulletproofsRangeproof::Prove(CBN_vector v, CBN_vector gamma)
{
    if (v.size() != gamma.size() || v.empty() || v.size() > BulletproofsRangeproof::maxM)
        throw std::runtime_error("BulletproofsRangeproof::Prove(): Invalid vector size");

    const size_t logN = 6;
    const size_t N = 1<<logN;

    CBigNum p = params->modulus;
    CBigNum q = params->groupOrder;

    size_t M, logM;
    for (logM = 0; (M = 1<<logM) <= maxM && M < v.size(); ++logM);
    const size_t logMN = logM + logN;
    const size_t MN = M * N;

    // V is a vector with commitments in the form g^v h^gamma
    this->V.resize(v.size());

    // This hash is updated for Fiat-Shamir throughout the proof
    CHashWriter hasher(0,0);

    for (unsigned int j = 0; j < v.size(); j++)
    {
        this->V[j] = params->g.pow_mod(gamma[j], p).mul_mod(params->h.pow_mod(v[j], p), p);
        hasher << this->V[j];
    }

    // PAPER LINES 41-42
    // Value to be obfuscated is encoded in binary in aL
    // aR is aL-1
    CBN_vector aL(MN), aR(MN);

    for (size_t j = 0; j < M; ++j)
    {
      for (size_t i = N; i-- > 0; )
      {
        if (j < v.size() && v[j].GetBit(i) == 1)
        {
          aL[j*N+i] = CBigNum(1);
          aR[j*N+i] = CBigNum(0);
        }
        else
        {
          aL[j*N+i] = CBigNum(0);
          aR[j*N+i] = CBigNum(-1);
        }
      }
    }

try_again:
    // PAPER LINES 43-44
    // Commitment to aL and aR (obfuscated with alpha)
    CBigNum alpha = CBigNum::randBignum(q);

    this->A = VectorExponent2Mod(params->gis, aL, params->his, aR, p);
    this->A = this->A.mul_mod(params->g.pow_mod(alpha, p), p);

    // PAPER LINES 45-47
    // Commitment to blinding sL and sR (obfuscated with rho)
    CBN_vector sL(MN);
    CBN_vector sR(MN);

    for (unsigned int i = 0; i < MN; i++)
    {
        sL[i] = CBigNum::randBignum(q);
        sR[i] = CBigNum::randBignum(q);
    }

    CBigNum rho = CBigNum::randBignum(q);

    this->S = VectorExponent2Mod(params->gis, sL, params->his, sR, p);
    this->S = this->S.mul_mod(params->g.pow_mod(rho, p), p);

    // PAPER LINES 48-50
    hasher << this->A;
    hasher << this->S;

    CBigNum y = CBigNum(hasher.GetHash());

    if (y == 0)
        goto try_again;

    hasher << y;

    CBigNum z = CBigNum(hasher.GetHash());

    if (z == 0)
        goto try_again;

    // Polynomial construction by coefficients
    // PAPER LINE AFTER 50
    CBN_vector l0;
    CBN_vector l1;
    CBN_vector r0;
    CBN_vector r1;

    // l(x) = (aL - z 1^n) + sL X
    l0 = VectorSubtract(aL, z);

    // l(1) is (aL - z 1^n) + sL, but this is reduced to sL
    l1 = sL;

    // This computes the ugly sum/concatenation from page 19
    // Calculation of r(0) and r(1)
    CBN_vector zerosTwos(MN);
    CBN_vector zpow(M+2);

    zpow = VectorPowers(z, M+2);

    for(unsigned int i = 0; i<MN; i++)
    {
        zerosTwos[i] = 0;

        for (unsigned int j = 1; j<=M; j++) // note this starts from 1
        {
            if (i >= (j-1)*N && i < j*N)
                zerosTwos[i] = zerosTwos[i] + (zpow[1+j]*twoN[i-(j-1)*N]);
        }
    }

    CBN_vector yMN = VectorPowers(y, MN);
    r0 = VectorAdd(Hadamard(VectorAdd(aR, z), yMN, q), zerosTwos, q);
    r1 = Hadamard(yMN, sR, q);

    // Polynomial construction before PAPER LINE 51
    CBigNum t1 = ((InnerProduct(l0, r1, q) + InnerProduct(l1, r0, q)) % q);
    CBigNum t2 = InnerProduct(l1, r1, q);

    // PAPER LINES 52-53
    CBigNum tau1 = CBigNum::randBignum(q);
    CBigNum tau2 = CBigNum::randBignum(q);

    this->T1 = params->h.pow_mod(t1, p).mul_mod(params->g.pow_mod(tau1, p), p);
    this->T2 = params->h.pow_mod(t2, p).mul_mod(params->g.pow_mod(tau2, p), p);

    // PAPER LINES 54-56
    hasher << z;
    hasher << this->T1;
    hasher << this->T2;

    CBigNum x = CBigNum(hasher.GetHash());

    if (x == 0)
        goto try_again;

    // PAPER LINES 58-59
    CBN_vector l = VectorAdd(l0, VectorScalar(l1, x, q), q);
    CBN_vector r = VectorAdd(r0, VectorScalar(r1, x, q), q);

    // PAPER LINE 60
    this->t = InnerProduct(l, r, q);

    // TEST
    // CBigNum test_t;
    // CBigNum t0 = InnerProduct(l0, r0, q);
    // test_t = ((t0 + (t1*x)) + (t2*x*x)) % q;
    // if (test_t!=this->t)
    //     throw std::runtime_error("BulletproofsRangeproof::Prove(): L60 Invalid test");

    // PAPER LINES 61-62
    this->taux = tau1 * x;
    this->taux = this->taux + (tau2 * x.pow(2));

    for (unsigned int j = 1; j <= M; j++) // note this starts from 1
        this->taux = (this->taux + (zpow[j+1]*(gamma[j-1]))) % q;

    this->mu = ((x * rho) + alpha) % q;

    // PAPER LINE 63
    hasher << x;
    hasher << this->taux;
    hasher << this->mu;
    hasher << this->t;

    CBigNum x_ip = CBigNum(hasher.GetHash());

    if (x_ip == 0)
        goto try_again;

    // These are used in the inner product rounds
    unsigned int nprime = MN;

    CBN_vector gprime(nprime);
    CBN_vector hprime(nprime);
    CBN_vector aprime(nprime);
    CBN_vector bprime(nprime);

    CBigNum yinv = y.inverse(q);
    CBN_vector yinvpow(nprime);

    yinvpow[0] = 1;
    yinvpow[1] = yinv;

    for (unsigned int i = 0; i < nprime; i++)
    {
        gprime[i] = params->gis[i];
        hprime[i] = params->his[i];

        if(i > 1)
            yinvpow[i] = yinvpow[i-1].mul_mod(yinv, q);

        aprime[i] = l[i];
        bprime[i] = r[i];
    }

    this->L.resize(logMN);
    this->R.resize(logMN);

    unsigned int round = 0;

    CBN_vector w(logMN);

    CBN_vector* scale = &yinvpow;

    CBigNum tmp;

    while (nprime > 1)
    {
        // PAPER LINE 20
        nprime /= 2;

        // PAPER LINES 21-22
        CBigNum cL = InnerProduct(VectorSlice(aprime, 0, nprime),
                                  VectorSlice(bprime, nprime, bprime.size()),
                                  q);

        CBigNum cR = InnerProduct(VectorSlice(aprime, nprime, aprime.size()),
                                  VectorSlice(bprime, 0, nprime),
                                  q);

        // PAPER LINES 23-24
        tmp = cL.mul_mod(x_ip, q);
        this->L[round] = CrossVectorExponent(nprime, gprime, nprime, hprime, 0, aprime, 0, bprime, nprime, scale, &params->h, &tmp, params);
        tmp = cR.mul_mod(x_ip, q);
        this->R[round] = CrossVectorExponent(nprime, gprime, 0, hprime, nprime, aprime, nprime, bprime, 0, scale, &params->h, &tmp, params);

        // PAPER LINES 25-27
        hasher << this->L[round];
        hasher << this->R[round];

        w[round] = CBigNum(hasher.GetHash());

        if (w[round] == 0)
            goto try_again;

        CBigNum winv = w[round].inverse(q);

        // PAPER LINES 29-31
        if (nprime > 1)
        {
            gprime = HadamardFold(gprime, NULL, winv, w[round], p, q);
            hprime = HadamardFold(hprime, scale, w[round], winv, p, q);
        }

        // PAPER LINES 33-34
        aprime = VectorAdd(VectorScalar(VectorSlice(aprime, 0, nprime), w[round], q),
                           VectorScalar(VectorSlice(aprime, nprime, aprime.size()), winv, q), q);

        bprime = VectorAdd(VectorScalar(VectorSlice(bprime, 0, nprime), winv, q),
                           VectorScalar(VectorSlice(bprime, nprime, bprime.size()), w[round], q), q);

        scale = NULL;

        round += 1;
    }

    this->a = aprime[0];
    this->b = bprime[0];
}

struct proof_data_t
{
    CBigNum x, y, z, x_ip;
    CBN_vector w;
    size_t logM, inv_offset;
};

bool VerifyBulletproof(const libzerocoin::IntegerGroupParams* params, const std::vector<BulletproofsRangeproof>& proofs)
{
    if (proofs.size() == 0)
        throw std::runtime_error("VerifyBulletproof(): Empty proofs vector");

    unsigned int logN = 6;
    unsigned int N = 1 << logN;

    CBigNum p = params->modulus;
    CBigNum q = params->groupOrder;

    size_t max_length = 0;
    size_t nV = 0;

    std::vector<proof_data_t> proof_data;
    proof_data.reserve(proofs.size());

    size_t inv_offset = 0;
    CBN_vector to_invert;

    for (const BulletproofsRangeproof proof: proofs)
    {
        if (!(proof.V.size() >= 1 && proof.L.size() == proof.R.size() &&
              proof.L.size() > 0))
            return false;

        max_length = std::max(max_length, proof.L.size());
        nV += proof.V.size();
        proof_data.resize(proof_data.size() + 1);
        proof_data_t &pd = proof_data.back();

        CHashWriter hasher(0,0);

        hasher << proof.V[0];

        for (unsigned int j = 1; j < proof.V.size(); j++)
            hasher << proof.V[j];

        hasher << proof.A;
        hasher << proof.S;

        pd.y = CBigNum(hasher.GetHash());

        hasher << pd.y;

        pd.z = CBigNum(hasher.GetHash());

        hasher << pd.z;
        hasher << proof.T1;
        hasher << proof.T2;

        pd.x = CBigNum(hasher.GetHash());

        hasher << pd.x;
        hasher << proof.taux;
        hasher << proof.mu;
        hasher << proof.t;

        pd.x_ip = CBigNum(hasher.GetHash());

        size_t M;
        for (pd.logM = 0; (M = 1<<pd.logM) <= BulletproofsRangeproof::maxM && M < proof.V.size(); ++pd.logM);

        const size_t rounds = pd.logM+logN;

        pd.w.resize(rounds);
        for (size_t i = 0; i < rounds; ++i)
        {
            hasher << proof.L[i];
            hasher << proof.R[i];

            pd.w[i] = CBigNum(hasher.GetHash());
        }

        pd.inv_offset = inv_offset;
        for (size_t i = 0; i < rounds; ++i)
            to_invert.push_back(pd.w[i]);
        to_invert.push_back(pd.y);
        inv_offset += rounds + 1;
    }

    size_t maxMN = 1u << max_length;

    CBN_vector inverses(to_invert.size());

    inverses = VectorInvert(to_invert, q);

    CBigNum z1 = 0;
    CBigNum z3 = 0;
    CBN_vector z4(maxMN, 0);
    CBN_vector z5(maxMN, 0);

    CBigNum y0 = 0;
    CBigNum y1 = 0;

    CBigNum tmp;

    int proof_data_index = 0;

    std::vector<MultiexpData> multiexpdata;

    multiexpdata.reserve(nV + (2 * (10/*logM*/ + logN) + 4) * proofs.size() + 2 * maxMN);
    multiexpdata.resize(2 * maxMN);

    for (const BulletproofsRangeproof proof: proofs)
    {
        const proof_data_t &pd = proof_data[proof_data_index++];

        if (proof.L.size() != logN+pd.logM)
            return false;

        const size_t M = 1 << pd.logM;
        const size_t MN = M*N;

        CBigNum weight_y = CBigNum::randBignum(q);
        CBigNum weight_z = CBigNum::randBignum(q);

        y0 = y0 - (proof.taux * weight_y);

        CBN_vector zpow = VectorPowers(pd.z, M+3);

        CBigNum k;
        CBigNum ip1y = VectorPowerSum(pd.y, MN);

        k = 0 - (zpow[2]*ip1y);

        for (size_t j = 1; j <= M; ++j)
        {
            k = k - (zpow[j+2]*ip12);
        }

        tmp = k + (pd.z*ip1y);

        tmp = (proof.t - tmp);

        y1 = y1 + (tmp * weight_y);

        for (size_t j = 0; j < proof.V.size(); j++)
        {
            tmp = zpow[j+2] * weight_y;
            multiexpdata.push_back({proof.V[j], tmp});
        }

        tmp = pd.x * weight_y;

        multiexpdata.push_back({proof.T1, tmp});

        tmp = pd.x * pd.x * weight_y;

        multiexpdata.push_back({proof.T2, tmp});

        multiexpdata.push_back({proof.A, weight_z});

        tmp = pd.x * weight_z;

        multiexpdata.push_back({proof.S, tmp});

        const size_t rounds = pd.logM+logN;

        CBigNum yinvpow = 1;
        CBigNum ypow = 1;

        const CBigNum *winv = &inverses[pd.inv_offset];
        const CBigNum yinv = inverses[pd.inv_offset + rounds];

        CBN_vector w_cache(1<<rounds, 1);
        w_cache[0] = winv[0];
        w_cache[1] = pd.w[0];

        for (size_t j = 1; j < rounds; ++j)
        {
            const size_t sl = 1<<(j+1);
            for (size_t s = sl; s-- > 0; --s)
            {
                w_cache[s] = w_cache[s/2] * pd.w[j];
                w_cache[s-1] = w_cache[s/2] * winv[j];
            }
        }

        for (size_t i = 0; i < MN; ++i)
        {
            CBigNum g_scalar = proof.a;
            CBigNum h_scalar;

            if (i == 0)
                h_scalar = proof.b;
            else
                h_scalar = proof.b * yinvpow;

            g_scalar = g_scalar * w_cache[i];
            h_scalar = h_scalar * w_cache[(~i) & (MN-1)];

            g_scalar = g_scalar + pd.z;

            tmp = zpow[2+i/N] * twoN[i%N];

            if (i == 0)
            {
                tmp = tmp + pd.z;
                h_scalar = h_scalar - tmp;
            }
            else
            {
                tmp = tmp + (pd.z*ypow);
                h_scalar = h_scalar - (tmp * yinvpow);
            }

            z4[i] = z4[i] - (g_scalar * weight_z);
            z5[i] = z5[i] - (h_scalar * weight_z);

            if (i == 0)
            {
                yinvpow = yinv;
                ypow = pd.y;
            }
            else if (i != MN-1)
            {
                yinvpow = yinvpow * yinv;
                ypow = ypow * pd.y;
            }
        }

        z1 = z1 + (proof.mu * weight_z);

        for (size_t i = 0; i < rounds; ++i)
        {
            tmp = pd.w[i] * pd.w[i];
            tmp = tmp * weight_z;

            multiexpdata.push_back({proof.L[i], tmp});

            tmp = winv[i] * winv[i];
            tmp = tmp * weight_z;

            multiexpdata.push_back({proof.R[i], tmp});
        }

        tmp = proof.t - (proof.a*proof.b);
        tmp = tmp * pd.x_ip;
        z3 = z3 + (tmp * weight_z);
    }

    tmp = y0 - z1;

    multiexpdata.push_back({params->g, tmp});

    tmp = z3 - y1;

    multiexpdata.push_back({params->h, tmp});

    for (size_t i = 0; i < maxMN; ++i)
    {
        multiexpdata[i * 2] = {params->gis[i], z4[i]};
        multiexpdata[i * 2 + 1] = {params->his[i], z5[i]};
    }

    return MultiExp(multiexpdata, params) == 1;
}

CBigNum CrossVectorExponent(size_t size, const std::vector<CBigNum> &A, size_t Ao, const std::vector<CBigNum> &B, size_t Bo, const std::vector<CBigNum> &a, size_t ao, const std::vector<CBigNum> &b, size_t bo, const std::vector<CBigNum> *scale, const CBigNum *extra_point, const CBigNum *extra_scalar, const libzerocoin::IntegerGroupParams* p)
{
    if (!(size + Ao <= A.size()))
        throw std::runtime_error("CrossVectorExponent(): Incompatible size for A");

    if (!(size + Bo <= B.size()))
        throw std::runtime_error("CrossVectorExponent(): Incompatible size for B");

    if (!(size + ao <= a.size()))
        throw std::runtime_error("CrossVectorExponent(): Incompatible size for a");

    if (!(size + bo <= b.size()))
        throw std::runtime_error("CrossVectorExponent(): Incompatible size for b");

    if (!(size <= BulletproofsRangeproof::maxN*BulletproofsRangeproof::maxM))
        throw std::runtime_error("CrossVectorExponent(): Size is too large");

    if (!(!scale || size == scale->size() / 2))
        throw std::runtime_error("CrossVectorExponent(): Incompatible size for scale");

    if (!(!!extra_point == !!extra_scalar))
        throw std::runtime_error("CrossVectorExponent(): Only one of extra base/exp present");

    std::vector<MultiexpData> multiexp_data;
    multiexp_data.resize(size*2 + (!!extra_point));
    for (size_t i = 0; i < size; ++i)
    {
        multiexp_data[i*2].exp = a[ao+i];
        multiexp_data[i*2].base = A[Ao+i];
        multiexp_data[i*2+1].exp = b[bo+i];

        if (scale)
            multiexp_data[i*2+1].exp =  multiexp_data[i*2+1].exp * (*scale)[Bo+i];

        multiexp_data[i*2+1].base = B[Bo+i];
    }
    if (extra_point)
    {
        multiexp_data.back().exp = *extra_scalar;
        multiexp_data.back().base = *extra_point;
    }

    return MultiExp(multiexp_data, p);
}

