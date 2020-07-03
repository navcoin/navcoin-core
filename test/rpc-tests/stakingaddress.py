#!/usr/bin/env python3
# Copyright (c) 2019 The Navcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

from test_framework.test_framework import NavCoinTestFramework
from test_framework.staticr_util import *

class StakingAddressTest(NavCoinTestFramework):
    """Tests staking to another address using -stakingaddress option."""

    def __init__(self):
        super().__init__()
        self.setup_clean_chain = True
        self.num_nodes = 1
        self.node_args = ["-stakingaddress="]

    def setup_network(self, split=False):
        self.nodes = []
        self.nodes.append(start_node(0, self.options.tmpdir, []))
        self.is_network_split = split

    def run_test(self):
        SATOSHI = 100000000
        COINBASE_REWARD = 5000000000

        ## Test stake to a single address ##
        # Get new address for receiving the staking rewards
        receive_addr_A = self.nodes[0].getnewaddress()

        print("Restart node with staking address option ...")
        self.stop_node(0)
        self.nodes[0] = start_node(0, self.options.tmpdir, [self.node_args[0] + receive_addr_A])

        # Activate static rewards
        activate_staticr(self.nodes[0])

        # Wait for a stake
        stake_tx, reward_vout, index = self.get_stake_tx()

        # check that the staking reward was sent to the receiving address
        assert_equal(self.nodes[0].getrawtransaction(stake_tx, True)["vout"][index]['valueSat'], 200000000)
        assert_equal(self.nodes[0].getrawtransaction(stake_tx, True)["vout"][index]['scriptPubKey']['addresses'][0], receive_addr_A)
        assert_equal(reward_vout["scriptPubKey"]["addresses"][0], receive_addr_A)


        ## Test stake address mapping ##
        # Get new addresses to hold staking coins and for receiving the staking rewards
        staking_addr_B = self.nodes[0].getnewaddress()
        receive_addr_B = self.nodes[0].getnewaddress()

        print("Restart node with staking address option ...")
        self.stop_node(0)
        staking_map = '{"' + staking_addr_B + '":"' + receive_addr_B + '"}'
        self.nodes[0] = start_node(0, self.options.tmpdir, [self.node_args[0] + staking_map])

        # Stop staking, make all coins mature and send all to one address
        self.nodes[0].staking(False)
        slow_gen(self.nodes[0], 50)
        self.nodes[0].sendtoaddress(staking_addr_B, self.nodes[0].getbalance(), "", "", "", True)
        time.sleep(2)
        slow_gen(self.nodes[0], 1)
        fee = 1000000
        total_fee = 0
        for unspent in self.nodes[0].listunspent():
            if unspent["amount"] == COINBASE_REWARD / SATOSHI:
                # Send away any additional coinbase reward txs which might be staked instead
                tx = self.nodes[0].createrawtransaction([unspent], {self.nodes[0].getnewaddress(): float(COINBASE_REWARD - fee) / SATOSHI})
                txid = self.nodes[0].sendrawtransaction(self.nodes[0].signrawtransaction(tx)["hex"])
                total_fee += fee

        assert(len(self.nodes[0].listunspent()) == 1)
        assert(self.nodes[0].listunspent()[0]["amount"] > COINBASE_REWARD / SATOSHI)

        # Wait for a new block to be staked from the large utxo sent to staking_addr_B
        self.nodes[0].staking(True)
        stake_tx, reward_vout, index = self.get_stake_tx(total_fee)

        # check that the staking reward was sent to the receiving address
        assert_equal(self.nodes[0].getrawtransaction(stake_tx, True)["vout"][index]['valueSat'], 200000000 + total_fee)
        assert_equal(self.nodes[0].getrawtransaction(stake_tx, True)["vout"][index]['scriptPubKey']['addresses'][0], receive_addr_B)
        assert_equal(reward_vout["scriptPubKey"]["addresses"][0], receive_addr_B)


        ## Test stake address mapping using 'all' option ##
        # Get new address for receiving the staking rewards
        receive_addr_C = self.nodes[0].getnewaddress()

        print("Restart node with staking address option ...")
        self.stop_node(0)
        staking_map = '{"all":"' + receive_addr_C + '"}'
        self.nodes[0] = start_node(0, self.options.tmpdir, [self.node_args[0] + staking_map])

        # Wait for a stake
        stake_tx, reward_vout, index = self.get_stake_tx()

        # check that the staking reward was sent to the receiving address
        assert_equal(self.nodes[0].getrawtransaction(stake_tx, True)["vout"][index]['valueSat'], 200000000)
        assert_equal(self.nodes[0].getrawtransaction(stake_tx, True)["vout"][index]['scriptPubKey']['addresses'][0], receive_addr_C)
        assert_equal(reward_vout["scriptPubKey"]["addresses"][0], receive_addr_C)


    def get_stake_tx(self, tx_fee=0):
        SATOSHI = 100000000
        blockcount = self.nodes[0].getblockcount()

        # wait for a new block to be staked
        while self.nodes[0].getblockcount() == blockcount:
            print("waiting for a new block...")
            time.sleep(5)

        stake_tx = self.nodes[0].getblock(self.nodes[0].getbestblockhash())["tx"][1]
        stake_vouts = self.nodes[0].decoderawtransaction(self.nodes[0].gettransaction(stake_tx)["hex"])["vout"]
        i = -1
        reward_vout = stake_vouts[i]
        while float(stake_vouts[i]["value"]) != (2.0 * SATOSHI + tx_fee) / SATOSHI:
            # The staking reward can also contain a cfund contribution which must be skipped
            i -= 1
            reward_vout = stake_vouts[i]

        return stake_tx, reward_vout, i

if __name__ == '__main__':
    StakingAddressTest().main()
