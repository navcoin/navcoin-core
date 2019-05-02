#!/usr/bin/env python3
# Copyright (c) 2018 The Navcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

from test_framework.test_framework import NavCoinTestFramework
from test_framework.staticr_util import *

#import time

class GetStakeReport(NavCoinTestFramework):
    """Tests getstakereport accounting."""

    def __init__(self):
        super().__init__()
        self.setup_clean_chain = True
        self.num_nodes = 3

    def setup_network(self, split=False):
        self.nodes = self.setup_nodes()
        connect_nodes(self.nodes[0], 1)
        connect_nodes(self.nodes[1], 2)
        connect_nodes(self.nodes[2], 0)
        self.is_network_split = False

    def run_test(self):
        # Turn off staking until we need it
        self.nodes[0].staking(False)
        self.nodes[1].staking(False)
        self.nodes[2].staking(False)

        # Make it to the static rewards fork!
        activate_staticr(self.nodes[0])
        self.sync_all()

        # Use THE spending address
        spending_address_public_key = self.nodes[1].getnewaddress()
        spending_address_private_key = self.nodes[1].dumpprivkey(spending_address_public_key)

        # Create a staking address
        staking_address_public_key = self.nodes[2].getnewaddress()
        staking_address_private_key = self.nodes[2].dumpprivkey(staking_address_public_key)

        # Import the 2 keys into a third wallet
        self.nodes[0].importprivkey(spending_address_private_key)
        self.nodes[0].importprivkey(staking_address_private_key)

        # Create the cold address
        coldstaking_address_staking = self.nodes[1].getcoldstakingaddress(staking_address_public_key, spending_address_public_key)

        # Send funds to the spending address (leave some NAV for fees)
        self.nodes[0].sendtoaddress(spending_address_public_key, self.nodes[0].getbalance() - 1)
        self.nodes[0].generate(1)
        self.sync_all()

        # Turn staking on
        self.nodes[1].staking(True)

        # Stake a block
        self.stake_block(self.nodes[1])

        # Turn staking off again
        self.nodes[1].staking(False)

        # Load the last 24h stake amount for the wallets/nodes
        merged_address_last_24h = self.nodes[0].getstakereport()['Last 24H']
        spending_address_last_24h = self.nodes[1].getstakereport()['Last 24H']
        staking_address_last_24h = self.nodes[2].getstakereport()['Last 24H']
        # print('spending', spending_address_last_24h)
        # print('staking', staking_address_last_24h)
        # print('merged', merged_address_last_24h)

        # Make sure we have staked 2 NAV to the spending address
        # So that means spending last 24h == 2
        # And staking last 24h == 0 We have not sent any coins yet
        # And merged will have the total of the spending + staking
        assert_equal('2.00', merged_address_last_24h)
        assert_equal('2.00', spending_address_last_24h)
        assert_equal('0.00', staking_address_last_24h)

        # Send funds to the cold staking address (leave some NAV for fees)
        self.nodes[1].sendtoaddress(coldstaking_address_staking, self.nodes[1].getbalance() - 1)
        self.nodes[1].generate(1)
        self.sync_all()

        # Turn staking on
        self.nodes[2].staking(True)

        # Stake a block
        self.stake_block(self.nodes[2])

        # Turn staking off again
        self.nodes[2].staking(False)

        # Load the last 24h stake amount for the wallets/nodes
        merged_address_last_24h = self.nodes[0].getstakereport()['Last 24H']
        spending_address_last_24h = self.nodes[1].getstakereport()['Last 24H']
        staking_address_last_24h = self.nodes[2].getstakereport()['Last 24H']
        # print('spending', spending_address_last_24h)
        # print('staking', staking_address_last_24h)
        # print('merged', merged_address_last_24h)

        # Make sure we staked 4 NAV in spending address (2 NAV via COLD Stake)
        # So that means spending last 24h == 4
        # And staking last 24h == 2 We stake 2 NAV via COLD already
        # And merged will have the total of the spending + staking
        assert_equal('4.00', merged_address_last_24h)
        assert_equal('4.00', spending_address_last_24h)
        assert_equal('2.00', staking_address_last_24h)

    def stake_block(self, node):
        # Get the current block count to check against while we wait for a stake
        blockcount = node.getblockcount()

        # wait for a new block to be mined
        while node.getblockcount() == blockcount:
            # print("waiting for a new block...")
            time.sleep(1)

        # We got one
        # print("found a new block...")

        # Make sure the blocks are mature before we check the report
        slow_gen(node, 5, 0.5)
        self.sync_all()


if __name__ == '__main__':
    GetStakeReport().main()
