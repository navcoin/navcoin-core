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
        self.nodes = start_nodes(self.num_nodes, self.options.tmpdir, [['-debug=dao']])

    def run_test(self):
        self.nodes[0].staking(False)
        activate_softfork(self.nodes[0], "dao_consensus")

        proposal = self.nodes[0].proposeconsensuschange(0, 7)['hash']

        slow_gen(self.nodes[0] , 1)

        assert ( self.nodes[0].listconsultations()[0]['hash'] == proposal)

        first_answer = self.nodes[0].getconsultation(proposal)['answers'][0]['hash']

        second_answer = self.nodes[0].proposeanswer(proposal, "5")["hash"]
        third_answer_not_supported = self.nodes[0].proposeanswer(proposal, "6")["hash"]

        slow_gen(self.nodes[0] , 1)
        end_cycle(self.nodes[0])

        for consultation in self.nodes[0].listconsultations():
            answers = []
            for answer in consultation["answers"]:
                if answer:
                    answers.append(answer["hash"])
            assert(second_answer in answers and third_answer_not_supported in answers)

        self.nodes[0].support(first_answer)
        self.nodes[0].support(second_answer)

        slow_gen(self.nodes[0] , 1)

        #cycle 1

        assert_equal(self.nodes[0].getconsultation(proposal)["status"], "found support, waiting for end of voting period")
        assert_equal(self.nodes[0].getconsultation(proposal)["votingCyclesFromCreation"], 1)

        end_cycle(self.nodes[0])
        slow_gen(self.nodes[0] , 1)

        #cycle 2

        assert_equal(self.nodes[0].getconsultation(proposal)["status"], "found support")
        assert_equal(self.nodes[0].getconsultation(proposal)["votingCyclesFromCreation"], 2)

        end_cycle(self.nodes[0])
        slow_gen(self.nodes[0] , 1)

        #cycle 3

        assert_equal(self.nodes[0].getconsultation(proposal)["status"], "reflection phase")
        assert_equal(self.nodes[0].getconsultation(proposal)["votingCyclesFromCreation"], 3)

        phash=self.nodes[0].createproposal(self.nodes[0].getnewaddress(), 1, 10000, "test")["hash"]
        createdin=self.nodes[0].getblockcount()

        end_cycle(self.nodes[0])
        slow_gen(self.nodes[0] , 1)

        #cycle 4

        assert_equal(self.nodes[0].getconsultation(proposal)["status"], "voting started")
        assert_equal(self.nodes[0].getconsultation(proposal)["votingCyclesFromCreation"], 4)


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

        assert_equal(self.nodes[0].getproposal(phash)["votingCycle"], 1)

        self.nodes[0].consultationvote(second_answer, "yes")
        blocks=end_cycle(self.nodes[0])
        self.nodes[0].generate(1)
        #cycle 4

        assert(self.nodes[0].getconsultation(proposal)["status"] == "passed")

        assert_equal(self.nodes[0].getconsensusparameters()[0], 5)
        assert_equal(self.nodes[0].getproposal(phash)["votingCycle"], 4)
        self.nodes[0].invalidateblock(blocks[-1])
        assert_equal(self.nodes[0].getconsensusparameters()[0], 10)
        assert_equal(self.nodes[0].getproposal(phash)["votingCycle"], 1)
        self.nodes[0].generate(2)
        assert_equal(self.nodes[0].getconsensusparameters()[0], 5)
        assert_equal(self.nodes[0].getproposal(phash)["votingCycle"], 4)


if __name__ == '__main__':
    ConsensusConsultationsTest().main()
