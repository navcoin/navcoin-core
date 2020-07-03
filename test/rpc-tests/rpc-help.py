#!/usr/bin/env python3
# Tests RPC help output at runtime

from test_framework.test_framework import NavCoinTestFramework
from test_framework.util import *

import os


class HelpRpcTest(NavCoinTestFramework):
    def __init__(self):
        super().__init__()
        self.setup_clean_chain = True
        self.num_nodes = 1

    def setup_network(self, split=False):
        self.nodes = start_nodes(self.num_nodes, self.options.tmpdir)
        self.is_network_split = False

    def run_test(self):
        self.test_categories()
        self.dump_help()

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

        components = ['Addressindex', 'Blockchain', 'Blsct', 'Communityfund', 'Control', 'Dao', 'Generating', 'Hacking', 'Mining', 'Network', 'Rawtransactions', 'Util']

        # titles and components will differ depending on whether wallet and/or zmq are compiled
        if 'Wallet' in titles:
            components.append('Wallet')

        if 'Zmq' in titles:
            components.append('Zmq')

        assert_equal(titles, components)

    def dump_help(self):
        dump_dir = os.path.join(self.options.tmpdir, 'rpc_help_dump')
        os.mkdir(dump_dir)
        calls = [line.split(' ', 1)[0] for line in self.nodes[0].help().splitlines() if line and not line.startswith('==')]
        for call in calls:
            with open(os.path.join(dump_dir, call), 'w', encoding='utf-8') as f:
                # Make sure node can generate the help at runtime without crashing
                f.write(self.nodes[0].help(call))


if __name__ == '__main__':
    HelpRpcTest().main()
