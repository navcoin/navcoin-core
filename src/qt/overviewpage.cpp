// Copyright (c) 2011-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/overviewpage.h>
#include <ui_overviewpage.h>

#include <qt/navcoinunits.h>
#include <qt/clientmodel.h>
#include <qt/guiconstants.h>
#include <qt/guiutil.h>
#include <qt/optionsmodel.h>
#include <qt/platformstyle.h>
#include <qt/transactionfilterproxy.h>
#include <qt/transactiontablemodel.h>
#include <qt/walletmodel.h>
#include <qt/walletframe.h>
#include <qt/askpassphrasedialog.h>
#include <util.h>

#include <QAbstractItemDelegate>
#include <QPainter>

#define DECORATION_SIZE 30
#define NUM_ITEMS 25

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
        int xspace = DECORATION_SIZE + 8;
        int ypad = 6;
        int halfheight = (mainRect.height() - 2*ypad)/2;
        QRect decorationRect(mainRect.left(), mainRect.top()+ypad, DECORATION_SIZE, DECORATION_SIZE);
        QRect amountRect(mainRect.left() + xspace, mainRect.top()+ypad, mainRect.width() - xspace - 10, halfheight);
        QRect addressRect(mainRect.left() + xspace, mainRect.top()+ypad+halfheight, mainRect.width() - xspace, halfheight);
        icon = platformStyle->Icon(icon);
        icon.paint(painter, decorationRect);

        QDateTime date = index.data(TransactionTableModel::DateRole).toDateTime();
        QString address = index.data(Qt::DisplayRole).toString();
        qint64 amount = index.data(TransactionTableModel::AmountRole).toLongLong();
        bool confirmed = index.data(TransactionTableModel::ConfirmedRole).toBool();
        QVariant value = index.data(Qt::ForegroundRole);
        QColor foreground = option.palette.color(QPalette::Text);
        if(value.canConvert<QBrush>())
        {
            QBrush brush = qvariant_cast<QBrush>(value);
            foreground = brush.color();
        }

        painter->setPen(foreground);
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
        painter->drawText(amountRect, Qt::AlignRight|Qt::AlignVCenter, amountText);

        painter->setPen(option.palette.color(QPalette::Text));
        painter->drawText(amountRect, Qt::AlignLeft|Qt::AlignVCenter, GUIUtil::dateTimeStr(date));

        painter->setPen(COLOR_UNCONFIRMED);
        painter->drawLine(mainRect.bottomLeft(), mainRect.bottomRight());

        painter->restore();
    }

    inline QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
    {
        return QSize(DECORATION_SIZE, DECORATION_SIZE*1.4);
    }

    int unit;
    const PlatformStyle *platformStyle;

};
#include <qt/overviewpage.moc>

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
    currentTotalBalance(-1),
    currentWatchOnlyBalance(-1),
    currentWatchUnconfBalance(-1),
    currentWatchImmatureBalance(-1),
    currentWatchOnlyTotalBalance(-1),
    txdelegate(new TxViewDelegate(platformStyle)),
    filter(0)
{
    ui->setupUi(this);

    // Recent transactions
    ui->listTransactions->setItemDelegate(txdelegate);
    ui->listTransactions->setIconSize(QSize(DECORATION_SIZE, DECORATION_SIZE));
    ui->listTransactions->setAttribute(Qt::WA_MacShowFocusRect, false);

    connect(ui->listTransactions, SIGNAL(clicked(QModelIndex)), this, SLOT(handleTransactionClicked(QModelIndex)));

    // start with displaying the "out of sync" warnings
    updateStakeReportNow();
}

void OverviewPage::handleTransactionClicked(const QModelIndex &index)
{
    if(filter)
        Q_EMIT transactionClicked(filter->mapToSource(index));
}

OverviewPage::~OverviewPage()
{
    delete ui;
}

void OverviewPage::setStakingStats(QString day, QString week, QString month, QString year, QString all)
{
    ui->label24hStakingStats->setText(day);
    ui->label7dStakingStats->setText(week);
    ui->label30dStakingStats->setText(month);
}

void OverviewPage::setBalance(
    const CAmount& balance,
    const CAmount& unconfirmedBalance,
    const CAmount& stakingBalance, // This is actually staked immature
    const CAmount& immatureBalance,
    const CAmount& watchOnlyBalance,
    const CAmount& watchUnconfBalance,
    const CAmount& watchImmatureBalance,
    const CAmount& coldStakingBalance
) {
    int unit = walletModel->getOptionsModel()->getDisplayUnit();
    currentBalance = balance;
    currentUnconfirmedBalance = unconfirmedBalance;
    currentStakingBalance = stakingBalance;
    currentColdStakingBalance = coldStakingBalance;
    currentImmatureBalance = immatureBalance;
    currentTotalBalance = balance + unconfirmedBalance + immatureBalance;
    currentWatchOnlyBalance = watchOnlyBalance;
    currentWatchUnconfBalance = watchUnconfBalance;
    currentWatchImmatureBalance = watchImmatureBalance;
    currentWatchOnlyTotalBalance = watchOnlyBalance + watchUnconfBalance + watchImmatureBalance;
    ui->labelBalance->setText(NavCoinUnits::formatWithUnit(unit, balance, false, NavCoinUnits::separatorAlways));
    ui->labelUnconfirmed->setText(NavCoinUnits::formatWithUnit(unit, unconfirmedBalance, false, NavCoinUnits::separatorAlways));
    ui->labelColdStaking->setText(NavCoinUnits::formatWithUnit(unit, currentColdStakingBalance, false, NavCoinUnits::separatorAlways));
    ui->labelImmature->setText(NavCoinUnits::formatWithUnit(unit, currentImmatureBalance, false, NavCoinUnits::separatorAlways));
    ui->labelWatchedBalance->setText(NavCoinUnits::formatWithUnit(unit, currentWatchOnlyTotalBalance, false, NavCoinUnits::separatorAlways));
    ui->labelTotal->setText(NavCoinUnits::formatWithUnit(unit, currentTotalBalance + currentWatchOnlyTotalBalance, false, NavCoinUnits::separatorAlways));

    updateStakeReportNow();

    uiInterface.SetBalance(currentBalance, currentUnconfirmedBalance, currentImmatureBalance);
}

// show/hide watch-only labels
void OverviewPage::updateWatchOnlyLabels(bool showWatchOnly)
{

}

void OverviewPage::setClientModel(ClientModel *model)
{
    this->clientModel = model;
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
        connect(model, SIGNAL(balanceChanged(CAmount, CAmount, CAmount, CAmount, CAmount, CAmount, CAmount, CAmount)), this, SLOT(setBalance(CAmount, CAmount, CAmount, CAmount, CAmount, CAmount, CAmount, CAmount)));
        connect(model, SIGNAL(balanceChanged(CAmount, CAmount, CAmount, CAmount, CAmount, CAmount, CAmount, CAmount)), this, SLOT(updateStakeReportbalanceChanged(CAmount, CAmount, CAmount, CAmount, CAmount, CAmount, CAmount, CAmount)));

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
    if (!walletModel || !walletModel->getOptionsModel())
        return;

    static vStakePeriodRange_T aRange;
    int nItemCounted=0;

    if (fImmediate) nLastReportUpdate = 0;

    // Skip report recalc if not immediate or before 5 minutes from last
    if (GetTime() - nLastReportUpdate > 300)
    {
        // Load the range
        aRange = PrepareRangeForStakeReport();

        // Get the subtotals
        nItemCounted = GetsStakeSubTotal(aRange);

        // Save the last update
        nLastReportUpdate = GetTime();
    }

    int unit = walletModel->getOptionsModel()->getDisplayUnit();

    CAmount amount24h = aRange[30].Total;
    CAmount amount7d  = aRange[31].Total;
    CAmount amount30d = aRange[32].Total;
    CAmount amount1y  = aRange[33].Total;
    CAmount amountAll = aRange[34].Total;

    ui->label24hStakingStats->setText(NavCoinUnits::formatWithUnit(unit, amount24h, false, NavCoinUnits::separatorAlways));
    ui->label7dStakingStats->setText(NavCoinUnits::formatWithUnit(unit, amount7d, false, NavCoinUnits::separatorAlways));
    ui->label30dStakingStats->setText(NavCoinUnits::formatWithUnit(unit, amount30d, false, NavCoinUnits::separatorAlways));
    ui->label1yStakingStats->setText(NavCoinUnits::formatWithUnit(unit, amount1y, false, NavCoinUnits::separatorAlways));
    ui->labelallStakingStats->setText(NavCoinUnits::formatWithUnit(unit, amountAll, false, NavCoinUnits::separatorAlways));

    uiInterface.SetStaked(amountAll, amount24h, amount7d);
}


void OverviewPage::updateStakeReportbalanceChanged(CAmount, CAmount, CAmount, CAmount, CAmount, CAmount, CAmount)
{
    OverviewPage::updateStakeReportNow();
}

void OverviewPage::updateStakeReportNow()
{
    updateStakeReport(true);
}

void OverviewPage::on_showStakingSetup_clicked()
{
    SplitRewardsDialog dlg(this);
    dlg.setModel(walletModel);
    dlg.exec();
}
