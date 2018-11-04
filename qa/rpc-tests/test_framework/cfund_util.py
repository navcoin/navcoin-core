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


