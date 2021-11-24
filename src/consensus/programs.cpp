// Copyright (c) 2018-2021 The Navcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <consensus/programs.h>

std::string PredicateToStr(const std::vector<unsigned char>& program) {
    try {
        Predicate p(program);

        std::string ret = "";

        switch(p.action)
        {
        case UPDATE_NAME:
            ret += "UPDATE_NAME: ";
            ret += (p.sParameters[0]) +" ";
            ret += HexStr(p.kParameters[0].Serialize()) +" ";
            ret += (p.sParameters[1]) +" ";
            ret += (p.sParameters[2]) +" ";
            ret += (p.sParameters[3]);
            break;
        case UPDATE_NAME_FIRST:
            ret += "UPDATE_NAME_FIRST: ";
            ret += (p.sParameters[0]) +" ";
            ret += HexStr(p.kParameters[0].Serialize()) +" ";
            ret += (p.sParameters[1]) +" ";
            ret += (p.sParameters[2]) +" ";
            ret += (p.sParameters[3]);
            break;
        case STOP_MINT:
            ret += "STOP_MINT:";
            ret += HexStr(p.kParameters[0].Serialize());
            break;
        case NO_PROGRAM:
            ret += "NO_PROGRAM";
            break;
        case ERR:
            ret += "ERR: "+HexStr(program);
            break;
        case CREATE_TOKEN:
            ret += "CREATE_TOKEN: ";
            ret += HexStr(p.kParameters[0].Serialize()) +" ";
            ret += (p.sParameters[0]) +" ";
            ret += std::to_string(p.nParameters[0]) +" ";
            ret += (p.sParameters[1]) +" ";
            ret += std::to_string(p.nParameters[1]);
            break;
        case MINT:
            ret += "MINT: ";
            ret += HexStr(p.kParameters[0].Serialize()) +" ";
            ret += std::to_string(p.nParameters[0]) +" ";
            ret += HexStr(p.vParameters[0]);
            break;
        case BURN:
            ret += "BURN: ";

            ret += std::to_string(p.nParameters[0]) +" ";
            ret += HexStr(p.vParameters[0]);
            break;
        case REGISTER_NAME:
            ret += "REGISTER_NAME: ";

            ret += p.hParameters[0].ToString();
            break;
        case RENEW_NAME:
            ret += "RENEW_NAME: ";

            ret += p.sParameters[0];
            break;
        }
        return ret;
    }  catch (...) {
        return "ERR";
    }
}
