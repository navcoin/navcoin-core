// Copyright (c) 2011-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef NAVCOIN_QT_NAVCOINGUI_H
#define NAVCOIN_QT_NAVCOINGUI_H

#if defined(HAVE_CONFIG_H)
#include <config/navcoin-config.h>
#endif

#include <amount.h>

#include <QAbstractButton>
#include <QComboBox>
#include <QEvent>
#include <QHoverEvent>
#include <QLabel>
#include <QMainWindow>
#include <QMap>
#include <QMenu>
#include <QPainter>
#include <QPoint>
#include <QToolButton>
#include <QSystemTrayIcon>
#include <QtNetwork>

#ifdef Q_OS_MAC
#include <qt/macappnapinhibitor.h>
#endif

class ClientModel;
class NetworkStyle;
class Notificator;
class OptionsModel;
class PlatformStyle;
class RPCConsole;
class SendCoinsRecipient;
class WalletFrame;
class WalletModel;
class HelpMessageDialog;
class ModalOverlay;

class CWallet;

QT_BEGIN_NAMESPACE
class QAction;
class QProgressBar;
class QProgressDialog;
class QAbstractButton;
QT_END_NAMESPACE

namespace GUIUtil {
class ClickableLabel;
class ClickableProgressBar;
}

/**
  NavCoin GUI main class. This class represents the main window of the NavCoin UI. It communicates with both the client and
  wallet models to give the user an up-to-date view of the current core state.
*/
class NavCoinGUI : public QMainWindow
{
    Q_OBJECT

public:
    static const QString DEFAULT_WALLET;

    explicit NavCoinGUI(const PlatformStyle *platformStyle, const NetworkStyle *networkStyle, QWidget *parent = 0);
    ~NavCoinGUI();

    /** Get the screen scale, usefull for scaling UI elements */
    float scale();

    /** Show/Hide the wallet sync warning notification */
    void showOutOfSyncWarning(bool fShow);

    /** Show/Hide a notification */
    void showHideNotification(bool show, int index);

    /** Set the active menuBtns */
    void setActiveMenu(int index);

    /** Sets the balance for the wallet GUI header */
    void setBalance(const CAmount &avail, const CAmount &pendi, const CAmount &immat, const CAmount &priv, const CAmount& privpending, const CAmount& privlocked);

    /** Sets the staked amounts for the wallet GUI header */
    void setStaked(const CAmount &all, const CAmount &today, const CAmount &week);

    /** Set the bubble counter on menubtns */
    void setMenuBubble(int index, int drak);

    /** Set the client model.
        The client model represents the part of the core that communicates with the P2P network, and is wallet-agnostic.
    */
    void setClientModel(ClientModel *clientModel);
    void showVotingDialog();

#ifdef ENABLE_WALLET
    /** Set the wallet model.
        The wallet model represents a navcoin wallet, and offers access to the list of transactions, address book and sending
        functionality.
    */
    bool addWallet(const QString& name, WalletModel *walletModel);
    bool setCurrentWallet(const QString& name);
    void removeAllWallets();
#endif // ENABLE_WALLET
    bool enableWallet;

protected:
    void changeEvent(QEvent *e);
    void closeEvent(QCloseEvent *event);
    void showEvent(QShowEvent *event);
    void dragEnterEvent(QDragEnterEvent *event);
    void dropEvent(QDropEvent *event);
    bool eventFilter(QObject *object, QEvent *event);

private:
    ClientModel *clientModel;
    WalletFrame *walletFrame;

    QComboBox *unitDisplayControl;
    QLabel *labelEncryptionIcon;
    QLabel *labelConnectionsIcon;
    GUIUtil::ClickableLabel* labelBlocksIcon;
    QLabel *labelStakingIcon;
    QLabel *labelPrice;
    QTimer *timerPrice;
    QLabel *progressBarLabel;
    GUIUtil::ClickableProgressBar* progressBar;
    QProgressDialog *progressDialog;

    QLabel* balanceAvail;
    QLabel* balancePendi;
    QLabel* balanceImmat;
    QLabel* privAvail;
    QLabel* privPendi;
    QLabel* privLocked;
    QLabel* stakedAvail;
    QLabel* stakedPendi;
    QLabel* stakedImmat;

    QMenuBar *appMenuBar;
    QAction *overviewAction;
    QAction *historyAction;
    QAction *quitAction;
    QAction *sendCoinsAction;
    QAction *sendCoinsMenuAction;
    QAction *usedSendingAddressesAction;
    QAction *usedReceivingAddressesAction;
    QAction *repairWalletAction;
    QAction *importPrivateKeyAction;
    QAction *exportMasterPrivateKeyAction;
    QAction *exportMnemonicAction;
    QAction *signMessageAction;
    QAction *verifyMessageAction;
    QAction *aboutAction;
    QAction *infoAction;
    QAction *receiveCoinsAction;
    QAction *receiveCoinsMenuAction;
    QAction *optionsAction;
    QAction *cfundProposalsAction;
    QAction *cfundPaymentRequestsAction;
    QAction *toggleHideAction;
    QAction *encryptWalletAction;
    QAction *backupWalletAction;
    QAction *changePassphraseAction;
    QAction *aboutQtAction;
    QAction *openRPCConsoleAction;
    QAction *openAction;
    QAction *showHelpMessageAction;
    QAction *unlockWalletAction;
    QAction *lockWalletAction;
    QAction *toggleStakingAction;
    QAction *splitRewardAction;
    QToolButton *menuBtns[5];
    QLabel *menuBubbles[5];
    QLabel *notifications[3];

    QSystemTrayIcon *trayIcon;
    QMenu *trayIconMenu;
    Notificator *notificator;
    RPCConsole *rpcConsole;
    HelpMessageDialog *helpMessageDialog;
    ModalOverlay* modalOverlay;

#ifdef Q_OS_MAC
    CAppNapInhibitor* appNapInhibitor = nullptr;
#endif

    /** Keep track of previous number of blocks, to detect progress */
    int prevBlocks;
    int spinnerFrame;

    uint64_t nWeight;

    const PlatformStyle *platformStyle;

    QAction *updatePriceAction;
    bool fShowingVoting;

    /** Create the main UI actions. */
    void createActions();
    /** Create the menu bar and sub-menus. */
    void createMenuBar();
    /** Create the header widgets */
    void createHeaderWidgets();
    /** Create the toolbars */
    void createToolBars();
    /** Create system tray icon and notification */
    void createTrayIcon(const NetworkStyle *networkStyle);
    /** Create system tray menu (or setup the dock menu) */
    void createTrayIconMenu();

    /** Enable or disable all wallet-related actions */
    void setWalletActionsEnabled(bool enabled);

    /** Connect core signals to GUI client */
    void subscribeToCoreSignals();
    /** Disconnect core signals from GUI client */
    void unsubscribeFromCoreSignals();

    void updateHeadersSyncProgressLabel();

    void updateWeight();

    void cfundProposalsOpen(bool fMode);



Q_SIGNALS:
    /** Signal raised when a URI was entered or dragged to the GUI */
    void receivedURI(const QString &uri);
    /** Restart handling */
    void requestedRestart(QStringList args);

public Q_SLOTS:
    /** Set number of connections shown in the UI */
    void setNumConnections(int count);
    /** Set number of blocks and last block date shown in the UI */
    void setNumBlocks(int count, const QDateTime& blockDate, double nVerificationProgress, bool headers);
    /** Get restart command-line parameters and request restart */
    void handleRestart(QStringList args);

    /** Notify the user of an event from the core network or transaction handling code.
       @param[in] title     the message box / notification title
       @param[in] message   the displayed text
       @param[in] style     modality and style definitions (icon and used buttons - buttons only for message boxes)
                            @see CClientUIInterface::MessageBoxFlags
       @param[in] ret       pointer to a bool that will be modified to whether Ok was clicked (modal only)
    */
    void message(const QString &title, const QString &message, unsigned int style, bool *ret = NULL);

#ifdef ENABLE_WALLET
    /** Set the encryption status as shown in the UI.
       @param[in] status            current encryption status
       @see WalletModel::EncryptionStatus
    */
    void setEncryptionStatus(int status);

    bool handlePaymentRequest(const SendCoinsRecipient& recipient);

    /** Show incoming transaction notification for new transactions. */
    void incomingTransaction(const QString& date, int unit, const CAmount& amount, const QString& type, const QString& address, const QString& label);
#endif // ENABLE_WALLET

private Q_SLOTS:
#ifdef ENABLE_WALLET
    /** Switch to overview (home) page */
    void gotoOverviewPage();
    /** Switch to history (transactions) page */
    void gotoHistoryPage();
    /** Switch to community fund page*/
    void gotoCommunityFundPage();
    /** Switch to receive coins page */
    void gotoReceiveCoinsPage();
    /** Switch to send coins page */
    void gotoSendCoinsPage(QString addr = "");
    void gotoRequestPaymentPage();

    /** Show Sign/Verify Message dialog and switch to sign message tab */
    void gotoSignMessageTab(QString addr = "");
    /** Show Sign/Verify Message dialog and switch to verify message tab */
    void gotoVerifyMessageTab(QString addr = "");

    /** Show open dialog */
    void openClicked();
    /** Update Staking status **/
    void updateStakingStatus();
    /** Fetch Price from CMC **/
    void updatePrice();

    /** Repairs wallet **/
    void repairWallet();

    /** Used by curl request in updatePrice */
    static size_t priceUdateWriteCallback(void *contents, size_t size, size_t nmemb, void *userp);

    /** Update the alerts notification */
    void updateAlerts(const QString &warnings);

    void onDaoEntriesChanged(int count);

#endif // ENABLE_WALLET
    /** Show configuration dialog */
    void optionsClicked();
    /** Community Fund related */
    void cfundProposalsClicked();
    void cfundPaymentRequestsClicked();
    /** Show about dialog */
    void aboutClicked();
    /** Open NavCoin Knowledge base */
    void infoClicked();
    /** Show debug window */
    void showDebugWindow();
    /** Show debug window and set focus to the console */
    void showDebugWindowActivateConsole();
    /** Show help message dialog */
    void showHelpMessageClicked();
    /** Update the display currency **/
    void comboBoxChanged(int index);
    /** When Display Units are changed on OptionsModel it will refresh the display text of the control on the status bar */
    void updateDisplayUnit(int unit);
    /** Toggle Staking **/
    void toggleStaking();
    void splitRewards();
#ifndef Q_OS_MAC
    /** Handle tray icon clicked */
    void trayIconActivated(QSystemTrayIcon::ActivationReason reason);
#endif

    /** Show window if hidden, unminimize when minimized, rise when obscured or show if hidden and fToggleHidden is true */
    void showNormalIfMinimized(bool fToggleHidden = false);
    /** Simply calls showNormalIfMinimized(true) for use in SLOT() macro */
    void toggleHidden();

    /** called by a timer to check if fRequestShutdown has been set **/
    void detectShutdown();

    /** Show progress dialog e.g. for verifychain */
    void showProgress(const QString &title, int nProgress);

    /** When hideTrayIcon setting is changed in OptionsModel hide or show the icon accordingly. */
    void setTrayIconVisible(bool);

    /** When the progressBar is clicked, this will show the details */
    void showModalOverlay();
};

#endif // NAVCOIN_QT_NAVCOINGUI_H
