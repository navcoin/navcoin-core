# Navcoin v6.1.0 Release Notes

## Check inputs standard xNAV

<[Pull Request 797](https://github.com/navcoin/navcoin-core/pull/797)>

## Prevent adding duplicated inputs from candidates

<[Pull Request 798](https://github.com/navcoin/navcoin-core/pull/798)>

## Restart testnet

<[Pull Request 799](https://github.com/navcoin/navcoin-core/pull/799)>

## GUI lock up optimization

<[Pull Request 801](https://github.com/navcoin/navcoin-core/pull/801)>

Optimized how the wallet manages GUI updates from wallet tx data and reports for balance and stake report

## Update aggregationsession.cpp

<[Pull Request 802](https://github.com/navcoin/navcoin-core/pull/802)>

Patch for xNAV duplicated candidates.

## Added a missing style for QWizard background color

<[Pull Request 804](https://github.com/navcoin/navcoin-core/pull/804)>

## Prevent duplicated remove vote

<[Pull Request 805](https://github.com/navcoin/navcoin-core/pull/805)>

When a staker removed a vote for a proposal or payment request, it keeps broadcasting the remove vote in every block instead of using the cache and broadcasting it only in one block.

Example: https://www.navexplorer.com/block/4987836

This PR fixes the behavior.

## Prevent mixed use of NAV and recently swapped xNAV->NAV

<[Pull Request 806](https://github.com/navcoin/navcoin-core/pull/806)>

This PR fixes a bug to prevent the use of NAV outputs together with recently swapped coins from XNAV to NAV as per an issue reported in Discord by mxaddict.

## Use random key instead of blinding key pool when constructing candidate tx

<[Pull Request 809](https://github.com/navcoin/navcoin-core/pull/809)>

## Fixed with-pic flag for libsodium build

<[Pull Request 813](https://github.com/navcoin/navcoin-core/pull/813)>

## Reduce blsCT-related logging

<[Pull Request 816](https://github.com/navcoin/navcoin-core/pull/816)>

## Fix xNAV tx history duplicated

<[Pull Request 817](https://github.com/navcoin/navcoin-core/pull/817)>

## Show encrypted msg in transaction details

<[Pull Request 818](https://github.com/navcoin/navcoin-core/pull/818)>

## Check for null pointer in BuildMixCounters

<[Pull Request 819](https://github.com/navcoin/navcoin-core/pull/819)>

## Fixed: Syntax Error

<[Pull Request 820](https://github.com/navcoin/navcoin-core/pull/820)>

## Update wallet with new logos

<[Pull Request 821](https://github.com/navcoin/navcoin-core/pull/821)>

## Fix memory exhaustion from candidates storage

<[Pull Request 823](https://github.com/navcoin/navcoin-core/pull/823)>

This PR fixes a bug where the in-memory storage of previously seen encrypted candidates, caused out-of-memory crashes and/or low performance.

## Recognise multisig coldstaking output as stakable

<[Pull Request 826](https://github.com/navcoin/navcoin-core/pull/826)>

This PR fixes a bug which did not allow the wallet to recognise multisig cold staking outputs as stakable.

## Prevent excessive remove votes

<[Pull Request 827](https://github.com/navcoin/navcoin-core/pull/827)>

Continuation of #805, this PR fixes a bug where the wallet would still add REMOVE votes if the entry expired.

## Only broadcast xnav when received version

<[Pull Request 829](https://github.com/navcoin/navcoin-core/pull/829)>

This PR fixes a bug which banned peers who broadcasted aggregation sessions and encrypted candidates before completing the handshake.

## Fix coin control issue

<[Pull Request 832](https://github.com/navcoin/navcoin-core/pull/832)>

When selecting an input through the coin control, the actual input used was not the selected one

## Revert patch to fix issues of transaction spending output from the memory pool

<[Pull Request 833](https://github.com/navcoin/navcoin-core/pull/833)>

## Fix address history sorting

<[Pull Request 834](https://github.com/navcoin/navcoin-core/pull/834)>

Changes to use txindex instead of the transaction timestamp to ensure the coinstake is not oddly positioned when calling getaddresshisotry.

## Remove BLSCT verification benchmark logging

<[Pull Request 835](https://github.com/navcoin/navcoin-core/pull/835)>

## Use inventory for aggregation sessions and encrypted candidates

<[Pull Request 836](https://github.com/navcoin/navcoin-core/pull/836)>

This PR introduces a change in the way aggregation sessions and encrypted candidates are propagated.

Old model:

Nodes broadcast through dandelion (first using stem and later in fluff phase) the aggregation sessions and encrypted candidates in full to every node. Due to the size of encrypted candidates (2,6kb), this behaviour exhausts some nodes with low specs and reduces the performance of the wallet.

New model:

Nodes broadcast an INVENTORY message using Dandelion with the hashes (32 bytes) of the aggregation sessions and encrypted candidates they now. The node will request the item's data, only if it's unknown for them, hence reducing the computational overhead and traffic.

Notes:

Nodes running this version will not propagate to older versions, and won't receive from those. This change requires a majority of peers in the network to upgrade for a correct propagation of sessions and candidates.

## Protocol version 80021

<[Pull Request 837](https://github.com/navcoin/navcoin-core/pull/837)>

This PR bumps protocol version to 80021.

Peers with prot.version older than 80020 are banned.
Peers with prot.version 80020 receive a message about the need to update to 80021 (xNAV INV PR #836)
