// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <script/standard.h>

#include <blsct/key.h>
#include <pubkey.h>
#include <script/script.h>
#include <script/sign.h>
#include <util.h>
#include <utilstrencodings.h>

using namespace std;

typedef vector<unsigned char> valtype;

bool fAcceptDatacarrier = DEFAULT_ACCEPT_DATACARRIER;
unsigned nMaxDatacarrierBytes = MAX_OP_RETURN_RELAY;

CScriptID::CScriptID(const CScript& in) : uint160(Hash160(in.begin(), in.end())) {}

const char* GetTxnOutputType(txnouttype t)
{
    switch (t)
    {
    case TX_NONSTANDARD: return "nonstandard";
    case TX_PUBKEY: return "pubkey";
    case TX_PUBKEYHASH: return "pubkeyhash";
    case TX_SCRIPTHASH: return "scripthash";
    case TX_MULTISIG: return "multisig";
    case TX_NULL_DATA: return "nulldata";
    case TX_CONTRIBUTION: return "cfund_contribution";
    case TX_PROPOSALYESVOTE: return "proposal_yes_vote";
    case TX_PAYMENTREQUESTYESVOTE: return "payment_request_yes_vote";
    case TX_PROPOSALNOVOTE: return "proposal_no_vote";
    case TX_PAYMENTREQUESTNOVOTE: return "payment_request_no_vote";
    case TX_PROPOSALABSVOTE: return "proposal_abstain_vote";
    case TX_PROPOSALREMOVEVOTE: return "proposal_remove_vote";
    case TX_PAYMENTREQUESTABSVOTE: return "payment_request_abstain_vote";
    case TX_PAYMENTREQUESTREMOVEVOTE: return "payment_request_remove_vote";
    case TX_CONSULTATIONVOTE: return "consultation_vote";
    case TX_CONSULTATIONVOTEREMOVE: return "consultation_vote_remove";
    case TX_CONSULTATIONVOTEABSTENTION: return "consultation_vote_abstention";
    case TX_DAOSUPPORT: return "dao_support";
    case TX_DAOSUPPORTREMOVE: return "dao_support_remove";
    case TX_WITNESS_V0_KEYHASH: return "witness_v0_keyhash";
    case TX_WITNESS_V0_SCRIPTHASH: return "witness_v0_scripthash";
    case TX_COLDSTAKING: return "cold_staking";
    case TX_COLDSTAKING_V2: return "cold_staking_v2";
    case TX_POOL: return "pool_staking";
    }
    return nullptr;
}

/**
 * Return public keys or hashes from scriptPubKey, for 'standard' transaction types.
 */
bool Solver(const CScript& scriptPubKey, txnouttype& typeRet, vector<vector<unsigned char> >& vSolutionsRet)
{
    // Templates
    static multimap<txnouttype, CScript> mTemplates;
    if (mTemplates.empty())
    {
        // Standard tx, sender provides pubkey, receiver adds signature
        mTemplates.insert(make_pair(TX_PUBKEY, CScript() << OP_PUBKEY << OP_CHECKSIG));

        // Navcoin address tx, sender provides hash of pubkey, receiver provides signature and pubkey
        mTemplates.insert(make_pair(TX_PUBKEYHASH, CScript() << OP_DUP << OP_HASH160 << OP_PUBKEYHASH << OP_EQUALVERIFY << OP_CHECKSIG));

        // Sender provides N pubkeys, receivers provides M signatures
        mTemplates.insert(make_pair(TX_MULTISIG, CScript() << OP_SMALLINTEGER << OP_PUBKEYS << OP_SMALLINTEGER << OP_CHECKMULTISIG));
    }

    vSolutionsRet.clear();

    // Shortcut for pay-to-script-hash, which are more constrained than the other types:
    // it is always OP_HASH160 20 [20 byte hash] OP_EQUAL
    if (scriptPubKey.IsPayToScriptHash())
    {
        typeRet = TX_SCRIPTHASH;
        vector<unsigned char> hashBytes(scriptPubKey.begin()+2, scriptPubKey.begin()+22);
        vSolutionsRet.push_back(hashBytes);
        return true;
    }

    // Shortcut for cold stake, so we don't need to match a template
    if (scriptPubKey.IsColdStaking() && scriptPubKey.size() == 1+1+25+1+25+1)
    {
        typeRet = TX_COLDSTAKING;
        vector<unsigned char> stakingPubKey(scriptPubKey.begin()+5, scriptPubKey.begin()+25);
        vSolutionsRet.push_back(stakingPubKey);
        vector<unsigned char> spendingPubKey(scriptPubKey.begin()+31, scriptPubKey.begin()+51);
        vSolutionsRet.push_back(spendingPubKey);
        return true;
    }

    if (scriptPubKey.IsColdStakingv2() && scriptPubKey.size() == 1+1+25+1+25+1+22)
    {
        typeRet = TX_COLDSTAKING_V2;
        vector<unsigned char> stakingPubKey(scriptPubKey.begin()+27, scriptPubKey.begin()+47);
        vSolutionsRet.push_back(stakingPubKey);
        vector<unsigned char> spendingPubKey(scriptPubKey.begin()+53, scriptPubKey.begin()+73);
        vSolutionsRet.push_back(spendingPubKey);
        vector<unsigned char> votingPubKey(scriptPubKey.begin()+1, scriptPubKey.begin()+21);
        vSolutionsRet.push_back(votingPubKey);
        return true;
    }

    if (scriptPubKey.IsCommunityFundContribution())
    {
        typeRet = TX_CONTRIBUTION;
        return true;
    }

    if(scriptPubKey.IsProposalVote() || scriptPubKey.IsPaymentRequestVote())
    {
        if (scriptPubKey.IsProposalVoteYes())
            typeRet = TX_PROPOSALYESVOTE;
        else if (scriptPubKey.IsProposalVoteNo())
            typeRet = TX_PROPOSALNOVOTE;
        else if (scriptPubKey.IsProposalVoteAbs())
            typeRet = TX_PROPOSALABSVOTE;
        else if (scriptPubKey.IsProposalVoteRemove())
            typeRet = TX_PROPOSALREMOVEVOTE;
        else if (scriptPubKey.IsPaymentRequestVoteYes())
            typeRet = TX_PAYMENTREQUESTYESVOTE;
        else if (scriptPubKey.IsPaymentRequestVoteNo())
            typeRet = TX_PAYMENTREQUESTNOVOTE;
        else if (scriptPubKey.IsPaymentRequestVoteAbs())
            typeRet = TX_PAYMENTREQUESTABSVOTE;
        else if (scriptPubKey.IsPaymentRequestVoteRemove())
            typeRet = TX_PAYMENTREQUESTREMOVEVOTE;
        vector<unsigned char> hashBytes(scriptPubKey.begin()+5, scriptPubKey.begin()+37);
        vSolutionsRet.push_back(hashBytes);
        return true;
    }

    if(scriptPubKey.IsSupportVote())
    {
        if(scriptPubKey.IsSupportVoteYes())
            typeRet = TX_DAOSUPPORT;
        else if(scriptPubKey.IsSupportVoteRemove())
            typeRet = TX_DAOSUPPORTREMOVE;
        vector<unsigned char> hashBytes(scriptPubKey.begin()+4, scriptPubKey.begin()+36);
        vSolutionsRet.push_back(hashBytes);
        return true;
    }

    if(scriptPubKey.IsConsultationVote())
    {
        if(scriptPubKey.IsConsultationVoteAnswer())
            typeRet = TX_CONSULTATIONVOTE;
        else if(scriptPubKey.IsConsultationVoteAbstention())
            typeRet = TX_CONSULTATIONVOTEABSTENTION;
        else if(scriptPubKey.IsConsultationVoteRemove())
            typeRet = TX_CONSULTATIONVOTEREMOVE;
        vector<unsigned char> hashBytes(scriptPubKey.begin()+4, scriptPubKey.begin()+36);
        vSolutionsRet.push_back(hashBytes);
        if (scriptPubKey.size() > 36)
        {
            vector<unsigned char> vVote(scriptPubKey.begin()+37, scriptPubKey.end());
            vSolutionsRet.push_back(vVote);
        }
        return true;
    }

    if(scriptPubKey.IsPool())
    {
        typeRet = TX_POOL;
        return true;
    }

    int witnessversion;
    std::vector<unsigned char> witnessprogram;
    if (scriptPubKey.IsWitnessProgram(witnessversion, witnessprogram)) {
        if (witnessversion == 0 && witnessprogram.size() == 20) {
            typeRet = TX_WITNESS_V0_KEYHASH;
            vSolutionsRet.push_back(witnessprogram);
            return true;
        }
        if (witnessversion == 0 && witnessprogram.size() == 32) {
            typeRet = TX_WITNESS_V0_SCRIPTHASH;
            vSolutionsRet.push_back(witnessprogram);
            return true;
        }
        return false;
    }

    // Provably prunable, data-carrying output
    //
    // So long as script passes the IsUnspendable() test and all but the first
    // byte passes the IsPushOnly() test we don't care what exactly is in the
    // script.
    if (scriptPubKey.size() >= 1 && scriptPubKey[0] == OP_RETURN && scriptPubKey.IsPushOnly(scriptPubKey.begin()+1)) {
        typeRet = TX_NULL_DATA;
        return true;
    }

    // Scan templates
    const CScript& script1 = scriptPubKey;
    for(const PAIRTYPE(txnouttype, CScript)& tplate: mTemplates)
    {
        const CScript& script2 = tplate.second;
        vSolutionsRet.clear();

        opcodetype opcode1, opcode2;
        vector<unsigned char> vch1, vch2;

        // Compare
        CScript::const_iterator pc1 = script1.begin();
        CScript::const_iterator pc2 = script2.begin();
        while (true)
        {
            if (pc1 == script1.end() && pc2 == script2.end())
            {
                // Found a match
                typeRet = tplate.first;
                if (typeRet == TX_MULTISIG)
                {
                    // Additional checks for TX_MULTISIG:
                    unsigned char m = vSolutionsRet.front()[0];
                    unsigned char n = vSolutionsRet.back()[0];
                    if (m < 1 || n < 1 || m > n || vSolutionsRet.size()-2 != n)
                        return false;
                }
                return true;
            }
            if (!script1.GetOp(pc1, opcode1, vch1))
                break;
            if (!script2.GetOp(pc2, opcode2, vch2))
                break;

            // Template matching opcodes:
            if (opcode2 == OP_PUBKEYS)
            {
                while (vch1.size() >= 33 && vch1.size() <= 65)
                {
                    vSolutionsRet.push_back(vch1);
                    if (!script1.GetOp(pc1, opcode1, vch1))
                        break;
                }
                if (!script2.GetOp(pc2, opcode2, vch2))
                    break;
                // Normal situation is to fall through
                // to other if/else statements
            }

            if (opcode2 == OP_PUBKEY)
            {
                if (vch1.size() < 33 || vch1.size() > 65)
                    break;
                vSolutionsRet.push_back(vch1);
            }
            else if (opcode2 == OP_PUBKEYHASH)
            {
                if (vch1.size() != sizeof(uint160))
                    break;
                vSolutionsRet.push_back(vch1);
            }
            else if (opcode2 == OP_SMALLINTEGER)
            {   // Single-byte small integer pushed onto vSolutions
                if (opcode1 == OP_0 ||
                    (opcode1 >= OP_1 && opcode1 <= OP_16))
                {
                    char n = (char)CScript::DecodeOP_N(opcode1);
                    vSolutionsRet.push_back(valtype(1, n));
                }
                else
                    break;
            }
            else if (opcode1 != opcode2 || vch1 != vch2)
            {
                // Others must match exactly
                break;
            }
        }
    }

    vSolutionsRet.clear();
    typeRet = TX_NONSTANDARD;
    return false;
}

bool ExtractDestination(const CScript& scriptPubKey, CTxDestination& addressRet)
{
    vector<valtype> vSolutions;
    txnouttype whichType;

    if (!Solver(scriptPubKey, whichType, vSolutions))
        return false;

    if (whichType == TX_PUBKEY)
    {
        CPubKey pubKey(vSolutions[0]);
        if (!pubKey.IsValid())
            return false;

        addressRet = pubKey.GetID();
        return true;
    }
    else if (whichType == TX_PUBKEYHASH)
    {
        addressRet = CKeyID(uint160(vSolutions[0]));
        return true;
    }
    else if (whichType == TX_SCRIPTHASH)
    {
        addressRet = CScriptID(uint160(vSolutions[0]));
        return true;
    }
    else if (whichType == TX_COLDSTAKING)
    {
        addressRet = make_pair(CKeyID(uint160(vSolutions[0])), CKeyID(uint160(vSolutions[1])));
        return true;
    }
    else if (whichType == TX_COLDSTAKING_V2)
    {
        addressRet = make_pair(CKeyID(uint160(vSolutions[0])), make_pair(CKeyID(uint160(vSolutions[1])), CKeyID(uint160(vSolutions[2]))));
        return true;
    }
    // Multisig txns have more than one address...
    return false;
}

bool ExtractDestinations(const CScript& scriptPubKey, txnouttype& typeRet, vector<CTxDestination>& addressRet, int& nRequiredRet)
{
    addressRet.clear();
    typeRet = TX_NONSTANDARD;
    vector<valtype> vSolutions;

    if (!Solver(scriptPubKey, typeRet, vSolutions))
        return false;
    if (typeRet == TX_NULL_DATA){
        // This is data, not addresses
        return false;
    }

    if (typeRet == TX_MULTISIG)
    {
        nRequiredRet = vSolutions.front()[0];
        for (unsigned int i = 1; i < vSolutions.size()-1; i++)
        {
            CPubKey pubKey(vSolutions[i]);
            if (!pubKey.IsValid())
                continue;

            CTxDestination address = pubKey.GetID();
            addressRet.push_back(address);
        }

        if (addressRet.empty())
            return false;
    }
    else if (typeRet == TX_COLDSTAKING || typeRet == TX_COLDSTAKING_V2)
    {
        nRequiredRet = 1;
        for (unsigned int i = 0; i < vSolutions.size(); i++)
        {
            uint160 keyInt(vSolutions[i]);
            CKeyID keyID(keyInt);
            addressRet.push_back(keyID);
        }

        if (addressRet.empty())
            return false;
    }
    else if (typeRet == TX_CONTRIBUTION || typeRet == TX_PAYMENTREQUESTNOVOTE || typeRet == TX_PAYMENTREQUESTYESVOTE
             || typeRet == TX_PAYMENTREQUESTREMOVEVOTE || typeRet == TX_PAYMENTREQUESTABSVOTE
             || typeRet == TX_PROPOSALNOVOTE || typeRet == TX_PROPOSALYESVOTE
             || typeRet == TX_PROPOSALABSVOTE || typeRet == TX_PROPOSALREMOVEVOTE
             || typeRet == TX_DAOSUPPORT || typeRet == TX_DAOSUPPORTREMOVE || typeRet == TX_CONSULTATIONVOTE
             || typeRet == TX_CONSULTATIONVOTEABSTENTION || typeRet == TX_CONSULTATIONVOTEREMOVE)
    {
        return true;
    }
    else
    {
        nRequiredRet = 1;
        CTxDestination address;
        if (!ExtractDestination(scriptPubKey, address))
           return false;
        addressRet.push_back(address);
    }

    return true;
}

namespace
{
class CScriptVisitor : public boost::static_visitor<bool>
{
private:
    CScript *script;
public:
    CScriptVisitor(CScript *scriptin) { script = scriptin; }

    bool operator()(const CNoDestination &dest) const {
        script->clear();
        return false;
    }

    bool operator()(const CKeyID &keyID) const {
        script->clear();
        *script << OP_DUP << OP_HASH160 << ToByteVector(keyID) << OP_EQUALVERIFY << OP_CHECKSIG;
        return true;
    }

    bool operator()(const pair<CKeyID, CKeyID>&keyPairID) const {
        script->clear();
        *script << OP_COINSTAKE << OP_IF << OP_DUP << OP_HASH160 << ToByteVector(keyPairID.first) << OP_EQUALVERIFY << OP_CHECKSIG << OP_ELSE << OP_DUP << OP_HASH160 << ToByteVector(keyPairID.second) << OP_EQUALVERIFY << OP_CHECKSIG << OP_ENDIF;
        return true;
    }

    bool operator()(const pair<CKeyID, pair<CKeyID, CKeyID>>&keyPairID) const {
        script->clear();
        *script << ToByteVector(keyPairID.second.second) << OP_DROP << OP_COINSTAKE << OP_IF << OP_DUP << OP_HASH160 << ToByteVector(keyPairID.first) << OP_EQUALVERIFY << OP_CHECKSIG << OP_ELSE << OP_DUP << OP_HASH160 << ToByteVector(keyPairID.second.first) << OP_EQUALVERIFY << OP_CHECKSIG << OP_ENDIF;
        return true;
    }

    bool operator()(const CScriptID &scriptID) const {
        script->clear();
        *script << OP_HASH160 << ToByteVector(scriptID) << OP_EQUAL;
        return true;
    }

    bool operator()(const CScript &scriptIn) const {
        script->clear();
        *script += scriptIn;
        return true;
    }

    bool operator()(const blsctDoublePublicKey &dest) const {
        script->clear();
        *script << OP_TRUE;
        return false;
    }
};
}

CScript GetScriptForDestination(const CTxDestination& dest)
{
    CScript script;

    boost::apply_visitor(CScriptVisitor(&script), dest);
    return script;
}

CScript GetScriptForRawPubKey(const CPubKey& pubKey)
{
    return CScript() << std::vector<unsigned char>(pubKey.begin(), pubKey.end()) << OP_CHECKSIG;
}

CScript GetScriptForMultisig(int nRequired, const std::vector<CPubKey>& keys)
{
    CScript script;

    script << CScript::EncodeOP_N(nRequired);
    for(const CPubKey& key: keys)
        script << ToByteVector(key);
    script << CScript::EncodeOP_N(keys.size()) << OP_CHECKMULTISIG;
    return script;
}

CScript GetScriptForWitness(const CScript& redeemscript)
{
    CScript ret;

    txnouttype typ;
    std::vector<std::vector<unsigned char> > vSolutions;
    if (Solver(redeemscript, typ, vSolutions)) {
        if (typ == TX_PUBKEY) {
            unsigned char h160[20];
            CHash160().Write(&vSolutions[0][0], vSolutions[0].size()).Finalize(h160);
            ret << OP_0 << std::vector<unsigned char>(&h160[0], &h160[20]);
            return ret;
        } else if (typ == TX_PUBKEYHASH) {
           ret << OP_0 << vSolutions[0];
           return ret;
        }
    }
    uint256 hash;
    CSHA256().Write(&redeemscript[0], redeemscript.size()).Finalize(hash.begin());
    ret << OP_0 << ToByteVector(hash);
    return ret;
}
