# NavCoin v4.6.0 Release Notes

## Community Fund UI

<[Pull Request 428](https://github.com/NAVCoin/navcoin-core/pull/428)>
<[Commit cbffaee](https://github.com/NAVCoin/navcoin-core/commit/cbffaeee68d649069e0964b4930d04c441a7b63c)>

- Added a Community Fund tab to the core wallet
- Ability to view and filter proposals and payment request
- Proposals and payment requests can be voted on and created
- Removed the old Community Fund UI

## Accumulation of staking rewards in different address

<[Pull Request 401](https://github.com/NAVCoin/navcoin-core/pull/401)>
<[Commit 2fb7b47](https://github.com/NAVCoin/navcoin-core/commit/2fb7b47625dfe866f6079d8c7ac8c1dfb9f9de1d)>
This features introduces support for the `-stakingaddress` launch argument which sets a NavCoin address where the staking rewards are accumulated. It also allows you to specify mappings from one address to another, such that the first address's staking rewards will be deposited in the second address.

`stakingaddress` can take:
- one argument (i.e. one NavCoin address), e.g. `-stakingaddress=NxxxxMyNavCoinAddressxxxxxxxxxxxxx`; or 
- a JSON argument, mapping several staking addresses to corresponding receiving addresses, e.g. `-stakingaddress={"NxxxxMyStakingAddress1xxxxxxxxxxxx":"NxxxxMyReceivingAddress1xxxxxxxxxx","NxxxxMyStakingAddress2xxxxxxxxxxxx":"NxxxxMyReceivingAddress2xxxxxxxxxx","NxxxxMyStakingAddress3xxxxxxxxxxxx":"NxxxxMyReceivingAddress3xxxxxxxxxx"}`. One staking address in the JSON argument can also be set to `all` and stakes from any staking address will be sent to the receiving address unless otherwise specified, e.g. `-stakingaddress={"all":"NxxxxMyReceivingAddressALLxxxxxxxx",...}`

Not compatible with cold staking.

## Mnemonic seed phrase support

<[Pull Request 400](https://github.com/NAVCoin/navcoin-core/pull/400)>
<[Commit 375c657](https://github.com/NAVCoin/navcoin-core/commit/375c657337c33c56a6b97350ba886bce9ba60c7c)>
This PR adds a new RPC command to export the existing master private key encoded as a mnemonic:
`dumpmnemonic` It admits an argument specifying the language.
Support for two new wallet options (`-importmnemonic` and `-mnemoniclanguage`) have also been added to allow to create a new wallet from the specified mnemonic.

## Other updates to the NavCoin client, docs and codebase

- Update FreeType depend file to 2.7.1 <[Pull Request 433](https://github.com/NAVCoin/navcoin-core/pull/433)> <[Commit 6025758](60257582df85c07b794ceb186e2289eada4d3832)>
- Fix crash with -banversion <[Pull Request 432](https://github.com/NAVCoin/navcoin-core/pull/432)> <[Commit a25b139](https://github.com/NAVCoin/navcoin-core/commit/a25b1391120b3906d12173a88abce64b405fa0f4)>
- Fixed cold staking report RPC command <[Pull Request 425](https://github.com/NAVCoin/navcoin-core/pull/425)> <[Commit 765d5be](https://github.com/NAVCoin/navcoin-core/commit/765d5bee07d1611acc12341f6b99d73c411095ac)>
- Fixed comment syntax in merkle_blocks.py <[Pull Request 426](https://github.com/NAVCoin/navcoin-core/pull/426)><[Commit f26e2a7](https://github.com/NAVCoin/navcoin-core/commit/f26e2a78e8ca6ec0c216af4e468e18bdf07a7835)>
- Fixed "GUI: libpng warning" in wallet <[Pull Request 419](https://github.com/NAVCoin/navcoin-core/pull/419)> <[Commit 0df065e](https://github.com/NAVCoin/navcoin-core/commit/0df065efe1241d588de1c2fc415bcc9701f679e9)>
- Fixed merkle_blocks.py test randomly failing <[Pull Request 423](https://github.com/NAVCoin/navcoin-core/pull/423)> <[Commit 52ed487](https://github.com/NAVCoin/navcoin-core/commit/52ed487a5c5c60f14fdfa3de5ee222c4b6953b4f)>
- Fixed while loops of cfund tests causing random test fails <[Pull Request 418](https://github.com/NAVCoin/navcoin-core/pull/418)> <[Commit 66a4852](https://github.com/NAVCoin/navcoin-core/commit/66a48524b98a8f3e382739a61ab763db52c9d670)>
