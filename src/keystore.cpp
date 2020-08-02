// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <keystore.h>

#include <key.h>
#include <pubkey.h>
#include <util.h>

bool CKeyStore::AddKey(const CKey &key) {
    return AddKeyPubKey(key, key.GetPubKey());
}

bool CBasicKeyStore::AddBLSCTBlindingKey(const blsctKey &key) {
    return AddBLSCTBlindingKeyPubKey(key, blsctPublicKey(key.GetPublicKey()));
}

bool CBasicKeyStore::GetPubKey(const CKeyID &address, CPubKey &vchPubKeyOut) const
{
    CKey key;
    if (!GetKey(address, key)) {
        LOCK(cs_KeyStore);
        WatchKeyMap::const_iterator it = mapWatchKeys.find(address);
        if (it != mapWatchKeys.end()) {
            vchPubKeyOut = it->second;
            return true;
        }
        return false;
    }
    vchPubKeyOut = key.GetPubKey();
    return true;
}

bool CBasicKeyStore::AddKeyPubKey(const CKey& key, const CPubKey &pubkey)
{
    LOCK(cs_KeyStore);
    mapKeys[pubkey.GetID()] = key;
    return true;
}

bool CBasicKeyStore::AddBLSCTBlindingKeyPubKey(const blsctKey& key, const blsctPublicKey &pubkey)
{
    LOCK(cs_KeyStore);
    mapBLSCTBlindingKeys[pubkey] = key;
    return true;
}

bool CBasicKeyStore::AddBLSCTSubAddress(const CKeyID &hashId, const std::pair<uint64_t, uint64_t>& index)
{
    LOCK(cs_KeyStore);
    mapBLSCTSubAddresses[hashId] = index;

    return true;
}

bool CBasicKeyStore::GetBLSCTHashId(const Point& outputKey, const Point& spendingKey, CKeyID& hashId) const
{
    if(!privateBlsViewKey.IsValid())
        return false;

    try
    {
        // D' = P - Hs(a*R)*G
        Point dh = bls::PrivateKey::FromBN(Scalar(Point(bls::BLS::DHKeyExchange(privateBlsViewKey.GetKey(), bls::PublicKey::FromBytes(outputKey.GetVch().data()))).Hash(0)).bn).GetPublicKey();
        Point D_prime = spendingKey - dh;
        std::cout << strprintf("%s: dh: %s\n", __func__, HexStr(bls::BLS::DHKeyExchange(privateBlsViewKey.GetKey(), bls::PublicKey::FromBytes(outputKey.GetVch().data())).Serialize()));
        std::cout << strprintf("%s: sk: %s\n", __func__, HexStr(bls::PublicKey::FromBytes(spendingKey.GetVch().data()).Serialize()));
        std::cout << strprintf("%s: ok: %s\n", __func__, HexStr(outputKey.GetVch()));
        hashId = blsctPublicKey(D_prime.GetVch()).GetID();
    }
    catch(...)
    {
        return false;
    }

    return true;
}

bool CBasicKeyStore::GetBLSCTSubAddressPublicKeys(const std::pair<uint64_t, uint64_t>& index, blsctDoublePublicKey& pk) const
{
    if(!privateBlsViewKey.IsValid())
        return false;

    if(!publicBlsKey.IsValid())
        return false;

    CHashWriter string(SER_GETHASH, 0);

    string << std::vector<unsigned char>(subAddressHeader.begin(), subAddressHeader.end());
    string << privateBlsViewKey;
    string << index.first;
    string << index.second;

    try
    {
        // m = Hs(a || i)
        // M = m*G
        // D = B + M
        // C = a*D
        Scalar m = string.GetHash();
        std::vector<bls::PublicKey> keys;
        bls::PublicKey M = bls::PrivateKey::FromBN(m.bn).GetPublicKey();
        keys.push_back(M);
        keys.push_back(publicBlsKey.GetSpendKey());
        bls::PublicKey D = bls::PublicKey::AggregateInsecure(keys);
        bls::PublicKey C = bls::BLS::DHKeyExchange(privateBlsViewKey.GetKey(), D);
        pk = blsctDoublePublicKey(C, D);
    }
    catch(...)
    {
        return false;
    }

    return true;
}


bool CBasicKeyStore::AddCScript(const CScript& redeemScript)
{
    if (redeemScript.size() > MAX_SCRIPT_ELEMENT_SIZE)
        return error("CBasicKeyStore::AddCScript(): redeemScripts > %i bytes are invalid", MAX_SCRIPT_ELEMENT_SIZE);

    LOCK(cs_KeyStore);
    mapScripts[CScriptID(redeemScript)] = redeemScript;
    return true;
}

bool CBasicKeyStore::HaveCScript(const CScriptID& hash) const
{
    LOCK(cs_KeyStore);
    return mapScripts.count(hash) > 0;
}

bool CBasicKeyStore::GetCScript(const CScriptID &hash, CScript& redeemScriptOut) const
{
    LOCK(cs_KeyStore);
    ScriptMap::const_iterator mi = mapScripts.find(hash);
    if (mi != mapScripts.end())
    {
        redeemScriptOut = (*mi).second;
        return true;
    }
    return false;
}

static bool ExtractPubKey(const CScript &dest, CPubKey& pubKeyOut)
{
    //TODO: Use Solver to extract this?
    CScript::const_iterator pc = dest.begin();
    opcodetype opcode;
    std::vector<unsigned char> vch;
    if (!dest.GetOp(pc, opcode, vch) || vch.size() < 33 || vch.size() > 65)
        return false;
    pubKeyOut = CPubKey(vch);
    if (!pubKeyOut.IsFullyValid())
        return false;
    if (!dest.GetOp(pc, opcode, vch) || opcode != OP_CHECKSIG || dest.GetOp(pc, opcode, vch))
        return false;
    return true;
}

bool CBasicKeyStore::AddWatchOnly(const CScript &dest)
{
    LOCK(cs_KeyStore);
    setWatchOnly.insert(dest);
    CPubKey pubKey;
    if (ExtractPubKey(dest, pubKey))
        mapWatchKeys[pubKey.GetID()] = pubKey;
    return true;
}

bool CBasicKeyStore::RemoveWatchOnly(const CScript &dest)
{
    LOCK(cs_KeyStore);
    setWatchOnly.erase(dest);
    CPubKey pubKey;
    if (ExtractPubKey(dest, pubKey))
        mapWatchKeys.erase(pubKey.GetID());
    return true;
}

bool CBasicKeyStore::HaveWatchOnly(const CScript &dest) const
{
    LOCK(cs_KeyStore);
    return setWatchOnly.count(dest) > 0;
}

bool CBasicKeyStore::HaveWatchOnly() const
{
    LOCK(cs_KeyStore);
    return (!setWatchOnly.empty());
}
