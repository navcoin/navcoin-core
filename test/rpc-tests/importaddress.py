#!/usr/bin/env python3
# Copyright (c) 2018 The Navcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

from test_framework.test_framework import NavCoinTestFramework
from test_framework.staticr_util import *

class ImportAddressTest(NavCoinTestFramework):

    def __init__(self):
        super().__init__()
        self.setup_clean_chain = True
        self.num_nodes = 6

    def setup_network(self, split=False):
        self.nodes = self.setup_nodes()
        connect_nodes(self.nodes[0], 1)
        connect_nodes(self.nodes[1], 2)
        connect_nodes(self.nodes[2], 3)
        connect_nodes(self.nodes[3], 4)
        connect_nodes(self.nodes[4], 5)
        connect_nodes(self.nodes[5], 0)
        self.is_network_split = False

    def run_test(self):
        # Turn off staking until we need it
        self.nodes[0].staking(False)
        self.nodes[1].staking(False)
        self.nodes[2].staking(False)
        self.nodes[3].staking(False)
        self.nodes[4].staking(False)
        self.nodes[5].staking(False)

        # Generate genesis block
        activate_staticr(self.nodes[0])
        self.sync_all()

        # Create a normal address
        address = self.nodes[0].getnewaddress()

        # Create a spending address
        spending_address = self.nodes[0].getnewaddress()

        # Create a staking address
        staking_address = self.nodes[1].getnewaddress()

        # Create the cold address
        coldstaking_address = self.nodes[0].getcoldstakingaddress(staking_address, spending_address)

        # Send some nav to new addresses
        self.nodes[0].sendtoaddress(address, 512)
        self.nodes[0].sendtoaddress(coldstaking_address, 256)
        self.nodes[0].sendtoaddress(coldstaking_address, 128)
        self.nodes[0].generate(6)
        self.sync_all()

        # Import address with balances
        self.nodes[2].importaddress(address)
        self.nodes[3].importaddress(spending_address)
        self.nodes[4].importaddress(staking_address)
        self.nodes[5].importaddress(coldstaking_address)

        # Assert transactions list has the old transactions with correct amounts
        transactions = self.nodes[2].listtransactions("*", 1, 0, True)
        assert_equal(512, transactions[0]['amount'])

        transactions = self.nodes[3].listtransactions("*", 2, 0, True)
        assert_equal(128, transactions[0]['amount'])
        assert_equal(256, transactions[1]['amount'])

        transactions = self.nodes[4].listtransactions("*", 2, 0, True)
        assert_equal(128, transactions[0]['amount'])
        assert_equal(256, transactions[1]['amount'])

        transactions = self.nodes[5].listtransactions("*", 2, 0, True)
        assert_equal(128, transactions[0]['amount'])
        assert_equal(256, transactions[1]['amount'])

        # Assert total balance
        assert_equal(512, self.nodes[2].getbalance("*", 1, True))
        assert_equal(384, self.nodes[3].getbalance("*", 1, True))
        assert_equal(384, self.nodes[4].getbalance("*", 1, True))
        assert_equal(384, self.nodes[5].getbalance("*", 1, True))


if __name__ == '__main__':
    ImportAddressTest().main()
