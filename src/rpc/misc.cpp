// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <base58.h>
#include <blsct/key.h>
#include <clientversion.h>
#include <init.h>
#include <main.h>
#include <net.h>
#include <netbase.h>
#include <rpc/server.h>
#include <timedata.h>
#include <txmempool.h>
#include <util.h>
#include <utilstrencodings.h>
#include <utils/dns_utils.h>
#ifdef ENABLE_WALLET
#include <wallet/wallet.h>
#include <wallet/walletdb.h>
#endif

#include <stdint.h>

#include <boost/assign/list_of.hpp>

#include <univalue.h>

/**
 * @note Do not add or change anything in the information returned by this
 * method. `getinfo` exists for backwards-compatibility only. It combines
 * information from wildly different sources in the program, which is a mess,
 * and is thus planned to be deprecated eventually.
 *
 * Based on the source of the information, new information should be added to:
 * - `getblockchaininfo`,
 * - `getnetworkinfo` or
 * - `getwalletinfo`
 *
 * Or alternatively, create a specific query method for the information.
 **/
UniValue getinfo(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() != 0)
        throw runtime_error(
            "getinfo\n"
            "Returns an object containing various state info.\n"
            "\nResult:\n"
            "{\n"
            "  \"version\": xxxxx,           (numeric) the server version\n"
            "  \"protocolversion\": xxxxx,   (numeric) the protocol version\n"
            "  \"walletversion\": xxxxx,     (numeric) the wallet version\n"
            "  \"balance\": xxxxxxx,         (numeric) the total navcoin balance of the wallet\n"
            "  \"blocks\": xxxxxx,           (numeric) the current number of blocks processed in the server\n"
            "  \"ntptimeoffset\": xxxxx,     (numeric) the time offset\n"
            "  \"timeoffset\": xxxxx,        (numeric) the time offset\n"
            "  \"connections\": xxxxx,       (numeric) the number of connections\n"
            "  \"proxy\": \"host:port\",     (string, optional) the proxy used by the server\n"
            "  \"difficulty\": xxxxxx,       (numeric) the current difficulty\n"
            "  \"testnet\": true|false,      (boolean) if the server is using testnet or not\n"
            "  \"keypoololdest\": xxxxxx,    (numeric) the timestamp (seconds since GMT epoch) of the oldest pre-generated key in the key pool\n"
            "  \"keypoolsize\": xxxx,        (numeric) how many new keys are pre-generated\n"
            "  \"unlocked_until\": ttt,      (numeric) the timestamp in seconds since epoch (midnight Jan 1 1970 GMT) that the wallet is unlocked for transfers, or 0 if the wallet is locked\n"
            "  \"paytxfee\": x.xxxx,         (numeric) the transaction fee set in " + CURRENCY_UNIT + "/kB\n"
            "  \"relayfee\": x.xxxx,         (numeric) minimum relay fee for non-free transactions in " + CURRENCY_UNIT + "/kB\n"
            "  \"errors\": \"...\"           (string) any error messages\n"
            "}\n"
            "\nExamples:\n"
            + HelpExampleCli("getinfo", "")
            + HelpExampleRpc("getinfo", "")
        );

#ifdef ENABLE_WALLET
    LOCK2(cs_main, pwalletMain ? &pwalletMain->cs_wallet : nullptr);
#else
    LOCK(cs_main);
#endif

    proxyType proxy;
    GetProxy(NET_IPV4, proxy);

    UniValue obj(UniValue::VOBJ);
    obj.pushKV("version", CLIENT_VERSION);
    obj.pushKV("protocolversion", PROTOCOL_VERSION);
#ifdef ENABLE_WALLET
    if (pwalletMain) {
        obj.pushKV("walletversion",           pwalletMain->GetVersion());
        obj.pushKV("balance",                 ValueFromAmount(pwalletMain->GetBalance()));
        obj.pushKV("private_balance",         ValueFromAmount(pwalletMain->GetPrivateBalance()));
        obj.pushKV("private_balance_pending", ValueFromAmount(pwalletMain->GetPrivateBalancePending()));
        obj.pushKV("coldstaking_balance",     ValueFromAmount(pwalletMain->GetColdStakingBalance()));
        obj.pushKV("newmint",                 ValueFromAmount(pwalletMain->GetNewMint()));
        obj.pushKV("stake",                   ValueFromAmount(pwalletMain->GetStake()));
    }
#endif
    obj.pushKV("blocks",        (int)chainActive.Height());

    UniValue cf(UniValue::VOBJ);
    cf.pushKV("available",           ValueFromAmount(chainActive.Tip()->nCFSupply));
    cf.pushKV("locked",              ValueFromAmount(chainActive.Tip()->nCFLocked));

    obj.pushKV("communityfund",      cf);
    obj.pushKV("publicmoneysupply",  FormatMoney(chainActive.Tip()->nPublicMoneySupply));
    obj.pushKV("privatemoneysupply", FormatMoney(chainActive.Tip()->nPrivateMoneySupply));
    obj.pushKV("timeoffset",         GetTimeOffset());
    obj.pushKV("ntptimeoffset",      GetNtpTimeOffset());
    obj.pushKV("connections",        (int)vNodes.size());
    obj.pushKV("proxy",              (proxy.IsValid() ? proxy.proxy.ToStringIPPort() : string()));
    obj.pushKV("testnet",            Params().TestnetToBeDeprecatedFieldRPC());
#ifdef ENABLE_WALLET
    if (pwalletMain) {
        obj.pushKV("keypoololdest", pwalletMain->GetOldestKeyPoolTime());
        obj.pushKV("keypoolsize",   (int)pwalletMain->GetKeyPoolSize());
    }
    if (pwalletMain && pwalletMain->IsCrypted())
        obj.pushKV("unlocked_until", nWalletUnlockTime);
    obj.pushKV("paytxfee",      ValueFromAmount(payTxFee.GetFeePerK()));
#endif
    obj.pushKV("relayfee",      ValueFromAmount(::minRelayTxFee.GetFeePerK()));
    obj.pushKV("errors",        GetWarnings("statusbar"));
    return obj;
}

#ifdef ENABLE_WALLET
class DescribeAddressVisitor : public boost::static_visitor<UniValue>
{
public:
    UniValue operator()(const CNoDestination &dest) const { return UniValue(UniValue::VOBJ); }

    UniValue operator()(blsctDoublePublicKey dest) const { return UniValue(UniValue::VOBJ); }

    UniValue operator()(const CKeyID &keyID) const {
        UniValue obj(UniValue::VOBJ);
        CPubKey vchPubKey;
        obj.pushKV("isscript", false);
        obj.pushKV("iscoldstaking", false);
        if (pwalletMain && pwalletMain->GetPubKey(keyID, vchPubKey)) {
            obj.pushKV("pubkey", HexStr(vchPubKey));
            obj.pushKV("iscompressed", vchPubKey.IsCompressed());
        }
        return obj;
    }

    UniValue operator()(const pair<CKeyID, CKeyID> &keyID) const {
        UniValue obj(UniValue::VOBJ);
        CPubKey vchPubKey;
        obj.pushKV("isscript", false);
        obj.pushKV("iscoldstaking", true);
        if (pwalletMain && pwalletMain->GetPubKey(keyID.first, vchPubKey)) {
            obj.pushKV("stakingpubkey", HexStr(vchPubKey));
        }
        if(pwalletMain->GetPubKey(keyID.second, vchPubKey)) {
            obj.pushKV("spendingpubkey", HexStr(vchPubKey));
        }
        return obj;
    }

    UniValue operator()(const pair<CKeyID, pair<CKeyID, CKeyID>> &keyID) const {
        UniValue obj(UniValue::VOBJ);
        CPubKey vchPubKey;
        obj.pushKV("isscript", false);
        obj.pushKV("iscoldstaking", true);
        if (pwalletMain && pwalletMain->GetPubKey(keyID.first, vchPubKey)) {
            obj.pushKV("stakingpubkey", HexStr(vchPubKey));
        }
        if(pwalletMain->GetPubKey(keyID.second.first, vchPubKey)) {
            obj.pushKV("spendingpubkey", HexStr(vchPubKey));
        }
        if(pwalletMain->GetPubKey(keyID.second.second, vchPubKey)) {
            obj.pushKV("votingpubkey", HexStr(vchPubKey));
        }
        return obj;
    }

    UniValue operator()(const CScriptID &scriptID) const {
        UniValue obj(UniValue::VOBJ);
        CScript subscript;
        obj.pushKV("isscript", true);
        obj.pushKV("iscoldstaking", false);
        if (pwalletMain && pwalletMain->GetCScript(scriptID, subscript)) {
            std::vector<CTxDestination> addresses;
            txnouttype whichType;
            int nRequired;
            ExtractDestinations(subscript, whichType, addresses, nRequired);
            obj.pushKV("script", GetTxnOutputType(whichType));
            obj.pushKV("hex", HexStr(subscript.begin(), subscript.end()));
            UniValue a(UniValue::VARR);
            for(const CTxDestination& addr: addresses)
                a.push_back(CNavcoinAddress(addr).ToString());
            obj.pushKV("addresses", a);
            if (whichType == TX_MULTISIG)
                obj.pushKV("sigsrequired", nRequired);
        }
        return obj;
    }
};
#endif

UniValue validateaddress(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() != 1)
        throw runtime_error(
            "validateaddress \"navcoinaddress\"\n"
            "\nReturn information about the given navcoin address.\n"
            "\nArguments:\n"
            "1. \"navcoinaddress\"     (string, required) The navcoin address to validate. This may also be an OpenAlias address.\n"
            "\nResult:\n"
            "{\n"
            "  \"isvalid\" : true|false,       (boolean) If the address is valid or not. If not, this is the only property returned.\n"
            "  \"address\" : \"navcoinaddress\", (string) The navcoin address validated\n"
            "  \"scriptPubKey\" : \"hex\",       (string) The hex encoded scriptPubKey generated by the address\n"
            "  \"stakingaddress\" : \"navcoinaddress\", (string) The navcoin staking address part of a cold staking address\n"
            "  \"spendingaddress\" : \"navcoinaddress\", (string) The navcoin spending address part of a cold staking address\n"
            "  \"votingaddress\" : \"navcoinaddress\", (string) The navcoin voting address part of a cold staking v2 address\n"
            "  \"ismine\" : true|false,        (boolean) If the address is yours or not\n"
            "  \"isstakable\" : true|false,    (boolean) If the coins from the address are stakable or not\n"
            "  \"iswatchonly\" : true|false,   (boolean) If the address is watchonly\n"
            "  \"isscript\" : true|false,      (boolean) If the key is a script\n"
            "  \"iscoldstaking\" : true|false,        (boolean) If the address is a cold staking address or not\n"
            "  \"pubkey\" : \"publickeyhex\",    (string) The hex value of the raw public key\n"
            "  \"iscompressed\" : true|false,  (boolean) If the address is compressed\n"
            "  \"account\" : \"account\"         (string) DEPRECATED. The account associated with the address, \"\" is the default account\n"
            "  \"hdkeypath\" : \"keypath\"       (string, optional) The HD keypath if the key is HD and available\n"
            "  \"hdmasterkeyid\" : \"<hash160>\" (string, optional) The Hash160 of the HD master pubkey\n"
            "}\n"
            "\nExamples:\n"
            + HelpExampleCli("validateaddress", "\"1PSSGeFHDnKNxiEyFrD1wcEaHr9hrQDDWc\"")
            + HelpExampleRpc("validateaddress", "\"1PSSGeFHDnKNxiEyFrD1wcEaHr9hrQDDWc\"")
        );

#ifdef ENABLE_WALLET
    LOCK2(cs_main, pwalletMain ? &pwalletMain->cs_wallet : nullptr);
#else
    LOCK(cs_main);
#endif

    string address_str = params[0].get_str();

    utils::DNSResolver *DNS = nullptr;
    bool dnssec_available; bool dnssec_valid;

    if(DNS->check_address_syntax(params[0].get_str().c_str()))
    {
        std::vector<std::string> addresses = utils::dns_utils::addresses_from_url(params[0].get_str().c_str(), dnssec_available, dnssec_valid);

        if(addresses.empty())
          throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid OpenAlias address");
        else
        {

          address_str = addresses.front();

        }

    }

    CNavcoinAddress address(address_str);
    bool isValid = address.IsValid();

    UniValue ret(UniValue::VOBJ);
    ret.pushKV("isvalid", isValid);
    if (isValid)
    {
        CTxDestination dest = address.Get();
        string currentAddress = address.ToString();
        ret.pushKV("address", currentAddress);
        if(DNS->check_address_syntax(params[0].get_str().c_str()))
            ret.pushKV("dnssec", dnssec_valid);

        CScript scriptPubKey = GetScriptForDestination(dest);
        ret.pushKV("scriptPubKey", HexStr(scriptPubKey.begin(), scriptPubKey.end()));

#ifdef ENABLE_WALLET
        isminetype mine = pwalletMain ? IsMine(*pwalletMain, dest) : ISMINE_NO;
        if (address.IsColdStakingAddress(Params())) {
            CNavcoinAddress stakingAddress;
            address.GetStakingAddress(stakingAddress);
            CNavcoinAddress spendingAddress;
            address.GetSpendingAddress(spendingAddress);
            ret.pushKV("stakingaddress", stakingAddress.ToString());
            ret.pushKV("spendingaddress", spendingAddress.ToString());
        }
        else if(address.IsColdStakingv2Address(Params())) {
            CNavcoinAddress stakingAddress;
            address.GetStakingAddress(stakingAddress);
            CNavcoinAddress spendingAddress;
            address.GetSpendingAddress(spendingAddress);
            CNavcoinAddress votingAddress;
            address.GetVotingAddress(votingAddress);
            ret.pushKV("stakingaddress", stakingAddress.ToString());
            ret.pushKV("spendingaddress", spendingAddress.ToString());
            ret.pushKV("votingaddress", votingAddress.ToString());
        }
        ret.pushKV("ismine", (mine & ISMINE_SPENDABLE) ? true : false);
        ret.pushKV("isstakable", (mine & ISMINE_STAKABLE || (mine & ISMINE_SPENDABLE &&
                     !address.IsColdStakingAddress(Params()))) ? true : false);
        ret.pushKV("iswatchonly", (mine & ISMINE_WATCH_ONLY) ? true: false);
        UniValue detail = boost::apply_visitor(DescribeAddressVisitor(), dest);
        ret.pushKVs(detail);
        if (pwalletMain && pwalletMain->mapAddressBook.count(dest))
            ret.pushKV("account", pwalletMain->mapAddressBook[dest].name);
        CKeyID keyID;
        if (pwalletMain && address.GetKeyID(keyID) && pwalletMain->mapKeyMetadata.count(keyID) && !pwalletMain->mapKeyMetadata[keyID].hdKeypath.empty())
        {
            ret.pushKV("hdkeypath", pwalletMain->mapKeyMetadata[keyID].hdKeypath);
            ret.pushKV("hdmasterkeyid", pwalletMain->mapKeyMetadata[keyID].hdMasterKeyID.GetHex());
        }
#endif
    }
    return ret;
}

/**
 * Used by addmultisigaddress / createmultisig:
 */
CScript _createmultisig_redeemScript(const UniValue& params)
{
    int nRequired = params[0].get_int();
    const UniValue& keys = params[1].get_array();

    // Gather public keys
    if (nRequired < 1)
        throw runtime_error("a multisignature address must require at least one key to redeem");
    if ((int)keys.size() < nRequired)
        throw runtime_error(
            strprintf("not enough keys supplied "
                      "(got %u keys, but need at least %d to redeem)", keys.size(), nRequired));
    if (keys.size() > 16)
        throw runtime_error("Number of addresses involved in the multisignature address creation > 16\nReduce the number");
    std::vector<CPubKey> pubkeys;
    pubkeys.resize(keys.size());
    for (unsigned int i = 0; i < keys.size(); i++)
    {
        const std::string& ks = keys[i].get_str();
#ifdef ENABLE_WALLET
        // Case 1: Navcoin address and we have full public key:
        CNavcoinAddress address(ks);
        if (pwalletMain && address.IsValid())
        {
            CKeyID keyID;
            if (!address.GetKeyID(keyID))
                throw runtime_error(
                    strprintf("%s does not refer to a key",ks));
            CPubKey vchPubKey;
            if (!pwalletMain->GetPubKey(keyID, vchPubKey))
                throw runtime_error(
                    strprintf("no full public key for address %s",ks));
            if (!vchPubKey.IsFullyValid())
                throw runtime_error(" Invalid public key: "+ks);
            pubkeys[i] = vchPubKey;
        }

        // Case 2: hex public key
        else
#endif
        if (IsHex(ks))
        {
            CPubKey vchPubKey(ParseHex(ks));
            if (!vchPubKey.IsFullyValid())
                throw runtime_error(" Invalid public key: "+ks);
            pubkeys[i] = vchPubKey;
        }
        else
        {
            throw runtime_error(" Invalid public key: "+ks);
        }
    }
    CScript result = GetScriptForMultisig(nRequired, pubkeys);

    if (result.size() > MAX_SCRIPT_ELEMENT_SIZE)
        throw runtime_error(
                strprintf("redeemScript exceeds size limit: %d > %d", result.size(), MAX_SCRIPT_ELEMENT_SIZE));

    return result;
}

UniValue createmultisig(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() < 2 || params.size() > 2)
    {
        string msg = "createmultisig nrequired [\"key\",...]\n"
            "\nCreates a multi-signature address with n signature of m keys required.\n"
            "It returns a json object with the address and redeemScript.\n"

            "\nArguments:\n"
            "1. nrequired      (numeric, required) The number of required signatures out of the n keys or addresses.\n"
            "2. \"keys\"       (string, required) A json array of keys which are navcoin addresses or hex-encoded public keys\n"
            "     [\n"
            "       \"key\"    (string) navcoin address or hex-encoded public key\n"
            "       ,...\n"
            "     ]\n"

            "\nResult:\n"
            "{\n"
            "  \"address\":\"multisigaddress\",  (string) The value of the new multisig address.\n"
            "  \"redeemScript\":\"script\"       (string) The string value of the hex-encoded redemption script.\n"
            "}\n"

            "\nExamples:\n"
            "\nCreate a multisig address from 2 addresses\n"
            + HelpExampleCli("createmultisig", "2 \"[\\\"16sSauSf5pF2UkUwvKGq4qjNRzBZYqgEL5\\\",\\\"171sgjn4YtPu27adkKGrdDwzRTxnRkBfKV\\\"]\"") +
            "\nAs a json rpc call\n"
            + HelpExampleRpc("createmultisig", "2, \"[\\\"16sSauSf5pF2UkUwvKGq4qjNRzBZYqgEL5\\\",\\\"171sgjn4YtPu27adkKGrdDwzRTxnRkBfKV\\\"]\"")
        ;
        throw runtime_error(msg);
    }

    // Construct using pay-to-script-hash:
    CScript inner = _createmultisig_redeemScript(params);
    CScriptID innerID(inner);
    CNavcoinAddress address(innerID);

    UniValue result(UniValue::VOBJ);
    result.pushKV("address", address.ToString());
    result.pushKV("redeemScript", HexStr(inner.begin(), inner.end()));

    return result;
}

UniValue createwitnessaddress(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() < 1 || params.size() > 1)
    {
        string msg = "createwitnessaddress \"script\"\n"
            "\nCreates a witness address for a particular script.\n"
            "It returns a json object with the address and witness script.\n"

            "\nArguments:\n"
            "1. \"script\"       (string, required) A hex encoded script\n"

            "\nResult:\n"
            "{\n"
            "  \"address\":\"multisigaddress\",  (string) The value of the new address (P2SH of witness script).\n"
            "  \"witnessScript\":\"script\"      (string) The string value of the hex-encoded witness script.\n"
            "}\n"
        ;
        throw runtime_error(msg);
    }

    if (!IsHex(params[0].get_str())) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Script must be hex-encoded");
    }

    std::vector<unsigned char> code = ParseHex(params[0].get_str());
    CScript script(code.begin(), code.end());
    CScript witscript = GetScriptForWitness(script);
    CScriptID witscriptid(witscript);
    CNavcoinAddress address(witscriptid);

    UniValue result(UniValue::VOBJ);
    result.pushKV("address", address.ToString());
    result.pushKV("witnessScript", HexStr(witscript.begin(), witscript.end()));

    return result;
}

UniValue verifymessage(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() != 3)
        throw runtime_error(
            "verifymessage \"navcoinaddress\" \"signature\" \"message\"\n"
            "\nVerify a signed message\n"
            "\nArguments:\n"
            "1. \"navcoinaddress\"  (string, required) The navcoin address to use for the signature.\n"
            "2. \"signature\"       (string, required) The signature provided by the signer in base 64 encoding (see signmessage).\n"
            "3. \"message\"         (string, required) The message that was signed.\n"
            "\nResult:\n"
            "true|false   (boolean) If the signature is verified or not.\n"
            "\nExamples:\n"
            "\nUnlock the wallet for 30 seconds\n"
            + HelpExampleCli("walletpassphrase", "\"mypassphrase\" 30") +
            "\nCreate the signature\n"
            + HelpExampleCli("signmessage", "\"1D1ZrZNe3JUo7ZycKEYQQiQAWd9y54F4XZ\" \"my message\"") +
            "\nVerify the signature\n"
            + HelpExampleCli("verifymessage", "\"1D1ZrZNe3JUo7ZycKEYQQiQAWd9y54F4XZ\" \"signature\" \"my message\"") +
            "\nAs json rpc\n"
            + HelpExampleRpc("verifymessage", "\"1D1ZrZNe3JUo7ZycKEYQQiQAWd9y54F4XZ\", \"signature\", \"my message\"")
        );

    LOCK(cs_main);

    string strAddress  = params[0].get_str();
    string strSign     = params[1].get_str();
    string strMessage  = params[2].get_str();

    CNavcoinAddress addr(strAddress);
    if (!addr.IsValid())
        throw JSONRPCError(RPC_TYPE_ERROR, "Invalid address");

    CKeyID keyID;
    if (!addr.GetKeyID(keyID))
        throw JSONRPCError(RPC_TYPE_ERROR, "Address does not refer to key");

    bool fInvalid = false;
    vector<unsigned char> vchSig = DecodeBase64(strSign.c_str(), &fInvalid);

    if (fInvalid)
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Malformed base64 encoding");

    CHashWriter ss(SER_GETHASH, 0);
    ss << strMessageMagic;
    ss << strMessage;

    CPubKey pubkey;
    if (!pubkey.RecoverCompact(ss.GetHash(), vchSig))
        return false;

    return (pubkey.GetID() == keyID);
}

UniValue signmessagewithprivkey(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() != 2)
        throw runtime_error(
            "signmessagewithprivkey \"privkey\" \"message\"\n"
            "\nSign a message with the private key of an address\n"
            "\nArguments:\n"
            "1. \"privkey\"         (string, required) The private key to sign the message with.\n"
            "2. \"message\"         (string, required) The message to create a signature of.\n"
            "\nResult:\n"
            "\"signature\"          (string) The signature of the message encoded in base 64\n"
            "\nExamples:\n"
            "\nCreate the signature\n"
            + HelpExampleCli("signmessagewithprivkey", "\"privkey\" \"my message\"") +
            "\nVerify the signature\n"
            + HelpExampleCli("verifymessage", "\"1D1ZrZNe3JUo7ZycKEYQQiQAWd9y54F4XZ\" \"signature\" \"my message\"") +
            "\nAs json rpc\n"
            + HelpExampleRpc("signmessagewithprivkey", "\"privkey\", \"my message\"")
        );

    string strPrivkey = params[0].get_str();
    string strMessage = params[1].get_str();

    CNavcoinSecret vchSecret;
    bool fGood = vchSecret.SetString(strPrivkey);
    if (!fGood)
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid private key");
    CKey key = vchSecret.GetKey();
    if (!key.IsValid())
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Private key outside allowed range");

    CHashWriter ss(SER_GETHASH, 0);
    ss << strMessageMagic;
    ss << strMessage;

    vector<unsigned char> vchSig;
    if (!key.SignCompact(ss.GetHash(), vchSig))
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Sign failed");

    return EncodeBase64(&vchSig[0], vchSig.size());
}

UniValue setmocktime(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() != 1)
        throw runtime_error(
            "setmocktime timestamp\n"
            "\nSet the local time to given timestamp (-regtest only)\n"
            "\nArguments:\n"
            "1. timestamp  (integer, required) Unix seconds-since-epoch timestamp\n"
            "   Pass 0 to go back to using the system time."
        );

    if (!Params().MineBlocksOnDemand())
        throw runtime_error("setmocktime for regression testing (-regtest mode) only");

    // cs_vNodes is locked and node send/receive times are updated
    // atomically with the time change to prevent peers from being
    // disconnected because we think we haven't communicated with them
    // in a long time.
    LOCK2(cs_main, cs_vNodes);

    RPCTypeCheck(params, boost::assign::list_of(UniValue::VNUM));
    SetMockTime(params[0].get_int64());

    uint64_t t = GetTime();
    for(CNode* pnode: vNodes) {
        pnode->nLastSend = pnode->nLastRecv = t;
    }

    return NullUniValue;
}

bool getAddressFromIndex(const int &type, const uint160 &hash, std::string &address)
{
    if (type == 2) {
        address = CNavcoinAddress(CScriptID(hash)).ToString();
    } else if (type == 1) {
        address = CNavcoinAddress(CKeyID(hash)).ToString();
    } else {
        return false;
    }
    return true;
}


bool getAddressesFromParams(const UniValue& params, std::vector<std::pair<uint160, int> > &addresses)
{
    if (params[0].isStr()) {
        CNavcoinAddress address(params[0].get_str());
        uint160 hashBytes;
        int type = 0;
        if (!address.GetIndexKey(hashBytes, type)) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid address");
        }
        addresses.push_back(std::make_pair(hashBytes, type));
    } else if (params[0].isObject()) {

        UniValue addressValues = find_value(params[0].get_obj(), "addresses");
        if (!addressValues.isArray()) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Addresses is expected to be an array");
        }

        std::vector<UniValue> values = addressValues.getValues();

        for (std::vector<UniValue>::iterator it = values.begin(); it != values.end(); ++it) {

            CNavcoinAddress address(it->get_str());
            uint160 hashBytes;
            int type = 0;
            if (!address.GetIndexKey(hashBytes, type)) {
                throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid address");
            }
            addresses.push_back(std::make_pair(hashBytes, type));
        }
    } else {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid address");
    }

    return true;
}

bool getAddressesFromParamsForHistory(const UniValue& params, std::vector<std::pair<std::pair<uint160, uint160>, AddressHistoryFilter>> &addresses)
{
    if (params[0].isStr()) {
        CNavcoinAddress address(params[0].get_str());
        if (address.IsColdStakingv2Address(Params()))
        {
            CNavcoinAddress addressVoting, addressStaking, addressSpending;
            uint160 hashBytes, hashBytesSpending, hashBytesVoting, hashBytesStaking;
            int type = 0;

            address.GetStakingAddress(addressStaking);
            address.GetVotingAddress(addressVoting);
            address.GetSpendingAddress(addressSpending);

            if (!address.GetIndexKey(hashBytes, type)) {
                throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid address");
            }

            if (!addressStaking.GetIndexKey(hashBytesStaking, type)) {
                throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid address");
            }
            addresses.push_back(std::make_pair(std::make_pair(hashBytes, hashBytesStaking), AddressHistoryFilter::STAKABLE));

            if (!addressVoting.GetIndexKey(hashBytesVoting, type)) {
                throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid address");
            }
            addresses.push_back(std::make_pair(std::make_pair(hashBytes, hashBytesVoting), AddressHistoryFilter::VOTING_WEIGHT));

            if (!addressSpending.GetIndexKey(hashBytesSpending, type)) {
                throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid address");
            }
            addresses.push_back(std::make_pair(std::make_pair(hashBytes, hashBytesSpending), AddressHistoryFilter::SPENDABLE));
        }
        else if (address.IsColdStakingAddress(Params()))
        {
            CNavcoinAddress addressStaking, addressSpending;
            uint160 hashBytes, hashBytesStaking, hashBytesSpending;
            int type = 0;

            if (!address.GetIndexKey(hashBytes, type)) {
                throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid address");
            }

            address.GetStakingAddress(addressStaking);
            address.GetSpendingAddress(addressSpending);

            if (!addressStaking.GetIndexKey(hashBytesStaking, type)) {
                throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid address");
            }
            addresses.push_back(std::make_pair(std::make_pair(hashBytes, hashBytesStaking), AddressHistoryFilter::STAKABLE_VOTING_WEIGHT));

            if (!addressSpending.GetIndexKey(hashBytesSpending, type)) {
                throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid address");
            }
            addresses.push_back(std::make_pair(std::make_pair(hashBytes, hashBytesSpending), AddressHistoryFilter::SPENDABLE));
        }
        else
        {
            uint160 hashBytes;
            int type = 0;
            if (!address.GetIndexKey(hashBytes, type)) {
                throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid address");
            }
            addresses.push_back(std::make_pair(std::make_pair(hashBytes, hashBytes), AddressHistoryFilter::ALL));
        }
    } else if (params[0].isObject()) {

        UniValue addressValues = find_value(params[0].get_obj(), "addresses");
        if (!addressValues.isArray()) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Addresses is expected to be an array");
        }

        std::vector<UniValue> values = addressValues.getValues();

        for (std::vector<UniValue>::iterator it = values.begin(); it != values.end(); ++it) {

            CNavcoinAddress address(it->get_str());
            if (address.IsColdStakingv2Address(Params()))
            {
                CNavcoinAddress addressSpending, addressVoting, addressStaking;
                uint160 hashBytes, hashBytesSpending, hashBytesVoting, hashBytesStaking;
                int type = 0;

                if (!address.GetIndexKey(hashBytes, type)) {
                    throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid address");
                }

                address.GetStakingAddress(addressStaking);
                address.GetVotingAddress(addressVoting);
                address.GetSpendingAddress(addressSpending);

                if (!addressStaking.GetIndexKey(hashBytesStaking, type)) {
                    throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid address");
                }
                addresses.push_back(std::make_pair(std::make_pair(hashBytes, hashBytesStaking), AddressHistoryFilter::STAKABLE));

                if (!addressVoting.GetIndexKey(hashBytesVoting, type)) {
                    throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid address");
                }
                addresses.push_back(std::make_pair(std::make_pair(hashBytes, hashBytesVoting), AddressHistoryFilter::VOTING_WEIGHT));

                if (!addressSpending.GetIndexKey(hashBytesSpending, type)) {
                    throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid address");
                }
                addresses.push_back(std::make_pair(std::make_pair(hashBytes, hashBytesSpending), AddressHistoryFilter::SPENDABLE));
            }
            else if (address.IsColdStakingAddress(Params()))
            {
                CNavcoinAddress addressStaking, addressSpending;
                uint160 hashBytes, hashBytesStaking, hashBytesSpending;
                int type = 0;

                if (!address.GetIndexKey(hashBytes, type)) {
                    throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid address");
                }

                address.GetStakingAddress(addressStaking);
                address.GetSpendingAddress(addressSpending);

                if (!addressStaking.GetIndexKey(hashBytesStaking, type)) {
                    throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid address");
                }
                addresses.push_back(std::make_pair(std::make_pair(hashBytes, hashBytesStaking), AddressHistoryFilter::STAKABLE_VOTING_WEIGHT));

                if (!addressSpending.GetIndexKey(hashBytesSpending, type)) {
                    throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid address");
                }
                addresses.push_back(std::make_pair(std::make_pair(hashBytes, hashBytesSpending), AddressHistoryFilter::SPENDABLE));
            }
            else
            {
                uint160 hashBytes;
                int type = 0;
                if (!address.GetIndexKey(hashBytes, type)) {
                    throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid address");
                }
                addresses.push_back(std::make_pair(std::make_pair(hashBytes, hashBytes), AddressHistoryFilter::ALL));
            }

        }
    } else {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid address");
    }

    return true;
}

bool heightSort(std::pair<CAddressUnspentKey, CAddressUnspentValue> a,
                std::pair<CAddressUnspentKey, CAddressUnspentValue> b) {
    return a.second.blockHeight < b.second.blockHeight;
}

bool timestampSort(std::pair<CMempoolAddressDeltaKey, CMempoolAddressDelta> a,
                   std::pair<CMempoolAddressDeltaKey, CMempoolAddressDelta> b) {
    return a.second.time < b.second.time;
}

UniValue getaddressmempool(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() != 1)
        throw runtime_error(
            "getaddressmempool\n"
            "\nReturns all mempool deltas for an address (requires addressindex to be enabled).\n"
            "\nArguments:\n"
            "{\n"
            "  \"addresses\"\n"
            "    [\n"
            "      \"address\"  (string) The base58check encoded address\n"
            "      ,...\n"
            "    ]\n"
            "}\n"
            "\nResult:\n"
            "[\n"
            "  {\n"
            "    \"address\"  (string) The base58check encoded address\n"
            "    \"txid\"  (string) The related txid\n"
            "    \"index\"  (number) The related input or output index\n"
            "    \"satoshis\"  (number) The difference of satoshis\n"
            "    \"timestamp\"  (number) The time the transaction entered the mempool (seconds)\n"
            "    \"prevtxid\"  (string) The previous txid (if spending)\n"
            "    \"prevout\"  (string) The previous transaction output index (if spending)\n"
            "  }\n"
            "]\n"
            "\nExamples:\n"
            + HelpExampleCli("getaddressmempool", "'{\"addresses\": [\"12c6DSiU4Rq3P4ZxziKxzrL5LmMBrzjrJX\"]}'")
            + HelpExampleRpc("getaddressmempool", "{\"addresses\": [\"12c6DSiU4Rq3P4ZxziKxzrL5LmMBrzjrJX\"]}")
        );

    std::vector<std::pair<uint160, int> > addresses;

    if (!getAddressesFromParams(params, addresses)) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid address");
    }

    std::vector<std::pair<CMempoolAddressDeltaKey, CMempoolAddressDelta> > indexes;

    if (!mempool.getAddressIndex(addresses, indexes)) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "No information available for address");
    }

    std::sort(indexes.begin(), indexes.end(), timestampSort);

    UniValue result(UniValue::VARR);

    for (std::vector<std::pair<CMempoolAddressDeltaKey, CMempoolAddressDelta> >::iterator it = indexes.begin();
         it != indexes.end(); it++) {

        std::string address;
        if (!getAddressFromIndex(it->first.type, it->first.addressBytes, address)) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Unknown address type");
        }

        UniValue delta(UniValue::VOBJ);
        delta.pushKV("address", address);
        delta.pushKV("txid", it->first.txhash.GetHex());
        delta.pushKV("index", (int)it->first.index);
        delta.pushKV("satoshis", it->second.amount);
        delta.pushKV("timestamp", it->second.time);
        if (it->second.amount < 0) {
            delta.pushKV("prevtxid", it->second.prevhash.GetHex());
            delta.pushKV("prevout", (int)it->second.prevout);
        }
        result.push_back(delta);
    }

    return result;
}

UniValue getaddressutxos(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() != 1)
        throw runtime_error(
            "getaddressutxos\n"
            "\nReturns all unspent outputs for an address (requires addressindex to be enabled).\n"
            "\nArguments:\n"
            "{\n"
            "  \"addresses\"\n"
            "    [\n"
            "      \"address\"  (string) The base58check encoded address\n"
            "      ,...\n"
            "    ],\n"
            "  \"chainInfo\"  (boolean) Include chain info with results\n"
            "}\n"
            "\nResult\n"
            "[\n"
            "  {\n"
            "    \"address\"  (string) The address base58check encoded\n"
            "    \"txid\"  (string) The output txid\n"
            "    \"height\"  (number) The block height\n"
            "    \"outputIndex\"  (number) The output index\n"
            "    \"script\"  (strin) The script hex encoded\n"
            "    \"satoshis\"  (number) The number of satoshis of the output\n"
            "  }\n"
            "]\n"
            "\nExamples:\n"
            + HelpExampleCli("getaddressutxos", "'{\"addresses\": [\"12c6DSiU4Rq3P4ZxziKxzrL5LmMBrzjrJX\"]}'")
            + HelpExampleRpc("getaddressutxos", "{\"addresses\": [\"12c6DSiU4Rq3P4ZxziKxzrL5LmMBrzjrJX\"]}")
            );

    bool includeChainInfo = false;
    if (params[0].isObject()) {
        UniValue chainInfo = find_value(params[0].get_obj(), "chainInfo");
        if (chainInfo.isBool()) {
            includeChainInfo = chainInfo.get_bool();
        }
    }

    std::vector<std::pair<uint160, int> > addresses;

    if (!getAddressesFromParams(params, addresses)) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid address");
    }

    std::vector<std::pair<CAddressUnspentKey, CAddressUnspentValue> > unspentOutputs;

    for (std::vector<std::pair<uint160, int> >::iterator it = addresses.begin(); it != addresses.end(); it++) {
        if (!GetAddressUnspent((*it).first, (*it).second, unspentOutputs)) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "No information available for address");
        }
    }

    std::sort(unspentOutputs.begin(), unspentOutputs.end(), heightSort);

    UniValue utxos(UniValue::VARR);

    for (std::vector<std::pair<CAddressUnspentKey, CAddressUnspentValue> >::const_iterator it=unspentOutputs.begin(); it!=unspentOutputs.end(); it++) {
        UniValue output(UniValue::VOBJ);
        std::string address;
        if (!getAddressFromIndex(it->first.type, it->first.hashBytes, address)) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Unknown address type");
        }

        output.pushKV("address", address);
        output.pushKV("txid", it->first.txhash.GetHex());
        output.pushKV("outputIndex", (int)it->first.index);
        output.pushKV("script", HexStr(it->second.script.begin(), it->second.script.end()));
        output.pushKV("satoshis", it->second.satoshis);
        output.pushKV("height", it->second.blockHeight);
        utxos.push_back(output);
    }

    if (includeChainInfo) {
        UniValue result(UniValue::VOBJ);
        result.pushKV("utxos", utxos);

        LOCK(cs_main);
        result.pushKV("hash", chainActive.Tip()->GetBlockHash().GetHex());
        result.pushKV("height", (int)chainActive.Height());
        return result;
    } else {
        return utxos;
    }
}

UniValue getaddressdeltas(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() != 1 || !params[0].isObject())
        throw runtime_error(
            "getaddressdeltas\n"
            "\nReturns all changes for an address (requires addressindex to be enabled).\n"
            "\nArguments:\n"
            "{\n"
            "  \"addresses\"\n"
            "    [\n"
            "      \"address\"  (string) The base58check encoded address\n"
            "      ,...\n"
            "    ]\n"
            "  \"start\" (number) The start block height\n"
            "  \"end\" (number) The end block height\n"
            "  \"chainInfo\" (boolean) Include chain info in results, only applies if start and end specified\n"
            "}\n"
            "\nResult:\n"
            "[\n"
            "  {\n"
            "    \"satoshis\"  (number) The difference of satoshis\n"
            "    \"txid\"  (string) The related txid\n"
            "    \"index\"  (number) The related input or output index\n"
            "    \"height\"  (number) The block height\n"
            "    \"address\"  (string) The base58check encoded address\n"
            "  }\n"
            "]\n"
            "\nExamples:\n"
            + HelpExampleCli("getaddressdeltas", "'{\"addresses\": [\"12c6DSiU4Rq3P4ZxziKxzrL5LmMBrzjrJX\"]}'")
            + HelpExampleRpc("getaddressdeltas", "{\"addresses\": [\"12c6DSiU4Rq3P4ZxziKxzrL5LmMBrzjrJX\"]}")
        );


    UniValue startValue = find_value(params[0].get_obj(), "start");
    UniValue endValue = find_value(params[0].get_obj(), "end");

    UniValue chainInfo = find_value(params[0].get_obj(), "chainInfo");
    bool includeChainInfo = false;
    if (chainInfo.isBool()) {
        includeChainInfo = chainInfo.get_bool();
    }

    int start = 0;
    int end = 0;

    if (startValue.isNum() && endValue.isNum()) {
        start = startValue.get_int();
        end = endValue.get_int();
        if (start <= 0 || end <= 0) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Start and end is expected to be greater than zero");
        }
        if (end < start) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "End value is expected to be greater than start");
        }
    }

    std::vector<std::pair<uint160, int> > addresses;

    if (!getAddressesFromParams(params, addresses)) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid address");
    }

    std::vector<std::pair<CAddressIndexKey, CAmount> > addressIndex;

    for (std::vector<std::pair<uint160, int> >::iterator it = addresses.begin(); it != addresses.end(); it++) {
        if (start > 0 && end > 0) {
            if (!GetAddressIndex((*it).first, (*it).second, addressIndex, start, end)) {
                throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "No information available for address");
            }
        } else {
            if (!GetAddressIndex((*it).first, (*it).second, addressIndex)) {
                throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "No information available for address");
            }
        }
    }

    UniValue deltas(UniValue::VARR);

    for (std::vector<std::pair<CAddressIndexKey, CAmount> >::const_iterator it=addressIndex.begin(); it!=addressIndex.end(); it++) {
        std::string address;
        if (!getAddressFromIndex(it->first.type, it->first.hashBytes, address)) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Unknown address type");
        }

        UniValue delta(UniValue::VOBJ);
        delta.pushKV("satoshis", it->second);
        delta.pushKV("txid", it->first.txhash.GetHex());
        delta.pushKV("index", (int)it->first.index);
        delta.pushKV("blockindex", (int)it->first.txindex);
        delta.pushKV("height", it->first.blockHeight);
        delta.pushKV("address", address);
        deltas.push_back(delta);
    }

    UniValue result(UniValue::VOBJ);

    if (includeChainInfo && start > 0 && end > 0) {
        LOCK(cs_main);

        if (start > chainActive.Height() || end > chainActive.Height()) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Start or end is outside chain range");
        }

        CBlockIndex* startIndex = chainActive[start];
        CBlockIndex* endIndex = chainActive[end];

        UniValue startInfo(UniValue::VOBJ);
        UniValue endInfo(UniValue::VOBJ);

        startInfo.pushKV("hash", startIndex->GetBlockHash().GetHex());
        startInfo.pushKV("height", start);

        endInfo.pushKV("hash", endIndex->GetBlockHash().GetHex());
        endInfo.pushKV("height", end);

        result.pushKV("deltas", deltas);
        result.pushKV("start", startInfo);
        result.pushKV("end", endInfo);

        return result;
    } else {
        return deltas;
    }
}

UniValue getaddressbalance(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() != 1)
        throw runtime_error(
            "getaddressbalance\n"
            "\nReturns the balance for an address(es) (requires addressindex to be enabled).\n"
            "\nArguments:\n"
            "{\n"
            "  \"addresses\"\n"
            "    [\n"
            "      \"address\"  (string) The base58check encoded address\n"
            "      ,...\n"
            "    ]\n"
            "}\n"
            "\nResult:\n"
            "{\n"
            "  \"balance\"  (string) The current balance in satoshis\n"
            "  \"received\"  (string) The total number of satoshis received (including change)\n"
            "}\n"
            "\nExamples:\n"
            + HelpExampleCli("getaddressbalance", "'{\"addresses\": [\"12c6DSiU4Rq3P4ZxziKxzrL5LmMBrzjrJX\"]}'")
            + HelpExampleRpc("getaddressbalance", "{\"addresses\": [\"12c6DSiU4Rq3P4ZxziKxzrL5LmMBrzjrJX\"]}")
        );

    std::vector<std::pair<std::pair<uint160, uint160>, AddressHistoryFilter>> addresses;

    if (!getAddressesFromParamsForHistory(params, addresses)) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid address");
    }

    std::vector<std::pair<CAddressHistoryKey, CAddressHistoryValue> > addressHistory;

    for (std::vector<std::pair<std::pair<uint160, uint160>, AddressHistoryFilter>>::iterator it = addresses.begin(); it != addresses.end(); it++) {
        if (!GetAddressHistory((*it).first.first, (*it).first.second, addressHistory, (*it).second)) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "No information available for address");
        }
    }

    CAmount spending = 0;
    CAmount stakable = 0;
    CAmount voting_weight = 0;
    CAmount received = 0;
    CAmount staked = 0;

    for (std::vector<std::pair<CAddressHistoryKey, CAddressHistoryValue> >::const_iterator it=addressHistory.begin(); it!=addressHistory.end(); it++) {
        spending += (*it).second.spendable;
        stakable += (*it).second.stakable;
        voting_weight += (*it).second.voting_weight;
        if ((*it).second.spendable > 0)
            received += (*it).second.spendable;
    }


    UniValue result(UniValue::VOBJ);
    result.pushKV("balance", spending);
    result.pushKV("stakable", stakable);
    result.pushKV("voting_weight", voting_weight);
    result.pushKV("received", received);
    result.pushKV("staked", staked);

    return result;

}

UniValue getaddresshistory(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() != 1)
        throw runtime_error(
            "getaddresshistory\n"
            "\nReturns the history for an address(es) (requires addressindex to be enabled).\n"
            "\nArguments:\n"
            "{\n"
            "  \"addresses\"\n"
            "    [\n"
            "      \"address\"  (string) The base58check encoded address\n"
            "      ,...\n"
            "    ]\n"
            "  \"start\" (number, optional) The start block height\n"
            "  \"end\" (number, optional) The end block height\n"
            "}\n"
            "\nResult:\n"
            "[\n"
            "  {\n"
            "    \"block\": (integer) the block height when this entry was seen\n"
            "    \"txindex\": (integer) the index of the transaction inside of the block\n"
            "    \"time\": (integer) timestamp when this entry happened\n"
            "    \"txid\": (string) hash of the transaction\n"
            "    \"txout\": (integer) the index of the output inside of the transaction\n"
            "    \"address\": (string) the address affected\n"
            "    \"changes\": {\n"
            "      \"balance\": (signed integer) the change in spendable balance\n"
            "      \"stakable\": (signed integer) the change in stakable balance\n"
            "      \"voting_weight\": (signed integer) the change in voting weight\n"
            "      \"flags\": (integer) 1 if is a coinstake or coinbase transaction, 0 otherwise\n"
            "    },\n"
            "    \"result\": {\n"
            "      \"balance\": (integer) the spendable balance after the change\n"
            "      \"stakable\": (integer) the stakable balance after the change\n"
            "      \"voting_weight\": (integer) the voting weight after the change\n"
            "    }\n"
            "  }\n"
            "]\n"
            "\nExamples:\n"
            + HelpExampleCli("getaddresshistory", "'{\"addresses\": [\"12c6DSiU4Rq3P4ZxziKxzrL5LmMBrzjrJX\"]}'")
            + HelpExampleRpc("getaddresshistory", "{\"addresses\": [\"12c6DSiU4Rq3P4ZxziKxzrL5LmMBrzjrJX\"]}")
        );


    UniValue startValue = find_value(params[0].get_obj(), "start");
    UniValue endValue = find_value(params[0].get_obj(), "end");

    int start = 0;
    int end = 0;

    bool range = false;

    if (startValue.isNum() && endValue.isNum()) {
        start = startValue.get_int();
        end = endValue.get_int();
        if (start <= 0 || end <= 0) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Start and end is expected to be greater than zero");
        }
        if (end < start) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "End value is expected to be greater than start");
        }

        range = true;
    }

    std::vector<std::pair<std::pair<uint160, uint160>, AddressHistoryFilter>> addresses;

    if (!getAddressesFromParamsForHistory(params, addresses)) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid address");
    }

    std::vector<std::pair<CAddressHistoryKey, CAddressHistoryValue> > addressHistory;

    for (std::vector<std::pair<std::pair<uint160, uint160>, AddressHistoryFilter>>::iterator it = addresses.begin(); it != addresses.end(); it++) {
        if (!GetAddressHistory((*it).first.first, (*it).first.second, addressHistory, (*it).second)) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "No information available for address");
        }
    }

    std::sort(addressHistory.begin(), addressHistory.end(), [](const std::pair<CAddressHistoryKey, CAddressHistoryValue> &a, const std::pair<CAddressHistoryKey, CAddressHistoryValue> &b) -> bool
    {
        auto a_ = a.first;
        auto b_ = b.first;

        if (a_.blockHeight == b_.blockHeight) {
            return a_.txindex < b_.txindex;
        } else {
            return a_.blockHeight < b_.blockHeight;
        }
    });

    struct balStruct
    {
        CAmount spendable;
        CAmount stakable;
        CAmount voting_weight;
    };

    std::map<CNavcoinAddress, balStruct> balance;

    UniValue result(UniValue::VARR);

    for (std::vector<std::pair<CAddressHistoryKey, CAddressHistoryValue> >::const_iterator it=addressHistory.begin(); it!=addressHistory.end(); it++) {
        CNavcoinAddress address;
        address.Set(CKeyID((*it).first.hashBytes2));

        if (balance.count(address) == 0) {
            balance.insert(std::make_pair(address, (struct balStruct){.spendable = 0, .stakable = 0, .voting_weight = 0}));
        }

        balance[address].spendable += (*it).second.spendable;
        balance[address].stakable += (*it).second.stakable;
        balance[address].voting_weight += (*it).second.voting_weight;

        if (!(!range || (range && (*it).first.blockHeight >= start && (*it).first.blockHeight <= end)))
            continue;

        UniValue entry(UniValue::VOBJ);
        entry.pushKV("block", (*it).first.blockHeight);
        entry.pushKV("txindex", (uint64_t)(*it).first.txindex);
        entry.pushKV("time", (uint64_t)(*it).first.time);
        entry.pushKV("txid", (*it).first.txhash.ToString());
        entry.pushKV("address", address.ToString());

        UniValue changes(UniValue::VOBJ);
        changes.pushKV("balance", (*it).second.spendable);
        changes.pushKV("stakable", (*it).second.stakable);
        changes.pushKV("voting_weight", (*it).second.voting_weight);
        changes.pushKV("flags", (*it).second.flags);
        entry.pushKV("changes", changes);

        UniValue balanceObj(UniValue::VOBJ);
        balanceObj.pushKV("balance", balance[address].spendable);
        balanceObj.pushKV("stakable", balance[address].stakable);
        balanceObj.pushKV("voting_weight", balance[address].voting_weight);
        entry.pushKV("result", balanceObj);

        result.push_back(entry);
    }

    return result;

}

UniValue getaddresstxids(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() != 1)
        throw runtime_error(
            "getaddresstxids\n"
            "\nReturns the txids for an address(es) (requires addressindex to be enabled).\n"
            "\nArguments:\n"
            "{\n"
            "  \"addresses\"\n"
            "    [\n"
            "      \"address\"  (string) The base58check encoded address\n"
            "      ,...\n"
            "    ]\n"
            "  \"start\" (number) The start block height\n"
            "  \"end\" (number) The end block height\n"
            "}\n"
            "\nResult:\n"
            "[\n"
            "  \"transactionid\"  (string) The transaction id\n"
            "  ,...\n"
            "]\n"
            "\nExamples:\n"
            + HelpExampleCli("getaddresstxids", "'{\"addresses\": [\"12c6DSiU4Rq3P4ZxziKxzrL5LmMBrzjrJX\"]}'")
            + HelpExampleRpc("getaddresstxids", "{\"addresses\": [\"12c6DSiU4Rq3P4ZxziKxzrL5LmMBrzjrJX\"]}")
        );

    std::vector<std::pair<uint160, int> > addresses;

    if (!getAddressesFromParams(params, addresses)) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid address");
    }

    int start = 0;
    int end = 0;
    if (params[0].isObject()) {
        UniValue startValue = find_value(params[0].get_obj(), "start");
        UniValue endValue = find_value(params[0].get_obj(), "end");
        if (startValue.isNum() && endValue.isNum()) {
            start = startValue.get_int();
            end = endValue.get_int();
        }
    }

    std::vector<std::pair<CAddressIndexKey, CAmount> > addressIndex;

    for (std::vector<std::pair<uint160, int> >::iterator it = addresses.begin(); it != addresses.end(); it++) {
        if (start > 0 && end > 0) {
            if (!GetAddressIndex((*it).first, (*it).second, addressIndex, start, end)) {
                throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "No information available for address");
            }
        } else {
            if (!GetAddressIndex((*it).first, (*it).second, addressIndex)) {
                throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "No information available for address");
            }
        }
    }

    std::set<std::pair<int, std::string> > txids;
    UniValue result(UniValue::VARR);

    for (std::vector<std::pair<CAddressIndexKey, CAmount> >::const_iterator it=addressIndex.begin(); it!=addressIndex.end(); it++) {
        int height = it->first.blockHeight;
        std::string txid = it->first.txhash.GetHex();

        if (addresses.size() > 1) {
            txids.insert(std::make_pair(height, txid));
        } else {
            if (txids.insert(std::make_pair(height, txid)).second) {
                result.push_back(txid);
            }
        }
    }

    if (addresses.size() > 1) {
        for (std::set<std::pair<int, std::string> >::const_iterator it=txids.begin(); it!=txids.end(); it++) {
            result.push_back(it->second);
        }
    }

    return result;

}

UniValue getspentinfo(const UniValue& params, bool fHelp)
{

    if (fHelp || params.size() != 1 || !params[0].isObject())
        throw runtime_error(
            "getspentinfo\n"
            "\nReturns the txid and index where an output is spent.\n"
            "\nArguments:\n"
            "{\n"
            "  \"txid\" (string) The hex string of the txid\n"
            "  \"index\" (number) The start block height\n"
            "}\n"
            "\nResult:\n"
            "{\n"
            "  \"txid\"  (string) The transaction id\n"
            "  \"index\"  (number) The spending input index\n"
            "  ,...\n"
            "}\n"
            "\nExamples:\n"
            + HelpExampleCli("getspentinfo", "'{\"txid\": \"0437cd7f8525ceed2324359c2d0ba26006d92d856a9c20fa0241106ee5a597c9\", \"index\": 0}'")
            + HelpExampleRpc("getspentinfo", "{\"txid\": \"0437cd7f8525ceed2324359c2d0ba26006d92d856a9c20fa0241106ee5a597c9\", \"index\": 0}")
        );

    UniValue txidValue = find_value(params[0].get_obj(), "txid");
    UniValue indexValue = find_value(params[0].get_obj(), "index");

    if (!txidValue.isStr() || !indexValue.isNum()) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid txid or index");
    }

    uint256 txid = ParseHashV(txidValue, "txid");
    int outputIndex = indexValue.get_int();

    CSpentIndexKey key(txid, outputIndex);
    CSpentIndexValue value;

    if (!GetSpentIndex(key, value)) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Unable to get spent info");
    }

    UniValue obj(UniValue::VOBJ);
    obj.pushKV("txid", value.txid.GetHex());
    obj.pushKV("index", (int)value.inputIndex);
    obj.pushKV("height", value.blockHeight);

    return obj;
}

static const CRPCCommand commands[] =
{ //  category              name                      actor (function)         okSafeMode
  //  --------------------- ------------------------  -----------------------  ----------
    { "control",            "getinfo",                &getinfo,                true  }, /* uses wallet if enabled */
    { "util",               "validateaddress",        &validateaddress,        true  }, /* uses wallet if enabled */
    { "util",               "createmultisig",         &createmultisig,         true  },
    { "util",               "createwitnessaddress",   &createwitnessaddress,   true  },
    { "util",               "verifymessage",          &verifymessage,          true  },
    { "util",               "signmessagewithprivkey", &signmessagewithprivkey, true  },

    /* Address index */
    { "addressindex",       "getaddressmempool",      &getaddressmempool,      true  },
    { "addressindex",       "getaddressutxos",        &getaddressutxos,        false },
    { "addressindex",       "getaddressdeltas",       &getaddressdeltas,       false },
    { "addressindex",       "getaddresstxids",        &getaddresstxids,        false },
    { "addressindex",       "getaddressbalance",      &getaddressbalance,      false },
    { "addressindex",       "getaddresshistory",      &getaddresshistory,      false },

    /* Blockchain */
    { "blockchain",         "getspentinfo",           &getspentinfo,           false },

    /* Not shown in help */
    { "hidden",             "setmocktime",            &setmocktime,            true  },
};

void RegisterMiscRPCCommands(CRPCTable &tableRPC)
{
    for (unsigned int vcidx = 0; vcidx < ARRAYLEN(commands); vcidx++)
        tableRPC.appendCommand(commands[vcidx].name, &commands[vcidx]);
}
