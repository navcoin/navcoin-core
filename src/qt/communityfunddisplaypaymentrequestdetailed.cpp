#include "communityfunddisplaypaymentrequestdetailed.h"
#include "ui_communityfunddisplaypaymentrequestdetailed.h"

CommunityFundDisplayPaymentRequestDetailed::CommunityFundDisplayPaymentRequestDetailed(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::CommunityFundDisplayPaymentRequestDetailed)
{
    ui->setupUi(this);
}

CommunityFundDisplayPaymentRequestDetailed::~CommunityFundDisplayPaymentRequestDetailed()
{
    delete ui;
}
