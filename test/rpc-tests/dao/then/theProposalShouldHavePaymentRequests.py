#!/usr/bin/env python3
# Copyright (c) 2019 The NavCoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

#
# Expanded helper routines for regression testing of the NAV Coin community fund
#

from test_framework.util import *

def thenTheProposalShouldHavePaymentRequests(node=None, 
proposalHash=None,
preqHashes=None):

  if (node is None
  or proposalHash is None
  or preqHashes is None):
    print('thenTheProposalShouldHavePaymentRequests: invalid parameters')
    assert(False)

  try:
    proposal = node.getproposal(proposalHash)
  except JSONRPCException as e:
    print(e.error)
    assert(False)

  if (len(preqHashes) == 0 and len(proposal["paymentRequests"]) == 0):
    assert(True)
    return
  
  proposalPreqHashes = []

  # Check all payment requests attached to the proposal are parsed
  for paymentRequest in proposal["paymentRequests"]:
    proposalPreqHashes.append(paymentRequest["hash"])
    assert(paymentRequest["hash"] in preqHashes)

  # Check all payment request hashes parsed are attached to the proposal
  for preqHash in preqHashes:
    assert(preqHash in proposalPreqHashes) 