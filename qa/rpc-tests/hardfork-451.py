#!/usr/bin/env python3
# Copyright (c) 2018 The Navcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

from test_framework.test_framework import NavCoinTestFramework

import time

class Hardfork451(NavCoinTestFramework):
    """Tests hardfork 451 activates."""

    def __init__(self):
        super().__init__()
        self.setup_clean_chain = True
        self.num_nodes = 1

    def setup_network(self, split=False):
        self.nodes = self.setup_nodes()
        self.is_network_split = split

    def run_test(self):

        # check the block reports the correct version bit (21) after the hardfork height
        slow_gen(self.nodes[0], 100)

        blockcount1 = self.nodes[0].getblockcount()

        # wait for a new block to be mined
        while self.nodes[0].getblockcount() == blockcount1:
            print("waiting for a new block...")
            time.sleep(1)

        assert(blockcount1 != self.nodes[0].getblockcount())

        blockHash1 = self.nodes[0].getbestblockhash()
        block1 = self.nodes[0].getblock(blockHash1)
        print(block1)
        print(bin(int(block1["versionHex"], 16))[2:])
        versionBinary1 = bin(int(block1["versionHex"], 16))[2:]
        print(versionBinary1[-22])
        versionBit1 = versionBinary1[-22]
        assert(versionBit1 = 0)

        # activate hard fork
        slow_gen(self.nodes[0], 900)

        blockcount2 = self.nodes[0].getblockcount()

        # wait for a new block to be mined
        while self.nodes[0].getblockcount() == blockcount2:
            print("waiting for a new block...")
            time.sleep(1)

        assert(blockcount2 != self.nodes[0].getblockcount())

        blockHash2 = self.nodes[0].getbestblockhash()
        block2 = self.nodes[0].getblock(blockHash2)
        print(block2)
        print(bin(int(block2["versionHex"], 16))[2:])
        versionBinary2 = bin(int(block2["versionHex"], 16))[2:]
        print(versionBinary2[-22])
        versionBit2 = versionBinary2[-22]
        assert(versionBit2 = 1)

if __name__ == '__main__':
    Hardfork451().main()
