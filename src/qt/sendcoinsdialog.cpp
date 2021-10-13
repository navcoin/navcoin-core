// Copyright (c) 2011-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/sendcoinsdialog.h>
#include <ui_sendcoinsdialog.h>

#include <qt/addresstablemodel.h>
#include <qt/navcoinunits.h>
#include <qt/clientmodel.h>
#include <qt/coincontroldialog.h>
#include <qt/guiutil.h>
#include <qt/optionsmodel.h>
#include <qt/platformstyle.h>
#include <qt/sendcoinsentry.h>
#include <qt/walletmodel.h>
#include <net.h>
#include <util.h>
#include <utilstrencodings.h>

#include <base58.h>
#include <coincontrol.h>
#include <main.h> // mempool and minRelayTxFee
#include <ui_interface.h>
#include <txmempool.h>
#include <wallet/wallet.h>

#include <stdexcept>

#include <QCheckBox>
#include <QMessageBox>
#include <QScrollBar>
#include <QSettings>
#include <QTextDocument>
#include <QTimer>

#define SEND_CONFIRM_DELAY   3

SendCoinsDialog::SendCoinsDialog(const PlatformStyle *platformStyle, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SendCoinsDialog),
    clientModel(0),
    model(0),
    fPrivate(false),
    fNewRecipientAllowed(true),
    platformStyle(platformStyle),
    fCoinControl(false)
{
    ui->setupUi(this);

    ui->sendButton->setIcon(QIcon());

    addEntry();

    // Coin Control: clipboard actions
    QAction *clipboardQuantityAction = new QAction(tr("Copy quantity"), this);
    QAction *clipboardAmountAction = new QAction(tr("Copy amount"), this);
    QAction *clipboardFeeAction = new QAction(tr("Copy fee"), this);
    QAction *clipboardAfterFeeAction = new QAction(tr("Copy after fee"), this);
    QAction *clipboardBytesAction = new QAction(tr("Copy bytes"), this);
    QAction *clipboardPriorityAction = new QAction(tr("Copy priority"), this);
    QAction *clipboardLowOutputAction = new QAction(tr("Copy dust"), this);
    QAction *clipboardChangeAction = new QAction(tr("Copy change"), this);
    connect(clipboardQuantityAction, SIGNAL(triggered()), this, SLOT(coinControlClipboardQuantity()));
    connect(clipboardAmountAction, SIGNAL(triggered()), this, SLOT(coinControlClipboardAmount()));
    connect(clipboardFeeAction, SIGNAL(triggered()), this, SLOT(coinControlClipboardFee()));
    connect(clipboardAfterFeeAction, SIGNAL(triggered()), this, SLOT(coinControlClipboardAfterFee()));
    connect(clipboardBytesAction, SIGNAL(triggered()), this, SLOT(coinControlClipboardBytes()));
    connect(clipboardPriorityAction, SIGNAL(triggered()), this, SLOT(coinControlClipboardPriority()));
    connect(clipboardLowOutputAction, SIGNAL(triggered()), this, SLOT(coinControlClipboardLowOutput()));
    connect(clipboardChangeAction, SIGNAL(triggered()), this, SLOT(coinControlClipboardChange()));
    connect(ui->resetButton, SIGNAL(clicked()), this, SLOT(reset()));

    ui->labelCoinControlQuantity->addAction(clipboardQuantityAction);
    ui->labelCoinControlAmount->addAction(clipboardAmountAction);
    ui->labelCoinControlFee->addAction(clipboardFeeAction);
    ui->labelCoinControlAfterFee->addAction(clipboardAfterFeeAction);
    ui->labelCoinControlBytes->addAction(clipboardBytesAction);
    ui->labelCoinControlPriority->addAction(clipboardPriorityAction);
    ui->labelCoinControlLowOutput->addAction(clipboardLowOutputAction);
    ui->labelCoinControlChange->addAction(clipboardChangeAction);

    // init settings values
    QSettings settings;
    if (!settings.contains("fFeeSectionMinimized"))
        settings.setValue("fFeeSectionMinimized", true);
    if (!settings.contains("nFeeRadio") && settings.contains("nTransactionFee") && settings.value("nTransactionFee").toLongLong() > 0) // compatibility
        settings.setValue("nFeeRadio", 1); // custom
    if (!settings.contains("nFeeRadio"))
        settings.setValue("nFeeRadio", 0); // recommended
    if (!settings.contains("nCustomFeeRadio") && settings.contains("nTransactionFee") && settings.value("nTransactionFee").toLongLong() > 0) // compatibility
        settings.setValue("nCustomFeeRadio", 1); // total at least
    if (!settings.contains("nCustomFeeRadio"))
        settings.setValue("nCustomFeeRadio", 0); // per kilobyte
    if (!settings.contains("nSmartFeeSliderPosition"))
        settings.setValue("nSmartFeeSliderPosition", 0);
    if (!settings.contains("nTransactionFee"))
        settings.setValue("nTransactionFee", (qint64)DEFAULT_TRANSACTION_FEE);
    if (!settings.contains("fPayOnlyMinFee"))
        settings.setValue("fPayOnlyMinFee", false);
    if (!settings.contains("sCustomChangeAddress"))
      settings.setValue("sCustomChangeAddress", "");
    if (!settings.contains("fUseCustomChangeAddress"))
      settings.setValue("fUseCustomChangeAddress", false);
}

void SendCoinsDialog::setClientModel(ClientModel *clientModel)
{
    this->clientModel = clientModel;
}

void SendCoinsDialog::setModel(WalletModel *model)
{
    this->model = model;

    if(model && model->getOptionsModel())
    {
        for(int i = 0; i < ui->entries->count(); ++i)
        {
            SendCoinsEntry *entry = qobject_cast<SendCoinsEntry*>(ui->entries->itemAt(i)->widget());
            if(entry)
            {
                entry->setModel(model);
            }
        }

        setBalance(model->getBalance(), model->getUnconfirmedBalance(), model->getStake(), model->getImmatureBalance(),
                   model->getWatchBalance(), model->getWatchUnconfirmedBalance(), model->getWatchImmatureBalance(),
                   model->getColdStakingBalance(), model->getPrivateBalance(), model->getPrivateBalancePending(),
                   model->getPrivateBalanceLocked());
        connect(model, SIGNAL(balanceChanged(CAmount,CAmount,CAmount,CAmount,CAmount,CAmount,CAmount,CAmount, CAmount,CAmount,CAmount)), this, SLOT(setBalance(CAmount,CAmount,CAmount,CAmount,CAmount,CAmount,CAmount,CAmount, CAmount,CAmount,CAmount)));
        connect(model->getOptionsModel(), SIGNAL(displayUnitChanged(int)), this, SLOT(updateDisplayUnit()));
        updateDisplayUnit();

        // Coin Control
        connect(model->getOptionsModel(), SIGNAL(displayUnitChanged(int)), this, SLOT(coinControlUpdateLabels()));
        ui->widgetCoinControl->setVisible(fCoinControl);
        coinControlUpdateLabels();

        // Toggle the checkbox for change address
        // Which in turn uses model and model->getOptionsModel()
        // via coinControlChangeEdited
    }
}

SendCoinsDialog::~SendCoinsDialog()
{
    QSettings settings;

    delete ui;
}

void SendCoinsDialog::on_sendButton_clicked()
{
    if(!model || !model->getOptionsModel())
        return;

    QList<SendCoinsRecipient> recipients;

    bool fPrivate = false;

    SendCoinsEntry *entry = qobject_cast<SendCoinsEntry*>(ui->entries->itemAt(0)->widget());

    // Check if we have an entry and make sure it's valid
    if(!entry || !entry->validate())
        return;

    SendCoinsRecipient recipient = entry->getValue();
    recipients.append(recipient);
    fPrivate = recipient.isanon;

    CandidateTransaction selectedCoins;

    if (fPrivate) {
        QSettings settings;

        int defaultPrivacy = settings.value("defaultPrivacy", 0).toInt();

        if (!IsBLSCTEnabled(chainActive.Tip(),Params().GetConsensus()))
        {
            QMessageBox::warning(this, tr("Not available"),
                    "xNAV is not active yet!");
            return;
        }

        if (defaultPrivacy == 0 && GetBoolArg("-blsctmix", DEFAULT_MIX))
        {
            QMessageBox msgBox;
            msgBox.setWindowTitle(tr("Increase privacy level"));
            msgBox.setText(tr("Would you like to increase the privacy level of your transaction by mixing it with other coins in exchange of a fee?"));
            msgBox.setIcon(QMessageBox::Question);
            msgBox.addButton(QMessageBox::Yes);
            msgBox.addButton(QMessageBox::No);
            msgBox.setDefaultButton(QMessageBox::Yes);
            QCheckBox dontShowCheckBox("Remember as default choice");
            dontShowCheckBox.blockSignals(true);
            msgBox.addButton(&dontShowCheckBox, QMessageBox::ResetRole);
            int32_t userReply = msgBox.exec();
            if (userReply == QMessageBox::Yes)
            {
                defaultPrivacy = 1;
            }

            if (dontShowCheckBox.checkState() == Qt::Checked)
            {
                settings.setValue("defaultPrivacy", userReply == QMessageBox::Yes ? 1 : -1);
            }
        }

        if(defaultPrivacy == 1 && GetBoolArg("-blsctmix", DEFAULT_MIX))
        {
            AggregationSessionDialog msd(this);
            msd.setWalletModel(model);
            msd.setClientModel(clientModel);
            if (!msd.exec())
                QMessageBox::critical(this, tr("Something failed"), tr("The mixing process failed, if you continue, the transaction would be sent with limited privacy and the source could be easily identified by an observer. Transaction amount and recipient would be perfectly obfuscated."));
            else
            {
                selectedCoins = msd.GetSelectedCoins();
                if (selectedCoins.tx.vout.size() == 0)
                    QMessageBox::critical(this, tr("No coins for mixing"), tr("We could not find any candidate coin, if you continue, the transaction would be sent with limited privacy and the source could be easily identified by an observer. Transaction amount and recipient would be perfectly obfuscated."));
            }
        }
    }

    fNewRecipientAllowed = false;
    WalletModel::UnlockContext ctx(model->requestUnlock());
    if(!ctx.isValid())
    {
        // Unlock wallet was cancelled
        fNewRecipientAllowed = true;
        return;
    }

    // prepare transaction for getting txFee earlier
    WalletModelTransaction currentTransaction(recipients);

    CAmount nTotalAmount = 0;
    CCoinControl* coinControl = fPrivate?CoinControlDialog::blscctCoinControl:CoinControlDialog::coinControl;

    WalletModel::SendCoinsReturn prepareStatus;
    if (fCoinControl) // coin control enabled
        prepareStatus = model->prepareTransaction(currentTransaction, nTotalAmount, coinControl, &selectedCoins);
    else
        prepareStatus = model->prepareTransaction(currentTransaction, nTotalAmount, 0, &selectedCoins);

    // process prepareStatus and on error generate message shown to user
    processSendCoinsReturn(prepareStatus,
        NavcoinUnits::formatWithUnit(model->getOptionsModel()->getDisplayUnit(), currentTransaction.getTransactionFee(), fPrivate));

    if(prepareStatus.status != WalletModel::OK) {
        fNewRecipientAllowed = true;
        return;
    }

    if (currentTransaction.fSpendsColdStaking && (!fCoinControl ||
        (fCoinControl && !CNavcoinAddress(CoinControlDialog::coinControl->destChange).IsColdStakingAddress(Params()))))
    {
        SendConfirmationDialog confirmationDialog(tr("Confirm send coins"),
            tr("This transaction will spend coins stored in a cold staking address.<br>You did not set any cold staking address as custom change destination, so those coins won't be locked anymore by the cold staking smart contract.<br><br>Do you still want to send this transaction?"), SEND_CONFIRM_DELAY, this);
        confirmationDialog.exec();
        QMessageBox::StandardButton retval = (QMessageBox::StandardButton)confirmationDialog.result();

        if(retval != QMessageBox::Yes)
        {
            fNewRecipientAllowed = true;
            return;
        }
    }

    CAmount txFee = currentTransaction.getTransactionFee();

    QString questionString = tr("Are you sure you want to send?");

    // Format confirmation message
    QStringList formatted;
    const SendCoinsRecipient &rcp = currentTransaction.recipients.first();
    {
        // generate bold amount string
        QString amount = "<b>" + NavcoinUnits::formatHtmlWithUnit(model->getOptionsModel()->getDisplayUnit(), nTotalAmount - (rcp.fSubtractFeeFromAmount ? txFee : 0) , false, NavcoinUnits::SeparatorStyle::separatorStandard, false, rcp.isanon);
        amount.append("</b>");
        // generate monospace address string
        QString splitAddr = rcp.address;
        for (int i = 50; i <= splitAddr.size(); i+=51)
            splitAddr.insert(i, "<br>");
        QString address = "<span style='font-family: monospace'>" + splitAddr;
        address.append("</span>");

        QString recipientElement;

        if(rcp.label.length() > 0) // label with address
        {
            recipientElement = tr("%1 to %2").arg((rcp.isanon ? tr(" Private payment ")  : "" ) +amount, GUIUtil::HtmlEscape(rcp.label));
            recipientElement.append(QString(" (%1)").arg(address));
        }
        else // just address
        {
            recipientElement = tr("%1 to %2").arg((rcp.isanon ?  tr(" Private payment ")  : "" ) +amount, address);
        }

        if (selectedCoins.tx.vin.size() > 0)
        {
            recipientElement.append(QString("<br /><br />%1").arg(tr("mixed with %1 coins")).arg(selectedCoins.tx.vin.size()));
        }

        formatted.append(recipientElement);

        questionString.append("<br /><br />%1");

        if(txFee)
        {
            // append fee string if a fee is required
            questionString.append("<hr /><span style='color:#aa0000;'>");
            questionString.append(NavcoinUnits::formatHtmlWithUnit(model->getOptionsModel()->getDisplayUnit(), txFee, false, NavcoinUnits::SeparatorStyle::separatorStandard, false, rcp.isanon));
            questionString.append("</span> ");
            questionString.append(tr("added as transaction fee"));

            // append transaction size
            questionString.append(" (" + QString::number((double)currentTransaction.getTransactionSize() / 1000) + " kB)");
        }
    }

    // add total amount in all subdivision units
    questionString.append("<hr />");
    CAmount totalAmount = nTotalAmount + (rcp.fSubtractFeeFromAmount ? 0 : txFee);
    QStringList alternativeUnits;

    // Check if we have selected a display unit that is not NAV
    if (model->getOptionsModel()->getDisplayUnit() != NavcoinUnits::NAV)
        alternativeUnits.append(NavcoinUnits::formatHtmlWithUnit(NavcoinUnits::NAV, totalAmount));

    // Check if we have selected a display unit that is not BTC
    if (model->getOptionsModel()->getDisplayUnit() != NavcoinUnits::BTC)
        alternativeUnits.append(NavcoinUnits::formatHtmlWithUnit(NavcoinUnits::BTC, totalAmount));

    questionString.append(tr("Total Amount %1")
        .arg(NavcoinUnits::formatHtmlWithUnit(model->getOptionsModel()->getDisplayUnit(), totalAmount)));
    questionString.append(QString("<span style='font-size:10pt;font-weight:normal;'><br />(=%2)</span>")
        .arg(alternativeUnits.join(" " + tr("or") + " ")));

    SendConfirmationDialog confirmationDialog(tr("Confirm send coins"),
        questionString.arg(formatted.join("<br />")), SEND_CONFIRM_DELAY, this);
    confirmationDialog.exec();
    QMessageBox::StandardButton retval = (QMessageBox::StandardButton)confirmationDialog.result();

    if(retval != QMessageBox::Yes)
    {
        fNewRecipientAllowed = true;
        return;
    }

    WalletModel::SendCoinsReturn sendStatus;

    // now send the prepared transaction
    if (fCoinControl) // coin control enabled
        sendStatus = model->sendCoins(currentTransaction, fPrivate, coinControl, &selectedCoins);
    else
        sendStatus = model->sendCoins(currentTransaction, fPrivate, 0, &selectedCoins);

    // process sendStatus and on error generate message shown to user
    processSendCoinsReturn(sendStatus);

    if (sendStatus.status == WalletModel::OK)
    {
        accept();
        coinControl->UnSelectAll();
        coinControlUpdateLabels();
    }
    fNewRecipientAllowed = true;
}

void SendCoinsDialog::reset()
{
    CCoinControl* coinControl = fPrivate?CoinControlDialog::blscctCoinControl:CoinControlDialog::coinControl;
    coinControl->UnSelectAll();
    coinControlUpdateLabels();
}

void SendCoinsDialog::clear()
{
    // Remove entries until only one left
    while(ui->entries->count())
    {
        ui->entries->takeAt(0)->widget()->deleteLater();
    }
    addEntry();

    updateTabsAndLabels();
}

void SendCoinsDialog::reject()
{
    clear();
}

void SendCoinsDialog::accept()
{
    clear();
}

SendCoinsEntry *SendCoinsDialog::addEntry()
{
    SendCoinsEntry *entry = new SendCoinsEntry(platformStyle, this);
    entry->setModel(model);
    ui->entries->addWidget(entry);
    connect(entry, SIGNAL(removeEntry(SendCoinsEntry*)), this, SLOT(removeEntry(SendCoinsEntry*)));
    connect(entry, SIGNAL(payAmountChanged()), this, SLOT(coinControlUpdateLabels()));
    connect(entry, SIGNAL(subtractFeeFromAmountChanged()), this, SLOT(coinControlUpdateLabels()));
    connect(entry, SIGNAL(privateOrPublicChanged(bool)), this, SLOT(updatePrivateOrPublic(bool)));
    connect(entry, SIGNAL(openCoinControl()), this, SLOT(coinControlButtonClicked()));
    connect(entry, SIGNAL(customChangeChanged(QString)), this, SLOT(coinControlChangeEdited(QString)));
    connect(entry, SIGNAL(coinControlChangeChecked(int)), this, SLOT(coinControlChangeChecked(int)));

    updatePrivateOrPublic(entry->fPrivate);

    // Focus the field, so that entry can start immediately
    entry->clear();
    entry->setFocus();
    qApp->processEvents();

    updateTabsAndLabels();
    return entry;
}

void SendCoinsDialog::updateTabsAndLabels()
{
//    setupTabChain(0);
    coinControlUpdateLabels();
}

void SendCoinsDialog::removeEntry(SendCoinsEntry* entry)
{
    entry->hide();

    // If the last entry is about to be removed add an empty one
    if (ui->entries->count() == 1)
        addEntry();

    entry->deleteLater();

    updateTabsAndLabels();
}

//QWidget *SendCoinsDialog::setupTabChain(QWidget *prev)
//{
//    for(int i = 0; i < ui->entries->count(); ++i)
//    {
//        SendCoinsEntry *entry = qobject_cast<SendCoinsEntry*>(ui->entries->itemAt(i)->widget());
//        if(entry)
//        {
//            prev = entry->setupTabChain(prev);
//        }
//    }
//    QWidget::setTabOrder(prev, ui->sendButton);

//    return ui->sendButton;
//}

void SendCoinsDialog::setAddress(const QString &address)
{
    SendCoinsEntry *entry = 0;
    // Replace the first entry if it is still unused
    if(ui->entries->count() == 1)
    {
        SendCoinsEntry *first = qobject_cast<SendCoinsEntry*>(ui->entries->itemAt(0)->widget());
        if(first->isClear())
        {
            entry = first;
        }
    }
    if(!entry)
    {
        entry = addEntry();
    }

    entry->setAddress(address);
}

void SendCoinsDialog::pasteEntry(const SendCoinsRecipient &rv)
{
    SendCoinsEntry *entry = 0;

    if(!fNewRecipientAllowed)
        return;

    while(ui->entries->count())
    {
        ui->entries->takeAt(0)->widget()->deleteLater();
    }

    entry = addEntry();

    entry->setValue(rv);
    updateTabsAndLabels();
}

bool SendCoinsDialog::handlePaymentRequest(const SendCoinsRecipient &rv)
{
    // Just paste the entry, all pre-checks
    // are done in paymentserver.cpp.
    pasteEntry(rv);
    return true;
}

void SendCoinsDialog::setBalance(const CAmount& balance, const CAmount& unconfirmedBalance, const CAmount& stakingBalance,const CAmount& immatureBalance,
                                 const CAmount& watchBalance, const CAmount& watchUnconfirmedBalance, const CAmount& watchImmatureBalance,
                                 const CAmount& coldStakingBalance, const CAmount& privateBalance, const CAmount& privPending, const CAmount& privLocked)
{
    Q_UNUSED(unconfirmedBalance);
    Q_UNUSED(stakingBalance);
    Q_UNUSED(coldStakingBalance);
    Q_UNUSED(privateBalance);
    Q_UNUSED(privPending);
    Q_UNUSED(privLocked);
    Q_UNUSED(immatureBalance);
    Q_UNUSED(watchBalance);
    Q_UNUSED(watchUnconfirmedBalance);
    Q_UNUSED(watchImmatureBalance);

    pubBalance = balance;
    privBalance = privateBalance;

    CoinControlDialog::fPrivate = fPrivate;
    CCoinControl* coinControl = fPrivate?CoinControlDialog::blscctCoinControl:CoinControlDialog::coinControl;

    for(int i = 0; i < ui->entries->count(); ++i)
    {
        SendCoinsEntry *entry = qobject_cast<SendCoinsEntry*>(ui->entries->itemAt(i)->widget());
        if(entry)
        {
            if (fPrivate)
            {
                entry->setTotalAmount(balance);
                entry->setTotalPrivateAmount(coinControl->HasSelected() ? ccAmount : privateBalance);
            }
            else
            {
                entry->setTotalAmount(coinControl->HasSelected() ? ccAmount  : balance);
                entry->setTotalPrivateAmount(privateBalance);
            }
        }
    }
}

void SendCoinsDialog::updateDisplayUnit()
{
    setBalance(model->getBalance(), 0, 0, 0, 0, 0, 0, 0, model->getPrivateBalance(), model->getPrivateBalancePending(), model->getPrivateBalanceLocked());
}

void SendCoinsDialog::updatePrivateOrPublic(bool fPrivate)
{
    this->fPrivate = fPrivate;
    CoinControlDialog::fPrivate = fPrivate;
    CCoinControl* coinControl = CoinControlDialog::fPrivate ? CoinControlDialog::blscctCoinControl : CoinControlDialog::coinControl;
    coinControl->UnSelectAll();
    coinControlUpdateLabels();
}

void SendCoinsDialog::processSendCoinsReturn(const WalletModel::SendCoinsReturn &sendCoinsReturn, const QString &msgArg)
{
    QPair<QString, CClientUIInterface::MessageBoxFlags> msgParams;
    // Default to a warning message, override if error message is needed
    msgParams.second = CClientUIInterface::MSG_WARNING;

    // This comment is specific to SendCoinsDialog usage of WalletModel::SendCoinsReturn.
    // WalletModel::TransactionCommitFailed is used only in WalletModel::sendCoins()
    // all others are used only in WalletModel::prepareTransaction()
    switch(sendCoinsReturn.status)
    {
    case WalletModel::InvalidAddress:
        msgParams.first = tr("The recipient address is not valid. Please recheck.");
        break;
    case WalletModel::InvalidAmount:
        msgParams.first = tr("The amount to pay must be larger than 0.");
        break;
    case WalletModel::AmountExceedsBalance:
        msgParams.first = tr("The amount exceeds your balance.");
        break;
    case WalletModel::AmountWithFeeExceedsBalance:
        msgParams.first = tr("The total exceeds your balance when the %1 transaction fee is included.").arg(msgArg);
        break;
    case WalletModel::DuplicateAddress:
        msgParams.first = tr("Duplicate address found: addresses should only be used once each.");
        break;
    case WalletModel::TransactionCreationFailed:
        msgParams.first = tr("Transaction creation failed!");
        msgParams.second = CClientUIInterface::MSG_ERROR;
        break;
    case WalletModel::TransactionCommitFailed:
        msgParams.first = tr("The transaction was rejected! This might happen if some of the coins in your wallet were already spent, such as if you used a copy of %1 and coins were spent in the copy but not marked as spent here.").arg(QString::fromStdString(GetArg("-wallet", DEFAULT_WALLET_DAT)));
        msgParams.second = CClientUIInterface::MSG_ERROR;
        break;
    case WalletModel::AbsurdFee:
        msgParams.first = tr("A fee higher than %1 is considered an absurdly high fee.").arg(NavcoinUnits::formatWithUnit(model->getOptionsModel()->getDisplayUnit(), maxTxFee));
        break;
    case WalletModel::PaymentRequestExpired:
        msgParams.first = tr("Payment request expired.");
        msgParams.second = CClientUIInterface::MSG_ERROR;
        break;
    // included to prevent a compiler warning.
    case WalletModel::OK:
    default:
        return;
    }

    Q_EMIT message(tr("Send Coins"), msgParams.first, msgParams.second);
}

// Coin Control: copy label "Quantity" to clipboard
void SendCoinsDialog::coinControlClipboardQuantity()
{
    GUIUtil::setClipboard(ui->labelCoinControlQuantity->text());
}

// Coin Control: copy label "Amount" to clipboard
void SendCoinsDialog::coinControlClipboardAmount()
{
    GUIUtil::setClipboard(ui->labelCoinControlAmount->text().left(ui->labelCoinControlAmount->text().indexOf(" ")));
}

// Coin Control: copy label "Fee" to clipboard
void SendCoinsDialog::coinControlClipboardFee()
{
    GUIUtil::setClipboard(ui->labelCoinControlFee->text().left(ui->labelCoinControlFee->text().indexOf(" ")).replace(ASYMP_UTF8, ""));
}

// Coin Control: copy label "After fee" to clipboard
void SendCoinsDialog::coinControlClipboardAfterFee()
{
    GUIUtil::setClipboard(ui->labelCoinControlAfterFee->text().left(ui->labelCoinControlAfterFee->text().indexOf(" ")).replace(ASYMP_UTF8, ""));
}

// Coin Control: copy label "Bytes" to clipboard
void SendCoinsDialog::coinControlClipboardBytes()
{
    GUIUtil::setClipboard(ui->labelCoinControlBytes->text().replace(ASYMP_UTF8, ""));
}

// Coin Control: copy label "Priority" to clipboard
void SendCoinsDialog::coinControlClipboardPriority()
{
    GUIUtil::setClipboard(ui->labelCoinControlPriority->text());
}

// Coin Control: copy label "Dust" to clipboard
void SendCoinsDialog::coinControlClipboardLowOutput()
{
    GUIUtil::setClipboard(ui->labelCoinControlLowOutput->text());
}

// Coin Control: copy label "Change" to clipboard
void SendCoinsDialog::coinControlClipboardChange()
{
    GUIUtil::setClipboard(ui->labelCoinControlChange->text().left(ui->labelCoinControlChange->text().indexOf(" ")).replace(ASYMP_UTF8, ""));
}

// Coin Control: button inputs -> show actual coin control dialog
void SendCoinsDialog::coinControlButtonClicked()
{
    CoinControlDialog dlg(platformStyle);
    dlg.setModel(model);
    dlg.exec();
    coinControlUpdateLabels();
}

// Coin Control: checkbox custom change address
void SendCoinsDialog::coinControlChangeChecked(int state)
{
    QSettings settings;

    if (state == Qt::Unchecked)
    {
        CoinControlDialog::coinControl->destChange = CNoDestination();

        // Clear the setting
        settings.setValue("fUseCustomChangeAddress", false);
    }
    else
    {
        // use this to re-validate an already entered address
        coinControlChangeEdited(sChangeAddress);

        // Save the setting
        settings.setValue("fUseCustomChangeAddress", true);
    }

}

// Coin Control: custom change address changed
void SendCoinsDialog::coinControlChangeEdited(const QString& text)
{
    // Check for address table and model
    if (!model || !model->getAddressTableModel())
        return;

    // We need access to settings
    QSettings settings;

    // Clear the setting
    settings.setValue("sCustomChangeAddress", "");

    // Default to no change address until verified
    CoinControlDialog::coinControl->destChange = CNoDestination();
    ui->labelCoinControlChangeLabel->setStyleSheet("QLabel{color:red;}");

    CNavcoinAddress addr = CNavcoinAddress(text.toStdString());

    if (text.isEmpty()) // Nothing entered
    {
        ui->labelCoinControlChangeLabel->setText("");

        // Give up!
        return;
    }

    if (!addr.IsValid()) // Invalid address
    {
        ui->labelCoinControlChangeLabel->setText(tr("Warning: Invalid Navcoin change address"));

        // Give up!
        return;
    }

    // Save the setting
    settings.setValue("sCustomChangeAddress", text);

    bool fHaveKey = false;
    if(addr.IsColdStakingAddress(Params()))
    {
        CKeyID stakingId, spendingId;
        addr.GetStakingKeyID(stakingId);
        addr.GetSpendingKeyID(spendingId);
        if(model->havePrivKey(stakingId) || model->havePrivKey(spendingId))
            fHaveKey = true;
    }
    else if(addr.IsColdStakingv2Address(Params()))
    {
        CKeyID stakingId, spendingId, votingId;
        addr.GetStakingKeyID(stakingId);
        addr.GetSpendingKeyID(spendingId);
        addr.GetVotingKeyID(votingId);
        if(model->havePrivKey(stakingId) || model->havePrivKey(spendingId) || model->havePrivKey(votingId))
            fHaveKey = true;
    }
    else
    {
        CKeyID keyid;
        addr.GetKeyID(keyid);
        if(model->havePrivKey(keyid))
            fHaveKey = true;
    }

    if (!fHaveKey) // Unknown change address
    {
        ui->labelCoinControlChangeLabel->setText(tr("Warning: Unknown change address"));
    }
    else // Known change address
    {
        ui->labelCoinControlChangeLabel->setStyleSheet("QLabel{color:black;}");

        // Query label
        QString associatedLabel = model->getAddressTableModel()->labelForAddress(text);
        if (!associatedLabel.isEmpty())
            ui->labelCoinControlChangeLabel->setText(associatedLabel);
        else
            ui->labelCoinControlChangeLabel->setText(tr("(no label)"));

        CoinControlDialog::coinControl->destChange = addr.Get();

        sChangeAddress = text;
    }
}

// Coin Control: update labels
void SendCoinsDialog::coinControlUpdateLabels()
{
    if (!model || !model->getOptionsModel())
        return;

    // set pay amounts
    CoinControlDialog::payAmounts.clear();
    CoinControlDialog::fSubtractFeeFromAmount = false;
    for(int i = 0; i < ui->entries->count(); ++i)
    {
        SendCoinsEntry *entry = qobject_cast<SendCoinsEntry*>(ui->entries->itemAt(i)->widget());
        if(entry && !entry->isHidden())
        {
            SendCoinsRecipient rcp = entry->getValue();
            CoinControlDialog::payAmounts.append(rcp.amount);
            if (rcp.fSubtractFeeFromAmount)
                CoinControlDialog::fSubtractFeeFromAmount = true;
        }
    }

    CoinControlDialog::fPrivate = fPrivate;
    CCoinControl* coinControl = fPrivate?CoinControlDialog::blscctCoinControl:CoinControlDialog::coinControl;

    if (coinControl->HasSelected())
    {
        // actual coin control calculation
        CoinControlDialog::updateLabels(model, this, this->ccAmount);
        if (fPrivate)
            setBalance(pubBalance, 0, 0, 0, 0, 0, 0, 0, ccAmount, 0, 0);
        else
            setBalance(ccAmount, 0, 0, 0, 0, 0, 0, 0, privBalance, 0, 0);
        // show coin control stats
        ui->widgetCoinControl->show();
        fCoinControl = true;
    }
    else
    {
        // hide coin control stats
        ui->widgetCoinControl->hide();
        ui->labelCoinControlInsuffFunds->hide();
        setBalance(pubBalance, 0, 0, 0, 0, 0, 0, 0, privBalance, 0, 0);
        fCoinControl = false;
    }
}

SendConfirmationDialog::SendConfirmationDialog(const QString &title, const QString &text, int secDelay,
    QWidget *parent) :
    QMessageBox(QMessageBox::Question, title, text, QMessageBox::Yes | QMessageBox::Cancel, parent), secDelay(secDelay)
{
    setDefaultButton(QMessageBox::Cancel);
    yesButton = button(QMessageBox::Yes);
    updateYesButton();
    connect(&countDownTimer, SIGNAL(timeout()), this, SLOT(countDown()));
}

int SendConfirmationDialog::exec()
{
    updateYesButton();
    countDownTimer.start(1000);
    return QMessageBox::exec();
}

void SendConfirmationDialog::countDown()
{
    secDelay--;
    updateYesButton();

    if(secDelay <= 0)
    {
        countDownTimer.stop();
    }
}

void SendConfirmationDialog::updateYesButton()
{
    if(secDelay > 0)
    {
        yesButton->setEnabled(false);
        yesButton->setText(tr("Yes") + " (" + QString::number(secDelay) + ")");
    }
    else
    {
        yesButton->setEnabled(true);
        yesButton->setText(tr("Yes"));
    }
}
