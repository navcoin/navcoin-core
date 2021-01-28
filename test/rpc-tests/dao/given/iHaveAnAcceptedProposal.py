#!/usr/bin/env python3
# Copyright (c) 2019 The Navcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

#
# Expanded helper routines for regression testing of the NAV Coin community fund
#

import sys, os #include the parent folder so the test_framework is available
sys.path.insert(1, os.path.abspath(os.path.join(os.path.dirname( __file__ ), '..')))

from test_framework.util import *
from dao.given import (givenIHaveActivatedTheCFund, 
  givenIHaveDonatedToTheCFund, 
  givenIHaveCreatedANewAddress, 
  givenIHaveCreatedAProposal, 
  givenIHaveVotedOnTheProposal)

from dao.when import *
from dao.then import *

def givenIHaveAnAcceptedProposal(node=None, 
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
    print('givenIHaveAnAcceptedProposal: invalid parameters')
    assert(False)

  givenIHaveActivatedTheCFund(node)
  givenIHaveDonatedToTheCFund(node, amount)

  if (address == False):
    address = givenIHaveCreatedANewAddress(node)["pubkey"]

  hash = givenIHaveCreatedAProposal(node, address, amount, duration, description)

  givenIHaveVotedOnTheProposal(node, hash, 'yes')

  whenTheVotingCycleEnds(node, 2)
  thenTheProposalShouldBeAccepted(node, hash)

  return {
    "hash": hash,
    "address": address
  } 