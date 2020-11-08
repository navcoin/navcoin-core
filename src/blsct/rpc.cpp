// Copyright (c) 2020 The NavCoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <base58.h>
#include <blsct/rpc.h>
#include <core_io.h>
#include <rpc/server.h>
#include <wallet/wallet.h>

bool EnsureWalletIsAvailable(bool avoidException)
{
    if (!pwalletMain)
    {
        if (!avoidException)
            throw JSONRPCError(RPC_METHOD_NOT_FOUND, "Method not found (disabled)");
        else
            return false;
    }
    return true;
}

UniValue startaggregationsession(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() != 0)
        throw std::runtime_error(
                "startaggregationsession\n"
                "\nStarts a BLSCT mix session\n"
                );

    {
        LOCK(pwalletMain->cs_wallet);
        if (!pwalletMain->aggSession)
            throw JSONRPCError(RPC_INTERNAL_ERROR, string("Error internal structures"));

        if (!pwalletMain->aggSession->Start())
            throw JSONRPCError(RPC_INTERNAL_ERROR, string("Could not start mix session"));

        return pwalletMain->aggSession->GetHiddenService();
    }
}

UniValue stopaggregationsession(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() != 0)
        throw std::runtime_error(
                "stopaggregationsession\n"
                "Stops a BLSCT mix session\n"
                );

    {
        LOCK(pwalletMain->cs_wallet);
        if (!pwalletMain->aggSession)
            throw JSONRPCError(RPC_INTERNAL_ERROR, string("Error internal structures"));

        if (!pwalletMain->aggSession->GetState())
            throw JSONRPCError(RPC_INTERNAL_ERROR, string("Could not find a started mix session"));

        pwalletMain->aggSession->Stop();

        return true;
    }
}

UniValue viewaggregationsession(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() != 0)
        throw std::runtime_error(
                "viewaggregationsession\n"
                "Shows the active mix session if any\n"
                );

    UniValue ret(UniValue::VOBJ);

    {
        LOCK(pwalletMain->cs_wallet);
        if (!pwalletMain->aggSession)
            return ret;
        ret.pushKV("hiddenService",pwalletMain->aggSession->GetHiddenService());
        ret.pushKV("running",pwalletMain->aggSession->GetState());
        UniValue candidates(UniValue::VARR);
        for(auto&it: pwalletMain->aggSession->GetTransactionCandidates())
        {
            UniValue obj(UniValue::VOBJ);
            obj.pushKV("hex",EncodeHexTx(it.tx));
            obj.pushKV("fee",it.fee);
            candidates.push_back(obj);
        }
        ret.pushKV("txCandidates",candidates);
    }

    return ret;
}

UniValue getaggregationfee(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() != 0)
        throw std::runtime_error(
                "getaggregationfee\n"
                "Shows the fee this node will ask for mixing its coins\n"
                );

    return GetArg("-aggregationfee", DEFAULT_MIX_FEE);
}

UniValue getaggregationmaxfee(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() != 0)
        throw std::runtime_error(
                "getaggregationmaxfee\n"
                "Shows the maximum fee this node will pay for mixing coins\n"
                );

    return GetArg("-aggregationmaxfee", DEFAULT_MAX_MIX_FEE);
}

UniValue setaggregationfee(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() != 1 || !params[0].isNum())
        throw std::runtime_error(
                "setaggregationfee <fee>\n"
                "Sets the fee this node will ask for mixing its coins\n"
                );

    std::string fee = params[0].get_str();

    SoftSetArg("-aggregationfee", fee, true);
    RemoveConfigFile("aggregationfee");
    WriteConfigFile("aggregationfee", fee);

    return true;
}

UniValue setaggregationmaxfee(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() != 1 || !params[0].isNum())
        throw std::runtime_error(
                "setaggregationmaxfee <fee>\n"
                "Sets the maximum fee this node will pay for each mixing output candidate\n"
                );

    std::string fee = params[0].get_str();

    SoftSetArg("-aggregationmaxfee", fee, true);
    RemoveConfigFile("aggregationmaxfee");
    WriteConfigFile("aggregationmaxfee", fee);

    return true;
}

UniValue dumpviewprivkey(const UniValue& params, bool fHelp)
{
    if (!EnsureWalletIsAvailable(fHelp))
        return NullUniValue;

    if (fHelp || params.size() != 0)
        throw runtime_error(
                "dumpviewprivkey\n"
                "\nReveals the current blsct view key.\n"
                "\nResult:\n"
                " \"key\"                 (string) The BLSCT private view key\n"
                "\nExamples:\n"
                + HelpExampleCli("dumpviewprivkey", "")
                );

    LOCK(pwalletMain->cs_wallet);

    //EnsureWalletIsUnlocked();

    blsctKey k;
    if (pwalletMain->GetBLSCTViewKey(k))
    {
        CNavCoinBLSCTViewKey key;
        key.SetKey(k);

        return key.ToString();
    }
    else
    {
        throw JSONRPCError(RPC_WALLET_ERROR, "Unable to retrieve BLSCT private view key");
        return NullUniValue;
    }
}

static const CRPCCommand blsctcommands[] =
{ //  category              name                        actor (function)           okSafeMode
  //  --------------------- ------------------------    -----------------------    ----------
  { "blsct",              "startaggregationsession",    &startaggregationsession,  false },
  { "blsct",              "stopaggregationsession",     &stopaggregationsession,   false },
  { "blsct",              "viewaggregationsession",     &viewaggregationsession,   false },
  { "blsct",              "setaggregationfee",          &setaggregationfee,        true  },
  { "blsct",              "setaggregationmaxfee",       &setaggregationmaxfee,     true  },
  { "blsct",              "getaggregationfee",          &getaggregationfee,        true  },
  { "blsct",              "getaggregationmaxfee",       &getaggregationmaxfee,     true  },
  { "blsct",              "dumpviewprivkey",            &dumpviewprivkey,          true  },
};

void RegisterBLSCTRPCCommands(CRPCTable &tableRPC)
{
    for (unsigned int vcidx = 0; vcidx < ARRAYLEN(blsctcommands); vcidx++)
        tableRPC.appendCommand(blsctcommands[vcidx].name, &blsctcommands[vcidx]);
}
