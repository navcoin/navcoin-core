// Copyright (c) 2020 The NavCoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "aggregationsession.h"
#include "main.h"

#include <algorithm>
#include <random>

std::set<uint256> setKnownSessions;
CCriticalSection cs_aggregation;

AggregationSession::AggregationSession(const CStateViewCache* inputsIn) : inputs(inputsIn), fState(0), nVersion(1)
{
}

bool AggregationSession::Start()
{
    if (fState && (es&&es->IsRunning()))
        return false;

    if (!inputs)
    {
        Stop();
        return false;
    }

    lock = true;

    es = new EphemeralServer([=](std::string& s) -> void {
        lock = false;
        LogPrint("aggregationsession", "AggregationSession::%s: created session %s\n", __func__, s);
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

bool AggregationSession::IsKnown(const AggregationSession& ms)
{
    return setKnownSessions.find(ms.GetHash()) != setKnownSessions.end();
}

void AggregationSession::Stop()
{
    if (es)
        es->Stop();
    fState = 0;
    RemoveDandelionAggregationSessionEmbargo(*this);
}

bool AggregationSession::GetState() const
{
    return fState && (es&&es->IsRunning());
}

bool AggregationSession::UpdateCandidateTransactions(const CTransaction &tx)
{
    {
        LOCK(cs_aggregation);

        for (auto& in: tx.vin)
        {
            vTransactionCandidates.erase(std::remove_if(vTransactionCandidates.begin(), vTransactionCandidates.end(), [in](CandidateTransaction x) {
                                             for (auto& in_candidate: x.tx.vin)
                                             {
                                                 if (in_candidate == in)
                                                 return true;
                                             }
                                             return false;
                                         }), vTransactionCandidates.end());
        }
    }

    return true;
}

bool AggregationSession::CleanCandidateTransactions()
{
    {
        AssertLockHeld(cs_aggregation);

        vTransactionCandidates.erase(std::remove_if(vTransactionCandidates.begin(), vTransactionCandidates.end(), [=](CandidateTransaction x) {
            if (!inputs->HaveInputs(x.tx))
            {
                return true;
            }

            if (CWalletTx(NULL, x.tx).InputsInMempool()) {
                return true;
            }

            if (CWalletTx(NULL, x.tx).InputsInStempool()) {
                return true;
            }
            return false;
        }), vTransactionCandidates.end());

        if (GetRandInt(10) == 0 && vTransactionCandidates.size() >= GetArg("-defaultmixin", DEFAULT_TX_MIXCOINS)*100)
            vTransactionCandidates.erase(vTransactionCandidates.begin(), vTransactionCandidates.begin() + GetArg("-defaultmixin", DEFAULT_TX_MIXCOINS));
    }

    return true;
}

bool AggregationSession::SelectCandidates(CandidateTransaction &ret)
{
    LOCK(cs_aggregation);

    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(vTransactionCandidates.begin(), vTransactionCandidates.end(), g);

    size_t nSelect = std::min(vTransactionCandidates.size(), (size_t)GetArg("-defaultmixin", DEFAULT_TX_MIXCOINS));

    if (nSelect == 0)
        return true;

    CAmount nFee = 0;

    CValidationState state;
    std::vector<RangeproofEncodedData> blsctData;
    std::vector<CTransaction> vTransactionsToCombine;

    unsigned int i = 0;
    unsigned int nSelected = 0;

    while (nSelected < nSelect && i < vTransactionCandidates.size())
    {
        if (!inputs->HaveInputs(vTransactionCandidates[i].tx))
        {
            i++;
            continue;
        }

        if (CWalletTx(NULL, vTransactionCandidates[i].tx).InputsInMempool()) {
            i++;
            continue;
        }

        if (CWalletTx(NULL, vTransactionCandidates[i].tx).InputsInStempool()) {
            i++;
            continue;
        }

        try
        {
            if (!VerifyBLSCT(vTransactionCandidates[i].tx, bls::PrivateKey::FromBN(Scalar::Rand().bn), blsctData, *inputs, state, false, vTransactionCandidates[i].fee))
            {
                i++;
                continue;
            }
        }
        catch(...)
        {
            i++;
            continue;
        }

        vTransactionsToCombine.push_back(vTransactionCandidates[i].tx);
        nFee += vTransactionCandidates[i].fee;
        i++;
        nSelected++;
    }

    CTransaction ctx;

    if (!CombineBLSCTTransactions(vTransactionsToCombine, ctx, *inputs, state, nFee))
        return error("AggregationSession::%s: Failed %s\n", state.GetRejectReason());

    ret = CandidateTransaction(ctx, nFee, DEFAULT_MIN_OUTPUT_AMOUNT, BulletproofsRangeproof());

    return true;
}

void AggregationSession::SetCandidateTransactions(std::vector<CandidateTransaction> candidates)
{
    vTransactionCandidates = candidates;
}

bool AggregationSession::AddCandidateTransaction(const std::vector<unsigned char>& v)
{
    LOCK(cs_aggregation);

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
        return error("AggregationSession::%s: Wrong serialization of transaction candidate\n", __func__);
    }

    if (!(tx.tx.IsBLSInput() && tx.tx.IsCTOutput() && tx.tx.vin.size() == 1 && tx.tx.vout.size() == 1 && tx.minAmountProofs.V.size() == tx.tx.vout.size()))
        return error("AggregationSession::%s: Received transaction is not BLSCT mix compliant\n", __func__);

    for (auto& it: vTransactionCandidates)
    {
        if (it.tx.vin == tx.tx.vin) // We already have this input
            return true;
    }

    std::vector<std::pair<int, BulletproofsRangeproof>> proofs;
    std::vector<bls::G1Element> nonces;
    std::vector<RangeproofEncodedData> blsctData;

    if (!inputs->HaveInputs(tx.tx))
        return error ("AggregationSession::%s: Received spent inputs\n", __func__);

    if (CWalletTx(NULL, tx.tx).InputsInMempool()) {
        return error ("AggregationSession::%s: Received transaction in mempool\n", __func__);
    }

    if (CWalletTx(NULL, tx.tx).InputsInStempool()) {
        return error ("AggregationSession::%s: Received transaction in stempool\n", __func__);
    }

    for (unsigned int i = 0; i < tx.tx.vout.size(); i++)
    {
        Scalar s = tx.minAmount;
        bls::G1Element l = (BulletproofsRangeproof::H*s.bn).Inverse();
        bls::G1Element r = tx.tx.vout[i].bp.V[0];
        l = l + r;
        if (!(l == tx.minAmountProofs.V[i]))
            return error ("AggregationSession::%s: Failed verification from output's amount %d\n", __func__, i);
        proofs.push_back(std::make_pair(i, tx.minAmountProofs));
    }

    if (!VerifyBulletproof(proofs, blsctData, nonces))
        return error("AggregationSession::%s: Failed verification of min amount range proofs\n", __func__);

    if (!MoneyRange(tx.fee))
        return error("AggregationSession::%s: Received transaction with incorrect fee %d\n", __func__, tx.fee);

    if (tx.fee > GetMaxFee())
        return error("AggregationSession::%s: Received transaction with too high fee %d\n", __func__, tx.fee);

    CValidationState state;

    try
    {
        if(!VerifyBLSCT(tx.tx, bls::PrivateKey::FromBN(Scalar::Rand().bn), blsctData, *inputs, state, false, tx.fee))
        {
            return error("AggregationSession::%s: Failed validation of transaction candidate %s\n", __func__, state.GetRejectReason());
        }
    }
    catch(...)
    {
        return error("AggregationSession::%s: Failed validation of transaction candidate %s. Catched exception.\n", __func__, state.GetRejectReason());
    }


    vTransactionCandidates.push_back(tx);

    LogPrint("aggregationsession", "AggregationSession::%s: received one candidate\n", __func__);

    return true;
}

void AggregationSession::AnnounceHiddenService()
{
    if (!GetBoolArg("-dandelion", true))
    {
        RelayAggregationSession(*this);
        return;
    }

    uint256 hash = GetHash();

    int64_t nCurrTime = GetTimeMicros();
    int64_t nEmbargo = 1000000*DANDELION_EMBARGO_MINIMUM+PoissonNextSend(nCurrTime, DANDELION_EMBARGO_AVG_ADD);
    InsertDandelionAggregationSessionEmbargo(*this,nEmbargo);

    if (!LocalDandelionDestinationPushAggregationSession(*this))
        RelayAggregationSession(*this);
}

std::string Socks5ErrorToString(int err)
{
    switch(err) {
        case 0x01: return "general failure";
        case 0x02: return "connection not allowed";
        case 0x03: return "network unreachable";
        case 0x04: return "host unreachable";
        case 0x05: return "connection refused";
        case 0x06: return "TTL expired";
        case 0x07: return "protocol error";
        case 0x08: return "address type not supported";
        default:   return "unknown";
    }
}

bool static IntRecv(char* data, size_t len, int timeout, SOCKET& hSocket)
{
    int64_t curTime = GetTimeMillis();
    int64_t endTime = curTime + timeout;
    // Maximum time to wait in one select call. It will take up until this time (in millis)
    // to break off in case of an interruption.
    const int64_t maxWait = 1000;
    while (len > 0 && curTime < endTime) {
        ssize_t ret = recv(hSocket, data, len, 0); // Optimistically try the recv first
        if (ret > 0) {
            len -= ret;
            data += ret;
        } else if (ret == 0) { // Unexpected disconnection
            return false;
        } else { // Other error or blocking
            int nErr = WSAGetLastError();
            if (nErr == WSAEINPROGRESS || nErr == WSAEWOULDBLOCK || nErr == WSAEINVAL) {
                if (!IsSelectableSocket(hSocket)) {
                    return false;
                }
                struct timeval tval = MillisToTimeval(std::min(endTime - curTime, maxWait));
                fd_set fdset;
                FD_ZERO(&fdset);
                FD_SET(hSocket, &fdset);
                int nRet = select(hSocket + 1, &fdset, nullptr, nullptr, &tval);
                if (nRet == SOCKET_ERROR) {
                    return false;
                }
            } else {
                return false;
            }
        }
        boost::this_thread::interruption_point();
        curTime = GetTimeMillis();
    }
    return len == 0;
}


bool AggregationSession::Join() const
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

    LogPrint("aggregationsession","AggregationSession::%s: new session %s\n", __func__, GetHiddenService());

    std::vector<COutput> vAvailableCoins;

    pwalletMain->AvailablePrivateCoins(vAvailableCoins, true, nullptr, false, DEFAULT_MIN_OUTPUT_AMOUNT);

    if (vAvailableCoins.size() == 0)
        return error("AggregationSession::%s: no coins available to offer", __func__);

    int nRand = 3+GetRandInt(3);

    if (GetRandInt(nRand+vAvailableCoins.size()) < nRand)
        return error("AggregationSession::%s: not sending this time", __func__);

    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(vAvailableCoins.begin(), vAvailableCoins.end(), g);

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
            return error("AggregationSession::%s: Could not get private key from blsct pool.\n",__func__);
        }

        ephemeralKey = bk.GetKey().Serialize();

        if (!pwalletMain->GetBLSCTSubAddressPublicKeys(prevcoin->vout[prevout].outputKey, prevcoin->vout[prevout].spendingKey, k))
        {
            return error("AggregationSession::%s: BLSCT keys not available\n", __func__);
        }

        if (!pwalletMain->GetBLSCTSubAddressSpendingKeyForOutput(prevcoin->vout[prevout].outputKey, prevcoin->vout[prevout].spendingKey, s))
        {
            return error("AggregationSession::%s: BLSCTed keys for subaddress not available\n", __func__);
        }

        if (!pwalletMain->GetBLSCTViewKey(v))
        {
            return error("AggregationSession::%s: BLSCT keys not available when getting view key\n", __func__);
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

    bls::G1Element nonce_;

    try
    {
        if (!CreateBLSCTOutput(bls::PrivateKey::FromBytes(ephemeralKey.data()), nonce_, newTxOut, k, prevcoin->vAmounts[prevout]+nAddedFee, "Mixing Reward", gammaOuts, strFailReason, true, vBLSSignatures))
        {
            return error("AggregationSession::%s: Error creating BLSCT output: %s\n",__func__, strFailReason);
        }
    }
    catch(...)
    {
        return error("AggregationSession::%s: Catched CreateBLSCTOutput exception.\n", __func__);
    }


    candidate.vout.push_back(newTxOut);

    try
    {
        // Balance Sig
        Scalar diff = gammaIns-gammaOuts;
        bls::PrivateKey balanceSigningKey = bls::PrivateKey::FromBN(diff.bn);
        candidate.vchBalanceSig = bls::BasicSchemeMPL::Sign(balanceSigningKey, balanceMsg).Serialize();
        // Tx Sig
        candidate.vchTxSig = bls::BasicSchemeMPL::Aggregate(vBLSSignatures).Serialize();
    }
    catch(...)
    {
        return error("AggregationSession::%s: Catched balanceSigningKey exception.\n", __func__);
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

    try
    {
        bprp.Prove(value, gammas, nonce, vMemo);
    }
    catch(...)
    {
        return error("AggregationSession::%s: Catched balanceSigningKey exception.\n", __func__);
    }

    std::vector<std::pair<int,BulletproofsRangeproof>> proofs;
    proofs.push_back(std::make_pair(0,bprp));

    if (!VerifyBulletproof(proofs, blsctData, nonces, false))
    {
        return error("AggregationSession::%s: Failed validation of range proof\n", __func__);
    }

    CandidateTransaction tx(candidate, nAddedFee, DEFAULT_MIN_OUTPUT_AMOUNT, bprp);

    if(!VerifyBLSCT(candidate, v.GetKey(), blsctData, *inputs, state, false, nAddedFee))
        return error("AggregationSession::%s: Failed validation of transaction candidate %s\n", __func__, state.GetRejectReason());

    size_t colonPos = GetHiddenService().find(':');

    if(colonPos != std::string::npos)
    {
        std::string host = GetHiddenService().substr(0,colonPos);
        std::string portPart = GetHiddenService().substr(colonPos+1);

        std::stringstream parser(portPart);

        int port = 0;
        if( parser >> port )
        {
            bool fFailed;

            proxyType proxy;
            GetProxy(NET_TOR, proxy);

            struct sockaddr_storage sockaddr;
            socklen_t len = sizeof(sockaddr);
            if (!proxy.proxy.GetSockAddr((struct sockaddr*)&sockaddr, &len)) {
                return error("%s: Could not create socket to %s:%d: unsupported network", __func__, host, port);
            }

            SOCKET so = socket(((struct sockaddr*)&sockaddr)->sa_family, SOCK_STREAM, IPPROTO_TCP);
            if (so == INVALID_SOCKET)
                return error("%s: Could not create socket to %s:%d", __func__, host, port);

            int set = 1;
#ifdef SO_NOSIGPIPE
            // Different way of disabling SIGPIPE on BSD
            setsockopt(so, SOL_SOCKET, SO_NOSIGPIPE, (void*)&set, sizeof(int));
#endif

            //Disable Nagle's algorithm
#ifdef WIN32
            setsockopt(so, IPPROTO_TCP, TCP_NODELAY, (const char*)&set, sizeof(int));
#else
            setsockopt(so, IPPROTO_TCP, TCP_NODELAY, (void*)&set, sizeof(int));
#endif

            // Set to non-blocking
            if (!SetSocketNonBlocking(so, true))
                return error("%s: Setting socket to non-blocking failed, error %s\n", __func__, NetworkErrorString(WSAGetLastError()));

            if (connect(so, (struct sockaddr*)&sockaddr, len) == SOCKET_ERROR)
            {
                int nErr = WSAGetLastError();
                // WSAEINVAL is here because some legacy version of winsock uses it
                if (nErr == WSAEINPROGRESS || nErr == WSAEWOULDBLOCK || nErr == WSAEINVAL)
                {
                    struct timeval timeout = MillisToTimeval(60);
                    fd_set fdset;
                    FD_ZERO(&fdset);
                    FD_SET(so, &fdset);
                    int nRet = select(so + 1, nullptr, &fdset, nullptr, &timeout);
                    if (nRet == 0)
                    {
                        LogPrint("%s: connection to %s timeout\n", __func__, proxy.proxy.ToString());
                        CloseSocket(so);
                        return false;
                    }
                    if (nRet == SOCKET_ERROR)
                    {
                        LogPrintf("%s: select() for %s failed: %s\n", __func__, proxy.proxy.ToString(), NetworkErrorString(WSAGetLastError()));
                        CloseSocket(so);
                        return false;
                    }
                    socklen_t nRetSize = sizeof(nRet);
#ifdef WIN32
                    if (getsockopt(so, SOL_SOCKET, SO_ERROR, (char*)(&nRet), &nRetSize) == SOCKET_ERROR)
#else
                    if (getsockopt(so, SOL_SOCKET, SO_ERROR, &nRet, &nRetSize) == SOCKET_ERROR)
#endif
                    {
                        LogPrintf("%s: getsockopt() for %s failed: %s\n", __func__, proxy.proxy.ToString(), NetworkErrorString(WSAGetLastError()));
                        CloseSocket(so);
                        return false;
                    }
                    if (nRet != 0)
                    {
                        LogPrintf("%s: connect() to %s failed after select(): %s\n", __func__, proxy.proxy.ToString(), NetworkErrorString(nRet));
                        CloseSocket(so);
                        return false;
                    }
                }
#ifdef WIN32
                else if (WSAGetLastError() != WSAEISCONN)
#else
                else
#endif
                {
                    LogPrintf("%s:connect() to %s failed: %s\n", __func__, proxy.proxy.ToString(), NetworkErrorString(WSAGetLastError()));
                    CloseSocket(so);
                    return false;
                }
            }

            LogPrint("net", "SOCKS5 connecting %s\n", host);
            if (host.size() > 255) {
                CloseSocket(so);
                return error("Hostname too long");
            }
            // Accepted authentication methods
            std::vector<uint8_t> vSocks5Init;
            vSocks5Init.push_back(0x05);
            vSocks5Init.push_back(0x01); // # METHODS
            vSocks5Init.push_back(0x00); // X'00' NO AUTHENTICATION REQUIRED
            ssize_t ret = send(so, (const char*)begin_ptr(vSocks5Init), vSocks5Init.size(), MSG_NOSIGNAL);
            if (ret != (ssize_t)vSocks5Init.size()) {
                CloseSocket(so);
                return error("Error sending to proxy");
            }
            char pchRet1[2];
            if (!IntRecv(pchRet1, 2, 20 * 1000, so)) {
                CloseSocket(so);
                LogPrintf("Socks5() connect to %s:%d failed: IntRecv() timeout or other failure\n", host, port);
                return false;
            }
            if (pchRet1[0] != 0x05) {
                CloseSocket(so);
                return error("Proxy failed to initialize");
            }
            if (pchRet1[1] == 0x00) {
                // Perform no authentication
            } else {
                CloseSocket(so);
                return error("Proxy requested wrong authentication method %02x", pchRet1[1]);
            }
            std::vector<uint8_t> vSocks5;
            vSocks5.push_back(0x05); // VER protocol version
            vSocks5.push_back(0x01); // CMD CONNECT
            vSocks5.push_back(0x00); // RSV Reserved
            vSocks5.push_back(0x03); // ATYP DOMAINNAME
            vSocks5.push_back(host.size()); // Length<=255 is checked at beginning of function
            vSocks5.insert(vSocks5.end(), host.begin(), host.end());
            vSocks5.push_back((port >> 8) & 0xFF);
            vSocks5.push_back((port >> 0) & 0xFF);
            ret = send(so, (const char*)begin_ptr(vSocks5), vSocks5.size(), MSG_NOSIGNAL);
            if (ret != (ssize_t)vSocks5.size()) {
                CloseSocket(so);
                return error("Error sending to proxy");
            }
            char pchRet2[4];
            if (!IntRecv(pchRet2, 4, 20 * 1000, so)) {
                CloseSocket(so);
                return error("Error reading proxy response");
            }
            if (pchRet2[0] != 0x05) {
                CloseSocket(so);
                return error("Proxy failed to accept request");
            }
            if (pchRet2[1] != 0x00) {
                // Failures to connect to a peer that are not proxy errors
                CloseSocket(so);
                LogPrintf("Socks5() connect to %s:%d failed: %s\n", host, port, Socks5ErrorToString(pchRet2[1]));
                return false;
            }
            if (pchRet2[2] != 0x00) {
                CloseSocket(so);
                return error("Error: malformed proxy response");
            }
            char pchRet3[256];
            switch (pchRet2[3])
            {
                case 0x01: ret = IntRecv(pchRet3, 4, 20 * 1000, so); break;
                case 0x04: ret = IntRecv(pchRet3, 16, 20 * 1000, so); break;
                case 0x03:
                {
                    ret = IntRecv(pchRet3, 1, 20 * 1000, so);
                    if (!ret) {
                        CloseSocket(so);
                        return error("Error reading from proxy");
                    }
                    int nRecv = pchRet3[0];
                    ret = IntRecv(pchRet3, nRecv, 20 * 1000, so);
                    break;
                }
                default: CloseSocket(so); return error("Error: malformed proxy response");
            }
            if (!ret) {
                CloseSocket(so);
                return error("Error reading from proxy");
            }
            if (!IntRecv(pchRet3, 2, 20 * 1000, so)) {
                CloseSocket(so);
                return error("Error reading from proxy");
            }
            LogPrint("net", "SOCKS5 connected %s\n", host);

            if (!IsSelectableSocket(so)) {
                CloseSocket(so);
                return error("AggregationSession::%s: Cannot create connection: non-selectable socket created (fd >= FD_SETSIZE ?)\n", __func__);
            }

            CDataStream ds(0,0);
            ds << tx;
            std::vector<unsigned char> vBuffer(ds.begin(), ds.end());
            vBuffer.push_back('\0');

            ret = send(so, reinterpret_cast<const char *>(vBuffer.data()), vBuffer.size(), MSG_NOSIGNAL);
            LogPrintf("AggregationSession::%s: Sent %d bytes\n", __func__, ret);

            CloseSocket(so);

            LockOutputFor(prevcoin->GetHash(), prevout, 60);
            pwalletMain->NotifyTransactionChanged(pwalletMain, prevcoin->GetHash(), CT_UPDATED);

            return ret;
        }
    }

    return false;
}

CAmount AggregationSession::GetDefaultFee()
{
    return GetArg("-aggregationfee", DEFAULT_MIX_FEE);
}

CAmount AggregationSession::GetMaxFee()
{
    return GetArg("-aggregationmaxfee", DEFAULT_MAX_MIX_FEE);
}

void AggregationSessionThread()
{
    LogPrintf("NavCoinCandidateCoinsThread started\n");
    SetThreadPriority(THREAD_PRIORITY_LOWEST);
    RenameThread("navcoin-candidate-coins-thread");

    try {
        while (true) {
            do {
                bool fvNodesEmpty;
                {
                    LOCK(cs_vNodes);
                    fvNodesEmpty = vNodes.empty();
                }
                if (!fvNodesEmpty && !IsInitialBlockDownload())
                    break;
                MilliSleep(1000);
            } while (true);

            MilliSleep(GetRand(120*1000));

            {
                LOCK2(pwalletMain->cs_wallet, cs_aggregation);

                pwalletMain->aggSession->CleanCandidateTransactions();
                pwalletMain->WriteCandidateTransactions();
            }

            {
                LOCK(pwalletMain->cs_wallet);
                if (pwalletMain->aggSession->GetTransactionCandidates().size() >= GetArg("-defaultmixin", DEFAULT_TX_MIXCOINS)*100)
                    continue;

                if (!pwalletMain->aggSession->Start())
                {
                    pwalletMain->aggSession->Stop();
                    if (!pwalletMain->aggSession->Start())
                    {
                        continue;
                    }
                }
            }

            MilliSleep(GetRand(600*1000));
        }
    }
    catch (const boost::thread_interrupted&)
    {
        LogPrintf("NavCoinCandidateCoinsThread terminated\n");
        throw;
    }
    catch (const std::runtime_error &e)
    {
        LogPrintf("NavCoinCandidateCoinsThread runtime error: %s\n", e.what());
        return;
    }
}
