#!/usr/bin/env python3
# Copyright (c) 2018 The NavCoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

#
# Test rejection of version bit votes
#

import time
from test_framework.test_framework import NavCoinTestFramework
from test_framework.util import *
from test_framework.mininode import *

class RejectVersionBitTest(NavCoinTestFramework):

    def setup_chain(self):
        print("Initializing test directory "+self.options.tmpdir)
        initialize_chain_clean(self.options.tmpdir, 4)

    def setup_network(self):
        self.nodes = []
        # Nodes 0/1 are "wallet" nodes
        self.nodes.append(start_node(0, self.options.tmpdir, ["-rejectversionbit=6"]))
        self.nodes.append(start_node(1, self.options.tmpdir, []))
        connect_nodes(self.nodes[0], 1)
        self.is_network_split = False
        self.sync_all()

    def run_test(self):
        print("Mining blocks...")
        blocks_node_1=self.nodes[0].generate(100)
        self.sync_all()
        assert(self.nodes[0].getblockchaininfo()["bip9_softforks"]["segwit"]["status"] == "started")
        blocks_node_1=self.nodes[0].generate(10)
        self.sync_all()
        blocks_node_2=self.nodes[1].generate(90)
        self.sync_all()
        assert(self.nodes[0].getblock(blocks_node_1[-1])["version"] & (1<<6) == 0)
        assert(self.nodes[1].getblock(blocks_node_2[-1])["version"] & (1<<6) == (1<<6))
        self.sync_all()
        assert(self.nodes[0].getblockchaininfo()["bip9_softforks"]["segwit"]["status"] == "locked_in")
        blocks_node_1=self.nodes[0].generate(50)
        blocks_node_2=self.nodes[1].generate(50)
        assert(self.nodes[0].getblock(blocks_node_1[-1])["version"] & (1<<6) == (1<<6))
        assert(self.nodes[1].getblock(blocks_node_2[-1])["version"] & (1<<6) == (1<<6))

if __name__ == '__main__':
    RejectVersionBitTest().main()
