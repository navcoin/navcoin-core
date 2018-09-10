#!/usr/bin/env python3
# Copyright (c) 2018 The Navcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

from test_framework.test_framework import NavCoinTestFramework
from test_framework.util import *


class DonateCommunityFundTest(NavCoinTestFramework):
    """Tests RPC commands for donating coins to the Community Fund."""

    def __init__(self):
        super().__init__()
        self.setup_clean_chain = True
        self.num_nodes = 1

    def setup_network(self, split=False):
        self.nodes = start_nodes(self.num_nodes, self.options.tmpdir)
        self.is_network_split = False

    def run_test(self):
        self.nodes[0].generate(300)
        self.nodes[0].donatefund(100)
        self.nodes[0].generate(1)

        # Verify the available coins in the fund
        assert(self.nodes[0].cfundstats()["funds"]["available"] == 100)

if __name__ == '__main__':
    DonateCommunityFundTest().main()
