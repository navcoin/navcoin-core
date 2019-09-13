# NavCoin v4.5.0 Release Notes

## Cold Staking Protocol Upgrade

<[Pull Request 249](https://github.com/navcoin/navcoin-core/pull/249)>
<[Commit b1c776c](https://github.com/navcoin/navcoin-core/commit/b1c776c605e5bace1d4f6bee50182b92951fd327 )>

This Protocol Upgrade will implement the [NPIP_0002](https://github.com/navcoin/npips/blob/master/npip-0002.mediawiki) which will introduce a new opcode (OP_COINSTAKE) for the NavCoin scripting language and a new standard transaction type using the new opcode.

The wallet will vote this Protocol Upgrade with `YES` by default.

- Adds support for Cold Staking Addresses.
- Signaled by version bit 3.
- Adds new rpc command `getcoldstakingaddress`.
- Wizard to create Cold Staking Addresses in the Receiving tab of the wallet.
- Updates several RPC commands to provide results relating to Cold Staking.
- Adds new RPC tests to test Cold Staking Address creation and use.
- Added a 'Cold Staking' balance display to the wallet GUI.

### Manual testing with a modified wallet client

We also performed a number of tests against the cold staking code using a modified wallet client in an attempt to exploit potential weakness in the code. None of these attempts managed to find an exploit.
Details can be found under [release-notes-4.5.0-additional-testing-notes/modified-coldstaking-client-notes-4.5.0.md](release-notes-4.5.0-additional-testing-notes/modified-coldstaking-client-notes-4.5.0.md).

### Reject this Protocol Upgrade

To not vote for this Protocol Upgrade, add the following line to your `navcoin.conf` file:
`rejectversionbit=3`

## Community Fund Voting GUI

- A GUI interface that can be used to vote for community fund proposals and payment requests.
- A new notification that will appear when a new community fund proposal or payment request is found on the blockchain.
- Also added a new warning to the wallet screen to inform users when their wallet is syncing that their balance may not be displaying accurately.

## Static Rewards Protocol Upgrade

<[Pull Request 328](https://github.com/navcoin/navcoin-core/pull/328)>
<[Commit 9601f85](https://github.com/navcoin/navcoin-core/commit/9601f8501526cba19ded59ae685e393345ef430c)>

This Protocol Upgrade will change the NavCoin Staking block reward to a fixed amount of 2 NAV per block. You can read more about this upgrade at its NPIP page, [NPIP_0004](https://github.com/navcoin/npips/blob/master/npip-0004.mediawiki).

The wallet will vote this Protocol Upgrade with `YES` by default.

- Signaled by version bit 15.
- Adds RPC tests for Static Rewards.

### Reject this Protocol Upgrade

To reject this Protocol Upgrade, add the following line to your `navcoin.conf` file:
`rejectversionbit=15`.

## Dynamic Community Fund Quorum Protocol Upgrade

<[Pull Request 328](https://github.com/navcoin/navcoin-core/pull/333)>
<[Commit 9601f85](https://github.com/navcoin/navcoin-core/commit/c1ea4ac484401d17230cb82481fe17beea168979)>

This Protocol Upgrade would introduce a reduction of the required quorum for the Community Fund in the second half of the votings from 50% to 40%.

The wallet will vote this Protocol Upgrade with `NO` by default.

- Signaled by version bit 17.

### Accept this Protocol Upgrade

To accept this Protocol Upgrade, add the following line to your `navcoin.conf` file:
`acceptversionbit=17`.

## Reject specific version bits by default

<[Pull Request 336](https://github.com/navcoin/navcoin-core/pull/336)>
<[Commit eb6a1a2](https://github.com/navcoin/navcoin-core/commit/eb6a1a27903a477306a7ef73d3d85bd52ff1f3c4)>

By default the wallet votes yes for the Protocol Upgrades included in the wallet. This change adds a list of version bits which will be voted no by default, while also adding an option to manually vote yes for those bits.

To manually vote yes for a version bit add the following to your `navcoin.conf` file:
`acceptversionbit=17`

## Block header spam protection

<[Pull Request 335](https://github.com/navcoin/navcoin-core/pull/335)>
<[Commit 210a22d](https://github.com/navcoin/navcoin-core/commit/210a22daaffbd36d90a5ee0121c0c4ce3de0ed75)>

The wallet will now rate-limit the amount of block headers received from a single peer before banning them for misbehaving. This is an anti-spam measure and is customizable via the config file or via launch arguments.

The new launch arguments are:  

`-headerspamfilter=<0 or 1>` -  1 will turn the filter on (it is on by default), and 0 will turn it off.

`-headerspamfiltermaxsize=<number>` - The number you wish to set as the new max size.

`-headerspamfiltermaxavg=<number>` - The number you wish to set as the new max average.

## Community Fund RPC commands

<[Pull Request 334](https://github.com/navcoin/navcoin-core/pull/334)>
<[Commit cc8e213](https://github.com/navcoin/navcoin-core/commit/cc8e21306cb804671676c6e10c0c2751061e7cc8)>

- Shows help for `proposalvotelist` and `paymentrequestvotelist`.
- Shows proposals and payment requests in pending state without vote.
- Categorises cfund rpc commands under own category.
- Updates RPC tests for these commands.

### Other modifications in the NavCoin client, docs and codebase

- Added github issue and pull request templates. <[Pull Request 347](https://github.com/navcoin/navcoin-core/pull/347)> <[Commit ce2e282](https://github.com/navcoin/navcoin-core/commit/ce2e28295e97398d538f23d795cf20b0544973b2)>.
- Updated link to bootstrap file in wallet gui. <[Pull Request 338](https://github.com/navcoin/navcoin-core/pull/338)> <[Commit 8aa7cdd](https://github.com/navcoin/navcoin-core/commit/8aa7cddc74acac9d1e8e5f7eb50627ec064896fe)>.
- Community Fund RPC Tests clean up. <[Pull Request 318](https://github.com/navcoin/navcoin-core/pull/318)> <[Commit 7730c7b](https://github.com/navcoin/navcoin-core/commit/7730c7bc84256ddb995408c1bc775015f0219d2d)>.
- Fixed changelog link to NPIP. <[Pull Request 317](https://github.com/navcoin/navcoin-core/pull/317)> <[Commit f0ea24c](https://github.com/navcoin/navcoin-core/commit/f0ea24c2228107f765735ec2136f9f20e6eda456)>.
- Updated link to NavCoin github repo. <[Pull Request 314](https://github.com/navcoin/navcoin-core/pull/314)> <[Commit dcdece2](https://github.com/navcoin/navcoin-core/commit/dcdece2be47b4ab55b6231024aef2bc20e7d3b0c)>
- New genesis block for testnet.
