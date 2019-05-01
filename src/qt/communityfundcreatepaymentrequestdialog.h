#ifndef COMMUNITYFUNDCREATEPAYMENTREQUESTDIALOG_H
#define COMMUNITYFUNDCREATEPAYMENTREQUESTDIALOG_H

#include <QDialog>
#include "../qvalidatedplaintextedit.h"
#include "uint256.h"

namespace Ui {
class CommunityFundCreatePaymentRequestDialog;
}

class CommunityFundCreatePaymentRequestDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CommunityFundCreatePaymentRequestDialog(QWidget *parent = 0);
    ~CommunityFundCreatePaymentRequestDialog();

private:
    Ui::CommunityFundCreatePaymentRequestDialog *ui;
    bool validate();
    bool isActiveProposal(uint256 hash);

public Q_SLOTS:
    void click_pushButtonSubmitPaymentRequest();
};

#endif // COMMUNITYFUNDCREATEPAYMENTREQUESTDIALOG_H
