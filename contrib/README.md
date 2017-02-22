Wallet Tools
---------------------

### [SpendFrom](/contrib/spendfrom) ###

Use the raw transactions API to send coins received on a particular
address (or addresses).

Repository Tools
---------------------

### [Developer tools](/contrib/devtools) ###
Specific tools for developers working on this repository.
Contains the script `github-merge.py` for merging github pull requests securely and signing them using GPG.

### [Verify-Commits](/contrib/verify-commits) ###
Tool to verify that every merge commit was signed by a developer using the above `github-merge.py` script.

### [Linearize](/contrib/linearize) ###
Construct a linear, no-fork, best version of the blockchain.

### [Qos](/contrib/qos) ###

A Linux bash script that will set up traffic control (tc) to limit the outgoing bandwidth for connections to the NavCoin network. This means one can have an always-on navcoind instance running, and another local navcoind/navcoin-qt instance which connects to this node and receives blocks from it.

### [Seeds](/contrib/seeds) ###
Utility to generate the pnSeed[] array that is compiled into the client.

Build Tools and Keys
---------------------

### [Debian](/contrib/debian) ###
Contains files used to package navcoind/navcoin-qt
for Debian-based Linux systems. If you compile navcoind/navcoin-qt yourself, there are some useful files here.

### [Gitian-descriptors](/contrib/gitian-descriptors) ###
Notes on getting Gitian builds up and running using KVM.

### [Gitian-keys](/contrib/gitian-keys)
PGP keys used for signing NavCoin Core [Gitian release](/doc/release-process.md) results.

### [MacDeploy](/contrib/macdeploy) ###
Scripts and notes for Mac builds. 

### [RPM](/contrib/rpm) ###
RPM spec file for building navcoin-core on RPM based distributions

Test and Verify Tools 
---------------------

### [TestGen](/contrib/testgen) ###
Utilities to generate test vectors for the data-driven NavCoin tests.

### [Verify Binaries](/contrib/verifybinaries) ###
This script attempts to download and verify the signature file SHA256SUMS.asc from navcoin.org.
