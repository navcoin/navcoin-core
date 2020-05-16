// Copyright (c) 2020 The NavCoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QT_AGGREGATIONSESSION_H
#define QT_AGGREGATIONSESSION_H

#include "clientmodel.h"
#include "blsct/aggregationsession.h"
#include "blsct/transaction.h"
#include "main.h"
#include "walletmodel.h"
#include "wallet/wallet.h"

#include <guiutil.h>

#include <QDialog>
#include <QMessageBox>
#include <QLabel>
#include <QProgressBar>
#include <QString>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

class ClientModel;
class PlatformStyle;
class WalletModel;

class AggregationSesionDialog : public QDialog
{
    Q_OBJECT

public:
    AggregationSesionDialog(QWidget *parent);
    void ShowError(QString string);
    void SetTopLabel(QString string);
    void SetBottomLabel(QString string);
    void SetBottom2Label(QString string);

    CandidateTransaction GetSelectedCoins();

    void setClientModel(ClientModel *clientModel);
    void setWalletModel(WalletModel *walletModel);

private Q_SLOTS:
    void NewAggregationSesion(std::string hs);
    void CheckStatus();

private:
    ClientModel *clientModel;
    WalletModel *walletModel;

    QVBoxLayout *layout;
    QLabel *topStatus;
    QLabel *bottomStatus;
    QLabel *bottom2Status;

    uint64_t nExpiresAt;

    CandidateTransaction selectedCoins;

    bool fReady;
};

#endif // QT_AGGREGATIONSESSION_H
