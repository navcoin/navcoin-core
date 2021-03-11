// Copyright (c) 2011-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef NAVCOIN_QT_OVERVIEWPAGE_H
#define NAVCOIN_QT_OVERVIEWPAGE_H

#include <amount.h>
#include <splitrewards.h>
#include <swapxnav.h>

#include <QWidget>
#include <QPushButton>
#include <QListView>
#include <QPainter>

class ClientModel;
class TransactionFilterProxy;
class TxViewDelegate;
class PlatformStyle;
class WalletModel;

namespace Ui {
    class OverviewPage;
}

QT_BEGIN_NAMESPACE
class QModelIndex;
QT_END_NAMESPACE

/** Overview ("home") page widget */
class OverviewPage : public QWidget
{
    Q_OBJECT

public:
    explicit OverviewPage(const PlatformStyle *platformStyle, QWidget *parent = 0);
    ~OverviewPage();

    void setClientModel(ClientModel *clientModel);
    void setWalletModel(WalletModel *walletModel);

public Q_SLOTS:
    void setBalance(
        const CAmount& balance,
        const CAmount& unconfirmedBalance,
        const CAmount& stakingBalance,
        const CAmount& immatureBalance,
        const CAmount& watchOnlyBalance,
        const CAmount& watchUnconfBalance,
        const CAmount& watchImmatureBalance,
        const CAmount& coldStakingBalance,
        const CAmount& privateBalance,
        const CAmount& privPending,
        const CAmount& privLocked
    );

    void setStakes(
        const CAmount& amount24h,
        const CAmount& amount7d,
        const CAmount& amount30d,
        const CAmount& amount1y,
        const CAmount& amountAll,
        const CAmount& amountExp
    );

    void setStakingStats(QString day, QString week, QString month, QString year, QString all);
    void on_showStakingSetup_clicked();

Q_SIGNALS:
    void transactionClicked(const QModelIndex &index);
    void outOfSyncWarningClicked();

private:
    Ui::OverviewPage *ui;
    ClientModel *clientModel;
    WalletModel *walletModel;
    CAmount currentBalance = -1;
    CAmount currentUnconfirmedBalance;
    CAmount currentStakingBalance;
    CAmount currentColdStakingBalance;
    CAmount currentImmatureBalance;
    CAmount currentTotalBalance;
    CAmount currentPrivateBalance;
    CAmount currentPrivateBalancePending;
    CAmount currentPrivateBalanceLocked;
    CAmount currentWatchOnlyBalance;
    CAmount currentWatchUnconfBalance;
    CAmount currentWatchImmatureBalance;
    CAmount currentWatchOnlyTotalBalance;
    CAmount currentAmount24h = -1;
    CAmount currentAmount7d;
    CAmount currentAmount30d;
    CAmount currentAmount1y;
    CAmount currentAmountAll;
    CAmount currentAmountExp;

    TxViewDelegate *txdelegate;
    TransactionFilterProxy *filter;

    SwapXNAVDialog* swapDialog;

private Q_SLOTS:
    void updateDisplayUnit();
    void handleTransactionClicked(const QModelIndex &index);
    void updateWatchOnlyLabels(bool showWatchOnly);
    void ShowSwapDialog();
};

#endif // NAVCOIN_QT_OVERVIEWPAGE_H
