#!/usr/bin/env python3
# Copyright (c) 2019 The Navcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

from test_framework.test_framework import NavCoinTestFramework
from test_framework.util import *

class ColdStakingFeeTest(NavCoinTestFramework):
    def __init__(self):
        super().__init__()
        self.setup_clean_chain = True
        self.num_nodes = 3

    def setup_network(self, split=False):
        self.nodes = []
        self.nodes.append(start_node(0, self.options.tmpdir, ["-addressindex","-pooladdress=n1hcSEk4ReyLwbStTKodGCq4kEwhJVxXwC","-poolfee=50"]))
        self.nodes.append(start_node(1, self.options.tmpdir, ["-addressindex","-pooladdress=n1hcSEk4ReyLwbStTKodGCq4kEwhJVxXwC","-poolfee=100"]))
        self.nodes.append(start_node(2, self.options.tmpdir, ["-addressindex","-pooladdress=n1hcSEk4ReyLwbStTKodGCq4kEwhJVxXwC","-poolfee=101"]))

    def run_test(self):
        slow_gen(self.nodes[0], 300)
        assert (get_bip9_status(self.nodes[0], "coldstaking")["status"] == "active")
        slow_gen(self.nodes[1], 300)
        assert (get_bip9_status(self.nodes[1], "coldstaking")["status"] == "active")
        slow_gen(self.nodes[2], 300)
        assert (get_bip9_status(self.nodes[2], "coldstaking")["status"] == "active")

        csaddress1 = self.nodes[0].getcoldstakingaddress(self.nodes[0].getnewaddress(),"n1wgKgwFPZYcQrm8qBtPrBvz2piqCwc1ry")
        csaddress2 = self.nodes[1].getcoldstakingaddress(self.nodes[1].getnewaddress(),"n1wgKgwFPZYcQrm8qBtPrBvz2piqCwc1ry")
        csaddress3 = self.nodes[2].getcoldstakingaddress(self.nodes[2].getnewaddress(),"n1wgKgwFPZYcQrm8qBtPrBvz2piqCwc1ry")

        self.nodes[0].sendtoaddress(csaddress1, self.nodes[0].getbalance(), "", "", "", True)
        self.nodes[1].sendtoaddress(csaddress2, self.nodes[1].getbalance(), "", "", "", True)
        self.nodes[2].sendtoaddress(csaddress3, self.nodes[2].getbalance(), "", "", "", True)

        self.nodes[0].generatetoaddress(100, "n1wgKgwFPZYcQrm8qBtPrBvz2piqCwc1ry")
        self.nodes[1].generatetoaddress(100, "n1wgKgwFPZYcQrm8qBtPrBvz2piqCwc1ry")
        self.nodes[2].generatetoaddress(100, "n1wgKgwFPZYcQrm8qBtPrBvz2piqCwc1ry")

        tx=self.nodes[0].sendtoaddress(csaddress1, self.nodes[0].getbalance(), "", "", "", True)
        fees0=self.nodes[0].gettransaction(tx)["fee"]*100000000/2
        tx2=self.nodes[1].sendtoaddress(csaddress2, self.nodes[1].getbalance(), "", "", "", True)
        fees1=self.nodes[1].gettransaction(tx2)["fee"]*100000000
        self.nodes[2].sendtoaddress(csaddress3, self.nodes[2].getbalance(), "", "", "", True)

        while self.nodes[0].getblockcount() < 401:
            time.sleep(1)

        node0balance = self.nodes[0].getaddressbalance('n1hcSEk4ReyLwbStTKodGCq4kEwhJVxXwC')["balance"]

        while self.nodes[1].getblockcount() < 401:
            time.sleep(1)

        node1balance = self.nodes[1].getaddressbalance('n1hcSEk4ReyLwbStTKodGCq4kEwhJVxXwC')["balance"]

        assert(node0balance > 0)
        assert(node1balance > 0)
        assert_equal((node0balance+fees0) % 100000000, 0)
        assert_equal((node1balance+fees1) % 200000000, 0)

        while "Coinstake tried to move cold staking coins to a non authorised script" not in open(self.options.tmpdir + '/node2/devnet/debug.log').read():
            time.sleep(1)

        assert("Coinstake tried to move cold staking coins to a non authorised script" in open(self.options.tmpdir + '/node2/devnet/debug.log').read())

if __name__ == '__main__':
    ColdStakingFeeTest().main()
