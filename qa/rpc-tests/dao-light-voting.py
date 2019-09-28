#!/usr/bin/env python3
# Copyright (c) 2019 The Navcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

from test_framework.test_framework import NavCoinTestFramework
from test_framework.cfund_util import *

import time


class LightVotingTest(NavCoinTestFramework):
    """Tests the voting from light wallets"""

    def __init__(self):
        super().__init__()
        self.setup_clean_chain = True
        self.num_nodes = 3

    def setup_network(self, split=False):
        self.nodes = []
        self.nodes = start_nodes(self.num_nodes, self.options.tmpdir, [["-debug"]]*3)
        connect_nodes(self.nodes[0], 1)
        connect_nodes(self.nodes[1], 2)
        connect_nodes(self.nodes[2], 0)

    def run_test(self):
        # Get cfund parameters
        blocks_per_voting_cycle = self.nodes[0].cfundstats()["consensus"]["blocksPerVotingCycle"]

        self.nodes[0].staking(False)
        self.nodes[1].staking(False)
        self.nodes[2].staking(False)
        activate_softfork(self.nodes[0], "coldstaking_v2")

        votingkey = self.nodes[2].getnewaddress()
        coldstaking = self.nodes[0].getcoldstakingaddress(self.nodes[1].getnewaddress(),self.nodes[1].getnewaddress(),votingkey)

        self.nodes[0].sendtoaddress(votingkey, 100)
        self.nodes[0].sendtoaddress(coldstaking, 30000000)

        slow_gen(self.nodes[0], 10)
        sync_blocks(self.nodes)

        proposalid0 = self.nodes[0].createproposal(self.nodes[0].getnewaddress(), 100, 3600, "test")["hash"]
        slow_gen(self.nodes[0], 1)

        start_new_cycle(self.nodes[0])

        reversed_hash = reverse_byte_str(proposalid0)
        vote_str = '6a' + 'c1' + 'c2' + 'c4' + '20' + reversed_hash
        voteno_str = '6a' + 'c1' + 'c2' + 'c5' + '20' + reversed_hash

        rawtx=self.nodes[2].createrawtransaction([],{vote_str:0,'6ac1':0.1})
        rawtx = "08" + rawtx[2:]
        fundedrawtx=self.nodes[2].fundrawtransaction(rawtx)['hex']
        signedrawtx=self.nodes[2].signrawtransaction(fundedrawtx)['hex']
        self.nodes[2].sendrawtransaction(signedrawtx)

        slow_gen(self.nodes[0], 1)
        sync_blocks(self.nodes)

        self.nodes[1].staking(True)
        self.nodes[1].coinbaseoutputs([self.nodes[1].createrawtransaction(
            [],
            {voteno_str: 0},
            "", 0
        )])

        block_height = self.nodes[1].getblockcount()
        while (block_height  >= self.nodes[1].getblockcount()):
            time.sleep(1)

        assert (self.nodes[1].getproposal(proposalid0)["votesYes"] == 1)
        assert (self.nodes[1].getproposal(proposalid0)["votesNo"] == 0)

if __name__ == '__main__':
    LightVotingTest().main()
