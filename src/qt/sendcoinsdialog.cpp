// Copyright (c) 2011-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "sendcoinsdialog.h"
#include "ui_sendcoinsdialog.h"

#include "addresstablemodel.h"
#include "navcoinunits.h"
#include "clientmodel.h"
#include "coincontroldialog.h"
#include "guiutil.h"
#include "optionsmodel.h"
#include "platformstyle.h"
#include "sendcoinsentry.h"
#include "walletmodel.h"
#include "net.h"
#include "skinize.h"
#include "util.h"
#include "utilstrencodings.h"
#include "navtechinit.h"
#include "navtechsetup.h"

#include "base58.h"
#include "coincontrol.h"
#include "main.h" // mempool and minRelayTxFee
#include "ui_interface.h"
#include "txmempool.h"
#include "wallet/wallet.h"

#include <stdexcept>

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
    fNewRecipientAllowed(true),
    fFeeMinimized(true),
    platformStyle(platformStyle)
{
    ui->setupUi(this);

    ui->sendButton->setIcon(QIcon());

    GUIUtil::setupAddressWidget(ui->lineEditCoinControlChange, this);

    addEntry();

    // Coin Control
    connect(ui->pushButtonCoinControl, SIGNAL(clicked()), this, SLOT(coinControlButtonClicked()));
    connect(ui->noNavtechButton, SIGNAL(clicked()), this, SLOT(showNavTechDialog()));
    connect(ui->checkBoxCoinControlChange, SIGNAL(stateChanged(int)), this, SLOT(coinControlChangeChecked(int)));
    connect(ui->lineEditCoinControlChange, SIGNAL(textEdited(const QString &)), this, SLOT(coinControlChangeEdited(const QString &)));

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
    connect(ui->anonsendCheckbox, SIGNAL(clicked()), this, SLOT(anonsendCheckboxClick()));
    connect(ui->fullAmountBtn,  SIGNAL(clicked()), this, SLOT(useFullAmount()));

    ui->labelCoinControlQuantity->addAction(clipboardQuantityAction);
    ui->labelCoinControlAmount->addAction(clipboardAmountAction);
    ui->labelCoinControlFee->addAction(clipboardFeeAction);
    ui->labelCoinControlAfterFee->addAction(clipboardAfterFeeAction);
    ui->labelCoinControlBytes->addAction(clipboardBytesAction);
    ui->labelCoinControlPriority->addAction(clipboardPriorityAction);
    ui->labelCoinControlLowOutput->addAction(clipboardLowOutputAction);
    ui->labelCoinControlChange->addAction(clipboardChangeAction);

    // init transaction fee section
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
    if (!settings.contains("fAnonSend"))
      settings.setValue("fAnonSend", false);
    ui->groupFee->setId(ui->radioSmartFee, 0);
    ui->groupFee->setId(ui->radioCustomFee, 1);
    ui->groupFee->button((int)std::max(0, std::min(1, settings.value("nFeeRadio").toInt())))->setChecked(true);
    ui->groupCustomFee->setId(ui->radioCustomPerKilobyte, 0);
    ui->groupCustomFee->setId(ui->radioCustomAtLeast, 1);
    ui->groupCustomFee->button((int)std::max(0, std::min(1, settings.value("nCustomFeeRadio").toInt())))->setChecked(true);
    ui->sliderSmartFee->setValue(settings.value("nSmartFeeSliderPosition").toInt());
    ui->customFee->setValue(settings.value("nTransactionFee").toLongLong());
    ui->checkBoxMinimumFee->setChecked(settings.value("fPayOnlyMinFee").toBool());
    minimizeFeeSection(settings.value("fFeeSectionMinimized").toBool());
    ui->anonsendCheckbox->setChecked(settings.value("fAnonSend").toBool());

    checkNavtechServers();
}

void SendCoinsDialog::anonsendCheckboxClick()
{
    QSettings settings;

    settings.setValue("fAnonSend", ui->anonsendCheckbox->isChecked());
}

void SendCoinsDialog::checkNavtechServers()
{
    bool notEnoughServers = vAddedAnonServers.size() < 1 && mapMultiArgs["-addanonserver"].size() < 1;

    ui->noNavtechLabel->setVisible(notEnoughServers);
    ui->anonsendCheckbox->setVisible(!notEnoughServers);
    if(notEnoughServers)
      ui->anonsendCheckbox->setChecked(false);
}

void SendCoinsDialog::setClientModel(ClientModel *clientModel)
{
    this->clientModel = clientModel;

    if (clientModel) {
        connect(clientModel, SIGNAL(numBlocksChanged(int,QDateTime,double,bool)), this, SLOT(updateSmartFeeLabel()));
    }
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
                   model->getWatchBalance(), model->getWatchUnconfirmedBalance(), model->getWatchImmatureBalance());
        connect(model, SIGNAL(balanceChanged(CAmount,CAmount,CAmount,CAmount,CAmount,CAmount,CAmount)), this, SLOT(setBalance(CAmount,CAmount,CAmount,CAmount,CAmount,CAmount,CAmount)));
        connect(model->getOptionsModel(), SIGNAL(displayUnitChanged(int)), this, SLOT(updateDisplayUnit()));
        updateDisplayUnit();

        // Coin Control
        connect(model->getOptionsModel(), SIGNAL(displayUnitChanged(int)), this, SLOT(coinControlUpdateLabels()));
        connect(model->getOptionsModel(), SIGNAL(coinControlFeaturesChanged(bool)), this, SLOT(coinControlFeatureChanged(bool)));
        ui->frameCoinControl->setVisible(model->getOptionsModel()->getCoinControlFeatures());
        coinControlUpdateLabels();

        // fee section
        connect(ui->sliderSmartFee, SIGNAL(valueChanged(int)), this, SLOT(updateSmartFeeLabel()));
        connect(ui->sliderSmartFee, SIGNAL(valueChanged(int)), this, SLOT(updateGlobalFeeVariables()));
        connect(ui->sliderSmartFee, SIGNAL(valueChanged(int)), this, SLOT(coinControlUpdateLabels()));
        connect(ui->groupFee, SIGNAL(buttonClicked(int)), this, SLOT(updateFeeSectionControls()));
        connect(ui->groupFee, SIGNAL(buttonClicked(int)), this, SLOT(updateGlobalFeeVariables()));
        connect(ui->groupFee, SIGNAL(buttonClicked(int)), this, SLOT(coinControlUpdateLabels()));
        connect(ui->groupCustomFee, SIGNAL(buttonClicked(int)), this, SLOT(updateGlobalFeeVariables()));
        connect(ui->groupCustomFee, SIGNAL(buttonClicked(int)), this, SLOT(coinControlUpdateLabels()));
        connect(ui->customFee, SIGNAL(valueChanged()), this, SLOT(updateGlobalFeeVariables()));
        connect(ui->customFee, SIGNAL(valueChanged()), this, SLOT(coinControlUpdateLabels()));
        connect(ui->checkBoxMinimumFee, SIGNAL(stateChanged(int)), this, SLOT(setMinimumFee()));
        connect(ui->checkBoxMinimumFee, SIGNAL(stateChanged(int)), this, SLOT(updateFeeSectionControls()));
        connect(ui->checkBoxMinimumFee, SIGNAL(stateChanged(int)), this, SLOT(updateGlobalFeeVariables()));
        connect(ui->checkBoxMinimumFee, SIGNAL(stateChanged(int)), this, SLOT(coinControlUpdateLabels()));
        ui->customFee->setSingleStep(CWallet::GetRequiredFee(1000));
        updateFeeSectionControls();
        updateMinFeeLabel();
        updateSmartFeeLabel();
        updateGlobalFeeVariables();
    }
}

SendCoinsDialog::~SendCoinsDialog()
{
    QSettings settings;
    settings.setValue("fFeeSectionMinimized", fFeeMinimized);
    settings.setValue("nFeeRadio", ui->groupFee->checkedId());
    settings.setValue("nCustomFeeRadio", ui->groupCustomFee->checkedId());
    settings.setValue("nSmartFeeSliderPosition", ui->sliderSmartFee->value());
    settings.setValue("nTransactionFee", (qint64)ui->customFee->value());
    settings.setValue("fPayOnlyMinFee", ui->checkBoxMinimumFee->isChecked());

    delete ui;
}

void SendCoinsDialog::on_sendButton_clicked()
{
    if(!model || !model->getOptionsModel())
        return;

    QList<SendCoinsRecipient> recipients;
    bool valid = true;

    int nEntropy = GetArg("anon_entropy",NAVTECH_DEFAULT_ENTROPY);

    unsigned int nTransactions = (rand() % nEntropy) + 2;

    SendCoinsEntry *entry = qobject_cast<SendCoinsEntry*>(ui->entries->itemAt(0)->widget());
    if(entry)
    {
        if(entry->validate())
        {
            SendCoinsRecipient recipient = entry->getValue();
            CAmount nAmount = recipient.amount;
            double nId = rand() % pindexBestHeader->GetMedianTimePast();

            if(ui->anonsendCheckbox->checkState() != 0) {
                try
                {
                    Navtech navtech;

                    UniValue navtechData = navtech.CreateAnonTransaction(recipient.address.toStdString() , recipient.amount / (nTransactions * 2), nTransactions);

                    UniValue pubKey = find_value(navtechData, "public_key");

                    std::vector<UniValue> serverNavAddresses(find_value(navtechData, "anonaddress").getValues());

                    if(serverNavAddresses.size() != nTransactions)
                    {
                        QMessageBox::warning(this, tr("Private payment"),
                                         "<qt>" +
                                         tr("NAVTech server returned a different number of addresses.")+"</qt>");
                        valid = false;
                    }

                    for(unsigned int i = 0; i < serverNavAddresses.size(); i++)
                    {
                        CNavCoinAddress serverNavAddress(serverNavAddresses[i].get_str());
                        if (!serverNavAddress.IsValid())
                        {

                            valid = false;
                            break;
                        }
                    }

                    if(valid)
                    {

                      CAmount nAmountAlreadyProcessed = 0;
                      CAmount nMinAmount = find_value(navtechData, "min_amount").get_int() * COIN;

                      for(unsigned int i = 0; i < serverNavAddresses.size(); i++)
                      {
                          SendCoinsRecipient cRecipient = recipient;
                          cRecipient.destaddress = QString::fromStdString(serverNavAddresses[i].get_str());
                          CAmount nAmountRound = 0;
                          CAmount nAmountNotProcessed = nAmount - nAmountAlreadyProcessed;
                          CAmount nAmountToSubstract = ((nAmountNotProcessed / ((rand() % nEntropy)+2))/1000)*1000;
                          if(i == serverNavAddresses.size() - 1 || (nAmountNotProcessed - nAmountToSubstract) < (nMinAmount + 0.001))
                          {
                              nAmountRound = nAmountNotProcessed;
                              i = serverNavAddresses.size();
                          }
                          else
                          {
                              nAmountRound = std::max(nAmountToSubstract,nMinAmount);
                          }


                          nAmountAlreadyProcessed += nAmountRound;
                          cRecipient.anondestination = QString::fromStdString(navtech.EncryptAddress(recipient.address.toStdString(), pubKey.get_str(), nTransactions, i+(i==serverNavAddresses.size()?0:1), nId));
                          if(!find_value(navtechData, "anonfee").isNull()){
                              cRecipient.anonfee = nAmountRound * ((float)find_value(navtechData, "anonfee").get_real() / 100.0);
                              cRecipient.transaction_fee = find_value(navtechData, "anonfee").get_real();
                          }else
                              valid = false;
                          cRecipient.isanon = true;
                          cRecipient.amount = nAmountRound;
                          recipients.append(cRecipient);

                      }

                    }

                }
                catch(const std::runtime_error &e)
                {
                    QMessageBox msgBox;
                    msgBox.setText(tr("Something went wrong:"));
                    msgBox.setInformativeText(tr(e.what()));
                    QAbstractButton *myYesButton = msgBox.addButton(tr("Do a normal transaction"), QMessageBox::YesRole);
                    msgBox.addButton(trUtf8("Abort"), QMessageBox::NoRole);
                    msgBox.setIcon(QMessageBox::Question);
                    msgBox.exec();

                    if(msgBox.clickedButton() == myYesButton)
                    {
                        QMessageBox::StandardButton retval = QMessageBox::question(this, tr("Switch to normal transaction"),
                            tr("Are you sure you want to do a normal transaction instead of a private payment?") + QString("<br><br>") + tr("Details of the payment would be publicly exposed on the blockchain."),
                            QMessageBox::Yes|QMessageBox::Cancel,
                            QMessageBox::Cancel);

                        if(retval == QMessageBox::Yes)
                        {
                            recipient.isanon = false;
                            recipients.append(recipient);
                            valid = true;
                        }
                        else
                        {
                            valid = false;
                        }
                    }
                    else
                    {
                        valid = false;
                    }
                }
            }
            else
            {

                recipient.isanon = false;
                recipients.append(recipient);

            }

        }
        else
        {
            valid = false;
        }
    }

    if(!valid || recipients.isEmpty())
    {
        return;
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

    WalletModel::SendCoinsReturn prepareStatus;
    if (model->getOptionsModel()->getCoinControlFeatures()) // coin control enabled
        prepareStatus = model->prepareTransaction(currentTransaction, CoinControlDialog::coinControl);
    else
        prepareStatus = model->prepareTransaction(currentTransaction);

    // process prepareStatus and on error generate message shown to user
    processSendCoinsReturn(prepareStatus,
        NavCoinUnits::formatWithUnit(model->getOptionsModel()->getDisplayUnit(), currentTransaction.getTransactionFee()));

    if(prepareStatus.status != WalletModel::OK) {
        fNewRecipientAllowed = true;
        return;
    }

    CAmount txFee = currentTransaction.getTransactionFee();
    CAmount anonfee;
    CAmount nTotalAmount = 0;

    QString questionString = tr("Are you sure you want to send?");

    Q_FOREACH(const SendCoinsRecipient &rcp, currentTransaction.getRecipients())
    {
      nTotalAmount += rcp.amount;
      if(rcp.fSubtractFeeFromAmount && rcp.isanon)
        nTotalAmount -= rcp.anonfee;
    }

    // Format confirmation message
    QStringList formatted;
    const SendCoinsRecipient &rcp = currentTransaction.recipients.first();
    {
        // generate bold amount string
        QString amount = "<b>" + NavCoinUnits::formatHtmlWithUnit(model->getOptionsModel()->getDisplayUnit(), nTotalAmount);
        amount.append("</b>");
        // generate monospace address string
        QString address = "<span style='font-family: monospace;'>" + rcp.address;
        address.append("</span>");

        QString recipientElement;
        int nLength = currentTransaction.recipients.length();

        if (!rcp.paymentRequest.IsInitialized()) // normal payment
        {
            if(rcp.label.length() > 0) // label with address
            {
                recipientElement = tr("%1 to %2").arg((rcp.isanon ? " Private payment " : "" ) +amount, GUIUtil::HtmlEscape(rcp.label));
                recipientElement.append(QString(" (%1)").arg(address));
            }
            else // just address
            {
                recipientElement = tr("%1 to %2").arg((rcp.isanon ?  " Private payment " : "" ) +amount, address);
            }
        }
        else if(!rcp.authenticatedMerchant.isEmpty()) // authenticated payment request
        {
            recipientElement = tr("%1 to %2").arg((rcp.isanon ? " Private payment " : "") +amount, GUIUtil::HtmlEscape(rcp.authenticatedMerchant));
        }
        else // unauthenticated payment request
        {
            recipientElement = tr("%1 to %2").arg((rcp.isanon ? " Private payment " : "") +amount, address);
        }

        formatted.append(recipientElement);

        questionString.append("<br /><br />%1");

        anonfee = rcp.isanon && !rcp.fSubtractFeeFromAmount ? rcp.anonfee : 0;

        if(txFee + anonfee > 0)
        {
            // append fee string if a fee is required
            questionString.append("<hr /><span style='color:#aa0000;'>");
            questionString.append(NavCoinUnits::formatHtmlWithUnit(model->getOptionsModel()->getDisplayUnit(), txFee + anonfee));
            questionString.append("</span> ");
            questionString.append(tr("added as transaction fee"));


            // append transaction size
            questionString.append(" (" + QString::number((double)currentTransaction.getTransactionSize() / 1000) + " kB)");

            if(rcp.fSubtractFeeFromAmount && anonfee > 0)
            {
                questionString.append("<br>" + tr("The following fee will be deducted") + ":");
                questionString.append(NavCoinUnits::formatHtmlWithUnit(model->getOptionsModel()->getDisplayUnit(), anonfee));
            }

            if(rcp.isanon){
                questionString.append("<br>" + tr("Navtech server fee:") +QString(" ")+ QString::number(rcp.transaction_fee) + "% "+ tr(rcp.fSubtractFeeFromAmount ? "" : "(already included)") + "<br>");
                if(rcp.fSubtractFeeFromAmount)
                    questionString.append("<span style='color:#aa0000;'>" + NavCoinUnits::formatHtmlWithUnit(model->getOptionsModel()->getDisplayUnit(), nTotalAmount * ((rcp.transaction_fee/100))) + "</span> " + tr("will be deducted as Navtech fee.") + "<br>");

            }

        }

    }



    // add total amount in all subdivision units
    questionString.append("<hr />");
    CAmount totalAmount = currentTransaction.getTotalTransactionAmount() + txFee + anonfee;
    QStringList alternativeUnits;
    Q_FOREACH(NavCoinUnits::Unit u, NavCoinUnits::availableUnits())
    {
        if(u != model->getOptionsModel()->getDisplayUnit())
            alternativeUnits.append(NavCoinUnits::formatHtmlWithUnit(u, totalAmount));
    }
    questionString.append(tr("Total Amount %1")
        .arg(NavCoinUnits::formatHtmlWithUnit(model->getOptionsModel()->getDisplayUnit(), totalAmount)));
    questionString.append(QString("<span style='font-size:10pt;font-weight:normal;'><br />(=%2)</span>")
        .arg(alternativeUnits.join(" " + tr("or") + "<br />")));

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
    if (model->getOptionsModel()->getCoinControlFeatures()) // coin control enabled
        sendStatus = model->sendCoins(currentTransaction, CoinControlDialog::coinControl);
    else
        sendStatus = model->sendCoins(currentTransaction);

    // process sendStatus and on error generate message shown to user
    processSendCoinsReturn(sendStatus);

    if (sendStatus.status == WalletModel::OK)
    {
        accept();
        CoinControlDialog::coinControl->UnSelectAll();
        coinControlUpdateLabels();
    }
    fNewRecipientAllowed = true;
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

void SendCoinsDialog::showNavTechDialog()
{
    navtechsetup* setupNavTech = new navtechsetup();
    setupNavTech->setWindowIcon(QIcon(":icons/navcoin"));
    setupNavTech->setStyleSheet(Skinize());

    setupNavTech->exec();

    checkNavtechServers();
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

    // Focus the field, so that entry can start immediately
    entry->clear();
    entry->setFocus();
    qApp->processEvents();

    updateTabsAndLabels();
    return entry;
}

void SendCoinsDialog::updateTabsAndLabels()
{
    setupTabChain(0);
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

QWidget *SendCoinsDialog::setupTabChain(QWidget *prev)
{
    for(int i = 0; i < ui->entries->count(); ++i)
    {
        SendCoinsEntry *entry = qobject_cast<SendCoinsEntry*>(ui->entries->itemAt(i)->widget());
        if(entry)
        {
            prev = entry->setupTabChain(prev);
        }
    }
    QWidget::setTabOrder(prev, ui->sendButton);

    return ui->sendButton;
}

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
                                 const CAmount& watchBalance, const CAmount& watchUnconfirmedBalance, const CAmount& watchImmatureBalance)
{
    Q_UNUSED(unconfirmedBalance);
    Q_UNUSED(stakingBalance);
    Q_UNUSED(immatureBalance);
    Q_UNUSED(watchBalance);
    Q_UNUSED(watchUnconfirmedBalance);
    Q_UNUSED(watchImmatureBalance);

    for(int i = 0; i < ui->entries->count(); ++i)
    {
        SendCoinsEntry *entry = qobject_cast<SendCoinsEntry*>(ui->entries->itemAt(i)->widget());
        if(entry)
        {
            entry->setTotalAmount(balance);
        }
    }

    if(model && model->getOptionsModel())
    {
        ui->labelBalance->setText(NavCoinUnits::formatWithUnit(0, balance) + (model->getOptionsModel()->getDisplayUnit() != 0 ?( " (" + NavCoinUnits::formatWithUnit(model->getOptionsModel()->getDisplayUnit(), balance) + ")") : ""));
    }
}

void SendCoinsDialog::useFullAmount()
{
  for(int i = 0; i < ui->entries->count(); ++i)
  {
      SendCoinsEntry *entry = qobject_cast<SendCoinsEntry*>(ui->entries->itemAt(i)->widget());
      if(entry)
      {
          entry->useFullAmount();
      }
  }
}

void SendCoinsDialog::updateDisplayUnit()
{
    setBalance(model->getBalance(), 0, 0, 0, 0, 0, 0);
    ui->customFee->setDisplayUnit(model->getOptionsModel()->getDisplayUnit());
    updateMinFeeLabel();
    updateSmartFeeLabel();
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
        msgParams.first = tr("The transaction was rejected! This might happen if some of the coins in your wallet were already spent, such as if you used a copy of wallet.dat and coins were spent in the copy but not marked as spent here.");
        msgParams.second = CClientUIInterface::MSG_ERROR;
        break;
    case WalletModel::AbsurdFee:
        msgParams.first = tr("A fee higher than %1 is considered an absurdly high fee.").arg(NavCoinUnits::formatWithUnit(model->getOptionsModel()->getDisplayUnit(), maxTxFee));
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

void SendCoinsDialog::minimizeFeeSection(bool fMinimize)
{
    //ui->labelFeeMinimized->setVisible(fMinimize);
    //ui->buttonChooseFee  ->setVisible(fMinimize);
    //ui->buttonMinimizeFee->setVisible(!fMinimize);
    ui->frameFeeSelection->setVisible(!fMinimize);
    ui->sendButton       ->setVisible(fMinimize);
    ui->label            ->setVisible(fMinimize);
    ui->labelBalance     ->setVisible(fMinimize);
    ui->noNavtechButton  ->setVisible(fMinimize);
    //ui->horizontalLayoutSmartFee->setContentsMargins(0, (fMinimize ? 0 : 6), 0, 0);

    if(fMinimize)
        checkNavtechServers();
    else
        ui->noNavtechLabel  ->setVisible(false);
        ui->anonsendCheckbox->setVisible(false);


    fFeeMinimized = fMinimize;
}

void SendCoinsDialog::setMinimumFee()
{
    ui->radioCustomPerKilobyte->setChecked(true);
    ui->customFee->setValue(CWallet::GetRequiredFee(1000));
}

void SendCoinsDialog::updateFeeSectionControls()
{
    ui->sliderSmartFee          ->setEnabled(ui->radioSmartFee->isChecked());
    ui->labelSmartFee           ->setEnabled(ui->radioSmartFee->isChecked());
    ui->labelSmartFee2          ->setEnabled(ui->radioSmartFee->isChecked());
    ui->labelSmartFee3          ->setEnabled(ui->radioSmartFee->isChecked());
    ui->labelFeeEstimation      ->setEnabled(ui->radioSmartFee->isChecked());
    ui->labelSmartFeeNormal     ->setEnabled(ui->radioSmartFee->isChecked());
    ui->labelSmartFeeFast       ->setEnabled(ui->radioSmartFee->isChecked());
    ui->checkBoxMinimumFee      ->setEnabled(ui->radioCustomFee->isChecked());
    ui->labelMinFeeWarning      ->setEnabled(ui->radioCustomFee->isChecked());
    ui->radioCustomPerKilobyte  ->setEnabled(ui->radioCustomFee->isChecked() && !ui->checkBoxMinimumFee->isChecked());
    ui->radioCustomAtLeast      ->setEnabled(ui->radioCustomFee->isChecked() && !ui->checkBoxMinimumFee->isChecked() && CoinControlDialog::coinControl->HasSelected());
    ui->customFee               ->setEnabled(ui->radioCustomFee->isChecked() && !ui->checkBoxMinimumFee->isChecked());
}

void SendCoinsDialog::updateGlobalFeeVariables()
{
    if (ui->radioSmartFee->isChecked())
    {
        nTxConfirmTarget = defaultConfirmTarget - ui->sliderSmartFee->value();
        payTxFee = CFeeRate(0);
    }
    else
    {
        nTxConfirmTarget = defaultConfirmTarget;
        payTxFee = CFeeRate(ui->customFee->value());

        // if user has selected to set a minimum absolute fee, pass the value to coincontrol
        // set nMinimumTotalFee to 0 in case of user has selected that the fee is per KB
        CoinControlDialog::coinControl->nMinimumTotalFee = ui->radioCustomAtLeast->isChecked() ? ui->customFee->value() : 0;
    }
}

void SendCoinsDialog::updateFeeMinimizedLabel()
{
    if(!model || !model->getOptionsModel())
        return;

  //  if (ui->radioSmartFee->isChecked())
  //      ui->labelFeeMinimized->setText(ui->labelSmartFee->text());
  //  else {
  //      ui->labelFeeMinimized->setText(NavCoinUnits::formatWithUnit(model->getOptionsModel()->getDisplayUnit(), ui->customFee->value()) +
  //          ((ui->radioCustomPerKilobyte->isChecked()) ? "/kB" : ""));
  //  }
}

void SendCoinsDialog::updateMinFeeLabel()
{
    if (model && model->getOptionsModel())
        ui->checkBoxMinimumFee->setText(tr("Pay only the required fee of %1").arg(
            NavCoinUnits::formatWithUnit(model->getOptionsModel()->getDisplayUnit(), CWallet::GetRequiredFee(1000)) + "/kB")
        );
}

void SendCoinsDialog::updateSmartFeeLabel()
{
    if(!model || !model->getOptionsModel())
        return;

    int nBlocksToConfirm = defaultConfirmTarget - ui->sliderSmartFee->value();
    int estimateFoundAtBlocks = nBlocksToConfirm;
    CFeeRate feeRate = mempool.estimateSmartFee(nBlocksToConfirm, &estimateFoundAtBlocks);
    if (feeRate <= CFeeRate(0)) // not enough data => minfee
    {
        ui->labelSmartFee->setText(NavCoinUnits::formatWithUnit(model->getOptionsModel()->getDisplayUnit(),
                                                                std::max(CWallet::fallbackFee.GetFeePerK(), CWallet::GetRequiredFee(1000))) + "/kB");
        ui->labelSmartFee2->show(); // (Smart fee not initialized yet. This usually takes a few blocks...)
        ui->labelFeeEstimation->setText("");
    }
    else
    {
        ui->labelSmartFee->setText(NavCoinUnits::formatWithUnit(model->getOptionsModel()->getDisplayUnit(),
                                                                std::max(feeRate.GetFeePerK(), CWallet::GetRequiredFee(1000))) + "/kB");
        ui->labelSmartFee2->hide();
        ui->labelFeeEstimation->setText(tr("Estimated to begin confirmation within %n block(s).", "", estimateFoundAtBlocks));
    }

    updateFeeMinimizedLabel();
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

// Coin Control: settings menu - coin control enabled/disabled by user
void SendCoinsDialog::coinControlFeatureChanged(bool checked)
{
    ui->frameCoinControl->setVisible(checked);

    if (!checked && model) // coin control features disabled
        CoinControlDialog::coinControl->SetNull();

    coinControlUpdateLabels();
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
    if (state == Qt::Unchecked)
    {
        CoinControlDialog::coinControl->destChange = CNoDestination();
        ui->labelCoinControlChangeLabel->clear();
    }
    else
        // use this to re-validate an already entered address
        coinControlChangeEdited(ui->lineEditCoinControlChange->text());

    ui->lineEditCoinControlChange->setEnabled((state == Qt::Checked));
}

// Coin Control: custom change address changed
void SendCoinsDialog::coinControlChangeEdited(const QString& text)
{
    if (model && model->getAddressTableModel())
    {
        // Default to no change address until verified
        CoinControlDialog::coinControl->destChange = CNoDestination();
        ui->labelCoinControlChangeLabel->setStyleSheet("QLabel{color:red;}");

        CNavCoinAddress addr = CNavCoinAddress(text.toStdString());

        if (text.isEmpty()) // Nothing entered
        {
            ui->labelCoinControlChangeLabel->setText("");
        }
        else if (!addr.IsValid()) // Invalid address
        {
            ui->labelCoinControlChangeLabel->setText(tr("Warning: Invalid NavCoin address"));
        }
        else // Valid address
        {
            CKeyID keyid;
            addr.GetKeyID(keyid);
            if (!model->havePrivKey(keyid)) // Unknown change address
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
            }
        }
    }
}

// Coin Control: update labels
void SendCoinsDialog::coinControlUpdateLabels()
{
    if (!model || !model->getOptionsModel())
        return;

    if (model->getOptionsModel()->getCoinControlFeatures())
    {
        // enable minimum absolute fee UI controls
        ui->radioCustomAtLeast->setVisible(true);

        // only enable the feature if inputs are selected
        ui->radioCustomAtLeast->setEnabled(CoinControlDialog::coinControl->HasSelected());
    }
    else
    {
        // in case coin control is disabled (=default), hide minimum absolute fee UI controls
        ui->radioCustomAtLeast->setVisible(false);
        return;
    }

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

    if (CoinControlDialog::coinControl->HasSelected())
    {
        // actual coin control calculation
        CoinControlDialog::updateLabels(model, this);

        // show coin control stats
        ui->labelCoinControlAutomaticallySelected->hide();
        ui->widgetCoinControl->show();
    }
    else
    {
        // hide coin control stats
        ui->labelCoinControlAutomaticallySelected->show();
        ui->widgetCoinControl->hide();
        ui->labelCoinControlInsuffFunds->hide();
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
