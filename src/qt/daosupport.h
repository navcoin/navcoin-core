// Copyright (c) 2019 The NavCoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef DAOSUPPORT_H
#define DAOSUPPORT_H

#include "consensus/dao.h"
#include "daoproposeanswer.h"
#include "main.h"
#include "skinize.h"
#include "walletmodel.h"

#include <QCheckBox>
#include <QDialog>
#include <QFont>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QVariant>
#include <QVBoxLayout>
#include <QWidget>

class DaoSupport : public QDialog
{
    Q_OBJECT

public:
    DaoSupport(QWidget *parent, CConsultation consultation);

    void setModel(WalletModel *model);

private:
    WalletModel *model;
    QVBoxLayout *layout;
    CConsultation consultation;
    QGroupBox* answerBox;
    QLabel *warningLbl;

    void showWarning(QString text);

private Q_SLOTS:
    void onSupport(bool fChecked);
    void onPropose();
    void onClose();
};

#endif // DAOSUPPORT_H
