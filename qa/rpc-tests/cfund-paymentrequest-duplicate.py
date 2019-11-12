#!/usr/bin/env python3
# Copyright (c) 2018 The Navcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

from test_framework.test_framework import NavCoinTestFramework
from test_framework.cfund_util import *

import time
import http.client
import urllib.parse

class CommunityFundPaymentRequestDuplicate(NavCoinTestFramework):
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

        proposals = [{},{},{},{},{}]

        # internships
        proposals[0]["address"] = self.nodes[0].getnewaddress()
        proposals[0]["amount"] = 10000
        proposals[0]["preqs"] = [{"amount": 3333}]

        # beekart posters
        proposals[1]["address"] = self.nodes[0].getnewaddress()
        proposals[1]["amount"] = 5000
        proposals[1]["preqs"] = [{"amount": 5000}]

        # vinyl stickers
        proposals[2]["address"] = self.nodes[0].getnewaddress()
        proposals[2]["amount"] = 2000
        proposals[2]["preqs"] = [{"amount": 2000}]

        # nio italia
        proposals[3]["address"] = self.nodes[0].getnewaddress()
        proposals[3]["amount"] = 1000
        proposals[3]["preqs"] = [{"amount": 1000}]

        # cryptocandor video
        proposals[4]["address"] = self.nodes[0].getnewaddress()
        proposals[4]["amount"] = 5500
        proposals[4]["preqs"] = [{"amount": 2500}, {"amount": 2000}]

        # Create proposals
        for proposal in proposals:
            proposal["proposalHash"] = self.nodes[0].createproposal(proposal["address"], proposal["amount"], 36000, "proposal description")["hash"]

        end_cycle(self.nodes[0])
        sync_blocks(self.nodes)

        # Accept the proposals
        for proposal in proposals:
            self.nodes[0].proposalvote(proposal["proposalHash"], "yes")

        slow_gen(self.nodes[0], 1)
        end_cycle(self.nodes[0])

        locked_accepted = self.nodes[0].cfundstats()["funds"]["locked"]

        sync_blocks(self.nodes)

        # Check proposals are accepted on all nodes
        for x in range(self.num_nodes):
            locked_tallied = 0
            for proposal in proposals:
                assert(self.nodes[x].getproposal(proposal["proposalHash"])["state"] == 1)
                assert(self.nodes[x].getproposal(proposal["proposalHash"])["status"] == "accepted")
                locked_tallied += float(self.nodes[x].getproposal(proposal["proposalHash"])["requestedAmount"])
            assert(locked_accepted == locked_tallied)

        # Create payment requests for all the proposals

        for proposal in proposals:
            for preq in proposal["preqs"]:
                preq["hash"] = self.nodes[0].createpaymentrequest(proposal["proposalHash"], preq["amount"], "payment request description")["hash"]

        slow_gen(self.nodes[0], 1)
        sync_blocks(self.nodes)

        # Check payment requests are present on all nodes

        for x in range(self.num_nodes):
            for proposal in proposals:
                for preq in proposal["preqs"]:
                    assert(self.nodes[x].getpaymentrequest(preq["hash"])["state"] == 0)
                    assert(self.nodes[x].getpaymentrequest(preq["hash"])["status"] == "pending")
                    assert(self.nodes[x].cfundstats()["funds"]["locked"] == locked_accepted)

        end_cycle(self.nodes[0])

        # vote yes for the payment requests

        for proposal in proposals:
            for preq in proposal["preqs"]:
                 self.nodes[0].paymentrequestvote(preq["hash"], "yes")

        slow_gen(self.nodes[0], 1)
        end_cycle(self.nodes[0])
        sync_blocks(self.nodes)

        for x in range(self.num_nodes):
            for proposal in proposals:
                for preq in proposal["preqs"]:
                    assert(self.nodes[x].getpaymentrequest(preq["hash"])["state"] == 1)
                    assert(self.nodes[x].getpaymentrequest(preq["hash"])["paidOnBlock"] == "0000000000000000000000000000000000000000000000000000000000000000")



        wallet_info1 = self.nodes[0].getwalletinfo()

        while self.nodes[0].getpaymentrequest(proposals[0]["preqs"][0]["hash"])["paidOnBlock"] == "0000000000000000000000000000000000000000000000000000000000000000":
            blocks = slow_gen(self.nodes[0], 1)

        sync_blocks(self.nodes)

        wallet_info2 = self.nodes[0].getwalletinfo()

        # check all wallets see the payout


        for x in range(self.num_nodes):
            paymentsFound = 0
            preqsFound = 0
            for proposal in proposals:
                for preq in proposal["preqs"]:
                    preqsFound += 1
                    payoutBlockHash = self.nodes[x].getpaymentrequest(preq["hash"])["paidOnBlock"]
                    payoutBlock = self.nodes[x].getblock(payoutBlockHash)
                    payoutHex = self.nodes[x].getrawtransaction(payoutBlock["tx"][0])
                    payoutTx = self.nodes[x].decoderawtransaction(payoutHex)

                    for vout in payoutTx["vout"]:
                        if vout["valueSat"] == preq["amount"] * SATOSHI and vout["scriptPubKey"]["addresses"][0] == proposal["address"]:
                            paymentsFound += 1

            assert(paymentsFound == 6)
            assert(preqsFound == 6)

        # check node zero received the payout

        lastTransactions = self.nodes[0].listtransactions("", 7)

        # print(lastTransactions)
        txFound = 0
        preqsFound = 0

        for tx in lastTransactions:
            for proposal in proposals:
                for preq in proposal["preqs"]:
                    preqsFound += 1
                    if tx["address"] == proposal["address"] and int(tx["amount"] * SATOSHI) == int(preq["amount"] * SATOSHI):
                        assert(tx["category"] == "immature")
                        assert(tx["confirmations"] == 1)
                        txFound += 1

        assert(txFound == 6)

        # disconnect the nodes mine blocks on each
        url = urllib.parse.urlparse(self.nodes[1].url)
        self.nodes[0].disconnectnode(url.hostname+":"+str(p2p_port(1)))

        blocks0 = self.nodes[0].getblockchaininfo()["blocks"]
        blocks1 = self.nodes[1].getblockchaininfo()["blocks"]

        slow_gen(self.nodes[0], 2)
        slow_gen(self.nodes[1], 1)

        assert(self.nodes[0].getblockchaininfo()["blocks"] == blocks0 + 2)
        assert(self.nodes[1].getblockchaininfo()["blocks"] == blocks1 + 1)
        assert(self.nodes[0].getbestblockhash() != self.nodes[1].getbestblockhash())

        # reconnect the node making node 1 reorg
        connect_nodes_bi(self.nodes,0,1)
        sync_blocks(self.nodes)

        assert(self.nodes[0].getblockchaininfo()["blocks"] == blocks0 + 2)
        assert(self.nodes[1].getblockchaininfo()["blocks"] == blocks1 + 2)
        assert(self.nodes[0].getbestblockhash() == self.nodes[1].getbestblockhash())

        slow_gen(self.nodes[1], 1)

        bestblockHash = self.nodes[1].getbestblockhash()
        bestBlock = self.nodes[1].getblock(bestblockHash)

        # Check that the only tx in the block is the block reward

        assert(len(bestBlock["tx"]) == 1)

if __name__ == '__main__':
    CommunityFundPaymentRequestDuplicate().main()
