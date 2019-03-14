// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Copyright (c) 2018 The NavCoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "primitives/transaction.h"
#include "hash.h"
#include "tinyformat.h"
#include "utilstrencodings.h"
#include "zerotx.h"

std::string COutPoint::ToString() const
{
    return strprintf("COutPoint(%s, %u)", hash.ToString().substr(0,10), n);
}

CTxIn::CTxIn(COutPoint prevoutIn, CScript scriptSigIn, uint32_t nSequenceIn)
{
    prevout = prevoutIn;
    scriptSig = scriptSigIn;
    nSequence = nSequenceIn;
}

CTxIn::CTxIn(uint256 hashPrevTx, uint32_t nOut, CScript scriptSigIn, uint32_t nSequenceIn)
{
    prevout = COutPoint(hashPrevTx, nOut);
    scriptSig = scriptSigIn;
    nSequence = nSequenceIn;
}

std::string CTxIn::ToString() const
{
    std::string str;
    str += "CTxIn(";
    if(!scriptSig.IsZeroCTSpend())
        str += prevout.ToString();
    if(scriptSig.IsZeroCTSpend()) {
        str += strprintf("zeroct_spend");
    } else if (prevout.IsNull())
        str += strprintf(", coinbase %s", HexStr(scriptSig));
    else  {
        str += strprintf(", scriptSig=%s", HexStr(scriptSig).substr(0, 24));
        if (nSequence != SEQUENCE_FINAL)
            str += strprintf(", nSequence=%u", nSequence);
    }
    str += ")";
    return str;
}

CTxOut::CTxOut(const CAmount& nValueIn, CScript scriptPubKeyIn)
{
    nValue = nValueIn;
    scriptPubKey = scriptPubKeyIn;
}

uint256 CTxOut::GetHash() const
{
    return SerializeHash(*this);
}

std::string CTxOut::ToString() const
{
    if (IsZeroCTMint()) {
        std::vector<unsigned char> c; CPubKey p; std::vector<unsigned char> i; std::vector<unsigned char> a;
        std::vector<unsigned char> ac;
        if(!scriptPubKey.ExtractZeroCTMintData(p, c, i, a, ac))
            return "CTxOut(zeroct_mint)";
        else
            return strprintf("CTxOut(zeroct_mint, coinvalue=%s, amountvalue=%s)", HexStr(c).substr(0,16), HexStr(ac).substr(0,16));
    }
    if (IsEmpty()) return "CTxOut(empty)";
    if (IsCommunityFundContribution())
      return strprintf("CTxOut(nValue=%d.%08d, CommunityFundContribution)", nValue / COIN, nValue % COIN);
    else
      return strprintf("CTxOut(nValue=%d.%08d, scriptPubKey=%s)", nValue / COIN, nValue % COIN, scriptPubKey.ToString());
}

CMutableTransaction::CMutableTransaction() : nVersion(CTransaction::CURRENT_VERSION), nTime(0), nLockTime(0) {
  // *const_cast<unsigned int*>(&nTime) = GetTime();
}
CMutableTransaction::CMutableTransaction(const CTransaction& tx) : nVersion(tx.nVersion), nTime(tx.nTime), vin(tx.vin), vout(tx.vout), wit(tx.wit), nLockTime(tx.nLockTime), strDZeel(tx.strDZeel), vchTxSig(tx.vchTxSig), vchRangeProof(tx.vchRangeProof), vchKernelHashProof(tx.vchKernelHashProof) {}

uint256 CMutableTransaction::GetHash() const
{
    return SerializeHash(*this, SER_GETHASH, SERIALIZE_TRANSACTION_NO_WITNESS);
}

uint256 CMutableTransaction::GetHashAmountSig() const
{
    return SerializeHash(*this, SER_GETHASHNOTXSIG, SERIALIZE_TRANSACTION_NO_WITNESS);
}

uint256 CTransaction::GetHashAmountSig() const
{
    return SerializeHash(*this, SER_GETHASHNOTXSIG, SERIALIZE_TRANSACTION_NO_WITNESS);
}

void CTransaction::UpdateHash() const
{
    *const_cast<uint256*>(&hash) = SerializeHash(*this, SER_GETHASH, SERIALIZE_TRANSACTION_NO_WITNESS);
}

uint256 CTransaction::GetWitnessHash() const
{
    return SerializeHash(*this, SER_GETHASH, 0);
}

CTransaction::CTransaction() : nVersion(CTransaction::CURRENT_VERSION), nTime(0), vin(), vout(), nLockTime(0) {
    // *const_cast<unsigned int*>(&nTime) = GetTime();
}

CTransaction::CTransaction(const CMutableTransaction &tx) : nVersion(tx.nVersion), nTime(tx.nTime), vin(tx.vin), vout(tx.vout), wit(tx.wit), nLockTime(tx.nLockTime), strDZeel(tx.strDZeel), vchTxSig(tx.vchTxSig), vchRangeProof(tx.vchRangeProof), vchKernelHashProof(tx.vchKernelHashProof) {
    UpdateHash();
}

CTransaction& CTransaction::operator=(const CTransaction &tx) {
    *const_cast<int*>(&nVersion) = tx.nVersion;
    *const_cast<std::vector<CTxIn>*>(&vin) = tx.vin;
    *const_cast<std::vector<CTxOut>*>(&vout) = tx.vout;
    *const_cast<CTxWitness*>(&wit) = tx.wit;
    *const_cast<unsigned int*>(&nLockTime) = tx.nLockTime;
    *const_cast<uint256*>(&hash) = tx.hash;
    *const_cast<unsigned int*>(&nTime) = tx.nTime;
    *const_cast<std::string*>(&strDZeel) = tx.strDZeel;
    *const_cast<std::vector<unsigned char>*>(&vchRangeProof) = tx.vchRangeProof;
    *const_cast<std::vector<unsigned char>*>(&vchTxSig) = tx.vchTxSig;
    *const_cast<std::vector<unsigned char>*>(&vchKernelHashProof) = tx.vchKernelHashProof;
    return *this;
}

CAmount CTransaction::GetValueOut() const
{
    CAmount nValueOut = 0;
    for (std::vector<CTxOut>::const_iterator it(vout.begin()); it != vout.end(); ++it)
    {
        nValueOut += it->nValue;
        if (!MoneyRange(it->nValue) || !MoneyRange(nValueOut))
            throw std::runtime_error("CTransaction::GetValueOut(): value out of range");
    }
    return nValueOut;
}

CAmount CTransaction::GetValueOutCFund() const
{
    CAmount nValueOut = 0;
    for (std::vector<CTxOut>::const_iterator it(vout.begin()); it != vout.end(); ++it)
    {
        if(it->IsCommunityFundContribution()) {
            nValueOut += it->nValue;
            if (!MoneyRange(it->nValue) || !MoneyRange(nValueOut))
                throw std::runtime_error("CTransaction::GetValueOut(): value out of range");
        }
    }
    return nValueOut;
}

double CTransaction::ComputePriority(double dPriorityInputs, unsigned int nTxSize) const
{
    nTxSize = CalculateModifiedSize(nTxSize);
    if (nTxSize == 0) return 0.0;

    return dPriorityInputs / nTxSize;
}

unsigned int CTransaction::CalculateModifiedSize(unsigned int nTxSize) const
{
    // In order to avoid disincentivizing cleaning up the UTXO set we don't count
    // the constant overhead for each txin and up to 110 bytes of scriptSig (which
    // is enough to cover a compressed pubkey p2sh redemption) for priority.
    // Providing any more cleanup incentive than making additional inputs free would
    // risk encouraging people to create junk outputs to redeem later.
    if (nTxSize == 0)
        nTxSize = (GetTransactionWeight(*this) + WITNESS_SCALE_FACTOR - 1) / WITNESS_SCALE_FACTOR;
    for (std::vector<CTxIn>::const_iterator it(vin.begin()); it != vin.end(); ++it)
    {
        unsigned int offset = 41U + std::min(110U, (unsigned int)it->scriptSig.size());
        if (nTxSize > offset)
            nTxSize -= offset;
    }
    return nTxSize;
}

std::string CTransaction::ToString() const
{
      std::string str;
      str += IsCoinBase()? "Coinbase" : (IsCoinStake()? "Coinstake" : "CTransaction");
      str += strprintf("(hash=%s, nTime=%d, ver=%d, vin.size=%u, vout.size=%u, nLockTime=%d), strDZeel=%s, vchTxSig=%s, vchRangeProof=%s)\n",
          GetHash().ToString(),
          nTime,
          nVersion,
          vin.size(),
          vout.size(),
          nLockTime,
          strDZeel.substr(0,30).c_str(),
          HexStr(vchTxSig).substr(0,16).c_str(),
          HexStr(vchRangeProof).substr(0,16).c_str()
  	);
    for (unsigned int i = 0; i < vin.size(); i++)
        str += "    " + vin[i].ToString() + "\n";
    for (unsigned int i = 0; i < vout.size(); i++)
        str += "    " + vout[i].ToString() + "\n";
    return str;
}

std::string CMutableTransaction::ToString() const
{
      std::string str;
      str += "Mutable";
      str += IsCoinBase()? "Coinbase" : (IsCoinStake()? "Coinstake" : "CTransaction");
      str += strprintf("(hash=%s, nTime=%d, ver=%d, vin.size=%u, vout.size=%u, nLockTime=%d), strDZeel=%s, vchTxSig=%s, vchRangeProof=%s)\n",
          GetHash().ToString(),
          nTime,
          nVersion,
          vin.size(),
          vout.size(),
          nLockTime,
          strDZeel.substr(0,30).c_str(),
          HexStr(vchTxSig).substr(0,16).c_str(),
          HexStr(vchRangeProof).substr(0,16).c_str()
  	);
    for (unsigned int i = 0; i < vin.size(); i++)
        str += "    " + vin[i].ToString() + "\n";
    for (unsigned int i = 0; i < wit.vtxinwit.size(); i++)
        str += "    " + wit.vtxinwit[i].scriptWitness.ToString() + "\n";
    for (unsigned int i = 0; i < vout.size(); i++)
        str += "    " + vout[i].ToString() + "\n";
    return str;
}

int64_t GetTransactionWeight(const CTransaction& tx)
{
    return ::GetSerializeSize(tx, SER_NETWORK, PROTOCOL_VERSION | SERIALIZE_TRANSACTION_NO_WITNESS) * (WITNESS_SCALE_FACTOR -1) + ::GetSerializeSize(tx, SER_NETWORK, PROTOCOL_VERSION);
}
