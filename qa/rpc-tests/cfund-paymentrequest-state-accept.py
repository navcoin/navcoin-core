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

        # Create a proposal and accept by voting
        proposalid0 = self.nodes[0].createproposal(self.nodes[0].getnewaddress(), 10, 3600, "test")["hash"]        
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
        assert(self.nodes[0].cfundstats()["funds"]["locked"] == locked_after_payment)


        # Create multiple payment requests

        paymentrequestid1 = self.nodes[0].createpaymentrequest(proposalid0, 4, "test1")["hash"]
        paymentrequestid2 = self.nodes[0].createpaymentrequest(proposalid0, 4, "test2")["hash"]
        slow_gen(self.nodes[0], 1)

        assert(self.nodes[0].getpaymentrequest(paymentrequestid1)["state"] == 0)
        assert(self.nodes[0].getpaymentrequest(paymentrequestid1)["status"] == "pending")
        assert(self.nodes[0].getpaymentrequest(paymentrequestid2)["state"] == 0)
        assert(self.nodes[0].getpaymentrequest(paymentrequestid2)["status"] == "pending")

        self.nodes[0].paymentrequestvote(paymentrequestid1, "yes")
        self.nodes[0].paymentrequestvote(paymentrequestid2, "yes")
        slow_gen(self.nodes[0], yes_votes)
        self.nodes[0].paymentrequestvote(paymentrequestid1, "no")
        self.nodes[0].paymentrequestvote(paymentrequestid2, "no")
        blocks = slow_gen(self.nodes[0], total_votes - yes_votes)
        self.nodes[0].paymentrequestvote(paymentrequestid1, "remove")
        self.nodes[0].paymentrequestvote(paymentrequestid2, "remove")

        assert(self.nodes[0].getpaymentrequest(paymentrequestid1)["state"] == 0)
        assert(self.nodes[0].getpaymentrequest(paymentrequestid1)["status"] == "accepted waiting for end of voting period")
        assert(self.nodes[0].getpaymentrequest(paymentrequestid2)["state"] == 0)
        assert(self.nodes[0].getpaymentrequest(paymentrequestid2)["status"] == "accepted waiting for end of voting period")

        time.sleep(0.2)

        end_cycle(self.nodes[0])
        blocks = slow_gen(self.nodes[0], 1)

        # Check status after acceptance

        assert(self.nodes[0].getpaymentrequest(paymentrequestid1)["state"] == 1)
        assert(self.nodes[0].getpaymentrequest(paymentrequestid1)["status"] == "accepted")
        assert(self.nodes[0].getpaymentrequest(paymentrequestid2)["state"] == 1)
        assert(self.nodes[0].getpaymentrequest(paymentrequestid2)["status"] == "accepted")
        locked_after_2nd_payment = (locked_after_payment -
                                    float(self.nodes[0].getpaymentrequest(paymentrequestid1)["requestedAmount"]) -
                                    float(self.nodes[0].getpaymentrequest(paymentrequestid2)["requestedAmount"]))
        assert(self.nodes[0].cfundstats()["funds"]["locked"] == locked_after_2nd_payment)


        # Create and vote on payment request more than the total proposal amount, must throw "JSONRPC error: Invalid amount."

        try:
            paymentrequestid3 = self.nodes[0].createpaymentrequest(proposalid0, 2, "test3")["hash"]
        except JSONRPCException:
            pass


if __name__ == '__main__':
    CommunityFundPaymentRequestsTest().main()
