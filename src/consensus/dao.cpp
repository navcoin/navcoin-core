// Copyright (c) 2018-2019 The NavCoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "consensus/dao.h"
#include "base58.h"
#include "main.h"
#include "rpc/server.h"
#include "utilmoneystr.h"

void GetVersionMask(uint64_t &nProposalMask, uint64_t &nPaymentRequestMask, uint64_t &nConsultationMask, uint64_t &nConsultationAnswerMask, CBlockIndex* pindex)
{
    bool fReducedQuorum = IsReducedCFundQuorumEnabled(pindex, Params().GetConsensus());
    bool fAbstainVote = IsAbstainVoteEnabled(pindex, Params().GetConsensus());

    nProposalMask = Params().GetConsensus().nProposalMaxVersion;
    nPaymentRequestMask = Params().GetConsensus().nPaymentRequestMaxVersion;
    nConsultationMask = Params().GetConsensus().nConsultationMaxVersion;
    nConsultationAnswerMask = Params().GetConsensus().nConsultationAnswerMaxVersion;

    if (!fReducedQuorum)
    {
        nProposalMask &= ~CProposal::REDUCED_QUORUM_VERSION;
        nPaymentRequestMask &= ~CPaymentRequest::REDUCED_QUORUM_VERSION;
    }

    if (!fAbstainVote)
    {
        nProposalMask &= ~CProposal::ABSTAIN_VOTE_VERSION;
        nPaymentRequestMask &= ~CPaymentRequest::ABSTAIN_VOTE_VERSION;
    }
}

void SetScriptForCommunityFundContribution(CScript &script)
{
    script.resize(2);
    script[0] = OP_RETURN;
    script[1] = OP_CFUND;
}

void SetScriptForProposalVote(CScript &script, uint256 proposalhash, int64_t vote)
{
    script.resize(37);
    script[0] = OP_RETURN;
    script[1] = OP_CFUND;
    script[2] = OP_PROP;
    script[3] = vote == VoteFlags::VOTE_REMOVE ? OP_REMOVE : (vote == VoteFlags::VOTE_ABSTAIN ? OP_ABSTAIN : (vote == VoteFlags::VOTE_YES ? OP_YES : OP_NO));
    script[4] = 0x20;
    memcpy(&script[5], proposalhash.begin(), 32);
}

void SetScriptForPaymentRequestVote(CScript &script, uint256 prequesthash, int64_t vote)
{
    script.resize(37);
    script[0] = OP_RETURN;
    script[1] = OP_CFUND;
    script[2] = OP_PREQ;
    script[3] = vote == VoteFlags::VOTE_REMOVE ? OP_REMOVE : (vote == VoteFlags::VOTE_ABSTAIN ? OP_ABSTAIN : (vote == VoteFlags::VOTE_YES ? OP_YES : OP_NO));
    script[4] = 0x20;
    memcpy(&script[5], prequesthash.begin(), 32);
}

void SetScriptForConsultationSupport(CScript &script, uint256 hash)
{
    script.resize(36);
    script[0] = OP_RETURN;
    script[1] = OP_DAO;
    script[2] = OP_YES;
    script[3] = 0x20;
    memcpy(&script[4], hash.begin(), 32);
}

void SetScriptForConsultationSupportRemove(CScript &script, uint256 hash)
{
    script.resize(36);
    script[0] = OP_RETURN;
    script[1] = OP_DAO;
    script[2] = OP_REMOVE;
    script[3] = 0x20;
    memcpy(&script[4], hash.begin(), 32);
}

void SetScriptForConsultationVote(CScript &script, uint256 hash, int64_t vote)
{
    script.resize(36);
    script[0] = OP_RETURN;
    script[1] = OP_CONSULTATION;
    script[2] = vote == VoteFlags::VOTE_ABSTAIN ? OP_ABSTAIN : OP_ANSWER;
    script[3] = 0x20;
    memcpy(&script[4], hash.begin(), 32);
    if (vote > -1)
        script << CScriptNum(vote);
}

void SetScriptForConsultationVoteRemove(CScript &script, uint256 hash)
{
    script.resize(36);
    script[0] = OP_RETURN;
    script[1] = OP_CONSULTATION;
    script[2] = OP_REMOVE;
    script[3] = 0x20;
    memcpy(&script[4], hash.begin(), 32);
}

bool Support(uint256 hash, bool &duplicate)
{
    AssertLockHeld(cs_main);

    std::string str = hash.ToString();

    if (mapSupported.count(hash) > 0)
        duplicate = true;

    RemoveConfigFile("support", str);
    RemoveConfigFile("support", str);

    std::string strCmd = "support";

    WriteConfigFile(strCmd, str);

    mapSupported[hash] = true;

    LogPrint("dao", "%s: Supporting %s\n", __func__, str);

    return true;
}

bool Vote(uint256 hash, int64_t vote, bool &duplicate)
{
    AssertLockHeld(cs_main);

    std::string str = hash.ToString();

    if (mapAddedVotes.count(hash) > 0 && mapAddedVotes[hash] == vote)
        duplicate = true;

    RemoveConfigFile("voteyes", str);
    RemoveConfigFile("voteabs", str);
    RemoveConfigFile("voteno", str);
    RemoveConfigFile("addproposalvoteyes", str);
    RemoveConfigFile("addproposalvoteabs", str);
    RemoveConfigFile("addproposalvoteno", str);

    CConsultationAnswer answer;

    CStateViewCache view(pcoinsTip);

    if (view.GetConsultationAnswer(hash, answer))
    {
        std::string strParent = answer.parent.ToString();
        mapAddedVotes.erase(answer.parent);
        RemoveConfigFile("voteyes", strParent);
        RemoveConfigFile("voteabs", strParent);
        RemoveConfigFile("voteno", strParent);
        RemoveConfigFile("addproposalvoteyes", strParent);
        RemoveConfigFile("addproposalvoteabs", strParent);
        RemoveConfigFile("addproposalvoteno", strParent);
    }

    std::string strCmd = "voteno";

    if (vote == -1)
        strCmd = "voteabs";

    if (vote == 1)
        strCmd = "voteyes";

    WriteConfigFile(strCmd, str);

    mapAddedVotes[hash] = vote;

    LogPrint("dao", "%s: Voting %d for %s\n", __func__, vote, hash.ToString());

    return true;
}

bool VoteValue(uint256 hash, int64_t vote, bool &duplicate)
{
    AssertLockHeld(cs_main);

    std::string str = hash.ToString();

    if (mapAddedVotes.count(hash) > 0 && mapAddedVotes[hash] == vote)
        duplicate = true;

    RemoveConfigFile("vote", str);

    WriteConfigFile("vote", str + "~" + to_string(vote));

    mapAddedVotes[hash] = vote;

    LogPrint("dao", "%s: Voting %d for %s\n", __func__, vote, hash.ToString());

    return true;
}

bool RemoveSupport(string str)
{
    RemoveConfigFile("support", str);
    if (mapSupported.count(uint256S(str)) > 0)
    {
        mapSupported.erase(uint256S(str));
        LogPrint("dao", "%s: Removing support for %s\n", __func__, str);
    }
    else
        return false;
    return true;
}

bool RemoveVote(string str)
{

    RemoveConfigFile("voteyes", str);
    RemoveConfigFile("voteabs", str);
    RemoveConfigFile("voteno", str);
    RemoveConfigFile("addproposalvoteyes", str);
    RemoveConfigFile("addproposalvoteabs", str);
    RemoveConfigFile("addproposalvoteno", str);
    RemoveConfigFile("addpaymentrequestvoteyes", str);
    RemoveConfigFile("addpaymentrequestvoteabs", str);
    RemoveConfigFile("addpaymentrequestvoteno", str);
    if (mapAddedVotes.count(uint256S(str)) > 0)
    {
        mapAddedVotes.erase(uint256S(str));
        LogPrint("dao", "%s: Removing vote for %s\n", __func__, str);
    }
    else
        return false;
    return true;
}

bool RemoveVoteValue(string str)
{

    RemoveConfigFilePair("vote", str);
    if (mapAddedVotes.count(uint256S(str)) > 0)
    {
        mapAddedVotes.erase(uint256S(str));
        LogPrint("dao", "%s: Removing support for %s\n", __func__, str);
    }
    else
        return false;
    return true;
}

bool RemoveVote(uint256 hash)
{
    return RemoveVote(hash.ToString());
}

bool IsBeginningCycle(const CBlockIndex* pindex, CChainParams params)
{
    return pindex->nHeight % GetConsensusParameter(Consensus::CONSENSUS_PARAM_VOTING_CYCLE_LENGTH) == 0;
}

bool IsEndCycle(const CBlockIndex* pindex, CChainParams params)
{
    return (pindex->nHeight+1) % GetConsensusParameter(Consensus::CONSENSUS_PARAM_VOTING_CYCLE_LENGTH) == 0;
}

std::string FormatConsensusParameter(Consensus::ConsensusParamsPos pos, std::string string)
{
    std::string ret = string;

    if (Consensus::vConsensusParamsType[pos] == Consensus::TYPE_NAV)
        ret = FormatMoney(stoll(string)) + " NAV";
    else if (Consensus::vConsensusParamsType[pos] == Consensus::TYPE_PERCENT)
    {
        std::ostringstream out;
        out.precision(2);
        out << std::fixed << (float)stoll(string) / 100.0;
        ret =  out.str() + "%";
    }

    return ret;
}

std::string RemoveFormatConsensusParameter(Consensus::ConsensusParamsPos pos, std::string string)
{
    string.erase(std::remove_if(string.begin(), string.end(),
    [](const char& c ) -> bool { return !std::isdigit(c) && c != '.' && c != ',' && c != '-'; } ), string.end());

    std::string ret = string;

    try
    {
    if (Consensus::vConsensusParamsType[pos] == Consensus::TYPE_NAV)
        ret = std::to_string((uint64_t)(stof(string) * COIN));
    else if (Consensus::vConsensusParamsType[pos] == Consensus::TYPE_PERCENT)
    {
        ret = std::to_string((uint64_t)(stof(string) * 100));
    }
    }
    catch(...)
    {
        return "";
    }

    return ret;
}

std::map<uint256, std::pair<std::pair<int, int>, int>> mapCacheProposalsToUpdate;
std::map<uint256, std::pair<std::pair<int, int>, int>> mapCachePaymentRequestToUpdate;
std::map<uint256, int> mapCacheSupportToUpdate;
std::map<std::pair<uint256, int64_t>, int> mapCacheConsultationToUpdate;

bool VoteStep(const CValidationState& state, CBlockIndex *pindexNew, const bool fUndo, CStateViewCache& view)
{
    AssertLockHeld(cs_main);

    const CBlockIndex* pindexDelete;
    if (fUndo)
    {
        pindexDelete = pindexNew;
        pindexNew = pindexNew->pprev;
        assert(pindexNew);
    }

    int64_t nTimeStart = GetTimeMicros();
    int nBlocks = (pindexNew->nHeight % GetConsensusParameter(Consensus::CONSENSUS_PARAM_VOTING_CYCLE_LENGTH)) + 1;
    const CBlockIndex* pindexblock = pindexNew;

    std::map<uint256, bool> mapSeen;
    std::map<uint256, bool> mapSeenSupport;

    if (fUndo || nBlocks == 1 || mapCacheProposalsToUpdate.empty() || mapCachePaymentRequestToUpdate.empty() || mapCacheSupportToUpdate.empty() || mapCacheConsultationToUpdate.empty()) {
        mapCacheProposalsToUpdate.clear();
        mapCachePaymentRequestToUpdate.clear();
        mapCacheSupportToUpdate.clear();
        mapCacheConsultationToUpdate.clear();
    } else {
        nBlocks = 1;
    }

    int64_t nTimeStart2 = GetTimeMicros();

    LogPrint("dao", "%s: Scanning %d block(s) starting at %d\n", __func__, nBlocks, pindexblock->nHeight);

    while(nBlocks > 0 && pindexblock != NULL)
    {
        CProposal proposal;
        CPaymentRequest prequest;
        CConsultation consultation;
        CConsultationAnswer answer;

        mapSeen.clear();
        mapSeenSupport.clear();

        for(unsigned int i = 0; i < pindexblock->vProposalVotes.size(); i++)
        {
            if(mapSeen.count(pindexblock->vProposalVotes[i].first) == 0)
            {
                LogPrint("dao", "%s: Found vote %d for proposal %s at block height %d\n", __func__,
                         pindexblock->vProposalVotes[i].second, pindexblock->vProposalVotes[i].first.ToString(),
                         pindexblock->nHeight);

                if(mapCacheProposalsToUpdate.count(pindexblock->vProposalVotes[i].first) == 0)
                    mapCacheProposalsToUpdate[pindexblock->vProposalVotes[i].first] = make_pair(make_pair(0, 0), 0);

                if(pindexblock->vProposalVotes[i].second == VoteFlags::VOTE_YES)
                    mapCacheProposalsToUpdate[pindexblock->vProposalVotes[i].first].first.first += 1;
                else if(pindexblock->vProposalVotes[i].second == VoteFlags::VOTE_ABSTAIN)
                    mapCacheProposalsToUpdate[pindexblock->vProposalVotes[i].first].second += 1;
                else if(pindexblock->vProposalVotes[i].second == VoteFlags::VOTE_NO)
                    mapCacheProposalsToUpdate[pindexblock->vProposalVotes[i].first].first.second += 1;

                mapSeen[pindexblock->vProposalVotes[i].first]=true;
            }
        }

        for(unsigned int i = 0; i < pindexblock->vPaymentRequestVotes.size(); i++)
        {
            if(mapSeen.count(pindexblock->vPaymentRequestVotes[i].first) == 0)
            {
                LogPrint("dao", "%s: Found vote %d for payment request %s at block height %d\n", __func__,
                         pindexblock->vPaymentRequestVotes[i].second, pindexblock->vPaymentRequestVotes[i].first.ToString(),
                         pindexblock->nHeight);

                if(mapCachePaymentRequestToUpdate.count(pindexblock->vPaymentRequestVotes[i].first) == 0)
                    mapCachePaymentRequestToUpdate[pindexblock->vPaymentRequestVotes[i].first] = make_pair(make_pair(0, 0), 0);

                if(pindexblock->vPaymentRequestVotes[i].second == VoteFlags::VOTE_YES)
                    mapCachePaymentRequestToUpdate[pindexblock->vPaymentRequestVotes[i].first].first.first += 1;
                else if(pindexblock->vPaymentRequestVotes[i].second == VoteFlags::VOTE_ABSTAIN)
                    mapCachePaymentRequestToUpdate[pindexblock->vPaymentRequestVotes[i].first].second += 1;
                else if(pindexblock->vPaymentRequestVotes[i].second == VoteFlags::VOTE_NO)
                    mapCachePaymentRequestToUpdate[pindexblock->vPaymentRequestVotes[i].first].first.second += 1;

                mapSeen[pindexblock->vPaymentRequestVotes[i].first]=true;
            }
        }

        for (auto& it: pindexblock->mapSupport)
        {
            if (!it.second)
                continue;

            if (!mapSeenSupport.count(it.first))
            {
                LogPrint("dao", "%s: Found support vote for %s at block height %d\n", __func__,
                         it.first.ToString(),
                         pindexblock->nHeight);

                if(mapCacheSupportToUpdate.count(it.first) == 0)
                    mapCacheSupportToUpdate[it.first] = 0;

                mapCacheSupportToUpdate[it.first] += 1;
                mapSeenSupport[it.first]=true;
            }
        }

        for (auto&it: pindexblock->mapConsultationVotes)
        {
            if (mapSeen.count(it.first))
                continue;

            mapSeen[it.first]=true;

            if (view.HaveConsultation(it.first) || view.HaveConsultationAnswer(it.first))
            {
                LogPrint("dao", "%s: Found consultation vote %d for %s at block height %d\n", __func__,
                         it.second, it.first.ToString(), pindexblock->nHeight);

                if (it.second == VoteFlags::VOTE_ABSTAIN && view.GetConsultationAnswer(it.first, answer))
                {
                    if(mapCacheConsultationToUpdate.count(std::make_pair(answer.parent,it.second)) == 0)
                        mapCacheConsultationToUpdate[std::make_pair(answer.parent,it.second)] = 0;

                    mapCacheConsultationToUpdate[std::make_pair(answer.parent,it.second)] += 1;
                }
                else
                {
                    if(mapCacheConsultationToUpdate.count(std::make_pair(it.first,it.second)) == 0)
                        mapCacheConsultationToUpdate[std::make_pair(it.first,it.second)] = 0;

                    mapCacheConsultationToUpdate[std::make_pair(it.first,it.second)] += 1;
                }
            }
        }
        pindexblock = pindexblock->pprev;
        nBlocks--;
    }

    mapSeen.clear();
    mapSeenSupport.clear();

    int64_t nTimeEnd2 = GetTimeMicros();
    LogPrint("bench", "   - CFund count votes from headers: %.2fms\n", (nTimeEnd2 - nTimeStart2) * 0.001);

    int64_t nTimeStart3 = GetTimeMicros();

    bool fLog = LogAcceptCategory("dao");

    for(auto& it: mapCacheProposalsToUpdate)
    {
        if (view.HaveProposal(it.first))
        {
            CProposalModifier proposal = view.ModifyProposal(it.first, pindexNew->nHeight);
            proposal->nVotesYes = it.second.first.first;
            proposal->nVotesAbs = it.second.second;
            proposal->nVotesNo = it.second.first.second;
            proposal->fDirty = true;
            mapSeen[proposal->hash]=true;
        }
    }

    for(auto& it: mapCachePaymentRequestToUpdate)
    {
        if(view.HavePaymentRequest(it.first))
        {
            CPaymentRequestModifier prequest = view.ModifyPaymentRequest(it.first, pindexNew->nHeight);
            prequest->nVotesYes = it.second.first.first;
            prequest->nVotesAbs = it.second.second;
            prequest->nVotesNo = it.second.first.second;
            prequest->fDirty = true;
            mapSeen[prequest->hash]=true;
        }
    }

    for(auto& it: mapCacheSupportToUpdate)
    {
        if (view.HaveConsultation(it.first))
        {
            CConsultationModifier consultation = view.ModifyConsultation(it.first, pindexNew->nHeight);
            consultation->nSupport = it.second;
            consultation->fDirty = true;
            mapSeenSupport[consultation->hash]=true;
        }
        else if (view.HaveConsultationAnswer(it.first))
        {
            CConsultationAnswerModifier answer = view.ModifyConsultationAnswer(it.first, pindexNew->nHeight);
            answer->nSupport = it.second;
            answer->fDirty = true;
            mapSeenSupport[answer->hash]=true;
        }
    }

    std::map<uint256, bool> mapAlreadyCleared;

    for(auto& it: mapCacheConsultationToUpdate)
    {

        if (view.HaveConsultation(it.first.first))
        {
            CConsultationModifier consultation = view.ModifyConsultation(it.first.first, pindexNew->nHeight);

            if (mapAlreadyCleared.count(it.first.first) == 0)
            {
                mapAlreadyCleared[it.first.first] = true;
                consultation->mapVotes.clear();
                consultation->fDirty = true;
            }
            consultation->mapVotes[it.first.second] = it.second;
            mapSeen[it.first.first]=true;
        }
        else if (view.HaveConsultationAnswer(it.first.first))
        {
            CConsultationAnswerModifier answer = view.ModifyConsultationAnswer(it.first.first, pindexNew->nHeight);
            answer->nVotes = it.second;
            answer->fDirty = true;
            mapSeen[it.first.first]=true;
            mapSeen[answer->parent]=true;
        }
    }

    int64_t nTimeEnd3 = GetTimeMicros();
    LogPrint("bench", "   - CFund update votes: %.2fms\n", (nTimeEnd3 - nTimeStart3) * 0.001);

    int64_t nTimeStart4 = GetTimeMicros();
    CPaymentRequestMap mapPaymentRequests;

    if(view.GetAllPaymentRequests(mapPaymentRequests))
    {
        for (CPaymentRequestMap::iterator it = mapPaymentRequests.begin(); it != mapPaymentRequests.end(); it++)
        {
            if (!view.HavePaymentRequest(it->first))
                continue;

            CPaymentRequestModifier prequest = view.ModifyPaymentRequest(it->first, pindexNew->nHeight);

            if (prequest->txblockhash == uint256())
                continue;

            CBlockIndex* pblockindex = mapBlockIndex[prequest->txblockhash];

            uint64_t nCreatedOnCycle = (pblockindex->nHeight / GetConsensusParameter(Consensus::CONSENSUS_PARAM_VOTING_CYCLE_LENGTH));
            uint64_t nCurrentCycle = (pindexNew->nHeight / GetConsensusParameter(Consensus::CONSENSUS_PARAM_VOTING_CYCLE_LENGTH));
            uint64_t nElapsedCycles = std::max(nCurrentCycle - nCreatedOnCycle, (uint64_t)0);
            uint64_t nVotingCycles = std::min(nElapsedCycles, GetConsensusParameter(Consensus::CONSENSUS_PARAM_PAYMENT_REQUEST_MAX_VOTING_CYCLES) + 1);

            auto oldCycle = prequest->nVotingCycle;

            CProposal proposal;

            if (!view.GetProposal(prequest->proposalhash, proposal))
                continue;

            if(fUndo)
            {
                prequest->ClearState(pindexDelete);
                if(prequest->GetLastState() == DAOFlags::NIL)
                    prequest->nVotingCycle = nVotingCycles;
            }
            else
            {
                auto oldState = prequest->GetLastState();

                if(oldState == DAOFlags::NIL && nVotingCycles != prequest->nVotingCycle)
                {
                    prequest->nVotingCycle = nVotingCycles;
                }

                if((pindexNew->nHeight + 1) % GetConsensusParameter(Consensus::CONSENSUS_PARAM_VOTING_CYCLE_LENGTH) == 0)
                {
                    if(prequest->IsExpired())
                    {
                        if (oldState != DAOFlags::EXPIRED)
                        {
                            prequest->SetState(pindexNew, DAOFlags::EXPIRED);
                        }
                    }
                    else if(prequest->IsRejected())
                    {
                        if (oldState != DAOFlags::REJECTED)
                        {
                            prequest->SetState(pindexNew, DAOFlags::REJECTED);
                        }
                    }
                    else if(oldState == DAOFlags::NIL)
                    {
                        flags proposalOldState = proposal.GetLastState();
                        if((proposalOldState == DAOFlags::ACCEPTED || proposalOldState == DAOFlags::PENDING_VOTING_PREQ) && prequest->IsAccepted())
                        {
                            if(prequest->nAmount <= pindexNew->nCFLocked && prequest->nAmount <= proposal.GetAvailable(view))
                            {
                                pindexNew->nCFLocked -= prequest->nAmount;
                                prequest->SetState(pindexNew, DAOFlags::ACCEPTED);
                                LogPrint("dao", "%s: Updated nCFSupply %s nCFLocked %s\n", __func__, FormatMoney(pindexNew->nCFSupply), FormatMoney(pindexNew->nCFLocked));
                            }
                        }
                    }
                }
            }

            if((pindexNew->nHeight) % GetConsensusParameter(Consensus::CONSENSUS_PARAM_VOTING_CYCLE_LENGTH) == 0)
            {
                flags proposalState = proposal.GetLastState();

                if (!mapSeen.count(prequest->hash) && prequest->GetLastState() == DAOFlags::NIL &&
                        !((proposalState == DAOFlags::ACCEPTED || proposalState == DAOFlags::PENDING_VOTING_PREQ) && prequest->IsAccepted())){
                    prequest->nVotesYes = 0;
                    prequest->nVotesNo = 0;
                }
            }
        }
    }

    int64_t nTimeEnd4 = GetTimeMicros();
    LogPrint("bench", "   - CFund update payment request status: %.2fms\n", (nTimeEnd4 - nTimeStart4) * 0.001);

    int64_t nTimeStart5 = GetTimeMicros();
    CProposalMap mapProposals;

    if(view.GetAllProposals(mapProposals))
    {
        for (CProposalMap::iterator it = mapProposals.begin(); it != mapProposals.end(); it++)
        {
            if (!view.HaveProposal(it->first))
                continue;

            CProposalModifier proposal = view.ModifyProposal(it->first, pindexNew->nHeight);

            if (proposal->txblockhash == uint256())
                continue;

            if (mapBlockIndex.count(proposal->txblockhash) == 0)
            {
                LogPrintf("%s: Can't find block %s of proposal %s\n",
                          __func__, proposal->txblockhash.ToString(), proposal->hash.ToString());
                continue;
            }

            CBlockIndex* pblockindex = mapBlockIndex[proposal->txblockhash];

            uint64_t nCreatedOnCycle = (pblockindex->nHeight / GetConsensusParameter(Consensus::CONSENSUS_PARAM_VOTING_CYCLE_LENGTH));
            uint64_t nCurrentCycle = (pindexNew->nHeight / GetConsensusParameter(Consensus::CONSENSUS_PARAM_VOTING_CYCLE_LENGTH));
            uint64_t nElapsedCycles = std::max(nCurrentCycle - nCreatedOnCycle, (uint64_t)0);
            uint64_t nVotingCycles = std::min(nElapsedCycles, GetConsensusParameter(Consensus::CONSENSUS_PARAM_PROPOSAL_MAX_VOTING_CYCLES) + 1);

            auto oldCycle = proposal->nVotingCycle;

            if(fUndo)
            {
                proposal->ClearState(pindexDelete);
                if(proposal->GetLastState() == DAOFlags::NIL)
                    proposal->nVotingCycle = nVotingCycles;
            }
            else
            {
                auto oldState = proposal->GetLastState();

                if(oldState == DAOFlags::NIL)
                {
                    proposal->nVotingCycle = nVotingCycles;
                }

                if((pindexNew->nHeight + 1) % GetConsensusParameter(Consensus::CONSENSUS_PARAM_VOTING_CYCLE_LENGTH) == 0)
                {
                    if(proposal->IsExpired(pindexNew->GetBlockTime()))
                    {
                        if (oldState != DAOFlags::EXPIRED)
                        {
                            if (proposal->HasPendingPaymentRequests(view))
                            {
                                proposal->SetState(pindexNew, DAOFlags::PENDING_VOTING_PREQ);
                            }
                            else
                            {
                                if(oldState == DAOFlags::ACCEPTED || oldState == DAOFlags::PENDING_VOTING_PREQ)
                                {
                                    pindexNew->nCFSupply += proposal->GetAvailable(view);
                                    pindexNew->nCFLocked -= proposal->GetAvailable(view);
                                    LogPrint("dao", "%s: Updated nCFSupply %s nCFLocked %s\n", __func__, FormatMoney(pindexNew->nCFSupply), FormatMoney(pindexNew->nCFLocked));
                                }
                                proposal->SetState(pindexNew, DAOFlags::EXPIRED);
                            }
                        }
                    }
                    else if(proposal->IsRejected())
                    {
                        if(oldState != DAOFlags::REJECTED)
                        {
                            proposal->SetState(pindexNew, DAOFlags::REJECTED);
                        }
                    } else if(proposal->IsAccepted())
                    {
                        if((oldState == DAOFlags::NIL || oldState == DAOFlags::PENDING_FUNDS))
                        {
                            if(pindexNew->nCFSupply >= proposal->GetAvailable(view))
                            {
                                pindexNew->nCFSupply -= proposal->GetAvailable(view);
                                pindexNew->nCFLocked += proposal->GetAvailable(view);
                                LogPrint("dao", "%s: Updated nCFSupply %s nCFLocked %s\n", __func__, FormatMoney(pindexNew->nCFSupply), FormatMoney(pindexNew->nCFLocked));
                                proposal->SetState(pindexNew, DAOFlags::ACCEPTED);
                            }
                            else if(oldState != DAOFlags::PENDING_FUNDS)
                            {
                                proposal->SetState(pindexNew, DAOFlags::PENDING_FUNDS);
                            }
                        }
                    }
                }
            }

            if((pindexNew->nHeight) % GetConsensusParameter(Consensus::CONSENSUS_PARAM_VOTING_CYCLE_LENGTH) == 0)
            {
                if (!mapSeen.count(proposal->hash) && proposal->GetLastState() == DAOFlags::NIL)
                {
                    proposal->nVotesYes = 0;
                    proposal->nVotesNo = 0;
                }
            }
        }
    }

    int64_t nTimeEnd5 = GetTimeMicros();

    LogPrint("bench", "   - CFund update proposal status: %.2fms\n", (nTimeEnd5 - nTimeStart5) * 0.001);

    int64_t nTimeStart6 = GetTimeMicros();
    CConsultationAnswerMap mapConsultationAnswers;

    if(view.GetAllConsultationAnswers(mapConsultationAnswers))
    {
        for (CConsultationAnswerMap::iterator it = mapConsultationAnswers.begin(); it != mapConsultationAnswers.end(); it++)
        {
            if (!view.HaveConsultationAnswer(it->first))
                continue;

            bool fParentExpired = false;
            bool fParentPassed = false;

            CConsultationAnswerModifier answer = view.ModifyConsultationAnswer(it->first, pindexNew->nHeight);

            if (answer->txblockhash == uint256())
                continue;

            if (mapBlockIndex.count(answer->txblockhash) == 0)
            {
                LogPrintf("%s: Can't find block %s of answer %s\n",
                          __func__, answer->txblockhash.ToString(), answer->hash.ToString());
                continue;
            }

            if(fUndo)
            {
                answer->ClearState(pindexDelete);
                answer->fDirty = true;
            }
            else if((pindexNew->nHeight + 1) % GetConsensusParameter(Consensus::CONSENSUS_PARAM_VOTING_CYCLE_LENGTH) == 0)
            {

                flags oldState = answer->GetLastState();

                if(answer->IsSupported())
                {
                    if(oldState == DAOFlags::NIL)
                    {
                        answer->SetState(pindexNew, DAOFlags::ACCEPTED);
                        answer->fDirty = true;
                    }
                }

                if (answer->IsConsensusAccepted() && oldState != DAOFlags::PASSED)
                {
                    CConsultation parent;
                    if (view.GetConsultation(answer->parent, parent) && parent.IsAboutConsensusParameter())
                    {
                        fParentExpired = parent.IsExpired(pindexNew);
                        fParentPassed = parent.GetLastState() == DAOFlags::PASSED;
                        pindexNew->mapConsensusParameters[(Consensus::ConsensusParamsPos)parent.nMin] = stoll(answer->sAnswer);
                        answer->SetState(pindexNew, DAOFlags::PASSED);
                        answer->fDirty = true;
                    }
                }
            }

            if((pindexNew->nHeight) % GetConsensusParameter(Consensus::CONSENSUS_PARAM_VOTING_CYCLE_LENGTH) == 0)
            {
                if (answer->GetLastState() == DAOFlags::NIL && !mapSeenSupport.count(answer->hash))
                {
                    answer->nSupport = 0;
                    answer->fDirty = true;
                }
            }
        }
    }

    int64_t nTimeEnd6 = GetTimeMicros();

    LogPrint("bench", "   - CFund update consultation answer status: %.2fms\n", (nTimeEnd6 - nTimeStart6) * 0.001);

    int64_t nTimeStart7 = GetTimeMicros();
    CConsultationMap mapConsultations;

    std::vector<uint256> vClearAnswers;

    if(view.GetAllConsultations(mapConsultations))
    {
        for (CConsultationMap::iterator it = mapConsultations.begin(); it != mapConsultations.end(); it++)
        {
            if (!view.HaveConsultation(it->first))
                continue;

            CConsultationModifier consultation = view.ModifyConsultation(it->first, pindexNew->nHeight);

            if (consultation->txblockhash == uint256())
                continue;

            if (mapBlockIndex.count(consultation->txblockhash) == 0)
            {
                LogPrintf("%s: Can't find block %s of consultation %s\n",
                          __func__, consultation->txblockhash.ToString(), consultation->hash.ToString());
                continue;
            }

            CBlockIndex* pblockindex = mapBlockIndex[consultation->txblockhash];

            uint64_t nCreatedOnCycle = (pblockindex->nHeight / GetConsensusParameter(Consensus::CONSENSUS_PARAM_VOTING_CYCLE_LENGTH));
            uint64_t nCurrentCycle = (pindexNew->nHeight / GetConsensusParameter(Consensus::CONSENSUS_PARAM_VOTING_CYCLE_LENGTH));
            uint64_t nElapsedCycles = std::max(nCurrentCycle - nCreatedOnCycle, (uint64_t)0);
            uint64_t nVotingCycles = std::min(nElapsedCycles, GetConsensusParameter(Consensus::CONSENSUS_PARAM_CONSULTATION_MAX_VOTING_CYCLES) + 1);

            if(fUndo)
            {
                consultation->ClearState(pindexDelete);
                auto newState = consultation->GetLastState();
                if (newState != DAOFlags::EXPIRED && newState != DAOFlags::PASSED)
                    consultation->nVotingCycle = nVotingCycles;
            }
            else
            {
                auto oldState = consultation->GetLastState();

                if (oldState != DAOFlags::EXPIRED && oldState != DAOFlags::PASSED)
                {
                    consultation->nVotingCycle = nVotingCycles;
                }

                if((pindexNew->nHeight + 1) % GetConsensusParameter(Consensus::CONSENSUS_PARAM_VOTING_CYCLE_LENGTH) == 0)
                {
                    if(consultation->IsExpired(pindexNew))
                    {
                        if (oldState != DAOFlags::EXPIRED)
                        {
                            consultation->SetState(pindexNew, DAOFlags::EXPIRED);
                            consultation->fDirty = true;
                        }
                    }
                    else if(consultation->IsSupported(view))
                    {
                        if(oldState == DAOFlags::NIL)
                        {
                            consultation->SetState(pindexNew, DAOFlags::SUPPORTED);
                            consultation->fDirty = true;
                        }
                        else if(oldState == DAOFlags::SUPPORTED && consultation->CanMoveInReflection())
                        {
                            consultation->SetState(pindexNew, DAOFlags::REFLECTION);
                            consultation->fDirty = true;
                        }
                        else if(oldState == DAOFlags::REFLECTION && consultation->IsReflectionOver(pindexNew))
                        {
                            consultation->SetState(pindexNew, DAOFlags::ACCEPTED);
                            consultation->fDirty = true;
                        }
                    }
                    if (consultation->IsAboutConsensusParameter() && pindexNew->mapConsensusParameters.count((Consensus::ConsensusParamsPos)consultation->nMin))
                    {
                        consultation->SetState(pindexNew, DAOFlags::PASSED);
                        consultation->fDirty = true;
                    }
                }

            }

            auto newState = consultation->GetLastState();

            if((pindexNew->nHeight) % GetConsensusParameter(Consensus::CONSENSUS_PARAM_VOTING_CYCLE_LENGTH) == 0 )
            {
                if (newState == DAOFlags::NIL && !mapSeenSupport.count(consultation->hash))
                {
                    consultation->nSupport = 0;
                    consultation->fDirty = true;
                }
                if (newState == DAOFlags::ACCEPTED)
                {
                    if (!mapSeen.count(consultation->hash))
                    {
                        consultation->mapVotes.clear();
                        consultation->fDirty = true;
                    }
                    if (!consultation->IsRange())
                        vClearAnswers.push_back(consultation->hash);
                }
            }
        }
    }

    if(view.GetAllConsultationAnswers(mapConsultationAnswers))
    {
        for (CConsultationAnswerMap::iterator it = mapConsultationAnswers.begin(); it != mapConsultationAnswers.end(); it++)
        {
            if (!view.HaveConsultationAnswer(it->first) || mapSeen.count(it->first))
                continue;

            if (std::find(vClearAnswers.begin(), vClearAnswers.end(), it->second.parent) == vClearAnswers.end())
                continue;

            CConsultationAnswerModifier answer = view.ModifyConsultationAnswer(it->first, pindexNew->nHeight);
            answer->nVotes = 0;
            answer->fDirty = true;
        }
    }

    int64_t nTimeEnd7 = GetTimeMicros();
    LogPrint("bench", "   - CFund update consultation status: %.2fms\n", (nTimeEnd7 - nTimeStart7) * 0.001);

    int64_t nTimeEnd = GetTimeMicros();
    LogPrint("bench", "  - CFund total VoteStep() function: %.2fms\n", (nTimeEnd - nTimeStart) * 0.001);

    return true;
}


bool IsValidDaoTxVote(const CTransaction& tx, CBlockIndex* pindex)
{
    CAmount nAmountContributed = 0;
    bool fHasVote = false;

    for (const CTxOut& out: tx.vout)
    {
        if (out.IsCommunityFundContribution())
            nAmountContributed += out.nValue;
        if (out.IsVote() || out.IsSupportVote() || out.IsConsultationVote())
            fHasVote = true;
    }

    return tx.nVersion == CTransaction::VOTE_VERSION && fHasVote && nAmountContributed >= GetConsensusParameter(Consensus::CONSENSUS_PARAMS_DAO_VOTE_LIGHT_MIN_FEE, pindex);
}

// CONSULTATIONS

bool IsValidConsultationAnswer(CTransaction tx, CStateViewCache& coins, uint64_t nMaskVersion, CBlockIndex* pindex)
{
    if(tx.strDZeel.length() > 255)
        return error("%s: Too long strdzeel for answer %s", __func__, tx.GetHash().ToString());

    UniValue metadata(UniValue::VOBJ);
    try {
        UniValue valRequest;
        if (!valRequest.read(tx.strDZeel))
            return error("%s: Wrong strdzeel for answer %s", __func__, tx.GetHash().ToString());

        if (valRequest.isObject())
            metadata = valRequest.get_obj();
        else
            return error("%s: Wrong strdzeel for answer %s", __func__, tx.GetHash().ToString());

    } catch (const UniValue& objError) {
        return error("%s: Wrong strdzeel for answer %s", __func__, tx.GetHash().ToString());
    } catch (const std::exception& e) {
        return error("%s: Wrong strdzeel for answer %s: %s", __func__, tx.GetHash().ToString(), e.what());
    }

    try
    {
        if(!find_value(metadata, "a").isStr() || !find_value(metadata, "h").isStr())
        {
            return error("%s: Wrong strdzeel for answer %s ()", __func__, tx.GetHash().ToString(), tx.strDZeel);
        }

        std::string sAnswer = find_value(metadata, "a").get_str();

        if (sAnswer == "")
            return error("%s: Empty text for answer %s", __func__, tx.GetHash().ToString());

        std::string Hash = find_value(metadata, "h").get_str();
        int nVersion = find_value(metadata, "v").isNum() ? find_value(metadata, "v").get_int64() : CConsultationAnswer::BASE_VERSION;

        CConsultation consultation;

        if(!coins.GetConsultation(uint256S(Hash), consultation))
            return error("%s: Could not find consultation %s for answer %s", __func__, Hash.c_str(), tx.GetHash().ToString());

        CHashWriter hashAnswer(0,0);

        hashAnswer << uint256S(Hash);
        hashAnswer << sAnswer;

        if(coins.HaveConsultationAnswer(hashAnswer.GetHash()))
            return error("%s: Duplicated answers are forbidden.", __func__);

        if(consultation.nVersion & CConsultation::ANSWER_IS_A_RANGE_VERSION)
            return error("%s: The consultation %s does not admit new answers", __func__, Hash.c_str());

        if(consultation.nVersion & CConsultation::CONSENSUS_PARAMETER_VERSION && !IsValidConsensusParameterProposal((Consensus::ConsensusParamsPos)consultation.nMin, sAnswer, pindex, coins))
            return error("%s: Invalid consultation %s. The proposed parameter %s is not valid", __func__, Hash, sAnswer);

        CAmount nContribution;

        for(unsigned int i=0;i<tx.vout.size();i++)
            if(tx.vout[i].IsCommunityFundContribution())
                nContribution +=tx.vout[i].nValue;

        bool ret = (nContribution >= GetConsensusParameter(Consensus::CONSENSUS_PARAM_CONSULTATION_ANSWER_MIN_FEE));

        if (!ret)
            return error("%s: Not enough fee for answer %s (%d < %d)", __func__, hashAnswer.GetHash().ToString(), nContribution, GetConsensusParameter(Consensus::CONSENSUS_PARAM_CONSULTATION_ANSWER_MIN_FEE));
    }
    catch(...)
    {
        return false;
    }

    return true;

}

bool IsValidConsultation(CTransaction tx, CStateViewCache& coins, uint64_t nMaskVersion, CBlockIndex* pindex)
{
    if(tx.strDZeel.length() > 1024)
        return error("%s: Too long strdzeel for consultation %s", __func__, tx.GetHash().ToString());

    UniValue metadata(UniValue::VOBJ);
    try {
        UniValue valRequest;
        if (!valRequest.read(tx.strDZeel))
            return error("%s: Wrong strdzeel for consultation %s", __func__, tx.GetHash().ToString());

        if (valRequest.isObject())
          metadata = valRequest.get_obj();
        else
            return error("%s: Wrong strdzeel for consultation %s", __func__, tx.GetHash().ToString());

    } catch (const UniValue& objError) {
        return error("%s: Wrong strdzeel for consultation %s", __func__, tx.GetHash().ToString());
    } catch (const std::exception& e) {
        return error("%s: Wrong strdzeel for consultation %s: %s", __func__, tx.GetHash().ToString(), e.what());
    }

    try
    {
        if(!(find_value(metadata, "n").isNum() &&
             find_value(metadata, "q").isStr()))
        {
            return error("%s: Wrong strdzeel for consultation %s (%s)", __func__, tx.GetHash().ToString(), tx.strDZeel);
        }

        CAmount nMin = find_value(metadata, "m").isNum() ? find_value(metadata, "m").get_int64() : 0;
        CAmount nMax = find_value(metadata, "n").get_int64();
        std::string sQuestion = find_value(metadata, "q").get_str();

        CAmount nContribution = 0;
        int nVersion = find_value(metadata, "v").isNum() ? find_value(metadata, "v").get_int64() : CConsultation::BASE_VERSION;

        for(unsigned int i=0;i<tx.vout.size();i++)
            if(tx.vout[i].IsCommunityFundContribution())
                nContribution +=tx.vout[i].nValue;

        bool fRange = nVersion & CConsultation::ANSWER_IS_A_RANGE_VERSION;
        bool fAcceptMoreAnswers = nVersion & CConsultation::MORE_ANSWERS_VERSION;
        bool fIsAboutConsensusParameter = nVersion & CConsultation::CONSENSUS_PARAMETER_VERSION;

        if (fIsAboutConsensusParameter)
        {
            if (fRange || !fAcceptMoreAnswers)
                return error("%s: Invalid consultation %s. A consultation about consensus parameters must allow proposals for answers and can't be a range", __func__, tx.GetHash().ToString());

            if (nMax != 1)
                return error("%s: Invalid consultation %s. n must be 1", __func__, tx.GetHash().ToString());

            if (nMin < Consensus::CONSENSUS_PARAM_VOTING_CYCLE_LENGTH || nMin >= Consensus::MAX_CONSENSUS_PARAMS)
                return error("%s: Invalid consultation %s. Invalid m", __func__, tx.GetHash().ToString());

            CConsultationMap consultationMap;

            if (coins.GetAllConsultations(consultationMap))
            {
                for (auto& it: consultationMap)
                {
                    CConsultation consultation = it.second;

                    if (consultation.txblockhash != uint256()) // only check if not mempool
                    {
                        if (mapBlockIndex.count(consultation.txblockhash) == 0)
                            continue;

                        if (!chainActive.Contains(mapBlockIndex[consultation.txblockhash]))
                            continue;
                    }

                    if (consultation.IsAboutConsensusParameter() && !consultation.IsFinished() && consultation.nMin == nMin)
                        return error("%s: Invalid consultation %s. There already exists an active consultation %s about that consensus parameter.", __func__, tx.GetHash().ToString(), consultation.ToString(pindex));
                }
            }
        }

        UniValue answers(UniValue::VARR);

        if (find_value(metadata, "a").isArray())
            answers = find_value(metadata, "a").get_array();

        std::map<std::string, bool> mapSeen;
        UniValue answersArray = answers.get_array();

        for (unsigned int i = 0; i < answersArray.size(); i++)
        {
            if (!answersArray[i].isStr())
                continue;
            std::string it = answersArray[i].get_str();
            if (mapSeen.count(it) == 0)
                mapSeen[it] = true;
            if(fIsAboutConsensusParameter && !IsValidConsensusParameterProposal((Consensus::ConsensusParamsPos)nMin, it, pindex, coins))
                return error("%s: Invalid consultation %s. The proposed value %s is not valid", __func__, tx.GetHash().ToString(), it);
        }

        CAmount nMinFee = GetConsensusParameter(Consensus::CONSENSUS_PARAM_CONSULTATION_MIN_FEE) + GetConsensusParameter(Consensus::CONSENSUS_PARAM_CONSULTATION_ANSWER_MIN_FEE) * answersArray.size();

        bool ret = (sQuestion != "" && nContribution >= nMinFee &&
                ((fRange && nMin >= 0 && nMax < (uint64_t)VoteFlags::VOTE_ABSTAIN  && nMax > nMin) ||
                 (!fRange && nMax > 0  && nMax < 16)) &&
                ((!fAcceptMoreAnswers && mapSeen.size() > 1) || fAcceptMoreAnswers || fRange) &&
                (nVersion & ~nMaskVersion) == 0);

        if (!ret)
            return error("%s: Wrong strdzeel %s for proposal %s", __func__, tx.strDZeel.c_str(), tx.GetHash().ToString());
    }
    catch(...)
    {
        return false;
    }

    return true;

}

bool IsValidConsensusParameterProposal(Consensus::ConsensusParamsPos pos, std::string proposal, CBlockIndex *pindex, CStateViewCache& coins)
{
    if (proposal.empty() || proposal.find_first_not_of("0123456789") != string::npos)
        return error("%s: Proposed parameter is empty or not integer", __func__);

    uint64_t val = stoll(proposal);

    if (Consensus::vConsensusParamsType[pos] == Consensus::TYPE_PERCENT)
    {
        if (val < 0 || val > 10000)
            return error("%s: Proposed parameter out of range for percentages", __func__);
    }

    if (Consensus::vConsensusParamsType[pos] == Consensus::TYPE_NAV)
    {
        if (val < 0 || val > MAX_MONEY)
            return error("%s: Proposed parameter out of range for coin amounts", __func__);
    }

    if (Consensus::vConsensusParamsType[pos] == Consensus::TYPE_NUMBER)
    {
        if (val <= 0 || val > pow(2,24))
            return error("%s: Proposed parameter out of range for type number", __func__);
    }

    if (pos == Consensus::CONSENSUS_PARAM_CONSULTATION_MIN_CYCLES && val > GetConsensusParameter(Consensus::CONSENSUS_PARAM_CONSULTATION_MAX_SUPPORT_CYCLES, pindex))
        return error("%s: Proposed cycles number out of range", __func__);

    if (pos == Consensus::CONSENSUS_PARAM_CONSULTATION_MAX_SUPPORT_CYCLES && val < GetConsensusParameter(Consensus::CONSENSUS_PARAM_CONSULTATION_MIN_CYCLES, pindex))
        return error("%s: Proposed cycles number out of range", __func__);

    if (pos == Consensus::CONSENSUS_PARAM_CONSULTATION_MIN_CYCLES || pos == Consensus::CONSENSUS_PARAM_CONSULTATION_MAX_SUPPORT_CYCLES)
    {
        auto lookFor = (pos == Consensus::CONSENSUS_PARAM_CONSULTATION_MIN_CYCLES) ? Consensus::CONSENSUS_PARAM_CONSULTATION_MAX_SUPPORT_CYCLES : Consensus::CONSENSUS_PARAM_CONSULTATION_MIN_CYCLES;

        CConsultationMap consultationMap;

        if (coins.GetAllConsultations(consultationMap))
        {
            for (auto& it: consultationMap)
            {
                CConsultation consultation = it.second;

                if (consultation.txblockhash != uint256()) // only check if not mempool
                {
                    if (mapBlockIndex.count(consultation.txblockhash) == 0)
                        continue;

                    if (!chainActive.Contains(mapBlockIndex[consultation.txblockhash]))
                        continue;
                }

                if (consultation.IsAboutConsensusParameter() && !consultation.IsFinished() && consultation.nMin == lookFor)
                    return error("%s: There already exists an active consultation %s about the consensus parameter %d. Both can not happen at the same time", __func__, consultation.ToString(pindex), lookFor);
            }
        }
    }

    if (val == GetConsensusParameter(pos, pindex))
        return error("%s: The proposed value is the current one", __func__);

    if (Consensus::vConsensusParamsType[pos] == Consensus::TYPE_BOOL && val != 0 && val != 1)
        return error("%s: Boolean values can only be 0 or 1", __func__);

    return true;
}

bool CConsultation::IsAboutConsensusParameter() const
{
    return (nVersion & CConsultation::CONSENSUS_PARAMETER_VERSION);
}

bool CConsultation::HaveEnoughAnswers() const {
    AssertLockHeld(cs_main);

    int nSupportedAnswersCount = 0;

    if (nVersion & CConsultation::ANSWER_IS_A_RANGE_VERSION)
    {
        return true;
    }
    else
    {        
        CStateViewCache view(pcoinsTip);
        CConsultationAnswer answer;

        for (auto& it: vAnswers)
        {
            if (!view.GetConsultationAnswer(it, answer))
                continue;

            if (answer.parent != hash)
                continue;

            if (!answer.IsSupported())
                continue;

            nSupportedAnswersCount++;
        }
    }

    return nSupportedAnswersCount >= (IsAboutConsensusParameter() ? 1 : 2);
}

flags CConsultation::GetLastState() const {
    flags ret = NIL;
    int nHeight = 0;
    for (auto& it: mapState)
    {
        if (mapBlockIndex.count(it.first) == 0)
            continue;

        if (mapBlockIndex[it.first]->nHeight > nHeight)
        {
            nHeight = mapBlockIndex[it.first]->nHeight;
            ret = it.second;
        }
    }
    return ret;
}

CBlockIndex* CConsultation::GetLastStateBlockIndex() const {
    CBlockIndex* ret = nullptr;
    int nHeight = 0;
    for (auto& it: mapState)
    {
        if (mapBlockIndex.count(it.first) == 0)
            continue;

        if (!chainActive.Contains(mapBlockIndex[it.first]))
            continue;

        if (mapBlockIndex[it.first]->nHeight > nHeight)
        {
            nHeight = mapBlockIndex[it.first]->nHeight;
            ret = mapBlockIndex[it.first];
        }
    }
    return ret;
}

CBlockIndex* CConsultation::GetLastStateBlockIndexForState(flags state) const {
    CBlockIndex* ret = nullptr;
    int nHeight = 0;
    for (auto& it: mapState)
    {
        if (it.second != state)
            continue;

        if (mapBlockIndex.count(it.first) == 0)
            continue;

        if (!chainActive.Contains(mapBlockIndex[it.first]))
            continue;

        if (mapBlockIndex[it.first]->nHeight > nHeight)
        {
            nHeight = mapBlockIndex[it.first]->nHeight;
            ret = mapBlockIndex[it.first];
        }
    }
    return ret;
}

bool CConsultation::SetState(const CBlockIndex* pindex, flags state) {
    mapState[pindex->GetBlockHash()] = state;
    return true;
}

bool CConsultation::ClearState(const CBlockIndex* pindex) {
    mapState.erase(pindex->GetBlockHash());
    return true;
}

bool CConsultation::IsFinished() const {
    flags fState = GetLastState();
    return fState == DAOFlags::EXPIRED || fState == DAOFlags::PASSED;
}

std::string CConsultation::GetState(const CBlockIndex* pindex) const {
    CStateViewCache view(pcoinsTip);
    std::string sFlags = "waiting for support";
    flags fState = GetLastState();

    if (!HaveEnoughAnswers())
        sFlags += ", waiting for having enough supported answers";

    if(IsSupported(view))
    {
        sFlags = "found support";
        if (!HaveEnoughAnswers())
            sFlags += ", waiting for having enough supported answers";
        if(fState != DAOFlags::SUPPORTED)
            sFlags += ", waiting for end of voting period";
        if(fState == DAOFlags::REFLECTION)
            sFlags = "reflection phase";
        else if(fState == DAOFlags::ACCEPTED)
            sFlags = "voting started";
    }

    if(IsExpired(pindex))
    {
        sFlags = IsSupported(view) ? "finished" : "expired";
        if (fState != DAOFlags::EXPIRED)
            sFlags = IsSupported(view) ? "last cycle, waiting for end of voting period" : "expiring, waiting for end of voting period";
    }

    if (fState == DAOFlags::PASSED)
        sFlags = "passed";

    return sFlags;
}

bool CConsultation::IsSupported(CStateViewCache& view) const
{
    if (IsRange())
    {
        float nMinimumSupport = GetConsensusParameter(Consensus::CONSENSUS_PARAM_CONSULTATION_MIN_SUPPORT) / 10000.0;
        return nSupport > GetConsensusParameter(Consensus::CONSENSUS_PARAM_VOTING_CYCLE_LENGTH) * nMinimumSupport;
    }
    else
        return HaveEnoughAnswers();
}

bool CConsultation::CanMoveInReflection() const
{
    return nVotingCycle >= GetConsensusParameter(Consensus::CONSENSUS_PARAM_CONSULTATION_MIN_CYCLES);
}

std::string CConsultation::ToString(const CBlockIndex* pindex) const {
    AssertLockHeld(cs_main);

    flags fState = GetLastState();
    uint256 blockhash;
    CBlockIndex* pblockindex = GetLastStateBlockIndex();
    if (pblockindex) blockhash = pblockindex->GetBlockHash();

    std::string sRet = strprintf("CConsultation(hash=%s, nVersion=%d, strDZeel=\"%s\", fState=%s, status=%s, answers=[",
                                 hash.ToString(), nVersion, strDZeel, fState, GetState(pindex));

    UniValue answers(UniValue::VARR);
    if (nVersion & CConsultation::ANSWER_IS_A_RANGE_VERSION)
    {
        UniValue a(UniValue::VOBJ);
        for (auto &it: mapVotes)
        {
            sRet += ((it.first == VoteFlags::VOTE_ABSTAIN ? "abstain" : to_string(it.first)) + "=" + to_string(it.second) + ", ");
        }

    }
    else
    {
        CStateViewCache view(pcoinsTip);

        if (mapVotes.count(VoteFlags::VOTE_ABSTAIN) != 0)
        {
             sRet += "abstain=" + to_string(mapVotes.at(VoteFlags::VOTE_ABSTAIN)) + ", ";
        }
        CConsultationAnswerMap mapConsultationAnswers;

        CConsultationAnswer answer;

        for (auto& it: vAnswers)
        {
            if (!view.GetConsultationAnswer(it, answer))
                continue;

            if (answer.parent != hash)
                continue;

            sRet += answer.ToString();
        }
    }

    sRet += strprintf("]");

    if (IsRange())
        sRet += strprintf(", fRange=true, nMin=%u, nMax=%u", nMin, nMax);

    sRet += strprintf(", nVotingCycle=%u, nSupport=%u, blockhash=%s)",
                     nVotingCycle, nSupport, blockhash.ToString().substr(0,10));

    return sRet;
}

void CConsultation::ToJson(UniValue& ret, CStateViewCache& view) const
{
    flags fState = GetLastState();
    uint256 blockhash;
    CBlockIndex* pblockindex = GetLastStateBlockIndex();
    if (pblockindex) blockhash = pblockindex->GetBlockHash();

    ret.pushKV("version",(uint64_t)nVersion);
    ret.pushKV("hash", hash.ToString());
    ret.pushKV("blockhash", txblockhash.ToString());
    ret.pushKV("question", strDZeel);
    ret.pushKV("support", nSupport);
    UniValue answers(UniValue::VARR);
    if (nVersion & CConsultation::ANSWER_IS_A_RANGE_VERSION)
    {
        UniValue a(UniValue::VOBJ);
        for (auto &it: mapVotes)
        {
            a.pushKV(it.first == VoteFlags::VOTE_ABSTAIN ? "abstain" : to_string(it.first), it.second);
        }
        answers.push_back(a);
    }
    else
    {
        if (mapVotes.count(VoteFlags::VOTE_ABSTAIN) != 0)
        {
            UniValue a(UniValue::VOBJ);
            a.pushKV("abstain", mapVotes.at(VoteFlags::VOTE_ABSTAIN));
            answers.push_back(a);
        }
        CConsultationAnswerMap mapConsultationAnswers;
        CConsultationAnswer answer;

        for (auto& it: vAnswers)
        {
            if (!view.GetConsultationAnswer(it, answer))
                continue;

            if (answer.parent != hash)
                continue;

            UniValue a(UniValue::VOBJ);
            answer.ToJson(a);
            answers.push_back(a);
        }
    }
    ret.pushKV("answers", answers);
    ret.pushKV("min", nMin);
    ret.pushKV("max", nMax);
    ret.pushKV("votingCycle", std::min((uint64_t)nVotingCycle, GetConsensusParameter(Consensus::CONSENSUS_PARAM_CONSULTATION_MAX_VOTING_CYCLES)+GetConsensusParameter(Consensus::CONSENSUS_PARAM_CONSULTATION_MAX_SUPPORT_CYCLES)));
    ret.pushKV("status", GetState(chainActive.Tip()));
    ret.pushKV("state", (uint64_t)fState);
    ret.pushKV("stateChangedOnBlock", blockhash.ToString());
}

bool CConsultation::IsExpired(const CBlockIndex* pindex) const
{
    flags fState = GetLastState();

    CBlockIndex* pblockindex = GetLastStateBlockIndexForState(ACCEPTED);

    if (fState == DAOFlags::ACCEPTED && pblockindex) {
        uint64_t nAcceptedOnCycle = ((uint64_t)pblockindex->nHeight / GetConsensusParameter(Consensus::CONSENSUS_PARAM_VOTING_CYCLE_LENGTH));
        uint64_t nCurrentCycle = ((uint64_t)pindex->nHeight / GetConsensusParameter(Consensus::CONSENSUS_PARAM_VOTING_CYCLE_LENGTH));
        uint64_t nElapsedCycles = std::max(nCurrentCycle - nAcceptedOnCycle, (uint64_t)0);
        return (nElapsedCycles >= GetConsensusParameter(Consensus::CONSENSUS_PARAM_CONSULTATION_MAX_VOTING_CYCLES));
    }

    return (fState == DAOFlags::EXPIRED) || (ExceededMaxVotingCycles() && fState == DAOFlags::NIL);
};

bool CConsultation::IsReflectionOver(const CBlockIndex* pindex) const
{
    auto nVotingLength = GetConsensusParameter(Consensus::CONSENSUS_PARAM_VOTING_CYCLE_LENGTH);
    CBlockIndex* pblockindex = GetLastStateBlockIndexForState(REFLECTION);

    if (pblockindex) {
        auto nCreated = (unsigned int)(pblockindex->nHeight / nVotingLength);
        auto nCurrent = (unsigned int)(pindex->nHeight / nVotingLength);
        auto nElapsedCycles = nCurrent - nCreated;
        auto nMaxCycles = GetConsensusParameter(Consensus::CONSENSUS_PARAM_CONSULTATION_REFLECTION_LENGTH);
        return nElapsedCycles >= nMaxCycles;
    }
    return false;
}
bool CConsultation::CanBeSupported() const
{
    flags fState = GetLastState();
    return fState == DAOFlags::NIL;
}

bool CConsultation::CanBeVoted() const
{
    flags fState = GetLastState();
    return fState == DAOFlags::ACCEPTED && IsRange();
}

bool CConsultation::IsRange() const
{
    return (nVersion & CConsultation::ANSWER_IS_A_RANGE_VERSION);
}

bool CConsultation::CanHaveNewAnswers() const
{
    flags fState = GetLastState();
    return fState == DAOFlags::NIL && (nVersion & MORE_ANSWERS_VERSION) && !IsRange();
}

bool CConsultation::CanHaveAnswers() const
{
    flags fState = GetLastState();
    return fState == DAOFlags::NIL && !IsRange();
}

bool CConsultation::IsValidVote(int64_t vote) const
{
    return (vote >= nMin && vote <= nMax);
}

bool CConsultation::ExceededMaxVotingCycles() const
{
    return nVotingCycle >= GetConsensusParameter(Consensus::CONSENSUS_PARAM_CONSULTATION_MAX_SUPPORT_CYCLES);
};

void CConsultationAnswer::Vote() {
    nVotes++;
}

void CConsultationAnswer::DecVote() {
    nVotes = std::max(0,nVotes-1);
}

void CConsultationAnswer::ClearVotes() {
    nVotes=0;
}

int CConsultationAnswer::GetVotes() const {
    return nVotes;
}

flags CConsultationAnswer::GetLastState() const {
    flags ret = NIL;
    int nHeight = 0;
    for (auto& it: mapState)
    {
        if (mapBlockIndex.count(it.first) == 0)
            continue;

        if (mapBlockIndex[it.first]->nHeight > nHeight)
        {
            nHeight = mapBlockIndex[it.first]->nHeight;
            ret = it.second;
        }
    }
    return ret;
}

CBlockIndex* CConsultationAnswer::GetLastStateBlockIndex() const {
    CBlockIndex* ret = nullptr;
    int nHeight = 0;
    for (auto& it: mapState)
    {
        if (mapBlockIndex.count(it.first) == 0)
            continue;

        if (!chainActive.Contains(mapBlockIndex[it.first]))
            continue;

        if (mapBlockIndex[it.first]->nHeight > nHeight)
        {
            nHeight = mapBlockIndex[it.first]->nHeight;
            ret = mapBlockIndex[it.first];
        }
    }
    return ret;
}

CBlockIndex* CConsultationAnswer::GetLastStateBlockIndexForState(flags state) const {
    CBlockIndex* ret = nullptr;
    int nHeight = 0;
    for (auto& it: mapState)
    {
        if (it.second != state)
            continue;

        if (mapBlockIndex.count(it.first) == 0)
            continue;

        if (!chainActive.Contains(mapBlockIndex[it.first]))
            continue;

        if (mapBlockIndex[it.first]->nHeight > nHeight)
        {
            nHeight = mapBlockIndex[it.first]->nHeight;
            ret = mapBlockIndex[it.first];
        }
    }
    return ret;
}

bool CConsultationAnswer::SetState(const CBlockIndex* pindex, flags state) {
    mapState[pindex->GetBlockHash()] = state;
    return true;
}

bool CConsultationAnswer::ClearState(const CBlockIndex* pindex) {
    mapState.erase(pindex->GetBlockHash());
    return true;
}

std::string CConsultationAnswer::GetState() const {
    flags fState = GetLastState();
    std::string sFlags = "waiting for support";
    if(IsSupported()) {
        sFlags = "found support";
        if(fState != DAOFlags::ACCEPTED)
            sFlags += ", waiting for end of voting period";
    }
    if (fState == DAOFlags::PASSED)
        sFlags = "passed";
    return sFlags;
}

std::string CConsultationAnswer::GetText() const {
    return sAnswer;
}

bool CConsultationAnswer::CanBeSupported(CStateViewCache& view) const {
    CConsultation consultation;

    flags fState = GetLastState();

    if (!view.GetConsultation(parent, consultation))
        return error("%s: Can't find parent consultation %s of answer %s", __func__, parent.ToString(), hash.ToString());

    flags fConsultationState = consultation.GetLastState();

    if (fConsultationState != DAOFlags::NIL)
        return false;

    return fState == DAOFlags::NIL;
}


bool CConsultationAnswer::CanBeVoted(CStateViewCache& view) const {
    CConsultation consultation;

    flags fState = GetLastState();

    if (!view.GetConsultation(parent, consultation))
            return false;

    flags fConsultationState = consultation.GetLastState();

    return fState == DAOFlags::ACCEPTED && fConsultationState == DAOFlags::ACCEPTED &&
            !(consultation.nVersion & CConsultation::ANSWER_IS_A_RANGE_VERSION);
}

bool CConsultationAnswer::IsSupported() const
{
    float nMinimumSupport = GetConsensusParameter(Consensus::CONSENSUS_PARAM_CONSULTATION_ANSWER_MIN_SUPPORT) / 10000.0;

    return nSupport >= GetConsensusParameter(Consensus::CONSENSUS_PARAM_VOTING_CYCLE_LENGTH) * nMinimumSupport;
}

bool CConsultationAnswer::IsConsensusAccepted() const
{
    float nMinimumQuorum = Params().GetConsensus().nConsensusChangeMinAccept/10000.0;

    return nVotes >= GetConsensusParameter(Consensus::CONSENSUS_PARAM_VOTING_CYCLE_LENGTH) * nMinimumQuorum;

}

std::string CConsultationAnswer::ToString() const {
    flags fState = GetLastState();
    return strprintf("CConsultationAnswer(hash=%s, fState=%u, sAnswer=\"%s\", nVotes=%u, nSupport=%u)", hash.ToString(), fState, sAnswer, nVotes, nSupport);
}

void CConsultationAnswer::ToJson(UniValue& ret) const {
    flags fState = GetLastState();
    ret.pushKV("version",(uint64_t)nVersion);
    ret.pushKV("string", sAnswer);
    ret.pushKV("support", nSupport);
    ret.pushKV("votes", nVotes);
    ret.pushKV("status", GetState());
    ret.pushKV("state", (uint64_t)fState);
    ret.pushKV("parent", parent.ToString());
    if (hash != uint256())
        ret.pushKV("hash", hash.ToString());
}

// CFUND

bool IsValidPaymentRequest(CTransaction tx, CStateViewCache& coins, uint64_t nMaskVersion)
{
    if(tx.strDZeel.length() > 1024)
        return error("%s: Too long strdzeel for payment request %s", __func__, tx.GetHash().ToString());

    UniValue metadata(UniValue::VOBJ);
    try {
        UniValue valRequest;
        if (!valRequest.read(tx.strDZeel))
            return error("%s: Wrong strdzeel for payment request %s", __func__, tx.GetHash().ToString());

        if (valRequest.isObject())
            metadata = valRequest.get_obj();
        else
            return error("%s: Wrong strdzeel for payment request %s", __func__, tx.GetHash().ToString());

    } catch (const UniValue& objError) {
        return error("%s: Wrong strdzeel for payment request %s", __func__, tx.GetHash().ToString());
    } catch (const std::exception& e) {
        return error("%s: Wrong strdzeel for payment request %s: %s", __func__, tx.GetHash().ToString(), e.what());
    }

    if(!(find_value(metadata, "n").isNum() && find_value(metadata, "s").isStr() && find_value(metadata, "h").isStr() && find_value(metadata, "i").isStr()))
        return error("%s: Wrong strdzeel for payment request %s", __func__, tx.GetHash().ToString());

    CAmount nAmount = find_value(metadata, "n").get_int64();
    std::string Signature = find_value(metadata, "s").get_str();
    std::string Hash = find_value(metadata, "h").get_str();
    std::string strDZeel = find_value(metadata, "i").get_str();
    int nVersion = find_value(metadata, "v").isNum() ? find_value(metadata, "v").get_int64() : 1;

    if (nAmount < 0) {
         return error("%s: Payment Request cannot have amount less than 0: %s", __func__, tx.GetHash().ToString());
    }

    CProposal proposal;

    if(!coins.GetProposal(uint256S(Hash), proposal)  || proposal.GetLastState() != DAOFlags::ACCEPTED)
        return error("%s: Could not find parent proposal %s for payment request %s", __func__, Hash.c_str(),tx.GetHash().ToString());

    std::string sRandom = "";

    if (nVersion >= CPaymentRequest::BASE_VERSION && find_value(metadata, "r").isStr())
        sRandom = find_value(metadata, "r").get_str();

    std::string Secret = sRandom + "I kindly ask to withdraw " +
            std::to_string(nAmount) + "NAV from the proposal " +
            proposal.hash.ToString() + ". Payment request id: " + strDZeel;

    CNavCoinAddress addr(proposal.GetOwnerAddress());
    if (!addr.IsValid())
        return error("%s: Address %s is not valid for payment request %s", __func__, proposal.GetOwnerAddress(), Hash.c_str(), tx.GetHash().ToString());

    CKeyID keyID;
    addr.GetKeyID(keyID);

    bool fInvalid = false;
    std::vector<unsigned char> vchSig = DecodeBase64(Signature.c_str(), &fInvalid);

    if (fInvalid)
        return error("%s: Invalid signature for payment request  %s", __func__, tx.GetHash().ToString());

    CHashWriter ss(SER_GETHASH, 0);
    ss << strMessageMagic;
    ss << Secret;

    CPubKey pubkey;
    if (!pubkey.RecoverCompact(ss.GetHash(), vchSig) || pubkey.GetID() != keyID)
        return error("%s: Invalid signature for payment request %s", __func__, tx.GetHash().ToString());

    if(nAmount > proposal.GetAvailable(coins, true))
        return error("%s: Invalid requested amount for payment request %s (%d vs %d available)",
                     __func__, tx.GetHash().ToString(), nAmount, proposal.GetAvailable(coins, true));

    CAmount nContribution = 0;

    for(unsigned int i=0;i<tx.vout.size();i++)
        if(tx.vout[i].IsCommunityFundContribution())
            nContribution +=tx.vout[i].nValue;

    bool ret = (nContribution >= GetConsensusParameter(Consensus::CONSENSUS_PARAM_PAYMENT_REQUEST_MIN_FEE) &&
                nVersion & ~nMaskVersion) == 0;

    if(!ret)
        return error("%s: Invalid version for payment request %s", __func__, tx.GetHash().ToString());

    return true;

}

flags CPaymentRequest::GetLastState() const {
    flags ret = NIL;
    int nHeight = 0;
    for (auto& it: mapState)
    {
        if (mapBlockIndex.count(it.first) == 0)
            continue;

        if (mapBlockIndex[it.first]->nHeight > nHeight)
        {
            nHeight = mapBlockIndex[it.first]->nHeight;
            ret = it.second;
        }
    }
    return ret;
}

CBlockIndex* CPaymentRequest::GetLastStateBlockIndex() const {
    CBlockIndex* ret = nullptr;
    int nHeight = 0;
    for (auto& it: mapState)
    {
        if (mapBlockIndex.count(it.first) == 0)
            continue;

        if (!chainActive.Contains(mapBlockIndex[it.first]))
            continue;

        if (mapBlockIndex[it.first]->nHeight > nHeight)
        {
            nHeight = mapBlockIndex[it.first]->nHeight;
            ret = mapBlockIndex[it.first];
        }
    }
    return ret;
}

CBlockIndex* CPaymentRequest::GetLastStateBlockIndexForState(flags state) const {
    CBlockIndex* ret = nullptr;
    int nHeight = 0;
    for (auto& it: mapState)
    {
        if (it.second != state)
            continue;

        if (mapBlockIndex.count(it.first) == 0)
            continue;

        if (!chainActive.Contains(mapBlockIndex[it.first]))
            continue;

        if (mapBlockIndex[it.first]->nHeight > nHeight)
        {
            nHeight = mapBlockIndex[it.first]->nHeight;
            ret = mapBlockIndex[it.first];
        }
    }
    return ret;
}

bool CPaymentRequest::SetState(const CBlockIndex* pindex, flags state) {
    mapState[pindex->GetBlockHash()] = state;
    return true;
}

bool CPaymentRequest::ClearState(const CBlockIndex* pindex) {
    mapState.erase(pindex->GetBlockHash());
    return true;
}

bool CPaymentRequest::CanVote(CStateViewCache& coins) const
{
    AssertLockHeld(cs_main);

    CProposal proposal;
    if(!coins.GetProposal(proposalhash, proposal))
        return false;

    flags fLastState = GetLastState();

    return nAmount <= proposal.GetAvailable(coins) && fLastState == NIL && !ExceededMaxVotingCycles();
}

bool CPaymentRequest::IsExpired() const {
    if(nVersion >= BASE_VERSION)
    {
        flags fLastState = GetLastState();
        return (ExceededMaxVotingCycles() && fLastState != ACCEPTED && fLastState != REJECTED && fLastState != PAID);
    }
    return false;
}

bool IsValidProposal(CTransaction tx, uint64_t nMaskVersion)
{
    if(tx.strDZeel.length() > 1024)
        return error("%s: Too long strdzeel for proposal %s", __func__, tx.GetHash().ToString());

    UniValue metadata(UniValue::VOBJ);
    try {
        UniValue valRequest;
        if (!valRequest.read(tx.strDZeel))
            return error("%s: Wrong strdzeel for proposal %s", __func__, tx.GetHash().ToString());

        if (valRequest.isObject())
          metadata = valRequest.get_obj();
        else
            return error("%s: Wrong strdzeel for proposal %s", __func__, tx.GetHash().ToString());

    } catch (const UniValue& objError) {
        return error("%s: Wrong strdzeel for proposal %s", __func__, tx.GetHash().ToString());
    } catch (const std::exception& e) {
        return error("%s: Wrong strdzeel for proposal %s: %s", __func__, tx.GetHash().ToString(), e.what());
    }

    if(!(find_value(metadata, "n").isNum() &&
            find_value(metadata, "a").isStr() &&
            find_value(metadata, "d").isNum() &&
            find_value(metadata, "s").isStr()))
    {

        return error("%s: Wrong strdzeel for proposal %s", __func__, tx.GetHash().ToString());
    }

    CAmount nAmount = find_value(metadata, "n").get_int64();
    std::string ownerAddress = find_value(metadata, "a").get_str();
    std::string paymentAddress = find_value(metadata, "p").isStr() ? find_value(metadata, "p").get_str() : ownerAddress;
    int64_t nDeadline = find_value(metadata, "d").get_int64();
    CAmount nContribution = 0;
    int nVersion = find_value(metadata, "v").isNum() ? find_value(metadata, "v").get_int64() : 1;

    CNavCoinAddress oaddress(ownerAddress);
    if (!oaddress.IsValid())
        return error("%s: Wrong address %s for proposal %s", __func__, ownerAddress.c_str(), tx.GetHash().ToString());

    CNavCoinAddress paddress(paymentAddress);
    if (!paddress.IsValid())
        return error("%s: Wrong address %s for proposal %s", __func__, paymentAddress.c_str(), tx.GetHash().ToString());

    for(unsigned int i=0;i<tx.vout.size();i++)
        if(tx.vout[i].IsCommunityFundContribution())
            nContribution +=tx.vout[i].nValue;

    bool ret = (nContribution >= GetConsensusParameter(Consensus::CONSENSUS_PARAM_PROPOSAL_MIN_FEE) &&
            ownerAddress != "" &&
            paymentAddress != "" &&
            nAmount < MAX_MONEY &&
            nAmount > 0 &&
            nDeadline > 0 &&
            (nVersion & ~nMaskVersion) == 0);

    if (!ret)
        return error("%s: Wrong strdzeel %s for proposal %s", __func__, tx.strDZeel.c_str(), tx.GetHash().ToString());

    return true;

}

bool CPaymentRequest::IsAccepted() const
{
    int nTotalVotes = nVotesYes + nVotesNo;
    float nMinimumQuorum = GetConsensusParameter(Consensus::CONSENSUS_PARAM_PAYMENT_REQUEST_MIN_QUORUM)/10000.0;

    if (nVersion & REDUCED_QUORUM_VERSION)
        nMinimumQuorum = nVotingCycle > GetConsensusParameter(Consensus::CONSENSUS_PARAM_PAYMENT_REQUEST_MAX_VOTING_CYCLES) / 2 ? Params().GetConsensus().nMinimumQuorumSecondHalf : Params().GetConsensus().nMinimumQuorumFirstHalf;

    if (nVersion & ABSTAIN_VOTE_VERSION)
        nTotalVotes += nVotesAbs;

    return nTotalVotes > GetConsensusParameter(Consensus::CONSENSUS_PARAM_VOTING_CYCLE_LENGTH) * nMinimumQuorum
           && ((float)nVotesYes > ((float)(nTotalVotes) * GetConsensusParameter(Consensus::CONSENSUS_PARAM_PAYMENT_REQUEST_MIN_ACCEPT) / 10000.0));
}

bool CPaymentRequest::IsRejected() const {
    int nTotalVotes = nVotesYes + nVotesNo;
    float nMinimumQuorum = GetConsensusParameter(Consensus::CONSENSUS_PARAM_PAYMENT_REQUEST_MIN_QUORUM)/10000.0;

    if (nVersion & REDUCED_QUORUM_VERSION)
        nMinimumQuorum = nVotingCycle > GetConsensusParameter(Consensus::CONSENSUS_PARAM_PAYMENT_REQUEST_MAX_VOTING_CYCLES) / 2 ? Params().GetConsensus().nMinimumQuorumSecondHalf : Params().GetConsensus().nMinimumQuorumFirstHalf;

    if (nVersion & ABSTAIN_VOTE_VERSION)
        nTotalVotes += nVotesAbs;

    return nTotalVotes > GetConsensusParameter(Consensus::CONSENSUS_PARAM_VOTING_CYCLE_LENGTH) * nMinimumQuorum
           && ((float)nVotesNo > ((float)(nTotalVotes) * GetConsensusParameter(Consensus::CONSENSUS_PARAM_PAYMENT_REQUEST_MIN_REJECT) / 10000.0));
}

bool CPaymentRequest::ExceededMaxVotingCycles() const {
    return nVotingCycle > GetConsensusParameter(Consensus::CONSENSUS_PARAM_PAYMENT_REQUEST_MAX_VOTING_CYCLES);
}

bool CProposal::IsAccepted() const
{
    int nTotalVotes = nVotesYes + nVotesNo;
    float nMinimumQuorum = GetConsensusParameter(Consensus::CONSENSUS_PARAM_PROPOSAL_MIN_QUORUM)/10000.0;

    if (nVersion & REDUCED_QUORUM_VERSION)
        nMinimumQuorum = nVotingCycle > GetConsensusParameter(Consensus::CONSENSUS_PARAM_PROPOSAL_MAX_VOTING_CYCLES) / 2 ? Params().GetConsensus().nMinimumQuorumSecondHalf : Params().GetConsensus().nMinimumQuorumFirstHalf;

    if (nVersion & ABSTAIN_VOTE_VERSION)
        nTotalVotes += nVotesAbs;

    return nTotalVotes > GetConsensusParameter(Consensus::CONSENSUS_PARAM_VOTING_CYCLE_LENGTH) * nMinimumQuorum
           && ((float)nVotesYes > ((float)(nTotalVotes) * GetConsensusParameter(Consensus::CONSENSUS_PARAM_PROPOSAL_MIN_ACCEPT) / 10000.0));
}

bool CProposal::IsRejected() const
{
    int nTotalVotes = nVotesYes + nVotesNo;
    float nMinimumQuorum = GetConsensusParameter(Consensus::CONSENSUS_PARAM_PROPOSAL_MIN_QUORUM)/10000.0;

    if (nVersion & REDUCED_QUORUM_VERSION)
        nMinimumQuorum = nVotingCycle > GetConsensusParameter(Consensus::CONSENSUS_PARAM_PROPOSAL_MAX_VOTING_CYCLES) / 2 ? Params().GetConsensus().nMinimumQuorumSecondHalf : Params().GetConsensus().nMinimumQuorumFirstHalf;

    if (nVersion & ABSTAIN_VOTE_VERSION)
        nTotalVotes += nVotesAbs;

    return nTotalVotes > GetConsensusParameter(Consensus::CONSENSUS_PARAM_VOTING_CYCLE_LENGTH) * nMinimumQuorum
           && ((float)nVotesNo > ((float)(nTotalVotes) * GetConsensusParameter(Consensus::CONSENSUS_PARAM_PROPOSAL_MIN_REJECT)/ 10000.0));
}

std::string CProposal::GetOwnerAddress() const {
    return ownerAddress;
}

std::string CProposal::GetPaymentAddress() const {
    return (nVersion & PAYMENT_ADDRESS_VERSION) ? paymentAddress : ownerAddress;
}

bool CProposal::CanVote() const {
    AssertLockHeld(cs_main);

    return (GetLastState() == NIL) && (!ExceededMaxVotingCycles());
}

uint64_t CProposal::getTimeTillExpired(uint32_t currentTime) const
{
    if(nVersion & BASE_VERSION) {
        CBlockIndex* pblockindex = GetLastStateBlockIndexForState(ACCEPTED);
        if (pblockindex) {
            return currentTime - (pblockindex->GetBlockTime() + nDeadline);
        }
    }
    return 0;
}

bool CProposal::IsExpired(uint32_t currentTime) const {
    if(nVersion & BASE_VERSION) {
        CBlockIndex* pblockindex = GetLastStateBlockIndexForState(ACCEPTED);
        flags fLastState = GetLastState();
        if (fLastState == ACCEPTED && pblockindex) {
            return (pblockindex->GetBlockTime() + nDeadline < currentTime);
        }
        return (fLastState == EXPIRED) || (fLastState == PENDING_VOTING_PREQ) || (ExceededMaxVotingCycles() && fLastState == NIL);
    } else {
        return (nDeadline < currentTime);
    }
}

bool CProposal::ExceededMaxVotingCycles() const {
    return nVotingCycle > GetConsensusParameter(Consensus::CONSENSUS_PARAM_PROPOSAL_MAX_VOTING_CYCLES);
}

CAmount CProposal::GetAvailable(CStateViewCache& coins, bool fIncludeRequests) const
{
    AssertLockHeld(cs_main);

    CAmount initial = nAmount;
    CPaymentRequestMap mapPaymentRequests;

    if(coins.GetAllPaymentRequests(mapPaymentRequests))
    {
        for (CPaymentRequestMap::iterator it_ = mapPaymentRequests.begin(); it_ != mapPaymentRequests.end(); it_++)
        {
            CPaymentRequest prequest;

            if (!coins.GetPaymentRequest(it_->first, prequest))
                continue;

            if (prequest.proposalhash != hash)
                continue;

            flags fLastState = prequest.GetLastState();

            if((fIncludeRequests && fLastState != DAOFlags::REJECTED && fLastState != DAOFlags::EXPIRED) || (!fIncludeRequests && (fLastState == DAOFlags::ACCEPTED || fLastState == DAOFlags::PAID)))
                initial -= prequest.nAmount;
        }
    }
    return initial;
}

std::string CProposal::ToString(CStateViewCache& coins, uint32_t currentTime) const {
    AssertLockHeld(cs_main);

    uint256 blockhash;
    CBlockIndex* pblockindex = GetLastStateBlockIndex();
    if (pblockindex) blockhash = pblockindex->GetBlockHash();

    std::string str;
    str += strprintf("CProposal(hash=%s, nVersion=%i, nAmount=%f, available=%f, nFee=%f, ownerAddress=%s, paymentAddress=%s, nDeadline=%u, nVotesYes=%u, "
                     "nVotesAbs=%u, nVotesNo=%u, nVotingCycle=%u, fState=%s, strDZeel=%s, blockhash=%s)",
                     hash.ToString(), nVersion, (float)nAmount/COIN, (float)GetAvailable(coins)/COIN, (float)nFee/COIN, ownerAddress, paymentAddress, nDeadline,
                     nVotesYes, nVotesAbs, nVotesNo, nVotingCycle, GetState(currentTime), strDZeel, blockhash.ToString().substr(0,10));
    CPaymentRequestMap mapPaymentRequests;

    if(coins.GetAllPaymentRequests(mapPaymentRequests))
    {
        for (CPaymentRequestMap::iterator it_ = mapPaymentRequests.begin(); it_ != mapPaymentRequests.end(); it_++)
        {
            CPaymentRequest prequest;

            if (!coins.GetPaymentRequest(it_->first, prequest))
                continue;

            if (prequest.proposalhash != hash)
                continue;

            str += "\n    " + prequest.ToString();
        }
    }
    return str + "\n";
}

bool CProposal::HasPendingPaymentRequests(CStateViewCache& coins) const {
    AssertLockHeld(cs_main);

    CPaymentRequestMap mapPaymentRequests;

    if(coins.GetAllPaymentRequests(mapPaymentRequests))
    {
        for (CPaymentRequestMap::iterator it_ = mapPaymentRequests.begin(); it_ != mapPaymentRequests.end(); it_++)
        {
            CPaymentRequest prequest;

            if (!coins.GetPaymentRequest(it_->first, prequest))
                continue;

            if (prequest.proposalhash != hash)
                continue;

            if(prequest.CanVote(coins))
                return true;
        }
    }
    return false;
}

std::string CProposal::GetState(uint32_t currentTime) const {
    std::string sFlags = "pending";
    flags fState = GetLastState();
    if(IsAccepted()) {
        sFlags = "accepted";
        if(fState == DAOFlags::PENDING_FUNDS)
            sFlags += ", waiting for enough coins in fund";
        else if(fState != DAOFlags::ACCEPTED)
            sFlags += ", waiting for end of voting period";
    }
    if(IsRejected()) {
        sFlags = "rejected";
        if(fState != DAOFlags::REJECTED)
            sFlags += ", waiting for end of voting period";
    }
    if(currentTime > 0 && IsExpired(currentTime)) {
        sFlags = "expired";
        if(fState != DAOFlags::EXPIRED && !ExceededMaxVotingCycles())
            // This branch only occurs when a proposal expires due to exceeding its nDeadline during a voting cycle, not due to exceeding max voting cycles
            sFlags += ", waiting for end of voting period";
    }
    if(fState == DAOFlags::PENDING_VOTING_PREQ) {
        sFlags = "expired, pending voting of payment requests";
    }
    return sFlags;
}

flags CProposal::GetLastState() const {
    flags ret = NIL;
    int nHeight = 0;
    for (auto& it: mapState)
    {
        if (mapBlockIndex.count(it.first) == 0)
            continue;

        if (mapBlockIndex[it.first]->nHeight > nHeight)
        {
            nHeight = mapBlockIndex[it.first]->nHeight;
            ret = it.second;
        }
    }
    return ret;
}

CBlockIndex* CProposal::GetLastStateBlockIndex() const {
    CBlockIndex* ret = nullptr;
    int nHeight = 0;
    for (auto& it: mapState)
    {
        if (mapBlockIndex.count(it.first) == 0)
            continue;

        if (!chainActive.Contains(mapBlockIndex[it.first]))
            continue;

        if (mapBlockIndex[it.first]->nHeight > nHeight)
        {
            nHeight = mapBlockIndex[it.first]->nHeight;
            ret = mapBlockIndex[it.first];
        }
    }
    return ret;
}

CBlockIndex* CProposal::GetLastStateBlockIndexForState(flags state) const {
    CBlockIndex* ret = nullptr;
    int nHeight = 0;
    for (auto& it: mapState)
    {
        if (it.second != state)
            continue;

        if (mapBlockIndex.count(it.first) == 0)
            continue;

        if (!chainActive.Contains(mapBlockIndex[it.first]))
            continue;

        if (mapBlockIndex[it.first]->nHeight > nHeight)
        {
            nHeight = mapBlockIndex[it.first]->nHeight;
            ret = mapBlockIndex[it.first];
        }
    }
    return ret;
}

bool CProposal::SetState(const CBlockIndex* pindex, flags state) {
    mapState[pindex->GetBlockHash()] = state;
    return true;
}

bool CProposal::ClearState(const CBlockIndex* pindex) {
    mapState.erase(pindex->GetBlockHash());
    return true;
}

void CProposal::ToJson(UniValue& ret, CStateViewCache& coins) const {
    AssertLockHeld(cs_main);

    flags fState = GetLastState();
    uint256 blockhash;
    CBlockIndex* pblockindex = GetLastStateBlockIndex();
    if (pblockindex) blockhash = pblockindex->GetBlockHash();

    ret.pushKV("version", (uint64_t)nVersion);
    ret.pushKV("hash", hash.ToString());
    ret.pushKV("blockHash", txblockhash.ToString());
    ret.pushKV("description", strDZeel);
    ret.pushKV("requestedAmount", FormatMoney(nAmount));
    ret.pushKV("notPaidYet", FormatMoney(GetAvailable(coins)));
    ret.pushKV("notRequestedYet", FormatMoney(GetAvailable(coins, true)));
    ret.pushKV("userPaidFee", FormatMoney(nFee));
    ret.pushKV("ownerAddress", ownerAddress);
    ret.pushKV("paymentAddress", paymentAddress);
    if(nVersion & BASE_VERSION) {
        ret.pushKV("proposalDuration", (uint64_t)nDeadline);
        if ((fState == DAOFlags::ACCEPTED || fState == DAOFlags::PAID) && mapBlockIndex.count(blockhash) > 0) {
            ret.pushKV("expiresOn", pblockindex->GetBlockTime() + (uint64_t)nDeadline);
        }
    } else {
        ret.pushKV("expiresOn", (uint64_t)nDeadline);
    }
    ret.pushKV("votesYes", nVotesYes);
    ret.pushKV("votesAbs", nVotesAbs);
    ret.pushKV("votesNo", nVotesNo);
    ret.pushKV("votingCycle", std::min((uint64_t)nVotingCycle, GetConsensusParameter(Consensus::CONSENSUS_PARAM_PROPOSAL_MAX_VOTING_CYCLES)));
    // votingCycle does not return higher than nCyclesProposalVoting to avoid reader confusion, since votes are not counted anyway when votingCycle > nCyclesProposalVoting
    ret.pushKV("status", GetState(chainActive.Tip()->GetBlockTime()));
    ret.pushKV("state", (uint64_t)fState);
    if(blockhash != uint256())
        ret.pushKV("stateChangedOnBlock", blockhash.ToString());
    CPaymentRequestMap mapPaymentRequests;

    if(coins.GetAllPaymentRequests(mapPaymentRequests))
    {
        UniValue preq(UniValue::VOBJ);
        UniValue arr(UniValue::VARR);
        CPaymentRequest prequest;

        for (CPaymentRequestMap::iterator it_ = mapPaymentRequests.begin(); it_ != mapPaymentRequests.end(); it_++)
        {
            CPaymentRequest prequest;

            if (!coins.GetPaymentRequest(it_->first, prequest))
                continue;

            if (prequest.proposalhash != hash)
                continue;

            prequest.ToJson(preq);
            arr.push_back(preq);
        }

        ret.pushKV("paymentRequests", arr);
    }
}

std::string CPaymentRequest::ToString() const {
    uint256 blockhash = uint256();
    CBlockIndex* pblockindex = GetLastStateBlockIndex();

    if (pblockindex)
        blockhash = pblockindex->GetBlockHash();

    return strprintf("CPaymentRequest(hash=%s, nVersion=%d, nAmount=%f, fState=%s, nVotesYes=%u, nVotesNo=%u, nVotesAbs=%u, nVotingCycle=%u, "
                     " proposalhash=%s, blockhash=%s, strDZeel=%s)",
                     hash.ToString(), nVersion, (float)nAmount/COIN, GetState(), nVotesYes, nVotesNo, nVotesAbs,
                     nVotingCycle, proposalhash.ToString(), blockhash.ToString().substr(0,10), strDZeel);
}

void CPaymentRequest::ToJson(UniValue& ret) const {
    ret.pushKV("version",(uint64_t)nVersion);
    ret.pushKV("hash", hash.ToString());
    ret.pushKV("blockHash", txblockhash.ToString());
    ret.pushKV("proposalHash", proposalhash.ToString());
    ret.pushKV("description", strDZeel);
    ret.pushKV("requestedAmount", FormatMoney(nAmount));
    ret.pushKV("votesYes", nVotesYes);
    ret.pushKV("votesAbs", nVotesAbs);
    ret.pushKV("votesNo", nVotesNo);
    ret.pushKV("votingCycle", std::min((uint64_t)nVotingCycle, GetConsensusParameter(Consensus::CONSENSUS_PARAM_PAYMENT_REQUEST_MAX_VOTING_CYCLES)));
    // votingCycle does not return higher than nCyclesPaymentRequestVoting to avoid reader confusion, since votes are not counted anyway when votingCycle > nCyclesPaymentRequestVoting
    ret.pushKV("status", GetState());
    ret.pushKV("state", (uint64_t)GetLastState());
    CBlockIndex* pblockindex = GetLastStateBlockIndex();
    if (pblockindex)
        ret.pushKV("stateChangedOnBlock", pblockindex->GetBlockHash().ToString());
}

uint256 GetCFundDBStateHash(CStateViewCache& view, const CAmount& nCFLocked, const CAmount& nCFSupply)
{
    CPaymentRequestMap mapPaymentRequests;
    CProposalMap mapProposals;
    CConsultationMap mapConsultations;
    CConsultationAnswerMap mapAnswers;
    CVoteMap mapVotes;

    int64_t nTimeStart = GetTimeMicros();

    CHashWriter writer(0,0);

    writer << nCFSupply;
    writer << nCFLocked;

    if (view.GetAllProposals(mapProposals) && view.GetAllPaymentRequests(mapPaymentRequests) && view.GetAllConsultations(mapConsultations) && view.GetAllVotes(mapVotes) && view.GetAllConsultationAnswers(mapAnswers))
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

        for (auto &it: mapConsultations)
        {
            if (!it.second.IsNull())
            {
                writer << it.second;
            }
        }

        for (auto &it: mapVotes)
        {
            if (!it.second.IsNull())
            {
                writer << it.first;
                writer << it.second.GetList();
            }
        }

        for (auto &it: mapAnswers)
        {
            if (!it.second.IsNull())
            {
                writer << it.second;
            }
        }

        for (unsigned int i = 0; i < Consensus::MAX_CONSENSUS_PARAMS; i++)
        {
            Consensus::ConsensusParamsPos id = (Consensus::ConsensusParamsPos)i;
            writer << GetConsensusParameter(id);
        }

    }

    uint256 ret = writer.GetHash();
    int64_t nTimeEnd = GetTimeMicros();
    LogPrint("bench", " Benchmark: Calculate CFundDB state hash: %.2fms\n", (nTimeEnd - nTimeStart) * 0.001);

    return ret;
}
