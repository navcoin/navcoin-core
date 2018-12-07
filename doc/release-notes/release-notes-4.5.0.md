# NavCoin v4.5.0 Release Notes

## Static rewards softfork

<[Pull Request 328](https://github.com/NAVCoin/navcoin-core/pull/328)>
<[Commit 9601f85](https://github.com/NAVCoin/navcoin-core/commit/9601f8501526cba19ded59ae685e393345ef430c)>

This softfork will change the NavCoin Staking block reward to a fixed amount of 2 NAV per block. You can read more about this softfork at it's NPIP page, [NPIP_04](https://github.com/NAVCoin/npips/blob/master/npip-0004.mediawiki)

- Adds static rewards softfork
- Adds version bit 15
- Adds RPC tests for Static Rewards

## Reject specific version bits by default

<[Pull Request 336](https://github.com/NAVCoin/navcoin-core/pull/336)>
<[Commit eb6a1a2](https://github.com/NAVCoin/navcoin-core/commit/eb6a1a27903a477306a7ef73d3d85bd52ff1f3c4)>

By default the wallet votes yes for soft forks included in the wallet. This changes adds a list (empty at the moment) of version bits which will be voted no by default, while also adding an option to manually vote yes for those bits.

### Other modifications in the NavCoin client

- Community Fund RPC Tests clean up. <[Pull Request 318](https://github.com/NAVCoin/navcoin-core/pull/318)> <[Commit 7730c7b](https://github.com/NAVCoin/navcoin-core/commit/7730c7bc84256ddb995408c1bc775015f0219d2d)>
- Fixed changelog link to NPIP. <[Pull Request 317](https://github.com/NAVCoin/navcoin-core/pull/317)> <[Commit f0ea24c](https://github.com/NAVCoin/navcoin-core/commit/f0ea24c2228107f765735ec2136f9f20e6eda456)>
- Updated link to NavCoin github repo. <[Pull Request 314](https://github.com/NAVCoin/navcoin-core/pull/314)> <[Commit dcdece2](https://github.com/NAVCoin/navcoin-core/commit/dcdece2be47b4ab55b6231024aef2bc20e7d3b0c)>