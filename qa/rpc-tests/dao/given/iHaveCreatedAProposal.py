#!/usr/bin/env python3
# Copyright (c) 2019 The NavCoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

#
# Expanded helper routines for regression testing of the NAV Coin community fund
#

from test_framework.util import *

def givenIHaveCreatedAProposal(node=None, 
address=None,
amount=None,
duration=None,
description=None,
dump=False):

  if (node is None
  or address is None
  or amount is None
  or duration is None
  or description is None):
    print('givenIHaveCreatedAProposal: invalid parameters')
    assert(False)

  try:
    hash = node.createproposal(address, amount, duration, description)["hash"]
  except JSONRPCException as e:
    print(e.error)
    assert(False)
  
  slow_gen(node, 1)

  proposal = node.getproposal(hash)

  assert(proposal["hash"] == hash)
  assert(proposal["blockHash"] == node.getbestblockhash())
  assert(proposal["paymentAddress"] == address)
  assert(satoshi_round(proposal["requestedAmount"]) == satoshi_round(amount))
  assert(proposal["proposalDuration"] == duration)
  assert(proposal["description"] == description)
  assert(satoshi_round(proposal["notPaidYet"]) == satoshi_round(amount))
  assert(proposal["status"] == 'pending')
  assert(proposal["state"] == 0)
  assert(proposal["votingCycle"] == 0)
  assert(proposal["votesNo"] == 0)
  assert(proposal["votesYes"] == 0)

  return hash 
