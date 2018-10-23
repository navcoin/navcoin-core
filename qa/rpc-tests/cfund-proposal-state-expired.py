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

        # Create proposal
        proposalid0 = self.nodes[0].createproposal(self.nodes[0].getnewaddress(), 1, 3600, "test")["hash"]
        self.start_new_cycle()

        time.sleep(0.2)

        # Proposal initial state at beginning of cycle 1
        assert(self.nodes[0].getproposal(proposalid0)["state"] == 0)
        assert(self.nodes[0].getproposal(proposalid0)["status"] == "pending")

        # Move the necessary amount of cycles for the proposal to expire
        i = 0
        necessary = self.nodes[0].cfundstats()["consensus"]["maxCountVotingCycleProposals"]
        while i < necessary:
            self.start_new_cycle()
            i = i + 1

        # Validate that the proposal will expire at the end of the current voting cycle
        assert (self.nodes[0].getproposal(proposalid0)["state"] == 0)
        assert (self.nodes[0].getproposal(proposalid0)["status"] == "expired waiting for end of voting period")

        # Move to the last block of the voting cycle, where the proposal state changes
        self.end_cycle()

        # Validate that the proposal expired
        assert(self.nodes[0].getproposal(proposalid0)["state"] == 3)
        assert(self.nodes[0].getproposal(proposalid0)["status"] == "expired")

    def end_cycle(self):
        # Move to the end of the cycle
        self.slow_gen(self.nodes[0].cfundstats()["votingPeriod"]["ending"] - self.nodes[0].cfundstats()["votingPeriod"]["current"])

    def start_new_cycle(self):
        # Move one past the end of the cycle
        self.slow_gen(self.nodes[0].cfundstats()["votingPeriod"]["ending"] - self.nodes[0].cfundstats()["votingPeriod"]["current"] + 1)

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
