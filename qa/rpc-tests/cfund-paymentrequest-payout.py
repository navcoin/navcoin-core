#!/usr/bin/env python3
# Copyright (c) 2018 The Navcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

from test_framework.test_framework import NavCoinTestFramework
from test_framework.cfund_util import *

import time
import http.client
import urllib.parse

class CommunityFundPaymentRequestPayout(NavCoinTestFramework):
    """Tests the payment request procedures of the Community fund."""

    def __init__(self):
        super().__init__()
        self.setup_clean_chain = True
        self.num_nodes = 2

    def setup_network(self, split=False):
        self.nodes = []

        self.nodes.append(start_node(0, self.options.tmpdir, []))
        self.nodes.append(start_node(1, self.options.tmpdir, []))

        connect_nodes(self.nodes[0], 1)

        self.is_network_split = split

    def run_test(self):

        activate_cfund(self.nodes[0])
        self.nodes[0].donatefund(100000)

        sync_blocks(self.nodes)

        balance = self.nodes[0].getbalance()

        balanceSplit = float(balance / self.num_nodes)

        addresses = []

        for x in range(self.num_nodes):
            addresses.append(self.nodes[x].getnewaddress())
            self.nodes[0].sendtoaddress(addresses[x], balanceSplit)

        slow_gen(self.nodes[0], 1)

        sync_blocks(self.nodes)

        for x in range(self.num_nodes):
            assert(self.nodes[x].getbalance() >= balanceSplit)

        SATOSHI = 100000000
        paymentAddress = self.nodes[0].getnewaddress()
        proposalAmount = 10000
        payoutAmount = 3333

        # Create a proposal and accept by voting

        proposalid0 = self.nodes[0].createproposal(paymentAddress, proposalAmount, 36000, "test")["hash"]
        locked_before = self.nodes[0].cfundstats()["funds"]["locked"]
        end_cycle(self.nodes[0])
        sync_blocks(self.nodes)

        self.nodes[0].proposalvote(proposalid0, "yes")
        slow_gen(self.nodes[0], 1)
        end_cycle(self.nodes[0])

        locked_accepted = self.nodes[0].cfundstats()["funds"]["locked"]

        sync_blocks(self.nodes)

        # Proposal should be accepted

        for x in range(self.num_nodes):
            assert(self.nodes[x].getproposal(proposalid0)["state"] == 1)
            assert(self.nodes[x].getproposal(proposalid0)["status"] == "accepted")
            assert(self.nodes[x].cfundstats()["funds"]["locked"] == float(locked_before) + float(self.nodes[x].getproposal(proposalid0)["requestedAmount"]))

        # Create 1 payment request

        paymentReq = self.nodes[0].createpaymentrequest(proposalid0, payoutAmount, "test0")["hash"]

        slow_gen(self.nodes[0], 1)
        sync_blocks(self.nodes)

        for x in range(self.num_nodes):
            assert(self.nodes[x].getpaymentrequest(paymentReq)["state"] == 0)
            assert(self.nodes[x].getpaymentrequest(paymentReq)["status"] == "pending")
            assert(self.nodes[x].cfundstats()["funds"]["locked"] == locked_accepted)

        end_cycle(self.nodes[0])

        # vote yes for the payment request

        self.nodes[0].paymentrequestvote(paymentReq, "yes")

        slow_gen(self.nodes[0], 1)
        end_cycle(self.nodes[0])
        sync_blocks(self.nodes)

        for x in range(self.num_nodes):
            assert(self.nodes[x].getpaymentrequest(paymentReq)["state"] == 1)

        wallet_info1 = self.nodes[0].getwalletinfo()

        while self.nodes[0].getpaymentrequest(paymentReq)["state"] != 6:
            blocks = slow_gen(self.nodes[0], 1)

        sync_blocks(self.nodes)

        wallet_info2 = self.nodes[0].getwalletinfo()

        # check all wallets see the payout

        for x in range(self.num_nodes):
            payoutBlockHash = self.nodes[x].getpaymentrequest(paymentReq)["stateChangedOnBlock"]
            payoutBlock = self.nodes[x].getblock(payoutBlockHash)
            payoutHex = self.nodes[x].getrawtransaction(payoutBlock["tx"][0])
            payoutTx = self.nodes[x].decoderawtransaction(payoutHex)
            assert(payoutTx["vout"][1]["valueSat"] == payoutAmount * SATOSHI)
            assert(payoutTx["vout"][1]["scriptPubKey"]["addresses"][0] == paymentAddress)

        # check node zero received the payout

        lastTransactions = self.nodes[0].listtransactions("", 2)

        assert(lastTransactions[0]["address"] == paymentAddress)
        assert(lastTransactions[0]["category"] == "immature")
        assert(lastTransactions[0]["amount"] == payoutAmount)
        assert(lastTransactions[0]["confirmations"] == 1)

        # roll back the block and try the other node
        for x in range(self.num_nodes):
            self.nodes[x].invalidateblock(blocks[-1])

        sync_blocks(self.nodes)

        bestBlockHash = self.nodes[0].getbestblockhash()
        for x in range(self.num_nodes):
            assert(self.nodes[x].getbestblockhash() == bestBlockHash)
            assert(self.nodes[x].getpaymentrequest(paymentReq)["state"] == 1)

        while self.nodes[1].getpaymentrequest(paymentReq)["state"] != 6:
            blocks = slow_gen(self.nodes[1], 1)

        sync_blocks(self.nodes)

        for x in range(self.num_nodes):
            payoutBlockHash = self.nodes[x].getpaymentrequest(paymentReq)["stateChangedOnBlock"]
            payoutBlock = self.nodes[x].getblock(payoutBlockHash)
            payoutHex = self.nodes[x].getrawtransaction(payoutBlock["tx"][0])
            payoutTx = self.nodes[x].decoderawtransaction(payoutHex)
            assert(payoutTx["vout"][1]["valueSat"] == payoutAmount * SATOSHI)
            assert(payoutTx["vout"][1]["scriptPubKey"]["addresses"][0] == paymentAddress)

        # roll back the block and try both nodes
        for x in range(self.num_nodes):
            self.nodes[x].invalidateblock(blocks[-1])

        sync_blocks(self.nodes)

        bestBlockHash = self.nodes[0].getbestblockhash()

        for x in range(self.num_nodes):
            assert(self.nodes[x].getbestblockhash() == bestBlockHash)
            assert(self.nodes[x].getpaymentrequest(paymentReq)["state"] == 1)
            assert(self.nodes[x].getpaymentrequest(paymentReq)["stateChangedOnBlock"] != "0000000000000000000000000000000000000000000000000000000000000000")

        # disconnect the nodes and generate the payout on each node
        url = urllib.parse.urlparse(self.nodes[1].url)
        self.nodes[0].disconnectnode(url.hostname+":"+str(p2p_port(1)))

        time.sleep(1)

        while self.nodes[0].getpaymentrequest(paymentReq)["state"] != 6:
            slow_gen(self.nodes[0], 1)

        while self.nodes[1].getpaymentrequest(paymentReq)["state"] != 6:
            slow_gen(self.nodes[1], 1)

        slow_gen(self.nodes[1], 1)
        connect_nodes_bi(self.nodes,0,1) #reconnect the node

        sync_blocks(self.nodes)

        payoutBlockHash = self.nodes[0].getpaymentrequest(paymentReq)["stateChangedOnBlock"]

        # check both agree on the payout block
        for x in range(self.num_nodes):
            assert(self.nodes[x].getpaymentrequest(paymentReq)["stateChangedOnBlock"] == payoutBlockHash)


if __name__ == '__main__':
    CommunityFundPaymentRequestPayout().main()
