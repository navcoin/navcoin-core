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
        self.nodes = start_nodes(self.num_nodes, self.options.tmpdir, [["-debug=dao"],["-debug=dao"]])

    def run_test(self):
        # Get cfund parameters
        blocks_per_voting_cycle = self.nodes[0].cfundstats()["consensus"]["blocksPerVotingCycle"]

        self.nodes[0].staking(False)
        activate_softfork(self.nodes[0], "consultations")

        # created_consultations = {}
        #
        # created_consultations["one_of_two"] = self.nodes[0].createconsultation("one_of_two", 1, 1)['hash']
        # created_consultations["one_of_two_not_enough_answers"] = self.nodes[0].createconsultation("one_of_two_not_enough_answers", 1, 1)['hash']
        # created_consultations["one_of_two_not_supported"] = self.nodes[0].createconsultation("one_of_two_not_supported", 1, 1)['hash']
        # created_consultations["range_one_one_hundred"] = self.nodes[0].createconsultation("range_one_one_hundred", 1, 100, True)['hash']
        # self.stake_block(self.nodes[0] , 1)
        #
        # # Verify the proposals are now in the proposals list with matching hashes
        # desc_lst = []
        #
        # for consultation in self.nodes[0].listconsultations():
        #     desc_lst.append(consultation['question'])
        #     assert (created_consultations[consultation['question']] == consultation['hash'])
        # assert (set(desc_lst) == set(created_consultations.keys()))
        #
        # created_answers = {}
        #
        # created_answers["one_of_two"] = []
        # created_answers["one_of_two"].append(self.nodes[0].proposeanswer(created_consultations["one_of_two"],"one")["hash"])
        # created_answers["one_of_two"].append(self.nodes[0].proposeanswer(created_consultations["one_of_two"],"two")["hash"])
        # created_answers["one_of_two_not_enough_answers"] = []
        # created_answers["one_of_two_not_enough_answers"].append(self.nodes[0].proposeanswer(created_consultations["one_of_two_not_enough_answers"],"one")["hash"])
        # created_answers["one_of_two_not_supported"] = []
        # created_answers["one_of_two_not_supported"].append(self.nodes[0].proposeanswer(created_consultations["one_of_two_not_supported"],"one")["hash"])
        # created_answers["one_of_two_not_supported"].append(self.nodes[0].proposeanswer(created_consultations["one_of_two_not_supported"],"two")["hash"])
        # created_answers["range_one_one_hundred"] = []
        #
        # self.stake_block(self.nodes[0] , 1)
        #
        # for consultation in self.nodes[0].listconsultations():
        #     answers = []
        #     for answer in consultation["answers"]:
        #         if answer:
        #             answers.append(answer["hash"])
        #     assert(set(answers) == set(created_answers[consultation["question"]]))
        #
        # for consultation in list(set(created_answers["one_of_two"] + created_answers["one_of_two_not_enough_answers"])):
        #     self.nodes[0].support(consultation)
        # self.nodes[0].support(created_consultations["range_one_one_hundred"])
        #
        # self.end_cycle_stake(self.nodes[0])
        # self.stake_block(self.nodes[0] , 1)
        #
        # #cycle 1
        #
        # assert(self.nodes[0].getconsultation(created_consultations["one_of_two"])["status"] == "waiting for support")
        # assert(self.nodes[0].getconsultation(created_consultations["one_of_two_not_enough_answers"])["status"] == "waiting for support, waiting for having enough supported answers")
        # assert(self.nodes[0].getconsultation(created_consultations["one_of_two_not_supported"])["status"] == "waiting for support, waiting for having enough supported answers")
        # assert(self.nodes[0].getconsultation(created_consultations["range_one_one_hundred"])["status"] == "waiting for support")
        #
        # self.end_cycle_stake(self.nodes[0])
        # self.stake_block(self.nodes[0] , 1)
        #
        # #cycle 2
        #
        # assert(self.nodes[0].getconsultation(created_consultations["one_of_two"])["status"] == "found support, waiting for end of voting period")
        # assert(self.nodes[0].getconsultation(created_consultations["one_of_two_not_enough_answers"])["status"] == "waiting for support, waiting for having enough supported answers")
        # assert(self.nodes[0].getconsultation(created_consultations["one_of_two_not_supported"])["status"] == "waiting for support, waiting for having enough supported answers")
        # assert(self.nodes[0].getconsultation(created_consultations["range_one_one_hundred"])["status"] == "found support, waiting for end of voting period")
        #
        # self.end_cycle_stake(self.nodes[0])
        # self.stake_block(self.nodes[0] , 1)
        #
        # #cycle 3
        #
        # assert(self.nodes[0].getconsultation(created_consultations["one_of_two"])["status"] == "reflection phase")
        # assert(self.nodes[0].getconsultation(created_consultations["one_of_two_not_enough_answers"])["status"] == "waiting for support, waiting for having enough supported answers")
        # assert(self.nodes[0].getconsultation(created_consultations["one_of_two_not_supported"])["status"] == "waiting for support, waiting for having enough supported answers")
        # assert(self.nodes[0].getconsultation(created_consultations["range_one_one_hundred"])["status"] == "reflection phase")
        #
        # self.end_cycle_stake(self.nodes[0])
        #
        # #cycle 4
        #
        # assert(self.nodes[0].getconsultation(created_consultations["one_of_two"])["status"] == "voting started")
        # assert(self.nodes[0].getconsultation(created_consultations["one_of_two_not_enough_answers"])["status"] == "waiting for support, waiting for having enough supported answers")
        # assert(self.nodes[0].getconsultation(created_consultations["one_of_two_not_supported"])["status"] == "waiting for support, waiting for having enough supported answers")
        # assert(self.nodes[0].getconsultation(created_consultations["range_one_one_hundred"])["status"] == "voting started")
        #
        # try:
        #     self.nodes[0].consultationvote(created_consultations["one_of_two"],"yes")
        #     raise AssertionError('Consultations cannot be directly voted')
        # except JSONRPCException as e:
        #     assert(e.error['code']==-5)
        #
        # try:
        #     self.nodes[0].consultationvote(created_consultations["one_of_two_not_enough_answers"],"yes")
        #     raise AssertionError('Not supported consultations cannot be voted')
        # except JSONRPCException as e:
        #     assert(e.error['code']==-5)
        #
        # try:
        #     self.nodes[0].consultationvote(created_consultations["one_of_two_not_supported"],"yes")
        #     raise AssertionError('Not supported consultations cannot be voted')
        # except JSONRPCException as e:
        #     assert(e.error['code']==-5)
        #
        # for answer in created_answers["one_of_two"]:
        #     self.nodes[0].consultationvote(answer,"yes")
        #
        # try:
        #     self.nodes[0].consultationvote(created_consultations["range_one_one_hundred"],"value",0)
        #     raise AssertionError('Vote out of range cant be allowed')
        # except JSONRPCException as e:
        #     assert(e.error['code']==-32602)
        #
        # try:
        #     self.nodes[0].consultationvote(created_consultations["range_one_one_hundred"],"value",101)
        #     raise AssertionError('Vote out of range cant be allowed')
        # except JSONRPCException as e:
        #     assert(e.error['code']==-32602)
        #
        # self.nodes[0].consultationvote(created_consultations["range_one_one_hundred"],"value",50)
        #
        # self.stake_block(self.nodes[0] , 5)
        #
        # valid = 0
        # finished = 0
        # round = 0
        #
        # while True:
        #     for consultation in self.nodes[0].listconsultations():
        #         if consultation["status"] == "voting started":
        #             count = 0
        #             if not consultation["answers"] == [{}]:
        #                 for answer in consultation["answers"]:
        #                     if not "votes" in answer.keys():
        #                         count = count + int(sum(answer.values()))
        #                     else:
        #                         count = count + answer["votes"]
        #                 assert_equal(count, 5)
        #                 valid = valid + 1
        #         elif consultation["status"] == "finished, waiting for end of voting period":
        #             valid = valid + 1
        #         if consultation["status"] == "finished":
        #             finished = finished + 1
        #     self.stake_block(self.nodes[0], self.nodes[0].getconsensusparameters()[0])
        #     if finished == 2:
        #         break
        #     finished = 0
        #     assert(valid==2)
        #     valid = 0
        #     round = round + 1
        #     assert(round <= self.nodes[0].getconsensusparameters()[0])

        self.end_cycle_stake(self.nodes[0])

        hash = self.nodes[0].createconsultationwithanswers("question",["a","b"])['hash']
        self.stake_block(self.nodes[0], 1)

        consultation = self.nodes[0].getconsultation(hash)
        assert_equal(len(consultation["answers"]), 2)

        self.nodes[0].support(consultation["answers"][0]['hash'])

        self.stake_block(self.nodes[0], 2)
        assert_equal(self.nodes[0].getconsultation(hash)["answers"][0]['support'], 2)
        assert_equal(self.nodes[0].getconsultation(hash)["answers"][1]['support'], 0)

        self.nodes[0].support(consultation["answers"][0]['hash'], 'remove')
        self.nodes[0].support(consultation["answers"][1]['hash'])

        self.stake_block(self.nodes[0], 2)
        assert_equal(self.nodes[0].getconsultation(hash)["answers"][0]['support'], 2)
        assert_equal(self.nodes[0].getconsultation(hash)["answers"][1]['support'], 2)

        self.nodes[0].support(consultation["answers"][0]['hash'])

        self.stake_block(self.nodes[0], 2)
        assert_equal(self.nodes[0].getconsultation(hash)["answers"][0]['support'], 4)
        assert_equal(self.nodes[0].getconsultation(hash)["answers"][1]['support'], 4)

        for i in range(self.nodes[0].getconsensusparameters()[2] + self.nodes[0].getconsensusparameters()[5] + 1):
            self.end_cycle_stake(self.nodes[0])
            self.stake_block(self.nodes[0] , 1)

        self.nodes[0].consultationvote(consultation["answers"][0]['hash'], 'yes')
        self.stake_block(self.nodes[0], 1)
        self.nodes[0].consultationvote(consultation["answers"][0]['hash'], 'remove')

        assert_equal(self.nodes[0].getconsultation(hash)["answers"][0]['votes'], 1)
        self.stake_block(self.nodes[0], 1)
        assert_equal(self.nodes[0].getconsultation(hash)["answers"][0]['votes'], 1)

        self.nodes[0].consultationvote(consultation["answers"][0]['hash'], 'yes')
        self.stake_block(self.nodes[0], 1)
        assert_equal(self.nodes[0].getconsultation(hash)["answers"][0]['votes'], 2)

        self.nodes[0].consultationvote(consultation["answers"][1]['hash'], 'yes')
        self.stake_block(self.nodes[0], 1)
        assert_equal(self.nodes[0].getconsultation(hash)["answers"][0]['votes'], 3)

        self.nodes[0].consultationvote(consultation["answers"][0]['hash'], 'remove')
        self.stake_block(self.nodes[0], 1)
        assert_equal(self.nodes[0].getconsultation(hash)["answers"][0]['votes'], 3)
        assert_equal(self.nodes[0].getconsultation(hash)["answers"][1]['votes'], 1)

        self.stake_block(self.nodes[0], 2)
        assert_equal(self.nodes[0].getconsultation(hash)["answers"][0]['votes'], 3)
        assert_equal(self.nodes[0].getconsultation(hash)["answers"][1]['votes'], 3)

        self.nodes[0].consultationvote(consultation["answers"][1]['hash'], 'remove')
        self.nodes[0].consultationvote(consultation["answers"][0]['hash'], 'yes')

        self.stake_block(self.nodes[0], 2)
        assert_equal(self.nodes[0].getconsultation(hash)["answers"][0]['votes'], 5)
        assert_equal(self.nodes[0].getconsultation(hash)["answers"][1]['votes'], 3)

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
