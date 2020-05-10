// Copyright (c) 2020 The NavCoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

// Diverse arithmetic operations in the bls curve
// inspired by https://github.com/b-g-goodell/research-lab/blob/master/source-code/StringCT-java/src/how/monero/hodl/bulletproof/Bulletproof.java
// and https://github.com/monero-project/monero/blob/master/src/ringct/bulletproofs.cc

#ifndef NAVCOIN_blsct_OPERATIONS_H
#define NAVCOIN_blsct_OPERATIONS_H

#include <amount.h>
#include <bls.hpp>
#include <blsct/types.h>
#include <utilstrencodings.h>

#include <boost/thread/mutex.hpp>
#include <boost/thread/lock_guard.hpp>

static const size_t maxN = 64;
static const size_t maxMessageSize = 20;
static const size_t maxM = 16;
static const size_t maxMN = maxM*maxN;

static const uint8_t balanceMsg[12] = {'B', 'L', 'S', 'C', 'T', 'B', 'A', 'L', 'A', 'N', 'C', 'E'};

class BulletproofsRangeproof
{
public:
    BulletproofsRangeproof() {}

    static bool Init();

    void Prove(const std::vector<Scalar> &v, const std::vector<Scalar> &gamma, Point nonce, const std::vector<uint8_t>& message = std::vector<uint8_t>());

    bool operator==(const BulletproofsRangeproof& rh) const {
        return (V == rh.V &&
                L == rh.L &&
                R == rh.R &&
                A == rh.A &&
                S == rh.S &&
                T1 == rh.T1 &&
                T2 == rh.T2 &&
                taux == rh.taux &&
                mu == rh.mu &&
                a == rh.a &&
                b == rh.b &&
                t == rh.t);
    }

    ADD_SERIALIZE_METHODS;
    template <typename Stream, typename Operation>  inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(V);
        READWRITE(L);
        READWRITE(R);
        READWRITE(A);
        READWRITE(S);
        READWRITE(T1);
        READWRITE(T2);
        READWRITE(taux);
        READWRITE(mu);
        READWRITE(a);
        READWRITE(b);
        READWRITE(t);
    }

    std::vector<Point> GetValueCommitments() const { return V; }

    static const size_t logN = 6;

    static Point G;
    static Point H;

    static Scalar one;
    static Scalar two;

    static std::vector<Point> Hi, Gi;
    static std::vector<Scalar> oneN;
    static std::vector<Scalar> twoN;
    static Scalar ip12;

    static boost::mutex init_mutex;

    std::vector<Point> V;
    std::vector<Point> L;
    std::vector<Point> R;
    Point A;
    Point S;
    Point T1;
    Point T2;
    Scalar taux;
    Scalar mu;
    Scalar a;
    Scalar b;
    Scalar t;
};

struct RangeproofEncodedData
{
    CAmount amount;
    Scalar gamma;
    std::vector<uint8_t> message;
    int index;
    bool valid = false;
};

bool VerifyBulletproof(const std::vector<std::pair<int, BulletproofsRangeproof>>& proofs, std::vector<RangeproofEncodedData>& data, const std::vector<Point>& nonces, const bool &fOnlyRecover = false);

#endif // NAVCOIN_blsct_OPERATIONS_H
