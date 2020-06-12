#!/usr/bin/env python3
# Copyright (c) 2018 The Navcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

from test_framework.test_framework import NavCoinTestFramework
from test_framework.hardfork_util import *
from test_framework.cfund_util import *

class PaymentRequest452(NavCoinTestFramework):
    """Tests whether payment requests can be double paid before and after the hardfork."""

    def __init__(self):
        super().__init__()
        self.setup_clean_chain = True
        self.num_nodes = 1

    def setup_network(self, split=False):
        self.nodes = self.setup_nodes()
        self.is_network_split = split

    def run_test(self):
        activate_cfund(self.nodes[0])
        self.nodes[0].donatefund(100000)
        self.nodes[0].staking(False)

        payoutTxID_1 = self.create_double_preq_payout()
        print("Second payment request payout:\n"+str(self.nodes[0].gettxout(payoutTxID_1, 1)))

        # If the payment request is paid out again, the coinbase tx will have 2 vouts, so check txout at index 1
        assert(self.nodes[0].gettxout(payoutTxID_1, 1) != None)


        activateHardFork(self.nodes[0], 21, 1000)


        payoutTxID_2 = self.create_double_preq_payout()
        print("Second payment request payout:\n"+str(self.nodes[0].gettxout(payoutTxID_2, 1)))


        # If the payment request is paid out again, the coinbase tx will have 2 vouts, so check txout at index 1
        assert(self.nodes[0].gettxout(payoutTxID_2, 1) == None)


    def create_double_preq_payout(self):
        # Creates a proposal and payment request that is paid out twice
        ## Returns the txid of the second payout
        paymentAddress = self.nodes[0].getnewaddress()
        proposalAmount = 10000
        payoutAmount = 3333

        # Create a proposal and accept by voting

        proposalid0 = self.nodes[0].createproposal(paymentAddress, proposalAmount, 36000, "test")["hash"]
        start_new_cycle(self.nodes[0])

        self.nodes[0].proposalvote(proposalid0, "yes")

        start_new_cycle(self.nodes[0])

        # Proposal should be accepted
        assert(self.nodes[0].getproposal(proposalid0)["state"] == 1)
        assert(self.nodes[0].getproposal(proposalid0)["status"] == "accepted")

        # Create 1 payment request

        paymentReq = self.nodes[0].createpaymentrequest(proposalid0, payoutAmount, "test0")["hash"]

        slow_gen(self.nodes[0], 1)

        # vote yes for the payment request and generate until paid out

        end_cycle(self.nodes[0])

        self.nodes[0].paymentrequestvote(paymentReq, "yes")

        slow_gen(self.nodes[0], 1)
        start_new_cycle(self.nodes[0])

        assert(self.nodes[0].getpaymentrequest(paymentReq)["state"] == 1)
        assert(self.nodes[0].getpaymentrequest(paymentReq)["stateChangedOnBlock"] != "0000000000000000000000000000000000000000000000000000000000000000")

        while self.nodes[0].getpaymentrequest(paymentReq)["state"] != 6:
            blocks = slow_gen(self.nodes[0], 1)

        slow_gen(self.nodes[0], 1)


        # try to payout accepted payment request a second time
        rawOutput = self.nodes[0].createrawtransaction([], {paymentAddress: payoutAmount}, "", 0)

        self.nodes[0].coinbaseoutputs([rawOutput])
        self.nodes[0].setcoinbasestrdzeel('[\"'+paymentReq+'\"]')

        payoutBlockHash = slow_gen(self.nodes[0], 1)[0]

        payoutTxID = self.nodes[0].getblock(payoutBlockHash)["tx"][0]

        return payoutTxID


if __name__ == '__main__':
    PaymentRequest452().main()
