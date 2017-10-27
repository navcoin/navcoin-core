// Copyright (c) 2017 The NavCoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef NAVCOIN_CFUND_H
#define NAVCOIN_CFUND_H

#include <stdint.h>
#include "script/script.h"

#define MIN_PROPOSAL_FEE 10000000000

namespace CFund {
void SetScriptForCommunityFundContribution(CScript &script);
}

#endif // NAVCOIN_CFUND_H
