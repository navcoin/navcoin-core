# NavCoin v4.6.0 Release Notes

## Community Fund UI

<[Pull Request 428](https://github.com/navcoin/navcoin-core/pull/428)>
<[Commit cbffaee](https://github.com/navcoin/navcoin-core/commit/cbffaeee68d649069e0964b4930d04c441a7b63c)>

- Added a Community Fund tab to the core wallet
- Ability to view and filter proposals and payment request
- Proposals and payment requests can be voted on and created
- Removed the old Community Fund UI

## Accumulation of staking rewards in different address

<[Pull Request 401](https://github.com/navcoin/navcoin-core/pull/401)>
<[Commit 2fb7b47](https://github.com/navcoin/navcoin-core/commit/2fb7b47625dfe866f6079d8c7ac8c1dfb9f9de1d)>
This features introduces support for the `-stakingaddress` launch argument which sets a NavCoin address where the staking rewards are accumulated. It also allows you to specify mappings from one address to another, such that the first address's staking rewards will be deposited in the second address.

`stakingaddress` can take:
- one argument (i.e. one NavCoin address), e.g. `-stakingaddress=NxxxxMyNavCoinAddressxxxxxxxxxxxxx`; or
- a JSON argument, mapping several staking addresses to corresponding receiving addresses, e.g. `-stakingaddress={"NxxxxMyStakingAddress1xxxxxxxxxxxx":"NxxxxMyReceivingAddress1xxxxxxxxxx","NxxxxMyStakingAddress2xxxxxxxxxxxx":"NxxxxMyReceivingAddress2xxxxxxxxxx","NxxxxMyStakingAddress3xxxxxxxxxxxx":"NxxxxMyReceivingAddress3xxxxxxxxxx"}`. One staking address in the JSON argument can also be set to `all` and stakes from any staking address will be sent to the receiving address unless otherwise specified, e.g. `-stakingaddress={"all":"NxxxxMyReceivingAddressALLxxxxxxxx",...}`

Not compatible with cold staking.

## Mnemonic seed phrase support

<[Pull Request 400](https://github.com/navcoin/navcoin-core/pull/400)>
<[Commit 375c657](https://github.com/navcoin/navcoin-core/commit/375c657337c33c56a6b97350ba886bce9ba60c7c)>
This PR adds a new RPC command to export the existing master private key encoded as a mnemonic:
`dumpmnemonic` It admits an argument specifying the language.
Support for two new wallet options (`-importmnemonic` and `-mnemoniclanguage`) have also been added to allow to create a new wallet from the specified mnemonic.

## Fix wrong balance after orphan stakes
<[Pull Request 438](https://github.com/navcoin/navcoin-core/pull/438)>
<[Commit 4041e3e](https://github.com/navcoin/navcoin-core/commit/4041e3ef5de672c6d4e6a20ce5b7f22df090ed14)>
This PR fixes an historical issue which made the wallet show a wrong balance after orphan stakes.

## Index cold staking address unspent output by spending address
<[Pull Request 434](https://github.com/navcoin/navcoin-core/pull/434)>
<[Commit 404d85f](https://github.com/navcoin/navcoin-core/commit/404d85f8ea65bf764d3fa681a4d1483c3e72c507)>

When running a node with -addressindex=1 executing the RPC command “getaddressutxos” with the spending pubkeyhash of a cold staking address will now return any utxo’s available to spend by that key including those where the pubkeyhash is the spending key of a coldstaking transaction. Previously only regular utxo’s sent directly to that pubkeyhash were returned.

## Fix for Payment Request reorganizations
<[Pull Request 456](https://github.com/navcoin/navcoin-core/pull/456)>
<[Commit 688bf4d](https://github.com/navcoin/navcoin-core/commit/688bf4d808ca5b5d3d08fef00d085397bb5b47f0)>

This PR prevents payment requests with invalid hashes (not set yet or out of the main chain) to count for the already requested balance of a proposal.

## Other updates to the NavCoin client, docs and codebase

- Missing increased buffer for cfund gui <[Pull Request 459](https://github.com/navcoin/navcoin-core/pull/459)> <[Commit 073ef14](https://github.com/navcoin/navcoin-core/commit/073ef14a9b46c92d03da20c3b279a8156f6cdaf9)>
- Updated the help text for 'getcoldstakingaddress' RPC/cli call <[Pull Request 458](https://github.com/navcoin/navcoin-core/pull/458)> <[Commit b4a1db5](https://github.com/navcoin/navcoin-core/commit/b4a1db5cdd3afe8e1e7f4a50068b15d162548447)>
- Combined fix for issues #445 and #446 <[Pull Request 450](https://github.com/navcoin/navcoin-core/pull/450)> <[Commit 96198f9](https://github.com/navcoin/navcoin-core/commit/96198f924bd71848d051e7a630c1818854bfa339)>
- Fixes coldstake tx amount for spending wallets <[Pull Request 447](https://github.com/navcoin/navcoin-core/pull/447)> <[Commit 8c93c6b](https://github.com/navcoin/navcoin-core/commit/8c93c6bea3f8aa926675ebe2e9e4bb604738d964)>
- Community Fund GUI date buffer increase <[Pull Request 443](https://github.com/navcoin/navcoin-core/pull/443)> <[Commit 8072efb](https://github.com/navcoin/navcoin-core/commit/8072efb01ad1882c7ea1a853d5d1e5960ae5c61b)>
- Use chainActive.Tip() instead of pindexBestHeader <[Pull Request 442](https://github.com/navcoin/navcoin-core/pull/442)> <[Commit 4de0827](https://github.com/navcoin/navcoin-core/commit/4de08271f82f888d73024317af08723a82fca467)>
- Fix Gitian Build <[Pull Request 441](https://github.com/navcoin/navcoin-core/pull/441)> <[Commit afa2e8b](https://github.com/navcoin/navcoin-core/commit/afa2e8b8e9fd8cf67605e15ac8671e996bcc2e2d)>
- Adds arrayslice.h to Makefile.am <[Pull Request 440](https://github.com/navcoin/navcoin-core/pull/440)> <[Commit 5ba6b6a](https://github.com/navcoin/navcoin-core/commit/5ba6b6affbee20e9298776a99a70331384b1a1e2)>
- Update FreeType depend file to 2.7.1 <[Pull Request 433](https://github.com/navcoin/navcoin-core/pull/433)> <[Commit 6025758](https://github.com/navcoin/navcoin-core/commit/60257582df85c07b794ceb186e2289eada4d3832)>
- Fix crash with -banversion <[Pull Request 432](https://github.com/navcoin/navcoin-core/pull/432)> <[Commit a25b139](https://github.com/navcoin/navcoin-core/commit/a25b1391120b3906d12173a88abce64b405fa0f4)>
- Fixed cold staking report RPC command <[Pull Request 425](https://github.com/navcoin/navcoin-core/pull/425)> <[Commit 765d5be](https://github.com/navcoin/navcoin-core/commit/765d5bee07d1611acc12341f6b99d73c411095ac)>
- Fixed comment syntax in merkle_blocks.py <[Pull Request 426](https://github.com/navcoin/navcoin-core/pull/426)><[Commit f26e2a7](https://github.com/navcoin/navcoin-core/commit/f26e2a78e8ca6ec0c216af4e468e18bdf07a7835)>
- Fixed "GUI: libpng warning" in wallet <[Pull Request 419](https://github.com/navcoin/navcoin-core/pull/419)> <[Commit 0df065e](https://github.com/navcoin/navcoin-core/commit/0df065efe1241d588de1c2fc415bcc9701f679e9)>
- Fixed merkle_blocks.py test randomly failing <[Pull Request 423](https://github.com/navcoin/navcoin-core/pull/423)> <[Commit 52ed487](https://github.com/navcoin/navcoin-core/commit/52ed487a5c5c60f14fdfa3de5ee222c4b6953b4f)>
- Fixed while loops of cfund tests causing random test fails <[Pull Request 418](https://github.com/navcoin/navcoin-core/pull/418)> <[Commit 66a4852](https://github.com/navcoin/navcoin-core/commit/66a48524b98a8f3e382739a61ab763db52c9d670)>
