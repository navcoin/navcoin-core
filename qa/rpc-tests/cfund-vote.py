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
        slow_gen(self.nodes[0], 100)
        # Verify the Community Fund is started
        assert(self.nodes[0].getblockchaininfo()["bip9_softforks"]["communityfund"]["status"] == "started")

        slow_gen(self.nodes[0], 100)
        # Verify the Community Fund is locked_in
        assert(self.nodes[0].getblockchaininfo()["bip9_softforks"]["communityfund"]["status"] == "locked_in")

        slow_gen(self.nodes[0], 100)
        # Verify the Community Fund is active
        assert(self.nodes[0].getblockchaininfo()["bip9_softforks"]["communityfund"]["status"] == "active")

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

        # Add dummy votes
        slow_gen(self.nodes[0], 5)
        self.nodes[0].proposalvote(proposalid0, "yes")
        slow_gen(self.nodes[0], 5)
        self.nodes[0].proposalvote(proposalid0, "remove")

        # Check votes are added
        assert(self.nodes[0].getproposal(proposalid0)["votesYes"] == 5 and self.nodes[0].getproposal(proposalid0)["votesNo"] == 5)

        # Move to the end of the cycle
        slow_gen(self.nodes[0], self.nodes[0].cfundstats()["votingPeriod"]["ending"] - self.nodes[0].cfundstats()["votingPeriod"]["current"])

        # Check we are still in the first cycle
        assert(self.nodes[0].getproposal(proposalid0)["votingCycle"] == 0)

        slow_gen(self.nodes[0], 1)

        # Check we are in a new cycle
        assert(self.nodes[0].getproposal(proposalid0)["votingCycle"] == 1)

        # Check the number of votes reseted and the proposal state
        assert(self.nodes[0].getproposal(proposalid0)["votesYes"] == 0 and self.nodes[0].getproposal(proposalid0)["votesNo"] == 0)
        assert(self.nodes[0].getproposal(proposalid0)["status"] == "pending" and self.nodes[0].getproposal(proposalid0)["state"] == 0)

        slow_gen(self.nodes[0], self.nodes[0].cfundstats()["consensus"]["blocksPerVotingCycle"] - 10)
        self.nodes[0].proposalvote(proposalid0, "yes")

        # Vote in the limits of a cycle
        slow_gen(self.nodes[0], 9)

        # Check we are still in the same cycle
        assert(self.nodes[0].getproposal(proposalid0)["votingCycle"] == 1)

        # Check the number of votes reseted and the proposal state
        assert(self.nodes[0].getproposal(proposalid0)["votesYes"] == 9 and self.nodes[0].getproposal(proposalid0)["votesNo"] == 0)
        assert(self.nodes[0].getproposal(proposalid0)["status"] == "pending" and self.nodes[0].getproposal(proposalid0)["state"] == 0)

        # Vote into the new cycle
        firstblockofcycle = slow_gen(self.nodes[0], 1)[0]

        # Check we are in the new cycle
        assert(self.nodes[0].getproposal(proposalid0)["votingCycle"] == 2)

        # Check the number of votes
        assert(self.nodes[0].getproposal(proposalid0)["votesYes"] == 0 and self.nodes[0].getproposal(proposalid0)["votesNo"] == 0)
        assert(self.nodes[0].getproposal(proposalid0)["status"] == "pending" and self.nodes[0].getproposal(proposalid0)["state"] == 0)

        # Move back to the end of the previous cycle
        self.nodes[0].invalidateblock(firstblockofcycle)

        # Check we are again in the old cycle
        assert(self.nodes[0].getproposal(proposalid0)["votingCycle"] == 1)
        assert(self.nodes[0].getproposal(proposalid0)["votesYes"] == 9 and self.nodes[0].getproposal(proposalid0)["votesNo"] == 0)
        assert(self.nodes[0].getproposal(proposalid0)["status"] == "pending" and self.nodes[0].getproposal(proposalid0)["state"] == 0)


if __name__ == '__main__':
    CommunityFundVotesTest().main()
