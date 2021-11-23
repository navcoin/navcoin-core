// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Copyright (c) 2021 The Navcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef NAVCOIN_NAME_DATA_H
#define NAVCOIN_NAME_DATA_H

#include <uint256.h>
#include <hash.h>

class NameDataKey {
public:
    uint256 id;
    uint64_t height;
    uint64_t key;

    NameDataKey() {
        SetNull();
    }

    NameDataKey(const std::string& name_, const uint64_t& height_, const std::string& key_ = "") :  id(SerializeHash(name_)), height(height_), key(key_ != "" ? SerializeHash(key_).GetCheapHash() : 0) {
    };

    NameDataKey(const uint256& id_, const uint64_t& height_, const std::string& key_ = "") :  id(id_), height(height_), key(key_ != "" ? SerializeHash(key_).GetCheapHash() : 0) {
    };

    void SetNull() {
        height = 0;
        key = 0;
        id = uint256();
    }

    bool IsNull() const {
        return height == 0 && id == uint256() && key == 0;
    }

    bool operator==(const NameDataKey& other) const {
        return (height == other.height && id == other.id && key == other.key);
    }

    void swap(NameDataKey &to) {
        std::swap(to.height, height);
        std::swap(to.id, id);
        std::swap(to.key, key);
    }

    size_t GetSerializeSize(int nType, int nVersion) const {
        return 32 + 8 + 8;
    }
    template<typename Stream>
    void Serialize(Stream& s, int nType, int nVersion) const {
        id.Serialize(s, nType, nVersion);
        ser_writedata32be(s, height);
        if (key != 0)
            ser_writedata32be(s, key);
    }
    template<typename Stream>
    void Unserialize(Stream& s, int nType, int nVersion) {
        id.Unserialize(s, nType, nVersion);
        height = ser_readdata32be(s);
        key = ser_readdata32be(s);
    }

    bool operator<(const NameDataKey& b) const {
        if (id == b.id) {
            if (height == b.height) {
                return key < b.key;
            }
            return height < b.height;
        } else {
            return id < b.id;
        }
    }
};

class NameDataValue {
public:
    std::string subdomain;
    std::string key;
    std::string value;

    NameDataValue() {
        SetNull();
    }

    NameDataValue(const std::string& key_, const std::string& value_, const std::string& subdomain_="") :  key(key_), value(value_), subdomain(subdomain_) {
    };

    void SetNull() {
        key = "";
        value = "";
        subdomain = "";
    }

    bool IsNull() const {
        return key == "" && value == "" && subdomain == "";
    }

    bool operator==(const NameDataValue& other) const {
        return (key == other.key && value == other.value && subdomain == other.subdomain);
    }

    void swap(NameDataValue &to) {
        std::swap(to.key, key);
        std::swap(to.subdomain, subdomain);
        std::swap(to.value, value);
    }

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(key);
        READWRITE(value);
        READWRITE(subdomain);
    }
};

typedef std::pair<uint64_t, NameDataValue> NameDataEntry;
typedef std::vector<NameDataEntry> NameDataValues;
typedef std::map<uint256, NameDataValues> NameDataMap;

#endif
