#!/usr/bin/env python3
# Copyright (c) 2019 The Navcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

from test_framework.test_framework import NavCoinTestFramework
from test_framework.cfund_util import *

import urllib.parse

class CommunityFundPaymentRequestReorg(NavCoinTestFramework):
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
        self.nodes[0].donatefund(100000)
        slow_gen(self.nodes[0], 100)
        time.sleep(2) # wait for blocks to generate

        sync_blocks(self.nodes)

        rawproposal = self.nodes[0].createproposal(self.nodes[0].getnewaddress(), 10, 360000, "test", 50, True)

        # disconnect the nodes and generate the proposal on each node
        url = urllib.parse.urlparse(self.nodes[1].url)
        self.nodes[0].disconnectnode(url.hostname+":"+str(p2p_port(1)))

        time.sleep(2) # wait for disconnect

        hash = self.nodes[0].sendrawtransaction(rawproposal)
        self.nodes[1].sendrawtransaction(rawproposal)

        self.nodes[0].generate(1)
        self.nodes[1].generate(2)

        blockHash1 = self.nodes[0].getproposal(hash)["blockHash"]
        blockHash2 = self.nodes[1].getproposal(hash)["blockHash"]

        assert(blockHash1 != blockHash2)

        time.sleep(1)

        connect_nodes_bi(self.nodes, 0, 1)
        sync_blocks(self.nodes)

        blockHash1a = self.nodes[0].getproposal(hash)["blockHash"]
        blockHash2a = self.nodes[1].getproposal(hash)["blockHash"]

        assert(blockHash1a == blockHash2a)

        assert(self.nodes[0].getproposal(hash) == self.nodes[1].getproposal(hash))
        assert(self.nodes[0].getproposal(hash)['hash'] == hash)

        # get the proposal to be accepted
        end_cycle(self.nodes[0])
        sync_blocks(self.nodes)

        self.nodes[0].proposalvote(hash, "yes")
        slow_gen(self.nodes[0], 1)
        end_cycle(self.nodes[0])

        sync_blocks(self.nodes)

        assert(self.nodes[0].getproposal(hash)["status"] == "accepted")
        assert(self.nodes[1].getproposal(hash)["status"] == "accepted")

        # disconnect the nodes and generate the payment request
        url = urllib.parse.urlparse(self.nodes[1].url)
        self.nodes[0].disconnectnode(url.hostname+":"+str(p2p_port(1)))

        time.sleep(2) # wait for disconnect

        paymentReq1 = self.nodes[0].createpaymentrequest(hash, 10, "paymentReq1")["hash"]

        self.nodes[0].generate(1)
        self.nodes[1].generate(2)

        longestChain = self.nodes[1].getbestblockhash()

        preq1 = self.nodes[0].getpaymentrequest(paymentReq1)

        try:
          preq2 = self.nodes[1].getpaymentrequest(paymentReq1)
          assert(preq1 != preq2)
          assert(False) # should not be able to get here because RPC error thrown
        except:
          assert(True)

        print(preq1)

        # reorg to node 1 and payment request should not exist
        connect_nodes_bi(self.nodes, 0, 1)
        sync_blocks(self.nodes)

        assert(self.nodes[0].getbestblockhash() == longestChain)
        assert(self.nodes[0].getbestblockhash() == self.nodes[1].getbestblockhash())

        self.nodes[0].paymentrequestvote(paymentReq1, "yes")

        slow_gen(self.nodes[0], 1)
        end_cycle(self.nodes[0])
        sync_blocks(self.nodes)

        print(self.nodes[0].getpaymentrequest(paymentReq1))
        print(self.nodes[1].getpaymentrequest(paymentReq1))

        slow_gen(self.nodes[0], 1)
        end_cycle(self.nodes[0])
        sync_blocks(self.nodes)

        print(self.nodes[0].getpaymentrequest(paymentReq1))
        print(self.nodes[1].getpaymentrequest(paymentReq1))

        print(self.nodes[0].getblock(self.nodes[0].getpaymentrequest(paymentReq1)["blockHash"]))
        print(self.nodes[1].getblock(self.nodes[0].getpaymentrequest(paymentReq1)["blockHash"]))

if __name__ == '__main__':
    CommunityFundPaymentRequestReorg().main()
