#include "communityfunddisplay.h"
#include "ui_communityfunddisplay.h"

CommunityFundDisplay::CommunityFundDisplay(QWidget *parent, const CFund::CProposal proposal) :
    QWidget(parent),
    ui(new Ui::CommunityFundDisplay),
    proposal(proposal)
{
    ui->setupUi(this);
}

CommunityFundDisplay::~CommunityFundDisplay()
{
    delete ui;
}
