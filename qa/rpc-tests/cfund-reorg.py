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

        self.nodes.append(start_node(0, self.options.tmpdir, ["-debug=cfund"]))
        self.nodes.append(start_node(1, self.options.tmpdir, ["-debug=cfund"]))

        connect_nodes_bi(self.nodes, 0, 1)

        self.is_network_split = split

    def run_test(self):
        self.nodes[0].staking(False)
        self.nodes[0].staking(False)

        activate_cfund(self.nodes[0])
        sync_blocks(self.nodes)

        self.nodes[0].donatefund(10)
        slow_gen(self.nodes[0], 1)

        rawproposal = self.nodes[0].createproposal(self.nodes[0].getnewaddress(), 10, 36000, "test", 50, True)["raw"]

        # disconnect the nodes and generate the proposal on each node
        url = urllib.parse.urlparse(self.nodes[1].url)
        self.nodes[0].disconnectnode(url.hostname+":"+str(p2p_port(1)))

        time.sleep(2) # wait for disconnect

        hash = self.nodes[0].sendrawtransaction(rawproposal)
        self.nodes[1].sendrawtransaction(rawproposal)

        self.nodes[0].generate(1)
        self.nodes[1].generate(2)

        connect_nodes_bi(self.nodes, 0, 1)
        sync_blocks(self.nodes)

        assert_equal(self.nodes[0].getproposal(hash), self.nodes[1].getproposal(hash))

        end_cycle(self.nodes[0])

        self.nodes[0].proposalvote(hash, "yes")
        slow_gen(self.nodes[0], 1)
        end_cycle(self.nodes[0])

        sync_blocks(self.nodes)

        assert_equal(self.nodes[0].getproposal(hash)['status'], 'accepted')

        rawpaymentrequest = self.nodes[0].createpaymentrequest(hash, 10, "paymentReq1", True)["raw"]

        # disconnect the nodes and generate the proposal on each node
        url = urllib.parse.urlparse(self.nodes[1].url)
        self.nodes[0].disconnectnode(url.hostname+":"+str(p2p_port(1)))

        time.sleep(2) # wait for disconnect

        hash = self.nodes[0].sendrawtransaction(rawpaymentrequest)
        assert_equal(self.nodes[1].sendrawtransaction(rawpaymentrequest), hash)

        self.nodes[0].generate(1)
        self.nodes[1].generate(2)

        blockhash0 = self.nodes[0].getpaymentrequest(hash)["blockHash"]
        blockhash1 = self.nodes[1].getpaymentrequest(hash)["blockHash"]

        longestChain = self.nodes[1].getbestblockhash()

        preq1 = self.nodes[0].getpaymentrequest(hash)

        # I would have assumed reorg to node 1 should reorg the payment request and probably include it in the next block?
        connect_nodes_bi(self.nodes, 0, 1)
        sync_blocks(self.nodes)

        # check the nodes reorged to the longest chain (node 1)
        assert_equal(self.nodes[0].getbestblockhash(), longestChain)
        assert_equal(self.nodes[0].getbestblockhash(), self.nodes[1].getbestblockhash())

        assert_equal(self.nodes[0].getpaymentrequest(hash), self.nodes[1].getpaymentrequest(hash))
        assert_equal(self.nodes[0].getpaymentrequest(hash)['hash'], hash)

        assert_equal(self.nodes[0].getpaymentrequest(hash)["blockHash"], blockhash1)
        assert_equal(self.nodes[1].getpaymentrequest(hash)["blockHash"], blockhash1)



if __name__ == '__main__':
    CommunityFundProposalReorg().main()
