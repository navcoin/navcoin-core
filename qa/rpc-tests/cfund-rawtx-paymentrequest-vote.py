#!/usr/bin/env python3
# Copyright (c) 2018 The Navcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

from test_framework.test_framework import NavCoinTestFramework
from test_framework.cfund_util import *


class CommunityFundVotePaymentrequestRawTX(NavCoinTestFramework):
    """Tests the state transition of payment requests of the Community fund."""

    def __init__(self):
        super().__init__()
        self.setup_clean_chain = True
        self.num_nodes = 1

    def setup_network(self, split=False):
        self.nodes = self.setup_nodes()
        self.is_network_split = split

    def run_test(self):
        # Make sure cfund is active
        activate_cfund(self.nodes[0])

        # Donate to the cfund
        self.nodes[0].donatefund(100)

        # Get address
        address = self.nodes[0].getnewaddress()

        # Create a proposal
        proposalid0 = self.nodes[0].createproposal(address, 10, 3600, "testprop")["hash"]
        start_new_cycle(self.nodes[0])

        # Accept the proposal
        self.nodes[0].proposalvote(proposalid0, "yes")
        start_new_cycle(self.nodes[0])
        self.nodes[0].proposalvote(proposalid0, "remove")
        slow_gen(self.nodes[0], 5)

        # Proposal should be accepted
        assert (self.nodes[0].getproposal(proposalid0)["state"] == 1)
        assert (self.nodes[0].getproposal(proposalid0)["status"] == "accepted")

        # Create payment requests
        paymentrequestid0 = self.nodes[0].createpaymentrequest(proposalid0, 1, "test0")["hash"]
        paymentrequestid1 = self.nodes[0].createpaymentrequest(proposalid0, 1, "test1")["hash"]
        slow_gen(self.nodes[0], 1)

        # pre-flight tests
        assert (self.nodes[0].getpaymentrequest(paymentrequestid0)['votesYes'] == 0)
        assert (self.nodes[0].getpaymentrequest(paymentrequestid0)['votesNo'] == 0)

        assert (self.nodes[0].getpaymentrequest(paymentrequestid1)['votesYes'] == 0)
        assert (self.nodes[0].getpaymentrequest(paymentrequestid1)['votesNo'] == 0)

        # Create valid vote tx's
        pr0_vote_tx_yes = create_vote_tx(self.nodes[0], 'c3', 'c4', paymentrequestid0)
        pr0_vote_tx_no = create_vote_tx(self.nodes[0], 'c3', 'c5', paymentrequestid0)
        pr1_vote_tx_yes = create_vote_tx(self.nodes[0], 'c3', 'c4', paymentrequestid1)

        # Make a proper good vote - yes
        self.nodes[0].coinbaseoutputs([pr0_vote_tx_yes])
        slow_gen(self.nodes[0], 1)
        assert (self.nodes[0].getpaymentrequest(paymentrequestid0)['votesYes'] == 1)
        assert (self.nodes[0].getpaymentrequest(paymentrequestid0)['votesNo'] == 0)

        assert (self.nodes[0].getpaymentrequest(paymentrequestid1)['votesYes'] == 0)
        assert (self.nodes[0].getpaymentrequest(paymentrequestid1)['votesNo'] == 0)

        # Make a proper good vote - no
        self.nodes[0].coinbaseoutputs([pr0_vote_tx_no])
        slow_gen(self.nodes[0], 1)
        assert (self.nodes[0].getpaymentrequest(paymentrequestid0)['votesYes'] == 1)
        assert (self.nodes[0].getpaymentrequest(paymentrequestid0)['votesNo'] == 1)

        assert (self.nodes[0].getpaymentrequest(paymentrequestid1)['votesYes'] == 0)
        assert (self.nodes[0].getpaymentrequest(paymentrequestid1)['votesNo'] == 0)

        # Insert multiple yes votes
        self.nodes[0].coinbaseoutputs([pr0_vote_tx_yes, pr0_vote_tx_yes, pr0_vote_tx_yes])
        slow_gen(self.nodes[0], 1)
        assert (self.nodes[0].getpaymentrequest(paymentrequestid0)['votesYes'] == 2)
        assert (self.nodes[0].getpaymentrequest(paymentrequestid0)['votesNo'] == 1)

        assert (self.nodes[0].getpaymentrequest(paymentrequestid1)['votesYes'] == 0)
        assert (self.nodes[0].getpaymentrequest(paymentrequestid1)['votesNo'] == 0)

        # Insert yes and no votes
        self.nodes[0].coinbaseoutputs([pr0_vote_tx_yes, pr0_vote_tx_no])
        slow_gen(self.nodes[0], 1)
        assert (self.nodes[0].getpaymentrequest(paymentrequestid0)['votesYes'] == 3)
        assert (self.nodes[0].getpaymentrequest(paymentrequestid0)['votesNo'] == 1)

        assert (self.nodes[0].getpaymentrequest(paymentrequestid1)['votesYes'] == 0)
        assert (self.nodes[0].getpaymentrequest(paymentrequestid1)['votesNo'] == 0)

        # Insert votes for multiple payment requests
        self.nodes[0].coinbaseoutputs([pr0_vote_tx_yes, pr1_vote_tx_yes])
        slow_gen(self.nodes[0], 1)
        assert (self.nodes[0].getpaymentrequest(paymentrequestid0)['votesYes'] == 4)
        assert (self.nodes[0].getpaymentrequest(paymentrequestid0)['votesNo'] == 1)

        assert (self.nodes[0].getpaymentrequest(paymentrequestid1)['votesYes'] == 1)
        assert (self.nodes[0].getpaymentrequest(paymentrequestid1)['votesNo'] == 0)

        # Insert bad vote tx with double vote in string
        pr0_bad_vote_tx = create_vote_tx(self.nodes[0], 'c3', 'c4c4', paymentrequestid0)
        self.nodes[0].coinbaseoutputs([pr0_bad_vote_tx])
        slow_gen(self.nodes[0], 1)
        assert (self.nodes[0].getpaymentrequest(paymentrequestid0)['votesYes'] == 4)


if __name__ == '__main__':
    CommunityFundVotePaymentrequestRawTX().main()
