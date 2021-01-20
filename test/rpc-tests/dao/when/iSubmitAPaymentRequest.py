#!/usr/bin/env python3
# Copyright (c) 2019 The Navcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

#
# Expanded helper routines for regression testing of the NAV Coin community fund
#

from test_framework.util import *

def whenISubmitAPaymentRequest(node=None, 
hash=None,
amount=None,
description=None,
expectSuccess=True):

  if (node is None
  or hash is None
  or amount is None
  or description is None
  or (expectSuccess != True and expectSuccess != False)):
    print('whenISubmitAPaymentRequest: invalid parameters')
    assert(False)
  
  try:
    hash = node.createpaymentrequest(hash, amount, description)["hash"]
  except JSONRPCException as e:
    
    if (expectSuccess == True):
      print(e.error)
      assert(False)

    assert(e.error["code"] == -3)
    assert(e.error["message"] == "Proposal has not been accepted.")
    return
  
  slow_gen(node, 1)

  paymentRequest = node.getpaymentrequest(hash)

  assert(paymentRequest["hash"] == hash)
  assert(paymentRequest["blockHash"] == node.getbestblockhash())
  assert(satoshi_round(paymentRequest["requestedAmount"]) == satoshi_round(amount))
  assert(paymentRequest["description"] == description)
  assert(paymentRequest["status"] == 'pending')
  assert(paymentRequest["state"] == 0)
  assert(paymentRequest["votingCycle"] == 0)
  assert(paymentRequest["votesNo"] == 0)
  assert(paymentRequest["votesYes"] == 0)

  return hash