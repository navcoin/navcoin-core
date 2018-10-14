# NavCoin v4.3.0 Release Notes

## Introduces OpenAlias:

This PR https://github.com/NAVCoin/navcoin-core/pull/213 completes the implementation of the OpenAlias protocol into the NavCoin Core client.

OpenAlias is an standard created by the Monero Core project used in the Monero wallet and some other clients like Bitcoin Electrum which allows the wallet to translate email addresses into cryptocurrency addresses using custom TXT entries in the DNS records of a domain.

More details about the specification can be found at www.openalias.org

Part of the code is based in the original Monero implementation. The main differences are:

- Prefix of the TXT entry is required to be set to oa1:nav
- The only parsed parameter is recipient_address
- NavCoin enforces by default the use of DNSSEC

### OpenAlias registration
You can now register a OpenAlias address at http://openalias.nav.community/

## Wallet support for bootstrapping

A new argument (-bootstrap) is can be passed on initialisation to specify an URL from where a copy of the blockchain in TAR format will be downloaded and extracted in the data folder.

Also adds a GUI submenu entry under FILE which lets you bootstrap using the GUI.

## New gui tx status for orphans

In version 4.2.0 the wallet started hiding orphan stakes in the transactions list. This has caused some confusion with users as OS notifications were still showing while the stakes did not appear.

We now clasify orphan transactions with a new "Orphan" status and show them in the transactions list.

## Remove BIGNUM use

This PR https://github.com/NAVCoin/navcoin-core/pull/214 completely removes the use of the OpenSSL's class BIGNUM, substituting the uses of CBigNum with the class uint256 with extended arithmetic capabilities (arith_uint256). OpenSSL deprecated some BIGNUM functions in version 1.1, making the wallet unable to compile in systems which use the newer version. This patch fixes this issue.

# ZeroMQ Windows Patch

Applies https://github.com/bitcoin/bitcoin/pull/8238/files to fix ZeroMQ compatibility with Windows systems.

# Staking

Staking can now be toggled "on-the-fly" without restarting the client through the GUI or using the `staking` RPC command.

# Updates secp256k1 Lib

The secp256k1 lib from bitcoin 16.2 release was included into the codebase. While there are a number of improvements, the update was mainly needed due to the better build options for developers and allows newer OS's to build. This reduces the friction in getting setup and working with NavCoin

# Transalations

The Wallet GUI is now fully translated to Spanish and Italian.

### Other modifications in the NavCoin client:

- New RPC command `resolveopenalias` resolves an OpenAlias address to a NavCoin address
- Added support for sending to OpenAlias addresses in the GUI, when parsing URIs and the RPC commands validateaddress and sendtoaddress
- New argument `-requirednssec` to set whether DNSSEC validation is required to resolve openalias addresses (true by default)
- New argument `-mininputvalue` to set the minimum value for an output to be considered as a possible coinstake input
 -New argument `-banversion` to ban nodes depending on their broadcasted version
- Added support to ban nodes with determined wallet versions using the config parameter `banversion`
- Blocks created with the rpc command `generate` now include a correct timestamp for the coinbase transaction
- Using the regtest will create a new genesis block on runtime
- A new testnet has been initiated
- The development-focused networks regtest and devnet won't require peers for blocks generation
- Update copyright notice
- Updates automake to enable dev compilation on New OS's like Ubuntu 18.04
- Updates Payment request UI to handle new OpenSSL Lib