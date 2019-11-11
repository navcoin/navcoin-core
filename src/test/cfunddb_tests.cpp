// Copyright (c) 2019 The NavCoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainparams.h>
#include <consensus/cfund.h>
#include <coins.h>
#include <random.h>
#include <uint256.h>
#include <test/test_navcoin.h>
#include <main.h>

#include <vector>
#include <map>

#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(coins_tests, BasicTestingSetup)

static const unsigned int NUM_SIMULATION_ITERATIONS = 250;

BOOST_AUTO_TEST_CASE(cfunddb_state)
{
    //CCoinsView base;
    CCoinsViewDB *pcoinsdbview = new CCoinsViewDB(1<<23, true);
    CCoinsViewCache *base = new CCoinsViewCache(pcoinsdbview);
    CCoinsViewCache view(base);

    std::map<uint256, CProposal> result_p;
    std::map<uint256, CPaymentRequest> result_r;

    int nRemovedCount = 0;

    for (unsigned int i = 0; i < NUM_SIMULATION_ITERATIONS; i++) {
        CProposal proposal;
        CProposal proposal2;
        CPaymentRequest prequest;
        CPaymentRequest prequest2;
        CPaymentRequestMap mapPaymentRequests;
        CProposalMap mapProposals;

        // Empty entries are not found
        CProposal emptyProposal;
        emptyProposal.hash = GetRandHash();

        view.AddProposal(emptyProposal);

        BOOST_CHECK(!view.HaveProposal(emptyProposal.hash));
        BOOST_CHECK(!view.GetProposal(emptyProposal.hash, proposal));
        BOOST_CHECK(!base->HaveProposal(emptyProposal.hash));
        BOOST_CHECK(!base->GetProposal(emptyProposal.hash, proposal));
        BOOST_CHECK(view.GetAllProposals(mapProposals));
        BOOST_CHECK(mapProposals.size()==0+2*i-nRemovedCount);
        BOOST_CHECK(view.GetAllPaymentRequests(mapPaymentRequests));
        BOOST_CHECK(mapPaymentRequests.size()==0+i);

        CPaymentRequest emptyPaymentRequest;
        emptyPaymentRequest.hash = GetRandHash();

        view.AddPaymentRequest(emptyPaymentRequest);

        BOOST_CHECK(!view.HavePaymentRequest(emptyPaymentRequest.hash));
        BOOST_CHECK(!view.GetPaymentRequest(emptyPaymentRequest.hash, prequest));
        BOOST_CHECK(!base->HavePaymentRequest(emptyPaymentRequest.hash));
        BOOST_CHECK(!base->GetPaymentRequest(emptyPaymentRequest.hash, prequest));
        BOOST_CHECK(view.GetAllProposals(mapProposals));
        BOOST_CHECK(mapProposals.size()==0+2*i-nRemovedCount);
        BOOST_CHECK(view.GetAllPaymentRequests(mapPaymentRequests));
        BOOST_CHECK(mapPaymentRequests.size()==0+i);

        // Construct valid entries
        CProposal validProposal;
        std::string strDZeel = "{\"n\":5000000000,\"a\":\"NP3h1uzYuZX9k3xmT5sZsrEceRFW5kxo2N\",\"d\":604800,\"s\":\"test\",\"v\":2}";
        uint256 hash = GetRandHash();
        uint256 blockhash = GetRandHash();
        CAmount nFee = Params().GetConsensus().nProposalMinimalFee;

        BOOST_CHECK(TxToProposal(strDZeel, hash, blockhash, nFee, validProposal));
        BOOST_CHECK(validProposal.hash == hash);
        BOOST_CHECK(validProposal.nAmount == 5000000000);
        BOOST_CHECK(validProposal.Address == "NP3h1uzYuZX9k3xmT5sZsrEceRFW5kxo2N");
        BOOST_CHECK(validProposal.nDeadline == 604800);
        BOOST_CHECK(validProposal.strDZeel == "test");
        BOOST_CHECK(validProposal.nFee == nFee);
        BOOST_CHECK(validProposal.txblockhash == blockhash);

        CProposal validProposal2;
        std::string strDZeel2 = "{\"n\":5100000000,\"a\":\"NP3h1uzYuZX9k3xmT5sZsrEceRFW5kxo2N\",\"d\":604800,\"s\":\"test 2\",\"v\":2}";
        uint256 hash2 = GetRandHash();
        uint256 blockhash2 = GetRandHash();

        BOOST_CHECK(TxToProposal(strDZeel2, hash2, blockhash2, nFee, validProposal2));
        BOOST_CHECK(validProposal2.hash == hash2);
        BOOST_CHECK(validProposal2.nAmount == 5100000000);
        BOOST_CHECK(validProposal2.Address == "NP3h1uzYuZX9k3xmT5sZsrEceRFW5kxo2N");
        BOOST_CHECK(validProposal2.nDeadline == 604800);
        BOOST_CHECK(validProposal2.strDZeel == "test 2");
        BOOST_CHECK(validProposal2.nFee == nFee);
        BOOST_CHECK(validProposal2.txblockhash == blockhash2);

        CPaymentRequest validPaymentRequest;
        std::string strDZeel3 = "{\"h\":\"e8b46f55fd222cf269ab2eb935ba38cc7c9d19b775d9dd781fb6c7b88059bdde\",\"n\":500000000000,\"s\":\"HxYeRn96t7B4H6G2FB11N/8ZpxJ9JhzSLIdn/kqQDV8ZGtfW0EdGS0kUJaoji+RGyBIH1MeA1yctQq4+QC+54lA=\",\"r\":\"eI852p895a9EfFAy\",\"i\":\"test\",\"v\":2}";
        uint256 hash3 = GetRandHash();
        uint256 blockhash3 = GetRandHash();

        BOOST_CHECK(TxToPaymentRequest(strDZeel3, hash3, blockhash3, validPaymentRequest, view));
        BOOST_CHECK(validPaymentRequest.hash == hash3);
        BOOST_CHECK(validPaymentRequest.nAmount == 500000000000);
        BOOST_CHECK(validPaymentRequest.proposalhash == uint256S("e8b46f55fd222cf269ab2eb935ba38cc7c9d19b775d9dd781fb6c7b88059bdde"));
        BOOST_CHECK(validPaymentRequest.strDZeel == "test");
        BOOST_CHECK(validPaymentRequest.nVersion == 2);
        BOOST_CHECK(validPaymentRequest.txblockhash == blockhash3);

        // View can not have the entry before adding it
        BOOST_CHECK(!view.HaveProposal(hash));
        BOOST_CHECK(!view.HaveProposal(hash2));
        BOOST_CHECK(!view.HavePaymentRequest(hash3));
        BOOST_CHECK(!view.GetProposal(hash, proposal));
        BOOST_CHECK(!view.GetProposal(hash2, proposal));
        BOOST_CHECK(!view.GetPaymentRequest(hash3, prequest));
        BOOST_CHECK(!base->HaveProposal(hash));
        BOOST_CHECK(!base->HaveProposal(hash2));
        BOOST_CHECK(!base->HavePaymentRequest(hash3));
        BOOST_CHECK(!base->GetProposal(hash, proposal));
        BOOST_CHECK(!base->GetProposal(hash2, proposal));
        BOOST_CHECK(!base->GetPaymentRequest(hash3, prequest));
        BOOST_CHECK(view.GetAllProposals(mapProposals));
        BOOST_CHECK(mapProposals.size()==0+2*i-nRemovedCount);
        BOOST_CHECK(view.GetAllPaymentRequests(mapPaymentRequests));
        BOOST_CHECK(mapPaymentRequests.size()==0+i);

        view.AddProposal(validProposal);
        view.AddPaymentRequest(validPaymentRequest);

        // top view should see the added entries
        BOOST_CHECK(view.HaveProposal(hash));
        BOOST_CHECK(!view.HaveProposal(hash2));
        BOOST_CHECK(view.HavePaymentRequest(hash3));
        BOOST_CHECK(view.GetProposal(hash, proposal));
        BOOST_CHECK(!view.GetProposal(hash2, proposal2));
        BOOST_CHECK(view.GetPaymentRequest(hash3, prequest));
        BOOST_CHECK(proposal == validProposal);
        BOOST_CHECK(prequest == validPaymentRequest);
        BOOST_CHECK(view.GetAllProposals(mapProposals));
        BOOST_CHECK(mapProposals.size()==1+2*i-nRemovedCount);
        BOOST_CHECK(view.GetAllPaymentRequests(mapPaymentRequests));
        BOOST_CHECK(mapPaymentRequests.size()==1*(i+1));
        // but they should not be on base view
        BOOST_CHECK(!base->HaveProposal(hash));
        BOOST_CHECK(!base->GetProposal(hash, proposal));
        BOOST_CHECK(!base->HavePaymentRequest(hash3));
        BOOST_CHECK(!base->HaveProposal(hash2));
        BOOST_CHECK(!base->GetProposal(hash2, proposal2));
        BOOST_CHECK(!base->GetPaymentRequest(hash3, prequest));

        view.AddProposal(validProposal2);

        BOOST_CHECK(view.HaveProposal(hash));
        BOOST_CHECK(view.HaveProposal(hash2));
        BOOST_CHECK(view.GetProposal(hash, proposal));
        BOOST_CHECK(view.GetProposal(hash2, proposal2));
        BOOST_CHECK(proposal == validProposal);
        BOOST_CHECK(proposal2 == validProposal2);
        // entries are not on the base view yet
        BOOST_CHECK(!base->HaveProposal(hash));
        BOOST_CHECK(!base->GetProposal(hash, proposal));
        BOOST_CHECK(!base->HaveProposal(hash2));
        BOOST_CHECK(!base->GetProposal(hash2, proposal2));
        BOOST_CHECK(view.GetAllProposals(mapProposals));
        BOOST_CHECK(mapProposals.size()==2*(i+1)-nRemovedCount);
        BOOST_CHECK(view.GetAllPaymentRequests(mapPaymentRequests));
        BOOST_CHECK(mapPaymentRequests.size()==1*(i+1));

        BOOST_CHECK(view.Flush());

        // entries should be removed from the cache top view after flush
        BOOST_CHECK(!view.HaveProposalInCache(hash));
        BOOST_CHECK(!view.HaveProposalInCache(hash2));
        BOOST_CHECK(!view.HavePaymentRequestInCache(hash3));
        // but still visible with Have/Get
        BOOST_CHECK(view.HaveProposal(hash));
        BOOST_CHECK(view.HaveProposal(hash2));
        BOOST_CHECK(view.HavePaymentRequest(hash3));
        BOOST_CHECK(view.GetProposal(hash, proposal));
        BOOST_CHECK(view.GetProposal(hash2, proposal2));
        BOOST_CHECK(view.GetPaymentRequest(hash3, prequest));
        BOOST_CHECK(proposal == validProposal);
        BOOST_CHECK(proposal2 == validProposal2);
        BOOST_CHECK(prequest == validPaymentRequest);
        // after Get they should be back in cache
        BOOST_CHECK(view.HaveProposalInCache(hash));
        BOOST_CHECK(view.HaveProposalInCache(hash2));
        BOOST_CHECK(view.HavePaymentRequestInCache(hash3));
        // entries should be now present on the base view
        BOOST_CHECK(base->HaveProposal(hash));
        BOOST_CHECK(base->GetProposal(hash, proposal));
        BOOST_CHECK(base->HaveProposal(hash2));
        BOOST_CHECK(base->GetProposal(hash2, proposal2));
        BOOST_CHECK(base->HavePaymentRequest(hash3));
        BOOST_CHECK(base->GetPaymentRequest(hash3, prequest));
        BOOST_CHECK(proposal == validProposal);
        BOOST_CHECK(proposal2 == validProposal2);
        BOOST_CHECK(prequest == validPaymentRequest);

        view.RemoveProposal(hash2);
        view.RemovePaymentRequest(hash3);

        // entry should not exist on the top view already before flush
        BOOST_CHECK(!view.HaveProposal(hash2));
        BOOST_CHECK(!view.GetProposal(hash2, proposal2));
        BOOST_CHECK(!view.HavePaymentRequest(hash3));
        BOOST_CHECK(!view.GetPaymentRequest(hash3, prequest));
        // and they should be removed from the top view full list
        BOOST_CHECK(view.GetAllProposals(mapProposals));
        BOOST_CHECK(mapProposals.size()==1+2*i-nRemovedCount);
        BOOST_CHECK(view.GetAllPaymentRequests(mapPaymentRequests));
        BOOST_CHECK(mapPaymentRequests.size()==i);

        BOOST_CHECK(view.Flush());

        // removed entries should not be in the cache after flush
        BOOST_CHECK(!view.HaveProposalInCache(hash));
        BOOST_CHECK(!view.HaveProposalInCache(hash2));
        BOOST_CHECK(!view.HavePaymentRequestInCache(hash3));
        BOOST_CHECK(view.HaveProposal(hash));
        BOOST_CHECK(!view.HaveProposal(hash2));
        BOOST_CHECK(!view.HavePaymentRequest(hash3));
        BOOST_CHECK(view.GetProposal(hash, proposal));
        BOOST_CHECK(!view.GetProposal(hash2, proposal2));
        BOOST_CHECK(!view.GetPaymentRequest(hash3, prequest));
        BOOST_CHECK(proposal == validProposal);
        // gone from base view too
        BOOST_CHECK(base->HaveProposal(hash));
        BOOST_CHECK(base->GetProposal(hash, proposal));
        BOOST_CHECK(proposal == validProposal);
        BOOST_CHECK(!base->HaveProposal(hash2));
        BOOST_CHECK(!base->GetProposal(hash2, proposal2));
        BOOST_CHECK(!base->HavePaymentRequest(hash3));
        BOOST_CHECK(!base->GetPaymentRequest(hash3, prequest));

        view.AddProposal(validProposal2);
        view.AddPaymentRequest(validPaymentRequest);

        // we should be able to re add the entry
        BOOST_CHECK(view.HaveProposal(hash));
        BOOST_CHECK(view.HaveProposal(hash2));
        BOOST_CHECK(view.HavePaymentRequest(hash3));
        BOOST_CHECK(view.GetProposal(hash, proposal));
        BOOST_CHECK(view.GetProposal(hash2, proposal2));
        BOOST_CHECK(view.GetPaymentRequest(hash3, prequest));
        BOOST_CHECK(proposal == validProposal);
        BOOST_CHECK(proposal2 == validProposal2);
        BOOST_CHECK(prequest == validPaymentRequest);
        // it should not be in base view before flush
        BOOST_CHECK(base->HaveProposal(hash));
        BOOST_CHECK(base->GetProposal(hash, proposal));
        BOOST_CHECK(!base->HaveProposal(hash2));
        BOOST_CHECK(!base->GetProposal(hash2, proposal2));
        BOOST_CHECK(!base->HavePaymentRequest(hash3));
        BOOST_CHECK(!base->GetPaymentRequest(hash3, prequest));
        // but it should be in the top view full list
        BOOST_CHECK(view.GetAllProposals(mapProposals));
        BOOST_CHECK(mapProposals.size()==2*(i+1)-nRemovedCount);
        BOOST_CHECK(view.GetAllPaymentRequests(mapPaymentRequests));
        BOOST_CHECK(mapPaymentRequests.size()==1*(i+1));

        BOOST_CHECK(view.Flush());

        // it should not be in cache now
        BOOST_CHECK(!view.HaveProposalInCache(hash));
        BOOST_CHECK(!view.HaveProposalInCache(hash2));
        BOOST_CHECK(!view.HavePaymentRequestInCache(hash3));
        // top view should be able to see it
        BOOST_CHECK(view.HaveProposal(hash));
        BOOST_CHECK(view.HaveProposal(hash2));
        BOOST_CHECK(view.HavePaymentRequest(hash3));
        BOOST_CHECK(view.GetProposal(hash, proposal));
        BOOST_CHECK(view.GetProposal(hash2, proposal2));
        BOOST_CHECK(view.GetPaymentRequest(hash3, prequest));
        // then back in the top view cache
        BOOST_CHECK(view.HaveProposalInCache(hash));
        BOOST_CHECK(view.HaveProposalInCache(hash2));
        BOOST_CHECK(view.HavePaymentRequestInCache(hash3));
        // and still be in the top view full list
        BOOST_CHECK(view.GetAllProposals(mapProposals));
        BOOST_CHECK(mapProposals.size()==2*(i+1)-nRemovedCount);
        BOOST_CHECK(view.GetAllPaymentRequests(mapPaymentRequests));
        BOOST_CHECK(mapPaymentRequests.size()==1*(i+1));
        BOOST_CHECK(proposal == validProposal);
        BOOST_CHECK(proposal2 == validProposal2);
        BOOST_CHECK(prequest == validPaymentRequest);
        // should be present on base view now
        BOOST_CHECK(base->HaveProposal(hash));
        BOOST_CHECK(base->GetProposal(hash, proposal));
        BOOST_CHECK(base->HaveProposal(hash2));
        BOOST_CHECK(base->GetProposal(hash2, proposal2));
        BOOST_CHECK(base->HavePaymentRequest(hash3));
        BOOST_CHECK(base->GetPaymentRequest(hash3, prequest));
        BOOST_CHECK(proposal == validProposal);
        BOOST_CHECK(proposal2 == validProposal2);
        BOOST_CHECK(prequest == validPaymentRequest);

        BOOST_CHECK(view.Flush());

        BOOST_CHECK(!view.HaveProposalInCache(hash));
        BOOST_CHECK(!view.HavePaymentRequestInCache(hash3));

        // test modifying an entry
        {
            CProposalModifier mProposal = view.ModifyProposal(hash);
            mProposal->nVotesYes = 1;
        }

        {
            CPaymentRequestModifier mPaymentRequest = view.ModifyPaymentRequest(hash3);
            mPaymentRequest->nVotesYes = 2;
        }

        BOOST_CHECK(view.HaveProposalInCache(hash));
        BOOST_CHECK(view.HaveProposal(hash));
        BOOST_CHECK(view.GetProposal(hash, proposal));
        BOOST_CHECK(proposal.nVotesYes == 1);

        BOOST_CHECK(view.HavePaymentRequestInCache(hash3));
        BOOST_CHECK(view.HavePaymentRequest(hash3));
        BOOST_CHECK(view.GetPaymentRequest(hash3, prequest));
        BOOST_CHECK(prequest.nVotesYes == 2);

        // we try to modify it again before flush
        {
            CProposalModifier mProposal = view.ModifyProposal(hash);
            mProposal->nVotesNo = 1;
        }

        {
            CPaymentRequestModifier mPaymentRequest = view.ModifyPaymentRequest(hash3);
            mPaymentRequest->nVotesNo = 5;
        }

        BOOST_CHECK(view.HaveProposalInCache(hash));
        BOOST_CHECK(view.HaveProposal(hash));
        BOOST_CHECK(view.GetProposal(hash, proposal));
        BOOST_CHECK(proposal.nVotesYes == 1);
        BOOST_CHECK(proposal.nVotesNo == 1);

        BOOST_CHECK(view.HavePaymentRequestInCache(hash3));
        BOOST_CHECK(view.HavePaymentRequest(hash3));
        BOOST_CHECK(view.GetPaymentRequest(hash3, prequest));
        BOOST_CHECK(prequest.nVotesYes == 2);
        BOOST_CHECK(prequest.nVotesNo == 5);

        BOOST_CHECK(view.Flush());

        // we verify after flush

        BOOST_CHECK(!view.HaveProposalInCache(hash));
        BOOST_CHECK(view.HaveProposal(hash));
        BOOST_CHECK(view.GetProposal(hash, proposal));
        BOOST_CHECK(proposal.nVotesYes == 1);
        BOOST_CHECK(proposal.nVotesNo == 1);

        BOOST_CHECK(!view.HavePaymentRequestInCache(hash3));
        BOOST_CHECK(view.HavePaymentRequest(hash3));
        BOOST_CHECK(view.GetPaymentRequest(hash3, prequest));
        BOOST_CHECK(prequest.nVotesYes == 2);
        BOOST_CHECK(prequest.nVotesNo == 5);

        // and modify afain after flush
        {
            CProposalModifier mProposal = view.ModifyProposal(hash);
            mProposal->nVotesYes = 2;
        }

        {
            CPaymentRequestModifier mPaymentRequest = view.ModifyPaymentRequest(hash3);
            mPaymentRequest->nVotesNo = 7;
        }

        BOOST_CHECK(view.HaveProposalInCache(hash));
        BOOST_CHECK(view.HaveProposal(hash));
        BOOST_CHECK(view.GetProposal(hash, proposal));
        BOOST_CHECK(proposal.nVotesYes == 2);
        BOOST_CHECK(proposal.nVotesNo == 1);

        BOOST_CHECK(view.HavePaymentRequestInCache(hash3));
        BOOST_CHECK(view.HavePaymentRequest(hash3));
        BOOST_CHECK(view.GetPaymentRequest(hash3, prequest));
        BOOST_CHECK(prequest.nVotesYes == 2);
        BOOST_CHECK(prequest.nVotesNo == 7);

        result_p.insert(std::make_pair(hash, proposal));
        result_r.insert(std::make_pair(hash3, prequest));

        BOOST_CHECK(view.GetAllProposals(mapProposals));
        BOOST_CHECK(mapProposals.size()==2*(i+1)-nRemovedCount);

        BOOST_CHECK(view.GetAllPaymentRequests(mapPaymentRequests));
        BOOST_CHECK(mapPaymentRequests.size()==1*(i+1));

        // flip a coin to decide if a proposal should be removed before flushing the base view
        if (insecure_rand() % 5 == 0)
        {
            view.RemoveProposal(hash2);
            view.Flush();
            nRemovedCount++;
            BOOST_CHECK(view.GetAllProposals(mapProposals));
            BOOST_CHECK(mapProposals.size()==2*(i+1)-nRemovedCount);
        }
        else
        {
            result_p.insert(std::make_pair(hash2, validProposal2));
        }

        BOOST_CHECK(base->Flush());
    }

    CProposal proposal;
    CPaymentRequest prequest;

    for (auto& it: result_p)
    {
        BOOST_CHECK(view.HaveProposal(it.first));
        BOOST_CHECK(view.GetProposal(it.first, proposal));
        BOOST_CHECK(proposal == it.second);
    }

    for (auto& it: result_r)
    {
        BOOST_CHECK(view.HavePaymentRequest(it.first));
        BOOST_CHECK(view.GetPaymentRequest(it.first, prequest));
        BOOST_CHECK(prequest == it.second);
    }
}

BOOST_AUTO_TEST_SUITE_END()

