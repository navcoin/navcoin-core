// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef NAVCOIN_COINS_H
#define NAVCOIN_COINS_H

#include <compressor.h>
#include <core_memusage.h>
#include <hash.h>
#include <memusage.h>
#include <serialize.h>
#include <uint256.h>

#include <consensus/dao.h>
#include <ctokens/ctokens.h>
#include <ctokens/tokenutxos.h>
#include <dotnav/namerecord.h>
#include <dotnav/namedata.h>

#include <assert.h>
#include <stdint.h>

#include <boost/unordered_map.hpp>

class CConsultation;
class CPaymentRequest;
class CProposal;
class CVoteList;
class CConsensusParameter;

/**
 * Pruned version of CTransaction: only retains metadata and unspent transaction outputs
 *
 * Serialized format:
 * - VARINT(nVersion)
 * - VARINT(nCode)
 * - unspentness bitvector, for vout[2] and further; least significant byte first
 * - the non-spent CTxOuts (via CTxOutCompressor)
 * - VARINT(nHeight)
 *
 * The nCode value consists of:
 * - bit 0: IsCoinBase()
 * - bit 1: vout[0] is not spent
 * - bit 2: vout[1] is not spent
 * - The higher bits encode N, the number of non-zero bytes in the following bitvector.
 *   - In case both bit 1 and bit 2 are unset, they encode N-1, as there must be at
 *     least one non-spent output).
 *
 * Example: 0104835800816115944e077fe7c803cfa57f29b36bf87c1d358bb85e
 *          <><><--------------------------------------------><---->
 *          |  \                  |                             /
 *    version   code             vout[1]                  height
 *
 *    - version = 1
 *    - code = 4 (vout[1] is not spent, and 0 non-zero bytes of bitvector follow)
 *    - unspentness bitvector: as 0 non-zero bytes follow, it has length 0
 *    - vout[1]: 835800816115944e077fe7c803cfa57f29b36bf87c1d35
 *               * 8358: compact amount representation for 60000000000 (600 NAV)
 *               * 00: special txout type pay-to-pubkey-hash
 *               * 816115944e077fe7c803cfa57f29b36bf87c1d35: address uint160
 *    - height = 203998
 *
 *
 * Example: 0109044086ef97d5790061b01caab50f1b8e9c50a5057eb43c2d9563a4eebbd123008c988f1a4a4de2161e0f50aac7f17e7f9555caa486af3b
 *          <><><--><--------------------------------------------------><----------------------------------------------><---->
 *         /  \   \                     |                                                           |                     /
 *  version  code  unspentness       vout[4]                                                     vout[16]           height
 *
 *  - version = 1
 *  - code = 9 (coinbase, neither vout[0] or vout[1] are unspent,
 *                2 (1, +1 because both bit 1 and bit 2 are unset) non-zero bitvector bytes follow)
 *  - unspentness bitvector: bits 2 (0x04) and 14 (0x4000) are set, so vout[2+2] and vout[14+2] are unspent
 *  - vout[4]: 86ef97d5790061b01caab50f1b8e9c50a5057eb43c2d9563a4ee
 *             * 86ef97d579: compact amount representation for 234925952 (2.35 NAV)
 *             * 00: special txout type pay-to-pubkey-hash
 *             * 61b01caab50f1b8e9c50a5057eb43c2d9563a4ee: address uint160
 *  - vout[16]: bbd123008c988f1a4a4de2161e0f50aac7f17e7f9555caa4
 *              * bbd123: compact amount representation for 110397 (0.001 NAV)
 *              * 00: special txout type pay-to-pubkey-hash
 *              * 8c988f1a4a4de2161e0f50aac7f17e7f9555caa4: address uint160
 *  - height = 120891
 */
class CCoins
{
public:
    //! whether transaction is a coinbase
    bool fCoinBase;
    bool fCoinStake;

    //! unspent transaction outputs; spent outputs are .IsNull(); spent outputs at the end of the array are dropped
    std::vector<CTxOut> vout;

    //! at which height this transaction was included in the active block chain
    int nHeight;

    //! version of the CTransaction; accesses to this value should probably check for nHeight as well,
    //! as new tx version will probably only be introduced at certain heights
    int nVersion;

    void FromTx(const CTransaction &tx, int nHeightIn) {
        fCoinBase = tx.IsCoinBase();
        fCoinStake = tx.IsCoinStake();
        vout = tx.vout;
        nHeight = nHeightIn;
        nVersion = tx.nVersion;
        ClearUnspendable();
    }

    //! construct a CCoins from a CTransaction, at a given height
    CCoins(const CTransaction &tx, int nHeightIn) {
        FromTx(tx, nHeightIn);
    }

    void Clear() {
        fCoinBase = false;
        fCoinStake = false;
        std::vector<CTxOut>().swap(vout);
        nHeight = 0;
        nVersion = 0;
    }

    //! empty constructor
    CCoins() : fCoinBase(false), fCoinStake(false), vout(0), nHeight(0), nVersion(0) { }

    //!remove spent outputs at the end of vout
    void Cleanup() {
        while (vout.size() > 0 && vout.back().IsNull())
            vout.pop_back();
        if (vout.empty())
            std::vector<CTxOut>().swap(vout);
    }

    void ClearUnspendable() {
        for(CTxOut &txout: vout) {
            if (!txout.HasRangeProof() && txout.scriptPubKey.IsUnspendable())
                txout.SetNull();
        }
        Cleanup();
    }

    void swap(CCoins &to) {
        std::swap(to.fCoinBase, fCoinBase);
        std::swap(to.fCoinStake, fCoinStake);
        to.vout.swap(vout);
        std::swap(to.nHeight, nHeight);
        std::swap(to.nVersion, nVersion);
    }

    //! equality test
    friend bool operator==(const CCoins &a, const CCoins &b) {
         // Empty CCoins objects are always equal.
         if (a.IsPruned() && b.IsPruned())
             return true;

         return a.fCoinBase == b.fCoinBase &&
                a.nHeight == b.nHeight &&
                a.nVersion == b.nVersion &&
                a.vout == b.vout;
    }
    friend bool operator!=(const CCoins &a, const CCoins &b) {
        return !(a == b);
    }

    void CalcMaskSize(unsigned int &nBytes, unsigned int &nNonzeroBytes) const;

    bool IsCoinBase() const {
        return fCoinBase;
    }

    bool IsCoinStake() const {
        return fCoinStake;
    }

    unsigned int GetSerializeSize(int nType, int nVersion) const {
        unsigned int nSize = 0;
        unsigned int nMaskSize = 0, nMaskCode = 0;
        CalcMaskSize(nMaskSize, nMaskCode);
        bool fFirst = vout.size() > 0 && !vout[0].IsNull();
        bool fSecond = vout.size() > 1 && !vout[1].IsNull();
        assert(fFirst || fSecond || nMaskCode);
        unsigned int nCode = 8*(nMaskCode - (fFirst || fSecond ? 0 : 1)) + (fCoinBase ? 1 : 0) + (fFirst ? 2 : 0) + (fSecond ? 4 : 0);
        // version
        nSize += ::GetSerializeSize(VARINT(this->nVersion), nType, nVersion);
        // size of header code
        nSize += ::GetSerializeSize(VARINT(nCode), nType, nVersion);
        // spentness bitmask
        nSize += nMaskSize;
        // txouts themself
        for (unsigned int i = 0; i < vout.size(); i++)
            if (!vout[i].IsNull())
                nSize += ::GetSerializeSize(CTxOutCompressor(REF(vout[i])), nType, nVersion);
        // height
        nSize += ::GetSerializeSize(VARINT(nHeight), nType, nVersion);
        return nSize;
    }

    template<typename Stream>
    void Serialize(Stream &s, int nType, int nVersion) const {
        unsigned int nMaskSize = 0, nMaskCode = 0;
        CalcMaskSize(nMaskSize, nMaskCode);
        bool fFirst = vout.size() > 0 && !vout[0].IsNull();
        bool fSecond = vout.size() > 1 && !vout[1].IsNull();
        assert(fFirst || fSecond || nMaskCode);
        unsigned int nCode = 8*(nMaskCode - (fFirst || fSecond ? 0 : 1)) + (fCoinBase ? 1 : 0) + (fFirst ? 2 : 0) + (fSecond ? 4 : 0);
        // version
        ::Serialize(s, VARINT(this->nVersion), nType, nVersion);
        // header code
        ::Serialize(s, VARINT(nCode), nType, nVersion);
        // spentness bitmask
        for (unsigned int b = 0; b<nMaskSize; b++) {
            unsigned char chAvail = 0;
            for (unsigned int i = 0; i < 8 && 2+b*8+i < vout.size(); i++)
                if (!vout[2+b*8+i].IsNull())
                    chAvail |= (1 << i);
            ::Serialize(s, chAvail, nType, nVersion);
        }
        // txouts themself
        for (unsigned int i = 0; i < vout.size(); i++) {
            if (!vout[i].IsNull())
                ::Serialize(s, CTxOutCompressor(REF(vout[i])), nType, nVersion);
        }
        // coinbase height
        ::Serialize(s, VARINT(nHeight), nType, nVersion);
    }

    template<typename Stream>
    void Unserialize(Stream &s, int nType, int nVersion) {
        unsigned int nCode = 0;
        // version
        ::Unserialize(s, VARINT(this->nVersion), nType, nVersion);
        // header code
        ::Unserialize(s, VARINT(nCode), nType, nVersion);
        fCoinBase = nCode & 1;
        std::vector<bool> vAvail(2, false);
        vAvail[0] = (nCode & 2) != 0;
        vAvail[1] = (nCode & 4) != 0;
        unsigned int nMaskCode = (nCode / 8) + ((nCode & 6) != 0 ? 0 : 1);
        // spentness bitmask
        while (nMaskCode > 0) {
            unsigned char chAvail = 0;
            ::Unserialize(s, chAvail, nType, nVersion);
            for (unsigned int p = 0; p < 8; p++) {
                bool f = (chAvail & (1 << p)) != 0;
                vAvail.push_back(f);
            }
            if (chAvail != 0)
                nMaskCode--;
        }
        // txouts themself
        vout.assign(vAvail.size(), CTxOut());
        for (unsigned int i = 0; i < vAvail.size(); i++) {
            if (vAvail[i])
                ::Unserialize(s, REF(CTxOutCompressor(vout[i])), nType, nVersion);
        }
        // coinbase height
        ::Unserialize(s, VARINT(nHeight), nType, nVersion);
        Cleanup();
    }

    //! mark a vout spent
    bool Spend(uint32_t nPos);

    //! check whether a particular output is still available
    bool IsAvailable(unsigned int nPos) const {
        return (nPos < vout.size() && !vout[nPos].IsNull());
    }

    //! check whether the entire CCoins is spent
    //! note that only !IsPruned() CCoins can be serialized
    bool IsPruned() const {
        for(const CTxOut &out: vout)
            if (!out.IsNull())
                return false;
        return true;
    }

    size_t DynamicMemoryUsage() const {
        size_t ret = memusage::DynamicUsage(vout);
        for(const CTxOut &out: vout) {
            ret += RecursiveDynamicUsage(out.scriptPubKey);
        }
        return ret;
    }
};

class SaltedTxidHasher
{
private:
    /** Salt */
    const uint64_t k0, k1;

public:
    SaltedTxidHasher();

    /**
     * This *must* return size_t. With Boost 1.46 on 32-bit systems the
     * unordered_map will behave unpredictably if the custom hasher returns a
     * uint64_t, resulting in failures when syncing the chain (#4634).
     */
    size_t operator()(const uint256& txid) const {
        return SipHashUint256(k0, k1, txid);
    }
};

template <typename T>
struct CCacheEntry
{
    T coins; // The actual cached data.
    unsigned char flags;

    enum Flags {
        DIRTY = (1 << 0), // This cache entry is potentially different from the version in the parent view.
        FRESH = (1 << 1), // The parent view does not have this entry (or it is pruned).
    };

    CCacheEntry() : coins(), flags(0) {}
};

typedef CCacheEntry<CCoins> CCoinsCacheEntry;
typedef boost::unordered_map<uint256, CCoinsCacheEntry, SaltedTxidHasher> CCoinsMap;
typedef std::vector<unsigned char> CVoteMapKey;
typedef CVoteList CVoteMapValue;
typedef std::map<CVoteMapKey, CVoteMapValue> CVoteMap;
typedef std::map<uint256, CProposal> CProposalMap;
typedef std::map<uint256, CPaymentRequest> CPaymentRequestMap;
typedef std::map<uint256, CConsultation> CConsultationMap;
typedef std::map<uint256, CConsultationAnswer> CConsultationAnswerMap;
typedef std::map<int, CConsensusParameter> CConsensusParameterMap;

/** Cursor for iterating over CoinsView state */
class CStateViewCursor
{
public:
    CStateViewCursor(const uint256 &hashBlockIn): hashBlock(hashBlockIn) {}
    virtual ~CStateViewCursor();

    virtual bool GetKey(uint256 &key) const = 0;
    virtual bool GetValue(CCoins &coins) const = 0;
    /* Don't care about GetKeySize here */
    virtual unsigned int GetValueSize() const = 0;

    virtual bool Valid() const = 0;
    virtual void Next() = 0;

    //! Get best block at the time this cursor was created
    const uint256 &GetBestBlock() const { return hashBlock; }
private:
    uint256 hashBlock;
};

/** Abstract view on the open txout dataset. */
class CStateView
{
public:
    //! Retrieve the CCoins (unspent transaction outputs) for a given txid
    virtual bool GetCoins(const uint256 &txid, CCoins &coins) const;

    //! Just check whether we have data for a given txid.
    //! This may (but cannot always) return true for fully spent transactions
    virtual bool HaveCoins(const uint256 &txid) const;

    //! Community Fund
    virtual bool GetProposal(const uint256 &pid, CProposal &proposal) const;
    virtual bool GetAllProposals(CProposalMap& map);
    virtual bool GetPaymentRequest(const uint256 &prid, CPaymentRequest &prequest) const;
    virtual bool GetAllPaymentRequests(CPaymentRequestMap& map);
    virtual bool HaveProposal(const uint256 &pid) const;
    virtual bool HavePaymentRequest(const uint256 &prid) const;

    //! Dao
    virtual bool GetCachedVoter(const CVoteMapKey &voter, CVoteMapValue& vote) const;
    virtual bool HaveCachedVoter(const CVoteMapKey &voter) const;
    virtual bool GetAllVotes(CVoteMap& map);

    virtual bool GetConsultation(const uint256 &cid, CConsultation &consultation) const;
    virtual bool GetConsultationAnswer(const uint256 &cid, CConsultationAnswer &answer) const;
    virtual bool GetAllConsultations(CConsultationMap& map);
    virtual bool HaveConsultation(const uint256 &cid) const;
    virtual bool HaveConsultationAnswer(const uint256 &cid) const;
    virtual bool GetAllConsultationAnswers(CConsultationAnswerMap& map);

    virtual bool GetConsensusParameter(const int &pid, CConsensusParameter& cparameter) const;
    virtual bool HaveConsensusParameter(const int &pid) const;

    virtual bool GetToken(const uint256 &id, TokenInfo& token) const;
    virtual bool GetTokenUtxos(const TokenId &id, TokenUtxoValues &tokenUtxos) const;
    virtual bool GetAllTokens(TokenMap& map);
    virtual bool HaveToken(const uint256 &id) const;
    virtual bool HaveTokenUtxo(const TokenId &id) const;

    virtual bool GetNameRecord(const uint256 &id, NameRecordValue& height) const;
    virtual bool GetAllNameRecords(NameRecordMap& map);
    virtual bool HaveNameRecord(const uint256 &id) const;

    virtual bool GetNameData(const uint256& id, NameDataValues& data);
    virtual bool HaveNameData(const uint256& id) const;

    virtual int GetExcludeVotes() const;
    virtual bool SetExcludeVotes(int count);

    //! Retrieve the block hash whose state this CStateView currently represents
    virtual uint256 GetBestBlock() const;

    //! Do a bulk modification (multiple CCoins changes + BestBlock change).
    //! The passed mapCoins can be modified.
    virtual bool BatchWrite(CCoinsMap &mapCoins, CProposalMap &mapProposals,
                            CPaymentRequestMap &mapPaymentRequests, CVoteMap &mapVotes,
                            CConsultationMap &mapConsultations, CConsultationAnswerMap &mapAnswers,
                            CConsensusParameterMap& mapConsensus, TokenMap& mapTokens, TokenUtxoMap mapTokenUtxos,
                            NameRecordMap& mapNameRecords, NameDataMap& mapNameData,
                            const uint256 &hashBlock, const int &nCacheExcludeVotes);

    //! Get a cursor to iterate over the whole state
    virtual CStateViewCursor *Cursor() const;

    //! As we use CStateViews polymorphically, have a virtual destructor
    virtual ~CStateView() {}
};


/** CStateView backed by another CStateView */
class CStateViewBacked : public CStateView
{
protected:
    CStateView *base;

public:
    CStateViewBacked(CStateView *viewIn);
    bool GetCoins(const uint256 &txid, CCoins &coins) const;
    bool HaveCoins(const uint256 &txid) const;
    bool GetProposal(const uint256 &txid, CProposal &proposal) const;
    bool GetAllProposals(CProposalMap& map);
    bool GetPaymentRequest(const uint256 &txid, CPaymentRequest &prequest) const;
    bool GetAllPaymentRequests(CPaymentRequestMap& map);
    bool HaveProposal(const uint256 &pid) const;
    bool HavePaymentRequest(const uint256 &prid) const;
    bool HaveCachedVoter(const CVoteMapKey &voter) const;
    bool GetCachedVoter(const CVoteMapKey &voter, CVoteMapValue& vote) const;
    bool GetAllVotes(CVoteMap& map);
    bool GetConsultation(const uint256 &cid, CConsultation &consultation) const;
    bool GetConsultationAnswer(const uint256 &cid, CConsultationAnswer &answer) const;
    bool GetAllConsultations(CConsultationMap& map);
    bool HaveConsultation(const uint256 &cid) const;
    bool HaveConsultationAnswer(const uint256 &cid) const;
    bool GetAllConsultationAnswers(CConsultationAnswerMap& map);
    bool GetConsensusParameter(const int &pid, CConsensusParameter& cparameter) const;
    bool HaveConsensusParameter(const int &pid) const;
    bool GetToken(const uint256 &id, TokenInfo& token) const;
    bool GetTokenUtxos(const TokenId &id, TokenUtxoValues &tokenUtxos) const;
    bool GetAllTokens(TokenMap& map);
    bool HaveToken(const uint256 &id) const;
    bool HaveTokenUtxo(const TokenId &id) const;

    bool GetNameRecord(const uint256 &id, NameRecordValue& height) const;
    bool GetAllNameRecords(NameRecordMap& map);
    bool HaveNameRecord(const uint256 &id) const;

    bool GetNameData(const uint256 &id, NameDataValues& data);
    bool HaveNameData(const uint256 &id) const;

    int GetExcludeVotes() const;
    bool SetExcludeVotes(int count);

    uint256 GetBestBlock() const;
    void SetBackend(CStateView &viewIn);
    bool BatchWrite(CCoinsMap &mapCoins, CProposalMap &mapProposals,
                    CPaymentRequestMap &mapPaymentRequests, CVoteMap &mapVotes,
                    CConsultationMap &mapConsultations, CConsultationAnswerMap &mapAnswers,
                    CConsensusParameterMap& mapConsensus, TokenMap& mapTokens, TokenUtxoMap mapTokenUtxos,
                    NameRecordMap& mapNameRecords, NameDataMap& mapNameData,
                    const uint256 &hashBlock, const int &nCacheExcludeVotes);
    CStateViewCursor *Cursor() const;
};


class CStateViewCache;

/**
 * A reference to a mutable cache entry. Encapsulating it allows us to run
 *  cleanup code after the modification is finished, and keeping track of
 *  concurrent modifications.
 */
class CCoinsModifier
{
private:
    CStateViewCache& cache;
    CCoinsMap::iterator it;
    size_t cachedCoinUsage; // Cached memory usage of the CCoins object before modification
    CCoinsModifier(CStateViewCache& cache_, CCoinsMap::iterator it_, size_t usage);

public:
    CCoins* operator->() { return &it->second.coins; }
    CCoins& operator*() { return it->second.coins; }
    ~CCoinsModifier();
    friend class CStateViewCache;
};

class CProposalModifier
{
private:
    CStateViewCache& cache;
    CProposalMap::iterator it;
    CProposalModifier(CStateViewCache& cache_, CProposalMap::iterator it_, int height=0);
    CProposal prev;
    int height;

public:
    CProposal* operator->() { return &it->second; }
    CProposal& operator*() { return it->second; }
    ~CProposalModifier();
    friend class CStateViewCache;
};


class CPaymentRequestModifier
{
private:
    CStateViewCache& cache;
    CPaymentRequestMap::iterator it;
    CPaymentRequestModifier(CStateViewCache& cache_, CPaymentRequestMap::iterator it_, int height=0);
    CPaymentRequest prev;
    int height;

public:
    CPaymentRequest* operator->() { return &it->second; }
    CPaymentRequest& operator*() { return it->second; }
    ~CPaymentRequestModifier();
    friend class CStateViewCache;
};

class CVoteModifier
{
private:
    CStateViewCache& cache;
    CVoteMap::iterator it;
    CVoteModifier(CStateViewCache& cache_, CVoteMap::iterator it_, int height=0);
    CVoteList prev;
    int height;

public:
    CVoteMapValue* operator->() { return &it->second; }
    CVoteMapValue& operator*() { return it->second; }
    ~CVoteModifier();
    friend class CStateViewCache;
};

class CConsultationModifier
{
private:
    CStateViewCache& cache;
    CConsultationMap::iterator it;
    CConsultationModifier(CStateViewCache& cache_, CConsultationMap::iterator it_, int height=0);
    CConsultation prev;
    int height;

public:
    CConsultation* operator->() { return &it->second; }
    CConsultation& operator*() { return it->second; }
    ~CConsultationModifier();
    friend class CStateViewCache;
};

class CConsensusParameterModifier
{
private:
    CStateViewCache& cache;
    CConsensusParameterMap::iterator it;
    CConsensusParameterModifier(CStateViewCache& cache_, CConsensusParameterMap::iterator it_, int height=0);
    CConsensusParameter prev;
    int height;

public:
    CConsensusParameter* operator->() { return &it->second; }
    CConsensusParameter& operator*() { return it->second; }
    ~CConsensusParameterModifier();
    friend class CStateViewCache;
};

class TokenModifier
{
private:
    CStateViewCache& cache;
    TokenMap::iterator it;
    TokenModifier(CStateViewCache& cache_, TokenMap::iterator it_, int height=0);
    TokenInfo prev;
    int height;

public:
    TokenInfo* operator->() { return &it->second; }
    TokenInfo& operator*() { return it->second; }
    ~TokenModifier();
    friend class CStateViewCache;
};

class TokenUtxosModifier
{
private:
    CStateViewCache& cache;
    TokenUtxoMap::iterator it;
    TokenUtxosModifier(CStateViewCache& cache_, TokenUtxoMap::iterator it_, int blockHeight=0);
    TokenUtxoValues prev;
    int blockHeight;

public:
    TokenUtxoValues* operator->() { return &it->second; }
    TokenUtxoValues& operator*() { return it->second; }
    ~TokenUtxosModifier();
    friend class CStateViewCache;
};

class NameRecordModifier
{
private:
    CStateViewCache& cache;
    NameRecordMap::iterator it;
    NameRecordModifier(CStateViewCache& cache_, NameRecordMap::iterator it_, int height=0);
    NameRecordValue prev;
    int height;

public:
    NameRecordValue* operator->() { return &it->second; }
    NameRecordValue& operator*() { return it->second; }
    ~NameRecordModifier();
    friend class CStateViewCache;
};

class NameDataModifier
{
private:
    CStateViewCache& cache;
    NameDataMap::iterator it;
    NameDataModifier(CStateViewCache& cache_, NameDataMap::iterator it_, int height=0);
    NameDataValues prev;
    int height;

public:
    NameDataValues* operator->() { return &it->second; }
    NameDataValues& operator*() { return it->second; }
    ~NameDataModifier();
    friend class CStateViewCache;
};

class CConsultationAnswerModifier
{
private:
    CStateViewCache& cache;
    CConsultationAnswerMap::iterator it;
    CConsultationAnswerModifier(CStateViewCache& cache_, CConsultationAnswerMap::iterator it_, int height=0);
    CConsultationAnswer prev;
    int height;

public:
    CConsultationAnswer* operator->() { return &it->second; }
    CConsultationAnswer& operator*() { return it->second; }
    ~CConsultationAnswerModifier();
    friend class CStateViewCache;
};

/** CStateView that adds a memory cache for transactions to another CStateView */
class CStateViewCache : public CStateViewBacked
{
protected:
    /* Whether this cache has an active modifier. */
    bool hasModifier;
    bool hasModifierConsensus;

    /**
     * Make mutable so that we can "fill the cache" even from Get-methods
     * declared as "const".
     */
    mutable uint256 hashBlock;
    mutable CCoinsMap cacheCoins;
    mutable CProposalMap cacheProposals;
    mutable TokenUtxoMap cacheTokenUtxos;
    mutable CPaymentRequestMap cachePaymentRequests;
    mutable CVoteMap cacheVotes;
    mutable CConsultationMap cacheConsultations;
    mutable CConsultationAnswerMap cacheAnswers;
    mutable CConsensusParameterMap cacheConsensus;
    mutable TokenMap cacheTokens;
    mutable NameRecordMap cacheNameRecords;
    mutable NameDataMap cacheNameData;
    mutable int nCacheExcludeVotes;

    /* Cached dynamic memory usage for the inner CCoins objects. */
    mutable size_t cachedCoinsUsage;

public:
    CStateViewCache(CStateView *baseIn);
    ~CStateViewCache();

    // Standard CStateView methods
    bool GetCoins(const uint256 &txid, CCoins &coins) const;
    bool HaveCoins(const uint256 &txid) const;
    bool HaveProposal(const uint256 &pid) const;
    bool HavePaymentRequest(const uint256 &prid) const;
    bool HaveCachedVoter(const CVoteMapKey &voter) const;
    bool HaveConsultation(const uint256 &cid) const;
    bool HaveConsultationAnswer(const uint256 &cid) const;
    bool HaveConsensusParameter(const int& pid) const;
    bool HaveToken(const uint256& id) const;
    bool HaveTokenUtxo(const TokenId& id) const;
    bool HaveNameRecord(const uint256& id) const;
    bool HaveNameData(const uint256& id) const;
    bool GetProposal(const uint256 &txid, CProposal &proposal) const;
    bool GetPaymentRequest(const uint256 &txid, CPaymentRequest &prequest) const;
    bool GetCachedVoter(const CVoteMapKey &voter, CVoteMapValue& vote) const;
    bool GetConsultation(const uint256 &cid, CConsultation& consultation) const;
    bool GetConsultationAnswer(const uint256 &cid, CConsultationAnswer& answer) const;
    bool GetConsensusParameter(const int& pid, CConsensusParameter& cparameter) const;
    bool GetToken(const uint256& pid, TokenInfo& token) const;
    bool GetTokenUtxos(const TokenId &id, TokenUtxoValues &tokenUtxos) const;
    bool GetNameRecord(const uint256& pid, NameRecordValue& height) const;
    bool GetNameData(const uint256& pid, NameDataValues& data);
    bool GetAllProposals(CProposalMap& map);
    bool GetAllPaymentRequests(CPaymentRequestMap& map);
    bool GetAllVotes(CVoteMap& map);
    bool GetAllConsultations(CConsultationMap& map);
    bool GetAllConsultationAnswers(CConsultationAnswerMap& map);
    bool GetAllTokens(TokenMap& map);
    bool GetAllNameRecords(NameRecordMap& map);
    bool GetAnswersForConsultation(CConsultationAnswerMap& map, const uint256& parent);
    uint256 GetBestBlock() const;
    void SetBestBlock(const uint256 &hashBlock);
    bool BatchWrite(CCoinsMap &mapCoins, CProposalMap &mapProposals,
                    CPaymentRequestMap &mapPaymentRequests, CVoteMap &mapVotes,
                    CConsultationMap &mapConsultations, CConsultationAnswerMap &mapAnswers,
                    CConsensusParameterMap& mapConsensus, TokenMap& mapTokens, TokenUtxoMap mapTokenUtxos,
                    NameRecordMap& mapNameRecords, NameDataMap& mapNameData,
                    const uint256 &hashBlockIn, const int &nCacheExcludeVotes);
    bool AddProposal(const CProposal& proposal) const;
    bool AddPaymentRequest(const CPaymentRequest& prequest) const;
    bool AddCachedVoter(const CVoteMapKey &voter, CVoteMapValue& vote) const;
    bool AddConsultation(const CConsultation& consultation) const;
    bool AddToken(const Token& token) const;
    bool AddTokenUtxo(const TokenId& id, const TokenUtxoEntry& utxo) const;
    bool AddNameRecord(const NameRecord& record) const;
    bool AddNameData(const uint256& id, const NameDataEntry& record) const;
    bool AddConsultationAnswer(const CConsultationAnswer& answer);
    bool RemoveProposal(const uint256 &pid) const;
    bool RemovePaymentRequest(const uint256 &prid) const;
    bool RemoveCachedVoter(const CVoteMapKey &voter) const;
    bool RemoveToken(const uint256 &pid) const;
    bool RemoveTokenUtxo(const TokenUtxoKey &key) const;
    bool RemoveNameRecord(const uint256 &pid) const;
    bool RemoveNameData(const NameDataKey &id) const;
    bool RemoveConsultation(const uint256 &cid);
    bool RemoveConsultationAnswer(const uint256 &cid);

    int GetExcludeVotes() const;
    bool SetExcludeVotes(int count);

    /**
     * Check if we have the given tx already loaded in this cache.
     * The semantics are the same as HaveCoins(), but no calls to
     * the backing CStateView are made.
     */
    bool HaveCoinsInCache(const uint256 &txid) const;
    bool HaveProposalInCache(const uint256 &pid) const;
    bool HavePaymentRequestInCache(const uint256 &prid) const;

    /**
     * Return a pointer to CCoins in the cache, or NULL if not found. This is
     * more efficient than GetCoins. Modifications to other cache entries are
     * allowed while accessing the returned pointer.
     */
    const CCoins* AccessCoins(const uint256 &txid) const;

    /**
     * Return a modifiable reference to a CCoins. If no entry with the given
     * txid exists, a new one is created. Simultaneous modifications are not
     * allowed.
     */
    CCoinsModifier ModifyCoins(const uint256 &txid);

    CProposalModifier ModifyProposal(const uint256 &pid, int nHeight = 0);
    CPaymentRequestModifier ModifyPaymentRequest(const uint256 &prid, int nHeight = 0);
    CVoteModifier ModifyVote(const CVoteMapKey &voter, int nHeight = 0);
    CConsultationModifier ModifyConsultation(const uint256 &cid, int nHeight = 0);
    CConsultationAnswerModifier ModifyConsultationAnswer(const uint256 &cid, int nHeight = 0);
    CConsensusParameterModifier ModifyConsensusParameter(const int &pid, int nHeight = 0);
    TokenModifier ModifyToken(const uint256 &id, int nHeight = 0);
    TokenUtxosModifier ModifyTokenUtxos(const TokenId &id, int blockHeight = 0);
    NameRecordModifier ModifyNameRecord(const uint256 &id, int nHeight = 0);
    NameDataModifier ModifyNameData(const uint256& id, int nHeight = 0);

    /**
     * Return a modifiable reference to a CCoins. Assumes that no entry with the given
     * txid exists and creates a new one. This saves a database access in the case where
     * the coins were to be wiped out by FromTx anyway.  This should not be called with
     * the 2 historical coinbase duplicate pairs because the new coins are marked fresh, and
     * in the event the duplicate coinbase was spent before a flush, the now pruned coins
     * would not properly overwrite the first coinbase of the pair. Simultaneous modifications
     * are not allowed.
     */
    CCoinsModifier ModifyNewCoins(const uint256 &txid, bool coinbase);

    /**
     * Push the modifications applied to this cache to its base.
     * Failure to call this method before destruction will cause the changes to be forgotten.
     * If false is returned, the state of this cache (and its backing view) will be undefined.
     */
    bool Flush();

    /**
     * Removes the transaction with the given hash from the cache, if it is
     * not modified.
     */
    void Uncache(const uint256 &txid);

    //! Calculate the size of the cache (in number of transactions)
    unsigned int GetCacheSize() const;

    //! Calculate the size of the cache (in bytes)
    size_t DynamicMemoryUsage() const;

    /**
     * Amount of navcoins coming in to a transaction
     * Note that lightweight clients may not know anything besides the hash of previous transactions,
     * so may not be able to calculate this.
     *
     * @param[in] tx	transaction for which we are checking input total
     * @return	Sum of value of all inputs (scriptSigs)
     */
    CAmount GetValueIn(const CTransaction& tx) const;

    //! Check whether all prevouts of the transaction are present in the UTXO set represented by this view
    bool HaveInputs(const CTransaction& tx) const;

    /**
     * Return priority of tx at height nHeight. Also calculate the sum of the values of the inputs
     * that are already in the chain.  These are the inputs that will age and increase priority as
     * new blocks are added to the chain.
     */
    double GetPriority(const CTransaction &tx, int nHeight, CAmount &inChainInputValue) const;

    const CTxOut &GetOutputFor(const CTxIn& input) const;

    friend class CCoinsModifier;
    friend class CProposalModifier;
    friend class CPaymentRequestModifier;
    friend class CVoteModifier;
    friend class CConsultationModifier;
    friend class CConsultationAnswerModifier;
    friend class CConsensusParameterModifier;
    friend class TokenModifier;
    friend class TokenUtxosModifier;
    friend class NameRecordModifier;
    friend class NameDataModifier;

private:
    CCoinsMap::iterator FetchCoins(const uint256 &txid);
    CCoinsMap::const_iterator FetchCoins(const uint256 &txid) const;
    CProposalMap::const_iterator FetchProposal(const uint256 &pid) const;
    CPaymentRequestMap::const_iterator FetchPaymentRequest(const uint256 &prid) const;
    CVoteMap::const_iterator FetchVote(const CVoteMapKey &voter) const;
    CConsultationMap::const_iterator FetchConsultation(const uint256 &cid) const;
    CConsultationAnswerMap::const_iterator FetchConsultationAnswer(const uint256 &cid) const;
    CConsensusParameterMap::const_iterator FetchConsensusParameter(const int &pid) const;
    TokenMap::const_iterator FetchToken(const uint256 &id) const;
    TokenUtxoMap::const_iterator FetchTokenUtxos(const TokenId &id) const;
    NameRecordMap::const_iterator FetchNameRecord(const uint256 &id) const;
    NameDataMap::const_iterator FetchNameData(const uint256 &id) const;

    /**
     * By making the copy constructor private, we prevent accidentally using it when one intends to create a cache on top of a base cache.
     */
    CStateViewCache(const CStateViewCache &);
};

#endif // NAVCOIN_COINS_H
