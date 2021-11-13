// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Copyright (c) 2021 The Navcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef NAVCOIN_NAME_RECORD_H
#define NAVCOIN_NAME_RECORD_H

#include <uint256.h>

class NameRecordValue {
public:
    uint64_t height;
    uint32_t nVersion;
    bool fDirty;

    NameRecordValue() {
        SetNull();
    }

    NameRecordValue(const uint64_t& height_) :  height(height_), nVersion(0) {
    };

    void SetNull() {
        height = -1;
        nVersion = -1;
        fDirty = false;
    }

    bool IsNull() const {
        return height == -1 && nVersion == -1;
    }

    bool operator==(const NameRecordValue& other) {
        return (height == other.height && nVersion == other.nVersion);
    }

    void swap(NameRecordValue &to) {
        std::swap(to.nVersion, nVersion);
        std::swap(to.height, height);
        std::swap(to.fDirty, fDirty);
    }

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(this->nVersion);
        READWRITE(height);
    }
};

typedef std::map<uint256, NameRecordValue> NameRecordMap;
typedef std::pair<uint256, NameRecordValue> NameRecord;


#endif
