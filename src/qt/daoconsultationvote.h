// Copyright (c) 2019-2020 The NavCoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef DAOCONSULTATIONVOTE_H
#define DAOCONSULTATIONVOTE_H

#include "consensus/dao.h"
#include "main.h"

#include <QCheckBox>
#include <QDialog>
#include <QFont>
#include <QGroupBox>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>
#include <QVariant>
#include <QVBoxLayout>
#include <QWidget>

class DaoConsultationVote : public QDialog
{
    Q_OBJECT

public:
    DaoConsultationVote(QWidget *parent, CConsultation consultation);

private:
    QVBoxLayout *layout;
    QLabel *questionLbl;
    CConsultation consultation;
    QGroupBox* answersBox;
    QVector<QCheckBox*> answers;
    QLabel *warningLbl;
    QSpinBox* amountBox;

    int nChecked;

private Q_SLOTS:
    void onAnswer(bool fChecked);
    void onAbstain();
    void onRemove();
    void onClose();
};

#endif // DAOCONSULTATIONVOTE_H
