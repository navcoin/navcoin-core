#!/usr/bin/env python3
# Copyright (c) 2019 The Navcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

from test_framework.test_framework import NavCoinTestFramework
from test_framework.util import *

class CreateRawScriptAddress(NavCoinTestFramework):
    """Tests the creation of a raw script address."""

    def __init__(self):
        super().__init__()
        self.setup_clean_chain = True
        self.num_nodes = 1

    def setup_network(self, split=False):
        self.nodes = self.setup_nodes()
        self.is_network_split = split

    def run_test(self):
        assert_equal(self.nodes[0].createrawscriptaddress("6ac1"), "3HnzbJ4TR9")
        assert_equal(self.nodes[0].validateaddress("3HnzbJ4TR9")["isvalid"], True)
        assert_equal(self.nodes[0].validateaddress("3HnzbJ4TR9")["scriptPubKey"], "6ac1")

        self.nodes[0].staking(False)
        self.nodes[0].generate(100)
        self.nodes[0].sendtoaddress("3HnzbJ4TR9", 100)
        self.nodes[0].generate(1)

        assert_equal(self.nodes[0].getinfo()["communityfund"]["available"], 100)

if __name__ == '__main__':
    CreateRawScriptAddress().main()
