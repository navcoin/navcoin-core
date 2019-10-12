#!/usr/bin/env python3
# Copyright (c) 2019 The Navcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

from test_framework.test_framework import NavCoinTestFramework
from test_framework.cfund_util import *

import urllib.parse

class CommunityFundProposalReorg(NavCoinTestFramework):
    def __init__(self):
        super().__init__()
        self.setup_clean_chain = True
        self.num_nodes = 2

    def setup_network(self, split=False):
        self.nodes = []

        self.nodes.append(start_node(0, self.options.tmpdir, []))
        self.nodes.append(start_node(1, self.options.tmpdir, []))

        connect_nodes_bi(self.nodes, 0, 1)

        self.is_network_split = split

    def run_test(self):

        activate_cfund(self.nodes[0])
        sync_blocks(self.nodes)

        rawproposal = self.nodes[0].createproposal(self.nodes[0].getnewaddress(), 10, 36000, "test", 50, True)

        # disconnect the nodes and generate the proposal on each node
        url = urllib.parse.urlparse(self.nodes[1].url)
        self.nodes[0].disconnectnode(url.hostname+":"+str(p2p_port(1)))

        time.sleep(2) # wait for disconnect

        hash = self.nodes[0].sendrawtransaction(rawproposal)
        self.nodes[1].sendrawtransaction(rawproposal)

        self.nodes[0].generate(1)
        self.nodes[1].generate(2)

        assert(self.nodes[0].getproposal(hash)['blockHash'] != self.nodes[1].getproposal(hash)['blockHash'])

        connect_nodes_bi(self.nodes, 0, 1)
        sync_blocks(self.nodes)

        assert(self.nodes[0].getproposal(hash) == self.nodes[1].getproposal(hash))
        assert(self.nodes[0].getproposal(hash)['hash'] == hash)

if __name__ == '__main__':
    CommunityFundProposalReorg().main()
