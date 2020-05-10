#!/bin/sh

export LC_ALL=C
set -e
git submodule update --init --recursive
srcdir="$(dirname $0)"
echo "$srcdir/src/bls-signatures/build"
mkdir -p "$srcdir/src/bls-signatures/build"
cd "$srcdir/src/bls-signatures/build"
cmake ../
cd "$srcdir"
if [ -z ${LIBTOOLIZE} ] && GLIBTOOLIZE="`which glibtoolize 2>/dev/null`"; then
  LIBTOOLIZE="${GLIBTOOLIZE}"
  export LIBTOOLIZE
fi
which autoreconf >/dev/null || \
  (echo "configuration failed, please install autoconf first" && exit 1)
autoreconf --install --force --warnings=all
