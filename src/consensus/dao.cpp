// Copyright (c) 2018-2019 The NavCoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "consensus/dao.h"
#include "base58.h"
#include "main.h"
#include "rpc/server.h"
#include "utilmoneystr.h"

void GetVersionMask(uint64_t &nProposalMask, uint64_t &nPaymentRequestMask, uint64_t &nConsultationMask, uint64_t &nConsultationAnswerMask)
{
    bool fReducedQuorum = IsReducedCFundQuorumEnabled(chainActive.Tip(), Params().GetConsensus());
    bool fAbstainVote = IsAbstainVoteEnabled(chainActive.Tip(), Params().GetConsensus());

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
    script[3] = vote == -2 ? OP_REMOVE : (vote == -1 ? OP_ABSTAIN : (vote == 1 ? OP_YES : OP_NO));
    script[4] = 0x20;
    memcpy(&script[5], proposalhash.begin(), 32);
}

void SetScriptForPaymentRequestVote(CScript &script, uint256 prequesthash, int64_t vote)
{
    script.resize(37);
    script[0] = OP_RETURN;
    script[1] = OP_CFUND;
    script[2] = OP_PREQ;
    script[3] = vote == -2 ? OP_REMOVE : (vote == -1 ? OP_ABSTAIN : (vote == 1 ? OP_YES : OP_NO));
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
    script[2] = vote == -1 ? OP_ABSTAIN : OP_ANSWER;
    script[3] = 0x20;
    memcpy(&script[4], hash.begin(), 32);
    if (vote > -1)
        script << vote;
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

    if (pcoinsTip->GetConsultationAnswer(hash, answer))
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

    return true;
}

bool RemoveSupport(string str)
{

    RemoveConfigFile("support", str);
    if (mapSupported.count(uint256S(str)) > 0)
        mapSupported.erase(uint256S(str));
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
        mapAddedVotes.erase(uint256S(str));
    else
        return false;
    return true;
}

bool RemoveVoteValue(string str)
{

    RemoveConfigFilePair("vote", str);
    if (mapAddedVotes.count(uint256S(str)) > 0)
        mapAddedVotes.erase(uint256S(str));
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
        ret =  std::to_string(((float)stoll(string) / 100.0)) + "%";

    return ret;
}

std::string RemoveFormatConsensusParameter(Consensus::ConsensusParamsPos pos, std::string string)
{
    string.erase(std::remove_if(string.begin(), string.end(),
    [](const char& c ) -> bool { return !std::isdigit(c) && c != '.' && c != ','; } ), string.end());

    std::string ret = string;

    if (Consensus::vConsensusParamsType[pos] == Consensus::TYPE_NAV)
        ret = std::to_string((uint64_t)(stof(string) * COIN));
    else if (Consensus::vConsensusParamsType[pos] == Consensus::TYPE_PERCENT)
        ret = std::to_string((uint64_t)(stof(string) * 100));

    return ret;
}

std::map<uint256, std::pair<std::pair<int, int>, int>> mapCacheProposalsToUpdate;
std::map<uint256, std::pair<std::pair<int, int>, int>> mapCachePaymentRequestToUpdate;
std::map<uint256, int> mapCacheSupportToUpdate;

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

    std::map<uint256, bool> vSeen;

    if (fUndo || nBlocks == 1 || mapCacheProposalsToUpdate.empty() || mapCachePaymentRequestToUpdate.empty()) {
        mapCacheProposalsToUpdate.clear();
        mapCachePaymentRequestToUpdate.clear();
        mapCacheSupportToUpdate.clear();
    } else {
        nBlocks = 1;
    }

    int64_t nTimeStart2 = GetTimeMicros();

    while(nBlocks > 0 && pindexblock != NULL)
    {
        CProposal proposal;
        CPaymentRequest prequest;
        CConsultation consultation;
        CConsultationAnswer answer;

        vSeen.clear();

        for(unsigned int i = 0; i < pindexblock->vProposalVotes.size(); i++)
        {
            if(!view.GetProposal(pindexblock->vProposalVotes[i].first, proposal))
                continue;

            if(vSeen.count(pindexblock->vProposalVotes[i].first) == 0)
            {
                if(mapCacheProposalsToUpdate.count(pindexblock->vProposalVotes[i].first) == 0)
                    mapCacheProposalsToUpdate[pindexblock->vProposalVotes[i].first] = make_pair(make_pair(0, 0), 0);

                if(pindexblock->vProposalVotes[i].second == 1)
                    mapCacheProposalsToUpdate[pindexblock->vProposalVotes[i].first].first.first += 1;
                else if(pindexblock->vProposalVotes[i].second == -1)
                    mapCacheProposalsToUpdate[pindexblock->vProposalVotes[i].first].second += 1;
                else if(pindexblock->vProposalVotes[i].second == 0)
                    mapCacheProposalsToUpdate[pindexblock->vProposalVotes[i].first].first.second += 1;

                vSeen[pindexblock->vProposalVotes[i].first]=true;
            }
        }

        for(unsigned int i = 0; i < pindexblock->vPaymentRequestVotes.size(); i++)
        {
            if(!view.GetPaymentRequest(pindexblock->vPaymentRequestVotes[i].first, prequest))
                continue;

            if(!view.GetProposal(prequest.proposalhash, proposal))
                continue;

            if (mapBlockIndex.count(proposal.blockhash) == 0)
                continue;

            CBlockIndex* pindexblockparent = mapBlockIndex[proposal.blockhash];

            if(pindexblockparent == NULL)
                continue;

            if(vSeen.count(pindexblock->vPaymentRequestVotes[i].first) == 0)
            {
                if(mapCachePaymentRequestToUpdate.count(pindexblock->vPaymentRequestVotes[i].first) == 0)
                    mapCachePaymentRequestToUpdate[pindexblock->vPaymentRequestVotes[i].first] = make_pair(make_pair(0, 0), 0);

                if(pindexblock->vPaymentRequestVotes[i].second == 1)
                    mapCachePaymentRequestToUpdate[pindexblock->vPaymentRequestVotes[i].first].first.first += 1;
                else if(pindexblock->vPaymentRequestVotes[i].second == -1)
                    mapCachePaymentRequestToUpdate[pindexblock->vPaymentRequestVotes[i].first].second += 1;
                else if(pindexblock->vPaymentRequestVotes[i].second == 0)
                    mapCachePaymentRequestToUpdate[pindexblock->vPaymentRequestVotes[i].first].first.second += 1;

                vSeen[pindexblock->vPaymentRequestVotes[i].first]=true;
            }
        }

        for (auto& it: pindexblock->mapSupport)
        {
            if (!it.second)
                continue;

            if ((view.GetConsultation(it.first, consultation) || view.GetConsultationAnswer(it.first, answer)) && !vSeen.count(it.first))
            {
                if(mapCacheSupportToUpdate.count(it.first) == 0)
                    mapCacheSupportToUpdate[it.first] = 0;

                mapCacheSupportToUpdate[it.first] += 1;
                vSeen[it.first]=true;
            }

        }
        pindexblock = pindexblock->pprev;
        nBlocks--;
    }

    vSeen.clear();

    int64_t nTimeEnd2 = GetTimeMicros();
    LogPrint("bench", "   - CFund count votes from headers: %.2fms\n", (nTimeEnd2 - nTimeStart2) * 0.001);

    int64_t nTimeStart3 = GetTimeMicros();
    std::map<uint256, std::pair<std::pair<int, int>, int>>::iterator it;
    std::vector<std::pair<uint256, CProposal>> vecProposalsToUpdate;
    std::vector<std::pair<uint256, CPaymentRequest>> vecPaymentRequestsToUpdate;

    for(auto& it: mapCacheProposalsToUpdate)
    {
        if (view.HaveProposal(it.first))
        {
            CProposalModifier proposal = view.ModifyProposal(it.first);
            proposal->nVotesYes = it.second.first.first;
            proposal->nVotesAbs = it.second.second;
            proposal->nVotesNo = it.second.first.second;
            vSeen[proposal->hash]=true;
        }
    }

    for(auto& it: mapCachePaymentRequestToUpdate)
    {
        if(view.HavePaymentRequest(it.first))
        {
            CPaymentRequestModifier prequest = view.ModifyPaymentRequest(it.first);
            prequest->nVotesYes = it.second.first.first;
            prequest->nVotesAbs = it.second.second;
            prequest->nVotesNo = it.second.first.second;
            vSeen[prequest->hash]=true;
        }
    }

    for(auto& it: mapCacheSupportToUpdate)
    {
        if (view.HaveConsultation(it.first))
        {
            CConsultationModifier consultation = view.ModifyConsultation(it.first);
            consultation->nSupport = it.second;
            vSeen[consultation->hash]=true;
        }
        else if (view.HaveConsultationAnswer(it.first))
        {
            CConsultationAnswerModifier answer = view.ModifyConsultationAnswer(it.first);
            answer->nSupport = it.second;
            vSeen[answer->hash]=true;
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

            bool fUpdate = false;

            CPaymentRequestModifier prequest = view.ModifyPaymentRequest(it->first);

            if(fUndo && prequest->paymenthash == pindexDelete->GetBlockHash())
            {
                prequest->paymenthash = uint256();
                fUpdate = true;
            }

            if(fUndo && prequest->blockhash == pindexDelete->GetBlockHash())
            {
                prequest->blockhash = uint256();
                prequest->fState = DAOFlags::NIL;
                fUpdate = true;
            }

            if (prequest->txblockhash == uint256())
                continue;

            if (mapBlockIndex.count(prequest->txblockhash) == 0)
            {
                LogPrintf("%s: Can't find block %s of payment request %s\n",
                          __func__, prequest->txblockhash.ToString(), prequest->hash.ToString());
                continue;
            }

            CBlockIndex* pblockindex = mapBlockIndex[prequest->txblockhash];

            CProposal proposal;

            if (!view.GetProposal(prequest->proposalhash, proposal))
                continue;

            auto nCreatedOnCycle = (unsigned )(pblockindex->nHeight / GetConsensusParameter(Consensus::CONSENSUS_PARAM_VOTING_CYCLE_LENGTH));
            auto nCurrentCycle = (unsigned )(pindexNew->nHeight / GetConsensusParameter(Consensus::CONSENSUS_PARAM_VOTING_CYCLE_LENGTH));
            auto nElapsedCycles = nCurrentCycle - nCreatedOnCycle;
            auto nVotingCycles = std::min((uint64_t)nElapsedCycles, GetConsensusParameter(Consensus::CONSENSUS_PARAM_PAYMENT_REQUEST_MAX_VOTING_CYCLES) + 1);

            auto oldState = prequest->fState;
            auto oldCycle = prequest->nVotingCycle;

            if((prequest->fState == DAOFlags::NIL || fUndo) && nVotingCycles != prequest->nVotingCycle)
            {
                prequest->nVotingCycle = nVotingCycles;
                fUpdate = true;
            }

            if((pindexNew->nHeight + 1) % GetConsensusParameter(Consensus::CONSENSUS_PARAM_VOTING_CYCLE_LENGTH) == 0)
            {
                if((!prequest->IsExpired() && prequest->fState == DAOFlags::EXPIRED) ||
                        (!prequest->IsRejected() && prequest->fState == DAOFlags::REJECTED))
                {
                    prequest->fState = DAOFlags::NIL;
                    prequest->blockhash = uint256();
                    fUpdate = true;
                }

                if(!prequest->IsAccepted() && prequest->fState == DAOFlags::ACCEPTED)
                {
                    prequest->fState = DAOFlags::NIL;
                    prequest->blockhash = uint256();
                    fUpdate = true;
                }

                if(prequest->IsExpired())
                {
                    if (prequest->fState != DAOFlags::EXPIRED)
                    {
                        prequest->fState = DAOFlags::EXPIRED;
                        prequest->blockhash = pindexNew->GetBlockHash();
                        fUpdate = true;
                    }
                }
                else if(prequest->IsRejected())
                {
                    if (prequest->fState != DAOFlags::REJECTED)
                    {
                        prequest->fState = DAOFlags::REJECTED;
                        prequest->blockhash = pindexNew->GetBlockHash();
                        fUpdate = true;
                    }
                }
                else if(prequest->fState == DAOFlags::NIL)
                {
                    if((proposal.fState == DAOFlags::ACCEPTED || proposal.fState == DAOFlags::PENDING_VOTING_PREQ) && prequest->IsAccepted())
                    {
                        if(prequest->nAmount <= pindexNew->nCFLocked && prequest->nAmount <= proposal.GetAvailable(view))
                        {
                            pindexNew->nCFLocked -= prequest->nAmount;
                            prequest->fState = DAOFlags::ACCEPTED;
                            prequest->blockhash = pindexNew->GetBlockHash();
                            fUpdate = true;
                        }
                    }
                }
            }

            if (fUndo && fUpdate && prequest->fState == oldState && prequest->fState != DAOFlags::NIL
                    && prequest->nVotingCycle != oldCycle)
                prequest->nVotingCycle = oldCycle;

            if((pindexNew->nHeight) % GetConsensusParameter(Consensus::CONSENSUS_PARAM_VOTING_CYCLE_LENGTH) == 0)
            {
                if (!vSeen.count(prequest->hash) && prequest->fState == DAOFlags::NIL &&
                    !((proposal.fState == DAOFlags::ACCEPTED || proposal.fState == DAOFlags::PENDING_VOTING_PREQ) && prequest->IsAccepted())){
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

            bool fUpdate = false;

            CProposalModifier proposal = view.ModifyProposal(it->first);

            if (proposal->txblockhash == uint256())
                continue;

            if (mapBlockIndex.count(proposal->txblockhash) == 0)
            {
                LogPrintf("%s: Can't find block %s of proposal %s\n",
                          __func__, proposal->txblockhash.ToString(), proposal->hash.ToString());
                continue;
            }

            if(fUndo && proposal->blockhash == pindexDelete->GetBlockHash())
            {
                proposal->blockhash = uint256();
                proposal->fState = DAOFlags::NIL;
                fUpdate = true;
            }

            CBlockIndex* pblockindex = mapBlockIndex[proposal->txblockhash];

            auto nCreatedOnCycle = (unsigned int)(pblockindex->nHeight / GetConsensusParameter(Consensus::CONSENSUS_PARAM_VOTING_CYCLE_LENGTH));
            auto nCurrentCycle = (unsigned int)(pindexNew->nHeight / GetConsensusParameter(Consensus::CONSENSUS_PARAM_VOTING_CYCLE_LENGTH));
            auto nElapsedCycles = nCurrentCycle - nCreatedOnCycle;
            auto nVotingCycles = std::min((uint64_t)nElapsedCycles, GetConsensusParameter(Consensus::CONSENSUS_PARAM_PROPOSAL_MAX_VOTING_CYCLES) + 1);

            auto oldState = proposal->fState;
            auto oldCycle = proposal->nVotingCycle;

            if((proposal->fState == DAOFlags::NIL || fUndo) && nVotingCycles != proposal->nVotingCycle)
            {
                proposal->nVotingCycle = nVotingCycles;
                fUpdate = true;
            }

            if((pindexNew->nHeight + 1) % GetConsensusParameter(Consensus::CONSENSUS_PARAM_VOTING_CYCLE_LENGTH) == 0)
            {
                if((!proposal->IsExpired(pindexNew->GetBlockTime()) && (proposal->fState == DAOFlags::EXPIRED || proposal->fState == DAOFlags::PENDING_VOTING_PREQ)) ||
                   (!proposal->IsRejected() && proposal->fState == DAOFlags::REJECTED))
                {
                    proposal->fState = DAOFlags::NIL;
                    proposal->blockhash = uint256();
                    fUpdate = true;
                }

                if(!proposal->IsAccepted() && (proposal->fState == DAOFlags::ACCEPTED || proposal->fState == DAOFlags::PENDING_FUNDS))
                {
                    proposal->fState = DAOFlags::NIL;
                    proposal->blockhash = uint256();
                    fUpdate = true;
                }

                if(proposal->IsExpired(pindexNew->GetBlockTime()))
                {
                    if (proposal->fState != DAOFlags::EXPIRED)
                    {
                        if (proposal->HasPendingPaymentRequests(view))
                        {
                            proposal->fState = DAOFlags::PENDING_VOTING_PREQ;
                            fUpdate = true;
                        }
                        else
                        {
                            if(proposal->fState == DAOFlags::ACCEPTED || proposal->fState == DAOFlags::PENDING_VOTING_PREQ)
                            {
                                pindexNew->nCFSupply += proposal->GetAvailable(view);
                                pindexNew->nCFLocked -= proposal->GetAvailable(view);
                            }
                            proposal->fState = DAOFlags::EXPIRED;
                            proposal->blockhash = pindexNew->GetBlockHash();
                            fUpdate = true;
                        }
                    }
                }
                else if(proposal->IsRejected())
                {
                    if(proposal->fState != DAOFlags::REJECTED)
                    {
                        proposal->fState = DAOFlags::REJECTED;
                        proposal->blockhash = pindexNew->GetBlockHash();
                        fUpdate = true;
                    }
                } else if(proposal->IsAccepted())
                {
                    if((proposal->fState == DAOFlags::NIL || proposal->fState == DAOFlags::PENDING_FUNDS))
                    {
                        if(pindexNew->nCFSupply >= proposal->GetAvailable(view))
                        {
                            pindexNew->nCFSupply -= proposal->GetAvailable(view);
                            pindexNew->nCFLocked += proposal->GetAvailable(view);
                            proposal->fState = DAOFlags::ACCEPTED;
                            proposal->blockhash = pindexNew->GetBlockHash();
                            fUpdate = true;
                        }
                        else if(proposal->fState != DAOFlags::PENDING_FUNDS)
                        {
                            proposal->fState = DAOFlags::PENDING_FUNDS;
                            proposal->blockhash = uint256();
                            fUpdate = true;
                        }
                    }
                }
            }

            if (fUndo && fUpdate && proposal->fState == oldState && proposal->fState != DAOFlags::NIL && proposal->nVotingCycle != oldCycle)
                proposal->nVotingCycle = oldCycle;

            if((pindexNew->nHeight) % GetConsensusParameter(Consensus::CONSENSUS_PARAM_VOTING_CYCLE_LENGTH) == 0)
            {
                if (!vSeen.count(proposal->hash) && proposal->fState == DAOFlags::NIL)
                {
                    proposal->nVotesYes = 0;
                    proposal->nVotesAbs = 0;
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

            bool fUpdate = false;
            bool fParentExpired = false;

            CConsultationAnswerModifier answer = view.ModifyConsultationAnswer(it->first);

            if (answer->txblockhash == uint256())
                continue;

            if (mapBlockIndex.count(answer->txblockhash) == 0)
            {
                LogPrintf("%s: Can't find block %s of answer %s\n",
                          __func__, answer->txblockhash.ToString(), answer->hash.ToString());
                continue;
            }

            if(fUndo && answer->blockhash == pindexDelete->GetBlockHash())
            {
                answer->blockhash = uint256();
                answer->fState = DAOFlags::NIL;
                fUpdate = true;
            }

            if((pindexNew->nHeight + 1) % GetConsensusParameter(Consensus::CONSENSUS_PARAM_VOTING_CYCLE_LENGTH) == 0)
            {

                if(!answer->IsSupported() && answer->fState == DAOFlags::ACCEPTED)
                {
                    answer->fState = DAOFlags::NIL;
                    answer->blockhash = uint256();
                    fUpdate = true;
                }

                if(answer->IsSupported())
                {
                    if(answer->fState == DAOFlags::NIL)
                    {
                        answer->fState = DAOFlags::ACCEPTED;
                        answer->blockhash = pindexNew->GetBlockHash();
                        fUpdate = true;
                    }
                }

                if (answer->IsConsensusAccepted())
                {
                    CConsultation parent;
                    if (view.GetConsultation(answer->parent, parent))
                    {
                        fParentExpired = parent.IsExpired(pindexNew);
                        pindexNew->mapConsensusParameters[(Consensus::ConsensusParamsPos)parent.nMin] = stoll(answer->sAnswer);
                        answer->fState = DAOFlags::PASSED;
                        answer->blockhash = pindexNew->GetBlockHash();
                        fUpdate = true;
                    }
                }
            }

            if((pindexNew->nHeight) % GetConsensusParameter(Consensus::CONSENSUS_PARAM_VOTING_CYCLE_LENGTH) == 0)
            {
                if (!vSeen.count(answer->hash) && answer->fState == DAOFlags::NIL)
                    answer->nSupport = 0;

                if (answer->fState != DAOFlags::PASSED && !fParentExpired)
                    answer->nVotes = 0;
            }
        }
    }

    int64_t nTimeEnd6 = GetTimeMicros();

    LogPrint("bench", "   - CFund update consultation answer status: %.2fms\n", (nTimeEnd6 - nTimeStart6) * 0.001);

    int64_t nTimeStart7 = GetTimeMicros();
    CConsultationMap mapConsultations;

    if(view.GetAllConsultations(mapConsultations))
    {
        for (CConsultationMap::iterator it = mapConsultations.begin(); it != mapConsultations.end(); it++)
        {
            if (!view.HaveConsultation(it->first))
                continue;

            bool fUpdate = false;

            CConsultationModifier consultation = view.ModifyConsultation(it->first);

            if (consultation->txblockhash == uint256())
                continue;

            if (mapBlockIndex.count(consultation->txblockhash) == 0)
            {
                LogPrintf("%s: Can't find block %s of consultation %s\n",
                          __func__, consultation->txblockhash.ToString(), consultation->hash.ToString());
                continue;
            }

            if(fUndo && consultation->blockhash == pindexDelete->GetBlockHash())
            {
                consultation->blockhash = pindexNew->GetBlockHash();
                if (consultation->fState == DAOFlags::PASSED)
                    consultation->fState = DAOFlags::ACCEPTED;
                else if (consultation->fState == DAOFlags::ACCEPTED)
                    consultation->fState = DAOFlags::REFLECTION;
                else
                    consultation->fState = DAOFlags::NIL;
                fUpdate = true;
            }

            CBlockIndex* pblockindex = mapBlockIndex[consultation->txblockhash];

            auto nCreatedOnCycle = (unsigned int)(pblockindex->nHeight / GetConsensusParameter(Consensus::CONSENSUS_PARAM_VOTING_CYCLE_LENGTH));
            auto nCurrentCycle = (unsigned int)(pindexNew->nHeight / GetConsensusParameter(Consensus::CONSENSUS_PARAM_VOTING_CYCLE_LENGTH));
            auto nElapsedCycles = nCurrentCycle - nCreatedOnCycle;
            auto nVotingCycles = std::min((uint64_t)nElapsedCycles, GetConsensusParameter(Consensus::CONSENSUS_PARAM_CONSULTATION_MAX_SUPPORT_CYCLES) + GetConsensusParameter(Consensus::CONSENSUS_PARAM_CONSULTATION_MAX_VOTING_CYCLES) + 1);

            auto oldState = consultation->fState;
            auto oldCycle = consultation->nVotingCycle;

            if((consultation->fState == DAOFlags::NIL || consultation->fState == DAOFlags::REFLECTION || fUndo) && nVotingCycles != consultation->nVotingCycle)
            {
                consultation->nVotingCycle = nVotingCycles;
                fUpdate = true;
            }

            if((pindexNew->nHeight + 1) % GetConsensusParameter(Consensus::CONSENSUS_PARAM_VOTING_CYCLE_LENGTH) == 0)
            {
                if((!consultation->IsExpired(pindexNew) && consultation->fState == DAOFlags::EXPIRED))
                {
                    consultation->fState = DAOFlags::NIL;
                    consultation->blockhash = uint256();
                    fUpdate = true;
                }

                if(fUndo && !consultation->IsSupported(view) && (consultation->fState == DAOFlags::ACCEPTED ||
                                                    consultation->fState == DAOFlags::REFLECTION))
                {
                    consultation->fState = DAOFlags::NIL;
                    consultation->blockhash = uint256();
                    fUpdate = true;
                }

                if(consultation->IsExpired(pindexNew))
                {
                    if (consultation->fState != DAOFlags::EXPIRED)
                    {
                        consultation->fState = DAOFlags::EXPIRED;
                        fUpdate = true;
                    }
                } else if(consultation->IsSupported(view))
                {
                    if(consultation->fState == DAOFlags::NIL)
                    {
                        consultation->fState = DAOFlags::REFLECTION;
                        consultation->blockhash = pindexNew->GetBlockHash();
                        fUpdate = true;
                    }
                    else if(consultation->fState == DAOFlags::REFLECTION)
                    {
                        consultation->fState = DAOFlags::ACCEPTED;
                        consultation->blockhash = pindexNew->GetBlockHash();
                        fUpdate = true;
                    }
                }
                if (consultation->IsAboutConsensusParameter() && pindexNew->mapConsensusParameters.count((Consensus::ConsensusParamsPos)consultation->nMin))
                {
                    consultation->fState = DAOFlags::PASSED;
                    consultation->blockhash = pindexNew->GetBlockHash();
                    fUpdate = true;
                }
            }

            if (fUndo && fUpdate && consultation->fState == oldState
                && consultation->fState != DAOFlags::NIL
                && consultation->fState != DAOFlags::REFLECTION
                && consultation->nVotingCycle != oldCycle)
                consultation->nVotingCycle = oldCycle;

            if((pindexNew->nHeight) % GetConsensusParameter(Consensus::CONSENSUS_PARAM_VOTING_CYCLE_LENGTH) == 0)
            {
                if (!vSeen.count(consultation->hash) && consultation->fState == DAOFlags::NIL)
                    consultation->nSupport = 0;

                if (consultation->fState != DAOFlags::EXPIRED)
                    consultation->mapVotes.clear();
            }
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

    return tx.nVersion == CTransaction::VOTE_VERSION && fHasVote && nAmountContributed > GetConsensusParameter(Consensus::CONSENSUS_PARAMS_DAO_VOTE_LIGHT_MIN_FEE, pindex);
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

    if(consultation.nVersion & CConsultation::ANSWER_IS_A_RANGE_VERSION)
        return error("%s: The consultation %s does not admit new answers", __func__, Hash.c_str());

    if(consultation.nVersion & CConsultation::CONSENSUS_PARAMETER_VERSION && !IsValidConsensusParameterProposal((Consensus::ConsensusParamsPos)consultation.nMin, sAnswer, pindex))
        return error("%s: Invalid consultation %s. The proposed parameter %s is not valid", __func__, Hash, sAnswer);

    CAmount nContribution;

    for(unsigned int i=0;i<tx.vout.size();i++)
        if(tx.vout[i].IsCommunityFundContribution())
            nContribution +=tx.vout[i].nValue;

    bool ret = (nContribution >= GetConsensusParameter(Consensus::CONSENSUS_PARAM_CONSULTATION_ANSWER_MIN_FEE));

    if (!ret)
        return error("%s: Not enough fee for answer %s", __func__, tx.GetHash().ToString());

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

                if (mapBlockIndex.count(consultation.txblockhash) == 0)
                    continue;

                if (!chainActive.Contains(mapBlockIndex[consultation.txblockhash]))
                    continue;

                if (consultation.IsAboutConsensusParameter() && consultation.fState != DAOFlags::EXPIRED && consultation.nMin == nMin)
                    return error("%s: Invalid consultation %s. There already exists an active consultation about that consensus parameter.", __func__, tx.GetHash().ToString());;
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
        if(fIsAboutConsensusParameter && !IsValidConsensusParameterProposal((Consensus::ConsensusParamsPos)nMin, it, pindex))
            return error("%s: Invalid consultation %s. The proposed value %s is not valid", __func__, tx.GetHash().ToString(), it);
    }

    bool ret = (sQuestion != "" && nContribution >= GetConsensusParameter(Consensus::CONSENSUS_PARAM_CONSULTATION_MIN_FEE) &&
               ((fRange && nMin >= 0 && nMax < (uint64_t)-5  && nMax > nMin) ||
                (!fRange && nMax > 0  && nMax < 16)) &&
               ((!fAcceptMoreAnswers && mapSeen.size() > 1) || fAcceptMoreAnswers || fRange) &&
               (nVersion & ~nMaskVersion) == 0);

    if (!ret)
        return error("%s: Wrong strdzeel %s for proposal %s", __func__, tx.strDZeel.c_str(), tx.GetHash().ToString());

    return true;

}

bool IsValidConsensusParameterProposal(Consensus::ConsensusParamsPos pos, std::string proposal, CBlockIndex *pindex)
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

    if (pos == Consensus::CONSENSUS_PARAM_CONSULTATION_MIN_CYCLES && val > GetConsensusParameter(Consensus::CONSENSUS_PARAM_CONSULTATION_MAX_SUPPORT_CYCLES, pindex))
        return error("%s: Proposed cycles number out of range", __func__);

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
    int nSupportedAnswersCount = 0;

    if (nVersion & CConsultation::ANSWER_IS_A_RANGE_VERSION)
    {
        return true;
    }
    else
    {
        CConsultationAnswerMap mapConsultationAnswers;

        if(pcoinsTip->GetAllConsultationAnswers(mapConsultationAnswers))
        {
            for (CConsultationAnswerMap::iterator it_ = mapConsultationAnswers.begin(); it_ != mapConsultationAnswers.end(); it_++)
            {
                CConsultationAnswer answer;

                if (!pcoinsTip->GetConsultationAnswer(it_->first, answer))
                    continue;

                if (answer.parent != hash)
                    continue;

                if (!answer.IsSupported())
                    continue;

                nSupportedAnswersCount++;

            }
        }
    }

    return nSupportedAnswersCount >= (IsAboutConsensusParameter() ? 1 : 2);
}

std::string CConsultation::GetState(CBlockIndex* pindex) const {
    CStateViewCache view(pcoinsTip);
    std::string sFlags = "waiting for support";

    if (!HaveEnoughAnswers())
        sFlags += ", waiting for having enough supported answers";

    if(IsSupported(view))
    {
        sFlags = "found support";
        if (!HaveEnoughAnswers())
            sFlags += ", waiting for having enough supported answers";
        if(fState != DAOFlags::REFLECTION)
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
            sFlags += ", waiting for end of voting period";
    }

    if (fState == DAOFlags::PASSED)
        sFlags = "passed";

    return sFlags;
}

bool CConsultation::IsSupported(CStateViewCache& view) const
{

    return HaveEnoughAnswers() && nVotingCycle >= GetConsensusParameter(Consensus::CONSENSUS_PARAM_CONSULTATION_MIN_CYCLES);
}

std::string CConsultation::ToString(CBlockIndex* pindex) const {
    std::string sRet = strprintf("CConsultation(hash=%s, nVersion=%d, strDZeel=\"%s\", fState=%s, status=%s, answers=[",
                                 hash.ToString(), nVersion, strDZeel, fState, GetState(pindex));

    CConsultationAnswerMap mapConsultationAnswers;

    if(pcoinsTip->GetAllConsultationAnswers(mapConsultationAnswers))
    {
        for (CConsultationAnswerMap::iterator it_ = mapConsultationAnswers.begin(); it_ != mapConsultationAnswers.end(); it_++)
        {
            CConsultationAnswer answer;

            if (!pcoinsTip->GetConsultationAnswer(it_->first, answer))
                continue;

            if (answer.parent != hash)
                continue;

            sRet += answer.ToString();
        }
    }

    sRet += strprintf("], nVotingCycle=%u, nSupport=%u, blockhash=%s)",
                     nVotingCycle, nSupport, blockhash.ToString().substr(0,10));
    return sRet;
}

void CConsultation::ToJson(UniValue& ret, CStateViewCache& view) const
{
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
            a.pushKV(it.first == (uint64_t)-5 ? "abstain" : to_string(it.first), it.second);
        }
        answers.push_back(a);
    }
    else
    {
        if (mapVotes.count((uint64_t)-5) != 0)
        {
            UniValue a(UniValue::VOBJ);
            a.pushKV("abstain", mapVotes.at((uint64_t)-5));
            answers.push_back(a);
        }
        CConsultationAnswerMap mapConsultationAnswers;
        if(view.GetAllConsultationAnswers(mapConsultationAnswers))
        {
            for (CConsultationAnswerMap::iterator it_ = mapConsultationAnswers.begin(); it_ != mapConsultationAnswers.end(); it_++)
            {
                CConsultationAnswer answer;

                if (!view.GetConsultationAnswer(it_->first, answer))
                    continue;

                if (answer.parent != hash)
                    continue;

                UniValue a(UniValue::VOBJ);
                answer.ToJson(a);
                answers.push_back(a);
            }
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

bool CConsultation::IsExpired(CBlockIndex* pindex) const
{
    if (fState == DAOFlags::ACCEPTED && mapBlockIndex.count(blockhash) > 0) {
        auto nCreatedOnCycle = (unsigned int)(mapBlockIndex[blockhash]->nHeight / GetConsensusParameter(Consensus::CONSENSUS_PARAM_VOTING_CYCLE_LENGTH));
        auto nCurrentCycle = (unsigned int)(pindex->nHeight / GetConsensusParameter(Consensus::CONSENSUS_PARAM_VOTING_CYCLE_LENGTH));
        auto nElapsedCycles = nCurrentCycle - nCreatedOnCycle;
        return (nElapsedCycles > GetConsensusParameter(Consensus::CONSENSUS_PARAM_CONSULTATION_MAX_VOTING_CYCLES));
    }
    return (fState == DAOFlags::EXPIRED) || (ExceededMaxVotingCycles() && fState == DAOFlags::NIL);
};

bool CConsultation::CanBeSupported() const
{
    return fState == DAOFlags::NIL;
}

bool CConsultation::CanBeVoted() const
{
    return fState == DAOFlags::ACCEPTED && IsRange();
}

bool CConsultation::IsRange() const
{
    return (nVersion & CConsultation::ANSWER_IS_A_RANGE_VERSION);
}

bool CConsultation::CanHaveNewAnswers() const
{
    return fState == DAOFlags::NIL && (nVersion & MORE_ANSWERS_VERSION) && !IsRange();
}

bool CConsultation::CanHaveAnswers() const
{
    return fState == DAOFlags::NIL && !IsRange();
}

bool CConsultation::IsValidVote(int64_t vote) const
{
    return (vote >= nMin && vote <= nMax);
}

bool CConsultation::ExceededMaxVotingCycles() const
{
    return nVotingCycle > GetConsensusParameter(Consensus::CONSENSUS_PARAM_CONSULTATION_MAX_SUPPORT_CYCLES);
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

std::string CConsultationAnswer::GetState() const {
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

    if (!view.GetConsultation(parent, consultation))
        return false;

    if (consultation.fState != DAOFlags::NIL)
        return false;

    return fState == DAOFlags::NIL;
}


bool CConsultationAnswer::CanBeVoted(CStateViewCache& view) const {
    CConsultation consultation;

    if (!view.GetConsultation(parent, consultation))
            return false;

    return fState == DAOFlags::ACCEPTED && consultation.fState == DAOFlags::ACCEPTED &&
            !(consultation.nVersion & CConsultation::ANSWER_IS_A_RANGE_VERSION);
}

bool CConsultationAnswer::IsSupported() const
{
    float nMinimumSupport = GetConsensusParameter(Consensus::CONSENSUS_PARAM_CONSULTATION_ANSWER_MIN_SUPPORT) / 10000.0;

    return nSupport > GetConsensusParameter(Consensus::CONSENSUS_PARAM_VOTING_CYCLE_LENGTH) * nMinimumSupport;
}

bool CConsultationAnswer::IsConsensusAccepted() const
{
    float nMinimumQuorum = Params().GetConsensus().nConsensusChangeMinAccept/10000.0;

    return nVotes > GetConsensusParameter(Consensus::CONSENSUS_PARAM_VOTING_CYCLE_LENGTH) * nMinimumQuorum;

}

std::string CConsultationAnswer::ToString() const {
    return strprintf("CConsultationAnswer(fState=%u, sAnswer=\"%s\", nVotes=%u)", fState, sAnswer, nVotes);
}

void CConsultationAnswer::ToJson(UniValue& ret) const {
    ret.pushKV("version",(uint64_t)nVersion);
    ret.pushKV("string", sAnswer);
    ret.pushKV("support", nSupport);
    ret.pushKV("votes", nVotes);
    ret.pushKV("status", GetState());
    ret.pushKV("state", fState);
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

    if(!coins.GetProposal(uint256S(Hash), proposal))
        return error("%s: Could not find parent proposal %s for payment request %s", __func__, Hash.c_str(),tx.GetHash().ToString());

    std::string sRandom = "";

    if (nVersion >= CPaymentRequest::BASE_VERSION && find_value(metadata, "r").isStr())
        sRandom = find_value(metadata, "r").get_str();

    std::string Secret = sRandom + "I kindly ask to withdraw " +
            std::to_string(nAmount) + "NAV from the proposal " +
            proposal.hash.ToString() + ". Payment request id: " + strDZeel;

    CNavCoinAddress addr(proposal.Address);
    if (!addr.IsValid())
        return error("%s: Address %s is not valid for payment request %s", __func__, proposal.Address, Hash.c_str(), tx.GetHash().ToString());

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

    bool ret = (nVersion & ~nMaskVersion) == 0;

    if(!ret)
        return error("%s: Invalid version for payment request %s", __func__, tx.GetHash().ToString());

    return true;

}

bool CPaymentRequest::CanVote(CStateViewCache& coins) const
{
    AssertLockHeld(cs_main);

    CBlockIndex* pindex;
    if(txblockhash == uint256() || !mapBlockIndex.count(txblockhash))
        return false;

    pindex = mapBlockIndex[txblockhash];
    if(!chainActive.Contains(pindex))
        return false;

    CProposal proposal;
    if(!coins.GetProposal(proposalhash, proposal))
        return false;

    return nAmount <= proposal.GetAvailable(coins) && fState != DAOFlags::ACCEPTED && fState != DAOFlags::REJECTED && fState != DAOFlags::EXPIRED && !ExceededMaxVotingCycles();
}

bool CPaymentRequest::IsExpired() const {
    if(nVersion >= BASE_VERSION)
        return (ExceededMaxVotingCycles() && fState != DAOFlags::ACCEPTED && fState != DAOFlags::REJECTED);
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
    std::string Address = find_value(metadata, "a").get_str();
    int64_t nDeadline = find_value(metadata, "d").get_int64();
    CAmount nContribution = 0;
    int nVersion = find_value(metadata, "v").isNum() ? find_value(metadata, "v").get_int64() : 1;

    CNavCoinAddress address(Address);
    if (!address.IsValid())
        return error("%s: Wrong address %s for proposal %s", __func__, Address.c_str(), tx.GetHash().ToString());

    for(unsigned int i=0;i<tx.vout.size();i++)
        if(tx.vout[i].IsCommunityFundContribution())
            nContribution +=tx.vout[i].nValue;

    bool ret = (nContribution >= GetConsensusParameter(Consensus::CONSENSUS_PARAM_PROPOSAL_MIN_FEE) &&
            Address != "" &&
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

bool CProposal::CanVote() const {
    AssertLockHeld(cs_main);

    CBlockIndex* pindex;
    if(txblockhash == uint256() || !mapBlockIndex.count(txblockhash))
        return false;

    pindex = mapBlockIndex[txblockhash];
    if(!chainActive.Contains(pindex))
        return false;

    return (fState == DAOFlags::NIL) && (!ExceededMaxVotingCycles());
}

uint64_t CProposal::getTimeTillExpired(uint32_t currentTime) const
{
    if(nVersion & BASE_VERSION) {
        if (mapBlockIndex.count(blockhash) > 0) {
            CBlockIndex* pblockindex = mapBlockIndex[blockhash];
            return currentTime - (pblockindex->GetBlockTime() + nDeadline);
        }
    }
    return 0;
}

bool CProposal::IsExpired(uint32_t currentTime) const {
    if(nVersion & BASE_VERSION) {
        if (fState == DAOFlags::ACCEPTED && mapBlockIndex.count(blockhash) > 0) {
            CBlockIndex* pBlockIndex = mapBlockIndex[blockhash];
            return (pBlockIndex->GetBlockTime() + nDeadline < currentTime);
        }
        return (fState == DAOFlags::EXPIRED) || (fState == DAOFlags::PENDING_VOTING_PREQ) || (ExceededMaxVotingCycles() && fState == DAOFlags::NIL);
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

            if (!coins.HaveCoins(prequest.hash))
            {
                CBlockIndex* pindex;
                if(prequest.txblockhash == uint256() || !mapBlockIndex.count(prequest.txblockhash))
                    continue;
                pindex = mapBlockIndex[prequest.txblockhash];
                if(!chainActive.Contains(pindex))
                    continue;
            }
            if((fIncludeRequests && prequest.fState != DAOFlags::REJECTED && prequest.fState != DAOFlags::EXPIRED) || (!fIncludeRequests && prequest.fState == DAOFlags::ACCEPTED))
                initial -= prequest.nAmount;
        }
    }
    return initial;
}

std::string CProposal::ToString(CStateViewCache& coins, uint32_t currentTime) const {
    std::string str;
    str += strprintf("CProposal(hash=%s, nVersion=%i, nAmount=%f, available=%f, nFee=%f, address=%s, nDeadline=%u, nVotesYes=%u, "
                     "nVotesAbs=%u, nVotesNo=%u, nVotingCycle=%u, fState=%s, strDZeel=%s, blockhash=%s)",
                     hash.ToString(), nVersion, (float)nAmount/COIN, (float)GetAvailable(coins)/COIN, (float)nFee/COIN, Address, nDeadline,
                     nVotesYes, nVotesAbs, nVotesNo, nVotingCycle, GetState(currentTime), strDZeel, blockhash.ToString().substr(0,10));
    CPaymentRequestMap mapPaymentRequests;

    if(coins.GetAllPaymentRequests(mapPaymentRequests))
    {
        for (CPaymentRequestMap::iterator it_ = mapPaymentRequests.begin(); it_ != mapPaymentRequests.end(); it_++)
        {
            CPaymentRequest prequest;

            if (!pcoinsTip->GetPaymentRequest(it_->first, prequest))
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

    if(pcoinsTip->GetAllPaymentRequests(mapPaymentRequests))
    {
        for (CPaymentRequestMap::iterator it_ = mapPaymentRequests.begin(); it_ != mapPaymentRequests.end(); it_++)
        {
            CPaymentRequest prequest;

            if (!pcoinsTip->GetPaymentRequest(it_->first, prequest))
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
    if(IsAccepted()) {
        sFlags = "accepted";
        if(fState == DAOFlags::PENDING_FUNDS)
            sFlags += " waiting for enough coins in fund";
        else if(fState != DAOFlags::ACCEPTED)
            sFlags += " waiting for end of voting period";
    }
    if(IsRejected()) {
        sFlags = "rejected";
        if(fState != DAOFlags::REJECTED)
            sFlags += " waiting for end of voting period";
    }
    if(currentTime > 0 && IsExpired(currentTime)) {
        sFlags = "expired";
        if(fState != DAOFlags::EXPIRED && !ExceededMaxVotingCycles())
            // This branch only occurs when a proposal expires due to exceeding its nDeadline during a voting cycle, not due to exceeding max voting cycles
            sFlags += " waiting for end of voting period";
    }
    if(fState == DAOFlags::PENDING_VOTING_PREQ) {
        sFlags = "expired pending voting of payment requests";
    }
    return sFlags;
}

void CProposal::ToJson(UniValue& ret, CStateViewCache& coins) const {
    AssertLockHeld(cs_main);

    ret.pushKV("version", (uint64_t)nVersion);
    ret.pushKV("hash", hash.ToString());
    ret.pushKV("blockHash", txblockhash.ToString());
    ret.pushKV("description", strDZeel);
    ret.pushKV("requestedAmount", FormatMoney(nAmount));
    ret.pushKV("notPaidYet", FormatMoney(GetAvailable(coins)));
    ret.pushKV("userPaidFee", FormatMoney(nFee));
    ret.pushKV("paymentAddress", Address);
    if(nVersion & BASE_VERSION) {
        ret.pushKV("proposalDuration", (uint64_t)nDeadline);
        if (fState == DAOFlags::ACCEPTED && mapBlockIndex.count(blockhash) > 0) {
            CBlockIndex* pBlockIndex = mapBlockIndex[blockhash];
            ret.pushKV("expiresOn", pBlockIndex->GetBlockTime() + (uint64_t)nDeadline);
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
    if(fState == DAOFlags::ACCEPTED)
        ret.pushKV("stateChangedOnBlock", blockhash.ToString());
    CPaymentRequestMap mapPaymentRequests;

    if(pcoinsTip->GetAllPaymentRequests(mapPaymentRequests))
    {
        UniValue preq(UniValue::VOBJ);
        UniValue arr(UniValue::VARR);
        CPaymentRequest prequest;

        for (CPaymentRequestMap::iterator it_ = mapPaymentRequests.begin(); it_ != mapPaymentRequests.end(); it_++)
        {
            CPaymentRequest prequest;

            if (!pcoinsTip->GetPaymentRequest(it_->first, prequest))
                continue;

            if (prequest.proposalhash != hash)
                continue;

            prequest.ToJson(preq);
            arr.push_back(preq);
        }

        ret.pushKV("paymentRequests", arr);
    }
}

void CPaymentRequest::ToJson(UniValue& ret) const {
    ret.pushKV("version",(uint64_t)nVersion);
    ret.pushKV("hash", hash.ToString());
    ret.pushKV("blockHash", txblockhash.ToString());
    ret.pushKV("description", strDZeel);
    ret.pushKV("requestedAmount", FormatMoney(nAmount));
    ret.pushKV("votesYes", nVotesYes);
    ret.pushKV("votesAbs", nVotesAbs);
    ret.pushKV("votesNo", nVotesNo);
    ret.pushKV("votingCycle", std::min((uint64_t)nVotingCycle, GetConsensusParameter(Consensus::CONSENSUS_PARAM_PAYMENT_REQUEST_MAX_VOTING_CYCLES)));
    // votingCycle does not return higher than nCyclesPaymentRequestVoting to avoid reader confusion, since votes are not counted anyway when votingCycle > nCyclesPaymentRequestVoting
    ret.pushKV("status", GetState());
    ret.pushKV("state", (uint64_t)fState);
    ret.pushKV("stateChangedOnBlock", blockhash.ToString());
    if(fState == DAOFlags::ACCEPTED) {
        ret.pushKV("paidOnBlock", paymenthash.ToString());
    }
}
