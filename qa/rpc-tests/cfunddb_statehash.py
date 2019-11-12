#!/usr/bin/env python3
# Copyright (c) 2019 The NavCoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.


from test_framework.test_framework import NavCoinTestFramework
from test_framework.util import *
import urllib.parse

class CFundDBStateHash(NavCoinTestFramework):
    def __init__(self):
        super().__init__()
        self.num_nodes = 2
        self.setup_clean_chain = True

    def setup_network(self):
        self.nodes = []
        self.nodes.append(start_node(0, self.options.tmpdir, ["-fakecfunddbstatehash=93020ecd66b76b08a3e62181151c2bce169003e85c4bdc63170904f1d0669258"]))
        self.nodes.append(start_node(1, self.options.tmpdir, ["-cfunddbstatehashthreshold=30", "-cfunddbstatehashcount=60"]))
        connect_nodes(self.nodes[0], 1)

    def run_test(self):
        self.nodes[1].generate(300)
        self.nodes[1].createproposal(self.nodes[1].getnewaddress(), 100, 1000, "test")
        self.nodes[1].generate(10)
        sync_blocks(self.nodes)

        assert_equal(self.nodes[0].getcfunddbstatehash(), self.nodes[1].getcfunddbstatehash())

        self.nodes[0].generate(30)
        sync_blocks(self.nodes)

        assert_equal(self.nodes[1].getinfo()['errors'],"")

        self.nodes[0].generate(1)
        sync_blocks(self.nodes)

        assert_equal(self.nodes[1].getinfo()['errors'],"It looks like your state database might be corrupted. Please close the wallet and reindex.")


if __name__ == '__main__':
    CFundDBStateHash().main()
