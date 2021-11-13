// Copyright (c) 2018-2021 The Navcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef NAVCOIN_PROGRAMS_H
#define NAVCOIN_PROGRAMS_H

#include <vector>
#include <amount.h>
#include <bls.hpp>
#include <streams.h>
#include <consensus/program_actions.h>
#include <utilstrencodings.h>
#include <uint256.h>

class Predicate
{
public:
    ProgramActions action;
    std::vector<std::vector<unsigned char>> vParameters;
    std::vector<uint64_t> nParameters;
    std::vector<std::string> sParameters;
    std::vector<bls::G1Element> kParameters;
    std::vector<bls::G2Element> sigParameters;
    std::vector<uint256> hParameters;

    Predicate(const ProgramActions& action_) : action(action_) {
        vParameters.clear();
        nParameters.clear();
        sParameters.clear();
        kParameters.clear();
        sigParameters.clear();
        hParameters.clear();
    }

    void Push(const std::vector<unsigned char>& parameter) {
        vParameters.push_back(parameter);
    }

    void Push(const std::string& parameter) {
        sParameters.push_back(parameter);
    }

    void Push(const uint256& parameter) {
        hParameters.push_back(parameter);
    }

    void Push(const uint64_t& parameter) {
        nParameters.push_back(parameter);
    }

    void Push(const bls::G1Element& parameter) {
        kParameters.push_back(parameter);
    }

    void Push(const bls::G2Element& parameter) {
        sigParameters.push_back(parameter);
    }

    Predicate(const std::vector<unsigned char>& program) : action(NO_PROGRAM) {
        try
        {
            CDataStream strm(program, 0, 0);
            strm >> *this;
        }
        catch(...)
        {
            action = ERR;
        }
    }

    std::vector<unsigned char> GetVchData() {
        CDataStream ss(0,0);
        ss << *this;
        return std::vector<unsigned char>(ss.begin(), ss.end());
    }

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(action);

        if (ser_action.ForRead())
        {
            if (action == CREATE_TOKEN)
            {
                bls::G1Element id;
                READWRITE(id);
                Push(id);
                std::string name;
                READWRITE(name);
                Push(name);
                uint64_t v;
                READWRITE(v);
                Push(v);
                std::string desc;
                READWRITE(desc);
                Push(desc);
                CAmount supply;
                READWRITE(supply);
                Push(supply);
            } else if (action == MINT)
            {
                bls::G1Element id;
                READWRITE(id);
                Push(id);
                CAmount amount;
                READWRITE(amount);
                Push(amount);
                std::vector<unsigned char> data;
                READWRITE(data);
                Push(data);
            } else if (action == STOP_MINT)
            {
                bls::G1Element id;
                READWRITE(id);
                Push(id);
            } else if (action == BURN)
            {
                CAmount amount;
                READWRITE(amount);
                Push(amount);
                std::vector<unsigned char> data;
                READWRITE(data);
                Push(data);
            }
            else if (action == REGISTER_NAME)
            {
                uint256 hash;
                READWRITE(hash);
                Push(hash);
            }
            else if (action == UPDATE_NAME_FIRST)
            {
                std::string name;
                READWRITE(name);
                Push(name);
                bls::G1Element pk;
                READWRITE(pk);
                Push(pk);
                std::string key;
                READWRITE(key);
                Push(key);
                std::string value;
                READWRITE(value);
                Push(value);
            }
            else if (action == UPDATE_NAME)
            {
                std::string name;
                READWRITE(name);
                Push(name);
                bls::G1Element pk;
                READWRITE(pk);
                Push(pk);
                std::string key;
                READWRITE(key);
                Push(key);
                std::string value;
                READWRITE(value);
                Push(value);
            }
            else if (action == RENEW_NAME)
            {
                std::string name;
                READWRITE(name);
                Push(name);
            }
        } else {
            if (action == CREATE_TOKEN && kParameters.size() == 1 && sParameters.size() == 2 && nParameters.size() == 2)
            {
                READWRITE(kParameters[0]);
                READWRITE(sParameters[0]);
                READWRITE(nParameters[0]);
                READWRITE(sParameters[1]);
                READWRITE(nParameters[1]);
            }
            else if (action == MINT && kParameters.size() == 1 && nParameters.size() == 1 && vParameters.size() == 1)
            {
                READWRITE(kParameters[0]);
                READWRITE(nParameters[0]);
                READWRITE(vParameters[0]);
            }
            else if (action == STOP_MINT && kParameters.size() == 1)
            {
                READWRITE(kParameters[0]);
            }
            else if (action == BURN && nParameters.size() == 1 && vParameters.size() == 1)
            {
                READWRITE(nParameters[0]);
                READWRITE(vParameters[0]);
            }
            else if (action == REGISTER_NAME && hParameters.size() == 1)
            {
                READWRITE(hParameters[0]);
            }
            else if (action == UPDATE_NAME_FIRST && sParameters.size() == 3 && kParameters.size() == 1)
            {
                READWRITE(sParameters[0]);
                READWRITE(kParameters[0]);
                READWRITE(sParameters[1]);
                READWRITE(sParameters[2]);
            }
            else if (action == UPDATE_NAME && sParameters.size() == 3 && kParameters.size() == 1)
            {
                READWRITE(sParameters[0]);
                READWRITE(kParameters[0]);
                READWRITE(sParameters[1]);
                READWRITE(sParameters[2]);
            }
            else if (action == RENEW_NAME && sParameters.size() == 1)
            {
                READWRITE(sParameters[0]);
            }
        }
    }
};

std::string PredicateToStr(const std::vector<unsigned char>& program);

#endif // NAVCOIN_PROGRAMS_H
