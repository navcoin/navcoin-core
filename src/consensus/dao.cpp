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
    return pindex->nHeight % params.GetConsensus().nBlocksPerVotingCycle == 0;
}

bool IsEndCycle(const CBlockIndex* pindex, CChainParams params)
{
    return (pindex->nHeight+1) % params.GetConsensus().nBlocksPerVotingCycle == 0;
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
    int nBlocks = (pindexNew->nHeight % Params().GetConsensus().nBlocksPerVotingCycle) + 1;
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

            auto nCreatedOnCycle = (unsigned )(pblockindex->nHeight / Params().GetConsensus().nBlocksPerVotingCycle);
            auto nCurrentCycle = (unsigned )(pindexNew->nHeight / Params().GetConsensus().nBlocksPerVotingCycle);
            auto nElapsedCycles = nCurrentCycle - nCreatedOnCycle;
            auto nVotingCycles = std::min(nElapsedCycles, Params().GetConsensus().nCyclesPaymentRequestVoting + 1);

            auto oldState = prequest->fState;
            auto oldCycle = prequest->nVotingCycle;

            if((prequest->fState == DAOFlags::NIL || fUndo) && nVotingCycles != prequest->nVotingCycle)
            {
                prequest->nVotingCycle = nVotingCycles;
                fUpdate = true;
            }

            if((pindexNew->nHeight + 1) % Params().GetConsensus().nBlocksPerVotingCycle == 0)
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

            if((pindexNew->nHeight) % Params().GetConsensus().nBlocksPerVotingCycle == 0)
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

            auto nCreatedOnCycle = (unsigned int)(pblockindex->nHeight / Params().GetConsensus().nBlocksPerVotingCycle);
            auto nCurrentCycle = (unsigned int)(pindexNew->nHeight / Params().GetConsensus().nBlocksPerVotingCycle);
            auto nElapsedCycles = nCurrentCycle - nCreatedOnCycle;
            auto nVotingCycles = std::min(nElapsedCycles, Params().GetConsensus().nCyclesProposalVoting + 1);

            auto oldState = proposal->fState;
            auto oldCycle = proposal->nVotingCycle;

            if((proposal->fState == DAOFlags::NIL || fUndo) && nVotingCycles != proposal->nVotingCycle)
            {
                proposal->nVotingCycle = nVotingCycles;
                fUpdate = true;
            }

            if((pindexNew->nHeight + 1) % Params().GetConsensus().nBlocksPerVotingCycle == 0)
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

            if((pindexNew->nHeight) % Params().GetConsensus().nBlocksPerVotingCycle == 0)
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

            if((pindexNew->nHeight + 1) % Params().GetConsensus().nBlocksPerVotingCycle == 0)
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
            }

            if((pindexNew->nHeight) % Params().GetConsensus().nBlocksPerVotingCycle == 0)
            {
                if (!vSeen.count(answer->hash) && answer->fState == DAOFlags::NIL)
                    answer->nSupport = 0;

                if (answer->fState != DAOFlags::EXPIRED)
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
                consultation->blockhash = uint256();
                if (consultation->fState == DAOFlags::ACCEPTED)
                    consultation->fState = DAOFlags::REFLECTION;
                else if (consultation->fState == DAOFlags::REFLECTION)
                    consultation->fState = DAOFlags::CONFIRMATION;
                else
                    consultation->fState = DAOFlags::NIL;
                fUpdate = true;
            }

            CBlockIndex* pblockindex = mapBlockIndex[consultation->txblockhash];

            auto nCreatedOnCycle = (unsigned int)(pblockindex->nHeight / Params().GetConsensus().nBlocksPerVotingCycle);
            auto nCurrentCycle = (unsigned int)(pindexNew->nHeight / Params().GetConsensus().nBlocksPerVotingCycle);
            auto nElapsedCycles = nCurrentCycle - nCreatedOnCycle;
            auto nVotingCycles = std::min(nElapsedCycles, Params().GetConsensus().nCyclesConsultationSupport + 1);

            auto oldState = consultation->fState;
            auto oldCycle = consultation->nVotingCycle;

            if((consultation->fState == DAOFlags::NIL || consultation->fState == DAOFlags::REFLECTION || consultation->fState == DAOFlags::CONFIRMATION || fUndo) && nVotingCycles != consultation->nVotingCycle)
            {
                consultation->nVotingCycle = nVotingCycles;
                fUpdate = true;
            }

            if((pindexNew->nHeight + 1) % Params().GetConsensus().nBlocksPerVotingCycle == 0)
            {
                if((!consultation->IsExpired(pindexNew) && consultation->fState == DAOFlags::EXPIRED))
                {
                    consultation->fState = DAOFlags::NIL;
                    consultation->blockhash = uint256();
                    fUpdate = true;
                }

                if(fUndo && !consultation->IsSupported(view) && (consultation->fState == DAOFlags::ACCEPTED ||
                                                    consultation->fState == DAOFlags::CONFIRMATION ||
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
                        consultation->fState = DAOFlags::CONFIRMATION;
                        consultation->blockhash = pindexNew->GetBlockHash();
                        fUpdate = true;
                    }
                    else if(consultation->fState == DAOFlags::CONFIRMATION)
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
            }

            if (fUndo && fUpdate && consultation->fState == oldState
                && consultation->fState != DAOFlags::NIL
                && consultation->fState != DAOFlags::CONFIRMATION
                && consultation->fState != DAOFlags::REFLECTION
                && consultation->nVotingCycle != oldCycle)
                consultation->nVotingCycle = oldCycle;

            if((pindexNew->nHeight) % Params().GetConsensus().nBlocksPerVotingCycle == 0)
            {
                if (!vSeen.count(consultation->hash) && (consultation->fState == DAOFlags::NIL || consultation->fState == DAOFlags::CONFIRMATION))
                    consultation->nSupport = 0;

                if (consultation->fState != DAOFlags::EXPIRED)
                    consultation->mapVotes.clear();
            }
        }
    }

    int64_t nTimeEnd7 = GetTimeMicros();

    LogPrint("bench", "   - CFund update consultation status: %.2fms\n", (nTimeEnd7 - nTimeStart7) * 0.001);

    if((pindexNew->nHeight + 1) % Params().GetConsensus().nBlocksPerVotingCycle == 0)
    {
        CVoteMap mapVoters;
        view.GetAllVotes(mapVoters);

        for (auto& it: mapVoters)
        {
            if(!view.RemoveCachedVoter(it.first))
                error("Could not remove cached votes for %s (%s)!", HexStr(it.first).substr(0,8), it.second.ToString());
        }

    }

    int64_t nTimeEnd = GetTimeMicros();
    LogPrint("bench", "  - CFund total VoteStep() function: %.2fms\n", (nTimeEnd - nTimeStart) * 0.001);

    return true;
}


// CONSULTATIONS

bool IsValidConsultationAnswer(CTransaction tx, CStateViewCache& coins, uint64_t nMaskVersion)
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
    std::string Hash = find_value(metadata, "h").get_str();
    int nVersion = find_value(metadata, "v").isNum() ? find_value(metadata, "v").get_int64() : CConsultationAnswer::BASE_VERSION;

    CConsultation consultation;

    if(!coins.GetConsultation(uint256S(Hash), consultation))
        return error("%s: Could not find consultation %s for answer %s", __func__, Hash.c_str(), tx.GetHash().ToString());

    if(consultation.nVersion & CConsultation::ANSWER_IS_A_RANGE_VERSION)
        return error("%s: The consultation %s does not admit new answers", __func__, Hash.c_str());

    CAmount nContribution;

    for(unsigned int i=0;i<tx.vout.size();i++)
        if(tx.vout[i].IsCommunityFundContribution())
            nContribution +=tx.vout[i].nValue;

    bool ret = (nContribution >= Params().GetConsensus().nConsultationAnswerMinimalFee);

    if (!ret)
        return error("%s: Not enough fee for answer %s", __func__, tx.GetHash().ToString());

    return true;

}

bool IsValidConsultation(CTransaction tx, uint64_t nMaskVersion)
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

    bool ret = (nContribution >= Params().GetConsensus().nConsultationMinimalFee &&
               ((nVersion & CConsultation::ANSWER_IS_A_RANGE_VERSION && nMin > 0 && nMax < (uint64_t)-5) ||
                (!(nVersion & CConsultation::ANSWER_IS_A_RANGE_VERSION) && nMax < 16)) &&
               (nVersion & ~nMaskVersion) == 0);

    if (!ret)
        return error("%s: Wrong strdzeel %s for proposal %s", __func__, tx.strDZeel.c_str(), tx.GetHash().ToString());

    return true;

}

std::string CConsultation::GetState(CBlockIndex* pindex) const {
    CStateViewCache view(pcoinsTip);
    std::string sFlags = "waiting for support";

    if(fState == DAOFlags::CONFIRMATION)
        sFlags += " (confirmation phase)";

    int nSupportedAnswersCount = 0;

    if (nVersion & CConsultation::ANSWER_IS_A_RANGE_VERSION)
    {
        nSupportedAnswersCount = 2;
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

    if (nSupportedAnswersCount < 2)
        sFlags += ", waiting for having enough supported answers";

    if(IsSupported(view))
    {
        sFlags = "found support";
        if (nSupportedAnswersCount < 2)
            sFlags += ", waiting for having enough supported answers";
        if(fState != DAOFlags::CONFIRMATION)
            sFlags += ", waiting for end of voting period";
        else if(fState == DAOFlags::CONFIRMATION)
            sFlags = "found support (confirmation phase)";
        if(fState == DAOFlags::REFLECTION)
            sFlags = "reflection phase";
        else if(fState == DAOFlags::ACCEPTED)
            sFlags = "voting started";
    }

    if(IsExpired(pindex))
        sFlags = IsSupported(view) ? "finished" : "expired";

    return sFlags;
}

bool CConsultation::IsSupported(CStateViewCache& view) const
{
    float nMinimumSupport = Params().GetConsensus().nMinimumConsultationSupport;

    int nSupportedAnswersCount = 0;

    if (nVersion & CConsultation::ANSWER_IS_A_RANGE_VERSION)
    {
        nSupportedAnswersCount = 2;
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

    return nSupportedAnswersCount >= 2 && nSupport > Params().GetConsensus().nBlocksPerVotingCycle * nMinimumSupport;
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
    ret.push_back(Pair("version",(uint64_t)nVersion));
    ret.push_back(Pair("hash", hash.ToString()));
    ret.push_back(Pair("blockhash", txblockhash.ToString()));
    ret.push_back(Pair("question", strDZeel));
    ret.push_back(Pair("support", nSupport));
    UniValue answers(UniValue::VARR);
    if (nVersion & CConsultation::ANSWER_IS_A_RANGE_VERSION)
    {
        UniValue a(UniValue::VOBJ);
        for (auto &it: mapVotes)
        {
            a.push_back(make_pair(it.first == (uint64_t)-5 ? "abstain" : to_string(it.first), it.second));
        }
        answers.push_back(a);
    }
    else
    {
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
    ret.push_back(Pair("answers", answers));
    ret.push_back(Pair("min", nMin));
    ret.push_back(Pair("max", nMax));
    ret.push_back(Pair("votingCycle", (uint64_t)std::min(nVotingCycle, Params().GetConsensus().nCyclesConsultationSupport)));
    ret.push_back(Pair("status", GetState(chainActive.Tip())));
    ret.push_back(Pair("state", (uint64_t)fState));
    ret.push_back(Pair("stateChangedOnBlock", blockhash.ToString()));
}

bool CConsultation::IsExpired(CBlockIndex* pindex) const
{
    if (fState == DAOFlags::ACCEPTED && mapBlockIndex.count(blockhash) > 0) {
        auto nCreatedOnCycle = (unsigned int)(mapBlockIndex[blockhash]->nHeight / Params().GetConsensus().nBlocksPerVotingCycle);
        auto nCurrentCycle = (unsigned int)(pindex->nHeight / Params().GetConsensus().nBlocksPerVotingCycle);
        auto nElapsedCycles = nCurrentCycle - nCreatedOnCycle;
        return (nElapsedCycles > Params().GetConsensus().nCyclesConsultationVoting);
    }
    return (fState == DAOFlags::EXPIRED) || (ExceededMaxVotingCycles() && fState == DAOFlags::NIL);
};

bool CConsultation::CanBeSupported() const
{
    return fState == DAOFlags::NIL || fState == DAOFlags::CONFIRMATION;
}

bool CConsultation::CanBeVoted() const
{
    return fState == DAOFlags::ACCEPTED && (nVersion & CConsultation::ANSWER_IS_A_RANGE_VERSION);
}

bool CConsultation::IsValidVote(int64_t vote) const
{
    return (vote >= nMin && vote <= nMax);
}

bool CConsultation::ExceededMaxVotingCycles() const
{
    return nVotingCycle > Params().GetConsensus().nCyclesConsultationSupport;
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
    float nMinimumSupport = Params().GetConsensus().nMinimumConsultationAnswerSupport;

    return nSupport > Params().GetConsensus().nBlocksPerVotingCycle * nMinimumSupport;
}

std::string CConsultationAnswer::ToString() const {
    return strprintf("CConsultationAnswer(fState=%u, sAnswer=\"%s\", nVotes=%u)", fState, sAnswer, nVotes);
}

void CConsultationAnswer::ToJson(UniValue& ret) const {
    ret.push_back(Pair("version",(uint64_t)nVersion));
    ret.push_back(Pair("string", sAnswer));
    ret.push_back(Pair("support", nSupport));
    ret.push_back(Pair("votes", nVotes));
    ret.push_back(Pair("status", GetState()));
    ret.push_back(Pair("state", fState));
    ret.push_back(Pair("parent", parent.ToString()));
    if (hash != uint256())
        ret.push_back(Pair("hash", hash.ToString()));
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

    return nVersion <= Params().GetConsensus().nPaymentRequestMaxVersion;

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

    bool ret = (nContribution >= Params().GetConsensus().nProposalMinimalFee &&
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
    float nMinimumQuorum = Params().GetConsensus().nMinimumQuorum;

    if (nVersion & REDUCED_QUORUM_VERSION)
        nMinimumQuorum = nVotingCycle > Params().GetConsensus().nCyclesPaymentRequestVoting / 2 ? Params().GetConsensus().nMinimumQuorumSecondHalf : Params().GetConsensus().nMinimumQuorumFirstHalf;

    if (nVersion & ABSTAIN_VOTE_VERSION)
        nTotalVotes += nVotesAbs;

    return nTotalVotes > Params().GetConsensus().nBlocksPerVotingCycle * nMinimumQuorum
           && ((float)nVotesYes > ((float)(nTotalVotes) * Params().GetConsensus().nVotesAcceptPaymentRequest));
}

bool CPaymentRequest::IsRejected() const {
    int nTotalVotes = nVotesYes + nVotesNo;
    float nMinimumQuorum = Params().GetConsensus().nMinimumQuorum;

    if (nVersion & REDUCED_QUORUM_VERSION)
        nMinimumQuorum = nVotingCycle > Params().GetConsensus().nCyclesPaymentRequestVoting / 2 ? Params().GetConsensus().nMinimumQuorumSecondHalf : Params().GetConsensus().nMinimumQuorumFirstHalf;

    if (nVersion & ABSTAIN_VOTE_VERSION)
        nTotalVotes += nVotesAbs;

    return nTotalVotes > Params().GetConsensus().nBlocksPerVotingCycle * nMinimumQuorum
           && ((float)nVotesNo > ((float)(nTotalVotes) * Params().GetConsensus().nVotesRejectPaymentRequest));
}

bool CPaymentRequest::ExceededMaxVotingCycles() const {
    return nVotingCycle > Params().GetConsensus().nCyclesPaymentRequestVoting;
}

bool CProposal::IsAccepted() const
{
    int nTotalVotes = nVotesYes + nVotesNo;
    float nMinimumQuorum = Params().GetConsensus().nMinimumQuorum;

    if (nVersion & REDUCED_QUORUM_VERSION)
        nMinimumQuorum = nVotingCycle > Params().GetConsensus().nCyclesProposalVoting / 2 ? Params().GetConsensus().nMinimumQuorumSecondHalf : Params().GetConsensus().nMinimumQuorumFirstHalf;

    if (nVersion & ABSTAIN_VOTE_VERSION)
        nTotalVotes += nVotesAbs;

    return nTotalVotes > Params().GetConsensus().nBlocksPerVotingCycle * nMinimumQuorum
           && ((float)nVotesYes > ((float)(nTotalVotes) * Params().GetConsensus().nVotesAcceptProposal));
}

bool CProposal::IsRejected() const
{
    int nTotalVotes = nVotesYes + nVotesNo;
    float nMinimumQuorum = Params().GetConsensus().nMinimumQuorum;

    if (nVersion & REDUCED_QUORUM_VERSION)
        nMinimumQuorum = nVotingCycle > Params().GetConsensus().nCyclesProposalVoting / 2 ? Params().GetConsensus().nMinimumQuorumSecondHalf : Params().GetConsensus().nMinimumQuorumFirstHalf;

    if (nVersion & ABSTAIN_VOTE_VERSION)
        nTotalVotes += nVotesAbs;

    return nTotalVotes > Params().GetConsensus().nBlocksPerVotingCycle * nMinimumQuorum
           && ((float)nVotesNo > ((float)(nTotalVotes) * Params().GetConsensus().nVotesRejectProposal));
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
    return nVotingCycle > Params().GetConsensus().nCyclesProposalVoting;
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

    ret.push_back(Pair("version", (uint64_t)nVersion));
    ret.push_back(Pair("hash", hash.ToString()));
    ret.push_back(Pair("blockHash", txblockhash.ToString()));
    ret.push_back(Pair("description", strDZeel));
    ret.push_back(Pair("requestedAmount", FormatMoney(nAmount)));
    ret.push_back(Pair("notPaidYet", FormatMoney(GetAvailable(coins))));
    ret.push_back(Pair("userPaidFee", FormatMoney(nFee)));
    ret.push_back(Pair("paymentAddress", Address));
    if(nVersion & BASE_VERSION) {
        ret.push_back(Pair("proposalDuration", (uint64_t)nDeadline));
        if (fState == DAOFlags::ACCEPTED && mapBlockIndex.count(blockhash) > 0) {
            CBlockIndex* pBlockIndex = mapBlockIndex[blockhash];
            ret.push_back(Pair("expiresOn", pBlockIndex->GetBlockTime() + (uint64_t)nDeadline));
        }
    } else {
        ret.push_back(Pair("expiresOn", (uint64_t)nDeadline));
    }
    ret.push_back(Pair("votesYes", nVotesYes));
    ret.push_back(Pair("votesAbs", nVotesAbs));
    ret.push_back(Pair("votesNo", nVotesNo));
    ret.push_back(Pair("votingCycle", (uint64_t)std::min(nVotingCycle, Params().GetConsensus().nCyclesProposalVoting)));
    // votingCycle does not return higher than nCyclesProposalVoting to avoid reader confusion, since votes are not counted anyway when votingCycle > nCyclesProposalVoting
    ret.push_back(Pair("status", GetState(chainActive.Tip()->GetBlockTime())));
    ret.push_back(Pair("state", (uint64_t)fState));
    if(fState == DAOFlags::ACCEPTED)
        ret.push_back(Pair("stateChangedOnBlock", blockhash.ToString()));
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

        ret.push_back(Pair("paymentRequests", arr));
    }
}

void CPaymentRequest::ToJson(UniValue& ret) const {
    ret.push_back(Pair("version",(uint64_t)nVersion));
    ret.push_back(Pair("hash", hash.ToString()));
    ret.push_back(Pair("blockHash", txblockhash.ToString()));
    ret.push_back(Pair("description", strDZeel));
    ret.push_back(Pair("requestedAmount", FormatMoney(nAmount)));
    ret.push_back(Pair("votesYes", nVotesYes));
    ret.push_back(Pair("votesAbs", nVotesAbs));
    ret.push_back(Pair("votesNo", nVotesNo));
    ret.push_back(Pair("votingCycle", (uint64_t)std::min(nVotingCycle, Params().GetConsensus().nCyclesPaymentRequestVoting)));
    // votingCycle does not return higher than nCyclesPaymentRequestVoting to avoid reader confusion, since votes are not counted anyway when votingCycle > nCyclesPaymentRequestVoting
    ret.push_back(Pair("status", GetState()));
    ret.push_back(Pair("state", (uint64_t)fState));
    ret.push_back(Pair("stateChangedOnBlock", blockhash.ToString()));
    if(fState == DAOFlags::ACCEPTED) {
        ret.push_back(Pair("paidOnBlock", paymenthash.ToString()));
    }
}
