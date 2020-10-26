// Copyright (c) 2020 The NavCoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef SWAPXNAV_H
#define SWAPXNAV_H

#include "aggregationsession.h"
#include "navcoinunits.h"
#include "optionsmodel.h"
#include "navcoinamountfield.h"
#include "blsct/key.h"
#include "wallet/wallet.h"
#include "clientmodel.h"
#include "walletmodel.h"

#include <QApplication>
#include <QPixmap>
#include <QBitmap>
#include <QPushButton>
#include <QPainter>
#include <QSpacerItem>
#include <QMessageBox>
#include <QDialog>
#include <QLabel>
#include <QHBoxLayout>
#include <QVBoxLayout>

#define DEFAULT_UNIT 0

class SwapXNAVDialog : public QDialog
{
    Q_OBJECT

public:
    SwapXNAVDialog(QWidget *parent);

    void setModel(WalletModel *model);
    void setClientModel(ClientModel *model);
    void SetPublicBalance(CAmount a);
    void SetPrivateBalance(CAmount a);

private:
    CAmount publicBalance;
    CAmount privateBalance;

    bool fMode;

    WalletModel *model;
    ClientModel *clientModel;
    QVBoxLayout* layout;
    QLabel *label1;
    QLabel *label2;
    QLabel *toplabel1;
    QLabel *toplabel2;
    QPushButton *swapButton;
    NavCoinAmountField *amount;

    QLabel *icon1;
    QLabel *icon2;

private Q_SLOTS:
    void Swap();
    void Ok();
};

#endif // SWAPXNAV_H
