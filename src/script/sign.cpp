// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The NavCoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "script/sign.h"

#include "key.h"
#include "keystore.h"
#include "policy/policy.h"
#include "primitives/transaction.h"
#include "script/standard.h"
#include "script/interpreter.h"
#include "uint256.h"

#include <boost/foreach.hpp>

using namespace std;

typedef std::vector<unsigned char> valtype;

uint256 SignatureHashNavcoin(CScript scriptCode, const CMutableTransaction& txTo, unsigned int nIn, int nHashType);

TransactionSignatureCreator::TransactionSignatureCreator(const CKeyStore* keystoreIn, const CTransaction* txToIn, unsigned int nInIn, const CAmount& amountIn, int nHashTypeIn) : BaseSignatureCreator(keystoreIn), txTo(txToIn), nIn(nInIn), nHashType(nHashTypeIn), amount(amountIn), checker(txTo, nIn, amountIn) {}

bool TransactionSignatureCreator::CreateSig(std::vector<unsigned char>& vchSig, const CKeyID& address, const CScript& scriptCode, SigVersion sigversion) const
{
    CKey key;
    if (!keystore->GetKey(address, key))
        return false;

    uint256 hash = SignatureHash(scriptCode, *txTo, nIn, nHashType, amount, sigversion);
    if (!key.Sign(hash, vchSig))
        return false;
    vchSig.push_back((unsigned char)nHashType);
    return true;
}

static bool Sign1(const CKeyID& address, const BaseSignatureCreator& creator, const CScript& scriptCode, std::vector<valtype>& ret, SigVersion sigversion)
{
    vector<unsigned char> vchSig;
    if (!creator.CreateSig(vchSig, address, scriptCode, sigversion))
        return false;
    ret.push_back(vchSig);
    return true;
}

static bool SignN(const vector<valtype>& multisigdata, const BaseSignatureCreator& creator, const CScript& scriptCode, std::vector<valtype>& ret, SigVersion sigversion)
{
    int nSigned = 0;
    int nRequired = multisigdata.front()[0];
    for (unsigned int i = 1; i < multisigdata.size()-1 && nSigned < nRequired; i++)
    {
        const valtype& pubkey = multisigdata[i];
        CKeyID keyID = CPubKey(pubkey).GetID();
        if (Sign1(keyID, creator, scriptCode, ret, sigversion))
            ++nSigned;
    }
    return nSigned==nRequired;
}

/**
 * Sign scriptPubKey using signature made with creator.
 * Signatures are returned in scriptSigRet (or returns false if scriptPubKey can't be signed),
 * unless whichTypeRet is TX_SCRIPTHASH, in which case scriptSigRet is the redemption script.
 * Returns false if scriptPubKey could not be completely satisfied.
 */
static bool SignStep(const BaseSignatureCreator& creator, const CScript& scriptPubKey,
                     std::vector<valtype>& ret, txnouttype& whichTypeRet, SigVersion sigversion)
{
    CScript scriptRet;
    uint160 h160;
    ret.clear();

    vector<valtype> vSolutions;
    if (!Solver(scriptPubKey, whichTypeRet, vSolutions))
        return false;

    CKeyID keyID;
    switch (whichTypeRet)
    {
    case TX_NONSTANDARD:
    case TX_NULL_DATA:
        return false;
    case TX_PUBKEY:
        keyID = CPubKey(vSolutions[0]).GetID();
        return Sign1(keyID, creator, scriptPubKey, ret, sigversion);
    case TX_PUBKEYHASH:
        keyID = CKeyID(uint160(vSolutions[0]));
        if (!Sign1(keyID, creator, scriptPubKey, ret, sigversion))
            return false;
        else
        {
            CPubKey vch;
            creator.KeyStore().GetPubKey(keyID, vch);
            ret.push_back(ToByteVector(vch));
        }
        return true;
    case TX_SCRIPTHASH:
        if (creator.KeyStore().GetCScript(uint160(vSolutions[0]), scriptRet)) {
            ret.push_back(std::vector<unsigned char>(scriptRet.begin(), scriptRet.end()));
            return true;
        }
        return false;

    case TX_MULTISIG:
        ret.push_back(valtype()); // workaround CHECKMULTISIG bug
        return (SignN(vSolutions, creator, scriptPubKey, ret, sigversion));

    case TX_WITNESS_V0_KEYHASH:
        ret.push_back(vSolutions[0]);
        return true;

    case TX_WITNESS_V0_SCRIPTHASH:
        CRIPEMD160().Write(&vSolutions[0][0], vSolutions[0].size()).Finalize(h160.begin());
        if (creator.KeyStore().GetCScript(h160, scriptRet)) {
            ret.push_back(std::vector<unsigned char>(scriptRet.begin(), scriptRet.end()));
            return true;
        }
        return false;

    default:
        return false;
    }
}

static CScript PushAll(const vector<valtype>& values)
{
    CScript result;
    BOOST_FOREACH(const valtype& v, values) {
        if (v.size() == 0) {
            result << OP_0;
        } else if (v.size() == 1 && v[0] >= 1 && v[0] <= 16) {
            result << CScript::EncodeOP_N(v[0]);
        } else {
            result << v;
        }
    }
    return result;
}

bool ProduceSignature(const BaseSignatureCreator& creator, const CScript& fromPubKey, SignatureData& sigdata)
{

    CScript script = fromPubKey;

    bool solved = true;

    std::vector<valtype> result;

    txnouttype whichType;

    solved = SignStep(creator, script, result, whichType, SIGVERSION_BASE);

    bool P2SH = false;
    CScript subscript;
    sigdata.scriptWitness.stack.clear();

    if (solved && whichType == TX_SCRIPTHASH)
    {
        // Solver returns the subscript that needs to be evaluated;
        // the final scriptSig is the signatures from that
        // and then the serialized subscript:
        script = subscript = CScript(result[0].begin(), result[0].end());
        solved = solved && SignStep(creator, script, result, whichType, SIGVERSION_BASE) && whichType != TX_SCRIPTHASH;
        P2SH = true;
    }

    if (solved && whichType == TX_WITNESS_V0_KEYHASH)
    {
        CScript witnessscript;
        witnessscript << OP_DUP << OP_HASH160 << ToByteVector(result[0]) << OP_EQUALVERIFY << OP_CHECKSIG;
        txnouttype subType;
        solved = solved && SignStep(creator, witnessscript, result, subType, SIGVERSION_WITNESS_V0);
        sigdata.scriptWitness.stack = result;
        result.clear();
    }
    else if (solved && whichType == TX_WITNESS_V0_SCRIPTHASH)
    {
        CScript witnessscript(result[0].begin(), result[0].end());
        txnouttype subType;
        solved = solved && SignStep(creator, witnessscript, result, subType, SIGVERSION_WITNESS_V0) && subType != TX_SCRIPTHASH && subType != TX_WITNESS_V0_SCRIPTHASH && subType != TX_WITNESS_V0_KEYHASH;
        result.push_back(std::vector<unsigned char>(witnessscript.begin(), witnessscript.end()));
        sigdata.scriptWitness.stack = result;
        result.clear();
    }

    if (P2SH) {
        result.push_back(std::vector<unsigned char>(subscript.begin(), subscript.end()));
    }
    sigdata.scriptSig = PushAll(result);

    // Test solution
    return solved && VerifyScript(sigdata.scriptSig, fromPubKey, &sigdata.scriptWitness, STANDARD_SCRIPT_VERIFY_FLAGS, creator.Checker());
}

SignatureData DataFromTransaction(const CMutableTransaction& tx, unsigned int nIn)
{
    SignatureData data;
    assert(tx.vin.size() > nIn);
    data.scriptSig = tx.vin[nIn].scriptSig;
    if (tx.wit.vtxinwit.size() > nIn) {
        data.scriptWitness = tx.wit.vtxinwit[nIn].scriptWitness;
    }
    return data;
}

void UpdateTransaction(CMutableTransaction& tx, unsigned int nIn, const SignatureData& data)
{
    assert(tx.vin.size() > nIn);
    tx.vin[nIn].scriptSig = data.scriptSig;
    if (!data.scriptWitness.IsNull() || tx.wit.vtxinwit.size() > nIn) {
        tx.wit.vtxinwit.resize(tx.vin.size());
        tx.wit.vtxinwit[nIn].scriptWitness = data.scriptWitness;
    }
}

void UpdateNoMutableTransaction(CTransaction& tx, unsigned int nIn, const SignatureData& data)
{
    assert(tx.vin.size() > nIn);
    tx.vin[nIn].scriptSig = data.scriptSig;
    if (!data.scriptWitness.IsNull() || tx.wit.vtxinwit.size() > nIn) {
        tx.wit.vtxinwit.resize(tx.vin.size());
        tx.wit.vtxinwit[nIn].scriptWitness = data.scriptWitness;
    }
}

bool SignNoMutableSignature(const CKeyStore &keystore, const CScript& fromPubKey, CTransaction& txTo, unsigned int nIn, const CAmount& amount, int nHashType)
{
    assert(nIn < txTo.vin.size());

    CTransaction txToConst(txTo);

    TransactionSignatureCreator creator(&keystore, &txToConst, nIn, amount, nHashType);

    SignatureData sigdata;

    bool ret = ProduceSignature(creator, fromPubKey, sigdata);

    UpdateNoMutableTransaction(txTo, nIn, sigdata);
    return ret;
}

bool SignSignature(const CKeyStore &keystore, const CScript& fromPubKey, CMutableTransaction& txTo, unsigned int nIn, const CAmount& amount, int nHashType)
{
    assert(nIn < txTo.vin.size());

    CTransaction txToConst(txTo);
    TransactionSignatureCreator creator(&keystore, &txToConst, nIn, amount, nHashType);

    SignatureData sigdata;
    bool ret = ProduceSignature(creator, fromPubKey, sigdata);
    UpdateTransaction(txTo, nIn, sigdata);
    return ret;
}

bool SignSignature(const CKeyStore &keystore, const CTransaction& txFrom, CMutableTransaction& txTo, unsigned int nIn, int nHashType)
{
    assert(nIn < txTo.vin.size());
    CTxIn& txin = txTo.vin[nIn];
    assert(txin.prevout.n < txFrom.vout.size());
    const CTxOut& txout = txFrom.vout[txin.prevout.n];

    return SignSignature(keystore, txout.scriptPubKey, txTo, nIn, txout.nValue, nHashType);
}

bool Sign1Navcoin(const CKeyID& address, const CKeyStore& keystore, uint256 hash, int nHashType, CScript& scriptSigRet)
{
    CKey key;
    if (!keystore.GetKey(address, key))
        return false;

    vector<unsigned char> vchSig;
    if (!key.Sign(hash, vchSig))
        return false;
    vchSig.push_back((unsigned char)nHashType);
    scriptSigRet << vchSig;

    return true;
}

bool SignNNavcoin(const vector<valtype>& multisigdata, const CKeyStore& keystore, uint256 hash, int nHashType, CScript& scriptSigRet)
{
    int nSigned = 0;
    int nRequired = multisigdata.front()[0];
    for (unsigned int i = 1; i < multisigdata.size()-1 && nSigned < nRequired; i++)
    {
        const valtype& pubkey = multisigdata[i];
        CKeyID keyID = CPubKey(pubkey).GetID();
        if (Sign1Navcoin(keyID, keystore, hash, nHashType, scriptSigRet))
            ++nSigned;
    }
    return nSigned==nRequired;
}

bool SignSignatureNavcoin(const CKeyStore &keystore, const CTransaction& txFrom, CMutableTransaction& txTo, unsigned int nIn, int nHashType)
{
    assert(nIn < txTo.vin.size());
    CTxIn& txin = txTo.vin[nIn];
    assert(txin.prevout.n < txFrom.vout.size());
    const CTxOut& txout = txFrom.vout[txin.prevout.n];

    return SignSignatureNavcoin(keystore, txout.scriptPubKey, txTo, nIn, nHashType);
}

bool SignSignatureNavcoin(const CKeyStore &keystore, const CScript& fromPubKey, CMutableTransaction& txTo, unsigned int nIn, int nHashType)
{
    assert(nIn < txTo.vin.size());
    CTxIn& txin = txTo.vin[nIn];

    // Leave out the signature from the hash, since a signature can't sign itself.
    // The checksig op will also drop the signatures from its hash.
    uint256 hash = SignatureHashNavcoin(fromPubKey, txTo, nIn, nHashType);

    txnouttype whichType;
    if (!SolverNavcoin(keystore, fromPubKey, hash, nHashType, txin.scriptSig, whichType))
        return false;

    if (whichType == TX_SCRIPTHASH)
    {
        // Solver returns the subscript that need to be evaluated;
        // the final scriptSig is the signatures from that
        // and then the serialized subscript:
        CScript subscript = txin.scriptSig;

        // Recompute txn hash using subscript in place of scriptPubKey:
        uint256 hash2 = SignatureHashNavcoin(subscript, txTo, nIn, nHashType);

        txnouttype subType;
        bool fSolved =
            SolverNavcoin(keystore, subscript, hash2, nHashType, txin.scriptSig, subType) && subType != TX_SCRIPTHASH;
        // Append serialized subscript whether or not it is completely signed:
        txin.scriptSig << subscript;
        if (!fSolved) return false;
    }

    // Test solution
    return VerifyScriptNavcoin(txin.scriptSig, fromPubKey, STANDARD_SCRIPT_VERIFY_FLAGS, SignatureCheckerNavcoin(txTo, nIn));
}

uint256 SignatureHashNavcoin(CScript scriptCode, const CMutableTransaction& txTo, unsigned int nIn, int nHashType)
{
    if (nIn >= txTo.vin.size())
    {
        return uint256S("0x1");
    }
    CMutableTransaction txTmp(txTo);

    // In case concatenating two scripts ends up with two codeseparators,
    // or an extra one at the end, this prevents all those possible incompatibilities.
    scriptCode.FindAndDelete(CScript(OP_CODESEPARATOR));

    // Blank out other inputs' signatures
    for (unsigned int i = 0; i < txTmp.vin.size(); i++)
        txTmp.vin[i].scriptSig = CScript();
    txTmp.vin[nIn].scriptSig = scriptCode;

    // Blank out some of the outputs
    if ((nHashType & 0x1f) == SIGHASH_NONE)
    {
        // Wildcard payee
        txTmp.vout.clear();

        // Let the others update at will
        for (unsigned int i = 0; i < txTmp.vin.size(); i++)
            if (i != nIn)
                txTmp.vin[i].nSequence = 0;
    }
    else if ((nHashType & 0x1f) == SIGHASH_SINGLE)
    {
        // Only lock-in the txout payee at same index as txin
        unsigned int nOut = nIn;
        if (nOut >= txTmp.vout.size())
        {
            return uint256S("0x1");
        }
        txTmp.vout.resize(nOut+1);
        for (unsigned int i = 0; i < nOut; i++)
            txTmp.vout[i].SetNull();

        // Let the others update at will
        for (unsigned int i = 0; i < txTmp.vin.size(); i++)
            if (i != nIn)
                txTmp.vin[i].nSequence = 0;
    }

    // Blank out other inputs completely, not recommended for open transactions
    if (nHashType & SIGHASH_ANYONECANPAY)
    {
        txTmp.vin[0] = txTmp.vin[nIn];
        txTmp.vin.resize(1);
    }

    // Serialize and hash
    CHashWriter ss(SER_GETHASH, 0);
    ss << txTmp << nHashType;
    return ss.GetHash();
}

bool SignatureCheckerNavcoin::VerifySignature(const std::vector<unsigned char>& vchSig, const CPubKey& pubkey, const uint256& sighash) const
{
    return pubkey.Verify(sighash, vchSig);
}

bool SignatureCheckerNavcoin::CheckSig(const vector<unsigned char>& vchSigIn, const vector<unsigned char>& vchPubKey, const CScript& scriptCode) const
{
    CPubKey pubkey(vchPubKey);
    if (!pubkey.IsValid())
        return false;

    // Hash type is one byte tacked on to the end of the signature
    vector<unsigned char> vchSig(vchSigIn);
    if (vchSig.empty())
        return false;
    int nHashType = vchSig.back();
    vchSig.pop_back();

    uint256 sighash = SignatureHash(scriptCode, txTo, nIn, nHashType);

    if (!VerifySignature(vchSig, pubkey, sighash))
        return false;

    return true;
}

bool VerifyScriptNavcoin(const CScript& scriptSig, const CScript& scriptPubKey, const CTransaction& txTo, const BaseSignatureCheckerNavcoin& checker, unsigned int nIn, unsigned int flags, int nHashType)
{
    vector<vector<unsigned char> > stack, stackCopy;
    if (!EvalScriptNavcoin(stack, scriptSig, txTo, checker, nIn, flags, nHashType))
        return false;

    stackCopy = stack;

    if (!EvalScriptNavcoin(stack, scriptPubKey, txTo, checker, nIn, flags, nHashType))
        return false;
    if (stack.empty())
        return false;

    if (CastToBool(stack.back()) == false)
        return false;

    // Additional validation for spend-to-script-hash transactions:
    if (scriptPubKey.IsPayToScriptHash())
    {
        if (!scriptSig.IsPushOnly()) // scriptSig must be literals-only
            return false;            // or validation fails

        const valtype& pubKeySerialized = stackCopy.back();
        CScript pubKey2(pubKeySerialized.begin(), pubKeySerialized.end());
        popstack(stackCopy);

        if (!EvalScriptNavcoin(stackCopy, pubKey2, txTo, checker, nIn, flags, nHashType))
            return false;
        if (stackCopy.empty())
            return false;
        return CastToBool(stackCopy.back());
    }

    return true;
}

bool VerifyScriptNavcoin(const CScript& scriptSig, const CScript& scriptPubKey, unsigned int flags, const BaseSignatureCheckerNavcoin& checker, ScriptError* serror)
{
    set_error(serror, SCRIPT_ERR_UNKNOWN_ERROR);

    if ((flags & SCRIPT_VERIFY_SIGPUSHONLY) != 0 && !scriptSig.IsPushOnly()) {
        return set_error(serror, SCRIPT_ERR_SIG_PUSHONLY);
    }

    vector<vector<unsigned char> > stack, stackCopy;
    if (!EvalScriptNavcoin(stack, scriptSig, flags, checker, serror))
        // serror is set
        return false;
    if (flags & SCRIPT_VERIFY_P2SH)
        stackCopy = stack;
    if (!EvalScriptNavcoin(stack, scriptPubKey, flags, checker, serror))
        // serror is set
        return false;
    if (stack.empty())
        return set_error(serror, SCRIPT_ERR_EVAL_FALSE);

    if (CastToBool(stack.back()) == false)
        return set_error(serror, SCRIPT_ERR_EVAL_FALSE);

    // Additional validation for spend-to-script-hash transactions:
    if ((flags & SCRIPT_VERIFY_P2SH) && scriptPubKey.IsPayToScriptHash())
    {
        // scriptSig must be literals-only or validation fails
        if (!scriptSig.IsPushOnly())
            return set_error(serror, SCRIPT_ERR_SIG_PUSHONLY);

        // stackCopy cannot be empty here, because if it was the
        // P2SH  HASH <> EQUAL  scriptPubKey would be evaluated with
        // an empty stack and the EvalScript above would return false.
        assert(!stackCopy.empty());

        const valtype& pubKeySerialized = stackCopy.back();
        CScript pubKey2(pubKeySerialized.begin(), pubKeySerialized.end());
        popstack(stackCopy);

        if (!EvalScriptNavcoin(stackCopy, pubKey2, flags, checker, serror))
            // serror is set
            return false;
        if (stackCopy.empty())
            return set_error(serror, SCRIPT_ERR_EVAL_FALSE);
        if (!CastToBool(stackCopy.back()))
            return set_error(serror, SCRIPT_ERR_EVAL_FALSE);
        else
            return set_success(serror);
    }

    return set_success(serror);
}

bool VerifyScript(const CScript& scriptSig, const CScript& scriptPubKey, unsigned int flags, const BaseSignatureCheckerNavcoin& checker, ScriptError* serror)
{
    set_error(serror, SCRIPT_ERR_UNKNOWN_ERROR);

    if ((flags & SCRIPT_VERIFY_SIGPUSHONLY) != 0 && !scriptSig.IsPushOnly()) {
        return set_error(serror, SCRIPT_ERR_SIG_PUSHONLY);
    }

    vector<vector<unsigned char> > stack, stackCopy;
    if (!EvalScriptNavcoin(stack, scriptSig, flags, checker, serror))
        // serror is set
        return false;
    if (flags & SCRIPT_VERIFY_P2SH)
        stackCopy = stack;
    if (!EvalScriptNavcoin(stack, scriptPubKey, flags, checker, serror))
        // serror is set
        return false;
    if (stack.empty())
        return set_error(serror, SCRIPT_ERR_EVAL_FALSE);

    if (CastToBool(stack.back()) == false)
        return set_error(serror, SCRIPT_ERR_EVAL_FALSE);

    // Additional validation for spend-to-script-hash transactions:
    if ((flags & SCRIPT_VERIFY_P2SH) && scriptPubKey.IsPayToScriptHash())
    {
        // scriptSig must be literals-only or validation fails
        if (!scriptSig.IsPushOnly())
            return set_error(serror, SCRIPT_ERR_SIG_PUSHONLY);

        // stackCopy cannot be empty here, because if it was the
        // P2SH  HASH <> EQUAL  scriptPubKey would be evaluated with
        // an empty stack and the EvalScript above would return false.
        assert(!stackCopy.empty());

        const valtype& pubKeySerialized = stackCopy.back();
        CScript pubKey2(pubKeySerialized.begin(), pubKeySerialized.end());
        popstack(stackCopy);

        if (!EvalScriptNavcoin(stackCopy, pubKey2, flags, checker, serror))
            // serror is set
            return false;
        if (stackCopy.empty())
            return set_error(serror, SCRIPT_ERR_EVAL_FALSE);
        if (!CastToBool(stackCopy.back()))
            return set_error(serror, SCRIPT_ERR_EVAL_FALSE);
        else
            return set_success(serror);
    }

    return set_success(serror);
}

bool SolverNavcoin(const CScript& scriptPubKey, txnouttype& typeRet, vector<vector<unsigned char> >& vSolutionsRet)
{
    // Templates
    static multimap<txnouttype, CScript> mTemplates;
    if (mTemplates.empty())
    {
        // Standard tx, sender provides pubkey, receiver adds signature
        mTemplates.insert(make_pair(TX_PUBKEY, CScript() << OP_PUBKEY << OP_CHECKSIG));

        // Bitcoin address tx, sender provides hash of pubkey, receiver provides signature and pubkey
        mTemplates.insert(make_pair(TX_PUBKEYHASH, CScript() << OP_DUP << OP_HASH160 << OP_PUBKEYHASH << OP_EQUALVERIFY << OP_CHECKSIG));

        // Sender provides N pubkeys, receivers provides M signatures
        mTemplates.insert(make_pair(TX_MULTISIG, CScript() << OP_SMALLINTEGER << OP_PUBKEYS << OP_SMALLINTEGER << OP_CHECKMULTISIG));

        // Empty, provably prunable, data-carrying output
        mTemplates.insert(make_pair(TX_NULL_DATA, CScript() << OP_RETURN << OP_SMALLDATA));
        mTemplates.insert(make_pair(TX_NULL_DATA, CScript() << OP_RETURN));
    }

    // Shortcut for pay-to-script-hash, which are more constrained than the other types:
    // it is always OP_HASH160 20 [20 byte hash] OP_EQUAL
    if (scriptPubKey.IsPayToScriptHash())
    {
        typeRet = TX_SCRIPTHASH;
        vector<unsigned char> hashBytes(scriptPubKey.begin()+2, scriptPubKey.begin()+22);
        vSolutionsRet.push_back(hashBytes);
        return true;
    }

    // Scan templates
    const CScript& script1 = scriptPubKey;
    BOOST_FOREACH(const PAIRTYPE(txnouttype, CScript)& tplate, mTemplates)
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
                while (vch1.size() >= 33 && vch1.size() <= 120)
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
                if (vch1.size() < 33 || vch1.size() > 120)
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
            else if (opcode2 == OP_SMALLDATA)
            {
                // small pushdata, <= MAX_OP_RETURN_RELAY bytes
                if (vch1.size() > MAX_OP_RETURN_RELAY)
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

bool SolverNavcoin(const CKeyStore& keystore, const CScript& scriptPubKey, uint256 hash, int nHashType,
                  CScript& scriptSigRet, txnouttype& whichTypeRet)
{
    scriptSigRet.clear();

    vector<valtype> vSolutions;
    if (!Solver(scriptPubKey, whichTypeRet, vSolutions))
        return false;

    CKeyID keyID;
    switch (whichTypeRet)
    {
    case TX_NONSTANDARD:
    case TX_NULL_DATA:
    case TX_WITNESS_V0_SCRIPTHASH:
    case TX_WITNESS_V0_KEYHASH:
        return false;
    case TX_PUBKEY:
        keyID = CPubKey(vSolutions[0]).GetID();
        return Sign1Navcoin(keyID, keystore, hash, nHashType, scriptSigRet);
    case TX_PUBKEYHASH:
        keyID = CKeyID(uint160(vSolutions[0]));
        if (!Sign1Navcoin(keyID, keystore, hash, nHashType, scriptSigRet))
            return false;
        else
        {
            CPubKey vch;
            keystore.GetPubKey(keyID, vch);
            scriptSigRet << vch;
        }
        return true;
    case TX_SCRIPTHASH:
        return keystore.GetCScript(uint160(vSolutions[0]), scriptSigRet);

    case TX_MULTISIG:
        scriptSigRet << OP_0; // workaround CHECKMULTISIG bug
        return (SignNNavcoin(vSolutions, keystore, hash, nHashType, scriptSigRet));
    }
    return false;
}


bool SignNoMutableSignature(const CKeyStore &keystore, const CTransaction& txFrom, CTransaction& txTo, unsigned int nIn, int nHashType)
{
    assert(nIn < txTo.vin.size());
    CTxIn& txin = txTo.vin[nIn];
    assert(txin.prevout.n < txFrom.vout.size());
    const CTxOut& txout = txFrom.vout[txin.prevout.n];

    return SignNoMutableSignature(keystore, txout.scriptPubKey, txTo, nIn, txout.nValue, nHashType);
}

static vector<valtype> CombineMultisig(const CScript& scriptPubKey, const BaseSignatureChecker& checker,
                               const vector<valtype>& vSolutions,
                               const vector<valtype>& sigs1, const vector<valtype>& sigs2, SigVersion sigversion)
{
    // Combine all the signatures we've got:
    set<valtype> allsigs;
    BOOST_FOREACH(const valtype& v, sigs1)
    {
        if (!v.empty())
            allsigs.insert(v);
    }
    BOOST_FOREACH(const valtype& v, sigs2)
    {
        if (!v.empty())
            allsigs.insert(v);
    }

    // Build a map of pubkey -> signature by matching sigs to pubkeys:
    assert(vSolutions.size() > 1);
    unsigned int nSigsRequired = vSolutions.front()[0];
    unsigned int nPubKeys = vSolutions.size()-2;
    map<valtype, valtype> sigs;
    BOOST_FOREACH(const valtype& sig, allsigs)
    {
        for (unsigned int i = 0; i < nPubKeys; i++)
        {
            const valtype& pubkey = vSolutions[i+1];
            if (sigs.count(pubkey))
                continue; // Already got a sig for this pubkey

            if (checker.CheckSig(sig, pubkey, scriptPubKey, sigversion))
            {
                sigs[pubkey] = sig;
                break;
            }
        }
    }
    // Now build a merged CScript:
    unsigned int nSigsHave = 0;
    std::vector<valtype> result; result.push_back(valtype()); // pop-one-too-many workaround
    for (unsigned int i = 0; i < nPubKeys && nSigsHave < nSigsRequired; i++)
    {
        if (sigs.count(vSolutions[i+1]))
        {
            result.push_back(sigs[vSolutions[i+1]]);
            ++nSigsHave;
        }
    }
    // Fill any missing with OP_0:
    for (unsigned int i = nSigsHave; i < nSigsRequired; i++)
        result.push_back(valtype());

    return result;
}

namespace
{
struct Stacks
{
    std::vector<valtype> script;
    std::vector<valtype> witness;

    Stacks() {}
    explicit Stacks(const std::vector<valtype>& scriptSigStack_) : script(scriptSigStack_), witness() {}
    explicit Stacks(const SignatureData& data) : witness(data.scriptWitness.stack) {
        EvalScript(script, data.scriptSig, SCRIPT_VERIFY_STRICTENC, BaseSignatureChecker(), SIGVERSION_BASE);
    }

    SignatureData Output() const {
        SignatureData result;
        result.scriptSig = PushAll(script);
        result.scriptWitness.stack = witness;
        return result;
    }
};
}

static Stacks CombineSignatures(const CScript& scriptPubKey, const BaseSignatureChecker& checker,
                                 const txnouttype txType, const vector<valtype>& vSolutions,
                                 Stacks sigs1, Stacks sigs2, SigVersion sigversion)
{
    switch (txType)
    {
    case TX_NONSTANDARD:
    case TX_NULL_DATA:
        // Don't know anything about this, assume bigger one is correct:
        if (sigs1.script.size() >= sigs2.script.size())
            return sigs1;
        return sigs2;
    case TX_PUBKEY:
    case TX_PUBKEYHASH:
        // Signatures are bigger than placeholders or empty scripts:
        if (sigs1.script.empty() || sigs1.script[0].empty())
            return sigs2;
        return sigs1;
    case TX_WITNESS_V0_KEYHASH:
        // Signatures are bigger than placeholders or empty scripts:
        if (sigs1.witness.empty() || sigs1.witness[0].empty())
            return sigs2;
        return sigs1;
    case TX_SCRIPTHASH:
        if (sigs1.script.empty() || sigs1.script.back().empty())
            return sigs2;
        else if (sigs2.script.empty() || sigs2.script.back().empty())
            return sigs1;
        else
        {
            // Recur to combine:
            valtype spk = sigs1.script.back();
            CScript pubKey2(spk.begin(), spk.end());

            txnouttype txType2;
            vector<vector<unsigned char> > vSolutions2;
            Solver(pubKey2, txType2, vSolutions2);
            sigs1.script.pop_back();
            sigs2.script.pop_back();
            Stacks result = CombineSignatures(pubKey2, checker, txType2, vSolutions2, sigs1, sigs2, sigversion);
            result.script.push_back(spk);
            return result;
        }
    case TX_MULTISIG:
        return Stacks(CombineMultisig(scriptPubKey, checker, vSolutions, sigs1.script, sigs2.script, sigversion));
    case TX_WITNESS_V0_SCRIPTHASH:
        if (sigs1.witness.empty() || sigs1.witness.back().empty())
            return sigs2;
        else if (sigs2.witness.empty() || sigs2.witness.back().empty())
            return sigs1;
        else
        {
            // Recur to combine:
            CScript pubKey2(sigs1.witness.back().begin(), sigs1.witness.back().end());
            txnouttype txType2;
            vector<valtype> vSolutions2;
            Solver(pubKey2, txType2, vSolutions2);
            sigs1.witness.pop_back();
            sigs1.script = sigs1.witness;
            sigs1.witness.clear();
            sigs2.witness.pop_back();
            sigs2.script = sigs2.witness;
            sigs2.witness.clear();
            Stacks result = CombineSignatures(pubKey2, checker, txType2, vSolutions2, sigs1, sigs2, SIGVERSION_WITNESS_V0);
            result.witness = result.script;
            result.script.clear();
            result.witness.push_back(valtype(pubKey2.begin(), pubKey2.end()));
            return result;
        }
    default:
        return Stacks();
    }
}

SignatureData CombineSignatures(const CScript& scriptPubKey, const BaseSignatureChecker& checker,
                          const SignatureData& scriptSig1, const SignatureData& scriptSig2)
{
    txnouttype txType;
    vector<vector<unsigned char> > vSolutions;
    Solver(scriptPubKey, txType, vSolutions);

    return CombineSignatures(scriptPubKey, checker, txType, vSolutions, Stacks(scriptSig1), Stacks(scriptSig2), SIGVERSION_BASE).Output();
}

namespace {
/** Dummy signature checker which accepts all signatures. */
class DummySignatureChecker : public BaseSignatureChecker
{
public:
    DummySignatureChecker() {}

    bool CheckSig(const std::vector<unsigned char>& scriptSig, const std::vector<unsigned char>& vchPubKey, const CScript& scriptCode, SigVersion sigversion) const
    {
        return true;
    }
};
const DummySignatureChecker dummyChecker;
}

const BaseSignatureChecker& DummySignatureCreator::Checker() const
{
    return dummyChecker;
}

bool DummySignatureCreator::CreateSig(std::vector<unsigned char>& vchSig, const CKeyID& keyid, const CScript& scriptCode, SigVersion sigversion) const
{
    // Create a dummy signature that is a valid DER-encoding
    vchSig.assign(72, '\000');
    vchSig[0] = 0x30;
    vchSig[1] = 69;
    vchSig[2] = 0x02;
    vchSig[3] = 33;
    vchSig[4] = 0x01;
    vchSig[4 + 33] = 0x02;
    vchSig[5 + 33] = 32;
    vchSig[6 + 33] = 0x01;
    vchSig[6 + 33 + 32] = SIGHASH_ALL;
    return true;
}
