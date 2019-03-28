// Copyright (c) 2018-2019 The NavCoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "zerochain.h"
#include "zerotx.h"

using namespace libzeroct;

CCriticalSection cs_dummy;

bool BlockToZeroCTMints(const ZeroCTParams *params, const CBlock* block, std::vector<PublicCoin> &vPubCoins)
{
    for (auto& tx : block->vtx) {
        for (auto& out : tx.vout) {
            if (!out.IsZeroCTMint())
                continue;

            std::vector<unsigned char> c; CPubKey p; std::vector<unsigned char> i;  std::vector<unsigned char> a;  std::vector<unsigned char> aco;
            if(!out.scriptPubKey.ExtractZeroCTMintData(p, c, i, a, aco))
                return error("BlockToZeroCTMints(): Could not extract ZeroCT mint data");

            CBigNum pid;
            pid.setvch(i);

            CBigNum oa;
            oa.setvch(a);

            CBigNum ac;
            ac.setvch(aco);

            PublicCoin checkPubCoin(params, CBigNum(c), p, pid, oa, ac);
            vPubCoins.push_back(checkPubCoin);
        }
    }

    return true;
}

bool CheckZeroCTMint(const ZeroCTParams *params, const CTxOut& txout, const CCoinsViewCache& view, CValidationState& state, PublicCoin* pPubCoin, bool fCheck, bool fFast)
{
    PublicCoin pubCoin(params);
    if(!TxOutToPublicCoin(params, txout, pubCoin, &state, false))
        return state.DoS(100, error("CheckZeroCTMint(): TxOutToPublicCoin() failed"));

    if (pPubCoin)
        *pPubCoin = pubCoin;

    if (fCheck && !pubCoin.isValid(fFast))
        return state.DoS(100, error("CheckZeroCTMint() : PubCoin does not validate"));

    PublicMintChainData zeroMint;

    if (view.GetMint(pubCoin.getCoinValue(), zeroMint) && !zeroMint.IsNull() && mapBlockIndex.count(zeroMint.GetBlockHash())) {
        CBlockIndex* pindex = mapBlockIndex.at(zeroMint.GetBlockHash());
        if (chainActive.Contains(pindex))
            return error("%s: pubcoin %s was already accumulated in tx %s from block %s", __func__,
                         pubCoin.getCoinValue().GetHex().substr(0, 8),
                         zeroMint.GetTxHash().GetHex(),
                         zeroMint.GetBlockHash().ToString());
    }

    return true;
}

bool CheckZeroCTSpend(const ZeroCTParams *params, const CTxIn& txin, const CCoinsViewCache& view, CValidationState& state, CoinSpend* pCoinSpend, Accumulator* pAccumulator, bool fSpendCheck)
{
    CoinSpend coinSpend(params);

    if(!TxInToCoinSpend(params, txin, coinSpend))
        return state.DoS(100, error("CheckZeroCTSpend() : TxInToCoinSpend() failed"));

    if (pCoinSpend)
        *pCoinSpend = coinSpend;

    uint256 blockAccumulatorHash = coinSpend.getBlockAccumulatorHash();

    if (!mapBlockIndex.count(blockAccumulatorHash))
        return state.DoS(100, error("CheckZeroCTSpend() : coinspend refers an invalid block hash %s", blockAccumulatorHash.ToString()));

    CBlockIndex* pindex = mapBlockIndex[blockAccumulatorHash];

    if (!chainActive.Contains(pindex))
        return state.DoS(20, error("CheckZeroCTSpend() : coinspend refers a block not contained in the main chain"));

    Accumulator accumulator(params);
    accumulator.setValue(pindex->nAccumulatorValue);

    if (pAccumulator)
        *pAccumulator = accumulator;

    if (fSpendCheck) {
        if (!VerifyCoinSpendCache(coinSpend, accumulator))
            return state.DoS(100, error("CheckZeroCTSpend() : CoinSpend does not verify"));
    }

    if (view.HaveSpendSerial(coinSpend.getCoinSerialNumber()))
        return state.DoS(100, error(strprintf("CheckZeroCTSpend() : Serial Number %s is already spent", coinSpend.getCoinSerialNumber().ToString(16))));

    return true;

}

bool VerifyCoinSpendCache(const CoinSpend& coinSpend, const Accumulator &accumulator)
{
    LOCK(cs_coinspend_cache);

    return VerifyCoinSpend(coinSpend, accumulator, true);
}

bool VerifyCoinSpendNoCache(const CoinSpend& coinSpend, const Accumulator &accumulator)
{
    return VerifyCoinSpend(coinSpend, accumulator, false);
}

bool VerifyCoinSpend(const CoinSpend& coinSpend, const Accumulator &accumulator, bool fWriteToCache)
{
    uint256 csHash = coinSpend.GetHash();
    bool fCached = (mapCacheValidCoinSpends.count(csHash) != 0);

    if ((!fCached && !coinSpend.Verify(accumulator)) || (fCached && mapCacheValidCoinSpends[csHash] == false)) {
        if (fWriteToCache)
            mapCacheValidCoinSpends[csHash] = false;
        return false;
    }

    if (fWriteToCache)
        mapCacheValidCoinSpends[csHash] = true;

    if (fWriteToCache && mapCacheValidCoinSpends.size() > COINSPEND_CACHE_SIZE)
        mapCacheValidCoinSpends.clear();

    return true;
}

bool CountMintsFromHeight(unsigned int nInitialHeight, unsigned int& nRet)
{
    nRet = 0;
    CBlockIndex* pindex = chainActive[nInitialHeight];

    while(pindex)
    {
        CBlock block;
        if (!ReadBlockFromDisk(block, pindex, Params().GetConsensus()))
            return false;

        for (auto& tx: block.vtx)
            for (CTxOut& out: tx.vout)
                if(out.IsZeroCTMint())
                    nRet++;

        pindex = chainActive.Next(pindex);
    }

    return true;
}

bool CalculateWitnessForMint(const CTxOut& txout, const CCoinsViewCache& view, const libzeroct::PublicCoin& pubCoin, Accumulator& accumulator, AccumulatorWitness& accumulatorWitness, CBigNum& accumulatorValue, uint256& blockAccumulatorHash, std::string& strError, int nRequiredMints, int nMaxHeight)
{
    const libzeroct::ZeroCTParams* params = &Params().GetConsensus().ZeroCT_Params;
    CBenchUtil bench;

    if (!txout.IsZeroCTMint()) {
        strError = "Transaction output script is not a ZeroCT mint.";
        return false;
    }

    if (!mapBlockIndex.count(blockAccumulatorHash)) {
        strError = strprintf("Could not find block hash %s of mint %s", blockAccumulatorHash.ToString(), pubCoin.getValue().ToString(16).substr(0,8));
        return false;
    }

    CBlockIndex* pindex = mapBlockIndex[blockAccumulatorHash];

    if (!chainActive.Contains(pindex)) {
        strError = strprintf("Block %s is not part of the active chain", blockAccumulatorHash.ToString());
        return false;
    }

    if(pindex->nAccumulatorValue != accumulator.getValue())
    {
           strError = strprintf("Accumulator %s does not match provided block hash %s (computed value %s)", accumulator.getValue().ToString(16).substr(0,8), blockAccumulatorHash.ToString(),
                                pindex->nAccumulatorValue.ToString(16).substr(0,8));
           return false;
    }

    accumulatorValue = accumulator.getValue();

    int nCount = 0;

    if (chainActive.Next(pindex)) {
        pindex = chainActive.Next(pindex);
        while (pindex && pindex->nHeight <= nMaxHeight) {
            CBlock block;

            if (!ReadBlockFromDisk(block, pindex, Params().GetConsensus())) {
                strError = strprintf("Could not read block %s from disk", pindex->GetBlockHash().ToString());
                return false;
            }

            if ((block.nVersion & VERSIONBITS_TOP_BITS_ZEROCT) != VERSIONBITS_TOP_BITS_ZEROCT) {
                strError = strprintf("Block %s is not a ZeroCT block", pindex->GetBlockHash().ToString());
                return false;
            }

            bool fFoundMint = false;

            for (auto& tx : block.vtx) {
                for (auto& out : tx.vout) {
                    if (!out.IsZeroCTMint())
                        continue;

                    PublicCoin pubCoinOut(params);

                    if (!TxOutToPublicCoin(params, out, pubCoinOut)) {
                        strError = strprintf("Could not extract ZeroCT mint data");
                        return false;
                    }

                    nCount++;
                    fFoundMint = true;

                    accumulatorWitness.AddElement(pubCoinOut);
                }
            }

            if (fFoundMint)
            {
                accumulatorValue = pindex->nAccumulatorValue;
                blockAccumulatorHash = pindex->GetBlockHash();
            }

            if(!chainActive.Next(pindex) || (nRequiredMints > 0 && nCount >= nRequiredMints))
                break;

            pindex = chainActive.Next(pindex);
        }
    }

    accumulator.setValue(accumulatorValue);

    if (!accumulatorWitness.VerifyWitness(accumulator, pubCoin)) {
        strError = "Witness did not verify";
        return false;
    }

    LogPrint("bench", "- %s took %.3fms for %d mints\n", __func__, bench.Checkpoint(), nCount);

    return true;
}


bool VerifyZeroCTBalance(const ZeroCTParams *params, const CTransaction& tx, const CCoinsViewCache& view, const CAmount nReward)
{
    if (!tx.IsZeroCT())
        return false;

    CDataStream ss(tx.vchTxSig, SER_NETWORK, PROTOCOL_VERSION);
    libzeroct::SerialNumberProofOfKnowledge snpok(&params->coinCommitmentGroup);

    ss >> snpok;

    CBigNum bnInputs = 1;
    CBigNum bnOutputs = 1;

    if (nReward > 0)
        bnInputs = bnInputs.mul_mod(params->coinCommitmentGroup.g2.pow_mod(nReward, params->coinCommitmentGroup.modulus), params->coinCommitmentGroup.modulus);

    for (auto &in: tx.vin)
    {
        if (in.scriptSig.IsZeroCTSpend())
        {
            libzeroct::CoinSpend spend(params);

            if (!ScriptToCoinSpend(params, in.scriptSig, spend))
                return false;

            bnInputs = bnInputs.mul_mod(spend.getAmountCommitment(), params->coinCommitmentGroup.modulus);
        }
        else
        {
            CAmount inputValue = view.GetOutputFor(in).nValue;
            bnInputs = bnInputs.mul_mod(params->coinCommitmentGroup.g2.pow_mod(inputValue, params->coinCommitmentGroup.modulus), params->coinCommitmentGroup.modulus);
        }
    }

    for (auto &out: tx.vout)
    {
        if (out.IsZeroCTMint())
        {
            CBigNum ac;

            if (!TxOutToPublicCoinAmountCommitment(params, out, ac))
                return false;

            bnOutputs = bnOutputs.mul_mod(ac, params->coinCommitmentGroup.modulus);
        }
        else
        {
            bnOutputs = bnOutputs.mul_mod(params->coinCommitmentGroup.g2.pow_mod(out.nValue, params->coinCommitmentGroup.modulus), params->coinCommitmentGroup.modulus);
        }
    }

    CBigNum bnPubKey = bnInputs.mul_mod(bnOutputs.inverse(params->coinCommitmentGroup.modulus), params->coinCommitmentGroup.modulus);

    return snpok.Verify(bnPubKey, tx.GetHashAmountSig());
}
