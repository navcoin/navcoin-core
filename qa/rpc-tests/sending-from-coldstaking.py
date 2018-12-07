#!/usr/bin/env python3
# copyright (c) 2018 the navcoin core developers
# distributed under the mit software license, see the accompanying
# file copying or http://www.opensource.org/licenses/mit-license.php.
import decimal
from test_framework.test_framework import NavCoinTestFramework
from test_framework.util import *
#/TODO leave comments for how much is sent in each func call
class SendingFromColdStaking(NavCoinTestFramework):
    """Tests spending and staking to/from a spending wallet."""

    # set up num of nodes
    def __init__(self):
        super().__init__()
        self.setup_clean_chain = True
        self.num_nodes = 2
    # set up nodes
    def setup_network(self, split=False):
        self.nodes = self.setup_nodes()
        self.is_network_split = split

    def run_test(self):
        self.nodes[0].staking(False)

        """generate first 300 blocks to lock in softfork, verify coldstaking is active"""

        slow_gen(self.nodes[0], 100)     
        # verify that cold staking has started
        assert(self.nodes[0].getblockchaininfo()["bip9_softforks"]["coldstaking"]["status"] == "started")
        slow_gen(self.nodes[0], 100)
        # verify that cold staking is locked_in
        assert(self.nodes[0].getblockchaininfo()["bip9_softforks"]["coldstaking"]["status"] == "locked_in")
        slow_gen(self.nodes[0], 100)
        # verify that cold staking is active
        assert(self.nodes[0].getblockchaininfo()["bip9_softforks"]["coldstaking"]["status"] == "active")

        """set up transaction-related constants and addresses"""

        # declare transaction-related constants
        SENDING_FEE= 0.00010000
        MIN_COLDSTAKING_SENDING_FEE = 0.00288400
        BLOCK_REWARD = 50
        # generate address owned by the wallet
        spending_address_public_key = self.nodes[0].getnewaddress()
        spending_address_private_key = self.nodes[0].dumpprivkey(spending_address_public_key)
        # declare third party addresses and keys
        staking_address_public_key = "mqyGZvLYfEH27Zk3z6JkwJgB1zpjaEHfiW"
        staking_address_private_key = "cMuNajSALbixZvApkcYVE4KgJoeQY92umhEVdQwqX9wSJUzkmdvF"
        address_Y_public_key = "mrfjgazyerYxDQHJAPDdUcC3jpmi8WZ2uv"
        address_Y_private_key = "cST2mj1kXtiRyk8VSXU3F9pnTp7GrGpyqHRv4Gtap8jo4LGUMvqo"
        # generate coldstaking address which we hold the spending key to
        coldstaking_address_spending = self.nodes[0].getcoldstakingaddress(staking_address_public_key, spending_address_public_key)

        """check wallet balance and staking weight"""

        # get wallet balance and staking weight before sending some navcoin
        balance_before_send = self.nodes[0].getbalance()
        staking_weight_before_send = self.nodes[0].getstakinginfo()["weight"]
        # check wallet staking weight roughly equals wallet balance
        assert(round(staking_weight_before_send / 100000000.0, -5) == round(balance_before_send, -5))

        """send navcoin to our coldstaking address, grab balance & staking weight"""

        # send funds to the cold staking address (leave some nav for fees) -- we specifically require
        # a transaction fee of minimum 0.002884 navcoin due to the complexity of this transaction
        self.nodes[0].sendtoaddress(coldstaking_address_spending, float(self.nodes[0].getbalance()) - MIN_COLDSTAKING_SENDING_FEE)
        # put transaction in new block & update blockchain
        slow_gen(self.nodes[0], 1)
        # create list for all coldstaking utxo recieved
        listunspent_txs = [n for n in self.nodes[0].listunspent() if n["address"] == coldstaking_address_spending]
        # asserts we have recieved funds to the coldstaking address
        assert(len(listunspent_txs) > 0)
        # asserts that the number of utxo recieved is only 1:
        assert(len(listunspent_txs) == 1)
        # asserts if amount recieved is what it should be; ~59812449.99711600 NAV
        assert(listunspent_txs[0]["amount"] == Decimal('59812449.99711600'))
        # grabs updated wallet balance and staking weight
        balance_post_send_one = self.nodes[0].getbalance()
        staking_weight_post_send = self.nodes[0].getstakinginfo()["weight"]

        """check balance decreased by just the fees"""
        
        # difference between balance after sending and previous balance is the same when block reward is removed
        # values are converted to string and "00" is added to right of == operand because values must have equal num of 
        # decimals
        assert(str(balance_post_send_one - BLOCK_REWARD) == (str(float(balance_before_send) - MIN_COLDSTAKING_SENDING_FEE) + "00"))
        
        """check staking weight now == 0 (we don't hold the staking key)"""
        
        # sent ~all funds to coldstaking address where we do not own the staking key hence our 
        # staking weight will be 0 as our recieved BLOCK_REWARD navcoin isn't mature enough to count towards
        # our staking weight
        assert(staking_weight_post_send / 100000000.0 == 0)

        """test spending from a cold staking address with the spending key"""

        # send half of our balance to a third party address with sendtoaddress(), change sent to a newly generated change address
        # hence coldstakingaddress should be empty after
        # amount to be sent has to have 8 decimals
        to_be_sent = round(float(balance_post_send_one) * float(0.5) - SENDING_FEE, 8)
        self.nodes[0].sendtoaddress(address_Y_public_key, (to_be_sent))
        # put transaction in new block & update blockchain
        slow_gen(self.nodes[0], 1)  
        # wallet balance after sending
        balance_post_send_two = self.nodes[0].getbalance()
        #check balance will not be less than ~half our balance before sending - this 
        # will occurs if we send to an address we do not own
        assert(balance_post_send_two - BLOCK_REWARD >= (float(balance_post_send_one) * float(0.5) - SENDING_FEE))

        """send funds back to coldstaking address, send raw tx from coldstaking address"""

        # resend to coldstaking, then re state listunspent_txs
        self.nodes[0].sendtoaddress(coldstaking_address_spending, round(float(balance_post_send_two) - SENDING_FEE, 8))
        slow_gen(self.nodes[0], 1)
        listunspent_txs = [n for n in self.nodes[0].listunspent() if n["address"] == coldstaking_address_spending]
        # send funds to a third party address using a signed raw transaction    
        # get unspent tx inputs
        self.send_raw_transaction(decoded_raw_transaction = listunspent_txs[0], \
        to_address = address_Y_public_key, \
        change_address = coldstaking_address_spending, \
        amount = float(str(float(listunspent_txs[0]["amount"]) - MIN_COLDSTAKING_SENDING_FEE) + "00")\
        )
        # put transaction in new block & update blockchain
        slow_gen(self.nodes[0], 1)
        # get new balance  
        balance_post_send_three = self.nodes[0].getbalance()
        # we expect our balance to be zero
        assert(balance_post_send_three - (BLOCK_REWARD * 2) == 0)
        # put transaction in new block & update blockchain
        slow_gen(self.nodes[0], 2)
        # send our entire wallet balance - minimum fee required to coldstaking address
        self.nodes[0].sendtoaddress(coldstaking_address_spending, float(str(float(self.nodes[0].getbalance()) - MIN_COLDSTAKING_SENDING_FEE) + "00"))
        # put transaction in new block & update blockchain
        slow_gen(self.nodes[0], 1)
        # send to our spending address (should work)
        send_worked = False
        current_balance = self.nodes[0].getbalance()
        try:
            #fails here - unsupported operand type(s) for *: 'decimal.decimal' and 'float'
            self.nodes[0].sendtoaddress(spending_address_public_key, float(current_balance) * 0.5 - 1)
            slow_gen(self.nodes[0], 1)
            # our balance should be the same minus fees, as we own the address we sent to
            assert(self.nodes[0].getbalance() >= current_balance - 1 + BLOCK_REWARD) 
            send_worked = True
        except Exception as e:
            print(e)

        assert(send_worked == True)
        
        slow_gen(self.nodes[0], 1)
        # self.sync_all()


        # send to our staking address
        send_worked = False
        current_balance = self.nodes[0].getbalance()

        try:
            self.nodes[0].sendtoaddress(staking_address_public_key, float(self.nodes[0].getbalance()) * 0.5 - 1)
            slow_gen(self.nodes[0], 1)
            # our balance should be half minus fees, as we dont own the address we sent to
            assert(self.nodes[0].getbalance() - BLOCK_REWARD <= float(current_balance) * 0.5 - 1 + 2) 
            send_worked = True
        except Exception as e:
            print(e)
            
        assert(send_worked == True)
        

    def send_raw_transaction(self, decoded_raw_transaction, to_address, change_address, amount):
        # create a raw tx
        inputs = [{ "txid" : decoded_raw_transaction["txid"], "vout" : decoded_raw_transaction["vout"]}]
        outputs = {to_address : amount}
        rawtx = self.nodes[0].createrawtransaction(inputs, outputs)
        # sign raw transaction
        signresult = self.nodes[0].signrawtransaction(rawtx)
        assert(signresult["complete"])
        # send raw transaction
        return self.nodes[0].sendrawtransaction(signresult['hex'])
        

if __name__ == '__main__':
    SendingFromColdStaking().main()


