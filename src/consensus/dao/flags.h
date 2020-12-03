// Copyright (c) 2019 The NavCoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef FLAGS_H
#define FLAGS_H


namespace DAOFlags
{
typedef unsigned int flags;
static const flags NIL = 0x0;
static const flags ACCEPTED = 0x1;
static const flags REJECTED = 0x2;
static const flags EXPIRED = 0x3;
static const flags PENDING_FUNDS = 0x4;
static const flags PENDING_VOTING_PREQ = 0x5;
static const flags PAID = 0x6;
static const flags PASSED = 0x7;
static const flags REFLECTION = 0x8;
static const flags SUPPORTED = 0x9;
static const flags ACCEPTED_EXPIRED = 0x10;
}
namespace VoteFlags
{
static const int64_t VOTE_YES = 1;
static const int64_t VOTE_NO = 0;
static const int64_t VOTE_ABSTAIN = -1;
static const int64_t VOTE_REMOVE = -2;
static const int64_t SUPPORT = -3;
static const int64_t SUPPORT_REMOVE = -4;
}

#endif // FLAGS_H
