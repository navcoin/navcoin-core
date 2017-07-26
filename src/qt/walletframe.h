// Copyright (c) 2011-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef NAVCOIN_QT_WALLETFRAME_H
#define NAVCOIN_QT_WALLETFRAME_H

#include <QFrame>
#include <QMap>
#include <QHBoxLayout>
#include <QPushButton>


class NavCoinGUI;
class ClientModel;
class PlatformStyle;
class SendCoinsRecipient;
class WalletModel;
class WalletView;

QT_BEGIN_NAMESPACE
class QStackedWidget;
QT_END_NAMESPACE

class WalletFrame : public QFrame
{
    Q_OBJECT

public:
    explicit WalletFrame(const PlatformStyle *platformStyle, NavCoinGUI *_gui = 0);
    ~WalletFrame();

    void setClientModel(ClientModel *clientModel);

    bool addWallet(const QString& name, WalletModel *walletModel);
    bool setCurrentWallet(const QString& name);
    bool removeWallet(const QString &name);
    void removeAllWallets();

    bool handlePaymentRequest(const SendCoinsRecipient& recipient);

    void showOutOfSyncWarning(bool fShow);
    WalletView *currentWalletView();

    QWidget *topMenu;

private:
    QStackedWidget *walletStack;
    NavCoinGUI *gui;
    ClientModel *clientModel;
    QMap<QString, WalletView*> mapWalletViews;

    bool bOutOfSync;

    const PlatformStyle *platformStyle;


public Q_SLOTS:

    /** Switch to overview (home) page */
    void gotoOverviewPage();
    /** Switch to history (transactions) page */
    void gotoHistoryPage();
    /** Switch to receive coins page */
    void gotoReceiveCoinsPage();
    /** Switch to send coins page */
    void gotoSendCoinsPage(QString addr = "");
    void gotoRequestPaymentPage();

    /** Show Sign/Verify Message dialog and switch to sign message tab */
    void gotoSignMessageTab(QString addr = "");
    /** Show Sign/Verify Message dialog and switch to verify message tab */
    void gotoVerifyMessageTab(QString addr = "");



    /** Encrypt the wallet */
    void encryptWallet(bool status);
    /** Backup the wallet */
    void backupWallet();
    /** Change encrypted wallet passphrase */
    void changePassphrase();
    /** Ask for passphrase to unlock wallet temporarily */
    void unlockWallet();
    void unlockWalletStaking();
    void lockWallet();
    void importPrivateKey();

    void setStatusTitleBlocks(QString text);

    void setStatusTitleConnections(QString text);

    void setStatusTitle(QString text);

    void showStatusTitleConnections();
    void hideStatusTitleConnections();
    void showStatusTitleBlocks();
    void hideStatusTitleBlocks();

    void showLockStaking(bool status);

    void setStakingStatus(QString text);
    void setStakingStats(QString day, QString week, QString month);

    void setVotingStatus(QString text);

    /** Show used sending addresses */
    void usedSendingAddresses();
    /** Show used receiving addresses */
    void usedReceivingAddresses();

};

#endif // NAVCOIN_QT_WALLETFRAME_H
