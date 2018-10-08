# NavCoin v4.4.0 Release Notes

## Community Fund:

The deployment of the Community Fund on mainnet is signaled through Version Bit 6.

It introduces the changes neccesary in the protocol to activate the claims mechanism for the Community Fund with the following parameters:

- Voting cycle (Vc): 2880 * 7 blocks (Approx 1 week)
- Min Quorum per period: 50% of a voting cycle
- Proposals/Payment Requests min age: 50 blocks
- % of Positive votes to accept a Proposal: 75%
- % of Negative votes to reject a Proposal: 67.5%
- % of Positive votes to accept a Payment Request: 50%
- % of Negative votes to reject a Payment Request: 50%
- Fee to create a Proposal: 50 NAV
- Maximum number of full elapsed Voting Cycles for a Proposal: 6 Voting Cycles (1 month and a half)
- Maximum number of full elapsed Voting Cycles for a Payment Request: 8 Voting Cycles (2 months)

## Community Fund Accumulation Spread

The Version Bit 14 will signal for the soft fork to activate NPIP0003.

This is largely a technical improvement which consolidates mined Community Fund contributions to every 500th block instead of every block to reduce blockchain bloat.

You can read more about NPIP0003 on the NPIP GitHub.

## Community Fund Contribution Increase

The Version Bit 15 will signal for the soft fork to increase the Community Fund contribution from 0.25 NAV to 0.50 NAV per block. 

You can read more about NPIP0004 on the NPIP GitHub.

## Reject specific version bits 

This release introduces the concept of bit version rejection. 

It is designed to make it easier for the network to reject individual soft forks when they are bundled together in 1 release. 

As soft forks come within the software signaling by default only the converse was needed for people to reject soft forks they did not agree with.

A new config concept has been added called `rejectversionbit`. Users can signal all the soft forks they reject by adding the following to the config.

```
rejectversionbit=15
rejectversionbit=16
rejectversionbit=17
```

### Other modifications in the NavCoin client:

- RPC Tests fix.
