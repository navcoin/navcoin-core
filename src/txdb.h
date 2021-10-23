// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef NAVCOIN_TXDB_H
#define NAVCOIN_TXDB_H

#include <main.h>
#include <coins.h>
#include <dbwrapper.h>
#include <chain.h>
#include <addressindex.h>
#include <spentindex.h>
#include <timestampindex.h>

#include <functional>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include <boost/function.hpp>

class CBlockIndex;
class CStateViewDBCursor;
class uint256;

//! Compensate for extra memory peak (x1.5-x1.9) at flush time.
static constexpr int DB_PEAK_USAGE_FACTOR = 2;
//! No need to periodic flush if at least this much space still available.
static constexpr int MAX_BLOCK_COINSDB_USAGE = 200 * DB_PEAK_USAGE_FACTOR;
//! Always periodic flush if less than this much space still available.
static constexpr int MIN_BLOCK_COINSDB_USAGE = 50 * DB_PEAK_USAGE_FACTOR;
//! -dbcache default (MiB)		  //! -dbcache default (MiB)
static const int64_t nDefaultDbCache = 450;
//! max. -dbcache (MiB)
static const int64_t nMaxDbCache = sizeof(void*) > 4 ? 16384 : 1024;
//! min. -dbcache (MiB)
static const int64_t nMinDbCache = 4;
//! Max memory allocated to block tree DB specific cache, if no -txindex (MiB)
static const int64_t nMaxBlockDBCache = 2;
//! Max memory allocated to block tree DB specific cache, if -txindex (MiB)
// Unlike for the UTXO database, for the txindex scenario the leveldb cache make
// a meaningful difference: https://github.com/navcoin/navcoin/pull/8273#issuecomment-229601991
static const int64_t nMaxTxIndexCache = 1024;
//! Max memory allocated to coin DB specific cache (MiB)
static const int64_t nMaxCoinsDBCache = 8;

template<typename T, typename M, template<typename> class C = std::less>
struct member_comparer : std::binary_function<T, T, bool>
{
    explicit member_comparer(M T::*p) : p_(p) { }

    bool operator ()(T const& lhs, T const& rhs) const
    {
        return C<M>()(lhs.*p_, rhs.*p_);
    }

private:
    M T::*p_;
};

template<template<typename> class C, typename T, typename M>
member_comparer<T, M, C> make_member_comparer(M T::*p)
{
    return member_comparer<T, M, C>(p);
}

struct CDiskTxPos : public CDiskBlockPos
{
    unsigned int nTxOffset; // after header

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(*(CDiskBlockPos*)this);
        READWRITE(VARINT(nTxOffset));
    }

    CDiskTxPos(const CDiskBlockPos &blockIn, unsigned int nTxOffsetIn) : CDiskBlockPos(blockIn.nFile, blockIn.nPos), nTxOffset(nTxOffsetIn) {
    }

    CDiskTxPos() {
        SetNull();
    }

    void SetNull() {
        CDiskBlockPos::SetNull();
        nTxOffset = 0;
    }
};

/** CStateView backed by the coin database (chainstate/) */
class CStateViewDB : public CStateView
{
protected:
    CDBWrapper db;
public:
    CStateViewDB(size_t nCacheSize, bool fMemory = false, bool fWipe = false);

    bool GetCoins(const uint256 &txid, CCoins &coins) const;
    bool HaveCoins(const uint256 &txid) const;
    bool GetProposal(const uint256 &pid, CProposal &coins) const;
    bool HaveProposal(const uint256 &pid) const;
    bool GetPaymentRequest(const uint256 &prid, CPaymentRequest &coins) const;
    bool HavePaymentRequest(const uint256 &prid) const;
    bool GetCachedVoter(const CVoteMapKey &voter, CVoteMapValue& vote) const;
    bool HaveCachedVoter(const CVoteMapKey &voter) const;
    bool GetConsultation(const uint256 &cid, CConsultation& consultation) const;
    bool HaveConsultation(const uint256 &cid) const;
    bool GetConsultationAnswer(const uint256 &aid, CConsultationAnswer& answer) const;
    bool HaveConsultationAnswer(const uint256 &aid) const;
    bool GetConsensusParameter(const int &pid, CConsensusParameter& cparameter) const;
    bool HaveConsensusParameter(const int &pid) const;
    bool GetToken(const bls::G1Element &id, TokenInfo &token) const;
    bool HaveToken(const bls::G1Element &id) const;

    uint256 GetBestBlock() const;
    bool BatchWrite(CCoinsMap &mapCoins, CProposalMap &mapProposals,
                    CPaymentRequestMap &mapPaymentRequests, CVoteMap &mapVotes,
                    CConsultationMap &mapConsultations, CConsultationAnswerMap &mapAnswers,
                    CConsensusParameterMap& mapConsensus, TokenMap& mapTokens, const uint256 &hashBlock,
                    const int &nExcludeVotes);
    bool GetAllProposals(CProposalMap& map);
    bool GetAllPaymentRequests(CPaymentRequestMap& map);
    bool GetAllVotes(CVoteMap &map);
    bool GetAllConsultations(CConsultationMap &map);
    bool GetAllConsultationAnswers(CConsultationAnswerMap &map);
    bool GetAllTokens(TokenMap &map);
    int GetExcludeVotes() const;
    CStateViewCursor *Cursor() const;
};

/** Specialization of CStateViewCursor to iterate over a CStateViewDB */
class CStateViewDBCursor: public CStateViewCursor
{
public:
    ~CStateViewDBCursor() {}

    bool GetKey(uint256 &key) const;
    bool GetValue(CCoins &coins) const;
    unsigned int GetValueSize() const;

    bool Valid() const;
    void Next();

private:
    CStateViewDBCursor(CDBIterator* pcursorIn, const uint256 &hashBlockIn):
        CStateViewCursor(hashBlockIn), pcursor(pcursorIn) {}
    boost::scoped_ptr<CDBIterator> pcursor;
    std::pair<char, uint256> keyTmp;

    friend class CStateViewDB;
};

/** Access to the block database (blocks/index/) */
class CBlockTreeDB : public CDBWrapper
{
public:
    CBlockTreeDB(size_t nCacheSize, bool fMemory = false, bool fWipe = false, bool compression = true, int maxOpenFiles = 1000);
private:
    CBlockTreeDB(const CBlockTreeDB&);
    void operator=(const CBlockTreeDB&);
public:
    bool WriteBatchSync(const std::vector<std::pair<int, const CBlockFileInfo*> >& fileInfo, int nLastFile, const std::vector<const CBlockIndex*>& blockinfo);
    bool ReadBlockFileInfo(int nFile, CBlockFileInfo &fileinfo);
    bool ReadLastBlockFile(int &nFile);
    bool WriteReindexing(bool fReindex);
    bool ReadReindexing(bool &fReindex);
    bool ReadTxIndex(const uint256 &txid, CDiskTxPos &pos);
    bool WriteTxIndex(const std::vector<std::pair<uint256, CDiskTxPos> > &list);
    bool ReadSpentIndex(CSpentIndexKey &key, CSpentIndexValue &value);
    bool UpdateSpentIndex(const std::vector<std::pair<CSpentIndexKey, CSpentIndexValue> >&vect);
    bool UpdateAddressUnspentIndex(const std::vector<std::pair<CAddressUnspentKey, CAddressUnspentValue > >&vect);
    bool ReadAddressUnspentIndex(uint160 addressHash, int type,
                                 std::vector<std::pair<CAddressUnspentKey, CAddressUnspentValue> > &vect);
    bool WriteAddressIndex(const std::vector<std::pair<CAddressIndexKey, CAmount> > &vect);
    bool EraseAddressIndex(const std::vector<std::pair<CAddressIndexKey, CAmount> > &vect);
    bool ReadAddressIndex(uint160 addressHash, int type,
                          std::vector<std::pair<CAddressIndexKey, CAmount> > &addressIndex,
                          int start = 0, int end = 0);
    bool WriteAddressHistory(const std::vector<std::pair<CAddressHistoryKey, CAddressHistoryValue> > &vect);
    bool EraseAddressHistory(const std::vector<std::pair<CAddressHistoryKey, CAddressHistoryValue> > &vect);
    bool ReadAddressHistory(uint160 addressHash, uint160 addressHash2,
                          std::vector<std::pair<CAddressHistoryKey, CAddressHistoryValue> > &addressIndex,
                          AddressHistoryFilter filter = AddressHistoryFilter::ALL, int start = 0, int end = 0);
    bool WriteTimestampIndex(const CTimestampIndexKey &timestampIndex);
    bool ReadTimestampIndex(const unsigned int &high, const unsigned int &low, const bool fActiveOnly, std::vector<std::pair<uint256, unsigned int> > &vect);
    bool WriteTimestampBlockIndex(const CTimestampBlockIndexKey &blockhashIndex, const CTimestampBlockIndexValue &logicalts);
    bool ReadTimestampBlockIndex(const uint256 &hash, unsigned int &logicalTS);
    bool WriteFlag(const std::string &name, bool fValue);
    bool ReadFlag(const std::string &name, bool &fValue);
    bool LoadBlockIndexGuts(boost::function<CBlockIndex*(const uint256&)> insertBlockIndex,
                            boost::function<std::vector<std::pair<uint256, int>>*(const uint256&)> insertProposalVotes,
                            boost::function<std::vector<std::pair<uint256, int>>*(const uint256&)> insertPaymentRequestVotes,
                            boost::function<std::map<uint256, bool>*(const uint256&)> insertSupport,
                            boost::function<std::map<uint256, uint64_t>*(const uint256&)> insertConsultationVotes);
    bool ReadProposalIndex(const uint256 &proposalid, CProposal &proposal);
    bool WriteProposalIndex(const std::vector<std::pair<uint256, CProposal> >&vect);
    bool GetProposalIndex(std::vector<CProposal>&vect);
    CProposal GetProposal(uint256 hash);
    bool UpdateProposalIndex(const std::vector<std::pair<uint256, CProposal> >&vect);
    bool ReadPaymentRequestIndex(const uint256 &prequestid, CPaymentRequest &prequest);
    bool WritePaymentRequestIndex(const std::vector<std::pair<uint256, CPaymentRequest> >&vect);
    bool GetPaymentRequestIndex(std::vector<CPaymentRequest>&vect);
    bool UpdatePaymentRequestIndex(const std::vector<std::pair<uint256, CPaymentRequest> >&vect);
    bool WriteToken(const std::vector<Token>&vect);
    bool ReadTokenIndex(const bls::G1Element &id, TokenInfo &token);
    bool GetTokenIndex(std::vector<Token>&vect);
    TokenInfo GetToken(const bls::G1Element& id);
    bool UpdateTokenIndex(const std::vector<Token>&vect);
    bool WriteTokenIndex(const std::vector<Token>&vect);

};

#endif // NAVCOIN_TXDB_H
