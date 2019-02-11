#include "communityfunddisplay.h"
#include "ui_communityfunddisplay.h"
#include "main.h"

CommunityFundDisplay::CommunityFundDisplay(QWidget *parent, CFund::CProposal proposal) :
    QWidget(parent),
    ui(new Ui::CommunityFundDisplay),
    proposal(proposal)
{
    ui->setupUi(this);

    //set labels from community fund
    ui->title->setText(QString::fromStdString(proposal.strDZeel));
    ui->labelStatus->setText(QString::fromStdString(proposal.GetState(pindexBestHeader->GetBlockTime())));
    ui->labelRequested->setNum(int(proposal.nAmount));
    ui->labelDuration->setNum(int (proposal.nDeadline));

}

CommunityFundDisplay::~CommunityFundDisplay()
{
    delete ui;
}
