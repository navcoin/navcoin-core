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
        self.nodes = start_nodes(self.num_nodes, self.options.tmpdir, [["-debug=dao","-dandelion=0", "-printtoconsole"]]*3)
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
        self.nodes[0].donatefund(100000)
        slow_gen(self.nodes[0], 10)

        time.sleep(3)

        proposalid0 = self.nodes[0].createproposal(self.nodes[0].getnewaddress(), 100, 3600, "test")["hash"]
        slow_gen(self.nodes[0], 1)

        start_new_cycle(self.nodes[0])

        reversed_hash = reverse_byte_str(proposalid0)
        vote_str = '6a' + 'c1' + 'c2' + 'c4' + '20' + reversed_hash
        voteno_str = '6a' + 'c1' + 'c2' + 'c5' + '20' + reversed_hash
        voteabs_str = '6a' + 'c1' + 'c2' + 'c7' + '20' + reversed_hash
        voterm_str = '6a' + 'c1' + 'c2' + 'c8' + '20' + reversed_hash

        rawtx=self.nodes[2].createrawtransaction([],{voteabs_str:0,'6ac1':0.1})
        rawtx = "08" + rawtx[2:]
        fundedrawtx=self.nodes[2].fundrawtransaction(rawtx,{'changeAddress':votingkey})['hex']
        signedrawtx=self.nodes[2].signrawtransaction(fundedrawtx)['hex']
        self.nodes[2].sendrawtransaction(signedrawtx)
        self.nodes[2].generatetoaddress(1, votingkey)
        sync_blocks(self.nodes)

        self.nodes[1].coinbaseoutputs([self.nodes[1].createrawtransaction(
            [],
            {voteno_str: 0},
            "", 0)])

        self.stake_block(self.nodes[1], False)
        self.stake_block(self.nodes[1], False)
        sync_blocks(self.nodes)

        assert_equal(self.nodes[1].getproposal(proposalid0)["votesYes"], 0)
        assert_equal(self.nodes[1].getproposal(proposalid0)["votesAbs"], 2)
        assert_equal(self.nodes[1].getproposal(proposalid0)["votesNo"], 0)


#       remove abstain vote
        rawtx=self.nodes[2].createrawtransaction([],{voterm_str:0,'6ac1':0.1})
        rawtx = "08" + rawtx[2:]
        fundedrawtx=self.nodes[2].fundrawtransaction(rawtx,{'changeAddress':votingkey})['hex']
        signedrawtx=self.nodes[2].signrawtransaction(fundedrawtx)['hex']
        self.nodes[2].sendrawtransaction(signedrawtx)
        self.nodes[2].generatetoaddress(1, votingkey)
        time.sleep(3)
        sync_blocks(self.nodes)
        self.stake_block(self.nodes[1], False)
        self.stake_block(self.nodes[1], False)
        sync_blocks(self.nodes)

        assert_equal(self.nodes[1].getproposal(proposalid0)["votesYes"], 0)
        assert_equal(self.nodes[1].getproposal(proposalid0)["votesAbs"], 2)
        assert_equal(self.nodes[1].getproposal(proposalid0)["votesNo"], 0)
        
        #start new cycle
        start_new_cycle(self.nodes[0])

        #stake 2 blocks with no vote
        self.stake_block(self.nodes[1], False)
        self.stake_block(self.nodes[1], False)
        sync_blocks(self.nodes)

        #set vote to yes
        rawtx=self.nodes[2].createrawtransaction([],{vote_str:0,'6ac1':0.1})
        rawtx = "08" + rawtx[2:]
        fundedrawtx=self.nodes[2].fundrawtransaction(rawtx,{'changeAddress':votingkey})['hex']
        signedrawtx=self.nodes[2].signrawtransaction(fundedrawtx)['hex']
        self.nodes[2].sendrawtransaction(signedrawtx)
        self.nodes[2].generatetoaddress(1, votingkey)
        sync_blocks(self.nodes)
        time.sleep(3)

        #stake 2 blocks with yes vote
        self.stake_block(self.nodes[1], False)
        self.stake_block(self.nodes[1], False)
        sync_blocks(self.nodes)

        assert_equal(self.nodes[1].getproposal(proposalid0)["votesYes"], 2)
        assert_equal(self.nodes[1].getproposal(proposalid0)["votesAbs"], 0)
        assert_equal(self.nodes[1].getproposal(proposalid0)["votesNo"], 0)

        #start new cycle
        start_new_cycle(self.nodes[0])

        #2 yes votes
        self.stake_block(self.nodes[1], False)
        self.stake_block(self.nodes[1], False)
        sync_blocks(self.nodes)

        #remove votes
        rawtx=self.nodes[2].createrawtransaction([],{voterm_str:0,'6ac1':0.1})
        rawtx = "08" + rawtx[2:]
        fundedrawtx=self.nodes[2].fundrawtransaction(rawtx,{'changeAddress':votingkey})['hex']
        signedrawtx=self.nodes[2].signrawtransaction(fundedrawtx)['hex']
        self.nodes[2].sendrawtransaction(signedrawtx)
        self.nodes[2].generatetoaddress(1, votingkey)
        sync_blocks(self.nodes)
        time.sleep(3)
        self.stake_block(self.nodes[1], False)
        self.stake_block(self.nodes[1], False)
        sync_blocks(self.nodes)
        assert_equal(self.nodes[1].getproposal(proposalid0)["votesYes"], 2)
        assert_equal(self.nodes[1].getproposal(proposalid0)["votesAbs"], 0)
        assert_equal(self.nodes[1].getproposal(proposalid0)["votesNo"], 0)

        #start new cycle
        start_new_cycle(self.nodes[0])
        rawtx=self.nodes[2].createrawtransaction([],{vote_str:0,'6ac1':0.1})
        rawtx = "08" + rawtx[2:]
        fundedrawtx=self.nodes[2].fundrawtransaction(rawtx,{'changeAddress':votingkey})['hex']
        signedrawtx=self.nodes[2].signrawtransaction(fundedrawtx)['hex']
        self.nodes[2].sendrawtransaction(signedrawtx)
        self.nodes[2].generatetoaddress(1, votingkey)
        sync_blocks(self.nodes)
        time.sleep(3)
        self.stake_block(self.nodes[1], False)
        self.stake_block(self.nodes[1], False)
        self.stake_block(self.nodes[1], False)
        self.stake_block(self.nodes[1], False)
        self.stake_block(self.nodes[1], False)
        self.stake_block(self.nodes[1], False)
        self.stake_block(self.nodes[1], False)
        self.stake_block(self.nodes[1], False)
        self.stake_block(self.nodes[1], False)
        self.stake_block(self.nodes[1], False)

#       print(self.nodes[0].getproposal(proposalid0)['status'])

        paymentReqid0 = self.nodes[0].createpaymentrequest(proposalid0, 10, "preq test")["hash"]
        slow_gen(self.nodes[0], 1)

        reversed_hash = reverse_byte_str(paymentReqid0)
        vote_str = '6a' + 'c1' + 'c3' + 'c4' + '20' + reversed_hash
        voteno_str = '6a' + 'c1' + 'c3' + 'c5' + '20' + reversed_hash
        voteabs_str = '6a' + 'c1' + 'c3' + 'c7' + '20' + reversed_hash
        voterm_str = '6a' + 'c1' + 'c3' + 'c8' + '20' + reversed_hash

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

        assert_equal(self.nodes[1].getpaymentrequest(paymentReqid0)["votesYes"], 0)
        assert_equal(self.nodes[1].getpaymentrequest(paymentReqid0)["votesAbs"], 2)
        assert_equal(self.nodes[1].getpaymentrequest(paymentReqid0)["votesNo"], 0)

        rawtx=self.nodes[2].createrawtransaction([],{voterm_str:0,'6ac1':0.1})
        rawtx = "08" + rawtx[2:]
        fundedrawtx=self.nodes[2].fundrawtransaction(rawtx,{'changeAddress':votingkey})['hex']
        signedrawtx=self.nodes[2].signrawtransaction(fundedrawtx)['hex']
        self.nodes[2].sendrawtransaction(signedrawtx)
        self.nodes[2].generatetoaddress(1, votingkey)
        time.sleep(3)
        sync_blocks(self.nodes)
        self.stake_block(self.nodes[1], False)
        self.stake_block(self.nodes[1], False)
        sync_blocks(self.nodes)

        assert_equal(self.nodes[1].getpaymentrequest(paymentReqid0)["votesYes"], 0)
        assert_equal(self.nodes[1].getpaymentrequest(paymentReqid0)["votesAbs"], 2)
        assert_equal(self.nodes[1].getpaymentrequest(paymentReqid0)["votesNo"], 0)
        
        #start new cycle
        start_new_cycle(self.nodes[0])

        #stake 2 blocks with no vote
        self.stake_block(self.nodes[1], False)
        self.stake_block(self.nodes[1], False)
        sync_blocks(self.nodes)

        #set vote to yes
        rawtx=self.nodes[2].createrawtransaction([],{vote_str:0,'6ac1':0.1})
        rawtx = "08" + rawtx[2:]
        fundedrawtx=self.nodes[2].fundrawtransaction(rawtx,{'changeAddress':votingkey})['hex']
        signedrawtx=self.nodes[2].signrawtransaction(fundedrawtx)['hex']
        self.nodes[2].sendrawtransaction(signedrawtx)
        self.nodes[2].generatetoaddress(1, votingkey)
        sync_blocks(self.nodes)
        time.sleep(3)

        #stake 2 blocks with yes vote
        self.stake_block(self.nodes[1], False)
        self.stake_block(self.nodes[1], False)
        sync_blocks(self.nodes)

        assert_equal(self.nodes[1].getpaymentrequest(paymentReqid0)["votesYes"], 2)
        assert_equal(self.nodes[1].getpaymentrequest(paymentReqid0)["votesAbs"], 0)
        assert_equal(self.nodes[1].getpaymentrequest(paymentReqid0)["votesNo"], 2)

        #start new cycle
        start_new_cycle(self.nodes[0])

        #2 yes votes
        self.stake_block(self.nodes[1], False)
        self.stake_block(self.nodes[1], False)
        sync_blocks(self.nodes)

        #remove votes
        rawtx=self.nodes[2].createrawtransaction([],{voterm_str:0,'6ac1':0.1})
        rawtx = "08" + rawtx[2:]
        fundedrawtx=self.nodes[2].fundrawtransaction(rawtx,{'changeAddress':votingkey})['hex']
        signedrawtx=self.nodes[2].signrawtransaction(fundedrawtx)['hex']
        self.nodes[2].sendrawtransaction(signedrawtx)
        self.nodes[2].generatetoaddress(1, votingkey)
        sync_blocks(self.nodes)
        time.sleep(3)
        self.stake_block(self.nodes[1], False)
        self.stake_block(self.nodes[1], False)
        sync_blocks(self.nodes)
        assert_equal(self.nodes[1].getpaymentrequest(paymentReqid0)["votesYes"], 2)
        assert_equal(self.nodes[1].getpaymentrequest(paymentReqid0)["votesAbs"], 0)
        assert_equal(self.nodes[1].getpaymentrequest(paymentReqid0)["votesNo"], 0)

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
