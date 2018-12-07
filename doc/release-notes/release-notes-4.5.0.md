# NavCoin v4.5.0 Release Notes

## Cold staking softfork

<[Pull Request 249](https://github.com/NAVCoin/navcoin-core/pull/249)>
<[Commit ?](https://github.com/NAVCoin/navcoin-core/commit/ ? )>

This softfork will implement the [NPIP_0002](https://github.com/NAVCoin/npips/blob/master/npip-0002.mediawiki) which will allow for the creation of "cold staking" addresses.

[short explanation of cold staking]

- Adds cold staking softfork
- Adds version bit 3
- Adds new gui wizard for the creation of cold staking addresses
- Adds new rpc command `getcoldstakingaddress`
- Updates several rpc commands to provide results relating to cold staking.
- Adds new RPC tests to test cold staking address creation and use
- Added a 'Cold Staking' balance display to the wallet GUI
- Restarted test net??
  
Full list of updated RPC commands:

(To be written)


### Reject this soft fork

To not vote for this soft fork, add the following line to your `navcoin.conf` file:
`rejectversionbit=3`

You can also pass this as a launch argument:  
`./navcoin-qt -rejectversionbit=3`

## Community Fund Voting GUI

A part of the Cold Staking PR above, this adds two things:

- A GUI interface that can be used to vote for community fund proposals and payment requests
- A new notification that will appear when a new community fund proposal or payment request is found on the blockchain

## Static rewards softfork

<[Pull Request 328](https://github.com/NAVCoin/navcoin-core/pull/328)>
<[Commit 9601f85](https://github.com/NAVCoin/navcoin-core/commit/9601f8501526cba19ded59ae685e393345ef430c)>

This softfork will change the NavCoin Staking block reward to a fixed amount of 2 NAV per block. You can read more about this softfork at it's NPIP page, [NPIP_0004](https://github.com/NAVCoin/npips/blob/master/npip-0004.mediawiki)

- Adds static rewards softfork
- Adds version bit 15
- Adds RPC tests for Static Rewards

### Reject this soft fork

To not vote for this soft fork, add the following line to your `navcoin.conf` file:
`rejectversionbit=15`

You can also pass this as a launch argument:  
`./navcoin-qt -rejectversionbit=15`

## Reject specific version bits by default

<[Pull Request 336](https://github.com/NAVCoin/navcoin-core/pull/336)>
<[Commit eb6a1a2](https://github.com/NAVCoin/navcoin-core/commit/eb6a1a27903a477306a7ef73d3d85bd52ff1f3c4)>

By default the wallet votes yes for soft forks included in the wallet. This changes adds a list (empty at the moment) of version bits which will be voted no by default, while also adding an option to manually vote yes for those bits.

To manually vote yes for a version bit add the following to  your `navcoin.conf` file:
`acceptversionbit=15`

You can also pass this as a launch argument:  
`./navcoin-qt -acceptversionbit=15`

## Block header spam protection fixes

<[Pull Request 335](https://github.com/NAVCoin/navcoin-core/pull/335)>
<[Commit 210a22d](https://github.com/NAVCoin/navcoin-core/commit/210a22daaffbd36d90a5ee0121c0c4ce3de0ed75)>

The wallet will now only accept a minumum number of block headers from a single peer before banning them for misbehaving. This is an anti-spam measure and is customizable via the config file or via launch arguments.

The new launch arguments are:  

`-headerspamfilter=<0 or 1>` -  1 will turn the filter on (it is on by default), and 0 will turn it off.

`-headerspamfiltermaxsize=<number>` - The number you wish to set as the new max size.

`-headerspamfiltermaxavg=<number>` - The number you wish to set as the new max average.

## Community Fund RPC commands

<[Pull Request 334](https://github.com/NAVCoin/navcoin-core/pull/334)>
<[Commit cc8e213](https://github.com/NAVCoin/navcoin-core/commit/cc8e21306cb804671676c6e10c0c2751061e7cc8)>

- Shows help for `proposalvotelist` and `paymentrequestvotelist`
- Shows proposals and payment requests in pending state without vote
- Categorises cfund rpc commands under own category
- Updates RPC tests for these commands

### Other modifications in the NavCoin client

- Community Fund RPC Tests clean up. <[Pull Request 318](https://github.com/NAVCoin/navcoin-core/pull/318)> <[Commit 7730c7b](https://github.com/NAVCoin/navcoin-core/commit/7730c7bc84256ddb995408c1bc775015f0219d2d)>
- Fixed changelog link to NPIP. <[Pull Request 317](https://github.com/NAVCoin/navcoin-core/pull/317)> <[Commit f0ea24c](https://github.com/NAVCoin/navcoin-core/commit/f0ea24c2228107f765735ec2136f9f20e6eda456)>
- Updated link to NavCoin github repo. <[Pull Request 314](https://github.com/NAVCoin/navcoin-core/pull/314)> <[Commit dcdece2](https://github.com/NAVCoin/navcoin-core/commit/dcdece2be47b4ab55b6231024aef2bc20e7d3b0c)>