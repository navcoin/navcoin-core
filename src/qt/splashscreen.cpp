// Copyright (c) 2011-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#if defined(HAVE_CONFIG_H)
#include <config/navcoin-config.h>
#endif

#include <qt/splashscreen.h>

#include <qt/networkstyle.h>

#include <clientversion.h>
#include <guiutil.h>
#include <init.h>
#include <util.h>
#include <ui_interface.h>
#include <version.h>

#ifdef ENABLE_WALLET
#include <wallet/wallet.h>
#endif

#include <QApplication>
#include <QCloseEvent>
#include <QDesktopWidget>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QTimer>

SplashScreen::SplashScreen(Qt::WindowFlags f, const NetworkStyle *networkStyle) :
    QWidget(0, f)
{
    // define text to place
    QString titleText       = tr(PACKAGE_NAME);
    QString versionText     = QString::fromStdString(FormatFullVersion());
    QString titleAddText    = networkStyle->getTitleAddText();

    // Size of the splash screen
    splashSize = QSize(480 * scale(), 320 * scale());

    // Size of the logo
    QSize logoSize(400 * scale(), 95 * scale());

    // Check if we have more text (IE testnet/devnet)
    if(!titleAddText.isEmpty())
        versionText += " <span style='font-weight: bold;'>" + titleAddText + "</span>";

    // Margin for the splashscreen
    int margin = 10 * scale();

    // Make the splashLayout
    QVBoxLayout* splashLayout = new QVBoxLayout(this);
    splashLayout->setContentsMargins(0, 0, 0, 0);
    splashLayout->setSpacing(0);

    // Make the layout for widgets
    QVBoxLayout* layout = new QVBoxLayout();
    layout->setContentsMargins(margin, margin, margin, margin);
    layout->setSpacing(0);
    splashLayout->addLayout(layout);

    // Build the new versionLabel
    QLabel* versionLabel = new QLabel();
    versionLabel->setText(versionText);
    versionLabel->setAlignment(Qt::AlignRight | Qt::AlignTop);
    versionLabel->setObjectName("SplashVersionLabel");
    layout->addWidget(versionLabel);

    // Load the icon
    QPixmap icon = QPixmap(":icons/navcoin_full").scaled(logoSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);

    // Build the new logo
    QLabel* logo = new QLabel();
    logo->setPixmap(icon);
    logo->setAlignment(Qt::AlignCenter);
    layout->addWidget(logo);

    // Build the new statusLabel
    statusLabel = new QLabel();
    statusLabel->setAlignment(Qt::AlignCenter | Qt::AlignBottom);
    statusLabel->setObjectName("SplashStatusLabel");
    layout->addWidget(statusLabel);

    // Build the splashBar
    QWidget* splashBar = new QWidget();
    splashBar->setMaximumHeight(7 * scale());
    splashBar->setObjectName("SplashBar");

    // Make the splashBarLayout
    splashBarLayout = new QHBoxLayout(splashBar);
    splashBarLayout->setContentsMargins(0, 0, 0, 0);
    splashBarLayout->setSpacing(0);

    // Build the splashBarInner section
    splashBarInner = new QWidget();
    splashBarInner->setObjectName("SplashBarInner");
    splashBarInner->setFixedWidth(0);
    splashBarLayout->addWidget(splashBarInner);

    // Add the splashbar to the splashLayout
    splashLayout->addWidget(splashBar);

    timerProgress = new QTimer();
    connect(timerProgress, SIGNAL(timeout()), this, SLOT(updateProgress()));
    timerProgress->start(50); // Update every 50ms

    // Set window title
    setWindowTitle(titleText + " " + titleAddText);
    QPalette pal = palette();

    // Set white background
    pal.setColor(QPalette::Background, Qt::white);
    setAutoFillBackground(true);
    setPalette(pal);

    // Resize window and move to center of desktop, disallow resizing
    QRect r(QPoint(), splashSize);
    resize(r.size());
    setFixedSize(r.size());
    move(QApplication::desktop()->screenGeometry().center() - r.center());

    subscribeToCoreSignals();
}

SplashScreen::~SplashScreen()
{
    unsubscribeFromCoreSignals();

    // Delete the timer
    delete timerProgress;
}

float SplashScreen::scale()
{
    return GUIUtil::scale();
}

void SplashScreen::slotFinish(QWidget *mainWin)
{
    Q_UNUSED(mainWin);

    /* If the window is minimized, hide() will be ignored. */
    /* Make sure we de-minimize the splashscreen window before hiding */
    if (isMinimized())
        showNormal();
    hide();

    // Stop the tick tocking
    timerProgress->stop();
}

static void InitMessage(SplashScreen *splash, const std::string &message)
{
    QMetaObject::invokeMethod(splash, "showMessage",
        Qt::QueuedConnection,
        Q_ARG(QString, QString::fromStdString(message)),
        Q_ARG(QColor, QColor(55,55,55)));
}

static void ShowProgress(SplashScreen *splash, const std::string &title, int nProgress)
{
    InitMessage(splash, title + strprintf("%d", nProgress) + (nProgress <= 100 ? "%" : ""));
}

#ifdef ENABLE_WALLET
static void ConnectWallet(SplashScreen *splash, CWallet* wallet)
{
    wallet->ShowProgress.connect(boost::bind(ShowProgress, splash, _1, _2));
}
#endif

void SplashScreen::subscribeToCoreSignals()
{
    // Connect signals to client
    uiInterface.InitMessage.connect(boost::bind(InitMessage, this, _1));
    uiInterface.ShowProgress.connect(boost::bind(ShowProgress, this, _1, _2));
#ifdef ENABLE_WALLET
    uiInterface.LoadWallet.connect(boost::bind(ConnectWallet, this, _1));
#endif
}

void SplashScreen::unsubscribeFromCoreSignals()
{
    // Disconnect signals from client
    uiInterface.InitMessage.disconnect(boost::bind(InitMessage, this, _1));
    uiInterface.ShowProgress.disconnect(boost::bind(ShowProgress, this, _1, _2));
#ifdef ENABLE_WALLET
    if(pwalletMain)
        pwalletMain->ShowProgress.disconnect(boost::bind(ShowProgress, this, _1, _2));
#endif
}

void SplashScreen::showMessage(const QString &message, const QColor &color)
{
    // Update the text for the statusLabel
    statusLabel->setText(message);
    update();
}

void SplashScreen::updateProgress()
{
    // What is the progressWidth ?
    static int progressWidth = splashSize.width() * 0.01f;

    // Are we shrinking?
    static bool shrinking = false;

    // Get the current width
    int barWidth = splashBarInner->width();

    // Are we shrinking?
    if (shrinking)
        barWidth -= progressWidth;
    else
        barWidth += progressWidth;

    // Check if progress is full
    if (barWidth >= splashSize.width())
        shrinking = true; // Start shrinking

    // Check if progress is empty
    if (barWidth <= 0)
        shrinking = false; // Start growing

    // Set the new width
    splashBarInner->setFixedWidth(barWidth);
    splashBarLayout->setAlignment((shrinking ? Qt::AlignRight : Qt::AlignLeft));
}

void SplashScreen::closeEvent(QCloseEvent *event)
{
    StartShutdown(); // allows an "emergency" shutdown during startup
    event->ignore();
}
