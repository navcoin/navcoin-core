#!/usr/bin/env python3
# Copyright (c) 2019 The Navcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

from test_framework.test_framework import NavcoinTestFramework
from test_framework.cfund_util import *

import time


class LightVotingTest(NavcoinTestFramework):
    """Tests the voting from light wallets"""

    def __init__(self):
        super().__init__()
        self.setup_clean_chain = True
        self.num_nodes = 3

    def setup_network(self, split=False):
        self.nodes = []
        self.nodes = start_nodes(self.num_nodes, self.options.tmpdir, [["-debug=dao","-dandelion=0"]]*3)
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
        self.nodes[0].sendtoaddress(coldstaking, 3000000)
        self.nodes[0].sendtoaddress(coldstaking, 3000000)
        self.nodes[0].sendtoaddress(coldstaking, 3000000)
        self.nodes[0].sendtoaddress(coldstaking, 3000000)
        self.nodes[0].sendtoaddress(coldstaking, 3000000)
        self.nodes[0].sendtoaddress(coldstaking, 3000000)

        slow_gen(self.nodes[0], 10)

        time.sleep(3)

        consultation_hash = self.nodes[0].createconsultationwithanswers("question",["a","b"],2)['hash']
        slow_gen(self.nodes[0], 1)
        consultation_answer_a_hash = self.nodes[0].getconsultation(consultation_hash)["answers"][0]['hash']
        consultation_answer_b_hash = self.nodes[0].getconsultation(consultation_hash)["answers"][1]['hash']

        start_new_cycle(self.nodes[0])

        reversed_hash = reverse_byte_str(consultation_hash)
        reversed_hash_a = reverse_byte_str(consultation_answer_a_hash)
        reversed_hash_b = reverse_byte_str(consultation_answer_b_hash)
        support_str_a = '6a' + 'c9' + 'c4' + '20' + reversed_hash_a
        supportrm_str_a = '6a' + 'c9' + 'c8' + '20' + reversed_hash_a
        support_str_b = '6a' + 'c9' + 'c4' + '20' + reversed_hash_b

        rawtx=self.nodes[2].createrawtransaction([],{support_str_a:0,'6ac1':0.1})
        rawtx = "08" + rawtx[2:]
        fundedrawtx=self.nodes[2].fundrawtransaction(rawtx,{'changeAddress':votingkey})['hex']
        signedrawtx=self.nodes[2].signrawtransaction(fundedrawtx)['hex']
        self.nodes[2].sendrawtransaction(signedrawtx)
        self.nodes[2].generatetoaddress(1, votingkey)
        sync_blocks(self.nodes)
        rawtx=self.nodes[2].createrawtransaction([],{support_str_b:0,'6ac1':0.1})
        rawtx = "08" + rawtx[2:]
        fundedrawtx=self.nodes[2].fundrawtransaction(rawtx,{'changeAddress':votingkey})['hex']
        signedrawtx=self.nodes[2].signrawtransaction(fundedrawtx)['hex']
        self.nodes[2].sendrawtransaction(signedrawtx)
        self.nodes[2].generatetoaddress(1, votingkey)
        sync_blocks(self.nodes)
        self.stake_block(self.nodes[1], False)
        self.stake_block(self.nodes[1], False)
        sync_blocks(self.nodes)
        time.sleep(3)
        assert_equal(self.nodes[0].getconsultation(consultation_hash)["answers"][0]['support'], 2)
        assert_equal(self.nodes[0].getconsultation(consultation_hash)["answers"][1]['support'], 2)

        rawtx=self.nodes[2].createrawtransaction([],{supportrm_str_a:0,'6ac1':0.1})
        rawtx = "08" + rawtx[2:]
        fundedrawtx=self.nodes[2].fundrawtransaction(rawtx,{'changeAddress':votingkey})['hex']
        signedrawtx=self.nodes[2].signrawtransaction(fundedrawtx)['hex']
        self.nodes[2].sendrawtransaction(signedrawtx)
        self.nodes[2].generatetoaddress(1, votingkey)
        sync_blocks(self.nodes)
        self.stake_block(self.nodes[1], False)
        self.stake_block(self.nodes[1], False)
        sync_blocks(self.nodes)
        time.sleep(3)
        assert_equal(self.nodes[0].getconsultation(consultation_hash)["answers"][0]['support'], 2)
        assert_equal(self.nodes[0].getconsultation(consultation_hash)["answers"][1]['support'], 4)


        start_new_cycle(self.nodes[0])
        start_new_cycle(self.nodes[0])
        start_new_cycle(self.nodes[0])
        start_new_cycle(self.nodes[0])

        vote_str_a = '6a' + 'cb' + 'ca' + '20' + reversed_hash_a
        voterm_str_a = '6a' + 'cb' + 'c8' + '20' + reversed_hash_a

        voteabs_str = '6a' + 'cb' + 'c7' + '20' + reversed_hash
        voterm_str = '6a' + 'cb' + 'c8' + '20' + reversed_hash

        support_str_b = '6a' + 'c4' + '20' + reversed_hash_b
        vote_str_b = '6a' + 'cb' + 'ca' + '20' + reversed_hash_b

        rawtx=self.nodes[2].createrawtransaction([],{vote_str_a:0,'6ac1':0.1})
        rawtx = "08" + rawtx[2:]
        fundedrawtx=self.nodes[2].fundrawtransaction(rawtx,{'changeAddress':votingkey})['hex']
        signedrawtx=self.nodes[2].signrawtransaction(fundedrawtx)['hex']
        self.nodes[2].sendrawtransaction(signedrawtx)
        self.nodes[2].generatetoaddress(1, votingkey)
        sync_blocks(self.nodes)
        self.stake_block(self.nodes[1], False)
        self.stake_block(self.nodes[1], False)
        sync_blocks(self.nodes)
        time.sleep(3)
        assert_equal(self.nodes[0].getconsultation(consultation_hash)["answers"][0]['votes'], 2)
        assert_equal(self.nodes[0].getconsultation(consultation_hash)["answers"][1]['votes'], 0)
        assert_equal(self.nodes[0].getconsultation(consultation_hash)['abstain'], 0)

        rawtx=self.nodes[2].createrawtransaction([],{voterm_str_a:0,'6ac1':0.1})
        rawtx = "08" + rawtx[2:]
        fundedrawtx=self.nodes[2].fundrawtransaction(rawtx,{'changeAddress':votingkey})['hex']
        signedrawtx=self.nodes[2].signrawtransaction(fundedrawtx)['hex']
        self.nodes[2].sendrawtransaction(signedrawtx)
        self.nodes[2].generatetoaddress(1, votingkey)
        sync_blocks(self.nodes)
        self.stake_block(self.nodes[1], False)
        self.stake_block(self.nodes[1], False)
        sync_blocks(self.nodes)
        time.sleep(3)
        assert_equal(self.nodes[0].getconsultation(consultation_hash)["answers"][0]['votes'], 2)
        assert_equal(self.nodes[0].getconsultation(consultation_hash)["answers"][1]['votes'], 0)
        assert_equal(self.nodes[0].getconsultation(consultation_hash)['abstain'], 0)

        rawtx=self.nodes[2].createrawtransaction([],{voteabs_str:0,'6ac1':0.1})
        rawtx = "08" + rawtx[2:]
        fundedrawtx=self.nodes[2].fundrawtransaction(rawtx,{'changeAddress':votingkey})['hex']
        signedrawtx=self.nodes[2].signrawtransaction(fundedrawtx)['hex']
        self.nodes[2].sendrawtransaction(signedrawtx)
        self.nodes[2].generatetoaddress(1, votingkey)
        sync_blocks(self.nodes)
        self.stake_block(self.nodes[1], False)
        self.stake_block(self.nodes[1], False)
        sync_blocks(self.nodes)
        time.sleep(3)
        assert_equal(self.nodes[0].getconsultation(consultation_hash)["answers"][0]['votes'], 2)
        assert_equal(self.nodes[0].getconsultation(consultation_hash)["answers"][1]['votes'], 0)
        assert_equal(self.nodes[0].getconsultation(consultation_hash)['abstain'], 2)


        start_new_cycle(self.nodes[0])

#       1 abstain vote
        self.stake_block(self.nodes[1], False)
        sync_blocks(self.nodes)
        time.sleep(3)

#       remove abstain vote
        rawtx=self.nodes[2].createrawtransaction([],{voterm_str:0,'6ac1':0.1})
        rawtx = "08" + rawtx[2:]
        fundedrawtx=self.nodes[2].fundrawtransaction(rawtx,{'changeAddress':votingkey})['hex']
        signedrawtx=self.nodes[2].signrawtransaction(fundedrawtx)['hex']
        self.nodes[2].sendrawtransaction(signedrawtx)
        self.nodes[2].generatetoaddress(1, votingkey)
        sync_blocks(self.nodes)
        time.sleep(3)
#       switch to vote answer b
        rawtx=self.nodes[2].createrawtransaction([],{vote_str_b:0,'6ac1':0.1})
        rawtx = "08" + rawtx[2:]
        fundedrawtx=self.nodes[2].fundrawtransaction(rawtx,{'changeAddress':votingkey})['hex']
        signedrawtx=self.nodes[2].signrawtransaction(fundedrawtx)['hex']
        self.nodes[2].sendrawtransaction(signedrawtx)
        self.nodes[2].generatetoaddress(1, votingkey)
        sync_blocks(self.nodes)
        self.stake_block(self.nodes[1], False)
        self.stake_block(self.nodes[1], False)
        sync_blocks(self.nodes)
        time.sleep(3)
        assert_equal(self.nodes[0].getconsultation(consultation_hash)["answers"][0]['votes'], 0)
        assert_equal(self.nodes[0].getconsultation(consultation_hash)["answers"][1]['votes'], 2)
        assert_equal(self.nodes[0].getconsultation(consultation_hash)['abstain'], 1)
       

#       add vote for answer a. a & b should both get votes
        rawtx=self.nodes[2].createrawtransaction([],{vote_str_a:0,'6ac1':0.1})
        rawtx = "08" + rawtx[2:]
        fundedrawtx=self.nodes[2].fundrawtransaction(rawtx,{'changeAddress':votingkey})['hex']
        signedrawtx=self.nodes[2].signrawtransaction(fundedrawtx)['hex']
        self.nodes[2].sendrawtransaction(signedrawtx)
        self.nodes[2].generatetoaddress(1, votingkey)
        sync_blocks(self.nodes)
        self.stake_block(self.nodes[1], False)
        self.stake_block(self.nodes[1], False)
        sync_blocks(self.nodes)
        time.sleep(3)
        assert_equal(self.nodes[0].getconsultation(consultation_hash)["answers"][0]['votes'], 2)
        assert_equal(self.nodes[0].getconsultation(consultation_hash)["answers"][1]['votes'], 4)
        assert_equal(self.nodes[0].getconsultation(consultation_hash)['abstain'], 1)

    def stake_block(self, node, mature = True):
        # Get the current block count to check against while we wait for a stake
        blockcount = node.getblockcount()

        # Turn staking on
        node.staking(True)

        # wait for a new block to be mined
        while node.getblockcount() == blockcount:
            #print("waiting for a new block...")
            time.sleep(1)

        # We got one
        #print("found a new block...")

        # Turn staking off
        node.staking(False)

        # Get the staked block
        block_hash = node.getbestblockhash()

        # Only mature the blocks if we asked for it
        if (mature):
            # Make sure the blocks are mature before we check the report
            slow_gen(node, 5, 0.5)
            self.sync_all()

        # return the block hash to the function caller
        return block_hash

if __name__ == '__main__':
    LightVotingTest().main()
