# NavCoin v4.7.2 Release Notes

## Fix for Verify Chain

<[Pull Request 634](https://github.com/navcoin/navcoin-core/pull/634)>
<[Commit 049978e](https://github.com/navcoin/navcoin-core/commit/049978e246de960370f629fa38a6509fad0cf5c8)>

This patch fixes [Issue 630](https://github.com/navcoin/navcoin-core/issues/630) and introduces several important changes.

- New RPC command `getcfunddbstatehash` covered by functional test `cfunddb-statehash.py`
- New state for payment requests `6` when those are paid.
- The payment request parameter paidOnBlock is substituted by `stateChangedOnBlock` when state is `6`
- New structure for storing the state history of CFundDB entries. Those are stored in a map associating blockhash and state, allowing to directly revert state transitions when reorganizations are seen.
- `verifychain` now checks for the consistency of the CFundDB state hash when level 4 is specified.

### IMPORTANT

This set of changes will require older clients to reindex on launch, keeping the node offline for some hours at best. In order to reduce downtime, node operators can proceed as follows if needed:

- Close node with old version.
- `mkdir /tmp/reindexdata; cp -rf <data_folder>/blocks /tmp/reindexdata/; cp -rf <data_folder>/chainstate /tmp/reindexdata/`
- Reopen node with old version. It will be again online
- Launch in parallel a second instance of the node, this time using the new version with the parameters `-reindex -datadir=/tmp/reindexdata/`
- Once the reindex finishes, close both nodes and copy back the reindexed data.
- `rm -rf <data_folder>/blocks <data_folder>/chainstate; cp -rf /tmp/reindexdata/* <data_folder>`
- Relaunch new version of the node.

## CFundDB extra log and ensure read before modify

<[Pull Request 622](https://github.com/navcoin/navcoin-core/pull/622)>
<[Commit 37fa72e](https://github.com/navcoin/navcoin-core/commit/37fa72e386ad3daff58b92ed7dda1a9b0676a43b)>

This PR adds extra log for all the modifications of the CFundDB and ensures entries are read in the memory cache before being modified.

## Restart testnet

<[Pull Request 628](https://github.com/navcoin/navcoin-core/pull/628)>
<[Commit b8ed018](https://github.com/navcoin/navcoin-core/commit/b8ed0180a2deaf616c8e6b38aec42385f0a73879)>

This pull request starts a new NavCoin testnet. If you're running a testnet node you will need to will need to wipe your testnet data directory and connect to the new testnet nodes. A list of some of the testnet nodes operated by NavCoin Core developers can be found on [Issue 626](https://github.com/navcoin/navcoin-core/issues/626). If you need testnet coins or want to be added to the list of nodes, please comment on the issue or join the #dev-testnet channel in [Discord](https://discord.gg/y4Vu9jw).

## Full list of Merged PRs

* [`Pull Request 652`](https://github.com/navcoin/navcoin-core/pull/652) [`Commit 0cde809`](https://github.com/navcoin/navcoin-core/commit/0cde80954fd2bdca1859c0e90dfcc3243dba6c61) Remove new version popup
* [`Pull Request 651`](https://github.com/navcoin/navcoin-core/pull/651) [`Commit 95b524e`](https://github.com/navcoin/navcoin-core/commit/95b524eefbfb87190a9038397d6c6a2c8ea081c9) Update translations
* [`Pull Request 648`](https://github.com/navcoin/navcoin-core/pull/648) [`Commit 09f0531`](https://github.com/navcoin/navcoin-core/commit/09f053152761aa254dae27a9da3e338e2e31e671) Update QT strings
* [`Pull Request 647`](https://github.com/navcoin/navcoin-core/pull/647) [`Commit 0eafc3b`](https://github.com/navcoin/navcoin-core/commit/0eafc3b404503c985993a7069bc8cb160100911d) Reactivate CFund
* [`Pull Request 641`](https://github.com/navcoin/navcoin-core/pull/641) [`Commit 494d4e2`](https://github.com/navcoin/navcoin-core/commit/494d4e2d598ad114c407d609d7b8141e4ab54f50) Add extra stats to getblock
* [`Pull Request 634`](https://github.com/navcoin/navcoin-core/pull/634) [`Commit 049978e`](https://github.com/navcoin/navcoin-core/commit/049978e246de960370f629fa38a6509fad0cf5c8) Fix for verifychain
* [`Pull Request 644`](https://github.com/navcoin/navcoin-core/pull/644) [`Commit 5d59483`](https://github.com/navcoin/navcoin-core/commit/5d59483c50d985d3053299febe941dcfb447be46) Adds proposalHash to the RPC getpaymentrequest
* [`Pull Request 643`](https://github.com/navcoin/navcoin-core/pull/643) [`Commit d501c89`](https://github.com/navcoin/navcoin-core/commit/d501c89ea412760de2074d8706fc1684d42d1445) CPaymentRequest fields incorrect in diff
* [`Pull Request 638`](https://github.com/navcoin/navcoin-core/pull/638) [`Commit 8131b42`](https://github.com/navcoin/navcoin-core/commit/8131b4236054c5ee57e253ce75dc77a9992a6bed) Fixed freezing GUI on reindex
* [`Pull Request 632`](https://github.com/navcoin/navcoin-core/pull/632) [`Commit 2ad3391`](https://github.com/navcoin/navcoin-core/commit/2ad3391e5195720af2d288f923081c24df023d99) Fix reference to chain tip
* [`Pull Request 629`](https://github.com/navcoin/navcoin-core/pull/629) [`Commit 2a6c64f`](https://github.com/navcoin/navcoin-core/commit/2a6c64f87858d3452b9b830a0853e993379449d6) Seed nodes
* [`Pull Request 637`](https://github.com/navcoin/navcoin-core/pull/637) [`Commit ad883cd`](https://github.com/navcoin/navcoin-core/commit/ad883cdfa1a1e478f6db30beb41511e97a37bb28) Set DEFAULT_SCRIPTCHECK_THREADS to auto
* [`Pull Request 624`](https://github.com/navcoin/navcoin-core/pull/624) [`Commit 7058550`](https://github.com/navcoin/navcoin-core/commit/7058550da5fe3a2f6ad5493fab763cea285f1a05) Updates to make the wallet.py and stakeimmaturebalance.py test more reliable
* [`Pull Request 636`](https://github.com/navcoin/navcoin-core/pull/636) [`Commit e6f3c23`](https://github.com/navcoin/navcoin-core/commit/e6f3c23f41d5a3a4228f95bd9d67c6acff81f1a3) Only count stakes in main chain
* [`Pull Request 633`](https://github.com/navcoin/navcoin-core/pull/633) [`Commit bfe9071`](https://github.com/navcoin/navcoin-core/commit/bfe90717910cfaa49562efdb14259deb6b208dac) Add second dns seeder
* [`Pull Request 622`](https://github.com/navcoin/navcoin-core/pull/622) [`Commit 37fa72e`](https://github.com/navcoin/navcoin-core/commit/37fa72e386ad3daff58b92ed7dda1a9b0676a43b) CFundDB extra log and ensure read before modify
* [`Pull Request 628`](https://github.com/navcoin/navcoin-core/pull/628) [`Commit b8ed018`](https://github.com/navcoin/navcoin-core/commit/b8ed0180a2deaf616c8e6b38aec42385f0a73879) Restart testnet
* [`Pull Request 623`](https://github.com/navcoin/navcoin-core/pull/623) [`Commit 09ea936`](https://github.com/navcoin/navcoin-core/commit/09ea936f9bd1bb6557d541344345889252d6aef9) Disable CFund client functionality
* [`Pull Request 609`](https://github.com/navcoin/navcoin-core/pull/609) [`Commit 1e73c05`](https://github.com/navcoin/navcoin-core/commit/1e73c05b17bc812057a866b414d66573211ad755) Added clearer error messages for the nRequest amount validation

For additional information about new features, check [https://navcoin.org/en/notices/](https://navcoin.org/en/notices/) 

