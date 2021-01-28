#!/usr/bin/env python3
# Copyright (c) 2018 The Navcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

from test_framework.test_framework import NavcoinTestFramework
from test_framework.staticr_util import *

#import time

class GetStakeReport(NavcoinTestFramework):
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

        # Send funds to the spending address (leave me NAV for fees)
        self.nodes[0].sendtoaddress(spending_address_public_key, self.nodes[0].getbalance() - 1)
        self.nodes[0].generate(1)
        self.sync_all()

        # Stake a block
        self.stake_block(self.nodes[1])

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

        # Stake a block
        self.stake_block(self.nodes[2])

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

        # Time travel 2 days in the future
        cur_time = int(time.time())
        self.nodes[0].setmocktime(cur_time + 172800)
        self.nodes[1].setmocktime(cur_time + 172800)
        self.nodes[2].setmocktime(cur_time + 172800)

        # Stake a block
        self.stake_block(self.nodes[2])

        # Load the last 24h stake amount for the wallets/nodes
        merged_address_last_24h = self.nodes[0].getstakereport()['Last 24H']
        spending_address_last_24h = self.nodes[1].getstakereport()['Last 24H']
        staking_address_last_24h = self.nodes[2].getstakereport()['Last 24H']

        # Check the amounts
        assert_equal('2.00', merged_address_last_24h)
        assert_equal('2.00', spending_address_last_24h)
        assert_equal('2.00', staking_address_last_24h)

        # Load the last 7 days stake amount for the wallets/nodes
        merged_address_last_7d = self.nodes[0].getstakereport()['Last 7 Days']
        spending_address_last_7d = self.nodes[1].getstakereport()['Last 7 Days']
        staking_address_last_7d = self.nodes[2].getstakereport()['Last 7 Days']

        # Check the amounts
        assert_equal('6.00', merged_address_last_7d)
        assert_equal('6.00', spending_address_last_7d)
        assert_equal('4.00', staking_address_last_7d)

        # Load the averages for stake amounts
        avg_last7d   = self.nodes[0].getstakereport()['Last 7 Days Avg']
        avg_last30d  = self.nodes[0].getstakereport()['Last 30 Days Avg']
        avg_last365d = self.nodes[0].getstakereport()['Last 365 Days Avg']

        # Check the amounts
        assert_equal('3.00', avg_last7d)
        assert_equal('3.00', avg_last30d)
        assert_equal('3.00', avg_last365d)

        # Time travel 8 days in the future
        cur_time = int(time.time())
        self.nodes[0].setmocktime(cur_time + 691200)
        self.nodes[1].setmocktime(cur_time + 691200)
        self.nodes[2].setmocktime(cur_time + 691200)

        # Load the last 24h stake amount for the wallets/nodes
        merged_address_last_24h = self.nodes[0].getstakereport()['Last 24H']
        spending_address_last_24h = self.nodes[1].getstakereport()['Last 24H']
        staking_address_last_24h = self.nodes[2].getstakereport()['Last 24H']

        # Check the amounts
        assert_equal('0.00', merged_address_last_24h)
        assert_equal('0.00', spending_address_last_24h)
        assert_equal('0.00', staking_address_last_24h)

        # Load the last 7 days stake amount for the wallets/nodes
        merged_address_last_7d = self.nodes[0].getstakereport()['Last 7 Days']
        spending_address_last_7d = self.nodes[1].getstakereport()['Last 7 Days']
        staking_address_last_7d = self.nodes[2].getstakereport()['Last 7 Days']

        # Check the amounts
        assert_equal('2.00', merged_address_last_7d)
        assert_equal('2.00', spending_address_last_7d)
        assert_equal('2.00', staking_address_last_7d)

        # Load the averages for stake amounts
        avg_last7d   = self.nodes[0].getstakereport()['Last 7 Days Avg']
        avg_last30d  = self.nodes[0].getstakereport()['Last 30 Days Avg']
        avg_last365d = self.nodes[0].getstakereport()['Last 365 Days Avg']

        # Check the amounts
        assert_equal('0.28571428', avg_last7d)
        assert_equal('0.75',       avg_last30d)
        assert_equal('0.75',       avg_last365d)

        # Time travel 31 days in the future
        cur_time = int(time.time())
        self.nodes[0].setmocktime(cur_time + 2678400)
        self.nodes[1].setmocktime(cur_time + 2678400)
        self.nodes[2].setmocktime(cur_time + 2678400)

        # Load the last 24h stake amount for the wallets/nodes
        merged_address_last_24h = self.nodes[0].getstakereport()['Last 24H']
        spending_address_last_24h = self.nodes[1].getstakereport()['Last 24H']
        staking_address_last_24h = self.nodes[2].getstakereport()['Last 24H']

        # Check the amounts
        assert_equal('0.00', merged_address_last_24h)
        assert_equal('0.00', spending_address_last_24h)
        assert_equal('0.00', staking_address_last_24h)

        # Load the last 7 days stake amount for the wallets/nodes
        merged_address_last_7d = self.nodes[0].getstakereport()['Last 7 Days']
        spending_address_last_7d = self.nodes[1].getstakereport()['Last 7 Days']
        staking_address_last_7d = self.nodes[2].getstakereport()['Last 7 Days']

        # Check the amounts
        assert_equal('0.00', merged_address_last_7d)
        assert_equal('0.00', spending_address_last_7d)
        assert_equal('0.00', staking_address_last_7d)

        # Load the averages for stake amounts
        avg_last7d   = self.nodes[0].getstakereport()['Last 7 Days Avg']
        avg_last30d  = self.nodes[0].getstakereport()['Last 30 Days Avg']
        avg_last365d = self.nodes[0].getstakereport()['Last 365 Days Avg']

        # Check the amounts
        assert_equal('0.00',       avg_last7d)
        assert_equal('0.06666666', avg_last30d)
        assert_equal('0.19354838', avg_last365d)

        # Disconnect the nodes
        for node in self.nodes[0].getpeerinfo():
            self.nodes[0].disconnectnode(node['addr'])
        time.sleep(2) #disconnecting a node needs a little bit of time
        assert(self.nodes[0].getpeerinfo() == [])

        # Stake a block on node 0
        orphaned_block_hash = self.stake_block(self.nodes[0], False)

        # Generate some blocks on node 1
        self.nodes[1].generate(100)

        # Reconnect the nodes
        connect_nodes(self.nodes[0], 1)
        connect_nodes(self.nodes[1], 2)
        connect_nodes(self.nodes[2], 0)

        # Wait for blocks to sync
        self.sync_all()

        # Make sure the block was orphaned
        assert(self.nodes[0].getblock(orphaned_block_hash)['confirmations'] == -1)

        # Check the staked amount
        # Should be 0 (Zero) as the last staked block is orphaned
        assert_equal('0.00', self.nodes[0].getstakereport()['Last 7 Days'])

    def stake_block(self, node, mature = True):
        # Get the current block count to check against while we wait for a stake
        blockcount = node.getblockcount()

        # Turn staking on
        node.staking(True)

        # wait for a new block to be mined
        while node.getblockcount() == blockcount:
            # print("waiting for a new block...")
            time.sleep(1)

        # We got one
        # print("found a new block...")

        # Turn staking off
        node.staking(False)

        # Get the staked block
        block_hash = node.getbestblockhash()

        # Only mature the blocks if we asked for it
        if (mature):
            # Make sure the blocks are mature before we check the report
            slow_gen(node, 5, 0.5)
            self.sync_all()

        # return the block hash to the function caller
        return block_hash


if __name__ == '__main__':
    GetStakeReport().main()
