// Copyright (c) 2018 alex v
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef COLDSTAKINGWIZARD_H
#define COLDSTAKINGWIZARD_H

#include <qt/guiconstants.h>
#include <qt/guiutil.h>
#include <qt/receiverequestdialog.h>

#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWizard>
#include <QtWidgets>

#ifdef USE_QRCODE
#include <qrencode.h>
#endif

class ColdStakingWizard : public QWizard
{
    Q_OBJECT

public:
    ColdStakingWizard(QWidget *parent = 0);

    void accept() override;
};

class IntroPage : public QWizardPage
{
    Q_OBJECT

public:
    IntroPage(QWidget *parent = 0);

private:
    QLabel *label;
};

class GetAddressesPage : public QWizardPage
{
    Q_OBJECT

public:
    GetAddressesPage(QWidget *parent = 0);
    bool validatePage();

private:
    QLabel *spendingAddressLabel;
    QLabel *stakingAddressLabel;
    QLabel *descriptionLabel;
    QLabel *errorLabel;
    QLineEdit *spendingAddressLineEdit;
    QLineEdit *stakingAddressLineEdit;
};

class ColdStakingAddressPage : public QWizardPage
{
    Q_OBJECT

public:
    ColdStakingAddressPage(QWidget *parent = 0);
    void initializePage();

private:
    QRImageWidget *image;
    QLineEdit *address;
    QPushButton *button;
    QString coldStakingAddress;

private Q_SLOTS:
    void copyToClipboard();
};

#endif // COLDSTAKINGWIZARD_H
