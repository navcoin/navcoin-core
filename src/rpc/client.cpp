// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <rpc/client.h>
#include <rpc/protocol.h>
#include <util.h>

#include <set>
#include <stdint.h>

#include <boost/algorithm/string/case_conv.hpp> // for to_lower()
#include <univalue.h>

class CRPCConvertParam
{
public:
    std::string methodName; //!< method whose params want conversion
    int paramIdx;           //!< 0-based idx of param to convert
};

static const CRPCConvertParam vRPCConvertParams[] =
{
    { "addmultisigaddress", 0 },
    { "addmultisigaddress", 1 },
    { "burntoken", 1 },
    { "coinbaseoutputs", 0 },
    { "coinstakeinputs", 0 },
    { "coinstakeoutputs", 0 },
    { "consultationvote", 2 },
    { "createconsultation", 1 },
    { "createconsultation", 2 },
    { "createconsultation", 3 },
    { "createconsultation", 4 },
    { "createconsultation", 5 },
    { "createconsultation", 6 },
    { "createconsultationwithanswers", 1 },
    { "createconsultationwithanswers", 2 },
    { "createconsultationwithanswers", 3 },
    { "createconsultationwithanswers", 4 },
    { "createconsultationwithanswers", 5 },
    { "createconsultationwithanswers", 6 },
    { "createmultisig", 0 },
    { "createmultisig", 1 },
    { "createnft", 2 },
    { "createpaymentrequest", 3 },
    { "createpaymentrequest", 4 },
    { "createpaymentrequest", 5 },
    { "createproposal", 2 },
    { "createproposal", 5 },
    { "createproposal", 7 },
    { "createrawtransaction", 0 },
    { "createrawtransaction", 1 },
    { "createrawtransaction", 3 },
    { "createrawtransaction", 4 },
    { "createtoken", 2 },
    { "donatefund", 1 },
    { "estimatefee", 0 },
    { "estimatepriority", 0 },
    { "estimatesmartfee", 0 },
    { "estimatesmartpriority", 0 },
    { "forcetransactions", 0 },
    { "fundrawtransaction", 1 },
    { "generate", 0 },
    { "generate", 1 },
    { "generatetoaddress", 0 },
    { "generatetoaddress", 2 },
    { "getaddednodeinfo", 0 },
    { "getaddressbalance", 0},
    { "getaddressdeltas", 0},
    { "getaddresshistory", 0},
    { "getaddressmempool", 0},
    { "getaddresstxids", 0},
    { "getaddressutxos", 0},
    { "getbalance", 1 },
    { "getbalance", 2 },
    { "getblock", 1 },
    { "getblockhash", 0 },
    { "getblockhashes", 0 },
    { "getblockhashes", 1 },
    { "getblockhashes", 2 },
    { "getblockheader", 1 },
    { "getblocktemplate", 0 },
    { "getmempoolancestors", 1 },
    { "getmempooldescendants", 1 },
    { "getnetworkhashps", 0 },
    { "getnetworkhashps", 1 },
    { "getnewprivateaddress", 0 },
    { "getrawmempool", 0 },
    { "getrawtransaction", 1 },
    { "getreceivedbyaccount", 1 },
    { "getreceivedbyaddress", 1 },
    { "getspentinfo", 0},
    { "getstakerscript", 1 },
    { "gettoken", 1 },
    { "getnft", 1 },
    { "getnft", 2 },
    { "gettransaction", 1 },
    { "gettxout", 1 },
    { "gettxout", 2 },
    { "gettxoutproof", 0 },
    { "importaddress", 2 },
    { "importaddress", 3 },
    { "importprivkey", 2 },
    { "importpubkey", 2 },
    { "keypoolrefill", 0 },
    { "listaccounts", 0 },
    { "listaccounts", 1 },
    { "listprivateunspent", 0 },
    { "listprivateunspent", 1 },
    { "listprivateunspent", 2 },
    { "listreceivedbyaccount", 0 },
    { "listreceivedbyaccount", 1 },
    { "listreceivedbyaccount", 2 },
    { "listreceivedbyaddress", 0 },
    { "listreceivedbyaddress", 1 },
    { "listreceivedbyaddress", 2 },
    { "listsinceblock", 1 },
    { "listsinceblock", 2 },
    { "listtokens", 0 },
    { "listnfts", 0 },
    { "listnfts", 1 },
    { "listtransactions", 1 },
    { "listtransactions", 2 },
    { "listtransactions", 3 },
    { "listunspent", 0 },
    { "listunspent", 1 },
    { "listunspent", 2 },
    { "lockunspent", 0 },
    { "lockunspent", 1 },
    { "mintnft", 1 },
    { "minttoken", 2 },
    { "move", 2 },
    { "move", 3 },
    { "prioritisetransaction", 1 },
    { "prioritisetransaction", 2 },
    { "proposeanswer", 1 },
    { "proposeanswer", 2 },
    { "proposeanswer", 3 },
    { "proposecombinedconsensuschange", 0},
    { "proposecombinedconsensuschange", 1},
    { "proposecombinedconsensuschange", 2},
    { "proposecombinedconsensuschange", 3},
    { "proposeconsensuschange", 0},
    { "proposeconsensuschange", 1},
    { "proposeconsensuschange", 2},
    { "proposeconsensuschange", 3},
    { "resolvename", 1 },
    { "scanviewkey", 1 },
    { "sendfrom", 2 },
    { "sendfrom", 3 },
    { "sendmany", 1 },
    { "sendmany", 2 },
    { "sendmany", 4 },
    { "sendnft", 1 },
    { "sendnft", 3 },
    { "sendrawtransaction", 1 },
    { "sendtoaddress", 1 },
    { "sendtoaddress", 5 },
    { "sendtoken", 2 },
    { "setaggregationfee", 0},
    { "setban", 2 },
    { "setban", 3 },
    { "setexclude", 0 },
    { "setmocktime", 0 },
    { "settxfee", 0 },
    { "signrawtransaction", 1 },
    { "signrawtransaction", 2 },
    { "staking", 0 },
    { "stop", 0 },
    { "support", 1 },
    { "verifychain", 0 },
    { "verifychain", 1 },
    { "viewaggregationsession", 0},
    { "walletpassphrase", 1 },
    { "walletpassphrase", 2 },
};

class CRPCConvertTable
{
private:
    std::set<std::pair<std::string, int> > members;

public:
    CRPCConvertTable();

    bool convert(const std::string& method, int idx) {
        return (members.count(std::make_pair(method, idx)) > 0);
    }
};

CRPCConvertTable::CRPCConvertTable()
{
    const unsigned int n_elem =
        (sizeof(vRPCConvertParams) / sizeof(vRPCConvertParams[0]));

    for (unsigned int i = 0; i < n_elem; i++) {
        members.insert(std::make_pair(vRPCConvertParams[i].methodName,
                                      vRPCConvertParams[i].paramIdx));
    }
}

static CRPCConvertTable rpcCvtTable;

/** Non-RFC4627 JSON parser, accepts internal values (such as numbers, true, false, null)
 * as well as objects and arrays.
 */
UniValue ParseNonRFCJSONValue(const std::string& strVal)
{
    UniValue jVal;

    if (!jVal.read(std::string("[")+strVal+std::string("]")) ||
        !jVal.isArray() || jVal.size()!=1)
        throw std::runtime_error(std::string("Error parsing JSON:")+strVal);
    return jVal[0];
}

/** Convert strings to command-specific RPC representation */
UniValue RPCConvertValues(const std::string &strMethod, const std::vector<std::string> &strParams)
{
    UniValue params(UniValue::VARR);

    for (unsigned int idx = 0; idx < strParams.size(); idx++) {
        const std::string& strVal = strParams[idx];

        if (!rpcCvtTable.convert(strMethod, idx)) {
            // insert string value directly
            params.push_back(strVal);
        } else {
            // parse string as JSON, insert bool/number/object/etc. value
            params.push_back(ParseNonRFCJSONValue(strVal));
        }
    }

    return params;
}
