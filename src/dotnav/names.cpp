// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Copyright (c) 2021 The Navcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <dotnav/names.h>

namespace DotNav
{

std::string to_lower(const std::string& str) {
    std::string ret;

    ret.resize(str.size());

    std::transform(str.begin(),
                   str.end(),
                   ret.begin(),
                   ::tolower);

    return ret;
}
bool IsValid(const std::string& name_)
{
    auto name = to_lower(name_);

    return name.substr(1, name.size()-5).find_first_not_of("abcdefghijklmnopqrstuvwxyz01234566789") == std::string::npos &&
            name.substr(0, 1).find_first_not_of("abcdefghijklmnopqrstuvwxyz") == std::string::npos &&
            name.size() <= 64 && name.size() >= 5 && name.substr(name.size()-4, 4) == ".nav";
}

bool IsValidKey(const std::string& name_)
{
    auto name = to_lower(name_);

    return name.substr(1).find_first_not_of("abcdefghijklmnopqrstuvwxyz01234566789") == std::string::npos &&
            name.substr(0, 1).find_first_not_of("abcdefghijklmnopqrstuvwxyz") == std::string::npos &&
            name.size() <= 64 && name.size() >= 1;
}

uint256 GetHashIdName(const std::string& name, const bls::G1Element& key)
{
    auto name_ = to_lower(name);

    return SerializeHash(name_ + HexStr(key.Serialize()));
}

uint256 GetHashName(const std::string& name)
{
    auto name_ = to_lower(name);

    return SerializeHash(name_);
}

std::vector<unsigned char> GetRegisterProgram(const std::string& name, const bls::G1Element& key)
{
    Predicate ret(REGISTER_NAME);
    ret.Push(GetHashIdName(name, key));
    return ret.GetVchData();
}

std::vector<unsigned char> GetRenewProgram(const std::string& name)
{
    auto name_ = to_lower(name);

    Predicate ret(RENEW_NAME);
    ret.Push(name_);
    return ret.GetVchData();
}

std::vector<unsigned char> GetUpdateProgram(const std::string& name, const bls::G1Element& pk, const std::string& key, const std::string& value)
{
    auto name_ = to_lower(name);
    auto key_ = to_lower(key);

    Predicate ret(UPDATE_NAME);
    ret.Push(name_);
    ret.Push(pk);
    ret.Push(key_);
    ret.Push(value);
    return ret.GetVchData();
}

std::vector<unsigned char> GetUpdateFirstProgram(const std::string& name, const bls::G1Element& pk, const std::string& key, const std::string& value)
{
    auto name_ = to_lower(name);
    auto key_ = to_lower(key);

    Predicate ret(UPDATE_NAME_FIRST);
    ret.Push(name_);
    ret.Push(pk);
    ret.Push(key_);
    ret.Push(value);
    return ret.GetVchData();
}

std::map<std::string, std::string> Consolidate(const NameDataValues& data)
{
    std::map<std::string, std::string> ret;

    for (auto &it: data) {
        if (it.second.value != "")
            ret[it.second.key] = it.second.value;
        else if (ret.count(it.second.key))
            ret.erase(it.second.key);
    }

    return ret;
}
}
