#!/usr/bin/env python3
# Copyright (c) 2018 The Navcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

from test_framework.test_framework import NavCoinTestFramework
from test_framework.cfund_util import *

import time


class CommunityFundProposalsTest(NavCoinTestFramework):
    """Tests the voting procedures of the Community fund."""

    def __init__(self):
        super().__init__()
        self.setup_clean_chain = True
        self.num_nodes = 1

    def setup_network(self, split=False):
        self.nodes = self.setup_nodes()
        self.is_network_split = split

    def run_test(self):
        # Get cfund parameters
        blocks_per_voting_cycle = self.nodes[0].cfundstats()["consensus"]["blocksPerVotingCycle"]

        self.nodes[0].staking(False)
        activate_cfund(self.nodes[0])
        self.nodes[0].donatefund(100)

        # Create proposals
        created_proposals = {
            "pure_yes": self.nodes[0].createproposal(self.nodes[0].getnewaddress(), 10, 3600, "pure_yes")["hash"],
            "pure_no": self.nodes[0].createproposal(self.nodes[0].getnewaddress(), 1, 3600, "pure_no")["hash"],
            "no_vote": self.nodes[0].createproposal(self.nodes[0].getnewaddress(), 1, 3600, "no_vote")["hash"],
            "50_50_vote": self.nodes[0].createproposal(self.nodes[0].getnewaddress(), 1, 3600, "50_50_vote")["hash"],
        }
        slow_gen(self.nodes[0] , 1)

        # Verify the proposals are now in the proposals list with matching hashes
        desc_lst = []

        for proposal in self.nodes[0].listproposals():
            desc_lst.append(proposal['description'])
            assert (created_proposals[proposal['description']] == proposal['hash'])
        assert (set(desc_lst) == set(created_proposals.keys()))

        # Move to the end of the 0th voting cycle
        end_cycle(self.nodes[0])

        # Verify we are still in the 0th voting cycle for all the created proposals
        desc_lst = []
        for proposal in self.nodes[0].listproposals():
            desc_lst.append(proposal['description'])
            assert (proposal['votingCycle'] == 0)
        assert (set(desc_lst) == set(created_proposals.keys()))

        # Move to the 1st voting cycle
        slow_gen(self.nodes[0] , 1)

        # Verify we are in voting cycle 1 for all the created proposals
        desc_lst = []
        for proposal in self.nodes[0].listproposals():
            desc_lst.append(proposal['description'])
            assert (proposal['votingCycle'] == 1)
        assert (set(desc_lst) == set(created_proposals.keys()))

        # Vote for proposals and move to 50% of the voting cycle
        self.nodes[0].proposalvote(created_proposals["pure_yes"], "yes")
        self.nodes[0].proposalvote(created_proposals["pure_no"], "no")
        self.nodes[0].proposalvote(created_proposals["50_50_vote"], "yes")
        slow_gen(self.nodes[0] , int(blocks_per_voting_cycle/2))

        # Vote for proposals and move to end of the 1st voting cycle
        self.nodes[0].proposalvote(created_proposals["50_50_vote"], "no")
        # Proposals get accepted on the last block of the current cycle
        end_cycle(self.nodes[0])

        # Verify proposal status and voting cycle for all the created proposals
        desc_lst = []
        for proposal in self.nodes[0].listproposals():
            desc_lst.append(proposal['description'])
            if proposal['description'] == "pure_yes":
                assert (proposal['votingCycle'] == 1)
                assert (proposal['status'] == 'accepted')
                assert (proposal['state'] == 1)
            elif proposal['description'] == "pure_no":
                assert (proposal['votingCycle'] == 1)
                assert (proposal['status'] == 'rejected')
                assert (proposal['state'] == 2)
            elif proposal['description'] in ["50_50_vote", "no_vote"]:
                assert (proposal['votingCycle'] == 1)
                assert (proposal['status'] == 'pending')
                assert (proposal['state'] == 0)
            else:
                assert False
        assert (set(desc_lst) == set(created_proposals.keys()))

        # The votingCycle increments on the first block of the new cycle
        slow_gen(self.nodes[0] , 1)
        # Verify proposal status and voting cycle for all the created proposals
        desc_lst = []
        for proposal in self.nodes[0].listproposals():
            desc_lst.append(proposal['description'])
            if proposal['description'] == "pure_yes":
                assert (proposal['votingCycle'] == 1)
                assert (proposal['status'] == 'accepted')
                assert (proposal['state'] == 1)
            elif proposal['description'] == "pure_no":
                assert (proposal['votingCycle'] == 1)
                assert (proposal['status'] == 'rejected')
                assert (proposal['state'] == 2)
            elif proposal['description'] in ["50_50_vote", "no_vote"]:
                assert (proposal['votingCycle'] == 2)
                assert (proposal['status'] == 'pending')
                assert (proposal['state'] == 0)
            else:
                assert False
        assert (set(desc_lst) == set(created_proposals.keys()))

        # Create payment request
        payreq0 = self.nodes[0].createpaymentrequest(created_proposals["pure_yes"], 4, "pure_yes_payreq")["hash"]
        slow_gen(self.nodes[0], 1)

        # Validate the payment request displays
        for proposal in self.nodes[0].listproposals():
            if proposal['description'] == "pure_yes":
                assert (float(proposal["notPaidYet"]) == 10)
                assert (proposal["paymentRequests"][0]["hash"] == payreq0)
                assert (float(proposal["paymentRequests"][0]["requestedAmount"]) == 4)
                assert (proposal["paymentRequests"][0]["status"] == "pending")
                assert (proposal["paymentRequests"][0]["state"] == 0)

        # Accept payment request
        self.nodes[0].paymentrequestvote(payreq0, "yes")
        end_cycle(self.nodes[0])

        # Validate that the request was accepted and check paid amounts
        for proposal in self.nodes[0].listproposals():
            if proposal['description'] == "pure_yes":
                assert (float(proposal["notPaidYet"]) == 6)
                assert (proposal["paymentRequests"][0]["hash"] == payreq0)
                assert (proposal["paymentRequests"][0]["status"] == "accepted")
                assert (proposal["paymentRequests"][0]["state"] == 1)


if __name__ == '__main__':
    CommunityFundProposalsTest().main()
