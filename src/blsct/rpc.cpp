// Copyright (c) 2020 The NavCoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <blsct/rpc.h>
#include <core_io.h>
#include <rpc/server.h>
#include <wallet/wallet.h>

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

static const CRPCCommand blsctcommands[] =
{ //  category              name                        actor (function)           okSafeMode
  //  --------------------- ------------------------    -----------------------    ----------
  { "blsct",              "startaggregationsession",    &startaggregationsession,  false },
  { "blsct",              "stopaggregationsession",     &stopaggregationsession,   false },
  { "blsct",              "viewaggregationsession",     &viewaggregationsession,   false },
};

void RegisterBLSCTRPCCommands(CRPCTable &tableRPC)
{
    for (unsigned int vcidx = 0; vcidx < ARRAYLEN(blsctcommands); vcidx++)
        tableRPC.appendCommand(blsctcommands[vcidx].name, &blsctcommands[vcidx]);
}
