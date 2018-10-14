#!/usr/bin/env python3
# Copyright (c) 2018 The Navcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

from test_framework.test_framework import NavCoinTestFramework
from test_framework.util import *

import time


class CommunityFundPaymentRequestStateTest(NavCoinTestFramework):
    """Tests the state transition of payment requests of the Community fund."""

    def __init__(self):
        super().__init__()
        self.setup_clean_chain = True
        self.num_nodes = 1

    def setup_network(self, split=False):
        self.nodes = start_nodes(self.num_nodes, self.options.tmpdir)
        self.is_network_split = False

    def run_test(self):
        # Make sure cfund is active
        self.activate_cfund()

        # Donate to the cfund
        self.nodes[0].donatefund(10)

        # Create a proposal
        proposalid0 = self.nodes[0].createproposal(self.nodes[0].getnewaddress(), 10, 3600, "test")["hash"]
        self.start_new_cycle()

        # Accept the proposal
        self.nodes[0].proposalvote(proposalid0, "yes")
        self.start_new_cycle()

        # Proposal should be accepted
        assert (self.nodes[0].getproposal(proposalid0)["state"] == 1)
        assert (self.nodes[0].getproposal(proposalid0)["status"] == "accepted")

        # Create a payment request
        paymentrequestid0 = self.nodes[0].createpaymentrequest(proposalid0, 1, "test_payreq")["hash"]
        self.start_new_cycle()

        # Payment request initial state at beginning of cycle
        assert (self.nodes[0].getpaymentrequest(paymentrequestid0)["state"] == 0)
        assert (self.nodes[0].getpaymentrequest(paymentrequestid0)["status"] == "pending")

        # Move the necessary amount of cycles for the payment request to expire
        i = 0
        necessary = self.nodes[0].cfundstats()["consensus"]["maxCountVotingCyclePaymentRequests"]
        while i < necessary:
            self.start_new_cycle()
            i = i + 1

        # Validate that the payment request will expire at the end of the current voting cycle
        assert (self.nodes[0].getpaymentrequest(paymentrequestid0)["state"] == 0)
        assert (self.nodes[0].getpaymentrequest(paymentrequestid0)["status"] == "expired waiting for end of voting period")

        # Move to the last block of the voting cycle, where the payment request state changes
        self.end_cycle()

        # Validate that the payment request expired
        assert (self.nodes[0].getpaymentrequest(paymentrequestid0)["state"] == 3)
        assert (self.nodes[0].getpaymentrequest(paymentrequestid0)["status"] == "expired")

    def activate_cfund(self):
        self.slow_gen(100)
        # Verify the Community Fund is started
        assert (self.nodes[0].getblockchaininfo()["bip9_softforks"]["communityfund"]["status"] == "started")

        self.slow_gen(100)
        # Verify the Community Fund is locked_in
        assert (self.nodes[0].getblockchaininfo()["bip9_softforks"]["communityfund"]["status"] == "locked_in")

        self.slow_gen(100)
        # Verify the Community Fund is active
        assert (self.nodes[0].getblockchaininfo()["bip9_softforks"]["communityfund"]["status"] == "active")

    def end_cycle(self):
        # Move to the end of the cycle
        self.slow_gen(self.nodes[0].cfundstats()["votingPeriod"]["ending"] - self.nodes[0].cfundstats()["votingPeriod"][
            "current"])

    def start_new_cycle(self):
        # Move one past the end of the cycle
        self.slow_gen(self.nodes[0].cfundstats()["votingPeriod"]["ending"] - self.nodes[0].cfundstats()["votingPeriod"][
            "current"] + 1)

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
    CommunityFundPaymentRequestStateTest().main()
