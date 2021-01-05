// Copyright (c) 2019-2020 The NavCoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef DAOCONSULTATIONCREATE_H
#define DAOCONSULTATIONCREATE_H

#include "consensus/dao.h"
#include "main.h"
#include "navcoinlistwidget.h"
#include "walletmodel.h"

#include <base58.h>

#include <QCheckBox>
#include <QDialog>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QRadioButton>
#include <QSpinBox>
#include <QVBoxLayout>

class DaoConsultationCreate : public QDialog
{
    Q_OBJECT

public:
    DaoConsultationCreate(QWidget *parent);
    DaoConsultationCreate(QWidget *parent, QString title, int consensuspos);

    void setModel(WalletModel *model);

private:
    WalletModel *model;
    QVBoxLayout* layout;
    QLineEdit* questionInput;
    QRadioButton* answerIsNumbersBtn;
    QRadioButton* answerIsFromListBtn;
    QSpinBox* minBox;
    QSpinBox* maxBox;
    QLabel* minLbl;
    QLabel* maxLbl;
    QLabel *warningLbl;
    NavCoinListWidget* listWidget;
    QCheckBox* moreAnswersBox;

    int cpos;
    QString title;

    void showWarning(QString text);

private Q_SLOTS:
    void onCreate();
    void onCreateConsensus();
    void onRange(bool fChecked);
    void onList(bool fChecked);
};

#endif // DAOCONSULTATIONCREATE_H
