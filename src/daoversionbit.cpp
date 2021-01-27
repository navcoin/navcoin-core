// Copyright (c) 2019-2020 The Navcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "daoversionbit.h"

#include <algorithm>

void VoteVersionBit(int bit, bool vote)
{
    std::string sBit = std::to_string(bit);

    RemoveConfigFile("acceptversionbit", sBit);
    RemoveConfigFile("rejectversionbit", sBit);

    std::vector<std::string>& versionBitVotesRejected = mapMultiArgs["-rejectversionbit"];
    std::vector<std::string>& versionBitVotesAccepted = mapMultiArgs["-acceptversionbit"];

    std::vector<std::string>::iterator positionRejected = std::find(versionBitVotesRejected.begin(), versionBitVotesRejected.end(), sBit);
    std::vector<std::string>::iterator positionAccepted = std::find(versionBitVotesAccepted.begin(), versionBitVotesAccepted.end(), sBit);

    if (positionRejected != versionBitVotesRejected.end())
        versionBitVotesRejected.erase(positionRejected);

    if (positionAccepted != versionBitVotesAccepted.end())
        versionBitVotesAccepted.erase(positionAccepted);

    if (vote)
    {
        WriteConfigFile("acceptversionbit", sBit);
        versionBitVotesAccepted.push_back(sBit);
    }
    else
    {
        WriteConfigFile("rejectversionbit", sBit);
        versionBitVotesRejected.push_back(sBit);
    }
}
