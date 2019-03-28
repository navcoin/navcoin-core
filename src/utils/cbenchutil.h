// Copyright (c) 2019 The NavCoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "utiltime.h"

#ifndef CBENCHUTIL_H
#define CBENCHUTIL_H


class CBenchUtil
{
public:
    CBenchUtil()
    {
        vCheckpoints.push_back(GetTimeMicros());
    }
    float Checkpoint()
    {
        vCheckpoints.push_back(GetTimeMicros());
        assert(vCheckpoints.size() >= 2);
        return (vCheckpoints[vCheckpoints.size()-1] - vCheckpoints[vCheckpoints.size()-2]) * 0.001;
    }
private:
    std::vector<int64_t> vCheckpoints;
};

#endif // CBENCHUTIL_H
