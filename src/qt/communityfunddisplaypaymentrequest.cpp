#include "communityfunddisplaypaymentrequest.h"
#include "ui_communityfunddisplaypaymentrequest.h"

CommunityFundDisplayPaymentRequest::CommunityFundDisplayPaymentRequest(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::CommunityFundDisplayPaymentRequest)
{
    ui->setupUi(this);
}

CommunityFundDisplayPaymentRequest::~CommunityFundDisplayPaymentRequest()
{
    delete ui;
}
