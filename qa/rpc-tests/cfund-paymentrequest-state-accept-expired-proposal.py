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
        self.nodes[0].staking(False)

        proposal_duration = 3
        proposal_amount = 10

        # Create a proposal and accept by voting
        proposalid0 = self.nodes[0].createproposal(self.nodes[0].getnewaddress(), proposal_amount, proposal_duration, "test")["hash"]        
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

        # Create a payment request
        paymentrequestid0 = self.nodes[0].createpaymentrequest(proposalid0, 1, "test0")["hash"]
        slow_gen(self.nodes[0], 1)

        # Payment request initial state at beginning of cycle

        assert(self.nodes[0].getpaymentrequest(paymentrequestid0)["state"] == 0)
        assert(self.nodes[0].getpaymentrequest(paymentrequestid0)["status"] == "pending")
        assert(self.nodes[0].cfundstats()["funds"]["locked"] == locked_accepted)

        # Wait for the proposal to expire
        while int(time.time()) <= int(self.nodes[0].getproposal(proposalid0)["expiresOn"]):
            time.sleep(1)

        blocks=slow_gen(self.nodes[0], 1)

        assert(self.nodes[0].getproposal(proposalid0)["status"] == "expired waiting for end of voting period")

        self.nodes[0].invalidateblock(blocks[-1])
        assert(self.nodes[0].getproposal(proposalid0)["status"] == "accepted")

        slow_gen(self.nodes[0], 1)
        assert(self.nodes[0].getproposal(proposalid0)["status"] == "expired waiting for end of voting period")

        end_cycle(self.nodes[0])
        slow_gen(self.nodes[0], 1)

        # No more payment requests are allowed as the proposal deadline has passed
        payment_request_not_created = True
        try:
            paymentrequestid1 = self.nodes[0].createpaymentrequest(proposalid0, 2, "test1")["hash"]
            payment_request_not_created = False
        except JSONRPCException:
            pass
        assert(payment_request_not_created)

        # Vote enough yes votes, without enough quorum

        total_votes = self.nodes[0].cfundstats()["consensus"]["minSumVotesPerVotingCycle"]
        min_yes_votes = self.nodes[0].cfundstats()["consensus"]["votesAcceptPaymentRequestPercentage"]/100
        yes_votes = int(total_votes * min_yes_votes) + 1

        self.nodes[0].paymentrequestvote(paymentrequestid0, "yes")
        slow_gen(self.nodes[0], yes_votes)
        self.nodes[0].paymentrequestvote(paymentrequestid0, "no")
        slow_gen(self.nodes[0], total_votes - yes_votes)
        self.nodes[0].paymentrequestvote(paymentrequestid0, "remove")

        # Should still be in pending

        assert(self.nodes[0].getpaymentrequest(paymentrequestid0)["state"] == 0)
        assert(self.nodes[0].getpaymentrequest(paymentrequestid0)["status"] == "pending")
        assert(self.nodes[0].cfundstats()["funds"]["locked"] == locked_accepted)

        end_cycle(self.nodes[0])
        time.sleep(0.2)

        # Payment request initial state at beginning of cycle

        assert(self.nodes[0].getpaymentrequest(paymentrequestid0)["state"] == 0)
        assert(self.nodes[0].getpaymentrequest(paymentrequestid0)["status"] == "pending")
        assert(self.nodes[0].cfundstats()["funds"]["locked"] == locked_accepted)

        # Vote enough quorum, but not enough positive votes

        total_votes = self.nodes[0].cfundstats()["consensus"]["minSumVotesPerVotingCycle"] + 1
        yes_votes = int(total_votes * min_yes_votes)

        self.nodes[0].paymentrequestvote(paymentrequestid0, "yes")
        slow_gen(self.nodes[0], yes_votes)
        self.nodes[0].paymentrequestvote(paymentrequestid0, "no")
        slow_gen(self.nodes[0], total_votes - yes_votes)
        self.nodes[0].paymentrequestvote(paymentrequestid0, "remove")

        assert(self.nodes[0].getpaymentrequest(paymentrequestid0)["state"] == 0)
        assert(self.nodes[0].getpaymentrequest(paymentrequestid0)["status"] == "pending")
        assert(self.nodes[0].cfundstats()["funds"]["locked"] == locked_accepted)

        end_cycle(self.nodes[0])
        time.sleep(0.2)

        # Payment request initial state at beginning of cycle

        assert(self.nodes[0].getpaymentrequest(paymentrequestid0)["state"] == 0)
        assert(self.nodes[0].getpaymentrequest(paymentrequestid0)["status"] == "pending")
        assert(self.nodes[0].cfundstats()["funds"]["locked"] == locked_accepted)

        assert(self.nodes[0].getproposal(proposalid0)["status"] == "expired pending voting of payment requests")

        # Vote enough quorum and enough positive votes

        total_votes = self.nodes[0].cfundstats()["consensus"]["minSumVotesPerVotingCycle"] + 1
        yes_votes = int(total_votes * min_yes_votes) + 1

        self.nodes[0].paymentrequestvote(paymentrequestid0, "yes")
        slow_gen(self.nodes[0], yes_votes)
        self.nodes[0].paymentrequestvote(paymentrequestid0, "no")
        blocks = slow_gen(self.nodes[0], total_votes - yes_votes)
        self.nodes[0].paymentrequestvote(paymentrequestid0, "remove")

        assert(self.nodes[0].getpaymentrequest(paymentrequestid0)["state"] == 0)
        assert(self.nodes[0].getpaymentrequest(paymentrequestid0)["status"] == "accepted waiting for end of voting period")
        assert(self.nodes[0].cfundstats()["funds"]["locked"] == locked_accepted)

        time.sleep(0.2)

        # Revert last vote and check status

        self.nodes[0].invalidateblock(blocks[-1])
        assert(self.nodes[0].getpaymentrequest(paymentrequestid0)["state"] == 0)
        assert(self.nodes[0].getpaymentrequest(paymentrequestid0)["status"] == "pending")
        assert(self.nodes[0].cfundstats()["funds"]["locked"] == locked_accepted)
        self.nodes[0].cfundstats()

        # Vote again

        self.nodes[0].paymentrequestvote(paymentrequestid0, "yes")
        slow_gen(self.nodes[0], 1)
        self.nodes[0].paymentrequestvote(paymentrequestid0, "remove")


        # Move to a new cycle...
        time.sleep(0.2)

        end_cycle(self.nodes[0])
        blocks = slow_gen(self.nodes[0], 1)
        locked_after_payment = float(locked_accepted) - float(self.nodes[0].getpaymentrequest(paymentrequestid0)["requestedAmount"])

        # Paymentrequest must be accepted now
        assert(self.nodes[0].getpaymentrequest(paymentrequestid0)["state"] == 1)
        assert(self.nodes[0].getpaymentrequest(paymentrequestid0)["status"] == "accepted")
        # Locked amount should be 0, as this was the only payment request and the proposal was expired
        assert(self.nodes[0].cfundstats()["funds"]["locked"] == 0)
        assert(self.nodes[0].getproposal(proposalid0)["status"] == "expired")

        # No more payment requests are allowed as the proposal is expired
        payment_request_not_created = True
        try:
            paymentrequestid2 = self.nodes[0].createpaymentrequest(proposalid0, 2, "test2")["hash"]
            payment_request_not_created = False
        except JSONRPCException:
            pass
        assert(payment_request_not_created)

if __name__ == '__main__':
    CommunityFundPaymentRequestsTest().main()
