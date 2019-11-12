#!/usr/bin/env python3
# Copyright (c) 2018 The Navcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

from test_framework.test_framework import NavCoinTestFramework
from test_framework.cfund_util import *

import urllib.parse

class CfundForkReorg(NavCoinTestFramework):
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

        # Active cfund
        activate_cfund(self.nodes[0])
        self.nodes[0].donatefund(100000)

        # Details for proposal
        paymentAddress = self.nodes[0].getnewaddress()
        proposalAmount = 10000

        # Create the proposal and save the id/hash
        proposalHash = self.nodes[0].createproposal(paymentAddress, proposalAmount, 36000, "test")["hash"]
        end_cycle(self.nodes[0])
        sync_blocks(self.nodes)

        # Make the node vote yes and generate blocks to accept it
        self.nodes[0].proposalvote(proposalHash, "yes")
        slow_gen(self.nodes[0], 1)
        end_cycle(self.nodes[0])
        sync_blocks(self.nodes)

        # disconnect the nodes and generate the proposal on each node
        url = urllib.parse.urlparse(self.nodes[0].url)
        self.nodes[0].disconnectnode(url.hostname+":"+str(p2p_port(1)))
        self.nodes[1].disconnectnode(url.hostname+":"+str(p2p_port(0)))
        time.sleep(2) # Wait for the nodes to disconnect

        # Create Payment request raw hex
        paymentHex = self.send_raw_paymentrequest(proposalAmount, paymentAddress, proposalHash, "test")

        # Broadcast on node 0
        paymentHash0 = self.nodes[0].sendrawtransaction(paymentHex)
        end_cycle(self.nodes[0])

        # Boardcast on node 1
        paymentHash1 = self.nodes[1].sendrawtransaction(paymentHex)
        end_cycle(self.nodes[1])

        # Assert that both hashes for payment request are identical
        assert(paymentHash0 == paymentHash1)

        # Make the node vote yes and generate blocks to accept it on node 0
        self.nodes[0].paymentrequestvote(paymentHash0, "yes")
        slow_gen(self.nodes[0], 1)
        end_cycle(self.nodes[0])

        # Make the node vote yes and generate blocks to accept it on node 1
        self.nodes[1].paymentrequestvote(paymentHash1, "yes")
        slow_gen(self.nodes[1], 1)
        end_cycle(self.nodes[1])

        # Now makre sure that both nodes are on different chains
        assert(self.nodes[0].getbestblockhash() != self.nodes[1].getbestblockhash())

        # Generate 10 more blocks on node 1
        # This should force reorg on node 0
        slow_gen(self.nodes[1], 10)

        # Reconnect the nodes
        connect_nodes_bi(self.nodes, 0, 1)

        # Wait for nodes to sync
        sync_blocks(self.nodes)

        # Now check that the hash for both nodes are the same
        assert(self.nodes[0].getbestblockhash() == self.nodes[1].getbestblockhash())

    def send_raw_paymentrequest(self, amount, address, proposal_hash, description):
        amount = amount * 100000000
        privkey = self.nodes[0].dumpprivkey(address)
        message = "I kindly ask to withdraw " + str(amount) + "NAV from the proposal " + proposal_hash + ". Payment request id: " + str(description)
        signature = self.nodes[0].signmessagewithprivkey(privkey, message)

        # Create a raw payment request
        raw_payreq_tx = self.nodes[0].createrawtransaction(
            [],
            {"6ac1": 1},
            json.dumps({"v": 2, "h": proposal_hash, "n": amount, "s": signature, "i": description})
        )

        # Modify version for payreq creation
        raw_payreq_tx = "05" + raw_payreq_tx[2:]

        # Fund raw transacion
        raw_payreq_tx = self.nodes[0].fundrawtransaction(raw_payreq_tx)['hex']

        # Sign raw transacion
        raw_payreq_tx = self.nodes[0].signrawtransaction(raw_payreq_tx)['hex']

        # Return the raw hex
        return raw_payreq_tx

if __name__ == '__main__':
    CfundForkReorg().main()
