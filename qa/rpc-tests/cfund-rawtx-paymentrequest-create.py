#!/usr/bin/env python3
# Copyright (c) 2018 The Navcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

from test_framework.test_framework import NavCoinTestFramework
from test_framework.cfund_util import *

import json


class CommunityFundCreatePaymentrequestRawTX(NavCoinTestFramework):
    """Tests the state transition of payment requests of the Community fund."""

    def __init__(self):
        super().__init__()
        self.setup_clean_chain = True
        self.num_nodes = 1

        self.goodDescription = "good_payreq"
        self.goodAddress = ""
        self.goodAmount = 1
        self.goodProposalHash = ""
        self.goodPayreqHash = ""

    def setup_network(self, split=False):
        self.nodes = self.setup_nodes()
        self.is_network_split = split

    def run_test(self):
        self.nodes[0].staking(False)
        activate_cfund(self.nodes[0])
        self.nodes[0].donatefund(100)

        # Get address
        self.goodAddress = self.nodes[0].getnewaddress()

        # Create a proposal
        self.goodProposalHash = self.nodes[0].createproposal(self.goodAddress, 10, 3600, "test")["hash"]
        start_new_cycle(self.nodes[0])

        # Accept the proposal
        self.nodes[0].proposalvote(self.goodProposalHash, "yes")
        start_new_cycle(self.nodes[0])

        # Proposal should be accepted
        assert (self.nodes[0].getproposal(self.goodProposalHash)["state"] == 1)
        assert (self.nodes[0].getproposal(self.goodProposalHash)["status"] == "accepted")

        # Create a good payment request
        self.goodPayreqHash = self.send_raw_paymentrequest(self.goodAmount, self.goodAddress, self.goodProposalHash, self.goodDescription)

        # Check good payment request is correct

        blocks = slow_gen(self.nodes[0], 1)
        goodPaymentRequest = self.nodes[0].listproposals()[0]['paymentRequests'][0]

        # The proposal should have all the same required fields
        assert (goodPaymentRequest['blockHash'] == blocks[0])
        self.check_good_paymentrequest(goodPaymentRequest)


        # Create payment request with negative amount
        try:
            self.send_raw_paymentrequest(-10, self.goodAddress, self.goodProposalHash, "negative_amount")
            raise ValueError("Error should be thrown for invalid rawtx (prequest)")
        except JSONRPCException as e:
            assert("bad-cfund-payment-request" in e.error['message'])

        # Create payment request with amount too large
        try:
            self.send_raw_paymentrequest(1000, self.goodAddress, self.goodProposalHash, "too_large_amount")
            raise ValueError("Error should be thrown for invalid rawtx (prequest)")
        except JSONRPCException as e:
            assert("bad-cfund-payment-request" in e.error['message'])

        # Create payment request with boolean description
        try:
            self.send_raw_paymentrequest(1, self.goodAddress, self.goodProposalHash, True)
            raise ValueError("Error should be thrown for invalid rawtx (prequest)")
        except JSONRPCException as e:
            assert ("bad-cfund-payment-request" in e.error['message'])

#        # Create payment request with string amount
#        try:
#            self.send_raw_paymentrequest('1', self.goodAddress, self.goodProposalHash, 'string_amount')
#        except JSONRPCException as e:
#            assert ("bad-cfund-payment-request" in e.error['message'])

        # Generate block
        slow_gen(self.nodes[0], 1)

        # Check there exists only 1 good payment request
        assert (len(self.nodes[0].listproposals()[0]['paymentRequests']) == 1)

        start_new_cycle(self.nodes[0])

        # # Verify nothing changed
        assert (float(self.nodes[0].getproposal(self.goodProposalHash)["notPaidYet"]) == 10)
        assert (float(self.nodes[0].cfundstats()["funds"]["locked"]) == 10)

        # Accept payment request
        self.nodes[0].paymentrequestvote(self.goodPayreqHash, "yes")
        start_new_cycle(self.nodes[0])
        slow_gen(self.nodes[0], 10)

        # Check the payment request is paid out
        assert (float(self.nodes[0].getproposal(self.goodProposalHash)["notPaidYet"]) == 9)
        assert (float(self.nodes[0].cfundstats()["funds"]["locked"]) == 9)


        # Create multiple payment requests
        self.send_raw_paymentrequest(6, self.goodAddress, self.goodProposalHash, "sum_to_too_large_amount0")
        self.send_raw_paymentrequest(6, self.goodAddress, self.goodProposalHash, "sum_to_too_large_amount1")
        self.send_raw_paymentrequest(6, self.goodAddress, self.goodProposalHash, "sum_to_too_large_amount2")

        slow_gen(self.nodes[0], 1)

        # Check there exists only 1 + 1 good payment request
        assert (len(self.nodes[0].listproposals()[0]['paymentRequests']) == 2)


        # Create payment request to somebody else's address

        address_3rd_party = self.nodes[0].getnewaddress()
        try:
            self.send_raw_paymentrequest(1, address_3rd_party, self.goodProposalHash, "3rd_party_address")
            raise ValueError("Error should be thrown for invalid rawtx (prequest)")
        except JSONRPCException as e:
            assert("bad-cfund-payment-request" in e.error['message'])


        # Create and test for expired and rejected proposals

        proposalid1_rejected = self.nodes[0].createproposal(self.goodAddress, 10, 3600, "test_rejected")["hash"]
        proposalid2_expired_timeout = self.nodes[0].createproposal(self.goodAddress, 10, 1, "test_expired_timeout")["hash"]
        proposalid3_expired_no_votes = self.nodes[0].createproposal(self.goodAddress, 10, 3600, "test_expired_no_votes")["hash"]
        start_new_cycle(self.nodes[0])

        # Reject Proposal 1 and accept Proposal 2
        self.nodes[0].proposalvote(proposalid1_rejected, "no")
        self.nodes[0].proposalvote(proposalid2_expired_timeout, "yes")
        start_new_cycle(self.nodes[0])

        # Proposal 1 should be rejected
        assert (self.nodes[0].getproposal(proposalid1_rejected)["state"] == 2)
        assert (self.nodes[0].getproposal(proposalid1_rejected)["status"] == "rejected")

        # Create a payment request for a rejected proposal
        try:
            self.send_raw_paymentrequest(1, self.goodAddress, proposalid1_rejected, "rejected_payreq")
            raise ValueError("Error should be thrown for invalid rawtx (prequest)")
        except JSONRPCException as e:
            assert ("bad-cfund-payment-request" in e.error['message'])

        time.sleep(1)
        slow_gen(self.nodes[0], 10)

        # Proposal 2 should be expired
        assert (self.nodes[0].getproposal(proposalid2_expired_timeout)["status"] == "expired waiting for end of voting period")

        start_new_cycle(self.nodes[0])

        # Create a payment request for an expired proposal
        try:
            self.send_raw_paymentrequest(1, self.goodAddress, proposalid2_expired_timeout, "expired_payreq")
            raise ValueError("Error should be thrown for invalid rawtx (prequest)")
        except JSONRPCException as e:
            assert ("bad-cfund-payment-request" in e.error['message'])

        # Let Proposal 3 expire
        start_new_cycle(self.nodes[0])
        start_new_cycle(self.nodes[0])
        start_new_cycle(self.nodes[0])

        # Proposal 3 should be expired
        assert (self.nodes[0].getproposal(proposalid3_expired_no_votes)["state"] == 3)
        assert (self.nodes[0].getproposal(proposalid3_expired_no_votes)["status"] == "expired")

        # Create a payment request for an expired proposal
        try:
            self.send_raw_paymentrequest(1, self.goodAddress, proposalid3_expired_no_votes, "expired_payreq")
            raise ValueError("Error should be thrown for invalid rawtx (prequest)")
        except JSONRPCException as e:
            assert ("bad-cfund-payment-request" in e.error['message'])


    def check_good_paymentrequest(self, paymentRequest):
        assert (paymentRequest['version'] == 2)
        assert (paymentRequest['hash'] == self.goodPayreqHash)
        assert (paymentRequest['description'] == self.goodDescription)
        assert (float(paymentRequest['requestedAmount']) == float(self.goodAmount))
        assert (paymentRequest['votesYes'] == 0)
        assert (paymentRequest['votesNo'] == 0)
        assert (paymentRequest['votingCycle'] == 0)
        assert (paymentRequest['status'] == 'pending')
        assert (paymentRequest['state'] == 0)
        assert (paymentRequest['stateChangedOnBlock'] == '0000000000000000000000000000000000000000000000000000000000000000')

    def send_raw_paymentrequest(self, amount, address, proposal_hash, description):
        amount = amount * 100000000
        privkey = self.nodes[0].dumpprivkey(address)
        message = "I kindly ask to withdraw " + str(amount) + "NAV from the proposal " + proposal_hash + ". Payment request id: " + str(description)
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
        tx_hash = self.nodes[0].sendrawtransaction(raw_payreq_tx)

        return tx_hash


if __name__ == '__main__':
    CommunityFundCreatePaymentrequestRawTX().main()
