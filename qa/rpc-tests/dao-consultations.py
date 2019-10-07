#!/usr/bin/env python3
# Copyright (c) 2019 The Navcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

from test_framework.test_framework import NavCoinTestFramework
from test_framework.cfund_util import *

import time


class ConsultationsTest(NavCoinTestFramework):
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
        activate_softfork(self.nodes[0], "consultations")

        created_consultations = {}

        created_consultations["one_of_two"] = self.nodes[0].createconsultation("one_of_two", 1, 1)['hash']
        created_consultations["one_of_two_not_enough_answers"] = self.nodes[0].createconsultation("one_of_two_not_enough_answers", 1, 1)['hash']
        created_consultations["one_of_two_not_supported"] = self.nodes[0].createconsultation("one_of_two_not_supported", 1, 1)['hash']
        created_consultations["range_one_one_hundred"] = self.nodes[0].createconsultation("range_one_one_hundred", 1, 100, True)['hash']
        slow_gen(self.nodes[0] , 1)

        # Verify the proposals are now in the proposals list with matching hashes
        desc_lst = []

        for consultation in self.nodes[0].listconsultations():
            desc_lst.append(consultation['question'])
            assert (created_consultations[consultation['question']] == consultation['hash'])
        assert (set(desc_lst) == set(created_consultations.keys()))

        created_answers = {}

        created_answers["one_of_two"] = []
        created_answers["one_of_two"].append(self.nodes[0].proposeanswer(created_consultations["one_of_two"],"one")["hash"])
        created_answers["one_of_two"].append(self.nodes[0].proposeanswer(created_consultations["one_of_two"],"two")["hash"])
        created_answers["one_of_two_not_enough_answers"] = []
        created_answers["one_of_two_not_enough_answers"].append(self.nodes[0].proposeanswer(created_consultations["one_of_two_not_enough_answers"],"one")["hash"])
        created_answers["one_of_two_not_supported"] = []
        created_answers["one_of_two_not_supported"].append(self.nodes[0].proposeanswer(created_consultations["one_of_two_not_supported"],"one")["hash"])
        created_answers["one_of_two_not_supported"].append(self.nodes[0].proposeanswer(created_consultations["one_of_two_not_supported"],"two")["hash"])
        created_answers["range_one_one_hundred"] = []

        slow_gen(self.nodes[0] , 1)

        for consultation in self.nodes[0].listconsultations():
            answers = []
            for answer in consultation["answers"]:
                if answer:
                    answers.append(answer["hash"])
            assert(set(answers) == set(created_answers[consultation["question"]]))

        for consultation in list(set(created_answers["one_of_two"] + created_answers["one_of_two_not_enough_answers"])):
            self.nodes[0].support(consultation)
        self.nodes[0].support(created_consultations["range_one_one_hundred"])

        end_cycle(self.nodes[0])
        slow_gen(self.nodes[0] , 1)

        #cycle 1

        assert(self.nodes[0].getconsultation(created_consultations["one_of_two"])["status"] == "waiting for support")
        assert(self.nodes[0].getconsultation(created_consultations["one_of_two_not_enough_answers"])["status"] == "waiting for support, waiting for having enough supported answers")
        assert(self.nodes[0].getconsultation(created_consultations["one_of_two_not_supported"])["status"] == "waiting for support, waiting for having enough supported answers")
        assert(self.nodes[0].getconsultation(created_consultations["range_one_one_hundred"])["status"] == "waiting for support")

        end_cycle(self.nodes[0])
        slow_gen(self.nodes[0] , 1)

        #cycle 2

        assert(self.nodes[0].getconsultation(created_consultations["one_of_two"])["status"] == "found support, waiting for end of voting period")
        assert(self.nodes[0].getconsultation(created_consultations["one_of_two_not_enough_answers"])["status"] == "waiting for support, waiting for having enough supported answers")
        assert(self.nodes[0].getconsultation(created_consultations["one_of_two_not_supported"])["status"] == "waiting for support, waiting for having enough supported answers")
        assert(self.nodes[0].getconsultation(created_consultations["range_one_one_hundred"])["status"] == "found support, waiting for end of voting period")

        end_cycle(self.nodes[0])
        slow_gen(self.nodes[0] , 1)

        #cycle 3

        assert(self.nodes[0].getconsultation(created_consultations["one_of_two"])["status"] == "reflection phase")
        assert(self.nodes[0].getconsultation(created_consultations["one_of_two_not_enough_answers"])["status"] == "waiting for support, waiting for having enough supported answers")
        assert(self.nodes[0].getconsultation(created_consultations["one_of_two_not_supported"])["status"] == "waiting for support, waiting for having enough supported answers")
        assert(self.nodes[0].getconsultation(created_consultations["range_one_one_hundred"])["status"] == "reflection phase")

        end_cycle(self.nodes[0])
        slow_gen(self.nodes[0] , 1)

        #cycle 4

        assert(self.nodes[0].getconsultation(created_consultations["one_of_two"])["status"] == "voting started")
        assert(self.nodes[0].getconsultation(created_consultations["one_of_two_not_enough_answers"])["status"] == "waiting for support, waiting for having enough supported answers")
        assert(self.nodes[0].getconsultation(created_consultations["one_of_two_not_supported"])["status"] == "waiting for support, waiting for having enough supported answers")
        assert(self.nodes[0].getconsultation(created_consultations["range_one_one_hundred"])["status"] == "voting started")

        try:
            self.nodes[0].consultationvote(created_consultations["one_of_two"],"yes")
            raise AssertionError('Consultations cannot be directly voted')
        except JSONRPCException as e:
            assert(e.error['code']==-5)

        try:
            self.nodes[0].consultationvote(created_consultations["one_of_two_not_enough_answers"],"yes")
            raise AssertionError('Not supported consultations cannot be voted')
        except JSONRPCException as e:
            assert(e.error['code']==-5)

        try:
            self.nodes[0].consultationvote(created_consultations["one_of_two_not_supported"],"yes")
            raise AssertionError('Not supported consultations cannot be voted')
        except JSONRPCException as e:
            assert(e.error['code']==-5)

        for answer in created_answers["one_of_two"]:
            self.nodes[0].consultationvote(answer,"yes")

        try:
            self.nodes[0].consultationvote(created_consultations["range_one_one_hundred"],"value",0)
            raise AssertionError('Vote out of range cant be allowed')
        except JSONRPCException as e:
            assert(e.error['code']==-32602)

        try:
            self.nodes[0].consultationvote(created_consultations["range_one_one_hundred"],"value",101)
            raise AssertionError('Vote out of range cant be allowed')
        except JSONRPCException as e:
            assert(e.error['code']==-32602)

        self.nodes[0].consultationvote(created_consultations["range_one_one_hundred"],"value",50)

        slow_gen(self.nodes[0] , 5)

        valid = 0
        finished = 0
        round = 0

        while True:
            for consultation in self.nodes[0].listconsultations():
                if consultation["status"] == "voting started":
                    count = 0
                    if not consultation["answers"] == [{}]:
                        for answer in consultation["answers"]:
                            if not "votes" in answer.keys():
                                count = count + int(sum(answer.values()))
                            else:
                                count = count + answer["votes"]
                        assert(count == 5)
                        valid = valid + 1
                elif consultation["status"] == "finished, waiting for end of voting period":
                    valid = valid + 1
                if consultation["status"] == "finished":
                    finished = finished + 1
            slow_gen(self.nodes[0], self.nodes[0].getconsensusparameters()[0])
            if finished == 2:
                break
            finished = 0
            assert(valid==2)
            valid = 0
            round = round + 1
            assert(round <= self.nodes[0].getconsensusparameters()[0])

if __name__ == '__main__':
    ConsultationsTest().main()
