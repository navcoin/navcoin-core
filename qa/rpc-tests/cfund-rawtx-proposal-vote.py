#!/usr/bin/env python3
# Copyright (c) 2018 The Navcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

from test_framework.test_framework import NavCoinTestFramework
from test_framework.util import *

import time

class CommunityFundRawTXProposalVoteTest(NavCoinTestFramework):
    """Tests the state transition of proposals of the Community fund."""

    def __init__(self):
        super().__init__()
        self.setup_clean_chain = True
        self.num_nodes = 1

    def setup_network(self, split=False):
        self.nodes = start_nodes(self.num_nodes, self.options.tmpdir)
        self.is_network_split = False

    def run_test(self):
        # Make sure cfund is active
        self.activate_cfund()
        self.nodes[0].donatefund(1000)

        # Get address
        address = self.nodes[0].getnewaddress()

        proposalid0 = self.nodes[0].createproposal(address, 1, 3600, "test proposal 0")["hash"]
        proposalid1 = self.nodes[0].createproposal(address, 1, 3600, "test proposal 1")["hash"]
        self.start_new_cycle()
        slow_gen(self.nodes[0], 1)

        # pre-flight checks
        assert (self.nodes[0].getproposal(proposalid0)['votesYes'] == 0)
        assert (self.nodes[0].getproposal(proposalid0)['votesNo'] == 0)
        assert (self.nodes[0].getproposal(proposalid1)['votesYes'] == 0)
        assert (self.nodes[0].getproposal(proposalid1)['votesNo'] == 0)

        # setup votes
        prop0_vote_tx_yes = self.create_vote_tx('c2', 'c4', proposalid0)
        prop0_vote_tx_no = self.create_vote_tx('c2', 'c5', proposalid0)
        prop1_vote_tx_yes = self.create_vote_tx('c2', 'c4', proposalid1)
        prop1_vote_tx_no = self.create_vote_tx('c2', 'c5', proposalid1)

        # perform 1 YES vote
        self.nodes[0].coinbaseoutputs([prop0_vote_tx_yes])
        slow_gen(self.nodes[0], 1)

        assert (self.nodes[0].getproposal(proposalid0)['votesYes'] == 1)
        assert (self.nodes[0].getproposal(proposalid0)['votesNo'] == 0)

        assert (self.nodes[0].getproposal(proposalid1)['votesYes'] == 0)
        assert (self.nodes[0].getproposal(proposalid1)['votesNo'] == 0)

        # perform 1 NO vote
        self.nodes[0].coinbaseoutputs([prop0_vote_tx_no])
        slow_gen(self.nodes[0], 1)

        assert (self.nodes[0].getproposal(proposalid0)['votesYes'] == 1)
        assert (self.nodes[0].getproposal(proposalid0)['votesNo'] == 1)

        assert (self.nodes[0].getproposal(proposalid1)['votesYes'] == 0)
        assert (self.nodes[0].getproposal(proposalid1)['votesNo'] == 0)

        # multiple votes
        self.nodes[0].coinbaseoutputs([prop0_vote_tx_yes, prop0_vote_tx_yes, prop0_vote_tx_yes, prop0_vote_tx_yes])
        slow_gen(self.nodes[0], 1)
        assert (self.nodes[0].getproposal(proposalid0)['votesYes'] == 2)
        assert (self.nodes[0].getproposal(proposalid0)['votesNo'] == 1)

        assert (self.nodes[0].getproposal(proposalid1)['votesYes'] == 0)
        assert (self.nodes[0].getproposal(proposalid1)['votesNo'] == 0)

        # multiple yes / no votes
        self.nodes[0].coinbaseoutputs([prop0_vote_tx_yes, prop0_vote_tx_no, prop0_vote_tx_yes, prop0_vote_tx_no])
        slow_gen(self.nodes[0], 1)
        assert (self.nodes[0].getproposal(proposalid0)['votesYes'] == 3)
        assert (self.nodes[0].getproposal(proposalid0)['votesNo'] == 1)

        assert (self.nodes[0].getproposal(proposalid1)['votesYes'] == 0)
        assert (self.nodes[0].getproposal(proposalid1)['votesNo'] == 0)

        # Insert YES votes for multiple  propsals
        self.nodes[0].coinbaseoutputs([prop0_vote_tx_yes, prop1_vote_tx_yes])
        slow_gen(self.nodes[0], 1)
        assert (self.nodes[0].getproposal(proposalid0)['votesYes'] == 4)
        assert (self.nodes[0].getproposal(proposalid0)['votesNo'] == 1)

        assert (self.nodes[0].getproposal(proposalid1)['votesYes'] == 1)
        assert (self.nodes[0].getproposal(proposalid1)['votesNo'] == 0)

        # Insert NO votes for multiple propsals
        self.nodes[0].coinbaseoutputs([prop0_vote_tx_no, prop1_vote_tx_no])
        slow_gen(self.nodes[0], 1)
        assert (self.nodes[0].getproposal(proposalid0)['votesYes'] == 4)
        assert (self.nodes[0].getproposal(proposalid0)['votesNo'] == 2)

        assert (self.nodes[0].getproposal(proposalid1)['votesYes'] == 1)
        assert (self.nodes[0].getproposal(proposalid1)['votesNo'] == 1)

        # Insert bad vote tx with double vote in string
        pr0_bad_vote_tx = self.create_vote_tx('c3', 'c4c4', proposalid0)
        self.nodes[0].coinbaseoutputs([pr0_bad_vote_tx])
        slow_gen(self.nodes[0], 1)
        assert (self.nodes[0].getproposal(proposalid0)['votesYes'] == 4)

        # vote to accept proposal0
        self.vote_to_new_cycle(prop0_vote_tx_yes)
        assert (self.nodes[0].getproposal(proposalid0)['status'] == 'accepted')
        assert (self.nodes[0].getproposal(proposalid0)['stateChangedOnBlock'])

        # vote to reject proposal 1
        self.vote_to_new_cycle(prop1_vote_tx_no)
        print(self.nodes[0].getproposal(proposalid1))
        assert (self.nodes[0].getproposal(proposalid1)['status'] == 'rejected')


    def vote_to_new_cycle(self, voteTx):
        # Move one past the end of the cycle

        blocksToEnd = self.nodes[0].cfundstats()["votingPeriod"]["ending"] - self.nodes[0].cfundstats()["votingPeriod"]["current"] + 1

        for i in range(0, blocksToEnd):
            self.nodes[0].coinbaseoutputs([voteTx])
            slow_gen(self.nodes[0], 1)


    def get_transaction_from_block(self, blochHash):

        block = self.nodes[0].getblock(blochHash)

        tx = self.nodes[0].gettransaction(block['tx'][0])

        decodedTx = self.nodes[0].decoderawtransaction(tx['hex'])

        return decodedTx

    def is_vote_hash_in_vout(self, decodedTx, voteHash):

        vout = decodedTx['vout']

        isInVout = False
        for tx in vout:
            if tx['scriptPubKey']['hex'] == voteHash:
                isInVout = True

        return isInVout

    def reverse_byte_str(self, hex_str):
        return ''.join([c for t in zip(hex_str[-2::-2], hex_str[::-2]) for c in t])

    def create_vote_tx(self, vote_type, vote, p_hash):
        """
        Creates voting hex to be included into the coinbase.
        Args:
            vote_type: proposal:'c2', payment request: 'c3'
            vote: yes: 'c4', no:'c5'
            p_hash: hash of the proposal/payment request

        Returns:
            str: hex data to include into the coinbase
        """
        # Byte-reverse hash
        reversed_hash = self.reverse_byte_str(p_hash)

        # Create voting string
        vote_str = '6a' + 'c1' + vote_type + vote + '20' + reversed_hash

        # Create raw vote tx
        vote_tx = self.nodes[0].createrawtransaction(
            [],
            {vote_str: 0},
            "", 0
        )
        return vote_tx

    def start_new_cycle(self):
        # Move to the end of the cycle
        slow_gen(self.nodes[0], self.nodes[0].cfundstats()["votingPeriod"]["ending"] - self.nodes[0].cfundstats()["votingPeriod"]["current"])

    def activate_cfund(self):
        slow_gen(self.nodes[0], 100)
        # Verify the Community Fund is started
        assert (self.nodes[0].getblockchaininfo()["bip9_softforks"]["communityfund"]["status"] == "started")

        slow_gen(self.nodes[0], 100)
        # Verify the Community Fund is locked_in
        assert (self.nodes[0].getblockchaininfo()["bip9_softforks"]["communityfund"]["status"] == "locked_in")

        slow_gen(self.nodes[0], 100)
        # Verify the Community Fund is active
        assert (self.nodes[0].getblockchaininfo()["bip9_softforks"]["communityfund"]["status"] == "active")



if __name__ == '__main__':
    CommunityFundRawTXProposalVoteTest().main()
