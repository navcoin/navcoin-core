# NavCoin v6.0 Release Notes

## blsCT: Boneh–Lynn–Shacham Confidential Transactions

<[Pull Request 743](https://github.com/navcoin/navcoin-core/pull/743)>

This PR proposes a consensus change signaled by version bit 10 to signal the activation of the blsCT protocol in the mainnet starting on January 15, 2021.

Read more details about blsCT in [doc.nav.community](https://doc.nav.community/blsct.html)

## [DAO] Exclude inactive stakers from votings

<[Pull Request 745](https://github.com/navcoin/navcoin-core/pull/745)>

This PR proposes a consensus change signaled by version bit 12 starting on January 15, 2021 to allow excluding stakers from the DAO votings.

Nodes (like users who do not want to participate in the votings or exchanges which activate staking) will be able to exclude their staked blocks from the quorum by indicating the option -excludevote=1, preventing a scenario where a big enough amount of staking coins not engaging in the governance process could make impossible to reach the acceptance/rejection thresholds.

Additionally, a node will label automatically their blocks to be excluded even if -excludevote=1 is no specified, whenever the staker hasn't been active voting in any of the last 10 voting cycles which had an active vote. This is reverted whenever the staker casts a vote.

Blocks are labeled to be excluded by activating the right-most bit of their nOnce parameter.

## Add support for wallet database encryption

<[Pull Request 717](https://github.com/navcoin/navcoin-core/pull/717)>

This PR introduces wallet transaction data encryption and updates LevelDB to the version 5.8. Wallets created or upgraded with NavCoin Core 6.0 are not compatible with previous versions of NavCoin Core.

## [TEST] Fixed qa/rpc-tests/cfunddb-statehash.py

<[Pull Request 715](https://github.com/navcoin/navcoin-core/pull/715)>

## [RPC] getaddressbalance returns staked value 

<[Pull Request 723](https://github.com/navcoin/navcoin-core/pull/723)>

## [RPC] Address history index
 
<[Pull Request 724](https://github.com/navcoin/navcoin-core/pull/724)>

## [DAO][GUI] Updated links to navexplorer dao pages 

<[Pull Request 725](https://github.com/navcoin/navcoin-core/pull/725)>

## [GUI] Disabled DAO notification if not staking

<[Pull Request 726](https://github.com/navcoin/navcoin-core/pull/726)>

## [RPC] Correctly label isstakable in the validateaddress help response

<[Pull Request 727](https://github.com/navcoin/navcoin-core/pull/727)>

## [GUI] Add voting address to the cold staking wizard

<[Pull Request 728](https://github.com/navcoin/navcoin-core/pull/728)>

## [WALLET] Fix -wallet issue #729

<[Pull Request 730](https://github.com/navcoin/navcoin-core/pull/730)>

## [INDEX] Index cold staking UTXOs by spending address

<[Pull Request 731](https://github.com/navcoin/navcoin-core/pull/731)>

## [RPC] getaddresshistory counts balance for whole history even when range

<[Pull Request 732](https://github.com/navcoin/navcoin-core/pull/732)>

## [GUI] Update getaddresstoreceive.cpp

<[Pull Request 733](https://github.com/navcoin/navcoin-core/pull/733)>

## [CLEAN] Remove declaration of undefined function

<[Pull Request 734](https://github.com/navcoin/navcoin-core/pull/734)>

## [RPC] Fix for getaddresshistory balance of multiple addresses

<[Pull Request 735](https://github.com/navcoin/navcoin-core/pull/735)>

## Update httpserver.cpp header

<[Pull Request 740](https://github.com/navcoin/navcoin-core/pull/740)>

## Add 0x2830 address to team list addresses

<[Pull Request 744](https://github.com/navcoin/navcoin-core/pull/744)>

