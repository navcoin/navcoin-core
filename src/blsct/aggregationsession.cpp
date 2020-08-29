// Copyright (c) 2020 The NavCoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "aggregationsession.h"

std::set<uint256> setKnownSessions;

AggregationSesion::AggregationSesion(const CStateViewCache* inputsIn) : inputs(inputsIn), fState(0), nVersion(1)
{
}

bool AggregationSesion::Start()
{
    if (fState && (es&&es->IsRunning()))
        return false;

    if (!inputs)
    {
        Stop();
        return false;
    }

    vTransactionCandidates.clear();

    lock = true;

    es = new EphemeralServer([=](std::string& s) -> void {
        lock = false;
        sHiddenService = s;
        if (!fState)
            AnnounceHiddenService();
        fState = !sHiddenService.empty();
    }, [=](std::vector<unsigned char>& v) -> void {
        AddCandidateTransaction(v);
    });

    while(lock) {}

    return true;
}

bool AggregationSesion::IsKnown(const AggregationSesion& ms)
{
    return setKnownSessions.find(ms.GetHash()) != setKnownSessions.end();
}

void AggregationSesion::Stop()
{
    if (es)
        es->Stop();
    fState = 0;
    vTransactionCandidates.clear();
    RemoveDandelionAggregationSesionEmbargo(*this);
}

bool AggregationSesion::GetState() const
{
    return fState && (es&&es->IsRunning());
}

bool AggregationSesion::SelectCandidates(CandidateTransaction &ret)
{
    random_shuffle(vTransactionCandidates.begin(), vTransactionCandidates.end(), GetRandInt);

    size_t nSelect = std::min(vTransactionCandidates.size(), (size_t)DEFAULT_TX_MIXCOINS);

    if (nSelect == 0)
        return false;

    CAmount nFee = 0;

    CValidationState state;
    std::vector<RangeproofEncodedData> blsctData;
    std::vector<CTransaction> vTransactionsToCombine;

    unsigned int i = 0;

    while (i < nSelect && i < vTransactionCandidates.size())
    {
        try
        {
            if (!VerifyBLSCT(vTransactionCandidates[i].tx, bls::PrivateKey::FromBN(Scalar::Rand().bn), blsctData, *inputs, state, false, vTransactionCandidates[i].fee))
                continue;
        }
        catch(...)
        {
            continue;
        }

        vTransactionsToCombine.push_back(vTransactionCandidates[i].tx);
        nFee += vTransactionCandidates[i].fee;
        i++;
    }

    CTransaction ctx;

    if (!CombineBLSCTTransactions(vTransactionsToCombine, ctx, *inputs, state, nFee))
        return error("AggregationSesion::%s: Failed %s\n", state.GetRejectReason());

    ret = CandidateTransaction(ctx, nFee, DEFAULT_MIN_OUTPUT_AMOUNT, BulletproofsRangeproof());

    return true;
}

bool AggregationSesion::AddCandidateTransaction(const std::vector<unsigned char>& v)
{
    if (!inputs)
        return false;

    CandidateTransaction tx;
    CDataStream ss(std::vector<unsigned char>(v.begin(), v.end()), 0, 0);
    try
    {
        ss >> tx;
    }
    catch(...)
    {
        return error("AggregationSesion::%s: Wrong serialization of transaction candidate\n", __func__);
    }

    if (!(tx.tx.IsBLSInput() && tx.tx.IsCTOutput() && tx.tx.vin.size() == 1 && tx.tx.vout.size() == 1 && tx.minAmountProofs.V.size() == tx.tx.vout.size()))
        return error("AggregationSesion::%s: Received transaction is not BLSCT mix compliant\n", __func__);

    std::vector<std::pair<int, BulletproofsRangeproof>> proofs;
    std::vector<bls::G1Element> nonces;
    std::vector<RangeproofEncodedData> blsctData;

    for (unsigned int i = 0; i < tx.tx.vout.size(); i++)
    {
        Scalar s = tx.minAmount;
        bls::G1Element l = BulletproofsRangeproof::H*s.bn;
        bls::G1Element r = tx.tx.vout[i].bp.V[0].Inverse();
        l = l + r;
        if (!(l == tx.minAmountProofs.V[i]))
            return error ("AggregationSesion::%s: Failed verification from output's amount %d\n", __func__, i);
        proofs.push_back(std::make_pair(i, tx.minAmountProofs));
    }

    if (!VerifyBulletproof(proofs, blsctData, nonces))
        return error("AggregationSesion::%s: Failed verification of min amount range proofs\n", __func__);

    if (!MoneyRange(tx.fee))
        return error("AggregationSesion::%s: Received transaction with incorrect fee %d\n", __func__, tx.fee);

    if (tx.fee > GetMaxFee())
        return error("AggregationSesion::%s: Received transaction with too high fee %d\n", __func__, tx.fee);

    CValidationState state;

    try
    {
        if(!VerifyBLSCT(tx.tx, bls::PrivateKey::FromBN(Scalar::Rand().bn), blsctData, *inputs, state, false, tx.fee))
        {
            return error("AggregationSesion::%s: Failed validation of transaction candidate %s\n", __func__, state.GetRejectReason());
        }
    }
    catch(...)
    {
        return error("AggregationSesion::%s: Failed validation of transaction candidate %s. Catched exception.\n", __func__, state.GetRejectReason());
    }


    vTransactionCandidates.push_back(tx);

    return true;
}

void AggregationSesion::AnnounceHiddenService()
{
    if (!GetBoolArg("-dandelion", true))
    {
        Stop();
    }
    else
    {
        uint256 hash = GetHash();

        int64_t nCurrTime = GetTimeMicros();
        int64_t nEmbargo = 1000000*DANDELION_EMBARGO_MINIMUM+PoissonNextSend(nCurrTime, DANDELION_EMBARGO_AVG_ADD);
        InsertDandelionAggregationSesionEmbargo(*this,nEmbargo);

        if (vNodes.size() <= 1)
            RelayAggregationSesion(*this);
        else if (!LocalDandelionDestinationPushAggregationSesion(*this))
            Stop();
    }
}

bool AggregationSesion::Join() const
{
    setKnownSessions.insert(this->GetHash());

    if (!pwalletMain)
        return false;

    {
        LOCK(pwalletMain->cs_wallet);
        if (*(pwalletMain->aggSession) == *this)
            return false;
    }

    if (!GetBoolArg("-blsctmix", DEFAULT_MIX))
        return false;

    LogPrint("AggregationSesion","AggregationSesion::%s: joining %s\n", __func__, GetHiddenService());

    std::vector<COutput> vAvailableCoins;

    pwalletMain->AvailablePrivateCoins(vAvailableCoins, true, nullptr, false, DEFAULT_MIN_OUTPUT_AMOUNT);

    if (vAvailableCoins.size() == 0)
        return false;

    int nRand = 3+GetRandInt(3);

    if (GetRandInt(nRand+vAvailableCoins.size()) < nRand)
        return false;

    random_shuffle(vAvailableCoins.begin(), vAvailableCoins.end(), GetRandInt);

    const CWalletTx *prevcoin = vAvailableCoins[0].tx;
    int prevout = vAvailableCoins[0].i;

    CAmount nAddedFee = GetDefaultFee();

    std::vector<bls::G2Element> vBLSSignatures;
    Scalar gammaIns = prevcoin->vGammas[prevout];
    Scalar gammaOuts = 0;

    CMutableTransaction candidate;
    candidate.nVersion = TX_BLS_INPUT_FLAG | TX_BLS_CT_FLAG;

    blsctDoublePublicKey k;
    blsctPublicKey pk;
    blsctKey bk, v, s;

    std::vector<shared_ptr<CReserveBLSCTBlindingKey>> reserveBLSCTKey;
    shared_ptr<CReserveBLSCTBlindingKey> rk(new CReserveBLSCTBlindingKey(pwalletMain));
    reserveBLSCTKey.insert(reserveBLSCTKey.begin(), std::move(rk));

    reserveBLSCTKey[0]->GetReservedKey(pk);

    std::vector<unsigned char> ephemeralKey;

    {
        LOCK(pwalletMain->cs_wallet);

        if (!pwalletMain->GetBLSCTBlindingKey(pk, bk))
        {
            return error("AggregationSesion::%s: Could not get private key from blsct pool.\n",__func__);
        }

        ephemeralKey = bk.GetKey().Serialize();

        if (!pwalletMain->GetBLSCTSubAddressPublicKeys(prevcoin->vout[prevout].outputKey, prevcoin->vout[prevout].spendingKey, k))
        {
            return error("AggregationSesion::%s: BLSCT keys not available\n", __func__);
        }

        if (!pwalletMain->GetBLSCTSubAddressSpendingKeyForOutput(prevcoin->vout[prevout].outputKey, prevcoin->vout[prevout].spendingKey, s))
        {
            return error("AggregationSesion::%s: BLSCT keys not available\n", __func__);
        }

        if (!pwalletMain->GetBLSCTViewKey(v))
        {
            return error("AggregationSesion::%s: BLSCT keys not available when getting view key\n", __func__);
        }
    }

    // Input
    candidate.vin.push_back(CTxIn(prevcoin->GetHash(), prevout, CScript(),
                              std::numeric_limits<unsigned int>::max()-1));

    if (prevcoin->vout[prevout].ephemeralKey.size() == 0)
        return error("MixSession::%s: prevout ephemeralkey length is 0\n", __func__);

    try
    {
        SignBLSInput(s.GetKey(), candidate.vin[0], vBLSSignatures);
    }
    catch(...)
    {
        return error("MixSession::%s: Catched signing key exception.\n", __func__);
    }

    // Output
    CTxOut newTxOut(0, CScript());

    std::string strFailReason;

    try
    {
        if (!CreateBLSCTOutput(bls::PrivateKey::FromBytes(ephemeralKey.data()), newTxOut, k, prevcoin->vAmounts[prevout]+nAddedFee, "Mixing Reward", gammaOuts, strFailReason, true, vBLSSignatures))
        {
            return error("AggregationSesion::%s: Error creating BLSCT output: %s\n",__func__, strFailReason);
        }
    }
    catch(...)
    {
        return error("AggregationSesion::%s: Catched CreateBLSCTOutput exception.\n", __func__);
    }


    candidate.vout.push_back(newTxOut);

    try
    {
        // Balance Sig
        Scalar diff = gammaIns-gammaOuts;
        bls::PrivateKey balanceSigningKey = bls::PrivateKey::FromBN(diff.bn);
        candidate.vchBalanceSig = bls::BasicSchemeMPL::Sign(balanceSigningKey, balanceMsg);
        // Tx Sig
        candidate.vchTxSig = bls::BasicSchemeMPL::Aggregate(vBLSSignatures).Serialize();
    }
    catch(...)
    {
        return error("AggregationSesion::%s: Catched balanceSigningKey exception.\n", __func__);
    }

    CValidationState state;

    // Range proof of min amount

    std::vector<RangeproofEncodedData> blsctData;
    BulletproofsRangeproof bprp;

    std::vector<Scalar> value;
    value.push_back(prevcoin->vAmounts[prevout]+nAddedFee-DEFAULT_MIN_OUTPUT_AMOUNT);

    bls::G1Element nonce = bls::PrivateKey::FromBN(Scalar::Rand().bn).GetG1Element();
    std::vector<bls::G1Element> nonces;
    nonces.push_back(nonce);

    std::vector<Scalar> gammas;
    gammas.push_back(gammaOuts);

    std::vector<unsigned char> vMemo;

    bprp.Prove(value, gammas, nonce, vMemo);

    std::vector<std::pair<int,BulletproofsRangeproof>> proofs;
    proofs.push_back(std::make_pair(0,bprp));

    if (!VerifyBulletproof(proofs, blsctData, nonces, false))
    {
        return error("AggregationSesion::%s: Failed validation of range proof\n", __func__);
    }

    CandidateTransaction tx(candidate, nAddedFee, DEFAULT_MIN_OUTPUT_AMOUNT, bprp);

    if(!VerifyBLSCT(candidate, v.GetKey(), blsctData, *inputs, state, false, nAddedFee))
        return error("AggregationSesion::%s: Failed validation of transaction candidate %s\n", __func__, state.GetRejectReason());

    size_t colonPos = GetHiddenService().find(':');

    if(colonPos != std::string::npos)
    {
         std::string host = GetHiddenService().substr(0,colonPos);
         std::string portPart = GetHiddenService().substr(colonPos+1);

         std::stringstream parser(portPart);

         int port = 0;
         if( parser >> port )
         {
             SOCKET so = INVALID_SOCKET;
             bool fFailed;

             proxyType proxy;
             GetProxy(NET_TOR, proxy);

             // first connect to proxy server
             if (!ConnectSocketDirectly(proxy.proxy, so, 60)) {
                 return error("AggregationSesion::%s: Failed to join session %s:%d, could not connect to tor socks proxy\n", __func__, host, port);
             }

             // do socks negotiation
             if (!Socks5(host, (unsigned short)port, 0, so))
                 return error("AggregationSesion::%s: Failed to join session %s:%d, could not negotiate with tor socks proxy\n", __func__, host, port);

             if (!IsSelectableSocket(so)) {
                 CloseSocket(so);
                 return error("AggregationSesion::%s: Cannot create connection: non-selectable socket created (fd >= FD_SETSIZE ?)\n", __func__);
             }

             CDataStream ds(0,0);
             ds << tx;
             std::vector<unsigned char> vBuffer(ds.begin(), ds.end());
             vBuffer.push_back('\0');

             auto ret = send(so, reinterpret_cast<const char *>(vBuffer.data()), vBuffer.size(), MSG_NOSIGNAL);
             LogPrintf("AggregationSesion::%s: Sent %d bytes\n", __func__, ret);

             CloseSocket(so);

             LockOutputFor(prevcoin->GetHash(), prevout, 60);
             pwalletMain->NotifyTransactionChanged(pwalletMain, prevcoin->GetHash(), CT_UPDATED);

             return ret;
         }
    }

    return false;
}

CAmount AggregationSesion::GetDefaultFee()
{
    return GetArg("-aggregationfee", DEFAULT_MIX_FEE);
}

CAmount AggregationSesion::GetMaxFee()
{
    return GetArg("-aggregationmaxfee", DEFAULT_MAX_MIX_FEE);
}
