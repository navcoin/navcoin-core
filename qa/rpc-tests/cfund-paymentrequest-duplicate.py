#!/usr/bin/env python3
# Copyright (c) 2018 The Navcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

from test_framework.test_framework import NavCoinTestFramework
from test_framework.cfund_util import *

import time

class CommunityFundPaymentRequestDuplicate(NavCoinTestFramework):
    """Tests the payment request procedures of the Community fund."""

    def __init__(self):
        super().__init__()
        self.setup_clean_chain = True
        self.num_nodes = 1

    def setup_network(self, split=False):
        self.nodes = self.setup_nodes()
        self.is_network_split = split

    def run_test(self):
        # self.nodes[0].staking(False)
        activate_cfund(self.nodes[0])
        self.nodes[0].donatefund(1000)

        paymentAddress = self.nodes[0].getnewaddress()

        # Create a proposal and accept by voting
        proposalid0 = self.nodes[0].createproposal(paymentAddress, 100, 36000, "test")["hash"]
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

        # Create 1 payment request

        paymentReq = self.nodes[0].createpaymentrequest(proposalid0, 20, "test0")["hash"]

        slow_gen(self.nodes[0], 1)

        assert(self.nodes[0].getpaymentrequest(paymentReq)["state"] == 0)
        assert(self.nodes[0].getpaymentrequest(paymentReq)["status"] == "pending")

        assert(self.nodes[0].cfundstats()["funds"]["locked"] == locked_accepted)

        print(self.nodes[0].getbalance())

        end_cycle(self.nodes[0])

        # vote yes for the payment request

        self.nodes[0].paymentrequestvote(paymentReq, "yes")

        slow_gen(self.nodes[0], 1)
        end_cycle(self.nodes[0])
        time.sleep(0.2)

        assert(self.nodes[0].getpaymentrequest(paymentReq)["state"] == 1)
        assert(self.nodes[0].getpaymentrequest(paymentReq)["paidOnBlock"] == "0000000000000000000000000000000000000000000000000000000000000000")

        print(self.nodes[0].getpaymentrequest(paymentReq))

        wallet_info1 = self.nodes[0].getwalletinfo()

        while self.nodes[0].getpaymentrequest(paymentReq)["paidOnBlock"] == "0000000000000000000000000000000000000000000000000000000000000000":
            slow_gen(self.nodes[0], 1)

        time.sleep(0.2)

        wallet_info2 = self.nodes[0].getwalletinfo()
        # print(self.nodes[0].getpaymentrequest(paymentReq))
        print(paymentAddress)

        payoutBlockHash = self.nodes[0].getpaymentrequest(paymentReq)["paidOnBlock"]
        payoutBlock = self.nodes[0].getblock(payoutBlockHash)

        print(self.nodes[0].getpaymentrequest(paymentReq)["paidOnBlock"])
        # print(self.nodes[0].getblockcount())

        payoutHex = self.nodes[0].getrawtransaction(payoutBlock["tx"][0])
        payoutTx = self.nodes[0].decoderawtransaction(payoutHex)

        # payout = self.nodes[0].gettransaction(payoutBlock["tx"][0])
        # print(payout)

        # print(payoutTx["vout"][0])
        # print(payoutTx["vout"][1])

        assert(payoutTx["vout"][1]["valueSat"] == 2000000000)
        assert(payoutTx["vout"][1]["scriptPubKey"]["addresses"][0] == paymentAddress)

        # print(wallet_info1)

        # print(wallet_info2)

        assert(wallet_info2['immature_balance'] == wallet_info1['immature_balance'] + 20)

        time.sleep(5)

        slow_gen(self.nodes[0], 1000)

        print(self.nodes[0].getpaymentrequest(paymentReq)["paidOnBlock"])

        print(self.nodes[0].getwalletinfo()["immature_balance"])

        print(self.nodes[0].getreceivedbyaddress(paymentAddress, 0))

        """
        blockcount = self.nodes[0].getblockcount()
        wallet_info1 = self.nodes[0].getwalletinfo()

        print(blockcount)

        # wait for a new block to be mined
        while self.nodes[0].getblockcount() == blockcount:
            print("waiting for a new block...")
            time.sleep(1)

        assert(blockcount != self.nodes[0].getblockcount())

        print(self.nodes[0].getblockcount())

        wallet_info2 = self.nodes[0].getwalletinfo()
        balance_diff = wallet_info1['balance'] - wallet_info2['balance']

        print(balance_diff)

        # check that only 2 new NAV were created
        # assert(wallet_info2['immature_balance'] == wallet_info1['immature_balance'] + balance_diff + 2)

        print(self.nodes[0].getbalance())
        print(paymentAddress)
        print(self.nodes[0].getreceivedbyaddress(paymentAddress, 0))
        """


if __name__ == '__main__':
    CommunityFundPaymentRequestDuplicate().main()
