// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef NAVCOIN_PRIMITIVES_TRANSACTION_H
#define NAVCOIN_PRIMITIVES_TRANSACTION_H

#include <amount.h>
#include <blsct/bulletproofs.h>
#include <consensus/dao.h>
#include <sph_keccak.h>
#include <script/script.h>
#include <serialize.h>
#include <uint256.h>
#include <arith_uint256.h>
#include <univalue/include/univalue.h>

#define TX_BLS_INPUT_FLAG 0x10
#define TX_BLS_CT_FLAG 0x20

static const int SERIALIZE_TRANSACTION_NO_WITNESS = 0x40000000;

static const int WITNESS_SCALE_FACTOR = 4;

class CWalletTx;
class CMerkleTx;

/** An outpoint - a combination of a transaction hash and an index n into its vout */
class COutPoint
{
public:
    uint256 hash;
    uint32_t n;

    COutPoint() { SetNull(); }
    COutPoint(uint256 hashIn, uint32_t nIn) { hash = hashIn; n = nIn; }

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(hash);
        READWRITE(n);
    }

    void SetNull() { hash.SetNull(); n = (uint32_t) -1; }
    bool IsNull() const { return (hash.IsNull() && n == (uint32_t) -1); }

    friend bool operator<(const COutPoint& a, const COutPoint& b)
    {
        int cmp = a.hash.Compare(b.hash);
        return cmp < 0 || (cmp == 0 && a.n < b.n);
    }

    friend bool operator==(const COutPoint& a, const COutPoint& b)
    {
        return (a.hash == b.hash && a.n == b.n);
    }

    friend bool operator!=(const COutPoint& a, const COutPoint& b)
    {
        return !(a == b);
    }

    uint256 GetHash() const
    {
        return SerializeHash(*this, SER_GETHASH, SERIALIZE_TRANSACTION_NO_WITNESS);
    }

    std::string ToString() const;
};

/** An input of a transaction.  It contains the location of the previous
 * transaction's output that it claims and a signature that matches the
 * output's public key.
 */
class CTxIn
{
public:
    COutPoint prevout;
    CScript scriptSig;
    uint32_t nSequence;

    /* Setting nSequence to this value for every input in a transaction
     * disables nLockTime. */
    static const uint32_t SEQUENCE_FINAL = 0xffffffff;

    /* Below flags apply in the context of BIP 68*/
    /* If this flag set, CTxIn::nSequence is NOT interpreted as a
     * relative lock-time. */
    static const uint32_t SEQUENCE_LOCKTIME_DISABLE_FLAG = (1 << 31);

    /* If CTxIn::nSequence encodes a relative lock-time and this flag
     * is set, the relative lock-time has units of 512 seconds,
     * otherwise it specifies blocks with a granularity of 1. */
    static const uint32_t SEQUENCE_LOCKTIME_TYPE_FLAG = (1 << 22);

    /* If CTxIn::nSequence encodes a relative lock-time, this mask is
     * applied to extract that lock-time from the sequence field. */
    static const uint32_t SEQUENCE_LOCKTIME_MASK = 0x0000ffff;

    /* In order to use the same number of bits to encode roughly the
     * same wall-clock duration, and because blocks are naturally
     * limited to occur every 600s on average, the minimum granularity
     * for time-based relative lock-time is fixed at 512 seconds.
     * Converting from CTxIn::nSequence to seconds is performed by
     * multiplying by 512 = 2^9, or equivalently shifting up by
     * 9 bits. */
    static const int SEQUENCE_LOCKTIME_GRANULARITY = 9;

    CTxIn()
    {
        nSequence = SEQUENCE_FINAL;
    }

    explicit CTxIn(COutPoint prevoutIn, CScript scriptSigIn=CScript(), uint32_t nSequenceIn=SEQUENCE_FINAL);
    CTxIn(uint256 hashPrevTx, uint32_t nOut, CScript scriptSigIn=CScript(), uint32_t nSequenceIn=SEQUENCE_FINAL);

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(prevout);
        READWRITE(*(CScriptBase*)(&scriptSig));
        READWRITE(nSequence);
    }

    friend bool operator==(const CTxIn& a, const CTxIn& b)
    {
        return (a.prevout   == b.prevout &&
                a.scriptSig == b.scriptSig &&
                a.nSequence == b.nSequence);
    }

    friend bool operator!=(const CTxIn& a, const CTxIn& b)
    {
        return !(a == b);
    }

    friend bool operator<(const CTxIn& a, const CTxIn& b)
    {
        CHashWriter ahw(0,0);
        ahw << a;
        uint256 ah = ahw.GetHash();
        CHashWriter bhw(0,0);
        bhw << b;
        uint256 bh = bhw.GetHash();
        return ah.Compare(bh);
    }

    std::string ToString() const;
};

/** An output of a transaction.  It contains the public key that the next input
 * must be able to sign with to claim it.
 */
class CTxOut
{
public:
    CAmount nValue;
    CScript scriptPubKey;
    std::vector<uint8_t> ephemeralKey;
    std::vector<uint8_t> outputKey;
    std::vector<uint8_t> spendingKey;
    std::vector<uint8_t> vData;
    std::pair<uint256,uint64_t> tokenId;
    BulletproofsRangeproof bp;
    uint256 cacheHash;

    CTxOut()
    {
        SetNull();
    }

    CTxOut(const CAmount& nValueIn, CScript scriptPubKeyIn);
    CTxOut(const CAmount& nValueIn, CScript scriptPubKeyIn, const bls::G1Element& ephemeralKeyIn, const bls::G1Element& outputKeyIn, const bls::G1Element& spendingKeyIn, const BulletproofsRangeproof& bpIn);

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        if (ser_action.ForRead())
        {
            bool fXNav = false;
            uint64_t nFlags;
            READWRITE(nFlags);
            if (nFlags == ~(uint64_t)0)
            {
                READWRITE(nValue);
                READWRITE(ephemeralKey);
                READWRITE(outputKey);
                READWRITE(spendingKey);
                READWRITE(bp);
                fXNav = true;
            }
            else if (nFlags & (uint64_t)0x2<<62)
            {
                if (nFlags & 0x1<<0)
                    READWRITE(nValue);
                else
                    nValue=0;
                if (nFlags & 0x1<<1)
                    READWRITE(ephemeralKey);
                if (nFlags & 0x1<<2)
                    READWRITE(outputKey);
                if (nFlags & 0x1<<3)
                    READWRITE(spendingKey);

                if (nFlags & 0x1<<4)
                {
                    READWRITE(bp);
                }
                if (nFlags & 0x1<<5)
                    READWRITE(tokenId.first);
                if (nFlags & 0x1<<6)
                    READWRITE(tokenId.second);
                if (nFlags & 0x1<<7)
                {
                    READWRITE(vData);
                }

                fXNav = true;
            }
            else
            {
                nValue = nFlags;
            }
            READWRITE(*(CScriptBase*)(&scriptPubKey));

            if (fXNav)
                cacheHash = SerializeHash(*this);
        }
        else
        {
            if (vData.size() > 0 || tokenId.first != uint256() || tokenId.second != -1)
            {
                CAmount nMarker = (uint64_t)0x2 << 62;

                if (nValue > 0)
                    nMarker  |= 0x1<<0;
                if (ephemeralKey.size() > 0)
                    nMarker  |= 0x1<<1;
                if (outputKey.size() > 0)
                    nMarker  |= 0x1<<2;
                if (spendingKey.size() > 0)
                    nMarker  |= 0x1<<3;
                if (bp.V.size() > 0) {
                    nMarker  |= 0x1<<4;
                }
                if (tokenId.first != uint256()) {
                    nMarker  |= 0x1<<5;
                }
                if (tokenId.second != -1) {
                    nMarker  |= 0x1<<6;
                }
                if (vData.size() > 0)
                {
                    nMarker  |= 0x1<<7;
                }

                READWRITE(nMarker);

                if (nMarker & 0x1<<0)
                    READWRITE(nValue);
                if (nMarker & 0x1<<1)
                    READWRITE(ephemeralKey);
                if (nMarker & 0x1<<2)
                    READWRITE(outputKey);
                if (nMarker & 0x1<<3)
                    READWRITE(spendingKey);
                if (nMarker & 0x1<<4) {
                    READWRITE(bp);
                }
                if (nMarker & 0x1<<5)
                    READWRITE(tokenId.first);
                if (nMarker & 0x1<<6)
                    READWRITE(tokenId.second);
                if (nMarker & 0x1<<7)
                {
                    READWRITE(vData);
                }
            }
            else if (IsBLSCT())
            {
                CAmount nMarker = ~(uint64_t)0;
                READWRITE(nMarker);
                READWRITE(nValue);
                READWRITE(ephemeralKey);
                READWRITE(outputKey);
                READWRITE(spendingKey);
                READWRITE(bp);
            }
            else
            {
                READWRITE(nValue);
            }
            READWRITE(*(CScriptBase*)(&scriptPubKey));
        }
    }

    void SetNull()
    {
        nValue = -1;
        scriptPubKey.clear();
        ephemeralKey.clear();
        outputKey.clear();
        spendingKey.clear();
        vData.clear();
        tokenId = std::make_pair(uint256(), -1);
    }

    BulletproofsRangeproof GetBulletproof() const
    {
        return bp;
    }

    bool IsBLSCT() const
    {
        return ephemeralKey.size() > 0 || spendingKey.size() > 0 || outputKey.size() > 0;
    }

    bool HasRangeProof() const
    {
        return GetBulletproof().V.size() > 0;
    }

    bool IsNull() const
    {
        return (nValue == -1 && scriptPubKey.empty() && spendingKey.empty() && ephemeralKey.empty() && outputKey.empty());
    }

    bool IsEmpty() const
    {
        return (nValue == 0 && scriptPubKey.empty() && spendingKey.empty() && ephemeralKey.empty() && outputKey.empty());
    }

    bool IsCommunityFundContribution() const
    {
        return scriptPubKey.IsCommunityFundContribution();
    }

    bool IsVote() const
    {
        return scriptPubKey.IsProposalVote() || scriptPubKey.IsPaymentRequestVote();
    }

    bool IsProposalVote() const
    {
        return scriptPubKey.IsProposalVote();
    }

    bool IsPaymentRequestVote() const
    {
        return scriptPubKey.IsPaymentRequestVote();
    }

    bool IsSupportVote() const
    {
        return scriptPubKey.IsSupportVote();
    }

    bool IsConsultationVote() const
    {
        return scriptPubKey.IsConsultationVote();
    }

    uint256 GetHash() const;

    CAmount GetDustThreshold(const CFeeRate &minRelayTxFee) const
    {
        // "Dust" is defined in terms of CTransaction::minRelayTxFee,
        // which has units satoshis-per-kilobyte.
        // If you'd pay more than 1/3 in fees
        // to spend something, then we consider it dust.
        // A typical spendable non-segwit txout is 34 bytes big, and will
        // need a CTxIn of at least 148 bytes to spend:
        // so dust is a spendable txout less than
        // 546*minRelayTxFee/1000 (in satoshis).
        // A typical spendable segwit txout is 31 bytes big, and will
        // need a CTxIn of at least 67 bytes to spend:
        // so dust is a spendable txout less than
        // 294*minRelayTxFee/1000 (in satoshis).
        if (scriptPubKey.IsUnspendable())
            return 0;

        size_t nSize = GetSerializeSize(SER_DISK, 0);
        int witnessversion = 0;
        std::vector<unsigned char> witnessprogram;

        if (scriptPubKey.IsWitnessProgram(witnessversion, witnessprogram)) {
            // sum the sizes of the parts of a transaction input
            // with 75% segwit discount applied to the script size.
            nSize += (32 + 4 + 1 + (107 / WITNESS_SCALE_FACTOR) + 4);
        } else {
            nSize += (32 + 4 + 1 + 107 + 4); // the 148 mentioned above
        }

        return 3 * minRelayTxFee.GetFee(nSize);
    }

    bool IsDust(const CFeeRate &minRelayTxFee) const
    {
        return (nValue < GetDustThreshold(minRelayTxFee));
    }

    friend bool operator==(const CTxOut& a, const CTxOut& b)
    {
        return (a.nValue       == b.nValue &&
                a.scriptPubKey == b.scriptPubKey &&
                a.ephemeralKey == b.ephemeralKey &&
                a.outputKey    == b.outputKey &&
                a.spendingKey  == b.spendingKey &&
                a.bp           == b.bp &&
                a.tokenId      == b.tokenId &&
                a.vData        == b.vData);
    }

    friend bool operator!=(const CTxOut& a, const CTxOut& b)
    {
        return !(a == b);
    }

    friend bool operator<(const CTxOut& a, const CTxOut& b)
    {
        return a.GetHash().Compare(b.GetHash());
    }

    std::string ToString() const;
};

class CTxInWitness
{
public:
    CScriptWitness scriptWitness;

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion)
    {
        READWRITE(scriptWitness.stack);
    }

    bool IsNull() const { return scriptWitness.IsNull(); }

    CTxInWitness() { }
};

class CTxWitness
{
public:
    /** In case vtxinwit is missing, all entries are treated as if they were empty CTxInWitnesses */
    std::vector<CTxInWitness> vtxinwit;

    ADD_SERIALIZE_METHODS;

    bool IsEmpty() const { return vtxinwit.empty(); }

    bool IsNull() const
    {
        for (size_t n = 0; n < vtxinwit.size(); n++) {
            if (!vtxinwit[n].IsNull()) {
                return false;
            }
        }
        return true;
    }

    void SetNull()
    {
        vtxinwit.clear();
    }

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion)
    {
        for (size_t n = 0; n < vtxinwit.size(); n++) {
            READWRITE(vtxinwit[n]);
        }
        if (IsNull()) {
            /* It's illegal to encode a witness when all vtxinwit entries are empty. */
            throw std::ios_base::failure("Superfluous witness record");
        }
    }
};

struct CMutableTransaction;

/**
 * Basic transaction serialization format:
 * - int32_t nVersion
 * - std::vector<CTxIn> vin
 * - std::vector<CTxOut> vout
 * - uint32_t nLockTime
 *
 * Extended transaction serialization format:
 * - int32_t nVersion
 * - unsigned char dummy = 0x00
 * - unsigned char flags (!= 0)
 * - std::vector<CTxIn> vin
 * - std::vector<CTxOut> vout
 * - if (flags & 1):
 *   - CTxWitness wit;
 * - uint32_t nLockTime
 */
template<typename Stream, typename Operation, typename TxType>
inline void SerializeTransaction(TxType& tx, Stream& s, Operation ser_action, int nType, int nVersion) {
    READWRITE(*const_cast<int32_t*>(&tx.nVersion));
    READWRITE(*const_cast<unsigned int*>(&tx.nTime));
    unsigned char flags = 0;
    if (ser_action.ForRead()) {
        const_cast<std::vector<CTxIn>*>(&tx.vin)->clear();
        const_cast<std::vector<CTxOut>*>(&tx.vout)->clear();
        const_cast<CTxWitness*>(&tx.wit)->SetNull();
        /* Try to read the vin. In case the dummy is there, this will be read as an empty vector. */
        READWRITE(*const_cast<std::vector<CTxIn>*>(&tx.vin));
        if (tx.vin.size() == 0 && !(nVersion & SERIALIZE_TRANSACTION_NO_WITNESS)) {
            /* We read a dummy or an empty vin. */
            READWRITE(flags);
            if (flags != 0) {
                READWRITE(*const_cast<std::vector<CTxIn>*>(&tx.vin));
                READWRITE(*const_cast<std::vector<CTxOut>*>(&tx.vout));
            }
        } else {
            /* We read a non-empty vin. Assume a normal vout follows. */
            READWRITE(*const_cast<std::vector<CTxOut>*>(&tx.vout));
        }
        if ((flags & 1) && !(nVersion & SERIALIZE_TRANSACTION_NO_WITNESS)) {
            /* The witness flag is present, and we support witnesses. */
            flags ^= 1;
            const_cast<CTxWitness*>(&tx.wit)->vtxinwit.resize(tx.vin.size());
            READWRITE(tx.wit);
        }
        if (flags) {
            /* Unknown flag in the serialization */
            throw std::ios_base::failure("Unknown transaction optional data");
        }
    } else {
        // Consistency check
        assert(tx.wit.vtxinwit.size() <= tx.vin.size());
        if (!(nVersion & SERIALIZE_TRANSACTION_NO_WITNESS)) {
            /* Check whether witnesses need to be serialized. */
            if (!tx.wit.IsNull()) {
                flags |= 1;
            }
        }
        if (flags) {
            /* Use extended format in case witnesses are to be serialized. */
            std::vector<CTxIn> vinDummy;
            READWRITE(vinDummy);
            READWRITE(flags);
        }
        READWRITE(*const_cast<std::vector<CTxIn>*>(&tx.vin));
        READWRITE(*const_cast<std::vector<CTxOut>*>(&tx.vout));
        if (flags & 1) {
            const_cast<CTxWitness*>(&tx.wit)->vtxinwit.resize(tx.vin.size());
            READWRITE(tx.wit);
        }
    }
    READWRITE(*const_cast<uint32_t*>(&tx.nLockTime));
    if(tx.nVersion >= 2) {
        READWRITE(*const_cast<std::string*>(&tx.strDZeel)); }
    if (tx.nVersion & TX_BLS_CT_FLAG || tx.nVersion & TX_BLS_INPUT_FLAG)
        READWRITE(*const_cast<std::vector<unsigned char>*>(&tx.vchBalanceSig));
    if (tx.nVersion & TX_BLS_INPUT_FLAG)
        READWRITE(*const_cast<std::vector<unsigned char>*>(&tx.vchTxSig));
}

/** The basic transaction that is broadcasted on the network and contained in
 * blocks.  A transaction can contain multiple inputs and outputs.
 */
class CTransaction
{
private:
    /** Memory only. */

public:
    // Default transaction version.
    static const int32_t CURRENT_VERSION=1;

    static const int32_t TXDZEEL_VERSION=2;
    static const int32_t TXDZEEL_VERSION_V2=3;
    static const int32_t PROPOSAL_VERSION=4;
    static const int32_t PAYMENT_REQUEST_VERSION=5;
    static const int32_t CONSULTATION_VERSION=6;
    static const int32_t ANSWER_VERSION=7;
    static const int32_t VOTE_VERSION=8;

    // Changing the default transaction version requires a two step process: first
    // adapting relay policy by bumping MAX_STANDARD_VERSION, and then later date
    // bumping the default CURRENT_VERSION at which point both CURRENT_VERSION and
    // MAX_STANDARD_VERSION will be equal.
    static const int32_t MAX_STANDARD_VERSION=255;

    // The local variables are made const to prevent unintended modification
    // without updating the cached hash value. However, CTransaction is not
    // actually immutable; deserialization and assignment are implemented,
    // and bypass the constness. This is safe, as they update the entire
    // structure, including the hash.
    const int32_t nVersion;
    unsigned int nTime;
    std::vector<CTxIn> vin;
    std::vector<CTxOut> vout;
    CTxWitness wit; // Not const: can change without invalidating the txid cache
    const uint32_t nLockTime;
    std::string strDZeel;
    const uint256 hash;
    std::vector<unsigned char> vchTxSig;
    std::vector<unsigned char> vchBalanceSig;

    /** Construct a CTransaction that qualifies as IsNull() */
    CTransaction();

    /** Convert a CMutableTransaction into a CTransaction. */
    CTransaction(const CMutableTransaction &tx);

    CTransaction& operator=(const CTransaction& tx);

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        SerializeTransaction(*this, s, ser_action, nType, nVersion);
        if (ser_action.ForRead()) {
            UpdateHash();
        }
    }

    bool IsNull() const {
        return vin.empty() && vout.empty();
    }

    const uint256& GetHash() const {
        return hash;
    }

    // Compute a hash that includes both transaction and witness data
    uint256 GetWitnessHash() const;

    // Return sum of txouts.
    CAmount GetValueOut() const;
    // GetValueIn() is a method on CStateViewCache, because
    // inputs must be known to compute value in.

    CAmount GetValueOutCFund() const;

    // Compute priority, given priority of inputs and (optionally) tx size
    double ComputePriority(double dPriorityInputs, unsigned int nTxSize=0) const;

    // Compute modified tx size for priority calculation (optionally given tx size)
    unsigned int CalculateModifiedSize(unsigned int nTxSize=0) const;

    bool IsBLSInput() const
    {
        return nVersion & TX_BLS_INPUT_FLAG;
    }

    bool IsCTOutput() const
    {
        return nVersion & TX_BLS_CT_FLAG;
    }

    bool IsBLSCT() const
    {
        return IsCTOutput() || IsBLSInput();
    }

    bool HasCTOutput() const
    {
        for (auto &out: vout)
        {
            if (out.HasRangeProof())
            {
                return true;
            }
        }
        return false;
    }

    CAmount GetFee() const
    {
        CAmount ret = 0;
        for(const CTxOut& txout : vout) {
            if (txout.scriptPubKey.IsFee())
                ret += txout.nValue;
        }
        return ret;
    }

    bool IsCoinBase() const
    {
        return (vin.size() == 1 && vin[0].prevout.IsNull());
    }

    bool IsCoinStake() const
    {
        // ppcoin: the coin stake transaction is marked with the first output empty
        return (vin.size() > 0 && (!vin[0].prevout.IsNull()) && vout.size() >= 2 && vout[0].IsEmpty());
    }

    friend bool operator==(const CTransaction& a, const CTransaction& b)
    {
        return a.hash == b.hash;
    }

    friend bool operator!=(const CTransaction& a, const CTransaction& b)
    {
        return a.hash != b.hash;
    }

    friend bool operator<(const CTransaction& a, const CTransaction& b)
    {
        return a.hash < b.hash;
    }

    std::string ToString() const;

    void UpdateHash() const;

    friend CWalletTx;
    friend CMerkleTx;
};

/** A mutable version of CTransaction. */
struct CMutableTransaction
{
    int32_t nVersion;
    unsigned int nTime;
    std::vector<CTxIn> vin;
    std::vector<CTxOut> vout;
    CTxWitness wit;
    uint32_t nLockTime;
    std::string strDZeel;
    std::vector<unsigned char> vchTxSig;
    std::vector<unsigned char> vchBalanceSig;

    CMutableTransaction();
    CMutableTransaction(const CTransaction& tx);

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        SerializeTransaction(*this, s, ser_action, nType, nVersion);
    }

    bool IsCoinBase() const
    {
        return (vin.size() == 1 && vin[0].prevout.IsNull());
    }

    bool IsCoinStake() const
    {
        // ppcoin: the coin stake transaction is marked with the first output empty
        return (vin.size() > 0 && (!vin[0].prevout.IsNull()) && vout.size() >= 2 && vout[0].IsEmpty());
    }

    void SetBalanceSignature(const bls::G2Element& sig);
    void SetTxSignature(const bls::G2Element& sig);

    /** Compute the hash of this CMutableTransaction. This is computed on the
     * fly, as opposed to GetHash() in CTransaction, which uses a cached result.
     */
    uint256 GetHash() const;
    std::string ToString() const;
};

/** Compute the weight of a transaction, as defined by BIP 141 */
int64_t GetTransactionWeight(const CTransaction &tx);

#endif // NAVCOIN_PRIMITIVES_TRANSACTION_H
