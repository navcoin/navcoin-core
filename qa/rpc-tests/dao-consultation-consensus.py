#!/usr/bin/env python3
# Copyright (c) 2019 The Navcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

from test_framework.test_framework import NavCoinTestFramework
from test_framework.cfund_util import *

import time


class ConsensusConsultationsTest(NavCoinTestFramework):
    """Tests the consultations of the DAO"""

    def __init__(self):
        super().__init__()
        self.setup_clean_chain = True
        self.num_nodes = 1

    def setup_network(self, split=False):
        self.nodes = []
        self.nodes = start_nodes(self.num_nodes, self.options.tmpdir, [[]])

    def run_test(self):
        # Get cfund parameters
        blocks_per_voting_cycle = self.nodes[0].cfundstats()["consensus"]["blocksPerVotingCycle"]

        self.nodes[0].staking(False)
        activate_softfork(self.nodes[0], "dao_consensus")

        proposal = self.nodes[0].proposeconsensuschange(21, 300000000)['hash']

        slow_gen(self.nodes[0] , 1)

        assert ( self.nodes[0].listconsultations()[0]['hash'] == proposal)

        first_answer = self.nodes[0].getconsultation(proposal)['answers'][0]['hash']

        second_answer = self.nodes[0].proposeanswer(proposal, "350000000")["hash"]
        third_answer_not_supported = self.nodes[0].proposeanswer(proposal, "400000000")["hash"]

        slow_gen(self.nodes[0] , 1)

        for consultation in self.nodes[0].listconsultations():
            answers = []
            for answer in consultation["answers"]:
                if answer:
                    answers.append(answer["hash"])
            assert(second_answer in answers and third_answer_not_supported in answers)

        self.nodes[0].support(first_answer)
        self.nodes[0].support(second_answer)

        end_cycle(self.nodes[0])
        slow_gen(self.nodes[0] , 1)

        #cycle 1

        assert(self.nodes[0].getconsultation(proposal)["status"] == "waiting for support")

        end_cycle(self.nodes[0])
        slow_gen(self.nodes[0] , 1)

        #cycle 2

        assert(self.nodes[0].getconsultation(proposal)["status"] == "found support, waiting for end of voting period")

        end_cycle(self.nodes[0])
        slow_gen(self.nodes[0] , 1)

        #cycle 3

        assert(self.nodes[0].getconsultation(proposal)["status"] == "reflection phase")

        end_cycle(self.nodes[0])
        slow_gen(self.nodes[0] , 1)

        #cycle 4

        assert(self.nodes[0].getconsultation(proposal)["status"] == "voting started")


        try:
            self.nodes[0].consultationvote(proposal,"yes")
            raise AssertionError('Consultations cannot be directly voted')
        except JSONRPCException as e:
            assert(e.error['code']==-5)

        try:
            self.nodes[0].consultationvote(third_answer_not_supported,"yes")
            raise AssertionError('Not supported answers can not be voted')
        except JSONRPCException as e:
            assert(e.error['code']==-5)

        self.nodes[0].consultationvote(second_answer, "yes")

        end_cycle(self.nodes[0])
        slow_gen(self.nodes[0], 1)

        #cycle 4

        assert(self.nodes[0].getconsultation(proposal)["status"] == "passed")

        self.nodes[0].staking(True)

        current_balance = self.nodes[0].getbalance()
        block_height = self.nodes[0].getblockcount()
        while (block_height  >= self.nodes[0].getblockcount()):
            time.sleep(1)

        assert(self.nodes[0].gettransaction(self.nodes[0].getblock(self.nodes[0].getbestblockhash())["tx"][1])["amount"] == Decimal('2.80000000')) # 3.5 * 0.8

        self.nodes[0].staking(False)

        proposal = self.nodes[0].proposeconsensuschange(21, 50000000)['hash']

        slow_gen(self.nodes[0] , 1)

        assert ( self.nodes[0].listconsultations()[0]['hash'] == proposal)

        first_answer = self.nodes[0].getconsultation(proposal)['answers'][0]['hash']

        second_answer = self.nodes[0].proposeanswer(proposal, "250000000")["hash"]

        try:
            third_answer_not_supported = self.nodes[0].proposeanswer(proposal, self.nodes[0].getconsensusparameters()[21])["hash"]
            raise AssertionError('Proposing the current value should not be possible')
        except JSONRPCException as e:
            pass

        slow_gen(self.nodes[0] , 1)

        self.nodes[0].support(first_answer)
        self.nodes[0].support(second_answer)

        end_cycle(self.nodes[0])
        slow_gen(self.nodes[0] , 1)

        #cycle 1

        assert(self.nodes[0].getconsultation(proposal)["status"] == "waiting for support")

        end_cycle(self.nodes[0])
        slow_gen(self.nodes[0] , 1)

        #cycle 2

        assert(self.nodes[0].getconsultation(proposal)["status"] == "found support, waiting for end of voting period")

        end_cycle(self.nodes[0])
        slow_gen(self.nodes[0] , 1)

        #cycle 3

        assert(self.nodes[0].getconsultation(proposal)["status"] == "reflection phase")

        end_cycle(self.nodes[0])
        slow_gen(self.nodes[0] , 1)

        #cycle 4

        assert(self.nodes[0].getconsultation(proposal)["status"] == "voting started")


        try:
            self.nodes[0].consultationvote(proposal,"yes")
            raise AssertionError('Consultations cannot be directly voted')
        except JSONRPCException as e:
            assert(e.error['code']==-5)

        try:
            self.nodes[0].consultationvote(third_answer_not_supported,"yes")
            raise AssertionError('Not supported answers can not be voted')
        except JSONRPCException as e:
            assert(e.error['code']==-5)

        self.nodes[0].consultationvote(second_answer, "yes")

        end_cycle(self.nodes[0])
        slow_gen(self.nodes[0], 1)

        #cycle 4

        assert(self.nodes[0].getconsultation(proposal)["status"] == "passed")

        self.nodes[0].staking(True)

        current_balance = self.nodes[0].getbalance()
        block_height = self.nodes[0].getblockcount()
        while (block_height  >= self.nodes[0].getblockcount()):
            time.sleep(1)

        assert(self.nodes[0].gettransaction(self.nodes[0].getblock(self.nodes[0].getbestblockhash())["tx"][1])["amount"] == Decimal('2.00000000')) # 2.5 * 0.8

if __name__ == '__main__':
    ConsensusConsultationsTest().main()
