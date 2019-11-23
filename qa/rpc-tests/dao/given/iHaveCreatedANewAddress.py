#!/usr/bin/env python3
# Copyright (c) 2019 The NavCoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

#
# Expanded helper routines for regression testing of the NAV Coin community fund
#

from test_framework.util import *

def givenIHaveCreatedANewAddress(node=None,
returnPrivateKey=False):

  if (node is None):
    print('givenIHaveCreatedANewAddress: invalid parameters')
    assert(False)

  try:
    pubkey = node.getnewaddress()
    node.validateaddress(pubkey)
    keypair = {"pubkey": pubkey}
  except JSONRPCException as e:
    print(e.error)
    assert(False)
  
  if (returnPrivateKey):
    try:
      keypair["privkey"] = node.dumpprivkey(pubkey)
    except JSONRPCException as e:
      print(e.error)
      assert(False)
  
  return keypair
    
    

  




  
