// Copyright (c) 2011-2014 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef NAVCOIN_QT_WALLETMODELTRANSACTION_H
#define NAVCOIN_QT_WALLETMODELTRANSACTION_H

#include <qt/walletmodel.h>
#include <wallet/wallet.h>

#include <QObject>

class SendCoinsRecipient;

class CReserveBLSCTBlindingKey;
class CReserveKey;
class CWallet;
class CWalletTx;

/** Data model for a walletmodel transaction. */
class WalletModelTransaction
{
public:
    explicit WalletModelTransaction(const QList<SendCoinsRecipient> &recipients);
    ~WalletModelTransaction();

    QList<SendCoinsRecipient> getRecipients();

    CWalletTx *getTransaction();
    unsigned int getTransactionSize();
    std::vector<CWalletTx> vTransactions;
    std::vector<CRecipient> vecSend;

    void setTransactionFee(const CAmount& newFee);
    CAmount getTransactionFee();

    CAmount getTotalTransactionAmount();

    void newPossibleKeyChange(CWallet *wallet);
    CReserveKey *getPossibleKeyChange();

    void newPossibleBLSCTBlindingKey(CWallet *wallet);
    std::vector<shared_ptr<CReserveBLSCTBlindingKey>> *getPossibleBLSCTBlindingKey();

    void reassignAmounts(int nChangePosRet, CWalletTx* wTx, int index); // needed for the subtract-fee-from-amount feature
    QList<SendCoinsRecipient> recipients;

    bool fSpendsColdStaking;

private:
    CWalletTx *walletTransaction = new CWalletTx();
    CReserveKey *keyChange;
    std::vector<shared_ptr<CReserveBLSCTBlindingKey>> *blsctBlindingKey;
    CAmount fee;
};

#endif // NAVCOIN_QT_WALLETMODELTRANSACTION_H
