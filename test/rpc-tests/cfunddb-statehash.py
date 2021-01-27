#!/usr/bin/env python3
# Copyright (c) 2019 The Navcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.


from test_framework.test_framework import NavcoinTestFramework
from test_framework.util import *
from test_framework.cfund_util import *
import urllib.parse

class CFundDBStateHash(NavcoinTestFramework):
    def __init__(self):
        super().__init__()
        self.num_nodes = 2
        self.setup_clean_chain = True

    def setup_network(self):
        self.nodes = []
        self.nodes.append(start_node(0, self.options.tmpdir, ["-headerspamfilter=0", "-debug=dao"]))
        self.nodes.append(start_node(1, self.options.tmpdir, ["-headerspamfilter=0", "-debug=dao"]))
        connect_nodes(self.nodes[0], 1)
        self.is_network_split = False

    def run_test(self):
        self.nodes[1].generate(300)
        self.nodes[1].sendtoaddress(self.nodes[0].getnewaddress(),100000)
        hash=""

        for i in range(20):
            phash=self.nodes[1].createproposal(self.nodes[1].getnewaddress(), 100, 1000, "test")["hash"]
            if (hash == ""):
                hash = phash
            self.nodes[1].generate(1)
            self.sync_all()
            self.nodes[1].proposalvote(phash, 'yes')

        self.nodes[1].invalidateblock(self.nodes[1].getblockhash(311))
        self.nodes[1].donatefund(100000)
        self.nodes[1].generate(30)
        self.sync_all()

        assert_equal(self.nodes[0].getcfunddbstatehash(), self.nodes[1].getcfunddbstatehash())

        self.nodes[1].generate(60)
        self.sync_all()

        assert_equal(self.nodes[1].getinfo()['errors'],"")

        raw_preq = self.nodes[1].createpaymentrequest(hash, 100, "preq", 1, True)["raw"]

        # Disconnect Nodes 0 and 1
        url = urllib.parse.urlparse(self.nodes[1].url)
        self.nodes[0].disconnectnode(url.hostname+":"+str(p2p_port(1)))

        self.nodes[0].sendrawtransaction(raw_preq)
        self.nodes[1].sendrawtransaction(raw_preq)

        self.nodes[0].generate(1)
        self.nodes[1].generate(60)

        assert(self.nodes[0].getbestblockhash() != self.nodes[1].getbestblockhash())
        assert(self.nodes[0].getcfunddbstatehash() != self.nodes[1].getcfunddbstatehash())

        connect_nodes(self.nodes[0], 1)

        self.sync_all()

        assert_equal(self.nodes[0].getbestblockhash(), self.nodes[1].getbestblockhash())
        assert_equal(self.nodes[0].getcfunddbstatehash(), self.nodes[1].getcfunddbstatehash())

        self.nodes[0].disconnectnode(url.hostname+":"+str(p2p_port(1)))

        self.nodes[0].createconsultation("test",1)

        self.nodes[0].generate(60)

        assert(self.nodes[0].getbestblockhash() != self.nodes[1].getbestblockhash())
        assert(self.nodes[0].getcfunddbstatehash() != self.nodes[1].getcfunddbstatehash())


        connect_nodes(self.nodes[0], 1)

        self.sync_all()

        assert_equal(self.nodes[0].getbestblockhash(), self.nodes[1].getbestblockhash())
        assert_equal(self.nodes[0].getcfunddbstatehash(), self.nodes[1].getcfunddbstatehash())

        self.nodes[0].disconnectnode(url.hostname+":"+str(p2p_port(1)))

        self.nodes[0].createconsultationwithanswers("test",["one","two"])

        self.nodes[0].generate(60)

        assert(self.nodes[0].getbestblockhash() != self.nodes[1].getbestblockhash())
        assert(self.nodes[0].getcfunddbstatehash() != self.nodes[1].getcfunddbstatehash())


        connect_nodes(self.nodes[0], 1)

        self.sync_all()

        assert_equal(self.nodes[0].getbestblockhash(), self.nodes[1].getbestblockhash())
        assert_equal(self.nodes[0].getcfunddbstatehash(), self.nodes[1].getcfunddbstatehash())

        self.nodes[0].disconnectnode(url.hostname+":"+str(p2p_port(1)))

        proposal = self.nodes[0].proposeconsensuschange(21, 300000000)['hash']

        slow_gen(self.nodes[0] , 1)

        first_answer = self.nodes[0].getconsultation(proposal)['answers'][0]['hash']

        self.nodes[0].support(first_answer)

        end_cycle(self.nodes[0])
        slow_gen(self.nodes[0] , 1)

        #cycle 1

        end_cycle(self.nodes[0])
        slow_gen(self.nodes[0] , 1)

        #cycle 2

        end_cycle(self.nodes[0])
        slow_gen(self.nodes[0] , 1)

        #cycle 3

        end_cycle(self.nodes[0])
        slow_gen(self.nodes[0] , 1)

        #cycle 4

        self.nodes[0].consultationvote(first_answer, "yes")

        end_cycle(self.nodes[0])
        slow_gen(self.nodes[0], 1)

        #cycle 4

        assert(self.nodes[0].getconsultation(proposal)["status"] == "passed")
        assert(self.nodes[0].getconsensusparameters() != self.nodes[1].getconsensusparameters())
        assert(self.nodes[0].getbestblockhash() != self.nodes[1].getbestblockhash())
        assert(self.nodes[0].getcfunddbstatehash() != self.nodes[1].getcfunddbstatehash())

        slow_gen(self.nodes[0], 10)

        connect_nodes(self.nodes[0], 1)

        self.sync_all()

        assert_equal(self.nodes[0].getbestblockhash(), self.nodes[1].getbestblockhash())
        assert_equal(self.nodes[0].getcfunddbstatehash(), self.nodes[1].getcfunddbstatehash())


if __name__ == '__main__':
    CFundDBStateHash().main()
