// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef NAVCOIN_NFTUNSPENTINDEX_H
#define NAVCOIN_NFTUNSPENTINDEX_H

#include <uint256.h>

struct CNftUnspentIndexKey {
    uint256 tokenId;
    int blockHeight;

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(tokenId);
        READWRITE(blockHeight);
    }

    CNftUnspentIndexKey(uint256 t, int h) {
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
};

struct CNftUnspentIndexValue {
    CTxOut tx;

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(tx);
    }

    CNftUnspentIndexValue(CTxOut txout) {
        tx = txout;
    }

    CNftUnspentIndexValue() {
        SetNull();
    }

    void SetNull() {
        tx.SetNull();
    }

    bool IsNull() const {
        return tx.IsNull();
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
