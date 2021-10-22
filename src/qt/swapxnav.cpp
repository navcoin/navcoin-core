// Copyright (c) 2020 The Navcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "swapxnav.h"

SwapXNAVDialog::SwapXNAVDialog(QWidget *parent) :
    layout(new QVBoxLayout)
{
    this->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Expanding);
    this->resize(600,250);
    this->setLayout(layout);

    QFont amountFont("Sans Serif", 8, QFont::Bold);
    QFont titleFont("Sans Serif", 16, QFont::Bold);
    label1 = new QLabel("");
    label1->setFont(amountFont);
    label2 = new QLabel("");
    label2->setFont(amountFont);

    toplabel1 = new QLabel("");
    toplabel1->setFont(titleFont);
    toplabel2 = new QLabel("");
    toplabel2->setFont(titleFont);

    icon1 = new QLabel("");
    icon2 = new QLabel("");

    amount = new NavcoinAmountField(0, 0);

    QPixmap pixmap(":/icons/swap");
    QIcon ButtonIcon(pixmap);
    swapButton = new QPushButton();
    swapButton->setIcon(ButtonIcon);
    swapButton->setIconSize(QSize(64, 64));
    swapButton->setFixedSize(QSize(64, 64));

    connect(swapButton, SIGNAL(clicked()), this, SLOT(Swap()));

    fMode = false;

    QHBoxLayout *iconsLayout = new QHBoxLayout();
    QVBoxLayout *icon1Layout = new QVBoxLayout();
    QVBoxLayout *icon2Layout = new QVBoxLayout();

    icon1Layout->addWidget(toplabel1);
    icon1Layout->addWidget(icon1);
    icon1Layout->addWidget(label1);
    icon1Layout->setAlignment(icon1, Qt::AlignCenter);
    icon1Layout->setAlignment(label1, Qt::AlignCenter);
    icon1Layout->setAlignment(toplabel1, Qt::AlignCenter);

    icon2Layout->addWidget(toplabel2);
    icon2Layout->addWidget(icon2);
    icon2Layout->addWidget(label2);
    icon2Layout->setAlignment(icon2, Qt::AlignCenter);
    icon2Layout->setAlignment(label2, Qt::AlignCenter);
    icon2Layout->setAlignment(toplabel2, Qt::AlignCenter);

    iconsLayout->addLayout(icon1Layout);
    iconsLayout->addWidget(swapButton);
    iconsLayout->addLayout(icon2Layout);

    QLabel *amountLabel = new QLabel(tr("Swap amount: "));
    amountLabel->setFont(titleFont);

    QHBoxLayout *okLayout = new QHBoxLayout();
    QPushButton* okButton = new QPushButton(tr("Swap"));

    okLayout->addStretch();
    okLayout->addWidget(okButton);
    okLayout->addStretch();

    connect(okButton, SIGNAL(clicked()), this, SLOT(Ok()));

    layout->addSpacing(10);
    layout->addWidget(amountLabel);
    layout->addWidget(amount);
    layout->addSpacing(20);
    layout->addLayout(iconsLayout);
    layout->addSpacing(20);
    layout->addLayout(okLayout);
    layout->addSpacing(10);

    layout->setAlignment(amountLabel, Qt::AlignCenter);
    layout->setAlignment(amount, Qt::AlignCenter);

    Swap();
}

void SwapXNAVDialog::setModel(WalletModel *model)
{
    this->model = model;
}

void SwapXNAVDialog::setClientModel(ClientModel *model)
{
    this->clientModel = model;
}

void SwapXNAVDialog::SetPublicBalance(CAmount a)
{
    int unit = 0;

    this->publicBalance = a;
    if (fMode)
        label1->setText(QString::fromStdString(_("Available: ")) + NavcoinUnits::formatWithUnit(unit, a, false, NavcoinUnits::separatorAlways, false));
    else
        label2->setText(QString::fromStdString(_("Available: ")) + NavcoinUnits::formatWithUnit(unit, a, false, NavcoinUnits::separatorAlways, false));
}

void SwapXNAVDialog::SetPrivateBalance(CAmount a)
{
    int unit = 0;

    this->privateBalance = a;
    if (fMode)
        label2->setText(QString::fromStdString(_("Available: ")) + NavcoinUnits::formatWithUnit(unit, a, false, NavcoinUnits::separatorAlways, true));
    else
        label1->setText(QString::fromStdString(_("Available: ")) + NavcoinUnits::formatWithUnit(unit, a, false, NavcoinUnits::separatorAlways, true));
}

void SwapXNAVDialog::Swap()
{
    fMode = !fMode;

    QPixmap pix(":/icons/mininav");
    QPixmap scaled = pix.scaled(64,64,Qt::KeepAspectRatio,Qt::SmoothTransformation);

    QBitmap map(64,64);
    map.fill(Qt::color0);
    QPainter painter( &map );
    painter.setBrush(Qt::color1);
    painter.drawRect( 0, 0, 64, 64);
    scaled.setMask(map);

    QPixmap pix2(":/icons/minixnav");
    QPixmap scaled2 = pix2.scaled(64,64,Qt::KeepAspectRatio,Qt::SmoothTransformation);

    QBitmap map2(64,64);
    map2.fill(Qt::color0);
    QPainter painter2( &map2 );
    painter2.setBrush(Qt::color1);
    painter2.drawRect( 0, 0, 64, 64);
    scaled2.setMask(map2);

    if (fMode)
    {
        toplabel1->setText(tr("From: NAV"));
        icon1->setPixmap(scaled);
        toplabel2->setText(tr("To: xNAV"));
        icon2->setPixmap(scaled2);
    }
    else
    {
        toplabel1->setText(tr("From: xNAV"));
        icon1->setPixmap(scaled2);
        toplabel2->setText(tr("To: NAV"));
        icon2->setPixmap(scaled);
    }

    SetPublicBalance(this->publicBalance);
    SetPrivateBalance(this->privateBalance);
}

void SwapXNAVDialog::Ok()
{
    CAmount nAmount = amount->value();

    if (nAmount <= 0)
    {
        QMessageBox msgBox(this);
        std::string str = tr("The amount must be greater than 0").toStdString();
        msgBox.setText(tr(str.c_str()));
        msgBox.addButton(tr("Ok"), QMessageBox::AcceptRole);
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.setWindowTitle("Invalid amount");
        msgBox.exec();
        return;
    }

    if ((fMode ? publicBalance : privateBalance) < nAmount)
    {
        QMessageBox msgBox(this);
        std::string str = tr("You don't have that many coins to swap!\n\nAvailable:\n%1").arg(NavcoinUnits::formatWithUnit(0, fMode ? publicBalance : privateBalance, false, NavcoinUnits::separatorAlways, !fMode)).toStdString();
        msgBox.setText(tr(str.c_str()));
        msgBox.addButton(tr("Ok"), QMessageBox::AcceptRole);
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.setWindowTitle("Insufficient NAV");
        msgBox.exec();
        return;
    }

    CWalletTx wtx;

    // Ensure wallet is unlocked
    WalletModel::UnlockContext ctx(model->requestUnlock());
    if(!ctx.isValid())
    {
        // Unlock wallet was cancelled
        return;
    }

    CNavcoinAddress address;

    if (fMode)
    {
        blsctDoublePublicKey k;
        if (pwalletMain->GetBLSCTSubAddressPublicKeys(std::make_pair(0, 0), k))
            address = CNavcoinAddress(k);
        else
        {
            QMessageBox msgBox(this);
            msgBox.setText(tr("Swap failed: your wallet does not support receiving xNAV"));
            msgBox.addButton(tr("Ok"), QMessageBox::AcceptRole);
            msgBox.setIcon(QMessageBox::Warning);
            msgBox.setWindowTitle("Error");
            msgBox.exec();

            QWidget::close();
            return;
        }
    }
    else
    {
        if (!pwalletMain->IsLocked())
            pwalletMain->TopUpKeyPool();

        // Generate a new key that is added to wallet
        CPubKey newKey;
        if (!pwalletMain->GetKeyFromPool(newKey))
        {
            QMessageBox msgBox(this);
            msgBox.setText(tr("Swap failed: keypool run out!"));
            msgBox.addButton(tr("Ok"), QMessageBox::AcceptRole);
            msgBox.setIcon(QMessageBox::Warning);
            msgBox.setWindowTitle("Error");
            msgBox.exec();

            QWidget::close();
            return;
        }
        CKeyID keyID = newKey.GetID();

        address = CNavcoinAddress(keyID);
    }

    CScript scriptPubKey = GetScriptForDestination(address.Get());

    // Create and send the transaction
    CReserveKey reservekey(pwalletMain);
    CAmount nFeeRequired;
    std::string strError;
    vector<CRecipient> vecSend;
    int nChangePosRet = -1;
    CRecipient recipient = {scriptPubKey, nAmount, true, fMode};
    if (fMode)
    {
        bls::G1Element vk, sk;
        blsctDoublePublicKey dk = boost::get<blsctDoublePublicKey>(address.Get());

        if (!dk.GetSpendKey(sk) || !dk.GetViewKey(vk))
        {
            QMessageBox msgBox(this);
            msgBox.setText(tr("Swap failed: invalid address"));
            msgBox.addButton(tr("Ok"), QMessageBox::AcceptRole);
            msgBox.setIcon(QMessageBox::Warning);
            msgBox.setWindowTitle("Error");
            msgBox.exec();

            QWidget::close();
            return;
        }
        recipient.sk = sk.Serialize();
        recipient.vk = vk.Serialize();
        recipient.sMemo = "Swap";
    }
    vecSend.push_back(recipient);

    std::vector<std::shared_ptr<CReserveBLSCTBlindingKey>> reserveBLSCTKey;

    for (unsigned int i = 0; i < vecSend.size()+2; i++)
    {
        std::shared_ptr<CReserveBLSCTBlindingKey> rk(new CReserveBLSCTBlindingKey(pwalletMain));
        reserveBLSCTKey.insert(reserveBLSCTKey.begin(), std::move(rk));
    }

    CandidateTransaction selectedCoins;

    if (!fMode)
    {
        AggregationSessionDialog msd(0);
        msd.setWalletModel(model);
        msd.setClientModel(clientModel);
        if (!msd.exec())
            QMessageBox::critical(this, tr("Something failed"), tr("The mixing process failed, if you continue, the transaction would be sent with limited privacy and the source could be easily identified by an observer. Please, review your connection to the network."));
        else
        {
            selectedCoins = msd.GetSelectedCoins();
            if (selectedCoins.tx.vout.size() == 0)
                QMessageBox::critical(this, tr("No coins for mixing"), tr("We could not find any candidate coin, if you continue, the transaction would be sent with limited privacy and the source could be easily identified by an observer. Please, review your connection to the network."));
        }
    }

    if (pwalletMain->CreateTransaction(vecSend, wtx, reservekey, reserveBLSCTKey, nFeeRequired, nChangePosRet, strError, !fMode, NULL, true, &selectedCoins)) {
        if (QMessageBox::No == QMessageBox(QMessageBox::Information,
                                           tr("Swap!"),
                                           tr("In order to swap %1 to %2, you will have to pay a fee of %3.").arg(
                                               NavcoinUnits::formatWithUnit(0, nAmount, false, NavcoinUnits::separatorAlways, !fMode),
                                               fMode ? "xNAV" : "NAV",
                                               NavcoinUnits::formatWithUnit(0, nFeeRequired, false, NavcoinUnits::separatorAlways, !fMode))
                                               +"<br><br>"+tr("You will receive a total of %1.").arg(
                                               NavcoinUnits::formatWithUnit(0, nAmount - nFeeRequired, false, NavcoinUnits::separatorAlways, fMode))
                                               +"<br><br>"+tr("Do you want to continue?"),
                                           QMessageBox::Yes|QMessageBox::No).exec())
        {
            QWidget::close();
            return;
        }
    }
    else
    {
        // Display something went wrong UI
        QMessageBox msgBox(this);
        msgBox.setText(tr("Swap failed: %1").arg(QString::fromStdString(strError)));
        msgBox.addButton(tr("Ok"), QMessageBox::AcceptRole);
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.setWindowTitle("Error");
        msgBox.exec();

        QWidget::close();
        return;
    }

    if (!pwalletMain->CommitTransaction(wtx, reservekey, reserveBLSCTKey)) {
        // Display something went wrong UI
        QMessageBox msgBox(this);
        msgBox.setText(tr("Swap failed"));
        msgBox.addButton(tr("Ok"), QMessageBox::AcceptRole);
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.setWindowTitle("Error");
        msgBox.exec();
    }
    else
    {
        // Display something went wrong UI
        QMessageBox msgBox(this);
        msgBox.setText(tr("Swap succeded"));
        msgBox.addButton(tr("Ok"), QMessageBox::AcceptRole);
        msgBox.setIcon(QMessageBox::Information);
        msgBox.setWindowTitle("Ok!");
        msgBox.exec();
    }

    QWidget::close();
    return;
}
