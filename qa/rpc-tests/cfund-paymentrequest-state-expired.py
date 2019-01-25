#!/usr/bin/env python3
# Copyright (c) 2018 The Navcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

from test_framework.test_framework import NavCoinTestFramework
from test_framework.cfund_util import *

import time


class CommunityFundPaymentRequestStateTest(NavCoinTestFramework):
    """Tests the state transition of payment requests of the Community fund."""

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
        self.nodes[0].donatefund(10)

        # Create a proposal
        proposalid0 = self.nodes[0].createproposal(self.nodes[0].getnewaddress(), 10, 3600, "test")["hash"]
        start_new_cycle(self.nodes[0])

        # Accept the proposal
        self.nodes[0].proposalvote(proposalid0, "yes")
        start_new_cycle(self.nodes[0])

        # Proposal should be accepted
        assert (self.nodes[0].getproposal(proposalid0)["state"] == 1)
        assert (self.nodes[0].getproposal(proposalid0)["status"] == "accepted")

        # Create a payment request
        paymentrequestid0 = self.nodes[0].createpaymentrequest(proposalid0, 1, "test_payreq")["hash"]
        start_new_cycle(self.nodes[0])

        # Payment request initial state at beginning of cycle
        assert (self.nodes[0].getpaymentrequest(paymentrequestid0)["state"] == 0)
        assert (self.nodes[0].getpaymentrequest(paymentrequestid0)["status"] == "pending")

        # Move the necessary amount of cycles for the payment request to expire
        i = 0
        necessary = self.nodes[0].cfundstats()["consensus"]["maxCountVotingCyclePaymentRequests"]
        while i < necessary:
            start_new_cycle(self.nodes[0])
            i = i + 1

        # Validate that the payment request will expire at the end of the current voting cycle
        assert (self.nodes[0].getpaymentrequest(paymentrequestid0)["state"] == 0)
        assert (self.nodes[0].getpaymentrequest(paymentrequestid0)["status"] == "expired waiting for end of voting period")

        # Move to the last block of the voting cycle, where the payment request state changes
        end_cycle(self.nodes[0])

        # Validate that the payment request expired
        assert (self.nodes[0].getpaymentrequest(paymentrequestid0)["state"] == 3)
        assert (self.nodes[0].getpaymentrequest(paymentrequestid0)["status"] == "expired")


if __name__ == '__main__':
    CommunityFundPaymentRequestStateTest().main()
