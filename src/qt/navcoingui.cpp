// Copyright (c) 2011-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#if defined(HAVE_CONFIG_H)
#include <config/navcoin-config.h>
#endif

#include <main.h>
#include <qt/navcoingui.h>
#include <qt/navcoinunits.h>
#include <clientversion.h>
#include <qt/clientmodel.h>
#include <qt/guiconstants.h>
#include <qt/guiutil.h>
#include <qt/modaloverlay.h>
#include <qt/networkstyle.h>
#include <qt/notificator.h>
#include <qt/openuridialog.h>
#include <qt/optionsdialog.h>
#include <qt/optionsmodel.h>
#include <qt/platformstyle.h>
#include <qt/rpcconsole.h>
#include <qt/utilitydialog.h>
#include <qt/walletmodel.h>
#include <wallet/rpcwallet.h>

#ifdef ENABLE_WALLET
#include <qt/walletframe.h>
#include <qt/walletmodel.h>
#include <qt/walletview.h>
#include <wallet/wallet.h>
#endif // ENABLE_WALLET

#ifdef Q_OS_MAC
#include <qt/macdockiconhandler.h>
#endif

#include <chainparams.h>
#include <init.h>
#include <miner.h>
#include <ui_interface.h>
#include <util.h>
#include <pos.h>
#include <main.h>

#include <iostream>
#include <thread>

#include <curl/curl.h>
#include <boost/lexical_cast.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>

#include <QAction>
#include <QApplication>
#include <QCheckBox>
#include <QDateTime>
#include <QDesktopServices>
#include <QDesktopWidget>
#include <QDir>
#include <QDragEnterEvent>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QListWidget>
#include <QMenuBar>
#include <QMessageBox>
#include <QMimeData>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QProgressBar>
#include <QProgressDialog>
#include <QSettings>
#include <QShortcut>
#include <QSizePolicy>
#include <QStackedWidget>
#include <QStatusBar>
#include <QTimer>
#include <QToolBar>
#include <QToolButton>
#include <QUrl>
#include <QUrlQuery>
#include <QVBoxLayout>
#include <QVariant>
#include <QVariantMap>
#include <QWidget>

static const struct {
    bool error;
    const char *text;
} notifs[] = {
    { true, "The DAO needs you! Please don't forget to vote!" },
    { false, "This wallet is currently syncing. Your balance may not be accurate until it has completed!" },
    { false, "GENERIC WARNINGS USE THIS" }
};

static const unsigned notifs_count = sizeof(notifs)/sizeof(*notifs);

const QString NavCoinGUI::DEFAULT_WALLET = "~Default";

NavCoinGUI::NavCoinGUI(const PlatformStyle *platformStyle, const NetworkStyle *networkStyle, QWidget *parent) :
    QMainWindow(parent),
    clientModel(0),
    walletFrame(0),
    unitDisplayControl(0),
    labelEncryptionIcon(0),
    labelConnectionsIcon(0),
    labelBlocksIcon(0),
    labelStakingIcon(0),
    labelPrice(0),
    progressBarLabel(0),
    progressBar(0),
    progressDialog(0),
    appMenuBar(0),
    overviewAction(0),
    historyAction(0),
    quitAction(0),
    sendCoinsAction(0),
    sendCoinsMenuAction(0),
    usedSendingAddressesAction(0),
    usedReceivingAddressesAction(0),
    repairWalletAction(0),
    importPrivateKeyAction(0),
    exportMasterPrivateKeyAction(0),
    signMessageAction(0),
    verifyMessageAction(0),
    aboutAction(0),
    infoAction(0),
    receiveCoinsAction(0),
    receiveCoinsMenuAction(0),
    optionsAction(0),
    cfundProposalsAction(0),
    cfundPaymentRequestsAction(0),
    toggleHideAction(0),
    encryptWalletAction(0),
    backupWalletAction(0),
    changePassphraseAction(0),
    aboutQtAction(0),
    openRPCConsoleAction(0),
    openAction(0),
    showHelpMessageAction(0),
    trayIcon(0),
    trayIconMenu(0),
    notificator(0),
    rpcConsole(0),
    helpMessageDialog(0),
    prevBlocks(0),
    spinnerFrame(0),
    unlockWalletAction(0),
    lockWalletAction(0),
    toggleStakingAction(0),
    splitRewardAction(0),
    platformStyle(platformStyle),
    updatePriceAction(0),
    fShowingVoting(0)
{
    GUIUtil::restoreWindowGeometry("nWindow", QSize(800, 600), this);
    QString windowTitle = tr(PACKAGE_NAME) + " - ";
#ifdef ENABLE_WALLET
    /* if compiled with wallet support, -disablewallet can still disable the wallet */
    enableWallet = !GetBoolArg("-disablewallet", false);
#else
    enableWallet = false;
#endif // ENABLE_WALLET
    if(enableWallet)
    {
        windowTitle += tr("Wallet");
    } else {
        windowTitle += tr("Node");
    }

    if(clientModel->isReleaseVersion() == false)
    {
         // default to test version
        QString titleExtra = tr("[TEST ONLY]");

        if(clientModel->isRCReleaseVersion() == true)
        {
            titleExtra = tr("[RELEASE CANDIDATE]");
        }

         windowTitle += " " + titleExtra;
    }

    windowTitle += " " + networkStyle->getTitleAddText();


#ifndef Q_OS_MAC
    QApplication::setWindowIcon(networkStyle->getTrayAndWindowIcon());
    setWindowIcon(networkStyle->getTrayAndWindowIcon());
#else
    MacDockIconHandler::instance()->setIcon(networkStyle->getAppIcon());
#endif
    setWindowTitle(windowTitle);

#if defined(Q_OS_MAC)
    // This property is not implemented in Qt 5. Setting it has no effect.
    // A replacement API (QtMacUnifiedToolBar) is available in QtMacExtras.
    setUnifiedTitleAndToolBarOnMac(true);
#endif

    rpcConsole = new RPCConsole(platformStyle, 0);
    helpMessageDialog = new HelpMessageDialog(this, false);
#ifdef ENABLE_WALLET
    if(enableWallet)
    {
        /** Create wallet frame and make it the central widget */
        walletFrame = new WalletFrame(platformStyle, this);
        setCentralWidget(walletFrame);
    } else
#endif // ENABLE_WALLET
    {
        /* When compiled without wallet or -disablewallet is provided,
         * the central widget is the rpc console.
         */
        setCentralWidget(rpcConsole);
    }

    // Accept D&D of URIs
    setAcceptDrops(true);

    // Create actions for the toolbar, menu bar and tray/dock icon
    // Needs walletFrame to be initialized
    createActions();

    // Create application menu bar
    createMenuBar();

    // Create the header widgets
    createHeaderWidgets();

    // Create the toolbars
    createToolBars();

    // Create system tray icon and notification
    createTrayIcon(networkStyle);

    // Create status bar
    statusBar();

    // Disable size grip because it looks ugly and nobody needs it
    statusBar()->setSizeGripEnabled(false);

    // Status bar notification icons
    unitDisplayControl = new QComboBox();
    unitDisplayControl->setEditable(true);
    unitDisplayControl->setInsertPolicy(QComboBox::NoInsert);
    unitDisplayControl->setFixedHeight(25 * scale());
    for(NavCoinUnits::Unit u: NavCoinUnits::availableUnits())
    {
        unitDisplayControl->addItem(QString(NavCoinUnits::name(u)), u);
    }
    connect(unitDisplayControl,SIGNAL(currentIndexChanged(int)),this,SLOT(comboBoxChanged(int)));
    labelEncryptionIcon = new QLabel();
    labelEncryptionIcon->setProperty("class", "status-icon");
    labelStakingIcon = new QLabel();
    labelStakingIcon->setProperty("class", "status-icon");
    labelPrice = new QLabel();
    labelPrice->setProperty("class", "StatusPrice");
    labelConnectionsIcon = new QLabel();
    labelConnectionsIcon->setProperty("class", "status-icon");
    labelBlocksIcon = new GUIUtil::ClickableLabel();
    labelBlocksIcon->setProperty("class", "status-icon");
    if(enableWallet)
    {
        walletFrame->statusLayout->addWidget(labelPrice);
        walletFrame->statusLayout->addWidget(labelEncryptionIcon);
        walletFrame->statusLayout->addWidget(labelStakingIcon);
    }
    walletFrame->statusLayout->addWidget(labelConnectionsIcon);
    walletFrame->statusLayout->addWidget(labelBlocksIcon);
    if(enableWallet)
        walletFrame->statusLayout->addWidget(unitDisplayControl);

    updatePrice(); // First price update

    int updateFiatPeriod = GetArg("-updatefiatperiod", PRICE_UPDATE_DELAY);
    if (updateFiatPeriod >= PRICE_UPDATE_DELAY)
    {
        QTimer *timerPrice = new QTimer(labelPrice);
        connect(timerPrice, SIGNAL(timeout()), this, SLOT(updatePrice()));
        timerPrice->start(updateFiatPeriod);
        info("Automatic price update set to " + std::to_string(updateFiatPeriod) + "ms");
    }
    else
    {
        info("Automatic price update turned OFF");
    }

    QTimer *timerStakingIcon = new QTimer(labelStakingIcon);
    connect(timerStakingIcon, SIGNAL(timeout()), this, SLOT(updateStakingStatus()));
    timerStakingIcon->start(45 * 1000);
    updateStakingStatus();

    if (GetArg("-zapwallettxes",0) == 2 && GetArg("-repairwallet",0) == 1)
    {
        RemoveConfigFile("zapwallettxes","2");
        RemoveConfigFile("repairwallet","1");

        QMessageBox::information(this, tr("Repair wallet"),
            tr("Wallet has been repaired."),
            QMessageBox::Ok, QMessageBox::Ok);
    }

    // Progress bar and label for blocks download
    progressBarLabel = new QLabel();
    progressBarLabel->setVisible(false);
    progressBar = new GUIUtil::ProgressBar();
    progressBar->setAlignment(Qt::AlignCenter);
    progressBar->setVisible(false);

    // Override style sheet for progress bar for styles that have a segmented progress bar,
    // as they make the text unreadable (workaround for issue #1071)
    // See https://qt-project.org/doc/qt-4.8/gallery.html
    statusBar()->addWidget(progressBarLabel);
    statusBar()->addWidget(progressBar);

    // Install event filter to be able to catch status tip events (QEvent::StatusTip)
    this->installEventFilter(this);

    // Initially wallet actions should be disabled
    setWalletActionsEnabled(false);

    // Subscribe to notifications from core
    subscribeToCoreSignals();

    modalOverlay = new ModalOverlay(platformStyle, this->centralWidget());
#ifdef ENABLE_WALLET
    if(enableWallet) {
        connect(walletFrame, &WalletFrame::requestedSyncWarningInfo, this, &NavCoinGUI::showModalOverlay);
        connect(labelBlocksIcon, &GUIUtil::ClickableLabel::clicked, this, &NavCoinGUI::showModalOverlay);
        connect(progressBar, &GUIUtil::ClickableProgressBar::clicked, this, &NavCoinGUI::showModalOverlay);
    }

    gotoOverviewPage();
#endif

#ifdef Q_OS_MAC
    appNapInhibitor = new CAppNapInhibitor;
#endif
}

NavCoinGUI::~NavCoinGUI()
{
    // Unsubscribe from notifications from core
    unsubscribeFromCoreSignals();

    GUIUtil::saveWindowGeometry("nWindow", this);
    if(trayIcon) // Hide tray icon, as deleting will let it linger until quit (on Ubuntu)
        trayIcon->hide();
#ifdef Q_OS_MAC
    delete appNapInhibitor;
    delete appMenuBar;
    MacDockIconHandler::cleanup();
#endif

    delete rpcConsole;
}

float NavCoinGUI::scale()
{
    return GUIUtil::scale();
}

void NavCoinGUI::createActions()
{
    QActionGroup *tabGroup = new QActionGroup(this);

    overviewAction = new QAction(platformStyle->Icon(":/icons/navcoin"), tr("&Overview"), this);
    overviewAction->setStatusTip(tr("Show general overview of wallet"));
    overviewAction->setToolTip(overviewAction->statusTip());
    overviewAction->setCheckable(true);
    overviewAction->setShortcut(QKeySequence(Qt::ALT + Qt::Key_1));
    tabGroup->addAction(overviewAction);

    sendCoinsAction = new QAction(platformStyle->Icon(":/icons/send"), tr("&Send"), this);
    sendCoinsAction->setStatusTip(tr("Send coins to a NavCoin address"));
    sendCoinsAction->setToolTip(sendCoinsAction->statusTip());
    sendCoinsAction->setCheckable(true);
    sendCoinsAction->setShortcut(QKeySequence(Qt::ALT + Qt::Key_2));
    tabGroup->addAction(sendCoinsAction);

    sendCoinsMenuAction = new QAction(platformStyle->IconAlt(":/icons/send"), sendCoinsAction->text(), this);
    sendCoinsMenuAction->setStatusTip(sendCoinsAction->statusTip());
    sendCoinsMenuAction->setToolTip(sendCoinsMenuAction->statusTip());

    receiveCoinsAction = new QAction(platformStyle->Icon(":/icons/receiving_addresses"), tr("&Receive"), this);
    receiveCoinsAction->setStatusTip(tr("Request payments (generates QR codes and navcoin: URIs)"));
    receiveCoinsAction->setToolTip(receiveCoinsAction->statusTip());
    receiveCoinsAction->setCheckable(true);
    receiveCoinsAction->setShortcut(QKeySequence(Qt::ALT + Qt::Key_3));
    tabGroup->addAction(receiveCoinsAction);

    receiveCoinsMenuAction = new QAction(platformStyle->IconAlt(":/icons/receiving_addresses"), receiveCoinsAction->text(), this);
    receiveCoinsMenuAction->setStatusTip(receiveCoinsAction->statusTip());
    receiveCoinsMenuAction->setToolTip(receiveCoinsMenuAction->statusTip());

    toggleStakingAction = new QAction(tr("Toggle &Staking"), this);
    toggleStakingAction->setStatusTip(tr("Toggle Staking"));

    splitRewardAction = new QAction(tr("Set up staking rewards"), this);
    splitRewardAction->setStatusTip(tr("Configure how to split the staking rewards"));

    historyAction = new QAction(platformStyle->Icon(":/icons/transactions"), tr("&Transactions"), this);
    historyAction->setStatusTip(tr("Browse transaction history"));
    historyAction->setToolTip(historyAction->statusTip());
    historyAction->setCheckable(true);
    historyAction->setShortcut(QKeySequence(Qt::ALT + Qt::Key_4));
    tabGroup->addAction(historyAction);

    updatePriceAction  = new QAction(tr("Update exchange prices"), this);
    updatePriceAction->setStatusTip(tr("Update exchange prices"));

    connect(updatePriceAction, SIGNAL(triggered()), this, SLOT(updatePrice()));

#ifdef ENABLE_WALLET
    // These showNormalIfMinimized are needed because Send Coins and Receive Coins
    // can be triggered from the tray menu, and need to show the GUI to be useful.
    connect(overviewAction, SIGNAL(triggered()), this, SLOT(showNormalIfMinimized()));
    connect(overviewAction, SIGNAL(triggered()), this, SLOT(gotoOverviewPage()));
    connect(sendCoinsAction, SIGNAL(triggered()), this, SLOT(showNormalIfMinimized()));
    connect(sendCoinsAction, SIGNAL(triggered()), this, SLOT(gotoSendCoinsPage()));
    connect(sendCoinsMenuAction, SIGNAL(triggered()), this, SLOT(showNormalIfMinimized()));
    connect(sendCoinsMenuAction, SIGNAL(triggered()), this, SLOT(gotoSendCoinsPage()));
    connect(receiveCoinsAction, SIGNAL(triggered()), this, SLOT(showNormalIfMinimized()));
    connect(receiveCoinsAction, SIGNAL(triggered()), this, SLOT(gotoRequestPaymentPage()));
    connect(receiveCoinsMenuAction, SIGNAL(triggered()), this, SLOT(showNormalIfMinimized()));
    connect(receiveCoinsMenuAction, SIGNAL(triggered()), this, SLOT(gotoRequestPaymentPage()));
    connect(historyAction, SIGNAL(triggered()), this, SLOT(showNormalIfMinimized()));
    connect(historyAction, SIGNAL(triggered()), this, SLOT(gotoHistoryPage()));
    connect(toggleStakingAction, SIGNAL(triggered()), this, SLOT(toggleStaking()));
    connect(splitRewardAction, SIGNAL(triggered()), this, SLOT(splitRewards()));
#endif // ENABLE_WALLET

    quitAction = new QAction(platformStyle->IconAlt(":/icons/quit"), tr("E&xit"), this);
    quitAction->setStatusTip(tr("Quit application"));
    quitAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_Q));
    quitAction->setMenuRole(QAction::QuitRole);
    aboutAction = new QAction(platformStyle->IconAlt(":/icons/about"), tr("&About %1").arg(tr(PACKAGE_NAME)), this);
    aboutAction->setStatusTip(tr("Show information about %1").arg(tr(PACKAGE_NAME)));
    aboutAction->setMenuRole(QAction::AboutRole);
    aboutAction->setEnabled(false);
    infoAction = new QAction(platformStyle->IconAlt(":/icons/address-book"), tr("%1 &Knowledge Base").arg(tr(PACKAGE_NAME)), this);
    infoAction->setStatusTip(tr("Open the %1 Knowledge Base in your browser").arg(tr(PACKAGE_NAME)));
    infoAction->setMenuRole(QAction::NoRole);
    infoAction->setEnabled(false);
    aboutQtAction = new QAction(platformStyle->IconAlt(":/icons/about_qt"), tr("About &Qt"), this);
    aboutQtAction->setStatusTip(tr("Show information about Qt"));
    aboutQtAction->setMenuRole(QAction::AboutQtRole);
    optionsAction = new QAction(platformStyle->IconAlt(":/icons/options"), tr("&Options..."), this);
    optionsAction->setStatusTip(tr("Modify configuration options for %1").arg(tr(PACKAGE_NAME)));
    optionsAction->setMenuRole(QAction::PreferencesRole);
    optionsAction->setEnabled(false);
    cfundProposalsAction = new QAction(tr("Vote for Proposals"), this);
    cfundPaymentRequestsAction = new QAction(tr("Vote for Payment Requests"), this);
    toggleHideAction = new QAction(platformStyle->IconAlt(":/icons/about"), tr("&Show / Hide"), this);
    toggleHideAction->setStatusTip(tr("Show or hide the main Window"));

    encryptWalletAction = new QAction(platformStyle->IconAlt(":/icons/lock_closed"), tr("&Encrypt Wallet..."), this);
    encryptWalletAction->setStatusTip(tr("Encrypt the private keys that belong to your wallet"));
    encryptWalletAction->setCheckable(true);
    unlockWalletAction = new QAction(tr("&Unlock Wallet for Staking..."), this);
    unlockWalletAction->setToolTip(tr("Unlock wallet for Staking"));
    backupWalletAction = new QAction(platformStyle->IconAlt(":/icons/filesave"), tr("&Backup Wallet..."), this);
    backupWalletAction->setStatusTip(tr("Backup wallet to another location"));
    changePassphraseAction = new QAction(platformStyle->IconAlt(":/icons/key"), tr("&Change Passphrase..."), this);
    changePassphraseAction->setStatusTip(tr("Change the passphrase used for wallet encryption"));

    signMessageAction = new QAction(platformStyle->IconAlt(":/icons/edit"), tr("Sign &message..."), this);
    signMessageAction->setStatusTip(tr("Sign messages with your NavCoin addresses to prove you own them"));
    verifyMessageAction = new QAction(platformStyle->IconAlt(":/icons/verify"), tr("&Verify message..."), this);
    verifyMessageAction->setStatusTip(tr("Verify messages to ensure they were signed with specified NavCoin addresses"));

    openRPCConsoleAction = new QAction(platformStyle->IconAlt(":/icons/debugwindow"), tr("&Debug window"), this);
    openRPCConsoleAction->setStatusTip(tr("Open debugging and diagnostic console"));
    // initially disable the debug window menu item
    openRPCConsoleAction->setEnabled(false);

    usedSendingAddressesAction = new QAction(platformStyle->IconAlt(":/icons/address-book"), tr("&Sending addresses..."), this);
    usedSendingAddressesAction->setStatusTip(tr("Show the list of used sending addresses and labels"));
    usedReceivingAddressesAction = new QAction(platformStyle->IconAlt(":/icons/address-book"), tr("&Receiving addresses..."), this);
    usedReceivingAddressesAction->setStatusTip(tr("Show the list of used receiving addresses and labels"));
    repairWalletAction = new QAction(tr("&Repair wallet"), this);
    repairWalletAction->setToolTip(tr("Repair wallet transactions"));

    importPrivateKeyAction = new QAction(tr("&Import private key"), this);
    importPrivateKeyAction->setToolTip(tr("Import private key"));

    exportMasterPrivateKeyAction = new QAction(tr("Show &master private key"), this);
    exportMasterPrivateKeyAction->setToolTip(tr("Show master private key"));

    openAction = new QAction(platformStyle->IconAlt(":/icons/open"), tr("Open &URI..."), this);
    openAction->setStatusTip(tr("Open a navcoin: URI or payment request"));

    showHelpMessageAction = new QAction(platformStyle->IconAlt(":/icons/info"), tr("&Command-line options"), this);
    showHelpMessageAction->setMenuRole(QAction::NoRole);
    showHelpMessageAction->setStatusTip(tr("Show the %1 help message to get a list with possible NavCoin command-line options").arg(tr(PACKAGE_NAME)));

    connect(quitAction, SIGNAL(triggered()), qApp, SLOT(quit()));
    connect(aboutAction, SIGNAL(triggered()), this, SLOT(aboutClicked()));
    connect(infoAction, SIGNAL(triggered()), this, SLOT(infoClicked()));
    connect(aboutQtAction, SIGNAL(triggered()), qApp, SLOT(aboutQt()));
    connect(optionsAction, SIGNAL(triggered()), this, SLOT(optionsClicked()));
    connect(cfundProposalsAction, SIGNAL(triggered()), this, SLOT(cfundProposalsClicked()));
    connect(cfundPaymentRequestsAction, SIGNAL(triggered()), this, SLOT(cfundPaymentRequestsClicked()));
    connect(toggleHideAction, SIGNAL(triggered()), this, SLOT(toggleHidden()));
    connect(showHelpMessageAction, SIGNAL(triggered()), this, SLOT(showHelpMessageClicked()));
    connect(openRPCConsoleAction, SIGNAL(triggered()), this, SLOT(showDebugWindow()));
    // prevents an open debug window from becoming stuck/unusable on client shutdown
    connect(quitAction, SIGNAL(triggered()), rpcConsole, SLOT(hide()));

#ifdef ENABLE_WALLET
    if(walletFrame)
    {
        connect(encryptWalletAction, SIGNAL(triggered(bool)), walletFrame, SLOT(encryptWallet(bool)));
        connect(unlockWalletAction, SIGNAL(triggered()), walletFrame, SLOT(unlockWalletStaking()));
        connect(backupWalletAction, SIGNAL(triggered()), walletFrame, SLOT(backupWallet()));
        connect(changePassphraseAction, SIGNAL(triggered()), walletFrame, SLOT(changePassphrase()));
        connect(signMessageAction, SIGNAL(triggered()), this, SLOT(gotoSignMessageTab()));
        connect(verifyMessageAction, SIGNAL(triggered()), this, SLOT(gotoVerifyMessageTab()));
        connect(usedSendingAddressesAction, SIGNAL(triggered()), walletFrame, SLOT(usedSendingAddresses()));
        connect(usedReceivingAddressesAction, SIGNAL(triggered()), walletFrame, SLOT(usedReceivingAddresses()));
        connect(repairWalletAction, SIGNAL(triggered()), this, SLOT(repairWallet()));
        connect(importPrivateKeyAction, SIGNAL(triggered()), walletFrame, SLOT(importPrivateKey()));
        connect(exportMasterPrivateKeyAction, SIGNAL(triggered()), walletFrame, SLOT(exportMasterPrivateKeyAction()));
        connect(openAction, SIGNAL(triggered()), this, SLOT(openClicked()));
    }
#endif // ENABLE_WALLET

    new QShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_C), this, SLOT(showDebugWindowActivateConsole()));
    new QShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_D), this, SLOT(showDebugWindow()));
}

void NavCoinGUI::createMenuBar()
{
#ifdef Q_OS_MAC
    // Create a decoupled menu bar on Mac which stays even if the window is closed
    appMenuBar = new QMenuBar();
#else
    // Get the main window's menu bar on other platforms
    appMenuBar = menuBar();
#endif

    // Configure the menus
    QMenu *file = appMenuBar->addMenu(tr("&File"));
    if(walletFrame)
    {
        file->addAction(openAction);
        file->addAction(backupWalletAction);
        file->addAction(signMessageAction);
        file->addAction(verifyMessageAction);
        file->addSeparator();
        file->addAction(usedSendingAddressesAction);
        file->addAction(usedReceivingAddressesAction);
        file->addSeparator();
        file->addAction(repairWalletAction);
        file->addSeparator();
        file->addAction(importPrivateKeyAction);
        file->addAction(exportMasterPrivateKeyAction);
    }
    file->addAction(quitAction);

    QMenu *settings = appMenuBar->addMenu(tr("&Settings"));
    if(walletFrame)
    {
        settings->addAction(encryptWalletAction);
        settings->addAction(unlockWalletAction);
        settings->addAction(changePassphraseAction);
        settings->addSeparator();
        settings->addAction(toggleStakingAction);
        settings->addAction(splitRewardAction);
        settings->addSeparator();
        settings->addAction(updatePriceAction);
    }
    settings->addAction(optionsAction);

    QMenu *help = appMenuBar->addMenu(tr("&Help"));
    if(walletFrame)
    {
        help->addAction(openRPCConsoleAction);
    }
    help->addAction(showHelpMessageAction);
    help->addSeparator();
    help->addAction(aboutAction);
    help->addAction(infoAction);
    help->addAction(aboutQtAction);
}

void NavCoinGUI::createHeaderWidgets()
{
    // Notifications layout vertical
    QVBoxLayout* notificationLayout = new QVBoxLayout();
    notificationLayout->setContentsMargins(0, 0, 0, 0);
    notificationLayout->setSpacing(2 * scale());
    notificationLayout->setAlignment(Qt::AlignVCenter);

    // Add a spacer to header to create a background
    QWidget* headerSpacer = new QWidget();
    headerSpacer->setObjectName("HeaderSpacer");
    headerSpacer->setLayout(notificationLayout);

    // Build each new notification
    for (unsigned i = 0; i < notifs_count; ++i)
    {
        // Add notifications
        notifications[i] = new QLabel();
        notifications[i]->hide();
        notifications[i]->setWordWrap(true);
        notifications[i]->setText(tr(notifs[i].text));
        if (notifs[i].error == true)
            notifications[i]->setProperty("class", "alert alert-danger");
        else
            notifications[i]->setProperty("class", "alert alert-warning");

        // Add to notification layout
        notificationLayout->addWidget(notifications[i]);
    }

    // Create the header by with gradient
    QWidget* headerBar = new QWidget();
    headerBar->setObjectName("HeaderBar");
    headerBar->setMinimumSize(1, 9 * scale());

    // Add the header spacer and header bar
    walletFrame->headerLayout->addWidget(headerSpacer);
    walletFrame->headerLayout->addWidget(headerBar);
}

void NavCoinGUI::createToolBars()
{
    if(walletFrame == nullptr)
        return;

    // Sizes
    QSize iconSize = QSize(35 * scale(), 35 * scale());
    QSize logoIconSize = QSize(60 * scale(), 60 * scale());

    // Create the logo icon
    QIcon logoIcon = QIcon(":/icons/logo_n");

    // Create the logo button
    QToolButton* logoBtn = new QToolButton();
    logoBtn->setIcon(logoIcon);
    logoBtn->setIconSize(logoIconSize);
    logoBtn->setProperty("class", "main-menu-btn");
    logoBtn->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

    // Attach the logo button to the layout
    walletFrame->menuLayout->addWidget(logoBtn);

    // Buttons icon
    QString btnNamesIcon[5] = {
        "home",
        "send",
        "receive",
        "transactions",
        "dao"
    };

    // Buttons text
    std::string btnNamesText[5] = {
        "HOME",
        "SEND",
        "RECEIVE",
        "HISTORY",
        "DAO"
    };

    // Build each new button
    for (unsigned i = 0; i < 5; ++i)
    {
        // Create the icon
        QIcon icon = platformStyle->Icon(":/icons/" + btnNamesIcon[i], COLOR_WHITE);

        // Update the disabled icon pixmap to use the same as QIcon::Normal
        icon.addPixmap(icon.pixmap(iconSize, QIcon::Normal, QIcon::On), QIcon::Disabled);

        // Create the menu button
        menuBtns[i] = new QToolButton();
        menuBtns[i]->setText(tr(btnNamesText[i].c_str()));
        menuBtns[i]->setIcon(icon);
        menuBtns[i]->setIconSize(iconSize);
        menuBtns[i]->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
        menuBtns[i]->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
        menuBtns[i]->setProperty("class", "main-menu-btn");

        // Attach to the layout and assign click events
        walletFrame->menuLayout->addWidget(menuBtns[i]);

        // Create a bubble layout
        QVBoxLayout* bubbleLayout = new QVBoxLayout(menuBtns[i]);
        bubbleLayout->setContentsMargins(0, 10 * scale(), 10 * scale(), 0);
        bubbleLayout->setSpacing(0);
        bubbleLayout->setAlignment(Qt::AlignRight | Qt::AlignTop); // Move it to the top right

        // Create the bubble and place in the bubble layout
        menuBubbles[i] = new QLabel();
        menuBubbles[i]->hide();
        menuBubbles[i]->setText("1");
        menuBubbles[i]->setProperty("class", "main-menu-bubble");
        bubbleLayout->addWidget(menuBubbles[i]);
    }

    /* This is to make the sidebar background consistent */
    QWidget *padding = new QWidget();
    padding->setProperty("class", "main-menu-btn");
    walletFrame->menuLayout->addWidget(padding);

    // Add the versionLabel
    QToolButton* versionLabel = new QToolButton();
    versionLabel->setText(QString::fromStdString(FormatVersion(CLIENT_VERSION)));
    versionLabel->setProperty("class", "main-menu-btn");
    versionLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    walletFrame->menuLayout->addWidget(versionLabel);

    // Link OverviewPage to main menu logo
    connect(logoBtn, SIGNAL(clicked()), this, SLOT(gotoOverviewPage()));

    // Menu Button actions
    connect(menuBtns[0], SIGNAL(clicked()), this, SLOT(gotoOverviewPage()));
    connect(menuBtns[1], SIGNAL(clicked()), this, SLOT(gotoSendCoinsPage()));
    connect(menuBtns[2], SIGNAL(clicked()), this, SLOT(gotoRequestPaymentPage()));
    connect(menuBtns[3], SIGNAL(clicked()), this, SLOT(gotoHistoryPage()));
    connect(menuBtns[4], SIGNAL(clicked()), this, SLOT(gotoCommunityFundPage()));

    // Open about when versionLabel is clicked
    connect(versionLabel, SIGNAL(clicked()), this, SLOT(aboutClicked()));
}

void NavCoinGUI::showOutOfSyncWarning(bool fShow)
{
    showHideNotification(fShow, 1);
}

void NavCoinGUI::showHideNotification(bool show, int index)
{
    notifications[index]->setVisible(show);
}

void NavCoinGUI::setActiveMenu(int index)
{
    for (int i = 0; i < 5; ++i)
    {
        menuBtns[i]->setDisabled(i == index);
    }
}

void NavCoinGUI::updateDaoNewCount()
{
    LOCK(cs_main);

    // Make sure we have coins db loaded
    if(!pcoinsTip || !pindexBestHeader)
        return;

    // Debug log
    info("[DAO] started dao count");

    // We start with none
    int newDaoCount = 0;

    CProposalMap mapProposals;

    if(pcoinsTip->GetAllProposals(mapProposals))
    {
        for (CProposalMap::iterator it = mapProposals.begin(); it != mapProposals.end(); it++)
        {
            CFund::CProposal proposal;
            if (!pcoinsTip->GetProposal(it->first, proposal))
                continue;

            if (!(proposal.fState == CFund::NIL && proposal.GetState(pindexBestHeader->GetBlockTime()).find("expired") == string::npos))
                continue;

            auto _it = std::find_if( vAddedProposalVotes.begin(), vAddedProposalVotes.end(),
                    [&proposal](const std::pair<std::string, bool>& element){ return element.first == proposal.hash.ToString();} );

            if (_it != vAddedProposalVotes.end())
                continue;

            // Add to the count
            newDaoCount++;
        }
    }

    //Payment request listings
    CPaymentRequestMap mapPaymentRequests;

    if(pcoinsTip->GetAllPaymentRequests(mapPaymentRequests))
    {
        for (CPaymentRequestMap::iterator it_ = mapPaymentRequests.begin(); it_ != mapPaymentRequests.end(); it_++)
        {
            CFund::CPaymentRequest prequest;
            if (!pcoinsTip->GetPaymentRequest(it_->first, prequest))
                continue;

            if (!(prequest.fState == CFund::NIL && prequest.GetState().find("expired") == string::npos))
                continue;

            auto it = std::find_if( vAddedPaymentRequestVotes.begin(), vAddedPaymentRequestVotes.end(),
                    [&prequest](const std::pair<std::string, bool>& element){ return element.first == prequest.hash.ToString();} );

            if (it != vAddedPaymentRequestVotes.end())
                continue;

            // Add to the count
            newDaoCount++;
        }
    }

    // Debug log
    info("[DAO] dao count found :" + std::to_string(newDaoCount));

    // Update the bubble
    setMenuBubble(4, newDaoCount);

    // New daos? SHOW notification
    showHideNotification(newDaoCount > 0, 0);
}

void NavCoinGUI::setMenuBubble(int index, int drak)
{
    menuBubbles[index]->setText(QString::number(drak));

    if (drak > 0)
        menuBubbles[index]->show();
    else
        menuBubbles[index]->hide();
}

void NavCoinGUI::setClientModel(ClientModel *clientModel)
{
    this->clientModel = clientModel;
    if(clientModel)
    {
        // Show warnings
        connect(clientModel, SIGNAL(alertsChanged(QString)), this, SLOT(updateAlerts(QString)));
        updateAlerts(clientModel->getStatusBarWarnings());

        // Create system tray menu (or setup the dock menu) that late to prevent users from calling actions,
        // while the client has not yet fully loaded
        createTrayIconMenu();

        // Keep up to date with client
        setNumConnections(clientModel->getNumConnections());
        connect(clientModel, SIGNAL(numConnectionsChanged(int)), this, SLOT(setNumConnections(int)));

        modalOverlay->setKnownBestHeight(clientModel->getHeaderTipHeight(), QDateTime::fromTime_t(clientModel->getHeaderTipTime()));
        setNumBlocks(clientModel->getNumBlocks(), clientModel->getLastBlockDate(), clientModel->getVerificationProgress(NULL), false);
        connect(clientModel, SIGNAL(numBlocksChanged(int,QDateTime,double,bool)), this, SLOT(setNumBlocks(int,QDateTime,double,bool)));

        // Receive and report messages from client model
        connect(clientModel, SIGNAL(message(QString,QString,unsigned int)), this, SLOT(message(QString,QString,unsigned int)));

        // Show progress dialog
        connect(clientModel, SIGNAL(showProgress(QString,int)), this, SLOT(showProgress(QString,int)));

        rpcConsole->setClientModel(clientModel);
#ifdef ENABLE_WALLET
        if(walletFrame)
        {
            walletFrame->setClientModel(clientModel);
        }
#endif // ENABLE_WALLET

        OptionsModel* optionsModel = clientModel->getOptionsModel();
        if(optionsModel)
        {
            // be aware of the tray icon disable state change reported by the OptionsModel object.
            connect(optionsModel,SIGNAL(hideTrayIconChanged(bool)),this,SLOT(setTrayIconVisible(bool)));

            // initialize the disable state of the tray icon with the current value in the model.
            setTrayIconVisible(optionsModel->getHideTrayIcon());

            // be aware of a display unit change reported by the OptionsModel object.
            connect(optionsModel,SIGNAL(displayUnitChanged(int)),this,SLOT(updateDisplayUnit(int)));

            // initialize the display units label with the current value in the model.
            updateDisplayUnit(optionsModel->getDisplayUnit());
        }
    } else {
        // Disable possibility to show main window via action
        toggleHideAction->setEnabled(false);
        if(trayIconMenu)
        {
            // Disable context menu on tray icon
            trayIconMenu->clear();
        }
    }
}

#ifdef ENABLE_WALLET
bool NavCoinGUI::addWallet(const QString& name, WalletModel *walletModel)
{
    if(!walletFrame)
        return false;
    setWalletActionsEnabled(true);
    return walletFrame->addWallet(name, walletModel);
}

bool NavCoinGUI::setCurrentWallet(const QString& name)
{
    if(!walletFrame)
        return false;
    return walletFrame->setCurrentWallet(name);
}

void NavCoinGUI::removeAllWallets()
{
    if(!walletFrame)
        return;
    setWalletActionsEnabled(false);
    walletFrame->removeAllWallets();
}

void NavCoinGUI::repairWallet()
{
    QMessageBox::StandardButton btnRetVal = QMessageBox::question(this, tr("Repair wallet"),
        tr("Client restart required to repair the wallet.") + "<br><br>" + tr("Client will be shut down. Do you want to proceed?"),
        QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel);

    if(btnRetVal == QMessageBox::Cancel)
        return;

    WriteConfigFile("zapwallettxes","2");
    WriteConfigFile("repairwallet","1");

    QApplication::quit();
}

void NavCoinGUI::updateAlerts(const QString &warnings)
{
    // Show or hide the warning
    notifications[2]->setVisible(!warnings.isEmpty());

    // Set the message
    notifications[2]->setText(tr(qPrintable(warnings)));
}

#endif // ENABLE_WALLET

void NavCoinGUI::setWalletActionsEnabled(bool enabled)
{
    overviewAction->setEnabled(enabled);
    sendCoinsAction->setEnabled(enabled);
    sendCoinsMenuAction->setEnabled(enabled);
    receiveCoinsAction->setEnabled(enabled);
    receiveCoinsMenuAction->setEnabled(enabled);
    historyAction->setEnabled(enabled);
    encryptWalletAction->setEnabled(enabled);
    backupWalletAction->setEnabled(enabled);
    changePassphraseAction->setEnabled(enabled);
    signMessageAction->setEnabled(enabled);
    verifyMessageAction->setEnabled(enabled);
    usedSendingAddressesAction->setEnabled(enabled);
    usedReceivingAddressesAction->setEnabled(enabled);
    repairWalletAction->setEnabled(enabled);
    importPrivateKeyAction->setEnabled(enabled);
    openAction->setEnabled(enabled);
}

void NavCoinGUI::createTrayIcon(const NetworkStyle *networkStyle)
{
#ifndef Q_OS_MAC
    trayIcon = new QSystemTrayIcon(this);
    QString toolTip = tr("%1 client").arg(tr(PACKAGE_NAME)) + " " + networkStyle->getTitleAddText();
    trayIcon->setToolTip(toolTip);
    trayIcon->setIcon(networkStyle->getTrayAndWindowIcon());
    trayIcon->hide();
#endif

    notificator = new Notificator(QApplication::applicationName(), trayIcon, this);
}

void NavCoinGUI::createTrayIconMenu()
{
#ifndef Q_OS_MAC
    // return if trayIcon is unset (only on non-Mac OSes)
    if (!trayIcon)
        return;

    trayIconMenu = new QMenu(this);
    trayIcon->setContextMenu(trayIconMenu);

    connect(trayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)),
            this, SLOT(trayIconActivated(QSystemTrayIcon::ActivationReason)));
#else
    // Note: On Mac, the dock icon is used to provide the tray's functionality.
    MacDockIconHandler *dockIconHandler = MacDockIconHandler::instance();
    dockIconHandler->setMainWindow((QMainWindow *)this);
    trayIconMenu = dockIconHandler->dockMenu();
#endif

    // Configuration of the tray icon (or dock icon) icon menu
    trayIconMenu->addAction(toggleHideAction);
    trayIconMenu->addSeparator();
    trayIconMenu->addAction(sendCoinsMenuAction);
    trayIconMenu->addAction(receiveCoinsMenuAction);
    trayIconMenu->addSeparator();
    trayIconMenu->addAction(signMessageAction);
    trayIconMenu->addAction(verifyMessageAction);
    trayIconMenu->addSeparator();
    trayIconMenu->addAction(optionsAction);
    trayIconMenu->addAction(openRPCConsoleAction);
#ifndef Q_OS_MAC // This is built-in on Mac
    trayIconMenu->addSeparator();
    trayIconMenu->addAction(quitAction);
#endif
}

#ifndef Q_OS_MAC
void NavCoinGUI::trayIconActivated(QSystemTrayIcon::ActivationReason reason)
{
    if(reason == QSystemTrayIcon::Trigger)
    {
        // Click on system tray icon triggers show/hide of the main window
        toggleHidden();
    }
}
#endif

void NavCoinGUI::optionsClicked()
{
    if(!clientModel || !clientModel->getOptionsModel())
        return;

    OptionsDialog dlg(this, enableWallet);
    dlg.setModel(clientModel->getOptionsModel());
    dlg.exec();
}

void NavCoinGUI::cfundProposalsClicked()
{
    if(!clientModel || !clientModel->getOptionsModel())
        return;
}

void NavCoinGUI::cfundProposalsOpen(bool fMode)
{
    if(!clientModel || !clientModel->getOptionsModel())
        return;
}

void NavCoinGUI::cfundPaymentRequestsClicked()
{
    if(!clientModel || !clientModel->getOptionsModel())
        return;
}

void NavCoinGUI::aboutClicked()
{
    if(!clientModel)
        return;

    HelpMessageDialog dlg(this, true);
    dlg.exec();
}

void NavCoinGUI::infoClicked()
{
    if(!clientModel)
        return;

    QString link = QString("https://info.navcoin.org/");
    QDesktopServices::openUrl(QUrl(link));
}

void NavCoinGUI::showDebugWindow()
{
    rpcConsole->showNormal();
    rpcConsole->show();
    rpcConsole->raise();
    rpcConsole->activateWindow();
}

void NavCoinGUI::showDebugWindowActivateConsole()
{
    rpcConsole->setTabFocus(RPCConsole::TAB_CONSOLE);
    showDebugWindow();
}

void NavCoinGUI::showHelpMessageClicked()
{
    helpMessageDialog->show();
}

#ifdef ENABLE_WALLET
void NavCoinGUI::openClicked()
{
    OpenURIDialog dlg(this);
    if(dlg.exec())
    {
        Q_EMIT receivedURI(dlg.getURI());
    }
}

void NavCoinGUI::gotoOverviewPage()
{
    setActiveMenu(0);
    overviewAction->setChecked(true);
    if (walletFrame) walletFrame->gotoOverviewPage();
}

void NavCoinGUI::gotoHistoryPage()
{
    setActiveMenu(3);
    historyAction->setChecked(true);
    if (walletFrame) walletFrame->gotoHistoryPage();
}

void NavCoinGUI::gotoCommunityFundPage()
{
    setActiveMenu(4);
    historyAction->setChecked(true);
    if (walletFrame) walletFrame->gotoCommunityFundPage();
}

void NavCoinGUI::gotoReceiveCoinsPage()
{
    setActiveMenu(2);
    receiveCoinsAction->setChecked(true);
    if (walletFrame) walletFrame->gotoReceiveCoinsPage();
}

void NavCoinGUI::gotoRequestPaymentPage()
{
    setActiveMenu(2);
    receiveCoinsAction->setChecked(true);
    if (walletFrame) walletFrame->gotoRequestPaymentPage();
}

void NavCoinGUI::gotoSendCoinsPage(QString addr)
{
    setActiveMenu(1);
    sendCoinsAction->setChecked(true);
    if (walletFrame) walletFrame->gotoSendCoinsPage(addr);
}

void NavCoinGUI::gotoSignMessageTab(QString addr)
{
    if (walletFrame) walletFrame->gotoSignMessageTab(addr);
}

void NavCoinGUI::gotoVerifyMessageTab(QString addr)
{
    if (walletFrame) walletFrame->gotoVerifyMessageTab(addr);
}
#endif // ENABLE_WALLET

void NavCoinGUI::setNumConnections(int count)
{
    QString icon;
    switch(count)
    {
        case 0: icon = ":/icons/connect_0"; break;
        case 1: case 2: case 3: icon = ":/icons/connect_1"; break;
        case 4: case 5: case 6: icon = ":/icons/connect_2"; break;
        case 7: case 8: case 9: icon = ":/icons/connect_3"; break;
        default: icon = ":/icons/connect_4"; break;
    }
    labelConnectionsIcon->setPixmap(platformStyle->IconAlt(icon).pixmap(STATUSBAR_ICONSIZE * GUIUtil::scale(), STATUSBAR_ICONSIZE * GUIUtil::scale()));
    labelConnectionsIcon->setToolTip(tr("%n active connection(s) to NavCoin network", "", count));
}

void NavCoinGUI::updateHeadersSyncProgressLabel()
{
    int64_t headersTipTime = clientModel->getHeaderTipTime();
    int headersTipHeight = clientModel->getHeaderTipHeight();
    int estHeadersLeft = (GetTime() - headersTipTime) / Params().GetConsensus().nPowTargetSpacing;
    if (estHeadersLeft > HEADER_HEIGHT_DELTA_SYNC)
        progressBarLabel->setText(tr("Syncing Headers (%1%)...").arg(QString::number(100.0 / (headersTipHeight+estHeadersLeft)*headersTipHeight, 'f', 1)));
}

bool showingVotingDialog = false;

void NavCoinGUI::setNumBlocks(int count, const QDateTime& blockDate, double nVerificationProgress, bool header)
{
#ifdef Q_OS_MAC
    // Disabling macOS App Nap on initial sync, disk and reindex operations.
    IsInitialBlockDownload() ? appNapInhibitor->disableAppNap() : appNapInhibitor->enableAppNap();
#endif

    if (modalOverlay)
    {
        if (header)
            modalOverlay->setKnownBestHeight(count, blockDate);
        else
            modalOverlay->tipUpdate(count, blockDate, nVerificationProgress);
    }

    if(!clientModel)
        return;

    // Prevent orphan statusbar messages (e.g. hover Quit in main menu, wait until chain-sync starts -> garbled text)
    statusBar()->clearMessage();

    // Acquire current block source
    enum BlockSource blockSource = clientModel->getBlockSource();
    switch (blockSource) {
        case BlockSource::NETWORK:
            if (header) {
                updateHeadersSyncProgressLabel();
                return;
            }
            progressBarLabel->setText(tr("Synchronizing with network..."));
            updateHeadersSyncProgressLabel();
            break;
        case BlockSource::DISK:
            if (header) {
                progressBarLabel->setText(tr("Indexing blocks on disk..."));
            } else {
                progressBarLabel->setText(tr("Processing blocks on disk..."));
            }
            break;
        case BlockSource::REINDEX:
            progressBarLabel->setText(tr("Reindexing blocks on disk..."));
            break;
        case BlockSource::NONE:
            if (header) {
                return;
            }
            progressBarLabel->setText(tr("Connecting to peers..."));
            break;
    }

    QString tooltip;

    QDateTime currentDate = QDateTime::currentDateTime();
    qint64 secs = blockDate.secsTo(currentDate);

    tooltip = tr("Processed %n block(s) of transaction history.", "", count);

    // Set icon state: spinning if catching up, tick otherwise
    if (secs < MAX_BLOCK_TIME_GAP) {
        tooltip = tr("Up to date") + QString(".<br>") + tooltip;
        labelBlocksIcon->setPixmap(platformStyle->IconAlt(":/icons/synced").pixmap(STATUSBAR_ICONSIZE * GUIUtil::scale(), STATUSBAR_ICONSIZE * GUIUtil::scale()));

#ifdef ENABLE_WALLET
        if(walletFrame)
        {
            showOutOfSyncWarning(false);
            modalOverlay->showHide(true, true);
        }
#endif // ENABLE_WALLET

        progressBarLabel->setVisible(false);
        progressBar->setVisible(false);
    }
    else
    {
        QString timeBehindText = GUIUtil::formatNiceTimeOffset(secs);

        progressBarLabel->setVisible(true);
        progressBar->setFormat(tr("%1 behind").arg(timeBehindText));
        progressBar->setMaximum(1000000000);
        progressBar->setValue(nVerificationProgress * 1000000000.0 + 0.5);
        progressBar->setVisible(true);

        tooltip = tr("Catching up...") + QString("<br>") + tooltip;
        labelBlocksIcon->setPixmap(platformStyle->IconAlt(QString(
            ":/movies/spinner-%1").arg(spinnerFrame, 3, 10, QChar('0')))
            .pixmap(STATUSBAR_ICONSIZE * GUIUtil::scale(), STATUSBAR_ICONSIZE * GUIUtil::scale()));
        spinnerFrame = (spinnerFrame + 1) % SPINNER_FRAMES;
        prevBlocks = count;

#ifdef ENABLE_WALLET
        if(walletFrame)
        {
            showOutOfSyncWarning(true);
            modalOverlay->showHide();
        }
#endif // ENABLE_WALLET

        tooltip += QString("<br>");
        tooltip += tr("Last received block was generated %1 ago.").arg(timeBehindText);
        tooltip += QString("<br>");
        tooltip += tr("Transactions after this will not yet be visible.");
    }

    // Don't word-wrap this (fixed-width) tooltip
    tooltip = QString("<nobr>") + tooltip + QString("</nobr>");

    labelBlocksIcon->setToolTip(tooltip);
    progressBarLabel->setToolTip(tooltip);
    progressBar->setToolTip(tooltip);
}

void NavCoinGUI::showVotingDialog()
{

  if(showingVotingDialog)
    return;

  showingVotingDialog = true;

  bool showVoting = !ExistsKeyInConfigFile("votefunding") &&
      pindexBestHeader->nTime > 1508284800 &&
      pindexBestHeader->nTime < 1510704000 &&
      GetBoolArg("-votefunding",true);

  bool vote = false;

  if(showVoting)
  {

    QMessageBox msgBox;
    msgBox.setText(tr("Important network notice."));
    msgBox.setInformativeText(tr("The Nav Coin Network is currently voting on introducing changes on the consensus protocol. As a participant in our network, we value your input and the decision ultimately is yours. Please cast your vote. <br><br>For more information on the proposal, please visit <a href=\"https://navcoin.org/community-fund\">this link</a><br><br>Would you like the Nav Coin Network to update the staking rewards to setup a decentralised community fund that will help grow the network?"));
    QAbstractButton *myYesButton = msgBox.addButton(tr("Yes"), QMessageBox::YesRole);
    msgBox.addButton(trUtf8("No"), QMessageBox::NoRole);
    msgBox.setIcon(QMessageBox::Question);
    msgBox.exec();

    if(msgBox.clickedButton() == myYesButton)
    {
        vote = true;
    }

    SoftSetArg("-votefunding",vote?"1":"0",true);

    RemoveConfigFile("votefunding",vote?"0":"1");
    WriteConfigFile("votefunding",vote?"1":"0");

  }

  showingVotingDialog = false;

}

void NavCoinGUI::message(const QString &title, const QString &message, unsigned int style, bool *ret)
{
    QString strTitle = tr("NavCoin"); // default title
    // Default to information icon
    int nMBoxIcon = QMessageBox::Information;
    int nNotifyIcon = Notificator::Information;

    QString msgType;

    // Prefer supplied title over style based title
    if (!title.isEmpty()) {
        msgType = title;
    }
    else {
        switch (style) {
        case CClientUIInterface::MSG_ERROR:
            msgType = tr("Error");
            break;
        case CClientUIInterface::MSG_WARNING:
            msgType = tr("Warning");
            break;
        case CClientUIInterface::MSG_INFORMATION:
            msgType = tr("Information");
            break;
        default:
            break;
        }
    }
    // Append title to "NavCoin - "
    if (!msgType.isEmpty())
        strTitle += " - " + msgType;

    // Check for error/warning icon
    if (style & CClientUIInterface::ICON_ERROR) {
        nMBoxIcon = QMessageBox::Critical;
        nNotifyIcon = Notificator::Critical;
    }
    else if (style & CClientUIInterface::ICON_WARNING) {
        nMBoxIcon = QMessageBox::Warning;
        nNotifyIcon = Notificator::Warning;
    }

    // Display message
    if (style & CClientUIInterface::MODAL) {
        // Check for buttons, use OK as default, if none was supplied
        QMessageBox::StandardButton buttons;
        if (!(buttons = (QMessageBox::StandardButton)(style & CClientUIInterface::BTN_MASK)))
            buttons = QMessageBox::Ok;

        showNormalIfMinimized();
        QMessageBox mBox((QMessageBox::Icon)nMBoxIcon, strTitle, message, buttons, this);
        int r = mBox.exec();
        if (ret != nullptr)
            *ret = r == QMessageBox::Ok;
    }
    else
        notificator->notify((Notificator::Class)nNotifyIcon, strTitle, message);
}

void NavCoinGUI::changeEvent(QEvent *e)
{
    QMainWindow::changeEvent(e);
#ifndef Q_OS_MAC // Ignored on Mac
    if(e->type() == QEvent::WindowStateChange)
    {
        if(clientModel && clientModel->getOptionsModel() && clientModel->getOptionsModel()->getMinimizeToTray())
        {
            QWindowStateChangeEvent *wsevt = static_cast<QWindowStateChangeEvent*>(e);
            if(!(wsevt->oldState() & Qt::WindowMinimized) && isMinimized())
            {
                QTimer::singleShot(0, this, SLOT(hide()));
                e->ignore();
            }
        }
    }
#endif
}

void NavCoinGUI::closeEvent(QCloseEvent *event)
{
#ifndef Q_OS_MAC // Ignored on Mac
    if(clientModel && clientModel->getOptionsModel())
    {
        if(!clientModel->getOptionsModel()->getMinimizeToTray() &&
           !clientModel->getOptionsModel()->getMinimizeOnClose())
        {
            // close rpcConsole in case it was open to make some space for the shutdown window
            rpcConsole->close();

            QApplication::quit();
        }
    }
#endif
    QMainWindow::closeEvent(event);
}

void NavCoinGUI::showEvent(QShowEvent *event)
{
    // enable the debug window when the main window shows up
    openRPCConsoleAction->setEnabled(true);
    aboutAction->setEnabled(true);
    infoAction->setEnabled(true);
    optionsAction->setEnabled(true);
}

#ifdef ENABLE_WALLET
void NavCoinGUI::incomingTransaction(const QString& date, int unit, const CAmount& amount, const QString& type, const QString& address, const QString& label)
{
    // On new transaction, make an info balloon
    QString msg = tr("Date: %1\n").arg(date) +
                  tr("Amount: %1\n").arg(NavCoinUnits::formatWithUnit(unit, amount, true)) +
                  tr("Type: %1\n").arg(type);
    if (!label.isEmpty())
        msg += tr("Label: %1\n").arg(label);
    else if (!address.isEmpty())
        msg += tr("Address: %1\n").arg(address);
    message((amount)<0 ? tr("Sent transaction") : tr("Incoming transaction"),
             msg, CClientUIInterface::MSG_INFORMATION);
}
#endif // ENABLE_WALLET

void NavCoinGUI::dragEnterEvent(QDragEnterEvent *event)
{
    // Accept only URIs
    if(event->mimeData()->hasUrls())
        event->acceptProposedAction();
}

void NavCoinGUI::dropEvent(QDropEvent *event)
{
    if(event->mimeData()->hasUrls())
    {
        for(const QUrl &uri: event->mimeData()->urls())
        {
            Q_EMIT receivedURI(uri.toString());
        }
    }
    event->acceptProposedAction();
}

bool NavCoinGUI::eventFilter(QObject *object, QEvent *event)
{
    // Catch status tip events
    if (event->type() == QEvent::StatusTip)
    {
        // Prevent adding text from setStatusTip(), if we currently use the status bar for displaying other stuff
        if (progressBarLabel->isVisible() || progressBar->isVisible())
            return true;
    }
    return QMainWindow::eventFilter(object, event);
}

#ifdef ENABLE_WALLET
bool NavCoinGUI::handlePaymentRequest(const SendCoinsRecipient& recipient)
{
    // URI has to be valid
    if (walletFrame && walletFrame->handlePaymentRequest(recipient))
    {
        showNormalIfMinimized();
        gotoSendCoinsPage();
        return true;
    }
    return false;
}

void NavCoinGUI::setEncryptionStatus(int status)
{
    if(fWalletUnlockStakingOnly)
    {
        labelEncryptionIcon->setPixmap(platformStyle->IconAlt(":/icons/lock_closed").pixmap(STATUSBAR_ICONSIZE * GUIUtil::scale(), STATUSBAR_ICONSIZE * GUIUtil::scale()));
        labelEncryptionIcon->setToolTip(tr("Wallet is <b>encrypted</b> and currently <b>unlocked for staking only</b>"));
        changePassphraseAction->setEnabled(false);
        unlockWalletAction->setVisible(false);
        encryptWalletAction->setEnabled(false);
    }
    else
    {
    switch(status)
    {
    case WalletModel::Unencrypted:
        labelEncryptionIcon->hide();
        encryptWalletAction->setChecked(false);
        changePassphraseAction->setEnabled(false);
        encryptWalletAction->setEnabled(true);
        unlockWalletAction->setVisible(false);
        break;
    case WalletModel::Unlocked:
        labelEncryptionIcon->show();
        labelEncryptionIcon->setPixmap(platformStyle->IconAlt(":/icons/lock_open").pixmap(STATUSBAR_ICONSIZE * GUIUtil::scale(), STATUSBAR_ICONSIZE * GUIUtil::scale()));
        labelEncryptionIcon->setToolTip(tr("Wallet is <b>encrypted</b> and currently <b>unlocked</b>"));
        unlockWalletAction->setVisible(false);
        encryptWalletAction->setChecked(true);
        changePassphraseAction->setEnabled(true);
        encryptWalletAction->setEnabled(false); // TODO: decrypt currently not supported
        break;
    case WalletModel::Locked:
        labelEncryptionIcon->show();
        labelEncryptionIcon->setPixmap(platformStyle->IconAlt(":/icons/lock_closed").pixmap(STATUSBAR_ICONSIZE * GUIUtil::scale(), STATUSBAR_ICONSIZE * GUIUtil::scale()));
        labelEncryptionIcon->setToolTip(tr("Wallet is <b>encrypted</b> and currently <b>locked</b>"));
        encryptWalletAction->setChecked(true);
        changePassphraseAction->setEnabled(true);
        unlockWalletAction->setVisible(true);
        encryptWalletAction->setEnabled(false); // TODO: decrypt currently not supported
        break;
    }
    }
    updateStakingStatus();
}
#endif // ENABLE_WALLET

void NavCoinGUI::showNormalIfMinimized(bool fToggleHidden)
{
    if(!clientModel)
        return;

    // activateWindow() (sometimes) helps with keyboard focus on Windows
    if (isHidden())
    {
        show();
        activateWindow();
    }
    else if (isMinimized())
    {
        showNormal();
        activateWindow();
    }
    else if (GUIUtil::isObscured(this))
    {
        raise();
        activateWindow();
    }
    else if(fToggleHidden)
        hide();
}

void NavCoinGUI::toggleHidden()
{
    showNormalIfMinimized(true);
}

void NavCoinGUI::detectShutdown()
{
    if (ShutdownRequested())
    {
        if(rpcConsole)
            rpcConsole->hide();
        qApp->quit();
    }
}

void NavCoinGUI::showProgress(const QString &title, int nProgress)
{
    if (nProgress == 0)
    {
        progressDialog = new QProgressDialog(title, "", 0, 100);
        progressDialog->setWindowModality(Qt::ApplicationModal);
        progressDialog->setMinimumDuration(0);
        progressDialog->setCancelButton(0);
        progressDialog->setAutoClose(false);
        progressDialog->setValue(0);
    }
    else if (nProgress == 100)
    {
        if (progressDialog)
        {
            progressDialog->close();
            progressDialog->deleteLater();
        }
    }
    else if (progressDialog)
        progressDialog->setValue(nProgress);
}

void NavCoinGUI::setTrayIconVisible(bool fHideTrayIcon)
{
    if (trayIcon)
    {
        trayIcon->setVisible(!fHideTrayIcon);
    }
}

void NavCoinGUI::showModalOverlay()
{
    if (modalOverlay && (progressBar->isVisible() || modalOverlay->isLayerVisible()))
        modalOverlay->toggleVisibility();
}

static bool ThreadSafeMessageBox(NavCoinGUI *gui, const std::string& message, const std::string& caption, unsigned int style)
{
    bool modal = (style & CClientUIInterface::MODAL);
    // The SECURE flag has no effect in the Qt GUI.
    // bool secure = (style & CClientUIInterface::SECURE);
    style &= ~CClientUIInterface::SECURE;
    bool ret = false;
    // In case of modal message, use blocking connection to wait for user to click a button
    QMetaObject::invokeMethod(gui, "message",
                               modal ? GUIUtil::blockingGUIThreadConnection() : Qt::QueuedConnection,
                               Q_ARG(QString, QString::fromStdString(caption)),
                               Q_ARG(QString, QString::fromStdString(message)),
                               Q_ARG(unsigned int, style),
                               Q_ARG(bool*, &ret));
    return ret;
}

static void UpdateDaoNewCount(NavCoinGUI *gui)
{
    // Call our instance method
    gui->updateDaoNewCount();
}

void NavCoinGUI::subscribeToCoreSignals()
{
    // Connect signals to client
    uiInterface.ThreadSafeMessageBox.connect(boost::bind(ThreadSafeMessageBox, this, _1, _2, _3));
    uiInterface.ThreadSafeQuestion.connect(boost::bind(ThreadSafeMessageBox, this, _1, _3, _4));
    uiInterface.UpdateDaoNewCount.connect(boost::bind(UpdateDaoNewCount, this));
}

void NavCoinGUI::unsubscribeFromCoreSignals()
{
    // Disconnect signals from client
    uiInterface.ThreadSafeMessageBox.disconnect(boost::bind(ThreadSafeMessageBox, this, _1, _2, _3));
    uiInterface.ThreadSafeQuestion.disconnect(boost::bind(ThreadSafeMessageBox, this, _1, _3, _4));
    uiInterface.UpdateDaoNewCount.disconnect(boost::bind(UpdateDaoNewCount, this));
}

/** When Display Units are changed on OptionsModel it will refresh the display text of the control on the status bar */
void NavCoinGUI::updateDisplayUnit(int unit)
{
    // Update the list value
    unitDisplayControl->setCurrentText(NavCoinUnits::name(unit));
}

/** Update the display currency **/
void NavCoinGUI::comboBoxChanged(int index)
{
    // Make sure we have a client model
    if (!clientModel)
        return;

    // Get the unit
    QVariant unit = unitDisplayControl->itemData(index);

    // Use the unit
    clientModel->getOptionsModel()->setDisplayUnit(unit);
}

void NavCoinGUI::toggleStaking()
{
    SetStaking(!GetStaking());

    Q_EMIT message(tr("Staking"), GetStaking() ? tr("Staking has been enabled") : tr("Staking has been disabled"),
                   CClientUIInterface::MSG_INFORMATION);
}

void NavCoinGUI::splitRewards()
{
    walletFrame->splitRewards();
}

#ifdef ENABLE_WALLET


void NavCoinGUI::updateWeight()
{
    if (!pwalletMain)
        return;

    TRY_LOCK(cs_main, lockMain);
    if (!lockMain)
        return;

    TRY_LOCK(pwalletMain->cs_wallet, lockWallet);
    if (!lockWallet)
        return;

    nWeight = pwalletMain->GetStakeWeight();
}

void NavCoinGUI::updatePrice()
{
    info("Updating prices");

    std::thread pThread{[this]{
        try {
            CURL *curl;
            std::string response;
            std::string url(
                    "https://min-api.cryptocompare.com/data/price?fsym=NAV&tsyms="
                    "BTC,"
                    "EUR,"
                    "USD,"
                    "ARS,"
                    "AUD,"
                    "BRL,"
                    "CAD,"
                    "CHF,"
                    "CLP,"
                    "CZK,"
                    "DKK,"
                    "GBP,"
                    "HKD,"
                    "HUF,"
                    "IDR,"
                    "ILS,"
                    "INR,"
                    "JPY,"
                    "KRW,"
                    "MXN,"
                    "MYR,"
                    "NOK,"
                    "NZD,"
                    "PHP,"
                    "PKR,"
                    "PLN,"
                    "RUB,"
                    "SEK,"
                    "SGD,"
                    "THB,"
                    "TRY,"
                    "TWD,"
                    "ZAR"
                    );

            // Start curl
            curl = curl_easy_init();

            // Check that we started
            if(!curl) {
                error("Update prices could not init curl");
                return;
            }

            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, priceUdateWriteCallback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
            curl_easy_perform(curl);
            curl_easy_cleanup(curl);

            // Parse json
            // NOTE: Had to use boost json as Q5's json support would not work with
            //       the json data that I was getting from the API, IDK why ¯\_(ツ)_/¯
            boost::property_tree::ptree json;
            std::istringstream jsonStream(response);
            boost::property_tree::read_json(jsonStream, json);

            // Get an instance of settings
            QSettings settings;

            // Save the values
            settings.setValue("btcFactor", (1.0 / boost::lexical_cast<double>(json.get<std::string>("BTC"))) * 100000000);
            settings.setValue("eurFactor", (1.0 / boost::lexical_cast<double>(json.get<std::string>("EUR"))) * 100000000);
            settings.setValue("usdFactor", (1.0 / boost::lexical_cast<double>(json.get<std::string>("USD"))) * 100000000);
            settings.setValue("arsFactor", (1.0 / boost::lexical_cast<double>(json.get<std::string>("ARS"))) * 100000000);
            settings.setValue("audFactor", (1.0 / boost::lexical_cast<double>(json.get<std::string>("AUD"))) * 100000000);
            settings.setValue("brlFactor", (1.0 / boost::lexical_cast<double>(json.get<std::string>("BRL"))) * 100000000);
            settings.setValue("cadFactor", (1.0 / boost::lexical_cast<double>(json.get<std::string>("CAD"))) * 100000000);
            settings.setValue("chfFactor", (1.0 / boost::lexical_cast<double>(json.get<std::string>("CHF"))) * 100000000);
            settings.setValue("clpFactor", (1.0 / boost::lexical_cast<double>(json.get<std::string>("CLP"))) * 100000000);
            settings.setValue("czkFactor", (1.0 / boost::lexical_cast<double>(json.get<std::string>("CZK"))) * 100000000);
            settings.setValue("dkkFactor", (1.0 / boost::lexical_cast<double>(json.get<std::string>("DKK"))) * 100000000);
            settings.setValue("gbpFactor", (1.0 / boost::lexical_cast<double>(json.get<std::string>("GBP"))) * 100000000);
            settings.setValue("hkdFactor", (1.0 / boost::lexical_cast<double>(json.get<std::string>("HKD"))) * 100000000);
            settings.setValue("hufFactor", (1.0 / boost::lexical_cast<double>(json.get<std::string>("HUF"))) * 100000000);
            settings.setValue("idrFactor", (1.0 / boost::lexical_cast<double>(json.get<std::string>("IDR"))) * 100000000);
            settings.setValue("ilsFactor", (1.0 / boost::lexical_cast<double>(json.get<std::string>("ILS"))) * 100000000);
            settings.setValue("inrFactor", (1.0 / boost::lexical_cast<double>(json.get<std::string>("INR"))) * 100000000);
            settings.setValue("jpyFactor", (1.0 / boost::lexical_cast<double>(json.get<std::string>("JPY"))) * 100000000);
            settings.setValue("krwFactor", (1.0 / boost::lexical_cast<double>(json.get<std::string>("KRW"))) * 100000000);
            settings.setValue("mxnFactor", (1.0 / boost::lexical_cast<double>(json.get<std::string>("MXN"))) * 100000000);
            settings.setValue("myrFactor", (1.0 / boost::lexical_cast<double>(json.get<std::string>("MYR"))) * 100000000);
            settings.setValue("nokFactor", (1.0 / boost::lexical_cast<double>(json.get<std::string>("NOK"))) * 100000000);
            settings.setValue("nzdFactor", (1.0 / boost::lexical_cast<double>(json.get<std::string>("NZD"))) * 100000000);
            settings.setValue("phpFactor", (1.0 / boost::lexical_cast<double>(json.get<std::string>("PHP"))) * 100000000);
            settings.setValue("pkrFactor", (1.0 / boost::lexical_cast<double>(json.get<std::string>("PKR"))) * 100000000);
            settings.setValue("plnFactor", (1.0 / boost::lexical_cast<double>(json.get<std::string>("PLN"))) * 100000000);
            settings.setValue("rubFactor", (1.0 / boost::lexical_cast<double>(json.get<std::string>("RUB"))) * 100000000);
            settings.setValue("sekFactor", (1.0 / boost::lexical_cast<double>(json.get<std::string>("SEK"))) * 100000000);
            settings.setValue("sgdFactor", (1.0 / boost::lexical_cast<double>(json.get<std::string>("SGD"))) * 100000000);
            settings.setValue("thbFactor", (1.0 / boost::lexical_cast<double>(json.get<std::string>("THB"))) * 100000000);
            settings.setValue("tryFactor", (1.0 / boost::lexical_cast<double>(json.get<std::string>("TRY"))) * 100000000);
            settings.setValue("twdFactor", (1.0 / boost::lexical_cast<double>(json.get<std::string>("TWD"))) * 100000000);
            settings.setValue("zarFactor", (1.0 / boost::lexical_cast<double>(json.get<std::string>("ZAR"))) * 100000000);

            if(clientModel)
                clientModel->getOptionsModel()->setDisplayUnit(clientModel->getOptionsModel()->getDisplayUnit());

            info("Updated prices");
        }
        catch (const boost::property_tree::json_parser::json_parser_error& e)
        {
            error("Could not parse price data json 'boost::property_tree::json_parser::json_parser_error'");
        }
        catch (const std::runtime_error& e)
        {
            error("Could not parse price data json 'std::runtime_error'");
        }
        catch (...)
        {
            error("Could not parse price data json 'drunk'");
        }
    }};

    // Make sure we don't get in anyones way :D
    pThread.detach();
}

size_t NavCoinGUI::priceUdateWriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    ((std::string*) userp)->append((char*) contents, size * nmemb);
    return size * nmemb;
}

void NavCoinGUI::updateStakingStatus()
{
    updateWeight();
    updateDaoNewCount();

    if(!walletFrame)
        return;

    if (!GetStaking())
    {
        labelStakingIcon->setPixmap(platformStyle->IconAlt(":/icons/staking_off").pixmap(STATUSBAR_ICONSIZE * GUIUtil::scale(), STATUSBAR_ICONSIZE * GUIUtil::scale()));
        labelStakingIcon->setToolTip(tr("Wallet Staking is <b>OFF</b>"));
    }
    else if (nLastCoinStakeSearchInterval && nWeight)
    {
        {
            LOCK(cs_main);

            CProposalMap mapProposals;

            if(pcoinsTip->GetAllProposals(mapProposals))
            {
                for (CProposalMap::iterator it_ = mapProposals.begin(); it_ != mapProposals.end(); it_++)
                {
                    CFund::CProposal proposal;
                    if (!pcoinsTip->GetProposal(it_->first, proposal))
                        continue;
                    if (proposal.fState != CFund::NIL)
                        continue;
                    auto it = std::find_if( vAddedProposalVotes.begin(), vAddedProposalVotes.end(),
                            [&proposal](const std::pair<std::string, int>& element){ return element.first == proposal.hash.ToString();} );
                    if (it == vAddedProposalVotes.end()) {
                        break;
                    }
                }
            }
        }
        {
            CPaymentRequestMap mapPaymentRequests;

            if(pcoinsTip->GetAllPaymentRequests(mapPaymentRequests))
            {
                for (CPaymentRequestMap::iterator it_ = mapPaymentRequests.begin(); it_ != mapPaymentRequests.end(); it_++)
                {
                    CFund::CPaymentRequest prequest;

                    if (!pcoinsTip->GetPaymentRequest(it_->first, prequest))
                        continue;

                    if (prequest.fState != CFund::NIL)
                        continue;
                    auto it = std::find_if( vAddedPaymentRequestVotes.begin(), vAddedPaymentRequestVotes.end(),
                            [&prequest](const std::pair<std::string, int>& element){ return element.first == prequest.hash.ToString();} );
                    if (it == vAddedPaymentRequestVotes.end()) {
                        break;
                    }
                }
            }
        }

        uint64_t nWeight = this->nWeight;
        uint64_t nNetworkWeight = GetPoSKernelPS();
        int nBestHeight = pindexBestHeader->nHeight;

        unsigned nEstimateTime = GetTargetSpacing(nBestHeight) * nNetworkWeight / nWeight;

        QString text = tr("You are staking");

        if (nEstimateTime > 60 && GetBoolArg("showexpectedstaketime", false))
        {
            if (nEstimateTime < 60*60)
            {
                text = tr("Expected time to earn reward is %n minute(s)", "", nEstimateTime/60);
            }
            else if (nEstimateTime < 24*60*60)
            {
                text = tr("Expected time to earn reward is %n hour(s)", "", nEstimateTime/(60*60));
            }
            else
            {
                text = tr("Expected time to earn reward is %n day(s)", "", nEstimateTime/(60*60*24));
            }
        }

        nWeight /= COIN;
        nNetworkWeight /= COIN;

        labelStakingIcon->setPixmap(platformStyle->IconAlt(":/icons/staking_on").pixmap(STATUSBAR_ICONSIZE * GUIUtil::scale(), STATUSBAR_ICONSIZE * GUIUtil::scale()));
        labelStakingIcon->setToolTip(text);
    }
    else
    {
        QString text = tr("Not staking, please wait");

        if (pwalletMain && pwalletMain->IsLocked())
            text = tr("Not staking because wallet is locked");
        else if (vNodes.empty())
            text = tr("Not staking because wallet is offline");
        else if (IsInitialBlockDownload())
            text = tr("Not staking because wallet is syncing");
        else if (!nWeight)
            text = tr("Not staking because you don't have mature coins");

        labelStakingIcon->setPixmap(platformStyle->IconAlt(":/icons/staking_off").pixmap(STATUSBAR_ICONSIZE * GUIUtil::scale(), STATUSBAR_ICONSIZE * GUIUtil::scale()));
        labelStakingIcon->setToolTip(text);
    }
}

#endif
