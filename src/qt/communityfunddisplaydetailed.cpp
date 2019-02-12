#include "communityfunddisplaydetailed.h"
#include "ui_communityfunddisplaydetailed.h"

CommunityFundDisplayDetailed::CommunityFundDisplayDetailed(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::CommunityFundDisplayDetailed)
{
    ui->setupUi(this);
}

CommunityFundDisplayDetailed::~CommunityFundDisplayDetailed()
{
    delete ui;
}
