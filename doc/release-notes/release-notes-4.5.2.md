# NavCoin v4.5.2 Release Notes

4.5.2 contains a hard fork which explicitly rejects blocks from obsolete versions (4.5.1 and below) signalled by version bit 21. Upgrading to 4.5.2 will avoid a potential network fork (as described below) and ensure you remain on the same blockchain as the rest of the network.

## Community Fund Hotfixes
### Rejection of Double Payout of Successful Payment Requests
<[Pull Request 413](https://github.com/NAVCoin/navcoin-core/pull/413)> 
<[Commit hash](https://github.com/NAVCoin/navcoin-core/commit/hash)>
Currently if a staker's community fund database gets corrupted, they will read blocks and payout payment requests inconsistently with the rest of the network, which may lead to double payout of successful payment requests. This patch adds checks to reject blocks containing second payouts staked by these nodes. Rejecting these blocks may lead the network to fork, thus requiring the hard fork in this release.

### Voting Cycle Counter Hotfix
<[Pull Request 396](https://github.com/NAVCoin/navcoin-core/pull/396)> 
<[Commit a6782c3](https://github.com/NAVCoin/navcoin-core/commit/a6782c3be14444433b8a2b9abeac9aef7151331d)>
Fixes a bug where the `votingCycle` field of community fund proposals and payment requests would continue to increment after their terminal state if a user reorgs. Also for proposal/payment requests which change from pending to expired, when the voting cycle is 1 over the cycle limit, staker's votes would continue to be counted even though such proposal/payment request is expired. This bug did not affect network consensus on the status of proposal or payment requests.

## Faster Blockchain Sync
<[Pull Request 390](https://github.com/NAVCoin/navcoin-core/pull/390)> 
<[Commit 1990d92](https://github.com/NAVCoin/navcoin-core/commit/1990d929f216e69efa96484b31d3e65ff4196aee)>
Keeps a cache of the community fund votes to avoid an unnecessary recalculation per block which was causing extreme slow synchronization per block. After applying the patch the CountVotes() function is significantly faster.

## Donate Button to the Community Fund
<[Pull Request 396](https://github.com/NAVCoin/navcoin-core/pull/396)> 
<[Commit hash](https://github.com/NAVCoin/navcoin-core/commit/hash)>
Users can now choose to donate to the community fund by using the Send tab of the NavCoin Core wallet. Previously, users could only donate with the `donatefund` rpc command.

## RPC Tests newly implemented and fixed
The RPC unit test work mainly involved fixing broken RPC tests inherited from Bitcoin as well as creating new ones. This expands our test suite allowing for greater coverage which provides quality checks to the existing codebase and future updates.

- Fixes to 17 rpc tests inherited from Bitcoin and implements 2 new tests <[Pull Request 405](https://github.com/NAVCoin/navcoin-core/pull/405)><[Commit ed3f590](https://github.com/NAVCoin/navcoin-core/commit/ed3f590ad8d1b25bfdc6caee153a9372c8180cb6)>
- Added rpc help test <[Pull Request 392](https://github.com/NAVCoin/navcoin-core/pull/392)> <[Commit 6707a98](https://github.com/NAVCoin/navcoin-core/commit/6707a98f4788251fdc5afcea914a456f38926349)>
- Fixes to cfund rpc tests <[Pull Request 376](https://github.com/NAVCoin/navcoin-core/pull/376)> <[Commit 6f157e4](https://github.com/NAVCoin/navcoin-core/commit/6f157e4ba2c92f3f038798baa30eb0aaa563b43d)>

## Other updates to the NavCoin client, docs and codebase
- Restart of testnet <[Pull Request 402](https://github.com/NAVCoin/navcoin-core/pull/402)> <[Commit c821bad](https://github.com/NAVCoin/navcoin-core/commit/c821badee5bfc4910671e37680b731ce52aadd6e)>
- RPC paymentrequestvotelist and proposalvotelist output changed to JSON <[Pull Request 380](https://github.com/NAVCoin/navcoin-core/pull/380)> <[Commit b2ed39a](https://github.com/NAVCoin/navcoin-core/commit/b2ed39a45d190b06b25eb404c02b4c8a3c90f5a7)>
- Disables dark mode on MacOS Mojave <[Pull Request 403](https://github.com/NAVCoin/navcoin-core/pull/403)> <[Commit e436506](https://github.com/NAVCoin/navcoin-core/commit/e4365060007ae08b17fe2de99971677c7d32ce11)>
- Updates to OS X building docs <[Pull Request 406](https://github.com/NAVCoin/navcoin-core/pull/406)><[Commit a1fc1fa](https://github.com/NAVCoin/navcoin-core/commit/a1fc1fa19fcb07194b5955a3a18e6fd5d4f81170)>
- Updates to Unix building docs <[Pull Request 408](https://github.com/NAVCoin/navcoin-core/pull/408)> <[Commit aacbb3d](https://github.com/NAVCoin/navcoin-core/commit/aacbb3dfc51374da649274754d2fec44dc27b342)>
- Updates to gitian building docs for Debian 9 <[Pull Request 365](https://github.com/NAVCoin/navcoin-core/pull/365)> <[Commit 732c257](https://github.com/NAVCoin/navcoin-core/commit/732c257b8a3c9c439c9fef9be7cbb726db118018)>
- Deletes duplicate Qt Creator files <[Pull Request 382](https://github.com/NAVCoin/navcoin-core/pull/382)> <[Commit 8eb4652](https://github.com/NAVCoin/navcoin-core/commit/8eb4652cb9e35524a8449cf4ef1645af47e435ba)>
