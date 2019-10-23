#!/usr/bin/env python3
# Copyright (c) 2018 The Navcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

from test_framework.test_framework import NavCoinTestFramework
from test_framework.util import *

#import time

class Confiscation(NavCoinTestFramework):
    """Tests confiscation of coins to the dao."""

    def __init__(self):
        super().__init__()
        self.setup_clean_chain = True
        self.num_nodes = 2

    def setup_network(self, split=False):
        self.nodes = []
        self.nodes = start_nodes(self.num_nodes, self.options.tmpdir, [["-txindex=1", "-voteconfiscation=1", "-staking=0"],["-txindex=1", "-voteconfiscation=1", "-staking=0"]])
        connect_nodes(self.nodes[0], 1)
        self.is_network_split = False

    def run_test(self):
        address = 'miPyKy95JPBs3pHJA7b3JAz9zSygURcbSb'
        privatekey = 'cTNhSjPk7xzq354aPtmhR4XCEpNxbadMEefovwFiSQivrwMYR13P'

        self.nodes[1].importprivkey(privatekey)

        slow_gen(self.nodes[0], 10)
        self.sync_all()

        slow_gen(self.nodes[0], 10)
        self.sync_all()

        txCanSpend=self.nodes[0].sendtoaddress(address, 10)
        slow_gen(self.nodes[0], 1)
        self.sync_all()

        raw = self.sweep_hash(self.nodes[1], txCanSpend, self.nodes[1].getnewaddress())
        if raw != "":
            txCanSpend=self.nodes[0].sendrawtransaction(raw)

        slow_gen(self.nodes[0], 9)
        self.sync_all()

        txCantSpend=self.nodes[0].sendtoaddress(address, 10)
        txCantSpend2=self.nodes[0].sendtoaddress(address, 10)

        slow_gen(self.nodes[0], 10)
        self.sync_all()

        for i in range(5):
            slow_gen(self.nodes[0], 1)
            raw = self.sweep_hash(self.nodes[1], txCantSpend2, self.nodes[1].getnewaddress())
            if raw != "":
                txCantSpend2=self.nodes[1].sendrawtransaction(raw)
            self.sync_all()

        slow_gen(self.nodes[0], 5)
        self.sync_all()

        assert_equal(self.nodes[0].getinfo()["communityfund"]["available"],  Decimal('0E-8'))

        slow_gen(self.nodes[0], 1)
        self.sync_all()
        assert_equal(self.nodes[0].getinfo()["communityfund"]["available"],  Decimal('740708.29555284'))

        shouldHaveBeenAccepted = self.nodes[0].sendrawtransaction(self.sweep_hash(self.nodes[1], txCanSpend, self.nodes[1].getnewaddress()))

        slow_gen(self.nodes[0], 1)
        self.sync_all()

        assert(self.nodes[1].gettransaction(shouldHaveBeenAccepted)["confirmations"] > 0)

        shouldHaveBeenRejected = ''
        shouldHaveBeenRejected2 = ''

        try:
            shouldHaveBeenRejected = self.nodes[0].sendrawtransaction(self.sweep_hash(self.nodes[1], txCantSpend, self.nodes[1].getnewaddress()))
            raise AssertionError('The first transaction should have been rejected')
        except JSONRPCException:
            pass

        try:
            shouldHaveBeenRejected = self.nodes[0].gettransaction(shouldHaveBeenRejected)
            raise AssertionError('The first transaction should not exist')
        except JSONRPCException:
            pass

        slow_gen(self.nodes[0], 1)
        self.sync_all()

        try:
            shouldHaveBeenRejected2 = self.nodes[0].sendrawtransaction(self.sweep_hash(self.nodes[1], txCantSpend2, self.nodes[1].getnewaddress()))
            raise AssertionError('The second transaction should have been rejected')
        except JSONRPCException:
            pass

        slow_gen(self.nodes[0], 1)
        self.sync_all()

        try:
            shouldHaveBeenRejected2 = self.nodes[0].gettransaction(shouldHaveBeenRejected2)
            raise AssertionError('The second transaction should not exist')
        except JSONRPCException:
            pass

    def sweep_hash(self, node, hash, address):
        prevouts = []
        amount = float(-0.01)
        for i in node.listunspent():
            if i['txid'] == hash:
                prevouts.append({"txid":hash,"vout":i['vout']})
                amount = round(amount + float(i['amount']), 2)

        if amount > 0:
            raw = node.signrawtransaction(node.createrawtransaction(prevouts,{address:amount}))
            if (raw['complete'] == True):
                return raw['hex']
        else:
            return ''


if __name__ == '__main__':
    Confiscation().main()
