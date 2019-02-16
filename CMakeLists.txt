CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

IF(NOT CMAKE_BUILD_TYPE)
  SET(CMAKE_BUILD_TYPE "RELEASE")
ENDIF()

if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  list(APPEND CMAKE_PREFIX_PATH /opt/local/opt/qt5)
endif()


project(devault)

include(CheckSymbolExists)
include(CheckCXXCompilerFlag)

# Add path for custom modules
set(CMAKE_MODULE_PATH
  ${CMAKE_MODULE_PATH}
  ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules
)

# If ccache is available, then use it.
find_program(CCACHE ccache)
if(CCACHE)
  set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ${CCACHE})
  set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ${CCACHE})
endif(CCACHE)

#---------------------------------------------------------------------------------------------------------
# Code coverage
#---------------------------------------------------------------------------------------------------------
if (${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
    set(CMAKE_XCODE_ATTRIBUTE_GCC_VERSION "com.apple.compilers.llvm.clang.1_0")
    # Setup Code Coverage for XCode when requested
    if ($ENV{COVERAGE})
        message(STATUS "********* Setting up Xcode compile for Code coverage **********")
        set(CMAKE_XCODE_ATTRIBUTE_GCC_INSTRUMENT_PROGRAM_FLOW_ARCS "YES")
        set(CMAKE_XCODE_ATTRIBUTE_GCC_GENERATE_TEST_COVERAGE_FILES "YES")
        # Works for Clang->
				#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -coverage")
    endif()
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 ")

set(CMAKE_POSITION_INDEPENDENT_CODE ON) # for all targets

message(STATUS "BUILD TYPE is ${CMAKE_BUILD_TYPE}")

if(CMAKE_CXX_FLAGS_RELEASE MATCHES DNDEBUG)
  string(REPLACE "-DNDEBUG" "" CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")
  string(REPLACE "-DNDEBUG" "" CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE}")
endif()

if(CMAKE_CXX_FLAGS_RELWITHDEBINFO MATCHES DNDEBUG)
  string(REPLACE "-DNDEBUG" "" CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELEASE}")
  string(REPLACE "-DNDEBUG" "" CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELEASE}")
endif()

if(CMAKE_CXX_FLAGS MATCHES DNDEBUG)
  string(REPLACE "-DNDEBUG" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
  string(REPLACE "-DNDEBUG" "" CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
endif()



IF (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
	add_compile_options(-Wno-potentially-evaluated-expression -Wno-unused-const-variable -Wno-unused-function -Wno-deprecated-declarations -Wno-inaccessible-base -Wno-unused-local-typedefs -Wno-logical-op-parentheses -Wno-shift-negative-value -Wno-array-bounds)
	add_definitions("-DOS_MACOSX -DLEVELDB_PLATFORM_POSIX -DENABLE_ZMQ")
	add_definitions("-DENABLE_WALLET")
#	add_definitions("-Werror")
else()
	add_definitions("-Wno-cpp -Wno-unused-function")
	set(ANL anl)
endif()

# Disable -DHAVE_BUILD_INFO for now to avoid compiler error
add_definitions("-DHAVE_CONFIG_H -DBOOST_SPIRIT_THREADSAFE -D__STDC_FORMAT_MACROS -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=2")

add_definitions("-Wall -Wextra -Wformat -Wformat-security -Wno-unused-parameter -Wstack-protector -fstack-protector-all -fPIC -fvisibility=hidden")

# run autogen.sh if missing header files from configure on Linux/Mac
if (EXISTS	"${CMAKE_CURRENT_SOURCE_DIR}/configure")
else()	
  execute_process(
	COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/autogen.sh
	WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
	)
endif()

# run configure if devault_config.h doesn't exist, For Linux add "--with-incompatible-bdb"
if (EXISTS	"${CMAKE_CURRENT_SOURCE_DIR}/src/config/devault-config.h")
else()	
	IF (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
	  execute_process(
		COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/configure 
		WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
		)
	  execute_process(
		COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/src/secp256k1/configure --enable-module-recovery
		WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
		)
	  execute_process(
		COMMAND make
		WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src/secp256k1
		)
else()
	  execute_process(
		COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/configure  --with-incompatible-bdb
		WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
		)
	  execute_process(
		COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/src/secp256k1/configure --enable-module-recovery
		WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
		)
	  execute_process(
		COMMAND make
		WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src/secp256k1
		)
	endif()
endif()


SET(COMMON_A
	./src/protocol.cpp
	./src/pubkey.cpp
	./src/netbase.cpp
	./src/keystore.cpp
	./src/key.cpp
	./src/hash.cpp
	./src/core_write.cpp
	./src/core_read.cpp
	./src/compressor.cpp
	./src/coins.cpp
	./src/chainparams.cpp
	./src/base58.cpp
  ./src/amount.cpp
	)

SET(UTIL_A
	./src/utiltime.cpp
	./src/utilmoneystr.cpp
	./src/util.cpp
	./src/sync.cpp
	./src/uint256.cpp
  #	./src/arith_uint256.cpp
	./src/rpc/protocol.cpp
	./src/random.cpp
	./src/clientversion.cpp
	./src/chainparamsbase.cpp
	)

SET(SERVER_A
  ./src/kernel.cpp
	./src/addrman.cpp
	./src/bloom.cpp
	./src/chain.cpp
	./src/checkpoints.cpp
	./src/init.cpp
	./src/main.cpp
  ./src/ui_interface.cpp
  ./src/ntpclient.cpp
  ./src/pos.cpp

	./src/merkleblock.cpp
	./src/miner.cpp
	./src/net.cpp
	./src/noui.cpp
	./src/pow.cpp
	./src/rest.cpp
	./src/rpc/blockchain.cpp
	./src/rpc/mining.cpp
	./src/rpc/client.cpp
	./src/rpc/rawtransaction.cpp
	./src/rpc/net.cpp
	./src/rpc/protocol.cpp
	./src/rpc/server.cpp
	./src/rpc/misc.cpp
#x	./src/rpc/util.cpp

  #// For QT
#  ./src/interfaces/node.cpp
#  ./src/interfaces/handler.cpp
#  ./src/interfaces/wallet.cpp
###./src/minerwhitelist.cpp
	)


SET(CONSENSUS
	./src/utilstrencodings.cpp
	./src/pubkey.cpp
	./src/uint256.cpp
#	./src/arith_uint256.cpp
	./src/hash.cpp
)	

SET(CONSENSUS_CRYPTO
	./src/crypto/hmac_sha512.cpp
	./src/crypto/hmac_sha256.cpp
	./src/crypto/ripemd160.cpp
	./src/crypto/sha256.cpp
	./src/crypto/sha1.cpp
	./src/crypto/sha512.cpp
  ./src/crypto/ctaes/ctaes.c
  ./src/crypto/aes.cpp
)

SET(SOURCES

  ./src/zerochain.cpp
  ./src/zerowallet.cpp
  ./src/zerowitnesser.cpp
  ./src/zeroaccumulators.cpp
  ./src/zerotx.cpp
  ./src/untar.cpp

	./src/compat/strnlen.cpp
	./src/compat/glibcxx_sanity.cpp
	./src/compat/glibc_sanity.cpp
  #	./src/compat/glibc_compat.cpp
  
  ./src/consensus/merkle.cpp
  ./src/consensus/cfund.cpp

  ./src/utils/dns_utils.cpp
  
  ./src/utiltime.cpp
  ./src/chain.cpp
  ./src/protocol.cpp
  ./src/versionbits.cpp
  ./src/dbwrapper.cpp
  
  ./src/blockencodings.cpp
	./src/miner.cpp
	./src/primitives/block.cpp
	./src/primitives/transaction.cpp
	./src/script/script_error.cpp
	./src/script/interpreter.cpp
	./src/script/script.cpp
	./src/script/devaultconsensus.cpp
	./src/script/sign.cpp
	./src/script/sigcache.cpp
	./src/script/standard.cpp
	./src/script/ismine.cpp
	./src/timedata.cpp
	./src/txdb.cpp
  ./src/rest.cpp
	./src/torcontrol.cpp
	./src/txmempool.cpp
	./src/wallet/zeropos.cpp
	./src/wallet/wallet.cpp
	./src/wallet/db.cpp
	./src/wallet/crypter.cpp
	./src/wallet/walletdb.cpp
	./src/wallet/rpcdump.cpp
	./src/wallet/rpcwallet.cpp
  ./src/univalue/lib/univalue_read.cpp
  ./src/univalue/lib/univalue_write.cpp
  ./src/univalue/lib/univalue.cpp
  ./src/httprpc.cpp
  ./src/httpserver.cpp
  ./src/scheduler.cpp
  ./src/validationinterface.cpp
  ./src/support/pagelocker.cpp
  ./src/support/cleanse.cpp
  ./src/policy/fees.cpp
  ./src/policy/policy.cpp

	)



SET(ZMQ_SOURCES
  ./src/zmq/zmqnotificationinterface.cpp
  ./src/zmq/zmqpublishnotifier.cpp
  ./src/zmq/zmqabstractnotifier.cpp
  )
  

# Dependencies
include(BrewHelper)
find_brew_prefix(OPENSSL_ROOT_DIR openssl)
find_package(OpenSSL REQUIRED)
if (OPENSSL_FOUND)
  message(STATUS "Found OpenSSL")
endif()  

find_package(BerkeleyDB REQUIRED)
if (BERKELEYDB_FOUND)
  message(STATUS "Found BerkeleyDB")
  message(STATUS "BDB inc dir is ${BDB_INCLUDE_DIR}")
endif()

find_package(gmp REQUIRED)
if (GMP_FOUND)
  message(STATUS "Found libgmp")
endif()

find_package( Boost COMPONENTS system filesystem thread program_options chrono)
link_directories ( ${Boost_LIBRARY_DIRS} )

include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}/src
  ${CMAKE_CURRENT_SOURCE_DIR}/src/config
  ${CMAKE_CURRENT_SOURCE_DIR}/src/obj
  ${Boost_INCLUDE_DIRS}
  ${OPENSSL_INCLUDE_DIR}
  ${BDB_INCLUDE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/src/leveldb/include
  ${CMAKE_CURRENT_SOURCE_DIR}/src/univalue/include
  ${CMAKE_CURRENT_SOURCE_DIR}/src/leveldb/helpers/memenv
  ${CMAKE_CURRENT_SOURCE_DIR}/src/secp256k1
  ${CMAKE_CURRENT_SOURCE_DIR}/src/secp256k1/include
  ${CMAKE_CURRENT_SOURCE_DIR}/src/secp256k1/src/modules/ecdh
  ${CMAKE_CURRENT_SOURCE_DIR}/src/secp256k1/src/
  ${CMAKE_CURRENT_SOURCE_DIR}/src/qt/
  ${CMAKE_CURRENT_SOURCE_DIR}/src/qt/forms
  )

file(GLOB HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h)

file(GLOB SCRIPT_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/src/script/*.h)
source_group("ScriptHeaders" FILES ${SCRIPT_HEADERS})

file(GLOB CRYPTO_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/src/crypto/*.h)
source_group("CryptoHeaders" FILES ${CRYPTO_HEADERS})

file(GLOB CONSENSUS_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/src/consensus/*.h)
source_group("ConsensusHeaders" FILES ${CONSENSUS_HEADERS})

file(GLOB PRIMITIVE_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/src/primitives/*.h)
source_group("PrimHeaders" FILES ${PRIMITIVE_HEADERS})

add_library(coin ${HEADERS} ${SCRIPT_HEADERS} ${CRYPTO_HEADERS} ${CONSENSUS_HEADERS} ${PRIMITIVE_HEADERS} ${SOURCES} ${ZMQ_SOURCES} ${COMMON_A} ${UTIL_A} ${SERVER_A} ${CONSENSUS} ${CONSENSUS_CRYPTO})
set_property(TARGET coin PROPERTY CXX_STANDARD 11)

add_subdirectory(src/libzerocoin)
add_subdirectory(src)

#	add_definitions("-DUSE_FIELD_5X52")
#	add_definitions("-DUSE_FIELD_5X52_ASM")

set(SECP256
	./src/secp256k1/src/secp256k1.c
	)
add_library(secp256k1 ${SECP256})

link_directories(${BDB_ROOT_DIR}/lib)

add_executable(devault-cli ${CMAKE_CURRENT_SOURCE_DIR}/src/devault-cli.cpp)
add_executable(devault-tx ${CMAKE_CURRENT_SOURCE_DIR}/src/devault-tx.cpp)
add_executable(devaultd ${CMAKE_CURRENT_SOURCE_DIR}/src/devaultd.cpp)

target_link_libraries(devault-cli coin ${BDBXX_LIBRARY} ${BDB_LIBRARY} secp256k1 zerocoin gmp leveldb ${OPENSSL_LIBRARIES} ${Boost_LIBRARIES} ${ANL} miniupnpc event event_pthreads gmp pthread )
target_link_libraries(devault-tx coin secp256k1 coin leveldb zerocoin ${OPENSSL_LIBRARIES} ${Boost_LIBRARIES} ${ANL} gmp pthread)

target_link_libraries(devaultd coin ${BDBXX_LIBRARY} ${BDB_LIBRARY} secp256k1 zerocoin leveldb ${OPENSSL_LIBRARIES} ${Boost_LIBRARIES} ${ANL} curl unbound gmp zmq miniupnpc event event_pthreads pthread )

add_subdirectory(src/qt)

#---------------------------------------------------------------------------------------------------------
# Create a target in Xcode to setup coverage script
#---------------------------------------------------------------------------------------------------------
if ($ENV{COVERAGE})
    add_custom_target(xcov COMMAND ./XcodeCoverage/exportenv.sh )
endif()

#add_subdirectory(src/test)
  


