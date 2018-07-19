# NavCoin v4.3.0 Release Notes

## Introduces OpenAlias:

This PR https://github.com/NAVCoin/navcoin-core/pull/213 completes the implementation of the open alias protocol in the NavCoin Core client.

OpenAlias is an standard created by the Monero Core project used in the Monero wallet and some other clients like Bitcoin Electrum which allows to translate email addresses into cryptocurrency addresses using custom TXT entries in the DNS records of a domain.

More details about the specification can be found at www.openalias.org

Part of the code is based in the original Monero implementation. Main differences are:

- Prefix of the TXT entry is required to be set to oa1:nav
- The only parsed parameter is recipient_address
- NavCoin enforces by default the use of DNSSEC
- Originally Unbound was used to resolve the entry. This created issues when linking the library in some systems, so alternatively support for using Google's DNS over HTTPS service has been implemented. Future implementation of libunbound+dnscrypt is highly recommended to reduce dependence on 3rd parties

### OpenAlias registration
You can now register a OpenAlias address at http://openalias.nav.community/

### Other modifications in the NavCoin client:

New RPC command `resolveopenalias` resolves an OpenAlias address to a NavCoin address
Added support for sending to OpenAlias addresses in the GUI, when parsing URIs and the RPC commands validateaddress and sendtoaddress
New argument `-requirednssec` to set whether DNSSEC validation is required to resolve openalias addresses (true by default).
Added support to ban nodes with determined wallet versions using the config parameter `banversion`
Update copyright notice

## Wallet support for bootstrapping

This PR https://github.com/NAVCoin/navcoin-core/pull/199 adds support for bootstrapping the blockchain directly from the wallet.

A new argument (-bootstrap) is handled on initialisation to specify an URL from where a copy of the blockchain in TAR format will be downloaded and extracted in the data folder.

Adds too in the GUI a submenu entry under FILE to do this operation graphically.

## New gui tx category for orphans

4.2.0 started hiding orphan stakes in the transactions list causing confusion in the users as OS notifications were still showing while the stakes did not appear.

Instead those transactions are included now in a new category "Orphan" shown in the GUI.

## Remove BIGNUM use

This PR https://github.com/NAVCoin/navcoin-core/pull/214 completely removes the use of the OpenSSL's class BIGNUM, substituting the uses of CBigNum with the class uint256 with extended arithmetic capabilities (arith_uint256). OpenSSL deprecated some BIGNUM functions in version 1.1, making the wallet not being able to compile in systems which use the newer version. This patch fixes the refereed issue.

# ZeroMQ Windows Patch

Applies https://github.com/bitcoin/bitcoin/pull/8238/files to fix ZeroMQ compatibility with Windows systems.
