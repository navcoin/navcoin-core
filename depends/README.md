## Usage

To build dependencies for the current arch+OS:

```bash
make
```

To build for another arch/OS:

```bash
make HOST=host-platform-triplet
```

For example:

```bash
make HOST=x86_64-w64-mingw32 -j4
```

A prefix will be generated that's suitable for plugging into Bitcoin's
configure. In the above example, a dir named x86_64-w64-mingw32 will be
created. To use it for Bitcoin:

```bash
./configure --prefix=`pwd`/depends/x86_64-w64-mingw32
```

Common `host-platform-triplets` for cross compilation are:

**These may differ depending on which version your operating system is on, for example the OSX folder will change with each update**

| Operating System | Prefix                      |
| ---------------- | --------------------------- |
| Windows 32bit    | `i686-w64-mingw32`          |
| Windows 64bit    | `x86_64-w64-mingw32`        |
| Mac OSX          | `x86_64-apple-darwin17.6.0` |
| Linux ARM 32bit  | `arm-linux-gnueabihf`       |
| Linux ARM 64bit  | `aarch64-linux-gnu`         |


No other options are needed, the paths are automatically configured.

### Dependency Options:

The following can be set when running make: make FOO=bar

`SOURCES_PATH`: downloaded sources will be placed here
`BASE_CACHE`: built packages will be placed here
`SDK_PATH`: Path where sdk's can be found (used by OSX)
`FALLBACK_DOWNLOAD_PATH`: If a source file can't be fetched, try here before giving up
`NO_QT`: Don't download/build/cache qt and its dependencies
`NO_WALLET`: Don't download/build/cache libs needed to enable the wallet
`NO_UPNP`: Don't download/build/cache packages needed for enabling upnp
`DEBUG`: disable some optimizations and enable more runtime checking
`HOST_ID_SALT`: Optional salt to use when generating host package ids
`BUILD_ID_SALT`: Optional salt to use when generating build package ids

If some packages are not built, for example `make NO_WALLET=1`, the appropriate
options will be passed to bitcoin's configure. In this case, `--disable-wallet`.

### Additional targets:

`download`: run 'make download' to fetch all sources without building them
`download-osx`: run 'make download-osx' to fetch all sources needed for osx builds
`download-win`: run 'make download-win' to fetch all sources needed for win builds
`download-linux`: run 'make download-linux' to fetch all sources needed for linux builds

### Other documentation

- [description.md](description.md): General description of the depends system
- [packages.md](packages.md): Steps for adding packages

