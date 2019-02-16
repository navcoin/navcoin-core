DIST_SUBDIRS = secp256k1 univalue

AM_LDFLAGS = $(PTHREAD_CFLAGS) $(LIBTOOL_LDFLAGS) $(HARDENED_LDFLAGS)
AM_CXXFLAGS = $(HARDENED_CXXFLAGS)
AM_CPPFLAGS = $(HARDENED_CPPFLAGS)
EXTRA_LIBRARIES =

if EMBEDDED_UNIVALUE
LIBUNIVALUE = univalue/libunivalue.la

$(LIBUNIVALUE): $(wildcard univalue/lib/*) $(wildcard univalue/include/*)
	$(AM_V_at)$(MAKE) $(AM_MAKEFLAGS) -C $(@D) $(@F)
else
LIBUNIVALUE = $(UNIVALUE_LIBS)
endif

DEVAULT_CONFIG_INCLUDES=-I$(builddir)/config
DEVAULT_INCLUDES=-I$(builddir) -I$(builddir)/obj $(BDB_CPPFLAGS) $(BOOST_CPPFLAGS) $(LEVELDB_CPPFLAGS) $(CRYPTO_CFLAGS) $(SSL_CFLAGS)

DEVAULT_INCLUDES += -I$(srcdir)/secp256k1/include
DEVAULT_INCLUDES += $(UNIVALUE_CFLAGS)

LIBDEVAULT_SERVER=libdevault_server.a
LIBDEVAULT_COMMON=libdevault_common.a
LIBDEVAULT_CONSENSUS=libdevault_consensus.a
LIBDEVAULT_CLI=libdevault_cli.a
LIBDEVAULT_UTIL=libdevault_util.a
LIBDEVAULT_CRYPTO=crypto/libdevault_crypto.a
LIBDEVAULT_ZEROCOIN=libzerocoin/libdevault_zerocoin.a
LIBDEVAULTQT=qt/libdevaultqt.a
LIBSECP256K1=secp256k1/libsecp256k1.la

if ENABLE_ZMQ
LIBDEVAULT_ZMQ=libdevault_zmq.a
endif
if BUILD_DEVAULT_LIBS
LIBDEVAULTCONSENSUS=libdevaultconsensus.la
endif
if ENABLE_WALLET
LIBDEVAULT_WALLET=libdevault_wallet.a
endif

$(LIBSECP256K1): $(wildcard secp256k1/src/*) $(wildcard secp256k1/include/*)
	$(AM_V_at)$(MAKE) $(AM_MAKEFLAGS) -C $(@D) $(@F)

# Make is not made aware of per-object dependencies to avoid limiting building parallelization
# But to build the less dependent modules first, we manually select their order here:
EXTRA_LIBRARIES += \
  $(LIBDEVAULT_CRYPTO) \
  $(LIBDEVAULT_UTIL) \
  $(LIBDEVAULT_COMMON) \
  $(LIBDEVAULT_CONSENSUS) \
  $(LIBDEVAULT_SERVER) \
  $(LIBDEVAULT_CLI) \
  $(LIBDEVAULT_WALLET) \
  $(LIBDEVAULT_ZEROCOIN) \
  $(LIBDEVAULT_ZMQ)

lib_LTLIBRARIES = $(LIBDEVAULTCONSENSUS)

bin_PROGRAMS =
TESTS =
BENCHMARKS =

if BUILD_DEVAULTD
  bin_PROGRAMS += devaultd
endif

if BUILD_DEVAULT_UTILS
  bin_PROGRAMS += devault-cli devault-tx
endif

.PHONY: FORCE check-symbols check-security
# devault core #
DEVAULT_CORE_H = \
  addressindex.h \
  spentindex.h \
  timestampindex.h \
  addrman.h \
  base58.h \
  bloom.h \
  blockencodings.h \
  chain.h \
  chainparams.h \
  chainparamsbase.h \
  chainparamsseeds.h \
  checkpoints.h \
  checkqueue.h \
  clientversion.h \
  coincontrol.h \
  coins.h \
  compat.h \
  compat/byteswap.h \
  compat/endian.h \
  compat/sanity.h \
  compressor.h \
  consensus/cfund.h \
  consensus/consensus.h \
  core_io.h \
  core_memusage.h \
  httprpc.h \
  httpserver.h \
  indirectmap.h \
  kernel.h \
  init.h \
  key.h \
  keystore.h \
  dbwrapper.h \
  limitedmap.h \
  main.h \
  memusage.h \
  merkleblock.h \
  miner.h \
  net.h \
  netbase.h \
  noui.h \
  ntpclient.h \
  policy/fees.h \
  policy/policy.h \
  pow.h \
  pos.h \
  protocol.h \
  random.h \
  reverselock.h \
  rpc/client.h \
  rpc/protocol.h \
  rpc/server.h \
  rpc/register.h \
  scheduler.h \
  script/sigcache.h \
  script/sign.h \
  script/standard.h \
  script/ismine.h \
  streams.h \
  support/allocators/secure.h \
  support/allocators/zeroafterfree.h \
  support/cleanse.h \
  support/pagelocker.h \
  sync.h \
  threadsafety.h \
  timedata.h \
  torcontrol.h \
  txdb.h \
  txmempool.h \
  ui_interface.h \
  undo.h \
  untar.h \
  util.h \
  utilmoneystr.h \
  utils/dns_utils.h \
  utiltime.h \
  validationinterface.h \
  versionbits.h \
  wallet/crypter.h \
  wallet/db.h \
  wallet/rpcwallet.h \
  wallet/wallet.h \
  wallet/walletdb.h \
  wallet/zeropos.h \
  zeroaccumulators.h \
  zerochain.h \
  zeromint.h \
  zerotx.h \
  zerowallet.h \
  zerowitnesser.h \
  zmq/zmqabstractnotifier.h \
  zmq/zmqconfig.h\
  zmq/zmqnotificationinterface.h \
  zmq/zmqpublishnotifier.h


obj/build.h: FORCE
	@$(MKDIR_P) $(builddir)/obj
	@$(top_srcdir)/share/genbuild.sh $(abs_top_builddir)/src/obj/build.h \
	  $(abs_top_srcdir)
libdevault_util_a-clientversion.$(OBJEXT): obj/build.h

# server: shared between devaultd and devault-qt
libdevault_server_a_CPPFLAGS = $(AM_CPPFLAGS) $(DEVAULT_INCLUDES) $(MINIUPNPC_CPPFLAGS) $(EVENT_CFLAGS) $(EVENT_PTHREADS_CFLAGS)
libdevault_server_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
libdevault_server_a_SOURCES = \
  addrman.cpp \
  bloom.cpp \
  blockencodings.cpp \
  chain.cpp \
  checkpoints.cpp \
  consensus/cfund.cpp \
  httprpc.cpp \
  httpserver.cpp \
  kernel.cpp \
  init.cpp \
  dbwrapper.cpp \
  main.cpp \
  merkleblock.cpp \
  miner.cpp \
  net.cpp \
  ntpclient.cpp \
  noui.cpp \
  policy/fees.cpp \
  policy/policy.cpp \
  pow.cpp \
  pos.cpp \
  rest.cpp \
  rpc/blockchain.cpp \
  rpc/mining.cpp \
  rpc/misc.cpp \
  rpc/net.cpp \
  rpc/rawtransaction.cpp \
  rpc/server.cpp \
  script/sigcache.cpp \
  script/ismine.cpp \
  timedata.cpp \
  torcontrol.cpp \
  txdb.cpp \
  txmempool.cpp \
  ui_interface.cpp \
  untar.cpp \
  utils/dns_utils.cpp \
  validationinterface.cpp \
  versionbits.cpp \
  zeroaccumulators.cpp \
  zerochain.cpp \
  zerotx.cpp \
  zerowallet.cpp \
  zerowitnesser.cpp \
  $(DEVAULT_CORE_H)

if ENABLE_ZMQ
libdevault_zmq_a_CPPFLAGS = $(DEVAULT_INCLUDES) $(ZMQ_CFLAGS)
libdevault_zmq_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
libdevault_zmq_a_SOURCES = \
  zmq/zmqabstractnotifier.cpp \
  zmq/zmqnotificationinterface.cpp \
  zmq/zmqpublishnotifier.cpp
endif


# wallet: shared between devaultd and devault-qt, but only linked
# when wallet enabled
libdevault_wallet_a_CPPFLAGS = $(AM_CPPFLAGS) $(DEVAULT_INCLUDES)
libdevault_wallet_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
libdevault_wallet_a_SOURCES = \
  consensus/cfund.cpp \
  wallet/crypter.cpp \
  wallet/db.cpp \
  wallet/rpcdump.cpp \
  wallet/rpcwallet.cpp \
  wallet/wallet.cpp \
  wallet/walletdb.cpp \
  wallet/zeropos.cpp \
  zeroaccumulators.cpp \
  zerochain.cpp \
  zerotx.cpp \
  zerowallet.cpp \
  $(DEVAULT_CORE_H)

# crypto primitives library
crypto_libdevault_crypto_a_CPPFLAGS = $(AM_CPPFLAGS) $(DEVAULT_CONFIG_INCLUDES)
crypto_libdevault_crypto_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
crypto_libdevault_crypto_a_SOURCES = \
  crypto/aes.cpp \
  crypto/aes.h \
  crypto/common.h \
  crypto/hmac_sha256.cpp \
  crypto/hmac_sha256.h \
  crypto/hmac_sha512.cpp \
  crypto/hmac_sha512.h \
  crypto/ripemd160.cpp \
  crypto/ripemd160.h \
  crypto/sha1.cpp \
  crypto/sha1.h \
  crypto/sha256.cpp \
  crypto/sha256.h \
  crypto/sha512.cpp \
  crypto/sha512.h

# consensus: shared between all executables that validate any consensus rules.
libdevault_consensus_a_CPPFLAGS = $(AM_CPPFLAGS) $(DEVAULT_INCLUDES)
libdevault_consensus_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
libdevault_consensus_a_SOURCES = \
  amount.h \
  consensus/merkle.cpp \
  consensus/merkle.h \
  consensus/params.h \
  consensus/validation.h \
  aes_helper.c \
  hashblock.h \
  hash.cpp \
  hash.h \
  prevector.h \
  primitives/block.cpp \
  primitives/block.h \
  primitives/transaction.cpp \
  primitives/transaction.h \
  pubkey.cpp \
  pubkey.h \
  script/devaultconsensus.cpp \
  script/interpreter.cpp \
  script/interpreter.h \
  script/script.cpp \
  script/script.h \
  script/script_error.cpp \
  script/script_error.h \
  serialize.h \
  tinyformat.h \
  uint256.cpp \
  uint256.h \
  uint512.h \
  utilstrencodings.cpp \
  utilstrencodings.h \
  version.h

# common: shared between devaultd, and devault-qt and non-server tools
libdevault_common_a_CPPFLAGS = $(AM_CPPFLAGS) $(DEVAULT_INCLUDES)
libdevault_common_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
libdevault_common_a_SOURCES = \
  amount.cpp \
  base58.cpp \
  chainparams.cpp \
  coins.cpp \
  compressor.cpp \
  core_read.cpp \
  core_write.cpp \
  key.cpp \
  keystore.cpp \
  netbase.cpp \
  protocol.cpp \
  scheduler.cpp \
  script/sign.cpp \
  script/standard.cpp \
  zerotx.cpp \
  $(DEVAULT_CORE_H)

# util: shared between all executables.
# This library *must* be included to make sure that the glibc
# backward-compatibility objects and their sanity checks are linked.
libdevault_util_a_CPPFLAGS = $(AM_CPPFLAGS) $(DEVAULT_INCLUDES)
libdevault_util_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
libdevault_util_a_SOURCES = \
  support/pagelocker.cpp \
  chainparamsbase.cpp \
  clientversion.cpp \
  compat/glibc_sanity.cpp \
  compat/glibcxx_sanity.cpp \
  compat/strnlen.cpp \
  random.cpp \
  rpc/protocol.cpp \
  support/cleanse.cpp \
  sync.cpp \
  util.cpp \
  utilmoneystr.cpp \
  utilstrencodings.cpp \
  utiltime.cpp \
  $(DEVAULT_CORE_H)

# libzerocoin library
libzerocoin_libdevault_zerocoin_a_SOURCES = \
  libzerocoin/Accumulator.h \
  libzerocoin/AccumulatorProofOfKnowledge.h \
  libzerocoin/ArithmeticCircuit.h \
  libzerocoin/bignum.h \
  libzerocoin/bignum.cpp \
  libzerocoin/Bulletproofs.h \
  libzerocoin/Coin.h \
  libzerocoin/CoinSpend.h \
  libzerocoin/Commitment.h \
  libzerocoin/Denominations.h \
  libzerocoin/Keys.h \
  libzerocoin/ParamGeneration.h \
  libzerocoin/Params.h \
  libzerocoin/PolynomialCommitment.h \
  libzerocoin/SerialNumberSignatureOfKnowledge.h \
  libzerocoin/SerialNumberSoK_small.h \
  libzerocoin/SerialNumberProofOfKnowledge.h \
  libzerocoin/SpendType.h \
  libzerocoin/ZerocoinDefines.h \
  libzerocoin/zkplib.h \
  libzerocoin/Accumulator.cpp \
  libzerocoin/AccumulatorProofOfKnowledge.cpp \
  libzerocoin/ArithmeticCircuit.cpp \
  libzerocoin/Bulletproofs.cpp \
  libzerocoin/Coin.cpp \
  libzerocoin/CoinSpend.cpp \
  libzerocoin/Commitment.cpp \
  libzerocoin/Denominations.cpp \
  libzerocoin/Keys.cpp \
  libzerocoin/ParamGeneration.cpp \
  libzerocoin/Params.cpp \
  libzerocoin/PolynomialCommitment.cpp \
  libzerocoin/SerialNumberSignatureOfKnowledge.cpp \
  libzerocoin/SerialNumberSoK_small.cpp \
  libzerocoin/SerialNumberProofOfKnowledge.cpp

libzerocoin_libdevault_zerocoin_a_LIBADD = $(LIBSECP256K1)
libzerocoin_libdevault_zerocoin_a_CPPFLAGS = $(AM_CPPFLAGS) -I$(srcdir)/secp256k1/include $(SSL_CFLAGS)
libzerocoin_libdevault_zerocoin_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
libzerocoin_libdevault_zerocoin_a_CFLAGS = $(AM_CFLAGS)

if GLIBC_BACK_COMPAT
libdevault_util_a_SOURCES += compat/glibc_compat.cpp
endif

# cli: shared between devault-cli and devault-qt
libdevault_cli_a_CPPFLAGS = $(AM_CPPFLAGS) $(DEVAULT_INCLUDES)
libdevault_cli_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
libdevault_cli_a_SOURCES = \
  rpc/client.cpp \
  $(DEVAULT_CORE_H)

nodist_libdevault_util_a_SOURCES = $(srcdir)/obj/build.h
#

# devaultd binary #
devaultd_SOURCES = devaultd.cpp
devaultd_CPPFLAGS = $(AM_CPPFLAGS) $(DEVAULT_INCLUDES)
devaultd_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
devaultd_LDFLAGS = $(RELDFLAGS) $(AM_LDFLAGS) $(LIBTOOL_APP_LDFLAGS)

if TARGET_WINDOWS
devaultd_SOURCES += devaultd-res.rc
endif

devaultd_LDADD = \
  $(LIBDEVAULT_SERVER) \
  $(LIBDEVAULT_COMMON) \
  $(LIBUNIVALUE) \
  $(LIBDEVAULT_UTIL) \
  $(LIBDEVAULT_WALLET) \
  $(LIBDEVAULT_ZMQ) \
  $(LIBDEVAULT_CONSENSUS) \
  $(LIBDEVAULT_CRYPTO) \
  $(LIBDEVAULT_ZEROCOIN) \
  $(LIBLEVELDB) \
  $(LIBMEMENV) \
  $(LIBSECP256K1)

devaultd_LDADD += $(BOOST_LIBS) $(BDB_LIBS) $(SSL_LIBS) $(UNBOUND_LIBS) $(CURL_LIBS) $(CRYPTO_LIBS) $(MINIUPNPC_LIBS) $(EVENT_PTHREADS_LIBS) $(EVENT_LIBS) $(ZMQ_LIBS)

# devault-cli binary #
devault_cli_SOURCES = devault-cli.cpp
devault_cli_CPPFLAGS = $(AM_CPPFLAGS) $(DEVAULT_INCLUDES) $(EVENT_CFLAGS)
devault_cli_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
devault_cli_LDFLAGS = $(RELDFLAGS) $(AM_LDFLAGS) $(LIBTOOL_APP_LDFLAGS)

if TARGET_WINDOWS
devault_cli_SOURCES += devault-cli-res.rc
endif

devault_cli_LDADD = \
  $(LIBDEVAULT_CLI) \
  $(LIBUNIVALUE) \
  $(LIBDEVAULT_UTIL) \
  $(LIBDEVAULT_CRYPTO)

devault_cli_LDADD += $(BOOST_LIBS) $(SSL_LIBS) $(UNBOUND_LIBS) $(CURL_LIBS) $(CRYPTO_LIBS) $(EVENT_LIBS)
#

# devault-tx binary #
devault_tx_SOURCES = devault-tx.cpp
devault_tx_CPPFLAGS = $(AM_CPPFLAGS) $(DEVAULT_INCLUDES)
devault_tx_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
devault_tx_LDFLAGS = $(RELDFLAGS) $(AM_LDFLAGS) $(LIBTOOL_APP_LDFLAGS)

if TARGET_WINDOWS
devault_tx_SOURCES += devault-tx-res.rc
endif

devault_tx_LDADD = \
  $(LIBUNIVALUE) \
  $(LIBDEVAULT_COMMON) \
  $(LIBDEVAULT_UTIL) \
  $(LIBDEVAULT_CONSENSUS) \
  $(LIBDEVAULT_CRYPTO) \
  $(LIBDEVAULT_ZEROCOIN) \
  $(LIBSECP256K1)

devault_tx_LDADD += $(BOOST_LIBS) $(CRYPTO_LIBS)
#

# devaultconsensus library #
if BUILD_DEVAULT_LIBS
include_HEADERS = script/devaultconsensus.h
libdevaultconsensus_la_SOURCES = $(crypto_libdevault_crypto_a_SOURCES) $(libdevault_consensus_a_SOURCES)

if GLIBC_BACK_COMPAT
  libdevaultconsensus_la_SOURCES += compat/glibc_compat.cpp
endif

libdevaultconsensus_la_LDFLAGS = $(AM_LDFLAGS) -no-undefined $(RELDFLAGS) $(BOOST_LIBS)
libdevaultconsensus_la_LIBADD = $(LIBSECP256K1)
libdevaultconsensus_la_CPPFLAGS = $(AM_CPPFLAGS) -I$(builddir)/obj -I$(srcdir)/secp256k1/include -DBUILD_DEVAULT_INTERNAL
libdevaultconsensus_la_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)

endif
#

CTAES_DIST =  crypto/ctaes/bench.c
CTAES_DIST += crypto/ctaes/ctaes.c
CTAES_DIST += crypto/ctaes/ctaes.h
CTAES_DIST += crypto/ctaes/README.md
CTAES_DIST += crypto/ctaes/test.c

CLEANFILES = $(EXTRA_LIBRARIES)

CLEANFILES += *.gcda *.gcno
CLEANFILES += compat/*.gcda compat/*.gcno
CLEANFILES += consensus/*.gcda consensus/*.gcno
CLEANFILES += crypto/*.gcda crypto/*.gcno
CLEANFILES += policy/*.gcda policy/*.gcno
CLEANFILES += primitives/*.gcda primitives/*.gcno
CLEANFILES += script/*.gcda script/*.gcno
CLEANFILES += support/*.gcda support/*.gcno
CLEANFILES += univalue/*.gcda univalue/*.gcno
CLEANFILES += wallet/*.gcda wallet/*.gcno
CLEANFILES += wallet/test/*.gcda wallet/test/*.gcno
CLEANFILES += zmq/*.gcda zmq/*.gcno

DISTCLEANFILES = obj/build.h

EXTRA_DIST = $(CTAES_DIST)

clean-local:
	-$(MAKE) -C secp256k1 clean
	-$(MAKE) -C univalue clean
	-rm -f leveldb/*/*.gcda leveldb/*/*.gcno leveldb/helpers/memenv/*.gcda leveldb/helpers/memenv/*.gcno
	-rm -f config.h
	-rm -rf test/__pycache__

.rc.o:
	@test -f $(WINDRES)
	## FIXME: How to get the appropriate modulename_CPPFLAGS in here?
	$(AM_V_GEN) $(WINDRES) $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) $(CPPFLAGS) -DWINDRES_PREPROC -i $< -o $@

.mm.o:
	$(AM_V_CXX) $(OBJCXX) $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) $(AM_CPPFLAGS) \
	  $(CPPFLAGS) $(AM_CXXFLAGS) $(QT_INCLUDES) $(AM_CXXFLAGS) $(PIE_FLAGS) $(CXXFLAGS) -c -o $@ $<

check-symbols: $(bin_PROGRAMS)
if GLIBC_BACK_COMPAT
	@echo "Checking glibc back compat..."
	$(AM_V_at) READELF=$(READELF) CPPFILT=$(CPPFILT) $(top_srcdir)/contrib/devtools/symbol-check.py < $(bin_PROGRAMS)
endif

check-security: $(bin_PROGRAMS)
if HARDEN
	@echo "Checking binary security..."
	$(AM_V_at) READELF=$(READELF) OBJDUMP=$(OBJDUMP) $(top_srcdir)/contrib/devtools/security-check.py < $(bin_PROGRAMS)
endif

if EMBEDDED_LEVELDB
include Makefile.leveldb.include
endif

if ENABLE_TESTS
include Makefile.test.include
endif

if ENABLE_BENCH
include Makefile.bench.include
endif

if ENABLE_QT
include Makefile.qt.include
endif

if ENABLE_QT_TESTS
include Makefile.qttest.include
endif
