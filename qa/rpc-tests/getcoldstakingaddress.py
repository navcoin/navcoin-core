#!/usr/bin/env python3
# Copyright (c) 2018 The Navcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

from test_framework.test_framework import NavCoinTestFramework
from test_framework.util import *

class GetColdStakingAddress(NavCoinTestFramework):
    """Tests the creation of a cold staking address."""

    def __init__(self):
        super().__init__()
        self.setup_clean_chain = True
        self.num_nodes = 1

    def setup_network(self, split=False):
        self.nodes = self.setup_nodes()
        self.is_network_split = split

    def run_test(self):
        slow_gen(self.nodes[0], 100)
        # Verify the Cold Staking is started
        assert(self.nodes[0].getblockchaininfo()["bip9_softforks"]["coldstaking"]["status"] == "started")

        slow_gen(self.nodes[0], 100)
        # Verify the Cold Staking is locked_in
        assert(self.nodes[0].getblockchaininfo()["bip9_softforks"]["coldstaking"]["status"] == "locked_in")

        slow_gen(self.nodes[0], 100)
        # Verify the Cold Staking is active
        assert(self.nodes[0].getblockchaininfo()["bip9_softforks"]["coldstaking"]["status"] == "active")

        # Success cases 
        
        # Two valid inputs

        address_one = "n1xHh1rPngXNCr2ZbyP9wveFPh4e14YfX2" 
        address_two = "midAY8TW8TpRY4i8ZEyagSeon3LZFjjMyN"

        expected_cold_staking_address = "2afhomfZnhs82Qy1r1MHc4yUnP9jSCvFUdN8vyKR96dbWNEPZ9xyK5bFhSaxe7"

        cold_staking_address_two =  "2ZSPY1TTcQVWhQPBAPWBAC7TJ5f72hLLQEsPY6CuN69Fx7u76qwysTAWsjA74R"

        coldstaking_address_one = self.nodes[0].getcoldstakingaddress(address_one, address_two)

        assert(self.nodes[0].validateaddress(coldstaking_address_one)["isvalid"] == True)
        
        assert(coldstaking_address_one == expected_cold_staking_address)

        # Things that should fail

        ## Two of the same address 

        make_addr_fail = True
        
        try:
            self.nodes[0].getcoldstakingaddress(address_one, address_one)
            make_addr_fail = False
        except JSONRPCException as e:
            assert("The staking address should be different to the spending address" in e.error['message'])

        assert(make_addr_fail)
        
        ## Using coldstaking addresses 
        
        try:
            self.nodes[0].getcoldstakingaddress(coldstaking_address_one, address_one)
            make_addr_fail = False
        except JSONRPCException as e:
            assert("Staking address is not a valid NavCoin address" in e.error['message'])
        
        try:
            self.nodes[0].getcoldstakingaddress(address_one, coldstaking_address_one)
            make_addr_fail = False
        except JSONRPCException as e:
            assert("Spending address is not a valid NavCoin address" in e.error['message'])

        try:
            self.nodes[0].getcoldstakingaddress(coldstaking_address_one, cold_staking_address_two)
            make_addr_fail = False
        except JSONRPCException as e:
            assert("Staking address is not a valid NavCoin address" in e.error['message'])

        try:
            self.nodes[0].getcoldstakingaddress(coldstaking_address_one, coldstaking_address_one)
            make_addr_fail = False
        except JSONRPCException as e:
            assert("The staking address should be different to the spending address" in e.error['message'])

        assert(make_addr_fail)

        ## Missing arguments

        try:
            self.nodes[0].getcoldstakingaddress(None, address_one)
            make_addr_fail = False
        except JSONRPCException as e:
            assert("JSON value is not a string as expected" in e.error['message'])

        try:
            self.nodes[0].getcoldstakingaddress("", address_one)
            make_addr_fail = False
        except JSONRPCException as e:
            assert("Staking address is not a valid NavCoin address" in e.error['message'])

        try:
            self.nodes[0].getcoldstakingaddress(address_one)
            make_addr_fail = False
        except JSONRPCException as e:
            assert(("getcoldstakingaddress \"stakingaddress\" \"spendingaddress\"\n"
            "Returns a coldstaking address based on two address inputs\n"
            "Arguments:\n"
            "1. \"stakingaddress\"  (string, required) The navcoin staking address.\n"
            "2. \"spendingaddress\" (string, required) The navcoin spending address.\n\n"
            "Result:\n"
            "coldstakingaddress (string) The coldstaking address\n\n"
            "Examples:\n\n"
            "> navcoin-cli getcoldstakingaddress \"mqyGZvLYfEH27Zk3z6JkwJgB1zpjaEHfiW\" \"mrfjgazyerYxDQHJAPDdUcC3jpmi8WZ2uv\"\n"
            "As a json rpc call:\n") in e.error['message']) # Check if we print out the help docs

        assert(make_addr_fail)

        ## Using invalid addresses        

        ### Numbers 
        
        try:
            self.nodes[0].getcoldstakingaddress(123, address_one)
            make_addr_fail = False
        except JSONRPCException as e:
            assert("JSON value is not a string as expected" in e.error['message'])

        try:
            self.nodes[0].getcoldstakingaddress("123", address_one)
            make_addr_fail = False
        except JSONRPCException as e:
            assert("Staking address is not a valid NavCoin address" in e.error['message'])

        try:
            self.nodes[0].getcoldstakingaddress(address_one, 123)
            make_addr_fail = False
        except JSONRPCException as e:
            assert("JSON value is not a string as expected" in e.error['message'])

        try:
            self.nodes[0].getcoldstakingaddress(address_one, "123")
            make_addr_fail = False
        except JSONRPCException as e:
            assert("Spending address is not a valid NavCoin address" in e.error['message'])

        assert(make_addr_fail)

        ### Other strings 

        try:
            self.nodes[0].getcoldstakingaddress("\"test\"", address_one)
            make_addr_fail = False
        except JSONRPCException as e:
            assert("Staking address is not a valid NavCoin address" in e.error['message'])

        try:
            self.nodes[0].getcoldstakingaddress(address_one, True)
            make_addr_fail = False
        except JSONRPCException as e:
            assert("JSON value is not a string as expected" in e.error['message'])

        assert(make_addr_fail)

        ### bitcoin address
        
        bitcoin_address_one = "13EBrRgbPSXwd64MUPaZuWz9aCYq78vpAR"
        bitcoin_address_two = "1DdGA8L9sMAz5sXJqcydivxvYicY75VsKa"

        try:
            self.nodes[0].getcoldstakingaddress(bitcoin_address_one, address_two)
            make_addr_fail = False
        except JSONRPCException as e:
            assert("Staking address is not a valid NavCoin address" in e.error['message'])
        
        try:
            self.nodes[0].getcoldstakingaddress(address_one, bitcoin_address_two)
            make_addr_fail = False
        except JSONRPCException as e:
            assert("Spending address is not a valid NavCoin address" in e.error['message'])

        assert(make_addr_fail)


if __name__ == '__main__':
    GetColdStakingAddress().main()
