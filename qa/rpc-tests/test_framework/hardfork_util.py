#!/usr/bin/env python3
# Copyright (c) 2014-2016 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.


#
# Expanded helper routines for regression testing of the NAV Coin community fund
#

from test_framework.util import *


def activate_451(node):
    # check the block reports the correct version bit (21) after the hardfork height
    slow_gen(node, 100)

    blockHash1 = node.getbestblockhash()
    block1 = node.getblock(blockHash1)
    versionBinary1 = bin(int(block1["versionHex"], 16))[2:]
    versionBit1 = versionBinary1[-21]
    assert(int(versionBit1) == 0)

    # activate hard fork
    slow_gen(node, 901)

    blockHash2 = node.getbestblockhash()
    block2 = node.getblock(blockHash2)
    versionBinary2 = bin(int(block2["versionHex"], 16))[2:]
    versionBit2 = versionBinary2[-21]
    assert(int(versionBit2) == 1)
