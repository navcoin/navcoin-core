#!/usr/bin/env python3
# Copyright (c) 2018 The Navcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

from test_framework.test_framework import NavCoinTestFramework
from test_framework.staticr_util import *

import time

class StaticRTxSend(NavCoinTestFramework):
    """Tests the tx sending after softfork activation."""

    def __init__(self):
        super().__init__()
        self.setup_clean_chain = True
        self.num_nodes = 1

    def setup_network(self, split=False):
        self.nodes = self.setup_nodes()
        self.is_network_split = split

    def run_test(self):

        #check that a transaction can be sent after the reward changes to static

        activate_staticr(self.nodes[0])

        blockcount = self.nodes[0].getblockcount()
        address = self.nodes[0].getnewaddress()
        txid = self.nodes[0].sendtoaddress(address, 100)

        # wait for a new block to be mined
        while self.nodes[0].getblockcount() == blockcount:
            print("waiting for a new block...")
            time.sleep(5)

        transaction = self.nodes[0].gettransaction(txid)

        # check the transaction confirmed
        assert(transaction["confirmations"] > 0)

if __name__ == '__main__':
    StaticRTxSend().main()
