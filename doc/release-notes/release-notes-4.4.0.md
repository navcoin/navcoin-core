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

You can read more about [NPIP0003 on the NPIP GitHub](https://github.com/navcoin/npips/blob/master/npip-0003.mediawiki).

## Community Fund Contribution Increase

The Version Bit 16 will signal for the soft fork to increase the Community Fund contribution from 0.25 NAV to 0.50 NAV per block. 

You can read more about [NPIP0005 on the NPIP GitHub](https://github.com/navcoin/npips/blob/master/npip-0005.mediawiki).

## Reject specific version bits 

This release introduces the concept of bit version rejection. 

It is designed to make it easier for the network to reject individual soft forks when they are bundled together in one release. 

As soft forks come within the software signaling by default only the converse was needed for people to reject soft forks they did not agree with.

A new config concept has been added called `rejectversionbit`. Users can signal all the soft forks they reject by adding the following to the config.

```
rejectversionbit=15
rejectversionbit=16
rejectversionbit=17
```


## Community fund RPC commands

With the release of the community fund additional RPC commands are included

### Create a community fund proposal

```
createproposal navcoinaddress amount duration "desc" fee

Arguments:
1. "navcoinaddress" (string, required) The navcoin address where coins would be sent if the proposal is approved.
2. "amount" The amount in NAV to request. eg 100
3. duration: Number of seconds the proposal will exist after being accepted.
4. "desc": Short description of the proposal.
5. fee (optional): Contribution to the fund used as a fee.


Result:
On success, the daemon responds with the hash(id) of the proposal that is used to reference it in other commands

```

### Vote for a community fund proposal

```
proposalvote proposal_hash command

Arguments:
1. "proposal_hash" (string, required) The proposal hash
2. "command"       (string, required) 'yes' to vote yes, 'no' to vote no,'remove' to remove a proposal from the list


Resp:
On success, the daemon responds with the hash(id) of the proposal that is used to reference it in other commands

```

### Create a payment request

```
createpaymentrequest proposal_hash amount id

Arguments:
1. "hash" (string, required) The hash of the proposal from which you want to withdraw funds. It must be approved.
2. "amount" (numeric or string, required) The amount in NAV to withdraw. eg 10
3. "id" (string, required) Unique id to identify the payment request

Result:
{ 
    hash: prequestid, (string) The payment request id.
    strDZeel: string  (string) The attached strdzeel property.
}  

```

### Vote a payment request

```
paymentrequestvote "request_hash" "command"

Adds/removes a proposal to the list of votes.

Arguments:
1. "request_hash" (string, required) The payment request hash
2. "command"      (string, required) 'yes' to vote yes, 'no' to vote no, 'remove' to remove a proposal from the list

```

###  List the community fund proposals

```
listproposals filter

List the propsals and all the releaing datat including payment requests and status.

1. "filter" (string, optional)    "accepted" | "rejected" | "expired" | "pending"
```

###  Donate to the community fund

```
donatefund amount

Donate NAV from your wallet to the commnuity fund

Arguments:
1. "amount" (string, required) The amount of NAV to donate
```

###  Community fund stats

```
cfundstats

Returns the current status of the Community Fund
```

### Other modifications in the NavCoin client:

- RPC Tests fix.
