#!/usr/bin/env python3
# Copyright (c) 2014-2016 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.


#
# Expanded helper routines for regression testing of the NAV Coin community fund
#

from test_framework.util import *

def activateHardFork(node, versionBit, activationHeight):
    # check the block reports the correct version bit after the hardfork height
    slow_gen(node, 1)

    blockHash1 = node.getbestblockhash()
    block1 = node.getblock(blockHash1)
    versionBinary1 = bin(int(block1["versionHex"], 16))[2:]
    versionBit1 = versionBinary1[(versionBit+1)*-1]
    assert(int(versionBit1) == 0)

    # activate hard fork
    slow_gen(node, activationHeight - node.getblockchaininfo()["blocks"] + 1)

    blockHash2 = node.getbestblockhash()
    block2 = node.getblock(blockHash2)
    versionBinary2 = bin(int(block2["versionHex"], 16))[2:]
    versionBit2 = versionBinary2[(versionBit+1)*-1]
    assert(int(versionBit2) == 1)
