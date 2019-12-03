// Copyright (c) 2012-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <coins.h>

#include <memusage.h>
#include <random.h>
#include <util.h>
#include <utiltime.h>

#include <assert.h>

using namespace CFund;

/**
 * calculate number of bytes for the bitmask, and its number of non-zero bytes
 * each bit in the bitmask represents the availability of one output, but the
 * availabilities of the first two outputs are encoded separately
 */
void CCoins::CalcMaskSize(unsigned int &nBytes, unsigned int &nNonzeroBytes) const {
    unsigned int nLastUsedByte = 0;
    for (unsigned int b = 0; 2+b*8 < vout.size(); b++) {
        bool fZero = true;
        for (unsigned int i = 0; i < 8 && 2+b*8+i < vout.size(); i++) {
            if (!vout[2+b*8+i].IsNull()) {
                fZero = false;
                continue;
            }
        }
        if (!fZero) {
            nLastUsedByte = b + 1;
            nNonzeroBytes++;
        }
    }
    nBytes += nLastUsedByte;
}

bool CCoins::Spend(uint32_t nPos)
{
    if (nPos >= vout.size() || vout[nPos].IsNull())
        return false;
    vout[nPos].SetNull();
    Cleanup();
    return true;
}

bool CCoinsView::GetCoins(const uint256 &txid, CCoins &coins) const { return false; }
bool CCoinsView::GetProposal(const uint256 &pid, CProposal &proposal) const { return false; }
bool CCoinsView::GetAllProposals(CProposalMap& map) { return false; }
bool CCoinsView::GetPaymentRequest(const uint256 &prid, CPaymentRequest &prequest) const { return false; }
bool CCoinsView::GetAllPaymentRequests(CPaymentRequestMap& map) { return false; }
bool CCoinsView::HaveCoins(const uint256 &txid) const { return false; }
bool CCoinsView::HaveProposal(const uint256 &pid) const { return false; }
bool CCoinsView::HavePaymentRequest(const uint256 &prid) const { return false; }
uint256 CCoinsView::GetBestBlock() const { return uint256(); }
bool CCoinsView::BatchWrite(CCoinsMap &mapCoins, CProposalMap &mapProposals,
                            CPaymentRequestMap &mapPaymentRequests, const uint256 &hashBlock) { return false; }
CCoinsViewCursor *CCoinsView::Cursor() const { return 0; }


CCoinsViewBacked::CCoinsViewBacked(CCoinsView *viewIn) : base(viewIn) { }
bool CCoinsViewBacked::GetCoins(const uint256 &txid, CCoins &coins) const { return base->GetCoins(txid, coins); }
bool CCoinsViewBacked::GetProposal(const uint256 &pid, CProposal &proposal) const { return base->GetProposal(pid, proposal); }
bool CCoinsViewBacked::GetAllProposals(CProposalMap& map) { return base->GetAllProposals(map); }
bool CCoinsViewBacked::GetPaymentRequest(const uint256 &prid, CPaymentRequest &prequest) const { return base->GetPaymentRequest(prid, prequest); }
bool CCoinsViewBacked::GetAllPaymentRequests(CPaymentRequestMap& map) { return base->GetAllPaymentRequests(map); }
bool CCoinsViewBacked::HaveCoins(const uint256 &txid) const { return base->HaveCoins(txid); }
bool CCoinsViewBacked::HaveProposal(const uint256 &pid) const { return base->HaveProposal(pid); }
bool CCoinsViewBacked::HavePaymentRequest(const uint256 &prid) const { return base->HavePaymentRequest(prid); }
uint256 CCoinsViewBacked::GetBestBlock() const { return base->GetBestBlock(); }
void CCoinsViewBacked::SetBackend(CCoinsView &viewIn) { base = &viewIn; }
bool CCoinsViewBacked::BatchWrite(CCoinsMap &mapCoins, CProposalMap &mapProposals,
                                  CPaymentRequestMap &mapPaymentRequests, const uint256 &hashBlock) {
    return base->BatchWrite(mapCoins, mapProposals, mapPaymentRequests, hashBlock);
}
CCoinsViewCursor *CCoinsViewBacked::Cursor() const { return base->Cursor(); }

SaltedTxidHasher::SaltedTxidHasher() : k0(GetRand(std::numeric_limits<uint64_t>::max())), k1(GetRand(std::numeric_limits<uint64_t>::max())) {}

CCoinsViewCache::CCoinsViewCache(CCoinsView *baseIn) : CCoinsViewBacked(baseIn), hasModifier(false), cachedCoinsUsage(0) {}

CCoinsViewCache::~CCoinsViewCache()
{
    assert(!hasModifier);
}

size_t CCoinsViewCache::DynamicMemoryUsage() const {
    return memusage::DynamicUsage(cacheCoins) + cachedCoinsUsage;
}

CCoinsMap::const_iterator CCoinsViewCache::FetchCoins(const uint256 &txid) const {
    CCoinsMap::iterator it = cacheCoins.find(txid);
    if (it != cacheCoins.end())
        return it;
    CCoins tmp;
    if (!base->GetCoins(txid, tmp))
        return cacheCoins.end();
    CCoinsMap::iterator ret = cacheCoins.insert(std::make_pair(txid, CCoinsCacheEntry())).first;
    tmp.swap(ret->second.coins);
    if (ret->second.coins.IsPruned()) {
        // The parent only has an empty entry for this txid; we can consider our
        // version as fresh.
        ret->second.flags = CCoinsCacheEntry::FRESH;
    }
    cachedCoinsUsage += ret->second.coins.DynamicMemoryUsage();
    return ret;
}

CProposalMap::const_iterator CCoinsViewCache::FetchProposal(const uint256 &pid) const {
    CProposalMap::iterator it = cacheProposals.find(pid);

    if (it != cacheProposals.end())
        return it->second.IsNull() ? cacheProposals.end() : it;

    CProposal tmp;

    if (!base->GetProposal(pid, tmp) || tmp.IsNull())
        return cacheProposals.end();

    CProposalMap::iterator ret = cacheProposals.insert(std::make_pair(pid, CProposal())).first;
    tmp.swap(ret->second);

    return ret;
}

CPaymentRequestMap::const_iterator CCoinsViewCache::FetchPaymentRequest(const uint256 &prid) const {
    CPaymentRequestMap::iterator it = cachePaymentRequests.find(prid);

    if (it != cachePaymentRequests.end())
        return it->second.IsNull() ? cachePaymentRequests.end() : it;

    CPaymentRequest tmp;

    if (!base->GetPaymentRequest(prid, tmp) || tmp.IsNull())
        return cachePaymentRequests.end();

    CPaymentRequestMap::iterator ret = cachePaymentRequests.insert(std::make_pair(prid, CPaymentRequest())).first;
    tmp.swap(ret->second);

    return ret;
}


bool CCoinsViewCache::GetCoins(const uint256 &txid, CCoins &coins) const {
    CCoinsMap::const_iterator it = FetchCoins(txid);
    if (it != cacheCoins.end()) {
        coins = it->second.coins;
        return true;
    }
    return false;
}

bool CCoinsViewCache::GetProposal(const uint256 &pid, CProposal &proposal) const {
    CProposalMap::const_iterator it = FetchProposal(pid);
    if (it != cacheProposals.end() && !it->second.IsNull()) {
        proposal = it->second;
        return true;
    }
    return false;
}

bool CCoinsViewCache::GetAllProposals(CProposalMap& mapProposal) {
    mapProposal.clear();
    mapProposal.insert(cacheProposals.begin(), cacheProposals.end());

    CProposalMap baseMap;

    if (!base->GetAllProposals(baseMap))
        return false;

    for (CProposalMap::iterator it = baseMap.begin(); it != baseMap.end(); it++)
        if (!it->second.IsNull())
            mapProposal.insert(make_pair(it->first, it->second));

    for (auto it = mapProposal.begin(); it != mapProposal.end();)
        it->second.IsNull() ? mapProposal.erase(it++) : ++it;

    return true;
}

bool CCoinsViewCache::GetPaymentRequest(const uint256 &pid, CPaymentRequest &prequest) const {
    CPaymentRequestMap::const_iterator it = FetchPaymentRequest(pid);
    if (it != cachePaymentRequests.end() && !it->second.IsNull()) {
        prequest = it->second;
        return true;
    }
    return false;
}

bool CCoinsViewCache::GetAllPaymentRequests(CPaymentRequestMap& mapPaymentRequests) {
    mapPaymentRequests.clear();
    mapPaymentRequests.insert(cachePaymentRequests.begin(), cachePaymentRequests.end());

    CPaymentRequestMap baseMap;

    if (!base->GetAllPaymentRequests(baseMap))
        return false;

    for (CPaymentRequestMap::iterator it = baseMap.begin(); it != baseMap.end(); it++)
        if (!it->second.IsNull())
            mapPaymentRequests.insert(make_pair(it->first, it->second));

    for (auto it = mapPaymentRequests.begin(); it != mapPaymentRequests.end();)
        it->second.IsNull() ? mapPaymentRequests.erase(it++) : ++it;

    return true;
}

CCoinsModifier CCoinsViewCache::ModifyCoins(const uint256 &txid) {
    assert(!hasModifier);
    std::pair<CCoinsMap::iterator, bool> ret = cacheCoins.insert(std::make_pair(txid, CCoinsCacheEntry()));
    size_t cachedCoinUsage = 0;
    if (ret.second) {
        if (!base->GetCoins(txid, ret.first->second.coins)) {
            // The parent view does not have this entry; mark it as fresh.
            ret.first->second.coins.Clear();
            ret.first->second.flags = CCoinsCacheEntry::FRESH;
        } else if (ret.first->second.coins.IsPruned()) {
            // The parent view only has a pruned entry for this; mark it as fresh.
            ret.first->second.flags = CCoinsCacheEntry::FRESH;
        }
    } else {
        cachedCoinUsage = ret.first->second.coins.DynamicMemoryUsage();
    }
    // Assume that whenever ModifyCoins is called, the entry will be modified.
    ret.first->second.flags |= CCoinsCacheEntry::DIRTY;
    return CCoinsModifier(*this, ret.first, cachedCoinUsage);
}

CProposalModifier CCoinsViewCache::ModifyProposal(const uint256 &pid) {
    assert(!hasModifier);
    std::pair<CProposalMap::iterator, bool> ret = cacheProposals.insert(std::make_pair(pid, CProposal()));
    if (ret.second) {
        if (!base->GetProposal(pid, ret.first->second)) {
            ret.first->second.SetNull();
        }
    }
    return CProposalModifier(*this, ret.first);
}

CPaymentRequestModifier CCoinsViewCache::ModifyPaymentRequest(const uint256 &prid) {
    assert(!hasModifier);
    std::pair<CPaymentRequestMap::iterator, bool> ret = cachePaymentRequests.insert(std::make_pair(prid, CPaymentRequest()));
    if (ret.second) {
        if (!base->GetPaymentRequest(prid, ret.first->second)) {
            ret.first->second.SetNull();
        }
    }
    return CPaymentRequestModifier(*this, ret.first);
}

// ModifyNewCoins has to know whether the new outputs its creating are for a
// coinbase or not.  If they are for a coinbase, it can not mark them as fresh.
// This is to ensure that the historical duplicate coinbases before BIP30 was
// in effect will still be properly overwritten when spent.
CCoinsModifier CCoinsViewCache::ModifyNewCoins(const uint256 &txid, bool coinbase) {
    assert(!hasModifier);
    std::pair<CCoinsMap::iterator, bool> ret = cacheCoins.insert(std::make_pair(txid, CCoinsCacheEntry()));
    ret.first->second.coins.Clear();
    if (!coinbase) {
        ret.first->second.flags = CCoinsCacheEntry::FRESH;
    }
    ret.first->second.flags |= CCoinsCacheEntry::DIRTY;
    return CCoinsModifier(*this, ret.first, 0);
}

bool CCoinsViewCache::AddProposal(const CProposal& proposal) const {
    if (HaveProposal(proposal.hash))
        return false;

    if (cacheProposals.count(proposal.hash))
        cacheProposals[proposal.hash]=proposal;
    else
        cacheProposals.insert(std::make_pair(proposal.hash, proposal));

    return true;
}

bool CCoinsViewCache::AddPaymentRequest(const CPaymentRequest& prequest) const {
    if (HavePaymentRequest(prequest.hash))
        return false;

    if (cachePaymentRequests.count(prequest.hash))
        cachePaymentRequests[prequest.hash]=prequest;
    else
        cachePaymentRequests.insert(std::make_pair(prequest.hash, prequest));

    return true;
}

bool CCoinsViewCache::RemoveProposal(const uint256 &pid) const {
    if (!HaveProposal(pid))
        return false;

    cacheProposals[pid] = CProposal();
    cacheProposals[pid].SetNull();

    assert(cacheProposals[pid].IsNull());

    return true;
}

bool CCoinsViewCache::RemovePaymentRequest(const uint256 &prid) const {
    if (!HavePaymentRequest(prid))
        return false;

    cachePaymentRequests[prid] = CPaymentRequest();
    cachePaymentRequests[prid].SetNull();

    assert(cachePaymentRequests[prid].IsNull());

    return true;
}

const CCoins* CCoinsViewCache::AccessCoins(const uint256 &txid) const {
    CCoinsMap::const_iterator it = FetchCoins(txid);
    if (it == cacheCoins.end()) {
        return nullptr;
    } else {
        return &it->second.coins;
    }
}

bool CCoinsViewCache::HaveCoins(const uint256 &txid) const {
    CCoinsMap::const_iterator it = FetchCoins(txid);
    // We're using vtx.empty() instead of IsPruned here for performance reasons,
    // as we only care about the case where a transaction was replaced entirely
    // in a reorganization (which wipes vout entirely, as opposed to spending
    // which just cleans individual outputs).
    return (it != cacheCoins.end() && !it->second.coins.vout.empty());
}

bool CCoinsViewCache::HaveProposal(const uint256 &id) const {
    CProposalMap::const_iterator it = FetchProposal(id);
    return (it != cacheProposals.end() && !it->second.IsNull());
}

bool CCoinsViewCache::HavePaymentRequest(const uint256 &id) const {
    CPaymentRequestMap::const_iterator it = FetchPaymentRequest(id);
    return (it != cachePaymentRequests.end() && !it->second.IsNull());
}

bool CCoinsViewCache::HaveCoinsInCache(const uint256 &txid) const {
    CCoinsMap::const_iterator it = cacheCoins.find(txid);
    return it != cacheCoins.end();
}

bool CCoinsViewCache::HaveProposalInCache(const uint256 &pid) const {
    auto it = cacheProposals.find(pid);
    return it != cacheProposals.end() && !it->second.IsNull();
}

bool CCoinsViewCache::HavePaymentRequestInCache(const uint256 &txid) const {
    auto it = cachePaymentRequests.find(txid);
    return it != cachePaymentRequests.end() && !it->second.IsNull();
}

uint256 CCoinsViewCache::GetBestBlock() const {
    if (hashBlock.IsNull())
        hashBlock = base->GetBestBlock();
    return hashBlock;
}

void CCoinsViewCache::SetBestBlock(const uint256 &hashBlockIn) {
    hashBlock = hashBlockIn;
}

bool CCoinsViewCache::BatchWrite(CCoinsMap &mapCoins, CProposalMap &mapProposals, CPaymentRequestMap &mapPaymentRequests, const uint256 &hashBlockIn) {
    assert(!hasModifier);
    for (CCoinsMap::iterator it = mapCoins.begin(); it != mapCoins.end();) {
        if (it->second.flags & CCoinsCacheEntry::DIRTY) { // Ignore non-dirty entries (optimization).
            CCoinsMap::iterator itUs = cacheCoins.find(it->first);
            if (itUs == cacheCoins.end()) {
                // The parent cache does not have an entry, while the child does
                // We can ignore it if it's both FRESH and pruned in the child
                if (!(it->second.flags & CCoinsCacheEntry::FRESH && it->second.coins.IsPruned())) {
                    // Otherwise we will need to create it in the parent
                    // and move the data up and mark it as dirty
                    CCoinsCacheEntry& entry = cacheCoins[it->first];
                    entry.coins.swap(it->second.coins);
                    cachedCoinsUsage += entry.coins.DynamicMemoryUsage();
                    entry.flags = CCoinsCacheEntry::DIRTY;
                    // We can mark it FRESH in the parent if it was FRESH in the child
                    // Otherwise it might have just been flushed from the parent's cache
                    // and already exist in the grandparent
                    if (it->second.flags & CCoinsCacheEntry::FRESH)
                        entry.flags |= CCoinsCacheEntry::FRESH;
                }
            } else {
                // Found the entry in the parent cache
                if ((itUs->second.flags & CCoinsCacheEntry::FRESH) && it->second.coins.IsPruned()) {
                    // The grandparent does not have an entry, and the child is
                    // modified and being pruned. This means we can just delete
                    // it from the parent.
                    cachedCoinsUsage -= itUs->second.coins.DynamicMemoryUsage();
                    cacheCoins.erase(itUs);
                } else {
                    // A normal modification.
                    cachedCoinsUsage -= itUs->second.coins.DynamicMemoryUsage();
                    itUs->second.coins.swap(it->second.coins);
                    cachedCoinsUsage += itUs->second.coins.DynamicMemoryUsage();
                    itUs->second.flags |= CCoinsCacheEntry::DIRTY;
                }
            }
        }
        CCoinsMap::iterator itOld = it++;
        mapCoins.erase(itOld);
    }

    for (CProposalMap::iterator it = mapProposals.begin(); it != mapProposals.end();) {
        CProposal& entry = cacheProposals[it->first];
        entry.swap(it->second);
        CProposalMap::iterator itOld = it++;
        mapProposals.erase(itOld);
    }

    for (CPaymentRequestMap::iterator it = mapPaymentRequests.begin(); it != mapPaymentRequests.end();) {
        CPaymentRequest& entry = cachePaymentRequests[it->first];
        entry.swap(it->second);
        CPaymentRequestMap::iterator itOld = it++;
        mapPaymentRequests.erase(itOld);
    }

    hashBlock = hashBlockIn;
    return true;
}

bool CCoinsViewCache::Flush() {
    bool fOk = base->BatchWrite(cacheCoins, cacheProposals, cachePaymentRequests, hashBlock);
    cacheCoins.clear();
    cacheProposals.clear();
    cachePaymentRequests.clear();
    cachedCoinsUsage = 0;
    return fOk;
}

void CCoinsViewCache::Uncache(const uint256& hash)
{
    CCoinsMap::iterator it = cacheCoins.find(hash);
    if (it != cacheCoins.end() && it->second.flags == 0) {
        cachedCoinsUsage -= it->second.coins.DynamicMemoryUsage();
        cacheCoins.erase(it);
    }
}

unsigned int CCoinsViewCache::GetCacheSize() const {
    return cacheCoins.size();
}

uint256 CCoinsViewCache::GetCFundDBStateHash(const CAmount& nCFLocked, const CAmount& nCFSupply)
{
    CPaymentRequestMap mapPaymentRequests;
    CProposalMap mapProposals;

    int64_t nTimeStart = GetTimeMicros();

    CHashWriter writer(0,0);

    writer << nCFSupply;
    writer << nCFLocked;

    if (GetAllProposals(mapProposals) && GetAllPaymentRequests(mapPaymentRequests))
    {

        for (auto &it: mapProposals)
        {
            if (!it.second.IsNull())
            {
                writer << it.second;
            }
        }

        for (auto &it: mapPaymentRequests)
        {
            if (!it.second.IsNull())
            {
                writer << it.second;
            }
        }

    }

    uint256 ret = writer.GetHash();
    int64_t nTimeEnd = GetTimeMicros();
    LogPrint("bench", " Benchmark: Calculate CFundDB state hash: %.2fms\n", (nTimeEnd - nTimeStart) * 0.001);

    return ret;
}

const CTxOut &CCoinsViewCache::GetOutputFor(const CTxIn& input) const
{
    const CCoins* coins = AccessCoins(input.prevout.hash);
    assert(coins && coins->IsAvailable(input.prevout.n));
    return coins->vout[input.prevout.n];
}

CAmount CCoinsViewCache::GetValueIn(const CTransaction& tx) const
{
    if (tx.IsCoinBase())
        return 0;

    CAmount nResult = 0;
    for (unsigned int i = 0; i < tx.vin.size(); i++)
        nResult += GetOutputFor(tx.vin[i]).nValue;

    return nResult;
}

bool CCoinsViewCache::HaveInputs(const CTransaction& tx) const
{
    if (!tx.IsCoinBase()) {
        for (unsigned int i = 0; i < tx.vin.size(); i++) {
            const COutPoint &prevout = tx.vin[i].prevout;
            const CCoins* coins = AccessCoins(prevout.hash);
            if (!coins || !coins->IsAvailable(prevout.n)) {
                return false;
            }
        }
    }
    return true;
}

double CCoinsViewCache::GetPriority(const CTransaction &tx, int nHeight, CAmount &inChainInputValue) const
{
    inChainInputValue = 0;
    if (tx.IsCoinBase())
        return 0.0;
    double dResult = 0.0;
    for(const CTxIn& txin: tx.vin)
    {
        const CCoins* coins = AccessCoins(txin.prevout.hash);
        assert(coins);
        if (!coins->IsAvailable(txin.prevout.n)) continue;
        if (coins->nHeight <= nHeight) {
            dResult += coins->vout[txin.prevout.n].nValue * (nHeight-coins->nHeight);
            inChainInputValue += coins->vout[txin.prevout.n].nValue;
        }
    }
    return tx.ComputePriority(dResult);
}

CCoinsModifier::CCoinsModifier(CCoinsViewCache& cache_, CCoinsMap::iterator it_, size_t usage) : cache(cache_), it(it_), cachedCoinUsage(usage) {
    assert(!cache.hasModifier);
    cache.hasModifier = true;
}

CCoinsModifier::~CCoinsModifier()
{
    assert(cache.hasModifier);
    cache.hasModifier = false;
    it->second.coins.Cleanup();
    cache.cachedCoinsUsage -= cachedCoinUsage; // Subtract the old usage
    if ((it->second.flags & CCoinsCacheEntry::FRESH) && it->second.coins.IsPruned()) {
        cache.cacheCoins.erase(it);
    } else {
        // If the coin still exists after the modification, add the new usage
        cache.cachedCoinsUsage += it->second.coins.DynamicMemoryUsage();
    }
}

CProposalModifier::CProposalModifier(CCoinsViewCache& cache_, CProposalMap::iterator it_) : cache(cache_), it(it_) {
    assert(!cache.hasModifier);
    cache.hasModifier = true;
}

CProposalModifier::~CProposalModifier()
{
    assert(cache.hasModifier);
    cache.hasModifier = false;

    if (it->second.IsNull()) {
        cache.cacheProposals.erase(it);
    }
}

CPaymentRequestModifier::CPaymentRequestModifier(CCoinsViewCache& cache_, CPaymentRequestMap::iterator it_) : cache(cache_), it(it_) {
    assert(!cache.hasModifier);
    cache.hasModifier = true;
}

CPaymentRequestModifier::~CPaymentRequestModifier()
{
    assert(cache.hasModifier);
    cache.hasModifier = false;

    if (it->second.IsNull()) {
        cache.cachePaymentRequests.erase(it);
    }
}

CCoinsViewCursor::~CCoinsViewCursor()
{
}
