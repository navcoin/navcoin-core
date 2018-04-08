// Copyright (c) 2011-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "overviewpage.h"
#include "ui_overviewpage.h"
#include "skinize.h"

#include "navcoinunits.h"
#include "clientmodel.h"
#include "guiconstants.h"
#include "guiutil.h"
#include "optionsmodel.h"
#include "platformstyle.h"
#include "transactionfilterproxy.h"
#include "transactiontablemodel.h"
#include "walletmodel.h"
#include "walletframe.h"
#include "askpassphrasedialog.h"
#include "util.h"


#include <QAbstractItemDelegate>
#include <QPainter>

#define DECORATION_SIZE 17
#define NUM_ITEMS 5

class TxViewDelegate : public QAbstractItemDelegate
{
    Q_OBJECT
public:
    TxViewDelegate(const PlatformStyle *platformStyle):
        QAbstractItemDelegate(), unit(NavCoinUnits::NAV),
        platformStyle(platformStyle)
    {

    }

    inline void paint(QPainter *painter, const QStyleOptionViewItem &option,
                      const QModelIndex &index ) const
    {
        painter->save();

        QIcon icon = qvariant_cast<QIcon>(index.data(TransactionTableModel::RawDecorationRole));
        QRect mainRect = option.rect;
        QRect decorationRect(mainRect.left(), mainRect.top()+DECORATION_SIZE, DECORATION_SIZE, DECORATION_SIZE);
        int xspace = DECORATION_SIZE + 6;
        int ypad = 1;
        int halfheight = (mainRect.height() - 3*ypad - 4)/3 ;
        QRect amountRect(mainRect.left() + xspace, mainRect.top()+ypad+halfheight, 150, DECORATION_SIZE);
        QRect addressRect(mainRect.left() + xspace, mainRect.top()+ypad, 300, DECORATION_SIZE);
        QRect dateRect(mainRect.left() + xspace, mainRect.top()+ypad+halfheight*2, 150, DECORATION_SIZE);
        icon = platformStyle->SingleColorIcon(icon);
        icon.paint(painter, decorationRect);

        QDateTime date = index.data(TransactionTableModel::DateRole).toDateTime();
        QString address = index.data(Qt::DisplayRole).toString();
        qint64 amount = index.data(TransactionTableModel::AmountRole).toLongLong();
        bool confirmed = index.data(TransactionTableModel::ConfirmedRole).toBool();
        QVariant value = index.data(Qt::ForegroundRole);
        QColor foreground = option.palette.color(QPalette::Text);

        painter->setPen(QColor(60,60,60));
        QRect boundingRect;
        painter->drawText(addressRect, Qt::AlignLeft|Qt::AlignVCenter, address, &boundingRect);

        if (index.data(TransactionTableModel::WatchonlyRole).toBool())
        {
            QIcon iconWatchonly = qvariant_cast<QIcon>(index.data(TransactionTableModel::WatchonlyDecorationRole));
            QRect watchonlyRect(boundingRect.right() + 5, mainRect.top()+ypad+halfheight, 16, halfheight);
            iconWatchonly.paint(painter, watchonlyRect);
        }

        if(amount < 0)
        {
            foreground = COLOR_NEGATIVE;
        }
        else if(!confirmed)
        {
            foreground = COLOR_UNCONFIRMED;
        }
        else
        {
            foreground = COLOR_POSITIVE;
        }
        painter->setPen(foreground);
        QString amountText = NavCoinUnits::formatWithUnit(unit, amount, true, NavCoinUnits::separatorAlways);
        if(!confirmed)
        {
            amountText = QString("[") + amountText + QString("]");
        }
        painter->drawText(amountRect, Qt::AlignLeft|Qt::AlignVCenter, amountText);

        painter->setPen(COLOR_BAREADDRESS);
        painter->drawText(dateRect, Qt::AlignLeft|Qt::AlignVCenter, GUIUtil::dateTimeStr(date));

        painter->restore();
    }

    inline QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
    {
        return QSize(DECORATION_SIZE, DECORATION_SIZE*2.9 + 4);
    }

    int unit;
    const PlatformStyle *platformStyle;

};
#include "overviewpage.moc"

OverviewPage::OverviewPage(const PlatformStyle *platformStyle, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::OverviewPage),
    clientModel(0),
    walletModel(0),
    currentBalance(-1),
    currentUnconfirmedBalance(-1),
    currentStakingBalance(-1),
    currentColdStakingBalance(-1),
    currentImmatureBalance(-1),
    currentWatchOnlyBalance(-1),
    currentWatchUnconfBalance(-1),
    currentWatchImmatureBalance(-1),
    txdelegate(new TxViewDelegate(platformStyle)),
    filter(0)
{
    ui->setupUi(this);

    // use a SingleColorIcon for the "out of sync warning" icon
    QIcon icon = platformStyle->SingleColorIcon(":/icons/warning");
    icon.addPixmap(icon.pixmap(QSize(64,64), QIcon::Normal), QIcon::Disabled); // also set the disabled icon because we are using a disabled QPushButton to work around missing HiDPI support of QLabel (https://bugreports.qt.io/browse/QTBUG-42503)
    ui->labelWalletStatus->setIcon(icon);

    // Recent transactions
    ui->listTransactions->setItemDelegate(txdelegate);
    ui->listTransactions->setIconSize(QSize(DECORATION_SIZE, DECORATION_SIZE));
    ui->listTransactions->setMinimumHeight(NUM_ITEMS * 3 * (DECORATION_SIZE + 2));
    ui->listTransactions->setAttribute(Qt::WA_MacShowFocusRect, false);

    connect(ui->listTransactions, SIGNAL(clicked(QModelIndex)), this, SLOT(handleTransactionClicked(QModelIndex)));
    connect(ui->unlockStakingButton, SIGNAL(clicked()), this, SLOT(unlockWalletStaking()));

    // start with displaying the "out of sync" warnings
    showOutOfSyncWarning(true);
    updateStakeReportNow();
}

void OverviewPage::handleTransactionClicked(const QModelIndex &index)
{
    if(filter)
        Q_EMIT transactionClicked(filter->mapToSource(index));
}

void OverviewPage::showLockStaking(bool status)
{
    ui->unlockStakingButton->setVisible(status);
}

OverviewPage::~OverviewPage()
{
    delete ui;
}

void OverviewPage::setStakingStats(QString day, QString week, QString month)
{
    ui->label24hStakingStats->setText(day);
    ui->label7dStakingStats->setText(week);
    ui->label30dStakingStats->setText(month);
}

void OverviewPage::setVotingStatus(QString text)
{
    ui->votingStatusLabel->setText(text);
}

void OverviewPage::setBalance(const CAmount& balance, const CAmount& unconfirmedBalance, const CAmount& stakingBalance, const CAmount& immatureBalance,
                              const CAmount& watchOnlyBalance, const CAmount& watchUnconfBalance, const CAmount& watchImmatureBalance,
                              const CAmount& coldStakingBalance)
{
    int unit = walletModel->getOptionsModel()->getDisplayUnit();
    currentBalance = balance;
    currentUnconfirmedBalance = unconfirmedBalance;
    currentStakingBalance = stakingBalance;
    currentColdStakingBalance = coldStakingBalance;
    currentImmatureBalance = immatureBalance;
    currentWatchOnlyBalance = watchOnlyBalance;
    currentWatchUnconfBalance = watchUnconfBalance;
    currentWatchImmatureBalance = watchImmatureBalance;
    ui->labelBalance->setText(NavCoinUnits::formatWithUnit(unit, balance, false, NavCoinUnits::separatorAlways));
    ui->labelUnconfirmed->setText(NavCoinUnits::formatWithUnit(unit, unconfirmedBalance, false, NavCoinUnits::separatorAlways));
    ui->labelStaking->setText(NavCoinUnits::formatWithUnit(unit, stakingBalance, false, NavCoinUnits::separatorAlways));
    ui->labelColdStaking->setText(NavCoinUnits::formatWithUnit(unit, coldStakingBalance, false, NavCoinUnits::separatorAlways));
    ui->labelImmature->setText(NavCoinUnits::formatWithUnit(unit, immatureBalance, false, NavCoinUnits::separatorAlways));
    ui->labelTotal->setText(NavCoinUnits::formatWithUnit(unit, balance + unconfirmedBalance + stakingBalance, false, NavCoinUnits::separatorAlways));

    bool showStaking = stakingBalance != 0;

    ui->labelStaking->setVisible(showStaking);
    ui->labelStakingText->setVisible(showStaking);

    bool showColdStaking = coldStakingBalance != 0;

    ui->labelColdStaking->setVisible(showColdStaking);
    ui->labelColdStakingText->setVisible(showColdStaking);

    // only show immature (newly mined) balance if it's non-zero, so as not to complicate things
    // for the non-mining users
    bool showImmature = false;
    bool showWatchOnlyImmature = watchImmatureBalance != 0;

    // for symmetry reasons also show immature label when the watch-only one is shown
    ui->labelImmature->setVisible(false);
    ui->labelImmatureText->setVisible(false);
}

// show/hide watch-only labels
void OverviewPage::updateWatchOnlyLabels(bool showWatchOnly)
{

}

void OverviewPage::setStakingStatus(QString text)
{
    ui->stakingStatusLabel->setText(text);
};


void OverviewPage::setStatusTitleBlocks(QString text)
{
    ui->statusTitleBlocks->setText(text);
}

void OverviewPage::setStatusTitleConnections(QString text)
{
    ui->statusTitleConnections->setText(text);

}

void OverviewPage::setStatusTitle(QString text)
{
    ui->statusTitle->setText(text);
}

void OverviewPage::showStatusTitleConnections(){
    ui->statusTitleConnections->show();
};
void OverviewPage::hideStatusTitleConnections(){
    ui->statusTitleConnections->hide();
};
void OverviewPage::showStatusTitleBlocks(){
    ui->statusTitleBlocks->show();
};
void OverviewPage::hideStatusTitleBlocks(){
    ui->statusTitleBlocks->hide();
};

void OverviewPage::setClientModel(ClientModel *model)
{
    this->clientModel = model;
    if(model)
    {
        // Show warning if this is a prerelease version
        connect(model, SIGNAL(alertsChanged(QString)), this, SLOT(updateAlerts(QString)));
        updateAlerts(model->getStatusBarWarnings());
    }
}

void OverviewPage::setWalletModel(WalletModel *model)
{
    this->walletModel = model;
    if(model && model->getOptionsModel())
    {
        // Set up transaction list
        filter = new TransactionFilterProxy();
        filter->setSourceModel(model->getTransactionTableModel());
        filter->setLimit(NUM_ITEMS);
        filter->setDynamicSortFilter(true);
        filter->setSortRole(Qt::EditRole);
        filter->setShowInactive(false);
        filter->sort(TransactionTableModel::Date, Qt::DescendingOrder);

        ui->listTransactions->setModel(filter);
        ui->listTransactions->setModelColumn(TransactionTableModel::ToAddress);

        // Keep up to date with wallet
        setBalance(model->getBalance(), model->getUnconfirmedBalance(), model->getStake(), model->getImmatureBalance(),
                   model->getWatchBalance(), model->getWatchUnconfirmedBalance(), model->getWatchImmatureBalance(),
                   model->getColdStakingBalance());
        connect(model, SIGNAL(balanceChanged(CAmount,CAmount,CAmount,CAmount,CAmount,CAmount,CAmount,CAmount)), this, SLOT(setBalance(CAmount,CAmount,CAmount,CAmount,CAmount,CAmount,CAmount,CAmount)));
        connect(model, SIGNAL(balanceChanged(CAmount, CAmount, CAmount, CAmount,CAmount,CAmount,CAmount,CAmount)), this, SLOT(updateStakeReportbalanceChanged(CAmount, CAmount, CAmount,CAmount, CAmount,CAmount,CAmount)));

        connect(model->getOptionsModel(), SIGNAL(displayUnitChanged(int)), this, SLOT(updateDisplayUnit()));

        updateWatchOnlyLabels(model->haveWatchOnly());
        connect(model, SIGNAL(notifyWatchonlyChanged(bool)), this, SLOT(updateWatchOnlyLabels(bool)));
    }

    // update the display unit, to not use the default ("NAV")
    updateDisplayUnit();
}

void OverviewPage::updateDisplayUnit()
{
    if(walletModel && walletModel->getOptionsModel())
    {
        if(currentBalance != -1)
            setBalance(currentBalance, currentUnconfirmedBalance, currentStakingBalance, currentImmatureBalance,
                       currentWatchOnlyBalance, currentWatchUnconfBalance, currentWatchImmatureBalance, currentColdStakingBalance);

        // Update txdelegate->unit with the current unit
        txdelegate->unit = walletModel->getOptionsModel()->getDisplayUnit();

        ui->listTransactions->update();
    }
}

void OverviewPage::unlockWalletStaking()
{
    if(!walletModel)
        return;
    // Unlock wallet when requested by wallet model
    if (walletModel->getEncryptionStatus() == WalletModel::Locked)
    {
        AskPassphraseDialog dlg(AskPassphraseDialog::UnlockStaking, this);
        dlg.setModel(walletModel);
        dlg.exec();
    }
}

void OverviewPage::updateAlerts(const QString &warnings)
{
    this->ui->labelAlerts->setVisible(!warnings.isEmpty());
    this->ui->labelAlerts->setText(warnings);
}

void OverviewPage::showOutOfSyncWarning(bool fShow)
{
    ui->labelWalletStatus->setVisible(fShow);
}

using namespace boost;
using namespace std;

struct StakePeriodRange_T {
    int64_t Start;
    int64_t End;
    int64_t Total;
    int Count;
    string Name;
};

typedef vector<StakePeriodRange_T> vStakePeriodRange_T;

extern vStakePeriodRange_T PrepareRangeForStakeReport();
extern int GetsStakeSubTotal(vStakePeriodRange_T& aRange);

void OverviewPage::updateStakeReport(bool fImmediate=false)
{
    static vStakePeriodRange_T aRange;
    int nItemCounted=0;

    if (fImmediate) nLastReportUpdate = 0;

    if (this->isHidden())
        return;

    int64_t nTook = GetTimeMillis();

    // Skip report recalc if not immediate or before 5 minutes from last
    if (GetTime() - nLastReportUpdate > 300)
    {

        aRange = PrepareRangeForStakeReport();

        // get subtotal calc
        nItemCounted = GetsStakeSubTotal(aRange);

        nLastReportUpdate = GetTime();

        nTook = GetTimeMillis() - nTook;

    }

    //int64_t nTook2 = GetTimeMillis();

    int i=30;

    int unit = walletModel->getOptionsModel()->getDisplayUnit();

    ui->label24hStakingStats->setText(NavCoinUnits::formatWithUnit(unit, aRange[i++].Total, false, NavCoinUnits::separatorAlways));
    ui->label7dStakingStats->setText(NavCoinUnits::formatWithUnit(unit, aRange[i++].Total, false, NavCoinUnits::separatorAlways));
    ui->label30dStakingStats->setText(NavCoinUnits::formatWithUnit(unit, aRange[i++].Total, false, NavCoinUnits::separatorAlways));

}


void OverviewPage::updateStakeReportbalanceChanged(qint64, qint64, qint64, qint64, qint64, qint64, qint64)
{
    OverviewPage::updateStakeReportNow();
}

void OverviewPage::updateStakeReportNow()
{
    updateStakeReport(true);
}
