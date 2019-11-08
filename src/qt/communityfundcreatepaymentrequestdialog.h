#ifndef COMMUNITYFUNDCREATEPAYMENTREQUESTDIALOG_H
#define COMMUNITYFUNDCREATEPAYMENTREQUESTDIALOG_H

#include <../qvalidatedplaintextedit.h>
#include <uint256.h>
#include <qt/walletmodel.h>

#include <QDialog>

namespace Ui {
class CommunityFundCreatePaymentRequestDialog;
}

class CommunityFundCreatePaymentRequestDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CommunityFundCreatePaymentRequestDialog(QWidget *parent = 0);
    ~CommunityFundCreatePaymentRequestDialog();

    void setModel(WalletModel *model);

private:
    Ui::CommunityFundCreatePaymentRequestDialog *ui;
    WalletModel *model;
    bool validate();
    bool isActiveProposal(uint256 hash);

public Q_SLOTS:
    void click_pushButtonSubmitPaymentRequest();
};

#endif // COMMUNITYFUNDCREATEPAYMENTREQUESTDIALOG_H
