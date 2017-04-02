# NavCoin 4.0

## RELEASE NOTES

### Faster synchronization

NavCoin Core now uses 'headers-first synchronization'. This means that we first ask peers for block headers and validate those. In a second stage, when the headers have been discovered, we download the blocks. However, as we already know about the whole chain in advance, the blocks can be downloaded in parallel from all available peers.

In practice, this means a much faster and more robust synchronization. On recent hardware with a decent network link, it can be as little as 1 hour for an initial full synchronization. You may notice a slower progress in the very first few minutes, when headers are still being fetched and verified, but it should gain speed afterwards.

A few RPCs were added/updated as a result of this:

getblockchaininfo now returns the number of validated headers in addition to the number of validated blocks.
getpeerinfo lists both the number of blocks and headers we know we have in common with each peer. While synchronizing, the heights of the blocks that we have requested from peers (but haven't received yet) are also listed as 'inflight'.

A new RPC getchaintips lists all known branches of the block chain, including those we only have headers for.

### Transaction malleability-related fixes

This release contains a few fixes for transaction ID (TXID) malleability issues:

-nospendzeroconfchange command-line option, to avoid spending zero-confirmation change IsStandard() transaction rules tightened to prevent relaying and mining of mutated transactions Additional information in listtransactions/gettransaction output to report wallet transactions that conflict with each other because they spend the same outputs.

Bug fixes to the getbalance/listaccounts RPC commands, which would report incorrect balances for double-spent (or mutated) transactions.

New option: -zapwallettxes to rebuild the wallet's transaction information

### Navcoin-cli

Another change in the 4.0 release is moving away from the navcoind executable functioning both as a server and as a RPC client. The RPC client functionality ("tell the running navcoin daemon to do THIS") was split into a separate executable, 'navcoin-cli'.

### RPC access control changes

Subnet matching for the purpose of access control is now done by matching the binary network address, instead of with string wildcard matching. For the user this means that -rpcallowip takes a subnet specification, which can be
a single IP address (e.g. 1.2.3.4 or fe80::0012:3456:789a:bcde) a network/CIDR (e.g. 1.2.3.0/24 or fe80::0000/64)
a network/netmask (e.g. 1.2.3.4/255.255.255.0 or fe80::0012:3456:789a:bcde/ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff)

An arbitrary number of -rpcallow arguments can be given. An incoming connection will be accepted if its origin address matches one of them.

Using wildcards will result in the rule being rejected with the following error in debug.log:

Error: Invalid -rpcallowip subnet specification: *. Valid are a single IP (e.g. 1.2.3.4), a network/netmask (e.g. 1.2.3.4/255.255.255.0) or a network/CIDR (e.g. 1.2.3.4/24).

### REST interface

A new HTTP API is exposed when running with the -rest flag, which allows unauthenticated access to public node data.

It is served on the same port as RPC, but does not need a password, and uses plain HTTP instead of JSON-RPC.

Assuming a local RPC server running on port 8332, it is possible to request:

Blocks: http://localhost:8332/rest/block/*HASH*.*EXT*
Blocks without transactions: http://localhost:8332/rest/block/notxdetails/*HASH*.*EXT* Transactions (requires -txindex): http://localhost:8332/rest/tx/*HASH*.*EXT*

In every case, EXT can be bin (for raw binary data), hex (for hex-encoded binary) or json.

For more details, see the doc/REST-interface.md document in the repository.

### RPC Server "Warm-Up" Mode

The RPC server is started earlier now, before most of the expensive intialisations like loading the block index. It is available now almost immediately after starting the process. However, until all initialisations are done, it always returns an immediate error with code -28 to all calls.

This new behaviour can be useful for clients to know that a server is already started and will be available soon (for instance, so that they do not have to start it themselves).

### navcoin-tx

It has been observed that many of the RPC functions offered by navcoind are "pure functions", and operate independently of the navcoind wallet. This included many of the RPC "raw transaction" API functions, such as createrawtransaction.
navcoin-tx is a newly introduced command line utility designed to enable easy manipulation of NavCoin transactions. A summary of its operation may be obtained via "navcoin-tx --help"

Transactions may be created or signed in a manner similar to the RPC raw tx API. Transactions may be updated, deleting inputs or outputs, or appending new inputs and outputs.

This tool may be used for experimenting with new transaction types, signing multi-party transactions, and many other uses. Long term, the goal is to deprecate and remove "pure function" RPC API calls, as those do not require a server round-trip to execute.

### Big endian support

Experimental support for big-endian CPU architectures was added in this release. All little-endian specific code was replaced with endian-neutral constructs. This has been tested on at least MIPS and PPC hosts. The build system will automatically detect the endianness of the target.

### Memory usage optimization

There have been many changes in this release to reduce the default memory usage of a node, among which:

Accurate UTXO cache size accounting; this makes the option -dbcache precise where this grossly underestimated memory usage before

Reduce size of per-peer data structure; this increases the number of connections that can be supported with the same amount of memory

Reduce the number of threads; lowers the amount of (esp. virtual) memory needed

### Privacy: Disable wallet transaction broadcast

This release adds an option -walletbroadcast=0 to prevent automatic transaction broadcast and rebroadcast. This option allows separating transaction submission from the node functionality.

Making use of this, third-party scripts can be written to take care of transaction (re)broadcast:

Send the transaction as normal, either through RPC or the GUI

Retrieve the transaction data through RPC using gettransaction (NOT getrawtransaction). The hex field of the result will contain the raw hexadecimal representation of the transaction

The transaction can then be broadcasted through arbitrary mechanisms supported by the script One such application is selective Tor usage, where the node runs on the normal internet but transactions are broadcasted over Tor.

### Privacy: Stream isolation for Tor

This release adds functionality to create a new circuit for every peer connection, when the software is used with Tor. The new option, -proxyrandomize, is on by default.

When enabled, every outgoing connection will (potentially) go through a different exit node. That significantly reduces the chance to get unlucky and pick a single exit node that is either malicious, or widely banned from the P2P network. This improves connection reliability as well as privacy, especially for the initial connections.

Important note: If a non-Tor SOCKS5 proxy is configured that supports authentication, but doesn't require it, this change may cause that proxy to reject connections. A user and password is sent where they weren't before. This setup is exceedingly rare, but in this case -proxyrandomize=0 can be passed to disable the behavior.

### Reduce upload traffic

A major part of the outbound traffic is caused by serving historic blocks to other nodes in initial block download state.

It is now possible to reduce the total upload traffic via the -maxuploadtarget parameter. This is not a hard limit but a threshold to minimize the outbound traffic. When the limit is about to be reached, the uploaded data is cut by not serving historic blocks (blocks older than one week). Moreover, any SPV peer is disconnected when they request a filtered block.

This option can be specified in MiB per day and is turned off by default (-maxuploadtarget=0). The recommended minimum is 144 * MAX_BLOCK_SIZE (currently 288MB) per day.

Whitelisted peers will never be disconnected, although their traffic counts for calculating the target.

### Direct headers announcement (BIP 130)

Between compatible peers, [BIP 130] (https://github.com/bitcoin/bips/blob/master/bip- 0130.mediawiki) direct headers announcement is used. This means that blocks are advertised by announcing their headers directly, instead of just announcing the hash. In a reorganization, all new headers are sent, instead of just the new tip. This can often prevent an extra roundtrip before the actual block is downloaded.

### Relay: New and only new blocks relayed when pruning

When running in pruned mode, the client will now relay new blocks. When responding to the getblocks message, only hashes of blocks that are on disk and are likely to remain there for some reasonable time window (1 hour) will be returned (previously all relevant hashes were returned).

### Privacy: Automatically use Tor hidden services

Starting with Tor version 0.2.7.1 it is possible, through Tor's control socket API, to create and destroy 'ephemeral' hidden services programmatically. NavCoin Core has been updated to make use of this.

This means that if Tor is running (and proper authorization is available), NavCoin Core automatically creates a hidden service to listen on, without manual configuration. NavCoin Core will also use Tor automatically to connect to other .onion nodes if the control socket can be successfully opened. This will positively affect the number of available .onion nodes and their usage.

This new feature is enabled by default if NavCoin Core is listening, and a connection to Tor can be made. It can be configured with the -listenonion, -torcontrol and -torpassword settings. To show verbose debugging information, pass -debug=tor.

### Notifications through ZMQ

Navcoind can now (optionally) asynchronously notify clients through a ZMQ-based PUB socket of the arrival of new transactions and blocks. This feature requires installation of the ZMQ C API library 4.x and configuring its use through the command line or configuration file. Please see docs/zmq.md for details of operation.

### Linux ARM builds

Linux ARM builds have been added to the uploaded executables.

The following extra files can be found in the download directory or torrent:

navcoin-${VERSION}-arm-linux-gnueabihf.tar.gz: Linux binaries targeting the 32-bit ARMv7-A architecture.

navcoin-${VERSION}-aarch64-linux-gnu.tar.gz: Linux binaries targeting the 64-bit ARMv8-A architecture.

ARM builds are still experimental. Note that the device you use must be (backward) compatible with the architecture targeted by the binary that you use. For example, a Raspberry Pi 2 Model B or Raspberry Pi 3 Model B (in its 32-bit execution state) device, can run the 32-bit ARMv7-A targeted binary. However, no model of Raspberry Pi 1 device can run either binary because they are all ARMv6 architecture devices that are not compatible with ARMv7-A or ARMv8-A.

Note that Android is not considered ARM Linux in this context. The executables are not expected to work out of the box on Android.

### Compact Block support (BIP 152)

Support for block relay using the Compact Blocks protocol has been implemented.

The primary goal is reducing the bandwidth spikes at relay time, though in many cases it also reduces propagation delay. It is automatically enabled between compatible peers.

As a side-effect, ordinary non-mining nodes will download and upload blocks faster if those blocks were produced by miners using similar transaction filtering policies. This means that a miner who produces a block with many transactions discouraged by your node will be relayed slower than one with only transactions already in your memory pool. The overall effect of such relay differences on the network may result in blocks which include widely- discouraged transactions losing a stale block race, and therefore miners may wish to configure their node to take common relay policies into consideration.

### Hierarchical Deterministic Key Generation

Newly created wallets will use hierarchical deterministic key generation according to BIP32 (keypath m/0'/0'/k'). Existing wallets will still use traditional key generation.

Backups of HD wallets, regardless of when they have been created, can therefore be used to re- generate all possible private keys, even the ones which haven't already been generated during the time of the backup. Attention: Encrypting the wallet will create a new seed which requires a new backup!

Wallet dumps (created using the dumpwallet RPC) will contain the deterministic seed. This is expected to allow future versions to import the seed and all associated funds, but this is not yet implemented.

HD key generation for new wallets can be disabled by -usehd=0. Keep in mind that this flag only has affect on newly created wallets. You can't disable HD key generation once you have created a HD wallet.

There is no distinction between internal (change) and external keys. HD wallets are incompatible with older versions of NavCoin Core. Segregated Witness

The code preparations for Segregated Witness ("segwit"), as described in BIP 141, BIP 143, BIP 144, and BIP 145 are finished and included in this release. However, BIP 141 does not yet specify activation parameters on mainnet, and so this release does not support segwit use on mainnet.

Furthermore, because segwit activation is not yet specified for mainnet, version 4.0 will behave similarly as other pre-segwit releases even after a future activation of BIP 141 on the network. Upgrading from 4.0 will be required in order to utilize segwit-related features on mainnet (such as signal BIP 141 activation, fully validate segwit blocks, relay segwit blocks to other segwit nodes, and use segwit transactions in the wallet, etc).

### Reduced block size to 2MB

Block size has been reduced to 2MB to prevent flood attacks. A possible reduction in transaction processing capabilities will be compensated with a future activation of Segregated Witness.

### New NAVTech RPC Commands

A command to generate a private payment has been added. NavCoin Core will take care of the negotiation with the NAVTech nodes and will create and broadcast the transaction. The syntax is:

`anonsend address amount`

Another command, getanondestination, has been added to obtain the encrypted anon destination of a given address.

### Management of NAVTech servers

NAVTech servers added/removed with the RPC command addanonserver are now stored on the config file when specified by an extra parameter.

Additionally a new command listanonservers has been included to list the already added servers.

