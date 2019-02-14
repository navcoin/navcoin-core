// Copyright (c) 2019 The NavCoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "zeropos.h"
#include "zerotx.h"

bool CheckZeroKernel(CBlockIndex* pindexPrev, const CCoinsViewCache& view, unsigned int nBits, int64_t nTime, const COutPoint& prevout, int64_t* pBlockTime, CAmount& nValue)
{
    uint256 hashProofOfStake, targetProofOfStake;

    CTransaction txPrev;
    uint256 hashBlock = uint256();

    if (!GetTransaction(prevout.hash, txPrev, view, Params().GetConsensus(), hashBlock, true)){
        return error("CheckZeroKernel : Could not find previous transaction\n");
    }

    if (!txPrev.vout[prevout.n].scriptPubKey.IsZerocoinMint()) {
        return error("CheckZeroKernel : Transaction output is no zerocoin mint\n",prevout.hash.ToString());
    }

    libzerocoin::PublicCoin pubCoin(&Params().GetConsensus().Zerocoin_Params);

    if (!TxOutToPublicCoin(&Params().GetConsensus().Zerocoin_Params, txPrev.vout[prevout.n], pubCoin, NULL)) {
        return error("CheckZeroKernel : Can not convert to public coin\n",prevout.hash.ToString());;
    }

    CKey zk; libzerocoin::BlindingCommitment bc; libzerocoin::ObfuscationValue oj;

    if (!pwalletMain->GetZeroKey(zk))
        return error("Could not get zero key");

    if (!pwalletMain->GetBlindingCommitment(bc))
        return error("Could not get blinding commitment");

    libzerocoin::PrivateCoin privateCoin(&Params().GetConsensus().Zerocoin_Params, pubCoin.getDenomination(), zk, pubCoin.getPubKey(), bc, pubCoin.getValue(), pubCoin.getPaymentId());

    if (!privateCoin.isValid())
        return error("Private coin is not valid");

    if (!pwalletMain->GetObfuscationJ(oj))
        return error("Could not get obfuscation j");

    nValue = libzerocoin::ZerocoinDenominationToAmount(pubCoin.getDenomination());

    return CheckZeroStakeKernelHash(pindexPrev, nBits, nTime, privateCoin.getPublicSerialNumber(oj), nValue, hashProofOfStake, targetProofOfStake);
}

bool CheckZeroStakeKernelHash(CBlockIndex* pindexPrev, unsigned int nBits, int64_t nTime, const CBigNum& serialPublicNumber, CAmount nAmount, uint256& hashProofOfStake, uint256& targetProofOfStake)
{
    // Base target
    targetProofOfStake.SetCompact(nBits);

    // Weighted target
    int64_t nValueIn = nAmount;
    uint512 bnWeight = uint512(nValueIn);

    // We need to convert to uint512 to prevent overflow when multiplying by 1st block coins
    base_uint<512> targetProofOfStake512(targetProofOfStake.GetHex());
    targetProofOfStake512 *= bnWeight;

    uint64_t nStakeModifier = pindexPrev->nStakeModifier;
    int nStakeModifierHeight = pindexPrev->nHeight;
    int64_t nStakeModifierTime = pindexPrev->nTime;

    // Calculate hash
    CDataStream ss(SER_GETHASH, 0);
    ss << nStakeModifier << nStakeModifierHeight << nStakeModifierTime << serialPublicNumber << nAmount << nTime;
    hashProofOfStake = Hash(ss.begin(), ss.end());

    // We need to convert type so it can be compared to target
    base_uint<512> hashProofOfStake512(hashProofOfStake.GetHex());

    // Now check if proof-of-stake hash meets target protocol
    if (hashProofOfStake512 > targetProofOfStake512)
      return false;

    return true;
}
