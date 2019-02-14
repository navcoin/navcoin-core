// Copyright (c) 2019 The NavCoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "chainparams.h"
#include "main.h"
#include "miner.h"

#include "zeroaccumulators.h"
#include "zerotx.h"
#include "zerowitnesser.h"


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


            std::map<CBigNum, PublicMintWitnessData> cachedMapWitness;
            cachedMapWitness.clear();
            {
                LOCK(pwalletMain->cs_witnesser);
                cachedMapWitness.insert(pwalletMain->mapWitness.begin(), pwalletMain->mapWitness.end());
            }

            for (std::pair<const CBigNum, PublicMintWitnessData> &it: cachedMapWitness)
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

                AccumulatorMap accumulatorMap(&chainparams.GetConsensus().Zerocoin_Params);
                if (!accumulatorMap.Load(witnessData.GetChecksum()))
                {
                    witnessData.Recover();
                    if (!accumulatorMap.Load(witnessData.GetChecksum()))
                    {
                        witnessData.Reset();
                        {
                            LOCK2(pwalletMain->cs_wallet, pwalletMain->cs_witnesser);
                            pwalletMain->WriteWitness(it.first, witnessData);
                        }
                        continue;
                    }
                }

                CBlockIndex *pindex;

                {
                    LOCK(cs_main);
                    bool fReset = false;

                    if(accumulatorMap.GetBlockHash() == uint256() || !mapBlockIndex.count(accumulatorMap.GetBlockHash()))
                    {
                        if (witnessData.GetCount() == 0)
                            continue;

                        witnessData.Recover();

                        if (!accumulatorMap.Load(witnessData.GetChecksum()))
                        {
                            witnessData.Reset();
                            {
                                LOCK2(pwalletMain->cs_wallet, pwalletMain->cs_witnesser);
                                pwalletMain->WriteWitness(it.first, witnessData);
                            }
                            continue;
                        }

                        if(accumulatorMap.GetBlockHash() == uint256() || !mapBlockIndex.count(accumulatorMap.GetBlockHash()))
                        {
                            witnessData.Reset();
                            {
                                LOCK2(pwalletMain->cs_wallet, pwalletMain->cs_witnesser);
                                pwalletMain->WriteWitness(it.first, witnessData);
                            }
                            continue;
                        }
                    }

                    pindex = mapBlockIndex[accumulatorMap.GetBlockHash()];
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

                        if ((block.nVersion & VERSIONBITS_TOP_BITS_ZEROCOIN) != VERSIONBITS_TOP_BITS_ZEROCOIN)
                            fRecover = true;

                        if (fRecover)
                        {
                            witnessData.Recover();
                            {
                                LOCK2(pwalletMain->cs_wallet, pwalletMain->cs_witnesser);
                                pwalletMain->WriteWitness(it.first, witnessData);
                            }
                            break;
                        }

                        for (auto& tx : block.vtx)
                        {
                            for (auto& out : tx.vout)
                            {
                                if (!out.IsZerocoinMint())
                                    continue;

                                PublicCoin pubCoinOut(&Params().GetConsensus().Zerocoin_Params);

                                if (!TxOutToPublicCoin(&Params().GetConsensus().Zerocoin_Params, out, pubCoinOut))
                                    continue;

                                if (pubCoinOut.getDenomination() != witnessData.GetPublicCoin().getDenomination())
                                    continue;

                                witnessData.Accumulate(pubCoinOut.getValue());
                                fShouldWrite = true;
                            }
                        }

                        if (witnessData.Verify())
                            witnessData.SetChecksum(pindex->nAccumulatorChecksum);
                        else
                            fRecover = true;

                        if (!accumulatorMap.Load(pindex->nAccumulatorChecksum) || witnessData.GetAccumulator().getValue() != accumulatorMap.GetValue(witnessData.GetPublicCoin().getDenomination()))
                            fRecover = true;

                        if (fRecover)
                        {
                            witnessData.Recover();
                            {
                                LOCK2(pwalletMain->cs_wallet, pwalletMain->cs_witnesser);
                                pwalletMain->WriteWitness(it.first, witnessData);
                            }
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

                    AccumulatorMap prevAccumulatorMap(&chainparams.GetConsensus().Zerocoin_Params);
                    if (!prevAccumulatorMap.Load(witnessData.GetPrevChecksum()))
                        fReset = true;

                    if(!fReset && (prevAccumulatorMap.GetBlockHash() == uint256() || !mapBlockIndex.count(prevAccumulatorMap.GetBlockHash())))
                        fReset = true;

                    if (fReset) {
                        witnessData.Reset();
                        {
                            LOCK2(pwalletMain->cs_wallet, pwalletMain->cs_witnesser);
                            pwalletMain->WriteWitness(it.first, witnessData);
                        }
                        continue;
                    }

                    CBlockIndex* pindexPrevState = mapBlockIndex[prevAccumulatorMap.GetBlockHash()];

                    if (plastindex->nHeight - pindexPrevState->nHeight >= GetArg("-witnesser_block_snapshot", DEFAULT_BLOCK_SNAPSHOT))
                    {
                        witnessData.Backup();
                        fShouldWrite = true;
                    }

                    if (fShouldWrite)
                    {
                        LOCK2(pwalletMain->cs_wallet, pwalletMain->cs_witnesser);
                        pwalletMain->WriteWitness(it.first, witnessData);
                    }
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
