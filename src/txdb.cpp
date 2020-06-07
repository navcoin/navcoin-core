// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <txdb.h>
#include <ui_interface.h>

#include <chainparams.h>
#include <hash.h>
#include <pow.h>
#include <uint256.h>

#include <stdint.h>

#include <boost/thread.hpp>

using namespace std;

static const char DB_COINS = 'c';
static const char DB_BLOCK_FILES = 'f';
static const char DB_TXINDEX = 't';
static const char DB_PROPINDEX = 'o';
static const char DB_PREQINDEX = 'r';
static const char DB_ADDRESSINDEX = 'a';
static const char DB_ADDRESSUNSPENTINDEX = 'u';
static const char DB_TIMESTAMPINDEX = 's';
static const char DB_BLOCKHASHINDEX = 'z';
static const char DB_SPENTINDEX = 'q';
static const char DB_BLOCK_INDEX = 'b';

static const char DB_VOTEINDEX = 'C';
static const char DB_CONSULTINDEX = 'K';
static const char DB_ANSWERINDEX = 'A';
static const char DB_CONSENSUSINDEX = 'p';

static const char DB_BEST_BLOCK = 'B';
static const char DB_FLAG = 'F';
static const char DB_REINDEX_FLAG = 'R';
static const char DB_LAST_BLOCK = 'l';

CStateViewDB::CStateViewDB(size_t nCacheSize, bool fMemory, bool fWipe) : db(GetDataDir() / "chainstate", nCacheSize, fMemory, fWipe, true, false, 64)
{
}

bool CStateViewDB::GetCoins(const uint256 &txid, CCoins &coins) const {
    return db.Read(make_pair(DB_COINS, txid), coins);
}

bool CStateViewDB::HaveCoins(const uint256 &txid) const {
    return db.Exists(make_pair(DB_COINS, txid));
}

bool CStateViewDB::GetProposal(const uint256 &pid, CProposal &proposal) const {
    return db.Read(make_pair(DB_PROPINDEX, pid), proposal);
}

bool CStateViewDB::HaveProposal(const uint256 &pid) const {
    return db.Exists(make_pair(DB_PROPINDEX, pid));
}

bool CStateViewDB::GetPaymentRequest(const uint256 &prid, CPaymentRequest &prequest) const {
    return db.Read(make_pair(DB_PREQINDEX, prid), prequest);
}

bool CStateViewDB::HavePaymentRequest(const uint256 &prid) const {
    return db.Exists(make_pair(DB_PREQINDEX, prid));
}

bool CStateViewDB::GetCachedVoter(const CVoteMapKey &voter, CVoteMapValue &vote) const {
    return db.Read(make_pair(DB_VOTEINDEX, voter), vote);
}

bool CStateViewDB::HaveCachedVoter(const CVoteMapKey &voter) const {
    return db.Exists(make_pair(DB_VOTEINDEX, voter));
}

bool CStateViewDB::GetConsultation(const uint256 &cid, CConsultation &consultation) const {
    return db.Read(make_pair(DB_CONSULTINDEX, cid), consultation);
}

bool CStateViewDB::HaveConsultation(const uint256 &cid) const {
    return db.Exists(make_pair(DB_CONSULTINDEX, cid));
}

bool CStateViewDB::GetConsultationAnswer(const uint256 &aid, CConsultationAnswer &answer) const {
    return db.Read(make_pair(DB_ANSWERINDEX, aid), answer);
}

bool CStateViewDB::HaveConsultationAnswer(const uint256 &aid) const {
    return db.Exists(make_pair(DB_ANSWERINDEX, aid));
}

bool CStateViewDB::GetConsensusParameter(const int &pid, CConsensusParameter& cparameter) const {
    return db.Read(make_pair(DB_CONSENSUSINDEX, pid), cparameter);
}

bool CStateViewDB::HaveConsensusParameter(const int &pid) const {
    return db.Exists(make_pair(DB_CONSENSUSINDEX, pid));
}

uint256 CStateViewDB::GetBestBlock() const {
    uint256 hashBestChain;
    if (!db.Read(DB_BEST_BLOCK, hashBestChain))
        return uint256();
    return hashBestChain;
}

bool CStateViewDB::GetAllProposals(CProposalMap& map) {
    map.clear();

    boost::scoped_ptr<CDBIterator> pcursor(db.NewIterator());

    pcursor->Seek(make_pair(DB_PROPINDEX, uint256()));

    while (pcursor->Valid()) {
        boost::this_thread::interruption_point();
        std::pair<char, uint256> key;
        if (pcursor->GetKey(key) && key.first == DB_PROPINDEX) {
            CProposal proposal;
            if (pcursor->GetValue(proposal)) {
                map.insert(make_pair(key.second, proposal));
                pcursor->Next();
            } else {
                return error("GetAllProposals() : failed to read value");
            }
        } else {
            break;
        }
    }

    return true;
}

bool CStateViewDB::GetAllPaymentRequests(CPaymentRequestMap &map) {
    map.clear();

    boost::scoped_ptr<CDBIterator> pcursor(db.NewIterator());

    pcursor->Seek(make_pair(DB_PREQINDEX, uint256()));

    while (pcursor->Valid()) {
        boost::this_thread::interruption_point();
        std::pair<char, uint256> key;
        if (pcursor->GetKey(key) && key.first == DB_PREQINDEX) {
            CPaymentRequest prequest;
            if (pcursor->GetValue(prequest)) {
                map.insert(make_pair(key.second, prequest));
                pcursor->Next();
            } else {
                return error("GetAllPaymentRequests() : failed to read value");
            }
        } else {
            break;
        }
    }

    return true;
}

bool CStateViewDB::GetAllVotes(CVoteMap &map) {
    map.clear();

    boost::scoped_ptr<CDBIterator> pcursor(db.NewIterator());

    pcursor->Seek(make_pair(DB_VOTEINDEX, uint256()));

    while (pcursor->Valid()) {
        boost::this_thread::interruption_point();
        std::pair<char, CVoteMapKey> key;
        if (pcursor->GetKey(key) && key.first == DB_VOTEINDEX) {
            CVoteMapValue vote;
            if (pcursor->GetValue(vote)) {
                map.insert(make_pair(key.second, vote));
                pcursor->Next();
            } else {
                return error("GetAllVotes() : failed to read value");
            }
        } else {
            break;
        }
    }

    return true;
}

bool CStateViewDB::GetAllConsultations(CConsultationMap &map) {
    map.clear();

    boost::scoped_ptr<CDBIterator> pcursor(db.NewIterator());

    pcursor->Seek(make_pair(DB_CONSULTINDEX, uint256()));

    while (pcursor->Valid()) {
        boost::this_thread::interruption_point();
        std::pair<char, uint256> key;
        if (pcursor->GetKey(key) && key.first == DB_CONSULTINDEX) {
            CConsultation consultation;
            if (pcursor->GetValue(consultation)) {
                map.insert(make_pair(key.second, consultation));
                pcursor->Next();
            } else {
                return error("GetAllConsultations() : failed to read value");
            }
        } else {
            break;
        }
    }

    return true;
}

bool CStateViewDB::GetAllConsultationAnswers(CConsultationAnswerMap &map) {
    map.clear();

    boost::scoped_ptr<CDBIterator> pcursor(db.NewIterator());

    pcursor->Seek(make_pair(DB_ANSWERINDEX, uint256()));

    while (pcursor->Valid()) {
        boost::this_thread::interruption_point();
        std::pair<char, uint256> key;
        if (pcursor->GetKey(key) && key.first == DB_ANSWERINDEX) {
            CConsultationAnswer answer;
            if (pcursor->GetValue(answer)) {
                map.insert(make_pair(key.second, answer));
                pcursor->Next();
            } else {
                return error("GetAllConsultationAnswers() : failed to read value");
            }
        } else {
            break;
        }
    }

    return true;
}

bool CStateViewDB::BatchWrite(CCoinsMap &mapCoins, CProposalMap &mapProposals,
                              CPaymentRequestMap &mapPaymentRequests, CVoteMap &mapVotes,
                              CConsultationMap &mapConsultations, CConsultationAnswerMap &mapAnswers,
                              CConsensusParameterMap &mapConsensus,
                              const uint256 &hashBlock) {

    CDBBatch batch(db);
    size_t count = 0;
    size_t changed = 0;
    for (CCoinsMap::iterator it = mapCoins.begin(); it != mapCoins.end();) {
        if (it->second.flags & CCoinsCacheEntry::DIRTY) {
            if (it->second.coins.IsPruned())
                batch.Erase(make_pair(DB_COINS, it->first));
            else
                batch.Write(make_pair(DB_COINS, it->first), it->second.coins);
            changed++;
        }
        count++;
        CCoinsMap::iterator itOld = it++;
        mapCoins.erase(itOld);
    }

    for (CProposalMap::iterator it = mapProposals.begin(); it != mapProposals.end();) {
        if (it->second.fDirty)
        {
            if (it->second.IsNull())
                batch.Erase(make_pair(DB_PROPINDEX, it->first));
            else
                batch.Write(make_pair(DB_PROPINDEX, it->first), it->second);
        }
        CProposalMap::iterator itOld = it++;
        mapProposals.erase(itOld);
    }

    for (CConsensusParameterMap::iterator it = mapConsensus.begin(); it != mapConsensus.end();) {
        if (it->second.fDirty)
        {
            if (it->second.IsNull())
                batch.Erase(make_pair(DB_CONSENSUSINDEX, it->first));
            else
                batch.Write(make_pair(DB_CONSENSUSINDEX, it->first), it->second);
        }
        CConsensusParameterMap::iterator itOld = it++;
        mapConsensus.erase(itOld);
    }

    for (CPaymentRequestMap::iterator it = mapPaymentRequests.begin(); it != mapPaymentRequests.end();) {
        if (it->second.fDirty)
        {
            if (it->second.IsNull())
                batch.Erase(make_pair(DB_PREQINDEX, it->first));
            else
                batch.Write(make_pair(DB_PREQINDEX, it->first), it->second);
        }
        CPaymentRequestMap::iterator itOld = it++;
        mapPaymentRequests.erase(itOld);
    }

    for (CConsultationMap::iterator it = mapConsultations.begin(); it != mapConsultations.end();) {
        if (it->second.fDirty)
        {
            if (it->second.IsNull())
                batch.Erase(make_pair(DB_CONSULTINDEX, it->first));
            else
                batch.Write(make_pair(DB_CONSULTINDEX, it->first), it->second);
        }
        CConsultationMap::iterator itOld = it++;
        mapConsultations.erase(itOld);
    }

    for (CConsultationAnswerMap::iterator it = mapAnswers.begin(); it != mapAnswers.end();) {
        if (it->second.fDirty)
        {
            if (it->second.IsNull())
                batch.Erase(make_pair(DB_ANSWERINDEX, it->first));
            else
                batch.Write(make_pair(DB_ANSWERINDEX, it->first), it->second);
        }
        CConsultationAnswerMap::iterator itOld = it++;
        mapAnswers.erase(itOld);
    }

    for (CVoteMap::iterator it = mapVotes.begin(); it != mapVotes.end();) {
        if (it->second.fDirty)
        {
            if (it->second.IsNull())
                batch.Erase(make_pair(DB_VOTEINDEX, it->first));
            else
                batch.Write(make_pair(DB_VOTEINDEX, it->first), it->second);
        }
        CVoteMap::iterator itOld = it++;
        mapVotes.erase(itOld);
    }

    if (!hashBlock.IsNull())
        batch.Write(DB_BEST_BLOCK, hashBlock);

    LogPrint("coindb", "Committing %u changed transactions (out of %u) to coin database...\n", (unsigned int)changed, (unsigned int)count);
    return db.WriteBatch(batch);
}

CBlockTreeDB::CBlockTreeDB(size_t nCacheSize, bool fMemory, bool fWipe, bool compression, int maxOpenFiles) : CDBWrapper(GetDataDir() / "blocks" / "index", nCacheSize, fMemory, fWipe, false, compression, maxOpenFiles) {
}

bool CBlockTreeDB::ReadBlockFileInfo(int nFile, CBlockFileInfo &info) {
    return Read(make_pair(DB_BLOCK_FILES, nFile), info);
}

bool CBlockTreeDB::WriteReindexing(bool fReindexing) {
    if (fReindexing)
        return Write(DB_REINDEX_FLAG, '1');
    else
        return Erase(DB_REINDEX_FLAG);
}

bool CBlockTreeDB::ReadReindexing(bool &fReindexing) {
    fReindexing = Exists(DB_REINDEX_FLAG);
    return true;
}

bool CBlockTreeDB::ReadLastBlockFile(int &nFile) {
    return Read(DB_LAST_BLOCK, nFile);
}

CStateViewCursor *CStateViewDB::Cursor() const
{
    CStateViewDBCursor *i = new CStateViewDBCursor(const_cast<CDBWrapper*>(&db)->NewIterator(), GetBestBlock());
    /* It seems that there are no "const iterators" for LevelDB.  Since we
       only need read operations on it, use a const-cast to get around
       that restriction.  */
    i->pcursor->Seek(DB_COINS);
    // Cache key of first record
    i->pcursor->GetKey(i->keyTmp);
    return i;
}

bool CStateViewDBCursor::GetKey(uint256 &key) const
{
    // Return cached key
    if (keyTmp.first == DB_COINS) {
        key = keyTmp.second;
        return true;
    }
    return false;
}

bool CStateViewDBCursor::GetValue(CCoins &coins) const
{
    return pcursor->GetValue(coins);
}

unsigned int CStateViewDBCursor::GetValueSize() const
{
    return pcursor->GetValueSize();
}

bool CStateViewDBCursor::Valid() const
{
    return keyTmp.first == DB_COINS;
}

void CStateViewDBCursor::Next()
{
    pcursor->Next();
    if (!pcursor->Valid() || !pcursor->GetKey(keyTmp))
        keyTmp.first = 0; // Invalidate cached key after last record so that Valid() and GetKey() return false
}

bool CBlockTreeDB::WriteBatchSync(const std::vector<std::pair<int, const CBlockFileInfo*> >& fileInfo, int nLastFile, const std::vector<const CBlockIndex*>& blockinfo) {
    CDBBatch batch(*this);
    for (std::vector<std::pair<int, const CBlockFileInfo*> >::const_iterator it=fileInfo.begin(); it != fileInfo.end(); it++) {
        batch.Write(make_pair(DB_BLOCK_FILES, it->first), *it->second);
    }
    batch.Write(DB_LAST_BLOCK, nLastFile);
    for (std::vector<const CBlockIndex*>::const_iterator it=blockinfo.begin(); it != blockinfo.end(); it++) {
        CDiskBlockIndex dbi(*it);
        auto pVotes = GetProposalVotes(dbi.GetBlockHash());
        if (pVotes != nullptr)
        {
            dbi.vProposalVotes = *pVotes;
        }
        auto prVotes = GetPaymentRequestVotes(dbi.GetBlockHash());
        if (prVotes != nullptr)
        {
            dbi.vPaymentRequestVotes = *prVotes;
        }
        auto supp = GetSupport(dbi.GetBlockHash());
        if (supp != nullptr)
        {
            dbi.mapSupport = *supp;
        }
        auto cVotes = GetConsultationVotes(dbi.GetBlockHash());
        if (cVotes != nullptr)
        {
            dbi.mapConsultationVotes = *cVotes;
        }
        batch.Write(make_pair(DB_BLOCK_INDEX, (*it)->GetBlockHash()), dbi);
    }
    return WriteBatch(batch, true);
}

bool CBlockTreeDB::ReadTxIndex(const uint256 &txid, CDiskTxPos &pos) {
    return Read(make_pair(DB_TXINDEX, txid), pos);
}

bool CBlockTreeDB::WriteTxIndex(const std::vector<std::pair<uint256, CDiskTxPos> >&vect) {
    CDBBatch batch(*this);
    for (std::vector<std::pair<uint256,CDiskTxPos> >::const_iterator it=vect.begin(); it!=vect.end(); it++)
        batch.Write(make_pair(DB_TXINDEX, it->first), it->second);
    return WriteBatch(batch);
}

bool CBlockTreeDB::ReadProposalIndex(const uint256 &proposalid, CProposal &proposal) {
    return Read(make_pair(DB_PROPINDEX, proposalid), proposal);
}

bool CBlockTreeDB::WriteProposalIndex(const std::vector<std::pair<uint256, CProposal> >&vect) {
    CDBBatch batch(*this);
    for (std::vector<std::pair<uint256,CProposal> >::const_iterator it=vect.begin(); it!=vect.end(); it++)
        batch.Write(make_pair(DB_PROPINDEX, it->first), it->second);
    return WriteBatch(batch);
}

bool CBlockTreeDB::UpdateProposalIndex(const std::vector<std::pair<uint256, CProposal> >&vect) {
    CDBBatch batch(*this);
    for (std::vector<std::pair<uint256,CProposal> >::const_iterator it=vect.begin(); it!=vect.end(); it++) {
        if (it->second.IsNull()) {
            batch.Erase(make_pair(DB_PROPINDEX, it->first));
        } else {
            batch.Write(make_pair(DB_PROPINDEX, it->first), it->second);
        }
    }
    return WriteBatch(batch, true);
}

bool CBlockTreeDB::GetProposalIndex(std::vector<CProposal>&vect) {
    boost::scoped_ptr<CDBIterator> pcursor(NewIterator());

    pcursor->Seek(make_pair(DB_PROPINDEX, uint256()));

    while (pcursor->Valid()) {
        boost::this_thread::interruption_point();
        std::pair<char, uint256> key;
        if (pcursor->GetKey(key) && key.first == DB_PROPINDEX) {
            CProposal proposal;
            if (pcursor->GetValue(proposal)) {
                vect.push_back(proposal);
                pcursor->Next();
            } else {
                return error("GetProposalIndex() : failed to read value");
            }
        } else {
            break;
        }
    }

    std::sort(vect.begin(), vect.end(), make_member_comparer<std::greater>(&CProposal::nFee));

    return true;
}

CProposal CBlockTreeDB::GetProposal(uint256 hash)
{
    boost::scoped_ptr<CDBIterator> pcursor(NewIterator());

    pcursor->Seek(make_pair(DB_PROPINDEX, uint256()));

    while (pcursor->Valid()) {
        boost::this_thread::interruption_point();
        std::pair<char, uint256> key;
        if (pcursor->GetKey(key) && key.first == DB_PROPINDEX) {
            CProposal proposal;
            if (pcursor->GetValue(proposal)) {
                if(proposal.hash == hash){
                    return proposal;

                }
            pcursor->Next();
            } else {
                return CProposal();
            }
        } else {
            break;
        }
    }

    return CProposal();
}

bool CBlockTreeDB::ReadPaymentRequestIndex(const uint256 &prequestid, CPaymentRequest &prequest) {
    return Read(make_pair(DB_PREQINDEX, prequestid), prequest);
}

bool CBlockTreeDB::WritePaymentRequestIndex(const std::vector<std::pair<uint256, CPaymentRequest> >&vect) {
    CDBBatch batch(*this);
    for (std::vector<std::pair<uint256,CPaymentRequest> >::const_iterator it=vect.begin(); it!=vect.end(); it++)
        batch.Write(make_pair(DB_PREQINDEX, it->first), it->second);
    return WriteBatch(batch);
}

bool CBlockTreeDB::UpdatePaymentRequestIndex(const std::vector<std::pair<uint256, CPaymentRequest> >&vect) {
    CDBBatch batch(*this);
    for (std::vector<std::pair<uint256,CPaymentRequest> >::const_iterator it=vect.begin(); it!=vect.end(); it++) {
        if (it->second.IsNull()) {
            batch.Erase(make_pair(DB_PREQINDEX, it->first));
        } else {
            batch.Write(make_pair(DB_PREQINDEX, it->first), it->second);
        }
    }
    return WriteBatch(batch);
}

bool CBlockTreeDB::GetPaymentRequestIndex(std::vector<CPaymentRequest>&vect) {
    boost::scoped_ptr<CDBIterator> pcursor(NewIterator());

    pcursor->Seek(make_pair(DB_PREQINDEX, uint256()));

    while (pcursor->Valid()) {
        boost::this_thread::interruption_point();
        std::pair<char, uint256> key;
        if (pcursor->GetKey(key) && key.first == DB_PREQINDEX) {
            CPaymentRequest prequest;
            if (pcursor->GetValue(prequest)) {
                vect.push_back(prequest);
                pcursor->Next();
            } else {
                return error("GetPaymentRequestIndex() : failed to read value");
            }
        } else {
            break;
        }
    }

    return true;
}

bool CBlockTreeDB::ReadSpentIndex(CSpentIndexKey &key, CSpentIndexValue &value) {
    return Read(make_pair(DB_SPENTINDEX, key), value);
}

bool CBlockTreeDB::UpdateSpentIndex(const std::vector<std::pair<CSpentIndexKey, CSpentIndexValue> >&vect) {
    CDBBatch batch(*this);
    for (std::vector<std::pair<CSpentIndexKey,CSpentIndexValue> >::const_iterator it=vect.begin(); it!=vect.end(); it++) {
        if (it->second.IsNull()) {
            batch.Erase(make_pair(DB_SPENTINDEX, it->first));
        } else {
            batch.Write(make_pair(DB_SPENTINDEX, it->first), it->second);
        }
    }
    return WriteBatch(batch);
}

bool CBlockTreeDB::UpdateAddressUnspentIndex(const std::vector<std::pair<CAddressUnspentKey, CAddressUnspentValue > >&vect) {
    CDBBatch batch(*this);
    for (std::vector<std::pair<CAddressUnspentKey, CAddressUnspentValue> >::const_iterator it=vect.begin(); it!=vect.end(); it++) {
        if (it->second.IsNull()) {
            batch.Erase(make_pair(DB_ADDRESSUNSPENTINDEX, it->first));
        } else {
            batch.Write(make_pair(DB_ADDRESSUNSPENTINDEX, it->first), it->second);
        }
    }
    return WriteBatch(batch);
}

bool CBlockTreeDB::ReadAddressUnspentIndex(uint160 addressHash, int type,
                                           std::vector<std::pair<CAddressUnspentKey, CAddressUnspentValue> > &unspentOutputs) {

    boost::scoped_ptr<CDBIterator> pcursor(NewIterator());

    pcursor->Seek(make_pair(DB_ADDRESSUNSPENTINDEX, CAddressIndexIteratorKey(type, addressHash)));

    while (pcursor->Valid()) {
        boost::this_thread::interruption_point();
        std::pair<char,CAddressUnspentKey> key;
        if (pcursor->GetKey(key) && key.first == DB_ADDRESSUNSPENTINDEX && key.second.hashBytes == addressHash) {
            CAddressUnspentValue nValue;
            if (pcursor->GetValue(nValue)) {
                unspentOutputs.push_back(make_pair(key.second, nValue));
                pcursor->Next();
            } else {
                return error("failed to get address unspent value");
            }
        } else {
            break;
        }
    }

    return true;
}

bool CBlockTreeDB::WriteAddressIndex(const std::vector<std::pair<CAddressIndexKey, CAmount > >&vect) {
    CDBBatch batch(*this);
    for (std::vector<std::pair<CAddressIndexKey, CAmount> >::const_iterator it=vect.begin(); it!=vect.end(); it++)
        batch.Write(make_pair(DB_ADDRESSINDEX, it->first), it->second);
    return WriteBatch(batch);
}

bool CBlockTreeDB::EraseAddressIndex(const std::vector<std::pair<CAddressIndexKey, CAmount > >&vect) {
    CDBBatch batch(*this);
    for (std::vector<std::pair<CAddressIndexKey, CAmount> >::const_iterator it=vect.begin(); it!=vect.end(); it++)
        batch.Erase(make_pair(DB_ADDRESSINDEX, it->first));
    return WriteBatch(batch);
}

bool CBlockTreeDB::ReadAddressIndex(uint160 addressHash, int type,
                                    std::vector<std::pair<CAddressIndexKey, CAmount> > &addressIndex,
                                    int start, int end) {

    boost::scoped_ptr<CDBIterator> pcursor(NewIterator());

    if (start > 0 && end > 0) {
        pcursor->Seek(make_pair(DB_ADDRESSINDEX, CAddressIndexIteratorHeightKey(type, addressHash, start)));
    } else {
        pcursor->Seek(make_pair(DB_ADDRESSINDEX, CAddressIndexIteratorKey(type, addressHash)));
    }

    while (pcursor->Valid()) {
        boost::this_thread::interruption_point();
        std::pair<char,CAddressIndexKey> key;
        if (pcursor->GetKey(key) && key.first == DB_ADDRESSINDEX && key.second.hashBytes == addressHash) {
            if (end > 0 && key.second.blockHeight > end) {
                break;
            }
            CAmount nValue;
            if (pcursor->GetValue(nValue)) {
                addressIndex.push_back(make_pair(key.second, nValue));
                pcursor->Next();
            } else {
                return error("failed to get address index value");
            }
        } else {
            break;
        }
    }

    return true;
}

bool CBlockTreeDB::WriteTimestampIndex(const CTimestampIndexKey &timestampIndex) {
    CDBBatch batch(*this);
    batch.Write(make_pair(DB_TIMESTAMPINDEX, timestampIndex), 0);
    return WriteBatch(batch);
}

bool CBlockTreeDB::ReadTimestampIndex(const unsigned int &high, const unsigned int &low, const bool fActiveOnly, std::vector<std::pair<uint256, unsigned int> > &hashes) {

    boost::scoped_ptr<CDBIterator> pcursor(NewIterator());

    pcursor->Seek(make_pair(DB_TIMESTAMPINDEX, CTimestampIndexIteratorKey(low)));

    while (pcursor->Valid()) {
        boost::this_thread::interruption_point();
        std::pair<char, CTimestampIndexKey> key;
        if (pcursor->GetKey(key) && key.first == DB_TIMESTAMPINDEX && key.second.timestamp < high) {
            if (fActiveOnly) {
                if (HashOnchainActive(key.second.blockHash)) {
                    hashes.push_back(std::make_pair(key.second.blockHash, key.second.timestamp));
                }
            } else {
                hashes.push_back(std::make_pair(key.second.blockHash, key.second.timestamp));
            }

            pcursor->Next();
        } else {
            break;
        }
    }

    return true;
}

bool CBlockTreeDB::WriteTimestampBlockIndex(const CTimestampBlockIndexKey &blockhashIndex, const CTimestampBlockIndexValue &logicalts) {
    CDBBatch batch(*this);
    batch.Write(make_pair(DB_BLOCKHASHINDEX, blockhashIndex), logicalts);
    return WriteBatch(batch);
}

bool CBlockTreeDB::ReadTimestampBlockIndex(const uint256 &hash, unsigned int &ltimestamp) {

    CTimestampBlockIndexValue(lts);
    if (!Read(std::make_pair(DB_BLOCKHASHINDEX, hash), lts))
	return false;

    ltimestamp = lts.ltimestamp;
    return true;
}

bool CBlockTreeDB::WriteFlag(const std::string &name, bool fValue) {
    return Write(std::make_pair(DB_FLAG, name), fValue ? '1' : '0');
}

bool CBlockTreeDB::ReadFlag(const std::string &name, bool &fValue) {
    char ch;
    if (!Read(std::make_pair(DB_FLAG, name), ch))
        return false;
    fValue = ch == '1';
    return true;
}

bool CBlockTreeDB::LoadBlockIndexGuts(boost::function<CBlockIndex*(const uint256&)> insertBlockIndex,
                                      boost::function<std::vector<std::pair<uint256, int>>*(const uint256&)> insertProposalVotes,
                                      boost::function<std::vector<std::pair<uint256, int>>*(const uint256&)> insertPaymentRequestVotes,
                                      boost::function<std::map<uint256, bool>*(const uint256&)> insertSupport,
                                      boost::function<std::map<uint256, uint64_t>*(const uint256&)> insertConsultationVotes)
{
    boost::scoped_ptr<CDBIterator> pcursor(NewIterator());

    pcursor->Seek(make_pair(DB_BLOCK_INDEX, uint256()));

    int nCount = 0;

    // Load mapBlockIndex
    while (pcursor->Valid()) {
        if (++nCount % PROGRESS_INTERVAL == 0) {
            // Update the progress
            uiInterface.ShowProgress(_("Loading block guts..."), nCount);
        }
        boost::this_thread::interruption_point();
        std::pair<char, uint256> key;
        if (pcursor->GetKey(key) && key.first == DB_BLOCK_INDEX) {
            CDiskBlockIndex diskindex;
            if (pcursor->GetValue(diskindex)) {
                // Construct block index object
                CBlockIndex* pindexNew = insertBlockIndex(diskindex.GetBlockHash());
                pindexNew->pprev          = insertBlockIndex(diskindex.hashPrev);
                pindexNew->nHeight        = diskindex.nHeight;
                pindexNew->nFile          = diskindex.nFile;
                pindexNew->nDataPos       = diskindex.nDataPos;
                pindexNew->nUndoPos       = diskindex.nUndoPos;
                pindexNew->nVersion       = diskindex.nVersion;
                pindexNew->hashMerkleRoot = diskindex.hashMerkleRoot;
                pindexNew->nTime          = diskindex.nTime;
                pindexNew->nBits          = diskindex.nBits;
                pindexNew->nNonce         = diskindex.nNonce;
                pindexNew->nStatus        = diskindex.nStatus;
                pindexNew->nTx            = diskindex.nTx;
                pindexNew->nMint          = diskindex.nMint;
                pindexNew->nCFSupply      = diskindex.nCFSupply;
                pindexNew->nCFLocked      = diskindex.nCFLocked;
                pindexNew->strDZeel       = diskindex.strDZeel;
                pindexNew->nFlags         = diskindex.nFlags;
                pindexNew->nStakeModifier = diskindex.nStakeModifier;
                pindexNew->hashProof      = diskindex.hashProof;
                if (diskindex.vProposalVotes.size() > 0)
                {
                    auto pVotes = insertProposalVotes(diskindex.GetBlockHash());
                    *pVotes = diskindex.vProposalVotes;
                }
                if (diskindex.vPaymentRequestVotes.size() > 0)
                {
                    auto prVotes = insertPaymentRequestVotes(diskindex.GetBlockHash());
                    *prVotes = diskindex.vPaymentRequestVotes;
                }
                if (diskindex.mapSupport.size() > 0)
                {
                    auto supp = insertSupport(diskindex.GetBlockHash());
                    *supp = diskindex.mapSupport;
                }
                if (diskindex.mapConsultationVotes.size() > 0)
                {
                    auto cVotes = insertConsultationVotes(diskindex.GetBlockHash());
                    *cVotes = diskindex.mapConsultationVotes;
                }

                pcursor->Next();
            } else {
                return error("LoadBlockIndex() : failed to read value");
            }
        } else {
            break;
        }
    }

    return true;
}
