// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Copyright (c) 2021 The Navcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef NAVCOIN_NAME_RECORD_NAME_H
#define NAVCOIN_NAME_RECORD_NAME_H

#include <uint256.h>

class NameRecordNameValue {
public:
    std::string domain;
    std::string subdomain;

    NameRecordNameValue() {
        SetNull();
    }

    NameRecordNameValue(const std::string domain_, const std::string subdomain_) {
        domain = domain_;
        subdomain = subdomain_;
    };

    void SetNull() {
        domain = "";
        subdomain = "";
    }

    bool IsNull() const {
        return domain == "";
    }

    bool operator==(const NameRecordNameValue& other) {
        return (domain == other.domain && subdomain == other.subdomain);
    }

    void swap(NameRecordNameValue &to) {
        std::swap(to.domain, domain);
        std::swap(to.subdomain, subdomain);
    }

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(domain);
        READWRITE(subdomain);
    }
};

typedef std::map<uint256, NameRecordNameValue> NameRecordNameMap;
typedef std::pair<uint256, NameRecordNameValue> NameRecordName;


#endif // NAVCOIN_NAME_RECORD_NAME_H
