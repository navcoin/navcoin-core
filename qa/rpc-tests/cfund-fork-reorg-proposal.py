#!/usr/bin/env python3
# Copyright (c) 2018 The Navcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

from test_framework.test_framework import NavCoinTestFramework
from test_framework.cfund_util import *

import urllib.parse
import string
import random

class CfundForkReorgProposal(NavCoinTestFramework):
    """Tests that reorg with 2 chains with same proposals/requests does not cause a fork"""

    def __init__(self):
        super().__init__()
        self.setup_clean_chain = True
        self.num_nodes = 2

    def setup_network(self, split=False):
        self.nodes = self.setup_nodes()
        connect_nodes_bi(self.nodes, 0, 1)
        self.is_network_split = False

    def run_test(self):
        # Make sure the nodes are not staking
        self.nodes[0].staking(False)
        self.nodes[1].staking(False)

        # Active cfund & ensure donation is avaialable
        activate_cfund(self.nodes[0])

        self.nodes[0].donatefund(100000)
        slow_gen(self.nodes[1], 1000)
        sync_blocks(self.nodes)

        # Details for proposal
        paymentAddress = self.nodes[0].getnewaddress()
        proposalAmount = 10000
        proposalDeadline = 60*60*24*7

        # disconnect the nodes and generate the proposal on each node
        url = urllib.parse.urlparse(self.nodes[0].url)
        self.nodes[0].disconnectnode(url.hostname+":"+str(p2p_port(1)))
        self.nodes[1].disconnectnode(url.hostname+":"+str(p2p_port(0)))
        time.sleep(2) # Wait for the nodes to disconnect

        # Create the proposal and save the id/hash
        proposalHex = self.nodes[0].createproposal(paymentAddress, proposalAmount, proposalDeadline, "test", 1, True)

        # Broadcast on node 0
        slow_gen(self.nodes[0], 1)
        proposalHash0 = self.nodes[0].sendrawtransaction(proposalHex)
        slow_gen(self.nodes[0], 1)

        # Boardcast on node 1
        proposalHash1 = self.nodes[1].sendrawtransaction(proposalHex)
        slow_gen(self.nodes[1], 1)

        # Assert that both hashes for payment request are identical
        assert(proposalHash0 == proposalHash1)

        # Assert that both payment requests have been included in different block hashes
        assert(self.nodes[0].getproposal(proposalHash0)["blockHash"] != self.nodes[1].getproposal(proposalHash1)["blockHash"])

        # Make the node vote yes
        self.nodes[1].proposalvote(proposalHash0, "yes")

        # End cycle 0
        slow_gen(self.nodes[1], 1)
        end_cycle(self.nodes[1])

        # End cycle 1
        slow_gen(self.nodes[1], 1)
        end_cycle(self.nodes[1])

        # Now make sure that both nodes are on different chains
        assert(self.nodes[0].getbestblockhash() != self.nodes[1].getbestblockhash())

        # we save node 1 best block hash to check node 0 reorgs correctly
        best_block = self.nodes[1].getbestblockhash()

        self.stop_node(0)
        self.nodes[0] = start_node(0, self.options.tmpdir, [])

        # Reconnect the nodes
        connect_nodes_bi(self.nodes, 0, 1)

        # Wait for nodes to sync
        sync_blocks(self.nodes)

        # Now check that the hash for both nodes are the same
        assert_equal(self.nodes[0].getbestblockhash(), best_block)
        assert_equal(self.nodes[0].getbestblockhash(), self.nodes[1].getbestblockhash())
        assert_equal(self.nodes[0].getblock(self.nodes[0].getproposal(proposalHash0)["blockHash"]), self.nodes[1].getblock(self.nodes[1].getproposal(proposalHash0)["blockHash"]))
        assert_equal(self.nodes[0].getproposal(proposalHash0), self.nodes[1].getproposal(proposalHash0))

        # End cycle 2
        slow_gen(self.nodes[1], 1)
        end_cycle(self.nodes[1])
        sync_blocks(self.nodes)

        # Create Payment Request
        paymentAmount = 5000

        letters = string.ascii_lowercase + string.ascii_uppercase + "0123456789"
        paymentId = "".join(random.choice(letters) for i in range(32))

        preqHash = self.nodes[0].createpaymentrequest(proposalHash0, paymentAmount, paymentId)["hash"]
        slow_gen(self.nodes[0], 1)
        sync_blocks(self.nodes)

        # Vote Yes
        self.nodes[1].paymentrequestvote(preqHash, "yes")
        self.nodes[0].paymentrequestvote(preqHash, "yes")

        # End cycle 0
        slow_gen(self.nodes[1], 1)
        end_cycle(self.nodes[1])
        sync_blocks(self.nodes)

        # End cycle 1
        slow_gen(self.nodes[1], 1)
        end_cycle(self.nodes[1])
        sync_blocks(self.nodes)

        # End cycle 2
        slow_gen(self.nodes[1], 1)
        end_cycle(self.nodes[1])
        sync_blocks(self.nodes)

        # Verify both nodes accpeted the payment

        assert_equal(self.nodes[0].getpaymentrequest(preqHash)["status"], "accepted")
        assert_equal(self.nodes[0].getblock(self.nodes[0].getpaymentrequest(preqHash)["stateChangedOnBlock"]), self.nodes[1].getblock(self.nodes[1].getpaymentrequest(preqHash)["stateChangedOnBlock"]))
        assert_equal(self.nodes[0].getbestblockhash(), self.nodes[1].getbestblockhash())
        assert_equal(self.nodes[0].getblock(self.nodes[0].getpaymentrequest(preqHash)["blockHash"]), self.nodes[1].getblock(self.nodes[1].getpaymentrequest(preqHash)["blockHash"]))
        assert_equal(self.nodes[0].getpaymentrequest(preqHash), self.nodes[1].getpaymentrequest(preqHash))

        # Verify the payment was actually received
        paidBlock = self.nodes[0].getblock(self.nodes[0].getpaymentrequest(preqHash)["stateChangedOnBlock"])
        unspent = self.nodes[0].listunspent(0, 80)

        assert_equal(unspent[0]['address'], paymentAddress)
        assert_equal(unspent[0]['amount'], paymentAmount)
        assert_equal(paidBlock['tx'][0], unspent[0]['txid'])

if __name__ == '__main__':
    CfundForkReorgProposal().main()
