#!/usr/bin/env python3
# Copyright (c) 2018 The Navcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

from test_framework.test_framework import NavCoinTestFramework
from test_framework.util import *

import time

class CommunityFundCreateProposalRawTX(NavCoinTestFramework):
    """Tests the state transition of proposals of the Community fund."""

    def __init__(self):
        super().__init__()
        self.setup_clean_chain = True
        self.num_nodes = 1


        self.goodDescription = "these are not the NAV Droids you are looking for"
        self.goodDuration = 360000
        self.goodAmount = 100
        self.goodPropHash = ""
        self.goodAddress = ""

    def setup_network(self, split=False):
        self.nodes = start_nodes(self.num_nodes, self.options.tmpdir)
        self.is_network_split = False

    def run_test(self):
        slow_gen(self.nodes[0], 100)
        # Verify the Community Fund is started
        assert(self.nodes[0].getblockchaininfo()["bip9_softforks"]["communityfund"]["status"] == "started")

        slow_gen(self.nodes[0], 100)
        # Verify the Community Fund is locked_in
        assert(self.nodes[0].getblockchaininfo()["bip9_softforks"]["communityfund"]["status"] == "locked_in")

        slow_gen(self.nodes[0], 100)
        # Verify the Community Fund is active
        assert(self.nodes[0].getblockchaininfo()["bip9_softforks"]["communityfund"]["status"] == "active")


        self.test_happy_path()
        self.test_negative_value()


    def test_negative_value(self):

        description = "this should not WORK"
        duration = 360000
        amount = -100

        # Create new payment request for more than the amount
        propHash = ""
        try:
            propHash = self.send_raw_propsalrequest(self.goodAddress, amount, duration, description)
        except Exception as e:
            assert(propHash == "")


        #check a gen - should still only have the last good prop
        blocks = slow_gen(self.nodes[0], 1)
        propsalList = self.nodes[0].listproposals()

        #should still only have 1 proposal from the good test run
        assert(len(propsalList) == 1)
        self.checkGoodPropsal(propsalList[0])





    # Test everything the way it should be
    def test_happy_path(self):

        self.goodAddress = self.nodes[0].getnewaddress()

        self.goodPropHash = self.send_raw_propsalrequest(self.goodAddress, self.goodAmount, self.goodDuration, self.goodDescription)

        blocks = slow_gen(self.nodes[0], 1)
        propsalList = self.nodes[0].listproposals()

        #should only have 1 propsal
        assert(len(propsalList) == 1)

        # The proposal should have all the same required fields
        assert (propsalList[0]['blockHash'] == blocks[0])
        self.checkGoodPropsal(propsalList[0])



    def checkGoodPropsal(self, proposal):

        assert (proposal['votingCycle'] == 0)
        assert (proposal['version'] == 2)
        assert (proposal['paymentAddress'] == self.goodAddress)
        assert (proposal['proposalDuration'] == self.goodDuration)
        assert (proposal['description'] == self.goodDescription)
        assert (proposal['votesYes'] == 0)
        assert (proposal['votesNo'] == 0)
        assert (proposal['status'] == 'pending')
        assert (proposal['state'] == 0)
        assert (proposal['hash'] == self.goodPropHash)
        assert (float(proposal['requestedAmount']) == float(self.goodAmount))
        assert (float(proposal['notPaidYet']) == float(self.goodAmount))
        assert (float(proposal['userPaidFee']) == float(1))


    def send_raw_propsalrequest(self, address, amount, time, description):

        amount = amount * 100000000

        # Create a raw proposal tx
        raw_proposal_tx = self.nodes[0].createrawtransaction(
            [],
            {"6ac1": 1},
            json.dumps({"v": 2, "n": amount, "a": address,  "d": time, "s": description})
        )

        # Modify version
        raw_proposal_tx = "04" + raw_proposal_tx[2:]

        # Fund raw transaction
        raw_proposal_tx = self.nodes[0].fundrawtransaction(raw_proposal_tx)['hex']

        # Sign raw transaction
        raw_proposal_tx = self.nodes[0].signrawtransaction(raw_proposal_tx)['hex']

        # Send raw transaction
        return self.nodes[0].sendrawtransaction(raw_proposal_tx)



    def start_new_cycle(self):
        # Move to the end of the cycle
        self.slow_gen(self.nodes[0].cfundstats()["votingPeriod"]["ending"] - self.nodes[0].cfundstats()["votingPeriod"]["current"])


if __name__ == '__main__':
    CommunityFundCreateProposalRawTX().main()
