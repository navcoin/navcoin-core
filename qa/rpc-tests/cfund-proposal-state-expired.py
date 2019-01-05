#!/usr/bin/env python3
# Copyright (c) 2018 The Navcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

from test_framework.test_framework import NavCoinTestFramework
from test_framework.cfund_util import *

import time

class CommunityFundProposalStateTest(NavCoinTestFramework):
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

        # Create proposal
        proposalid0 = self.nodes[0].createproposal(self.nodes[0].getnewaddress(), 1, 3600, "test")["hash"]
        start_new_cycle(self.nodes[0])

        time.sleep(0.2)

        # Proposal initial state at beginning of cycle 1
        assert(self.nodes[0].getproposal(proposalid0)["state"] == 0)
        assert(self.nodes[0].getproposal(proposalid0)["status"] == "pending")

        # Move the necessary amount of cycles for the proposal to expire
        i = 0
        necessary = self.nodes[0].cfundstats()["consensus"]["maxCountVotingCycleProposals"]
        while i < necessary:
            start_new_cycle(self.nodes[0])
            i = i + 1

        # Validate that the proposal will expire at the end of the current voting cycle
        assert (self.nodes[0].getproposal(proposalid0)["state"] == 0)
        assert (self.nodes[0].getproposal(proposalid0)["status"] == "expired waiting for end of voting period")

        # Move to the last block of the voting cycle, where the proposal state changes
        end_cycle(self.nodes[0])

        # Validate that the proposal expired
        assert(self.nodes[0].getproposal(proposalid0)["state"] == 3)
        assert(self.nodes[0].getproposal(proposalid0)["status"] == "expired")


if __name__ == '__main__':
    CommunityFundProposalStateTest().main()
