// Copyright (c) 2020 The NavCoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef KEY_H
#define KEY_H

#ifdef _WIN32
/* Avoid redefinition warning. */
#undef ERROR
#undef WSIZE
#undef DOUBLE
#endif

#include <bls.hpp>
#include <blsct/types.h>
#include <hash.h>
#include <key.h>
#include <serialize.h>
#include <streams.h>
#include <uint256.h>
#include <version.h>

static const std::string subAddressHeader = "SubAddress\0";

class blsctDoublePublicKey
{
public:
    blsctDoublePublicKey() { vk.clear(); sk.clear(); }
    blsctDoublePublicKey(const bls::PublicKey& vk_, const bls::PublicKey& sk_) : vk(vk_.Serialize()), sk(sk_.Serialize()) {}
    blsctDoublePublicKey(const std::vector<uint8_t>& vk_, const std::vector<uint8_t>& sk_) : vk(vk_), sk(sk_) {}

    unsigned int GetSerializeSize(int nType=0, int nVersion=PROTOCOL_VERSION) const
    {
        return ::GetSerializeSize(vk, nType, nVersion) + ::GetSerializeSize(sk, nType, nVersion);
    }

    template<typename Stream>
    void Serialize(Stream& s, int nType=0, int nVersion=PROTOCOL_VERSION) const
    {
        ::Serialize(s, vk, nType, nVersion);
        ::Serialize(s, sk, nType, nVersion);
    }

    template<typename Stream>
    void Unserialize(Stream& s, int nType=0, int nVersion=PROTOCOL_VERSION)
    {
        ::Unserialize(s, vk, nType, nVersion);
        ::Unserialize(s, sk, nType, nVersion);
    }

    uint256 GetHash() const {
        CHashWriter ss(SER_GETHASH, 0);
        ss << vk;
        ss << sk;
        return ss.GetHash();
    }

    CKeyID GetID() const
    {
        return CKeyID(Hash160(sk.data(), sk.data() + sk.size()));
    }

    bls::PublicKey GetViewKey() const {
        return bls::PublicKey::FromBytes(&vk.front());
    }

    bls::PublicKey GetSpendKey() const {
        return bls::PublicKey::FromBytes(&sk.front());
    }

    bool operator<(const blsctDoublePublicKey& rhs) const {
        Point l, r;
        l = bls::PublicKey::FromBytes(&vk.front());
        r = bls::PublicKey::FromBytes(&(rhs.vk).front());
        return g1_cmp(l.g1, r.g1);
    }

    bool operator==(const blsctDoublePublicKey& rhs) const {
        return vk==rhs.vk && sk==rhs.sk;
    }

    bool IsValid() const {
        return vk.size() > 0 && sk.size() > 0;
    }

    std::vector<uint8_t> GetVkVch() const {
        return vk;
    }

    std::vector<uint8_t> GetSkVch() const {
        return sk;
    }

protected:
    std::vector<uint8_t> vk;
    std::vector<uint8_t> sk;
};

class blsctPublicKey
{
public:
    blsctPublicKey() { vk.clear(); }
    blsctPublicKey(const bls::PublicKey& vk_) : vk(vk_.Serialize()) {}
    blsctPublicKey(const std::vector<uint8_t>& vk_) : vk(vk_) {}

    unsigned int GetSerializeSize(int nType=0, int nVersion=PROTOCOL_VERSION) const
    {
        return ::GetSerializeSize(vk, nType, nVersion);
    }

    template<typename Stream>
    void Serialize(Stream& s, int nType=0, int nVersion=PROTOCOL_VERSION) const
    {
        ::Serialize(s, vk, nType, nVersion);
    }

    template<typename Stream>
    void Unserialize(Stream& s, int nType=0, int nVersion=PROTOCOL_VERSION)
    {
        ::Unserialize(s, vk, nType, nVersion);
    }

    uint256 GetHash() const {
        CHashWriter ss(SER_GETHASH, 0);
        ss << vk;
        return ss.GetHash();
    }

    CKeyID GetID() const
    {
        return CKeyID(Hash160(vk.data(), vk.data() + vk.size()));
    }

    bls::PublicKey GetPublicKey() const {
        return bls::PublicKey::FromBytes(&vk.front());
    }

    bool operator<(const blsctPublicKey& rhs) const {
        Point l, r;
        l = bls::PublicKey::FromBytes(&vk.front());
        r = bls::PublicKey::FromBytes(&(rhs.vk).front());
        return g1_cmp(l.g1, r.g1);
    }

    bool operator==(const blsctPublicKey& rhs) const {
        return vk==rhs.vk;
    }

    bool IsValid() const {
        return vk.size() > 0;
    }

    std::vector<uint8_t> GetVch() const {
        return vk;
    }

protected:
    std::vector<uint8_t> vk;
};

class blsctKey
{
private:
    CPrivKey k;

public:
    blsctKey() { k.clear(); }

    blsctKey(bls::PrivateKey k_) {
        k.resize(bls::PrivateKey::PRIVATE_KEY_SIZE);
        std::vector<uint8_t> v = k_.Serialize();
        memcpy(k.data(), &v.front(), k.size());
    }

    blsctKey(CPrivKey k_) {
        k.resize(bls::PrivateKey::PRIVATE_KEY_SIZE);
        memcpy(k.data(), &k_.front(), k.size());
    }

    unsigned int GetSerializeSize(int nType=0, int nVersion=PROTOCOL_VERSION) const
    {
        return ::GetSerializeSize(k, nType, nVersion);
    }

    template<typename Stream>
    void Serialize(Stream& s, int nType=0, int nVersion=PROTOCOL_VERSION) const
    {
        ::Serialize(s, k, nType, nVersion);
    }

    template<typename Stream>
    void Unserialize(Stream& s, int nType=0, int nVersion=PROTOCOL_VERSION)
    {
        ::Unserialize(s, k, nType, nVersion);
    }

    bool operator<(const blsctKey& rhs) const {
        Scalar l, r;
        l = bls::PrivateKey::FromBytes(&k.front());
        r = bls::PrivateKey::FromBytes(&(rhs.k).front());
        return bn_cmp(l.bn, r.bn);
    }

    bool operator==(const blsctKey& rhs) const {
        return k==rhs.k;
    }

    bls::PublicKey GetPublicKey() const {
        return bls::PrivateKey::FromBytes(&k.front()).GetPublicKey();
    }

    bls::PrivateKey GetKey() const {
        return bls::PrivateKey::FromBytes(&k.front());
    }

    Scalar GetScalar() const {
        return Scalar(GetKey());
    }

    bool IsValid() const {
        return k.size() > 0;
    }

    void SetToZero() {
        k.clear();
    }

    friend class CCryptoKeyStore;
    friend class CBasicKeyStore;
};

class blsctExtendedKey
{
private:
    std::vector<uint8_t> k;

public:
    blsctExtendedKey() { k.clear(); }

    blsctExtendedKey(bls::ExtendedPrivateKey k_) {
        k.resize(bls::ExtendedPrivateKey::EXTENDED_PRIVATE_KEY_SIZE);
        std::vector<uint8_t> v = k_.Serialize();
        memcpy(k.data(), &v.front(), k.size());
    }

    blsctExtendedKey(std::vector<uint8_t> k_) {
        k.resize(bls::ExtendedPrivateKey::EXTENDED_PRIVATE_KEY_SIZE);
        memcpy(k.data(), &k_.front(), k.size());
    }

    unsigned int GetSerializeSize(int nType=0, int nVersion=PROTOCOL_VERSION) const
    {
        return ::GetSerializeSize(k, nType, nVersion);
    }

    template<typename Stream>
    void Serialize(Stream& s, int nType=0, int nVersion=PROTOCOL_VERSION) const
    {
        ::Serialize(s, k, nType, nVersion);
    }

    template<typename Stream>
    void Unserialize(Stream& s, int nType=0, int nVersion=PROTOCOL_VERSION)
    {
        ::Unserialize(s, k, nType, nVersion);
    }

    bool operator<(const blsctExtendedKey& rhs) const {
        Scalar l, r;
        l = bls::ExtendedPrivateKey::FromBytes(&k.front()).GetPrivateKey();
        r = bls::ExtendedPrivateKey::FromBytes(&(rhs.k).front()).GetPrivateKey();
        return bn_cmp(l.bn, r.bn);
    }

    bool operator==(const blsctExtendedKey& rhs) const {
        return k==rhs.k;
    }

    bls::ExtendedPrivateKey PrivateChild(uint32_t i) const {
        return bls::ExtendedPrivateKey::FromBytes(&k.front()).PrivateChild(i);
    }

    bls::ExtendedPublicKey PublicChild(uint32_t i) const {
        return bls::ExtendedPublicKey::FromBytes(&k.front()).PublicChild(i);
    }

    bls::PublicKey GetPublicKey() const {
        return bls::ExtendedPrivateKey::FromBytes(&k.front()).GetPublicKey();
    }

    bls::PrivateKey GetPrivateKey() const {
        return bls::ExtendedPrivateKey::FromBytes(&k.front()).GetPrivateKey();
    }

    bool IsValid() const {
        return k.size() > 0;
    }

    void SetToZero() {
        k.clear();
    }

    friend class CCryptoKeyStore;
};

#endif // KEY_H
