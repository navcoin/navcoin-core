#!/usr/bin/env python3
# Copyright (c) 2019 The Navcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

import sys, os #include the parent folder so the test_framework is available
sys.path.insert(1, os.path.abspath(os.path.join(os.path.dirname( __file__ ), '..')))

from test_framework.test_framework import NavCoinTestFramework
from test_framework.cfund_util import *

from dao.given import *
from dao.when import *
from dao.then import *

import time

class CFund004ProposalExpiredPreq(NavCoinTestFramework):
  """It should create a proposal and let it expire"""

  def __init__(self):
      super().__init__()
      self.setup_clean_chain = True
      self.num_nodes = 1

  def setup_network(self, split=False):
      self.nodes = []
      self.nodes = start_nodes(self.num_nodes, self.options.tmpdir, [["-debug=dao"],["-debug=dao"]])

  def run_test(self):

      self.nodes[0].staking(False)

      # Given
      proposalDetails = givenIHaveAnExpiredProposal(self.nodes[0], False, 5000, 60*60*24, "This is my proposal")

      # When
      whenISubmitAPaymentRequest(self.nodes[0], proposalDetails["hash"], 2000, "This is my payment request", False)

      # Then
      thenTheProposalShouldHavePaymentRequests(self.nodes[0], proposalDetails["hash"], [])

if __name__ == '__main__':
    CFund004ProposalExpiredPreq().main()