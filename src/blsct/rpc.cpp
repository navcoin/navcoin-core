// Copyright (c) 2020 The Navcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <base58.h>
#include <blsct/rpc.h>
#include <core_io.h>
#include <main.h>
#include <rpc/server.h>
#include <wallet/wallet.h>

bool EnsureWalletIsAvailable_(bool avoidException)
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

    if (!pwalletMain->aggSession)
        throw JSONRPCError(RPC_INTERNAL_ERROR, string("Error internal structures"));

    if (!pwalletMain->aggSession->Start())
        throw JSONRPCError(RPC_INTERNAL_ERROR, string("Could not start mix session"));

    return true;
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
    if (fHelp || params.size() > 1)
        throw std::runtime_error(
                "viewaggregationsession <show_candidates>\n"
                "Shows the active mix session if any\n"
                );

    UniValue ret(UniValue::VOBJ);

    {
        LOCK(pwalletMain->cs_wallet);
        if (!pwalletMain->aggSession)
            return ret;
        ret.pushKV("nVersion",pwalletMain->aggSession->GetVersion());
        if (pwalletMain->aggSession->GetVersion() == 1)
            ret.pushKV("hiddenService",pwalletMain->aggSession->GetHiddenService());
        else if (pwalletMain->aggSession->GetVersion() == 2)
            ret.pushKV("pubKey",pwalletMain->aggSession->GetHiddenService());
        ret.pushKV("running",pwalletMain->aggSession->GetState());
        UniValue candidates(UniValue::VARR);

        bool fShow = false;

        if (params.size() == 1)
            fShow = params[0].get_bool();

        if (fShow)
        {
            for(auto&it: pwalletMain->aggSession->GetTransactionCandidates())
            {
                UniValue obj(UniValue::VOBJ);
                obj.pushKV("hex",EncodeHexTx(it.tx));
                obj.pushKV("fee",it.fee);
                candidates.push_back(obj);
            }
            ret.pushKV("txCandidates",candidates);
        }
        ret.pushKV("txCandidatesCount", (uint64_t)pwalletMain->aggSession->GetTransactionCandidates().size());
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
    if (!EnsureWalletIsAvailable_(fHelp))
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
        CNavcoinBLSCTViewKey key;
        key.SetKey(k);

        return key.ToString();
    }
    else
    {
        throw JSONRPCError(RPC_WALLET_ERROR, "Unable to retrieve BLSCT private view key");
        return NullUniValue;
    }
}

struct blsctOutputInfo
{
    CAmount amount;
    std::string message;
    bool spent;
    uint64_t time;
};

UniValue scanviewkey(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() < 1 || params.size() > 2)
        throw runtime_error(
                "scanviewkey \"view key\" ( \"from time\")\n"
                "\nScans the blockchain using the specified BLSCT view key.\n"
                "\nArguments:\n"
                "1. \"view key\"          (string, required) The BLSCT view key\n"
                "1. \"from time\"         (integer, optional) The timestamp to start scanning at, by default it uses the current chain tip. Use 0 to scan from genesis.\n"
                "\nExamples:\n"
                + HelpExampleCli("scanviewkey", "PVpSXkdpeLSTLMNLRTKLMNDLPFTqjX4Ug34ALU8")
                );

    int64_t nTimeBegin = chainActive.Tip()->GetBlockTime();

    if (params.size() > 1)
        nTimeBegin = std::max(1601287047, params[1].get_int());

    CNavcoinBLSCTViewKey key(params[0].get_str());

    CBlockIndex *pindex = chainActive.Tip();
    while (pindex && pindex->pprev && pindex->GetBlockTime() > nTimeBegin - 7200)
        pindex = pindex->pprev;

    blsctKey vk = key.GetKey();

    if (!vk.IsValid())
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, string("Invalid view key: ")+params[0].get_str());

    UniValue ret(UniValue::VOBJ);

    std::map<COutPoint, blsctOutputInfo> mapOutputs;
    CAmount nBalance = 0;

    int64_t nLastChecked = pindex->GetBlockTime();

    {
        LOCK2(cs_main, pwalletMain->cs_wallet);

        while (pindex)
        {
            nLastChecked = pindex->GetBlockTime();

            if (!pindex->HasBLSCTTransactions())
                continue;

            CBlock block;
            ReadBlockFromDisk(block, pindex, Params().GetConsensus());

            if (pindex->nHeight % 25000 == 0)
                LogPrintf("%s: Scanning using view key. Current height: %d\n", __func__, pindex->nHeight);

            for(CTransaction& tx: block.vtx)
            {
                for (CTxIn& in: tx.vin)
                {
                    if (mapOutputs.count(in.prevout))
                    {
                        mapOutputs[in.prevout].spent = true;
                        nBalance -= mapOutputs[in.prevout].amount;
                    }
                }
                for (unsigned int i = 0; i < tx.vout.size(); i++)
                {
                    CTxOut txout = tx.vout[i];
                    if (txout.HasRangeProof() && txout.IsBLSCT())
                    {
                        std::vector<RangeproofEncodedData> blsctData;
                        std::vector<std::pair<int, BulletproofsRangeproof>> proofs;
                        std::vector<bls::G1Element> nonces;

                        if (txout.ephemeralKey.size() > 0 && txout.outputKey.size() > 0 && txout.spendingKey.size() > 0)
                        {
                            try
                            {
                                proofs.push_back(std::make_pair(0,txout.GetBulletproof()));
                                bls::G1Element t = bls::G1Element::FromByteVector(txout.outputKey);
                                bls::PrivateKey k = vk.GetKey();
                                t = t * k;

                                nonces.push_back(t);
                                bool fValidBP = VerifyBulletproof(proofs, blsctData, nonces, true);
                                if (fValidBP && blsctData.size() == 1)
                                {
                                    blsctOutputInfo info;
                                    info.amount = blsctData[0].amount;
                                    info.message = blsctData[0].message;
                                    info.spent = false;
                                    info.time = tx.nTime;
                                    nBalance += blsctData[0].amount;
                                    mapOutputs.insert(std::make_pair(COutPoint(tx.GetHash(), i), info));
                                }
                            }
                            catch(...)
                            {
                                continue;
                            }
                        }
                    }
                }
            }

            pindex = chainActive.Next(pindex);
        }
    }

    UniValue outs(UniValue::VARR);

    for (auto& it: mapOutputs)
    {
        UniValue entry(UniValue::VOBJ);
        entry.pushKV("hash", it.first.hash.ToString());
        entry.pushKV("n", (uint64_t)it.first.n);
        entry.pushKV("amount", it.second.amount);
        entry.pushKV("message", it.second.message);
        entry.pushKV("time", it.second.time);
        entry.pushKV("spent", it.second.spent);
        outs.push_back(entry);
    }

    ret.pushKV("outputs", outs);
    ret.pushKV("last_checked", nLastChecked);

    if (nTimeBegin <= 1601287047)
        ret.pushKV("balance", nBalance);

    return ret;
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
  { "blsct",              "scanviewkey",                &scanviewkey,              true  },
};

void RegisterBLSCTRPCCommands(CRPCTable &tableRPC)
{
    for (unsigned int vcidx = 0; vcidx < ARRAYLEN(blsctcommands); vcidx++)
        tableRPC.appendCommand(blsctcommands[vcidx].name, &blsctcommands[vcidx]);
}
