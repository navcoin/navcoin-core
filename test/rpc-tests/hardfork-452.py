#!/usr/bin/env python3
# Copyright (c) 2018 The Navcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

from test_framework.test_framework import NavcoinTestFramework
from test_framework.hardfork_util import *

import time

class Hardfork452(NavcoinTestFramework):
    """Tests rejection of obsolete blocks."""

    def __init__(self):
        super().__init__()
        self.setup_clean_chain = True
        self.num_nodes = 1

    def setup_network(self, split=False):
        self.nodes = self.setup_nodes()
        self.is_network_split = split

    def run_test(self):
        # test
        activateHardFork(self.nodes[0], 21, 1000)


if __name__ == '__main__':
    Hardfork452().main()
