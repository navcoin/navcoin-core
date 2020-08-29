// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <wallet/wallet.h>

#include <base58.h>
#include <checkpoints.h>
#include <chain.h>
#include <coincontrol.h>
#include <consensus/dao.h>
#include <consensus/consensus.h>
#include <consensus/validation.h>
#include <init.h>
#include <key.h>
#include <keystore.h>
#include <main.h>
#include <net.h>
#include <policy/policy.h>
#include <primitives/block.h>
#include <primitives/transaction.h>
#include <script/script.h>
#include <script/sign.h>
#include <timedata.h>
#include <txmempool.h>
#include <util.h>
#include <ui_interface.h>
#include <utilmoneystr.h>
#include <kernel.h>
#include <pos.h>

#include <assert.h>

#include <boost/algorithm/string/replace.hpp>
#include <boost/filesystem.hpp>
#include <boost/thread.hpp>

using namespace std;

CWallet* pwalletMain = nullptr;
/** Transaction fee set by the user */
CFeeRate payTxFee(DEFAULT_TRANSACTION_FEE);
unsigned int nTxConfirmTarget = DEFAULT_TX_CONFIRM_TARGET;
bool bSpendZeroConfChange = DEFAULT_SPEND_ZEROCONF_CHANGE;
bool fSendFreeTransactions = DEFAULT_SEND_FREE_TRANSACTIONS;

const uint32_t BIP32_HARDENED_KEY_LIMIT = 0x80000000;

int64_t StakeCombineThreshold = 1000 * COIN;
int64_t StakeSplitThreshold = 2 * StakeCombineThreshold;

/**
 * Fees smaller than this (in satoshi) are considered zero fee (for transaction creation)
 * Override with -mintxfee
 */
CFeeRate CWallet::minTxFee = CFeeRate(DEFAULT_TRANSACTION_MINFEE);
/**
 * If fee estimation does not have enough data to provide estimates, use this fee instead.
 * Has no effect if not using fee estimation
 * Override with -fallbackfee
 */
CFeeRate CWallet::fallbackFee = CFeeRate(DEFAULT_FALLBACK_FEE);


int64_t nReserveBalance = 0;
int64_t nMinimumInputValue = GetArg("-mininputvalue", 1 * COIN);

const uint256 CMerkleTx::ABANDON_HASH(uint256S("0000000000000000000000000000000000000000000000000000000000000001"));

/** @defgroup mapWallet
 *
 * @{
 */

struct CompareValueOnly
{
    bool operator()(const pair<CAmount, pair<const CWalletTx*, unsigned int> >& t1,
                    const pair<CAmount, pair<const CWalletTx*, unsigned int> >& t2) const
    {
        return t1.first < t2.first;
    }
};

std::string COutput::ToString() const
{
    return strprintf("COutput(%s, %d, %d) [%s]", tx->GetHash().ToString(), i, nDepth, FormatMoney(tx->vout[i].nValue));
}

const CWalletTx* CWallet::GetWalletTx(const uint256& hash) const
{
    LOCK(cs_wallet);
    std::map<uint256, CWalletTx>::const_iterator it = mapWallet.find(hash);
    if (it == mapWallet.end())
        return nullptr;
    return &(it->second);
}

bool CWallet::IsHDEnabled() const
{
    return !hdChain.masterKeyID.IsNull();
}

CPubKey CWallet::GenerateNewKey()
{
    AssertLockHeld(cs_wallet); // mapKeyMetadata
    bool fCompressed = CanSupportFeature(FEATURE_COMPRPUBKEY); // default to compressed public keys if we want 0.6.0 wallets

    CKey secret;

    // Create new metadata
    int64_t nCreationTime = GetTime();
    CKeyMetadata metadata(nCreationTime);

    // use HD key derivation if HD was enabled during wallet creation
    if (!hdChain.masterKeyID.IsNull()) {
        // for now we use a fixed keypath scheme of m/0'/0'/k
        CKey key;                      //master key seed (256bit)
        CExtKey masterKey;             //hd master key
        CExtKey accountKey;            //key at m/0'
        CExtKey externalChainChildKey; //key at m/0'/0'
        CExtKey childKey;              //key at m/0'/0'/<n>'

        // try to get the master key
        if (!GetKey(hdChain.masterKeyID, key))
            throw std::runtime_error("CWallet::GenerateNewKey(): Master key not found");

        masterKey.SetMaster(key.begin(), key.size());

        // derive m/0'
        // use hardened derivation (child keys >= 0x80000000 are hardened after bip32)
        masterKey.Derive(accountKey, BIP32_HARDENED_KEY_LIMIT);

        // derive m/0'/0'
        accountKey.Derive(externalChainChildKey, BIP32_HARDENED_KEY_LIMIT);

        // derive child key at next index, skip keys already known to the wallet
        do
        {
            // always derive hardened keys
            // childIndex | BIP32_HARDENED_KEY_LIMIT = derive childIndex in hardened child-index-range
            // example: 1 | BIP32_HARDENED_KEY_LIMIT == 0x80000001 == 2147483649
            externalChainChildKey.Derive(childKey, hdChain.nExternalChainCounter | BIP32_HARDENED_KEY_LIMIT);
            metadata.hdKeypath     = "m/0'/0'/"+std::to_string(hdChain.nExternalChainCounter)+"'";
            metadata.hdMasterKeyID = hdChain.masterKeyID;
            // increment childkey index
            hdChain.nExternalChainCounter++;
        } while(HaveKey(childKey.key.GetPubKey().GetID()));
        secret = childKey.key;

        // update the chain model in the database
        if (!CWalletDB(strWalletFile).WriteHDChain(hdChain))
            throw std::runtime_error("CWallet::GenerateNewKey(): Writing HD chain model failed");
    } else {
        secret.MakeNewKey(fCompressed);
    }

    // Compressed public keys were introduced in version 0.6.0
    if (fCompressed)
        SetMinVersion(FEATURE_COMPRPUBKEY);

    CPubKey pubkey = secret.GetPubKey();
    assert(secret.VerifyPubKey(pubkey));

    mapKeyMetadata[pubkey.GetID()] = metadata;
    if (!nTimeFirstKey || nCreationTime < nTimeFirstKey)
        nTimeFirstKey = nCreationTime;

    if (!AddKeyPubKey(secret, pubkey))
        throw std::runtime_error("CWallet::GenerateNewKey(): AddKey failed");
    return pubkey;
}

bool CWallet::GenerateNewSubAddress(const uint64_t& account, blsctDoublePublicKey& pk)
{
    if(hdChain.nExternalBLSCTSubAddressCounter.count(account) == 0)
        hdChain.nExternalBLSCTSubAddressCounter.insert(std::make_pair(account, 0));

    std::pair<uint64_t, uint64_t> index;

    do
    {
        index = std::make_pair(account, hdChain.nExternalBLSCTSubAddressCounter[account]);

        if (!GetBLSCTSubAddressPublicKeys(index, pk))
            return false;

        hdChain.nExternalBLSCTSubAddressCounter[account] = hdChain.nExternalBLSCTSubAddressCounter[account] + 1;

        // update the chain model in the database
        if (!CWalletDB(strWalletFile).WriteHDChain(hdChain))
            throw std::runtime_error("CWallet::GenerateNewSubAddress(): Writing HD chain model failed");

    } while (HaveBLSCTSubAddress(pk.GetID()));

    if (!AddBLSCTSubAddress(pk.GetID(), index))
        throw std::runtime_error("CWallet::GenerateNewSubAddress(): AddBLSCTSubAddress failed");

    return true;
}

blsctPublicKey CWallet::GenerateNewBlindingKey()
{
    AssertLockHeld(cs_wallet); // mapKeyMetadata

    blsctKey secret;
    blsctKey ephemeralKey;

    // Create new metadata
    int64_t nCreationTime = GetTime();
    CBLSCTBlindingKeyMetadata metadata(nCreationTime);

    // use HD key derivation if HD was enabled during wallet creation
    if (GetBLSCTBlindingMasterKey(ephemeralKey)) {
        blsctKey childKey;
        // derive child key at next index, skip keys already known to the wallet
        do
        {
            childKey = blsctKey(ephemeralKey.PrivateChild(hdChain.nExternalBLSCTChainCounter | BIP32_HARDENED_KEY_LIMIT));
            metadata.hdKeypath     = "m/130'/1'/"+std::to_string(hdChain.nExternalBLSCTChainCounter)+"'";
            // increment childkey index
            hdChain.nExternalBLSCTChainCounter++;
        } while(HaveBLSCTBlindingKey(childKey.GetG1Element()));

        secret = childKey;

        // update the chain model in the database
        if (!CWalletDB(strWalletFile).WriteHDChain(hdChain))
            throw std::runtime_error("CWallet::GenerateNewBlindingKey(): Writing HD chain model failed");
    } else {
        CPrivKey rand;
        rand.reserve(32);
        GetRandBytes(rand.data(), 32);
        secret = blsctKey(bls::PrivateKey::FromSeed(rand.data(), 32));
    }

    blsctPublicKey pubkey = secret.GetG1Element();

    mapBLSCTBlindingKeyMetadata[pubkey] = metadata;

    if (!AddBLSCTBlindingKeyPubKey(secret, pubkey))
        throw std::runtime_error("CWallet::GenerateNewBlindingKey(): AddBLSCTKey failed");

    return pubkey;
}

// ppcoin: total coins staked (non-spendable until maturity)
int64_t CWallet::GetStake() const
{
    int64_t nTotal = 0;
    LOCK2(cs_main, cs_wallet);
    for (map<uint256, CWalletTx>::const_iterator it = mapWallet.begin(); it != mapWallet.end(); ++it)
    {
        const CWalletTx* pcoin = &(*it).second;
        if (pcoin->IsCoinStake() && pcoin->GetBlocksToMaturity() > 0 && pcoin->GetDepthInMainChain() > 0)
            nTotal += CWallet::GetCredit(*pcoin,ISMINE_SPENDABLE);
    }
    return nTotal;
}

int64_t CWallet::GetNewMint() const
{
    int64_t nTotal = 0;
    LOCK2(cs_main, cs_wallet);
    for (map<uint256, CWalletTx>::const_iterator it = mapWallet.begin(); it != mapWallet.end(); ++it)
    {
        const CWalletTx* pcoin = &(*it).second;
        if (pcoin->IsCoinBase() && pcoin->GetBlocksToMaturity() > 0 && pcoin->GetDepthInMainChain() > 0)
            nTotal += CWallet::GetCredit(*pcoin,ISMINE_SPENDABLE);
    }
    return nTotal;
}

uint64_t CWallet::GetStakeWeight() const
{
    // Choose coins to use
    int64_t nBalance = GetBalance() + GetColdStakingBalance();

    if (nBalance <= nReserveBalance)
        return 0;

    set<pair<const CWalletTx*,unsigned int> > vwtxPrev;

    set<pair<const CWalletTx*,unsigned int> > setCoins;
    int64_t nValueIn = 0;

    if (!SelectCoinsForStaking(nBalance - nReserveBalance, GetTime(), setCoins, nValueIn))
        return 0;

    if (setCoins.empty())
        return 0;

    uint64_t nWeight = 0;

    int64_t nCurrentTime = GetTime();

    LOCK2(cs_main, cs_wallet);
    for(PAIRTYPE(const CWalletTx*, unsigned int) pcoin: setCoins)
    {

        if (!mapWallet.count(pcoin.first->GetHash()))
            continue;

        if (nCurrentTime - pcoin.first->nTime > Params().GetConsensus().nStakeMinAge)
            nWeight += pcoin.first->vout[pcoin.second].nValue;
    }

    return nWeight;
}

void CWallet::AvailableCoinsForStaking(vector<COutput>& vCoins, unsigned int nSpendTime) const
{
    vCoins.clear();

    nMinimumInputValue = GetArg("-mininputvalue", 1 * COIN);

    {
        LOCK2(cs_main, cs_wallet);
        for (map<uint256, CWalletTx>::const_iterator it = mapWallet.begin(); it != mapWallet.end(); ++it)
        {
            const CWalletTx* pcoin = &(*it).second;
            const uint256& wtxid = it->first;

            // Filtering by tx timestamp instead of block timestamp may give false positives but never false negatives
            if (pcoin->nTime + Params().GetConsensus().nStakeMinAge > nSpendTime)
                continue;

            if (pcoin->GetBlocksToMaturity() > 0)
                continue;

            if (pcoin->isAbandoned())
                continue;

            int nDepth = pcoin->GetDepthInMainChain();
            if (nDepth < 1)
                continue;

            for (unsigned int i = 0; i < pcoin->vout.size(); i++)
                if (!(IsSpent(wtxid,i)) && IsMine(pcoin->vout[i]) && pcoin->vout[i].nValue >= nMinimumInputValue){
                    vCoins.push_back(COutput(pcoin, i, nDepth, true,
                                           ((IsMine(pcoin->vout[i]) & (ISMINE_SPENDABLE)) != ISMINE_NO &&
                                           !pcoin->vout[i].scriptPubKey.IsColdStaking() &&
                                           !pcoin->vout[i].scriptPubKey.IsColdStakingv2()) ||
                                           ((IsMine(pcoin->vout[i]) & (ISMINE_STAKABLE)) != ISMINE_NO &&
                                           IsColdStakingEnabled(chainActive.Tip(), Params().GetConsensus()))));
                }
        }
    }

    if (GetBoolArg("-stakingsortbycoinage",false)) {
        std::sort(vCoins.begin(), vCoins.end(), sortByCoinAgeDescending());
    }
}

// Select some coins without random shuffle or best subset approximation
bool CWallet::SelectCoinsForStaking(int64_t nTargetValue, unsigned int nSpendTime, set<pair<const CWalletTx*,unsigned int> >& setCoinsRet, int64_t& nValueRet) const
{
    vector<COutput> vCoins;
    AvailableCoinsForStaking(vCoins, nSpendTime);

    setCoinsRet.clear();
    nValueRet = 0;

    for(COutput output: vCoins)
    {
        const CWalletTx *pcoin = output.tx;
        int i = output.i;

        // Stop if we've chosen enough inputs
        if (nValueRet >= nTargetValue)
            break;

        int64_t n = pcoin->vout[i].nValue;

        pair<int64_t,pair<const CWalletTx*,unsigned int> > coin = make_pair(n,make_pair(pcoin, i));

        if (n >= nTargetValue)
        {
            // If input value is greater or equal to target then simply insert
            //    it into the current subset and exit
            setCoinsRet.insert(coin.second);
            nValueRet += coin.first;
            break;
        }
        else if (n < nTargetValue + CENT)
        {
            setCoinsRet.insert(coin.second);
            nValueRet += coin.first;
        }
    }

    return true;
}

bool CWallet::CreateCoinStake(const CKeyStore& keystore, unsigned int nBits, int64_t nSearchInterval, int64_t nFees, CMutableTransaction& txNew, CKey& key, CScript& kernelScriptPubKey)
{
    CBlockIndex* pindexPrev = chainActive.Tip();
    arith_uint256 bnTargetPerCoinDay;
    bnTargetPerCoinDay.SetCompact(nBits);

    txNew.vin.clear();
    txNew.vout.clear();

    // Mark coin stake transaction
    CScript scriptEmpty;
    scriptEmpty.clear();
    txNew.vout.push_back(CTxOut(0, scriptEmpty));

    // Choose coins to use
    int64_t nBalance = GetBalance() + GetColdStakingBalance();

    if (nBalance <= nReserveBalance)
        return false;

    set<pair<const CWalletTx*,unsigned int> > vwtxPrev;

    set<pair<const CWalletTx*,unsigned int> > setCoins;
    int64_t nValueIn = 0;

    // Select coins with suitable depth
    if (!SelectCoinsForStaking(nBalance - nReserveBalance, txNew.nTime, setCoins, nValueIn))
        return false;

    if (setCoins.empty())
        return false;

    CStateViewCache view(pcoinsTip);

    int64_t nCredit = 0;
    CScript scriptPubKeyKernel;
    for(PAIRTYPE(const CWalletTx*, unsigned int) pcoin: setCoins)
    {
        static int nMaxStakeSearchInterval = 60;
        bool fKernelFound = false;
        for (unsigned int n=0; n<min(nSearchInterval,(int64_t)nMaxStakeSearchInterval) && !fKernelFound && pindexPrev == chainActive.Tip(); n++)
        {
            boost::this_thread::interruption_point();
            // Search backward in time from the given txNew timestamp
            // Search nSearchInterval seconds back up to nMaxStakeSearchInterval
            COutPoint prevoutStake = COutPoint(pcoin.first->GetHash(), pcoin.second);
            int64_t nBlockTime;
            if (CheckKernel(pindexPrev, nBits, txNew.nTime - n, prevoutStake, view, &nBlockTime))
            {
                // Found a kernel
                LogPrint("coinstake", "CreateCoinStake : kernel found\n");
                vector<std::vector<unsigned char>> vSolutions;
                txnouttype whichType;
                CScript scriptPubKeyOut;
                scriptPubKeyKernel = pcoin.first->vout[pcoin.second].scriptPubKey;
                if (!Solver(scriptPubKeyKernel, whichType, vSolutions))
                {
                    LogPrint("coinstake", "CreateCoinStake : failed to parse kernel\n");
                    break;
                }
                LogPrint("coinstake", "CreateCoinStake : parsed kernel type=%d\n", whichType);
                if (whichType != TX_PUBKEY && whichType != TX_PUBKEYHASH && whichType != TX_COLDSTAKING && whichType != TX_COLDSTAKING_V2)
                {
                    LogPrint("coinstake", "CreateCoinStake : no support for kernel type=%d\n", whichType);
                    break;  // only support pay to public key and pay to address
                }
                if (whichType == TX_COLDSTAKING || whichType == TX_COLDSTAKING_V2) // cold staking
                {
                    // try to find staking key
                    if (!keystore.GetKey(uint160(vSolutions[0]), key))
                    {
                        LogPrint("coinstake", "CreateCoinStake : failed to get key for kernel type=%d\n", whichType);
                        break;  // unable to find corresponding public key
                    } else {
                        // we keep the same script
                        scriptPubKeyOut = scriptPubKeyKernel;
                    }
                }
                if (whichType == TX_PUBKEYHASH) // pay to address type
                {
                    // convert to pay to public key type
                    if (!keystore.GetKey(uint160(vSolutions[0]), key))
                    {
                        LogPrint("coinstake", "CreateCoinStake : failed to get key for kernel type=%d\n", whichType);
                        break;  // unable to find corresponding public key
                    }
                    scriptPubKeyOut << ToByteVector(key.GetPubKey()) << OP_CHECKSIG;
                }
                if (whichType == TX_PUBKEY)
                {
                    std::vector<unsigned char>& vchPubKey = vSolutions[0];
                    if (!keystore.GetKey(Hash160(vchPubKey), key))
                    {
                        LogPrint("coinstake", "CreateCoinStake : failed to get key for kernel type=%d\n", whichType);
                        break;  // unable to find corresponding public key
                    }

                    if (key.GetPubKey() != vchPubKey)
                    {
                        LogPrint("coinstake", "CreateCoinStake : invalid key for kernel type=%d\n", whichType);
                        break; // keys mismatch
                    }

                    scriptPubKeyOut = scriptPubKeyKernel;
                }

                txNew.nTime -= n;
                txNew.vin.push_back(CTxIn(pcoin.first->GetHash(), pcoin.second));
                nCredit += pcoin.first->vout[pcoin.second].nValue;
                vwtxPrev.insert(make_pair(pcoin.first,pcoin.second));
                txNew.vout.push_back(CTxOut(0, scriptPubKeyOut));
                kernelScriptPubKey = scriptPubKeyKernel;

                LogPrint("coinstake", "CreateCoinStake : added kernel type=%d\n", whichType);
                fKernelFound = true;
                break;
            }
        }

        if (fKernelFound)
            break; // if kernel is found stop searching
    }

    if (nCredit == 0 || nCredit > nBalance - nReserveBalance)
        return false;

    CTransaction txPrev;
    uint256 hashBlock = uint256();

    for(PAIRTYPE(const CWalletTx*, unsigned int) pcoin: setCoins)
    {
        // Attempt to add more inputs
        // Only add coins of the same key/address as kernel
        if (txNew.vout.size() == 2 && ((pcoin.first->vout[pcoin.second].scriptPubKey == scriptPubKeyKernel || pcoin.first->vout[pcoin.second].scriptPubKey == txNew.vout[1].scriptPubKey))
            && pcoin.first->GetHash() != txNew.vin[0].prevout.hash)
        {
            int64_t nTimeWeight = GetWeight((int64_t)pcoin.first->nTime, (int64_t)txNew.nTime);

            // Stop adding more inputs if already too many inputs
            if (txNew.vin.size() >= 100)
                break;
            // Stop adding inputs if reached reserve limit
            if (nCredit + pcoin.first->vout[pcoin.second].nValue > nBalance - nReserveBalance)
                break;
            // Do not add additional significant input
            if (pcoin.first->vout[pcoin.second].nValue >= Params().GetConsensus().nStakeCombineThreshold)
                continue;
            // Do not add input that is still too young
            if (nTimeWeight < Params().GetConsensus().nStakeMinAge)
                continue;

            if (!GetTransaction(pcoin.first->GetHash(), txPrev, Params().GetConsensus(), hashBlock, view, true))
                continue;

            txNew.vin.push_back(CTxIn(pcoin.first->GetHash(), pcoin.second));
            nCredit += pcoin.first->vout[pcoin.second].nValue;
            vwtxPrev.insert(make_pair(pcoin.first,pcoin.second));
        }
    }

    // Calculate coin age reward
    int64_t nReward;
    {
        uint64_t nCoinAge;
        CTransaction ptxNew = CTransaction(txNew);
        if (!TransactionGetCoinAge(ptxNew, nCoinAge, view))
            return error("CreateCoinStake : failed to calculate coin age");

        nReward = GetProofOfStakeReward(pindexPrev->nHeight + 1, nCoinAge, nFees, chainActive.Tip(), view);
        if (nReward <= 0)
            return false;

        nCredit += nReward;
    }

    int64_t blockValue = nCredit;
    std::map<CNavCoinAddress, double> splitMap;
    double nAccumulatedFee = 0.0;

    CNavCoinAddress poolFeeAddress(GetArg("-pooladdress", ""));
    double nPoolFee = GetArg("-poolfee", 0) / 100.0;

    if (nPoolFee > 0 && poolFeeAddress.IsValid())
    {
        CAmount nRewardAsFee = nReward * nPoolFee;
        blockValue -= nRewardAsFee;
        txNew.vout.push_back(CTxOut(nRewardAsFee, GetScriptForDestination(poolFeeAddress.Get())));
    }
    else if (GetArg("-stakingaddress", "") != "")
    {
        CNavCoinAddress address;
        UniValue stakingAddress;
        UniValue addressMap(UniValue::VOBJ);
        std::map<std::string, UniValue> splitObject;

        if (stakingAddress.read(GetArg("-stakingaddress", "")))
        {
            try {
                if (stakingAddress.isObject())
                    addressMap = stakingAddress.get_obj();
                else
                    return error("%s: Failed to read JSON from -stakingaddress argument", __func__);

                std::string lookForKey = "all";

                if (find_value(addressMap, CNavCoinAddress(key.GetPubKey().GetID()).ToString()).isStr())
                    lookForKey = CNavCoinAddress(key.GetPubKey().GetID()).ToString();

                if (find_value(addressMap, CNavCoinAddress(key.GetPubKey().GetID()).ToString()).isObject())
                {
                    find_value(addressMap, CNavCoinAddress(key.GetPubKey().GetID()).ToString()).getObjMap(splitObject);
                    if (splitObject.size() > 0)
                        lookForKey = CNavCoinAddress(key.GetPubKey().GetID()).ToString();
                }

                if(find_value(addressMap, lookForKey).isStr())
                {
                    address = CNavCoinAddress(find_value(addressMap, lookForKey).get_str());

                    if (address.IsValid())
                    {
                        splitMap[address] = 100.0;
                    }
                    else
                    {
                        return error("%s: -stakingaddress includes a wrong address %s", __func__, find_value(addressMap, lookForKey).get_str());
                    }
                }
                else if(find_value(addressMap, lookForKey).isObject())
                {
                    find_value(addressMap, lookForKey).getObjMap(splitObject);

                    for ( const auto &pair : splitObject ) {
                        address = CNavCoinAddress(pair.first);
                        if (!address.IsValid() || pair.second.get_real() <= 0)
                            continue;
                        if (nAccumulatedFee+pair.second.get_real() > 100.0)
                            return error("%s: -stakingaddress tries to contribute more than 100%");

                        nAccumulatedFee += pair.second.get_real();

                        splitMap[address] = pair.second.get_real();
                    }
                }

            } catch (const UniValue& objError) {
                return error("%s: Failed to read JSON from -stakingaddress argument", __func__);
            } catch (const std::exception& e) {
                return error("%s: Failed to read JSON from -stakingaddress argument", __func__);
            }
        }
        else
        {
            address = CNavCoinAddress(GetArg("-stakingaddress", ""));
            if (address.IsValid()) {
                splitMap[address] = 100;
            }
        }

        for (const auto& entry: splitMap)
        {
            CAmount thisOut = nReward * entry.second/100.0;
            blockValue -= thisOut;
            txNew.vout.push_back(CTxOut(thisOut, GetScriptForDestination(entry.first.Get())));
        }
    }

    bool fSplit = false;

    if (blockValue >= Params().GetConsensus().nStakeSplitThreshold)
    {
        fSplit = true;
        txNew.vout.insert(txNew.vout.begin()+2, CTxOut(0, txNew.vout[1].scriptPubKey)); //split stake
    }

    // Set output amount
    if (fSplit) // 2 stake outputs, stake was split
    {
        txNew.vout[1].nValue = (blockValue / 2 / CENT) * CENT;
        txNew.vout[2].nValue = blockValue - txNew.vout[1].nValue;
    }
    else if(!fSplit) // only 1 stake output, was not split
        txNew.vout[1].nValue = blockValue;

    // Adds Community Fund output if enabled
    if(IsCommunityFundAccumulationEnabled(pindexPrev, Params().GetConsensus(), false))
    {
        if(IsCommunityFundAccumulationSpreadEnabled(pindexPrev, Params().GetConsensus()))
        {
            if((pindexPrev->nHeight + 1) % GetConsensusParameter(Consensus::CONSENSUS_PARAM_FUND_SPREAD_ACCUMULATION, view) == 0)
            {
                int fundIndex = txNew.vout.size() + 1;
                txNew.vout.resize(fundIndex);
                SetScriptForCommunityFundContribution(txNew.vout[fundIndex-1].scriptPubKey);

                if(IsCommunityFundAmountV2Enabled(pindexPrev, Params().GetConsensus())) {
                    txNew.vout[fundIndex-1].nValue = GetFundContributionPerBlock(view) * GetConsensusParameter(Consensus::CONSENSUS_PARAM_FUND_SPREAD_ACCUMULATION, view);
                } else {
                    txNew.vout[fundIndex-1].nValue = Params().GetConsensus().nCommunityFundAmount * GetConsensusParameter(Consensus::CONSENSUS_PARAM_FUND_SPREAD_ACCUMULATION, view);
                }
            }
        }
        else
        {
            int fundIndex = txNew.vout.size() + 1;
            txNew.vout.resize(fundIndex);
            SetScriptForCommunityFundContribution(txNew.vout[fundIndex-1].scriptPubKey);

             if(IsCommunityFundAmountV2Enabled(pindexPrev, Params().GetConsensus())) {
                txNew.vout[fundIndex-1].nValue = GetFundContributionPerBlock(view);
             } else {
                txNew.vout[fundIndex-1].nValue = Params().GetConsensus().nCommunityFundAmount;
             }
        }
    }

    txNew.nVersion = IsCommunityFundEnabled(chainActive.Tip(),Params().GetConsensus()) ? CTransaction::TXDZEEL_VERSION_V2 : CTransaction::TXDZEEL_VERSION;

    // Sign
    int nIn = 0;
    CTransaction txNewConst(txNew);
    for(const PAIRTYPE(const CWalletTx*,unsigned int)& coin: vwtxPrev)
    {
        bool signSuccess;
        const CScript& scriptPubKey = coin.first->vout[coin.second].scriptPubKey;
        SignatureData sigdata;
        signSuccess = ProduceSignature(TransactionSignatureCreator(this, &txNewConst, nIn, coin.first->vout[coin.second].nValue, SIGHASH_ALL), scriptPubKey, sigdata, true);

        if (!signSuccess)
        {
            return error("Signing transaction failed");
        } else {
            UpdateTransaction(txNew, nIn, sigdata);
        }

        nIn++;
    }

    // Limit size
    unsigned int nBytes = ::GetSerializeSize(txNew, SER_NETWORK, PROTOCOL_VERSION);
    if (nBytes >= MAX_BLOCK_SIZE_GEN/5)
        return error("CreateCoinStake : exceeded coinstake size limit");

    LogPrint("coinstake", "Created coin stake\n");

    // Successfully generated coinstake
    return true;
}

// optional setting to unlock wallet for staking only
// serves to disable the trivial sendmoney when OS account compromised
// provides no real security
bool fWalletUnlockStakingOnly = false;

bool CWallet::AddKeyPubKey(const CKey& secret, const CPubKey &pubkey)
{
    AssertLockHeld(cs_wallet); // mapKeyMetadata
    if (!CCryptoKeyStore::AddKeyPubKey(secret, pubkey))
        return false;

    // check if we need to remove from watch-only
    CScript script;
    script = GetScriptForDestination(pubkey.GetID());
    if (HaveWatchOnly(script))
        RemoveWatchOnly(script);
    script = GetScriptForRawPubKey(pubkey);
    if (HaveWatchOnly(script))
        RemoveWatchOnly(script);

    if (!fFileBacked)
        return true;
    if (!IsCrypted()) {
        return CWalletDB(strWalletFile).WriteKey(pubkey,
                                                 secret.GetPrivKey(),
                                                 mapKeyMetadata[pubkey.GetID()]);
    }
    return true;
}

bool CWallet::AddBLSCTBlindingKeyPubKey(const blsctKey& key, const blsctPublicKey &pubkey)
{
    AssertLockHeld(cs_wallet); // mapKeyMetadata
    if (!CBasicKeyStore::AddBLSCTBlindingKeyPubKey(key, pubkey))
        return false;

    if (!fFileBacked)
        return true;

    return CWalletDB(strWalletFile).WriteBLSCTBlindingKey(pubkey,
                                             key,
                                             mapBLSCTBlindingKeyMetadata[pubkey]);
}

bool CWallet::AddBLSCTSubAddress(const CKeyID &hashId, const std::pair<uint64_t, uint64_t>& index)
{
    AssertLockHeld(cs_wallet); // mapKeyMetadata
    if (!CBasicKeyStore::AddBLSCTSubAddress(hashId, index))
        return false;

    if (!fFileBacked)
        return true;

    return CWalletDB(strWalletFile).WriteBLSCTSubAddress(hashId,
                                             index);
}

bool CWallet::AddCryptedKey(const CPubKey &vchPubKey,
                            const vector<unsigned char> &vchCryptedSecret)
{
    if (!CCryptoKeyStore::AddCryptedKey(vchPubKey, vchCryptedSecret))
        return false;
    if (!fFileBacked)
        return true;
    {
        LOCK(cs_wallet);
        if (pwalletdbEncryption)
            return pwalletdbEncryption->WriteCryptedKey(vchPubKey,
                                                        vchCryptedSecret,
                                                        mapKeyMetadata[vchPubKey.GetID()]);
        else
            return CWalletDB(strWalletFile).WriteCryptedKey(vchPubKey,
                                                            vchCryptedSecret,
                                                            mapKeyMetadata[vchPubKey.GetID()]);
    }
    return false;
}

bool CWallet::LoadKeyMetadata(const CPubKey &pubkey, const CKeyMetadata &meta)
{
    AssertLockHeld(cs_wallet); // mapKeyMetadata
    if (meta.nCreateTime && (!nTimeFirstKey || meta.nCreateTime < nTimeFirstKey))
        nTimeFirstKey = meta.nCreateTime;

    mapKeyMetadata[pubkey.GetID()] = meta;
    return true;
}

bool CWallet::LoadBLSCTBlindingKeyMetadata(const blsctPublicKey &pubkey, const CBLSCTBlindingKeyMetadata &meta)
{
    AssertLockHeld(cs_wallet); // mapKeyMetadata

    mapBLSCTBlindingKeyMetadata[pubkey] = meta;
    return true;
}

bool CWallet::LoadCryptedKey(const CPubKey &vchPubKey, const std::vector<unsigned char> &vchCryptedSecret)
{
    return CCryptoKeyStore::AddCryptedKey(vchPubKey, vchCryptedSecret);
}

bool CWallet::AddCScript(const CScript& redeemScript)
{
    if (!CCryptoKeyStore::AddCScript(redeemScript))
        return false;
    if (!fFileBacked)
        return true;
    return CWalletDB(strWalletFile).WriteCScript(Hash160(redeemScript), redeemScript);
}

bool CWallet::LoadCScript(const CScript& redeemScript)
{
    /* A sanity check was added in pull #3843 to avoid adding redeemScripts
     * that never can be redeemed. However, old wallets may still contain
     * these. Do not add them to the wallet and warn. */
    if (redeemScript.size() > MAX_SCRIPT_ELEMENT_SIZE)
    {
        std::string strAddr = CNavCoinAddress(CScriptID(redeemScript)).ToString();
        LogPrintf("%s: Warning: This wallet contains a redeemScript of size %i which exceeds maximum size %i thus can never be redeemed. Do not use address %s.\n",
            __func__, redeemScript.size(), MAX_SCRIPT_ELEMENT_SIZE, strAddr);
        return true;
    }

    return CCryptoKeyStore::AddCScript(redeemScript);
}

bool CWallet::AddWatchOnly(const CScript &dest)
{
    if (!CCryptoKeyStore::AddWatchOnly(dest))
        return false;
    nTimeFirstKey = 1; // No birthday information for watch-only keys.
    NotifyWatchonlyChanged(true);
    if (!fFileBacked)
        return true;
    return CWalletDB(strWalletFile).WriteWatchOnly(dest);
}

bool CWallet::RemoveWatchOnly(const CScript &dest)
{
    AssertLockHeld(cs_wallet);
    if (!CCryptoKeyStore::RemoveWatchOnly(dest))
        return false;
    if (!HaveWatchOnly())
        NotifyWatchonlyChanged(false);
    if (fFileBacked)
        if (!CWalletDB(strWalletFile).EraseWatchOnly(dest))
            return false;

    return true;
}

bool CWallet::LoadWatchOnly(const CScript &dest)
{
    return CCryptoKeyStore::AddWatchOnly(dest);
}

bool CWallet::Unlock(const SecureString& strWalletPassphrase)
{
    CCrypter crypter;
    CKeyingMaterial vMasterKey;

    {
        LOCK(cs_wallet);
        for(const MasterKeyMap::value_type& pMasterKey: mapMasterKeys)
        {
            if(!crypter.SetKeyFromPassphrase(strWalletPassphrase, pMasterKey.second.vchSalt, pMasterKey.second.nDeriveIterations, pMasterKey.second.nDerivationMethod))
                return false;
            if (!crypter.Decrypt(pMasterKey.second.vchCryptedKey, vMasterKey))
                continue; // try another master key
            if (CCryptoKeyStore::Unlock(vMasterKey))
                return true;
        }
    }
    return false;
}

bool CWallet::ChangeWalletPassphrase(const SecureString& strOldWalletPassphrase, const SecureString& strNewWalletPassphrase)
{
    bool fWasLocked = IsLocked();

    {
        LOCK(cs_wallet);
        Lock();

        CCrypter crypter;
        CKeyingMaterial vMasterKey;
        for(MasterKeyMap::value_type& pMasterKey: mapMasterKeys)
        {
            if(!crypter.SetKeyFromPassphrase(strOldWalletPassphrase, pMasterKey.second.vchSalt, pMasterKey.second.nDeriveIterations, pMasterKey.second.nDerivationMethod))
                return false;
            if (!crypter.Decrypt(pMasterKey.second.vchCryptedKey, vMasterKey))
                return false;
            if (CCryptoKeyStore::Unlock(vMasterKey))
            {
                int64_t nStartTime = GetTimeMillis();
                crypter.SetKeyFromPassphrase(strNewWalletPassphrase, pMasterKey.second.vchSalt, pMasterKey.second.nDeriveIterations, pMasterKey.second.nDerivationMethod);
                pMasterKey.second.nDeriveIterations = pMasterKey.second.nDeriveIterations * (100 / ((double)(GetTimeMillis() - nStartTime)));

                nStartTime = GetTimeMillis();
                crypter.SetKeyFromPassphrase(strNewWalletPassphrase, pMasterKey.second.vchSalt, pMasterKey.second.nDeriveIterations, pMasterKey.second.nDerivationMethod);
                pMasterKey.second.nDeriveIterations = (pMasterKey.second.nDeriveIterations + pMasterKey.second.nDeriveIterations * 100 / ((double)(GetTimeMillis() - nStartTime))) / 2;

                if (pMasterKey.second.nDeriveIterations < 25000)
                    pMasterKey.second.nDeriveIterations = 25000;

                LogPrintf("Wallet passphrase changed to an nDeriveIterations of %i\n", pMasterKey.second.nDeriveIterations);

                if (!crypter.SetKeyFromPassphrase(strNewWalletPassphrase, pMasterKey.second.vchSalt, pMasterKey.second.nDeriveIterations, pMasterKey.second.nDerivationMethod))
                    return false;
                if (!crypter.Encrypt(vMasterKey, pMasterKey.second.vchCryptedKey))
                    return false;
                CWalletDB(strWalletFile).WriteMasterKey(pMasterKey.first, pMasterKey.second);
                if (fWasLocked)
                    Lock();
                return true;
            }
        }
    }

    return false;
}

void CWallet::SetBestChain(const CBlockLocator& loc)
{
    CWalletDB walletdb(strWalletFile);
    walletdb.WriteBestBlock(loc);
}

bool CWallet::SetMinVersion(enum WalletFeature nVersion, CWalletDB* pwalletdbIn, bool fExplicit)
{
    LOCK(cs_wallet); // nWalletVersion
    if (nWalletVersion >= nVersion)
        return true;

    // when doing an explicit upgrade, if we pass the max version permitted, upgrade all the way
    if (fExplicit && nVersion > nWalletMaxVersion)
            nVersion = FEATURE_LATEST;

    nWalletVersion = nVersion;

    if (nVersion > nWalletMaxVersion)
        nWalletMaxVersion = nVersion;

    if (fFileBacked)
    {
        CWalletDB* pwalletdb = pwalletdbIn ? pwalletdbIn : new CWalletDB(strWalletFile);
        if (nWalletVersion > 40000)
            pwalletdb->WriteMinVersion(nWalletVersion);
        if (!pwalletdbIn)
            delete pwalletdb;
    }

    return true;
}

bool CWallet::SetMaxVersion(int nVersion)
{
    LOCK(cs_wallet); // nWalletVersion, nWalletMaxVersion
    // cannot downgrade below current version
    if (nWalletVersion > nVersion)
        return false;

    nWalletMaxVersion = nVersion;

    return true;
}

set<uint256> CWallet::GetConflicts(const uint256& txid) const
{
    set<uint256> result;
    AssertLockHeld(cs_wallet);

    std::map<uint256, CWalletTx>::const_iterator it = mapWallet.find(txid);
    if (it == mapWallet.end())
        return result;
    const CWalletTx& wtx = it->second;

    std::pair<TxSpends::const_iterator, TxSpends::const_iterator> range;

    for(const CTxIn& txin: wtx.vin)
    {
        if (mapTxSpends.count(txin.prevout) <= 1)
            continue;  // No conflict if zero or one spends
        range = mapTxSpends.equal_range(txin.prevout);
        for (TxSpends::const_iterator it = range.first; it != range.second; ++it)
            result.insert(it->second);
    }
    return result;
}

void CWallet::Flush(bool shutdown)
{
    bitdb.Flush(shutdown);
}

bool CWallet::Verify()
{
    LogPrintf("Using BerkeleyDB version %s\n", DbEnv::version(0, 0, 0));
    std::string walletFile = GetArg("-wallet", DEFAULT_WALLET_DAT);

    LogPrintf("Using wallet %s\n", walletFile);
    uiInterface.InitMessage(_("Verifying wallet..."));

    // Wallet file must be a plain filename without a directory
    if (walletFile != boost::filesystem::basename(walletFile) + boost::filesystem::extension(walletFile))
        return InitError(strprintf(_("Wallet %s resides outside data directory %s"), walletFile, GetDataDir().string()));

    if (!bitdb.Open(GetDataDir()))
    {
        // try moving the database env out of the way
        boost::filesystem::path pathDatabase = GetDataDir() / "database";
        boost::filesystem::path pathDatabaseBak = GetDataDir() / strprintf("database.%d.bak", GetTime());
        try {
            boost::filesystem::rename(pathDatabase, pathDatabaseBak);
            LogPrintf("Moved old %s to %s. Retrying.\n", pathDatabase.string(), pathDatabaseBak.string());
        } catch (const boost::filesystem::filesystem_error&) {
            // failure is ok (well, not really, but it's not worse than what we started with)
        }

        // try again
        if (!bitdb.Open(GetDataDir())) {
            // if it still fails, it probably means we can't even create the database env
            return InitError(strprintf(_("Error initializing wallet database environment %s!"), GetDataDir()));
        }
    }

    if (GetBoolArg("-salvagewallet", false))
    {
        // Recover readable keypairs:
        if (!CWalletDB::Recover(bitdb, walletFile, true))
            return false;
    }

    if (boost::filesystem::exists(GetDataDir() / walletFile))
    {
        CDBEnv::VerifyResult r = bitdb.Verify(walletFile, CWalletDB::Recover);
        if (r == CDBEnv::RECOVER_OK)
        {
            InitWarning(strprintf(_("Warning: Wallet file corrupt, data salvaged!"
                                         " Original %s saved as %s in %s; if"
                                         " your balance or transactions are incorrect you should"
                                         " restore from a backup."),
                walletFile, "wallet.{timestamp}.bak", GetDataDir()));
        }
        if (r == CDBEnv::RECOVER_FAIL)
            return InitError(strprintf(_("%s corrupt, salvage failed"), walletFile));
    }

    return true;
}

void CWallet::SyncMetaData(pair<TxSpends::iterator, TxSpends::iterator> range)
{
    // We want all the wallet transactions in range to have the same metadata as
    // the oldest (smallest nOrderPos).
    // So: find smallest nOrderPos:

    int nMinOrderPos = std::numeric_limits<int>::max();
    const CWalletTx* copyFrom = nullptr;
    for (TxSpends::iterator it = range.first; it != range.second; ++it)
    {
        const uint256& hash = it->second;
        int n = mapWallet[hash].nOrderPos;
        if (n < nMinOrderPos)
        {
            nMinOrderPos = n;
            copyFrom = &mapWallet[hash];
        }
    }
    // Now copy data from copyFrom to rest:
    for (TxSpends::iterator it = range.first; it != range.second; ++it)
    {
        const uint256& hash = it->second;
        CWalletTx* copyTo = &mapWallet[hash];
        if (copyFrom == copyTo) continue;
        if (!copyFrom->IsEquivalentTo(*copyTo)) continue;
        copyTo->mapValue = copyFrom->mapValue;
        copyTo->vOrderForm = copyFrom->vOrderForm;
        copyTo->vAmounts = copyFrom->vAmounts;
        copyTo->vMemos = copyFrom->vMemos;
        copyTo->vGammas = copyFrom->vGammas;
        // fTimeReceivedIsTxTime not copied on purpose
        // nTimeReceived not copied on purpose
        copyTo->nTimeSmart = copyFrom->nTimeSmart;
        copyTo->fFromMe = copyFrom->fFromMe;
        copyTo->strFromAccount = copyFrom->strFromAccount;
        // nOrderPos not copied on purpose
        // cached members not copied on purpose
    }
}

/**
 * Outpoint is spent if any non-conflicted transaction
 * spends it:
 */
bool CWallet::IsSpent(const uint256& hash, unsigned int n) const
{
    const COutPoint outpoint(hash, n);
    pair<TxSpends::const_iterator, TxSpends::const_iterator> range;
    range = mapTxSpends.equal_range(outpoint);

    for (TxSpends::const_iterator it = range.first; it != range.second; ++it)
    {
        const uint256& wtxid = it->second;
        std::map<uint256, CWalletTx>::const_iterator mit = mapWallet.find(wtxid);
        if (mit != mapWallet.end()) {
            int depth = mit->second.GetDepthInMainChain();
            if (depth > 0  || (depth == 0 && !mit->second.isAbandoned() && !mit->second.IsCoinStake()))
                return true; // Spent
        }
    }
    return false;
}

void CWallet::AddToSpends(const COutPoint& outpoint, const uint256& wtxid)
{
    mapTxSpends.insert(make_pair(outpoint, wtxid));

    pair<TxSpends::iterator, TxSpends::iterator> range;
    range = mapTxSpends.equal_range(outpoint);
    SyncMetaData(range);
}


void CWallet::AddToSpends(const uint256& wtxid)
{
    assert(mapWallet.count(wtxid));
    CWalletTx& thisTx = mapWallet[wtxid];
    if (thisTx.IsCoinBase()) // Coinbases don't spend anything!
        return;

    for(const CTxIn& txin: thisTx.vin)
        AddToSpends(txin.prevout, wtxid);
}

bool CWallet::EncryptWallet(const SecureString& strWalletPassphrase)
{
    if (IsCrypted())
        return false;

    CKeyingMaterial vMasterKey;

    vMasterKey.resize(WALLET_CRYPTO_KEY_SIZE);
    GetStrongRandBytes(&vMasterKey[0], WALLET_CRYPTO_KEY_SIZE);

    CMasterKey kMasterKey;

    kMasterKey.vchSalt.resize(WALLET_CRYPTO_SALT_SIZE);
    GetStrongRandBytes(&kMasterKey.vchSalt[0], WALLET_CRYPTO_SALT_SIZE);

    CCrypter crypter;
    int64_t nStartTime = GetTimeMillis();
    crypter.SetKeyFromPassphrase(strWalletPassphrase, kMasterKey.vchSalt, 25000, kMasterKey.nDerivationMethod);
    kMasterKey.nDeriveIterations = 2500000 / ((double)(GetTimeMillis() - nStartTime));

    nStartTime = GetTimeMillis();
    crypter.SetKeyFromPassphrase(strWalletPassphrase, kMasterKey.vchSalt, kMasterKey.nDeriveIterations, kMasterKey.nDerivationMethod);
    kMasterKey.nDeriveIterations = (kMasterKey.nDeriveIterations + kMasterKey.nDeriveIterations * 100 / ((double)(GetTimeMillis() - nStartTime))) / 2;

    if (kMasterKey.nDeriveIterations < 25000)
        kMasterKey.nDeriveIterations = 25000;

    LogPrintf("Encrypting Wallet with an nDeriveIterations of %i\n", kMasterKey.nDeriveIterations);

    if (!crypter.SetKeyFromPassphrase(strWalletPassphrase, kMasterKey.vchSalt, kMasterKey.nDeriveIterations, kMasterKey.nDerivationMethod))
        return false;
    if (!crypter.Encrypt(vMasterKey, kMasterKey.vchCryptedKey))
        return false;

    {
        LOCK(cs_wallet);
        mapMasterKeys[++nMasterKeyMaxID] = kMasterKey;

        if (fFileBacked)
        {
            assert(!pwalletdbEncryption);
            pwalletdbEncryption = new CWalletDB(strWalletFile);
            if (!pwalletdbEncryption->TxnBegin()) {
                delete pwalletdbEncryption;
                pwalletdbEncryption = nullptr;
                return false;
            }
            pwalletdbEncryption->WriteMasterKey(nMasterKeyMaxID, kMasterKey);
        }

        if (!EncryptKeys(vMasterKey) || !EncryptBLSCTParameters(vMasterKey))
        {
            if (fFileBacked) {
                pwalletdbEncryption->TxnAbort();
                delete pwalletdbEncryption;
            }
            // We now probably have half of our keys encrypted in memory, and half not...
            // die and let the user reload the unencrypted wallet.
            assert(false);
        }

        pwalletdbEncryption->WriteBLSCTKey(this);

        LogPrintf("Encrypted BLSCT parameters written\n");

        // Encryption was introduced in version 0.4.0
        SetMinVersion(FEATURE_WALLETCRYPT, pwalletdbEncryption, true);

        if (fFileBacked)
        {
            if (!pwalletdbEncryption->TxnCommit()) {
                delete pwalletdbEncryption;
                // We now have keys encrypted in memory, but not on disk...
                // die to avoid confusion and let the user reload the unencrypted wallet.
                assert(false);
            }

            delete pwalletdbEncryption;
            pwalletdbEncryption = nullptr;
        }

        Lock();

        // Need to completely rewrite the wallet file; if we don't, bdb might keep
        // bits of the unencrypted private key in slack space in the database file.
        CDB::Rewrite(strWalletFile);

    }
    NotifyStatusChanged(this);

    return true;
}

int64_t CWallet::IncOrderPosNext(CWalletDB *pwalletdb)
{
    AssertLockHeld(cs_wallet); // nOrderPosNext
    int64_t nRet = nOrderPosNext++;
    if (pwalletdb) {
        pwalletdb->WriteOrderPosNext(nOrderPosNext);
    } else {
        CWalletDB(strWalletFile).WriteOrderPosNext(nOrderPosNext);
    }
    return nRet;
}

bool CWallet::AccountMove(std::string strFrom, std::string strTo, CAmount nAmount, std::string strComment)
{
    CWalletDB walletdb(strWalletFile);
    if (!walletdb.TxnBegin())
        return false;

    int64_t nNow = GetAdjustedTime();

    // Debit
    CAccountingEntry debit;
    debit.nOrderPos = IncOrderPosNext(&walletdb);
    debit.strAccount = strFrom;
    debit.nCreditDebit = -nAmount;
    debit.nTime = nNow;
    debit.strOtherAccount = strTo;
    debit.strComment = strComment;
    AddAccountingEntry(debit, walletdb);

    // Credit
    CAccountingEntry credit;
    credit.nOrderPos = IncOrderPosNext(&walletdb);
    credit.strAccount = strTo;
    credit.nCreditDebit = nAmount;
    credit.nTime = nNow;
    credit.strOtherAccount = strFrom;
    credit.strComment = strComment;
    AddAccountingEntry(credit, walletdb);

    if (!walletdb.TxnCommit())
        return false;

    return true;
}

bool CWallet::GetAccountPubkey(CPubKey &pubKey, std::string strAccount, bool bForceNew)
{
    CWalletDB walletdb(strWalletFile);

    CAccount account;
    walletdb.ReadAccount(strAccount, account);

    if (!bForceNew) {
        if (!account.vchPubKey.IsValid())
            bForceNew = true;
        else {
            // Check if the current key has been used
            CScript scriptPubKey = GetScriptForDestination(account.vchPubKey.GetID());
            for (map<uint256, CWalletTx>::iterator it = mapWallet.begin();
                 it != mapWallet.end() && account.vchPubKey.IsValid();
                 ++it)
                for(const CTxOut& txout: (*it).second.vout)
                    if (txout.scriptPubKey == scriptPubKey) {
                        bForceNew = true;
                        break;
                    }
        }
    }

    // Generate a new key
    if (bForceNew) {
        if (!GetKeyFromPool(account.vchPubKey))
            return false;

        SetAddressBook(account.vchPubKey.GetID(), strAccount, "receive");
        walletdb.WriteAccount(strAccount, account);
    }

    pubKey = account.vchPubKey;

    return true;
}

void CWallet::MarkDirty()
{
    {
        LOCK(cs_wallet);
        for(PAIRTYPE(const uint256, CWalletTx)& item: mapWallet)
            item.second.MarkDirty();
    }
}

bool CWallet::AddToWallet(const CWalletTx& wtxIn, bool fFromLoadWallet, CWalletDB* pwalletdb, const std::vector<RangeproofEncodedData>* blsctData)
{
    uint256 hash = wtxIn.GetHash();

    if (fFromLoadWallet)
    {
        mapWallet[hash] = wtxIn;
        CWalletTx& wtx = mapWallet[hash];
        wtx.BindWallet(this);
        wtxOrdered.insert(make_pair(wtx.nOrderPos, TxPair(&wtx, (CAccountingEntry*)0)));
        AddToSpends(hash);
        for(const CTxIn& txin: wtx.vin) {
            if (mapWallet.count(txin.prevout.hash)) {
                CWalletTx& prevtx = mapWallet[txin.prevout.hash];
                if (prevtx.nIndex == -1 && !prevtx.hashUnset()) {
                    MarkConflicted(prevtx.hashBlock, wtx.GetHash());
                }
            }
        }
    }
    else
    {
        LOCK(cs_wallet);
        // Inserts only if not already there, returns tx inserted or tx found
        pair<map<uint256, CWalletTx>::iterator, bool> ret = mapWallet.insert(make_pair(hash, wtxIn));
        CWalletTx& wtx = (*ret.first).second;
        wtx.BindWallet(this);
        bool fInsertedNew = ret.second;
        if (fInsertedNew)
        {
            wtx.nTimeReceived = GetAdjustedTime();
            wtx.nOrderPos = IncOrderPosNext(pwalletdb);
            wtxOrdered.insert(make_pair(wtx.nOrderPos, TxPair(&wtx, (CAccountingEntry*)0)));

            wtx.nTimeSmart = wtx.nTimeReceived;
            if (!wtxIn.hashUnset())
            {
                if (mapBlockIndex.count(wtxIn.hashBlock))
                {
                    int64_t latestNow = wtx.nTimeReceived;
                    int64_t latestEntry = 0;
                    {
                        // Tolerate times up to the last timestamp in the wallet not more than 5 minutes into the future
                        int64_t latestTolerated = latestNow + 300;
                        const TxItems & txOrdered = wtxOrdered;
                        for (TxItems::const_reverse_iterator it = txOrdered.rbegin(); it != txOrdered.rend(); ++it)
                        {
                            CWalletTx *const pwtx = (*it).second.first;
                            if (pwtx == &wtx)
                                continue;
                            CAccountingEntry *const pacentry = (*it).second.second;
                            int64_t nSmartTime;
                            if (pwtx)
                            {
                                nSmartTime = pwtx->nTimeSmart;
                                if (!nSmartTime)
                                    nSmartTime = pwtx->nTimeReceived;
                            }
                            else
                                nSmartTime = pacentry->nTime;
                            if (nSmartTime <= latestTolerated)
                            {
                                latestEntry = nSmartTime;
                                if (nSmartTime > latestNow)
                                    latestNow = nSmartTime;
                                break;
                            }
                        }
                    }

                    int64_t blocktime = mapBlockIndex[wtxIn.hashBlock]->GetBlockTime();
                    wtx.nTimeSmart = std::max(latestEntry, std::min(blocktime, latestNow));
                }
                else
                    LogPrintf("AddToWallet(): found %s in block %s not in index\n",
                             wtxIn.GetHash().ToString(),
                             wtxIn.hashBlock.ToString());
            }
            AddToSpends(hash);
        }

        bool fUpdated = false;
        if (!fInsertedNew)
        {
            // Merge
            if (!wtxIn.hashUnset() && wtxIn.hashBlock != wtx.hashBlock)
            {
                wtx.hashBlock = wtxIn.hashBlock;
                fUpdated = true;
            }
            // If no longer abandoned, update
            if (wtxIn.hashBlock.IsNull() && wtx.isAbandoned())
            {
                wtx.hashBlock = wtxIn.hashBlock;
                fUpdated = true;
            }
            if (wtxIn.nIndex != -1 && (wtxIn.nIndex != wtx.nIndex))
            {
                wtx.nIndex = wtxIn.nIndex;
                fUpdated = true;
            }
            if (wtxIn.fFromMe && wtxIn.fFromMe != wtx.fFromMe)
            {
                wtx.fFromMe = wtxIn.fFromMe;
                fUpdated = true;
            }
        }

        if (wtx.IsCTOutput())
        {
            std::vector<RangeproofEncodedData> data;

            if (blsctData == nullptr)
            {
                CValidationState state;
                CStateView dummy;
                CStateViewCache view(&dummy);
                blsctKey k;

                if (GetBLSCTViewKey(k))
                {
                    if (VerifyBLSCT(wtx, k.GetKey(), data, view, state, true))
                    {
                        blsctData = &data;
                    }
                    else
                    {
                        return error("%s: VerifyBLSCT returned false: %s\n", __func__, state.GetRejectReason());
                    }
                }
            }

            wtx.vAmounts.resize(wtx.vout.size());
            wtx.vMemos.resize(wtx.vout.size());
            wtx.vGammas.resize(wtx.vout.size());

            if (blsctData != nullptr)
            {
                for (auto& it: *blsctData)
                {
                    if (it.valid)
                    {
                        wtx.vAmounts[it.index] = it.amount;
                        wtx.vMemos[it.index] = it.message;
                        wtx.vGammas[it.index] = it.gamma;
                    }
                }
            }
        }

        //// debug print
        LogPrintf("AddToWallet %s  %s%s\n", wtxIn.GetHash().ToString(), (fInsertedNew ? "new" : ""), (fUpdated ? "update" : ""));

        // Write to disk
        if (fInsertedNew || fUpdated)
            if (!pwalletdb->WriteTx(wtx))
                return false;

        // Break debit/credit balance caches:
        wtx.MarkDirty();

        // Notify UI of new or updated transaction
        NotifyTransactionChanged(this, hash, fInsertedNew ? CT_NEW : CT_UPDATED);

        // notify an external script when a wallet transaction comes in or is updated
        std::string strCmd = GetArg("-walletnotify", "");

        if ( !strCmd.empty())
        {
            boost::replace_all(strCmd, "%s", wtxIn.GetHash().GetHex());
            boost::thread t(runCommand, strCmd); // thread runs free
        }

    }
    return true;
}

/**
 * Add a transaction to the wallet, or update it.
 * pblock is optional, but should be provided if the transaction is known to be in a block.
 * If fUpdate is true, existing transactions will be updated.
 */
bool CWallet::AddToWalletIfInvolvingMe(const CTransaction& tx, const CBlock* pblock, bool fUpdate, const std::vector<RangeproofEncodedData> *blsctData)
{
    {
        AssertLockHeld(cs_wallet);

        if (pblock) {
            for(const CTxIn& txin: tx.vin) {
                std::pair<TxSpends::const_iterator, TxSpends::const_iterator> range = mapTxSpends.equal_range(txin.prevout);
                while (range.first != range.second) {
                    if (range.first->second != tx.GetHash()) {
                        if (!tx.IsBLSInput())
                        {
                            LogPrintf("Transaction %s (in block %s) conflicts with wallet transaction %s (both spend %s:%i)\n", tx.GetHash().ToString(), pblock->GetHash().ToString(), range.first->second.ToString(), range.first->first.hash.ToString(), range.first->first.n);
                            MarkConflicted(pblock->GetHash(), range.first->second);
                        }
                        else
                        {
                            AbandonTransaction(range.first->second);
                        }
                    }
                    range.first++;
                }
            }
        }

        bool fExisted = mapWallet.count(tx.GetHash()) != 0;
        if (fExisted && !fUpdate) return false;
        if (fExisted || IsMine(tx) || IsFromMe(tx))
        {
            CWalletTx wtx(this,tx);

            // Get merkle branch if transaction was found in a block
            if (pblock)
                wtx.SetMerkleBranch(*pblock);

            // Do not flush the wallet here for performance reasons
            // this is safe, as in case of a crash, we rescan the necessary blocks on startup through our SetBestChain-mechanism
            CWalletDB walletdb(strWalletFile, "r+", false);

            return AddToWallet(wtx, false, &walletdb);
        }
    }
    return false;
}

bool CWallet::AbandonTransaction(const uint256& hashTx)
{
    LOCK2(cs_main, cs_wallet);

    // Do not flush the wallet here for performance reasons
    CWalletDB walletdb(strWalletFile, "r+", false);

    std::set<uint256> todo;
    std::set<uint256> done;

    // Can't mark abandoned if confirmed or in mempool
    assert(mapWallet.count(hashTx));
    CWalletTx& origtx = mapWallet[hashTx];
    if (origtx.GetDepthInMainChain() > 0 || origtx.InMempool() || origtx.InStempool()) {
        LogPrintf("AbandonTransaction : Could not abandon transaction.%s%s",origtx.InMempool()?" Tx in Mempool":"",origtx.GetDepthInMainChain() > 0?" Tx already confirmed":"");
        return false;
    }

    todo.insert(hashTx);

    while (!todo.empty()) {
        uint256 now = *todo.begin();
        todo.erase(now);
        done.insert(now);
        assert(mapWallet.count(now));
        CWalletTx& wtx = mapWallet[now];
        int currentconfirm = wtx.GetDepthInMainChain();
        // If the orig tx was not in block, none of its spends can be
        assert(currentconfirm <= 0);
        // if (currentconfirm < 0) {Tx and spends are already conflicted, no need to abandon}
        if (currentconfirm == 0 && !wtx.isAbandoned()) {
            // If the orig tx was not in block/mempool, none of its spends can be in mempool
            assert(!wtx.InMempool());
            assert(!wtx.InStempool());
            wtx.nIndex = -1;
            wtx.setAbandoned();
            wtx.MarkDirty();
            walletdb.WriteTx(wtx);
            NotifyTransactionChanged(this, wtx.GetHash(), CT_UPDATED);
            // Iterate over all its outputs, and mark transactions in the wallet that spend them abandoned too
            TxSpends::const_iterator iter = mapTxSpends.lower_bound(COutPoint(hashTx, 0));
            while (iter != mapTxSpends.end() && iter->first.hash == now) {
                if (!done.count(iter->second)) {
                    todo.insert(iter->second);
                }
                iter++;
            }
            // If a transaction changes 'conflicted' state, that changes the balance
            // available of the outputs it spends. So force those to be recomputed
            for(const CTxIn& txin: wtx.vin)
            {
                if (mapWallet.count(txin.prevout.hash))
                    mapWallet[txin.prevout.hash].MarkDirty();
            }
        }
    }

    return true;
}

void CWallet::MarkConflicted(const uint256& hashBlock, const uint256& hashTx)
{
    LOCK2(cs_main, cs_wallet);

    int conflictconfirms = 0;
    if (mapBlockIndex.count(hashBlock)) {
        CBlockIndex* pindex = mapBlockIndex[hashBlock];
        if (chainActive.Contains(pindex)) {
            conflictconfirms = -(chainActive.Height() - pindex->nHeight + 1);
        }
    }
    // If number of conflict confirms cannot be determined, this means
    // that the block is still unknown or not yet part of the main chain,
    // for example when loading the wallet during a reindex. Do nothing in that
    // case.
    if (conflictconfirms >= 0)
        return;

    // Do not flush the wallet here for performance reasons
    CWalletDB walletdb(strWalletFile, "r+", false);

    std::set<uint256> todo;
    std::set<uint256> done;

    todo.insert(hashTx);

    while (!todo.empty()) {
        uint256 now = *todo.begin();
        todo.erase(now);
        done.insert(now);
        assert(mapWallet.count(now));
        CWalletTx& wtx = mapWallet[now];
        int currentconfirm = wtx.GetDepthInMainChain();
        if (conflictconfirms < currentconfirm) {
            // Block is 'more conflicted' than current confirm; update.
            // Mark transaction as conflicted with this block.
            wtx.nIndex = -1;
            wtx.hashBlock = hashBlock;
            wtx.MarkDirty();
            walletdb.WriteTx(wtx);
            // Iterate over all its outputs, and mark transactions in the wallet that spend them conflicted too
            TxSpends::const_iterator iter = mapTxSpends.lower_bound(COutPoint(now, 0));
            while (iter != mapTxSpends.end() && iter->first.hash == now) {
                 if (!done.count(iter->second)) {
                     todo.insert(iter->second);
                 }
                 iter++;
            }
            // If a transaction changes 'conflicted' state, that changes the balance
            // available of the outputs it spends. So force those to be recomputed
            for(const CTxIn& txin: wtx.vin)
            {
                if (mapWallet.count(txin.prevout.hash))
                    mapWallet[txin.prevout.hash].MarkDirty();
            }
        }
    }
}

void CWallet::SyncTransaction(const CTransaction& tx, const CBlockIndex *pindex, const CBlock* pblock, bool fConnect, const std::vector<RangeproofEncodedData> *blsctData)
{
    LOCK2(cs_main, cs_wallet);

    if (!AddToWalletIfInvolvingMe(tx, pblock, true, blsctData))
       return; // Not one of ours

    // If a transaction changes 'conflicted' state, that changes the balance
    // available of the outputs it spends. So force those to be
    // recomputed, also:
    for(const CTxIn& txin: tx.vin)
    {
        if (mapWallet.count(txin.prevout.hash))
            mapWallet[txin.prevout.hash].MarkDirty();
    }
}

isminetype CWallet::IsMine(const CTxIn &txin) const
{
    {
        LOCK(cs_wallet);
        map<uint256, CWalletTx>::const_iterator mi = mapWallet.find(txin.prevout.hash);
        if (mi != mapWallet.end())
        {
            const CWalletTx& prev = (*mi).second;
            if (txin.prevout.n < prev.vout.size())
                return IsMine(prev.vout[txin.prevout.n]);
        }
    }
    return ISMINE_NO;
}

CAmount CWallet::GetDebit(const CTxIn &txin, const isminefilter& filter) const
{
    {
        LOCK(cs_wallet);
        map<uint256, CWalletTx>::const_iterator mi = mapWallet.find(txin.prevout.hash);
        if (mi != mapWallet.end())
        {
            const CWalletTx& prev = (*mi).second;
            if (txin.prevout.n < prev.vout.size())
                if (IsMine(prev.vout[txin.prevout.n]) & filter)
                {
                    if (prev.vout[txin.prevout.n].HasRangeProof())
                        return prev.vAmounts[txin.prevout.n];
                    else
                        return prev.vout[txin.prevout.n].nValue;
                }
        }
    }
    return 0;
}

isminetype CWallet::IsMine(const CTxOut& txout) const
{
    if (txout.HasRangeProof() && txout.IsBLSCT())
    {
        std::vector<RangeproofEncodedData> blsctData;
        std::vector<std::pair<int, BulletproofsRangeproof>> proofs;
        std::vector<bls::G1Element> nonces;
        blsctKey v;

        if (GetBLSCTViewKey(v) && txout.ephemeralKey.size() > 0 && txout.outputKey.size() > 0 && txout.spendingKey.size() > 0)
        {
            try
            {
                proofs.push_back(std::make_pair(0,txout.bp));
                bls::G1Element t = bls::G1Element::FromByteVector(txout.outputKey);
                bls::PrivateKey k = v.GetKey();
                t = t * k;

                nonces.push_back(t);
                bool fValidBP = VerifyBulletproof(proofs, blsctData, nonces, true);
                bool fHaveSubAddressKey = HaveBLSCTSubAddress(txout.outputKey, txout.spendingKey);
                if (fValidBP && blsctData.size() == 1 && fHaveSubAddressKey)
                {
                    return ISMINE_SPENDABLE_PRIVATE;
                }
            }
            catch(...)
            {
                return ISMINE_NO;
            }
        }
        return ISMINE_NO;
    }
    else
        return ::IsMine(*this, txout.scriptPubKey);
}

CAmount CWallet::GetCredit(const CTxOut& txout, const isminefilter& filter) const
{
    if (!MoneyRange(txout.nValue))
        throw std::runtime_error("CWallet::GetCredit(): value out of range");
    return ((IsMine(txout) & filter) ? txout.nValue : 0);
}

bool CWallet::IsChange(const CTxOut& txout) const
{
    // TODO: fix handling of 'change' outputs. The assumption is that any
    // payment to a script that is ours, but is not in the address book
    // is change. That assumption is likely to break when we implement multisignature
    // wallets that return change back into a multi-signature-protected address;
    // a better way of identifying which outputs are 'the send' and which are
    // 'the change' will need to be implemented (maybe extend CWalletTx to remember
    // which output, if any, was change).
    if (::IsMine(*this, txout.scriptPubKey))
    {
        CTxDestination address;
        if (!ExtractDestination(txout.scriptPubKey, address))
            return true;

        LOCK(cs_wallet);
        if (!mapAddressBook.count(address))
            return true;
    }
    return false;
}

CAmount CWallet::GetChange(const CTxOut& txout) const
{
    if (!MoneyRange(txout.nValue))
        throw std::runtime_error("CWallet::GetChange(): value out of range");
    return (IsChange(txout) ? txout.nValue : 0);
}

bool CWallet::IsMine(const CTransaction& tx) const
{
    for(const CTxOut& txout: tx.vout)
        if (IsMine(txout))
            return true;
    return false;
}

bool CWallet::IsFromMe(const CTransaction& tx) const
{
    return (GetDebit(tx, ISMINE_ALL) > 0);
}

CAmount CWallet::GetDebit(const CTransaction& tx, const isminefilter& filter) const
{
    CAmount nDebit = 0;
    for(const CTxIn& txin: tx.vin)
    {
        nDebit += GetDebit(txin, filter);
        if (!MoneyRange(nDebit))
            throw std::runtime_error("CWallet::GetDebit(): value out of range");
    }
    return nDebit;
}

CAmount CWallet::GetCredit(const CTransaction& tx, const isminefilter& filter) const
{
    CAmount nCredit = 0;
    for(const CTxOut& txout: tx.vout)
    {
        nCredit += GetCredit(txout, filter);
        if (!MoneyRange(nCredit))
            throw std::runtime_error("CWallet::GetCredit(): value out of range");
    }
    return nCredit;
}

CAmount CWallet::GetChange(const CTransaction& tx) const
{
    CAmount nChange = 0;
    for(const CTxOut& txout: tx.vout)
    {
        nChange += GetChange(txout);
        if (!MoneyRange(nChange))
            throw std::runtime_error("CWallet::GetChange(): value out of range");
    }
    return nChange;
}

CPubKey CWallet::ImportMnemonic(word_list mnemonic, dictionary lang)
{
    CKey key;

    std::vector<unsigned char> vKey = key_from_mnemonic(mnemonic, lang);

    key.Set(vKey.begin(), vKey.end(), false);

    int64_t nCreationTime = GetArg("-importmnemonicfromtime", chainActive.Genesis()->GetBlockTime());
    CKeyMetadata metadata(nCreationTime);

    // calculate the pubkey
    CPubKey pubkey = key.GetPubKey();
    assert(key.VerifyPubKey(pubkey));

    // set the hd keypath to "m" -> Master, refers the masterkeyid to itself
    metadata.hdKeypath     = "m";
    metadata.hdMasterKeyID = pubkey.GetID();

    {
        LOCK(cs_wallet);

        // mem store the metadata
        mapKeyMetadata[pubkey.GetID()] = metadata;

        if (!nTimeFirstKey || nCreationTime < nTimeFirstKey)
            nTimeFirstKey = nCreationTime;

        // write the key&metadata to the database
        if (!AddKeyPubKey(key, pubkey))
            throw std::runtime_error("CWallet::GenerateNewKey(): AddKey failed");
    }

    return pubkey;
}


CPubKey CWallet::GenerateNewHDMasterKey()
{
    CKey key;
    key.MakeNewKey(true);

    int64_t nCreationTime = GetTime();
    CKeyMetadata metadata(nCreationTime);

    // calculate the pubkey
    CPubKey pubkey = key.GetPubKey();
    assert(key.VerifyPubKey(pubkey));

    // set the hd keypath to "m" -> Master, refers the masterkeyid to itself
    metadata.hdKeypath     = "m";
    metadata.hdMasterKeyID = pubkey.GetID();

    {
        LOCK(cs_wallet);

        // mem store the metadata
        mapKeyMetadata[pubkey.GetID()] = metadata;

        // write the key&metadata to the database
        if (!AddKeyPubKey(key, pubkey))
            throw std::runtime_error("CWallet::GenerateNewKey(): AddKey failed");
    }

    return pubkey;
}

bool CWallet::SetHDMasterKey(const CPubKey& pubkey)
{
    LOCK(cs_wallet);

    // ensure this wallet.dat can only be opened by clients supporting HD
    SetMinVersion(FEATURE_HD);

    // store the keyid (hash160) together with
    // the child index counter in the database
    // as a hdchain object
    CHDChain newHdChain;
    newHdChain.masterKeyID = pubkey.GetID();
    SetHDChain(newHdChain, false);

    return true;
}

bool CWallet::SetHDChain(const CHDChain& chain, bool memonly)
{
    LOCK(cs_wallet);
    if (!memonly && !CWalletDB(strWalletFile).WriteHDChain(chain))
        throw runtime_error("AddHDChain(): writing chain failed");

    hdChain = chain;
    return true;
}

int64_t CWalletTx::GetTxTime() const
{
    int64_t n = nTimeSmart;
    return n ? n : nTimeReceived;
}

int CWalletTx::GetRequestCount() const
{
    // Returns -1 if it wasn't being tracked
    int nRequests = -1;
    {
        LOCK(pwallet->cs_wallet);
        if (IsCoinBase() || IsCoinStake())
        {
            // Generated block
            if (!hashUnset())
            {
                map<uint256, int>::const_iterator mi = pwallet->mapRequestCount.find(hashBlock);
                if (mi != pwallet->mapRequestCount.end())
                    nRequests = (*mi).second;
            }
        }
        else
        {
            // Did anyone request this transaction?
            map<uint256, int>::const_iterator mi = pwallet->mapRequestCount.find(GetHash());
            if (mi != pwallet->mapRequestCount.end())
            {
                nRequests = (*mi).second;

                // How about the block it's in?
                if (nRequests == 0 && !hashUnset())
                {
                    map<uint256, int>::const_iterator mi = pwallet->mapRequestCount.find(hashBlock);
                    if (mi != pwallet->mapRequestCount.end())
                        nRequests = (*mi).second;
                    else
                        nRequests = 1; // If it's in someone else's block it must have got out
                }
            }
        }
    }
    return nRequests;
}

void CWalletTx::GetAmounts(list<COutputEntry>& listReceived,
                           list<COutputEntry>& listSent, CAmount& nFee, string& strSentAccount, const isminefilter& filter) const
{
    nFee = 0;
    listReceived.clear();
    listSent.clear();
    strSentAccount = strFromAccount;

    // Compute fee:
    CAmount nDebit = GetDebit(filter);
    if (nDebit > 0) // debit>0 means we signed/sent this transaction
    {
        CAmount nValueOut = GetValueOut();
        CAmount nValueOutCFund = GetValueOutCFund();
        nFee = IsBLSCT() ? GetFee() : nDebit - nValueOut + nValueOutCFund;
    }

    CAmount nReward = -nDebit;
    if (IsCoinStake())
    {
        for (unsigned int j = 0; j < vout.size(); j++)
        {
            if (vout[j].scriptPubKey == vout[1].scriptPubKey)
            {
                nReward += vout[j].nValue;
            }
        }
    }

    bool fAddedReward = false;

    // Sent/received.
    for (unsigned int i = 0; i < vout.size(); ++i)
    {
        const CTxOut& txout = vout[i];

        // Skip special stake out
        if (txout.scriptPubKey.empty() || (IsCoinStake() && txout.scriptPubKey.IsCommunityFundContribution()))
            continue;

        isminetype fIsMine = pwallet->IsMine(txout);
        // Only need to handle txouts if AT LEAST one of these is true:
        //   1) they debit from us (sent)
        //   2) the output is to us (received)
        if (nDebit > 0)
        {
            // Don't report 'change' txouts
            // if (pwallet->IsChange(txout))
            //     continue;
            fIsMine = pwallet->IsMine(txout);
        }
        else if (!(fIsMine & filter))
            continue;

        // In either case, we need to get the destination address
        CTxDestination address;

        if (!ExtractDestination(txout.scriptPubKey, address))
        {
            if(!txout.scriptPubKey.IsCommunityFundContribution() && !txout.IsBLSCT())
                LogPrintf("CWalletTx::GetAmounts: Unknown transaction type found, txid %s n: %d %s\n",
                         this->GetHash().ToString(), i, txout.ToString());
            address = CNoDestination();
        }

        CAmount amount = txout.HasRangeProof() ? vAmounts[i] : txout.nValue;

        COutputEntry output = {address, amount, (int)i};

        // If we are debited by the transaction, add the output as a "sent" entry
        if (nDebit > 0)
            listSent.push_back(output);

        // If we are receiving the output, add it as a "received" entry
        if (IsCoinStake() && fIsMine & filter && txout.scriptPubKey == vout[1].scriptPubKey)
        {
            if (fAddedReward)
            {
                continue; // only append details of the address with reward output
            }
            else
            {
                fAddedReward = true;
                output.amount = nReward;
                listReceived.push_back(output);
            }
        }
        else if (fIsMine & filter)
            listReceived.push_back(output);
    }

}

void CWalletTx::GetAccountAmounts(const string& strAccount, CAmount& nReceived,
                                  CAmount& nSent, CAmount& nFee, const isminefilter& filter) const
{
    nReceived = nSent = nFee = 0;

    CAmount allFee;
    string strSentAccount;
    list<COutputEntry> listReceived;
    list<COutputEntry> listSent;
    GetAmounts(listReceived, listSent, allFee, strSentAccount, filter);

    if (strAccount == strSentAccount)
    {
        for(const COutputEntry& s: listSent)
            nSent += s.amount;
        nFee = allFee;
    }
    {
        LOCK(pwallet->cs_wallet);
        for(const COutputEntry& r: listReceived)
        {
            if (pwallet->mapAddressBook.count(r.destination))
            {
                map<CTxDestination, CAddressBookData>::const_iterator mi = pwallet->mapAddressBook.find(r.destination);
                if (mi != pwallet->mapAddressBook.end() && (*mi).second.name == strAccount)
                    nReceived += r.amount;
            }
            else if (strAccount.empty())
            {
                nReceived += r.amount;
            }
        }
    }
}

/**
 * Scan the block chain (starting in pindexStart) for transactions
 * from or to us. If fUpdate is true, found transactions that already
 * exist in the wallet will be updated.
 */
int CWallet::ScanForWalletTransactions(CBlockIndex* pindexStart, bool fUpdate)
{
    int ret = 0;
    int64_t nNow = GetTime();
    const CChainParams& chainParams = Params();

    CBlockIndex* pindex = pindexStart;
    {
        LOCK2(cs_main, cs_wallet);

        // no need to read and scan block, if block was created before
        // our wallet birthday (as adjusted for block time variability)
        while (pindex && nTimeFirstKey && (pindex->GetBlockTime() < (nTimeFirstKey - 7200)))
            pindex = chainActive.Next(pindex);

        ShowProgress(_("Rescanning..."), 0); // show rescan progress in GUI as dialog or on splashscreen, if -rescan on startup
        double dProgressStart = Checkpoints::GuessVerificationProgress(chainParams.Checkpoints(), pindex, false);
        double dProgressTip = Checkpoints::GuessVerificationProgress(chainParams.Checkpoints(), chainActive.Tip(), false);
        while (pindex)
        {
            if (pindex->nHeight % 100 == 0 && dProgressTip - dProgressStart > 0.0)
                ShowProgress(_("Rescanning..."), std::max(1, std::min(99, (int)((Checkpoints::GuessVerificationProgress(chainParams.Checkpoints(), pindex, false) - dProgressStart) / (dProgressTip - dProgressStart) * 100))));

            CBlock block;
            ReadBlockFromDisk(block, pindex, Params().GetConsensus());
            for(CTransaction& tx: block.vtx)
            {
                std::vector<RangeproofEncodedData> blsctData;
                CValidationState state;
                CStateView dummy;
                CStateViewCache view(&dummy);
                blsctKey k;

                bool fValid = true;

                if (GetBLSCTViewKey(k))
                    fValid=VerifyBLSCT(tx, k.GetKey(), blsctData, view, state, true);

                if (!fValid)
                    continue;

                if (AddToWalletIfInvolvingMe(tx, &block, fUpdate, &blsctData))
                    ret++;
            }
            pindex = chainActive.Next(pindex);
            if (GetTime() >= nNow + 60) {
                nNow = GetTime();
                LogPrintf("Still rescanning. At block %d. Progress=%f\n", pindex->nHeight, Checkpoints::GuessVerificationProgress(chainParams.Checkpoints(), pindex));
            }
        }
        ShowProgress(_("Rescanning..."), 100); // hide progress dialog in GUI
    }
    return ret;
}

void CWallet::ReacceptWalletTransactions()
{
    // If transactions aren't being broadcasted, don't let them into local mempool either
    if (!fBroadcastTransactions)
        return;
    LOCK2(cs_main, cs_wallet);
    std::map<int64_t, CWalletTx*> mapSorted;

    // Sort pending wallet transactions based on their initial wallet insertion order
    for(PAIRTYPE(const uint256, CWalletTx)& item: mapWallet)
    {
        const uint256& wtxid = item.first;
        CWalletTx& wtx = item.second;
        assert(wtx.GetHash() == wtxid);

        int nDepth = wtx.GetDepthInMainChain();

        if (!(wtx.IsCoinBase() || wtx.IsCoinStake()) && (nDepth == 0 && !wtx.isAbandoned())) {
            mapSorted.insert(std::make_pair(wtx.nOrderPos, &wtx));
        }
    }

    // Try to add wallet transactions to memory pool
    for(PAIRTYPE(const int64_t, CWalletTx*)& item: mapSorted)
    {
        CWalletTx& wtx = *(item.second);

        LOCK2(stempool.cs, mempool.cs);
        wtx.AcceptToMemoryPool(false, maxTxFee);
        // If Dandelion enabled, relay transaction once again.
        if (GetBoolArg("-dandelion", true)) {
            wtx.RelayWalletTransaction();
        }
    }
}

bool CWalletTx::RelayWalletTransaction()
{
    assert(pwallet->GetBroadcastTransactions());
    if (!IsCoinBase() && !isAbandoned() && GetDepthInMainChain() == 0) {
        CValidationState state;
        /* GetDepthInMainChain already catches known conflicts. */
        if (InMempool() || InStempool()) {
            // If Dandelion enabled, push inventory item to just one destination.
            if (GetBoolArg("-dandelion", true)) {
                int64_t nCurrTime = GetTimeMicros();
                int64_t nEmbargo = 1000000*DANDELION_EMBARGO_MINIMUM+PoissonNextSend(nCurrTime, DANDELION_EMBARGO_AVG_ADD);
                InsertDandelionEmbargo(GetHash(),nEmbargo);
                LogPrint("dandelion", "dandeliontx %s embargoed for %d seconds\n", GetHash().ToString(), (nEmbargo-nCurrTime)/1000000);
                CInv inv(MSG_DANDELION_TX, GetHash());
                return LocalDandelionDestinationPushInventory(inv);
            } else {
                LogPrintf("Relaying wtx %s\n", GetHash().ToString());
                RelayTransaction((CTransaction)*this);
                return true;
            }
        }
    }
    return false;
}

set<uint256> CWalletTx::GetConflicts() const
{
    set<uint256> result;
    if (pwallet != nullptr)
    {
        uint256 myHash = GetHash();
        result = pwallet->GetConflicts(myHash);
        result.erase(myHash);
    }
    return result;
}

CAmount CWalletTx::GetDebit(const isminefilter& filter) const
{
    if (vin.empty())
        return 0;

    CAmount debit = 0;

    if(filter & ISMINE_SPENDABLE)
    {
        if (fDebitCached)
            debit += nDebitCached;
        else
        {
            nDebitCached = pwallet->GetDebit(*this, ISMINE_SPENDABLE);
            fDebitCached = true;
            debit += nDebitCached;
        }
    }
    else if (filter & ISMINE_STAKABLE)
    {
        if (fColdStakingDebitCached)
            debit += nColdStakingDebitCached;
        else
        {
            nColdStakingDebitCached = pwallet->GetDebit(*this, ISMINE_STAKABLE);
            fColdStakingDebitCached = true;
            debit += nColdStakingDebitCached;
        }
    }

    if(filter & ISMINE_WATCH_ONLY)
    {
        if(fWatchDebitCached)
            debit += nWatchDebitCached;
        else
        {
            nWatchDebitCached = pwallet->GetDebit(*this, ISMINE_WATCH_ONLY);
            fWatchDebitCached = true;
            debit += nWatchDebitCached;
        }
    }

    if (filter & ISMINE_SPENDABLE_PRIVATE)
    {
        if (fPrivateDebitCached) {
            debit += nPrivateDebitCached;
        } else
        {
            nPrivateDebitCached = pwallet->GetDebit(*this, ISMINE_SPENDABLE_PRIVATE);
            fPrivateDebitCached = true;
            debit += nPrivateDebitCached;
        }
    }

    return debit;
}

CAmount CWalletTx::GetCredit(const isminefilter& filter, bool fCheckMaturity) const
{
    // Must wait until coinbase is safely deep enough in the chain before valuing it
    if (fCheckMaturity && (IsCoinBase() || IsCoinStake()) && GetBlocksToMaturity() > 0)
        return 0;

    CAmount credit = 0;

    if (filter & ISMINE_SPENDABLE)
    {
        // GetBalance can assume transactions in mapWallet won't change
        if (fCreditCached)
            credit += nCreditCached;
        else
        {
            nCreditCached = pwallet->GetCredit(*this, ISMINE_SPENDABLE);
            fCreditCached = true;
            credit += nCreditCached;
        }
    }
    else if (filter & ISMINE_STAKABLE)
    {
        if (fColdStakingCreditCached)
            credit += nColdStakingCreditCached;
        else
        {
            nColdStakingCreditCached = pwallet->GetCredit(*this, ISMINE_STAKABLE);
            fColdStakingCreditCached = true;
            credit += nColdStakingCreditCached;
        }
    }

    if (filter & ISMINE_WATCH_ONLY)
    {
        if (fWatchCreditCached)
            credit += nWatchCreditCached;
        else
        {
            nWatchCreditCached = pwallet->GetCredit(*this, ISMINE_WATCH_ONLY);
            fWatchCreditCached = true;
            credit += nWatchCreditCached;
        }
    }

    if (filter & ISMINE_SPENDABLE_PRIVATE)
    {
        if (fPrivateCreditCached)
            credit += nPrivateCreditCached;
        else
        {
            CAmount nCredit = 0;
            unsigned int i = -1;
            for(const CTxOut& txout: this->vout)
            {
                i++;
                if (txout.spendingKey.size() == 0)
                    continue;

                CAmount amount = i < vAmounts.size() ? vAmounts[i] : 0;

                if (!MoneyRange(amount))
                    throw std::runtime_error("CWallet::GetAvailableCredit(): value out of range");

                nCredit += ((pwallet->IsMine(txout) & filter) ? amount : 0);

                if (!MoneyRange(nCredit))
                    throw std::runtime_error("CWallet::GetCredit(): value out of range");
            }
            nPrivateCreditCached = nCredit;
            fPrivateCreditCached = true;
            credit += nPrivateCreditCached;
        }
    }

    return credit;
}

std::map<std::pair<uint256,int>, uint64_t> mapLockedOutputs;

void LockOutputFor(uint256 hash, unsigned int n, uint64_t time)
{
    mapLockedOutputs[std::make_pair(hash, n)] = GetTime() + time;
}

bool IsOutputLocked(uint256 hash, unsigned int n)
{
    if (mapLockedOutputs.count(std::make_pair(hash, n)) == 0)
        return false;

    bool ret = mapLockedOutputs.at(std::make_pair(hash, n)) > GetTime();

    if (!ret)
        mapLockedOutputs.erase(std::make_pair(hash, n));

    return ret;
}

CAmount CWalletTx::GetAvailablePrivateCredit(bool fLocked) const
{
    if (pwallet == 0)
        return 0;

    CAmount nCredit = 0;
    uint256 hashTx = GetHash();
    for (unsigned int i = 0; i < vout.size(); i++)
    {
        if (vout[i].spendingKey.size() == 0)
            continue;

        CAmount amount = vAmounts[i];

        if (!pwallet->IsSpent(hashTx, i) && IsOutputLocked(hashTx, i) == fLocked)
        {
            const CTxOut &txout = vout[i];

            nCredit += (pwallet->IsMine(txout) & ISMINE_SPENDABLE_PRIVATE) ? amount : 0;
            if (!MoneyRange(nCredit))
                throw std::runtime_error("CWalletTx::GetAvailableCredit() : value out of range");
        }
    }

    return nCredit;
}

CAmount CWalletTx::GetImmatureCredit(bool fUseCache) const
{
    if ((IsCoinBase() || IsCoinStake()) && GetBlocksToMaturity() > 0 && IsInMainChain())
    {
        if (fUseCache && fImmatureCreditCached)
            return nImmatureCreditCached;
        nImmatureCreditCached = pwallet->GetCredit(*this, ISMINE_SPENDABLE);
        fImmatureCreditCached = true;
        return nImmatureCreditCached;
    }

    return 0;
}

CAmount CWalletTx::GetAvailableCredit(bool fUseCache) const
{
    if (pwallet == 0)
        return 0;

    // Must wait until coinbase is safely deep enough in the chain before valuing it
    if ((IsCoinBase() || IsCoinStake()) && GetBlocksToMaturity() > 0)
        return 0;

    if (fUseCache && fAvailableCreditCached)
        return nAvailableCreditCached;

    CAmount nCredit = 0;
    uint256 hashTx = GetHash();
    for (unsigned int i = 0; i < vout.size(); i++)
    {
        if (!pwallet->IsSpent(hashTx, i))
        {
            const CTxOut &txout = vout[i];
            nCredit += pwallet->GetCredit(txout, ISMINE_SPENDABLE);
            if (!MoneyRange(nCredit))
                throw std::runtime_error("CWalletTx::GetAvailableCredit() : value out of range");
        }
    }

    nAvailableCreditCached = nCredit;
    fAvailableCreditCached = true;
    return nCredit;
}

CAmount CWalletTx::GetAvailableStakableCredit() const
{
    if (pwallet == 0)
        return 0;

    // Must wait until coinbase is safely deep enough in the chain before valuing it
    if ((IsCoinBase() || IsCoinStake()) && GetBlocksToMaturity() > 0)
        return 0;

    CAmount nCredit = 0;
    uint256 hashTx = GetHash();
    for (unsigned int i = 0; i < vout.size(); i++)
    {
        if (!pwallet->IsSpent(hashTx, i))
        {
            const CTxOut &txout = vout[i];
            nCredit += pwallet->GetCredit(txout, ISMINE_STAKABLE);
            if (!MoneyRange(nCredit))
                throw std::runtime_error("CWalletTx::GetAvailableCredit() : value out of range");
        }
    }

    return nCredit;
}

CAmount CWalletTx::GetImmatureWatchOnlyCredit(const bool& fUseCache) const
{
    if ((IsCoinBase() || IsCoinStake()) && GetBlocksToMaturity() > 0 && IsInMainChain())
    {
        if (fUseCache && fImmatureWatchCreditCached)
            return nImmatureWatchCreditCached;
        nImmatureWatchCreditCached = pwallet->GetCredit(*this, ISMINE_WATCH_ONLY);
        fImmatureWatchCreditCached = true;
        return nImmatureWatchCreditCached;
    }

    return 0;
}

CAmount CWalletTx::GetAvailableWatchOnlyCredit(const bool& fUseCache) const
{
    if (pwallet == 0)
        return 0;

    // Must wait until coinbase is safely deep enough in the chain before valuing it
    if ((IsCoinBase() || IsCoinStake()) && GetBlocksToMaturity() > 0)
        return 0;

    if (fUseCache && fAvailableWatchCreditCached)
        return nAvailableWatchCreditCached;

    CAmount nCredit = 0;
    for (unsigned int i = 0; i < vout.size(); i++)
    {
        if (!pwallet->IsSpent(GetHash(), i))
        {
            const CTxOut &txout = vout[i];
            nCredit += pwallet->GetCredit(txout, ISMINE_WATCH_ONLY);
            if (!MoneyRange(nCredit))
                throw std::runtime_error("CWalletTx::GetAvailableCredit() : value out of range");
        }
    }

    nAvailableWatchCreditCached = nCredit;
    fAvailableWatchCreditCached = true;
    return nCredit;
}

CAmount CWalletTx::GetChange() const
{
    if (fChangeCached)
        return nChangeCached;
    nChangeCached = pwallet->GetChange(*this);
    fChangeCached = true;
    return nChangeCached;
}

bool CWalletTx::InMempool() const
{
    LOCK(mempool.cs);
    if (mempool.exists(GetHash())) {
        return true;
    }
    return false;
}


bool CWalletTx::InStempool() const
{
    LOCK(stempool.cs);
    if (stempool.exists(GetHash())) {
        return true;
    }
    return false;
}

bool CWalletTx::IsTrusted() const
{
    // Quick answer in most cases
    if (!CheckFinalTx(*this))
        return false;
    int nDepth = GetDepthInMainChain();
    if (nDepth >= 1)
        return true;
    if (nDepth < 0)
        return false;
    if (!bSpendZeroConfChange || !IsFromMe(ISMINE_ALL)) // using wtx's cached debit
        return false;

    // Don't trust unconfirmed transactions from us unless they are in the mempool.
    if (!InMempool() && !InStempool())
        return false;

    // Trusted if all inputs are from us and are in the mempool:
    for(const CTxIn& txin: vin)
    {
        // Transactions not sent by us: not trusted
        const CWalletTx* parent = pwallet->GetWalletTx(txin.prevout.hash);
        if (parent == nullptr)
            return false;
        const CTxOut& parentOut = parent->vout[txin.prevout.n];
        if (pwallet->IsMine(parentOut) != ISMINE_SPENDABLE && pwallet->IsMine(parentOut) != ISMINE_SPENDABLE_PRIVATE)
            return false;
    }
    return true;
}

bool CWalletTx::IsEquivalentTo(const CWalletTx& tx) const
{
        CMutableTransaction tx1 = *this;
        CMutableTransaction tx2 = tx;
        for (unsigned int i = 0; i < tx1.vin.size(); i++) tx1.vin[i].scriptSig = CScript();
        for (unsigned int i = 0; i < tx2.vin.size(); i++) tx2.vin[i].scriptSig = CScript();
        return CTransaction(tx1) == CTransaction(tx2);
}

std::vector<uint256> CWallet::ResendWalletTransactionsBefore(int64_t nTime)
{
    std::vector<uint256> result;

    LOCK(cs_wallet);
    // Sort them in chronological order
    multimap<unsigned int, CWalletTx*> mapSorted;
    for(PAIRTYPE(const uint256, CWalletTx)& item: mapWallet)
    {
        CWalletTx& wtx = item.second;
        // Don't rebroadcast if newer than nTime:
        if (wtx.nTimeReceived > nTime)
            continue;
        mapSorted.insert(make_pair(wtx.nTimeReceived, &wtx));
    }
    for(PAIRTYPE(const unsigned int, CWalletTx*)& item: mapSorted)
    {
        CWalletTx& wtx = *item.second;
        if (wtx.RelayWalletTransaction())
            result.push_back(wtx.GetHash());
    }
    return result;
}

void CWallet::ResendWalletTransactions(int64_t nBestBlockTime)
{
    // Do this infrequently and randomly to avoid giving away
    // that these are our transactions.
    if (GetTime() < nNextResend || !fBroadcastTransactions)
        return;
    bool fFirst = (nNextResend == 0);
    nNextResend = GetTime() + GetRand(30 * 60);
    if (fFirst)
        return;

    // Only do it if there's been a new block since last time
    if (nBestBlockTime < nLastResend)
        return;
    nLastResend = GetTime();

    // Rebroadcast unconfirmed txes older than 5 minutes before the last
    // block was found:
    std::vector<uint256> relayed = ResendWalletTransactionsBefore(nBestBlockTime-5*60);
    if (!relayed.empty())
        LogPrintf("%s: rebroadcast %u unconfirmed transactions\n", __func__, relayed.size());
}

/** @} */ // end of mapWallet




/** @defgroup Actions
 *
 * @{
 */


CAmount CWallet::GetBalance() const
{
    CAmount nTotal = 0;
    {
        LOCK2(cs_main, cs_wallet);
        for (map<uint256, CWalletTx>::const_iterator it = mapWallet.begin(); it != mapWallet.end(); ++it)
        {
            const CWalletTx* pcoin = &(*it).second;
            if (pcoin->IsTrusted())
                nTotal += pcoin->GetAvailableCredit();
        }
    }

    return nTotal;
}

CAmount CWallet::GetPrivateBalance() const
{
    CAmount nTotal = 0;
    {
        LOCK2(cs_main, cs_wallet);
        for (map<uint256, CWalletTx>::const_iterator it = mapWallet.begin(); it != mapWallet.end(); ++it)
        {
            const CWalletTx* pcoin = &(*it).second;
            if (pcoin->IsCTOutput())
                if (pcoin->IsInMainChain())
                    nTotal += pcoin->GetAvailablePrivateCredit();
        }
    }

    return nTotal;
}

CAmount CWallet::GetPrivateBalancePending() const
{
    CAmount nTotal = 0;
    {
        LOCK2(cs_main, cs_wallet);
        for (map<uint256, CWalletTx>::const_iterator it = mapWallet.begin(); it != mapWallet.end(); ++it)
        {
            const CWalletTx* pcoin = &(*it).second;
            if (pcoin->IsCTOutput())
                if (pcoin->GetDepthInMainChain() == 0 && (pcoin->InMempool() || pcoin->InStempool()))
                    nTotal += pcoin->GetAvailablePrivateCredit();
        }
    }

    return nTotal;
}

CAmount CWallet::GetPrivateBalanceLocked() const
{
    CAmount nTotal = 0;

    {
        LOCK2(cs_main, cs_wallet);
        for (map<uint256, CWalletTx>::const_iterator it = mapWallet.begin(); it != mapWallet.end(); ++it)
        {
            const CWalletTx* pcoin = &(*it).second;
            if (pcoin->IsCTOutput())
                if (pcoin->IsInMainChain())
                    nTotal += pcoin->GetAvailablePrivateCredit(true);
        }
    }

    return nTotal;
}

CAmount CWallet::GetColdStakingBalance() const
{
    CAmount nTotal = 0;
    {
        LOCK2(cs_main, cs_wallet);
        for (map<uint256, CWalletTx>::const_iterator it = mapWallet.begin(); it != mapWallet.end(); ++it)
        {
            const CWalletTx* pcoin = &(*it).second;
            if (pcoin->IsTrusted())
                nTotal += pcoin->GetAvailableStakableCredit();
        }
    }

    return nTotal;
}

CAmount CWallet::GetUnconfirmedBalance() const
{
    CAmount nTotal = 0;
    {
        LOCK2(cs_main, cs_wallet);
        for (map<uint256, CWalletTx>::const_iterator it = mapWallet.begin(); it != mapWallet.end(); ++it)
        {
            const CWalletTx* pcoin = &(*it).second;
            if (!pcoin->IsTrusted() && pcoin->GetDepthInMainChain() == 0 && (pcoin->InMempool() || pcoin->InStempool()))
                nTotal += pcoin->GetAvailableCredit() + pcoin->GetAvailablePrivateCredit();
        }
    }
    return nTotal;
}

CAmount CWallet::GetImmatureBalance() const
{
    CAmount nTotal = 0;
    {
        LOCK2(cs_main, cs_wallet);
        for (map<uint256, CWalletTx>::const_iterator it = mapWallet.begin(); it != mapWallet.end(); ++it)
        {
            const CWalletTx* pcoin = &(*it).second;
            nTotal += pcoin->GetImmatureCredit();
        }
    }
    return nTotal;
}

CAmount CWallet::GetWatchOnlyBalance() const
{
    CAmount nTotal = 0;
    {
        LOCK2(cs_main, cs_wallet);
        for (map<uint256, CWalletTx>::const_iterator it = mapWallet.begin(); it != mapWallet.end(); ++it)
        {
            const CWalletTx* pcoin = &(*it).second;
            if (pcoin->IsTrusted())
                nTotal += pcoin->GetAvailableWatchOnlyCredit();
        }
    }

    return nTotal;
}

CAmount CWallet::GetUnconfirmedWatchOnlyBalance() const
{
    CAmount nTotal = 0;
    {
        LOCK2(cs_main, cs_wallet);
        for (map<uint256, CWalletTx>::const_iterator it = mapWallet.begin(); it != mapWallet.end(); ++it)
        {
            const CWalletTx* pcoin = &(*it).second;
            if (!pcoin->IsTrusted() && pcoin->GetDepthInMainChain() == 0 && (pcoin->InMempool() || pcoin->InStempool()))
                nTotal += pcoin->GetAvailableWatchOnlyCredit();
        }
    }
    return nTotal;
}

CAmount CWallet::GetImmatureWatchOnlyBalance() const
{
    CAmount nTotal = 0;
    {
        LOCK2(cs_main, cs_wallet);
        for (map<uint256, CWalletTx>::const_iterator it = mapWallet.begin(); it != mapWallet.end(); ++it)
        {
            const CWalletTx* pcoin = &(*it).second;
            nTotal += pcoin->GetImmatureWatchOnlyCredit();
        }
    }
    return nTotal;
}

void CWallet::AvailableCoins(vector<COutput>& vCoins, bool fOnlyConfirmed, const CCoinControl *coinControl, bool fIncludeZeroValue, bool fIncludeColdStaking) const
{
    vCoins.clear();

    {
        LOCK2(cs_main, cs_wallet);
        for (map<uint256, CWalletTx>::const_iterator it = mapWallet.begin(); it != mapWallet.end(); ++it)
        {
            const uint256& wtxid = it->first;
            const CWalletTx* pcoin = &(*it).second;

            if (!CheckFinalTx(*pcoin))
                continue;

            if (fOnlyConfirmed && !pcoin->IsTrusted())
                continue;

            if ((pcoin->IsCoinBase() || pcoin->IsCoinStake()) && pcoin->GetBlocksToMaturity() > 0)
                continue;

            int nDepth = pcoin->GetDepthInMainChain();
            if (nDepth < 0)
                continue;

            // We should not consider coins which aren't at least in our mempool
            // It's possible for these to be conflicted via ancestors which we may never be able to detect
            if (nDepth == 0 && !(pcoin->InMempool() || pcoin->InStempool()))
                continue;

            for (unsigned int i = 0; i < pcoin->vout.size(); i++) {
                isminetype mine = IsMine(pcoin->vout[i]);
                if (!(IsSpent(wtxid, i)) && mine != ISMINE_NO &&
                    !IsLockedCoin((*it).first, i) && (pcoin->vout[i].nValue > 0 || fIncludeZeroValue) &&
                    (!coinControl || !coinControl->HasSelected() || coinControl->fAllowOtherInputs || coinControl->IsSelected(COutPoint((*it).first, i))))
                        vCoins.push_back(COutput(pcoin, i, nDepth,
                                                 ((mine & ISMINE_SPENDABLE) != ISMINE_NO) ||
                                                  (coinControl && coinControl->fAllowWatchOnly && (mine & ISMINE_WATCH_SOLVABLE) != ISMINE_NO),
                                                 (mine & (ISMINE_SPENDABLE | ISMINE_WATCH_SOLVABLE | (fIncludeColdStaking ? ISMINE_STAKABLE : ISMINE_NO))) != ISMINE_NO));
            }
        }
    }
}

void CWallet::AvailablePrivateCoins(vector<COutput>& vCoins, bool fOnlyConfirmed, const CCoinControl *coinControl, bool fIncludeZeroValue, CAmount nMinAmount) const
{
    vCoins.clear();

    {
        LOCK2(cs_main, cs_wallet);
        for (map<uint256, CWalletTx>::const_iterator it = mapWallet.begin(); it != mapWallet.end(); ++it)
        {
            const uint256& wtxid = it->first;
            const CWalletTx* pcoin = &(*it).second;

            if (!CheckFinalTx(*pcoin))
                continue;

            if (fOnlyConfirmed && !pcoin->IsTrusted())
                continue;

            if ((pcoin->IsCoinBase() || pcoin->IsCoinStake()) && pcoin->GetBlocksToMaturity() > 0)
                continue;

            int nDepth = pcoin->GetDepthInMainChain();
            if (nDepth < 0)
                continue;

            if (!pcoin->IsCTOutput())
                continue;

            // We should not consider coins which aren't at least in our mempool
            // It's possible for these to be conflicted via ancestors which we may never be able to detect
            if (nDepth == 0 && !pcoin->InMempool())
                continue;

            if (!mapBlockIndex.count(pcoin->hashBlock))
                continue;

            CBlockIndex* pindex = mapBlockIndex[pcoin->hashBlock];
            if (!pindex)
                continue;

            for (unsigned int i = 0; i < pcoin->vout.size(); i++)
            {
                isminetype mine = IsMine(pcoin->vout[i]);

                if (pcoin->vout[i].spendingKey.size() == 0)
                    continue;

                std::string memo = pcoin->vMemos[i];
                CAmount amount = pcoin->vAmounts[i];
                Scalar gamma = pcoin->vGammas[i];

                if (amount < nMinAmount)
                    continue;

                if (!(IsSpent(wtxid, i)) && mine != ISMINE_NO &&
                        !IsLockedCoin((*it).first, i) && (amount > 0 || fIncludeZeroValue) &&
                        (!coinControl || !coinControl->HasSelected() || coinControl->fAllowOtherInputs || coinControl->IsSelected(COutPoint((*it).first, i))))
                    vCoins.push_back(COutput(pcoin, i, nDepth,
                                             ((mine & ISMINE_SPENDABLE_PRIVATE) != ISMINE_NO),
                                             ((mine & ISMINE_SPENDABLE_PRIVATE) != ISMINE_NO),
                                             memo, amount, gamma));
            }
        }
    }
}

static void ApproximateBestSubset(vector<pair<CAmount, pair<const CWalletTx*,unsigned int> > >vValue, const CAmount& nTotalLower, const CAmount& nTargetValue,
                                  vector<char>& vfBest, CAmount& nBest, int iterations = 1000)
{
    vector<char> vfIncluded;

    vfBest.assign(vValue.size(), true);
    nBest = nTotalLower;

    seed_insecure_rand();

    for (int nRep = 0; nRep < iterations && nBest != nTargetValue; nRep++)
    {
        vfIncluded.assign(vValue.size(), false);
        CAmount nTotal = 0;
        bool fReachedTarget = false;
        for (int nPass = 0; nPass < 2 && !fReachedTarget; nPass++)
        {
            for (unsigned int i = 0; i < vValue.size(); i++)
            {
                //The solver here uses a randomized algorithm,
                //the randomness serves no real security purpose but is just
                //needed to prevent degenerate behavior and it is important
                //that the rng is fast. We do not use a constant random sequence,
                //because there may be some privacy improvement by making
                //the selection random.
                if (nPass == 0 ? insecure_rand()&1 : !vfIncluded[i])
                {
                    nTotal += vValue[i].first;
                    vfIncluded[i] = true;
                    if (nTotal >= nTargetValue)
                    {
                        fReachedTarget = true;
                        if (nTotal < nBest)
                        {
                            nBest = nTotal;
                            vfBest = vfIncluded;
                        }
                        nTotal -= vValue[i].first;
                        vfIncluded[i] = false;
                    }
                }
            }
        }
    }
}

bool CWallet::SelectCoinsMinConf(const CAmount& nTargetValue, int nConfMine, int nConfTheirs, vector<COutput> vCoins,
                                 set<pair<const CWalletTx*,unsigned int> >& setCoinsRet, CAmount& nValueRet) const
{
    setCoinsRet.clear();
    nValueRet = 0;

    // List of values less than target
    pair<CAmount, pair<const CWalletTx*,unsigned int> > coinLowestLarger;
    coinLowestLarger.first = std::numeric_limits<CAmount>::max();
    coinLowestLarger.second.first = nullptr;
    vector<pair<CAmount, pair<const CWalletTx*,unsigned int> > > vValue;
    CAmount nTotalLower = 0;

    random_shuffle(vCoins.begin(), vCoins.end(), GetRandInt);

    for(const COutput &output: vCoins)
    {
        if (!output.fSpendable)
            continue;

        const CWalletTx *pcoin = output.tx;

        if (output.nDepth < (pcoin->IsFromMe(ISMINE_ALL) ? nConfMine : nConfTheirs))
            continue;

        int i = output.i;
        CAmount n = pcoin->vout[i].HasRangeProof() ? output.nAmount : pcoin->vout[i].nValue;

        pair<CAmount,pair<const CWalletTx*,unsigned int> > coin = make_pair(n,make_pair(pcoin, i));

        if (n == nTargetValue)
        {
            setCoinsRet.insert(coin.second);
            nValueRet += coin.first;
            return true;
        }
        else if (n < nTargetValue + MIN_CHANGE)
        {
            vValue.push_back(coin);
            nTotalLower += n;
        }
        else if (n < coinLowestLarger.first)
        {
            coinLowestLarger = coin;
        }
    }

    if (nTotalLower == nTargetValue)
    {
        for (unsigned int i = 0; i < vValue.size(); ++i)
        {
            setCoinsRet.insert(vValue[i].second);
            nValueRet += vValue[i].first;
        }
        return true;
    }

    if (nTotalLower < nTargetValue)
    {
        if (coinLowestLarger.second.first == nullptr)
            return false;
        setCoinsRet.insert(coinLowestLarger.second);
        nValueRet += coinLowestLarger.first;
        return true;
    }

    // Solve subset sum by stochastic approximation
    std::sort(vValue.begin(), vValue.end(), CompareValueOnly());
    std::reverse(vValue.begin(), vValue.end());
    vector<char> vfBest;
    CAmount nBest;

    ApproximateBestSubset(vValue, nTotalLower, nTargetValue, vfBest, nBest);
    if (nBest != nTargetValue && nTotalLower >= nTargetValue + MIN_CHANGE)
        ApproximateBestSubset(vValue, nTotalLower, nTargetValue + MIN_CHANGE, vfBest, nBest);

    // If we have a bigger coin and (either the stochastic approximation didn't find a good solution,
    //                                   or the next bigger coin is closer), return the bigger coin
    if (coinLowestLarger.second.first &&
        ((nBest != nTargetValue && nBest < nTargetValue + MIN_CHANGE) || coinLowestLarger.first <= nBest))
    {
        setCoinsRet.insert(coinLowestLarger.second);
        nValueRet += coinLowestLarger.first;
    }
    else {
        for (unsigned int i = 0; i < vValue.size(); i++)
            if (vfBest[i])
            {
                setCoinsRet.insert(vValue[i].second);
                nValueRet += vValue[i].first;
            }

        LogPrint("selectcoins", "SelectCoins() best subset: ");
        for (unsigned int i = 0; i < vValue.size(); i++)
            if (vfBest[i])
                LogPrint("selectcoins", "%s ", FormatMoney(vValue[i].first));
        LogPrint("selectcoins", "total %s\n", FormatMoney(nBest));
    }

    return true;
}

bool CWallet::SelectCoins(const vector<COutput>& vAvailableCoins, const CAmount& nTargetValue, set<pair<const CWalletTx*,unsigned int> >& setCoinsRet, CAmount& nValueRet, const CCoinControl* coinControl) const
{
    vector<COutput> vCoins(vAvailableCoins);

    // coin control -> return all selected outputs (we want all selected to go into the transaction for sure)
    if (coinControl && coinControl->HasSelected() && !coinControl->fAllowOtherInputs)
    {
        for(const COutput& out: vCoins)
        {
            if (!out.fSpendable)
                 continue;
            nValueRet += out.tx->vout[out.i].HasRangeProof() ? out.nAmount : out.tx->vout[out.i].nValue;
            setCoinsRet.insert(make_pair(out.tx, out.i));
        }
        return (nValueRet >= nTargetValue);
    }

    // calculate value from preset inputs and store them
    set<pair<const CWalletTx*, uint32_t> > setPresetCoins;
    CAmount nValueFromPresetInputs = 0;

    std::vector<COutPoint> vPresetInputs;
    if (coinControl)
        coinControl->ListSelected(vPresetInputs);
    for(const COutPoint& outpoint: vPresetInputs)
    {
        map<uint256, CWalletTx>::const_iterator it = mapWallet.find(outpoint.hash);
        if (it != mapWallet.end())
        {
            const CWalletTx* pcoin = &it->second;
            // Clearly invalid input, fail
            if (pcoin->vout.size() <= outpoint.n)
                return false;
            nValueFromPresetInputs += pcoin->vout[outpoint.n].nValue;
            setPresetCoins.insert(make_pair(pcoin, outpoint.n));
        } else
            return false; // TODO: Allow non-wallet inputs
    }

    // remove preset inputs from vCoins
    for (vector<COutput>::iterator it = vCoins.begin(); it != vCoins.end() && coinControl && coinControl->HasSelected();)
    {
        if (setPresetCoins.count(make_pair(it->tx, it->i)))
            it = vCoins.erase(it);
        else
            ++it;
    }

    bool res = nTargetValue <= nValueFromPresetInputs ||
        SelectCoinsMinConf(nTargetValue - nValueFromPresetInputs, 1, 6, vCoins, setCoinsRet, nValueRet) ||
        SelectCoinsMinConf(nTargetValue - nValueFromPresetInputs, 1, 1, vCoins, setCoinsRet, nValueRet) ||
        (bSpendZeroConfChange && SelectCoinsMinConf(nTargetValue - nValueFromPresetInputs, 0, 1, vCoins, setCoinsRet, nValueRet));

    // because SelectCoinsMinConf clears the setCoinsRet, we now add the possible inputs to the coinset
    setCoinsRet.insert(setPresetCoins.begin(), setPresetCoins.end());

    // add preset inputs to the total value selected
    nValueRet += nValueFromPresetInputs;

    return res;
}

bool CWallet::FundTransaction(CMutableTransaction& tx, CAmount& nFeeRet, bool overrideEstimatedFeeRate, const CFeeRate& specificFeeRate, int& nChangePosInOut, std::string& strFailReason, bool includeWatching, bool lockUnspents, const CTxDestination& destChange, bool fPrivate)
{
    vector<CRecipient> vecSend;

    std::vector<shared_ptr<CReserveBLSCTBlindingKey>> reserveBLSCTKey;

    bool fBLSCT = false;

    // Turn the txout set into a CRecipient vector
    for(const CTxOut& txOut: tx.vout)
    {
        CRecipient recipient = {txOut.scriptPubKey, txOut.nValue, false, txOut.spendingKey.size() > 0};
        fBLSCT |= txOut.spendingKey.size() > 0;
        vecSend.push_back(recipient);
    }

    if (fBLSCT || fPrivate)
    {
        for (unsigned int i = 0; i < vecSend.size()+2; i++)
        {
            shared_ptr<CReserveBLSCTBlindingKey> rk(new CReserveBLSCTBlindingKey(this));
            reserveBLSCTKey.insert(reserveBLSCTKey.begin(), std::move(rk));
        }
    }

    CCoinControl coinControl;
    coinControl.destChange = destChange;
    coinControl.fAllowOtherInputs = true;
    coinControl.fAllowWatchOnly = includeWatching;
    coinControl.fOverrideFeeRate = overrideEstimatedFeeRate;
    coinControl.nFeeRate = specificFeeRate;

    for(const CTxIn& txin: tx.vin)
        coinControl.Select(txin.prevout);

    CReserveKey reservekey(this);
    CWalletTx wtx;
    if (!CreateTransaction(vecSend, wtx, reservekey, reserveBLSCTKey, nFeeRet, nChangePosInOut, strFailReason, fPrivate, &coinControl, false))
        return false;

    if (nChangePosInOut != -1)
        tx.vout.insert(tx.vout.begin() + nChangePosInOut, wtx.vout[nChangePosInOut]);

    // Add new txins (keeping original txin scriptSig/order)
    for(const CTxIn& txin: wtx.vin)
    {
        if (!coinControl.IsSelected(txin.prevout))
        {
            tx.vin.push_back(txin);

            if (lockUnspents)
            {
              LOCK2(cs_main, cs_wallet);
              LockCoin(txin.prevout);
            }
        }
    }

    return true;
}

bool CWallet::CreateTransaction(const vector<CRecipient>& vecSend, CWalletTx& wtxNew, CReserveKey& reservekey,
                                std::vector<shared_ptr<CReserveBLSCTBlindingKey>>& reserveBLSCTKey, CAmount& nFeeRet,
                                int& nChangePosInOut, std::string& strFailReason, bool fPrivate, const CCoinControl* coinControl,
                                bool sign, const CandidateTransaction* coinsToMix, uint64_t nBLSCTAccount)
{
    CAmount nValue = 0;
    int nChangePosRequest = nChangePosInOut;
    unsigned int nSubtractFeeFromAmount = 0;
    bool fBLSCT = false;

    for(const CRecipient& recipient: vecSend)
    {
        if (nValue < 0 || recipient.nAmount < 0)
        {
            strFailReason = _("Transaction amounts must be positive");
            return false;
        }
        nValue += recipient.nAmount;

        if (recipient.fSubtractFeeFromAmount)
            nSubtractFeeFromAmount++;

        if (recipient.fBLSCT)
            fBLSCT = true;
    }
    if (vecSend.empty() || nValue < 0)
    {
        strFailReason = _("Transaction amounts must be positive");
        return false;
    }

    if ((fBLSCT || fPrivate) && reserveBLSCTKey.size() != vecSend.size() + 2)
    {
        strFailReason = _("Wrong size of reserved keys for private transaction");
        return false;
    }

    wtxNew.fTimeReceivedIsTxTime = true;
    wtxNew.nTime = GetAdjustedTime();
    wtxNew.BindWallet(this);

    CMutableTransaction txNew;

    txNew.strDZeel = wtxNew.strDZeel; // Use it baby!

    txNew.nVersion = IsCommunityFundEnabled(chainActive.Tip(),Params().GetConsensus()) ? CTransaction::TXDZEEL_VERSION_V2 : CTransaction::TXDZEEL_VERSION;

    if(wtxNew.nCustomVersion > 0) txNew.nVersion = wtxNew.nCustomVersion;

    if (fBLSCT) {
        txNew.nVersion |= TX_BLS_CT_FLAG;
    }

    CAmount nMixFee = 0;

    if (fPrivate) {
        txNew.nVersion |= TX_BLS_INPUT_FLAG;
        nMixFee = (coinsToMix ? coinsToMix->fee : 0);
    }

    auto nInMix = (coinsToMix ? coinsToMix->tx.vin.size() : 0);
    auto nOutMix = (coinsToMix ? coinsToMix->tx.vout.size() : 0);

    // Discourage fee sniping.
    //
    // For a large miner the value of the transactions in the best block and
    // the mempool can exceed the cost of deliberately attempting to mine two
    // blocks to orphan the current best block. By setting nLockTime such that
    // only the next block can include the transaction, we discourage this
    // practice as the height restricted and limited blocksize gives miners
    // considering fee sniping fewer options for pulling off this attack.
    //
    // A simple way to think about this is from the wallet's point of view we
    // always want the blockchain to move forward. By setting nLockTime this
    // way we're basically making the statement that we only want this
    // transaction to appear in the next block; we don't want to potentially
    // encourage reorgs by allowing transactions to appear at lower heights
    // than the next block in forks of the best chain.
    //
    // Of course, the subsidy is high enough, and transaction volume low
    // enough, that fee sniping isn't a problem yet, but by implementing a fix
    // now we ensure code won't be written that makes assumptions about
    // nLockTime that preclude a fix later.
    txNew.nLockTime = chainActive.Height();
    txNew.nTime = GetAdjustedTime();

    // Secondly occasionally randomly pick a nLockTime even further back, so
    // that transactions that are delayed after signing for whatever reason,
    // e.g. high-latency mix networks and some CoinJoin implementations, have
    // better privacy.
    if (GetRandInt(10) == 0)
        txNew.nLockTime = std::max(0, (int)txNew.nLockTime - GetRandInt(100));

    assert(txNew.nLockTime <= (unsigned int)chainActive.Height());
    assert(txNew.nLockTime < LOCKTIME_THRESHOLD);

    {
        LOCK2(cs_main, cs_wallet);
        {
            std::vector<COutput> vAvailableCoins;

            if(fPrivate)
                AvailablePrivateCoins(vAvailableCoins, true, coinControl);
            else
                AvailableCoins(vAvailableCoins, true, coinControl);

            nFeeRet = nMixFee;
            // Start with no fee and loop until there is enough fee
            while (true)
            {
                nChangePosInOut = nChangePosRequest;
                txNew.vin.clear();
                txNew.vout.clear();
                wtxNew.fFromMe = true;
                bool fFirst = true;

                Scalar gammaIns = 0;
                Scalar gammaOuts = 0;
                std::vector<bls::G2Element> vBLSSignatures;

                CAmount nValueToSelect = nValue;
                if (nSubtractFeeFromAmount == 0)
                    nValueToSelect += nFeeRet;
                double dPriority = 0;
                unsigned int i = 0;
                if (fBLSCT)
                    uiInterface.ShowProgress("Constructing BLSCT transaction...", -1);

                // vouts to the payees
                for(const CRecipient& recipient: vecSend)
                {
                    CTxOut txout(recipient.fBLSCT ? 0 : recipient.nAmount, recipient.scriptPubKey);

                    if(recipient.scriptPubKey.IsCommunityFundContribution())
                        wtxNew.fCFund = true;

                    CAmount nValue = recipient.nAmount;

                    if (recipient.fSubtractFeeFromAmount)
                    {
                        nValue -= nFeeRet / nSubtractFeeFromAmount; // Subtract fee equally from each selected recipient

                        if (fFirst) // first receiver pays the remainder not divisible by output count
                        {
                            fFirst = false;
                            nValue -= nFeeRet % nSubtractFeeFromAmount;
                        }
                    }

                    if (!recipient.fBLSCT)
                    {
                        txout.nValue = nValue;

                        if (txout.IsDust(::minRelayTxFee))
                        {
                            if (recipient.fSubtractFeeFromAmount && nFeeRet > 0)
                            {
                                if (txout.nValue < 0)
                                    strFailReason = _("The transaction amount is too small to pay the fee");
                                else
                                    strFailReason = _("The transaction amount is too small to send after the fee has been deducted");
                            }
                            else
                                strFailReason = _("Transaction amount too small");
                            return false;
                        }

                        if(fPrivate)
                        {
                            blsctPublicKey pk;
                            blsctKey bk;

                            reserveBLSCTKey[2+i]->GetReservedKey(pk);

                            if (!GetBLSCTBlindingKey(pk, bk))
                            {
                                strFailReason = _("Could not get private key from blsct pool.");
                                return false;
                            }
                            bls::PrivateKey ephemeralKey = bk.GetKey();
                            SignBLSOutput(ephemeralKey, txout, vBLSSignatures);
                        }
                    }
                    else
                    {

                        blsctPublicKey pk;
                        blsctKey bk;

                        reserveBLSCTKey[2+i]->GetReservedKey(pk);

                        if (!GetBLSCTBlindingKey(pk, bk))
                        {
                            strFailReason = _("Could not get private key from blsct pool.");
                            return false;
                        }

                        bls::PrivateKey ephemeralKey = bk.GetKey();

                        if (!CreateBLSCTOutput(ephemeralKey, txout, blsctDoublePublicKey(recipient.vk, recipient.sk), nValue, recipient.sMemo, gammaOuts, strFailReason, fPrivate, vBLSSignatures))
                        {
                            uiInterface.ShowProgress("Constructing BLSCT transaction...", 100);
                            return false;
                        }

                    }

                    txNew.vout.push_back(txout);
                    i++;
                }

                if (fBLSCT)
                    uiInterface.ShowProgress("Constructing BLSCT transaction...", 100);

                // Choose coins to use
                set<pair<const CWalletTx*,unsigned int> > setCoins;
                CAmount nValueIn = 0;

                if (!SelectCoins(vAvailableCoins, nValueToSelect, setCoins, nValueIn, coinControl))
                {
                    strFailReason = _("Insufficient funds");
                    return false;
                }

                for(PAIRTYPE(const CWalletTx*, unsigned int) pcoin: setCoins)
                {
                    CAmount nCredit = pcoin.first->vout[pcoin.second].nValue;
                    if(pcoin.first->vout[pcoin.second].scriptPubKey.IsColdStaking() || pcoin.first->vout[pcoin.second].scriptPubKey.IsColdStakingv2())
                        wtxNew.fSpendsColdStaking = true;
                    //The coin age after the next block (depth+1) is used instead of the current,
                    //reflecting an assumption the user would accept a bit more delay for
                    //a chance at a free transaction.
                    //But mempool inputs might still be in the mempool, so their age stays 0
                    int age = pcoin.first->GetDepthInMainChain();
                    assert(age >= 0);
                    if (age != 0)
                        age += 1;
                    dPriority += (double)nCredit * age;
                }

                const CAmount nChange = nValueIn - nValueToSelect;

                if (nChange > 0)
                {
                    // Fill a vout to ourself
                    // TODO: pass in scriptChange instead of reservekey so
                    // change transaction isn't always pay-to-navcoin-address
                    CScript scriptChange;

                    if (fPrivate)
                    {
                        CTxOut newTxOut(0, CScript());

                        blsctPublicKey pk;
                        blsctKey bk;

                        reserveBLSCTKey[1]->GetReservedKey(pk);

                        if (!GetBLSCTBlindingKey(pk, bk))
                        {
                            strFailReason = _("Could not get private key from blsct pool.");
                            return false;
                        }
                        bls::PrivateKey ephemeralKey = bk.GetKey();

                        blsctDoublePublicKey k;

                        if (!pwalletMain->GetBLSCTSubAddressPublicKeys(std::make_pair(nBLSCTAccount, 0), k))
                        {
                            strFailReason = _("BLSCT not supported in your wallet");
                            return false;
                        }

                        uiInterface.ShowProgress("Constructing BLSCT transaction...", -1);

                        if (!CreateBLSCTOutput(ephemeralKey, newTxOut, k, nChange, "Change", gammaOuts, strFailReason, fPrivate, vBLSSignatures))
                        {
                            uiInterface.ShowProgress("Constructing BLSCT transaction...", 100);
                            strFailReason = _("Error creating BLSCT change output");
                            return false;
                        }

                        uiInterface.ShowProgress("Constructing BLSCT transaction...", 100);

                        txNew.nVersion |= TX_BLS_CT_FLAG;
                        txNew.vout.push_back(newTxOut);
                    }
                    else
                    {

                        // coin control: send change to custom address
                        if (coinControl && !boost::get<CNoDestination>(&coinControl->destChange))
                            scriptChange = GetScriptForDestination(coinControl->destChange);

                        // no coin control: send change to newly generated address
                        else
                        {
                            // Note: We use a new key here to keep it from being obvious which side is the change.
                            //  The drawback is that by not reusing a previous key, the change may be lost if a
                            //  backup is restored, if the backup doesn't have the new private key for the change.
                            //  If we reused the old key, it would be possible to add code to look for and
                            //  rediscover unknown transactions that were written with keys of ours to recover
                            //  post-backup change.

                            // Reserve a new key pair from key pool
                            CPubKey vchPubKey;
                            bool ret;
                            ret = reservekey.GetReservedKey(vchPubKey);
                            assert(ret); // should never fail, as we just unlocked

                            scriptChange = GetScriptForDestination(vchPubKey.GetID());
                        }

                        CTxOut newTxOut(nChange, scriptChange);

                        // We do not move dust-change to fees, because the sender would end up paying more than requested.
                        // This would be against the purpose of the all-inclusive feature.
                        // So instead we raise the change and deduct from the recipient.
                        if (nSubtractFeeFromAmount > 0 && newTxOut.IsDust(::minRelayTxFee))
                        {
                            CAmount nDust = newTxOut.GetDustThreshold(::minRelayTxFee) - newTxOut.nValue;
                            newTxOut.nValue += nDust; // raise change until no more dust
                            for (unsigned int i = 0; i < vecSend.size(); i++) // subtract from first recipient
                            {
                                if (vecSend[i].fSubtractFeeFromAmount)
                                {
                                    txNew.vout[i].nValue -= nDust;
                                    if (txNew.vout[i].IsDust(::minRelayTxFee))
                                    {
                                        strFailReason = _("The transaction amount is too small to send after the fee has been deducted");
                                        return false;
                                    }
                                    break;
                                }
                            }
                        }

                        // Never create dust outputs; if we would, just
                        // add the dust to the fee.
                        if (newTxOut.IsDust(::minRelayTxFee))
                        {
                            nChangePosInOut = -1;
                            nFeeRet += nChange;
                            reservekey.ReturnKey();
                        }
                        else
                        {
                            if (nChangePosInOut == -1)
                            {
                                // Insert change txn at random position:
                                nChangePosInOut = GetRandInt(txNew.vout.size()+1);
                            }
                            else if ((unsigned int)nChangePosInOut > txNew.vout.size())
                            {
                                strFailReason = _("Change index out of range");
                                return false;
                            }

                            vector<CTxOut>::iterator position = txNew.vout.begin()+nChangePosInOut;
                            txNew.vout.insert(position, newTxOut);
                        }
                    }
                }
                else
                    reservekey.ReturnKey();

                if (fBLSCT || fPrivate)
                {
                    CTxOut feeOut = CTxOut(nFeeRet-nMixFee, CScript(OP_RETURN));
                    txNew.vout.push_back(feeOut);
                }

                // Fill vin
                //
                // Note how the sequence number is set to max()-1 so that the
                // nLockTime set above actually works.
                for(const PAIRTYPE(const CWalletTx*,unsigned int)& coin: setCoins)
                {
                    if(coin.first->nTime > txNew.nTime)
                        txNew.nTime = coin.first->nTime;
                    txNew.vin.push_back(CTxIn(coin.first->GetHash(),coin.second,CScript(),
                                              std::numeric_limits<unsigned int>::max()-1));
                    if (fPrivate)
                    {
                        gammaIns = gammaIns + coin.first->vGammas[coin.second];
                    }
                }

                // Sign
                int nIn = 0;

                if (fPrivate)
                     uiInterface.ShowProgress(_("Signing..."), -1);

                if (fPrivate || fBLSCT)
                {
                    Scalar diff = gammaIns-gammaOuts;
                    try
                    {
                        bls::PrivateKey balanceSigningKey = bls::PrivateKey::FromBN(diff.bn);
                        txNew.vchBalanceSig = bls::BasicSchemeMPL::Sign(balanceSigningKey, balanceMsg);
                    }
                    catch(...)
                    {
                        strFailReason = _("Catched balance signing key exception");
                        return false;
                    }
                }

                CTransaction txNewConst(txNew);
                for(const PAIRTYPE(const CWalletTx*,unsigned int)& coin: setCoins)
                {
                    bool signSuccess = false;
                    const CScript& scriptPubKey = coin.first->vout[coin.second].scriptPubKey;
                    SignatureData sigdata;

                    if (sign)
                    {
                        if (!fPrivate)
                        {
                            signSuccess = ProduceSignature(TransactionSignatureCreator(this, &txNewConst, nIn, coin.first->vout[coin.second].nValue, SIGHASH_ALL), scriptPubKey, sigdata);
                        }
                        else
                        {
                            blsctKey s;

                            if (!pwalletMain->GetBLSCTSubAddressSpendingKeyForOutput(coin.first->vout[coin.second].outputKey, coin.first->vout[coin.second].spendingKey, s))
                            {
                                strFailReason = _("BLSCT keys not available");
                                if (fPrivate)
                                     uiInterface.ShowProgress(_("Signing..."), 100);
                                return false;
                            }

                            if (coin.first->vout[coin.second].ephemeralKey.size() == 0)
                            {
                                strFailReason = _("Empty ephemeral key");
                                return false;
                            }
                            try
                            {

                                if (!(s.GetG1Element() == bls::G1Element::FromBytes(coin.first->vout[coin.second].spendingKey.data())))
                                {
                                    strFailReason = _("Failed to recover signing key");
                                    if (fPrivate)
                                        uiInterface.ShowProgress(_("Signing..."), 100);
                                    return false;
                                }

                                SignBLSInput(s.GetKey(), txNewConst.vin[nIn], vBLSSignatures);
                            }
                            catch(...)
                            {
                                strFailReason = _("Catched exception");
                                return false;
                            }

                            signSuccess = true;
                        }
                    }
                    else
                        signSuccess = ProduceSignature(DummySignatureCreator(this), scriptPubKey, sigdata);

                    if (!signSuccess)
                    {
                        strFailReason = _("Signing transaction failed");
                        if (fPrivate)
                             uiInterface.ShowProgress(_("Signing..."), 100);
                        return false;
                    } else {
                        UpdateTransaction(txNew, nIn, sigdata);
                    }

                    nIn++;
                }

                if (fPrivate)
                {
                    txNew.vchTxSig = bls::BasicSchemeMPL::Aggregate(vBLSSignatures).Serialize();
                    uiInterface.ShowProgress(_("Signing..."), 100);
                }

                unsigned int nBytes = GetVirtualTransactionSize(txNew);

                // Remove scriptSigs if we used dummy signatures for fee calculation
                if (!sign) {
                    for(CTxIn& vin: txNew.vin)
                        vin.scriptSig = CScript();
                    txNew.wit.SetNull();
                }
                else if (fPrivate || fBLSCT)
                {
                    CValidationState state;
                    std::vector<RangeproofEncodedData> blsctData;

                    CStateViewCache inputs(pcoinsTip);
                    blsctKey v;

                    try
                    {
                        if (!(coinsToMix && coinsToMix->tx.vin.size() > 0) && !VerifyBLSCT(txNew, bls::PrivateKey::FromBN(Scalar::Rand().bn), blsctData, inputs, state, false))
                        {
                            strFailReason = FormatStateMessage(state);
                            return false;
                        }
                    }
                    catch(...)
                    {
                        strFailReason = _("Catched mixing exception");
                        return false;
                    }

                }

                // Embed the constructed transaction data in wtxNew.
                *static_cast<CTransaction*>(&wtxNew) = CTransaction(txNew);

                // Limit size
                if (GetTransactionWeight(txNew) >= MAX_STANDARD_TX_WEIGHT)
                {
                    strFailReason = _("Transaction too large");
                    return false;
                }

                dPriority = wtxNew.ComputePriority(dPriority, nBytes);

                // Can we complete this as a free transaction?
                if (fSendFreeTransactions && nBytes <= MAX_FREE_TRANSACTION_CREATE_SIZE)
                {
                    // Not enough fee: enough priority?
                    double dPriorityNeeded = mempool.estimateSmartPriority(nTxConfirmTarget);
                    // Require at least hard-coded AllowFree.
                    if (dPriority >= dPriorityNeeded && AllowFree(dPriority))
                        break;
                }

                CAmount nFeeNeeded = GetMinimumFee(nBytes, nTxConfirmTarget, mempool);

                if (fPrivate)
                    nFeeNeeded = (wtxNew.vin.size()+nInMix)*BLSCT_TX_INPUT_FEE+(wtxNew.vout.size()+nOutMix)*BLSCT_TX_OUTPUT_FEE;
                else
                    nFeeNeeded = nFeeNeeded > 10000 ? nFeeNeeded : 10000;

                if (coinControl && nFeeNeeded > 0 && coinControl->nMinimumTotalFee > nFeeNeeded) {
                    nFeeNeeded = coinControl->nMinimumTotalFee;
                }
                if (!fPrivate && coinControl && coinControl->fOverrideFeeRate)
                    nFeeNeeded = coinControl->nFeeRate.GetFee(nBytes);

                // If we made it here and we aren't even able to meet the relay fee on the next pass, give up
                // because we must be at the maximum allowed fee.
                if (nFeeNeeded < ::minRelayTxFee.GetFee(nBytes))
                {
                    strFailReason = _("Transaction too large for fee policy");
                    return false;
                }

                if (nFeeRet-nMixFee >= nFeeNeeded)
                    break; // Done, enough fee included.

                // Include more fee and try again.
                nFeeRet = nFeeNeeded+nMixFee;
                continue;
            }
        }
    }

    if (fPrivate && coinsToMix && coinsToMix->tx.vin.size() > 0)
    {
        std::vector<CTransaction> vTransactionsToCombine;
        vTransactionsToCombine.push_back(coinsToMix->tx);
        vTransactionsToCombine.push_back(wtxNew);

        CTransaction ctx;
        CValidationState state;

        if (!CombineBLSCTTransactions(vTransactionsToCombine, ctx, *pcoinsTip, state))
            return error("CWallet::%s: Failed %s\n", __func__, state.GetRejectReason());

        *static_cast<CTransaction*>(&wtxNew) = CTransaction(ctx);
    }

    return true;
}

/**
 * Call after CreateTransaction unless you want to abort
 */
bool CWallet::CommitTransaction(CWalletTx& wtxNew, CReserveKey& reservekey, std::vector<shared_ptr<CReserveBLSCTBlindingKey>>& reserveBLSCTKey)
{
    {
        LOCK2(cs_main, cs_wallet);
        LogPrintf("CommitTransaction:\n%s", wtxNew.ToString());
        {
            // This is only to keep the database open to defeat the auto-flush for the
            // duration of this scope.  This is the only place where this optimization
            // maybe makes sense; please don't do it anywhere else.
            CWalletDB* pwalletdb = fFileBacked ? new CWalletDB(strWalletFile,"r+") : nullptr;

            // Take key pair from key pool so it won't be used again
            if (wtxNew.IsBLSCT())
            {
                for (auto &it: reserveBLSCTKey)
                    it->KeepKey();
            }
            else
            {
                reservekey.KeepKey();
            }

            // Add tx to wallet, because if it has change it's also ours,
            // otherwise just for transaction history.
            AddToWallet(wtxNew, false, pwalletdb);

            // Notify that old coins are spent
            set<CWalletTx*> setCoins;
            for(const CTxIn& txin: wtxNew.vin)
            {
                CWalletTx &coin = mapWallet[txin.prevout.hash];
                coin.BindWallet(this);
                NotifyTransactionChanged(this, coin.GetHash(), CT_UPDATED);
            }

            if (fFileBacked)
                delete pwalletdb;
        }

        // Track how many getdata requests our transaction gets
        mapRequestCount[wtxNew.GetHash()] = 0;

        if (fBroadcastTransactions)
        {
            // Broadcast
            if (!wtxNew.AcceptToMemoryPool(false, maxTxFee))
            {
                // This must not fail. The transaction has already been signed and recorded.
                mapWallet[wtxNew.GetHash()].MarkDirty();
                LogPrintf("CommitTransaction(): Error: Transaction not valid\n");
                return false;
            }
            wtxNew.RelayWalletTransaction();
        }
    }
    return true;
}

bool CWallet::AddAccountingEntry(const CAccountingEntry& acentry, CWalletDB & pwalletdb)
{
    if (!pwalletdb.WriteAccountingEntry_Backend(acentry))
        return false;

    laccentries.push_back(acentry);
    CAccountingEntry & entry = laccentries.back();
    wtxOrdered.insert(make_pair(entry.nOrderPos, TxPair((CWalletTx*)0, &entry)));

    return true;
}

CAmount CWallet::GetRequiredFee(unsigned int nTxBytes)
{
    return std::max(minTxFee.GetFee(nTxBytes), ::minRelayTxFee.GetFee(nTxBytes));
}

CAmount CWallet::GetMinimumFee(unsigned int nTxBytes, unsigned int nConfirmTarget, const CTxMemPool& pool)
{
     // payTxFee is user-set "I want to pay this much"
     CAmount nFeeNeeded = payTxFee.GetFee(nTxBytes);
     // User didn't set: use -txconfirmtarget to estimate...
     if (nFeeNeeded == 0) {
         int estimateFoundTarget = nConfirmTarget;
         nFeeNeeded = pool.estimateSmartFee(nConfirmTarget, &estimateFoundTarget).GetFee(nTxBytes);
         // ... unless we don't have enough mempool data for estimatefee, then use fallbackFee
         if (nFeeNeeded == 0)
             nFeeNeeded = fallbackFee.GetFee(nTxBytes);
     }
     // prevent user from paying a fee below minRelayTxFee or minTxFee
     nFeeNeeded = std::max(nFeeNeeded, GetRequiredFee(nTxBytes));
     // But always obey the maximum
     if (nFeeNeeded > maxTxFee)
         nFeeNeeded = maxTxFee;
    return nFeeNeeded;
}




DBErrors CWallet::LoadWallet(bool& fFirstRunRet, bool& fFirstBLSCTRunRet)
{
    if (!fFileBacked)
        return DB_LOAD_OK;
    fFirstRunRet = false;
    fFirstBLSCTRunRet = false;
    DBErrors nLoadWalletRet = CWalletDB(strWalletFile,"cr+").LoadWallet(this);
    if (nLoadWalletRet == DB_NEED_REWRITE)
    {
        if (CDB::Rewrite(strWalletFile, "\x04pool"))
        {
            LOCK(cs_wallet);
            setKeyPool.clear();
            // Note: can't top-up keypool here, because wallet is locked.
            // User will be prompted to unlock wallet the next operation
            // that requires a new key.
        }
    }

    if (nLoadWalletRet != DB_LOAD_OK)
        return nLoadWalletRet;
    fFirstRunRet = !vchDefaultKey.IsValid();

    fFirstBLSCTRunRet = !publicBlsKey.IsValid();

    uiInterface.LoadWallet(this);

    return DB_LOAD_OK;
}

bool CWallet::SetBLSCTKeys(const bls::PrivateKey& v, const bls::PrivateKey& s, const bls::PrivateKey& b)
{
    SetMinVersion(FEATURE_BLSCT);

    if (!this->SetBLSCTDoublePublicKey(blsctDoublePublicKey(v.GetG1Element(), s.GetG1Element())))
        return false;

    if (!this->SetBLSCTViewKey(blsctKey(v)))
        return false;

    if (!this->SetBLSCTSpendKey(blsctKey(s)))
        return false;

    if (!this->SetBLSCTBlindingMasterKey(blsctKey(b)))
        return false;

    if (fFileBacked)
    {
        if (!CWalletDB(strWalletFile).WriteBLSCTKey(this))
            return false;
    }
    return true;
}

DBErrors CWallet::ZapSelectTx(vector<uint256>& vHashIn, vector<uint256>& vHashOut)
{
    if (!fFileBacked)
        return DB_LOAD_OK;
    DBErrors nZapSelectTxRet = CWalletDB(strWalletFile,"cr+").ZapSelectTx(this, vHashIn, vHashOut);
    if (nZapSelectTxRet == DB_NEED_REWRITE)
    {
        if (CDB::Rewrite(strWalletFile, "\x04pool"))
        {
            LOCK(cs_wallet);
            setKeyPool.clear();
            // Note: can't top-up keypool here, because wallet is locked.
            // User will be prompted to unlock wallet the next operation
            // that requires a new key.
        }
    }

    if (nZapSelectTxRet != DB_LOAD_OK)
        return nZapSelectTxRet;

    MarkDirty();

    return DB_LOAD_OK;

}

DBErrors CWallet::ZapWalletTx(std::vector<CWalletTx>& vWtx)
{
    if (!fFileBacked)
        return DB_LOAD_OK;
    DBErrors nZapWalletTxRet = CWalletDB(strWalletFile,"cr+").ZapWalletTx(this, vWtx);
    if (nZapWalletTxRet == DB_NEED_REWRITE)
    {
        if (CDB::Rewrite(strWalletFile, "\x04pool"))
        {
            LOCK(cs_wallet);
            setKeyPool.clear();
            // Note: can't top-up keypool here, because wallet is locked.
            // User will be prompted to unlock wallet the next operation
            // that requires a new key.
        }
    }

    if (nZapWalletTxRet != DB_LOAD_OK)
        return nZapWalletTxRet;

    return DB_LOAD_OK;
}


bool CWallet::SetAddressBook(const CTxDestination& address, const string& strName, const string& strPurpose)
{
    bool fUpdated = false;
    {
        LOCK(cs_wallet); // mapAddressBook
        std::map<CTxDestination, CAddressBookData>::iterator mi = mapAddressBook.find(address);
        fUpdated = mi != mapAddressBook.end();
        mapAddressBook[address].name = strName;
        if (!strPurpose.empty()) /* update purpose only if requested */
            mapAddressBook[address].purpose = strPurpose;
    }
    NotifyAddressBookChanged(this, address, strName, ::IsMine(*this, address) != ISMINE_NO,
                             strPurpose, (fUpdated ? CT_UPDATED : CT_NEW) );
    if (!fFileBacked)
        return false;
    if (!strPurpose.empty() && !CWalletDB(strWalletFile).WritePurpose(CNavCoinAddress(address).ToString(), strPurpose))
        return false;
    return CWalletDB(strWalletFile).WriteName(CNavCoinAddress(address).ToString(), strName);
}

bool CWallet::DelAddressBook(const CTxDestination& address)
{
    {
        LOCK(cs_wallet); // mapAddressBook

        if(fFileBacked)
        {
            // Delete destdata tuples associated with address
            std::string strAddress = CNavCoinAddress(address).ToString();
            for(const PAIRTYPE(string, string) &item: mapAddressBook[address].destdata)
            {
                CWalletDB(strWalletFile).EraseDestData(strAddress, item.first);
            }
        }
        mapAddressBook.erase(address);
    }

    NotifyAddressBookChanged(this, address, "", ::IsMine(*this, address) != ISMINE_NO, "", CT_DELETED);

    if (!fFileBacked)
        return false;
    CWalletDB(strWalletFile).ErasePurpose(CNavCoinAddress(address).ToString());
    return CWalletDB(strWalletFile).EraseName(CNavCoinAddress(address).ToString());
}

bool CWallet::SetDefaultKey(const CPubKey &vchPubKey)
{
    if (fFileBacked)
    {
        if (!CWalletDB(strWalletFile).WriteDefaultKey(vchPubKey))
            return false;
    }
    vchDefaultKey = vchPubKey;
    return true;
}

/**
 * Mark old keypool keys as used,
 * and generate all new keys
 */
bool CWallet::NewKeyPool()
{
    {
        LOCK(cs_wallet);
        CWalletDB walletdb(strWalletFile);
        for(int64_t nIndex: setKeyPool)
            walletdb.ErasePool(nIndex);
        setKeyPool.clear();

        if (IsLocked())
            return false;

        int64_t nKeys = max(GetArg("-keypool", DEFAULT_KEYPOOL_SIZE), (int64_t)0);
        for (int i = 0; i < nKeys; i++)
        {
            int64_t nIndex = i+1;
            walletdb.WritePool(nIndex, CKeyPool(GenerateNewKey()));
            setKeyPool.insert(nIndex);
        }
        LogPrintf("CWallet::NewKeyPool wrote %d new keys\n", nKeys);
    }
    return true;
}

bool CWallet::TopUpKeyPool(unsigned int kpSize)
{
    {
        LOCK(cs_wallet);

        if (IsLocked())
            return false;

        CWalletDB walletdb(strWalletFile);

        // Top up key pool
        unsigned int nTargetSize;
        if (kpSize > 0)
            nTargetSize = kpSize;
        else
            nTargetSize = max(GetArg("-keypool", DEFAULT_KEYPOOL_SIZE), (int64_t) 0);

        while (setKeyPool.size() < (nTargetSize + 1))
        {
            int64_t nEnd = 1;
            if (!setKeyPool.empty())
                nEnd = *(--setKeyPool.end()) + 1;
            if (!walletdb.WritePool(nEnd, CKeyPool(GenerateNewKey())))
                throw runtime_error("TopUpKeyPool(): writing generated key failed");
            setKeyPool.insert(nEnd);
            LogPrintf("keypool added key %d, size=%u\n", nEnd, setKeyPool.size());
        }
    }
    return true;
}

void CWallet::ReserveKeyFromKeyPool(int64_t& nIndex, CKeyPool& keypool)
{
    nIndex = -1;
    keypool.vchPubKey = CPubKey();
    {
        LOCK(cs_wallet);

        if (!IsLocked())
            TopUpKeyPool();

        // Get the oldest key
        if(setKeyPool.empty())
            return;

        CWalletDB walletdb(strWalletFile);

        nIndex = *(setKeyPool.begin());
        setKeyPool.erase(setKeyPool.begin());
        if (!walletdb.ReadPool(nIndex, keypool))
            throw runtime_error("ReserveKeyFromKeyPool(): read failed");
        if (!HaveKey(keypool.vchPubKey.GetID()))
            throw runtime_error("ReserveKeyFromKeyPool(): unknown key in key pool");
        assert(keypool.vchPubKey.IsValid());
        LogPrintf("keypool reserve %d\n", nIndex);
    }
}

void CWallet::KeepKey(int64_t nIndex)
{
    // Remove from key pool
    if (fFileBacked)
    {
        CWalletDB walletdb(strWalletFile);
        walletdb.ErasePool(nIndex);
    }
    LogPrintf("keypool keep %d\n", nIndex);
}

void CWallet::ReturnKey(int64_t nIndex)
{
    // Return to key pool
    {
        LOCK(cs_wallet);
        setKeyPool.insert(nIndex);
    }
    LogPrintf("keypool return %d\n", nIndex);
}

bool CWallet::GetKeyFromPool(CPubKey& result)
{
    int64_t nIndex = 0;
    CKeyPool keypool;
    {
        LOCK(cs_wallet);
        ReserveKeyFromKeyPool(nIndex, keypool);
        if (nIndex == -1)
        {
            if (IsLocked()) return false;
            result = GenerateNewKey();
            return true;
        }
        KeepKey(nIndex);
        result = keypool.vchPubKey;
    }
    return true;
}

// BLSCT Blinding Key Pool

bool CWallet::NewBLSCTBlindingKeyPool()
{
    {
        LOCK(cs_wallet);
        CWalletDB walletdb(strWalletFile);
        for(int64_t nIndex: setBLSCTBlindingKeyPool)
            walletdb.EraseBLSCTBlindingPool(nIndex);
        setBLSCTBlindingKeyPool.clear();

        if (IsLocked())
            return false;

        int64_t nKeys = max(GetArg("-keypool", DEFAULT_KEYPOOL_SIZE), (int64_t)0);
        for (int i = 0; i < nKeys; i++)
        {
            int64_t nIndex = i+1;
            walletdb.WriteBLSCTBlindingPool(nIndex, CBLSCTBlindingKeyPool(GenerateNewBlindingKey()));
            setBLSCTBlindingKeyPool.insert(nIndex);
        }
        LogPrintf("CWallet::%s: wrote %d new keys\n", __func__, nKeys);
    }
    return true;
}

bool CWallet::TopUpBLSCTBlindingKeyPool(unsigned int kpSize)
{
    {
        LOCK(cs_wallet);

        if (IsLocked())
            return false;

        CWalletDB walletdb(strWalletFile);

        // Top up key pool
        unsigned int nTargetSize;
        if (kpSize > 0)
            nTargetSize = kpSize;
        else
            nTargetSize = max(GetArg("-keypool", DEFAULT_KEYPOOL_SIZE), (int64_t) 0);

        while (setBLSCTBlindingKeyPool.size() < (nTargetSize + 1))
        {
            int64_t nEnd = 1;
            if (!setBLSCTBlindingKeyPool.empty())
                nEnd = *(--setBLSCTBlindingKeyPool.end()) + 1;
            if (!walletdb.WriteBLSCTBlindingPool(nEnd, CBLSCTBlindingKeyPool(GenerateNewBlindingKey())))
                throw runtime_error("TopUpBLSCTBlindingKeyPool(): writing generated key failed");
            setBLSCTBlindingKeyPool.insert(nEnd);
            LogPrintf("blsctblindingkeypool added key %d, size=%u\n", nEnd, setBLSCTBlindingKeyPool.size());
        }
    }
    return true;
}

void CWallet::ReserveBLSCTBlindingKeyFromKeyPool(int64_t& nIndex, CBLSCTBlindingKeyPool& keypool)
{
    nIndex = -1;
    keypool.vchPubKey = blsctPublicKey();
    {
        LOCK(cs_wallet);

        if (!IsLocked())
            TopUpBLSCTBlindingKeyPool();

        // Get the oldest key
        if(setBLSCTBlindingKeyPool.empty())
            return;

        CWalletDB walletdb(strWalletFile);

        nIndex = *(setBLSCTBlindingKeyPool.begin());
        setBLSCTBlindingKeyPool.erase(setBLSCTBlindingKeyPool.begin());
        if (!walletdb.ReadBLSCTBlindingPool(nIndex, keypool))
            throw runtime_error("ReserveBLSCTBlindingKeyFromKeyPool(): read failed");
        if (!HaveBLSCTBlindingKey(keypool.vchPubKey))
            throw runtime_error("ReserveBLSCTBlindingKeyFromKeyPool(): unknown key in key pool");
        assert(keypool.vchPubKey.IsValid());
        LogPrintf("blsctblindingkeypool reserve %d\n", nIndex);
    }
}

void CWallet::KeepBLSCTBlindingKey(int64_t nIndex)
{
    // Remove from key pool
    if (fFileBacked)
    {
        CWalletDB walletdb(strWalletFile);
        walletdb.EraseBLSCTBlindingPool(nIndex);
    }
    LogPrintf("blsctblindingkeypool keep %d\n", nIndex);
}

void CWallet::ReturnBLSCTBlindingKey(int64_t nIndex)
{
    // Return to key pool
    {
        LOCK(cs_wallet);
        setBLSCTBlindingKeyPool.insert(nIndex);
    }
    LogPrintf("blsctblindingkeypool return %d\n", nIndex);
}

bool CWallet::GetBLSCTBlindingKeyFromPool(blsctPublicKey& result)
{
    int64_t nIndex = 0;
    CBLSCTBlindingKeyPool keypool;
    {
        LOCK(cs_wallet);
        ReserveBLSCTBlindingKeyFromKeyPool(nIndex, keypool);
        if (nIndex == -1)
        {
            if (IsLocked()) return false;
            result = GenerateNewBlindingKey();
            return true;
        }
        KeepBLSCTBlindingKey(nIndex);
        result = keypool.vchPubKey;
    }
    return true;
}

int64_t CWallet::GetOldestBLSCTBlindingKeyPoolTime()
{
    LOCK(cs_wallet);

    // if the keypool is empty, return <NOW>
    if (setBLSCTBlindingKeyPool.empty())
        return GetTime();

    // load oldest key from keypool, get time and return
    CBLSCTBlindingKeyPool keypool;
    CWalletDB walletdb(strWalletFile);
    int64_t nIndex = *(setBLSCTBlindingKeyPool.begin());
    if (!walletdb.ReadBLSCTBlindingPool(nIndex, keypool))
        throw runtime_error("GetOldestBLSCTBlindingKeyPoolTime(): read oldest key in keypool failed");
    assert(keypool.vchPubKey.IsValid());
    return keypool.nTime;
}

// BLSCT Sub Address Key Pool

bool CWallet::NewBLSCTSubAddressKeyPool(const uint64_t& account)
{
    LogPrintf("%s: Creating new blsct sub address key pool\n", __func__);
    {
        LOCK(cs_wallet);
        CWalletDB walletdb(strWalletFile);

        if (mapBLSCTSubAddressKeyPool.count(account))
        {
            for(int64_t nIndex: mapBLSCTSubAddressKeyPool[account])
                walletdb.EraseBLSCTSubAddressPool(account, nIndex);
            mapBLSCTSubAddressKeyPool[account].clear();
        }
        else
        {
            mapBLSCTSubAddressKeyPool.insert(std::make_pair(account, std::set<uint64_t>()));
        }

        if (IsLocked())
            return false;

        int64_t nKeys = max(GetArg("-keypool", DEFAULT_KEYPOOL_SIZE), (int64_t)0);
        blsctDoublePublicKey pk;
        for (int i = 0; i < nKeys; i++)
        {
            if (GenerateNewSubAddress(account, pk))
            {
                int64_t nIndex = i+1;
                walletdb.WriteBLSCTSubAddressPool(account, nIndex, CBLSCTSubAddressKeyPool(pk.GetID()));
                mapBLSCTSubAddressKeyPool[account].insert(nIndex);
            }
        }
        LogPrintf("CWallet::%s: wrote %d new sub address keys\n", __func__, nKeys);
    }
    return true;
}

bool CWallet::TopUpBLSCTSubAddressKeyPool(const uint64_t& account, unsigned int kpSize)
{
    {
        LOCK(cs_wallet);

        if (IsLocked())
            return false;

        CWalletDB walletdb(strWalletFile);

        // Top up key pool
        unsigned int nTargetSize;
        if (kpSize > 0)
            nTargetSize = kpSize;
        else
            nTargetSize = max(GetArg("-keypool", DEFAULT_KEYPOOL_SIZE), (int64_t) 0);

        if (mapBLSCTSubAddressKeyPool.count(account) == 0)
            mapBLSCTSubAddressKeyPool.insert(std::make_pair(account, std::set<uint64_t>()));

        while (mapBLSCTSubAddressKeyPool[account].size() < (nTargetSize + 1))
        {
            int64_t nEnd = 1;
            if (!mapBLSCTSubAddressKeyPool[account].empty())
                nEnd = *(--mapBLSCTSubAddressKeyPool[account].end()) + 1;
            blsctDoublePublicKey pk;
            if (GenerateNewSubAddress(account, pk))
            {
                if (!walletdb.WriteBLSCTSubAddressPool(account, nEnd, CBLSCTSubAddressKeyPool(pk.GetID())))
                    throw runtime_error("TopUpBLSCTSubAddressKeyPool(): writing generated key failed");
                mapBLSCTSubAddressKeyPool[account].insert(nEnd);
                LogPrintf("blsctSubAddressKeyPool added key %d, size=%u\n", nEnd, mapBLSCTSubAddressKeyPool[account].size());
            }
        }
    }
    return true;
}

void CWallet::ReserveBLSCTSubAddressKeyFromKeyPool(const uint64_t& account, int64_t& nIndex, CBLSCTSubAddressKeyPool& keypool)
{
    nIndex = -1;
    keypool.hashId = CKeyID();
    {
        LOCK(cs_wallet);

        if (!IsLocked())
            TopUpBLSCTSubAddressKeyPool(account);

        if (mapBLSCTSubAddressKeyPool.count(account) == 0)
            mapBLSCTSubAddressKeyPool.insert(std::make_pair(account, std::set<uint64_t>()));

        // Get the oldest key
        if(mapBLSCTSubAddressKeyPool[account].empty())
            return;

        CWalletDB walletdb(strWalletFile);

        nIndex = *(mapBLSCTSubAddressKeyPool[account].begin());
        mapBLSCTSubAddressKeyPool[account].erase(mapBLSCTSubAddressKeyPool[account].begin());
        if (!walletdb.ReadBLSCTSubAddressPool(account, nIndex, keypool))
            throw runtime_error("ReserveBLSCTSubAddressKeyFromKeyPool(): read failed");
        LogPrintf("%s: read %s with index %d\n", __func__, keypool.hashId.ToString(), nIndex);
        if (!HaveBLSCTSubAddress(keypool.hashId))
            throw runtime_error("ReserveBLSCTSubAddressKeyFromKeyPool(): unknown key in key pool");
        LogPrintf("blsctSubAddressKeyPool reserve %d\n", nIndex);
    }
}

void CWallet::KeepBLSCTSubAddressKey(const uint64_t& account, int64_t nIndex)
{
    // Remove from key pool
    if (fFileBacked)
    {
        CWalletDB walletdb(strWalletFile);
        walletdb.EraseBLSCTSubAddressPool(account, nIndex);
    }
    LogPrintf("blsctSubAddressKeyPool keep %d\n", nIndex);
}

void CWallet::ReturnBLSCTSubAddressKey(const uint64_t& account, int64_t nIndex)
{
    // Return to key pool
    {
        LOCK(cs_wallet);
        if (mapBLSCTSubAddressKeyPool.count(account) == 0)
            mapBLSCTSubAddressKeyPool.insert(std::make_pair(account, std::set<uint64_t>()));
        mapBLSCTSubAddressKeyPool[account].insert(nIndex);
    }
    LogPrintf("blsctSubAddressKeyPool return %d\n", nIndex);
}

bool CWallet::GetBLSCTSubAddressKeyFromPool(const uint64_t& account, CKeyID& result)
{
    int64_t nIndex = 0;
    CBLSCTSubAddressKeyPool keypool;
    {
        LOCK(cs_wallet);
        ReserveBLSCTSubAddressKeyFromKeyPool(account, nIndex, keypool);
        if (nIndex == -1)
        {
            if (IsLocked()) return false;
            blsctDoublePublicKey pk;
            if (!GenerateNewSubAddress(account, pk))
                return false;
            result = pk.GetID();
            return true;
        }
        KeepBLSCTSubAddressKey(account, nIndex);
        result = keypool.hashId;
    }
    return true;
}

int64_t CWallet::GetOldestBLSCTSubAddressKeyPoolTime(const uint64_t& account)
{
    LOCK(cs_wallet);

    if (mapBLSCTSubAddressKeyPool.count(account) == 0)
        mapBLSCTSubAddressKeyPool.insert(std::make_pair(account, std::set<uint64_t>()));

    // if the keypool is empty, return <NOW>
    if (mapBLSCTSubAddressKeyPool[account].empty())
        return GetTime();

    // load oldest key from keypool, get time and return
    CBLSCTSubAddressKeyPool keypool;
    CWalletDB walletdb(strWalletFile);
    int64_t nIndex = *(mapBLSCTSubAddressKeyPool[account].begin());
    if (!walletdb.ReadBLSCTSubAddressPool(account, nIndex, keypool))
        throw runtime_error("GetOldestBLSCTSubAddressKeyPoolTime(): read oldest key in keypool failed");
    return keypool.nTime;
}

int64_t CWallet::GetOldestKeyPoolTime()
{
    LOCK(cs_wallet);

    // if the keypool is empty, return <NOW>
    if (setKeyPool.empty())
        return GetTime();

    // load oldest key from keypool, get time and return
    CKeyPool keypool;
    CWalletDB walletdb(strWalletFile);
    int64_t nIndex = *(setKeyPool.begin());
    if (!walletdb.ReadPool(nIndex, keypool))
        throw runtime_error("GetOldestKeyPoolTime(): read oldest key in keypool failed");
    assert(keypool.vchPubKey.IsValid());
    return keypool.nTime;
}

std::map<CTxDestination, CAmount> CWallet::GetAddressBalances()
{
    map<CTxDestination, CAmount> balances;

    {
        LOCK(cs_wallet);
        for(PAIRTYPE(uint256, CWalletTx) walletEntry: mapWallet)
        {
            CWalletTx *pcoin = &walletEntry.second;

            if (!CheckFinalTx(*pcoin) || !pcoin->IsTrusted())
                continue;

            if ((pcoin->IsCoinStake() || pcoin->IsCoinBase()) && pcoin->GetBlocksToMaturity() > 0)
                continue;

            int nDepth = pcoin->GetDepthInMainChain();
            if (nDepth < (pcoin->IsFromMe(ISMINE_ALL) ? 0 : 1))
                continue;

            for (unsigned int i = 0; i < pcoin->vout.size(); i++)
            {
                CTxDestination addr;
                if (!IsMine(pcoin->vout[i]))
                    continue;
                if(!ExtractDestination(pcoin->vout[i].scriptPubKey, addr))
                    continue;

                CAmount n = IsSpent(walletEntry.first, i) ? 0 : pcoin->vout[i].nValue;

                if (!balances.count(addr))
                    balances[addr] = 0;
                balances[addr] += n;
            }
        }
    }

    return balances;
}

set< set<CTxDestination> > CWallet::GetAddressGroupings()
{
    AssertLockHeld(cs_wallet); // mapWallet
    set< set<CTxDestination> > groupings;
    set<CTxDestination> grouping;

    for(PAIRTYPE(uint256, CWalletTx) walletEntry: mapWallet)
    {
        CWalletTx *pcoin = &walletEntry.second;

        if (pcoin->vin.size() > 0)
        {
            bool any_mine = false;
            // group all input addresses with each other
            for(CTxIn txin: pcoin->vin)
            {
                CTxDestination address;
                if(!IsMine(txin)) /* If this input isn't mine, ignore it */
                    continue;
                if(!ExtractDestination(mapWallet[txin.prevout.hash].vout[txin.prevout.n].scriptPubKey, address))
                    continue;
                grouping.insert(address);
                any_mine = true;
            }

            // group change with input addresses
            if (any_mine)
            {
               for(CTxOut txout: pcoin->vout)
                   if (IsChange(txout))
                   {
                       CTxDestination txoutAddr;
                       if(!ExtractDestination(txout.scriptPubKey, txoutAddr))
                           continue;
                       grouping.insert(txoutAddr);
                   }
            }
            if (grouping.size() > 0)
            {
                groupings.insert(grouping);
                grouping.clear();
            }
        }

        // group lone addrs by themselves
        for (unsigned int i = 0; i < pcoin->vout.size(); i++)
            if (IsMine(pcoin->vout[i]))
            {
                CTxDestination address;
                if(!ExtractDestination(pcoin->vout[i].scriptPubKey, address))
                    continue;
                grouping.insert(address);
                groupings.insert(grouping);
                grouping.clear();
            }
    }

    set< set<CTxDestination>* > uniqueGroupings; // a set of pointers to groups of addresses
    map< CTxDestination, set<CTxDestination>* > setmap;  // map addresses to the unique group containing it
    for(set<CTxDestination> grouping: groupings)
    {
        // make a set of all the groups hit by this new group
        set< set<CTxDestination>* > hits;
        map< CTxDestination, set<CTxDestination>* >::iterator it;
        for(CTxDestination address: grouping)
            if ((it = setmap.find(address)) != setmap.end())
                hits.insert((*it).second);

        // merge all hit groups into a new single group and delete old groups
        set<CTxDestination>* merged = new set<CTxDestination>(grouping);
        for(set<CTxDestination>* hit: hits)
        {
            merged->insert(hit->begin(), hit->end());
            uniqueGroupings.erase(hit);
            delete hit;
        }
        uniqueGroupings.insert(merged);

        // update setmap
        for(CTxDestination element: *merged)
            setmap[element] = merged;
    }

    set< set<CTxDestination> > ret;
    for(set<CTxDestination>* uniqueGrouping: uniqueGroupings)
    {
        ret.insert(*uniqueGrouping);
        delete uniqueGrouping;
    }

    return ret;
}

CAmount CWallet::GetAccountBalance(const std::string& strAccount, int nMinDepth, const isminefilter& filter)
{
    CWalletDB walletdb(strWalletFile);
    return GetAccountBalance(walletdb, strAccount, nMinDepth, filter);
}

CAmount CWallet::GetAccountBalance(CWalletDB& walletdb, const std::string& strAccount, int nMinDepth, const isminefilter& filter)
{
    CAmount nBalance = 0;

    // Tally wallet transactions
    for (map<uint256, CWalletTx>::iterator it = mapWallet.begin(); it != mapWallet.end(); ++it)
    {
        const CWalletTx& wtx = (*it).second;
        if (!CheckFinalTx(wtx) || wtx.GetBlocksToMaturity() > 0 || wtx.GetDepthInMainChain() < 0)
            continue;

        CAmount nReceived, nSent, nFee;
        wtx.GetAccountAmounts(strAccount, nReceived, nSent, nFee, filter);

        if (nReceived != 0 && wtx.GetDepthInMainChain() >= nMinDepth)
            nBalance += nReceived;
        nBalance -= nSent + nFee;
    }

    // Tally internal accounting entries
    nBalance += walletdb.GetAccountCreditDebit(strAccount);

    return nBalance;
}

std::set<CTxDestination> CWallet::GetAccountAddresses(const std::string& strAccount) const
{
    LOCK(cs_wallet);
    set<CTxDestination> result;
    for(const PAIRTYPE(CTxDestination, CAddressBookData)& item: mapAddressBook)
    {
        const CTxDestination& address = item.first;
        const string& strName = item.second.name;
        if (strName == strAccount)
            result.insert(address);
    }
    return result;
}

bool CReserveKey::GetReservedKey(CPubKey& pubkey)
{
    if (nIndex == -1)
    {
        CKeyPool keypool;
        pwallet->ReserveKeyFromKeyPool(nIndex, keypool);
        if (nIndex != -1)
            vchPubKey = keypool.vchPubKey;
        else {
            return false;
        }
    }
    assert(vchPubKey.IsValid());
    pubkey = vchPubKey;
    return true;
}

void CReserveKey::KeepKey()
{
    if (nIndex != -1)
        pwallet->KeepKey(nIndex);
    nIndex = -1;
    vchPubKey = CPubKey();
}

void CReserveKey::ReturnKey()
{
    if (nIndex != -1)
        pwallet->ReturnKey(nIndex);
    nIndex = -1;
    vchPubKey = CPubKey();
}

bool CReserveBLSCTBlindingKey::GetReservedKey(blsctPublicKey& pubkey)
{
    if (nIndex == -1)
    {
        CBLSCTBlindingKeyPool keypool;
        pwallet->ReserveBLSCTBlindingKeyFromKeyPool(nIndex, keypool);
        if (nIndex != -1)
            vchPubKey = keypool.vchPubKey;
        else {
            return false;
        }
    }
    assert(vchPubKey.IsValid());
    pubkey = vchPubKey;
    return true;
}

void CReserveBLSCTBlindingKey::KeepKey()
{
    if (nIndex != -1)
        pwallet->KeepBLSCTBlindingKey(nIndex);
    nIndex = -1;
    vchPubKey = blsctPublicKey();
}

void CReserveBLSCTBlindingKey::ReturnKey()
{
    if (nIndex != -1)
        pwallet->ReturnBLSCTBlindingKey(nIndex);
    nIndex = -1;
    vchPubKey = blsctPublicKey();
}

void CWallet::GetAllReserveKeys(set<CKeyID>& setAddress) const
{
    setAddress.clear();

    CWalletDB walletdb(strWalletFile);

    LOCK2(cs_main, cs_wallet);
    for(const int64_t& id: setKeyPool)
    {
        CKeyPool keypool;
        if (!walletdb.ReadPool(id, keypool))
            throw runtime_error("GetAllReserveKeyHashes(): read failed");
        assert(keypool.vchPubKey.IsValid());
        CKeyID keyID = keypool.vchPubKey.GetID();
        if (!HaveKey(keyID))
            throw runtime_error("GetAllReserveKeyHashes(): unknown key in key pool");
        setAddress.insert(keyID);
    }
}

void CWallet::GetAllReserveBLSCTBlindingKeys(set<blsctPublicKey>& setAddress) const
{
    setAddress.clear();

    CWalletDB walletdb(strWalletFile);

    LOCK2(cs_main, cs_wallet);
    for(const int64_t& id: setBLSCTBlindingKeyPool)
    {
        CBLSCTBlindingKeyPool keypool;
        if (!walletdb.ReadBLSCTBlindingPool(id, keypool))
            throw runtime_error("GetAllReserveBLSCTBlindingKeys(): read failed");
        assert(keypool.vchPubKey.IsValid());
        if (!HaveBLSCTBlindingKey(keypool.vchPubKey))
            throw runtime_error("GetAllReserveBLSCTBlindingKeys(): unknown key in key pool");
        setAddress.insert(keypool.vchPubKey);
    }
}

void CWallet::UpdatedTransaction(const uint256 &hashTx)
{
    {
        LOCK(cs_wallet);
        // Only notify UI if this transaction is in this wallet
        map<uint256, CWalletTx>::const_iterator mi = mapWallet.find(hashTx);
        if (mi != mapWallet.end())
            NotifyTransactionChanged(this, hashTx, CT_UPDATED);
    }
}

void CWallet::GetScriptForMining(boost::shared_ptr<CReserveScript> &script)
{
    boost::shared_ptr<CReserveKey> rKey(new CReserveKey(this));
    CPubKey pubkey;
    if (!rKey->GetReservedKey(pubkey))
        return;

    script = rKey;
    script->reserveScript = CScript() << ToByteVector(pubkey) << OP_CHECKSIG;
}

void CWallet::LockCoin(const COutPoint& output)
{
    AssertLockHeld(cs_wallet); // setLockedCoins
    setLockedCoins.insert(output);
}

void CWallet::UnlockCoin(const COutPoint& output)
{
    AssertLockHeld(cs_wallet); // setLockedCoins
    setLockedCoins.erase(output);
}

void CWallet::UnlockAllCoins()
{
    AssertLockHeld(cs_wallet); // setLockedCoins
    setLockedCoins.clear();
}

bool CWallet::IsLockedCoin(uint256 hash, unsigned int n) const
{
    AssertLockHeld(cs_wallet); // setLockedCoins
    COutPoint outpt(hash, n);

    return (setLockedCoins.count(outpt) > 0);
}

void CWallet::ListLockedCoins(std::vector<COutPoint>& vOutpts)
{
    AssertLockHeld(cs_wallet); // setLockedCoins
    for (std::set<COutPoint>::iterator it = setLockedCoins.begin();
         it != setLockedCoins.end(); it++) {
        COutPoint outpt = (*it);
        vOutpts.push_back(outpt);
    }
}

/** @} */ // end of Actions

class CAffectedKeysVisitor : public boost::static_visitor<void> {
private:
    const CKeyStore &keystore;
    std::vector<CKeyID> &vKeys;

public:
    CAffectedKeysVisitor(const CKeyStore &keystoreIn, std::vector<CKeyID> &vKeysIn) : keystore(keystoreIn), vKeys(vKeysIn) {}

    void Process(const CScript &script) {
        txnouttype type;
        std::vector<CTxDestination> vDest;
        int nRequired;
        if (ExtractDestinations(script, type, vDest, nRequired)) {
            for(const CTxDestination &dest: vDest)
                boost::apply_visitor(*this, dest);
        }
    }

    void operator()(const CKeyID &keyId) {
        if (keystore.HaveKey(keyId))
            vKeys.push_back(keyId);
    }

    void operator()(const blsctDoublePublicKey &keyId) {
    }

    void operator()(const pair<CKeyID, CKeyID> &keyId) {
        if (keystore.HaveKey(keyId.first))
            vKeys.push_back(keyId.first);
        if (keystore.HaveKey(keyId.second))
            vKeys.push_back(keyId.second);
    }

    void operator()(const pair<CKeyID, pair<CKeyID, CKeyID>> &keyId) {
        if (keystore.HaveKey(keyId.first))
            vKeys.push_back(keyId.first);
        if (keystore.HaveKey(keyId.second.first))
            vKeys.push_back(keyId.second.first);
        if (keystore.HaveKey(keyId.second.second))
            vKeys.push_back(keyId.second.second);
    }

    void operator()(const CScriptID &scriptId) {
        CScript script;
        if (keystore.GetCScript(scriptId, script))
            Process(script);
    }

    void operator()(const CNoDestination &none) {}
};

void CWallet::GetKeyBirthTimes(std::map<CKeyID, int64_t> &mapKeyBirth) const {
    AssertLockHeld(cs_wallet); // mapKeyMetadata
    mapKeyBirth.clear();

    // get birth times for keys with metadata
    for (std::map<CKeyID, CKeyMetadata>::const_iterator it = mapKeyMetadata.begin(); it != mapKeyMetadata.end(); it++)
        if (it->second.nCreateTime)
            mapKeyBirth[it->first] = it->second.nCreateTime;

    // map in which we'll infer heights of other keys
    CBlockIndex *pindexMax = chainActive[std::max(0, chainActive.Height() - 144)]; // the tip can be reorganised; use a 144-block safety margin
    std::map<CKeyID, CBlockIndex*> mapKeyFirstBlock;
    std::set<CKeyID> setKeys;
    GetKeys(setKeys);
    for(const CKeyID &keyid: setKeys) {
        if (mapKeyBirth.count(keyid) == 0)
            mapKeyFirstBlock[keyid] = pindexMax;
    }
    setKeys.clear();

    // if there are no such keys, we're done
    if (mapKeyFirstBlock.empty())
        return;

    // find first block that affects those keys, if there are any left
    std::vector<CKeyID> vAffected;
    for (std::map<uint256, CWalletTx>::const_iterator it = mapWallet.begin(); it != mapWallet.end(); it++) {
        // iterate over all wallet transactions...
        const CWalletTx &wtx = (*it).second;
        BlockMap::const_iterator blit = mapBlockIndex.find(wtx.hashBlock);
        if (blit != mapBlockIndex.end() && chainActive.Contains(blit->second)) {
            // ... which are already in a block
            int nHeight = blit->second->nHeight;
            for(const CTxOut &txout: wtx.vout) {
                // iterate over all their outputs
                CAffectedKeysVisitor(*this, vAffected).Process(txout.scriptPubKey);
                for(const CKeyID &keyid: vAffected) {
                    // ... and all their affected keys
                    std::map<CKeyID, CBlockIndex*>::iterator rit = mapKeyFirstBlock.find(keyid);
                    if (rit != mapKeyFirstBlock.end() && nHeight < rit->second->nHeight)
                        rit->second = blit->second;
                }
                vAffected.clear();
            }
        }
    }

    // Extract block timestamps for those keys
    for (std::map<CKeyID, CBlockIndex*>::const_iterator it = mapKeyFirstBlock.begin(); it != mapKeyFirstBlock.end(); it++)
        mapKeyBirth[it->first] = it->second->GetBlockTime() - 7200; // block times can be 2h off
}

bool CWallet::AddDestData(const CTxDestination &dest, const std::string &key, const std::string &value)
{
    if (boost::get<CNoDestination>(&dest))
        return false;

    mapAddressBook[dest].destdata.insert(std::make_pair(key, value));
    if (!fFileBacked)
        return true;
    return CWalletDB(strWalletFile).WriteDestData(CNavCoinAddress(dest).ToString(), key, value);
}

bool CWallet::EraseDestData(const CTxDestination &dest, const std::string &key)
{
    if (!mapAddressBook[dest].destdata.erase(key))
        return false;
    if (!fFileBacked)
        return true;
    return CWalletDB(strWalletFile).EraseDestData(CNavCoinAddress(dest).ToString(), key);
}

bool CWallet::LoadDestData(const CTxDestination &dest, const std::string &key, const std::string &value)
{
    mapAddressBook[dest].destdata.insert(std::make_pair(key, value));
    return true;
}

bool CWallet::GetDestData(const CTxDestination &dest, const std::string &key, std::string *value) const
{
    std::map<CTxDestination, CAddressBookData>::const_iterator i = mapAddressBook.find(dest);
    if(i != mapAddressBook.end())
    {
        CAddressBookData::StringMap::const_iterator j = i->second.destdata.find(key);
        if(j != i->second.destdata.end())
        {
            if(value)
                *value = j->second;
            return true;
        }
    }
    return false;
}

std::string CWallet::GetWalletHelpString(bool showDebug)
{
    std::string strUsage = HelpMessageGroup(_("Wallet options:"));
    strUsage += HelpMessageOpt("-disablewallet", _("Do not load the wallet and disable wallet RPC calls"));
    strUsage += HelpMessageOpt("-keypool=<n>", strprintf(_("Set key pool size to <n> (default: %u)"), DEFAULT_KEYPOOL_SIZE));
    strUsage += HelpMessageOpt("-fallbackfee=<amt>", strprintf(_("A fee rate (in %s/kB) that will be used when fee estimation has insufficient data (default: %s)"),
                                                               CURRENCY_UNIT, FormatMoney(DEFAULT_FALLBACK_FEE)));
    strUsage += HelpMessageOpt("-importmnemonic=\"<word list>\"", _("Create a new wallet out of the specified mnemonic"));
    strUsage += HelpMessageOpt("-mnemoniclanguage=<lang>", _("Use the specified language for the mnemonic import"));
    strUsage += HelpMessageOpt("-mintxfee=<amt>", strprintf(_("Fees (in %s/kB) smaller than this are considered zero fee for transaction creation (default: %s)"),
                                                            CURRENCY_UNIT, FormatMoney(DEFAULT_TRANSACTION_MINFEE)));
    strUsage += HelpMessageOpt("-paytxfee=<amt>", strprintf(_("Fee (in %s/kB) to add to transactions you send (default: %s)"),
                                                            CURRENCY_UNIT, FormatMoney(payTxFee.GetFeePerK())));
    strUsage += HelpMessageOpt("-rescan", _("Rescan the block chain for missing wallet transactions on startup"));
    strUsage += HelpMessageOpt("-salvagewallet", _("Attempt to recover private keys from a corrupt wallet on startup"));
    if (showDebug)
        strUsage += HelpMessageOpt("-sendfreetransactions", strprintf(_("Send transactions as zero-fee transactions if possible (default: %u)"), DEFAULT_SEND_FREE_TRANSACTIONS));
    strUsage += HelpMessageOpt("-spendzeroconfchange", strprintf(_("Spend unconfirmed change when sending transactions (default: %u)"), DEFAULT_SPEND_ZEROCONF_CHANGE));
    strUsage += HelpMessageOpt("-stakingaddress", strprintf(_("Specify a customised navcoin address to accumulate the staking rewards.")));
    strUsage += HelpMessageOpt("-suppressblsctwarning", _("Disables the warning when wallet can't configure BLSCT") + " " + strprintf(_("(default: %u)"), DEFAULT_SUPPRESS_BLSCT_WARNING));
    strUsage += HelpMessageOpt("-txconfirmtarget=<n>", strprintf(_("If paytxfee is not set, include enough fee so transactions begin confirmation on average within n blocks (default: %u)"), DEFAULT_TX_CONFIRM_TARGET));
    strUsage += HelpMessageOpt("-upgradewallet", _("Upgrade wallet to latest format on startup"));
    strUsage += HelpMessageOpt("-usehd", _("Use hierarchical deterministic key generation (HD) after BIP32. Only has effect during wallet creation/first start") + " " + strprintf(_("(default: %u)"), DEFAULT_USE_HD_WALLET));
    strUsage += HelpMessageOpt("-wallet=<file>", _("Specify wallet file (within data directory)") + " " + strprintf(_("(default: %s)"), DEFAULT_WALLET_DAT));
    strUsage += HelpMessageOpt("-walletbroadcast", _("Make the wallet broadcast transactions") + " " + strprintf(_("(default: %u)"), DEFAULT_WALLETBROADCAST));
    strUsage += HelpMessageOpt("-walletnotify=<cmd>", _("Execute command when a wallet transaction changes (%s in cmd is replaced by TxID)"));
    strUsage += HelpMessageOpt("-zapwallettxes=<mode>", _("Delete all wallet transactions and only recover those parts of the blockchain through -rescan on startup") +
                               " " + _("(1 = keep tx meta data e.g. account owner and payment request information, 2 = drop tx meta data)"));

    if (showDebug)
    {
        strUsage += HelpMessageGroup(_("Wallet debugging/testing options:"));

        strUsage += HelpMessageOpt("-dblogsize=<n>", strprintf("Flush wallet database activity from memory to disk log every <n> megabytes (default: %u)", DEFAULT_WALLET_DBLOGSIZE));
        strUsage += HelpMessageOpt("-flushwallet", strprintf("Run a thread to flush wallet periodically (default: %u)", DEFAULT_FLUSHWALLET));
        strUsage += HelpMessageOpt("-privdb", strprintf("Sets the DB_PRIVATE flag in the wallet db environment (default: %u)", DEFAULT_WALLET_PRIVDB));
    }

    return strUsage;
}

bool CWallet::InitLoadWallet(const std::string& wordlist, const std::string& password)
{
    std::string walletFile = GetArg("-wallet", DEFAULT_WALLET_DAT);

    // needed to restore wallet transaction meta data after -zapwallettxes
    std::vector<CWalletTx> vWtx;

    if (GetBoolArg("-zapwallettxes", false)) {
        uiInterface.InitMessage(_("Zapping all transactions from wallet..."));

        CWallet *tempWallet = new CWallet(walletFile);
        DBErrors nZapWalletRet = tempWallet->ZapWalletTx(vWtx);
        if (nZapWalletRet != DB_LOAD_OK) {
            return InitError(strprintf(_("Error loading %s: Wallet corrupted"), walletFile));
        }

        delete tempWallet;
        tempWallet = nullptr;
    }

    uiInterface.InitMessage(_("Loading wallet..."));

    int64_t nStart = GetTimeMillis();
    bool fFirstRun = true;
    bool fFirstBLSCTRun = true;
    CWallet *walletInstance = new CWallet(walletFile);
    DBErrors nLoadWalletRet = walletInstance->LoadWallet(fFirstRun, fFirstBLSCTRun);
    if (nLoadWalletRet != DB_LOAD_OK)
    {
        if (nLoadWalletRet == DB_CORRUPT)
            return InitError(strprintf(_("Error loading %s: Wallet corrupted"), walletFile));
        else if (nLoadWalletRet == DB_NONCRITICAL_ERROR)
        {
            InitWarning(strprintf(_("Error reading %s! All keys read correctly, but transaction data"
                                         " or address book entries might be missing or incorrect."),
                walletFile));
        }
        else if (nLoadWalletRet == DB_TOO_NEW)
            return InitError(strprintf(_("Error loading %s: Wallet requires newer version of %s"),
                               walletFile, _(PACKAGE_NAME)));
        else if (nLoadWalletRet == DB_NEED_REWRITE)
        {
            return InitError(strprintf(_("Wallet needed to be rewritten: restart %s to complete"), _(PACKAGE_NAME)));
        }
        else
            return InitError(strprintf(_("Error loading %s"), walletFile));
    }

    if (GetBoolArg("-upgradewallet", fFirstRun))
    {
        int nMaxVersion = GetArg("-upgradewallet", 0);
        if (nMaxVersion == 0) // the -upgradewallet without argument case
        {
            LogPrintf("Performing wallet upgrade to %i\n", FEATURE_LATEST);
            nMaxVersion = CLIENT_VERSION;
            walletInstance->SetMinVersion(FEATURE_LATEST); // permanently upgrade the wallet immediately
        }
        else
            LogPrintf("Allowing wallet upgrade up to %i\n", nMaxVersion);
        if (nMaxVersion < walletInstance->GetVersion())
        {
            return InitError(_("Cannot downgrade wallet"));
        }
        walletInstance->SetMaxVersion(nMaxVersion);
    }

    if (fFirstRun)
    {
        // Create new keyUser and set as default key
        if (GetBoolArg("-usehd", DEFAULT_USE_HD_WALLET) && walletInstance->hdChain.masterKeyID.IsNull()) {
            // generate a new master key
            CKey key;
            CPubKey masterPubKey;
            word_list words;
            if (GetArg("-importmnemonic","") != "" || !wordlist.empty()) {
                if (!wordlist.empty()) {
                    words = sentence_to_word_list(wordlist);
                } else {
                    words = sentence_to_word_list(GetArg("-importmnemonic",""));
                }
                dictionary lexicon = string_to_lexicon(GetArg("-mnemoniclanguage","english"));
                if (!validate_mnemonic(words, lexicon)) {
                    if (validate_mnemonic(words, language::all))
                        return InitError(strprintf(_("The specified language does not correspond to the mnemonic.")));
                    return InitError(strprintf(_("You specified a wrong mnemonic")));
                }
                masterPubKey = walletInstance->ImportMnemonic(words, lexicon);

                // Check if we gave a password
                if (!password.empty()) {
                    // Is this safe? or do I need to do something more
                    // to the string after I've converted it to SecureString
                    SecureString strWalletPass;
                    strWalletPass.reserve(100);
                    strWalletPass = password.c_str();
                    walletInstance->EncryptWallet(strWalletPass);
                }
            } else
                masterPubKey = walletInstance->GenerateNewHDMasterKey();
            if (!walletInstance->SetHDMasterKey(masterPubKey))
                throw std::runtime_error("CWallet::GenerateNewKey(): Storing master key failed");
        }
        CPubKey newDefaultKey;
        if (walletInstance->GetKeyFromPool(newDefaultKey)) {
            walletInstance->SetDefaultKey(newDefaultKey);
            if (!walletInstance->SetAddressBook(walletInstance->vchDefaultKey.GetID(), "", "receive"))
                return InitError(_("Cannot write default address") += "\n");
        }

        walletInstance->SetBestChain(chainActive.GetLocator());
    }
    else if (GetArg("-importmnemonic","") != "") {
        return InitError(strprintf(_("You are trying to import a new mnemonic but a wallet already exists. Please rename the existing %s before trying to import again."), GetArg("-wallet", DEFAULT_WALLET_DAT)));
    }
    else if (mapArgs.count("-usehd")) {
        bool useHD = GetBoolArg("-usehd", DEFAULT_USE_HD_WALLET);
        if (!walletInstance->hdChain.masterKeyID.IsNull() && !useHD)
            return InitError(strprintf(_("Error loading %s: You can't disable HD on a already existing HD wallet"), walletFile));
        if (walletInstance->hdChain.masterKeyID.IsNull() && useHD)
            return InitError(strprintf(_("Error loading %s: You can't enable HD on a already existing non-HD wallet"), walletFile));
    }

    if(fFirstBLSCTRun)
    {
        CKeyID masterKeyID = walletInstance->GetHDChain().masterKeyID;
        CKey key;

        if (!walletInstance->GetKey(masterKeyID, key))
        {
            if (!GetBoolArg("-suppressblsctwarning", DEFAULT_SUPPRESS_BLSCT_WARNING))
                InitWarning("Could not generate BLSCT parameters. If your wallet is encrypted, you must first unlock your wallet and then trigger manually the parameter generation before being able to use the BLSCT functionality!");
            LogPrintf("Could not generate BLSCT parameters. If your wallet is encrypted, you must first unlock your wallet and then trigger manually the parameter generation before being able to use the BLSCT functionality!");
        }
        else
        {
            CHashWriter h(0, 0);
            std::vector<unsigned char> vKey(key.begin(), key.end());

            h << vKey;

            uint256 hash = h.GetHash();
            unsigned char h_[32];
            memcpy(h_, &hash, 32);

            blsctKey masterBLSKey = blsctKey(bls::PrivateKey::FromSeed(h_, 32));
            blsctKey childBLSKey = blsctKey(masterBLSKey.PrivateChild(BIP32_HARDENED_KEY_LIMIT|130));
            blsctKey transactionBLSKey = blsctKey(childBLSKey.PrivateChild(BIP32_HARDENED_KEY_LIMIT));
            bls::PrivateKey blindingBLSKey = childBLSKey.PrivateChild(BIP32_HARDENED_KEY_LIMIT|1);
            bls::PrivateKey viewKey = transactionBLSKey.PrivateChild(BIP32_HARDENED_KEY_LIMIT);
            bls::PrivateKey spendKey = transactionBLSKey.PrivateChild(BIP32_HARDENED_KEY_LIMIT|1);

            walletInstance->SetBLSCTKeys(viewKey, spendKey, blindingBLSKey);

            walletInstance->NewBLSCTBlindingKeyPool();
            walletInstance->NewBLSCTSubAddressKeyPool(0);

            LogPrintf("Generated BLSCT parameters.\n");
        }
    }

    LogPrintf(" wallet      %15dms\n", GetTimeMillis() - nStart);

    RegisterValidationInterface(walletInstance);

    CBlockIndex *pindexRescan = chainActive.Tip();
    if (GetBoolArg("-rescan", false))
        pindexRescan = chainActive.Genesis();
    else
    {
        CWalletDB walletdb(walletFile);
        CBlockLocator locator;
        if (walletdb.ReadBestBlock(locator))
            pindexRescan = FindForkInGlobalIndex(chainActive, locator);
        else
            pindexRescan = chainActive.Genesis();
    }
    if (chainActive.Tip() && chainActive.Tip() != pindexRescan)
    {
        //We can't rescan beyond non-pruned blocks, stop and throw an error
        //this might happen if a user uses a old wallet within a pruned node
        // or if he ran -disablewallet for a longer time, then decided to re-enable
        if (fPruneMode)
        {
            CBlockIndex *block = chainActive.Tip();
            while (block && block->pprev && (block->pprev->nStatus & BLOCK_HAVE_DATA) && block->pprev->nTx > 0 && pindexRescan != block)
                block = block->pprev;

            if (pindexRescan != block)
                return InitError(_("Prune: last wallet synchronisation goes beyond pruned data. You need to -reindex (download the whole blockchain again in case of pruned node)"));
        }

        uiInterface.InitMessage(_("Rescanning..."));
        LogPrintf("Rescanning last %i blocks (from block %i)...\n", chainActive.Height() - pindexRescan->nHeight, pindexRescan->nHeight);
        nStart = GetTimeMillis();
        walletInstance->ScanForWalletTransactions(pindexRescan, true);
        LogPrintf(" rescan      %15dms\n", GetTimeMillis() - nStart);
        walletInstance->SetBestChain(chainActive.GetLocator());
        nWalletDBUpdated++;

        // Restore wallet transaction metadata after -zapwallettxes=1
        if (GetBoolArg("-zapwallettxes", false) && GetArg("-zapwallettxes", "1") != "2")
        {
            CWalletDB walletdb(walletFile);

            for(const CWalletTx& wtxOld: vWtx)
            {
                uint256 hash = wtxOld.GetHash();
                std::map<uint256, CWalletTx>::iterator mi = walletInstance->mapWallet.find(hash);
                if (mi != walletInstance->mapWallet.end())
                {
                    const CWalletTx* copyFrom = &wtxOld;
                    CWalletTx* copyTo = &mi->second;
                    copyTo->mapValue = copyFrom->mapValue;
                    copyTo->vOrderForm = copyFrom->vOrderForm;
                    copyTo->nTimeReceived = copyFrom->nTimeReceived;
                    copyTo->nTimeSmart = copyFrom->nTimeSmart;
                    copyTo->fFromMe = copyFrom->fFromMe;
                    copyTo->strFromAccount = copyFrom->strFromAccount;
                    copyTo->nOrderPos = copyFrom->nOrderPos;
                    walletdb.WriteTx(*copyTo);
                }
            }
        }
    }
    walletInstance->SetBroadcastTransactions(GetBoolArg("-walletbroadcast", DEFAULT_WALLETBROADCAST));

    pwalletMain = walletInstance;
    pwalletMain->aggSession = new AggregationSesion(pcoinsTip);
    return true;
}

bool CWallet::ParameterInteraction()
{
    if (mapArgs.count("-mintxfee"))
    {
        CAmount n = 0;
        if (ParseMoney(mapArgs["-mintxfee"], n) && n > 0)
            CWallet::minTxFee = CFeeRate(n);
        else
            return InitError(AmountErrMsg("mintxfee", mapArgs["-mintxfee"]));
    }
    if (mapArgs.count("-fallbackfee"))
    {
        CAmount nFeePerK = 0;
        if (!ParseMoney(mapArgs["-fallbackfee"], nFeePerK))
            return InitError(strprintf(_("Invalid amount for -fallbackfee=<amount>: '%s'"), mapArgs["-fallbackfee"]));
        if (nFeePerK > HIGH_TX_FEE_PER_KB)
            InitWarning(_("-fallbackfee is set very high! This is the transaction fee you may pay when fee estimates are not available."));
        CWallet::fallbackFee = CFeeRate(nFeePerK);
    }
    if (mapArgs.count("-paytxfee"))
    {
        CAmount nFeePerK = 0;
        if (!ParseMoney(mapArgs["-paytxfee"], nFeePerK))
            return InitError(AmountErrMsg("paytxfee", mapArgs["-paytxfee"]));
        if (nFeePerK > HIGH_TX_FEE_PER_KB)
            InitWarning(_("-paytxfee is set very high! This is the transaction fee you will pay if you send a transaction."));
        payTxFee = CFeeRate(nFeePerK, 10000);
        if (payTxFee < ::minRelayTxFee)
        {
            return InitError(strprintf(_("Invalid amount for -paytxfee=<amount>: '%s' (must be at least %s)"),
                                       mapArgs["-paytxfee"], ::minRelayTxFee.ToString()));
        }
    }
    if (mapArgs.count("-maxtxfee"))
    {
        CAmount nMaxFee = 0;
        if (!ParseMoney(mapArgs["-maxtxfee"], nMaxFee))
            return InitError(AmountErrMsg("maxtxfee", mapArgs["-maxtxfee"]));
        if (nMaxFee > HIGH_MAX_TX_FEE)
            InitWarning(_("-maxtxfee is set very high! Fees this large could be paid on a single transaction."));
        maxTxFee = nMaxFee;
        if (CFeeRate(maxTxFee, 1000) < ::minRelayTxFee)
        {
            return InitError(strprintf(_("Invalid amount for -maxtxfee=<amount>: '%s' (must be at least the minrelay fee of %s to prevent stuck transactions)"),
                                       mapArgs["-maxtxfee"], ::minRelayTxFee.ToString()));
        }
    }
    nTxConfirmTarget = GetArg("-txconfirmtarget", DEFAULT_TX_CONFIRM_TARGET);
    bSpendZeroConfChange = GetBoolArg("-spendzeroconfchange", DEFAULT_SPEND_ZEROCONF_CHANGE);
    fSendFreeTransactions = GetBoolArg("-sendfreetransactions", DEFAULT_SEND_FREE_TRANSACTIONS);

    return true;
}

bool CWallet::BackupWallet(const std::string& strDest)
{
    if (!fFileBacked)
        return false;
    while (true)
    {
        {
            LOCK(bitdb.cs_db);
            if (!bitdb.mapFileUseCount.count(strWalletFile) || bitdb.mapFileUseCount[strWalletFile] == 0)
            {
                // Flush log data to the dat file
                bitdb.CloseDb(strWalletFile);
                bitdb.CheckpointLSN(strWalletFile);
                bitdb.mapFileUseCount.erase(strWalletFile);

                // Copy wallet file
                boost::filesystem::path pathSrc = GetDataDir() / strWalletFile;
                boost::filesystem::path pathDest(strDest);
                if (boost::filesystem::is_directory(pathDest))
                    pathDest /= strWalletFile;

                try {
#if BOOST_VERSION >= 104000
                    boost::filesystem::copy_file(pathSrc, pathDest, boost::filesystem::copy_option::overwrite_if_exists);
#else
                    boost::filesystem::copy_file(pathSrc, pathDest);
#endif
                    LogPrintf("copied %s to %s\n", strWalletFile, pathDest.string());
                    return true;
                } catch (const boost::filesystem::filesystem_error& e) {
                    LogPrintf("error copying %s to %s - %s\n", strWalletFile, pathDest.string(), e.what());
                    return false;
                }
            }
        }
        MilliSleep(100);
    }
    return false;
}

std::string CWallet::formatDisplayAmount(CAmount amount) {
    stringstream n;
    n.imbue(std::locale(""));
    n << fixed << setprecision(8) << amount/COIN;
    string nav_amount = n.str();
    if(nav_amount.at(nav_amount.length()-1) == '.') {
        nav_amount = nav_amount.substr(0, nav_amount.size()-1);
    }
    nav_amount.append(" NAV");

    return nav_amount;
}

CKeyPool::CKeyPool()
{
    nTime = GetTime();
}

CKeyPool::CKeyPool(const CPubKey& vchPubKeyIn)
{
    nTime = GetTime();
    vchPubKey = vchPubKeyIn;
}


CBLSCTBlindingKeyPool::CBLSCTBlindingKeyPool()
{
    nTime = GetTime();
}

CBLSCTBlindingKeyPool::CBLSCTBlindingKeyPool(const blsctPublicKey& vchPubKeyIn)
{
    nTime = GetTime();
    vchPubKey = vchPubKeyIn;
}

CBLSCTSubAddressKeyPool::CBLSCTSubAddressKeyPool()
{
    nTime = GetTime();
}

CBLSCTSubAddressKeyPool::CBLSCTSubAddressKeyPool(const CKeyID& hashIdIn)
{
    nTime = GetTime();
    hashId = hashIdIn;
}

CWalletKey::CWalletKey(int64_t nExpires)
{
    nTimeCreated = (nExpires ? GetTime() : 0);
    nTimeExpires = nExpires;
}

int CMerkleTx::SetMerkleBranch(const CBlock& block)
{
    AssertLockHeld(cs_main);
    CBlock blockTmp;

    // Update the tx's hashBlock
    hashBlock = block.GetHash();

    // Locate the transaction
    for (nIndex = 0; nIndex < (int)block.vtx.size(); nIndex++)
        if (block.vtx[nIndex] == *(CTransaction*)this)
            break;
    if (nIndex == (int)block.vtx.size())
    {
        nIndex = -1;
        LogPrintf("ERROR: SetMerkleBranch(): couldn't find tx in block\n");
        return 0;
    }

    // Is the tx in a block that's in the main chain
    BlockMap::iterator mi = mapBlockIndex.find(hashBlock);
    if (mi == mapBlockIndex.end())
        return 0;
    const CBlockIndex* pindex = (*mi).second;
    if (!pindex || !chainActive.Contains(pindex))
        return 0;

    return chainActive.Height() - pindex->nHeight + 1;
}

int CMerkleTx::GetDepthInMainChain(const CBlockIndex* &pindexRet) const
{
    if (hashUnset())
        return 0;

    AssertLockHeld(cs_main);

    // Find the block it claims to be in
    BlockMap::iterator mi = mapBlockIndex.find(hashBlock);
    if (mi == mapBlockIndex.end())
        return 0;
    CBlockIndex* pindex = (*mi).second;
    if (!pindex || !chainActive.Contains(pindex))
        return 0;

    pindexRet = pindex;
    return ((nIndex == -1) ? (-1) : 1) * (chainActive.Height() - pindex->nHeight + 1);
}

int CMerkleTx::GetBlocksToMaturity() const
{
    if (!(IsCoinBase() || IsCoinStake()) || GetBoolArg("-testnet",false))
        return 0;
    return max(0, (Params().GetConsensus().nCoinbaseMaturity+1) - GetDepthInMainChain());
}


bool CMerkleTx::AcceptToMemoryPool(bool fLimitFree, CAmount nAbsurdFee)
{
    CValidationState state;
    bool ret;
    if (GetBoolArg("-dandelion", true) && !(GetBoolArg("-devnet", false) && vNodes.size() == 0)) {
        ret = ::AcceptToMemoryPool(stempool, state, *this, fLimitFree /* pfMissingInputs */,
                                   nullptr /* plTxnReplaced */, false /* bypass_limits */, nAbsurdFee);
    } else {
        ret = ::AcceptToMemoryPool(mempool, state, *this, fLimitFree /* pfMissingInputs */,
                                   nullptr /* plTxnReplaced */, false /* bypass_limits */, nAbsurdFee);
        // Changes to mempool should also be made to Dandelion stempool
        CValidationState dummyState;
        ret &= ::AcceptToMemoryPool(stempool, dummyState, *this, fLimitFree /* pfMissingInputs */,
                                   nullptr /* plTxnReplaced */, false /* bypass_limits */, nAbsurdFee);
    }
    return ret;
}
