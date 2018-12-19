#!/usr/bin/env python3
# Copyright (c) 2018 The Navcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

from test_framework.test_framework import NavCoinTestFramework
from test_framework.cfund_util import *

import time

class CommunityFundPaymentRequestVoteListTest(NavCoinTestFramework):
    """Tests the paymentrequestvotelist function of the Community fund."""

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

        address0 = self.nodes[0].getnewaddress()

        # Set up 2 proposals
        proposalid0 = self.nodes[0].createproposal(address0, 100, 3600, "test0")["hash"]
        proposalid1 = self.nodes[0].createproposal(address0, 100, 3600, "test1")["hash"]
        start_new_cycle(self.nodes[0])

        # Vote yes and pass proposals
        self.nodes[0].proposalvote(proposalid0, "yes")
        self.nodes[0].proposalvote(proposalid1, "yes")
        start_new_cycle(self.nodes[0])

        # Preflight checks
        assert(len(self.nodes[0].paymentrequestvotelist()["yes"]) == 0)
        assert(len(self.nodes[0].paymentrequestvotelist()["no"]) == 0)
        assert(len(self.nodes[0].paymentrequestvotelist()["null"]) == 0)

        # Create payment requests
        preqid0_a = self.nodes[0].createpaymentrequest(proposalid0, 1, "preq0_a")["hash"]
        preqid0_b = self.nodes[0].createpaymentrequest(proposalid0, 1, "preq0_b")["hash"]
        preqid1_c = self.nodes[0].createpaymentrequest(proposalid1, 10, "preq1_c")["hash"]
        slow_gen(self.nodes[0], 1)

        # Verify the payment requests are now in the payment request vote list
        assert(len(self.nodes[0].paymentrequestvotelist()["yes"]) == 0)
        assert(len(self.nodes[0].paymentrequestvotelist()["no"]) == 0)
        assert(len(self.nodes[0].paymentrequestvotelist()["null"]) == 3)

        # Vote on the payment requests as wished
        self.nodes[0].paymentrequestvote(preqid0_a, "yes")
        self.nodes[0].paymentrequestvote(preqid0_b, "no")
        self.nodes[0].paymentrequestvote(preqid1_c, "yes")
        self.nodes[0].paymentrequestvote(preqid1_c, "remove")

        # Verify the payment request vote list has changed according to the wallet's votes
        assert(len(self.nodes[0].paymentrequestvotelist()["yes"]) == 1)
        assert(len(self.nodes[0].paymentrequestvotelist()["no"]) == 1)
        assert(len(self.nodes[0].paymentrequestvotelist()["null"]) == 1)

        # Verify the hashes are contained in the output vote list
        assert(preqid0_a == self.nodes[0].paymentrequestvotelist()["yes"][0]["hash"])
        assert(preqid0_b == self.nodes[0].paymentrequestvotelist()["no"][0]["hash"])
        assert(preqid1_c == self.nodes[0].paymentrequestvotelist()["null"][0]["hash"])

        # Revote differently
        self.nodes[0].paymentrequestvote(preqid0_a, "no")
        self.nodes[0].paymentrequestvote(preqid0_b, "yes")
        self.nodes[0].paymentrequestvote(preqid1_c, "no")

        # Verify the payment request vote list has changed according to the wallet's votes
        assert(len(self.nodes[0].paymentrequestvotelist()["yes"]) == 1)
        assert(len(self.nodes[0].paymentrequestvotelist()["no"]) == 2)
        assert(len(self.nodes[0].paymentrequestvotelist()["null"]) == 0)


        # Create new payment request
        preq0_d = self.nodes[0].createpaymentrequest(proposalid0, 1, "preq0_d")["hash"]
        slow_gen(self.nodes[0], 1)

        # Check the new payment request has been added to "null" of payment request vote list
        assert(len(self.nodes[0].paymentrequestvotelist()["null"]) == 1)
        assert(preq0_d == self.nodes[0].paymentrequestvotelist()["null"][0]["hash"])


if __name__ == '__main__':
    CommunityFundPaymentRequestVoteListTest().main()
