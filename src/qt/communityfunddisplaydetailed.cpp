#include "communityfunddisplaydetailed.h"
#include "ui_communityfunddisplaydetailed.h"
#include "main.h"
#include <iomanip>
#include <sstream>

CommunityFundDisplayDetailed::CommunityFundDisplayDetailed(QWidget *parent, CFund::CProposal proposal) :
    QDialog(parent),
    ui(new Ui::CommunityFundDisplayDetailed),
    proposal(proposal)
{
    ui->setupUi(this);

    //connect ui elements to functions
    connect(ui->pushButtonClose, SIGNAL(clicked()), this, SLOT(reject()));

    //update labels
    setProposalLabels();

    //hide ui voting elements on proposals which are not allowed vote states
    if(!proposal.CanVote())
    {
        ui->buttonBoxYesNoVote->setStandardButtons(QDialogButtonBox::NoButton);
    }
}

void CommunityFundDisplayDetailed::setProposalLabels() const
{
    ui->labelProposalTitle->setText(QString::fromStdString(proposal.strDZeel));
    ui->labelAddress->setText(QString::fromStdString(proposal.Address));
    ui->labelDeadline->setText(QString::fromStdString(std::to_string(proposal.nDeadline)));
    //ui->labelExpiresIn
    ui->labelStatus->setText(QString::fromStdString(proposal.GetState(pindexBestHeader->GetBlockTime())));
    ui->labelNumberOfYesVotes->setText(QString::fromStdString(std::to_string(proposal.nVotesYes)));
    ui->labelNumberOfNoVotes->setText(QString::fromStdString(std::to_string(proposal.nVotesNo)));
    //ui->labelTransactionBlockHash->setText(QString::fromStdString(std::string(proposal.blockhash)));
    //ui->labelVersionNumber-setText(QString::fromStdString(std::to_string(proposal.nVersion)));
    //ui->labelVotingCycleNumber->setText(QString::fromStdString(std::to_string(proposal.nVersion)));
    //ui->labelLinkToProposal->setText(QString::fromStdString("https://navcommunity.net/view-proposal/" + std::to_string(proposal.blockhash)));

    //set hyperlink for
    ui->labelLinkToProposal->setTextInteractionFlags(Qt::TextBrowserInteraction);
    ui->labelLinkToProposal->setOpenExternalLinks(true);
    //ui->labelLinkToProposal->setHtml(ui->labelLinkToProposal->text());

    stringstream a;
    a.imbue(std::locale(""));
    a << fixed << setprecision(8) << proposal.nAmount/100000000.0;
    string amount = a.str();
    amount.erase(amount.find_last_not_of("0") + 1, std::string::npos );
    if(amount.at(amount.length()-1) == '.') {
        amount = amount.substr(0, amount.size()-1);
    }
    amount.append(" NAV");
    ui->labelAmount->setText(QString::fromStdString(amount));

    stringstream f;
    f.imbue(std::locale(""));
    f << fixed << setprecision(8) << proposal.nFee/100000000.0;
    string fee = f.str();
    fee.erase(fee.find_last_not_of("0") + 1, std::string::npos );
    if(fee.at(fee.length()-1) == '.') {
        fee = fee.substr(0, fee.size()-1);
    }
    fee.append(" NAV");
    ui->labelFee->setText(QString::fromStdString(fee));
}

CommunityFundDisplayDetailed::~CommunityFundDisplayDetailed()
{
    delete ui;
}
