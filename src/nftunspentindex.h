// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef NAVCOIN_NFTUNSPENTINDEX_H
#define NAVCOIN_NFTUNSPENTINDEX_H

#include <ctokens/tokenid.h>

struct CNftUnspentIndexKey {
    TokenId tokenId;
    uint32_t blockHeight;

    size_t GetSerializeSize(int nType, int nVersion) const {
        return 32 + 8 + 4;
    }
    template<typename Stream>
    void Serialize(Stream& s, int nType, int nVersion) const {
        tokenId.Serialize(s, nType, nVersion);
        ser_writedata32be(s, blockHeight);
    }
    template<typename Stream>
    void Unserialize(Stream& s, int nType, int nVersion) {
        tokenId.Unserialize(s, nType, nVersion);
        blockHeight = ser_readdata32be(s);
    }

    CNftUnspentIndexKey(TokenId t, int h) {
        tokenId = t;
        blockHeight = h;
    }

    CNftUnspentIndexKey() {
        SetNull();
    }

    void SetNull() {
        tokenId.SetNull();
        blockHeight = 0;
    }

    bool IsNull() const {
        return tokenId.IsNull();
    }
};

struct CNftUnspentIndexValue {
    uint256 hash;
    std::vector<uint8_t> spendingKey;
    uint32_t n;

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(hash);
        READWRITE(spendingKey);
        READWRITE(n);
    }

    CNftUnspentIndexValue(uint256 _hash, std::vector<uint8_t> _spendingKey, uint32_t _n) {
        hash = _hash;
        spendingKey = _spendingKey;
        n = _n;
    }

    CNftUnspentIndexValue() {
        SetNull();
    }

    void SetNull() {
        hash.SetNull();
        spendingKey.clear();
        n = (uint32_t) -1;
    }

    bool IsNull() const {
        return hash.IsNull();
    }
};

struct CNftUnspentIndexKeyCompare
{
    bool operator()(const CNftUnspentIndexKey& a, const CNftUnspentIndexKey& b) const {
        if (a.tokenId == b.tokenId) {
            return a.blockHeight < b.blockHeight;
        } else {
            return a.tokenId < b.tokenId;
        }
    }
};

#endif // NAVCOIN_NFTUNSPENTINDEX_H
