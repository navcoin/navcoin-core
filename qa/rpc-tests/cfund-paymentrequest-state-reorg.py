#!/usr/bin/env python3
# Copyright (c) 2019 The Navcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

from test_framework.test_framework import NavCoinTestFramework
from test_framework.cfund_util import *

import time
import urllib.parse


class CFundPaymentRequestStateReorg(NavCoinTestFramework):
    """Tests consistency of Community Fund Payment Requests state through reorgs."""

    def __init__(self):
        super().__init__()
        self.setup_clean_chain = True
        self.num_nodes = 2

    def setup_network(self, split=False):
        self.nodes = []
        self.nodes.append(start_node(0, self.options.tmpdir, ["-debug","-headerspamfiltermaxsize=1000"]))
        self.nodes.append(start_node(1, self.options.tmpdir, ["-debug","-headerspamfiltermaxsize=1000"]))
        connect_nodes(self.nodes[0], 1)
        self.is_network_split = False

    def run_test(self):
        self.nodes[0].staking(False)
        self.nodes[1].staking(False)
        activate_cfund(self.nodes[0])
        self.sync_all()

        self.nodes[0].donatefund(100)

        # Generate our addresses
        node_0_address = self.nodes[0].getnewaddress()
        node_1_address = self.nodes[1].getnewaddress()

        # Split funds
        self.nodes[0].sendtoaddress(node_1_address, 5000000)
        proposal=self.nodes[0].createproposal(node_0_address, 100, 36000, "test")
        proposal_id=proposal["hash"]

        slow_gen(self.nodes[0], 1)
        end_cycle(self.nodes[0])

        self.sync_all()

        self.nodes[0].proposalvote(proposal_id, "yes")

        slow_gen(self.nodes[0], 1)
        end_cycle(self.nodes[0])

        self.sync_all()

        assert(self.nodes[0].getproposal(proposal_id)["state"] == 1)
        assert(self.nodes[0].getproposal(proposal_id)["status"] == "accepted")

        assert(self.nodes[1].getproposal(proposal_id)["state"] == 1)
        assert(self.nodes[1].getproposal(proposal_id)["status"] == "accepted")

        raw_preq = self.nodes[0].createpaymentrequest(proposal_id, 100, "preq", 1, True)["raw"]
        self.sync_all()

        # Disconnect Nodes 0 and 1
        url = urllib.parse.urlparse(self.nodes[1].url)
        self.nodes[0].disconnectnode(url.hostname+":"+str(p2p_port(1)))

        self.nodes[0].forcetransactions([raw_preq])
        self.nodes[1].forcetransactions([raw_preq])

        blockcount_0 = self.nodes[0].getblockcount()
        blockcount_1 = self.nodes[1].getblockcount()

        self.nodes[0].staking(True)
        self.nodes[1].staking(True)

        # Let's wait for at least 20 blocks from Node 0
        while self.nodes[0].getblockcount() - blockcount_0 < 3:
            time.sleep(1)

        # Node 1 only has 1 output so it will only stake 1 block
        while self.nodes[1].getblockcount() == blockcount_1:
            time.sleep(1)

        print("nodes staked!")

        self.nodes[0].staking(False)
        self.nodes[1].staking(False)

        node_0_best_hash = self.nodes[0].getblockhash(self.nodes[0].getblockcount())
        node_1_best_hash = self.nodes[1].getblockhash(self.nodes[1].getblockcount())

        # Node 0 and Node 1 have forked.
        assert(node_0_best_hash != node_1_best_hash)

        connect_nodes(self.nodes[0], 1)

        self.sync_all()

        node_1_best_hash_ = self.nodes[1].getblockhash(self.nodes[1].getblockcount())

        # Node 1 must have reorg'd to Node 0 chain
        assert(node_0_best_hash == node_1_best_hash_)

if __name__ == '__main__':
    CFundPaymentRequestStateReorg().main()
