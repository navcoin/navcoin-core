#!/usr/bin/env python3
# Copyright (c) 2019 The NavCoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

#
# Expanded helper routines for regression testing of the NAV Coin community fund
#

from test_framework.util import *

def givenIHaveVotedOnTheProposal(node=None, 
hash=None,
vote=None):

  if (node is None
  or hash is None
  or (vote != 'yes' and vote != 'no')):
    print('givenIHaveVotedOnTheProposal: invalid parameters')
    assert(False)

  try:
    node.proposalvote(hash, vote)
  except JSONRPCException as e:
    print(e.error)
    assert(False)
  
  try:
    myvotes = node.proposalvotelist()
  except JSONRPCException as e:
    print(e.error)
    assert(False)

  voteFound = False

  for myvote in myvotes[vote]:
    if (myvote["hash"] == hash):
      voteFound = True

  if (voteFound == False):
    print("Vote Not Found")
    print("proposalvotelist: ", myvotes)
    assert(False)

    
