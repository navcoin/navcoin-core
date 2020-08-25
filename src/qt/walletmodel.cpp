// Copyright (c) 2011-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/walletmodel.h>

#include <qt/addresstablemodel.h>
#include <qt/guiconstants.h>
#include <qt/guiutil.h>
#include <qt/recentrequeststablemodel.h>
#include <qt/transactiontablemodel.h>

#include <base58.h>
#include <keystore.h>
#include <main.h>
#include <sync.h>
#include <ui_interface.h>
#include <utils/dns_utils.h>
#include <wallet/wallet.h>
#include <wallet/walletdb.h> // for BackupWallet

#include <stdint.h>

#include <QDebug>
#include <QSet>
#include <QTimer>

WalletModel::WalletModel(const PlatformStyle *platformStyle, CWallet *wallet, OptionsModel *optionsModel, QObject *parent) :
    QObject(parent), wallet(wallet), optionsModel(optionsModel), addressTableModel(0),
    transactionTableModel(0),
    recentRequestsTableModel(0),
    cachedBalance(0), cachedUnconfirmedBalance(0), cachedImmatureBalance(0), cachedColdStakingBalance(0),
    cachedPrivateBalance(0),
    cachedEncryptionStatus(Unencrypted),
    cachedNumBlocks(0)
{
    fHaveWatchOnly = wallet->HaveWatchOnly();
    fForceCheckBalanceChanged = false;

    addressTableModel = new AddressTableModel(wallet, this);
    transactionTableModel = new TransactionTableModel(platformStyle, wallet, this);
    recentRequestsTableModel = new RecentRequestsTableModel(wallet, this);

    // This timer will be fired repeatedly to update the balance
    pollTimer = new QTimer(this);
    connect(pollTimer, SIGNAL(timeout()), this, SLOT(pollBalanceChanged()));
    pollTimer->start(MODEL_UPDATE_DELAY);

    subscribeToCoreSignals();
}

WalletModel::~WalletModel()
{
    unsubscribeFromCoreSignals();
}

CAmount WalletModel::getBalance(const CCoinControl *coinControl) const
{
    if (coinControl)
    {
        CAmount nBalance = 0;
        std::vector<COutput> vCoins;
        wallet->AvailableCoins(vCoins, true, coinControl);
        for(const COutput& out: vCoins)
            if(out.fSpendable)
                nBalance += out.tx->vout[out.i].nValue;

        return nBalance;
    }

    return wallet->GetBalance();
}

CAmount WalletModel::getUnconfirmedBalance() const
{
    return wallet->GetUnconfirmedBalance();
}

CAmount WalletModel::getPrivateBalance() const
{
    return wallet->GetPrivateBalance();
}

CAmount WalletModel::getPrivateBalancePending() const
{
    return wallet->GetPrivateBalancePending();
}

CAmount WalletModel::getPrivateBalanceLocked() const
{
    return wallet->GetPrivateBalanceLocked();
}

CAmount WalletModel::getStake() const
{
    return wallet->GetStake();
}

CAmount WalletModel::getImmatureBalance() const
{
    return wallet->GetImmatureBalance();
}

CAmount WalletModel::getColdStakingBalance() const
{
    return wallet->GetColdStakingBalance();
}

bool WalletModel::haveWatchOnly() const
{
    return fHaveWatchOnly;
}

CAmount WalletModel::getWatchBalance() const
{
    return wallet->GetWatchOnlyBalance();
}

CAmount WalletModel::getWatchUnconfirmedBalance() const
{
    return wallet->GetUnconfirmedWatchOnlyBalance();
}

CAmount WalletModel::getWatchImmatureBalance() const
{
    return wallet->GetImmatureWatchOnlyBalance();
}

void WalletModel::updateStatus()
{
    EncryptionStatus newEncryptionStatus = getEncryptionStatus();

    if(cachedEncryptionStatus != newEncryptionStatus)
        Q_EMIT encryptionStatusChanged(newEncryptionStatus);
}

void WalletModel::pollBalanceChanged()
{
    // Get required locks upfront. This avoids the GUI from getting stuck on
    // periodical polls if the core is holding the locks for a longer time -
    // for example, during a wallet rescan.
    TRY_LOCK(cs_main, lockMain);
    if(!lockMain)
        return;
    TRY_LOCK(wallet->cs_wallet, lockWallet);
    if(!lockWallet)
        return;

    if(fForceCheckBalanceChanged || chainActive.Height() != cachedNumBlocks)
    {
        fForceCheckBalanceChanged = false;

        // Balance and number of transactions might have changed
        cachedNumBlocks = chainActive.Height();

        checkBalanceChanged();
        if(transactionTableModel)
            transactionTableModel->updateConfirmations();
    }
}

void WalletModel::checkBalanceChanged()
{
    CAmount newBalance = getBalance();
    CAmount newUnconfirmedBalance = getUnconfirmedBalance();
    CAmount newImmatureBalance = getImmatureBalance();
    CAmount newStakingBalance = getStake();
    CAmount newColdStakingBalance = getColdStakingBalance();
    CAmount newPrivateBalance = getPrivateBalance();
    CAmount newPrivateBalancePending = getPrivateBalancePending();
    CAmount newPrivateBalanceLocked = getPrivateBalanceLocked();
    CAmount newWatchOnlyBalance = 0;
    CAmount newWatchUnconfBalance = 0;
    CAmount newWatchImmatureBalance = 0;
    if (haveWatchOnly())
    {
        newWatchOnlyBalance = getWatchBalance();
        newWatchUnconfBalance = getWatchUnconfirmedBalance();
        newWatchImmatureBalance = getWatchImmatureBalance();
    }

    if(cachedBalance != newBalance || cachedUnconfirmedBalance != newUnconfirmedBalance || cachedImmatureBalance != newImmatureBalance ||
        cachedWatchOnlyBalance != newWatchOnlyBalance || cachedWatchUnconfBalance != newWatchUnconfBalance || cachedWatchImmatureBalance != newWatchImmatureBalance ||
            cachedStakingBalance != newStakingBalance || cachedColdStakingBalance != newColdStakingBalance || cachedPrivateBalance != newPrivateBalance ||
            cachedPrivateBalancePending != newPrivateBalancePending || cachedPrivateBalanceLocked != newPrivateBalanceLocked)
    {
        cachedBalance = newBalance;
        cachedUnconfirmedBalance = newUnconfirmedBalance;
        cachedStakingBalance = newStakingBalance;
        cachedColdStakingBalance = newColdStakingBalance;
        cachedImmatureBalance = newImmatureBalance;
        cachedWatchOnlyBalance = newWatchOnlyBalance;
        cachedWatchUnconfBalance = newWatchUnconfBalance;
        cachedWatchImmatureBalance = newWatchImmatureBalance;
        cachedPrivateBalance = newPrivateBalance;
        cachedPrivateBalancePending = newPrivateBalancePending;
        cachedPrivateBalanceLocked = newPrivateBalanceLocked;
        Q_EMIT balanceChanged(newBalance, newUnconfirmedBalance, newStakingBalance, newImmatureBalance,
                            newWatchOnlyBalance, newWatchUnconfBalance, newWatchImmatureBalance, newColdStakingBalance, newPrivateBalance,
                            newPrivateBalancePending, newPrivateBalanceLocked);
    }
}

void WalletModel::updateTransaction()
{
    // Balance and number of transactions might have changed
    fForceCheckBalanceChanged = true;
}

void WalletModel::updateAddressBook(const QString &address, const QString &label,
        bool isMine, const QString &purpose, int status)
{
    if(addressTableModel)
        addressTableModel->updateEntry(address, label, isMine, purpose, status);
}

void WalletModel::updateWatchOnlyFlag(bool fHaveWatchonly)
{
    fHaveWatchOnly = fHaveWatchonly;
    Q_EMIT notifyWatchonlyChanged(fHaveWatchonly);
}

bool WalletModel::validateAddress(const QString &address)
{
  std::string address_str = address.toStdString();
  utils::DNSResolver* DNS = nullptr;;

  // Validate the passed NavCoin address
  if(DNS->check_address_syntax(address_str.c_str()))
  {

    bool dnssec_valid; bool dnssec_available;
    std::vector<std::string> addresses = utils::dns_utils::addresses_from_url(address_str.c_str(), dnssec_available, dnssec_valid);

    if(addresses.empty() || (!dnssec_valid && GetBoolArg("-requirednssec",true)))
      return false;
    else
      address_str = addresses.front();

  }

  CNavCoinAddress addressParsed(address_str);
  return addressParsed.IsValid();
}

WalletModel::SendCoinsReturn WalletModel::prepareTransaction(WalletModelTransaction &transaction, CAmount& total, const CCoinControl *coinControl, const CandidateTransaction* selectedCoins)
{
    total = 0;
    bool fSubtractFeeFromAmount = false;
    QList<SendCoinsRecipient> recipients = transaction.getRecipients();
    std::vector<CRecipient> vecSend;

    if(recipients.empty())
    {
        return OK;
    }

    QSet<QString> setAddress; // Used to detect duplicates
    int nAddresses = 0;
    bool fPrivate = false;
    bool fAnyBLSCT = false;

    // Pre-check input data for validity
    for(const SendCoinsRecipient &rcp: recipients)
    {
        if (rcp.fSubtractFeeFromAmount)
            fSubtractFeeFromAmount = true;

        if (rcp.isanon)
            fPrivate = true;

        // User-entered navcoin address / amount:
        if(!validateAddress(rcp.address))
        {
            return InvalidAddress;
        }
        if(rcp.amount <= 0)
        {
            return InvalidAmount;
        }
        setAddress.insert(rcp.address);
        ++nAddresses;

        CTxDestination address = CNavCoinAddress(rcp.address.toStdString()).Get();
        CScript scriptPubKey = GetScriptForDestination(address);
        bool fBLSCT = address.type() == typeid(blsctDoublePublicKey);
        CRecipient recipient = {scriptPubKey, rcp.amount, rcp.fSubtractFeeFromAmount, fBLSCT};
        fAnyBLSCT |= fBLSCT;
        if (fBLSCT)
        {
            bls::G1Element vk = bls::G1Element::Infinity();
            bls::G1Element sk = bls::G1Element::Infinity();

            if (!boost::get<blsctDoublePublicKey>(address).GetSpendKey(sk) || !boost::get<blsctDoublePublicKey>(address).GetViewKey(vk))
            {
                return InvalidAddress;
            }
            recipient.sk = sk.Serialize();
            recipient.vk = vk.Serialize();
            recipient.sMemo = rcp.message.toStdString();
        }
        vecSend.push_back(recipient);

        total += rcp.amount;

    }
    if(setAddress.size() != nAddresses)
    {
        return DuplicateAddress;
    }

    CAmount nBalance = fPrivate ? getPrivateBalance() : getBalance(coinControl);

    if(total > nBalance)
    {
        return AmountExceedsBalance;
    }

    transaction.vecSend = vecSend;

    int i = 0;

    for(const CRecipient &rcp: vecSend)
    {
        LOCK2(cs_main, wallet->cs_wallet);
        std::vector<CRecipient> vec;
        vec.push_back(rcp);
        transaction.newPossibleKeyChange(wallet);
        transaction.newPossibleBLSCTBlindingKey(wallet);

        CAmount nFeeRequired = 0;
        int nChangePosRet = -1;
        std::string strFailReason;
        bool fCreated;
        CReserveKey *keyChange = transaction.getPossibleKeyChange();
        std::vector<shared_ptr<CReserveBLSCTBlindingKey>> *reserveBLSCTKey = transaction.getPossibleBLSCTBlindingKey();

        CWalletTx *newTx;
        newTx = new CWalletTx();

        fCreated = wallet->CreateTransaction(vec, *newTx, *keyChange, *reserveBLSCTKey, nFeeRequired, nChangePosRet, strFailReason, fPrivate, coinControl, true, selectedCoins);
        if (newTx->fSpendsColdStaking)
            transaction.fSpendsColdStaking = true;
        transaction.vTransactions.push_back(*newTx);

        transaction.setTransactionFee(nFeeRequired);
        if (fSubtractFeeFromAmount && fCreated)
           transaction.reassignAmounts(nChangePosRet,newTx,i);

        if(!fCreated)
        {
            if(!fSubtractFeeFromAmount && (total + nFeeRequired) > nBalance)
            {
                return SendCoinsReturn(AmountWithFeeExceedsBalance);
            }
            Q_EMIT message(tr("Send Coins"), QString::fromStdString(strFailReason),
                         CClientUIInterface::MSG_ERROR);
            return TransactionCreationFailed;
        }

        // reject absurdly high fee. (This can never happen because the
        // wallet caps the fee at maxTxFee. This merely serves as a
        // belt-and-suspenders check)
        if (nFeeRequired > maxTxFee)
            return AbsurdFee;

        i++;
    }

    return SendCoinsReturn(OK);
}

WalletModel::SendCoinsReturn WalletModel::sendCoins(WalletModelTransaction &transaction, const bool& fPrivate, const CCoinControl *coinControl, const CandidateTransaction* selectedCoins)
{
    QByteArray transaction_array; /* store serialized transaction */

    {
        LOCK2(cs_main, wallet->cs_wallet);

        for(const CRecipient &rcp: transaction.vecSend)
        {

          for(const CWalletTx newTx: transaction.vTransactions)
          {

            for(const SendCoinsRecipient &rcp: transaction.getRecipients())
            {
              if (!rcp.message.isEmpty()) // Message from normal navcoin:URI (navcoin:123...?message=example)
                const_cast<CWalletTx&>(newTx).vOrderForm.push_back(make_pair("Message", rcp.message.toStdString()));
            }
          }

          CAmount nFeeRequired = 0;
          int nChangePosRet = -1;
          std::string strFailReason;
          CReserveKey *keyChange = transaction.getPossibleKeyChange();
          std::vector<shared_ptr<CReserveBLSCTBlindingKey>> *reserveBLSCTKey = transaction.getPossibleBLSCTBlindingKey();

          CWalletTx *wTx;
          wTx = new CWalletTx();

          std::vector<CRecipient> vec;
          vec.push_back(rcp);

          wallet->CreateTransaction(vec, *wTx, *keyChange, *reserveBLSCTKey, nFeeRequired, nChangePosRet, strFailReason, fPrivate, coinControl, true, selectedCoins);

          if(!wallet->CommitTransaction(*wTx, *keyChange, *reserveBLSCTKey))
            return TransactionCommitFailed;

          CTransaction* t = (CTransaction*)wTx;
          CDataStream ssTx(SER_NETWORK, PROTOCOL_VERSION);
          ssTx << *t;
          transaction_array.append(&(ssTx[0]), ssTx.size());
        }
    }

    // Add addresses / update labels that we've sent to to the address book,
    // and emit coinsSent signal for each recipient
    for(const SendCoinsRecipient &rcp: transaction.getRecipients())
    {
        std::string strAddress = rcp.address.toStdString();
        CTxDestination dest = CNavCoinAddress(strAddress).Get();
        std::string strLabel = rcp.label.toStdString();
        {
            LOCK(wallet->cs_wallet);

            std::map<CTxDestination, CAddressBookData>::iterator mi = wallet->mapAddressBook.find(dest);

            // Check if we have a new address or an updated label
            if (mi == wallet->mapAddressBook.end())
            {
                wallet->SetAddressBook(dest, strLabel, "send");
            }
            else if (mi->second.name != strLabel)
            {
                wallet->SetAddressBook(dest, strLabel, ""); // "" means don't change purpose
            }
        }
        Q_EMIT coinsSent(wallet, rcp, transaction_array);
    }
    checkBalanceChanged(); // update balance immediately, otherwise there could be a short noticeable delay until pollBalanceChanged hits

    return SendCoinsReturn(OK);
}

OptionsModel *WalletModel::getOptionsModel()
{
    return optionsModel;
}

AddressTableModel *WalletModel::getAddressTableModel()
{
    return addressTableModel;
}

TransactionTableModel *WalletModel::getTransactionTableModel()
{
    return transactionTableModel;
}

RecentRequestsTableModel *WalletModel::getRecentRequestsTableModel()
{
    return recentRequestsTableModel;
}

WalletModel::EncryptionStatus WalletModel::getEncryptionStatus() const
{
    if(!wallet->IsCrypted())
    {
        return Unencrypted;
    }
    else if(!wallet->IsLocked())
    {
        return Unlocked;
    }
    else if(wallet->IsLocked())
    {
        return Locked;
    }
    else if(fWalletUnlockStakingOnly)
    {
        return Locked;
    }
    else
    {
        return Unlocked;
    }
}

bool WalletModel::setWalletEncrypted(bool encrypted, const SecureString &passphrase)
{
    if(encrypted)
    {
        // Encrypt
        return wallet->EncryptWallet(passphrase);
    }
    else
    {
        // Decrypt -- TODO; not supported yet
        return false;
    }
}

bool WalletModel::setWalletLocked(bool locked, const SecureString &passPhrase)
{
    if(locked)
    {
        // Lock
        return wallet->Lock();
    }
    else
    {
        // Unlock
        return wallet->Unlock(passPhrase);
    }
}

bool WalletModel::changePassphrase(const SecureString &oldPass, const SecureString &newPass)
{
    bool retval;
    {
        LOCK(wallet->cs_wallet);
        wallet->Lock(); // Make sure wallet is locked before attempting pass change
        retval = wallet->ChangeWalletPassphrase(oldPass, newPass);
    }
    return retval;
}

bool WalletModel::backupWallet(const QString &filename)
{
    return wallet->BackupWallet(filename.toLocal8Bit().data());
}

// Handlers for core signals
static void NotifyKeyStoreStatusChanged(WalletModel *walletmodel, CCryptoKeyStore *wallet)
{
    qDebug() << "NotifyKeyStoreStatusChanged";
    QMetaObject::invokeMethod(walletmodel, "updateStatus", Qt::QueuedConnection);
}

static void NotifyAddressBookChanged(WalletModel *walletmodel, CWallet *wallet,
        const CTxDestination &address, const std::string &label, bool isMine,
        const std::string &purpose, ChangeType status)
{
    QString strAddress = QString::fromStdString(CNavCoinAddress(address).ToString());
    QString strLabel = QString::fromStdString(label);
    QString strPurpose = QString::fromStdString(purpose);

    qDebug() << "NotifyAddressBookChanged: " + strAddress + " " + strLabel + " isMine=" + QString::number(isMine) + " purpose=" + strPurpose + " status=" + QString::number(status);
    QMetaObject::invokeMethod(walletmodel, "updateAddressBook", Qt::QueuedConnection,
                              Q_ARG(QString, strAddress),
                              Q_ARG(QString, strLabel),
                              Q_ARG(bool, isMine),
                              Q_ARG(QString, strPurpose),
                              Q_ARG(int, status));
}

static void NotifyTransactionChanged(WalletModel *walletmodel, CWallet *wallet, const uint256 &hash, ChangeType status)
{
    Q_UNUSED(wallet);
    Q_UNUSED(hash);
    Q_UNUSED(status);
    QMetaObject::invokeMethod(walletmodel, "updateTransaction", Qt::QueuedConnection);
}

static void ShowProgress(WalletModel *walletmodel, const std::string &title, int nProgress)
{
    // emits signal "showProgress"
    QMetaObject::invokeMethod(walletmodel, "showProgress", Qt::QueuedConnection,
                              Q_ARG(QString, QString::fromStdString(title)),
                              Q_ARG(int, nProgress));
}

static void NotifyWatchonlyChanged(WalletModel *walletmodel, bool fHaveWatchonly)
{
    QMetaObject::invokeMethod(walletmodel, "updateWatchOnlyFlag", Qt::QueuedConnection,
                              Q_ARG(bool, fHaveWatchonly));
}

void WalletModel::subscribeToCoreSignals()
{
    // Connect signals to wallet
    wallet->NotifyStatusChanged.connect(boost::bind(&NotifyKeyStoreStatusChanged, this, _1));
    wallet->NotifyAddressBookChanged.connect(boost::bind(NotifyAddressBookChanged, this, _1, _2, _3, _4, _5, _6));
    wallet->NotifyTransactionChanged.connect(boost::bind(NotifyTransactionChanged, this, _1, _2, _3));
    wallet->ShowProgress.connect(boost::bind(ShowProgress, this, _1, _2));
    wallet->NotifyWatchonlyChanged.connect(boost::bind(NotifyWatchonlyChanged, this, _1));
}

void WalletModel::unsubscribeFromCoreSignals()
{
    // Disconnect signals from wallet
    wallet->NotifyStatusChanged.disconnect(boost::bind(&NotifyKeyStoreStatusChanged, this, _1));
    wallet->NotifyAddressBookChanged.disconnect(boost::bind(NotifyAddressBookChanged, this, _1, _2, _3, _4, _5, _6));
    wallet->NotifyTransactionChanged.disconnect(boost::bind(NotifyTransactionChanged, this, _1, _2, _3));
    wallet->ShowProgress.disconnect(boost::bind(ShowProgress, this, _1, _2));
    wallet->NotifyWatchonlyChanged.disconnect(boost::bind(NotifyWatchonlyChanged, this, _1));
}

// WalletModel::UnlockContext implementation
WalletModel::UnlockContext WalletModel::requestUnlock()
{
    bool was_locked = getEncryptionStatus() == Locked;
    if ((!was_locked) && fWalletUnlockStakingOnly)
    {
       setWalletLocked(true);
       was_locked = getEncryptionStatus() == Locked;

    }
    if(was_locked)
    {
        // Request UI to unlock wallet
        Q_EMIT requireUnlock();
    }
    // If wallet is still locked, unlock was failed or cancelled, mark context as invalid
    bool valid = getEncryptionStatus() != Locked;

    return UnlockContext(this, valid, was_locked && !fWalletUnlockStakingOnly);
}

WalletModel::UnlockContext::UnlockContext(WalletModel *wallet, bool valid, bool relock):
        wallet(wallet),
        valid(valid),
        relock(relock)
{
}

WalletModel::UnlockContext::~UnlockContext()
{
    if(valid && relock)
    {
        wallet->setWalletLocked(true);
    }
}

void WalletModel::UnlockContext::CopyFrom(const UnlockContext& rhs)
{
    // Transfer context; old object no longer relocks wallet
    *this = rhs;
    rhs.relock = false;
}

bool WalletModel::getPubKey(const CKeyID &address, CPubKey& vchPubKeyOut) const
{
    return wallet->GetPubKey(address, vchPubKeyOut);
}

bool WalletModel::havePrivKey(const CKeyID &address) const
{
    return wallet->HaveKey(address);
}

// returns a list of COutputs from COutPoints
void WalletModel::getOutputs(const std::vector<COutPoint>& vOutpoints, std::vector<COutput>& vOutputs)
{
    LOCK2(cs_main, wallet->cs_wallet);
    for(const COutPoint& outpoint: vOutpoints)
    {
        if (!wallet->mapWallet.count(outpoint.hash)) continue;
        int nDepth = wallet->mapWallet[outpoint.hash].GetDepthInMainChain();
        if (nDepth < 0) continue;
        COutput out(&wallet->mapWallet[outpoint.hash], outpoint.n, nDepth, true, true);
        vOutputs.push_back(out);
    }
}

bool WalletModel::isSpent(const COutPoint& outpoint) const
{
    LOCK2(cs_main, wallet->cs_wallet);
    return wallet->IsSpent(outpoint.hash, outpoint.n);
}

void WalletModel::listPrivateCoins(std::map<QString, std::vector<COutput> >& mapCoins) const
{
    std::vector<COutput> vCoins;
    wallet->AvailablePrivateCoins(vCoins, true);

    LOCK2(cs_main, wallet->cs_wallet);

    for(const COutput& out: vCoins)
    {
        COutput cout = out;

        while (wallet->IsChange(cout.tx->vout[cout.i]) && cout.tx->vin.size() > 0 && wallet->IsMine(cout.tx->vin[0]))
        {
            if (!wallet->mapWallet.count(cout.tx->vin[0].prevout.hash)) break;
            cout = COutput(&wallet->mapWallet[cout.tx->vin[0].prevout.hash], cout.tx->vin[0].prevout.n, 0, true, true, out.vMemo, out.nAmount);
        }

        CTxDestination address;
        if(!out.fSpendable)
            continue;
        std::string memo(out.vMemo.begin(), out.vMemo.end());
        mapCoins[QString::fromStdString(memo!=""?memo:"(none)")].push_back(out);
    }
}

// AvailableCoins + LockedCoins grouped by wallet address (put change in one group with wallet address)
void WalletModel::listCoins(std::map<QString, std::vector<COutput> >& mapCoins) const
{
    std::vector<COutput> vCoins;
    wallet->AvailableCoins(vCoins);

    LOCK2(cs_main, wallet->cs_wallet); // ListLockedCoins, mapWallet
    std::vector<COutPoint> vLockedCoins;
    wallet->ListLockedCoins(vLockedCoins);

    // add locked coins
    for(const COutPoint& outpoint: vLockedCoins)
    {
        if (!wallet->mapWallet.count(outpoint.hash)) continue;
        int nDepth = wallet->mapWallet[outpoint.hash].GetDepthInMainChain();
        if (nDepth < 0) continue;
        COutput out(&wallet->mapWallet[outpoint.hash], outpoint.n, nDepth, true, true);
        if (outpoint.n < out.tx->vout.size() && wallet->IsMine(out.tx->vout[outpoint.n]) == ISMINE_SPENDABLE)
            vCoins.push_back(out);
    }

    for(const COutput& out: vCoins)
    {
        COutput cout = out;

        while (wallet->IsChange(cout.tx->vout[cout.i]) && cout.tx->vin.size() > 0 && wallet->IsMine(cout.tx->vin[0]))
        {
            if (!wallet->mapWallet.count(cout.tx->vin[0].prevout.hash)) break;
            cout = COutput(&wallet->mapWallet[cout.tx->vin[0].prevout.hash], cout.tx->vin[0].prevout.n, 0, true, true);
        }

        CTxDestination address;
        if(!out.fSpendable || !ExtractDestination(cout.tx->vout[cout.i].scriptPubKey, address))
            continue;
        mapCoins[QString::fromStdString(CNavCoinAddress(address).ToString())].push_back(out);
    }
}

bool WalletModel::isLockedCoin(uint256 hash, unsigned int n) const
{
    LOCK2(cs_main, wallet->cs_wallet);
    return wallet->IsLockedCoin(hash, n);
}

void WalletModel::lockCoin(COutPoint& output)
{
    LOCK2(cs_main, wallet->cs_wallet);
    wallet->LockCoin(output);
}

void WalletModel::unlockCoin(COutPoint& output)
{
    LOCK2(cs_main, wallet->cs_wallet);
    wallet->UnlockCoin(output);
}

void WalletModel::listLockedCoins(std::vector<COutPoint>& vOutpts)
{
    LOCK2(cs_main, wallet->cs_wallet);
    wallet->ListLockedCoins(vOutpts);
}

void WalletModel::loadReceiveRequests(std::vector<std::string>& vReceiveRequests)
{
    LOCK(wallet->cs_wallet);
    for(const PAIRTYPE(CTxDestination, CAddressBookData)& item: wallet->mapAddressBook)
        for(const PAIRTYPE(std::string, std::string)& item2: item.second.destdata)
            if (item2.first.size() > 2 && item2.first.substr(0,2) == "rr") // receive request
                vReceiveRequests.push_back(item2.second);
}

bool WalletModel::saveReceiveRequest(const std::string &sAddress, const int64_t nId, const std::string &sRequest)
{
    CTxDestination dest = CNavCoinAddress(sAddress).Get();

    std::stringstream ss;
    ss << nId;
    std::string key = "rr" + ss.str(); // "rr" prefix = "receive request" in destdata

    LOCK(wallet->cs_wallet);
    if (sRequest.empty())
        return wallet->EraseDestData(dest, key);
    else
        return wallet->AddDestData(dest, key, sRequest);
}

bool WalletModel::transactionCanBeAbandoned(uint256 hash) const
{
    LOCK2(cs_main, wallet->cs_wallet);
    const CWalletTx *wtx = wallet->GetWalletTx(hash);
    if (!wtx || wtx->isAbandoned() || wtx->GetDepthInMainChain() > 0 || wtx->InMempool() || wtx->InStempool())
        return false;
    return true;
}

bool WalletModel::abandonTransaction(uint256 hash) const
{
    LOCK2(cs_main, wallet->cs_wallet);
    return wallet->AbandonTransaction(hash);
}
