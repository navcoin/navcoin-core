// Copyright (c) 2019 The NavCoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "zeropos.h"
#include "zerotx.h"

bool CheckZeroKernel(CBlockIndex* pindexPrev, const CCoinsViewCache& view, unsigned int nBits, int64_t nTime, const COutPoint& prevout, int64_t* pBlockTime, CAmount& nValue, uint256& hashProofOfStake, uint256& targetProofOfStake)
{
    CTransaction txPrev;
    uint256 hashBlock = uint256();

    if (!GetTransaction(prevout.hash, txPrev, view, Params().GetConsensus(), hashBlock, true)){
        return error("CheckZeroKernel : Could not find previous transaction %s\n", prevout.ToString());
    }

    if (!txPrev.vout[prevout.n].scriptPubKey.IsZeroCTMint()) {
        return error("CheckZeroKernel : Transaction output is no ZeroCT mint\n",prevout.hash.ToString());
    }

    libzeroct::PublicCoin pubCoin(&Params().GetConsensus().ZeroCT_Params);

    if (!TxOutToPublicCoin(&Params().GetConsensus().ZeroCT_Params, txPrev.vout[prevout.n], pubCoin, NULL)) {
        return error("CheckZeroKernel : Can not convert to public coin\n",prevout.hash.ToString());;
    }

    CKey zk; libzeroct::BlindingCommitment bc; libzeroct::ObfuscationValue oj;

    if (!pwalletMain->GetZeroKey(zk))
        return error("Could not get zero key");

    if (!pwalletMain->GetBlindingCommitment(bc))
        return error("Could not get blinding commitment");

    libzeroct::PrivateCoin privateCoin(&Params().GetConsensus().ZeroCT_Params, zk, pubCoin.getPubKey(), bc, pubCoin.getValue(), pubCoin.getPaymentId(), pubCoin.getAmount());

    if (!privateCoin.isValid())
        return error("Private coin is not valid");

    if (!pwalletMain->GetObfuscationJ(oj))
        return error("Could not get obfuscation j");

    nValue = privateCoin.getAmount();

    return CheckZeroStakeKernelHashCandidate(pindexPrev, nBits, nTime, privateCoin.getPublicSerialNumber(oj), nValue, hashProofOfStake, targetProofOfStake);
}

bool CheckZeroCTKernelHash(CBlockIndex* pindexPrev, unsigned int nBits, const CTransaction& tx, const libzeroct::CoinSpend& cs, uint256& hashProofOfStake, uint256& targetProofOfStake)
{
    if (tx.vchKernelHashProof.empty())
        return false;

    // Base target
    targetProofOfStake.SetCompact(nBits);

    CBigNum target(targetProofOfStake);

    int nExp = (nBits >> 24);
    if (nExp < 3) nExp = 3;
    int nShift = 8 * (nExp - 3);

    target >>= nShift;

    // Calculate hash
    hashProofOfStake = GetZeroCTKernelHash(pindexPrev->nStakeModifier, pindexPrev->nHeight, pindexPrev->nTime, cs.getCoinSerialNumber(), tx.nTime, nShift);

    const CBigNum kernel(hashProofOfStake);
    const CBigNum amountCommitment = cs.getAmountCommitment();

    const libzeroct::IntegerGroupParams* group = &Params().GetConsensus().ZeroCT_Params.coinCommitmentGroup;
    const CBigNum p = group->modulus;
    const CBigNum q = group->groupOrder;
    const CBigNum g2 = group->g2;

    CBigNum kernelHashCommitment = amountCommitment.pow_mod(target, p).mul_mod(g2.pow_mod(-kernel, p), p);

    CBN_matrix mValueCommitments = {{kernelHashCommitment}};

    std::vector<unsigned char> dataBpRp;

    dataBpRp.clear();
    dataBpRp.insert(dataBpRp.end(), tx.vchKernelHashProof.begin(), tx.vchKernelHashProof.end());

    CDataStream serializedKernelHashProof(dataBpRp, SER_NETWORK, PROTOCOL_VERSION);

    BulletproofsRangeproof bprp(group, serializedKernelHashProof);

    std::vector<BulletproofsRangeproof> proofs;
    proofs.push_back(bprp);

    if (!VerifyBulletproof(group, proofs, mValueCommitments, KERNEL_PROOF_BITS))
        return false;

    return true;
}

bool CheckZeroStakeKernelHashCandidate(CBlockIndex* pindexPrev, unsigned int nBits, int64_t nTime, const CBigNum& serialPublicNumber, CAmount nAmount, uint256& hashProofOfStake, uint256& targetProofOfStake)
{
    int nExp = (nBits >> 24);
    if (nExp < 3) nExp = 3;
    int nShift = 8 * (nExp - 3);

    // Base target
    targetProofOfStake.SetCompact(nBits);    
    targetProofOfStake >>= nShift;

    // Weighted target
    uint256 bnWeight = nAmount;

    // We need to convert to uint512 to prevent overflow when multiplying by 1st block coins
    uint256 targetProofOfStakeMulAmount = targetProofOfStake * bnWeight;

    // Calculate hash
    hashProofOfStake = GetZeroCTKernelHash(pindexPrev->nStakeModifier, pindexPrev->nHeight, pindexPrev->nTime, serialPublicNumber, nTime, nShift);

    // Now check if proof-of-stake hash meets target protocol
    if (hashProofOfStake > targetProofOfStakeMulAmount)
      return false;

    return true;
}

uint256 GetZeroCTKernelHash(uint64_t nStakeModifier, int nStakeModifierHeight, int64_t nStakeModifierTime, CBigNum bnSerialNumber, uint64_t nTime, int nShift)
{
    CHashWriter hasher(0,0);

    hasher << nStakeModifier;
    hasher << nStakeModifierHeight;
    hasher << nStakeModifierTime;
    hasher << bnSerialNumber;
    hasher << nTime;

    return hasher.GetHash() >> nShift;
}
