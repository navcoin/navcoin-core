#!/usr/bin/env python3
#
# This test exercises the sendtoaddress api
# Tests valid transactions with amounts of different types,
# Invalid addresses and invalid amounts

from test_framework.test_framework import NavcoinTestFramework
from test_framework.util import *


class SendToAddressTest (NavcoinTestFramework):

    def __init__(self):
        super().__init__()
        self.setup_clean_chain = True
        self.num_nodes = 2

    def setup_network(self, split=False):
        self.nodes = start_nodes(self.num_nodes, self.options.tmpdir)
        connect_nodes_bi(self.nodes,0,1)
        self.is_network_split=False
        self.sync_all()

    def add_options(self, parser):
        parser.add_option("--sendtoaddresstest", dest="sendtoaddresstest",
                          help="Tests the sendtoaddress api under a variety of inputs")

    def run_test (self):
        # Turn staking off on all nodes
        self.nodes[0].staking(False)
        self.nodes[1].staking(False)

        # Generate necessary NAV for transactions
        print("Mining blocks...")
        slow_gen(self.nodes[0], 1)
        time.sleep(2)
        self.sync_all()
        slow_gen(self.nodes[1], 30)
        self.sync_all()

        # Assert correct amount of NAV generated
        assert_equal(self.nodes[0].getbalance(), 59800000)
        assert_equal(self.nodes[1].getbalance(), 1250)

        # Make transactions to valid addresses
        txid0 = self.nodes[0].sendtoaddress(self.nodes[1].getnewaddress(), 60)
        txid1 = self.nodes[1].sendtoaddress(self.nodes[0].getnewaddress(), Decimal("10.0"))
        txid7 = self.nodes[0].sendtoaddress(self.nodes[1].getnewaddress(), "50.0")
        txid9 = self.nodes[0].sendtoaddress(self.nodes[0].getnewaddress(), 100)
        self.sync_all()
        # Generate a block to confirm transactions
        slow_gen(self.nodes[0], 1)
        time.sleep(2)
        self.sync_all()
        # Assert transactions went through by validating balances
        assert_equal(self.nodes[0].getbalance(), 59799900 + self.nodes[0].gettransaction(txid0)["fee"] + self.nodes[0].gettransaction(txid7)["fee"] + self.nodes[0].gettransaction(txid9)["fee"])
        assert_equal(self.nodes[1].getbalance(), 1400 + self.nodes[1].gettransaction(txid1)["fee"])

        # Make transactions with invalid addresses
        exception_assert = False
        try:
            txid2 = self.nodes[0].sendtoaddress("n2USJi4FFP9HVgxskVA44rMr8RUgRhvKXm", 5)
        except JSONRPCException as e:
            if "Invalid Navcoin address" in e.error["message"]:
                exception_assert = True
            # Correct format but non-existant address
        assert(exception_assert)
        exception_assert = False
        try:
            txid3 = self.nodes[0].sendtoaddress("", 5)
        except JSONRPCException as e:
            if "Invalid Navcoin address" in e.error["message"]:
                exception_assert = True
                # Empty string
        assert(exception_assert)
        exception_assert = False
        try:
            txid4 = self.nodes[0].sendtoaddress(1234567890, 5)
        except JSONRPCException as e:
            if "JSON value is not a string as expected" in e.error["message"]:
                exception_assert = True
                # Incorrect type
        assert(exception_assert)
        exception_assert = False

        # Make transactions with invalid amounts
        try:
            txid5 = self.nodes[0].sendtoaddress(self.nodes[1].getnewaddress(), 60000000)
        except JSONRPCException as e:
            if "Insufficient funds" in e.error["message"]:
                exception_assert = True
                # Not enought NAV to make transaction
        assert(exception_assert)
        exception_assert = False
        try:
            txid6 = self.nodes[0].sendtoaddress(self.nodes[1].getnewaddress(), self.nodes[0].getbalance())
        except JSONRPCException as e:
            if "Error: This transaction requires a transaction fee of at least 0.0001 because of its amount, complexity, or use of recently received funds!" in e.error["message"]:
                exception_assert = True
                # Try send entire balance, not enought NAV to pay for fee
        assert(exception_assert)
        exception_assert = False
        try:
            txid8 = self.nodes[0].sendtoaddress(self.nodes[1].getnewaddress(), "")
        except JSONRPCException as e:
            if "Invalid amount" in e.error["message"]:
                exception_assert = True
                # Empty string as amount is not valid
        assert(exception_assert)
        exception_assert = False
        try:
            txid8 = self.nodes[0].sendtoaddress(self.nodes[1].getnewaddress(), "five")
        except JSONRPCException as e:
            if "Invalid amount" in e.error["message"]:
                exception_assert = True
                # String as amount is not valid
        assert(exception_assert)
        exception_assert = False
        try:
            txid8 = self.nodes[0].sendtoaddress(self.nodes[1].getnewaddress(), -100)
        except JSONRPCException as e:
            if "Amount out of range" in e.error["message"]:
                exception_assert = True
                # Negative amount is not valid
        assert(exception_assert)
        exception_assert = False
        try:
            txid8 = self.nodes[0].sendtoaddress(self.nodes[1].getnewaddress(), 0.000000001)
        except JSONRPCException as e:
            if "Invalid amount" in e.error["message"]:
                exception_assert = True
                # Too many dp not valid
        assert(exception_assert)
        exception_assert = False

        # Ensure that node balances are still the same as before attempting these invalid transactions
        slow_gen(self.nodes[1], 1)
        time.sleep(2)
        self.sync_all()
        assert_equal(self.nodes[0].getbalance(), 59799900 + self.nodes[0].gettransaction(txid0)["fee"] + self.nodes[0].gettransaction(txid7)["fee"] + self.nodes[0].gettransaction(txid9)["fee"])
        assert_equal(self.nodes[1].getbalance(), 1450 + self.nodes[1].gettransaction(txid1)["fee"])

        # Make a transaction with immature coins included
        balance = self.nodes[0].getbalance()
        slow_gen(self.nodes[0], 1)
        time.sleep(2)
        self.sync_all()
        try:
            txid10 = self.nodes[1].sendtoaddress(self.nodes[0].getnewaddress(), balance+45)
        except JSONRPCException as e:
            if "Insufficient funds" in e.error["message"]:
                exception_assert = True
        assert(exception_assert)
        exception_assert = False


if __name__ == '__main__':
    SendToAddressTest ().main ()
