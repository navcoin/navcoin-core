#!/usr/bin/env python3
# Copyright (c) 2019 The Navcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

from test_framework.test_framework import NavCoinTestFramework
from test_framework.util import *

class MinInputValueTest(NavCoinTestFramework):
    def __init__(self):
        super().__init__()
        self.setup_clean_chain = False
        self.num_nodes = 3

    def setup_network(self, split=False):
        self.nodes = []
        base_optio = []
        node_textname = [ "notset", "1800.0", "1500.0", "  51.0"]
        node_option = [ [], ["-mininputvalue=180000000000"], ["-mininputvalue=150000000000"], ["-mininputvalue=5100000000"]]

        for i in range(self.num_nodes):
            self.nodes.append(start_node(i, self.options.tmpdir, base_optio + node_option[i]))
            self.nodes[i].textname = node_textname[i]

    def show_status(self, Bfrom, Bto=0):
        print()
        for node in self.nodes:
            node.LastUtxos = node.listunspent(-Bto, -Bfrom)

        for b in range(Bfrom, Bto):
            nodestat = "BLK %-3d:  " % b
            for node in self.nodes:
                blknodeUTX = []
                blknodeUTX = [n for n in node.LastUtxos if n["confirmations"] == -b]
                if blknodeUTX:
                    nodestat = nodestat + "%10.1f  %-6s  " % (blknodeUTX[0]["amount"], blknodeUTX[0].get("account", "")[:6])
                else:
                    nodestat = nodestat + "%10s  %-6s  " % ("nd","nd")

            print(nodestat)

        nodestat = "blockcount:      "
        nameline = "MinInputValue:   "
        for node in self.nodes:
            node.LastUtxos = ""

            nodestat = nodestat + " %4d " % node.getblockcount()
            nodestat = nodestat + " "*14

            nameline = nameline + " %6s " % node.textname
            nameline = nameline + " "*12
        print(nodestat)
        print(nameline)


    def run_test(self):

        SmallValue = 1001
        MediumValue= 1500
        LargeValue = 1980
        LargeItem  = 20

        BurnAddress = "mfWxJ45yp2SFn7UciZyNpvDKrzbhyfKrY8"

        distr = [
            ["small1", SmallValue], ["small2", SmallValue],
            ["small3", SmallValue], ["small4", SmallValue],
            ["small5", SmallValue],
            ["med__1", MediumValue], ["med__2", MediumValue],
            ["large",  LargeValue]
            ]

        distr_list = [n[0] for n in distr]

        address_list = distr_list + ["main", "ForSingle"]
        for node in self.nodes:
            node.addr = {}
            for n in address_list:
                node.addr[n] = node.getnewaddress(n)

            node.addr["dontstake"] = node.addmultisigaddress(1,[node.addr["ForSingle"]], "dontstake")

            node.generate(7)

            node.distr_out={node.addr[n[0]] : n[1] for n in distr}
            node.dontspent = [n for n in node.distr_out.keys()]

        # distribute value
        for i in range(LargeItem):
            for node in self.nodes:
                node.sendmany("", node.distr_out, 0)
                node.generatetoaddress(1, BurnAddress)

                # lock distribution address
                exlist  = node.listunspent(0, 600, node.dontspent)
                locklist = [{"txid" : n["txid"], "vout" : n["vout"]} for n in exlist]
                node.lockunspent(False, locklist)


        for node in self.nodes:
            # send all the remaining to a non staking address
            unlockedValue = sum([n["amount"] for n in node.listunspent(0)])
            val_out =satoshi_round(float(unlockedValue) - 0.004)
            # print(val_out)
            node.sendtoaddress(node.addr["dontstake"], val_out)
            node.generatetoaddress(1, BurnAddress)

        # let mature the addreess filled
        for node in self.nodes:
            node.generatetoaddress(6, BurnAddress)
            node.lockunspent(True)

        time.sleep(5)

        # self.show_status(-10, -6)

        for node in self.nodes:
            node.staking(True)
            node.BlkLast = node.getblockcount()
            node.BlkEnd  = node.BlkLast + 6
            breaktime = time.time() +68

        assert(max([node.BlkLast for node in self.nodes]) < min([node.BlkEnd for node in self.nodes]))

        # staking wait loop
        while True:
            time.sleep(4)

            fBlockOver = True
            for node in self.nodes:
                if node.BlkLast != node.getblockcount():
                    node.BlkLast = node.getblockcount()
                    breaktime = time.time() +40
                    if node.BlkLast > node.BlkEnd:
                        node.staking(False)

                fBlockOver = fBlockOver and (node.BlkLast > node.BlkEnd)

            if (breaktime < time.time()) or fBlockOver:
                break

        # make all stakes mature without alter the balance
        for node in self.nodes:
            node.generatetoaddress(6, BurnAddress)

        if 1:
           self.show_status(-14, -5)

        # get the lowest amout staked by each node
        for node in self.nodes:
            UTXlist  = node.listunspent(5, 14)
            if UTXlist:
                node.SmallestStake = min([n["amount"] for n in UTXlist])
            else:
                node.SmallestStake = 0

        assert(fBlockOver)
        assert(self.nodes[0].SmallestStake > 0)
        if self.num_nodes >1:
            assert(self.nodes[1].SmallestStake > LargeValue)

        if self.num_nodes >2:
            assert(self.nodes[2].SmallestStake > MediumValue)

        if self.num_nodes >3:
            assert(self.nodes[3].SmallestStake > SmallValue)


if __name__ == '__main__':
    MinInputValueTest().main()
