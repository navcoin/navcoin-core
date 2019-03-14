// Copyright (c) 2019 The NavCoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "chainparams.h"
#include "main.h"
#include "miner.h"

#include "zeroaccumulators.h"
#include "zerotx.h"
#include "zerowitnesser.h"

typedef std::function<bool(std::pair<const CBigNum, PublicMintWitnessData>, std::pair<const CBigNum, PublicMintWitnessData>)> Comparator;

Comparator compare =
        [](std::pair<const CBigNum, PublicMintWitnessData> a, std::pair<const CBigNum, PublicMintWitnessData> b)
{
    return a.second.GetCount() < b.second.GetCount();
};

void NavCoinWitnesser(const CChainParams& chainparams)
{
    LogPrintf("Witnesser thread started\n");
    SetThreadPriority(THREAD_PRIORITY_LOWEST);
    RenameThread("navcoin-witnesser");

    try {
        while (true)
        {
            while (!pwalletMain)
            {
                MilliSleep(1000);
            }

            boost::this_thread::interruption_point();

            std::set<std::pair<const CBigNum, PublicMintWitnessData>, Comparator> cachedMapWitness;

            {
                LOCK(pwalletMain->cs_witnesser);
                cachedMapWitness = std::set<std::pair<const CBigNum, PublicMintWitnessData>, Comparator>(pwalletMain->mapWitness.begin(), pwalletMain->mapWitness.end(), compare);
            }

            CacheWitnessToWrite toWrite;

            for (const std::pair<const CBigNum, PublicMintWitnessData> &it: cachedMapWitness)
            {
                LOCK(cs_main);

                PublicMintWitnessData witnessData = it.second;

                {
                    LOCK(pwalletMain->cs_wallet);

                    if (pwalletMain->IsSpent(witnessData.GetChainData().GetTxHash(), witnessData.GetChainData().GetOutput()))
                        continue;

                    if (!pwalletMain->mapWallet.count(witnessData.GetChainData().GetTxHash()))
                        continue;

                    CWalletTx pwtx = pwalletMain->mapWallet[witnessData.GetChainData().GetTxHash()];

                    if (!pwtx.IsInMainChain())
                        continue;
                }

                if (!mapBlockIndex.count(witnessData.GetBlockAccumulatorHash()))
                {
                    witnessData.Recover();
                    if (!mapBlockIndex.count(witnessData.GetBlockAccumulatorHash()))
                    {
                        witnessData.Reset();
                        toWrite.Add(it.first, witnessData);
                        continue;
                    }
                }

                CBlockIndex *pindex;

                bool fReset = false;

                pindex = mapBlockIndex[witnessData.GetBlockAccumulatorHash()];
                pindex = chainActive.Next(pindex);

                if (!pindex)
                    continue;

                CBlockIndex *plastindex = pindex;
                bool fShouldWrite = false;
                int nCount = GetArg("-witnesser_blocks_per_round", DEFAULT_BLOCKS_PER_ROUND);

                while (pindex && nCount > 0)
                {
                    CBlock block;
                    bool fRecover = false;
                    bool fStake = (chainActive.Tip()->nHeight - pindex->nHeight) > COINBASE_MATURITY;

                    if (GetStaking() && fStake && witnessData.GetCount() > 0)
                        break;

                    if (!ReadBlockFromDisk(block, pindex, Params().GetConsensus()))
                        fRecover = true;

                    if ((block.nVersion & VERSIONBITS_TOP_BITS_ZEROCT) != VERSIONBITS_TOP_BITS_ZEROCT)
                        fRecover = true;

                    if (fRecover)
                    {
                        witnessData.Recover();
                        toWrite.Add(it.first, witnessData);
                        break;
                    }

                    for (auto& tx : block.vtx)
                    {
                        for (auto& out : tx.vout)
                        {
                            if (!out.IsZeroCTMint())
                                continue;

                            PublicCoin pubCoinOut(&Params().GetConsensus().ZeroCT_Params);

                            if (!TxOutToPublicCoin(&Params().GetConsensus().ZeroCT_Params, out, pubCoinOut))
                                continue;

                            witnessData.Accumulate(pubCoinOut.getValue());
                            fShouldWrite = true;
                        }
                    }

                    if (witnessData.Verify())
                        witnessData.SetBlockAccumulatorHash(pindex->GetBlockHash());
                    else
                        fRecover = true;

                    if (witnessData.GetAccumulator().getValue() != pindex->nAccumulatorValue)
                        fRecover = true;

                    if (fRecover)
                    {
                        witnessData.Recover();
                        toWrite.Add(it.first, witnessData);
                        break;
                    }

                    plastindex = pindex;

                    pindex = chainActive.Next(pindex);
                    nCount--;
                }

                if (!witnessData.Verify())
                {
                    witnessData.Recover();
                    if (!witnessData.Verify())
                        fReset = true;
                }

                if (!mapBlockIndex.count(witnessData.GetPrevBlockAccumulatorHash()))
                    fReset = true;

                pindex = mapBlockIndex[witnessData.GetPrevBlockAccumulatorHash()];

                if(!fReset && !chainActive.Contains(pindex))
                    fReset = true;

                if (fReset) {
                    witnessData.Reset();
                    toWrite.Add(it.first, witnessData);
                    continue;
                }

                CBlockIndex* pindexPrevState = mapBlockIndex[witnessData.GetPrevBlockAccumulatorHash()];

                if (plastindex->nHeight - pindexPrevState->nHeight >= GetArg("-witnesser_block_snapshot", DEFAULT_BLOCK_SNAPSHOT))
                {
                    witnessData.Backup();
                    fShouldWrite = true;
                }

                if (fShouldWrite)
                {
                    toWrite.Add(it.first, witnessData);
                }
            }

            {
                LOCK2(pwalletMain->cs_wallet, pwalletMain->cs_witnesser);

                for (std::pair<const CBigNum, PublicMintWitnessData> it: toWrite.GetMap())
                {
                    pwalletMain->WriteWitness(it.first, it.second);
                }
            }

            MilliSleep(250);
        }
    }
    catch (const boost::thread_interrupted&)
    {
        LogPrintf("Witnesser thread terminated\n");
        throw;
    }
    catch (const std::runtime_error &e)
    {
        LogPrintf("Witnesser runtime error: %s\n", e.what());
        return;
    }
}
