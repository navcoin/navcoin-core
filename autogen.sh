#!/bin/sh

export LC_ALL=C
set -e
git submodule update --init --recursive
mkdir -p src/bls-signatures/build
srcdir="$(dirname $0)"
cd "$srcdir"
if [ -z ${LIBTOOLIZE} ] && GLIBTOOLIZE="`which glibtoolize 2>/dev/null`"; then
  LIBTOOLIZE="${GLIBTOOLIZE}"
  export LIBTOOLIZE
fi
which autoreconf >/dev/null || \
  (echo "configuration failed, please install autoconf first" && exit 1)
autoreconf --install --force --warnings=all

cd "$srcdir/src/bls-signatures/build"
cmake ../
cd "$srcdir"
