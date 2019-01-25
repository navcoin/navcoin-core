#!/usr/bin/env python3
# Tests RPC help output at runtime

from test_framework.test_framework import NavCoinTestFramework
from test_framework.util import *

import os


class HelpRpcTest(NavCoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1

    def run_test(self):
        self.test_categories()

    def test_categories(self):
        node = self.nodes[0]

        # wrong argument count
        assert_raises_rpc_error(-1, 'help ( "command" )', node.help, 'foo', 'bar')

        # invalid argument
        assert_raises_rpc_error(-1, 'JSON value is not a string as expected', node.help, 0)

        # help of unknown command
        assert_equal(node.help('foo'), 'help: unknown command: foo')

        # command titles
        titles = [line[3:-3] for line in node.help().splitlines() if line.startswith('==')]

        components = ['Addressindex', 'Blockchain', 'Communityfund', 'Control', 'Generating', 'Hacking', 'Mining', 'Network', 'Rawtransactions', 'Util']

        # titles and components will differ depending on whether wallet and/or zmq are compiled
        if 'Wallet' in titles:
            components.append('Wallet')

        if 'Zmq' in titles:
            components.append('Zmq')

        assert_equal(titles, components)


if __name__ == '__main__':
    HelpRpcTest().main()