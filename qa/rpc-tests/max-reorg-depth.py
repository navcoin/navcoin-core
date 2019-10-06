#!/usr/bin/env python3
# Copyright (c) 2018 The Navcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

from test_framework.test_framework import NavCoinTestFramework
from test_framework.cfund_util import *

import time
import http.client
import urllib.parse

class MaxReorgDepth(NavCoinTestFramework):
    """Tests the maximum reorganisation depth."""

    def __init__(self):
        super().__init__()
        self.setup_clean_chain = True
        self.num_nodes = 2

    def setup_network(self, split=False):
        self.nodes = self.setup_nodes()
        connect_nodes(self.nodes[0], 1)
        self.is_network_split = False

    def run_test(self):

        self.nodes[0].staking(False)
        self.nodes[1].staking(False)
        slow_gen(self.nodes[0], 1000)
        self.sync_all()

        # verify the nodes are on the same chain
        assert(self.nodes[0].getblockcount() == self.nodes[1].getblockcount())
        assert(self.nodes[0].getbestblockhash() == self.nodes[1].getbestblockhash())

        # disconnect the nodes mine blocks on each, confirming they are on different chains
        url = urllib.parse.urlparse(self.nodes[1].url)
        self.nodes[0].disconnectnode(url.hostname+":"+str(p2p_port(1)))
        time.sleep(2) #disconnecting

        blocks0 = self.nodes[0].getblockcount()
        blocks1 = self.nodes[1].getblockcount()

        cur_time = int(time.time())

        filter_size = 500

        for i in range(filter_size):
          self.nodes[0].setmocktime(cur_time + 30)
          self.nodes[0].generate(1)
          self.nodes[1].setmocktime(cur_time + 30)
          self.nodes[1].generate(1)
          cur_time += 30

        print(self.nodes[0].getblockcount(), blocks0, self.nodes[0].getbestblockhash())
        print(self.nodes[1].getblockcount(), blocks1, self.nodes[1].getbestblockhash())
        
        assert(self.nodes[0].getblockcount() == blocks0 + filter_size)
        assert(self.nodes[1].getblockcount() == blocks1 + filter_size)
        assert(self.nodes[0].getbestblockhash() != self.nodes[1].getbestblockhash())

        self.nodes[1].setmocktime(cur_time)
        slow_gen(self.nodes[1], 1)

        assert(self.nodes[1].getblockcount() == blocks1 + filter_size + 1)

        longestChainHash = self.nodes[1].getbestblockhash()

        # reconnect the node trying to force node 0 reorg to node 1 and hit the filter
        connect_nodes_bi(self.nodes,0,1)
        sync_blocks(self.nodes)

        print("After Reconnect")

        print(self.nodes[0].getblockcount(), blocks0, self.nodes[0].getbestblockhash())
        print(self.nodes[1].getblockcount(), blocks1, self.nodes[1].getbestblockhash())

        assert(self.nodes[0].getblockcount() == blocks0 + filter_size)
        assert(self.nodes[1].getblockcount() == blocks1 + filter_size + 1)

        assert(self.nodes[0].getbestblockhash() != longestChainHash)
        assert(self.nodes[0].getbestblockhash() != self.nodes[1].getbestblockhash())

        print("Reorg Prevented")
       
if __name__ == '__main__':
    MaxReorgDepth().main()
