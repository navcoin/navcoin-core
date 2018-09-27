# NavCoin v4.4.0 Release Notes

## Community Fund:



## Reject specific version bits 
This release introduces the concept of bit version rejection. 

It is designed to make it easier for the network to reject individual soft forks when they are bundled together in 1 release. As soft forks come within the software signaling by default only the converse was needed for people to reject soft forks they did not agree with.

A new config concept has been added called `rejectversionbit`  users can signal all the soft forks they reject by adding the following to the config.

```
rejectversionbit=15
rejectversionbit=16
rejectversionbit=17
```


### Other modifications in the NavCoin client:
