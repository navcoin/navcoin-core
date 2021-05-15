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
#include <main.h>
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
        QAbstractItemDelegate(), unit(NavcoinUnits::NAV),
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
        bool fPrivate = index.data(TransactionTableModel::PrivateRole).toBool();
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
        QString amountText = NavcoinUnits::formatWithUnit(unit, amount, true, NavcoinUnits::separatorAlways, fPrivate);
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
    txdelegate(new TxViewDelegate(platformStyle)),
    filter(0)
{
    ui->setupUi(this);

    // Recent transactions
    ui->listTransactions->setItemDelegate(txdelegate);
    ui->listTransactions->setIconSize(QSize(DECORATION_SIZE, DECORATION_SIZE));
    ui->listTransactions->setAttribute(Qt::WA_MacShowFocusRect, false);

    connect(ui->listTransactions, SIGNAL(clicked(QModelIndex)), this, SLOT(handleTransactionClicked(QModelIndex)));
    connect(ui->swapButton, SIGNAL(clicked()), this, SLOT(ShowSwapDialog()));

    swapDialog = new SwapXNAVDialog(this);
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
    const CAmount& coldStakingBalance,
    const CAmount& privateBalance,
    const CAmount& privPending,
    const CAmount& privLocked
) {
    int unit = walletModel->getOptionsModel()->getDisplayUnit();
    currentBalance = balance;
    currentUnconfirmedBalance = unconfirmedBalance;
    currentStakingBalance = stakingBalance;
    currentColdStakingBalance = coldStakingBalance;
    currentPrivateBalance = privateBalance;
    currentPrivateBalancePending = privPending;
    currentPrivateBalanceLocked = privLocked;
    currentImmatureBalance = immatureBalance;
    currentTotalBalance = balance + unconfirmedBalance + immatureBalance;
    currentWatchOnlyBalance = watchOnlyBalance;
    currentWatchUnconfBalance = watchUnconfBalance;
    currentWatchImmatureBalance = watchImmatureBalance;
    currentWatchOnlyTotalBalance = watchOnlyBalance + watchUnconfBalance + watchImmatureBalance;
    ui->labelBalance->setText(NavcoinUnits::formatWithUnit(unit, balance, false, NavcoinUnits::separatorAlways));
    ui->labelPrivateBalance->setText(NavcoinUnits::formatWithUnit(unit, privateBalance, false, NavcoinUnits::separatorAlways, true));
    ui->labelPrivateBalancePending->setText(NavcoinUnits::formatWithUnit(unit, privPending, false, NavcoinUnits::separatorAlways, true));
    ui->labelUnconfirmed->setText(NavcoinUnits::formatWithUnit(unit, unconfirmedBalance, false, NavcoinUnits::separatorAlways));
    ui->labelColdStaking->setText(NavcoinUnits::formatWithUnit(unit, currentColdStakingBalance, false, NavcoinUnits::separatorAlways));
    ui->labelImmature->setText(NavcoinUnits::formatWithUnit(unit, currentImmatureBalance, false, NavcoinUnits::separatorAlways));
    ui->labelWatchedBalance->setText(NavcoinUnits::formatWithUnit(unit, currentWatchOnlyTotalBalance, false, NavcoinUnits::separatorAlways));
    ui->labelTotal->setText(NavcoinUnits::formatWithUnit(unit, currentTotalBalance + currentPrivateBalance + currentPrivateBalancePending + currentWatchOnlyTotalBalance, false, NavcoinUnits::separatorAlways));

    swapDialog->SetPublicBalance(balance);
    swapDialog->SetPrivateBalance(privateBalance);

    uiInterface.SetBalance(currentBalance, currentUnconfirmedBalance, currentImmatureBalance, currentPrivateBalance, currentPrivateBalancePending, currentPrivateBalanceLocked);
}

// show/hide watch-only labels
void OverviewPage::updateWatchOnlyLabels(bool showWatchOnly)
{

}

void OverviewPage::setClientModel(ClientModel *model)
{
    this->clientModel = model;
    this->swapDialog->setClientModel(model);
}

void OverviewPage::setWalletModel(WalletModel *model)
{
    this->walletModel = model;
    this->swapDialog->setModel(model);
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
        setBalance(
            model->getBalance(),
            model->getUnconfirmedBalance(),
            model->getStake(),
            model->getImmatureBalance(),
            model->getWatchBalance(),
            model->getWatchUnconfirmedBalance(),
            model->getWatchImmatureBalance(),
            model->getColdStakingBalance(),
            model->getPrivateBalance(),
            model->getPrivateBalancePending(),
            0
        );

        connect(model, SIGNAL(balanceChanged(
            CAmount,
            CAmount,
            CAmount,
            CAmount,
            CAmount,
            CAmount,
            CAmount,
            CAmount,
            CAmount,
            CAmount,
            CAmount
        )), this, SLOT(setBalance(
            CAmount,
            CAmount,
            CAmount,
            CAmount,
            CAmount,
            CAmount,
            CAmount,
            CAmount,
            CAmount,
            CAmount,
            CAmount
        )));

        connect(model, SIGNAL(stakesChanged(
            CAmount,
            CAmount,
            CAmount,
            CAmount,
            CAmount,
            CAmount
        )), this, SLOT(setStakes(
            CAmount,
            CAmount,
            CAmount,
            CAmount,
            CAmount,
            CAmount
        )));

        connect(model->getOptionsModel(), SIGNAL(displayUnitChanged(int)), this, SLOT(updateDisplayUnit()));

        updateWatchOnlyLabels(model->haveWatchOnly());
        connect(model, SIGNAL(notifyWatchonlyChanged(bool)), this, SLOT(updateWatchOnlyLabels(bool)));

        model->StartBalanceTimer();
    }

    // update the display unit, to not use the default ("NAV")
    updateDisplayUnit();
}

void OverviewPage::updateDisplayUnit()
{
    if (!walletModel || !walletModel->getOptionsModel())
        return;

    if (currentBalance != -1)
        setBalance(
            currentBalance,
            currentUnconfirmedBalance,
            currentStakingBalance,
            currentImmatureBalance,
            currentWatchOnlyBalance,
            currentWatchUnconfBalance,
            currentWatchImmatureBalance,
            currentColdStakingBalance,
            currentPrivateBalance,
            currentPrivateBalancePending,
            currentPrivateBalanceLocked
        );

    if (currentAmount24h != -1)
        setStakes(
            currentAmount24h,
            currentAmount7d,
            currentAmount30d,
            currentAmount1y,
            currentAmountAll,
            currentAmountExp
        );

    // Update txdelegate->unit with the current unit
    txdelegate->unit = walletModel->getOptionsModel()->getDisplayUnit();

    ui->listTransactions->update();
}

void OverviewPage::ShowSwapDialog()
{
    if (!IsBLSCTEnabled(chainActive.Tip(),Params().GetConsensus()))
    {
        QMessageBox::warning(this, tr("Not available"),
                "xNAV is not active yet!");
        return;
    }

    this->swapDialog->exec();
}

void OverviewPage::setStakes(
    const CAmount& amount24h,
    const CAmount& amount7d,
    const CAmount& amount30d,
    const CAmount& amount1y,
    const CAmount& amountAll,
    const CAmount& amountExp
) {
    // Check if we have a wallet model, it's required to move forward
    if (!walletModel || !walletModel->getOptionsModel())
        return;

    // Get the unit for currency
    int unit = walletModel->getOptionsModel()->getDisplayUnit();

    // Save the current values
    currentAmount24h = amount24h;
    currentAmount7d  = amount7d;
    currentAmount30d = amount30d;
    currentAmount1y  = amount1y;
    currentAmountAll = amountAll;
    currentAmountExp = amountExp;

    ui->label24hStakingStats->setText(NavcoinUnits::formatWithUnit(unit, amount24h, false, NavcoinUnits::separatorAlways));
    ui->label7dStakingStats->setText(NavcoinUnits::formatWithUnit(unit, amount7d, false, NavcoinUnits::separatorAlways));
    ui->label30dStakingStats->setText(NavcoinUnits::formatWithUnit(unit, amount30d, false, NavcoinUnits::separatorAlways));
    ui->label1yStakingStats->setText(NavcoinUnits::formatWithUnit(unit, amount1y, false, NavcoinUnits::separatorAlways));
    ui->labelallStakingStats->setText(NavcoinUnits::formatWithUnit(unit, amountAll, false, NavcoinUnits::separatorAlways));
    ui->labelExpectedStakingStats->setText(NavcoinUnits::formatWithUnit(unit, amountExp, false, NavcoinUnits::separatorAlways));

    uiInterface.SetStaked(amountAll, amount24h, amount7d);
}

void OverviewPage::on_showStakingSetup_clicked()
{
    SplitRewardsDialog dlg(this);
    dlg.setModel(walletModel);
    dlg.exec();
}
