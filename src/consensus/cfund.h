// Copyright (c) 2017 The NavCoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef NAVCOIN_CFUND_H
#define NAVCOIN_CFUND_H

#include <stdint.h>

namespace CFund {
void SetScriptForCommunityFundContribution(CScript &script)
{
    script.resize(4);
    script[0] = OP_RETURN;
    script[1] = 0x20;
    script[2] = 0x20;
    script[3] = 0x20;
}
}

#endif // NAVCOIN_CONSENSUS_CONSENSUS_H
