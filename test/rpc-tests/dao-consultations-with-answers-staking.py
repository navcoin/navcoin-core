#!/usr/bin/env python3
# Copyright (c) 2019 The Navcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

from test_framework.test_framework import NavcoinTestFramework
from test_framework.cfund_util import *

import time


class ConsultationsTest(NavcoinTestFramework):
    """Tests the consultations of the DAO"""

    def __init__(self):
        super().__init__()
        self.setup_clean_chain = True
        self.num_nodes = 1

    def setup_network(self, split=False):
        self.nodes = []
        self.nodes = start_nodes(self.num_nodes, self.options.tmpdir, [["-debug=dao"]])

    def run_test(self):
        # Get cfund parameters
        blocks_per_voting_cycle = self.nodes[0].cfundstats()["consensus"]["blocksPerVotingCycle"]

        self.nodes[0].staking(False)
        activate_softfork(self.nodes[0], "consultations")

        self.end_cycle_stake(self.nodes[0])

        hash = self.nodes[0].createconsultationwithanswers("question",["a","b"])['hash']
        hash2 = self.nodes[0].createconsultationwithanswers("question2",["c","d"])['hash']
        self.stake_block(self.nodes[0], 1)

        consultation = self.nodes[0].getconsultation(hash)
        consultation2 = self.nodes[0].getconsultation(hash2)

        assert_equal(len(consultation["answers"]), 2)
        assert_equal(len(consultation2["answers"]), 2)

        self.nodes[0].support(consultation["answers"][0]['hash'])

        self.stake_block(self.nodes[0], 2)
        assert_equal(self.nodes[0].getconsultation(hash)["answers"][0]['support'], 2)
        assert_equal(self.nodes[0].getconsultation(hash)["answers"][1]['support'], 0)
        assert_equal(self.nodes[0].getconsultation(hash2)["answers"][0]['support'], 0)
        assert_equal(self.nodes[0].getconsultation(hash2)["answers"][1]['support'], 0)

        self.nodes[0].support(consultation["answers"][0]['hash'], 'remove')
        self.nodes[0].support(consultation["answers"][1]['hash'])
        self.nodes[0].support(consultation2["answers"][0]['hash'])
        self.nodes[0].support(consultation2["answers"][1]['hash'])

        self.stake_block(self.nodes[0], 2)
        assert_equal(self.nodes[0].getconsultation(hash)["answers"][0]['support'], 2)
        assert_equal(self.nodes[0].getconsultation(hash)["answers"][1]['support'], 2)
        assert_equal(self.nodes[0].getconsultation(hash2)["answers"][0]['support'], 2)
        assert_equal(self.nodes[0].getconsultation(hash2)["answers"][1]['support'], 2)

        self.nodes[0].support(consultation["answers"][0]['hash'])
        self.nodes[0].support(consultation2["answers"][0]['hash'], 'remove')

        self.stake_block(self.nodes[0], 2)
        assert_equal(self.nodes[0].getconsultation(hash)["answers"][0]['support'], 4)
        assert_equal(self.nodes[0].getconsultation(hash)["answers"][1]['support'], 4)
        assert_equal(self.nodes[0].getconsultation(hash2)["answers"][0]['support'], 2)
        assert_equal(self.nodes[0].getconsultation(hash2)["answers"][1]['support'], 4)

        self.nodes[0].support(consultation2["answers"][0]['hash'])

        statusesAnswer = ["found support, waiting for end of voting period"]
        statusesAnswer = statusesAnswer + ["found support"] * (self.nodes[0].getconsensusparameters()[3])
        statusesAnswer = statusesAnswer + ["found support"] * self.nodes[0].getconsensusparameters()[6]
        statusesAnswer = statusesAnswer + ["found support", "found support"]

        statuses = ["waiting for support"]
        statuses = statuses + ["waiting for support"] * (self.nodes[0].getconsensusparameters()[3]-1)
        statuses = statuses + ["found support, waiting for end of voting period"]
        statuses = statuses + ["reflection phase"] * self.nodes[0].getconsensusparameters()[6]
        statuses = statuses + ["voting started", "voting started"]
        int = 0

        for i in range(self.nodes[0].getconsensusparameters()[3] + self.nodes[0].getconsensusparameters()[6] + 2):
            assert_equal(self.nodes[0].getconsultation(hash)["answers"][0]['status'], statusesAnswer[i])
            assert_equal(self.nodes[0].getconsultation(hash)["answers"][1]['status'], statusesAnswer[i])
            assert_equal(self.nodes[0].getconsultation(hash)['status'], statuses[i])
            count = self.nodes[0].getinfo()["blocks"]
            self.end_cycle_stake(self.nodes[0])
            self.stake_block(self.nodes[0] , 1)
            self.nodes[0].invalidateblock(self.nodes[0].getblockhash(count -2))
            self.stake_block(self.nodes[0] , 3)
            assert_equal(self.nodes[0].getinfo()["blocks"], count)
            assert_equal(self.nodes[0].getconsultation(hash)["answers"][0]['status'], statusesAnswer[i])
            assert_equal(self.nodes[0].getconsultation(hash)["answers"][1]['status'], statusesAnswer[i])
            assert_equal(self.nodes[0].getconsultation(hash)['status'], statuses[i])
            self.end_cycle_stake(self.nodes[0])
            self.stake_block(self.nodes[0] , 1)
            i = i + 1
            assert_equal(self.nodes[0].getconsultation(hash)["answers"][0]['status'], statusesAnswer[i])
            assert_equal(self.nodes[0].getconsultation(hash)["answers"][1]['status'], statusesAnswer[i])
            assert_equal(self.nodes[0].getconsultation(hash)['status'], statuses[i])

        self.nodes[0].consultationvote(consultation["answers"][0]['hash'], 'yes')
        self.stake_block(self.nodes[0], 1)

        assert_equal(self.nodes[0].getconsultation(hash)["answers"][0]['votes'], 1)
        assert_equal(self.nodes[0].getconsultation(hash)["answers"][1]['votes'], 0)
        assert_equal(self.nodes[0].getconsultation(hash2)["answers"][0]['votes'], 0)
        assert_equal(self.nodes[0].getconsultation(hash2)["answers"][1]['votes'], 0)

        self.nodes[0].consultationvote(consultation2["answers"][0]['hash'], 'yes')

        self.stake_block(self.nodes[0], 1)
        assert_equal(self.nodes[0].getconsultation(hash)["answers"][0]['votes'], 2)
        assert_equal(self.nodes[0].getconsultation(hash)["answers"][1]['votes'], 0)
        assert_equal(self.nodes[0].getconsultation(hash2)["answers"][0]['votes'], 1)
        assert_equal(self.nodes[0].getconsultation(hash2)["answers"][1]['votes'], 0)

        self.nodes[0].consultationvote(consultation2["answers"][0]['hash'], 'remove')
        self.nodes[0].consultationvote(consultation2["answers"][1]['hash'], 'yes')

        self.stake_block(self.nodes[0], 1)
        assert_equal(self.nodes[0].getconsultation(hash)["answers"][0]['votes'], 3)
        assert_equal(self.nodes[0].getconsultation(hash)["answers"][1]['votes'], 0)
        assert_equal(self.nodes[0].getconsultation(hash2)["answers"][0]['votes'], 1)
        assert_equal(self.nodes[0].getconsultation(hash2)["answers"][1]['votes'], 1)

        self.nodes[0].consultationvote(consultation["answers"][0]['hash'], 'remove')

        self.stake_block(self.nodes[0], 1)
        assert_equal(self.nodes[0].getconsultation(hash)["answers"][0]['votes'], 3)
        assert_equal(self.nodes[0].getconsultation(hash)["answers"][1]['votes'], 0)
        assert_equal(self.nodes[0].getconsultation(hash2)["answers"][0]['votes'], 1)
        assert_equal(self.nodes[0].getconsultation(hash2)["answers"][1]['votes'], 2)

        self.nodes[0].consultationvote(consultation["answers"][1]['hash'], 'yes')
        self.nodes[0].consultationvote(consultation2["answers"][0]['hash'], 'yes')
        self.nodes[0].consultationvote(consultation2["answers"][1]['hash'], 'remove')

        self.stake_block(self.nodes[0], 1)
        assert_equal(self.nodes[0].getconsultation(hash)["answers"][0]['votes'], 3)
        assert_equal(self.nodes[0].getconsultation(hash)["answers"][1]['votes'], 1)
        assert_equal(self.nodes[0].getconsultation(hash2)["answers"][0]['votes'], 2)
        assert_equal(self.nodes[0].getconsultation(hash2)["answers"][1]['votes'], 2)

        self.stake_block(self.nodes[0], 2)
        assert_equal(self.nodes[0].getconsultation(hash)["answers"][0]['votes'], 3)
        assert_equal(self.nodes[0].getconsultation(hash)["answers"][1]['votes'], 3)
        assert_equal(self.nodes[0].getconsultation(hash2)["answers"][0]['votes'], 4)
        assert_equal(self.nodes[0].getconsultation(hash2)["answers"][1]['votes'], 2)

        self.nodes[0].consultationvote(consultation["answers"][1]['hash'], 'remove')
        self.nodes[0].consultationvote(consultation["answers"][0]['hash'], 'yes')
        self.nodes[0].consultationvote(consultation2["answers"][0]['hash'], 'remove')
        self.nodes[0].consultationvote(consultation2["answers"][1]['hash'], 'yes')

        count = self.nodes[0].getinfo()["blocks"]
        self.stake_block(self.nodes[0], 2)
        assert_equal(self.nodes[0].getconsultation(hash)["answers"][0]['votes'], 5)
        assert_equal(self.nodes[0].getconsultation(hash)["answers"][1]['votes'], 3)
        assert_equal(self.nodes[0].getconsultation(hash2)["answers"][0]['votes'], 4)
        assert_equal(self.nodes[0].getconsultation(hash2)["answers"][1]['votes'], 4)

        self.end_cycle_stake(self.nodes[0])
        self.stake_block(self.nodes[0], 1)

        assert_equal(self.nodes[0].getconsultation(hash)["answers"][0]['votes'], 1)
        assert_equal(self.nodes[0].getconsultation(hash)["answers"][1]['votes'], 0)
        assert_equal(self.nodes[0].getconsultation(hash2)["answers"][0]['votes'], 0)
        assert_equal(self.nodes[0].getconsultation(hash2)["answers"][1]['votes'], 1)

        self.nodes[0].invalidateblock(self.nodes[0].getblockhash(count + 1))

        assert_equal(self.nodes[0].getconsultation(hash)["answers"][0]['votes'], 3)
        assert_equal(self.nodes[0].getconsultation(hash)["answers"][1]['votes'], 3)
        assert_equal(self.nodes[0].getconsultation(hash2)["answers"][0]['votes'], 4)
        assert_equal(self.nodes[0].getconsultation(hash2)["answers"][1]['votes'], 2)

        self.stake_block(self.nodes[0], 2)
        assert_equal(self.nodes[0].getconsultation(hash)["answers"][0]['votes'], 5)
        assert_equal(self.nodes[0].getconsultation(hash)["answers"][1]['votes'], 3)
        assert_equal(self.nodes[0].getconsultation(hash2)["answers"][0]['votes'], 4)
        assert_equal(self.nodes[0].getconsultation(hash2)["answers"][1]['votes'], 4)

        self.end_cycle_stake(self.nodes[0])
        self.stake_block(self.nodes[0], 1)

        assert_equal(self.nodes[0].getconsultation(hash)["answers"][0]['votes'], 1)
        assert_equal(self.nodes[0].getconsultation(hash)["answers"][1]['votes'], 0)
        assert_equal(self.nodes[0].getconsultation(hash2)["answers"][0]['votes'], 0)
        assert_equal(self.nodes[0].getconsultation(hash2)["answers"][1]['votes'], 1)

        self.nodes[0].consultationvote(consultation["answers"][0]['hash'], 'remove')
        self.nodes[0].consultationvote(consultation2["answers"][1]['hash'], 'remove')

        self.end_cycle_stake(self.nodes[0])
        self.stake_block(self.nodes[0], 1)

        assert_equal(self.nodes[0].getconsultation(hash)["answers"][0]['votes'], 0)
        assert_equal(self.nodes[0].getconsultation(hash)["answers"][1]['votes'], 0)
        assert_equal(self.nodes[0].getconsultation(hash2)["answers"][0]['votes'], 0)
        assert_equal(self.nodes[0].getconsultation(hash2)["answers"][1]['votes'], 0)

    def stake_block(self, node, count=1):
        # Get the current block count to check against while we wait for a stake
        blockcount = node.getblockcount()
        lastblock = blockcount

        # Turn staking on
        node.staking(True)

        print("Trying to stake", count, "blocks")

        # wait for a new block to be mined
        while node.getblockcount() <= blockcount+count-1:
            # print("waiting for a new block...")
            time.sleep(1)
            if node.getblockcount() != lastblock:
                lastblock = node.getblockcount()
                print("Staked block at height", lastblock)

        # Turn staking off
        node.staking(False)

    def end_cycle_stake(self, node):
        # Move to the end of the cycle
        self.stake_block(node, node.cfundstats()["votingPeriod"]["ending"] - node.cfundstats()["votingPeriod"][
            "current"])

if __name__ == '__main__':
    ConsultationsTest().main()
