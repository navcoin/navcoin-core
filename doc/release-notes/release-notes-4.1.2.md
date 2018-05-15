
# NavCoin v4.1.2 Release Notes

## Introduces two new Soft Forks:

### Accumulation of coins in the Community Fund, signaled by version bit 7:
  - Reduction of the staking rewards to 4%.
  - Inclusion of an additional output in the Coinstake transaction contributing 0.25NAV to the Community Fund
  - Consensus validation of the previous rule.

### NTP Sync, signaled by version bit number 8 of staked blocks:
- Requires mandatory clock sync against a NTP server for every node on launch.
- New consensus rule where no blocks in the past can exist, and maximal drift in the future for a block is 60 seconds.
- Peers whose clock drifts more than 30 seconds are disconnected.
- Clock change attempts are automatically readjusted.
- New log category “ntp”.
- New multi argument -ntpserver= to allow user to manually specify servers.
- New argument -ntpminmeasures= to allow user to specify the req. min. of measures.
- New argument -ntptimeout= to allow user to specify how many seconds should we wait for the response of a ntp server.
- New argument -maxtimeoffset= to set the max tolerated clock drift for peers.


  
## An additional network “devnet” is added.
  - Default p2p port: 18886
  - Default rpc port: 44446
  - Default datadir: OS_DATADIR/devnet
  - Enabled through argument -devnet=1
### Devnet Notes
- Ignores Coinstake Output when calculating the Witness Merkle Root, fixing a bug related to Segregated Witness transactions.
- Bans nodes with obsolete versions.
- Removes some recurring log messages.
- Updates the package version of some dependencies, solving compatibilities issues with some operating systems.
- Bundles some dependency libraries with the binaries.
- Updates the seed nodes.
- Fixes some of the test units.
- navcoin-tx tool has been updated to use navcoin’s own transaction structure
