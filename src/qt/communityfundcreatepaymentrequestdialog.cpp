#include "communityfundcreatepaymentrequestdialog.h"
#include "ui_communityfundcreatepaymentrequestdialog.h"

CommunityFundCreatePaymentRequestDialog::CommunityFundCreatePaymentRequestDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CommunityFundCreatePaymentRequestDialog)
{
    ui->setupUi(this);
}

CommunityFundCreatePaymentRequestDialog::~CommunityFundCreatePaymentRequestDialog()
{
    delete ui;
}
