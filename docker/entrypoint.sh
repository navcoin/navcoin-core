#!/usr/bin/env bash

cd /data

alias yacc="bison"

./autogen.sh
#./configure --prefix=/depends/x86_64-pc-linux-gnu --without-gui
./configure --without-gui --disable-hardening
make
make install

rm -rf /root/.navcoin4/regtest

/usr/local/bin/navcoind