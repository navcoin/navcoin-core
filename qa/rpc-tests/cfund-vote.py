#!/usr/bin/env python3
# Copyright (c) 2018 The Navcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

from test_framework.test_framework import NavCoinTestFramework
from test_framework.cfund_util import *

import time

class CommunityFundVotesTest(NavCoinTestFramework):
    """Tests the voting procedures of the Community fund."""

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
        self.nodes[0].donatefund(1000)

        proposalid0 = self.nodes[0].createproposal(self.nodes[0].getnewaddress(), 1, 3600, "test")["hash"]
        slow_gen(self.nodes[0], 1)

        # Verify the proposal is now in the proposals list
        assert(self.nodes[0].getproposal(proposalid0)["hash"] == proposalid0)

        self.nodes[0].proposalvote(proposalid0, "yes")
        blockhash_yes = slow_gen(self.nodes[0], 1)[0]

        # Verify the vote has been counted
        assert(self.nodes[0].getproposal(proposalid0)["votingCycle"] == 0)
        assert(self.nodes[0].getproposal(proposalid0)["votesYes"] == 1 and self.nodes[0].getproposal(proposalid0)["votesNo"] == 0)

        self.nodes[0].proposalvote(proposalid0, "no")
        blockhash_no = slow_gen(self.nodes[0], 1)[0]

        # Verify the vote has been counted
        assert(self.nodes[0].getproposal(proposalid0)["votesYes"] == 1 and self.nodes[0].getproposal(proposalid0)["votesNo"] == 1)

        self.nodes[0].invalidateblock(blockhash_no)

        # Verify the votes have been reseted
        assert(self.nodes[0].getproposal(proposalid0)["votesYes"] == 1 and self.nodes[0].getproposal(proposalid0)["votesNo"] == 0)

        self.nodes[0].invalidateblock(blockhash_yes)

        # Verify the votes have been reseted
        assert(self.nodes[0].getproposal(proposalid0)["votesYes"] == 0 and self.nodes[0].getproposal(proposalid0)["votesNo"] == 0)

        slow_gen(self.nodes[0], 1)
        assert(self.nodes[0].getproposal(proposalid0)["votesYes"] == 0 and self.nodes[0].getproposal(proposalid0)["votesNo"] == 1)
        assert_equal(self.nodes[0].getproposal(proposalid0)["votingCycle"], 0)

        self.nodes[0].proposalvote(proposalid0, "remove")

        # Add dummy votes
        end_cycle(self.nodes[0])

        total_votes = self.nodes[0].cfundstats()["consensus"]["minSumVotesPerVotingCycle"]
        min_yes_votes = self.nodes[0].cfundstats()["consensus"]["votesAcceptPaymentRequestPercentage"]/100
        yes_votes = int(total_votes * min_yes_votes) + 1
        no_votes = total_votes - yes_votes

        self.nodes[0].proposalvote(proposalid0, "no")
        slow_gen(self.nodes[0], no_votes)
        self.nodes[0].proposalvote(proposalid0, "yes")
        slow_gen(self.nodes[0], yes_votes)
        self.nodes[0].proposalvote(proposalid0, "remove")

        assert_equal(self.nodes[0].getproposal(proposalid0)["votingCycle"], 1)

        # Check votes are added
        assert_equal(self.nodes[0].getproposal(proposalid0)["votesYes"], yes_votes)
        assert_equal(self.nodes[0].getproposal(proposalid0)["votesNo"], no_votes)

        # Move to the end of the cycle
        slow_gen(self.nodes[0], self.nodes[0].cfundstats()["votingPeriod"]["ending"] - self.nodes[0].cfundstats()["votingPeriod"]["current"])

        assert(self.nodes[0].getproposal(proposalid0)["votingCycle"] == 1)

        slow_gen(self.nodes[0], 1)
        # Check we are in a new cycle
        assert(self.nodes[0].getproposal(proposalid0)["votingCycle"] == 2)

        # Check the number of votes reseted and the proposal state
        assert(self.nodes[0].getproposal(proposalid0)["votesYes"] == 0 and self.nodes[0].getproposal(proposalid0)["votesNo"] == 0)
        assert(self.nodes[0].getproposal(proposalid0)["status"] == "pending" and self.nodes[0].getproposal(proposalid0)["state"] == 0)

        slow_gen(self.nodes[0], self.nodes[0].cfundstats()["consensus"]["blocksPerVotingCycle"] - 5)

        self.nodes[0].proposalvote(proposalid0, "yes")
        # Vote in the limits of a cycle
        slow_gen(self.nodes[0], 4)

        # Check we are still in the same cycle
        assert(self.nodes[0].getproposal(proposalid0)["votingCycle"] == 2)

        # Check the number of votes reseted and the proposal state
        assert(self.nodes[0].getproposal(proposalid0)["votesYes"] == 4 and self.nodes[0].getproposal(proposalid0)["votesNo"] == 0)
        assert(self.nodes[0].getproposal(proposalid0)["status"] == "pending" and self.nodes[0].getproposal(proposalid0)["state"] == 0)

        # Vote into the new cycle
        firstblockofcycle = slow_gen(self.nodes[0], 1)[0]

        # Check we are in the new cycle
        assert(self.nodes[0].getproposal(proposalid0)["votingCycle"] == 3)

        # Check the number of votes
        assert(self.nodes[0].getproposal(proposalid0)["votesYes"] == 1 and self.nodes[0].getproposal(proposalid0)["votesNo"] == 0)
        assert(self.nodes[0].getproposal(proposalid0)["status"] == "pending" and self.nodes[0].getproposal(proposalid0)["state"] == 0)

        # Move back to the end of the previous cycle
        self.nodes[0].invalidateblock(firstblockofcycle)

        # Check we are again in the old cycle
        assert(self.nodes[0].getproposal(proposalid0)["votingCycle"] == 2)
        assert(self.nodes[0].getproposal(proposalid0)["votesYes"] == 4 and self.nodes[0].getproposal(proposalid0)["votesNo"] == 0)
        assert(self.nodes[0].getproposal(proposalid0)["status"] == "pending" and self.nodes[0].getproposal(proposalid0)["state"] == 0)


if __name__ == '__main__':
    CommunityFundVotesTest().main()
