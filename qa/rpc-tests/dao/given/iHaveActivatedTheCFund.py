#!/usr/bin/env python3
# Copyright (c) 2019 The NavCoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

#
# Expanded helper routines for regression testing of the NAV Coin community fund
#

from test_framework.util import *

def givenIHaveActivatedTheCFund(node=None, 
text=None, 
questions=None, 
withAnswers=False):

  if (node is None):
    print('givenIHaveActivatedTheCFund: invalid parameters')
    assert(False)

  slow_gen(node, 100)
  # Verify the Community Fund is started
  assert (get_bip9_status(node, "communityfund")["status"] == "started")

  slow_gen(node, 100)
  # Verify the Community Fund is locked_in
  assert (get_bip9_status(node, "communityfund")["status"] == "locked_in")

  slow_gen(node, 100)
  # Verify the Community Fund is active
  assert (get_bip9_status(node, "communityfund")["status"] == "active")
  