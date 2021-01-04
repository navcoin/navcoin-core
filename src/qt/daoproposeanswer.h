// Copyright (c) 2019-2020 The NavCoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef DAOPROPOSEANSWER_H
#define DAOPROPOSEANSWER_H

#include "consensus/dao.h"
#include "main.h"
#include "walletmodel.h"

#include <QCheckBox>
#include <QDialog>
#include <QFont>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QVariant>
#include <QVBoxLayout>
#include <QWidget>

typedef std::function<bool(QString)> ValidatorFunc;

class DaoProposeAnswer : public QDialog
{
    Q_OBJECT

public:
    DaoProposeAnswer(QWidget *parent, CConsultation consultation, ValidatorFunc validator);

    void setModel(WalletModel *model);

private:
    WalletModel *model;
    QVBoxLayout *layout;
    CConsultation consultation;
    QLineEdit* answerInput;
    QLabel *warningLbl;

    ValidatorFunc validatorFunc;

    void showWarning(QString text);

private Q_SLOTS:
    void onPropose();
    void onClose();
};


#endif // DAOPROPOSEANSWER_H
