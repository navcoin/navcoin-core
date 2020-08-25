// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Copyright (c) 2018 The NavCoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <miner.h>

#include <amount.h>
#include <base58.h>
#include <chain.h>
#include <chainparams.h>
#include <coins.h>
#include <consensus/consensus.h>
#include <consensus/merkle.h>
#include <consensus/validation.h>
#include <core_io.h>
#include <hash.h>
#include <init.h>
#include <main.h>
#include <net.h>
#include <ntpclient.h>
#include <policy/policy.h>
#include <pos.h>
#include <primitives/transaction.h>
#include <script/sign.h>
#include <script/standard.h>
#include <timedata.h>
#include <txmempool.h>
#include <util.h>
#include <utiltime.h>
#include <utilmoneystr.h>
#include <validationinterface.h>
#include <versionbits.h>
#include <wallet/wallet.h>
#include <kernel.h>

#include <algorithm>
#include <boost/thread.hpp>
#include <boost/tuple/tuple.hpp>
#include <queue>

using namespace std;

//////////////////////////////////////////////////////////////////////////////
//
// BitcoinStaker
//

//
// Unconfirmed transactions in the memory pool often depend on other
// transactions in the memory pool. When we select transactions from the
// pool, we select by highest priority or fee rate, so we might consider
// transactions that depend on transactions that aren't yet in the block.

uint64_t nLastBlockTx = 0;
uint64_t nLastBlockSize = 0;
uint64_t nLastBlockWeight = 0;
uint64_t nLastCoinStakeSearchInterval = 0;

uint64_t nLastTime = 0;
uint64_t nLastSteadyTime = 0;

bool fIncorrectTime = false;

class ScoreCompare
{
public:
    ScoreCompare() {}

    bool operator()(const CTxMemPool::txiter a, const CTxMemPool::txiter b)
    {
        return CompareTxMemPoolEntryByScore()(*b,*a); // Convert to less than
    }
};

int64_t UpdateTime(CBlockHeader* pblock, const Consensus::Params& consensusParams, const CBlockIndex* pindexPrev)
{
    int64_t nOldTime = pblock->nTime;
    int64_t nNewTime = std::max(pindexPrev->GetMedianTimePast()+1, GetAdjustedTime());

    if (nOldTime < nNewTime)
        pblock->nTime = nNewTime;

    // Updating time can change work required on testnet:
    if (consensusParams.fPowAllowMinDifficultyBlocks)
        pblock->nBits = GetNextTargetRequired(pindexPrev, true);

    return nNewTime - nOldTime;
}

BlockAssembler::BlockAssembler(const CChainParams& _chainparams)
    : chainparams(_chainparams)
{
    // Block resource limits
    // If neither -blockmaxsize or -blockmaxweight is given, limit to DEFAULT_BLOCK_MAX_*
    // If only one is given, only restrict the specified resource.
    // If both are given, restrict both.
    nBlockMaxWeight = DEFAULT_BLOCK_MAX_WEIGHT;
    nBlockMaxSize = DEFAULT_BLOCK_MAX_SIZE;
    bool fWeightSet = false;
    if (mapArgs.count("-blockmaxweight")) {
        nBlockMaxWeight = GetArg("-blockmaxweight", DEFAULT_BLOCK_MAX_WEIGHT);
        nBlockMaxSize = MAX_BLOCK_SERIALIZED_SIZE;
        fWeightSet = true;
    }
    if (mapArgs.count("-blockmaxsize")) {
        nBlockMaxSize = GetArg("-blockmaxsize", DEFAULT_BLOCK_MAX_SIZE);
        if (!fWeightSet) {
            nBlockMaxWeight = nBlockMaxSize * WITNESS_SCALE_FACTOR;
        }
    }

    // Limit weight to between 4K and MAX_BLOCK_WEIGHT-4K for sanity:
    nBlockMaxWeight = std::max((unsigned int)4000, std::min((unsigned int)(MAX_BLOCK_WEIGHT-4000), nBlockMaxWeight));
    // Limit size to between 1K and MAX_BLOCK_SERIALIZED_SIZE-1K for sanity:
    nBlockMaxSize = std::max((unsigned int)1000, std::min((unsigned int)(MAX_BLOCK_SERIALIZED_SIZE-1000), nBlockMaxSize));

    // Whether we need to account for byte usage (in addition to weight usage)
    fNeedSizeAccounting = (nBlockMaxSize < MAX_BLOCK_SERIALIZED_SIZE-1000);
}

void BlockAssembler::resetBlock()
{
    inBlock.clear();

    // Reserve space for coinbase tx
    nBlockSize = 1000;
    nBlockWeight = 4000;
    nBlockSigOpsCost = 400;
    fIncludeWitness = false;

    // These counters do not include coinbase tx
    nBlockTx = 0;
    nFees = 0;

    lastFewTxs = 0;
    blockFinished = false;
}

CBlockTemplate* BlockAssembler::CreateNewBlock(const CScript& scriptPubKeyIn, bool fProofOfStake, uint64_t* pFees, std::string &sLog)
{
    resetBlock();

    pblocktemplate.reset(new CBlockTemplate());

    if(!pblocktemplate.get())
        return nullptr;
    pblock = &pblocktemplate->block; // pointer for convenience

    // Add dummy coinbase tx as first transaction
    pblock->vtx.push_back(CTransaction());

    pblocktemplate->vTxFees.push_back(-1); // updated at end
    pblocktemplate->vTxSigOpsCost.push_back(-1); // updated at end

    LOCK2(cs_main, mempool.cs);
    CBlockIndex* pindexPrev = chainActive.Tip();
    nHeight = pindexPrev->nHeight + 1;
    CStateViewCache view(pcoinsTip);

    // Decide whether to include witness transactions
    // This is only needed in case the witness softfork activation is reverted
    // (which would require a very deep reorganization) or when
    // -promiscuousmempoolflags is used.
    // TODO: replace this with a call to main to assess validity of a mempool
    // transaction (which in most cases can be a no-op).
    fIncludeWitness = IsWitnessEnabled(pindexPrev, chainparams.GetConsensus());

    pblock->nVersion = ComputeBlockVersion(pindexPrev, chainparams.GetConsensus());

    // -regtest only: allow overriding block.nVersion with
    // -blockversion=N to test forking scenarios
    // if (chainparams.MineBlocksOnDemand())
    //     pblock->nVersion = GetArg("-blockversion", pblock->nVersion);

    pblock->nTime = GetAdjustedTime();

    const int64_t nMedianTimePast = pindexPrev->GetMedianTimePast();

    nLockTimeCutoff = (STANDARD_LOCKTIME_VERIFY_FLAGS & LOCKTIME_MEDIAN_TIME_PAST)
                       ? nMedianTimePast
                       : pblock->GetBlockTime();

    addCombinedBLSCT(view);
    addPriorityTxs(fProofOfStake, pblock->vtx[0].nTime);
    addPackageTxs();

    nLastBlockTx = nBlockTx;
    nLastBlockSize = nBlockSize;
    nLastBlockWeight = nBlockWeight;

    // Create coinbase transaction.
    CMutableTransaction coinbaseTx;

    coinbaseTx.nVersion = CTransaction::TXDZEEL_VERSION_V2;
    coinbaseTx.vin.resize(1);
    coinbaseTx.vin[0].prevout.SetNull();
    coinbaseTx.vout.resize(1);

    if(fProofOfStake)
    {
        coinbaseTx.vout[0].scriptPubKey.clear();
        coinbaseTx.vout[0].nValue = 0;
    }
    else
    {
        coinbaseTx.vout[0].scriptPubKey = scriptPubKeyIn;
        coinbaseTx.vout[0].nValue = GetBlockSubsidy(nHeight, chainparams.GetConsensus());
    }

    coinbaseTx.vin[0].scriptSig = CScript() << nHeight << OP_0;

    std::map<uint256, bool> votes;
    CStateViewCache coins(pcoinsTip);

    bool fDAOConsultations = IsDAOEnabled(pindexPrev, chainparams.GetConsensus());
    bool fCFund = IsCommunityFundEnabled(pindexPrev, chainparams.GetConsensus());

    if(fDAOConsultations)
    {
        CConsultation consultation;
        CConsultationAnswer answer;

        for (auto& it: mapSupported)
        {
            if (votes.count(it.first) != 0)
                continue;

            if (coins.GetConsultation(it.first, consultation))
            {
                if (consultation.CanBeSupported())
                {
                    coinbaseTx.vout.resize(coinbaseTx.vout.size()+1);

                    SetScriptForConsultationSupport(coinbaseTx.vout[coinbaseTx.vout.size()-1].scriptPubKey,consultation.hash);
                    coinbaseTx.vout[coinbaseTx.vout.size()-1].nValue = 0;
                    if (LogAcceptCategory("dao")) sLog += strprintf("%s: Adding consultation-support output %s\n", __func__, coinbaseTx.vout[coinbaseTx.vout.size()-1].ToString());
                    votes[consultation.hash] = true;

                    continue;
                }
            }
            else if (coins.GetConsultationAnswer(it.first, answer))
            {
                if (answer.CanBeSupported(coins))
                {
                    coinbaseTx.vout.resize(coinbaseTx.vout.size()+1);

                    SetScriptForConsultationSupport(coinbaseTx.vout[coinbaseTx.vout.size()-1].scriptPubKey,answer.hash);
                    coinbaseTx.vout[coinbaseTx.vout.size()-1].nValue = 0;
                    if (LogAcceptCategory("dao")) sLog += strprintf("%s: Adding consultation-support output %s\n", __func__, coinbaseTx.vout[coinbaseTx.vout.size()-1].ToString());
                    votes[answer.hash] = true;

                    continue;
                }
            }
        }

        std::map<uint256, int> mapCountAnswers;
        std::map<uint256, int> mapCacheMaxAnswers;

        for (auto& it: mapAddedVotes)
        {
            int64_t vote = it.second;

            if (!fDAOConsultations && vote == VoteFlags::VOTE_ABSTAIN)
                continue;

            if (votes.count(it.first) != 0)
                continue;

            if (coins.GetConsultation(it.first, consultation))
            {
                if (consultation.CanBeVoted(vote) && consultation.IsValidVote(vote))
                {
                    coinbaseTx.vout.resize(coinbaseTx.vout.size()+1);

                    SetScriptForConsultationVote(coinbaseTx.vout[coinbaseTx.vout.size()-1].scriptPubKey,consultation.hash,vote);
                    coinbaseTx.vout[coinbaseTx.vout.size()-1].nValue = 0;

                    if (LogAcceptCategory("dao")) sLog += strprintf("%s: Adding consultation-vote output %s\n", __func__, coinbaseTx.vout[coinbaseTx.vout.size()-1].ToString());

                    votes[consultation.hash] = true;

                    continue;
                }
            }
            else if (coins.GetConsultationAnswer(it.first, answer))
            {
                if (answer.CanBeVoted(coins))
                {
                    coinbaseTx.vout.resize(coinbaseTx.vout.size()+1);

                    SetScriptForConsultationVote(coinbaseTx.vout[coinbaseTx.vout.size()-1].scriptPubKey,answer.hash,-2);
                    coinbaseTx.vout[coinbaseTx.vout.size()-1].nValue = 0;

                    if (LogAcceptCategory("dao")) sLog += strprintf("%s: Adding consultation-answer-vote output %s\n", __func__, coinbaseTx.vout[coinbaseTx.vout.size()-1].ToString());

                    votes[answer.hash] = true;

                    continue;
                }
            }
        }
    }

    if(fCFund)
    {
        CProposal proposal;
        CPaymentRequest prequest;
        CConsultation consultation;
        CConsultationAnswer answer;

        for (auto& it: mapAddedVotes)
        {
            int64_t vote = it.second;

            if (!fDAOConsultations && vote == VoteFlags::VOTE_ABSTAIN)
                continue;

            if (votes.count(it.first) != 0)
                continue;

            if(coins.GetProposal(it.first, proposal))
            {
                if(proposal.CanVote(coins))
                {
                    coinbaseTx.vout.resize(coinbaseTx.vout.size()+1);
                    SetScriptForProposalVote(coinbaseTx.vout[coinbaseTx.vout.size()-1].scriptPubKey,proposal.hash, vote);
                    coinbaseTx.vout[coinbaseTx.vout.size()-1].nValue = 0;
                    votes[proposal.hash] = vote;

                    if (LogAcceptCategory("dao")) sLog += strprintf("%s: Adding proposal-vote output %s\n", __func__, coinbaseTx.vout[coinbaseTx.vout.size()-1].ToString());

                    continue;
                }
            }
            else if(coins.GetPaymentRequest(it.first, prequest))
            {
                if(!view.GetProposal(prequest.proposalhash, proposal))
                    continue;
                CBlockIndex* pblockindex = proposal.GetLastStateBlockIndexForState(DAOFlags::ACCEPTED);
                if(pblockindex == nullptr)
                    continue;
                if((proposal.CanRequestPayments() || proposal.GetLastState() == DAOFlags::PENDING_VOTING_PREQ)
                        && prequest.CanVote(view) && votes.count(prequest.hash) == 0 &&
                        pindexPrev->nHeight - pblockindex->nHeight > Params().GetConsensus().nCommunityFundMinAge)
                {
                    coinbaseTx.vout.resize(coinbaseTx.vout.size()+1);
                    SetScriptForPaymentRequestVote(coinbaseTx.vout[coinbaseTx.vout.size()-1].scriptPubKey,prequest.hash, vote);
                    coinbaseTx.vout[coinbaseTx.vout.size()-1].nValue = 0;
                    votes[prequest.hash] = vote;

                    if (LogAcceptCategory("dao")) sLog += strprintf("%s: Adding payment-request-vote output %s\n", __func__, coinbaseTx.vout[coinbaseTx.vout.size()-1].ToString());

                    continue;
                }
            }
        }

        UniValue strDZeel(UniValue::VARR);
        CPaymentRequestMap mapPaymentRequests;

        if(view.GetAllPaymentRequests(mapPaymentRequests))
        {
            for (CPaymentRequestMap::iterator it_ = mapPaymentRequests.begin(); it_ != mapPaymentRequests.end(); it_++)
            {
                CPaymentRequest prequest;

                if (!view.GetPaymentRequest(it_->first, prequest))
                    continue;
                CBlockIndex* pblockindex = prequest.GetLastStateBlockIndexForState(DAOFlags::ACCEPTED);
                if(pblockindex == nullptr)
                    continue;

                if(prequest.hash == uint256())
                    continue;

                if(prequest.GetLastState() == DAOFlags::ACCEPTED &&
                        pindexPrev->nHeight - pblockindex->nHeight > Params().GetConsensus().nCommunityFundMinAge) {
                    CProposal proposal;
                    if(view.GetProposal(prequest.proposalhash, proposal)) {
                        CNavCoinAddress addr(proposal.GetPaymentAddress());

                        if (!addr.IsValid())
                            continue;

                        coinbaseTx.vout.resize(coinbaseTx.vout.size()+1);
                        coinbaseTx.vout[coinbaseTx.vout.size()-1].scriptPubKey = GetScriptForDestination(addr.Get());
                        coinbaseTx.vout[coinbaseTx.vout.size()-1].nValue = prequest.nAmount;

                        if (LogAcceptCategory("dao")) sLog += strprintf("%s: Adding payment-request-payout output %s\n", __func__, coinbaseTx.vout[coinbaseTx.vout.size()-1].ToString());

                        strDZeel.push_back(prequest.hash.ToString());
                    }
                    else
                        if (LogAcceptCategory("dao")) sLog += strprintf("Could not find parent proposal of payment request %s.\n", prequest.hash.ToString());
                }
            }
        }
        if(sCoinBaseStrDZeel == "") {
            coinbaseTx.strDZeel = strDZeel.write();
        } else {
            coinbaseTx.strDZeel = sCoinBaseStrDZeel;
        }
    }

    for(unsigned int i = 0; i < vCoinBaseOutputs.size(); i++)
    {
        CTxOut forcedTxOut;
        if (!DecodeHexTxOut(forcedTxOut, vCoinBaseOutputs[i]))
            LogPrintf("Tried to force a wrong transaction output in the coinbase: %s\n", vCoinBaseOutputs[i]);
        else
            coinbaseTx.vout.insert(coinbaseTx.vout.end(), forcedTxOut);
    }

    pblock->vtx[0] = coinbaseTx;

    pblocktemplate->vchCoinbaseCommitment = GenerateCoinbaseCommitment(*pblock, pindexPrev, chainparams.GetConsensus());
    pblocktemplate->vTxFees[0] = -nFees;

    // Fill in header
    pblock->hashPrevBlock  = pindexPrev->GetBlockHash();
    if (!fProofOfStake)
        UpdateTime(pblock, chainparams.GetConsensus(), pindexPrev);
    pblock->vtx[0].nTime   = pblock->nTime;
    pblock->nBits          = GetNextTargetRequired(pindexPrev, fProofOfStake);
    pblock->nNonce         = 0;
    pblocktemplate->vTxSigOpsCost[0] = WITNESS_SCALE_FACTOR * GetLegacySigOpCount(pblock->vtx[0]);

    if (pFees)
      *pFees = nFees;

    return pblocktemplate.release();
}

bool BlockAssembler::isStillDependent(CTxMemPool::txiter iter)
{
    for(CTxMemPool::txiter parent: mempool.GetMemPoolParents(iter))
    {
        if (!inBlock.count(parent)) {
            return true;
        }
    }
    return false;
}

void BlockAssembler::onlyUnconfirmed(CTxMemPool::setEntries& testSet)
{
    for (CTxMemPool::setEntries::iterator iit = testSet.begin(); iit != testSet.end(); ) {
        // Only test txs not already in the block
        if (inBlock.count(*iit)) {
            testSet.erase(iit++);
        }
        else {
            iit++;
        }
    }
}

bool BlockAssembler::TestPackage(uint64_t packageSize, int64_t packageSigOpsCost)
{
    // TODO: switch to weight-based accounting for packages instead of vsize-based accounting.
    if (nBlockWeight + WITNESS_SCALE_FACTOR * packageSize >= nBlockMaxWeight)
        return false;
    if (nBlockSigOpsCost + packageSigOpsCost >= MAX_BLOCK_SIGOPS_COST)
        return false;
    return true;
}

// Perform transaction-level checks before adding to block:
// - transaction finality (locktime)
// - premature witness (in case segwit transactions are added to mempool before
//   segwit activation)
// - serialized size (in case -blockmaxsize is in use)
bool BlockAssembler::TestPackageTransactions(const CTxMemPool::setEntries& package)
{
    uint64_t nPotentialBlockSize = nBlockSize; // only used with fNeedSizeAccounting
    for(const CTxMemPool::txiter it: package) {
        if (!IsFinalTx(it->GetTx(), nHeight, nLockTimeCutoff))
            return false;
        if (!fIncludeWitness && !it->GetTx().wit.IsNull())
            return false;
        if (fNeedSizeAccounting) {
            uint64_t nTxSize = ::GetSerializeSize(it->GetTx(), SER_NETWORK, PROTOCOL_VERSION);
            if (nPotentialBlockSize + nTxSize >= nBlockMaxSize) {
                return false;
            }
            nPotentialBlockSize += nTxSize;
        }
    }
    return true;
}

bool BlockAssembler::TestForBlock(CTxMemPool::txiter iter)
{
    if (nBlockWeight + iter->GetTxWeight() >= nBlockMaxWeight) {
        // If the block is so close to full that no more txs will fit
        // or if we've tried more than 50 times to fill remaining space
        // then flag that the block is finished
        if (nBlockWeight >  nBlockMaxWeight - 400 || lastFewTxs > 50) {
             blockFinished = true;
             return false;
        }
        // Once we're within 4000 weight of a full block, only look at 50 more txs
        // to try to fill the remaining space.
        if (nBlockWeight > nBlockMaxWeight - 4000) {
            lastFewTxs++;
        }
        return false;
    }

    if (fNeedSizeAccounting) {
        if (nBlockSize + ::GetSerializeSize(iter->GetTx(), SER_NETWORK, PROTOCOL_VERSION) >= nBlockMaxSize) {
            if (nBlockSize >  nBlockMaxSize - 100 || lastFewTxs > 50) {
                 blockFinished = true;
                 return false;
            }
            if (nBlockSize > nBlockMaxSize - 1000) {
                lastFewTxs++;
            }
            return false;
        }
    }

    if (nBlockSigOpsCost + iter->GetSigOpCost() >= MAX_BLOCK_SIGOPS_COST) {
        // If the block has room for no more sig ops then
        // flag that the block is finished
        if (nBlockSigOpsCost > MAX_BLOCK_SIGOPS_COST - 8) {
            blockFinished = true;
            return false;
        }
        // Otherwise attempt to find another tx with fewer sigops
        // to put in the block.
        return false;
    }

    // Must check that lock times are still valid
    // This can be removed once MTP is always enforced
    // as long as reorgs keep the mempool consistent.
    if (!IsFinalTx(iter->GetTx(), nHeight, nLockTimeCutoff))
        return false;

    return true;
}

void BlockAssembler::AddToBlock(CTxMemPool::txiter iter)
{
    pblock->vtx.push_back(iter->GetTx());
    pblocktemplate->vTxFees.push_back(iter->GetFee());
    pblocktemplate->vTxSigOpsCost.push_back(iter->GetSigOpCost());
    if (fNeedSizeAccounting) {
        nBlockSize += ::GetSerializeSize(iter->GetTx(), SER_NETWORK, PROTOCOL_VERSION);
    }
    nBlockWeight += iter->GetTxWeight();
    ++nBlockTx;
    nBlockSigOpsCost += iter->GetSigOpCost();
    nFees += iter->GetFee();
    inBlock.insert(iter);

    bool fPrintPriority = GetBoolArg("-printpriority", DEFAULT_PRINTPRIORITY);
    if (fPrintPriority) {
        double dPriority = iter->GetPriority(nHeight);
        CAmount dummy;
        mempool.ApplyDeltas(iter->GetTx().GetHash(), dPriority, dummy);
        LogPrintf("priority %.1f fee %s txid %s\n",
                  dPriority,
                  CFeeRate(iter->GetModifiedFee(), iter->GetTxSize()).ToString(),
                  iter->GetTx().GetHash().ToString());
    }
}

void BlockAssembler::UpdatePackagesForAdded(const CTxMemPool::setEntries& alreadyAdded,
        indexed_modified_transaction_set &mapModifiedTx)
{
    for(const CTxMemPool::txiter it: alreadyAdded) {
        CTxMemPool::setEntries descendants;
        mempool.CalculateDescendants(it, descendants);
        // Insert all descendants (not yet in block) into the modified set
        for(CTxMemPool::txiter desc: descendants) {
            if (alreadyAdded.count(desc))
                continue;
            modtxiter mit = mapModifiedTx.find(desc);
            if (mit == mapModifiedTx.end()) {
                CTxMemPoolModifiedEntry modEntry(desc);
                modEntry.nSizeWithAncestors -= it->GetTxSize();
                modEntry.nModFeesWithAncestors -= it->GetModifiedFee();
                modEntry.nSigOpCostWithAncestors -= it->GetSigOpCost();
                mapModifiedTx.insert(modEntry);
            } else {
                mapModifiedTx.modify(mit, update_for_parent_inclusion(it));
            }
        }
    }
}

// Skip entries in mapTx that are already in a block or are present
// in mapModifiedTx (which implies that the mapTx ancestor state is
// stale due to ancestor inclusion in the block)
// Also skip transactions that we've already failed to add. This can happen if
// we consider a transaction in mapModifiedTx and it fails: we can then
// potentially consider it again while walking mapTx.  It's currently
// guaranteed to fail again, but as a belt-and-suspenders check we put it in
// failedTx and avoid re-evaluation, since the re-evaluation would be using
// cached size/sigops/fee values that are not actually correct.
bool BlockAssembler::SkipMapTxEntry(CTxMemPool::txiter it, indexed_modified_transaction_set &mapModifiedTx, CTxMemPool::setEntries &failedTx)
{
    assert (it != mempool.mapTx.end());
    if (mapModifiedTx.count(it) || inBlock.count(it) || failedTx.count(it) || it->GetTx().IsBLSInput())
        return true;
    return false;
}

void BlockAssembler::SortForBlock(const CTxMemPool::setEntries& package, CTxMemPool::txiter entry, std::vector<CTxMemPool::txiter>& sortedEntries)
{
    // Sort package by ancestor count
    // If a transaction A depends on transaction B, then A's ancestor count
    // must be greater than B's.  So this is sufficient to validly order the
    // transactions for block inclusion.
    sortedEntries.clear();
    sortedEntries.insert(sortedEntries.begin(), package.begin(), package.end());
    std::sort(sortedEntries.begin(), sortedEntries.end(), CompareTxIterByAncestorCount());
}

void BlockAssembler::addCombinedBLSCT(const CStateViewCache& inputs)
{
    std::vector<CTransaction> vToCombine;
    std::vector<RangeproofEncodedData> blsctData;
    CValidationState state;

    for (auto &it: mempool.mapTx)
    {
        CTransaction tx = it.GetTx();

        if (!tx.IsBLSInput())
            continue;

        try
        {
            if (inputs.HaveInputs(tx) && VerifyBLSCT(tx, bls::BasicSchemeMPL::KeyGen(std::vector<uint8_t>(32, 0)), blsctData, inputs, state))
                vToCombine.push_back(tx);
            else
                LogPrintf("%s: Missing inputs or invalid blsct of %s (%s)\n", __func__, it.GetTx().GetHash().ToString(), FormatStateMessage(state));
        }
        catch(...)
        {
            continue;
        }
    }

    if (vToCombine.size() == 0)
        return;

    if (vToCombine.size() == 1)
    {
        pblock->vtx.push_back(vToCombine[0]);
        return;
    }

    CTransaction combinedTx;

    if (!CombineBLSCTTransactions(vToCombine, combinedTx, inputs, state))
    {
        LogPrintf("%s: Could not combine BLSCT transactions: %s\n", __func__, FormatStateMessage(state));
        return;
    }

    pblock->vtx.push_back(combinedTx);

}

// This transaction selection algorithm orders the mempool based
// on feerate of a transaction including all unconfirmed ancestors.
// Since we don't remove transactions from the mempool as we select them
// for block inclusion, we need an alternate method of updating the feerate
// of a transaction with its not-yet-selected ancestors as we go.
// This is accomplished by walking the in-mempool descendants of selected
// transactions and storing a temporary modified state in mapModifiedTxs.
// Each time through the loop, we compare the best transaction in
// mapModifiedTxs with the next transaction in the mempool to decide what
// transaction package to work on next.
void BlockAssembler::addPackageTxs()
{
    // mapModifiedTx will store sorted packages after they are modified
    // because some of their txs are already in the block
    indexed_modified_transaction_set mapModifiedTx;
    // Keep track of entries that failed inclusion, to avoid duplicate work
    CTxMemPool::setEntries failedTx;

    // Start by adding all descendants of previously added txs to mapModifiedTx
    // and modifying them for their already included ancestors
    UpdatePackagesForAdded(inBlock, mapModifiedTx);

    CTxMemPool::indexed_transaction_set::index<ancestor_score>::type::iterator mi = mempool.mapTx.get<ancestor_score>().begin();
    CTxMemPool::txiter iter;
    while (mi != mempool.mapTx.get<ancestor_score>().end() || !mapModifiedTx.empty())
    {
        // First try to find a new transaction in mapTx to evaluate.
        if (mi != mempool.mapTx.get<ancestor_score>().end() &&
                SkipMapTxEntry(mempool.mapTx.project<0>(mi), mapModifiedTx, failedTx)) {
            ++mi;
            continue;
        }

        // Now that mi is not stale, determine which transaction to evaluate:
        // the next entry from mapTx, or the best from mapModifiedTx?
        bool fUsingModified = false;

        modtxscoreiter modit = mapModifiedTx.get<ancestor_score>().begin();
        if (mi == mempool.mapTx.get<ancestor_score>().end()) {
            // We're out of entries in mapTx; use the entry from mapModifiedTx
            iter = modit->iter;
            fUsingModified = true;
        } else {
            // Try to compare the mapTx entry to the mapModifiedTx entry
            iter = mempool.mapTx.project<0>(mi);
            if (modit != mapModifiedTx.get<ancestor_score>().end() &&
                    CompareModifiedEntry()(*modit, CTxMemPoolModifiedEntry(iter))) {
                // The best entry in mapModifiedTx has higher score
                // than the one from mapTx.
                // Switch which transaction (package) to consider
                iter = modit->iter;
                fUsingModified = true;
            } else {
                // Either no entry in mapModifiedTx, or it's worse than mapTx.
                // Increment mi for the next loop iteration.
                ++mi;
            }
        }

        // We skip mapTx entries that are inBlock, and mapModifiedTx shouldn't
        // contain anything that is inBlock.
        assert(!inBlock.count(iter));

        uint64_t packageSize = iter->GetSizeWithAncestors();
        CAmount packageFees = iter->GetModFeesWithAncestors();
        int64_t packageSigOpsCost = iter->GetSigOpCostWithAncestors();
        if (fUsingModified) {
            packageSize = modit->nSizeWithAncestors;
            packageFees = modit->nModFeesWithAncestors;
            packageSigOpsCost = modit->nSigOpCostWithAncestors;
        }

        if (packageFees < ::minRelayTxFee.GetFee(packageSize)) {
            // Everything else we might consider has a lower fee rate
            return;
        }

        if (!TestPackage(packageSize, packageSigOpsCost)) {
            if (fUsingModified) {
                // Since we always look at the best entry in mapModifiedTx,
                // we must erase failed entries so that we can consider the
                // next best entry on the next loop iteration
                mapModifiedTx.get<ancestor_score>().erase(modit);
                failedTx.insert(iter);
            }
            continue;
        }

        CTxMemPool::setEntries ancestors;
        uint64_t nNoLimit = std::numeric_limits<uint64_t>::max();
        std::string dummy;
        mempool.CalculateMemPoolAncestors(*iter, ancestors, nNoLimit, nNoLimit, nNoLimit, nNoLimit, dummy, false);

        onlyUnconfirmed(ancestors);
        ancestors.insert(iter);

        // Test if all tx's are Final
        if (!TestPackageTransactions(ancestors)) {
            if (fUsingModified) {
                mapModifiedTx.get<ancestor_score>().erase(modit);
                failedTx.insert(iter);
            }
            continue;
        }

        // Package can be added. Sort the entries in a valid order.
        vector<CTxMemPool::txiter> sortedEntries;
        SortForBlock(ancestors, iter, sortedEntries);

        for (size_t i=0; i<sortedEntries.size(); ++i) {
            AddToBlock(sortedEntries[i]);
            // Erase from the modified set, if present
            mapModifiedTx.erase(sortedEntries[i]);
        }

        // Update transactions that depend on each of these
        UpdatePackagesForAdded(ancestors, mapModifiedTx);
    }
}

void BlockAssembler::addPriorityTxs(bool fProofOfStake, int blockTime)
{
    // How much of the block should be dedicated to high-priority transactions,
    // included regardless of the fees they pay
    unsigned int nBlockPrioritySize = GetArg("-blockprioritysize", DEFAULT_BLOCK_PRIORITY_SIZE);
    nBlockPrioritySize = std::min(nBlockMaxSize, nBlockPrioritySize);

    if (nBlockPrioritySize == 0) {
        return;
    }

    bool fSizeAccounting = fNeedSizeAccounting;
    fNeedSizeAccounting = true;

    // This vector will be sorted into a priority queue:
    vector<TxCoinAgePriority> vecPriority;
    TxCoinAgePriorityCompare pricomparer;
    std::map<CTxMemPool::txiter, double, CTxMemPool::CompareIteratorByHash> waitPriMap;
    typedef std::map<CTxMemPool::txiter, double, CTxMemPool::CompareIteratorByHash>::iterator waitPriIter;
    double actualPriority = -1;

    vecPriority.reserve(mempool.mapTx.size());
    for (CTxMemPool::indexed_transaction_set::iterator mi = mempool.mapTx.begin();
         mi != mempool.mapTx.end(); ++mi)
    {
        double dPriority = mi->GetPriority(nHeight);
        CAmount dummy;
        mempool.ApplyDeltas(mi->GetTx().GetHash(), dPriority, dummy);
        vecPriority.push_back(TxCoinAgePriority(dPriority, mi));
    }
    std::make_heap(vecPriority.begin(), vecPriority.end(), pricomparer);

    CTxMemPool::txiter iter;
    while (!vecPriority.empty() && !blockFinished) { // add a tx from priority queue to fill the blockprioritysize
        iter = vecPriority.front().second;
        actualPriority = vecPriority.front().first;
        std::pop_heap(vecPriority.begin(), vecPriority.end(), pricomparer);
        vecPriority.pop_back();

        // If tx already in block, skip
        if (inBlock.count(iter)) {
            assert(false); // shouldn't happen for priority txs
            continue;
        }

        // cannot accept witness transactions into a non-witness block
        if (!fIncludeWitness && !iter->GetTx().wit.IsNull())
            continue;

        if ((fProofOfStake && iter->GetTx().nTime > (unsigned int)blockTime))
            continue;

        // If tx is dependent on other mempool txs which haven't yet been included
        // then put it in the waitSet
        if (isStillDependent(iter)) {
            waitPriMap.insert(std::make_pair(iter, actualPriority));
            continue;
        }

        // If this tx fits in the block add it, otherwise keep looping
        if (TestForBlock(iter)) {
            AddToBlock(iter);

            // If now that this txs is added we've surpassed our desired priority size
            // or have dropped below the AllowFreeThreshold, then we're done adding priority txs
            if (nBlockSize >= nBlockPrioritySize || !AllowFree(actualPriority)) {
                break;
            }

            // This tx was successfully added, so
            // add transactions that depend on this one to the priority queue to try again
            for(CTxMemPool::txiter child: mempool.GetMemPoolChildren(iter))
            {
                waitPriIter wpiter = waitPriMap.find(child);
                if (wpiter != waitPriMap.end()) {
                    vecPriority.push_back(TxCoinAgePriority(wpiter->second,child));
                    std::push_heap(vecPriority.begin(), vecPriority.end(), pricomparer);
                    waitPriMap.erase(wpiter);
                }
            }
        }
    }
    fNeedSizeAccounting = fSizeAccounting;
}

void IncrementExtraNonce(CBlock* pblock, const CBlockIndex* pindexPrev, unsigned int& nExtraNonce)
{
    // Update nExtraNonce
    static uint256 hashPrevBlock;
    if (hashPrevBlock != pblock->hashPrevBlock)
    {
        nExtraNonce = 0;
        hashPrevBlock = pblock->hashPrevBlock;
    }
    ++nExtraNonce;
    unsigned int nHeight = pindexPrev->nHeight+1; // Height first in coinbase required for block.version=2
    CMutableTransaction txCoinbase(pblock->vtx[0]);
    txCoinbase.vin[0].scriptSig = (CScript() << nHeight << CScriptNum(nExtraNonce)) + COINBASE_FLAGS;
    assert(txCoinbase.vin[0].scriptSig.size() <= 100);

    pblock->vtx[0] = txCoinbase;
    pblock->hashMerkleRoot = BlockMerkleRoot(*pblock);
}

//////////////////////////////////////////////////////////////////////////////
//
// Internal Staker
//

extern unsigned int nMinerSleep;

void NavCoinStaker(const CChainParams& chainparams)
{
    LogPrintf("NavCoinStaker started\n");
    SetThreadPriority(THREAD_PRIORITY_LOWEST);
    RenameThread("navcoin-staker");

    boost::shared_ptr<CReserveScript> coinbaseScript;
    GetMainSignals().ScriptForMining(coinbaseScript);

    try {
        // Throw an error if no script was provided.  This can happen
        // due to some internal error but also if the keypool is empty.
        // In the latter case, already the pointer is NULL.
        if (!coinbaseScript || coinbaseScript->reserveScript.empty())
            throw std::runtime_error("No coinbase script available (staking requires a wallet)");

        while (true) {
            if (chainparams.MiningRequiresPeers()) {
                // Busy-wait for the network to come online so we don't waste time mining
                // on an obsolete chain. In regtest mode we expect to fly solo.
                do {
                    bool fvNodesEmpty;
                    {
                        LOCK(cs_vNodes);
                        fvNodesEmpty = vNodes.empty();
                    }
                    if (!fvNodesEmpty && !IsInitialBlockDownload())
                        break;
                    MilliSleep(1000);
                } while (true);
            }

            while (!fStaking)
            {
                MilliSleep(1000);
            }

            while (pwalletMain->IsLocked())
            {
                nLastCoinStakeSearchInterval = 0;
                MilliSleep(1000);
            }

            if (nLastTime != 0 && nLastSteadyTime != 0)
            {
                int64_t nClockDifference = GetTimeMillis() - nLastTime;
                int64_t nSteadyClockDifference = GetSteadyTime() - nLastSteadyTime;

                if(abs64(nClockDifference - nSteadyClockDifference) > 1000)
                {
                    fIncorrectTime = true;
                    LogPrintf("*** System clock change detected. Staking will be paused until the clock is synced again.\n");
                }
                if(fIncorrectTime) {
                    if(!NtpClockSync()) {
                        MilliSleep(10000);
                        continue;
                    } else {
                        fIncorrectTime = false;
                        LogPrintf("*** Starting staking thread again.\n");
                    }
                }
            }

            nLastTime = GetTimeMillis();
            nLastSteadyTime = GetSteadyTime();

            //
            // Create new block
            //
            uint64_t nFees = 0;
            std::string sLog = "";

            std::unique_ptr<CBlockTemplate> pblocktemplate(BlockAssembler(Params()).CreateNewBlock(coinbaseScript->reserveScript, true, &nFees, sLog));
            if (!pblocktemplate.get())
            {
                LogPrintf("Error in NavCoinStaker: Keypool ran out, please call keypoolrefill before restarting the staking thread\n");
                return;
            }
            CBlock *pblock = &pblocktemplate->block;

            //LogPrint("coinstake","Running NavCoinStaker with %u transactions in block (%u bytes)\n", pblock->vtx.size(),
            //     ::GetSerializeSize(*pblock, SER_NETWORK, PROTOCOL_VERSION));

            //Trying to sign a block
            if (SignBlock(pblock, *pwalletMain, nFees, sLog))
            {
                LogPrint("coinstake", "PoS Block signed\n");
                SetThreadPriority(THREAD_PRIORITY_NORMAL);
                CheckStake(pblock, *pwalletMain, chainparams);
                SetThreadPriority(THREAD_PRIORITY_LOWEST);
                MilliSleep(500);
            }
            else
                MilliSleep(nMinerSleep);

        }
    }
    catch (const boost::thread_interrupted&)
    {
        LogPrintf("NavCoinStaker terminated\n");
        throw;
    }
    catch (const std::runtime_error &e)
    {
        LogPrintf("NavCoinStaker runtime error: %s\n", e.what());
        return;
    }
}

bool SignBlock(CBlock *pblock, CWallet& wallet, int64_t nFees, std::string sLog)
{
  std::vector<CTransaction> vtx = pblock->vtx;
  // if we are trying to sign
  //    something except proof-of-stake block template
  if (!vtx[0].vout[0].IsEmpty()){
      LogPrintf("SignBlock() : Trying to sign malformed proof-of-stake block template\n");
      return false;
  }

  // if we are trying to sign
  //    a complete proof-of-stake block
  if (pblock->IsProofOfStake())
      return true;

  CStateViewCache view(pcoinsTip);

  static int64_t nLastCoinStakeSearchTime = GetAdjustedTime(); // startup timestamp

  CKey key;
  CMutableTransaction txCoinStake;
  CTransaction txNew;
  int nBestHeight = chainActive.Tip()->nHeight;

  txCoinStake.nTime = GetAdjustedTime();
  txCoinStake.nTime &= ~STAKE_TIMESTAMP_MASK;

  int64_t nSearchTime = txCoinStake.nTime; // search to current time

  if (nSearchTime > nLastCoinStakeSearchTime)
  {

      int64_t nSearchInterval = nBestHeight+1 > 0 ? 1 : nSearchTime - nLastCoinStakeSearchTime;
      CScript kernelScriptPubKey;
      if (wallet.CreateCoinStake(wallet, pblock->nBits, nSearchInterval, nFees, txCoinStake, key, kernelScriptPubKey))
      {
          if (txCoinStake.nTime >= chainActive.Tip()->GetPastTimeLimit()+1)
          {
              if (sLog != "")
                  LogPrintf("%s", sLog);

              // make sure coinstake would meet timestamp protocol
              //    as it would be the same as the block timestamp
              pblock->vtx[0].nTime = pblock->nTime = txCoinStake.nTime;

              // we have to make sure that we have no future timestamps in
              //    our transactions set
              for (vector<CTransaction>::iterator it = vtx.begin(); it != vtx.end();)
                  if (it->nTime > pblock->nTime) { it = vtx.erase(it); } else { ++it; }

              txCoinStake.nVersion = CTransaction::TXDZEEL_VERSION_V2;
              txCoinStake.strDZeel = sCoinStakeStrDZeel == "" ?
                          GetArg("-stakervote","") + ";" + std::to_string(CLIENT_VERSION) :
                          sCoinStakeStrDZeel;

              bool fCFund = IsCommunityFundEnabled(chainActive.Tip(), Params().GetConsensus());
              bool fDAOConsultations = IsDAOEnabled(chainActive.Tip(), Params().GetConsensus());
              bool fColdStakingv2 = IsColdStakingv2Enabled(chainActive.Tip(), Params().GetConsensus());
              bool fStakerIsColdStakingv2 = false;
              if (txCoinStake.vout[1].scriptPubKey.IsColdStakingv2())
                  fStakerIsColdStakingv2 = true;
              std::vector<unsigned char> stakerScript;

              CTransaction txPrev;
              uint256 hashBlock = uint256();

              if (IsVoteCacheStateEnabled(chainActive.Tip(), Params().GetConsensus())
                      && kernelScriptPubKey.GetStakerScript(stakerScript))
              {
                  LogPrint("daoextra", "%s: Reviewing vote outputs for stake script %s\n", __func__, HexStr(stakerScript));
                  CVoteList pVoteList;
                  view.GetCachedVoter(stakerScript, pVoteList);
                  std::map<uint256, int64_t> list = pVoteList.GetList();

                  std::map<uint256, int> mapCountAnswers;
                  std::map<uint256, int> mapCacheMaxAnswers;

                  CConsultation consultation;
                  CConsultationAnswer answer;

                  for (auto& it: list)
                  {
                      uint256 hash = it.first;
                      int64_t val = it.second;

                      bool fAnswer = view.HaveConsultationAnswer(hash) && view.GetConsultationAnswer(hash, answer);
                      bool fParent = false;
                      if (fAnswer)
                          fParent = view.HaveConsultation(answer.parent) && view.GetConsultation(answer.parent, consultation);

                      if (fAnswer && fParent)
                      {
                          if (answer.CanBeVoted(view) && val != VoteFlags::SUPPORT && val != VoteFlags::SUPPORT_REMOVE && val != VoteFlags::VOTE_REMOVE)
                          {
                              if (mapCacheMaxAnswers.count(answer.parent) == 0)
                                  mapCacheMaxAnswers[answer.parent] = consultation.nMax;

                              for (auto &it: pblock->vtx[0].vout)
                              {
                                  uint256 hash_;
                                  int64_t vote_;
                                  it.scriptPubKey.ExtractVote(hash_, vote_);

                                  if (hash == hash_)
                                  {
                                      mapCountAnswers[answer.parent]++;
                                      break;
                                  }
                              }
                          }
                      }
                  }

                  for (auto it = pblock->vtx[0].vout.begin(); it != pblock->vtx[0].vout.end();)
                  {
                      CTxOut out = *it;

                      if (fCFund && out.IsVote())
                      {
                          if (fColdStakingv2 && fStakerIsColdStakingv2)
                          {
                              LogPrint("daoextra", "%s: Removing vote output %s because the staker delegated to a light wallet\n", __func__, out.ToString());
                              it = pblock->vtx[0].vout.erase(it);
                              continue;
                          }

                          uint256 hash;
                          int64_t vote;
                          out.scriptPubKey.ExtractVote(hash, vote);

                          if (list.count(hash) > 0)
                          {
                              int64_t nval = list[hash];
                              if (nval == vote)
                              {
                                  LogPrint("daoextra", "%s: Removing vote output %s because it is already in the voting cache\n", __func__, out.ToString());
                                  it = pblock->vtx[0].vout.erase(it);
                                  continue;
                              }
                          }
                      }

                      if (fDAOConsultations && out.IsSupportVote())
                      {
                          if (fColdStakingv2 && fStakerIsColdStakingv2)
                          {
                              LogPrint("daoextra", "%s: Removing support vote output %s because the staker delegated to a light wallet\n", __func__, out.ToString());
                              it = pblock->vtx[0].vout.erase(it);
                              continue;
                          }

                          uint256 hash;
                          int64_t vote;
                          out.scriptPubKey.ExtractSupportVote(hash, vote);

                          if (list.count(hash) > 0)
                          {
                              int64_t nval = list[hash];

                              if (nval == vote)
                              {
                                  LogPrint("daoextra", "%s: Removing support vote output %s because it is already in the voting cache\n", __func__, out.ToString());
                                  it = pblock->vtx[0].vout.erase(it);
                                  continue;
                              }
                          }
                      }

                      if (fDAOConsultations && out.IsConsultationVote())
                      {
                          if (fColdStakingv2 && fStakerIsColdStakingv2)
                          {
                              LogPrint("daoextra", "%s: Removing consultation vote output %s because the staker delegated to a light wallet\n", __func__, out.ToString());
                              it = pblock->vtx[0].vout.erase(it);
                              continue;
                          }

                          uint256 hash;
                          int64_t vote;
                          out.scriptPubKey.ExtractConsultationVote(hash, vote);

                          if (view.HaveConsultationAnswer(hash) && view.GetConsultationAnswer(hash, answer))
                          {
                              CConsultation parentConsultation;
                              if (mapCacheMaxAnswers.count(answer.parent) == 0 && view.GetConsultation(answer.parent, parentConsultation))
                                  mapCacheMaxAnswers[answer.parent] = parentConsultation.nMax;
                              mapCountAnswers[answer.parent]++;
                              if (mapCountAnswers[answer.parent] > mapCacheMaxAnswers[answer.parent])
                              {
                                  LogPrint("daoextra", "%s: Removing consultation vote output %s because it already voted the max amount of times\n", __func__, out.ToString());
                                  it = pblock->vtx[0].vout.erase(it);
                                  continue;
                              }
                          }

                          if (list.count(hash) > 0)
                          {
                              int64_t nval = list[hash];

                              if (nval == vote)
                              {
                                  LogPrint("daoextra", "%s: Removing consultation vote output %s because it is already in the voting cache\n", __func__, out.ToString());
                                  it = pblock->vtx[0].vout.erase(it);
                                  continue;
                              }
                          }
                      }
                      ++it;
                  }

                  std::map<uint256, bool> votes;
                  std::map<uint256, bool> supports;

                  if (!fStakerIsColdStakingv2)
                  {
                      for (auto& it: list)
                      {
                          uint256 hash = it.first;
                          int64_t val = it.second;

                          bool fProposal = view.HaveProposal(hash);
                          bool fPaymentRequest = view.HavePaymentRequest(hash);

                          if (mapAddedVotes.count(hash) == 0 && votes.count(hash) == 0 && (fProposal || fPaymentRequest))
                          {
                              pblock->vtx[0].vout.insert(pblock->vtx[0].vout.begin(), CTxOut());

                              if (fProposal)
                                  SetScriptForProposalVote(pblock->vtx[0].vout[0].scriptPubKey, hash, VoteFlags::VOTE_REMOVE);
                              else if (fPaymentRequest)
                                  SetScriptForPaymentRequestVote(pblock->vtx[0].vout[0].scriptPubKey, hash, VoteFlags::VOTE_REMOVE);

                              pblock->vtx[0].vout[0].nValue = 0;
                              LogPrint("daoextra", "%s: Adding remove-vote output %s\n", __func__, pblock->vtx[0].vout[0].ToString());
                              votes[hash] = true;
                          }

                          bool fConsultation = view.HaveConsultation(hash) && view.GetConsultation(hash, consultation);
                          bool fAnswer = view.HaveConsultationAnswer(hash) && view.GetConsultationAnswer(hash, answer);

                          if (fConsultation || fAnswer)
                          {
                              if (val == VoteFlags::SUPPORT && mapSupported.count(hash) == 0 && supports.count(hash) == 0)
                              {
                                  pblock->vtx[0].vout.insert(pblock->vtx[0].vout.begin(), CTxOut());

                                  SetScriptForConsultationSupportRemove(pblock->vtx[0].vout[0].scriptPubKey, hash);
                                  pblock->vtx[0].vout[0].nValue = 0;
                                  LogPrint("daoextra", "%s: Adding remove-support output %s\n", __func__, pblock->vtx[0].vout[0].ToString());
                                  supports[hash] = true;
                              }

                              if (val != VoteFlags::SUPPORT && mapAddedVotes.count(hash) == 0 && votes.count(hash) == 0 && val != VoteFlags::SUPPORT_REMOVE && val != VoteFlags::VOTE_REMOVE)
                              {
                                  pblock->vtx[0].vout.insert(pblock->vtx[0].vout.begin(), CTxOut());

                                  SetScriptForConsultationVoteRemove(pblock->vtx[0].vout[0].scriptPubKey, hash);
                                  pblock->vtx[0].vout[0].nValue = 0;
                                  LogPrint("daoextra", "%s: Adding remove-vote output %s\n", __func__, pblock->vtx[0].vout[0].ToString());
                                  votes[hash] = true;
                              }
                          }
                      }
                  }
              }

              for(unsigned int i = 0; i < vCoinStakeOutputs.size(); i++)
              {
                  CTxOut forcedTxOut;
                  if (!DecodeHexTxOut(forcedTxOut, vCoinStakeOutputs[i]))
                      LogPrintf("Tried to force a wrong transaction output in the coinstake: %s\n", vCoinStakeOutputs[i]);
                  else
                      txCoinStake.vout.insert(txCoinStake.vout.end(), forcedTxOut);
              }

              for(unsigned int i = 0; i < vCoinStakeInputs.size(); i++)
              {
                  CTxIn forcedTxIn;
                  if (!DecodeHexTxIn(forcedTxIn, vCoinStakeInputs[i]))
                      LogPrintf("Tried to force a wrong transaction input in the coinstake: %s\n", vCoinStakeInputs[i]);
                  else
                      txCoinStake.vin.insert(txCoinStake.vin.end(), forcedTxIn);
              }

              // After the changes, we need to resign inputs.

              CTransaction txNewConst(txCoinStake);
              for(unsigned int i = 0; i < txCoinStake.vin.size(); i++)
              {
                  bool signSuccess = false;
                  uint256 prevHash = txCoinStake.vin[i].prevout.hash;
                  uint32_t n = txCoinStake.vin[i].prevout.n;
                  assert(pwalletMain->mapWallet.count(prevHash));
                  CWalletTx& prevTx = pwalletMain->mapWallet[prevHash];
                  const CScript& scriptPubKey = prevTx.vout[n].scriptPubKey;
                  SignatureData sigdata;
                  signSuccess = ProduceSignature(TransactionSignatureCreator(&wallet, &txNewConst, i, prevTx.vout[n].nValue, SIGHASH_ALL), scriptPubKey, sigdata, true);

                  if (!signSuccess) {
                      return false;
                  } else {
                      UpdateTransaction(txCoinStake, i, sigdata);
                  }
              }

              *static_cast<CTransaction*>(&txNew) = CTransaction(txCoinStake);
              pblock->vtx.insert(pblock->vtx.begin() + 1, txNew);

              for(unsigned int i = 0; i < vForcedTransactions.size(); i++)
              {
                  CTransaction forcedTx;
                  if (!DecodeHexTx(forcedTx, vForcedTransactions[i]))
                      LogPrintf("Tried to force a wrong transaction in a block: %s\n", vForcedTransactions[i]);
                  else
                      pblock->vtx.insert(pblock->vtx.begin() + 2, forcedTx);
              }

              pblock->vtx[0].UpdateHash();

              pblock->hashMerkleRoot = BlockMerkleRoot(*pblock);
              return key.Sign(pblock->GetHash(), pblock->vchBlockSig);
          }
      }
      nLastCoinStakeSearchInterval = nSearchTime - nLastCoinStakeSearchTime;
      nLastCoinStakeSearchTime = nSearchTime;
  }

  return false;
}

bool CheckStake(CBlock* pblock, CWallet& wallet, const CChainParams& chainparams)
{
    arith_uint256 proofHash = arith_uint256(0), hashTarget = arith_uint256(0);
    uint256 hashBlock = pblock->GetHash();

    if(!pblock->IsProofOfStake())
        return error("CheckStake() : %s is not a proof-of-stake block", hashBlock.GetHex());

    if (mapBlockIndex.count(pblock->hashPrevBlock) == 0)
        return error("CheckStake(): could not find previous block");

    CStateViewCache view(pcoinsTip);

    // verify hash target and signature of coinstake tx
    if (!CheckProofOfStake(mapBlockIndex[pblock->hashPrevBlock], pblock->vtx[1], pblock->nBits, proofHash, hashTarget, nullptr, view, false))
        return error("CheckStake() : proof-of-stake checking failed");

    //// debug print
    LogPrintf("CheckStake() : new proof-of-stake block found hash: %s\n", hashBlock.GetHex());

    // Found a solution
    {
        LOCK(cs_main);
        if (pblock->hashPrevBlock != hashBestChain)
            return error("CheckStake() : generated block is stale");

        // Track how many getdata requests this block gets
        {
            LOCK(wallet.cs_wallet);
            wallet.mapRequestCount[hashBlock] = 0;
        }

        GetMainSignals().BlockFound(pblock->GetHash());

        // Process this block the same as if we had received it from another node
        CValidationState state;
        if (!ProcessNewBlock(state, chainparams, nullptr, pblock, true, nullptr))
        {
            return error("NavCoinStaker: ProcessNewBlock, block not accepted");
        }
        else
        {
            sCoinBaseStrDZeel = sCoinStakeStrDZeel = "";
            vForcedTransactions.clear();
            vCoinBaseOutputs.clear();
            vCoinStakeOutputs.clear();
            vCoinStakeInputs.clear();
        }
    }

    return true;
}

void SetStaking(bool mode) {
    fStaking = mode;
}

bool GetStaking() {
    return fStaking;
}

void SetCoinBaseOutputs(std::vector<std::string> v){
    vCoinBaseOutputs.clear();
    if(!v.empty())
        vCoinBaseOutputs.insert(vCoinBaseOutputs.end(), v.begin(), v.end());
}

void SetCoinStakeOutputs(std::vector<std::string> v){
    vCoinStakeOutputs.clear();
    if(!v.empty())
        vCoinStakeOutputs.insert(vCoinStakeOutputs.end(), v.begin(), v.end());
}

void SetCoinStakeInputs(std::vector<std::string> v){
    vCoinStakeInputs.clear();
    if(!v.empty())
        vCoinStakeInputs.insert(vCoinStakeInputs.end(), v.begin(), v.end());
}

void SetForceTransactions(std::vector<std::string> v){
    vForcedTransactions.clear();
    if(!v.empty())
        vForcedTransactions.insert(vForcedTransactions.end(), v.begin(), v.end());
}
void SetCoinStakeStrDZeel(std::string s){
    sCoinStakeStrDZeel = s;
}
void SetCoinBaseStrDZeel(std::string s){
    sCoinBaseStrDZeel = s;
}
std::vector<std::string> GetCoinBaseOutputs(){
    return vCoinBaseOutputs;
}
std::vector<std::string> GetCoinStakeOutputs(){
    return vCoinStakeOutputs;
}
std::vector<std::string> GetCoinStakeInputs(){
    return vCoinStakeInputs;
}
std::vector<std::string> GetForceTransactions(){
    return vForcedTransactions;
}
std::string GetCoinStakeStrDZeel(){
    return sCoinStakeStrDZeel;
}
std::string GetCoinBaseStrDZeel(){
    return sCoinBaseStrDZeel;
}
