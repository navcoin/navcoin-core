# Mac OS X Build Instructions and Notes

The commands in this guide should be executed in a Terminal application.
The built-in one is located in `/Applications/Utilities/Terminal.app`.

## Preparation

Install the OS X command line tools:

`xcode-select --install`

When the popup appears, click `Install`.

Then install [Homebrew](http://brew.sh).

## Dependencies

1. Install `brew` depends.

    ```bash
    brew install automake berkeley-db4 libtool boost --c++11 miniupnpc openssl pkg-config homebrew/core/protobuf260 --c++11 qt5 libevent curl
    ```

2. Visit [README.md](../depends/README.md) for instructions on building the rest of the dependancies.

NOTE: Building with Qt4 is still supported, however, could result in a broken UI. Building with Qt5 is recommended.

## Compiling libunbound from source

Libunbound needs to be compiled from source. Download last version from http://www.unbound.net, untar, cd to the folder and then `./configure`, `make` and `make install-all`.


## Build NavCoin Core

1.  Clone the navcoin source code and cd into `navcoin`

    ```bash
    git clone https://github.com/NAVCoin/navcoin-core.git
    cd navcoin-core
    ```

2. Build navcoin-core:

    These instructions will configure and build the headless navcoin binaries as well as the GUI (if Qt is found).
    You can disable the GUI build by passing `--without-gui` to configure.

    For an explanation on the `./configure` command and how it might be out of date see [README.md](README.md)

    ```bash
    ./autogen.sh
    ./configure --prefix=`pwd`/depends/x86_64-apple-darwin17.6.0 # This command may be out of date due to OS updates
    make
    ```

3. It is recommended to build and run the unit tests:

    ```bash
    make check
    ```

4. You can also create a .dmg that contains the .app bundle (optional):

    ```bash
    make deploy
    ```

## Running

NavCoin Core is now available at `./src/navcoind`

Before running, it's recommended you create an RPC configuration file.

```bash
echo -e "rpcuser=navcoinrpc\nrpcpassword=$(xxd -l 16 -p /dev/urandom)" > "/Users/${USER}/Library/Application Support/NavCoin4/navcoin.conf"
chmod 600 "/Users/${USER}/Library/Application Support/NavCoin4/navcoin.conf"
```

The first time you run navcoind, it will start downloading the blockchain. This process could take several hours.

You can monitor the download process by looking at the debug.log file:

`tail -f $HOME/Library/Application\ Support/NavCoin4/debug.log`

## Other commands:

```bash
./src/navcoind -daemon # Starts the navcoin daemon.
./src/navcoin-cli --help # Outputs a list of command-line options.
./src/navcoin-cli help # Outputs a list of RPC commands when the daemon is running.
```

## Using Qt Creator as IDE

You can use Qt Creator as an IDE, for navcoin development.
Download and install the community edition of [Qt Creator](https://www.qt.io/download/).
Uncheck everything except Qt Creator during the installation process.

1. Make sure you installed everything through Homebrew mentioned above
2. Do a proper ./configure --enable-debug
3. In Qt Creator do "New Project" -> Import Project -> Import Existing Project
4. Enter "navcoin-qt" as project name, enter src/qt as location
5. Leave the file selection as it is
6. Confirm the "summary page"
7. In the "Projects" tab select "Manage Kits..."
8. Select the default "Desktop" kit and select "Clang (x86 64bit in /usr/bin)" as compiler
9. Select LLDB as debugger (you might need to set the path to your installation)
10. Start debugging with Qt Creator

## QT and Mac OS Sierra

Installing QT through brew in MacOS Sierra could fail during compilation.

https://github.com/Homebrew/homebrew-core/pull/9297

Use the following commands to install Qt 5.7 under MacOS Sierra using a different formula.

```bash
chown root /usr/local/bin/brew
sudo brew update
sudo chown -R $USER /usr/local
brew install qt@5.7
```

## Notes

- Tested on OS X 10.7 through 10.13.5 on 64-bit Intel processors only.

- Building with downloaded Qt binaries is not officially supported. See the notes in [#7714](https://github.com/navcoin/navcoin/issues/7714)
