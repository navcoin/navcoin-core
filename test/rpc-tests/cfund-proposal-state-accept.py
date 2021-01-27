#!/usr/bin/env python3
# Copyright (c) 2018 The Navcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

from test_framework.test_framework import NavcoinTestFramework
from test_framework.cfund_util import *

import time

class CommunityFundProposalStateTest(NavcoinTestFramework):
    """Tests the state transition of proposals of the Community fund."""

    def __init__(self):
        super().__init__()
        self.setup_clean_chain = True
        self.num_nodes = 1

    def setup_network(self, split=False):
        self.nodes = self.setup_nodes()
        self.is_network_split = split

    def run_test(self):
        self.nodes[0].staking(False)
        activate_cfund(self.nodes[0])

        proposal_duration = 30

        proposalid0 = self.nodes[0].createproposal(self.nodes[0].getnewaddress(), 100, proposal_duration, "test")["hash"]
        slow_gen(self.nodes[0], 1)

        start_new_cycle(self.nodes[0])

        time.sleep(0.2)

        # Proposal initial state at beginning of cycle

        assert(self.nodes[0].getproposal(proposalid0)["state"] == 0)
        assert(self.nodes[0].getproposal(proposalid0)["status"] == "pending")

        # Vote enough yes votes, without enough quorum

        total_votes = self.nodes[0].cfundstats()["consensus"]["minSumVotesPerVotingCycle"]
        min_yes_votes = self.nodes[0].cfundstats()["consensus"]["votesAcceptProposalPercentage"]/100
        yes_votes = int(total_votes * min_yes_votes) + 1

        self.nodes[0].proposalvote(proposalid0, "yes")
        slow_gen(self.nodes[0], yes_votes)
        self.nodes[0].proposalvote(proposalid0, "no")
        slow_gen(self.nodes[0], total_votes - yes_votes - 2)
        self.nodes[0].proposalvote(proposalid0, "abs")
        slow_gen(self.nodes[0], 2)
        self.nodes[0].proposalvote(proposalid0, "remove")

        # Should still be in pending

        assert(self.nodes[0].getproposal(proposalid0)["state"] == 0)
        assert(self.nodes[0].getproposal(proposalid0)["status"] == "pending")

        start_new_cycle(self.nodes[0])
        time.sleep(0.2)

        # Proposal initial state at beginning of cycle

        assert(self.nodes[0].getproposal(proposalid0)["state"] == 0)
        assert(self.nodes[0].getproposal(proposalid0)["status"] == "pending")

        # Vote enough quorum, but not enough positive votes
        total_votes = self.nodes[0].cfundstats()["consensus"]["minSumVotesPerVotingCycle"] + 1
        yes_votes = int(total_votes * min_yes_votes)

        self.nodes[0].proposalvote(proposalid0, "yes")
        slow_gen(self.nodes[0], yes_votes)
        self.nodes[0].proposalvote(proposalid0, "no")
        slow_gen(self.nodes[0], total_votes - yes_votes - 5)
        self.nodes[0].proposalvote(proposalid0, "abs")
        slow_gen(self.nodes[0], 5)
        self.nodes[0].proposalvote(proposalid0, "remove")

        assert(self.nodes[0].getproposal(proposalid0)["state"] == 0)
        assert(self.nodes[0].getproposal(proposalid0)["status"] == "pending")

        start_new_cycle(self.nodes[0])
        time.sleep(0.2)

        # Proposal initial state at beginning of cycle

        assert(self.nodes[0].getproposal(proposalid0)["state"] == 0)
        assert(self.nodes[0].getproposal(proposalid0)["status"] == "pending")

        # Vote enough quorum and enough positive votes

        total_votes = self.nodes[0].cfundstats()["consensus"]["minSumVotesPerVotingCycle"] + 1
        yes_votes = int(total_votes * min_yes_votes) + 1


        self.nodes[0].proposalvote(proposalid0, "no")
        slow_gen(self.nodes[0], total_votes - yes_votes - 2)
        self.nodes[0].proposalvote(proposalid0, "abs")
        slow_gen(self.nodes[0], 2)
        self.nodes[0].proposalvote(proposalid0, "yes")
        blocks = slow_gen(self.nodes[0], yes_votes)

        self.nodes[0].proposalvote(proposalid0, "remove")

        assert(self.nodes[0].getproposal(proposalid0)["state"] == 0)
        assert(self.nodes[0].getproposal(proposalid0)["status"] == "pending")

        time.sleep(0.2)

        # Revert last vote and check status

        self.nodes[0].invalidateblock(blocks[-1])
        assert(self.nodes[0].getproposal(proposalid0)["state"] == 0)
        assert(self.nodes[0].getproposal(proposalid0)["status"] == "pending")
        self.nodes[0].cfundstats()

        # Vote again

        self.nodes[0].proposalvote(proposalid0, "yes")
        slow_gen(self.nodes[0], 1)
        self.nodes[0].proposalvote(proposalid0, "remove")


        # Move to a new cycle...
        time.sleep(0.2)

        start_new_cycle(self.nodes[0])
        blocks = slow_gen(self.nodes[0], 1)

        # Proposal must be accepted waiting for fund now
        assert(self.nodes[0].getproposal(proposalid0)["state"] == 4)
        assert(self.nodes[0].getproposal(proposalid0)["status"] == "accepted, waiting for enough coins in fund")

        # Check the available and locked funds
        assert(self.nodes[0].cfundstats()["funds"]["available"] == self.nodes[0].cfundstats()["consensus"]["proposalMinimalFee"])
        assert(self.nodes[0].cfundstats()["funds"]["locked"] == 0)

        # Donate to the fund
        self.nodes[0].donatefund(100)
        slow_gen(self.nodes[0], 1)

        # Check the available and locked funds
        assert (self.nodes[0].cfundstats()["funds"]["available"] == 100+self.nodes[0].cfundstats()["consensus"]["proposalMinimalFee"])
        assert (self.nodes[0].cfundstats()["funds"]["locked"] == 0)

        # Move to the end of the cycle
        start_new_cycle(self.nodes[0])

        # Validate that the proposal is accepted
        assert(self.nodes[0].getproposal(proposalid0)["state"] == 1)
        assert(self.nodes[0].getproposal(proposalid0)["status"] == "accepted")

        # Check the available and locked funds
        assert (self.nodes[0].cfundstats()["funds"]["available"] == self.nodes[0].cfundstats()["consensus"]["proposalMinimalFee"])
        assert (self.nodes[0].cfundstats()["funds"]["locked"] == 100)

        # Check the voting cycle does not increment in later cycle after reorg
        votingCycle_after_state_change = self.nodes[0].getproposal(proposalid0)["votingCycle"]
        start_new_cycle(self.nodes[0])
        blocks=slow_gen(self.nodes[0], 1)
        self.nodes[0].invalidateblock(blocks[-1])
        assert(self.nodes[0].getproposal(proposalid0)["votingCycle"] == votingCycle_after_state_change)

        blocks=slow_gen(self.nodes[0], 1)

        # Load the expires_on value before the loop
        expires_on = int(self.nodes[0].getproposal(proposalid0)["expiresOn"])

        # Wait for the proposal to expire
        while int(self.nodes[0].getblock(blocks[0])["time"]) <= expires_on:
            blocks=slow_gen(self.nodes[0], 1, 0.5)

        assert(self.nodes[0].getproposal(proposalid0)["status"] == "pending")

        self.nodes[0].invalidateblock(blocks[-1])
        assert(self.nodes[0].getproposal(proposalid0)["status"] == "accepted")

        slow_gen(self.nodes[0], 1)
        assert(self.nodes[0].getproposal(proposalid0)["status"] == "pending")

        start_new_cycle(self.nodes[0])

        # Should be expired now
        assert(self.nodes[0].getproposal(proposalid0)["status"] == "accepted, expired")

        slow_gen(self.nodes[0], 1)

        start_new_cycle(self.nodes[0])

        # Should still be expired
        assert(self.nodes[0].getproposal(proposalid0)["status"] == "accepted, expired")

if __name__ == '__main__':
    CommunityFundProposalStateTest().main()
