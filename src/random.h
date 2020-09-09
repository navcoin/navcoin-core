// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2016 The Bitcoin Core developers
// Copyright (c) 2019 DeVault developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "crypto/chacha20.h"
#include "crypto/common.h"
#include "uint256.h"
#include <cstdint>

// Functions to gather random data
void GetRandBytes(uint8_t *buf, int num);
void GetRandBytes(std::vector<uint8_t> &buf);
void GetStrongRandBytes(uint8_t *buf, int num);
uint64_t GetRand(uint64_t nMax = 0xffffffff);
int GetRandInt(int nMax);
uint256 GetRandHash();

/**
 * Fast randomness source. This is seeded once with secure random data, but is
 * completely deterministic and insecure after that.
 * This class is not thread-safe.
 */
class FastRandomContext {
 private:
  bool requires_seed;
  ChaCha20 rng;

  uint8_t bytebuf[64];
  int bytebuf_size = 0;

  uint64_t bitbuf;
  int bitbuf_size = 0;

  void RandomSeed();

  void FillByteBuffer() {
    if (requires_seed) { RandomSeed(); }
    rng.Output(bytebuf, sizeof(bytebuf));
    bytebuf_size = sizeof(bytebuf);
  }

  void FillBitBuffer() {
    bitbuf = rand64();
    bitbuf_size = 64;
  }

 public:
  explicit FastRandomContext(bool fDeterministic = false);

  /** Initialize with explicit seed (only for testing) */
  explicit FastRandomContext(const uint256 &seed);

  /** Generate a random 64-bit integer. */
  uint64_t rand64() {
    if (bytebuf_size < 8) { FillByteBuffer(); }
    uint64_t ret = ReadLE64(bytebuf + 64 - bytebuf_size);
    bytebuf_size -= 8;
    return ret;
  }

  /** Generate a random (bits)-bit integer. */
  uint64_t randbits(int bits) {
    if (bits == 0) {
      return 0;
    } else if (bits > 32) {
      return rand64() >> (64 - bits);
    } else {
      if (bitbuf_size < bits) { FillBitBuffer(); }
      uint64_t ret = bitbuf & (~uint64_t(0) >> (64 - bits));
      bitbuf >>= bits;
      bitbuf_size -= bits;
      return ret;
    }
  }

  /** Generate a random integer in the range [0..range). */
  uint64_t randrange(uint64_t range) {
    --range;
    int bits = CountBits(range);
    while (true) {
      uint64_t ret = randbits(bits);
      if (ret <= range) { return ret; }
    }
  }

  /** Generate random bytes. */
  std::vector<uint8_t> randbytes(size_t len);

  /** Generate a random 32-bit integer. */
  uint32_t rand32() { return randbits(32); }

  /** generate a random uint256. */
  uint256 rand256();

  /** Generate a random boolean. */
  bool randbool() { return randbits(1); }
};
