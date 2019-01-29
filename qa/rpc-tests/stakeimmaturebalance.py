#!/usr/bin/python3

from test_framework.test_framework import NavCoinTestFramework
from test_framework.util import *
import logging

'''
Checks that there is no interaction between immature balance and staking weight
node0 has both confirmed and immature balance, it sends away its confirmed balance to node1 so that only immature balance remains
node0 checks that immature balance does not affect stake weight
'''

SENDING_FEE= 0.003
BLOCK_REWARD = 50

logging.basicConfig(format='%(levelname)s: %(message)s', level=logging.INFO, stream=sys.stdout)
class StakeImmatureBalance(NavCoinTestFramework):

    def __init__(self):
        super().__init__()
        self.num_nodes = 2

    def setup_network(self, split=False):
        self.nodes = start_nodes(self.num_nodes, self.options.tmpdir)
        connect_nodes_bi(self.nodes,0,1)
        self.is_network_split=False
        self.sync_all()

    def run_test(self):
        addr = self.nodes[1].getnewaddress()
        slow_gen(self.nodes[0], 300) #300 for soft fork voting
        self.nodes[0].sendtoaddress(addr, satoshi_round(float(self.nodes[0].getbalance()) - SENDING_FEE))
        slow_gen(self.nodes[0], 1)
        logging.info('Checking stake weight')
        assert(self.nodes[0].getbalance() - BLOCK_REWARD < 1), 'Wallet balance not sent'
        assert(self.nodes[0].getwalletinfo()['immature_balance'] > 0), 'No immature balance'
        assert(self.nodes[0].getstakinginfo()['weight'] == 0), 'Immature balance affecting staking weight'

if __name__ == '__main__':
    StakeImmatureBalance().main()

