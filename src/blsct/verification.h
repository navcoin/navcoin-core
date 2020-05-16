// Copyright (c) 2020 The NavCoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BLSCT_VERIFICATION_H
#define BLSCT_VERIFICATION_H

#include <bls.hpp>
#include <blsct/bulletproofs.h>
#include <coins.h>
#include <consensus/validation.h>
#include <primitives/transaction.h>
#include <utiltime.h>

bool VerifyBLSCT(const CTransaction &tx, const bls::PrivateKey& viewKey, std::vector<RangeproofEncodedData> &vData, const CCoinsViewCache& view, CValidationState& state, bool fOnlyRecover = false, CAmount nMixFee = 0);
bool CombineBLSCTTransactions(std::vector<CTransaction> &vTx, CTransaction& outTx, const CCoinsViewCache& inputs, CValidationState& state, CAmount nMixFee = 0);
#endif // BLSCT_VERIFICATION_H
