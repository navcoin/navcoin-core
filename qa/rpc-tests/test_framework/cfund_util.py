#!/usr/bin/env python3
# Copyright (c) 2014-2016 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.


#
# Expanded helper routines for regression testing of the NAV Coin community fund
#

from test_framework.util import *


def activate_cfund(node):
    slow_gen(node, 100)
    # Verify the Community Fund is started
    assert (get_bip9_status(node, "communityfund")["status"] == "started")

    slow_gen(node, 100)
    # Verify the Community Fund is locked_in
    assert (get_bip9_status(node, "communityfund")["status"] == "locked_in")

    slow_gen(node, 100)
    # Verify the Community Fund is active
    assert (get_bip9_status(node, "communityfund")["status"] == "active")

def end_cycle(node):
    # Move to the end of the cycle
    slow_gen(node, node.cfundstats()["votingPeriod"]["ending"] - node.cfundstats()["votingPeriod"][
        "current"])

def start_new_cycle(node):
    # Move one past the end of the cycle
    end_cycle(node)
    slow_gen(node, 1)

def reverse_byte_str(hex_str):
    return ''.join([c for t in zip(hex_str[-2::-2], hex_str[::-2]) for c in t])

def create_vote_tx(node, vote_type, vote, p_hash):
    """
    Creates voting hex to be included into the coinbase.
    Args:
        node: self.nodes[0]
        vote_type: proposal:'c2', payment request: 'c3'
        vote: yes: 'c4', no:'c5'
        p_hash: hash of the proposal/payment request

    Returns:
        str: hex data to include into the coinbase
    """
    # Byte-reverse hash
    reversed_hash = reverse_byte_str(p_hash)

    # Create voting string
    vote_str = '6a' + 'c1' + vote_type + vote + '20' + reversed_hash

    # Create raw vote tx
    vote_tx = node.createrawtransaction(
        [],
        {vote_str: 0},
        "", 0
    )
    return vote_tx

