#!/usr/bin/env python3
# Copyright (c) 2018 The Navcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

from test_framework.test_framework import NavCoinTestFramework
from test_framework.cfund_util import *

import time

class CommunityFundProposalVoteListTest(NavCoinTestFramework):
    """Tests the proposalvotelist function of the Community fund."""

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

        # Preflight checks
        assert(len(self.nodes[0].proposalvotelist()["yes"]) == 0)
        assert(len(self.nodes[0].proposalvotelist()["no"]) == 0)
        assert(len(self.nodes[0].proposalvotelist()["null"]) == 0)

        address0 = self.nodes[0].getnewaddress()

        # Create 3 proposals
        proposalid0 = self.nodes[0].createproposal(address0, 1, 3600, "test0")["hash"]
        proposalid1 = self.nodes[0].createproposal(address0, 1, 3600, "test1")["hash"]
        proposalid2 = self.nodes[0].createproposal(address0, 1, 3600, "test2")["hash"]
        slow_gen(self.nodes[0], 1)

        # Verify the proposals are now in the proposal vote list
        assert(len(self.nodes[0].proposalvotelist()["yes"]) == 0)
        assert(len(self.nodes[0].proposalvotelist()["no"]) == 0)
        assert(len(self.nodes[0].proposalvotelist()["null"]) == 3)

        # Vote on the proposals as wished
        self.nodes[0].proposalvote(proposalid0, "yes")
        self.nodes[0].proposalvote(proposalid1, "no")
        self.nodes[0].proposalvote(proposalid2, "yes")
        self.nodes[0].proposalvote(proposalid2, "remove")

        # Verify the proposal vote list has changed according to the wallet's votes
        assert(len(self.nodes[0].proposalvotelist()["yes"]) == 1)
        assert(len(self.nodes[0].proposalvotelist()["no"]) == 1)
        assert(len(self.nodes[0].proposalvotelist()["null"]) == 1)

        # Verify the hashes are contained in the output vote list
        assert(proposalid0 == self.nodes[0].proposalvotelist()["yes"][0]["hash"])
        assert(proposalid1 == self.nodes[0].proposalvotelist()["no"][0]["hash"])
        assert(proposalid2 == self.nodes[0].proposalvotelist()["null"][0]["hash"])

        # Revote differently
        self.nodes[0].proposalvote(proposalid0, "no")
        self.nodes[0].proposalvote(proposalid1, "yes")
        self.nodes[0].proposalvote(proposalid2, "no")

        # Verify the proposal vote list has changed according to the wallet's votes
        assert(len(self.nodes[0].proposalvotelist()["yes"]) == 1)
        assert(len(self.nodes[0].proposalvotelist()["no"]) == 2)
        assert(len(self.nodes[0].proposalvotelist()["null"]) == 0)


        # Create new proposal
        proposalid3 = self.nodes[0].createproposal(address0, 1, 3600, "test3")["hash"]
        slow_gen(self.nodes[0], 1)

        # Check the new proposal has been added to "null" of proposal vote list
        assert(len(self.nodes[0].proposalvotelist()["null"]) == 1)
        assert(proposalid3 == self.nodes[0].proposalvotelist()["null"][0]["hash"])


if __name__ == '__main__':
    CommunityFundProposalVoteListTest().main()
