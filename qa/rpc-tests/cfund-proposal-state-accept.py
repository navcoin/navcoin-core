#!/usr/bin/env python3
# Copyright (c) 2018 The Navcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

from test_framework.test_framework import NavCoinTestFramework
from test_framework.util import *

import time

class CommunityFundProposalStateTest(NavCoinTestFramework):
    """Tests the state transition of proposals of the Community fund."""

    def __init__(self):
        super().__init__()
        self.setup_clean_chain = True
        self.num_nodes = 1

    def setup_network(self, split=False):
        self.nodes = start_nodes(self.num_nodes, self.options.tmpdir)
        self.is_network_split = False

    def run_test(self):
        self.slow_gen(100)
        # Verify the Community Fund is started
        assert(self.nodes[0].getblockchaininfo()["bip9_softforks"]["communityfund"]["status"] == "started")

        self.slow_gen(100)
        # Verify the Community Fund is locked_in
        assert(self.nodes[0].getblockchaininfo()["bip9_softforks"]["communityfund"]["status"] == "locked_in")

        self.slow_gen(100)
        # Verify the Community Fund is active
        assert(self.nodes[0].getblockchaininfo()["bip9_softforks"]["communityfund"]["status"] == "active")

        proposalid0 = self.nodes[0].createproposal(self.nodes[0].getnewaddress(), 1, 3600, "test")["hash"]
        self.slow_gen(1)

        self.start_new_cycle()

        time.sleep(0.2)

        # Proposal initial state at beginning of cycle

        assert(self.nodes[0].getproposal(proposalid0)["state"] == 0)
        assert(self.nodes[0].getproposal(proposalid0)["status"] == "pending")

        # Vote enough yes votes, without enough quorum

        total_votes = self.nodes[0].cfundstats()["consensus"]["minSumVotesPerVotingCycle"]
        min_yes_votes = self.nodes[0].cfundstats()["consensus"]["votesAcceptProposalPercentage"]/100
        yes_votes = int(total_votes * min_yes_votes) + 1

        self.nodes[0].proposalvote(proposalid0, "yes")
        self.slow_gen(yes_votes)
        self.nodes[0].proposalvote(proposalid0, "no")
        self.slow_gen(total_votes - yes_votes)
        self.nodes[0].proposalvote(proposalid0, "remove")

        # Should still be in pending

        assert(self.nodes[0].getproposal(proposalid0)["state"] == 0)
        assert(self.nodes[0].getproposal(proposalid0)["status"] == "pending")

        self.start_new_cycle()
        time.sleep(0.2)

        # Proposal initial state at beginning of cycle

        assert(self.nodes[0].getproposal(proposalid0)["state"] == 0)
        assert(self.nodes[0].getproposal(proposalid0)["status"] == "pending")

        # Vote enough quorum, but not enough positive votes
        total_votes = self.nodes[0].cfundstats()["consensus"]["minSumVotesPerVotingCycle"] + 1
        yes_votes = int(total_votes * min_yes_votes)

        self.nodes[0].proposalvote(proposalid0, "yes")
        self.slow_gen(yes_votes)
        self.nodes[0].proposalvote(proposalid0, "no")
        self.slow_gen(total_votes - yes_votes)
        self.nodes[0].proposalvote(proposalid0, "remove")

        assert(self.nodes[0].getproposal(proposalid0)["state"] == 0)
        assert(self.nodes[0].getproposal(proposalid0)["status"] == "pending")

        self.start_new_cycle()
        time.sleep(0.2)

        # Proposal initial state at beginning of cycle

        assert(self.nodes[0].getproposal(proposalid0)["state"] == 0)
        assert(self.nodes[0].getproposal(proposalid0)["status"] == "pending")

        # Vote enough quorum and enough positive votes

        total_votes = self.nodes[0].cfundstats()["consensus"]["minSumVotesPerVotingCycle"] + 1
        yes_votes = int(total_votes * min_yes_votes) + 1

        self.nodes[0].proposalvote(proposalid0, "yes")
        self.slow_gen(yes_votes)
        self.nodes[0].proposalvote(proposalid0, "no")
        blocks = self.slow_gen(total_votes - yes_votes)
        self.nodes[0].proposalvote(proposalid0, "remove")

        assert(self.nodes[0].getproposal(proposalid0)["state"] == 0)
        assert(self.nodes[0].getproposal(proposalid0)["status"] == "accepted waiting for end of voting period")

        time.sleep(0.2)

        # Revert last vote and check status

        self.nodes[0].invalidateblock(blocks[-1])
        assert(self.nodes[0].getproposal(proposalid0)["state"] == 0)
        assert(self.nodes[0].getproposal(proposalid0)["status"] == "pending")
        self.nodes[0].cfundstats()

        # Vote again

        self.nodes[0].proposalvote(proposalid0, "yes")
        self.slow_gen(1)
        self.nodes[0].proposalvote(proposalid0, "remove")


        # Move to a new cycle...
        time.sleep(0.2)

        self.start_new_cycle()
        blocks=self.slow_gen(1)

        # Proposal must be accepted waiting for fund now
        assert(self.nodes[0].getproposal(proposalid0)["state"] == 4)
        assert(self.nodes[0].getproposal(proposalid0)["status"] == "accepted waiting for enough coins in fund")

        # Check the available and locked funds
        assert(self.nodes[0].cfundstats()["funds"]["available"] == self.nodes[0].cfundstats()["consensus"]["proposalMinimalFee"])
        assert(self.nodes[0].cfundstats()["funds"]["locked"] == 0)

        # Donate to the fund
        self.nodes[0].donatefund(1)
        self.slow_gen(1)

        # Check the available and locked funds
        assert (self.nodes[0].cfundstats()["funds"]["available"] == 1+self.nodes[0].cfundstats()["consensus"]["proposalMinimalFee"])
        assert (self.nodes[0].cfundstats()["funds"]["locked"] == 0)

        # Move to the end of the cycle
        self.start_new_cycle()

        # Validate that the proposal is accepted
        assert(self.nodes[0].getproposal(proposalid0)["state"] == 1)
        assert(self.nodes[0].getproposal(proposalid0)["status"] == "accepted")

        # Check the available and locked funds
        assert (self.nodes[0].cfundstats()["funds"]["available"] == self.nodes[0].cfundstats()["consensus"]["proposalMinimalFee"])
        assert (self.nodes[0].cfundstats()["funds"]["locked"] == 1)

    def start_new_cycle(self):
        # Move to the end of the cycle
        self.slow_gen(self.nodes[0].cfundstats()["votingPeriod"]["ending"] - self.nodes[0].cfundstats()["votingPeriod"]["current"])
        
    def slow_gen(self, count):
        total = count
        blocks = []
        while total > 0:
            now = min(total, 5)
            blocks.extend(self.nodes[0].generate(now))
            total -= now
            time.sleep(0.1)
        return blocks


if __name__ == '__main__':
    CommunityFundProposalStateTest().main()
