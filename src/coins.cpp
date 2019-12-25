// Copyright (c) 2012-2015 The Bitcoin Core developers
// Copyright (c) 2018-2019 The NavCoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <coins.h>

#include <main.h>
#include <memusage.h>
#include <random.h>
#include <util.h>
#include <utiltime.h>

#include <assert.h>

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

bool CStateView::GetCoins(const uint256 &txid, CCoins &coins) const { return false; }
bool CStateView::GetProposal(const uint256 &pid, CProposal &proposal) const { return false; }
bool CStateView::GetPaymentRequest(const uint256 &prid, CPaymentRequest &prequest) const { return false; }
bool CStateView::GetCachedVoter(const CVoteMapKey &voter, CVoteMapValue& vote) const { return false; }
bool CStateView::GetConsultation(const uint256 &cid, CConsultation& consultation) const { return false; }
bool CStateView::GetConsultationAnswer(const uint256 &cid, CConsultationAnswer& answer) const { return false; }
bool CStateView::HaveCoins(const uint256 &txid) const { return false; }
bool CStateView::HaveProposal(const uint256 &pid) const { return false; }
bool CStateView::HavePaymentRequest(const uint256 &prid) const { return false; }
bool CStateView::HaveCachedVoter(const CVoteMapKey &voter) const { return false; }
bool CStateView::HaveConsultation(const uint256 &cid) const { return false; }
bool CStateView::HaveConsultationAnswer(const uint256 &cid) const { return false; }
bool CStateView::GetAllProposals(CProposalMap& map) { return false; }
bool CStateView::GetAllPaymentRequests(CPaymentRequestMap& map) { return false; }
bool CStateView::GetAllVotes(CVoteMap& map) { return false; }
bool CStateView::GetAllConsultations(CConsultationMap& map) { return false; }
bool CStateView::GetAllConsultationAnswers(CConsultationAnswerMap& map) { return false; }
uint256 CStateView::GetBestBlock() const { return uint256(); }
bool CStateView::BatchWrite(CCoinsMap &mapCoins, CProposalMap &mapProposals,
                            CPaymentRequestMap &mapPaymentRequests, CVoteMap &mapVotes,
                            CConsultationMap& mapConsultations, CConsultationAnswerMap& mapAnswers,
                            const uint256 &hashBlock) { return false; }
CStateViewCursor *CStateView::Cursor() const { return 0; }


CStateViewBacked::CStateViewBacked(CStateView *viewIn) : base(viewIn) { }
bool CStateViewBacked::GetCoins(const uint256 &txid, CCoins &coins) const { return base->GetCoins(txid, coins); }
bool CStateViewBacked::GetProposal(const uint256 &pid, CProposal &proposal) const { return base->GetProposal(pid, proposal); }
bool CStateViewBacked::GetPaymentRequest(const uint256 &prid, CPaymentRequest &prequest) const { return base->GetPaymentRequest(prid, prequest); }
bool CStateViewBacked::GetConsultation(const uint256 &cid, CConsultation &consultation) const { return base->GetConsultation(cid, consultation); }
bool CStateViewBacked::GetConsultationAnswer(const uint256 &cid, CConsultationAnswer &answer) const { return base->GetConsultationAnswer(cid, answer); }
bool CStateViewBacked::HaveCoins(const uint256 &txid) const { return base->HaveCoins(txid); }
bool CStateViewBacked::HaveProposal(const uint256 &pid) const { return base->HaveProposal(pid); }
bool CStateViewBacked::HavePaymentRequest(const uint256 &prid) const { return base->HavePaymentRequest(prid); }
bool CStateViewBacked::HaveCachedVoter(const CVoteMapKey &voter) const { return base->HaveCachedVoter(voter); }
bool CStateViewBacked::HaveConsultation(const uint256 &cid) const { return base->HaveConsultation(cid); }
bool CStateViewBacked::HaveConsultationAnswer(const uint256 &cid) const { return base->HaveConsultationAnswer(cid); }
bool CStateViewBacked::GetCachedVoter(const CVoteMapKey &voter, CVoteMapValue& vote) const { return base->GetCachedVoter(voter, vote); }
bool CStateViewBacked::GetAllProposals(CProposalMap& map) { return base->GetAllProposals(map); }
bool CStateViewBacked::GetAllPaymentRequests(CPaymentRequestMap& map) { return base->GetAllPaymentRequests(map); }
bool CStateViewBacked::GetAllVotes(CVoteMap& map) { return base->GetAllVotes(map); }
bool CStateViewBacked::GetAllConsultations(CConsultationMap& map) { return base->GetAllConsultations(map); }
bool CStateViewBacked::GetAllConsultationAnswers(CConsultationAnswerMap& map) { return base->GetAllConsultationAnswers(map); }
uint256 CStateViewBacked::GetBestBlock() const { return base->GetBestBlock(); }
void CStateViewBacked::SetBackend(CStateView &viewIn) { base = &viewIn; }
bool CStateViewBacked::BatchWrite(CCoinsMap &mapCoins, CProposalMap &mapProposals,
                                  CPaymentRequestMap &mapPaymentRequests, CVoteMap &mapVotes,
                                  CConsultationMap &mapConsultations, CConsultationAnswerMap &mapAnswers,
                                  const uint256 &hashBlock) {
    return base->BatchWrite(mapCoins, mapProposals, mapPaymentRequests, mapVotes, mapConsultations, mapAnswers, hashBlock);
}
CStateViewCursor *CStateViewBacked::Cursor() const { return base->Cursor(); }

SaltedTxidHasher::SaltedTxidHasher() : k0(GetRand(std::numeric_limits<uint64_t>::max())), k1(GetRand(std::numeric_limits<uint64_t>::max())) {}

CStateViewCache::CStateViewCache(CStateView *baseIn) : CStateViewBacked(baseIn), hasModifier(false), cachedCoinsUsage(0) { }

CStateViewCache::~CStateViewCache()
{
    assert(!hasModifier);
}

size_t CStateViewCache::DynamicMemoryUsage() const {
    return memusage::DynamicUsage(cacheCoins) + cachedCoinsUsage;
}

CCoinsMap::const_iterator CStateViewCache::FetchCoins(const uint256 &txid) const {
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

CProposalMap::const_iterator CStateViewCache::FetchProposal(const uint256 &pid) const {
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

CPaymentRequestMap::const_iterator CStateViewCache::FetchPaymentRequest(const uint256 &prid) const {
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

CVoteMap::const_iterator CStateViewCache::FetchVote(const CVoteMapKey &voter) const
{
    CVoteMap::iterator it = cacheVotes.find(voter);

    if (it != cacheVotes.end())
        return it;

    CVoteList tmp;

    if (!base->GetCachedVoter(voter, tmp) || tmp.IsNull())
        return cacheVotes.end();

    CVoteMap::iterator ret = cacheVotes.insert(std::make_pair(voter, CVoteList())).first;
    tmp.swap(ret->second);

    return ret;
}

CConsultationMap::const_iterator CStateViewCache::FetchConsultation(const uint256 &cid) const {
    CConsultationMap::iterator it = cacheConsultations.find(cid);

    if (it != cacheConsultations.end())
        return it;
    
    CConsultation tmp;

    if (!base->GetConsultation(cid, tmp) || tmp.IsNull())
        return cacheConsultations.end();
    
    CConsultationMap::iterator ret = cacheConsultations.insert(std::make_pair(cid, CConsultation())).first;
    tmp.swap(ret->second);

    return ret;
}

CConsultationAnswerMap::const_iterator CStateViewCache::FetchConsultationAnswer(const uint256 &cid) const {
    CConsultationAnswerMap::iterator it = cacheAnswers.find(cid);

    if (it != cacheAnswers.end())
        return it;

    CConsultationAnswer tmp;

    if (!base->GetConsultationAnswer(cid, tmp) || tmp.IsNull())
        return cacheAnswers.end();

    CConsultationAnswerMap::iterator ret = cacheAnswers.insert(std::make_pair(cid, CConsultationAnswer())).first;
    tmp.swap(ret->second);

    return ret;
}


bool CStateViewCache::GetCoins(const uint256 &txid, CCoins &coins) const {
    CCoinsMap::const_iterator it = FetchCoins(txid);
    if (it != cacheCoins.end()) {
        coins = it->second.coins;
        return true;
    }
    return false;
}

bool CStateViewCache::GetCachedVoter(const CVoteMapKey &voter, CVoteMapValue& vote) const {
    CVoteMap::const_iterator it = FetchVote(voter);
    if (it != cacheVotes.end() && !it->second.IsNull()) {
        vote = it->second;
        return true;
    }
    return false;
}

bool CStateViewCache::GetProposal(const uint256 &pid, CProposal &proposal) const {
    CProposalMap::const_iterator it = FetchProposal(pid);
    if (it != cacheProposals.end() && !it->second.IsNull()) {
        proposal = it->second;
        return true;
    }
    return false;
}

bool CStateViewCache::GetPaymentRequest(const uint256 &pid, CPaymentRequest &prequest) const {
    CPaymentRequestMap::const_iterator it = FetchPaymentRequest(pid);
    if (it != cachePaymentRequests.end() && !it->second.IsNull()) {
        prequest = it->second;
        return true;
    }
    return false;
}

bool CStateViewCache::GetConsultation(const uint256 &cid, CConsultation &consultation) const {
    CConsultationMap::const_iterator it = FetchConsultation(cid);
    if (it != cacheConsultations.end()) {
        consultation = it->second;
        return !it->second.IsNull();
    }
    return false;
}

bool CStateViewCache::GetConsultationAnswer(const uint256 &cid, CConsultationAnswer &answer) const {
    CConsultationAnswerMap::const_iterator it = FetchConsultationAnswer(cid);
    if (it != cacheAnswers.end() && !it->second.IsNull()) {
        answer = it->second;
        return true;
    }
    return false;
}

bool CStateViewCache::GetAllProposals(CProposalMap& mapProposal) {
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

bool CStateViewCache::GetAllPaymentRequests(CPaymentRequestMap& mapPaymentRequests) {
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

bool CStateViewCache::GetAllVotes(CVoteMap& mapVotes) {
    mapVotes.clear();
    mapVotes.insert(cacheVotes.begin(), cacheVotes.end());

    CVoteMap baseMap;

    if (!base->GetAllVotes(baseMap))
        return false;

    for (CVoteMap::iterator it = baseMap.begin(); it != baseMap.end(); it++)
        if (!it->second.IsNull())
            mapVotes.insert(make_pair(it->first, it->second));

    for (auto it = mapVotes.begin(); it != mapVotes.end();)
        it->second.IsNull() ? mapVotes.erase(it++) : ++it;

    return true;
}

bool CStateViewCache::GetAllConsultations(CConsultationMap& mapConsultations) {
    mapConsultations.clear();
    mapConsultations.insert(cacheConsultations.begin(), cacheConsultations.end());

    CConsultationMap baseMap;

    if (!base->GetAllConsultations(baseMap))
        return false;

    for (CConsultationMap::iterator it = baseMap.begin(); it != baseMap.end(); it++)
        if (!it->second.IsNull())
            mapConsultations.insert(make_pair(it->first, it->second));

    for (auto it = mapConsultations.begin(); it != mapConsultations.end();)
        it->second.IsNull() ? mapConsultations.erase(it++) : ++it;

    return true;
}

bool CStateViewCache::GetAllConsultationAnswers(CConsultationAnswerMap& mapAnswers) {
    mapAnswers.clear();
    mapAnswers.insert(cacheAnswers.begin(), cacheAnswers.end());

    CConsultationAnswerMap baseMap;

    if (!base->GetAllConsultationAnswers(baseMap))
        return false;

    for (CConsultationAnswerMap::iterator it = baseMap.begin(); it != baseMap.end(); it++)
        if (!it->second.IsNull())
            mapAnswers.insert(make_pair(it->first, it->second));

    for (auto it = mapAnswers.begin(); it != mapAnswers.end();)
        it->second.IsNull() ? mapAnswers.erase(it++) : ++it;

    return true;
}

CCoinsModifier CStateViewCache::ModifyCoins(const uint256 &txid) {
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

CProposalModifier CStateViewCache::ModifyProposal(const uint256 &pid) {
    assert(!hasModifier);
    std::pair<CProposalMap::iterator, bool> ret = cacheProposals.insert(std::make_pair(pid, CProposal()));
    if (ret.second) {
        if (!base->GetProposal(pid, ret.first->second)) {
            ret.first->second.SetNull();
        }
    }
    return CProposalModifier(*this, ret.first);
}

CVoteModifier CStateViewCache::ModifyVote(const CVoteMapKey &voter) {
    assert(!hasModifier);
    std::pair<CVoteMap::iterator, bool> ret = cacheVotes.insert(std::make_pair(voter, CVoteList()));
    if (ret.second) {
        if (!base->GetCachedVoter(voter, ret.first->second)) {
            ret.first->second.SetNull();
        }
    }
    return CVoteModifier(*this, ret.first);
}

CPaymentRequestModifier CStateViewCache::ModifyPaymentRequest(const uint256 &prid) {
    assert(!hasModifier);
    std::pair<CPaymentRequestMap::iterator, bool> ret = cachePaymentRequests.insert(std::make_pair(prid, CPaymentRequest()));
    if (ret.second) {
        if (!base->GetPaymentRequest(prid, ret.first->second)) {
            ret.first->second.SetNull();
        }
    }
    return CPaymentRequestModifier(*this, ret.first);
}

CConsultationModifier CStateViewCache::ModifyConsultation(const uint256 &cid) {
    assert(!hasModifier);
    std::pair<CConsultationMap::iterator, bool> ret = cacheConsultations.insert(std::make_pair(cid, CConsultation()));
    if (ret.second) {
        if (!base->GetConsultation(cid, ret.first->second)) {
            ret.first->second.SetNull();
        }
    }
    ret.first->second.fDirty = true;
    return CConsultationModifier(*this, ret.first);
}

CConsultationAnswerModifier CStateViewCache::ModifyConsultationAnswer(const uint256 &cid) {
    assert(!hasModifier);
    std::pair<CConsultationAnswerMap::iterator, bool> ret = cacheAnswers.insert(std::make_pair(cid, CConsultationAnswer()));
    if (ret.second) {
        if (!base->GetConsultationAnswer(cid, ret.first->second)) {
            ret.first->second.SetNull();
        }
    }
    ret.first->second.fDirty = true;
    return CConsultationAnswerModifier(*this, ret.first);
}

// ModifyNewCoins has to know whether the new outputs its creating are for a
// coinbase or not.  If they are for a coinbase, it can not mark them as fresh.
// This is to ensure that the historical duplicate coinbases before BIP30 was
// in effect will still be properly overwritten when spent.
CCoinsModifier CStateViewCache::ModifyNewCoins(const uint256 &txid, bool coinbase) {
    assert(!hasModifier);
    std::pair<CCoinsMap::iterator, bool> ret = cacheCoins.insert(std::make_pair(txid, CCoinsCacheEntry()));
    ret.first->second.coins.Clear();
    if (!coinbase) {
        ret.first->second.flags = CCoinsCacheEntry::FRESH;
    }
    ret.first->second.flags |= CCoinsCacheEntry::DIRTY;
    return CCoinsModifier(*this, ret.first, 0);
}

bool CStateViewCache::AddProposal(const CProposal& proposal) const {
    if (HaveProposal(proposal.hash))
        return false;

    if (cacheProposals.count(proposal.hash))
        cacheProposals[proposal.hash]=proposal;
    else
        cacheProposals.insert(std::make_pair(proposal.hash, proposal));

    return true;
}

bool CStateViewCache::AddCachedVoter(const CVoteMapKey &voter, CVoteMapValue& vote) const {
    if (HaveCachedVoter(voter))
        return false;

    if (cacheVotes.count(voter))
        cacheVotes[voter]=vote;
    else
        cacheVotes.insert(std::make_pair(voter, vote));

    return true;
}

bool CStateViewCache::AddPaymentRequest(const CPaymentRequest& prequest) const {
    if (HavePaymentRequest(prequest.hash))
        return false;

    if (cachePaymentRequests.count(prequest.hash))
        cachePaymentRequests[prequest.hash]=prequest;
    else
        cachePaymentRequests.insert(std::make_pair(prequest.hash, prequest));

    return true;
}

bool CStateViewCache::AddConsultation(const CConsultation& consultation) const {
    if (HaveConsultation(consultation.hash))
        return false;

    if (cacheConsultations.count(consultation.hash))
        cacheConsultations[consultation.hash]=consultation;
    else
        cacheConsultations.insert(std::make_pair(consultation.hash, consultation));

    return true;
}

bool CStateViewCache::AddConsultationAnswer(const CConsultationAnswer& answer) {
    if (HaveConsultationAnswer(answer.hash))
        return false;

    CConsultationModifier mConsultation = ModifyConsultation(answer.parent);
    mConsultation->vAnswers.push_back(answer.hash);
    mConsultation->fDirty = true;

    if (cacheAnswers.count(answer.hash))
        cacheAnswers[answer.hash]=answer;
    else
        cacheAnswers.insert(std::make_pair(answer.hash, answer));

    return true;
}

bool CStateViewCache::RemoveProposal(const uint256 &pid) const {
    if (!HaveProposal(pid))
        return false;

    cacheProposals[pid] = CProposal();
    cacheProposals[pid].SetNull();

    assert(cacheProposals[pid].IsNull());

    return true;
}

bool CStateViewCache::RemovePaymentRequest(const uint256 &prid) const {
    if (!HavePaymentRequest(prid))
        return false;

    cachePaymentRequests[prid] = CPaymentRequest();
    cachePaymentRequests[prid].SetNull();

    assert(cachePaymentRequests[prid].IsNull());

    return true;
}

bool CStateViewCache::RemoveCachedVoter(const CVoteMapKey &voter) const {
    if (!HaveCachedVoter(voter))
        return false;

    cacheVotes[voter] = CVoteList();
    cacheVotes[voter].SetNull();

    assert(cacheVotes[voter].IsNull());

    return true;
}

bool CStateViewCache::RemoveConsultation(const uint256 &cid) {
    CConsultation consultation;

    if (!GetConsultation(cid, consultation))
        return false;

    for (auto &it: consultation.vAnswers)
    {
        RemoveConsultationAnswer(it);
    }

    cacheConsultations[cid] = CConsultation();
    cacheConsultations[cid].SetNull();

    assert(cachePaymentRequests[cid].IsNull());

    return true;
}

bool CStateViewCache::RemoveConsultationAnswer(const uint256 &cid) {
    CConsultationAnswer answer;
    if (!GetConsultationAnswer(cid, answer))
        return false;

    CConsultationModifier mConsultation = ModifyConsultation(answer.parent);
    vector<uint256>::iterator it;
    for (it = mConsultation->vAnswers.begin(); it < mConsultation->vAnswers.end(); it++)
        if (*it == cid)
            mConsultation->vAnswers.erase(it);
    mConsultation->fDirty = true;

    cacheAnswers[cid] = CConsultationAnswer();
    cacheAnswers[cid].SetNull();

    assert(cacheAnswers[cid].IsNull());

    return true;
}

const CCoins* CStateViewCache::AccessCoins(const uint256 &txid) const {
    CCoinsMap::const_iterator it = FetchCoins(txid);
    if (it == cacheCoins.end()) {
        return nullptr;
    } else {
        return &it->second.coins;
    }
}

bool CStateViewCache::HaveCoins(const uint256 &txid) const {
    CCoinsMap::const_iterator it = FetchCoins(txid);
    // We're using vtx.empty() instead of IsPruned here for performance reasons,
    // as we only care about the case where a transaction was replaced entirely
    // in a reorganization (which wipes vout entirely, as opposed to spending
    // which just cleans individual outputs).
    return (it != cacheCoins.end() && !it->second.coins.vout.empty());
}

bool CStateViewCache::HaveProposal(const uint256 &id) const {
    CProposalMap::const_iterator it = FetchProposal(id);
    return (it != cacheProposals.end() && !it->second.IsNull());
}

bool CStateViewCache::HavePaymentRequest(const uint256 &id) const {
    CPaymentRequestMap::const_iterator it = FetchPaymentRequest(id);
    return (it != cachePaymentRequests.end() && !it->second.IsNull());
}

bool CStateViewCache::HaveCachedVoter(const CVoteMapKey &voter) const {
    CVoteMap::const_iterator it = FetchVote(voter);
    // We're using vtx.empty() instead of IsPruned here for performance reasons,
    // as we only care about the case where a transaction was replaced entirely
    // in a reorganization (which wipes vout entirely, as opposed to spending
    // which just cleans individual outputs).
    return (it != cacheVotes.end() && !it->second.IsNull());
}

bool CStateViewCache::HaveConsultation(const uint256 &cid) const {
    CConsultationMap::const_iterator it = FetchConsultation(cid);
    return (it != cacheConsultations.end() && !it->second.IsNull());
}

bool CStateViewCache::HaveConsultationAnswer(const uint256 &cid) const {
    CConsultationAnswerMap::const_iterator it = FetchConsultationAnswer(cid);
    return (it != cacheAnswers.end() && !it->second.IsNull());
}

bool CStateViewCache::HaveCoinsInCache(const uint256 &txid) const {
    CCoinsMap::const_iterator it = cacheCoins.find(txid);
    return it != cacheCoins.end();
}

bool CStateViewCache::HaveProposalInCache(const uint256 &pid) const {
    auto it = cacheProposals.find(pid);
    return it != cacheProposals.end() && !it->second.IsNull();
}

bool CStateViewCache::HavePaymentRequestInCache(const uint256 &txid) const {
    auto it = cachePaymentRequests.find(txid);
    return it != cachePaymentRequests.end() && !it->second.IsNull();
}

uint256 CStateViewCache::GetBestBlock() const {
    if (hashBlock.IsNull())
        hashBlock = base->GetBestBlock();
    return hashBlock;
}

void CStateViewCache::SetBestBlock(const uint256 &hashBlockIn) {
    hashBlock = hashBlockIn;
}

bool CStateViewCache::BatchWrite(CCoinsMap &mapCoins, CProposalMap &mapProposals, CPaymentRequestMap &mapPaymentRequests,
                                 CVoteMap& mapVotes, CConsultationMap& mapConsultations, CConsultationAnswerMap& mapAnswers,
                                 const uint256 &hashBlockIn) {
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

    for (CVoteMap::iterator it = mapVotes.begin(); it != mapVotes.end();){
        CVoteList& entry = cacheVotes[it->first];
        entry.swap(it->second);
        CVoteMap::iterator itOld = it++;
        mapVotes.erase(itOld);
    }

    for (CConsultationMap::iterator it = mapConsultations.begin(); it != mapConsultations.end();) {
        CConsultation& entry = cacheConsultations[it->first];
        entry.swap(it->second);
        CConsultationMap::iterator itOld = it++;
        mapConsultations.erase(itOld);
    }

    for (CConsultationAnswerMap::iterator it = mapAnswers.begin(); it != mapAnswers.end();) {
        CConsultationAnswer& entry = cacheAnswers[it->first];
        entry.swap(it->second);
        CConsultationAnswerMap::iterator itOld = it++;
        mapAnswers.erase(itOld);
    }

    hashBlock = hashBlockIn;
    return true;
}

bool CStateViewCache::Flush() {
    bool fOk = base->BatchWrite(cacheCoins, cacheProposals, cachePaymentRequests, cacheVotes, cacheConsultations, cacheAnswers, hashBlock);
    cacheCoins.clear();
    cacheProposals.clear();
    cachePaymentRequests.clear();
    cacheVotes.clear();
    cacheConsultations.clear();
    cacheAnswers.clear();
    cachedCoinsUsage = 0;
    return fOk;
}

void CStateViewCache::Uncache(const uint256& hash)
{
    CCoinsMap::iterator it = cacheCoins.find(hash);
    if (it != cacheCoins.end() && it->second.flags == 0) {
        cachedCoinsUsage -= it->second.coins.DynamicMemoryUsage();
        cacheCoins.erase(it);
    }
}

unsigned int CStateViewCache::GetCacheSize() const {
    return cacheCoins.size();
}

const CTxOut &CStateViewCache::GetOutputFor(const CTxIn& input) const
{
    const CCoins* coins = AccessCoins(input.prevout.hash);
    assert(coins && coins->IsAvailable(input.prevout.n));
    return coins->vout[input.prevout.n];
}

CAmount CStateViewCache::GetValueIn(const CTransaction& tx) const
{
    if (tx.IsCoinBase())
        return 0;

    CAmount nResult = 0;
    for (unsigned int i = 0; i < tx.vin.size(); i++)
        nResult += GetOutputFor(tx.vin[i]).nValue;

    return nResult;
}

bool CStateViewCache::HaveInputs(const CTransaction& tx) const
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

double CStateViewCache::GetPriority(const CTransaction &tx, int nHeight, CAmount &inChainInputValue) const
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

CCoinsModifier::CCoinsModifier(CStateViewCache& cache_, CCoinsMap::iterator it_, size_t usage) : cache(cache_), it(it_), cachedCoinUsage(usage) {
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

CProposalModifier::CProposalModifier(CStateViewCache& cache_, CProposalMap::iterator it_) : cache(cache_), it(it_) {
    assert(!cache.hasModifier);
    cache.hasModifier = true;
    prev = it->second;
}

CProposalModifier::~CProposalModifier()
{
    assert(cache.hasModifier);
    cache.hasModifier = false;

    if (it->second.IsNull()) {
        cache.cacheProposals[it->first].SetNull();
    }

    if (prev != it->second)
    {
        it->second.fDirty = true;
        LogPrint("dao", "%s: Modified %s: %s\n", __func__, it->first.ToString(), prev.diff(it->second));
    }
}

CPaymentRequestModifier::CPaymentRequestModifier(CStateViewCache& cache_, CPaymentRequestMap::iterator it_) : cache(cache_), it(it_) {
    assert(!cache.hasModifier);
    cache.hasModifier = true;
    prev = it->second;
}

CPaymentRequestModifier::~CPaymentRequestModifier()
{
    assert(cache.hasModifier);
    cache.hasModifier = false;

    if (it->second.IsNull()) {
        cache.cacheProposals[it->first].SetNull();
    }

    if (prev != it->second)
    {
        it->second.fDirty = true;
        LogPrint("dao", "%s: Modified %s: %s\n", __func__, it->first.ToString(), prev.diff(it->second));
    }
}

CVoteModifier::CVoteModifier(CStateViewCache& cache_, CVoteMap::iterator it_) : cache(cache_), it(it_) {
    assert(!cache.hasModifier);
    cache.hasModifier = true;
    prev = it->second;
}

CVoteModifier::~CVoteModifier()
{
    assert(cache.hasModifier);
    cache.hasModifier = false;

    if (it->second.IsNull()) {
        cache.cacheVotes[it->first].SetNull();
    }

    if (prev != it->second)
    {
        it->second.fDirty = true;
        LogPrint("dao", "%s: Modified %s: %s\n", __func__, HexStr(it->first), prev.diff(it->second));
    }
}

CConsultationModifier::CConsultationModifier(CStateViewCache& cache_, CConsultationMap::iterator it_) : cache(cache_), it(it_) {
    assert(!cache.hasModifier);
    cache.hasModifier = true;
    prev = it->second;
}

CConsultationModifier::~CConsultationModifier()
{
    assert(cache.hasModifier);
    cache.hasModifier = false;

    if (it->second.IsNull()) {
        cache.cacheConsultations[it->first].SetNull();
    }

    if (prev != it->second)
    {
        it->second.fDirty = true;
        LogPrint("dao", "%s: Modified %s: %s\n", __func__, it->first.ToString(), prev.diff(it->second));
    }
}

CConsultationAnswerModifier::CConsultationAnswerModifier(CStateViewCache& cache_, CConsultationAnswerMap::iterator it_) : cache(cache_), it(it_) {
    assert(!cache.hasModifier);
    cache.hasModifier = true;
    prev = it->second;
}

CConsultationAnswerModifier::~CConsultationAnswerModifier()
{
    assert(cache.hasModifier);
    cache.hasModifier = false;

    if (it->second.IsNull()) {
        cache.cacheAnswers[it->first].SetNull();
    }

    if (prev != it->second)
    {
        it->second.fDirty = true;
        LogPrint("dao", "%s: Modified %s: %s\n", __func__, it->first.ToString(), prev.diff(it->second));
    }
}

CStateViewCursor::~CStateViewCursor()
{
}
