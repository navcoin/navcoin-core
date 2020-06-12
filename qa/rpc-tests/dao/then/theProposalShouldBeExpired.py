#!/usr/bin/env python3
# Copyright (c) 2019 The NavCoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

#
# Expanded helper routines for regression testing of the NAV Coin community fund
#

from test_framework.util import *

def thenTheProposalShouldBeExpired(node=None,
proposalHash=None):

  if (node is None
  or proposalHash is None):
    print('thenTheProposalShouldBeExpired: invalid parameters')
    assert(False)

  try:
    proposal = node.getproposal(proposalHash)
  except JSONRPCException as e:
    print(e.error)
    assert(False)

  try:
    maxCycles = node.cfundstats()["consensus"]["maxCountVotingCycleProposals"]
  except JSONRPCException as e:
    print(e.error)
    assert(False)

  assert(proposal["votingCycle"] == maxCycles)
  assert(proposal["status"] == 'expired')
  assert(proposal["state"] == 3)
