// Copyright (c) 2019 The NavCoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainparams.h>
#include <consensus/dao.h>
#include <coins.h>
#include <random.h>
#include <uint256.h>
#include <test/test_navcoin.h>
#include <main.h>

#include <vector>
#include <map>

#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(cfund_tests, TestingSetup)

BOOST_AUTO_TEST_CASE(cfund_proposals)
{
    LOCK(cs_main);

    CStateViewDB *pcoinsdbview = new CStateViewDB(1<<23, true);
    CStateViewCache *base = new CStateViewCache(pcoinsdbview);
    CStateViewCache view(base);

    CProposal p;


    BOOST_CHECK(p.IsNull());
    p.nVersion = 2;
    p.nDeadline = 10; // each block is 1 second apart, so it will last 10 blocks.

    BOOST_CHECK(p.GetLastState() == DAOFlags::NIL);
    BOOST_CHECK(p.GetLastStateBlockIndex() == nullptr);

    // Create a fake chain with 1000 blocks
    while (chainActive.Tip()->nHeight < 1000) {
        CBlockIndex* prev = chainActive.Tip();
        CBlockIndex* next = new CBlockIndex();
        next->phashBlock = new uint256(GetRandHash());
        pcoinsTip->SetBestBlock(next->GetBlockHash());
        mapBlockIndex.insert(make_pair(*next->phashBlock, next));
        next->pprev = prev;
        next->nHeight = prev->nHeight + 1;
        // every block is 1 second apart
        next->nTime = next->nHeight;
        next->BuildSkip();
        chainActive.SetTip(next);
    }

    BOOST_CHECK(chainActive.Tip()->GetBlockTime() == 1000);

    // We set states and we check that the highest block is always giving the state

    p.SetState(chainActive[810], DAOFlags::ACCEPTED);

    BOOST_CHECK(p.GetLastState() == DAOFlags::ACCEPTED);
    BOOST_CHECK(p.GetLastStateBlockIndexForState(DAOFlags::ACCEPTED) == chainActive[810]);
    BOOST_CHECK(p.GetLastStateBlockIndexForState(DAOFlags::REJECTED) == nullptr);
    BOOST_CHECK(p.GetLastStateBlockIndexForState(DAOFlags::EXPIRED) == nullptr);
    BOOST_CHECK(p.GetLastStateBlockIndex() == chainActive[810]);
    BOOST_CHECK(p.CanRequestPayments());

    p.SetState(chainActive[811], DAOFlags::REJECTED);

    BOOST_CHECK(p.GetLastState() == DAOFlags::REJECTED);
    BOOST_CHECK(p.GetLastStateBlockIndexForState(DAOFlags::ACCEPTED) == chainActive[810]);
    BOOST_CHECK(p.GetLastStateBlockIndexForState(DAOFlags::REJECTED) == chainActive[811]);
    BOOST_CHECK(p.GetLastStateBlockIndexForState(DAOFlags::EXPIRED) == nullptr);
    BOOST_CHECK(p.GetLastStateBlockIndex() == chainActive[811]);

    p.SetState(chainActive[809], DAOFlags::EXPIRED);

    BOOST_CHECK(p.GetLastState() == DAOFlags::REJECTED);
    BOOST_CHECK(p.GetLastStateBlockIndexForState(DAOFlags::ACCEPTED) == chainActive[810]);
    BOOST_CHECK(p.GetLastStateBlockIndexForState(DAOFlags::REJECTED) == chainActive[811]);
    BOOST_CHECK(p.GetLastStateBlockIndexForState(DAOFlags::EXPIRED) == chainActive[809]);
    BOOST_CHECK(p.GetLastStateBlockIndex() == chainActive[811]);
    BOOST_CHECK(!p.CanRequestPayments());

    p.SetState(chainActive[808], DAOFlags::EXPIRED);

    BOOST_CHECK(p.GetLastState() == DAOFlags::REJECTED);
    BOOST_CHECK(p.GetLastStateBlockIndexForState(DAOFlags::ACCEPTED) == chainActive[810]);
    BOOST_CHECK(p.GetLastStateBlockIndexForState(DAOFlags::REJECTED) == chainActive[811]);
    BOOST_CHECK(p.GetLastStateBlockIndexForState(DAOFlags::EXPIRED) == chainActive[809]);
    BOOST_CHECK(p.GetLastStateBlockIndex() == chainActive[811]);
    BOOST_CHECK(!p.CanRequestPayments());

    p.ClearState(chainActive[811]);

    BOOST_CHECK(p.GetLastState() == DAOFlags::ACCEPTED);
    BOOST_CHECK(p.GetLastStateBlockIndexForState(DAOFlags::ACCEPTED) == chainActive[810]);
    BOOST_CHECK(p.GetLastStateBlockIndexForState(DAOFlags::REJECTED) == nullptr);
    BOOST_CHECK(p.GetLastStateBlockIndexForState(DAOFlags::EXPIRED) == chainActive[809]);
    BOOST_CHECK(p.GetLastStateBlockIndex() == chainActive[810]);

    // Proposals in accepted state can request payments

    BOOST_CHECK(p.CanRequestPayments());

    // Proposals should have no payment request yet

    BOOST_CHECK(!p.HasPendingPaymentRequests(view));

    p.nAmount = 1000;

    // Proposals without payment requests should always have the full amount available

    BOOST_CHECK(p.GetAvailable(view, true) == 1000);
    BOOST_CHECK(p.GetAvailable(view, false) == 1000);

    CPaymentRequest pr;
    pr.hash = GetRandHash();
    pr.nAmount = 10;

    view.AddPaymentRequest(pr);

    BOOST_CHECK(p.GetAvailable(view, true) == 990);
    BOOST_CHECK(p.GetAvailable(view, false) == 1000);

    CPaymentRequestModifier mpr = view.ModifyPaymentRequest(pr.hash);

    mpr->SetState(chainActive[810], DAOFlags::REJECTED);

    BOOST_CHECK(p.GetAvailable(view, true) == 1000);
    BOOST_CHECK(p.GetAvailable(view, false) == 1000);

    mpr->SetState(chainActive[811], DAOFlags::NIL);

    BOOST_CHECK(p.GetAvailable(view, true) == 990);
    BOOST_CHECK(p.GetAvailable(view, false) == 1000);

    mpr->SetState(chainActive[812], DAOFlags::EXPIRED);

    BOOST_CHECK(p.GetAvailable(view, true) == 1000);
    BOOST_CHECK(p.GetAvailable(view, false) == 1000);

    mpr->SetState(chainActive[813], DAOFlags::ACCEPTED);

    BOOST_CHECK(p.GetAvailable(view, true) == 990);
    BOOST_CHECK(p.GetAvailable(view, false) == 990);

    mpr->SetState(chainActive[814], DAOFlags::PAID);

    BOOST_CHECK(p.GetAvailable(view, true) == 990);
    BOOST_CHECK(p.GetAvailable(view, false) == 990);

    // We check the max amount of cycles for a proposal

    p.nVotingCycle = 0;

    BOOST_CHECK(!p.ExceededMaxVotingCycles());
    // Status is accepted so can't get votes
    BOOST_CHECK(!p.CanVote());

    p.SetState(chainActive[812], DAOFlags::REJECTED);
    // Status is rejected so can't get votes
    BOOST_CHECK(p.GetLastState() == DAOFlags::REJECTED);
    BOOST_CHECK(!p.CanVote());

    p.SetState(chainActive[813], DAOFlags::EXPIRED);
    // Status is expired so can't get votes
    BOOST_CHECK(p.GetLastState() == DAOFlags::EXPIRED);
    BOOST_CHECK(!p.CanVote());

    p.SetState(chainActive[814], DAOFlags::PAID);
    // Status is paid so can't get votes
    BOOST_CHECK(p.GetLastState() == DAOFlags::PAID);
    BOOST_CHECK(!p.CanVote());

    p.SetState(chainActive[815], DAOFlags::NIL);
    // Status is nil so can get votes
    BOOST_CHECK(p.GetLastState() == DAOFlags::NIL);
    BOOST_CHECK(p.CanVote());

    auto nMaxCycles = GetConsensusParameter(Consensus::CONSENSUS_PARAM_PROPOSAL_MAX_VOTING_CYCLES);

    p.nVotingCycle = nMaxCycles;

    BOOST_CHECK(!p.ExceededMaxVotingCycles());
    BOOST_CHECK(!p.IsExpired(chainActive.Tip()->GetBlockTime()));
    // Status is nil so can't get votes
    BOOST_CHECK(p.CanVote());

    p.SetState(chainActive[816], DAOFlags::REJECTED);
    // Status is rejected so can't get votes
    BOOST_CHECK(p.GetLastState() == DAOFlags::REJECTED);
    BOOST_CHECK(!p.CanVote());

    p.SetState(chainActive[817], DAOFlags::EXPIRED);
    BOOST_CHECK(p.IsExpired(chainActive.Tip()->GetBlockTime()));
    // Status is expired so can't get votes
    BOOST_CHECK(p.GetLastState() == DAOFlags::EXPIRED);
    BOOST_CHECK(!p.CanVote());

    p.nVotingCycle = nMaxCycles+1;

    // Should be expired
    BOOST_CHECK(p.ExceededMaxVotingCycles());
    BOOST_CHECK(p.IsExpired(chainActive.Tip()->GetBlockTime()));
    // Status is expired so can't get votes
    BOOST_CHECK(!p.CanVote());

    p.SetState(chainActive[817], DAOFlags::ACCEPTED);
    BOOST_CHECK(p.ExceededMaxVotingCycles());
    BOOST_CHECK(p.IsExpired(chainActive.Tip()->GetBlockTime()));

    // Proposal accepted 10 blocks (10 seconds) before the tip, should not be expired yet (deadline was 10)
    p.SetState(chainActive[990], DAOFlags::ACCEPTED);
    BOOST_CHECK(p.ExceededMaxVotingCycles());
    BOOST_CHECK(!p.IsExpired(chainActive.Tip()->GetBlockTime()));

    // Proposal accepted 11 blocks (11 seconds) before the tip, should be expired
    p.ClearState(chainActive[990]);
    p.SetState(chainActive[989], DAOFlags::ACCEPTED);
    BOOST_CHECK(p.ExceededMaxVotingCycles());
    BOOST_CHECK(p.IsExpired(chainActive.Tip()->GetBlockTime()));

    // Even being in the range of allowed cycles, it is still expired
    p.nVotingCycle = nMaxCycles;
    BOOST_CHECK(p.IsExpired(chainActive.Tip()->GetBlockTime()));
    p.nVotingCycle = nMaxCycles+1;

    p.ClearState(chainActive[991]);
    p.ClearState(chainActive[989]);

    // Status is accepted so can't get votes
    BOOST_CHECK(p.GetLastState() == DAOFlags::ACCEPTED);
    BOOST_CHECK(!p.CanVote());

    p.SetState(chainActive[819], DAOFlags::EXPIRED);
    // Status is expired so can't get votes
    BOOST_CHECK(p.GetLastState() == DAOFlags::EXPIRED);
    BOOST_CHECK(!p.CanVote());

    p.SetState(chainActive[820], DAOFlags::NIL);
    // Status is nil but can't get votes because it ran out of cycles
    BOOST_CHECK(p.GetLastState() == DAOFlags::NIL);
    BOOST_CHECK(!p.CanVote());

    // Can we know when a proposal is accepted or rejected

    float nMinimumQuorum = GetConsensusParameter(Consensus::CONSENSUS_PARAM_PROPOSAL_MIN_QUORUM)/10000.0;
    auto nMinSumVotes = GetConsensusParameter(Consensus::CONSENSUS_PARAM_VOTING_CYCLE_LENGTH) * nMinimumQuorum;
    auto nMinYesVotes = nMinSumVotes * GetConsensusParameter(Consensus::CONSENSUS_PARAM_PROPOSAL_MIN_ACCEPT)/10000.0;
    auto nMinNoVotes = nMinSumVotes * GetConsensusParameter(Consensus::CONSENSUS_PARAM_PROPOSAL_MIN_REJECT)/10000.0;

    BOOST_CHECK(p.nVotesYes == 0);
    BOOST_CHECK(p.nVotesNo == 0);
    BOOST_CHECK(!p.IsAccepted());
    BOOST_CHECK(!p.IsRejected());

    p.nVotesYes = nMinSumVotes - nMinNoVotes;
    p.nVotesNo = nMinNoVotes - 1;

    BOOST_CHECK(!p.IsAccepted());
    BOOST_CHECK(!p.IsRejected());

    p.nVotesYes = nMinYesVotes - 1;
    p.nVotesNo = nMinSumVotes - nMinYesVotes;

    BOOST_CHECK(!p.IsAccepted());
    BOOST_CHECK(!p.IsRejected());

    p.nVotesYes = nMinSumVotes - nMinNoVotes - 1;
    p.nVotesNo = nMinNoVotes + 1;

    BOOST_CHECK(!p.IsAccepted());
    BOOST_CHECK(!p.IsRejected());

    p.nVotesYes = nMinYesVotes + 1;
    p.nVotesNo = nMinSumVotes - nMinYesVotes - 1;

    BOOST_CHECK(!p.IsAccepted());
    BOOST_CHECK(!p.IsRejected());

    p.nVotesYes = nMinSumVotes - nMinNoVotes + 1;
    p.nVotesNo = nMinNoVotes - 1;

    BOOST_CHECK(!p.IsAccepted());
    BOOST_CHECK(!p.IsRejected());

    p.nVotesYes = nMinYesVotes - 1;
    p.nVotesNo = nMinSumVotes - nMinYesVotes + 1;

    BOOST_CHECK(!p.IsAccepted());
    BOOST_CHECK(!p.IsRejected());

    p.nVotesYes = nMinSumVotes - nMinNoVotes;
    p.nVotesNo = nMinNoVotes;

    BOOST_CHECK(!p.IsAccepted());
    BOOST_CHECK(!p.IsRejected());

    p.nVotesYes = nMinYesVotes;
    p.nVotesNo = nMinSumVotes - nMinYesVotes;

    BOOST_CHECK(!p.IsAccepted());
    BOOST_CHECK(!p.IsRejected());

    p.nVotesYes = nMinSumVotes - nMinNoVotes;
    p.nVotesNo = nMinNoVotes + 1;

    BOOST_CHECK(!p.IsAccepted());
    BOOST_CHECK(p.IsRejected());

    p.nVotesYes = nMinYesVotes + 1;
    p.nVotesNo = nMinSumVotes - nMinYesVotes;

    BOOST_CHECK(p.IsAccepted());
    BOOST_CHECK(!p.IsRejected());
}

BOOST_AUTO_TEST_CASE(cfund_prequests)
{
    LOCK(cs_main);

    CStateViewDB *pcoinsdbview = new CStateViewDB(1<<23, true);
    CStateViewCache *base = new CStateViewCache(pcoinsdbview);
    CStateViewCache view(base);

    CPaymentRequest pr;

    BOOST_CHECK(pr.IsNull());
    pr.nVersion = 2;

    CProposal p;
    p.hash = GetRandHash();
    p.nAmount = 10;

    pr.proposalhash = p.hash;

    view.AddProposal(p);

    BOOST_CHECK(pr.GetLastState() == DAOFlags::NIL);
    BOOST_CHECK(pr.GetLastStateBlockIndex() == nullptr);

    // Create a fake chain with 1000 blocks
    while (chainActive.Tip()->nHeight < 1000) {
        CBlockIndex* prev = chainActive.Tip();
        CBlockIndex* next = new CBlockIndex();
        next->phashBlock = new uint256(GetRandHash());
        pcoinsTip->SetBestBlock(next->GetBlockHash());
        mapBlockIndex.insert(make_pair(*next->phashBlock, next));
        next->pprev = prev;
        next->nHeight = prev->nHeight + 1;
        // every block is 1 second apart
        next->nTime = next->nHeight;
        next->BuildSkip();
        chainActive.SetTip(next);
    }

    BOOST_CHECK(chainActive.Tip()->GetBlockTime() == 1000);

    CProposalModifier pm = view.ModifyProposal(pr.proposalhash);
    pm->SetState(chainActive[809], DAOFlags::ACCEPTED);

    // We set states and we check that the highest block is always giving the state

    pr.SetState(chainActive[810], DAOFlags::ACCEPTED);

    BOOST_CHECK(pr.GetLastState() == DAOFlags::ACCEPTED);
    BOOST_CHECK(pr.GetLastStateBlockIndexForState(DAOFlags::ACCEPTED) == chainActive[810]);
    BOOST_CHECK(pr.GetLastStateBlockIndexForState(DAOFlags::REJECTED) == nullptr);
    BOOST_CHECK(pr.GetLastStateBlockIndexForState(DAOFlags::EXPIRED) == nullptr);
    BOOST_CHECK(pr.GetLastStateBlockIndex() == chainActive[810]);
    BOOST_CHECK(!pr.CanVote(view));

    pr.SetState(chainActive[811], DAOFlags::REJECTED);

    BOOST_CHECK(pr.GetLastState() == DAOFlags::REJECTED);
    BOOST_CHECK(pr.GetLastStateBlockIndexForState(DAOFlags::ACCEPTED) == chainActive[810]);
    BOOST_CHECK(pr.GetLastStateBlockIndexForState(DAOFlags::REJECTED) == chainActive[811]);
    BOOST_CHECK(pr.GetLastStateBlockIndexForState(DAOFlags::EXPIRED) == nullptr);
    BOOST_CHECK(pr.GetLastStateBlockIndex() == chainActive[811]);
    BOOST_CHECK(!pr.CanVote(view));

    pr.SetState(chainActive[809], DAOFlags::EXPIRED);

    BOOST_CHECK(pr.GetLastState() == DAOFlags::REJECTED);
    BOOST_CHECK(pr.GetLastStateBlockIndexForState(DAOFlags::ACCEPTED) == chainActive[810]);
    BOOST_CHECK(pr.GetLastStateBlockIndexForState(DAOFlags::REJECTED) == chainActive[811]);
    BOOST_CHECK(pr.GetLastStateBlockIndexForState(DAOFlags::EXPIRED) == chainActive[809]);
    BOOST_CHECK(pr.GetLastStateBlockIndex() == chainActive[811]);
    BOOST_CHECK(!pr.CanVote(view));

    pr.SetState(chainActive[808], DAOFlags::EXPIRED);

    BOOST_CHECK(pr.GetLastState() == DAOFlags::REJECTED);
    BOOST_CHECK(pr.GetLastStateBlockIndexForState(DAOFlags::ACCEPTED) == chainActive[810]);
    BOOST_CHECK(pr.GetLastStateBlockIndexForState(DAOFlags::REJECTED) == chainActive[811]);
    BOOST_CHECK(pr.GetLastStateBlockIndexForState(DAOFlags::EXPIRED) == chainActive[809]);
    BOOST_CHECK(pr.GetLastStateBlockIndex() == chainActive[811]);
    BOOST_CHECK(!pr.CanVote(view));

    pr.ClearState(chainActive[811]);

    BOOST_CHECK(pr.GetLastState() == DAOFlags::ACCEPTED);
    BOOST_CHECK(pr.GetLastStateBlockIndexForState(DAOFlags::ACCEPTED) == chainActive[810]);
    BOOST_CHECK(pr.GetLastStateBlockIndexForState(DAOFlags::REJECTED) == nullptr);
    BOOST_CHECK(pr.GetLastStateBlockIndexForState(DAOFlags::EXPIRED) == chainActive[809]);
    BOOST_CHECK(pr.GetLastStateBlockIndex() == chainActive[810]);
    BOOST_CHECK(!pr.CanVote(view));

    // We check the max amount of cycles for a payment request

    pr.nVotingCycle = 0;

    BOOST_CHECK(!pr.IsExpired());

    pr.SetState(chainActive[812], DAOFlags::REJECTED);
    // Status is rejected so can't get votes
    BOOST_CHECK(pr.GetLastState() == DAOFlags::REJECTED);
    BOOST_CHECK(!pr.CanVote(view));

    pr.SetState(chainActive[813], DAOFlags::EXPIRED);
    // Status is expired so can't get votes
    BOOST_CHECK(pr.GetLastState() == DAOFlags::EXPIRED);
    BOOST_CHECK(!pr.CanVote(view));

    pr.SetState(chainActive[814], DAOFlags::NIL);
    // Status is nil so can get votes
    BOOST_CHECK(pr.GetLastState() == DAOFlags::NIL);
    BOOST_CHECK(pr.CanVote(view));

    // It can request up to the proposal amount
    pr.nAmount = p.nAmount;
    BOOST_CHECK(pr.CanVote(view));

    pr.nAmount = p.nAmount+1;
    BOOST_CHECK(!pr.CanVote(view));

    pr.nAmount = p.nAmount;

    auto nMaxCycles = GetConsensusParameter(Consensus::CONSENSUS_PARAM_PAYMENT_REQUEST_MAX_VOTING_CYCLES);

    pr.nVotingCycle = nMaxCycles;

    BOOST_CHECK(!pr.IsExpired());
    // Status is nil so can't get votes
    BOOST_CHECK(pr.CanVote(view));

    pm->SetState(chainActive[710], DAOFlags::REJECTED);
    // If parent proposal is rejected it should still be able to be voted
    BOOST_CHECK(pr.CanVote(view));

    pm->SetState(chainActive[711], DAOFlags::NIL);
    // If parent proposal is rejected it should still be able to be voted
    BOOST_CHECK(pr.CanVote(view));

    pm->SetState(chainActive[712], DAOFlags::EXPIRED);
    // If parent proposal is rejected it should still be able to be voted
    BOOST_CHECK(pr.CanVote(view));

    view.RemoveProposal(p.hash);
    // If we remove the parent proposal it should not be able to be voted
    BOOST_CHECK(!pr.CanVote(view));

    pr.SetState(chainActive[815], DAOFlags::REJECTED);
    BOOST_CHECK(!pr.IsExpired());

    // Status is rejected so can't get votes
    BOOST_CHECK(pr.GetLastState() == DAOFlags::REJECTED);
    BOOST_CHECK(!pr.CanVote(view));

    pr.SetState(chainActive[816], DAOFlags::EXPIRED);
    // Status is expired so can't get votes
    BOOST_CHECK(pr.GetLastState() == DAOFlags::EXPIRED);
    BOOST_CHECK(!pr.CanVote(view));

    pr.nVotingCycle = nMaxCycles+1;

    // Should be expired
    BOOST_CHECK(pr.ExceededMaxVotingCycles());
    BOOST_CHECK(pr.IsExpired());
    // Status is expired so can't get votes
    BOOST_CHECK(!pr.CanVote(view));

    // Can we know when a proposal is accepted or rejected

    float nMinimumQuorum = GetConsensusParameter(Consensus::CONSENSUS_PARAM_PAYMENT_REQUEST_MIN_QUORUM)/10000.0;
    auto nMinSumVotes = GetConsensusParameter(Consensus::CONSENSUS_PARAM_VOTING_CYCLE_LENGTH) * nMinimumQuorum;
    auto nMinYesVotes = nMinSumVotes * GetConsensusParameter(Consensus::CONSENSUS_PARAM_PAYMENT_REQUEST_MIN_ACCEPT)/10000.0;
    auto nMinNoVotes = nMinSumVotes * GetConsensusParameter(Consensus::CONSENSUS_PARAM_PAYMENT_REQUEST_MIN_REJECT)/10000.0;


    BOOST_CHECK(pr.nVotesYes == 0);
    BOOST_CHECK(pr.nVotesNo == 0);
    BOOST_CHECK(!pr.IsAccepted());
    BOOST_CHECK(!pr.IsRejected());

    pr.nVotesYes = nMinSumVotes - nMinNoVotes;
    pr.nVotesNo = nMinNoVotes - 1;

    BOOST_CHECK(!pr.IsAccepted());
    BOOST_CHECK(!pr.IsRejected());

    pr.nVotesYes = nMinYesVotes - 1;
    pr.nVotesNo = nMinSumVotes - nMinYesVotes;

    BOOST_CHECK(!pr.IsAccepted());
    BOOST_CHECK(!pr.IsRejected());

    pr.nVotesYes = nMinSumVotes - nMinNoVotes - 1;
    pr.nVotesNo = nMinNoVotes + 1;

    BOOST_CHECK(!pr.IsAccepted());
    BOOST_CHECK(!pr.IsRejected());

    pr.nVotesYes = nMinYesVotes + 1;
    pr.nVotesNo = nMinSumVotes - nMinYesVotes - 1;

    BOOST_CHECK(!pr.IsAccepted());
    BOOST_CHECK(!pr.IsRejected());

    pr.nVotesYes = nMinSumVotes - nMinNoVotes + 1;
    pr.nVotesNo = nMinNoVotes - 1;

    BOOST_CHECK(!pr.IsAccepted());
    BOOST_CHECK(!pr.IsRejected());

    pr.nVotesYes = nMinYesVotes - 1;
    pr.nVotesNo = nMinSumVotes - nMinYesVotes + 1;

    BOOST_CHECK(!pr.IsAccepted());
    BOOST_CHECK(!pr.IsRejected());

    pr.nVotesYes = nMinSumVotes - nMinNoVotes;
    pr.nVotesNo = nMinNoVotes;

    BOOST_CHECK(!pr.IsAccepted());
    BOOST_CHECK(!pr.IsRejected());

    pr.nVotesYes = nMinYesVotes;
    pr.nVotesNo = nMinSumVotes - nMinYesVotes;

    BOOST_CHECK(!pr.IsAccepted());
    BOOST_CHECK(!pr.IsRejected());

    pr.nVotesYes = nMinSumVotes - nMinNoVotes;
    pr.nVotesNo = nMinNoVotes + 1;

    BOOST_CHECK(!pr.IsAccepted());
    BOOST_CHECK(pr.IsRejected());

    pr.nVotesYes = nMinYesVotes + 1;
    pr.nVotesNo = nMinSumVotes - nMinYesVotes;

    BOOST_CHECK(pr.IsAccepted());
    BOOST_CHECK(!pr.IsRejected());

}

BOOST_AUTO_TEST_SUITE_END()

