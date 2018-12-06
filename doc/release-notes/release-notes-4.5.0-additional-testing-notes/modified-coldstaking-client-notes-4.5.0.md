# Modified client testing for cold-staking v4.5.0
*Date: 2018-12-03, 2018-12-04, 2018-12-05*  


## 1. Modified spending wallet to stake inputs

#### Aim: 
To test whether a modified wallet holding a spending key (but not staking key) can stake cold-staking inputs.

#### Wallet setup:
The subject wallet holds:
- The spending private key to a cold-staking address.

Coins are held in the cold-staking address only. There are no other coins in any other addresses held by the wallet.

#### Modifications:
- changed CreateCoinStake() in wallet.cpp (line 384) GetKey() from staking address to spending address, i.e. from 
`if (!keystore.GetKey(uint160(vSolutions[0]), key))` to ` if (!keystore.GetKey(uint160(vSolutions[1]), key))`
- changed IsMine() in ismine.cpp (line 76 to 81) all conditional branches to return `ISMINE_SPENDABLE_STAKABLE` when case is `TX_COLDSTAKING`
- changed SignStep() in sign.cpp (line 106) CKeyID() from staking address to spending address, i.e. from `keyID = CKeyID(uint160(fCoinStake ? vSolutions[0] : vSolutions[1]));` to `keyID = CKeyID(uint160(fCoinStake ? vSolutions[1] : vSolutions[1]));`
- This will still return false from ProduceSignature() and cause the staking to fail because it will fail the checks in VerifyScript(), namely OP_EQUALSVERIFY, so
		- changed EvalScript() in interpreter.cpp (line 732) when entering conditional statement `if (opcode == OP_EQUALVERIFY)`, always `popstack(stack);` instead of `return set_error(serror, SCRIPT_ERR_EQUALVERIFY);`

At the time of writing, the changes can be viewed here:  
<https://github.com/matt-auckland/navcoin-core/compare/cold-staking...marcus290:modified-spending-wallet>

#### Result:
- The modified spending wallet will stake the cold-staking address inputs and broadcast blocks found to other nodes.
- But other nodes will reject these blocks and ban the peer (i.e. our wallet) from the network.
- The modified spending wallet will continue to stake and create its own blockchain.
- Log record:
```
2018-12-05 02:07:28 CheckStake() : new proof-of-stake block found hash: 9ec735c1ea6c020fe5b55de17898b4fbd67091eefd27f5afde8797e7612ecb06
2018-12-05 02:07:28 UpdateTip: new best=9ec735c1ea6c020fe5b55de17898b4fbd67091eefd27f5afde8797e7612ecb06 height=69274 version=0x710171f1 log2_work=85.821494 tx=134912 date='2018-12-05 02:07:28' progress=1.000000 cache=0.0MiB(29tx)
2018-12-05 02:07:28 AddToWallet 597c8f7bdb79c7d906b0fd15bda4f8039fdc2e01d07e9381f1d83359650bc548  new
2018-12-05 02:07:29 connect() to [2403:4d00:301:10:d99a:d453:b9cf:6196]:15556 failed: Network is unreachable (101)
2018-12-05 02:07:29 connect() to [2403:4d00:301:10:7ee3:755a:be36:a5c0]:15556 failed: Network is unreachable (101)
2018-12-05 02:07:31 connect() to [2403:4d00:301:10:7ee3:755a:be36:a5c0]:15556 failed: Network is unreachable (101)
2018-12-05 02:07:32 connect() to 103.247.155.206:15556 failed after select(): Connection refused (111)
2018-12-05 02:07:33 connect() to [2403:4d00:301:10:7ee3:755a:be36:a5c0]:15556 failed: Network is unreachable (101)
2018-12-05 02:07:37 connect() to 103.247.155.206:15556 failed after select(): Connection refused (111)
2018-12-05 02:07:40 connect() to [2403:4d00:301:10:7ee3:755a:be36:a5c0]:15556 failed: Network is unreachable (101)
2018-12-05 02:07:50 connect() to 103.247.155.206:15556 failed after select(): Connection refused (111)
2018-12-05 02:07:52 connect() to [2403:4d00:301:10:7ee3:755a:be36:a5c0]:15556 failed: Network is unreachable (101)
2018-12-05 02:07:53 connect() to 103.247.155.206:15556 failed after select(): Connection refused (111)
2018-12-05 02:07:54 connect() to [2403:4d00:301:10:d99a:d453:b9cf:6196]:15556 failed: Network is unreachable (101)
2018-12-05 02:07:55 connect() to 103.247.155.206:15556 failed after select(): Connection refused (111)
2018-12-05 02:07:57 connect() to [2403:4d00:301:10:d99a:d453:b9cf:6196]:15556 failed: Network is unreachable (101)
2018-12-05 02:08:01 connect() to 103.247.155.206:15556 failed after select(): Connection refused (111)
2018-12-05 02:08:02 connect() to 103.247.155.206:15556 failed after select(): Connection refused (111)
2018-12-05 02:08:04 connect() to 103.247.155.206:15556 failed after select(): Connection refused (111)
2018-12-05 02:08:05 connect() to [2403:4d00:301:10:7ee3:755a:be36:a5c0]:15556 failed: Network is unreachable (101)
2018-12-05 02:08:05 connect() to [2403:4d00:301:10:7ee3:755a:be36:a5c0]:15556 failed: Network is unreachable (101)
2018-12-05 02:08:06 connect() to [2403:4d00:301:10:d99a:d453:b9cf:6196]:15556 failed: Network is unreachable (101)
2018-12-05 02:08:07 connect() to [2403:4d00:301:10:d99a:d453:b9cf:6196]:15556 failed: Network is unreachable (101)
2018-12-05 02:08:09 connect() to 103.247.155.206:15556 failed after select(): Connection refused (111)
2018-12-05 02:08:09 connect() to [2403:4d00:301:10:d99a:d453:b9cf:6196]:15556 failed: Network is unreachable (101)
2018-12-05 02:08:11 connect() to [2403:4d00:301:10:7ee3:755a:be36:a5c0]:15556 failed: Network is unreachable (101)
2018-12-05 02:08:12 socket recv error Connection reset by peer (104)
2018-12-05 02:08:14 connect() to [2403:4d00:301:10:7ee3:755a:be36:a5c0]:15556 failed: Network is unreachable (101)
2018-12-05 02:08:14 connect() to [2403:4d00:301:10:7ee3:755a:be36:a5c0]:15556 failed: Network is unreachable (101)
```

#### Conclusion:
Based on the modifications above, blocks staked by the modified wallet will be rejected by the network.





## 2. Modified staking wallet to send staking outputs to another address

#### Aim: 
To test whether a modified wallet holding a staking key (but not spending key) can send staking outputs to another address.

#### Wallet setup:
The subject wallet holds:
- The staking private key to a cold-staking address.
- The private key to another address, not related to the cold-staking address ("**Address X**").

Coins are held in the cold-staking address only. There are no other coins in any other addresses held by the wallet.

#### Modifications:
- changed CreateCoinStake() in wallet.cpp (line 390) scripting for cold-staking from `scriptPubKeyOut = scriptPubKeyKernel;` to `scriptPubKeyOut = CScript(ToByteVector([newkey].GetPubKey())); scriptPubKeyOut << OP_CHECKSIG;` where `[newkey]` is the key to Address X.
- changed CheckProofOfStake() in main.cpp (lines 8655-8659) removing all checks for when IsColdStaking() is true.

At the time of writing, the changes can be viewed here:  
<https://github.com/matt-auckland/navcoin-core/compare/cold-staking...marcus290:modified-staking-wallet-stake>

#### Result:
- The modified staking wallet will continue to stake.
- At the first successful stake by the wallet, the staking output (i.e. staking reward + cold-staking balance) will be sent to Address X, and the stake will be broadcast to other nodes. 
- But other nodes will reject these stakes.
- The modified staking wallet will re-sync with the network and all its stakes will be orphaned.
- Log record:
```
2018-12-04 03:44:00 CheckStake() : new proof-of-stake block found hash: ba0484285df2c38def7581a989ebb00ef784bc7652526d9a92bc39fc8c91d764
2018-12-04 03:44:00 UpdateTip: new best=ba0484285df2c38def7581a989ebb00ef784bc7652526d9a92bc39fc8c91d764 height=66629 version=0x7101f1f1 log2_work=85.821494 tx=129619 date='2018-12-04 03:44:00' progress=1.000000 cache=0.0MiB(136tx)
2018-12-04 03:44:00 AddToWallet 88de5054da44fc8ad4f3e9f3cd1d9aa5b84c018df52d6f788869e89fd9892b66  new
2018-12-04 03:44:49 UpdateTip: new best=d647c2fbb9f0be80ba064a7c68a30f7a3ada9d68e95c57ed2506e0def4a5d7fb height=66628 version=0x710171f1 log2_work=85.821494 tx=129617 date='2018-12-04 03:43:44' progress=0.999959 cache=0.0MiB(136tx)
2018-12-04 03:44:49 SyncTransaction : Refunding inputs of orphan tx 88de5054da44fc8ad4f3e9f3cd1d9aa5b84c018df52d6f788869e89fd9892b66
2018-12-04 03:44:49 AddToWallet 88de5054da44fc8ad4f3e9f3cd1d9aa5b84c018df52d6f788869e89fd9892b66  
2018-12-04 03:44:49 SyncTransaction : Removing tx 88de5054da44fc8ad4f3e9f3cd1d9aa5b84c018df52d6f788869e89fd9892b66 from mapTxSpends
2018-12-04 03:44:49 UpdateTip: new best=2c6fbcf8d9fe08e7b34d8aeffd552c321d3d807e135fdea5d3470e93578ed8c8 height=66627 version=0x710171f1 log2_work=85.821494 tx=129615 date='2018-12-04 03:43:12' progress=0.999939 cache=0.0MiB(135tx)
2018-12-04 03:44:49 UpdateTip: new best=7fb237026243f2736cdd40435130d576a6b3104406c9be0e79a648bc7c20c018 height=66628 version=0x710171f1 log2_work=85.821494 tx=129617 date='2018-12-04 03:43:44' progress=0.999959 cache=0.0MiB(137tx)
2018-12-04 03:44:50 UpdateTip: new best=310d44343a977ae806b57130d91ab884c54b4af720578abc4c75770f29c87473 height=66629 version=0x710171f1 log2_work=85.821494 tx=129619 date='2018-12-04 03:44:00' progress=0.999969 cache=0.0MiB(139tx)
2018-12-04 03:44:50 UpdateTip: new best=dab8fa0bf6b0f8e2a1e9d01f75912413af99788bdd43f7f32088c7410ff3951c height=66630 version=0x710171f1 log2_work=85.821494 tx=129621 date='2018-12-04 03:44:48' progress=0.999999 cache=0.0MiB(142tx)
2018-12-04 03:45:04 CheckStake() : new proof-of-stake block found hash: 0b26c39e07b56ab8cea0e74a6d2cdbda1f15bc6a98054b93187e3ec1bef1feed
2018-12-04 03:45:04 UpdateTip: new best=0b26c39e07b56ab8cea0e74a6d2cdbda1f15bc6a98054b93187e3ec1bef1feed height=66631 version=0x7101f1f1 log2_work=85.821494 tx=129623 date='2018-12-04 03:45:04' progress=1.000000 cache=0.0MiB(143tx)
2018-12-04 03:45:04 AddToWallet 11cc895db90b260933718e05e3d2ef4783a70208f24f5a326abf03803ce36b33  new
2018-12-04 03:45:36 CheckStake() : new proof-of-stake block found hash: ca29e549b6afecd87b39847ace6e9b73cc8ceed7049e6b5e013f41a18f94bb6a
2018-12-04 03:45:36 UpdateTip: new best=ca29e549b6afecd87b39847ace6e9b73cc8ceed7049e6b5e013f41a18f94bb6a height=66632 version=0x7101f1f1 log2_work=85.821494 tx=129625 date='2018-12-04 03:45:36' progress=1.000000 cache=0.0MiB(145tx)
2018-12-04 03:45:36 AddToWallet 0c9503ec50c3cdf297ee7818cd6e9d20d1187e3289725573b3decef0319791f9  new
2018-12-04 03:45:53 UpdateTip: new best=0b26c39e07b56ab8cea0e74a6d2cdbda1f15bc6a98054b93187e3ec1bef1feed height=66631 version=0x7101f1f1 log2_work=85.821494 tx=129623 date='2018-12-04 03:45:04' progress=0.999969 cache=0.0MiB(144tx)
2018-12-04 03:45:53 SyncTransaction : Refunding inputs of orphan tx 0c9503ec50c3cdf297ee7818cd6e9d20d1187e3289725573b3decef0319791f9
2018-12-04 03:45:53 AddToWallet 0c9503ec50c3cdf297ee7818cd6e9d20d1187e3289725573b3decef0319791f9  
2018-12-04 03:45:53 SyncTransaction : Removing tx 0c9503ec50c3cdf297ee7818cd6e9d20d1187e3289725573b3decef0319791f9 from mapTxSpends
2018-12-04 03:45:53 UpdateTip: new best=dab8fa0bf6b0f8e2a1e9d01f75912413af99788bdd43f7f32088c7410ff3951c height=66630 version=0x710171f1 log2_work=85.821494 tx=129621 date='2018-12-04 03:44:48' progress=0.999959 cache=0.0MiB(144tx)
2018-12-04 03:45:53 SyncTransaction : Refunding inputs of orphan tx 11cc895db90b260933718e05e3d2ef4783a70208f24f5a326abf03803ce36b33
2018-12-04 03:45:53 AddToWallet 11cc895db90b260933718e05e3d2ef4783a70208f24f5a326abf03803ce36b33  
2018-12-04 03:45:53 SyncTransaction : Removing tx 11cc895db90b260933718e05e3d2ef4783a70208f24f5a326abf03803ce36b33 from mapTxSpends
2018-12-04 03:45:53 UpdateTip: new best=0faf8ff5b51df87d382de126b17b1871d03d7b52e43dc26363f6d35654faab95 height=66631 version=0x710171f1 log2_work=85.821494 tx=129623 date='2018-12-04 03:45:04' progress=0.999969 cache=0.0MiB(146tx)
2018-12-04 03:45:54 UpdateTip: new best=f62850ba69ea4b5d9ca2111a71190bdaf9c673d0a1af2c8ae03cc1a8af5a298c height=66632 version=0x710171f1 log2_work=85.821494 tx=129625 date='2018-12-04 03:45:36' progress=0.999989 cache=0.0MiB(148tx)
```

#### Conclusion:
Based on the modifications above, blocks staked by the modified wallet will be rejected by the network.


## 3. Modified staking wallet to spend inputs

#### Aim: 
To test whether a modified wallet with a staking key can spend cold-staking inputs.

#### Wallet setup:
The subject wallet holds:
- The staking private key to a cold-staking address.

Coins are held in the cold-staking address only. There are no other coins in any other addresses held by the wallet.

#### Modifications:
- changed SelectCoinsMinConf() and SelectCoins() in wallet.cpp (lines 2518-2519, 2612-2613) removed `if (!output.fSpendable) continue;`
- changed IsMine() in ismine.cpp (lines 63, 76-81) all conditional branches to return `ISMINE_SPENDABLE_STAKABLE` when case is `TX_COLDSTAKING` and `TX_PUBKEY`
- changed SignStep() in sign.cpp (line 106) CKeyID() from spending address to staking address, i.e. from `keyID = CKeyID(uint160(fCoinStake ? vSolutions[0] : vSolutions[1]));` to `keyID = CKeyID(uint160(fCoinStake ? vSolutions[0] : vSolutions[0]));`
- This will still return false from ProduceSignature() and cause the staking to fail because it will fail the checks in VerifyScript(), namely OP_EQUALSVERIFY, so
    - changed EvalScript() in interpreter.cpp (line 732) when entering conditional statement `if (opcode == OP_EQUALVERIFY)`, always `popstack(stack);` instead of `return set_error(serror, SCRIPT_ERR_EQUALVERIFY);`

At the time of writing, the changes can be viewed here:  
<https://github.com/matt-auckland/navcoin-core/compare/cold-staking...marcus290:modified-staking-wallet-spend>

#### Result:
- The modified staking wallet can spend the cold-staking address inputs and broadcast these transactions to other nodes.
- But other nodes will reject these transactions and ban the peer (i.e. our wallet) from the network.
- The modified staking wallet will continue to stake and create its own blockchain.
- Log record:
```
2018-12-04 04:13:40 connect() to 103.247.155.206:15556 failed after select(): Connection refused (111)
2018-12-04 04:13:52 UpdateTip: new best=0012bb917db9e27c7705e31bb428f4f9f4620814d3ca2f5f7b0ec217ee4aee9e height=66689 version=0x710171f1 log2_work=85.821494 tx=129739 date='2018-12-04 04:13:52' progress=1.000000 cache=0.0MiB(19tx)
2018-12-04 04:14:01 keypool reserve 4
2018-12-04 04:14:01 CommitTransaction:
CTransaction(hash=0e86a8f71b8a3b580935cb790c1fce039d9243b867f29564a76ee316f3c8f593, nTime=1543896841, ver=3, vin.size=1, vout.size=2, nLockTime=66689), strDZeel=fQgWJaM0usxdF8p1zediWS6yiqCYiT)
    CTxIn(COutPoint(b71b2409fb, 1), scriptSig=47304402202a9db7180af8a9, nSequence=4294967294)
    CTxOut(nValue=48011.64988460, scriptPubKey=OP_DUP OP_HASH160 de88eb286c4e9cab6e772f488b52ddc596ba55d7 OP_EQUALVERIFY OP_CHECKSIG)
    CTxOut(nValue=100000.00000000, scriptPubKey=OP_DUP OP_HASH160 9194ae60493f6492a4662133eb3b05894830c3f7 OP_EQUALVERIFY OP_CHECKSIG)
2018-12-04 04:14:01 keypool keep 4
2018-12-04 04:14:01 AddToWallet 0e86a8f71b8a3b580935cb790c1fce039d9243b867f29564a76ee316f3c8f593  new
2018-12-04 04:14:02 AddToWallet 0e86a8f71b8a3b580935cb790c1fce039d9243b867f29564a76ee316f3c8f593  
2018-12-04 04:14:02 Relaying wtx 0e86a8f71b8a3b580935cb790c1fce039d9243b867f29564a76ee316f3c8f593
2018-12-04 04:14:04 socket recv error Connection reset by peer (104)
2018-12-04 04:14:09 UpdateTip: new best=6157ee14743db35d1de569c1a39cc4e1b1137f3b87c9fb75dc74fa7d72133c06 height=66690 version=0x710171f1 log2_work=85.821494 tx=129741 date='2018-12-04 04:14:08' progress=0.999999 cache=0.0MiB(22tx)
2018-12-04 04:18:25 receive version message: /NavCoin:4.4.0/: version 70020, blocks=66690, us=[2403:4d00:301:10:7ee3:755a:be36:a5c0]:15556, peer=5
2018-12-04 04:18:25 AdvertiseLocal: advertising address [2403:4d00:301:10:7ee3:755a:be36:a5c0]:15556
2018-12-04 04:18:26 CheckStake() : new proof-of-stake block found hash: 7679f66ced702c32db7f20227f56fae863a4d093758458daf3643a84976962ee
2018-12-04 04:18:26 UpdateTip: new best=7679f66ced702c32db7f20227f56fae863a4d093758458daf3643a84976962ee height=66691 version=0x7101f1f1 log2_work=85.821494 tx=129744 date='2018-12-04 04:18:24' progress=0.999999 cache=0.0MiB(25tx)
2018-12-04 04:18:26 AddToWallet 641c0f39d60eb92661b65b413db56122d32562576c7ad77a1a04745eab52ac78  new
2018-12-04 04:18:26 AddToWallet 0e86a8f71b8a3b580935cb790c1fce039d9243b867f29564a76ee316f3c8f593  update
2018-12-04 04:23:22 socket recv error Connection reset by peer (104)
2018-12-04 04:23:40 connect() to 103.247.155.206:15556 failed after select(): Connection refused (111)
2018-12-04 04:24:04 socket recv error Connection reset by peer (104)
```

#### Conclusion:
Based on the modifications above, coins spent by the modified wallet will be rejected by the network.


