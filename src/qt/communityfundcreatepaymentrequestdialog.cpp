#include "communityfundcreatepaymentrequestdialog.h"
#include "ui_communityfundcreatepaymentrequestdialog.h"

CommunityFundCreatePaymentRequestDialog::CommunityFundCreatePaymentRequestDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CommunityFundCreatePaymentRequestDialog)
{
    ui->setupUi(this);

    //connect
    connect(ui->pushButtonClose, SIGNAL(clicked()), this, SLOT(reject()));
}

CommunityFundCreatePaymentRequestDialog::~CommunityFundCreatePaymentRequestDialog()
{
    delete ui;
}
