// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef NAVCOIN_KEYSTORE_H
#define NAVCOIN_KEYSTORE_H

#include <blsct/key.h>
#include <key.h>
#include <pubkey.h>
#include <script/script.h>
#include <script/standard.h>
#include <sync.h>

#include <boost/signals2/signal.hpp>
#include <boost/variant.hpp>

/** A virtual base class for key stores */
class CKeyStore
{
protected:
    mutable CCriticalSection cs_KeyStore;

public:
    virtual ~CKeyStore() {}

    //! Add a key to the store.
    virtual bool AddKeyPubKey(const CKey &key, const CPubKey &pubkey) =0;
    virtual bool AddKey(const CKey &key);
    virtual bool AddBLSCTKeyPubKey(const blsctKey& key, const blsctPublicKey &pubkey) =0;
    virtual bool AddBLSCTKey(const blsctKey &key) = 0;

    //! Check whether a key corresponding to a given address is present in the store.
    virtual bool HaveKey(const CKeyID &address) const =0;
    virtual bool GetKey(const CKeyID &address, CKey& keyOut) const =0;
    virtual void GetKeys(std::set<CKeyID> &setAddress) const =0;
    virtual bool GetPubKey(const CKeyID &address, CPubKey& vchPubKeyOut) const =0;

    virtual bool HaveBLSCTKey(const blsctPublicKey &pk) const =0;
    virtual bool GetBLSCTKey(const blsctPublicKey &pk, blsctKey &k) const =0;

    //! Support for BIP 0013 : see https://github.com/navcoin/bips/blob/master/bip-0013.mediawiki
    virtual bool AddCScript(const CScript& redeemScript) =0;
    virtual bool HaveCScript(const CScriptID &hash) const =0;
    virtual bool GetCScript(const CScriptID &hash, CScript& redeemScriptOut) const =0;

    //! Support for Watch-only addresses
    virtual bool AddWatchOnly(const CScript &dest) =0;
    virtual bool RemoveWatchOnly(const CScript &dest) =0;
    virtual bool HaveWatchOnly(const CScript &dest) const =0;
    virtual bool HaveWatchOnly() const =0;

    //! Support for BLSCT address
    virtual bool GetBLSCTViewKey(blsctKey& k) const =0;
    virtual bool GetBLSCTSpendKey(blsctKey& k) const =0;
    virtual bool GetBLSCTDoublePublicKey(blsctDoublePublicKey& k) const =0;
    virtual bool GetBLSCTBlindingKey(blsctExtendedKey& k) const =0;
    virtual bool SetBLSCTViewKey(const blsctKey& v) =0;
    virtual bool SetBLSCTSpendKey(const blsctKey& v) =0;
    virtual bool SetBLSCTDoublePublicKey(const blsctDoublePublicKey& k) =0;
    virtual bool SetBLSCTBlindingKey(const blsctExtendedKey& k) =0;
};

typedef std::map<CKeyID, CKey> KeyMap;
typedef std::map<blsctPublicKey, blsctKey> BLSCTKeyMap;
typedef std::map<CKeyID, CPubKey> WatchKeyMap;
typedef std::map<CScriptID, CScript > ScriptMap;
typedef std::set<CScript> WatchOnlySet;

/** Basic key store, that keeps keys in an address->secret map */
class CBasicKeyStore : public CKeyStore
{
protected:
    KeyMap mapKeys;
    BLSCTKeyMap mapBLSCTKeys;
    WatchKeyMap mapWatchKeys;
    ScriptMap mapScripts;
    WatchOnlySet setWatchOnly;
    blsctKey privateBlsViewKey;
    blsctKey privateBlsSpendKey;
    blsctDoublePublicKey publicBlsKey;
    blsctExtendedKey privateBlsBlindingKey;


public:
    bool AddKeyPubKey(const CKey& key, const CPubKey &pubkey);
    bool AddBLSCTKeyPubKey(const blsctKey& key, const blsctPublicKey &pubkey);
    bool AddBLSCTKey(const blsctKey &key);
    bool GetPubKey(const CKeyID &address, CPubKey& vchPubKeyOut) const;
    bool HaveKey(const CKeyID &address) const
    {
        bool result;
        {
            LOCK(cs_KeyStore);
            result = (mapKeys.count(address) > 0);
        }
        return result;
    }

    bool HaveBLSCTKey(const blsctPublicKey &address) const
    {
        {
            LOCK(cs_KeyStore);
            for(auto& it: mapBLSCTKeys)
            {
                if (it.first == address) return true;
            }
        }
        return false;
    }

    void GetKeys(std::set<CKeyID> &setAddress) const
    {
        setAddress.clear();
        {
            LOCK(cs_KeyStore);
            KeyMap::const_iterator mi = mapKeys.begin();
            while (mi != mapKeys.end())
            {
                setAddress.insert((*mi).first);
                mi++;
            }
        }
    }

    bool GetKey(const CKeyID &address, CKey &keyOut) const
    {
        {
            LOCK(cs_KeyStore);
            KeyMap::const_iterator mi = mapKeys.find(address);
            if (mi != mapKeys.end())
            {
                keyOut = mi->second;
                return true;
            }
        }
        return false;
    }

    bool GetBLSCTKey(const blsctPublicKey &address, blsctKey &keyOut) const
    {
        {
            LOCK(cs_KeyStore);
            for(auto& mi: mapBLSCTKeys)
            {
                if (mi.first == address)
                {
                    keyOut = mi.second;
                    return true;
                }
            }
        }
        return false;
    }

    bool GetBLSCTViewKey(blsctKey& zk) const {
        if(!privateBlsViewKey.IsValid())
            return false;

        zk = privateBlsViewKey;
        return true;
    }

    bool GetBLSCTBlindingKey(blsctExtendedKey& zk) const {
        if(!privateBlsBlindingKey.IsValid())
            return false;

        zk = privateBlsBlindingKey;
        return true;
    }

    bool GetBLSCTSpendKey(blsctKey& zk) const {
        if(!privateBlsSpendKey.IsValid())
            return false;

        zk = privateBlsSpendKey;
        return true;
    }

    bool GetBLSCTDoublePublicKey(blsctDoublePublicKey& zk) const {
        if(!publicBlsKey.IsValid())
            return false;

        zk = publicBlsKey;
        return true;
    }

    bool SetBLSCTViewKey(const blsctKey& v) {
        if(!v.IsValid())
            return false;
        privateBlsViewKey = v;
        return true;
    }

    bool SetBLSCTBlindingKey(const blsctExtendedKey& v) {
        if(!v.IsValid())
            return false;
        privateBlsBlindingKey = v;
        return true;
    }

    bool SetBLSCTSpendKey(const blsctKey& s) {
        if(!s.IsValid())
            return false;
        privateBlsSpendKey = s;
        return true;
    }

    bool SetBLSCTDoublePublicKey(const blsctDoublePublicKey &k) {
        if (!k.IsValid())
            return false;
        publicBlsKey = k;
        return true;
    }

    virtual bool AddCScript(const CScript& redeemScript);
    virtual bool HaveCScript(const CScriptID &hash) const;
    virtual bool GetCScript(const CScriptID &hash, CScript& redeemScriptOut) const;

    virtual bool AddWatchOnly(const CScript &dest);
    virtual bool RemoveWatchOnly(const CScript &dest);
    virtual bool HaveWatchOnly(const CScript &dest) const;
    virtual bool HaveWatchOnly() const;
};

typedef std::vector<unsigned char, secure_allocator<unsigned char> > CKeyingMaterial;
typedef std::map<CKeyID, std::pair<CPubKey, std::vector<unsigned char> > > CryptedKeyMap;

#endif // NAVCOIN_KEYSTORE_H
