#!/usr/bin/env python3
# Copyright (c) 2018 The Navcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

from test_framework.test_framework import NavCoinTestFramework
from test_framework.cfund_util import *

import time

class CommunityFundPaymentRequestsTest(NavCoinTestFramework):
    """Tests the payment request procedures of the Community fund."""

    def __init__(self):
        super().__init__()
        self.setup_clean_chain = True
        self.num_nodes = 1

    def setup_network(self, split=False):
        self.nodes = self.setup_nodes()
        self.is_network_split = split

    def run_test(self):
        activate_cfund(self.nodes[0])
        self.nodes[0].donatefund(100)

        # Create a proposal and accept it
        proposal_one_hash = self.nodes[0].createproposal(self.nodes[0].getnewaddress(), 10, 60, "test proposal")["hash"]        
        locked_before = self.nodes[0].cfundstats()["funds"]["locked"]
        end_cycle(self.nodes[0])

        time.sleep(0.2)

        self.nodes[0].proposalvote(proposal_one_hash, "yes")
        slow_gen(self.nodes[0], 1)
        end_cycle(self.nodes[0])
        locked_accepted = self.nodes[0].cfundstats()["funds"]["locked"]

        time.sleep(0.2)

        # Proposal should be accepted

        assert(self.nodes[0].getproposal(proposal_one_hash)["state"] == 1)
        assert(self.nodes[0].getproposal(proposal_one_hash)["status"] == "accepted")
        assert(float(locked_accepted) == float(locked_before) + float(self.nodes[0].getproposal(proposal_one_hash)["requestedAmount"]))

        # Create a payment request
        payment_request_one_hash = self.nodes[0].createpaymentrequest(proposal_one_hash, 1, "test payment request")["hash"]
        slow_gen(self.nodes[0], 1)

        # Payment request initial state at beginning of cycle

        assert(self.nodes[0].getpaymentrequest(payment_request_one_hash)["state"] == 0)
        assert(self.nodes[0].getpaymentrequest(payment_request_one_hash)["status"] == "pending")
        assert(self.nodes[0].cfundstats()["funds"]["locked"] == locked_accepted)

        # Wait for proposal to expire
        while int(time.time()) <= int(self.nodes[0].getproposal(proposal_one_hash)["expiresOn"]):
          time.sleep(1)

        slow_gen(self.nodes[0], 1)
        end_cycle(self.nodes[0])

        # Check the state of the proposal is expired waiting for payment request (aka pending)
        print("proposal status", self.nodes[0].getproposal(proposal_one_hash)["status"])
        print("payment request status", self.nodes[0].getpaymentrequest(payment_request_one_hash)["status"])
        print("cfund stats", self.nodes[0].cfundstats())
        assert(self.nodes[0].getproposal(proposal_one_hash)["status"] == "pending")


        # self.nodes[0].paymentrequestvote(payment_request_one_hash, "yes")
        # end_cycle(self.nodes[0])

        # assert(self.nodes[0].getpaymentrequest(payment_request_one_hash)["state"] == 0)
        # assert(self.nodes[0].getpaymentrequest(payment_request_one_hash)["status"] == "")
        # assert(self.nodes[0].cfundstats()["funds"]["locked"] == locked_accepted)

        time.sleep(0.2)


if __name__ == '__main__':
    CommunityFundPaymentRequestsTest().main()
