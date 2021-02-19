# Navcoin v6.0.1 Release Notes

## Tx structure memory optimisation

<[Pull Request 785](https://github.com/navcoin/navcoin-core/pull/785)>

This PR changes the memory structure of a transaction, storing the range proof as a vector instead of a BulletproofsRangeproof element when an output is not private. As the vector's size is 0 when no bulletproof is present, it allows to save the memory used by the range proof for a big part of the transaction history of the chain.

## Optimise sync speed

<[Pull Request 766](https://github.com/navcoin/navcoin-core/pull/766)>

## Apply -blsctmix to threads and gui/rpc send 

<[Pull Request 778](https://github.com/navcoin/navcoin-core/pull/778)>

This PR takes in account if blsctmix has been turned off (blsctmix=0) to disable the background threads and aggregation when sending xNAV.

## [FIX] Wallet false positive on txdata encryption

<[Pull Request 774](https://github.com/navcoin/navcoin-core/pull/774)>

## Update Currency Icons

<[Pull Request 780](https://github.com/navcoin/navcoin-core/pull/780)>

## Removes support for i686 windows binaries in Gitian.

<[Pull Request 782](https://github.com/navcoin/navcoin-core/pull/782)>

## Fix proposal filter accepted expired 

<[Pull Request 777](https://github.com/navcoin/navcoin-core/pull/777)>

## Updated gitian descriptors as well to 6.0.1

<[Pull Request 783](https://github.com/navcoin/navcoin-core/pull/783)>

## Remove i686 commands

<[Pull Request 784](https://github.com/navcoin/navcoin-core/pull/784)>

## Add extra data about supply in RPC commands

<[Pull Request 776](https://github.com/navcoin/navcoin-core/pull/776)>

## Only run xNAV threads if privatebalance > 0

<[Pull Request 786](https://github.com/navcoin/navcoin-core/pull/786)>

## Removes dirty tag from gitian

<[Pull Request 787](https://github.com/navcoin/navcoin-core/pull/787)>



