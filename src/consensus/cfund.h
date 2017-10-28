// Copyright (c) 2017 The NavCoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef NAVCOIN_CFUND_H
#define NAVCOIN_CFUND_H

#include "amount.h"
#include "script/script.h"

namespace CFund {
void SetScriptForCommunityFundContribution(CScript &script);
CAmount nMinimalFee = 10000000000;
}

#endif // NAVCOIN_CFUND_H
