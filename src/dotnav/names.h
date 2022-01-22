// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Copyright (c) 2021 The Navcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef NAVCOIN_NAMES_H
#define NAVCOIN_NAMES_H

#include <consensus/programs.h>
#include <dotnav/namedata.h>
#include <utilstrencodings.h>
#include <util.h>
#include <hash.h>
#include <locale>

namespace DotNav
{

std::string to_lower(const std::string& str);
bool IsValid(const std::string& name);
bool IsValidKey(const std::string& name);

uint256 GetHashIdName(const std::string& name, const bls::G1Element& key);

uint256 GetHashName(const std::string& name);

std::vector<unsigned char> GetRegisterProgram(const std::string& name, const bls::G1Element& key);

std::vector<unsigned char> GetRenewProgram(const std::string& name);

std::vector<unsigned char> GetUpdateProgram(const std::string& name, const bls::G1Element& pk, const std::string& key, const std::string& value, const std::string& subdomain="");

std::vector<unsigned char> GetUpdateFirstProgram(const std::string& name, const bls::G1Element& pk, const std::string& key, const std::string& value, const std::string& subdomain="");

std::map<std::string, std::string> Consolidate(const NameDataValues& data, const int32_t& nHeight, const std::string& subdomain="");
std::map<std::string, std::map<std::string, std::string>> ConsolidateSubdomains(const NameDataValues& data, const int32_t& nHeight);

size_t CalculateSize(const std::map<std::string, std::string>& map);
}

#endif // NAVCOIN_
