#!/usr/bin/env python3
# Copyright (c) 2018 The Navcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

from test_framework.test_framework import NavCoinTestFramework
from test_framework.cfund_util import *

import time

class CommunityFundPaymentRequestExtractFundsTest(NavCoinTestFramework):
    """Tests the payment request procedures of the Community fund."""

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

        # Create a proposal and accept by voting
        proposalid0 = self.nodes[0].createproposal(self.nodes[0].getnewaddress(), 100, 36000, "test")["hash"]
        locked_before = self.nodes[0].cfundstats()["funds"]["locked"]
        end_cycle(self.nodes[0])

        time.sleep(0.2)

        self.nodes[0].proposalvote(proposalid0, "yes")
        slow_gen(self.nodes[0], 1)
        end_cycle(self.nodes[0])
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

        slow_gen(self.nodes[0], 1)

        # One of them should have been rejected at creation
        valid = 0
        invalid = 0
        invalid_pos = -1

        for paymentReq in paymentRequests:
            try:
                assert(self.nodes[0].getpaymentrequest(paymentReq)["state"] == 0)
                assert(self.nodes[0].getpaymentrequest(paymentReq)["status"] == "pending")
                valid = valid + 1
            except JSONRPCException:
                invalid = invalid + 1
                invalid_pos = valid 
                continue

        assert(valid == 5)
        assert(invalid == 1)

        paymentRequests.pop(invalid_pos)

        assert(self.nodes[0].cfundstats()["funds"]["locked"] == locked_accepted)
        
        end_cycle(self.nodes[0])

        # Lets reject one of them with votes
        preqid = paymentRequests.pop(0)
        self.nodes[0].paymentrequestvote(preqid, "no")

        slow_gen(self.nodes[0], 1)
        end_cycle(self.nodes[0])

        assert(self.nodes[0].getpaymentrequest(preqid)["state"] == 2)
        assert(self.nodes[0].getpaymentrequest(preqid)["status"] == "rejected")

        # Add a new payment request
        paymentRequests.append(self.nodes[0].createpaymentrequest(proposalid0, 20, "test0")["hash"])
        slow_gen(self.nodes[0], 1)

        for paymentReq in paymentRequests:
            assert(self.nodes[0].getpaymentrequest(paymentReq)["state"] == 0)
            assert(self.nodes[0].getpaymentrequest(paymentReq)["status"] == "pending")

        # vote for all payment requests
        for paymentReq in paymentRequests:
            self.nodes[0].paymentrequestvote(paymentReq, "yes")

        slow_gen(self.nodes[0], 1)
        end_cycle(self.nodes[0])
        time.sleep(0.2)

        slow_gen(self.nodes[0], 1)
        end_cycle(self.nodes[0])
        time.sleep(0.2)

        # the 5 payment requests should be accepted
        allAccepted = True
        for paymentReq in paymentRequests:
            if self.nodes[0].getpaymentrequest(paymentReq)["state"] != 1:
                allAccepted = False

        # all the payment requests should have been validated
        assert(allAccepted is True)


if __name__ == '__main__':
    CommunityFundPaymentRequestExtractFundsTest().main()
