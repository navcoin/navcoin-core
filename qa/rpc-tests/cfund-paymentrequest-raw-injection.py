#!/usr/bin/env python3
# Copyright (c) 2018 The Navcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

from test_framework.test_framework import NavCoinTestFramework
from test_framework.util import *

import time
import json


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
        self.nodes[0].donatefund(100)

        # Get address
        address = self.nodes[0].getnewaddress()

        # Create a proposal
        proposalid0 = self.nodes[0].createproposal(address, 10, 3600, "test")["hash"]
        self.start_new_cycle()

        # Accept the proposal
        self.nodes[0].proposalvote(proposalid0, "yes")
        self.start_new_cycle()

        # Proposal should be accepted
        assert (self.nodes[0].getproposal(proposalid0)["state"] == 1)
        assert (self.nodes[0].getproposal(proposalid0)["status"] == "accepted")

        # see if we can send a raw negative payment request
        try:
            # Create negative raw payment request and increase unpaid amount
            self.send_raw_paymentrequest(-1000, address, proposalid0, "test1")
        except JSONRPCException as e:
            assert("bad-cfund-payment-request" in e.error['message'])


        # Generate some
        self.slow_gen(1)

        # look for the payment request - should not exist
        paymentrequestid0 = ""
        try:
            paymentrequestid0 = self.nodes[0].listproposals()[0]['paymentRequests'][0]['hash']
        except Exception as e:
           assert(paymentrequestid0 == "")

        # Accept payment request
        self.start_new_cycle()

        # # Verify nothing changed
        assert (float(self.nodes[0].getproposal(proposalid0)["notPaidYet"]) == 10)
        assert (float(self.nodes[0].cfundstats()["funds"]["locked"]) == 10)


        # Create new payment request for more than the amount
        paymentrequestid1 = ""
        try:
            paymentrequestid1 = self.nodes[0].createpaymentrequest(proposalid0, 1000, "payreq1")["hash"]
        except Exception as e:
            assert(paymentrequestid1 == "")


        # self.slow_gen(1)
        #
        # # Vote yes
        # self.nodes[0].paymentrequestvote(paymentrequestid1, "yes")
        #
        # # Start new cycle
        # self.start_new_cycle()
        # for paymentrequest in self.nodes[0].listproposals()[0]['paymentRequests']:
        #     self.nodes[0].paymentrequestvote(paymentrequest['hash'], "remove")
        #
        # # Wait for maturing
        # self.slow_gen(20)
        #
        # # Pay out the positive payment request
        # raw_payment_tx = self.nodes[0].createrawtransaction(
        #     [],
        #     {address: 1000},
        #     "", 0
        # )
        # self.nodes[0].coinbaseoutputs([raw_payment_tx])
        # self.nodes[0].setcoinbasestrdzeel(json.dumps([paymentrequestid1]))
        #
        # # Generate block and receive payment
        # payblockhash = self.nodes[0].generate(1)[0]
        #
        # # Verify it was paid out
        # assert (self.nodes[0].getpaymentrequest(paymentrequestid1)["paidOnBlock"] == payblockhash)

    def send_raw_paymentrequest(self, amount, address, proposal_hash, description):
        amount = amount * 100000000
        privkey = self.nodes[0].dumpprivkey(address)
        message = "I kindly ask to withdraw " + str(amount) + "NAV from the proposal " + proposal_hash + ". Payment request id: " + description
        signature = self.nodes[0].signmessagewithprivkey(privkey, message)

        # Create a raw payment request
        raw_payreq_tx = self.nodes[0].createrawtransaction(
            [],
            {"6ac1": 1},
            json.dumps({"v": 2, "h": proposal_hash, "n": amount, "s": signature, "i": description})
        )

        # Modify version for payreq creation
        raw_payreq_tx = "05" + raw_payreq_tx[2:]

        # Fund raw transacion
        raw_payreq_tx = self.nodes[0].fundrawtransaction(raw_payreq_tx)['hex']

        # Sign raw transacion
        raw_payreq_tx = self.nodes[0].signrawtransaction(raw_payreq_tx)['hex']

        # Send raw transacion
        self.nodes[0].sendrawtransaction(raw_payreq_tx)

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
