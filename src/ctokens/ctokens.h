// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef NAVCOIN_CTOKENS_H
#define NAVCOIN_CTOKENS_H

#include <blsct/scalar.h>
#include <bls.hpp>

#include <serialize.h>
#include <uint256.h>
#include <amount.h>

class TokenInfo {
public:
    int nVersion;
    std::string sName;
    CAmount totalSupply;
    CAmount currentSupply;
    bool canMint;
    bool fDirty;

    TokenInfo() {
        SetNull();
    }

    TokenInfo(const std::string& name, const CAmount& totalSupply_) : nVersion(0), sName(name), totalSupply(totalSupply_), canMint(true), currentSupply(0), fDirty(false) {};

    void SetNull() {
        nVersion = 0;
        sName = "";
        totalSupply = 0;
        currentSupply = 0;
        canMint = false;
        fDirty = false;
    }

    bool IsNull() const {
        return nVersion == 0 &&
        sName == "" &&
        totalSupply == 0 &&
        currentSupply == 0 &&
        canMint == false &&
        fDirty == false;
    }

    bool operator==(const TokenInfo& other) {
        return (nVersion == other.nVersion &&
                sName == other.sName &&
                totalSupply == other.totalSupply &&
                currentSupply == other.currentSupply &&
                canMint == other.canMint);
    }

    void swap(TokenInfo &to) {
        std::swap(to.nVersion, nVersion);
        std::swap(to.sName, sName);
        std::swap(to.totalSupply, totalSupply);
        std::swap(to.currentSupply, currentSupply);
        std::swap(to.canMint, canMint);
        std::swap(to.fDirty, fDirty);
    }

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(this->nVersion);

        if (this->nVersion == 0)
        {
            READWRITE(sName);
            READWRITE(totalSupply);
            READWRITE(currentSupply);
            READWRITE(canMint);
        }
    }
};

typedef std::pair<bls::G1Element, TokenInfo> Token;
typedef std::map<bls::G1Element, TokenInfo> TokenMap;

#endif
