Release Process
====================

Before any code can be accepted into NavCoin Core a Release Candidate branch and PR must be presented to the community for a minimum stand-down period - as detailed below.

### Release Candidates

Release candidates are critical to the NavCoin release eco-system and give the community and interested parties time to review the code and potentially prepare for any changes that may be introduced.  

#### Release Candidates and Release Version Convention

NavCoin follows the Semantic Versioning.

e.g `v(MAJOR).(MINOR).(PATCH)` = `v4.2.1`

1. MAJOR version when you make incompatible changes - This applies to hard forks or any type of breaking changes,
2. MINOR version when you add functionality in a backward-compatible manner - This also applies to any soft forks.
3. PATCH version when you make backward-compatible bug fixes.

When preparing an RC branch and PR the versioning should have `-rc.(release)` appended to the end.

E.g: `v4.5.0-rc.1`

Every time a new release branch is cut the RC version should be increased to help inform the community.

E.g: `v2.5.0-rc.1`, `v2.5.0-rc.2`, `v2.5.0-rc.3`

#### Release Candidates Review Period

* PATCH - 2 Weeks minimum from the time of notification to the community, via signaling by the PR date.

* MINOR - 4 Weeks minimum from the time of notification to the community, via signaling by the PR date.

* MAJOR - 8 Weeks minimum from the time of notification to the community, via signaling by the PR date.

Please note any type of network security or stability issues will be prioritized and might not have any applied stand-down period.

#### Feature Lockdown

During the release candidate review period, no new pull requests should be merged into master except ones designed explicity to fix any issues found during testing of the release candidate. This ensures no new issues are inadvertently introduced into new release candidate or final release versions which could force the complete restart of the testing and review process.

#### Release Candidates Preparation

Before every release candidate:

* Update translations see [translation_process.md](https://github.com/navcoin/navcoin-core/blob/master/doc/translation_process.md#synchronising-translations).

Before every minor and major release:

* Update [npips.md](npips.md) to account for the protocol changes since the last release
* Update version in sources (see below)
* Write release notes (see below)

Before every major release:

* Update hardcoded [seeds](/contrib/seeds/README.md).

### First time / New builders

Check out the source code in the following directory hierarchy.

    cd /path/to/your/toplevel/build
    git clone https://github.com/navcoin/navcoin-core.git

### NavCoin maintainers/release engineers, update version in sources

Update the following:

- `configure.ac`:
    - `_CLIENT_VERSION_MAJOR`
    - `_CLIENT_VERSION_MINOR`
    - `_CLIENT_VERSION_REVISION`
    - Don't forget to set `_CLIENT_VERSION_IS_RELEASE` to `true`, `_CLIENT_BUILD_IS_TEST_RELEASE` to `false` and `_CLIENT_BUILD_IS_RELEASE_CANDIDATE` to `false`
- `src/clientversion.h`: (this mirrors `configure.ac` - see issue #3539)
    - `CLIENT_VERSION_MAJOR`
    - `CLIENT_VERSION_MINOR`
    - `CLIENT_VERSION_REVISION`
    - Don't forget to set `CLIENT_VERSION_IS_RELEASE` to `true`
- `doc/README.md` and `doc/README_windows.txt`
- `doc/Doxyfile`: `PROJECT_NUMBER` contains the full version
- `contrib/gitian-descriptors/*.yml`: usually one'd want to do this on master after branching off the release - but be sure to at least do it before a new major release

Write release notes. git shortlog helps a lot, for example:

    git shortlog --no-merges v(current version, e.g. 4.0.6)..v(new version, e.g. 4.1.0)

Generate list of authors:

    git log --format='%aN' "$*" | sort -ui | sed -e 's/^/- /'

Tag version (or release candidate) in git

    git tag -s v(new version, e.g. 4.1.0)

### Setup and perform Gitian builds

Setup Gitian descriptors:

    pushd ./navcoin-core
    export VERSION=(new version, e.g. v4.1.0, which should also be the name of the repository branch)
    git fetch
    git checkout v${VERSION}
    popd

Ensure gitian-builder is up-to-date:

    pushd ./gitian-builder
    git pull
    popd

### Fetch and create inputs: (first time, or when dependency versions change)

    pushd ./gitian-builder
    mkdir -p inputs
    wget -P inputs https://bitcoincore.org/cfields/osslsigncode-Backports-to-1.7.1.patch
    wget -P inputs http://downloads.sourceforge.net/project/osslsigncode/osslsigncode/osslsigncode-1.7.1.tar.gz
    popd

Create the OS X SDK tarball, see the [OS X readme](README_osx.md) for details, and copy it into the inputs directory.

### Optional: Seed the Gitian sources cache and offline git repositories

By default, Gitian will fetch source files as needed. To cache them ahead of time:

    pushd ./gitian-builder
    make -C ../navcoin-core/depends download SOURCES_PATH=`pwd`/cache/common
    popd

Only missing files will be fetched, so this is safe to re-run for each build.

NOTE: Offline builds must use the --url flag to ensure Gitian fetches only from local URLs. For example:

    pushd ./gitian-builder
    ./bin/gbuild --url navcoin-core=/path/to/navcoin,signature=/path/to/sigs {rest of arguments}
    popd

The gbuild invocations below <b>DO NOT DO THIS</b> by default.

### Build and sign NavCoin Core for Linux, Windows, and OS X:

    pushd ./gitian-builder
    ./bin/gbuild --memory 3000 --commit navcoin-core=${VERSION} ../navcoin-core/contrib/gitian-descriptors/gitian-arm.yml
    mv build/out/navcoin-*.tar.gz build/out/src/navcoin-*.tar.gz ../
    
    ./bin/gbuild --memory 3000 --commit navcoin-core=${VERSION} ../navcoin-core/contrib/gitian-descriptors/gitian-linux.yml
    mv build/out/navcoin-*.tar.gz build/out/src/navcoin-*.tar.gz ../

    ./bin/gbuild --memory 3000 --commit navcoin-core=${VERSION} ../navcoin-core/contrib/gitian-descriptors/gitian-win.yml
    mv build/out/navcoin-*-win-unsigned.tar.gz inputs/navcoin-win-unsigned.tar.gz
    mv build/out/navcoin-*.zip build/out/navcoin-*.exe ../

    ./bin/gbuild --memory 3000 --commit navcoin-core=${VERSION} ../navcoin-core/contrib/gitian-descriptors/gitian-osx.yml
    mv build/out/navcoin-*-osx-unsigned.tar.gz inputs/navcoin-osx-unsigned.tar.gz
    mv build/out/navcoin-*.tar.gz build/out/navcoin-*.dmg ../
    popd

### Next steps:

Commit your signature to gitian.sigs:

    pushd gitian.sigs
    git add ${VERSION}-linux/"${SIGNER}"
    git add ${VERSION}-win-unsigned/"${SIGNER}"
    git add ${VERSION}-osx-unsigned/"${SIGNER}"
    git commit -m "Add ${VERSION} unsigned sigs for ${SIGNER}"
    git push  # Assuming you can push to the gitian.sigs tree
    popd

Codesigner only: Create Windows/macOS detached signatures:
- Only one person handles codesigning. Everyone else should skip to the next step.
- Only once the Windows/macOS builds each have 3 matching signatures may they be signed with their respective release keys.

Codesigner only: Sign the macOS binary:

    transfer navcoin-osx-unsigned.tar.gz to macOS for signing
    tar xf navcoin-osx-unsigned.tar.gz
    ./detached-sig-create.sh -s "Key ID"
    Enter the keychain password and authorize the signature

Now a manual deterministic disk image (dmg) creation is required (gbuilt with `gitian-osx-signer.yml` while having the signatures-osx.tar.gz file in the inputs)

notarize the disk image:

    xcrun altool --notarize-app --primary-bundle-id "org.navcoin.Navcoin-Qt" -u "<code-signer-apple-developer-account-username>" -p "<password>" --file navcoin-${VERSION}-osx.dmg

The notarization takes a few minutes. Check the status:

    xcrun altool --notarization-info <RequestUUID-from-notarize-app-step> -u "<code-signer-apple-developer-account-username>" -p "<password>"

Staple the notarization ticket onto the application

    xcrun stapler staple dist/NavCoin-Qt.app

Codesigner only: Sign the windows binaries:

    tar xf navcoin-win-unsigned.tar.gz
    ./detached-sig-create.sh -key /path/to/codesign.key
    Enter the passphrase for the key when prompted
    signature-win.tar.gz will be created

Codesigner only: Commit the detached codesign payloads:

    cd ~/navcoin-detached-sigs
    #checkout the appropriate branch for this release series
    rm -rf *
    tar xf signature-osx.tar.gz
    tar xf signature-win.tar.gz
    #copy the notarization ticket
    cp dist/NavCoin-Qt.app/Contents/CodeResources osx/dist/NavCoin-Qt.app/Contents/
    git add -a
    git commit -m "point to ${VERSION}"
    git tag -s v${VERSION} HEAD
    git push the current branch and new tag

Non-codesigners: wait for Windows/macOS detached signatures:

- Once the Windows/macOS builds each have 3 matching signatures, they will be signed with their respective release keys.
- Detached signatures will then be committed to the [navcoin-detached-sigs](https://github.com/navcoin/navcoin-detached-sigs) repository, which can be combined with the unsigned apps to create signed binaries.

Create (and optionally verify) the signed macOS binary:

    pushd ./gitian-builder
    ./bin/gbuild -i --commit signature=v${VERSION} ../navcoin-core/contrib/gitian-descriptors/gitian-osx-signer.yml
    ./bin/gsign --signer "$SIGNER" --release ${VERSION}-osx-signed --destination ../gitian.sigs/ ../navcoin-core/contrib/gitian-descriptors/gitian-osx-signer.yml
    ./bin/gverify -v -d ../gitian.sigs/ -r ${VERSION}-osx-signed ../navcoin-core/contrib/gitian-descriptors/gitian-osx-signer.yml
    mv build/out/navcoin-osx-signed.dmg ../navcoin-${VERSION}-osx.dmg
    popd

Create (and optionally verify) the signed Windows binaries:

    pushd ./gitian-builder
    ./bin/gbuild -i --commit signature=v${VERSION} ../navcoin-core/contrib/gitian-descriptors/gitian-win-signer.yml
    ./bin/gsign --signer "$SIGNER" --release ${VERSION}-win-signed --destination ../gitian.sigs/ ../navcoin-core/contrib/gitian-descriptors/gitian-win-signer.yml
    ./bin/gverify -v -d ../gitian.sigs/ -r ${VERSION}-win-signed ../navcoin-core/contrib/gitian-descriptors/gitian-win-signer.yml
    mv build/out/navcoin-*win64-setup.exe ../navcoin-${VERSION}-win64-setup.exe
    popd

Commit your signature for the signed macOS/Windows binaries:

    pushd gitian.sigs
    git add ${VERSION}-osx-signed/"${SIGNER}"
    git add ${VERSION}-win-signed/"${SIGNER}"
    git commit -m "Add ${SIGNER} ${VERSION} signed binaries signatures"
    git push  # Assuming you can push to the gitian.sigs tree
    popd

Build output expected:

  1. source tarball (`navcoin-${VERSION}.tar.gz`)
  2. linux 32-bit and 64-bit dist tarballs (`navcoin-${VERSION}-linux[32|64].tar.gz`)
  3. windows 32-bit and 64-bit unsigned installers and dist zips (`navcoin-${VERSION}-win[32|64]-setup-unsigned.exe`, `navcoin-${VERSION}-win[32|64].zip`)
  4. OS X unsigned installer and dist tarball (`navcoin-${VERSION}-osx-unsigned.dmg`, `navcoin-${VERSION}-osx64.tar.gz`)
  5. Gitian signatures (in `gitian.sigs/${VERSION}-<linux|{win,osx}-unsigned>/(your Gitian key)/`)


The list of files should be:
```
navcoin-${VERSION}-aarch64-linux-gnu.tar.gz
navcoin-${VERSION}-arm-linux-gnueabihf.tar.gz
navcoin-${VERSION}-i686-pc-linux-gnu.tar.gz
navcoin-${VERSION}-x86_64-linux-gnu.tar.gz
navcoin-${VERSION}-osx64.tar.gz
navcoin-${VERSION}-osx.dmg
navcoin-${VERSION}.tar.gz
navcoin-${VERSION}-win32-setup.exe
navcoin-${VERSION}-win32.zip
navcoin-${VERSION}-win64-setup.exe
navcoin-${VERSION}-win64.zip
```
The `*-debug*` files generated by the gitian build contain debug symbols
for troubleshooting by developers. It is assumed that anyone that is interested
in debugging can run gitian to generate the files for themselves. To avoid
end-user confusion about which file to pick, as well as save storage
space *do not upload these to the navcoin.org server, nor put them in the torrent*.

- GPG-sign it, delete the unsigned file:
```
gpg --digest-algo sha256 --clearsign SHA256SUMS # outputs SHA256SUMS.asc
rm SHA256SUMS
```
(the digest algorithm is forced to sha256 to avoid confusion of the `Hash:` header that GPG adds with the SHA256 used for the files)
Note: check that SHA256SUMS itself doesn't end up in SHA256SUMS, which is a spurious/nonsensical entry.

- Upload zips and installers, as well as `SHA256SUMS.asc` from last step, to the navcoin.org server
  into `/var/www/bin/navcoin-core-${VERSION}`

- A `.torrent` will appear in the directory after a few minutes. Optionally help seed this torrent. To get the `magnet:` URI use:
```bash
transmission-show -m <torrent file>
```
Insert the magnet URI into the announcement sent to mailing lists. This permits
people without access to `navcoin.org` to download the binary distribution.
Also put it into the `optional_magnetlink:` slot in the YAML file for
navcoin.org (see below for navcoin.org update instructions).

### Prepare the Release Tag

Once the release candidate is approved and promoted to a final release, a new release tag should be created with the final release notes and binaries attached.

### Update Services

Before publicly announcing the release the Bootstrap & NavPay servers should be updated to the new version.

### Update The NavCoin Website

Update the version number and download links on all translations of the Wallets page;

https://github.com/navcoin/navcoin-org/tree/master/content/wallets

Also create the notice for the release; 

https://github.com/navcoin/navcoin-org/tree/master/content/notices

The notice can be written manually by duplicating and modifying an existing notice, or through the admin section of the website. The admin section relies on your GitHub login having push access to the repo, so you will need to ensure you have the correct repository rights if you want to create it that way.

The hero image for the release notice is usually generated with the navoin canva template to ensure it fits the social sharing spec and aligns wtih the brand guidelies.

### Publicly Announce the release

To ensure resonable due diligence is done to inform the communtiy of new software releases the final release should be announced on all possible NavCoin platforms;

[Reddit](https://reddit.com/r/navcoin), [Twitter](https://twitter.com/navcoin), [Facebook](https://facebook.com/navcoin), [Telegram](https://t.me/navcoin), [Discord](https://discord.gg/y4Vu9jw), [BitcoinTalk](https://bitcointalk.org/index.php?topic=679791.new#new), [Medium](https://medium.com/nav-coin/), Blockfolio Signal & MailChimp.


### Notify Exchanges, Commercial Nodes

All exchanges should be notified of the update;

https://navcoin.org/en/buy-navcoin

Most of the exchanges have their contact emails are consolidated into a mailing list inside the admin@navcoin email account which can be set to the BCC to ensure they receive the update. For ones which are not part of the mailing list their is usually a support form on their site which needs to be filled out.

Additional to exhcanges, the following wallets & services should be notified;

NavExplorer, NavPool, CryptoId, Coinomi, Magnum Wallet, AtomicDex & CoinPayments.

### Celebrate with the Community

Take the time to celebrate the success with everyone who helped to make the release happen! Every release is another step towards a new future where we've claimed back our financial freedom.
