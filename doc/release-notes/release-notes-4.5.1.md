# NavCoin v4.5.1 Release Notes

4.5.1 contains a hard fork which explicitly rejects blocks from obsolete versions (4.5.0 and below) signalled by version bit 20. Upgrading to 4.5.1 will avoid a potential network fork (as described below) and ensure you remain on the same blockchain as the rest of the network.

## Community Fund Hotfixes

Currently if a node reindexes their chainstate db, the node will recalculate the deadline of expired proposals and rewrite such proposals as accepted. A node which reindexes will end up with a different state and potentially reject blocks containing payment requests causing some users to end up on their own fork of the blockchain.

<[Pull Request 367](https://github.com/navcoin/navcoin-core/pull/367)>
<[Commit c59cd80](https://github.com/navcoin/navcoin-core/commit/c59cd802e43960ab4ff88dece294dbcecc6bce8e)>

- This patch ensures the state of previously expired proposals are not rewritten as accepted when a chainstate db is reindexed locally.
- Ensures proper expiry of proposals after their duration has been exceeded.
- Adds tests to check the proper expiry of proposals.

<[Pull Request 370](https://github.com/navcoin/navcoin-core/pull/370)>

- Fixes the payout of payment requests when the proposal is already expired. Adds a new state for proposals (`PENDING_VOTING_PREQ`) to handle expired proposals with pending payment requests.
- Adds tests to check the correct implementation of this state.

## Static Rewards Hotfix

<[Pull Request 369](https://github.com/navcoin/navcoin-core/pull/369)>
<[Commit 88a9060](https://github.com/navcoin/navcoin-core/commit/88a9060b80603afdab6dc374ef1144fcb58bc462)>

Currently if a block included both a static reward and some transaction(s), the network will reject such blocks since the total amount of fees would be verified incorrectly. This patch fixes the verification and adds tests to check the blocks are correctly accepted.

## Other modifications in the NavCoin client, docs and codebase

- Fixed output of `cfundstats` rpc command. <[Pull Request 374](https://github.com/navcoin/navcoin-core/pull/374)> <[Commit fe5f8c7](https://github.com/navcoin/navcoin-core/commit/fe5f8c79ea5708692181dfb913e8b17d5517c4ea)>.
- Improvements to core wallet CFund voting window. <[Pull Request 368](https://github.com/navcoin/navcoin-core/pull/368)> <[Commit 1b1077b](https://github.com/navcoin/navcoin-core/commit/1b1077be384c1a230d7c568a7fa05d4b43a4111b)>
- Minor typo fix to coldstaking wizard. <[Pull Request 364](https://github.com/navcoin/navcoin-core/pull/364)> <[Commit 769ff16](https://github.com/navcoin/navcoin-core/commit/769ff16b05f70ccfa24adcaf589bfa5a4157c067)>
