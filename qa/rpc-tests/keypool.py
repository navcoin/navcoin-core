#!/usr/bin/env python3
# Copyright (c) 2014-2016 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

# Exercise the wallet keypool, and interaction with wallet encryption/locking

from test_framework.test_framework import NavCoinTestFramework
from test_framework.util import *

class KeyPoolTest(NavCoinTestFramework):

    def __init__(self):
        super().__init__()
        self.setup_clean_chain = False
        self.num_nodes = 1

    def setup_network(self):
        self.nodes = self.setup_nodes()

    def run_test(self):
        
        addr_before_encrypting = self.nodes[0].getnewaddress()
        print('addr 1 ' + addr_before_encrypting)
        addr_before_encrypting_data = self.nodes[0].validateaddress(addr_before_encrypting)
        wallet_info_old = self.nodes[0].getwalletinfo()
        print('getwalletinfo ' + wallet_info_old['hdmasterkeyid'])
        print('validateaddress ' + addr_before_encrypting_data['hdmasterkeyid'])
        assert_equal(addr_before_encrypting_data['hdmasterkeyid'],  wallet_info_old['hdmasterkeyid'])
        
        # Encrypt wallet and wait to terminate
        print('Encypting wallet...')
        self.nodes[0].encryptwallet('test')
        navcoind_processes[0].wait()
        # Restart node 0
        self.nodes[0] = start_node(0, self.options.tmpdir)
        # Keep creating keys
        addr = self.nodes[0].getnewaddress()
        print('addr 2 ' + addr)
        wallet_info = self.nodes[0].getwalletinfo()
        addr_data = self.nodes[0].validateaddress(addr)
        assert(addr_before_encrypting_data['hdmasterkeyid'] != wallet_info['hdmasterkeyid'])

        print('getwalletinfo ' + wallet_info['hdmasterkeyid'])
        print('validateaddress ' + addr_data['hdmasterkeyid'])

        assert_equal(addr_data['hdmasterkeyid'], wallet_info['hdmasterkeyid'])
        
        try:
            addr = self.nodes[0].getnewaddress()
            raise AssertionError('Keypool should be exhausted after one address')
        except JSONRPCException as e:
            assert(e.error['code']==-12)

        # put three new keys in the keypool
        self.nodes[0].walletpassphrase('test', 12000)
        self.nodes[0].keypoolrefill(3)
        self.nodes[0].walletlock()

        # drain the keys
        addr = set()
        addr.add(self.nodes[0].getrawchangeaddress())
        addr.add(self.nodes[0].getrawchangeaddress())
        addr.add(self.nodes[0].getrawchangeaddress())
        addr.add(self.nodes[0].getrawchangeaddress())
        # assert that four unique addresses were returned
        assert(len(addr) == 4)
        # the next one should fail
        try:
            addr = self.nodes[0].getrawchangeaddress()
            raise AssertionError('Keypool should be exhausted after three addresses')
        except JSONRPCException as e:
            assert(e.error['code']==-12)

        # refill keypool with three new addresses
        self.nodes[0].walletpassphrase('test', 1)
        self.nodes[0].keypoolrefill(3)
        # test walletpassphrase timeout
        time.sleep(1.1)
        assert_equal(self.nodes[0].getwalletinfo()["unlocked_until"], 0)

        # drain them by mining
        slow_gen(self.nodes[0], 1)
        slow_gen(self.nodes[0], 1)
        slow_gen(self.nodes[0], 1)
        slow_gen(self.nodes[0], 1)

        try:
            slow_gen(self.nodes[0],1)
            raise AssertionError('Keypool should be exhausted after three addesses')
        except JSONRPCException as e:
            assert(e.error['code']==-12)

if __name__ == '__main__':
    KeyPoolTest().main()
