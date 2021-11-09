// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef NAVCOIN_CTOKENS_H
#define NAVCOIN_CTOKENS_H

#include <consensus/programs.h>
#include <blsct/scalar.h>
#include <bls.hpp>

#include <uint256.h>
#include <amount.h>

class TokenInfo {
public:
    bls::G1Element key;
    int16_t nVersion;
    std::string sName;
    std::string sDesc;
    CAmount totalSupply;
    CAmount currentSupply;
    std::map<uint64_t, std::string> mapMetadata;
    bool canMint;
    bool fDirty;

    TokenInfo() {
        SetNull();
    }

    TokenInfo(const bls::G1Element& key_, const std::string& name, const std::string& desc, const CAmount& totalSupply_=-1, const CAmount& nVersion_=0) : key(key_), nVersion(nVersion_), sName(name), sDesc(desc), canMint(true), currentSupply(0), fDirty(false) {
        if (!MoneyRange(totalSupply_))
            totalSupply = MAX_MONEY;
        else
            totalSupply = totalSupply_;
    };

    std::vector<unsigned char> GetCreateProgram() {
        Predicate program(CREATE_TOKEN);
        program.Push(key);
        program.Push(nVersion);
        program.Push(sName);
        program.Push(sDesc);
        program.Push(totalSupply);
        return program.GetVchData();
    }

    std::vector<unsigned char> GetMintProgram(const CAmount& amount, const bls::G1Element& key, const std::vector<unsigned char>& vData=std::vector<unsigned char>()) {
        if (nVersion == 0 && !IncreaseSupply(amount))
            return std::vector<unsigned char>();

        Predicate program(MINT);
        program.Push(amount);
        program.Push(key);
        program.Push(vData);
        return program.GetVchData();
    }

    std::vector<unsigned char> GetStopMintingProgram(const bls::G1Element& key) {
        if (nVersion != 0)
            return std::vector<unsigned char>();

        if (!StopMinting())
            return std::vector<unsigned char>();

        Predicate program(STOP_MINT);
        program.Push(key);
        return program.GetVchData();
    }

    std::vector<unsigned char> GetBurnProgram(const uint64_t& amount, const std::vector<unsigned char>& vData=std::vector<unsigned char>()) {
        if (nVersion != 0)
            return std::vector<unsigned char>();

        if (!DecreaseSupply(amount))
            return std::vector<unsigned char>();

        Predicate program(BURN);
        program.Push(amount);
        program.Push(vData);
        return program.GetVchData();
    }

    void SetNull() {
        key = bls::G1Element();
        nVersion = 0;
        sName = "";
        sDesc = "";
        totalSupply = 0;
        currentSupply = 0;
        canMint = false;
        fDirty = false;
        mapMetadata.clear();
    }

    bool IsNull() const {
        return key == bls::G1Element() &&
                mapMetadata.size() == 0 &&
                nVersion == 0 &&
                sName == "" &&
                sDesc == "" &&
                totalSupply == 0 &&
                currentSupply == 0 &&
                canMint == false &&
                fDirty == false;
    }

    bool CanMint() const {
        return canMint;
    }

    bool operator==(const TokenInfo& other) {
        return (key == other.key &&
                nVersion == other.nVersion &&
                sName == other.sName &&
                sDesc == other.sDesc &&
                totalSupply == other.totalSupply &&
                currentSupply == other.currentSupply &&
                canMint == other.canMint &&
                mapMetadata == other.mapMetadata);
    }

    void swap(TokenInfo &to) {
        std::swap(to.nVersion, nVersion);
        std::swap(to.sName, sName);
        std::swap(to.sDesc, sDesc);
        std::swap(to.totalSupply, totalSupply);
        std::swap(to.currentSupply, currentSupply);
        std::swap(to.mapMetadata, mapMetadata);
        std::swap(to.canMint, canMint);
        std::swap(to.fDirty, fDirty);
        std::swap(to.key, key);
    }

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(this->nVersion);
        READWRITE(key);
        READWRITE(sName);
        READWRITE(sDesc);

        if (this->nVersion == 0)
        {
            READWRITE(totalSupply);
            READWRITE(currentSupply);
            READWRITE(canMint);
        }
        else if (this->nVersion == 1)
        {
            READWRITE(totalSupply);
            READWRITE(mapMetadata);
        }
    }

    bool IncreaseSupply(CAmount amount) {
        if (!CanMint())
            return false;

        if (!MoneyRange(amount))
            return false;

        if (currentSupply + amount > totalSupply)
            return false;

        if (!MoneyRange(currentSupply + amount))
            return false;

        currentSupply += amount;

        return true;
    }

    bool DecreaseSupply(CAmount amount) {
        if (!MoneyRange(amount))
            return false;

        if (!MoneyRange(currentSupply - amount))
            return false;

        currentSupply -= amount;

        return true;
    }

    uint256 GetId() {
        return SerializeHash(key);
    }

    bool StopMinting() {
        if (!CanMint())
            return false;
        canMint = false;
        return true;
    }
};

typedef std::pair<uint256, TokenInfo> Token;
typedef std::map<uint256, TokenInfo> TokenMap;

#endif
