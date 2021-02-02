// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "amount.h"
#include "base58.h"
#include "chain.h"
#include "chainparams.h"
#include "checkpoints.h"
#include "coins.h"
#include "consensus/validation.h"
#include "consensus/daoconsensusparams.h"
#include "main.h"
#include "policy/policy.h"
#include "primitives/transaction.h"
#include "rpc/server.h"
#include "script/script.h"
#include "script/script_error.h"
#include "script/sign.h"
#include "script/standard.h"
#include "streams.h"
#include "sync.h"
#include "txmempool.h"
#include "util.h"
#include "utilstrencodings.h"
#include "hash.h"
#include "pos.h"

#include <stdint.h>

#include <univalue.h>

#include <boost/thread/thread.hpp> // boost::thread::interrupt

using namespace std;

extern void TxToJSON(const CTransaction& tx, const uint256 hashBlock, UniValue& entry);
void ScriptPubKeyToJSON(const CScript& scriptPubKey, UniValue& out, bool fIncludeHex);

UniValue blockheaderToJSON(const CBlockIndex* blockindex)
{
    UniValue result(UniValue::VOBJ);
    result.pushKV("hash", blockindex->GetBlockHash().GetHex());
    int confirmations = -1;
    // Only report confirmations if the block is on the main chain
    if (chainActive.Contains(blockindex))
        confirmations = chainActive.Height() - blockindex->nHeight + 1;
    result.pushKV("confirmations", confirmations);
    result.pushKV("height", blockindex->nHeight);
    result.pushKV("version", blockindex->nVersion);
    result.pushKV("versionHex", strprintf("%08x", blockindex->nVersion));
    result.pushKV("merkleroot", blockindex->hashMerkleRoot.GetHex());
    result.pushKV("time", (int64_t)blockindex->nTime);
    result.pushKV("mediantime", (int64_t)blockindex->GetMedianTimePast());
    result.pushKV("mint", ValueFromAmount(blockindex->nMint));
    result.pushKV("nonce", (uint64_t)blockindex->nNonce);
    result.pushKV("bits", strprintf("%08x", blockindex->nBits));
    result.pushKV("difficulty", GetDifficulty(blockindex));
    result.pushKV("chainwork", blockindex->nChainWork.GetHex());
    result.pushKV("ncfsupply", FormatMoney(blockindex->nCFSupply));
    result.pushKV("ncflocked", FormatMoney(blockindex->nCFLocked));
    result.pushKV("flags", strprintf("%s%s", blockindex->IsProofOfStake()? "proof-of-stake" : "proof-of-work", blockindex->GeneratedStakeModifier()? " stake-modifier": ""));
    result.pushKV("proofhash", blockindex->hashProof.GetHex());
    result.pushKV("entropybit", (int)blockindex->GetStakeEntropyBit());
    result.pushKV("modifier", strprintf("%016x", blockindex->nStakeModifier));

    UniValue votes(UniValue::VARR);
    auto pVotes = GetProposalVotes(blockindex->GetBlockHash());
    if (pVotes != nullptr)
    {
        for (auto& it: *pVotes)
        {
            UniValue entry(UniValue::VOBJ);
            entry.pushKV("hash", it.first.ToString());
            entry.pushKV("vote", it.second);
            votes.push_back(entry);
        }
    }
    result.pushKV("cfund_votes", votes);

    UniValue votes_pr(UniValue::VARR);
    auto prVotes = GetPaymentRequestVotes(blockindex->GetBlockHash());
    if (prVotes != nullptr)
    {
        for (auto& it: *prVotes)
        {
            UniValue entry(UniValue::VOBJ);
            entry.pushKV("hash", it.first.ToString());
            entry.pushKV("vote", it.second);
            votes_pr.push_back(entry);
        }
    }
    result.pushKV("cfund_request_votes", votes_pr);

    UniValue daosupport(UniValue::VARR);
    auto supp = GetSupport(blockindex->GetBlockHash());
    if (supp != nullptr)
    {
        for (auto& it: *supp)
        {
            daosupport.push_back(it.first.ToString());
        }
    }
    result.pushKV("dao_support", daosupport);

    UniValue daovotes(UniValue::VARR);
    auto cVotes = GetConsultationVotes(blockindex->GetBlockHash());
    if (cVotes != nullptr)
    {
        for (auto& it: *cVotes)
        {
            UniValue entry(UniValue::VOBJ);
            entry.pushKV("hash", it.first.ToString());
            entry.pushKV("vote", it.second);
            daovotes.push_back(entry);
        }
    }
    result.pushKV("dao_votes", daovotes);

    if (blockindex->pprev)
        result.pushKV("previousblockhash", blockindex->pprev->GetBlockHash().GetHex());
    CBlockIndex *pnext = chainActive.Next(blockindex);
    if (pnext)
        result.pushKV("nextblockhash", pnext->GetBlockHash().GetHex());

    return result;
}

UniValue blockToDeltasJSON(const CBlock& block, const CBlockIndex* blockindex)
{
    UniValue result(UniValue::VOBJ);
    result.pushKV("hash", block.GetHash().GetHex());
    int confirmations = -1;
    // Only report confirmations if the block is on the main chain
    if (chainActive.Contains(blockindex)) {
        confirmations = chainActive.Height() - blockindex->nHeight + 1;
    } else {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Block is an orphan");
    }
    result.pushKV("confirmations", confirmations);
    result.pushKV("size", (int)::GetSerializeSize(block, SER_NETWORK, PROTOCOL_VERSION));
    result.pushKV("height", blockindex->nHeight);
    result.pushKV("version", block.nVersion);
    result.pushKV("merkleroot", block.hashMerkleRoot.GetHex());
    if (block.IsProofOfStake())
        result.pushKV("signature", HexStr(block.vchBlockSig.begin(), block.vchBlockSig.end()));

    UniValue deltas(UniValue::VARR);

    for (unsigned int i = 0; i < block.vtx.size(); i++) {
        const CTransaction &tx = block.vtx[i];
        const uint256 txhash = tx.GetHash();

        UniValue entry(UniValue::VOBJ);
        entry.pushKV("txid", txhash.GetHex());
        entry.pushKV("index", (int)i);

        UniValue inputs(UniValue::VARR);

        if (!tx.IsCoinBase()) {

            for (size_t j = 0; j < tx.vin.size(); j++) {
                const CTxIn input = tx.vin[j];

                UniValue delta(UniValue::VOBJ);

                CSpentIndexValue spentInfo;
                CSpentIndexKey spentKey(input.prevout.hash, input.prevout.n);

                if (GetSpentIndex(spentKey, spentInfo)) {
                    if (spentInfo.addressType == 1) {
                        delta.pushKV("address", CNavCoinAddress(CKeyID(spentInfo.addressHash)).ToString());
                    } else if (spentInfo.addressType == 2)  {
                        delta.pushKV("address", CNavCoinAddress(CScriptID(spentInfo.addressHash)).ToString());
                    } else {
                        continue;
                    }
                    delta.pushKV("satoshis", -1 * spentInfo.satoshis);
                    delta.pushKV("index", (int)j);
                    delta.pushKV("prevtxid", input.prevout.hash.GetHex());
                    delta.pushKV("prevout", (int)input.prevout.n);

                    inputs.push_back(delta);
                } else {
                    throw JSONRPCError(RPC_INTERNAL_ERROR, "Spent information not available");
                }

            }
        }

        entry.pushKV("inputs", inputs);

        UniValue outputs(UniValue::VARR);

        for (unsigned int k = 0; k < tx.vout.size(); k++) {
            const CTxOut &out = tx.vout[k];

            UniValue delta(UniValue::VOBJ);

            if (out.scriptPubKey.IsPayToScriptHash()) {
                vector<unsigned char> hashBytes(out.scriptPubKey.begin()+2, out.scriptPubKey.begin()+22);
                delta.pushKV("address", CNavCoinAddress(CScriptID(uint160(hashBytes))).ToString());

            } else if (out.scriptPubKey.IsPayToPublicKeyHash()) {
                vector<unsigned char> hashBytes(out.scriptPubKey.begin()+3, out.scriptPubKey.begin()+23);
                delta.pushKV("address", CNavCoinAddress(CKeyID(uint160(hashBytes))).ToString());
            } else {
                continue;
            }

            delta.pushKV("satoshis", out.nValue);
            delta.pushKV("index", (int)k);

            outputs.push_back(delta);
        }

        entry.pushKV("outputs", outputs);
        deltas.push_back(entry);

    }
    result.pushKV("deltas", deltas);
    result.pushKV("time", block.GetBlockTime());
    result.pushKV("mediantime", (int64_t)blockindex->GetMedianTimePast());
    result.pushKV("nonce", (uint64_t)block.nNonce);
    result.pushKV("bits", strprintf("%08x", block.nBits));
    result.pushKV("difficulty", GetDifficulty(blockindex));
    result.pushKV("chainwork", blockindex->nChainWork.GetHex());

    if (blockindex->pprev)
        result.pushKV("previousblockhash", blockindex->pprev->GetBlockHash().GetHex());
    CBlockIndex *pnext = chainActive.Next(blockindex);
    if (pnext)
        result.pushKV("nextblockhash", pnext->GetBlockHash().GetHex());
    return result;
}

UniValue blockToJSON(const CBlock& block, const CBlockIndex* blockindex, bool txDetails = false)
{
    UniValue result(UniValue::VOBJ);
    result.pushKV("hash", blockindex->GetBlockHash().GetHex());
    int confirmations = -1;
    // Only report confirmations if the block is on the main chain
    if (chainActive.Contains(blockindex))
        confirmations = chainActive.Height() - blockindex->nHeight + 1;
    result.pushKV("confirmations", confirmations);
    result.pushKV("strippedsize", (int)::GetSerializeSize(block, SER_NETWORK, PROTOCOL_VERSION | SERIALIZE_TRANSACTION_NO_WITNESS));
    result.pushKV("size", (int)::GetSerializeSize(block, SER_NETWORK, PROTOCOL_VERSION));
    result.pushKV("weight", (int)::GetBlockWeight(block));
    result.pushKV("height", blockindex->nHeight);
    result.pushKV("version", block.nVersion);
    result.pushKV("versionHex", strprintf("%08x", block.nVersion));
    result.pushKV("merkleroot", block.hashMerkleRoot.GetHex());
    UniValue txs(UniValue::VARR);
    int nCountProposalVotes = 0;
    int nCountPaymentRequestVotes = 0;
    int nCountPaymentRequestPayouts = 0;
    int nCountProposals = 0;
    int nCountPaymentRequests = 0;
    int nCountTransactions = 0;
    CAmount nValueTransactions = 0;

    CAmount nPOWBlockReward = block.IsProofOfWork() ? GetBlockSubsidy(blockindex->nHeight, Params().GetConsensus()) : 0;

    for(const CTransaction&tx: block.vtx)
    {
        if(txDetails)
        {
            UniValue objTx(UniValue::VOBJ);
            TxToJSON(tx, uint256(), objTx);
            txs.push_back(objTx);
        }
        else
            txs.push_back(tx.GetHash().GetHex());
        if (tx.IsCoinBase())
        {
            CAmount nAccValue = 0;
            for (auto&it: tx.vout)
            {
                nAccValue += it.nValue;
                if (it.IsProposalVote())
                {
                    nCountProposalVotes++;
                }
                else if(it.IsPaymentRequestVote())
                {
                    nCountPaymentRequestVotes++;
                }
                if(nAccValue > nPOWBlockReward && it.nValue > 0)
                {
                    nCountPaymentRequestPayouts++;
                }
            }
        }
        else if(tx.IsCoinStake())
        {

        }
        else
        {
            if ((tx.nVersion&0xF) == CTransaction::PROPOSAL_VERSION)
                nCountProposals++;
            else if((tx.nVersion&0xF) == CTransaction::PAYMENT_REQUEST_VERSION)
                nCountPaymentRequests++;
            else
            {
                nCountTransactions++;
                for (auto &out: tx.vout)
                {
                    nValueTransactions += out.nValue;
                }
            }
        }

    }
    result.pushKV("tx", txs);
    result.pushKV("tx_count", nCountTransactions);
    result.pushKV("tx_value_count", nValueTransactions);
    result.pushKV("proposal_count", nCountProposals);
    result.pushKV("payment_request_count", nCountPaymentRequests);
    result.pushKV("proposal_votes_count", nCountProposalVotes);
    result.pushKV("payment_request_votes_count", nCountPaymentRequestVotes);
    result.pushKV("payment_request_payouts_count", nCountPaymentRequestPayouts);
    result.pushKV("time", block.GetBlockTime());
    result.pushKV("mediantime", (int64_t)blockindex->GetMedianTimePast());
    result.pushKV("nonce", (uint64_t)block.nNonce);
    result.pushKV("bits", strprintf("%08x", block.nBits));
    result.pushKV("difficulty", GetDifficulty(blockindex));
    result.pushKV("chainwork", blockindex->nChainWork.GetHex());

    if (blockindex->pprev)
        result.pushKV("previousblockhash", blockindex->pprev->GetBlockHash().GetHex());
    CBlockIndex *pnext = chainActive.Next(blockindex);
    if (pnext)
        result.pushKV("nextblockhash", pnext->GetBlockHash().GetHex());
    return result;
}

UniValue getblockcount(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() != 0)
        throw runtime_error(
                "getblockcount\n"
                "\nReturns the number of blocks in the longest block chain.\n"
                "\nResult:\n"
                "n    (numeric) The current block count\n"
                "\nExamples:\n"
                + HelpExampleCli("getblockcount", "")
                + HelpExampleRpc("getblockcount", "")
                );

    LOCK(cs_main);
    return chainActive.Height();
}

UniValue getbestblockhash(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() != 0)
        throw runtime_error(
                "getbestblockhash\n"
                "\nReturns the hash of the best (tip) block in the longest block chain.\n"
                "\nResult\n"
                "\"hex\"      (string) the block hash hex encoded\n"
                "\nExamples\n"
                + HelpExampleCli("getbestblockhash", "")
                + HelpExampleRpc("getbestblockhash", "")
                );

    LOCK(cs_main);
    return chainActive.Tip()->GetBlockHash().GetHex();
}

UniValue getdifficulty(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() != 0)
        throw runtime_error(
                "getdifficulty\n"
                "\nReturns the proof-of-work difficulty as a multiple of the minimum difficulty.\n"
                "\nResult:\n"
                "n.nnn       (numeric) the proof-of-work difficulty as a multiple of the minimum difficulty.\n"
                "\nExamples:\n"
                + HelpExampleCli("getdifficulty", "")
                + HelpExampleRpc("getdifficulty", "")
                );

    LOCK(cs_main);
    return GetDifficulty();
}

std::string EntryDescriptionString()
{
    return "    \"size\" : n,             (numeric) transaction size in bytes\n"
           "    \"fee\" : n,              (numeric) transaction fee in " + CURRENCY_UNIT + "\n"
                                                                                           "    \"modifiedfee\" : n,      (numeric) transaction fee with fee deltas used for mining priority\n"
                                                                                           "    \"time\" : n,             (numeric) local time transaction entered pool in seconds since 1 Jan 1970 GMT\n"
                                                                                           "    \"height\" : n,           (numeric) block height when transaction entered pool\n"
                                                                                           "    \"startingpriority\" : n, (numeric) priority when transaction entered pool\n"
                                                                                           "    \"currentpriority\" : n,  (numeric) transaction priority now\n"
                                                                                           "    \"descendantcount\" : n,  (numeric) number of in-mempool descendant transactions (including this one)\n"
                                                                                           "    \"descendantsize\" : n,   (numeric) size of in-mempool descendants (including this one)\n"
                                                                                           "    \"descendantfees\" : n,   (numeric) modified fees (see above) of in-mempool descendants (including this one)\n"
                                                                                           "    \"ancestorcount\" : n,    (numeric) number of in-mempool ancestor transactions (including this one)\n"
                                                                                           "    \"ancestorsize\" : n,     (numeric) size of in-mempool ancestors (including this one)\n"
                                                                                           "    \"ancestorfees\" : n,     (numeric) modified fees (see above) of in-mempool ancestors (including this one)\n"
                                                                                           "    \"depends\" : [           (array) unconfirmed transactions used as inputs for this transaction\n"
                                                                                           "        \"transactionid\",    (string) parent transaction id\n"
                                                                                           "       ... ]\n";
}

void entryToJSON(UniValue &info, const CTxMemPoolEntry &e)
{
    AssertLockHeld(mempool.cs);

    info.pushKV("size", (int)e.GetTxSize());
    info.pushKV("fee", ValueFromAmount(e.GetFee()));
    info.pushKV("modifiedfee", ValueFromAmount(e.GetModifiedFee()));
    info.pushKV("time", e.GetTime());
    info.pushKV("height", (int)e.GetHeight());
    info.pushKV("startingpriority", e.GetPriority(e.GetHeight()));
    info.pushKV("currentpriority", e.GetPriority(chainActive.Height()));
    info.pushKV("descendantcount", e.GetCountWithDescendants());
    info.pushKV("descendantsize", e.GetSizeWithDescendants());
    info.pushKV("descendantfees", e.GetModFeesWithDescendants());
    info.pushKV("ancestorcount", e.GetCountWithAncestors());
    info.pushKV("ancestorsize", e.GetSizeWithAncestors());
    info.pushKV("ancestorfees", e.GetModFeesWithAncestors());
    const CTransaction& tx = e.GetTx();
    set<string> setDepends;
    for(const CTxIn& txin: tx.vin)
    {
        if (mempool.exists(txin.prevout.hash))
            setDepends.insert(txin.prevout.hash.ToString());
    }

    UniValue depends(UniValue::VARR);
    for(const string& dep: setDepends)
    {
        depends.push_back(dep);
    }

    info.pushKV("depends", depends);
}

UniValue mempoolToJSON(bool fVerbose = false)
{
    if (fVerbose)
    {
        LOCK(mempool.cs);
        UniValue o(UniValue::VOBJ);
        for(const CTxMemPoolEntry& e: mempool.mapTx)
        {
            const uint256& hash = e.GetTx().GetHash();
            UniValue info(UniValue::VOBJ);
            entryToJSON(info, e);
            o.pushKV(hash.ToString(), info);
        }
        return o;
    }
    else
    {
        vector<uint256> vtxid;
        mempool.queryHashes(vtxid);

        UniValue a(UniValue::VARR);
        for(const uint256& hash: vtxid)
            a.push_back(hash.ToString());

        return a;
    }
}

UniValue getrawmempool(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() > 1)
        throw runtime_error(
                "getrawmempool ( verbose )\n"
                "\nReturns all transaction ids in memory pool as a json array of string transaction ids.\n"
                "\nArguments:\n"
                "1. verbose           (boolean, optional, default=false) true for a json object, false for array of transaction ids\n"
                "\nResult: (for verbose = false):\n"
                "[                     (json array of string)\n"
                "  \"transactionid\"     (string) The transaction id\n"
                "  ,...\n"
                "]\n"
                "\nResult: (for verbose = true):\n"
                "{                           (json object)\n"
                "  \"transactionid\" : {       (json object)\n"
                + EntryDescriptionString()
                + "  }, ...\n"
                  "}\n"
                  "\nExamples\n"
                + HelpExampleCli("getrawmempool", "true")
                + HelpExampleRpc("getrawmempool", "true")
                );

    bool fVerbose = false;
    if (params.size() > 0)
        fVerbose = params[0].get_bool();

    return mempoolToJSON(fVerbose);
}

UniValue getmempoolancestors(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() < 1 || params.size() > 2) {
        throw runtime_error(
                    "getmempoolancestors txid (verbose)\n"
                    "\nIf txid is in the mempool, returns all in-mempool ancestors.\n"
                    "\nArguments:\n"
                    "1. \"txid\"                   (string, required) The transaction id (must be in mempool)\n"
                    "2. verbose                  (boolean, optional, default=false) true for a json object, false for array of transaction ids\n"
                    "\nResult (for verbose=false):\n"
                    "[                       (json array of strings)\n"
                    "  \"transactionid\"           (string) The transaction id of an in-mempool ancestor transaction\n"
                    "  ,...\n"
                    "]\n"
                    "\nResult (for verbose=true):\n"
                    "{                           (json object)\n"
                    "  \"transactionid\" : {       (json object)\n"
                    + EntryDescriptionString()
                    + "  }, ...\n"
                      "}\n"
                      "\nExamples\n"
                    + HelpExampleCli("getmempoolancestors", "\"mytxid\"")
                    + HelpExampleRpc("getmempoolancestors", "\"mytxid\"")
                    );
    }

    bool fVerbose = false;
    if (params.size() > 1)
        fVerbose = params[1].get_bool();

    uint256 hash = ParseHashV(params[0], "parameter 1");

    LOCK(mempool.cs);

    CTxMemPool::txiter it = mempool.mapTx.find(hash);
    if (it == mempool.mapTx.end()) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Transaction not in mempool");
    }

    CTxMemPool::setEntries setAncestors;
    uint64_t noLimit = std::numeric_limits<uint64_t>::max();
    std::string dummy;
    mempool.CalculateMemPoolAncestors(*it, setAncestors, noLimit, noLimit, noLimit, noLimit, dummy, false);

    if (!fVerbose) {
        UniValue o(UniValue::VARR);
        for(CTxMemPool::txiter ancestorIt: setAncestors) {
            o.push_back(ancestorIt->GetTx().GetHash().ToString());
        }

        return o;
    } else {
        UniValue o(UniValue::VOBJ);
        for(CTxMemPool::txiter ancestorIt: setAncestors) {
            const CTxMemPoolEntry &e = *ancestorIt;
            const uint256& hash = e.GetTx().GetHash();
            UniValue info(UniValue::VOBJ);
            entryToJSON(info, e);
            o.pushKV(hash.ToString(), info);
        }
        return o;
    }
}

UniValue getmempooldescendants(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() < 1 || params.size() > 2) {
        throw runtime_error(
                    "getmempooldescendants txid (verbose)\n"
                    "\nIf txid is in the mempool, returns all in-mempool descendants.\n"
                    "\nArguments:\n"
                    "1. \"txid\"                   (string, required) The transaction id (must be in mempool)\n"
                    "2. verbose                  (boolean, optional, default=false) true for a json object, false for array of transaction ids\n"
                    "\nResult (for verbose=false):\n"
                    "[                       (json array of strings)\n"
                    "  \"transactionid\"           (string) The transaction id of an in-mempool descendant transaction\n"
                    "  ,...\n"
                    "]\n"
                    "\nResult (for verbose=true):\n"
                    "{                           (json object)\n"
                    "  \"transactionid\" : {       (json object)\n"
                    + EntryDescriptionString()
                    + "  }, ...\n"
                      "}\n"
                      "\nExamples\n"
                    + HelpExampleCli("getmempooldescendants", "\"mytxid\"")
                    + HelpExampleRpc("getmempooldescendants", "\"mytxid\"")
                    );
    }

    bool fVerbose = false;
    if (params.size() > 1)
        fVerbose = params[1].get_bool();

    uint256 hash = ParseHashV(params[0], "parameter 1");

    LOCK(mempool.cs);

    CTxMemPool::txiter it = mempool.mapTx.find(hash);
    if (it == mempool.mapTx.end()) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Transaction not in mempool");
    }

    CTxMemPool::setEntries setDescendants;
    mempool.CalculateDescendants(it, setDescendants);
    // CTxMemPool::CalculateDescendants will include the given tx
    setDescendants.erase(it);

    if (!fVerbose) {
        UniValue o(UniValue::VARR);
        for(CTxMemPool::txiter descendantIt: setDescendants) {
            o.push_back(descendantIt->GetTx().GetHash().ToString());
        }

        return o;
    } else {
        UniValue o(UniValue::VOBJ);
        for(CTxMemPool::txiter descendantIt: setDescendants) {
            const CTxMemPoolEntry &e = *descendantIt;
            const uint256& hash = e.GetTx().GetHash();
            UniValue info(UniValue::VOBJ);
            entryToJSON(info, e);
            o.pushKV(hash.ToString(), info);
        }
        return o;
    }
}

UniValue getmempoolentry(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() != 1) {
        throw runtime_error(
                    "getmempoolentry txid\n"
                    "\nReturns mempool data for given transaction\n"
                    "\nArguments:\n"
                    "1. \"txid\"                   (string, required) The transaction id (must be in mempool)\n"
                    "\nResult:\n"
                    "{                           (json object)\n"
                    + EntryDescriptionString()
                    + "}\n"
                      "\nExamples\n"
                    + HelpExampleCli("getmempoolentry", "\"mytxid\"")
                    + HelpExampleRpc("getmempoolentry", "\"mytxid\"")
                    );
    }

    uint256 hash = ParseHashV(params[0], "parameter 1");

    LOCK(mempool.cs);

    CTxMemPool::txiter it = mempool.mapTx.find(hash);
    if (it == mempool.mapTx.end()) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Transaction not in mempool");
    }

    const CTxMemPoolEntry &e = *it;
    UniValue info(UniValue::VOBJ);
    entryToJSON(info, e);
    return info;
}

UniValue getblockdeltas(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() != 1)
        throw runtime_error("");

    std::string strHash = params[0].get_str();
    uint256 hash(uint256S(strHash));

    if (mapBlockIndex.count(hash) == 0)
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Block not found");

    CBlock block;
    CBlockIndex* pblockindex = mapBlockIndex[hash];

    if (fHavePruned && !(pblockindex->nStatus & BLOCK_HAVE_DATA) && pblockindex->nTx > 0)
        throw JSONRPCError(RPC_INTERNAL_ERROR, "Block not available (pruned data)");

    if(!ReadBlockFromDisk(block, pblockindex, Params().GetConsensus()))
        throw JSONRPCError(RPC_INTERNAL_ERROR, "Can't read block from disk");

    return blockToDeltasJSON(block, pblockindex);
}

UniValue getblockhashes(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() < 2)
        throw runtime_error(
                "getblockhashes timestamp\n"
                "\nReturns array of hashes of blocks within the timestamp range provided.\n"
                "\nArguments:\n"
                "1. high         (numeric, required) The newer block timestamp\n"
                "2. low          (numeric, required) The older block timestamp\n"
                "3. options      (string, required) A json object\n"
                "    {\n"
                "      \"noOrphans\":true   (boolean) will only include blocks on the main chain\n"
                "      \"logicalTimes\":true   (boolean) will include logical timestamps with hashes\n"
                "    }\n"
                "\nResult:\n"
                "[\n"
                "  \"hash\"         (string) The block hash\n"
                "]\n"
                "[\n"
                "  {\n"
                "    \"blockhash\": (string) The block hash\n"
                "    \"logicalts\": (numeric) The logical timestamp\n"
                "  }\n"
                "]\n"
                "\nExamples:\n"
                + HelpExampleCli("getblockhashes", "1231614698 1231024505")
                + HelpExampleRpc("getblockhashes", "1231614698, 1231024505")
                + HelpExampleCli("getblockhashes", "1231614698 1231024505 '{\"noOrphans\":false, \"logicalTimes\":true}'")
                );

    unsigned int high = params[0].get_int();
    unsigned int low = params[1].get_int();
    bool fActiveOnly = false;
    bool fLogicalTS = false;

    if (params.size() > 2) {
        if (params[2].isObject()) {
            UniValue noOrphans = find_value(params[2].get_obj(), "noOrphans");
            UniValue returnLogical = find_value(params[2].get_obj(), "logicalTimes");

            if (noOrphans.isBool())
                fActiveOnly = noOrphans.get_bool();

            if (returnLogical.isBool())
                fLogicalTS = returnLogical.get_bool();
        }
    }

    std::vector<std::pair<uint256, unsigned int> > blockHashes;

    if (fActiveOnly)
        LOCK(cs_main);

    if (!GetTimestampIndex(high, low, fActiveOnly, blockHashes)) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "No information available for block hashes");
    }

    UniValue result(UniValue::VARR);

    for (std::vector<std::pair<uint256, unsigned int> >::const_iterator it=blockHashes.begin(); it!=blockHashes.end(); it++) {
        if (fLogicalTS) {
            UniValue item(UniValue::VOBJ);
            item.pushKV("blockhash", it->first.GetHex());
            item.pushKV("logicalts", (int)it->second);
            result.push_back(item);
        } else {
            result.push_back(it->first.GetHex());
        }
    }

    return result;
}

UniValue getblockhash(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() != 1)
        throw runtime_error(
                "getblockhash index\n"
                "\nReturns hash of block in best-block-chain at index provided.\n"
                "\nArguments:\n"
                "1. index         (numeric, required) The block index\n"
                "\nResult:\n"
                "\"hash\"         (string) The block hash\n"
                "\nExamples:\n"
                + HelpExampleCli("getblockhash", "1000")
                + HelpExampleRpc("getblockhash", "1000")
                );

    LOCK(cs_main);

    int nHeight = params[0].get_int();
    if (nHeight < 0 || nHeight > chainActive.Height())
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Block height out of range");

    CBlockIndex* pblockindex = chainActive[nHeight];
    return pblockindex->GetBlockHash().GetHex();
}

UniValue getblockheader(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() < 1 || params.size() > 2)
        throw runtime_error(
                "getblockheader \"hash\" ( verbose )\n"
                "\nIf verbose is false, returns a string that is serialized, hex-encoded data for blockheader 'hash'.\n"
                "If verbose is true, returns an Object with information about blockheader <hash>.\n"
                "\nArguments:\n"
                "1. \"hash\"          (string, required) The block hash\n"
                "2. verbose           (boolean, optional, default=true) true for a json object, false for the hex encoded data\n"
                "\nResult (for verbose = true):\n"
                "{\n"
                "  \"hash\" : \"hash\",     (string) the block hash (same as provided)\n"
                "  \"confirmations\" : n,   (numeric) The number of confirmations, or -1 if the block is not on the main chain\n"
                "  \"height\" : n,          (numeric) The block height or index\n"
                "  \"version\" : n,         (numeric) The block version\n"
                "  \"versionHex\" : \"00000000\", (string) The block version formatted in hexadecimal\n"
                "  \"merkleroot\" : \"xxxx\", (string) The merkle root\n"
                "  \"time\" : ttt,          (numeric) The block time in seconds since epoch (Jan 1 1970 GMT)\n"
                "  \"mediantime\" : ttt,    (numeric) The median block time in seconds since epoch (Jan 1 1970 GMT)\n"
                "  \"nonce\" : n,           (numeric) The nonce\n"
                "  \"bits\" : \"1d00ffff\", (string) The bits\n"
                "  \"difficulty\" : x.xxx,  (numeric) The difficulty\n"
                "  \"previousblockhash\" : \"hash\",  (string) The hash of the previous block\n"
                "  \"nextblockhash\" : \"hash\",      (string) The hash of the next block\n"
                "  \"chainwork\" : \"0000...1f3\"     (string) Expected number of hashes required to produce the current chain (in hex)\n"
                "}\n"
                "\nResult (for verbose=false):\n"
                "\"data\"             (string) A string that is serialized, hex-encoded data for block 'hash'.\n"
                "\nExamples:\n"
                + HelpExampleCli("getblockheader", "\"00000000c937983704a73af28acdec37b049d214adbda81d7e2a3dd146f6ed09\"")
                + HelpExampleRpc("getblockheader", "\"00000000c937983704a73af28acdec37b049d214adbda81d7e2a3dd146f6ed09\"")
                );

    LOCK(cs_main);

    std::string strHash = params[0].get_str();
    uint256 hash(uint256S(strHash));

    bool fVerbose = true;
    if (params.size() > 1)
        fVerbose = params[1].get_bool();

    if (mapBlockIndex.count(hash) == 0)
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Block not found");

    CBlockIndex* pblockindex = mapBlockIndex[hash];

    if (!fVerbose)
    {
        CDataStream ssBlock(SER_NETWORK, PROTOCOL_VERSION);
        ssBlock << pblockindex->GetBlockHeader();
        std::string strHex = HexStr(ssBlock.begin(), ssBlock.end());
        return strHex;
    }

    return blockheaderToJSON(pblockindex);
}

UniValue getblock(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() < 1 || params.size() > 2)
        throw runtime_error(
                "getblock \"hash\" ( verbose )\n"
                "\nIf verbose is false, returns a string that is serialized, hex-encoded data for block 'hash'.\n"
                "If verbose is true, returns an Object with information about block <hash>.\n"
                "\nArguments:\n"
                "1. \"hash\"          (string, required) The block hash\n"
                "2. verbose           (boolean, optional, default=true) true for a json object, false for the hex encoded data\n"
                "\nResult (for verbose = true):\n"
                "{\n"
                "  \"hash\" : \"hash\",     (string) the block hash (same as provided)\n"
                "  \"confirmations\" : n,   (numeric) The number of confirmations, or -1 if the block is not on the main chain\n"
                "  \"size\" : n,            (numeric) The block size\n"
                "  \"strippedsize\" : n,    (numeric) The block size excluding witness data\n"
                "  \"weight\" : n           (numeric) The block weight (BIP 141)\n"
                "  \"height\" : n,          (numeric) The block height or index\n"
                "  \"version\" : n,         (numeric) The block version\n"
                "  \"versionHex\" : \"00000000\", (string) The block version formatted in hexadecimal\n"
                "  \"merkleroot\" : \"xxxx\", (string) The merkle root\n"
                "  \"tx\" : [               (array of string) The transaction ids\n"
                "     \"transactionid\"     (string) The transaction id\n"
                "     ,...\n"
                "  ],\n"
                "  \"time\" : ttt,          (numeric) The block time in seconds since epoch (Jan 1 1970 GMT)\n"
                "  \"mediantime\" : ttt,    (numeric) The median block time in seconds since epoch (Jan 1 1970 GMT)\n"
                "  \"nonce\" : n,           (numeric) The nonce\n"
                "  \"bits\" : \"1d00ffff\", (string) The bits\n"
                "  \"difficulty\" : x.xxx,  (numeric) The difficulty\n"
                "  \"chainwork\" : \"xxxx\",  (string) Expected number of hashes required to produce the chain up to this block (in hex)\n"
                "  \"previousblockhash\" : \"hash\",  (string) The hash of the previous block\n"
                "  \"nextblockhash\" : \"hash\"       (string) The hash of the next block\n"
                "}\n"
                "\nResult (for verbose=false):\n"
                "\"data\"             (string) A string that is serialized, hex-encoded data for block 'hash'.\n"
                "\nExamples:\n"
                + HelpExampleCli("getblock", "\"00000000c937983704a73af28acdec37b049d214adbda81d7e2a3dd146f6ed09\"")
                + HelpExampleRpc("getblock", "\"00000000c937983704a73af28acdec37b049d214adbda81d7e2a3dd146f6ed09\"")
                );

    LOCK(cs_main);

    std::string strHash = params[0].get_str();
    uint256 hash(uint256S(strHash));

    bool fVerbose = true;
    if (params.size() > 1)
        fVerbose = params[1].get_bool();

    if (mapBlockIndex.count(hash) == 0)
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Block not found");

    CBlock block;
    CBlockIndex* pblockindex = mapBlockIndex[hash];

    if (fHavePruned && !(pblockindex->nStatus & BLOCK_HAVE_DATA) && pblockindex->nTx > 0)
        throw JSONRPCError(RPC_INTERNAL_ERROR, "Block not available (pruned data)");

    if(!ReadBlockFromDisk(block, pblockindex, Params().GetConsensus()))
        throw JSONRPCError(RPC_INTERNAL_ERROR, "Can't read block from disk");

    if (!fVerbose)
    {
        CDataStream ssBlock(SER_NETWORK, PROTOCOL_VERSION);
        ssBlock << block;
        std::string strHex = HexStr(ssBlock.begin(), ssBlock.end());
        return strHex;
    }

    return blockToJSON(block, pblockindex);
}

struct CCoinsStats
{
    int nHeight;
    uint256 hashBlock;
    uint64_t nTransactions;
    uint64_t nTransactionOutputs;
    uint64_t nSerializedSize;
    uint256 hashSerialized;
    CAmount nTotalAmount;
    CAmount nTotalColdAmount;
    CAmount nTotalColdv2Amount;

    CCoinsStats() : nHeight(0), nTransactions(0), nTransactionOutputs(0), nSerializedSize(0), nTotalAmount(0), nTotalColdAmount(0), nTotalColdv2Amount(0) {}
};

//! Calculate statistics about the unspent transaction output set
static bool GetUTXOStats(CStateView *view, CCoinsStats &stats)
{
    boost::scoped_ptr<CStateViewCursor> pcursor(view->Cursor());

    CHashWriter ss(SER_GETHASH, PROTOCOL_VERSION);
    stats.hashBlock = pcursor->GetBestBlock();
    {
        LOCK(cs_main);
        stats.nHeight = mapBlockIndex.find(stats.hashBlock)->second->nHeight;
    }
    ss << stats.hashBlock;
    CAmount nTotalAmount = 0;
    CAmount nTotalColdAmount = 0;
    CAmount nTotalColdv2Amount = 0;
    while (pcursor->Valid()) {
        boost::this_thread::interruption_point();
        uint256 key;
        CCoins coins;
        if (pcursor->GetKey(key) && pcursor->GetValue(coins)) {
            stats.nTransactions++;
            ss << key;
            for (unsigned int i=0; i<coins.vout.size(); i++) {
                const CTxOut &out = coins.vout[i];
                if (!out.IsNull()) {
                    stats.nTransactionOutputs++;
                    ss << VARINT(i+1);
                    ss << out;
                    nTotalAmount += out.nValue;
                    if (out.scriptPubKey.IsColdStaking()) nTotalColdAmount += out.nValue;
                    if (out.scriptPubKey.IsColdStakingv2()) nTotalColdv2Amount += out.nValue;
                }
            }
            stats.nSerializedSize += 32 + pcursor->GetValueSize();
            ss << VARINT(0);
        } else {
            return error("%s: unable to read value", __func__);
        }
        pcursor->Next();
    }
    stats.hashSerialized = ss.GetHash();
    stats.nTotalAmount = nTotalAmount;
    stats.nTotalColdAmount = nTotalColdAmount;
    stats.nTotalColdv2Amount = nTotalColdv2Amount;
    return true;
}

UniValue gettxoutsetinfo(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() != 0)
        throw runtime_error(
                "gettxoutsetinfo\n"
                "\nReturns statistics about the unspent transaction output set.\n"
                "Note this call may take some time.\n"
                "\nResult:\n"
                "{\n"
                "  \"height\":n,     (numeric) The current block height (index)\n"
                "  \"bestblock\": \"hex\",   (string) the best block hash hex\n"
                "  \"transactions\": n,      (numeric) The number of transactions\n"
                "  \"txouts\": n,            (numeric) The number of output transactions\n"
                "  \"bytes_serialized\": n,  (numeric) The serialized size\n"
                "  \"hash_serialized\": \"hash\",   (string) The serialized hash\n"
                "  \"total_amount\": x.xxx          (numeric) The total amount\n"
                "}\n"
                "\nExamples:\n"
                + HelpExampleCli("gettxoutsetinfo", "")
                + HelpExampleRpc("gettxoutsetinfo", "")
                );

    UniValue ret(UniValue::VOBJ);

    CCoinsStats stats;
    FlushStateToDisk();
    if (GetUTXOStats(pcoinsTip, stats)) {
        ret.pushKV("height", (int64_t)stats.nHeight);
        ret.pushKV("bestblock", stats.hashBlock.GetHex());
        ret.pushKV("transactions", (int64_t)stats.nTransactions);
        ret.pushKV("txouts", (int64_t)stats.nTransactionOutputs);
        ret.pushKV("bytes_serialized", (int64_t)stats.nSerializedSize);
        ret.pushKV("hash_serialized", stats.hashSerialized.GetHex());
        ret.pushKV("total_amount", ValueFromAmount(stats.nTotalAmount));
        ret.pushKV("total_cold_amount", ValueFromAmount(stats.nTotalColdAmount));
        ret.pushKV("total_coldv2_amount", ValueFromAmount(stats.nTotalColdv2Amount));
    }
    return ret;
}

UniValue getcfunddbstatehash(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() != 0)
        throw runtime_error(
                "getcfunddbstatehash\n"
                "\nReturns the hash of the Cfund DB current state.\n"
                "\nResult\n"
                "\"hex\"      (string) the hash hex encoded\n"
                "\nExamples\n"
                + HelpExampleCli("getcfunddbstatehash", "")
                + HelpExampleRpc("getcfunddbstatehash", "")
                );

    LOCK(cs_main);

    CStateViewCache view(pcoinsTip);
    return GetDAOStateHash(view, chainActive.Tip()->nCFLocked, chainActive.Tip()->nCFSupply).ToString();
}

UniValue listconsultations(const UniValue& params, bool fHelp)
{
    if (fHelp)
        throw runtime_error(
                "listconsultations \"filter\"\n"
                "\nList the consultations and all the relating data including answers and status.\n"
                "\nNote passing no argument returns all consultations regardless of state.\n"
                "\nArguments:\n"
                "\n1. \"filter\" (string, optional)   \"not_enough_answers\" | \"waiting_for_support\" | \"reflection\" | \"voting\" | \"finished\"\n"
                "\nExamples:\n"
                + HelpExampleCli("listconsultations", "finished voting")
                );

    LOCK(cs_main);

    UniValue ret(UniValue::VARR);

    bool showNotEnoughAnswers = false;
    bool showLookingForSupport = false;
    bool showReflection = false;
    bool showVoting = false;
    bool showFinished = false;
    bool showAll = true;

    if(params.size() >= 1)
    {
        for(unsigned int i = 0; i < params.size(); i++)
        {
            auto p = params[i];
            if (p.get_str() == "not_enough_answers")
            {
                showNotEnoughAnswers = true;
                showAll = false;
            }
            else if (p.get_str() == "waiting_for_support")
            {
                showLookingForSupport = true;
                showAll = false;
            }
            else if (p.get_str() == "reflection")
            {
                showReflection = true;
                showAll = false;
            }
            else if (p.get_str() == "voting")
            {
                showVoting = true;
                showAll = false;
            }
            else if (p.get_str() == "finished")
            {
                showFinished = true;
                showAll = false;
            }
        }
    }

    CConsultationMap mapConsultations;
    CStateViewCache view(pcoinsTip);

    if(view.GetAllConsultations(mapConsultations))
    {
        for (CConsultationMap::iterator it = mapConsultations.begin(); it != mapConsultations.end(); it++)
        {
            CConsultation consultation;
            if (!view.GetConsultation(it->first, consultation))
                continue;

            if (!mapBlockIndex.count(consultation.txblockhash))
                continue;

            CBlockIndex* pindex = mapBlockIndex[consultation.txblockhash];

            if (!chainActive.Contains(pindex))
                continue;

            std::vector<CConsultationAnswer> vAnswers;

            for (auto& it: consultation.vAnswers)
            {
                CConsultationAnswer answer;
                if (!view.GetConsultationAnswer(it, answer) || answer.parent != consultation.hash)
                    continue;
                vAnswers.push_back(answer);
            }

            flags fState = consultation.GetLastState();

            if((showNotEnoughAnswers && fState == DAOFlags::NIL && vAnswers.size() < 2) ||
                    (showLookingForSupport && fState == DAOFlags::NIL) ||
                    (showReflection && fState == DAOFlags::REFLECTION) ||
                    (showReflection && fState == DAOFlags::ACCEPTED) ||
                    (showFinished && fState == DAOFlags::EXPIRED) ||
                    showAll) {
                UniValue o(UniValue::VOBJ);
                consultation.ToJson(o, view);
                ret.push_back(o);
            }
        }
    }
    return ret;
}

UniValue cfundstats(const UniValue& params, bool fHelp)
{

    if (fHelp || params.size() != 0)
        throw runtime_error(
                "cfundstats\n"
                "\nReturns statistics about the community fund.\n"
                + HelpExampleCli("cfundstats", "")
                + HelpExampleRpc("cfundstats", "")
                );

    LOCK(cs_main);

    CProposal proposal; CPaymentRequest prequest;

    CStateViewCache view(pcoinsTip);

    int nBlocks = (chainActive.Tip()->nHeight % GetConsensusParameter(Consensus::CONSENSUS_PARAM_VOTING_CYCLE_LENGTH, view)) + 1;
    CBlockIndex* pindexblock = chainActive.Tip();

    std::map<uint256, bool> vSeen;
    std::map<uint256, std::pair<std::pair<int, int>, int>> vCacheProposalsRPC;
    std::map<uint256, std::pair<std::pair<int, int>, int>> vCachePaymentRequestRPC;

    vCacheProposalsRPC.clear();
    vCachePaymentRequestRPC.clear();

    while(nBlocks > 0 && pindexblock != nullptr) {
        vSeen.clear();

        auto pVotes = GetProposalVotes(pindexblock->GetBlockHash());
        if (pVotes != nullptr)
        {
            for (unsigned int i = 0; i < pVotes->size(); i++)
            {
                if(!view.GetProposal((*pVotes)[i].first, proposal))
                    continue;

                if(vSeen.count((*pVotes)[i].first) == 0)
                {
                    if(vCacheProposalsRPC.count((*pVotes)[i].first) == 0)
                        vCacheProposalsRPC[(*pVotes)[i].first] = make_pair(make_pair(0,0), 0);

                    if((*pVotes)[i].second == 1)
                        vCacheProposalsRPC[(*pVotes)[i].first].first.first += 1;
                    else if((*pVotes)[i].second == -1)
                        vCacheProposalsRPC[(*pVotes)[i].first].second += 1;
                    else if ((*pVotes)[i].second == 0)
                        vCacheProposalsRPC[(*pVotes)[i].first].first.second += 1;

                    vSeen[(*pVotes)[i].first]=true;
                }
            }
        }

        auto prVotes = GetPaymentRequestVotes(pindexblock->GetBlockHash());
        if (prVotes != nullptr)
        {
            for(unsigned int i = 0; i < prVotes->size(); i++)
            {
                if(!view.GetPaymentRequest((*prVotes)[i].first, prequest))
                    continue;

                if(!view.GetProposal(prequest.proposalhash, proposal))
                    continue;
                if(vSeen.count((*prVotes)[i].first) == 0) {
                    if(vCachePaymentRequestRPC.count((*prVotes)[i].first) == 0)
                        vCachePaymentRequestRPC[(*prVotes)[i].first] = make_pair(make_pair(0,0), 0);

                    if((*prVotes)[i].second == 1)
                        vCachePaymentRequestRPC[(*prVotes)[i].first].first.first += 1;
                    else if((*prVotes)[i].second == -1)
                        vCachePaymentRequestRPC[(*prVotes)[i].first].second += 1;
                    else if ((*prVotes)[i].second == 0)
                        vCachePaymentRequestRPC[(*prVotes)[i].first].first.second += 1;

                    vSeen[(*prVotes)[i].first]=true;
                }
            }
        }

        pindexblock = pindexblock->pprev;
        nBlocks--;
    }

    UniValue ret(UniValue::VOBJ);
    UniValue cf(UniValue::VOBJ);

    cf.pushKV("available",      ValueFromAmount(pindexBestHeader->nCFSupply));
    cf.pushKV("locked",         ValueFromAmount(pindexBestHeader->nCFLocked));
    ret.pushKV("funds", cf);

    UniValue vp(UniValue::VOBJ);
    int starting = chainActive.Tip()->nHeight - (chainActive.Tip()->nHeight % GetConsensusParameter(Consensus::CONSENSUS_PARAM_VOTING_CYCLE_LENGTH, view));

    vp.pushKV("starting",       starting);
    vp.pushKV("ending",         starting+GetConsensusParameter(Consensus::CONSENSUS_PARAM_VOTING_CYCLE_LENGTH, view)-1);
    vp.pushKV("current",        chainActive.Tip()->nHeight);

    UniValue consensus(UniValue::VOBJ);

    consensus.pushKV("blocksPerVotingCycle",GetConsensusParameter(Consensus::CONSENSUS_PARAM_VOTING_CYCLE_LENGTH, view));

    if (!IsReducedCFundQuorumEnabled(chainActive.Tip(), Params().GetConsensus()))
        consensus.pushKV("minSumVotesPerVotingCycle",GetConsensusParameter(Consensus::CONSENSUS_PARAM_VOTING_CYCLE_LENGTH, view) * GetConsensusParameter(Consensus::CONSENSUS_PARAM_PROPOSAL_MIN_QUORUM, view)/10000.0);
    else
    {
        consensus.pushKV("minSumVotesPerVotingCycle",GetConsensusParameter(Consensus::CONSENSUS_PARAM_VOTING_CYCLE_LENGTH, view) * Params().GetConsensus().nMinimumQuorumFirstHalf);
        consensus.pushKV("minSumVotesPerVotingCycleSecondHalf",GetConsensusParameter(Consensus::CONSENSUS_PARAM_VOTING_CYCLE_LENGTH, view) * Params().GetConsensus().nMinimumQuorumSecondHalf);
    }

    consensus.pushKV("maxCountVotingCycleProposals",GetConsensusParameter(Consensus::CONSENSUS_PARAM_PROPOSAL_MAX_VOTING_CYCLES, view)+1);
    consensus.pushKV("maxCountVotingCyclePaymentRequests",GetConsensusParameter(Consensus::CONSENSUS_PARAM_PAYMENT_REQUEST_MAX_VOTING_CYCLES, view)+1);
    consensus.pushKV("votesAcceptProposalPercentage",GetConsensusParameter(Consensus::CONSENSUS_PARAM_PROPOSAL_MIN_ACCEPT, view)/100);
    consensus.pushKV("votesRejectProposalPercentage",GetConsensusParameter(Consensus::CONSENSUS_PARAM_PROPOSAL_MIN_REJECT, view)/100);
    consensus.pushKV("votesAcceptPaymentRequestPercentage",GetConsensusParameter(Consensus::CONSENSUS_PARAM_PAYMENT_REQUEST_MIN_ACCEPT, view)/100);
    consensus.pushKV("votesRejectPaymentRequestPercentage",GetConsensusParameter(Consensus::CONSENSUS_PARAM_PAYMENT_REQUEST_MIN_REJECT, view)/100);
    consensus.pushKV("proposalMinimalFee",ValueFromAmount(GetConsensusParameter(Consensus::CONSENSUS_PARAM_PROPOSAL_MIN_FEE, view)));
    consensus.pushKV("paymentRequestMinimalFee",ValueFromAmount(GetConsensusParameter(Consensus::CONSENSUS_PARAM_PAYMENT_REQUEST_MIN_FEE, view)));
    ret.pushKV("consensus", consensus);

    UniValue votesProposals(UniValue::VARR);
    UniValue votesPaymentRequests(UniValue::VARR);

    std::map<uint256, std::pair<std::pair<int, int>, int>>::iterator it;

    for(auto& it: vCacheProposalsRPC)
    {
        CProposal proposal;

        if(!view.GetProposal(it.first, proposal))
            continue;

        UniValue op(UniValue::VOBJ);

        op.pushKV("str", proposal.strDZeel);
        op.pushKV("hash", proposal.hash.ToString());
        op.pushKV("amount", ValueFromAmount(proposal.nAmount));
        op.pushKV("yes", it.second.first.first);
        op.pushKV("abs", it.second.second);
        op.pushKV("no", it.second.first.second);
        votesProposals.push_back(op);
    }

    for(auto& it: vCachePaymentRequestRPC)
    {
        CPaymentRequest prequest; CProposal proposal;

        if(!view.GetPaymentRequest(it.first, prequest))
            continue;

        if(!view.GetProposal(prequest.proposalhash, proposal))
            continue;

        UniValue op(UniValue::VOBJ);
        op.pushKV("hash", prequest.hash.ToString());
        op.pushKV("proposalDesc", proposal.strDZeel);
        op.pushKV("desc", prequest.strDZeel);
        op.pushKV("amount", ValueFromAmount(prequest.nAmount));
        op.pushKV("yes", it.second.first.first);
        op.pushKV("abs", it.second.second);
        op.pushKV("no", it.second.first.second);
        votesPaymentRequests.push_back(op);
    }

    vp.pushKV("votedProposals",       votesProposals);
    vp.pushKV("votedPaymentrequests", votesPaymentRequests);
    ret.pushKV("votingPeriod", vp);

    return ret;

}

UniValue gettxout(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() < 2 || params.size() > 3)
        throw runtime_error(
                "gettxout \"txid\" n ( includemempool )\n"
                "\nReturns details about an unspent transaction output.\n"
                "\nArguments:\n"
                "1. \"txid\"       (string, required) The transaction id\n"
                "2. n              (numeric, required) vout number\n"
                "3. includemempool  (boolean, optional) Whether to include the mem pool\n"
                "\nResult:\n"
                "{\n"
                "  \"bestblock\" : \"hash\",    (string) the block hash\n"
                "  \"confirmations\" : n,       (numeric) The number of confirmations\n"
                "  \"value\" : x.xxx,           (numeric) The transaction value in " + CURRENCY_UNIT + "\n"
                                                                                                       "  \"scriptPubKey\" : {         (json object)\n"
                                                                                                       "     \"asm\" : \"code\",       (string) \n"
                                                                                                       "     \"hex\" : \"hex\",        (string) \n"
                                                                                                       "     \"reqSigs\" : n,          (numeric) Number of required signatures\n"
                                                                                                       "     \"type\" : \"pubkeyhash\", (string) The type, eg pubkeyhash\n"
                                                                                                       "     \"addresses\" : [          (array of string) array of navcoin addresses\n"
                                                                                                       "        \"navcoinaddress\"     (string) navcoin address\n"
                                                                                                       "        ,...\n"
                                                                                                       "     ]\n"
                                                                                                       "  },\n"
                                                                                                       "  \"version\" : n,            (numeric) The version\n"
                                                                                                       "  \"coinbase\" : true|false   (boolean) Coinbase or not\n"
                                                                                                       "}\n"

                                                                                                       "\nExamples:\n"
                                                                                                       "\nGet unspent transactions\n"
                + HelpExampleCli("listunspent", "") +
                "\nView the details\n"
                + HelpExampleCli("gettxout", "\"txid\" 1") +
                "\nAs a json rpc call\n"
                + HelpExampleRpc("gettxout", "\"txid\", 1")
                );

    LOCK(cs_main);

    UniValue ret(UniValue::VOBJ);

    std::string strHash = params[0].get_str();
    uint256 hash(uint256S(strHash));
    int n = params[1].get_int();
    bool fMempool = true;
    if (params.size() > 2)
        fMempool = params[2].get_bool();

    CCoins coins;
    if (fMempool) {
        LOCK(mempool.cs);
        CStateViewMemPool view(pcoinsTip, mempool);
        if (!view.GetCoins(hash, coins))
            return NullUniValue;
        mempool.pruneSpent(hash, coins); // TODO: this should be done by the CStateViewMemPool
    } else {
        if (!pcoinsTip->GetCoins(hash, coins))
            return NullUniValue;
    }
    if (n<0 || (unsigned int)n>=coins.vout.size() || coins.vout[n].IsNull())
        return NullUniValue;

    BlockMap::iterator it = mapBlockIndex.find(pcoinsTip->GetBestBlock());
    CBlockIndex *pindex = it->second;
    ret.pushKV("bestblock", pindex->GetBlockHash().GetHex());
    if ((unsigned int)coins.nHeight == MEMPOOL_HEIGHT)
        ret.pushKV("confirmations", 0);
    else
        ret.pushKV("confirmations", pindex->nHeight - coins.nHeight + 1);
    ret.pushKV("value", ValueFromAmount(coins.vout[n].nValue));
    UniValue o(UniValue::VOBJ);
    ScriptPubKeyToJSON(coins.vout[n].scriptPubKey, o, true);
    ret.pushKV("scriptPubKey", o);
    ret.pushKV("version", coins.nVersion);
    ret.pushKV("coinbase", coins.fCoinBase);

    return ret;
}

UniValue verifychain(const UniValue& params, bool fHelp)
{
    int nCheckLevel = GetArg("-checklevel", DEFAULT_CHECKLEVEL);
    int nCheckDepth = GetArg("-checkblocks", DEFAULT_CHECKBLOCKS);
    if (fHelp || params.size() > 2)
        throw runtime_error(
                "verifychain ( checklevel numblocks )\n"
                "\nVerifies blockchain database.\n"
                "\nArguments:\n"
                "1. checklevel   (numeric, optional, 0-4, default=" + strprintf("%d", nCheckLevel) + ") How thorough the block verification is.\n"
                                                                                                     "2. numblocks    (numeric, optional, default=" + strprintf("%d", nCheckDepth) + ", 0=all) The number of blocks to check.\n"
                                                                                                                                                                                     "\nResult:\n"
                                                                                                                                                                                     "true|false       (boolean) Verified or not\n"
                                                                                                                                                                                     "\nExamples:\n"
                + HelpExampleCli("verifychain", "")
                + HelpExampleRpc("verifychain", "")
                );

    LOCK(cs_main);

    if (params.size() > 0)
        nCheckLevel = params[0].get_int();
    if (params.size() > 1)
        nCheckDepth = params[1].get_int();

    fVerifyChain = true;
    bool ret = CVerifyDB().VerifyDB(Params(), pcoinsTip, nCheckLevel, nCheckDepth);
    fVerifyChain = false;

    return ret;
}

/** Implementation of IsSuperMajority with better feedback */
static UniValue SoftForkMajorityDesc(int minVersion, CBlockIndex* pindex, int nRequired, const Consensus::Params& consensusParams)
{
    int nFound = 0;
    CBlockIndex* pstart = pindex;
    for (int i = 0; i < consensusParams.nMajorityWindow && pstart != nullptr; i++)
    {
        if (pstart->nVersion >= minVersion)
            ++nFound;
        pstart = pstart->pprev;
    }

    UniValue rv(UniValue::VOBJ);
    rv.pushKV("status", nFound >= nRequired);
    rv.pushKV("found", nFound);
    rv.pushKV("required", nRequired);
    rv.pushKV("window", consensusParams.nMajorityWindow);
    return rv;
}

static UniValue SoftForkDesc(const std::string &name, int version, CBlockIndex* pindex, const Consensus::Params& consensusParams)
{
    UniValue rv(UniValue::VOBJ);
    rv.pushKV("id", name);
    rv.pushKV("version", version);
    rv.pushKV("enforce", SoftForkMajorityDesc(version, pindex, consensusParams.nMajorityEnforceBlockUpgrade, consensusParams));
    rv.pushKV("reject", SoftForkMajorityDesc(version, pindex, consensusParams.nMajorityRejectBlockOutdated, consensusParams));
    return rv;
}

static UniValue BIP9SoftForkDesc(const Consensus::Params& consensusParams, Consensus::DeploymentPos id)
{
    UniValue rv(UniValue::VOBJ);
    rv.pushKV("id", (int)id);

    const ThresholdState thresholdState = VersionBitsTipState(consensusParams, id);
    switch (thresholdState) {
    case THRESHOLD_DEFINED: rv.pushKV("status", "defined"); break;
    case THRESHOLD_STARTED: rv.pushKV("status", "started"); break;
    case THRESHOLD_LOCKED_IN: rv.pushKV("status", "locked_in"); break;
    case THRESHOLD_ACTIVE: rv.pushKV("status", "active"); break;
    case THRESHOLD_FAILED: rv.pushKV("status", "failed"); break;
    }
    rv.pushKV("bit", consensusParams.vDeployments[id].bit);
    rv.pushKV("startTime", consensusParams.vDeployments[id].nStartTime);
    rv.pushKV("timeout", consensusParams.vDeployments[id].nTimeout);

    return rv;
}

void BIP9SoftForkDescPushBack(UniValue& bip9_softforks, const std::string &name, const Consensus::Params& consensusParams, Consensus::DeploymentPos id)
{
    // Deployments with timeout value of 0 are hidden.
    // A timeout value of 0 guarantees a softfork will never be activated.
    // This is used when softfork codes are merged without specifying the deployment schedule.
    if (consensusParams.vDeployments[id].nTimeout > 0)
        bip9_softforks.pushKV(name, BIP9SoftForkDesc(consensusParams, id));
}

UniValue getblockchaininfo(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() != 0)
        throw runtime_error(
                "getblockchaininfo\n"
                "Returns an object containing various state info regarding block chain processing.\n"
                "\nResult:\n"
                "{\n"
                "  \"chain\": \"xxxx\",        (string) current network name as defined in BIP70 (main, test, regtest)\n"
                "  \"blocks\": xxxxxx,         (numeric) the current number of blocks processed in the server\n"
                "  \"headers\": xxxxxx,        (numeric) the current number of headers we have validated\n"
                "  \"bestblockhash\": \"...\", (string) the hash of the currently best block\n"
                "  \"difficulty\": xxxxxx,     (numeric) the current difficulty\n"
                "  \"mediantime\": xxxxxx,     (numeric) median time for the current best block\n"
                "  \"verificationprogress\": xxxx, (numeric) estimate of verification progress [0..1]\n"
                "  \"chainwork\": \"xxxx\"     (string) total amount of work in active chain, in hexadecimal\n"
                "  \"pruned\": xx,             (boolean) if the blocks are subject to pruning\n"
                "  \"pruneheight\": xxxxxx,    (numeric) heighest block available\n"
                "  \"softforks\": [            (array) status of softforks in progress\n"
                "     {\n"
                "        \"id\": \"xxxx\",        (string) name of softfork\n"
                "        \"version\": xx,         (numeric) block version\n"
                "        \"enforce\": {           (object) progress toward enforcing the softfork rules for new-version blocks\n"
                "           \"status\": xx,       (boolean) true if threshold reached\n"
                "           \"found\": xx,        (numeric) number of blocks with the new version found\n"
                "           \"required\": xx,     (numeric) number of blocks required to trigger\n"
                "           \"window\": xx,       (numeric) maximum size of examined window of recent blocks\n"
                "        },\n"
                "        \"reject\": { ... }      (object) progress toward rejecting pre-softfork blocks (same fields as \"enforce\")\n"
                "     }, ...\n"
                "  ],\n"
                "  \"bip9_softforks\": {          (object) status of BIP9 softforks in progress\n"
                "     \"xxxx\" : {                (string) name of the softfork\n"
                "        \"status\": \"xxxx\",    (string) one of \"defined\", \"started\", \"locked_in\", \"active\", \"failed\"\n"
                "        \"bit\": xx,             (numeric) the bit (0-28) in the block version field used to signal this softfork (only for \"started\" status)\n"
                "        \"startTime\": xx,       (numeric) the minimum median time past of a block at which the bit gains its meaning\n"
                "        \"timeout\": xx          (numeric) the median time past of a block at which the deployment is considered failed if not yet locked in\n"
                "     }\n"
                "  }\n"
                "}\n"
                "\nExamples:\n"
                + HelpExampleCli("getblockchaininfo", "")
                + HelpExampleRpc("getblockchaininfo", "")
                );

    LOCK(cs_main);

    UniValue obj(UniValue::VOBJ);
    obj.pushKV("chain",                 Params().NetworkIDString());
    obj.pushKV("blocks",                (int)chainActive.Height());
    obj.pushKV("headers",               pindexBestHeader ? pindexBestHeader->nHeight : -1);
    obj.pushKV("bestblockhash",         chainActive.Tip()->GetBlockHash().GetHex());
    obj.pushKV("difficulty",            (double)GetDifficulty());
    obj.pushKV("mediantime",            (int64_t)chainActive.Tip()->GetMedianTimePast());
    obj.pushKV("verificationprogress",  Checkpoints::GuessVerificationProgress(Params().Checkpoints(), chainActive.Tip()));
    obj.pushKV("chainwork",             chainActive.Tip()->nChainWork.GetHex());
    obj.pushKV("pruned",                fPruneMode);

    const Consensus::Params& consensusParams = Params().GetConsensus();
    CBlockIndex* tip = chainActive.Tip();
    UniValue softforks(UniValue::VARR);
    UniValue bip9_softforks(UniValue::VOBJ);
    softforks.push_back(SoftForkDesc("bip34", 2, tip, consensusParams));
    softforks.push_back(SoftForkDesc("bip66", 3, tip, consensusParams));
    softforks.push_back(SoftForkDesc("bip65", 4, tip, consensusParams));
    BIP9SoftForkDescPushBack(bip9_softforks, "csv", consensusParams, Consensus::DEPLOYMENT_CSV);
    BIP9SoftForkDescPushBack(bip9_softforks, "segwit", consensusParams, Consensus::DEPLOYMENT_SEGWIT);
    BIP9SoftForkDescPushBack(bip9_softforks, "communityfund", consensusParams, Consensus::DEPLOYMENT_COMMUNITYFUND);
    BIP9SoftForkDescPushBack(bip9_softforks, "communityfund_accumulation", consensusParams, Consensus::DEPLOYMENT_COMMUNITYFUND_ACCUMULATION);
    BIP9SoftForkDescPushBack(bip9_softforks, "ntpsync", consensusParams, Consensus::DEPLOYMENT_NTPSYNC);
    BIP9SoftForkDescPushBack(bip9_softforks, "coldstaking", consensusParams, Consensus::DEPLOYMENT_COLDSTAKING);
    BIP9SoftForkDescPushBack(bip9_softforks, "coldstaking_pool_fee", consensusParams, Consensus::DEPLOYMENT_POOL_FEE);
    BIP9SoftForkDescPushBack(bip9_softforks, "spread_cfund_accumulation", consensusParams, Consensus::DEPLOYMENT_COMMUNITYFUND_ACCUMULATION_SPREAD);
    BIP9SoftForkDescPushBack(bip9_softforks, "communityfund_amount_v2", consensusParams, Consensus::DEPLOYMENT_COMMUNITYFUND_AMOUNT_V2);
    BIP9SoftForkDescPushBack(bip9_softforks, "static", consensusParams, Consensus::DEPLOYMENT_STATIC_REWARD);
    BIP9SoftForkDescPushBack(bip9_softforks, "reduced_quorum", consensusParams, Consensus::DEPLOYMENT_QUORUM_CFUND);
    BIP9SoftForkDescPushBack(bip9_softforks, "votestatecache", consensusParams, Consensus::DEPLOYMENT_VOTE_STATE_CACHE);
    BIP9SoftForkDescPushBack(bip9_softforks, "consultations", consensusParams, Consensus::DEPLOYMENT_CONSULTATIONS);
    BIP9SoftForkDescPushBack(bip9_softforks, "dao_consensus", consensusParams, Consensus::DEPLOYMENT_DAO_CONSENSUS);
    BIP9SoftForkDescPushBack(bip9_softforks, "coldstaking_v2", consensusParams, Consensus::DEPLOYMENT_COLDSTAKING_V2);
    obj.pushKV("softforks",      softforks);
    obj.pushKV("bip9_softforks", bip9_softforks);

    if (fPruneMode)
    {
        CBlockIndex *block = chainActive.Tip();
        while (block && block->pprev && (block->pprev->nStatus & BLOCK_HAVE_DATA))
            block = block->pprev;

        obj.pushKV("pruneheight",        block->nHeight);
    }
    return obj;
}

/** Comparison function for sorting the getchaintips heads.  */
struct CompareBlocksByHeight
{
    bool operator()(const CBlockIndex* a, const CBlockIndex* b) const
    {
        /* Make sure that unequal blocks with the same height do not compare
           equal. Use the pointers themselves to make a distinction. */

        if (a->nHeight != b->nHeight)
            return (a->nHeight > b->nHeight);

        return a < b;
    }
};

UniValue getchaintips(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() != 0)
        throw runtime_error(
                "getchaintips\n"
                "Return information about all known tips in the block tree,"
                " including the main chain as well as orphaned branches.\n"
                "\nResult:\n"
                "[\n"
                "  {\n"
                "    \"height\": xxxx,         (numeric) height of the chain tip\n"
                "    \"hash\": \"xxxx\",         (string) block hash of the tip\n"
                "    \"branchlen\": 0          (numeric) zero for main chain\n"
                "    \"status\": \"active\"      (string) \"active\" for the main chain\n"
                "  },\n"
                "  {\n"
                "    \"height\": xxxx,\n"
                "    \"hash\": \"xxxx\",\n"
                "    \"branchlen\": 1          (numeric) length of branch connecting the tip to the main chain\n"
                "    \"status\": \"xxxx\"        (string) status of the chain (active, valid-fork, valid-headers, headers-only, invalid)\n"
                "  }\n"
                "]\n"
                "Possible values for status:\n"
                "1.  \"invalid\"               This branch contains at least one invalid block\n"
                "2.  \"headers-only\"          Not all blocks for this branch are available, but the headers are valid\n"
                "3.  \"valid-headers\"         All blocks are available for this branch, but they were never fully validated\n"
                "4.  \"valid-fork\"            This branch is not part of the active chain, but is fully validated\n"
                "5.  \"active\"                This is the tip of the active main chain, which is certainly valid\n"
                "\nExamples:\n"
                + HelpExampleCli("getchaintips", "")
                + HelpExampleRpc("getchaintips", "")
                );

    LOCK(cs_main);

    /*
     * Idea:  the set of chain tips is chainActive.tip, plus orphan blocks which do not have another orphan building off of them.
     * Algorithm:
     *  - Make one pass through mapBlockIndex, picking out the orphan blocks, and also storing a set of the orphan block's pprev pointers.
     *  - Iterate through the orphan blocks. If the block isn't pointed to by another orphan, it is a chain tip.
     *  - add chainActive.Tip()
     */
    std::set<const CBlockIndex*, CompareBlocksByHeight> setTips;
    std::set<const CBlockIndex*> setOrphans;
    std::set<const CBlockIndex*> setPrevs;

    for(const PAIRTYPE(const uint256, CBlockIndex*)& item: mapBlockIndex)
    {
        if (!chainActive.Contains(item.second)) {
            setOrphans.insert(item.second);
            setPrevs.insert(item.second->pprev);
        }
    }

    for (std::set<const CBlockIndex*>::iterator it = setOrphans.begin(); it != setOrphans.end(); ++it)
    {
        if (setPrevs.erase(*it) == 0) {
            setTips.insert(*it);
        }
    }

    // Always report the currently active tip.
    setTips.insert(chainActive.Tip());

    /* Construct the output array.  */
    UniValue res(UniValue::VARR);
    for(const CBlockIndex* block: setTips)
    {
        UniValue obj(UniValue::VOBJ);
        obj.pushKV("height", block->nHeight);
        obj.pushKV("hash", block->phashBlock->GetHex());

        const int branchLen = block->nHeight - chainActive.FindFork(block)->nHeight;
        obj.pushKV("branchlen", branchLen);

        string status;
        if (chainActive.Contains(block)) {
            // This block is part of the currently active chain.
            status = "active";
        } else if (block->nStatus & BLOCK_FAILED_MASK) {
            // This block or one of its ancestors is invalid.
            status = "invalid";
        } else if (block->nChainTx == 0) {
            // This block cannot be connected because full block data for it or one of its parents is missing.
            status = "headers-only";
        } else if (block->IsValid(BLOCK_VALID_SCRIPTS)) {
            // This block is fully validated, but no longer part of the active chain. It was probably the active block once, but was reorganized.
            status = "valid-fork";
        } else if (block->IsValid(BLOCK_VALID_STAKE)) {
            status = "valid-stake";
        } else if (block->IsValid(BLOCK_VALID_TREE)) {
            // The headers for this block are valid, but it has not been validated. It was probably never part of the most-work chain.
            status = "valid-headers";
        } else {
            // No clue.
            status = "unknown";
        }
        obj.pushKV("status", status);

        res.push_back(obj);
    }

    return res;
}

UniValue mempoolInfoToJSON()
{
    UniValue ret(UniValue::VOBJ);
    ret.pushKV("size", (int64_t) mempool.size());
    ret.pushKV("bytes", (int64_t) mempool.GetTotalTxSize());
    ret.pushKV("usage", (int64_t) mempool.DynamicMemoryUsage());
    size_t maxmempool = GetArg("-maxmempool", DEFAULT_MAX_MEMPOOL_SIZE) * 1000000;
    ret.pushKV("maxmempool", (int64_t) maxmempool);
    ret.pushKV("mempoolminfee", ValueFromAmount(mempool.GetMinFee(maxmempool).GetFeePerK()));

    return ret;
}

UniValue stempoolInfoToJSON()
{
    UniValue ret(UniValue::VOBJ);
    ret.pushKV("size", (int64_t) stempool.size());
    ret.pushKV("bytes", (int64_t) stempool.GetTotalTxSize());

    return ret;
}

UniValue getmempoolinfo(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() != 0)
        throw runtime_error(
                "getmempoolinfo\n"
                "\nReturns details on the active state of the TX memory pool.\n"
                "\nResult:\n"
                "{\n"
                "  \"size\": xxxxx,               (numeric) Current tx count\n"
                "  \"bytes\": xxxxx,              (numeric) Sum of all tx sizes\n"
                "  \"usage\": xxxxx,              (numeric) Total memory usage for the mempool\n"
                "  \"maxmempool\": xxxxx,         (numeric) Maximum memory usage for the mempool\n"
                "  \"mempoolminfee\": xxxxx       (numeric) Minimum fee for tx to be accepted\n"
                "}\n"
                "\nExamples:\n"
                + HelpExampleCli("getmempoolinfo", "")
                + HelpExampleRpc("getmempoolinfo", "")
                );

    return mempoolInfoToJSON();
}


UniValue getstempoolinfo(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() != 0)
        throw runtime_error(
                "getstempoolinfo\n"
                "\nReturns details on the active state of the dandelion stem pool.\n"
                "\nResult:\n"
                "{\n"
                "  \"size\": xxxxx,               (numeric) Current tx count\n"
                "  \"bytes\": xxxxx,              (numeric) Sum of all tx sizes\n"
                "}\n"
                "\nExamples:\n"
                + HelpExampleCli("getstempoolinfo", "")
                + HelpExampleRpc("getstempoolinfo", "")
                );

    return stempoolInfoToJSON();
}

UniValue getproposal(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() != 1)
        throw runtime_error(
                "getproposal \"hash\"\n"
                "\nShows information about the given proposal.\n"
                "\nArguments:\n"
                "1. hash   (string, required) the hash of the proposal\n"
                );

    LOCK(cs_main);

    CStateViewCache view(pcoinsTip);

    CProposal proposal;
    if(!view.GetProposal(uint256S(params[0].get_str()), proposal))
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Proposal not found");

    UniValue ret(UniValue::VOBJ);
    proposal.ToJson(ret, view);

    return ret;
}

UniValue getconsultation(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() != 1)
        throw runtime_error(
                "getconsultation \"hash\"\n"
                "\nShows information about the given consultation.\n"
                "\nArguments:\n"
                "1. hash   (string, required) the hash of the consultation\n"
                );

    LOCK(cs_main);

    CConsultation consultation;
    CStateViewCache view(pcoinsTip);

    if(!view.GetConsultation(uint256S(params[0].get_str()), consultation))
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Consultation not found");

    UniValue ret(UniValue::VOBJ);
    consultation.ToJson(ret, view);

    return ret;
}

UniValue getconsultationanswer(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() != 1)
        throw runtime_error(
                "getconsultationanswer \"hash\"\n"
                "\nShows information about the given consultation answer.\n"
                "\nArguments:\n"
                "1. hash   (string, required) the hash of the consultation answer\n"
                );

    LOCK(cs_main);

    CConsultationAnswer answer;
    CStateViewCache view(pcoinsTip);

    if(!view.GetConsultationAnswer(uint256S(params[0].get_str()), answer))
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Consultation answer not found");

    UniValue ret(UniValue::VOBJ);

    answer.ToJson(ret, view);

    return ret;
}

UniValue getpaymentrequest(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() != 1)
        throw runtime_error(
                "getpaymentrequest \"hash\"\n"
                "\nShows information about the given payment request.\n"
                "\nArguments:\n"
                "1. hash   (string, required) the hash of the payment request\n"
                );

    LOCK(cs_main);

    CPaymentRequest prequest;
    CStateViewCache view(pcoinsTip);

    if(!view.GetPaymentRequest(uint256S(params[0].get_str()), prequest))
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Payment request not found");

    UniValue ret(UniValue::VOBJ);

    prequest.ToJson(ret, view);

    return ret;
}

UniValue invalidateblock(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() != 1)
        throw runtime_error(
                "invalidateblock \"hash\"\n"
                "\nPermanently marks a block as invalid, as if it violated a consensus rule.\n"
                "\nArguments:\n"
                "1. hash   (string, required) the hash of the block to mark as invalid\n"
                "\nResult:\n"
                "\nExamples:\n"
                + HelpExampleCli("invalidateblock", "\"blockhash\"")
                + HelpExampleRpc("invalidateblock", "\"blockhash\"")
                );

    std::string strHash = params[0].get_str();
    uint256 hash(uint256S(strHash));
    CValidationState state;

    {
        LOCK(cs_main);
        if (mapBlockIndex.count(hash) == 0)
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Block not found");

        CBlockIndex* pblockindex = mapBlockIndex[hash];
        InvalidateBlock(state, Params(), pblockindex);
    }

    if (state.IsValid()) {
        ActivateBestChain(state, Params());
    }

    if (!state.IsValid()) {
        throw JSONRPCError(RPC_DATABASE_ERROR, state.GetRejectReason());
    }

    return NullUniValue;
}

UniValue reconsiderblock(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() != 1)
        throw runtime_error(
                "reconsiderblock \"hash\"\n"
                "\nRemoves invalidity status of a block and its descendants, reconsider them for activation.\n"
                "This can be used to undo the effects of invalidateblock.\n"
                "\nArguments:\n"
                "1. hash   (string, required) the hash of the block to reconsider\n"
                "\nResult:\n"
                "\nExamples:\n"
                + HelpExampleCli("reconsiderblock", "\"blockhash\"")
                + HelpExampleRpc("reconsiderblock", "\"blockhash\"")
                );

    std::string strHash = params[0].get_str();
    uint256 hash(uint256S(strHash));

    {
        LOCK(cs_main);
        if (mapBlockIndex.count(hash) == 0)
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Block not found");

        CBlockIndex* pblockindex = mapBlockIndex[hash];
        ResetBlockFailureFlags(pblockindex);
    }

    CValidationState state;
    ActivateBestChain(state, Params());

    if (!state.IsValid()) {
        throw JSONRPCError(RPC_DATABASE_ERROR, state.GetRejectReason());
    }

    return NullUniValue;
}

static const CRPCCommand commands[] =
{ //  category              name                      actor (function)         okSafeMode
  //  --------------------- ------------------------  -----------------------  ----------
  { "communityfund",      "cfundstats",             &cfundstats,             true  },
  { "blockchain",         "getblockchaininfo",      &getblockchaininfo,      true  },
  { "blockchain",         "getbestblockhash",       &getbestblockhash,       true  },
  { "blockchain",         "getblockcount",          &getblockcount,          true  },
  { "blockchain",         "getblock",               &getblock,               true  },
  { "blockchain",         "getblockdeltas",         &getblockdeltas,         false },
  { "blockchain",         "getblockhashes",         &getblockhashes,         true  },
  { "blockchain",         "getblockhash",           &getblockhash,           true  },
  { "blockchain",         "getblockheader",         &getblockheader,         true  },
  { "blockchain",         "getchaintips",           &getchaintips,           true  },
  { "blockchain",         "getdifficulty",          &getdifficulty,          true  },
  { "blockchain",         "getmempoolancestors",    &getmempoolancestors,    true  },
  { "blockchain",         "getmempooldescendants",  &getmempooldescendants,  true  },
  { "blockchain",         "getmempoolentry",        &getmempoolentry,        true  },
  { "blockchain",         "getmempoolinfo",         &getmempoolinfo,         true  },
  { "blockchain",         "getstempoolinfo",        &getstempoolinfo,        true  },
  { "communityfund",      "getproposal",            &getproposal,            true  },
  { "communityfund",      "getpaymentrequest",      &getpaymentrequest,      true  },
  { "blockchain",         "getrawmempool",          &getrawmempool,          true  },
  { "blockchain",         "gettxout",               &gettxout,               true  },
  { "blockchain",         "gettxoutsetinfo",        &gettxoutsetinfo,        true  },
  { "blockchain",         "verifychain",            &verifychain,            true  },
  { "dao",                "listconsultations",      &listconsultations,      true  },
  { "dao",                "getconsultation",        &getconsultation,        true  },
  { "dao",                "getconsultationanswer",  &getconsultationanswer,  true  },
  { "dao",                "getcfunddbstatehash",    &getcfunddbstatehash,    true  },

  /* Not shown in help */
  { "hidden",             "invalidateblock",        &invalidateblock,        true  },
  { "hidden",             "reconsiderblock",        &reconsiderblock,        true  },
};

void RegisterBlockchainRPCCommands(CRPCTable &tableRPC)
{
    for (unsigned int vcidx = 0; vcidx < ARRAYLEN(commands); vcidx++)
        tableRPC.appendCommand(commands[vcidx].name, &commands[vcidx]);
}
