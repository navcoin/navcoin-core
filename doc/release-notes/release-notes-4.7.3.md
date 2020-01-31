# NavCoin v4.7.3 Release Notes

## Anti Header Spam v2

<[Pull Request 656](https://github.com/navcoin/navcoin-core/pull/656)>
<[Commit 5f11875](https://github.com/navcoin/navcoin-core/commit/5f118753a1900241e9cf8ea38281e4fe75cfeae8)>

Introduces a new anti header spam system which improves the previous implementation and addresses the art-of-bug reports.

Features:

- Every time a header or block is received from another peer, its hash is added to a `points` list associated with the peer. 
- Peers are discerned by their ip address, this means peers sharing ip address will also share the same `points` list. This can be changed with `-headerspamfilterignoreport` (default: `true`).
- Before proceeding with the block or headers validation, the `points` list will be cleared removing all the hashes of blocks whose scripts have already been correctly validated.
- The peer is banned if the size of the `points` list is greater than `MAX_HEADERS_RESULTS*2` once cleared of already validated blocks.
- The maximum allowed size of the `points` list can be changed using the `-headerspamfiltermaxsize` parameter.
- The log category `headerspam` has been added, which prints to the log the current size of a peers `points` list.
- When `-debug=bench` is specified, execution time for the `updateState` function is logged.

#### Considerations

- The maximum size of the `points` list by default is 4,000. With a block time of 30 seconds, NavCoin sees an average of 2,880 blocks per day. A maximum value of 4000 is roughly one and a half times more than the count of blocks a peer needs to be behind the chain tip to be in Initial Block Download mode. When on IBD, the header spam filter is turned off. This ensures that normal synchronisation is not affected by this filter.
- An attacker would be able to exhaust 32 bytes from the hash inserted in the `points` list + 181 bytes from the `CBlockIndex` inserted in `mapBlockIndex` for every invalid header/block before being banned. The `points` list is cleared when the attacker is banned, but those headers are not removed from `mapBlockIndex` or the hard disk in the current implementation. The size of CBlockIndex has been measured with:
```c++
    CBlockIndex* pindex = new CBlockIndex();
    CDataStream ssPeers(SER_DISK, CLIENT_VERSION);
    ss << CDiskBlockIndex(pindex);
    std::vector<unsigned char>vch(ss.begin(), ss.end());
    std::cout << to_string(vch.size()) << std::endl;
```
- The default maximum value means that a single malicious peer with a unique IP can exhaust at max `3,999*213=831 kilobytes` without being banned or `4,000*181=707 kilobytes` being banned.

For additional information about new features, check [https://navcoin.org/en/notices/](https://navcoin.org/en/notices/) 

