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

class Predicate
{
public:
    ProgramActions action;
    std::vector<std::vector<unsigned char>> vParameters;
    std::vector<uint64_t> nParameters;
    std::vector<std::string> sParameters;
    std::vector<bls::G1Element> kParameters;
    std::vector<bls::G2Element> sigParameters;

    Predicate(const ProgramActions& action_) : action(action_) {
        vParameters.clear();
        nParameters.clear();
        sParameters.clear();
        kParameters.clear();
        sigParameters.clear();
    }

    void Push(const std::vector<unsigned char>& parameter) {
        vParameters.push_back(parameter);
    }

    void Push(const std::string& parameter) {
        sParameters.push_back(parameter);
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
            }
        } else {
            if (action == CREATE_TOKEN && kParameters.size() == 1 && sParameters.size() == 2 && nParameters.size() == 1)
            {
                READWRITE(kParameters[0]);
                READWRITE(sParameters[0]);
                READWRITE(sParameters[1]);
                READWRITE(nParameters[0]);
            }
            else if (action == MINT && kParameters.size() == 1 && nParameters.size() == 1)
            {
                READWRITE(kParameters[0]);
                READWRITE(nParameters[0]);
            }
            else if (action == STOP_MINT && kParameters.size() == 1)
            {
                READWRITE(kParameters[0]);
            }
            else if (action == BURN && nParameters.size() == 1)
            {
                READWRITE(nParameters[0]);
            }
        }
    }
};

#endif // NAVCOIN_PROGRAMS_H
