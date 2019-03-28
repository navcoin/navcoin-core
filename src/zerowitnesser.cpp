// Copyright (c) 2019 The NavCoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "chainparams.h"
#include "main.h"
#include "miner.h"

#include "zeroaccumulators.h"
#include "zerotx.h"
#include "zerowitnesser.h"

#include <boost/thread.hpp>

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
    const libzeroct::ZeroCTParams* params = &Params().GetConsensus().ZeroCT_Params;

    try {
        while (true)
        {            
            while (!pwalletMain || !chainActive.Tip())
            {
                MilliSleep(1000);
            }

            do {
                bool fmapWitnessEmpty;
                {
                    LOCK(pwalletMain->cs_witnesser);
                    fmapWitnessEmpty = pwalletMain->mapWitness.empty();
                }
                if (!fmapWitnessEmpty && !IsInitialBlockDownload())
                    break;
                MilliSleep(1000);
            } while (true);

            boost::this_thread::interruption_point();

            std::map<CBigNum, PublicMintWitnessData> cachedMapWitness;

            {
                LOCK(pwalletMain->cs_witnesser);
                cachedMapWitness = pwalletMain->mapWitness;
            }

            int nZeroTip = 0;

            {
                LOCK(cs_main);
                if (chainActive.ZeroTip())
                    nZeroTip = chainActive.ZeroTip()->nHeight;
            }

            if (nZeroTip == 0)
            {
                MilliSleep(30000);
                continue;
            }

            CacheWitnessToWrite toWrite;

            for (const std::pair<const CBigNum, PublicMintWitnessData> &it: cachedMapWitness)
            {
                PublicMintWitnessData witnessData = it.second;

                {
                    LOCK2(cs_main, pwalletMain->cs_wallet);

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

                if (pindex->nHeight == nZeroTip)
                    continue;

                pindex = chainActive.Next(pindex);

                if (!pindex)
                    continue;

                CBlockIndex *plastindex = pindex;
                bool fShouldWrite = false;
                int nCount = GetArg("-witnesser_blocks_per_round", DEFAULT_BLOCKS_PER_ROUND);

                while (chainActive.Tip() && pindex && nCount > 0)
                {
                    CBlock block;
                    bool fRecover = false;
                    bool fStake = (chainActive.Tip()->nHeight - pindex->nHeight) <= COINBASE_MATURITY;

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

                    bool fHasZeroMint = false;

                    for (auto& tx : block.vtx)
                    {
                        for (auto& out : tx.vout)
                        {
                            if (!out.IsZeroCTMint())
                                continue;

                            libzeroct::PublicCoin pubCoin(params);

                            if (!TxOutToPublicCoin(&Params().GetConsensus().ZeroCT_Params, out, pubCoin))
                                continue;

                            witnessData.Accumulate(pubCoin.getValue(), pindex->nAccumulatorValue);
                            assert(witnessData.GetAccumulator().getValue() == pindex->nAccumulatorValue);

                            fHasZeroMint = true;
                            fShouldWrite = true;

                            nCount--;
                        }
                    }

                    if (fHasZeroMint) {
                        witnessData.SetBlockAccumulatorHash(pindex->GetBlockHash());
                        assert(witnessData.GetBlockAccumulatorHash() == pindex->GetBlockHash());
                    }

                    plastindex = pindex;

                    pindex = chainActive.Next(pindex);
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

            for (std::pair<const CBigNum, PublicMintWitnessData> it: toWrite.GetMap())
            {
                LOCK(pwalletMain->cs_witnesser);
                if (pwalletMain->mapWitness.count(it.first))
                    pwalletMain->WriteWitness(it.first, it.second);
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
