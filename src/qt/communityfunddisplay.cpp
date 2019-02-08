#include "communityfunddisplay.h"
#include "ui_communityfunddisplay.h"

CommunityFundDisplay::CommunityFundDisplay(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::CommunityFundDisplay)
{
    ui->setupUi(this);
}

CommunityFundDisplay::~CommunityFundDisplay()
{
    delete ui;
}
