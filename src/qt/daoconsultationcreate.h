// Copyright (c) 2019 The NavCoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef DAOCONSULTATIONCREATE_H
#define DAOCONSULTATIONCREATE_H

#include "consensus/dao.h"
#include "main.h"
#include "skinize.h"
#include "walletmodel.h"

#include <QCheckBox>
#include <QDialog>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>

class DaoConsultationCreate : public QDialog
{
    Q_OBJECT

public:
    DaoConsultationCreate(QWidget *parent);

    void setModel(WalletModel *model);

private:
    WalletModel *model;
    QVBoxLayout* layout;
    QLineEdit* questionInput;
    QCheckBox* rangeBox;
    QSpinBox* minBox;
    QSpinBox* maxBox;
    QLabel* minLbl;
    QLabel* maxLbl;
    QLabel *warningLbl;

    void showWarning(QString text);

private Q_SLOTS:
    void onCreate();
    void onRange(bool fChecked);
};

#endif // DAOCONSULTATIONCREATE_H
