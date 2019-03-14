// Copyright (c) 2019 The NavCoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef ZEROWALLET_H
#define ZEROWALLET_H

#include "base58.h"
#include "wallet/wallet.h"
#include "libzeroct/CoinSpend.h"
#include "random.h"
#include "script/sign.h"
#include "zerochain.h"

#define MIN_MINT_SECURITY 100
#define WITNESS_ADDED_ENTROPY 40
#define DEFAULT_SPEND_MIN_MINT_COUNT 0

// NEEDS UNIT TEST
bool DestinationToVecRecipients(CAmount nValue, const std::string &address, vector<CRecipient> &vecSend,
                                bool fSubtractFeeFromAmount, bool fDonate, bool fShowDialog=false);
bool DestinationToVecRecipients(CAmount nValue, const CTxDestination &address, vector<CRecipient> &vecSend,
                                bool fSubtractFeeFromAmount, bool fDonate, bool fShowDialog=false);
bool PrepareAndSignCoinSpend(const BaseSignatureCreator& creator, const std::map<CBigNum,
                             PublicMintWitnessData>& mapWitness, const CCoinsViewCache& view,
                             const CScript& scriptPubKey, const CAmount& amount, CScript& ret, CBigNum& r,
                             bool fStake);
bool ProduceCoinSpend(const BaseSignatureCreator& creator, const std::map<CBigNum, PublicMintWitnessData>& mapWitness,
                      const CCoinsViewCache& view, const CScript& fromPubKey, SignatureData& sigdata,
                      bool fCoinStake, CAmount amount);

#endif // ZEROWALLET_H
