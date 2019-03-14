/**
* @file       Keys.cpp
*
* @brief      Keys class for the Zerocoin library.
*
* @author     alex v
* @date       September 2018
*
* @copyright  Copyright 2018 alex v
* @license    This project is released under the MIT license.
**/
// Copyright (c) 2018-2019 The NavCoin Core developers

#ifndef KEYS_H
#define KEYS_H

#include "amount.h"
#include "key.h"
#include "Params.h"

#include <vector>

namespace libzeroct {
typedef std::pair<CBigNum,CBigNum> ObfuscationValue;
typedef std::pair<CBigNum,CBigNum> BlindingCommitment;
class CPrivateAddress
{
public:
    CPrivateAddress(const ZeroCTParams* p, std::string pid = "", CAmount amount = 0) : params(p), strPid(pid), nAmount(amount) { }
    CPrivateAddress(const ZeroCTParams* p, BlindingCommitment blindingCommitment, CPubKey zeroKey, std::string pid = "", CAmount amount = 0) : params(p), bc1(blindingCommitment.first.getvch()), bc2(blindingCommitment.second.getvch()), zpk(zeroKey), strPid(pid), nAmount(amount) {
        unsigned int vectorSize = (params->coinCommitmentGroup.modulus.bitSize()/8)+1;
        bc1.resize(vectorSize);
        bc2.resize(vectorSize);
    }
    CPrivateAddress(const ZeroCTParams* p, BlindingCommitment blindingCommitment, CKey zeroKey, std::string pid = "", CAmount amount = 0) : params(p), bc1(blindingCommitment.first.getvch()), bc2(blindingCommitment.second.getvch()), zpk(zeroKey.GetPubKey()), strPid(pid), nAmount(amount) {
        unsigned int vectorSize = (params->coinCommitmentGroup.modulus.bitSize()/8)+1;
        bc1.resize(vectorSize);
        bc2.resize(vectorSize);
    }

    bool GetBlindingCommitment(BlindingCommitment& blindingCommitment) const;
    bool GetPubKey(CPubKey& zerokey) const;

    void SetPaymentId(std::string pid) {
        strPid = pid;
    }

    std::string GetPaymentId() const {
        return strPid;
    }

    void SetAmount(CAmount amount) {
        nAmount = amount;
    }

    CAmount GetAmount() const {
        return nAmount;
    }

    void SetGamma(CBigNum gamma) const {
        bnGamma = gamma;
    }

    CBigNum GetGamma() const {
        return bnGamma;
    }

    const ZeroCTParams* GetParams() const { return params; }

    bool operator<(const CPrivateAddress& rhs) const {
        BlindingCommitment lhsBN; BlindingCommitment rhsBN;
        this->GetBlindingCommitment(lhsBN);
        rhs.GetBlindingCommitment(rhsBN);
        return lhsBN.first < rhsBN.first;
    }

    bool operator==(const CPrivateAddress& rhs) const {
        return this->bc1 == rhs.bc1 && this->bc2 == rhs.bc2 && this->zpk == rhs.zpk;
    }

    ADD_SERIALIZE_METHODS;
    template <typename Stream, typename Operation>  inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(bc1);
        READWRITE(bc2);
        READWRITE(zpk);
    }
private:
    const ZeroCTParams* params;
    std::vector<unsigned char> bc1;
    std::vector<unsigned char> bc2;
    CPubKey zpk;
    std::string strPid;
    CAmount nAmount;
    mutable CBigNum bnGamma;
};
class CPrivateViewKey
{
public:
    CPrivateViewKey(const ZeroCTParams* p) : params(p) { }
    CPrivateViewKey(const ZeroCTParams* p, BlindingCommitment blindingCommitment, CPrivKey zeroPrivKey) : params(p), bc1(blindingCommitment.first.getvch()), bc2(blindingCommitment.second.getvch()), zpk(zeroPrivKey) {
        unsigned int vectorSize = (params->coinCommitmentGroup.modulus.bitSize()/8)+1;
        bc1.resize(vectorSize);
        bc2.resize(vectorSize);
    }

    bool GetBlindingCommitment(BlindingCommitment& blindingCommitment) const;
    bool GetPrivKey(CPrivKey& zerokey) const;

    ADD_SERIALIZE_METHODS;
    template <typename Stream, typename Operation>  inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(bc1);
        READWRITE(bc2);
        READWRITE(zpk);
    }
private:
    const ZeroCTParams* params;
    std::vector<unsigned char> bc1;
    std::vector<unsigned char> bc2;
    CPrivKey zpk;
};
class CPrivateSpendKey
{
public:
    CPrivateSpendKey(const ZeroCTParams* p) : params(p) { }
    CPrivateSpendKey(const ZeroCTParams* p, libzeroct::ObfuscationValue obfuscationJ, libzeroct::ObfuscationValue obfuscationK, CPrivKey zeroPrivKey) : params(p), oj1(obfuscationJ.first.getvch()), oj2(obfuscationJ.second.getvch()), ok1(obfuscationK.first.getvch()), ok2(obfuscationK.second.getvch()), zpk(zeroPrivKey) {
        unsigned int vectorSize = (params->coinCommitmentGroup.groupOrder.bitSize()/8)+1;
        oj1.resize(vectorSize);
        oj2.resize(vectorSize);
        ok1.resize(vectorSize);
        ok2.resize(vectorSize);
    }

    bool GetObfuscationJ(libzeroct::ObfuscationValue& obfuscationJ) const;
    bool GetObfuscationK(libzeroct::ObfuscationValue& obfuscationK) const;
    bool GetPrivKey(CPrivKey& zerokey) const;

    ADD_SERIALIZE_METHODS;
    template <typename Stream, typename Operation>  inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(oj1);
        READWRITE(oj2);
        READWRITE(ok1);
        READWRITE(ok2);
        READWRITE(zpk);
    }
private:
    const ZeroCTParams* params;
    std::vector<unsigned char> oj1;
    std::vector<unsigned char> oj2;
    std::vector<unsigned char> ok1;
    std::vector<unsigned char> ok2;
    CPrivKey zpk;
};
void GenerateParameters(const ZeroCTParams* params, std::vector<unsigned char> seed, libzeroct::ObfuscationValue& oj, libzeroct::ObfuscationValue& ok, libzeroct::BlindingCommitment& bc, CKey& zk);
}
#endif // KEYS_H
