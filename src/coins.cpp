// Copyright (c) 2012-2015 The Bitcoin Core developers
// Copyright (c) 2018-2020 The Navcoin Core developers
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
bool CStateView::GetConsensusParameter(const int &pid, CConsensusParameter& cparameter) const { return false; }
bool CStateView::GetToken(const uint256 &id, TokenInfo& token) const { return false; }
bool CStateView::GetNameRecord(const uint256 &id, NameRecordValue& height) const { return false; }
bool CStateView::GetNameData(const uint256 &id, NameDataValues& data) { return false; }
bool CStateView::HaveCoins(const uint256 &txid) const { return false; }
bool CStateView::HaveProposal(const uint256 &pid) const { return false; }
bool CStateView::HavePaymentRequest(const uint256 &prid) const { return false; }
bool CStateView::HaveCachedVoter(const CVoteMapKey &voter) const { return false; }
bool CStateView::HaveConsultation(const uint256 &cid) const { return false; }
bool CStateView::HaveConsultationAnswer(const uint256 &cid) const { return false; }
bool CStateView::HaveConsensusParameter(const int &pid) const { return false; }
bool CStateView::HaveToken(const uint256 &id) const { return false; }
bool CStateView::HaveNameRecord(const uint256 &id) const { return false; }
bool CStateView::HaveNameData(const uint256 &id) const { return false; }
bool CStateView::GetAllProposals(CProposalMap& map) { return false; }
int CStateView::GetExcludeVotes() const { return 0; }
bool CStateView::SetExcludeVotes(int count) { return 0; }
bool CStateView::GetAllPaymentRequests(CPaymentRequestMap& map) { return false; }
bool CStateView::GetAllVotes(CVoteMap& map) { return false; }
bool CStateView::GetAllConsultations(CConsultationMap& map) { return false; }
bool CStateView::GetAllConsultationAnswers(CConsultationAnswerMap& map) { return false; }
bool CStateView::GetAllTokens(TokenMap& map) { return false; }
bool CStateView::GetAllNameRecords(NameRecordMap& map) { return false; }
uint256 CStateView::GetBestBlock() const { return uint256(); }
bool CStateView::BatchWrite(CCoinsMap &mapCoins, CProposalMap &mapProposals,
                            CPaymentRequestMap &mapPaymentRequests, CVoteMap &mapVotes,
                            CConsultationMap& mapConsultations, CConsultationAnswerMap& mapAnswers,
                            CConsensusParameterMap& mapConsensus, TokenMap& mapTokens,
                            NameRecordMap& mapNameRecords, NameDataMap& mapNameData,
                            const uint256 &hashBlock, const int& nCacheExcludeVotes) { return false; }
CStateViewCursor *CStateView::Cursor() const { return 0; }


CStateViewBacked::CStateViewBacked(CStateView *viewIn) : base(viewIn) { }
bool CStateViewBacked::GetCoins(const uint256 &txid, CCoins &coins) const { return base->GetCoins(txid, coins); }
bool CStateViewBacked::GetProposal(const uint256 &pid, CProposal &proposal) const { return base->GetProposal(pid, proposal); }
bool CStateViewBacked::GetPaymentRequest(const uint256 &prid, CPaymentRequest &prequest) const { return base->GetPaymentRequest(prid, prequest); }
bool CStateViewBacked::GetConsultation(const uint256 &cid, CConsultation &consultation) const { return base->GetConsultation(cid, consultation); }
bool CStateViewBacked::GetConsultationAnswer(const uint256 &cid, CConsultationAnswer &answer) const { return base->GetConsultationAnswer(cid, answer); }
bool CStateViewBacked::GetConsensusParameter(const int &pid, CConsensusParameter& cparameter) const { return base->GetConsensusParameter(pid, cparameter); }
bool CStateViewBacked::GetToken(const uint256 &id, TokenInfo& token) const { return base->GetToken(id, token); }
bool CStateViewBacked::GetNameRecord(const uint256 &id, NameRecordValue& height) const { return base->GetNameRecord(id, height); }
bool CStateViewBacked::GetNameData(const uint256 &id, NameDataValues& data) { return base->GetNameData(id, data); }
bool CStateViewBacked::HaveCoins(const uint256 &txid) const { return base->HaveCoins(txid); }
bool CStateViewBacked::HaveProposal(const uint256 &pid) const { return base->HaveProposal(pid); }
bool CStateViewBacked::HavePaymentRequest(const uint256 &prid) const { return base->HavePaymentRequest(prid); }
bool CStateViewBacked::HaveCachedVoter(const CVoteMapKey &voter) const { return base->HaveCachedVoter(voter); }
bool CStateViewBacked::HaveConsultation(const uint256 &cid) const { return base->HaveConsultation(cid); }
bool CStateViewBacked::HaveConsultationAnswer(const uint256 &cid) const { return base->HaveConsultationAnswer(cid); }
bool CStateViewBacked::HaveConsensusParameter(const int &pid) const { return base->HaveConsensusParameter(pid); }
bool CStateViewBacked::HaveToken(const uint256 &id) const { return base->HaveToken(id); }
bool CStateViewBacked::HaveNameRecord(const uint256 &id) const { return base->HaveNameRecord(id); }
bool CStateViewBacked::HaveNameData(const uint256 &id) const { return base->HaveNameData(id); }
int CStateViewBacked::GetExcludeVotes() const { return base->GetExcludeVotes(); }
bool CStateViewBacked::SetExcludeVotes(int count) { return base->SetExcludeVotes(count); }
bool CStateViewBacked::GetCachedVoter(const CVoteMapKey &voter, CVoteMapValue& vote) const { return base->GetCachedVoter(voter, vote); }
bool CStateViewBacked::GetAllProposals(CProposalMap& map) { return base->GetAllProposals(map); }
bool CStateViewBacked::GetAllPaymentRequests(CPaymentRequestMap& map) { return base->GetAllPaymentRequests(map); }
bool CStateViewBacked::GetAllVotes(CVoteMap& map) { return base->GetAllVotes(map); }
bool CStateViewBacked::GetAllConsultations(CConsultationMap& map) { return base->GetAllConsultations(map); }
bool CStateViewBacked::GetAllConsultationAnswers(CConsultationAnswerMap& map) { return base->GetAllConsultationAnswers(map); }
bool CStateViewBacked::GetAllTokens(TokenMap& map) { return base->GetAllTokens(map); }
bool CStateViewBacked::GetAllNameRecords(NameRecordMap& map) { return base->GetAllNameRecords(map); }
uint256 CStateViewBacked::GetBestBlock() const { return base->GetBestBlock(); }
void CStateViewBacked::SetBackend(CStateView &viewIn) { base = &viewIn; }
bool CStateViewBacked::BatchWrite(CCoinsMap &mapCoins, CProposalMap &mapProposals,
                                  CPaymentRequestMap &mapPaymentRequests, CVoteMap &mapVotes,
                                  CConsultationMap &mapConsultations, CConsultationAnswerMap &mapAnswers,
                                  CConsensusParameterMap& mapConsensus, TokenMap& mapTokens,
                                  NameRecordMap& mapNameRecords, NameDataMap& mapNameData,
                                  const uint256 &hashBlock, const int &nCacheExcludeVotes) {
    return base->BatchWrite(mapCoins, mapProposals, mapPaymentRequests, mapVotes, mapConsultations, mapAnswers, mapConsensus, mapTokens, mapNameRecords, mapNameData, hashBlock, nCacheExcludeVotes);
}
CStateViewCursor *CStateViewBacked::Cursor() const { return base->Cursor(); }

SaltedTxidHasher::SaltedTxidHasher() : k0(GetRand(std::numeric_limits<uint64_t>::max())), k1(GetRand(std::numeric_limits<uint64_t>::max())) {}

CStateViewCache::CStateViewCache(CStateView *baseIn) : CStateViewBacked(baseIn), hasModifier(false), hasModifierConsensus(false), cachedCoinsUsage(0), nCacheExcludeVotes(-1) { }

CStateViewCache::~CStateViewCache()
{
    assert(!hasModifier);
    assert(!hasModifierConsensus);
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

CConsensusParameterMap::const_iterator CStateViewCache::FetchConsensusParameter(const int &pid) const {
    CConsensusParameterMap::iterator it = cacheConsensus.find(pid);

    if (it != cacheConsensus.end())
        return it;

    CConsensusParameter tmp;

    if (!base->GetConsensusParameter(pid, tmp) || tmp.IsNull())
        return cacheConsensus.end();

    CConsensusParameterMap::iterator ret = cacheConsensus.insert(std::make_pair(pid, CConsensusParameter())).first;
    tmp.swap(ret->second);

    return ret;
}

TokenMap::const_iterator CStateViewCache::FetchToken(const uint256 &id) const {
    TokenMap::iterator it = cacheTokens.find(id);

    if (it != cacheTokens.end())
        return it;

    TokenInfo tmp;

    if (!base->GetToken(id, tmp) || tmp.IsNull())
        return cacheTokens.end();

    TokenMap::iterator ret = cacheTokens.insert(std::make_pair(id, TokenInfo())).first;
    tmp.swap(ret->second);

    return ret;
}

NameRecordMap::const_iterator CStateViewCache::FetchNameRecord(const uint256 &id) const {
    NameRecordMap::iterator it = cacheNameRecords.find(id);

    if (it != cacheNameRecords.end())
    {
        return it;
    }

    NameRecordValue tmp;

    if (!base->GetNameRecord(id, tmp) || tmp.IsNull())
    {
        return cacheNameRecords.end();
    }

    NameRecordMap::iterator ret = cacheNameRecords.insert(std::make_pair(id, NameRecordValue())).first;
    tmp.swap(ret->second);

    return ret;
}

NameDataMap::const_iterator CStateViewCache::FetchNameData(const uint256 &id) const {
    NameDataMap::iterator it = cacheNameData.find(id);

    if (it != cacheNameData.end() && it->second.size() > 0)
        return it;

    NameDataValues tmp;

    if (!base->GetNameData(id, tmp) || tmp.size() == 0)
        return cacheNameData.end();

    NameDataMap::iterator ret = cacheNameData.insert(std::make_pair(id, NameDataValues())).first;
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

bool CStateViewCache::GetConsensusParameter(const int &pid, CConsensusParameter &cparameter) const {
    CConsensusParameterMap::const_iterator it = FetchConsensusParameter(pid);
    if (it != cacheConsensus.end() && !it->second.IsNull()) {
        cparameter = it->second;
        return true;
    }
    return false;
}

bool CStateViewCache::GetToken(const uint256 &id, TokenInfo &token) const {
    TokenMap::const_iterator it = FetchToken(id);
    if (it != cacheTokens.end() && !it->second.IsNull()) {
        token = it->second;
        return true;
    }
    return false;
}

bool CStateViewCache::GetNameRecord(const uint256 &id, NameRecordValue &height) const {
    NameRecordMap::const_iterator it = FetchNameRecord(id);
    if (it != cacheNameRecords.end() && !it->second.IsNull()) {
        height = it->second;
        return true;
    }

    return false;
}

bool CStateViewCache::GetNameData(const uint256 &id, NameDataValues &data) {
    NameDataMap::const_iterator it = FetchNameData(id);
    if (it != cacheNameData.end() && it->second.size() > 0) {
        data = it->second;
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
            mapProposal.insert(std::make_pair(it->first, it->second));

    for (auto it = mapProposal.begin(); it != mapProposal.end();)
        it->second.IsNull() ? mapProposal.erase(it++) : ++it;

    return true;
}

int CStateViewCache::GetExcludeVotes() const {
    if (nCacheExcludeVotes == -1) nCacheExcludeVotes = base->GetExcludeVotes();
    return nCacheExcludeVotes;
}

bool CStateViewCache::SetExcludeVotes(int count) {
    nCacheExcludeVotes = count;
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
            mapPaymentRequests.insert(std::make_pair(it->first, it->second));

    for (auto it = mapPaymentRequests.begin(); it != mapPaymentRequests.end();)
        it->second.IsNull() ? mapPaymentRequests.erase(it++) : ++it;

    return true;
}

bool CStateViewCache::GetAllTokens(TokenMap& mapTokens) {
    mapTokens.clear();
    mapTokens.insert(cacheTokens.begin(), cacheTokens.end());

    TokenMap baseMap;

    if (!base->GetAllTokens(baseMap))
        return false;

    for (TokenMap::iterator it = baseMap.begin(); it != baseMap.end(); it++)
        if (!it->second.IsNull())
            mapTokens.insert(std::make_pair(it->first, it->second));

    for (auto it = mapTokens.begin(); it != mapTokens.end();)
        it->second.IsNull() ? mapTokens.erase(it++) : ++it;

    return true;
}

bool CStateViewCache::GetAllNameRecords(NameRecordMap& mapNameRecords) {
    mapNameRecords.clear();
    mapNameRecords.insert(cacheNameRecords.begin(), cacheNameRecords.end());

    NameRecordMap baseMap;

    if (!base->GetAllNameRecords(baseMap))
        return false;

    for (NameRecordMap::iterator it = baseMap.begin(); it != baseMap.end(); it++)
        if (!it->second.IsNull())
            mapNameRecords.insert(std::make_pair(it->first, it->second));

    for (auto it = mapNameRecords.begin(); it != mapNameRecords.end();)
        it->second == 0 ? mapNameRecords.erase(it++) : ++it;

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
            mapVotes.insert(std::make_pair(it->first, it->second));

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
            mapConsultations.insert(std::make_pair(it->first, it->second));

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
            mapAnswers.insert(std::make_pair(it->first, it->second));

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

CProposalModifier CStateViewCache::ModifyProposal(const uint256 &pid, int nHeight) {
    assert(!hasModifier);
    std::pair<CProposalMap::iterator, bool> ret = cacheProposals.insert(std::make_pair(pid, CProposal()));
    if (ret.second) {
        if (!base->GetProposal(pid, ret.first->second)) {
            ret.first->second.SetNull();
        }
    }
    return CProposalModifier(*this, ret.first, nHeight);
}

CVoteModifier CStateViewCache::ModifyVote(const CVoteMapKey &voter, int nHeight) {
    assert(!hasModifier);
    std::pair<CVoteMap::iterator, bool> ret = cacheVotes.insert(std::make_pair(voter, CVoteList()));
    if (ret.second) {
        if (!base->GetCachedVoter(voter, ret.first->second)) {
            ret.first->second.SetNull();
        }
    }
    return CVoteModifier(*this, ret.first, nHeight);
}

CConsensusParameterModifier CStateViewCache::ModifyConsensusParameter(const int &pid, int nHeight) {
    assert(!hasModifierConsensus);
    std::pair<CConsensusParameterMap::iterator, bool> ret = cacheConsensus.insert(std::make_pair(pid, CConsensusParameter()));
    if (ret.second) {
        if (!base->GetConsensusParameter(pid, ret.first->second)) {
            ret.first->second.SetNull();
        }
    }
    return CConsensusParameterModifier(*this, ret.first, nHeight);
}

TokenModifier CStateViewCache::ModifyToken(const uint256 &id, int nHeight) {
    assert(!hasModifier);
    std::pair<TokenMap::iterator, bool> ret = cacheTokens.insert(std::make_pair(id, TokenInfo()));
    if (ret.second) {
        if (!base->GetToken(id, ret.first->second)) {
            ret.first->second.SetNull();
        }
    }
    return TokenModifier(*this, ret.first, nHeight);
}

NameRecordModifier CStateViewCache::ModifyNameRecord(const uint256 &id, int nHeight) {
    assert(!hasModifier);
    std::pair<NameRecordMap::iterator, bool> ret = cacheNameRecords.insert(std::make_pair(id, 0));
    if (ret.second) {
        if (!base->GetNameRecord(id, ret.first->second)) {
            ret.first->second = 0;
        }
    }
    return NameRecordModifier(*this, ret.first, nHeight);
}

NameDataModifier CStateViewCache::ModifyNameData(const uint256 &id, int nHeight) {
    assert(!hasModifier);
    std::pair<NameDataMap::iterator, bool> ret = cacheNameData.insert(std::make_pair(id, NameDataValues()));
    if (ret.second) {
        if (!base->GetNameData(id, ret.first->second)) {
            ret.first->second.clear();
        }
    }
    return NameDataModifier(*this, ret.first, nHeight);
}

CPaymentRequestModifier CStateViewCache::ModifyPaymentRequest(const uint256 &prid, int nHeight) {
    assert(!hasModifier);
    std::pair<CPaymentRequestMap::iterator, bool> ret = cachePaymentRequests.insert(std::make_pair(prid, CPaymentRequest()));
    if (ret.second) {
        if (!base->GetPaymentRequest(prid, ret.first->second)) {
            ret.first->second.SetNull();
        }
    }
    return CPaymentRequestModifier(*this, ret.first, nHeight);
}

CConsultationModifier CStateViewCache::ModifyConsultation(const uint256 &cid, int nHeight) {
    assert(!hasModifier);
    std::pair<CConsultationMap::iterator, bool> ret = cacheConsultations.insert(std::make_pair(cid, CConsultation()));
    if (ret.second) {
        if (!base->GetConsultation(cid, ret.first->second)) {
            ret.first->second.SetNull();
        }
    }
    return CConsultationModifier(*this, ret.first, nHeight);
}

CConsultationAnswerModifier CStateViewCache::ModifyConsultationAnswer(const uint256 &cid, int nHeight) {
    assert(!hasModifier);
    std::pair<CConsultationAnswerMap::iterator, bool> ret = cacheAnswers.insert(std::make_pair(cid, CConsultationAnswer()));
    if (ret.second) {
        if (!base->GetConsultationAnswer(cid, ret.first->second)) {
            ret.first->second.SetNull();
        }
    }
    return CConsultationAnswerModifier(*this, ret.first, nHeight);
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

    assert(proposal.fDirty == true);

    if (cacheProposals.count(proposal.hash))
        cacheProposals[proposal.hash]=proposal;
    else
        cacheProposals.insert(std::make_pair(proposal.hash, proposal));

    return true;
}

bool CStateViewCache::AddCachedVoter(const CVoteMapKey &voter, CVoteMapValue& vote) const {
    if (HaveCachedVoter(voter))
        return false;

    assert(vote.fDirty == true);

    if (cacheVotes.count(voter))
        cacheVotes[voter]=vote;
    else
        cacheVotes.insert(std::make_pair(voter, vote));

    return true;
}

bool CStateViewCache::AddPaymentRequest(const CPaymentRequest& prequest) const {
    if (HavePaymentRequest(prequest.hash))
        return false;

    assert(prequest.fDirty == true);

    if (cachePaymentRequests.count(prequest.hash))
        cachePaymentRequests[prequest.hash]=prequest;
    else
        cachePaymentRequests.insert(std::make_pair(prequest.hash, prequest));

    return true;
}

bool CStateViewCache::AddConsultation(const CConsultation& consultation) const {
    if (HaveConsultation(consultation.hash))
        return false;

    assert(consultation.fDirty == true);

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

    assert(answer.fDirty == true);

    if (cacheAnswers.count(answer.hash))
        cacheAnswers[answer.hash]=answer;
    else
        cacheAnswers.insert(std::make_pair(answer.hash, answer));

    return true;
}

bool CStateViewCache::AddToken(const Token& token) const {
    if (HaveToken(token.first))
        return false;

    assert(token.second.fDirty == true);

    if (cacheTokens.count(token.first))
        cacheTokens[token.first]=token.second;
    else
        cacheTokens.insert(std::make_pair(token.first, token.second));

    return true;
}

bool CStateViewCache::AddNameRecord(const NameRecord& namerecord) const {
    if (HaveNameRecord(namerecord.first))
        return false;

    if (cacheNameRecords.count(namerecord.first))
        cacheNameRecords[namerecord.first]=namerecord.second;
    else
        cacheNameRecords.insert(std::make_pair(namerecord.first, namerecord.second));

    return true;
}

bool CStateViewCache::AddNameData(const uint256& id, const NameDataEntry& namerecord) const {
    if (cacheNameData.count(id)) {
        cacheNameData[id].erase(
            std::remove_if(cacheNameData[id].begin(), cacheNameData[id].end(),
                [&namerecord](const NameDataEntry & o) { return o.first == namerecord.first && o.second.IsNull(); }),
            cacheNameData[id].end());
        cacheNameData[id].push_back(namerecord);
    }
    else
    {
        cacheNameData.insert(std::make_pair(id, NameDataValues()));
        cacheNameData[id].push_back(namerecord);
    }

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

bool CStateViewCache::RemoveToken(const uint256 &id) const {
    if (!HaveToken(id))
        return false;

    cacheTokens[id] = TokenInfo();
    cacheTokens[id].SetNull();

    assert(cacheTokens[id].IsNull());

    return true;
}

bool CStateViewCache::RemoveNameRecord(const uint256 &id) const {
    if (!HaveNameRecord(id))
        return false;

    cacheNameRecords[id] = NameRecordValue();

    assert(cacheNameRecords[id].IsNull());

    return true;
}

bool CStateViewCache::RemoveNameData(const NameDataKey &id) const {
    if (!HaveNameData(id.id))
        return false;

    if (cacheNameData.count(id.id))
    {
        NameDataValues temp;

        for (auto& it: cacheNameData[id.id]) {
            if (it.first == id.height) {
                temp.push_back(NameDataEntry(id.height, NameDataValue()));
            }
            else {
                temp.push_back(it);
            }
        }

        cacheNameData[id.id] = temp;
    }

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

bool CStateViewCache::HaveConsensusParameter(const int &pid) const {
    CConsensusParameterMap::const_iterator it = FetchConsensusParameter(pid);
    return (it != cacheConsensus.end() && !it->second.IsNull());
}

bool CStateViewCache::HaveToken(const uint256 &id) const {
    TokenMap::const_iterator it = FetchToken(id);
    return (it != cacheTokens.end() && !it->second.IsNull());
}

bool CStateViewCache::HaveNameRecord(const uint256 &id) const {
    NameRecordMap::const_iterator it = FetchNameRecord(id);
    return (it != cacheNameRecords.end() && !it->second.IsNull());
}

bool CStateViewCache::HaveNameData(const uint256 &id) const {
    NameDataMap::const_iterator it = FetchNameData(id);
    return (it != cacheNameData.end() && it->second.size());
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
                                 CConsensusParameterMap& mapConsensus, TokenMap& mapTokens, NameRecordMap& mapNameRecords,
                                 NameDataMap& mapNameData, const uint256 &hashBlockIn, const int &nCacheExcludeVotesIn) {
    assert(!hasModifier);
    assert(!hasModifierConsensus);
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
        if (it->second.fDirty) { // Ignore non-dirty entries (optimization).
            CProposal& entry = cacheProposals[it->first];
            entry.swap(it->second);
        }
        CProposalMap::iterator itOld = it++;
        mapProposals.erase(itOld);
    }

    for (CPaymentRequestMap::iterator it = mapPaymentRequests.begin(); it != mapPaymentRequests.end();) {
        if (it->second.fDirty) { // Ignore non-dirty entries (optimization).
            CPaymentRequest& entry = cachePaymentRequests[it->first];
            entry.swap(it->second);
        }
        CPaymentRequestMap::iterator itOld = it++;
        mapPaymentRequests.erase(itOld);
    }

    for (CVoteMap::iterator it = mapVotes.begin(); it != mapVotes.end();){
        if (it->second.fDirty) { // Ignore non-dirty entries (optimization).
            CVoteList& entry = cacheVotes[it->first];
            entry.swap(it->second);
        }
        CVoteMap::iterator itOld = it++;
        mapVotes.erase(itOld);
    }

    for (CConsultationMap::iterator it = mapConsultations.begin(); it != mapConsultations.end();) {
        if (it->second.fDirty) { // Ignore non-dirty entries (optimization).
            CConsultation& entry = cacheConsultations[it->first];
            entry.swap(it->second);
        }
        CConsultationMap::iterator itOld = it++;
        mapConsultations.erase(itOld);
    }

    for (CConsultationAnswerMap::iterator it = mapAnswers.begin(); it != mapAnswers.end();) {
        if (it->second.fDirty) { // Ignore non-dirty entries (optimization).
            CConsultationAnswer& entry = cacheAnswers[it->first];
            entry.swap(it->second);
        }
        CConsultationAnswerMap::iterator itOld = it++;
        mapAnswers.erase(itOld);
    }

    for (CConsensusParameterMap::iterator it = mapConsensus.begin(); it != mapConsensus.end();) {
        if (it->second.fDirty) { // Ignore non-dirty entries (optimization).
            CConsensusParameter& entry = cacheConsensus[it->first];
            entry.swap(it->second);
        }
        CConsensusParameterMap::iterator itOld = it++;
        mapConsensus.erase(itOld);
    }

    for (TokenMap::iterator it = mapTokens.begin(); it != mapTokens.end();) {
        TokenInfo& entry = cacheTokens[it->first];
        entry.swap(it->second);
        TokenMap::iterator itOld = it++;
        mapTokens.erase(itOld);
    }

    for (NameRecordMap::iterator it = mapNameRecords.begin(); it != mapNameRecords.end();) {
        NameRecordValue& entry = cacheNameRecords[it->first];
        entry.swap(it->second);
        NameRecordMap::iterator itOld = it++;
        mapNameRecords.erase(itOld);
    }

    for (NameDataMap::iterator it = mapNameData.begin(); it != mapNameData.end();) {
        NameDataValues& entry = cacheNameData[it->first];
        entry.swap(it->second);
        NameDataMap::iterator itOld = it++;
        mapNameData.erase(itOld);
    }

    hashBlock = hashBlockIn;
    nCacheExcludeVotes = nCacheExcludeVotesIn;
    return true;
}

bool CStateViewCache::Flush() {
    bool fOk = base->BatchWrite(cacheCoins, cacheProposals, cachePaymentRequests, cacheVotes, cacheConsultations, cacheAnswers, cacheConsensus, cacheTokens, cacheNameRecords, cacheNameData, hashBlock, nCacheExcludeVotes);
    cacheCoins.clear();
    cacheProposals.clear();
    cachePaymentRequests.clear();
    cacheVotes.clear();
    cacheConsultations.clear();
    cacheAnswers.clear();
    cacheConsensus.clear();
    cacheTokens.clear();
    cacheNameRecords.clear();
    cacheNameData.clear();
    cachedCoinsUsage = 0;
    nCacheExcludeVotes = -1;
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
    {
        nResult += GetOutputFor(tx.vin[i]).nValue;
    }

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

CProposalModifier::CProposalModifier(CStateViewCache& cache_, CProposalMap::iterator it_, int height_) : cache(cache_), it(it_), height(height_) {
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
        LogPrint("daoextra", "%s: Modified %s%s: %s\n", __func__, height>0?strprintf("at height %d ",height):"",it->first.ToString(), prev.diff(it->second));
    }
}

CPaymentRequestModifier::CPaymentRequestModifier(CStateViewCache& cache_, CPaymentRequestMap::iterator it_, int height_) : cache(cache_), it(it_), height(height_) {
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
        LogPrint("daoextra", "%s: Modified %s%s: %s\n", __func__, height>0?strprintf("at height %d ",height):"",it->first.ToString(), prev.diff(it->second));
    }
}

CVoteModifier::CVoteModifier(CStateViewCache& cache_, CVoteMap::iterator it_, int height_) : cache(cache_), it(it_), height(height_) {
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
        LogPrint("daoextra", "%s: Modified %s%s: %s\n", __func__, height>0?strprintf("at height %d ",height):"", HexStr(it->first), prev.diff(it->second));
    }
}

CConsultationModifier::CConsultationModifier(CStateViewCache& cache_, CConsultationMap::iterator it_, int height_) : cache(cache_), it(it_), height(height_) {
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
        LogPrint("daoextra", "%s: Modified %s%s: %s\n", __func__, height>0?strprintf("at height %d ",height):"",it->first.ToString(), prev.diff(it->second));
    }
}

CConsultationAnswerModifier::CConsultationAnswerModifier(CStateViewCache& cache_, CConsultationAnswerMap::iterator it_, int height_) : cache(cache_), it(it_), height(height_) {
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
        LogPrint("daoextra", "%s: Modified %s%s: %s\n", __func__, height>0?strprintf("at height %d ",height):"",it->first.ToString(), prev.diff(it->second));
    }
}

CConsensusParameterModifier::CConsensusParameterModifier(CStateViewCache& cache_, CConsensusParameterMap::iterator it_, int height_) : cache(cache_), it(it_), height(height_) {
    assert(!cache.hasModifierConsensus);
    cache.hasModifierConsensus = true;
    prev = it->second;
}

CConsensusParameterModifier::~CConsensusParameterModifier()
{
    assert(cache.hasModifierConsensus);
    cache.hasModifierConsensus = false;

    if (it->second.IsNull()) {
        cache.cacheConsensus[it->first].SetNull();
    }

    if (prev != it->second)
    {
        it->second.fDirty = true;
        LogPrint("daoextra", "%s: Modified %sconsensus parameter %d: %s\n", __func__, height>0?strprintf("at height %d ",height):"",it->first, prev.diff(it->second));
    }
}

TokenModifier::TokenModifier(CStateViewCache& cache_, TokenMap::iterator it_, int height_) : cache(cache_), it(it_), height(height_) {
    assert(!cache.hasModifier);
    cache.hasModifier = true;
    prev = it->second;
}

TokenModifier::~TokenModifier()
{
    assert(cache.hasModifier);
    cache.hasModifier = false;

    if (it->second.IsNull()) {
        cache.cacheTokens[it->first].SetNull();
    }

    if (!(prev == it->second))
    {
        it->second.fDirty = true;
        LogPrint("daoextra", "%s: Modified %stoken %s\n", __func__, height>0?strprintf("at height %d ",height):"",it->first.ToString());
    }
}

NameRecordModifier::NameRecordModifier(CStateViewCache& cache_, NameRecordMap::iterator it_, int height_) : cache(cache_), it(it_), height(height_) {
    assert(!cache.hasModifier);
    cache.hasModifier = true;
    prev = it->second;
}

NameRecordModifier::~NameRecordModifier()
{
    assert(cache.hasModifier);
    cache.hasModifier = false;

    if (it->second.IsNull()) {
        cache.cacheNameRecords[it->first].SetNull();
    }

    if (!(prev == it->second))
    {
        LogPrint("daoextra", "%s: Modified %sname record %s\n", __func__, height>0?strprintf("at height %d ",height):"",it->first.ToString());
    }
}

NameDataModifier::NameDataModifier(CStateViewCache& cache_, NameDataMap::iterator it_, int height_) : cache(cache_), it(it_), height(height_) {
    assert(!cache.hasModifier);
    cache.hasModifier = true;
    prev = it->second;
}

NameDataModifier::~NameDataModifier()
{
    assert(cache.hasModifier);
    cache.hasModifier = false;

    if (it->second.size() == 0) {
        cache.cacheNameData[it->first].clear();
    }

    if (!(prev == it->second))
    {
        LogPrint("daoextra", "%s: Modified %sname data %s\n", __func__, height>0?strprintf("at height %d ",height):"",it->first.ToString());
    }
}

CStateViewCursor::~CStateViewCursor()
{
}
