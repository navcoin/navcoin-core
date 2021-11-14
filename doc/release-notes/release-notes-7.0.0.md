# Navcoin v7.0.0 Release Notes

## xNAV upgrades for Navcoin Core 7.0.0

<[Pull Request 862](https://github.com/navcoin/navcoin-core/pull/862)>

This Pull Request introduces various consensus changes related to xNAV signalled by version bit 33. As we did run out of version bits for updates, this PR also introduces a patch to use the nBits field of the block header to signal for version bits greater than 32.

As the right-most bit of nBits is already used to signal blocks excluded from voting, there are 31 bits left for version bit signalling. Therefor the max version bit supported at the moment is 63 and `nBit` version signalling starts at the second right-most bit of the field.

## Summary of changes

### Improvements in the xNAV inter-node mixing protocol.

Sending the public key associated to a candidate transaction leaks no private details about it. By broadcasting the candidate transaction together with the candidates, allows node to filter which transaction they should try to decrypt instead of trying to decrypt every transaction, hence greatly improving CPU load.

### Optimizations in the xNAV verification routines.

There have been improvements in the verification functions of the cryptographic proofs attached to xNAV transactions, resulting in huge speed increases. Seeing numbers down to 15ms in my laptop.

### Changes in the serialisation format of outputs.

- Transaction outputs with the new serialisation format optimise space by using the `value` field to signal which fields contain data and hence only (de)serialising those. (Previous situation would (de)serialise all the fields whether they had useful data or not).

For example, the amount of an output is never assigned space when the output is private. Before it was an 8 byte field filled with zeroes for private transactions. Similar situation was given for NAV outputs in a transaction with xNAV inputs, where the spendingKey and ephemeralKey were not significant but were still consuming 96 bytes of space in total until this patch is applied.

- Two new fields are optionally added to outputs: `std::vector<unsigned char> vData` and `uint256 tokenId`. They will only (de)serialise when their values differ from their initial state.

`vData` can be used to hold arbitrary data. The original intent of this is to substitute the use of the transaction's field `strDZeel` to hold metadata or program predicates. As the `strDZeel` fields of different xNAV transactions can't be aggregated into one, we were missing the extended use derived from the utility of `strDZeel` for private transactions.

`tokenId` signals the token a private output refers to. Its default value `0x0` refers to xNAV. From `tokenId` it is calculated the generator `H` used in the verification of the Bulletproof Rangeproofs without breaking the zero-sum-balance property in the xNAV verification protocol.

### Confidential Tokens: nAssets

This Pull Request introduces Confidential Tokens. They are disabled initially in Mainnet,  but allowing its late activation when the network decides to, through a consensus change vote without the need of a hard fork. Testnet, Devnet and Regnet will have it activated as to allow testing of the new feature.

Built on top of the same cryptographic protocol which makes xNAV possible, we are adding the possibility to mint confidential tokens on the Navcoin blockchain. Transactions of those tokens will veil the details of their sender, receiver and amount transferred.

nAssets can represent arbitrary tokens created by users of the Navcoin network as well as stablecoins with their value pegged and backed by different underlying assets, while benefiting from the privacy-preserving properties of blsCT.

### RPC Commands

- `createtoken <name> <symbol> <max_supply>`

Creates a Confidential Token with name `name`, symbol `symbol` and max supply `max_supply`. If `max_supply` is not indicated, its value will be 100,000,000,000 with divisions down to 8 decimal precision. Names can be duplicated and therefor the token id should be used to differentiate them.

xNAV is used to pay the creation fees.

The token will be managed by a BLS private key from the wallet which runs the RPC command. Token Keys are derived from the master BLS key using the `m/130'/2'/index` path.

- `listtokens`

Shows a list of network-wide available tokens together with their properties and wallet balance of that token, if any.

- `minttoken <token_id> <xnav_address> <amount>`

Mints `amount` of tokens to `xnav_address`. Minting tokens add to the token's current supply and can be executed until the max. supply of a token is depleted.

xNAV is used to pay the minting fees.

- `sendtoken <token_id> <xnav_address> <amount>`

Sends `amount` of tokens to `xnav_address` from current wallet.

xNAV is used to paid the transaction fees.

- `burntoken <token_id> <amount>`

Burns `amount` of tokens from current wallet.

xNAV is used to paid the transaction fees.

## dotNAV

<[Pull Request 877](https://github.com/navcoin/navcoin-core/pull/877)>

This Pull Request introduces the consensus changed needed for a naming system dubbed dotNAV secured by Navcoin's blockchain and signalled for activation with version bit 34 (using block header's nonce).

Names are only allowed with the suffix `.nav` and to have a maximum length of 64 characters. Only allowed characters are alphanumeric and `-`, and the first character can't be a `-`.

Valid names: `satoshi.nav` `satoshi-nakamoto.nav` `satoshi21.nav` `21satoshi.nav`
Invalid names: `-satoshi.nav` `satoshi_nakamoto.nav`

*RPC Command*: `registername name`

Examples:

`registername alex.nav`

Registration has a cost set by a consensus parameter (initially `10 NAV`) and lasts 400 days worth of blocks (modifiable through consensus parameters). Names can be renewed by everyone (not only the owner) with the rpc command `renewname name`. The name's expiry date will be set to 400 days worth of blocks after the moment of the renewal, and it will have the same cost as a registration.

After registration, it requires a minimum of 6 blocks to allow the set of values.

Each name can have different keys, each pointing to an arbitrary value. Those can be exclusively set by the owner of the name and each name has a maximum allowance of 1024 bytes of data.

Domains allow additionally one subdomain, which must stay in the same character set constrains as a name.

*RPC Command*: `updatename name key value`

Examples: 

`updatename alex.nav ip 127.0.0.1`
`updatename alex.nav email alex@nav.community`
`updatename personal.alex.nav email alex.home@nav.community`

Names can be resolved through the RPC command `resolvename name`

## DAO protocol upgrade

<[Pull Request 856](https://github.com/navcoin/navcoin-core/pull/856)>

This PR proposes changes in the DAO voting system signalled by version bit 11.

### Super Proposals

Super Proposals do not take coins out of the Community Fund but print them new.

Super Proposals can be created through the GUI or using the `createproposal` RPC command by setting to `true` the `super` parameter.

They are signalled in the RPC commands with `super_proposal` set to `true`.

Requirement for approval is 75% quorum and 90% acceptance.

#### What to test

- Super Proposals only work when the deployment is active.
- Super Proposals are only approved with the specified requirements.
- Super Proposals do not depend on the amount available on the Community Fund.
- Approval, rejection, expiration, block reorganisation do not affect the amount available in the Community Fund.
- Both RPC and GUI functionality work correctly.

### Combined voting of consensus parameters

Not implemented in GUI yet, only in RPC commands.

A combined vote for change of consensus parameters can be started using the RPC command `proposecombinedconsensuschange`.

Example:

`proposecombinedconsensuschange '[1, 2]' '[100, 100]'`

Additional values can be proposed with `proposeanswer`:

`proposeanswer f54c67f64fa1be51f23d9739d1f6b6a8e10d73f81ea8b7438445b3e52371ba43 '[150, 100]'` 

## [RPC] gettransactionkeys

<[Pull Request 850](https://github.com/navcoin/navcoin-core/pull/850)>

This PR adds a new RPC command to show a transaction input and output keys when combined with txindex.

This helps to greatly improve sync time when using a xNAV light wallet.

Use:

`gettransactionkeys rehash`

Example output:

```
{
  "vin": [
    {
      "txid": "e9a87d14c9afddde24aea40e6da9de48b276b20b0fbee1c3a3518a84d4652a25",
      "vout": 0,
      "outputKey": "89bef7ddb16cef274628d49531a879b75f01e37a7489ed63b315a4c451bd07e785d4b4075c002698ce2efa0aa21d6f19",
      "spendingKey": "b93ad3309d2672c45b1b35ff61de199ecd0ac463d5d7e6d0117dec4a20bded6cf92297cb32c5efd0fa63ed81dd53bf52"
    }
  ],
  "vout": [
    {
      "outputKey": "afc9bc88f80174b46aedee2c01dc5faf0a4c6f8389b42c4c88c88a69a754f44442bab0fdf2079be4d6346914d4fabe99",
      "spendingKey": "a0735dcf02303c50cbc0afe3371900efd73031534a9cc9e5d6b48976b84107de3ba25f921859ea01c209600c228853a8"
    },
    {
      "outputKey": "81a598defb7878411b440029ef4149265eea8403d2c3bca2f8d78d74a7cf940532fabcafc25564067960ed837e7a0c51",
      "spendingKey": "934312ae9d7b553c682117253b347f0917d7991b0557b19d3d07bfdc257597e016d2492f125a0af00d3935090d023e93"
    },
    {
      "script": "6a"
    }
  ]
}
```

## Protocol change: Burn Fees

<[Pull Request 855](https://github.com/navcoin/navcoin-core/pull/855)>

 This PR introduces a consensus change proposal signalled by version bit 9.

If the change gets approved, stakers would not get the transactions fees of their mined blocks added to the reward. This would effectively burn the transaction fees.

## Reset testnet

<[Pull Request 870](https://github.com/navcoin/navcoin-core/pull/870)>

## Update coin control legacy code

<[Pull Request 847](https://github.com/navcoin/navcoin-core/pull/847)>

## build,boost: update download url

<[Pull Request 851](https://github.com/navcoin/navcoin-core/pull/851)>

## fix mutex detection when building bdb on macOS

<[Pull Request 852](https://github.com/navcoin/navcoin-core/pull/852)>

This PR fixes a bug to prevent the use of NAV outputs together with recently swapped coins from XNAV to NAV as per an issue reported in Discord by mxaddict.

## Removed program_options dependency from boost

<[Pull Request 858](https://github.com/navcoin/navcoin-core/pull/858)>

## Updated the configure.ac python version requirement to match upstream changes

<[Pull Request 859](https://github.com/navcoin/navcoin-core/pull/859)>

## Replaced boost::function with std::function calls 

<[Pull Request 860](https://github.com/navcoin/navcoin-core/pull/860)>

## Replaced boost::filesystem with fs:: from upstream code changes

<[Pull Request 863](https://github.com/navcoin/navcoin-core/pull/863)>

## Remove using namespace std; usage 

<[Pull Request 864](https://github.com/navcoin/navcoin-core/pull/864)>

## Removed the use of PAIRTYPE 

<[Pull Request 865](https://github.com/navcoin/navcoin-core/pull/865)>

## Updated the build aux pthread file

<[Pull Request 866](https://github.com/navcoin/navcoin-core/pull/866)>

## Add asm support in configure.ac and updated crc32c based on upstream changes

<[Pull Request 867](https://github.com/navcoin/navcoin-core/pull/867)>

## Update stressor for super dao and combined consensus changes

<[Pull Request 868](https://github.com/navcoin/navcoin-core/pull/868)>

## Updated univalue based on upstream changes

<[Pull Request 869](https://github.com/navcoin/navcoin-core/pull/869)>
