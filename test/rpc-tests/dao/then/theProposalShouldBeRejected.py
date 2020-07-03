#!/usr/bin/env python3
# Copyright (c) 2019 The NavCoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

#
# Expanded helper routines for regression testing of the NAV Coin community fund
#

from test_framework.util import *

def thenTheProposalShouldBeRejected(node=None, 
proposalHash=None):

  if (node is None
  or proposalHash is None):
    print('thenTheProposalShouldBeRejected: invalid parameters')
    assert(False)

  try:
    proposal = node.getproposal(proposalHash)
  except JSONRPCException as e:
    print(e.error)
    assert(False)

  assert(proposal["status"] == "rejected")
  assert(proposal["state"] == 2)