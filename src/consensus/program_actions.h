// Copyright (c) 2018-2021 The Navcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef PROGRAM_ACTIONS_H
#define PROGRAM_ACTIONS_H

enum ProgramActions
{
    NO_PROGRAM,
    ERR,
    CREATE_TOKEN,
    MINT,
    STOP_MINT,
    BURN,
    REGISTER_NAME,
    UPDATE_NAME_FIRST,
    UPDATE_NAME,
    RENEW_NAME
};

#endif // PROGRAM_ACTIONS_H
