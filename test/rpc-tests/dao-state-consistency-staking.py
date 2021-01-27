#!/usr/bin/env python3
# Copyright (c) 2018 The Navcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

from test_framework.test_framework import NavcoinTestFramework
from test_framework.cfund_util import *

import time
import http.client
import urllib.parse

class DaoStateConsistency(NavcoinTestFramework):
    """Tests the consistency of the dao state between different nodes with reorganizations."""

    def __init__(self):
        super().__init__()
        self.setup_clean_chain = True
        self.num_nodes = 2

    def setup_network(self, split=False):
        self.nodes = []
        self.nodes.append(start_node(0, self.options.tmpdir, ["-debug=dao", "-staking=0"]))
        self.nodes.append(start_node(1, self.options.tmpdir, ["-debug=dao", "-staking=0"]))
        self.is_network_split = split
        connect_nodes(self.nodes[0], 1)

    def run_test(self):

        self.nodes[0].importprivkey("cRiEVWeGUKNctkdQGiq99KdNVLgPaZtS5auoCCieFYHJ65kvkiyL")
        self.nodes[0].generatetoaddress(300, "mhxM53meBHKpVUe6PY5DzssGGDs6NhV45G")

        sync_blocks(self.nodes)

        hash = self.nodes[0].createconsultationwithanswers("test",["a","b"])["hash"]
        print(hash)
        self.stake_block(self.nodes[1], 1)

        self.nodes[0].support(self.nodes[0].getconsultation(hash)["answers"][0]["hash"])
        self.nodes[1].support(self.nodes[1].getconsultation(hash)["answers"][1]["hash"])

        self.stake_block(self.nodes[1], 1)

        print(self.nodes[0].getstakervote("210301753de0f8d53816b73174e53f8da9cb392ae2a3b3070ae4f0b0a1be46e1c6d3ac"))

        self.nodes[0].consultationvote(self.nodes[0].getconsultation(hash)["answers"][0]["hash"], "yes")
        self.nodes[1].consultationvote(self.nodes[1].getconsultation(hash)["answers"][1]["hash"], "yes")

    def stake_block(self, node, count=1):
        # Get the current block count to check against while we wait for a stake
        blockcount = node.getblockcount()
        lastblock = blockcount

        # Turn staking on
        node.staking(True)

        print("Trying to stake", count, "blocks")

        # wait for a new block to be mined
        while node.getblockcount() <= blockcount+count-1:
            #print(node.getblock(node.getbestblockhash())["time"]+1)
            #node.setmocktime(int(node.getblock(node.getbestblockhash())["time"]+16)%16)
            # print("waiting for a new block...")
            time.sleep(1)
            if node.getblockcount() != lastblock:
                lastblock = node.getblockcount()
                print("Staked block at height", lastblock)

        # Turn staking off
        node.staking(False)

    def end_cycle_stake(self, node):
        # Move to the end of the cycle
        self.stake_block(node, node.cfundstats()["votingPeriod"]["ending"] - node.cfundstats()["votingPeriod"][
            "current"])

if __name__ == '__main__':
    DaoStateConsistency().main()
