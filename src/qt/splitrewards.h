// Copyright (c) 2019-2020 The NavCoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef SPLITREWARDS_H
#define SPLITREWARDS_H

#include "main.h"
#include "util.h"
#include "qjsonmodel.h"
#include "utilmoneystr.h"
#include "walletmodel.h"

#include <QComboBox>
#include <QDialog>
#include <QInputDialog>
#include <QJsonObject>
#include <QJsonDocument>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QStringList>
#include <QTreeView>
#include <QVBoxLayout>

static QMap<QString, QString> teamAddresses = {{"NN5QSSMAdtRU35BffLZUw9vChnhHKKMeuL", "Alex aguycalled - Core Dev"},
                                               {"NPAxaKnCb7ukrUvFmntSyZWcZgbrGHUeC4", "Craig proletesseract - Core Dev"},
                                               {"NRXfZ1egFxMSUsc4Ufpi4Lm7DdXStYmeBG", "mxaddict - Core Dev"},
                                               {"NcPw2tTTEgfrCGD53TwxRbkfG8oniyED1o", "salmonskinroll - Core Tester"},
                                               {"XinrZWZvvp7QAizF8CREuVbA5LKDzB5bMgeK7uM3YdYgeYsniYFcyuinJzs8N", "0x2830 - Privacy Dev"},
                                               {"3HnzbJ4TR9", "Community Fund"},
                                               {"XVPMwBdNU9ou3a3TnwaVgAgEecbdsEVZHbVmeY4TMAHbY6BdtY8xW6m1Q1rkb", "Core Dev Bounty"},
                                               {"XAznGHuQ35hvgSGsVWi5Nu2Y6n3rT4cycE3yfZWCfnNjycCGdGAEnta2G24Mi", "Web Dev Bounty"},
                                               {"", "Custom"}};

class SplitRewardsDialog : public QDialog
{
    Q_OBJECT

public:
    SplitRewardsDialog(QWidget *parent);

    void setModel(WalletModel *model);
    void showFor(QString address);

private:
    WalletModel *model;
    QJsonObject jsonObject;
    QString currentAddress;
    QLabel *strDesc;
    QTreeView *tree;
    QComboBox *comboAddress;

    int availableAmount;

private Q_SLOTS:
    void onAdd();
    void onRemove();
    void onSave();
    void onEdit();
    void onQuit();
};

CAmount PercentageToNav(int percentage);

#endif // SPLITREWARDS_H
