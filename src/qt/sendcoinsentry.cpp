// Copyright (c) 2011-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/sendcoinsentry.h>
#include <ui_sendcoinsentry.h>

#include <qt/addressbookpage.h>
#include <qt/addresstablemodel.h>
#include <qt/guiutil.h>
#include <qt/optionsmodel.h>
#include <qt/platformstyle.h>
#include <util.h>
#include <utils/dns_utils.h>
#include <qt/walletmodel.h>
#include <qt/coincontroldialog.h>

#include <QApplication>
#include <QClipboard>
#include <QSettings>

SendCoinsEntry::SendCoinsEntry(const PlatformStyle *platformStyle, QWidget *parent) :
    QStackedWidget(parent),
    ui(new Ui::SendCoinsEntry),
    model(0),
    totalAmount(0),
    totalPrivateAmount(0),
    fPrivate(0),
    platformStyle(platformStyle)
{
    ui->setupUi(this);

    ui->addressBookButton->setIcon(platformStyle->Icon(":/icons/address-book"));
    ui->deleteButton_is->setIcon(platformStyle->Icon(":/icons/remove"));
    ui->deleteButton_s->setIcon(platformStyle->Icon(":/icons/remove"));

    setCurrentWidget(ui->SendCoins);

    ui->addAsLabel->setPlaceholderText(tr("Enter a label for this address to add it to your address book"));

    // normal navcoin address field
    GUIUtil::setupAddressWidget(ui->payTo, this);
    GUIUtil::setupAddressWidget(ui->customChange, this);
    // just a label for displaying navcoin address(es)
    ui->payTo_is->setFont(GUIUtil::fixedPitchFont());

    QPixmap p1(":/icons/mininav");
    QPixmap p2(":/icons/minixnav");

    ui->fromBox->insertItem(0,"Public NAV");
    ui->fromBox->insertItem(1,"Private xNAV");
    ui->fromBox->setItemData(0,p1,Qt::DecorationRole);
    ui->fromBox->setItemData(1,p2,Qt::DecorationRole);
    ui->fromBox->setIconSize(QSize(32,32));

    QSettings settings;

    // Connect signals
    connect(ui->payAmount, SIGNAL(valueChanged()), this, SIGNAL(payAmountChanged()));
    connect(ui->checkboxSubtractFeeFromAmount, SIGNAL(toggled(bool)), this, SIGNAL(subtractFeeFromAmountChanged()));
    connect(ui->deleteButton_is, SIGNAL(clicked()), this, SLOT(deleteClicked()));
    connect(ui->deleteButton_s, SIGNAL(clicked()), this, SLOT(deleteClicked()));
    connect(ui->addressBookCheckBox, SIGNAL(clicked()), this, SLOT(updateAddressBook()));
    connect(ui->useFullButton, SIGNAL(clicked()), this, SLOT(useFullAmount()));
    connect(ui->fromBox, SIGNAL(currentIndexChanged(int)), this, SLOT(fromChanged(int)));
    connect(ui->checkBoxCoinControlChange, SIGNAL(stateChanged(int)), this, SLOT(coinControlChangeCheckedSlot(int)));
    connect(ui->selectCoinsBtn, SIGNAL(clicked()), this, SIGNAL(openCoinControl()));

    ui->checkBoxCoinControlChange->setChecked(settings.value("fUseCustomChangeAddress").toBool());
    coinControlChangeCheckedSlot(ui->checkBoxCoinControlChange->isChecked());
    ui->customChange->setText(settings.value("sCustomChangeAddress").toString());

    connect(ui->customChange, SIGNAL(textChanged(const QString &)), this, SIGNAL(customChangeChanged(const QString &)));

    bool fDefaultPrivate = settings.value("defaultprivate", false).toBool();

    ui->fromBox->setCurrentIndex(fDefaultPrivate);
    ui->amountLabel->setText(fDefaultPrivate ? "A&mount (xNAV):" : "A&mount (NAV):");
    ui->memo->setVisible(false);
    ui->memoLabel->setVisible(false);
    fPrivate = fDefaultPrivate;

    ui->labellLabel->setVisible(ui->addressBookCheckBox->isChecked());
    ui->addAsLabel->setVisible(ui->addressBookCheckBox->isChecked());
}

SendCoinsEntry::~SendCoinsEntry()
{
    delete ui;
}

void SendCoinsEntry::coinControlChangeCheckedSlot(int state)
{
    ui->customChange->clear();
    ui->customChangeLbl->setVisible(state);
    ui->customChange->setVisible(state);
    ui->customChange->setEnabled(state);

    Q_EMIT coinControlChangeChecked(state);
}

void SendCoinsEntry::fromChanged(int index)
{
    fPrivate = index;
    QSettings settings;
    settings.setValue("defaultprivate", index);
    ui->amountLabel->setText(fPrivate ? "A&mount (xNAV):" : "A&mount (NAV):");
    ui->checkBoxCoinControlChange->setEnabled(!fPrivate);
    ui->customChange->setVisible(ui->checkBoxCoinControlChange->isChecked() && ui->checkBoxCoinControlChange->isEnabled());
    ui->customChangeLbl->setVisible(ui->checkBoxCoinControlChange->isChecked() && ui->checkBoxCoinControlChange->isEnabled());
    Q_EMIT privateOrPublicChanged(index);
}

void SendCoinsEntry::setTotalPrivateAmount(const CAmount& amount)
{
    totalPrivateAmount = amount;
    if (fPrivate)
        ui->availableLabel->setText(tr("Available: %1").arg(NavCoinUnits::formatWithUnit(unit, amount, false, NavCoinUnits::separatorAlways, fPrivate)));
}

void SendCoinsEntry::setTotalAmount(const CAmount& amount)
{
    totalAmount = amount;
    if (!fPrivate)
        ui->availableLabel->setText(tr("Available: %1").arg(NavCoinUnits::formatWithUnit(unit, amount, false, NavCoinUnits::separatorAlways, fPrivate)));
}

void SendCoinsEntry::useFullAmount()
{
    ui->payAmount->setValue(fPrivate ? totalPrivateAmount : totalAmount);
}

void SendCoinsEntry::updateAddressBook()
{
    ui->labellLabel->setVisible(ui->addressBookCheckBox->isChecked());
    ui->addAsLabel->setVisible(ui->addressBookCheckBox->isChecked());
}

void SendCoinsEntry::on_addressBookButton_clicked()
{
    if(!model)
        return;
    AddressBookPage dlg(platformStyle, AddressBookPage::ForSelection, AddressBookPage::SendingTab, this);
    dlg.setModel(model->getAddressTableModel());
    if(dlg.exec())
    {
        ui->payTo->setText(dlg.getReturnValue());
        ui->payAmount->setFocus();
    }
}

void SendCoinsEntry::on_payTo_textChanged(const QString &address)
{
    CNavCoinAddress a(address.toStdString());

    bool fShowmemo = (a.IsPrivateAddress(Params()));

    ui->memo->setVisible(fShowmemo);
    ui->memoLabel->setVisible(fShowmemo);

    updateLabel(address);
}

void SendCoinsEntry::setModel(WalletModel *model)
{
    this->model = model;

    if (model && model->getOptionsModel()) {
        connect(model->getOptionsModel(), SIGNAL(displayUnitChanged(int)), this, SLOT(updateDisplayUnit()));
        connect(model->getOptionsModel(), SIGNAL(coinControlFeaturesChanged(bool)), this, SLOT(coinControlFeaturesChanged(bool)));
    }

    Q_EMIT coinControlChangeChecked(ui->checkBoxCoinControlChange->isChecked() ? Qt::Checked : Qt::Unchecked);

    clear();
}

void SendCoinsEntry::clear()
{
    // clear UI elements for normal payment
    ui->payTo->clear();
    ui->addAsLabel->clear();
    ui->payAmount->clear();
    ui->checkboxSubtractFeeFromAmount->setCheckState(Qt::Unchecked);
    ui->messageTextLabel->clear();
    ui->messageTextLabel->hide();
    ui->messageLabel->hide();
    ui->memo->clear();
    // clear UI elements for unauthenticated payment request
    ui->payTo_is->clear();
    ui->memoTextLabel_is->clear();
    ui->payAmount_is->clear();
    // clear UI elements for authenticated payment request
    ui->payTo_s->clear();
    ui->memoTextLabel_s->clear();
    ui->payAmount_s->clear();

    // update the display unit, to not use the default ("NAV")
    updateDisplayUnit();
}

void SendCoinsEntry::deleteClicked()
{
    Q_EMIT removeEntry(this);
}

bool SendCoinsEntry::validate()
{
    if (!model)
        return false;

    // Check input validity
    bool retval = true;

    utils::DNSResolver* DNS = nullptr;

    if(DNS->check_address_syntax(ui->payTo->text().toStdString().c_str()))
    {

        bool dnssec_valid; bool dnssec_available;
        std::vector<std::string> addresses = utils::dns_utils::addresses_from_url(ui->payTo->text().toStdString().c_str(), dnssec_available, dnssec_valid);

        if(addresses.empty() || (!dnssec_valid && GetBoolArg("-requirednssec",true)))
          retval = false;
        else
        {

          ui->addAsLabel->setText(ui->payTo->text());
          ui->payTo->setText(QString::fromStdString(addresses.front()));

        }

    }

    else if (!model->validateAddress(ui->payTo->text()))
    {
        ui->payTo->setValid(false);
        retval = false;
    }

    if (!ui->payAmount->validate())
    {
        retval = false;
    }

    // Sending a zero amount is invalid
    if (ui->payAmount->value(0) <= 0)
    {
        ui->payAmount->setValid(false);
        retval = false;
    }

    if (fPrivate && ui->memo->text().size() > maxMessageSize)
    {
        QMessageBox::critical(this, tr("Wrong size for encrypted message"), tr("The encrypted message can't be longer than %1 characters.").arg(QString::number(maxMessageSize)));
        retval = false;
    }

    // Reject dust outputs:
    if (retval && GUIUtil::isDust(ui->payTo->text(), ui->payAmount->value())) {
        ui->payAmount->setValid(false);
        retval = false;
    }

    return retval;
}

SendCoinsRecipient SendCoinsEntry::getValue()
{
    // Normal payment
    recipient.address = ui->payTo->text();
    recipient.label = ui->addAsLabel->text();
    recipient.amount = ui->payAmount->value();
    recipient.message = ui->memo->text();
    recipient.isanon = ui->fromBox->currentIndex();
    recipient.fSubtractFeeFromAmount = (ui->checkboxSubtractFeeFromAmount->checkState() == Qt::Checked);

    return recipient;
}

//QWidget *SendCoinsEntry::setupTabChain(QWidget *prev)
//{
//    QWidget::setTabOrder(prev, ui->payTo);
//    QWidget::setTabOrder(ui->payTo, ui->addAsLabel);
//    QWidget *w = ui->payAmount->setupTabChain(ui->addAsLabel);
//    QWidget::setTabOrder(w, ui->checkboxSubtractFeeFromAmount);
//    QWidget::setTabOrder(ui->checkboxSubtractFeeFromAmount, ui->addressBookButton);
//    return ui->addressBookButton;
//}

void SendCoinsEntry::setValue(const SendCoinsRecipient &value)
{
    recipient = value;

    // message
    ui->messageTextLabel->setText(recipient.message);
    ui->memo->setText(recipient.message);
    ui->messageTextLabel->setVisible(!recipient.message.isEmpty());
    ui->messageLabel->setVisible(!recipient.message.isEmpty());

    ui->addAsLabel->clear();
    ui->payTo->setText(recipient.address); // this may set a label from addressbook
    if (!recipient.label.isEmpty()) // if a label had been set from the addressbook, don't overwrite with an empty label
        ui->addAsLabel->setText(recipient.label);
    ui->payAmount->setValue(recipient.amount);
}

void SendCoinsEntry::setAddress(const QString &address)
{
    ui->payTo->setText(address);
    ui->payAmount->setFocus();
}

bool SendCoinsEntry::isClear()
{
    return ui->payTo->text().isEmpty() && ui->payTo_is->text().isEmpty() && ui->payTo_s->text().isEmpty();
}

void SendCoinsEntry::setFocus()
{
    ui->payTo->setFocus();
}

void SendCoinsEntry::updateDisplayUnit()
{
    if(model && model->getOptionsModel())
    {
        // Update payAmount with the current unit
        ui->payAmount->setDisplayUnit(model->getOptionsModel()->getDisplayUnit());
        ui->payAmount_is->setDisplayUnit(model->getOptionsModel()->getDisplayUnit());
        ui->payAmount_s->setDisplayUnit(model->getOptionsModel()->getDisplayUnit());
        unit = model->getOptionsModel()->getDisplayUnit();
    }
}

bool SendCoinsEntry::updateLabel(const QString &address)
{
    if(!model)
        return false;

    // Fill in label from address book, if address has an associated label
    QString associatedLabel = model->getAddressTableModel()->labelForAddress(address);
    if(!associatedLabel.isEmpty())
    {
        ui->addAsLabel->setText(associatedLabel);
        return true;
    }

    return false;
}
