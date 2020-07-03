#!/usr/bin/env python3
# Copyright (c) 2018 The Navcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

from test_framework.test_framework import NavCoinTestFramework
from test_framework.util import *

#import time

class ColdStakingStaking(NavCoinTestFramework):
    """Tests spending and staking to/from a staking wallet."""

    def __init__(self):
        super().__init__()
        self.setup_clean_chain = True
        self.num_nodes = 2

    def setup_network(self, split=False):
        self.nodes = self.setup_nodes()
        connect_nodes(self.nodes[0], 1)
        self.is_network_split = False

    def run_test(self):

        self.nodes[1].staking(False)
        slow_gen(self.nodes[0], 100)
        self.sync_all()

        # Verify the Cold Staking is started
        assert(self.nodes[0].getblockchaininfo()["bip9_softforks"]["coldstaking"]["status"] == "started")

        slow_gen(self.nodes[0], 100)
        self.sync_all()

        # Verify the Cold Staking is locked_in
        assert(self.nodes[0].getblockchaininfo()["bip9_softforks"]["coldstaking"]["status"] == "locked_in")

        slow_gen(self.nodes[0], 100)
        self.sync_all()

        # Verify the Cold Staking is active
        assert(self.nodes[0].getblockchaininfo()["bip9_softforks"]["coldstaking"]["status"] == "active")

        # Generate our addresses

        SENDING_FEE= 0.00010000

        staking_address_public_key = self.nodes[0].getnewaddress()
        staking_address_private_key = self.nodes[0].dumpprivkey(staking_address_public_key)

        # Third party addresses and keys
        spending_address_public_key = "mqyGZvLYfEH27Zk3z6JkwJgB1zpjaEHfiW"
        spending_address_private_key = "cMuNajSALbixZvApkcYVE4KgJoeQY92umhEVdQwqX9wSJUzkmdvF"
        address_Y_public_key = "mrfjgazyerYxDQHJAPDdUcC3jpmi8WZ2uv"
        address_Y_private_key = "cST2mj1kXtiRyk8VSXU3F9pnTp7GrGpyqHRv4Gtap8jo4LGUMvqo"

        ## Our wallet holds the staking address
        coldstaking_address_staking = self.nodes[0].getcoldstakingaddress(staking_address_public_key, spending_address_public_key)

        # Sending to cold address:
            # Success case:
                # Available balance decrease (if our wallet is the staking wallet)
                # Staking weight increase (or stay the same if sending wallet = staking wallet)

        balance_before = self.nodes[0].getbalance()
        staking_weight_before = self.nodes[0].getstakinginfo()["weight"]

        # Check wallet weight roughly equals wallet balance
        assert(round(staking_weight_before / 100000000.0, -5) == round(balance_before, -5))

        # Send funds to the cold staking address (leave some NAV for fees)
        self.nodes[0].sendtoaddress(coldstaking_address_staking, balance_before - 1)
        self.nodes[0].generate(1)
        self.sync_all()
        # get block height
        # getbockhash
        #getblock
        cs_tx_block = self.nodes[0].getblock(self.nodes[0].getblockhash(self.nodes[0].getblockcount()))
        cs_tx_id = cs_tx_block["tx"][1]
        utxo_before = self.nodes[0].gettxout(cs_tx_id, 0, True)
        # print(self.nodes[0].gettxout(cs_tx_id, 0, True))

        balance_step_one = self.nodes[0].getbalance()
        staking_weight_one = self.nodes[0].getstakinginfo()["weight"]

        # We expect our balance to decrease by tx amount + fees
        # We expect our staking weight to remain the same
        # print(staking_weight_before, staking_weight_one)
        assert(balance_step_one <= 51 - SENDING_FEE )

        self.sync_all()

        # Test spending from a cold staking wallet with the staking key
        spending_fail = True

        # Send funds to a third party address using a signed raw transaction
        listunspent_txs = []
        try:
            listunspent_txs = [ n for n in self.nodes[0].listunspent() if n["address"] == coldstaking_address_staking]
            self.send_raw_transaction(listunspent_txs[0], address_Y_public_key, coldstaking_address_staking, float(balance_step_one) * 0.5)
            spending_fail = False
        except IndexError:
            pass
        except Exception as e:
            pass

        # We expect our balance and weight to be unchanged
        assert(self.nodes[0].getbalance() == balance_step_one)
        assert(spending_fail)
        assert(self.nodes[0].getstakinginfo()["weight"] / 100000000.0 >= staking_weight_one / 100000000.0 - 1)

        try:
            listunspent_txs = [ n for n in self.nodes[0].listunspent() if n["address"] == coldstaking_address_staking]
            self.send_raw_transaction(listunspent_txs[0], staking_address_public_key , coldstaking_address_staking, float(balance_step_one) * 0.5)
            spending_fail = False
        except IndexError:
            pass
        except Exception as e:
            pass
        
        # We expect our balance and weight to be unchanged
        assert(self.nodes[0].getbalance() == balance_step_one)
        assert(spending_fail)
        assert(self.nodes[0].getstakinginfo()["weight"] / 100000000.0 >= staking_weight_one / 100000000.0 - 1)

        # Send funds using rpc to third party
        try:
            self.nodes[0].sendtoaddress(address_Y_public_key, float(balance_before) * 0.5)
            spending_fail = False
        except JSONRPCException as e:
            assert("Insufficient funds" in e.error['message'])

        # We expect our balance and weight to be unchanged
        assert(self.nodes[0].getbalance() == balance_step_one)
        assert(spending_fail)
        assert(self.nodes[0].getstakinginfo()["weight"] / 100000000.0 >= staking_weight_one / 100000000.0 - 1)


        # Send funds using rpc to staking address
        try:
            self.nodes[0].sendtoaddress(staking_address_public_key, float(balance_before) * 0.5)
            spending_fail = False
        except JSONRPCException as e:
            assert("Insufficient funds" in e.error['message'])

        # We expect our balance and weight to be unchanged
        assert(self.nodes[0].getbalance() == balance_step_one)
        assert(spending_fail)
        assert(self.nodes[0].getstakinginfo()["weight"] / 100000000.0 >= staking_weight_one / 100000000.0 - 1)


        # Send funds using rpc to spending address
        try:
            self.nodes[0].sendtoaddress(spending_address_public_key, float(balance_before) * 0.5)
            spending_fail = False
        except JSONRPCException as e:
            assert("Insufficient funds" in e.error['message'])

        # We expect our balance and weight to be unchanged
        assert(self.nodes[0].getbalance() == balance_step_one)
        assert(spending_fail)
        assert(self.nodes[0].getstakinginfo()["weight"] / 100000000.0 >= staking_weight_one / 100000000.0 - 1)

        
        # Staking
        slow_gen(self.nodes[1], 300)
        self.sync_all()


        current_balance = self.nodes[0].getbalance()
        # print("weight b4:",self.nodes[0].getstakinginfo()["weight"])
        block_height = self.nodes[0].getblockcount()
        # print("weight after:",self.nodes[0].getstakinginfo()["weight"])
        while (block_height  >= self.nodes[0].getblockcount()):
            time.sleep(5)

        # print('blockheight', self.nodes[0].getblockcount())

        # print('current bal', current_balance)
        # print('new bal',self.nodes[0].getbalance())
        utxo_after = self.nodes[0].gettxout(cs_tx_id, 0, True)
        utxo_after_two = self.nodes[0].gettxout(cs_tx_id, 1, True)

        # print(utxo_before, utxo_after, utxo_after_two)
        assert(utxo_before != utxo_after)
        

    def send_raw_transaction(self, decoded_raw_transaction, to_address, change_address, amount):
        # Create a raw tx
        inputs = [{ "txid" : decoded_raw_transaction["txid"], "vout" : 1}]
        outputs = { to_address : amount, change_address : float(decoded_raw_transaction["amount"]) - amount - 0.01 }
        rawtx = self.nodes[0].createrawtransaction(inputs, outputs)

        # Sign raw transaction
        signresult = self.nodes[0].signrawtransaction(rawtx)
        # signresult["complete"]   

        # Send raw transaction
        return self.nodes[0].sendrawtransaction(signresult['hex'])
        

if __name__ == '__main__':
    ColdStakingStaking().main()
