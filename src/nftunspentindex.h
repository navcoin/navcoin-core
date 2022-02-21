// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef NAVCOIN_NFTUNSPENTINDEX_H
#define NAVCOIN_NFTUNSPENTINDEX_H

#include <uint256.h>

struct CNftUnspentIndexKey {
    uint256 tokenId;

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(tokenId);
    }

    CNftUnspentIndexKey(uint256 t) {
        tokenId = t;
    }

    CNftUnspentIndexKey() {
        SetNull();
    }

    void SetNull() {
        tokenId.SetNull();
    }
};

struct CNftUnspentIndexValue {
    uint256 tokenId;
    uint256 txHash;

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(tokenId);
        READWRITE(txHash);
    }

    CNftUnspentIndexValue(uint256 t, uint256 h) {
        tokenId = t;
        txHash = h;
    }

    CNftUnspentIndexValue() {
        SetNull();
    }

    void SetNull() {
        tokenId.SetNull();
        txHash.SetNull();
    }

    bool IsNull() const {
        return tokenId.IsNull();
    }
};

#endif // NAVCOIN_NFTUNSPENTINDEX_H
