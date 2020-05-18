#!/usr/bin/env python3
# Copyright (c) 2018 The Navcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

from test_framework.test_framework import NavCoinTestFramework
from test_framework.staticr_util import *

#import time

class GetStakingInfo(NavCoinTestFramework):
    """Tests getstakereport accounting."""

    def __init__(self):
        super().__init__()
        self.setup_clean_chain = True
        self.num_nodes = 1

    def setup_network(self, split=False):
        self.nodes = self.setup_nodes()
        self.is_network_split = False

    def run_test(self):
        # Turn off staking
        self.nodes[0].staking(False)

        # Check if we get the error for nWeight
        assert(not self.nodes[0].getstakinginfo()['enabled'])
        assert(not self.nodes[0].getstakinginfo()['staking'])
        assert_equal("Warning: We don't appear to have mature coins.", self.nodes[0].getstakinginfo()['errors'])

        # Make it to the static rewards fork!
        activate_staticr(self.nodes[0])

        # Check balance
        assert_equal(59814950, self.nodes[0].getwalletinfo()['balance'] + self.nodes[0].getwalletinfo()['immature_balance'])

        # Turn on staking
        self.nodes[0].generate(2000)
        self.nodes[0].staking(True)

        # Check for staking after we have matured coins
        assert(self.nodes[0].getstakinginfo()['enabled'])
        # Wait for the node to start staking
        while not self.nodes[0].getstakinginfo()['staking']:
            time.sleep(0.5)
        assert(self.nodes[0].getstakinginfo()['staking'])
        assert_equal("", self.nodes[0].getstakinginfo()['errors'])

        # Get the current block count to check against while we wait for a stake
        blockcount = self.nodes[0].getblockcount()

        # wait for a new block to be mined
        while self.nodes[0].getblockcount() == blockcount:
            print("waiting for a new block...")
            time.sleep(1)

        # We got one
        print("found a new block...")

        # Check balance
        assert_equal(59914952, self.nodes[0].getwalletinfo()['balance'] + self.nodes[0].getwalletinfo()['immature_balance'])

        # Check if we get the error for nWeight again after a stake
        assert(self.nodes[0].getstakinginfo()['enabled'])
        assert(self.nodes[0].getstakinginfo()['staking'])
        assert_equal("", self.nodes[0].getstakinginfo()['errors'])

        # Check expecteddailyreward
        assert_equal(86400 / (self.nodes[0].getstakinginfo()['expectedtime'] + 1) * 2, self.nodes[0].getstakinginfo()['expecteddailyreward'])

        # LOCK the wallet
        self.nodes[0].encryptwallet("password")
        stop_nodes(self.nodes)
        wait_navcoinds()
        self.nodes = start_nodes(self.num_nodes, self.options.tmpdir)

        # Check if we get the error for nWeight again after a stake
        assert(self.nodes[0].getstakinginfo()['enabled'])
        assert(not self.nodes[0].getstakinginfo()['staking'])
        assert_equal("Warning: Wallet is locked. Please enter the wallet passphrase with walletpassphrase first.", self.nodes[0].getstakinginfo()['errors'])


if __name__ == '__main__':
    GetStakingInfo().main()
