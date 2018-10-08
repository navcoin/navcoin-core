#!/usr/bin/env python3
# Copyright (c) 2018 The Navcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

from test_framework.test_framework import NavCoinTestFramework
from test_framework.util import *

import time

class CommunityFundPaymentRequestExtractFundsTest(NavCoinTestFramework):
    """Tests the payment request procedures of the Community fund."""

    def __init__(self):
        super().__init__()
        self.setup_clean_chain = True
        self.num_nodes = 1

    def setup_network(self, split=False):
        self.nodes = start_nodes(self.num_nodes, self.options.tmpdir)
        self.is_network_split = False

    def run_test(self):
        self.nodes[0].staking(False)
        self.slow_gen(300)
        self.nodes[0].donatefund(1000)

        # Create a proposal and accept by voting
        proposalid0 = self.nodes[0].createproposal(self.nodes[0].getnewaddress(), 100, 36000, "test")["hash"]
        locked_before = self.nodes[0].cfundstats()["funds"]["locked"]
        self.start_new_cycle()

        time.sleep(0.2)

        self.nodes[0].proposalvote(proposalid0, "yes")
        self.slow_gen(1)
        self.start_new_cycle()
        locked_accepted = self.nodes[0].cfundstats()["funds"]["locked"]

        time.sleep(0.2)

        # Proposal should be accepted

        assert(self.nodes[0].getproposal(proposalid0)["state"] == 1)
        assert(self.nodes[0].getproposal(proposalid0)["status"] == "accepted")
        assert(self.nodes[0].cfundstats()["funds"]["locked"] == float(locked_before) + float(self.nodes[0].getproposal(proposalid0)["requestedAmount"]))

        # Create 6 payment requests
        paymentRequests = []
        for x in range(6):
            paymentRequests.append(self.nodes[0].createpaymentrequest(proposalid0, 20, "test0")["hash"])

        self.slow_gen(1)

        for paymentReq in paymentRequests:
            assert(self.nodes[0].getpaymentrequest(paymentReq)["state"] == 0)
            assert(self.nodes[0].getpaymentrequest(paymentReq)["status"] == "pending")
            assert(self.nodes[0].cfundstats()["funds"]["locked"] == locked_accepted)

        self.start_new_cycle()

        # vote for all payment requests
        for paymentReq in paymentRequests:
            self.nodes[0].paymentrequestvote(paymentReq, "yes")

        self.slow_gen(1)
        self.start_new_cycle()
        time.sleep(0.2)

        self.slow_gen(1)
        self.start_new_cycle()
        time.sleep(0.2)

        # only 5 of the 6 payment requests should be accepted
        allAccepeted = True
        for paymentReq in paymentRequests:
            if self.nodes[0].getpaymentrequest(paymentReq)["state"] != 1:
                allAccepeted = False

        # not all the payment requests should have been validated
        assert(allAccepeted is False)


        # create a new propsal and test for the funds extraction edge case described
        # https://github.com/NAVCoin/navcoin-core/issues/307
        proposalid1 = self.nodes[0].createproposal(self.nodes[0].getnewaddress(), 100, 36000, "test")["hash"]
        locked_before = self.nodes[0].cfundstats()["funds"]["locked"]
        self.slow_gen(1)

        self.nodes[0].proposalvote(proposalid1, "yes")
        self.slow_gen(1)
        self.start_new_cycle()
        locked_accepted = self.nodes[0].cfundstats()["funds"]["locked"]


        assert(self.nodes[0].getproposal(proposalid1)["state"] == 1)
        assert(self.nodes[0].getproposal(proposalid1)["status"] == "accepted")
        assert(self.nodes[0].cfundstats()["funds"]["locked"] == float(locked_before) + float(self.nodes[0].getproposal(proposalid1)["requestedAmount"]))


        self.slow_gen(400)
        self.start_new_cycle()

        # only 5 of the 6 payment requests should be accepted
        allAccepeted = True
        for paymentReq in paymentRequests:
            if self.nodes[0].getpaymentrequest(paymentReq)["state"] != 1:
                allAccepeted = False

        # not all the payment requests should have been validated
        assert(allAccepeted is False)



    def start_new_cycle(self):
        # Move to the end of the cycle
        self.slow_gen(self.nodes[0].cfundstats()["votingPeriod"]["ending"] - self.nodes[0].cfundstats()["votingPeriod"]["current"])


    def slow_gen(self, count):
        total = count
        blocks = []
        while total > 0:
            now = min(total, 10)
            blocks.extend(self.nodes[0].generate(now))
            total -= now
            time.sleep(0.1)
        return blocks

if __name__ == '__main__':
    CommunityFundPaymentRequestExtractFundsTest().main()
