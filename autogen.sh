#!/bin/sh
# Copyright (c) 2013-2019 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C
git submodule update --init --recursive
if grep -q 'ifdef _MSC_VER' src/bls/contrib/relic/include/relic_alloc.h; then
  git apply contrib/relic.patch
fi
set -e
srcdir="$(dirname $0)"
cd "$srcdir"
mkdir -p src/bls/build
if [ -z "${LIBTOOLIZE}" ] && GLIBTOOLIZE="$(command -v glibtoolize)"; then
  LIBTOOLIZE="${GLIBTOOLIZE}"
  export LIBTOOLIZE
fi
command -v autoreconf >/dev/null || \
  (echo "configuration failed, please install autoconf first" && exit 1)
autoreconf --install --force --warnings=all
