#include "communityfunddisplaydetailed.h"
#include "ui_communityfunddisplaydetailed.h"

CommunityFundDisplayDetailed::CommunityFundDisplayDetailed(QWidget *parent, CFund::CProposal proposal) :
    QDialog(parent),
    ui(new Ui::CommunityFundDisplayDetailed),
    proposal(proposal)
{
    ui->setupUi(this);
    setProposalLabels();
}

void CommunityFundDisplayDetailed::setProposalLabels() const
{
    ui->labelProposalTitle->setText(QString::fromStdString(proposal.strDZeel));
    ui->labelFee->setText(QString::fromStdString(std::string(std::to_string(proposal.nFee) + " NAV")));
    ui->labelAddress->setText(QString::fromStdString(proposal.Address));
    ui->labelDeadline->setText(QString::fromStdString(std::to_string(proposal.nDeadline)));

}

CommunityFundDisplayDetailed::~CommunityFundDisplayDetailed()
{
    delete ui;
}
