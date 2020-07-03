#!/usr/bin/env python3
# Copyright (c) 2019 Navcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

from test_framework.test_framework import NavCoinTestFramework
from test_framework.util import (
    start_node,
    assert_equal,
    slow_gen,
    connect_nodes
)
import os

class MnemonicTest(NavCoinTestFramework):

    def __init__(self):
        super().__init__()
        self.setup_clean_chain = True
        self.num_nodes = 1
        self.languages = ["english", "spanish", "italian", "japanese",\
                           "french", "russian", "czech", "ukrainian",\
                           "simplified chinese", "traditional chinese"]
        self.mnemonics = {}

    def setup_network(self):
        self.nodes = []
        self.nodes.append(start_node(0, self.options.tmpdir, ["-keypool=5"]))
        self.is_network_split = False

    def run_test (self):
        # Record masterkeyid in base58
        masterkeyid = self.nodes[0].dumpmasterprivkey()

        # Record default mnemonic in English
        mnemonic_eng = self.nodes[0].dumpmnemonic()

        # Record mnemonics in other languages
        for language in self.languages:
            self.mnemonics[language] = self.nodes[0].dumpmnemonic(language)

        self.nodes[0].staking(False)
        self.nodes[0].generate(1)
        self.sync_all()

        print("Restoring from mnemonic ...")
        self.check_mnemonic_works(masterkeyid, mnemonic_eng)

        for language in self.languages:
            self.check_mnemonic_works(masterkeyid, self.mnemonics[language], language)

    def check_mnemonic_works(self, masterprivkey, mnemonic, language="english"):
        self.stop_node(0)
        os.remove(self.options.tmpdir + "/node0/devnet/wallet.dat")

        self.nodes[0] = start_node(0, self.options.tmpdir, ["-importmnemonic=" + mnemonic, "-mnemoniclanguage=" + language, "-keypool=5"])
        self.nodes[0].staking(False)

        assert(len(self.nodes[0].listtransactions()) == 1);
        assert_equal(masterprivkey, self.nodes[0].dumpmasterprivkey())

if __name__ == '__main__':
    MnemonicTest().main ()
