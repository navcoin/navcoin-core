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

using namespace std;

class CRPCConvertParam
{
public:
    std::string methodName; //!< method whose params want conversion
    int paramIdx;           //!< 0-based idx of param to convert
};

static const CRPCConvertParam vRPCConvertParams[] =
{
    { "stop", 0 },
    { "setmocktime", 0 },
    { "getaddednodeinfo", 0 },
    { "generate", 0 },
    { "generate", 1 },
    { "generatetoaddress", 0 },
    { "generatetoaddress", 2 },
    { "getnetworkhashps", 0 },
    { "getnetworkhashps", 1 },
    { "sendtoaddress", 1 },
    { "sendtoaddress", 5 },
    { "setaggregationfee", 0},
    { "settxfee", 0 },
    { "getreceivedbyaddress", 1 },
    { "getreceivedbyaccount", 1 },
    { "listreceivedbyaddress", 0 },
    { "listreceivedbyaddress", 1 },
    { "listreceivedbyaddress", 2 },
    { "listreceivedbyaccount", 0 },
    { "listreceivedbyaccount", 1 },
    { "listreceivedbyaccount", 2 },
    { "getbalance", 1 },
    { "getbalance", 2 },
    { "getblockhash", 0 },
    { "viewaggregationsession", 0},
    { "move", 2 },
    { "move", 3 },
    { "sendfrom", 2 },
    { "sendfrom", 3 },
    { "listtransactions", 1 },
    { "listtransactions", 2 },
    { "listtransactions", 3 },
    { "listaccounts", 0 },
    { "listaccounts", 1 },
    { "walletpassphrase", 1 },
    { "walletpassphrase", 2 },
    { "getblocktemplate", 0 },
    { "listsinceblock", 1 },
    { "listsinceblock", 2 },
    { "sendmany", 1 },
    { "sendmany", 2 },
    { "sendmany", 4 },
    { "addmultisigaddress", 0 },
    { "addmultisigaddress", 1 },
    { "createmultisig", 0 },
    { "createmultisig", 1 },
    { "createproposal", 2 },
    { "createproposal", 5 },
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
    { "proposeconsensuschange", 0},
    { "proposeconsensuschange", 1},
    { "proposeconsensuschange", 2},
    { "proposeconsensuschange", 3},
    { "consultationvote", 2 },
    { "support", 1 },
    { "proposeanswer", 1 },
    { "proposeanswer", 2 },
    { "proposeanswer", 3 },
    { "createpaymentrequest", 3 },
    { "createpaymentrequest", 4 },
    { "createpaymentrequest", 5 },
    { "listunspent", 0 },
    { "listunspent", 1 },
    { "listunspent", 2 },
    { "listprivateunspent", 0 },
    { "listprivateunspent", 1 },
    { "listprivateunspent", 2 },
    { "getblock", 1 },
    { "getblockheader", 1 },
    { "gettransaction", 1 },
    { "getrawtransaction", 1 },
    { "getstakerscript", 1 },
    { "createrawtransaction", 0 },
    { "createrawtransaction", 1 },
    { "createrawtransaction", 3 },
    { "createrawtransaction", 4 },
    { "signrawtransaction", 1 },
    { "signrawtransaction", 2 },
    { "sendrawtransaction", 1 },
    { "fundrawtransaction", 1 },
    { "gettxout", 1 },
    { "gettxout", 2 },
    { "gettxoutproof", 0 },
    { "lockunspent", 0 },
    { "lockunspent", 1 },
    { "importprivkey", 2 },
    { "importaddress", 2 },
    { "importaddress", 3 },
    { "importpubkey", 2 },
    { "verifychain", 0 },
    { "verifychain", 1 },
    { "keypoolrefill", 0 },
    { "getrawmempool", 0 },
    { "estimatefee", 0 },
    { "estimatepriority", 0 },
    { "estimatesmartfee", 0 },
    { "estimatesmartpriority", 0 },
    { "prioritisetransaction", 1 },
    { "prioritisetransaction", 2 },
    { "setban", 2 },
    { "setban", 3 },
    { "getmempoolancestors", 1 },
    { "getmempooldescendants", 1 },
    { "getblockhashes", 0 },
    { "getblockhashes", 1 },
    { "getblockhashes", 2 },
    { "getspentinfo", 0},
    { "getaddresstxids", 0},
    { "getaddressbalance", 0},
    { "getaddresshistory", 0},
    { "getaddressdeltas", 0},
    { "getaddressutxos", 0},
    { "getaddressmempool", 0},
    { "staking", 0 },
    { "setexclude", 0 },
    { "coinbaseoutputs", 0 },
    { "coinstakeoutputs", 0 },
    { "coinstakeinputs", 0 },
    { "forcetransactions", 0 },
    { "donatefund", 1 },
    { "getnewprivateaddress", 0 },
    { "scanviewkey", 1 },
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
        throw runtime_error(string("Error parsing JSON:")+strVal);
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
