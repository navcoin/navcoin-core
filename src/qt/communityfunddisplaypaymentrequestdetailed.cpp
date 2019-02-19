#include "communityfunddisplaypaymentrequestdetailed.h"
#include "ui_communityfunddisplaypaymentrequestdetailed.h"
#include "main.h"
#include <iomanip>
#include <sstream>

CommunityFundDisplayPaymentRequestDetailed::CommunityFundDisplayPaymentRequestDetailed(QWidget *parent, CFund::CPaymentRequest prequest) :
    QDialog(parent),
    ui(new Ui::CommunityFundDisplayPaymentRequestDetailed),
    prequest(prequest)
{
    ui->setupUi(this);

    //connect ui elements to functions
    connect(ui->buttonBoxYesNoVote_2, SIGNAL(clicked( QAbstractButton*)), this, SLOT(on_click_buttonBoxYesNoVote(QAbstractButton*)));
    connect(ui->pushButtonClose_2, SIGNAL(clicked()), this, SLOT(reject()));

    //update labels
    setPrequestLabels();

    // Shade in yes/no buttons is user has voted
    // If the prequest is pending and not prematurely expired (ie can be voted on):
    if (prequest.fState == CFund::NIL && prequest.GetState().find("expired") == string::npos) {
        // Get prequest votes list
        auto it = std::find_if( vAddedPaymentRequestVotes.begin(), vAddedPaymentRequestVotes.end(),
                                [&prequest](const std::pair<std::string, bool>& element){ return element.first == prequest.hash.ToString();} );
        if (it != vAddedPaymentRequestVotes.end()) {
            if (it->second) {
                // Payment Request was voted yes, shade in yes button and unshade no button
                ui->buttonBoxYesNoVote_2->button(QDialogButtonBox::Yes)->setStyleSheet("background-color: #35db03;");
                ui->buttonBoxYesNoVote_2->button(QDialogButtonBox::No)->setStyleSheet("background-color: #F3F4F6;");
            }
            else {
                // Payment Request was noted no, shade in no button and unshade yes button
                ui->buttonBoxYesNoVote_2->button(QDialogButtonBox::Yes)->setStyleSheet("background-color: #F3F4F6;");
                ui->buttonBoxYesNoVote_2->button(QDialogButtonBox::No)->setStyleSheet("background-color: #de1300;");
            }
        }
        else {
            // Payment Request was not voted on, reset shades of both buttons
            ui->buttonBoxYesNoVote_2->button(QDialogButtonBox::Yes)->setStyleSheet("background-color: #F3F4F6;");
            ui->buttonBoxYesNoVote_2->button(QDialogButtonBox::No)->setStyleSheet("background-color: #F3F4F6;");

        }
    }

    //hide ui voting elements on prequests which are not allowed vote states
    if(!prequest.CanVote())
    {
        ui->buttonBoxYesNoVote_2->setStandardButtons(QDialogButtonBox::NoButton);
    }

    // Hide ability to vote is the status is expired
    std::string status = ui->labelPrequestStatus->text().toStdString();
    if (status.find("expired") != string::npos) {
        ui->buttonBoxYesNoVote_2->setStandardButtons(QDialogButtonBox::NoButton);
    }
}

void CommunityFundDisplayPaymentRequestDetailed::setPrequestLabels() const
{
    // Title
    ui->labelPaymentRequestTitle->setText(QString::fromStdString(prequest.strDZeel));

    // Amount
    stringstream a;
    a.imbue(std::locale(""));
    a << fixed << setprecision(8) << prequest.nAmount/100000000.0;
    string amount = a.str();
    amount.erase(amount.find_last_not_of("0") + 1, std::string::npos );
    if(amount.at(amount.length()-1) == '.') {
        amount = amount.substr(0, amount.size()-1);
    }
    amount.append(" NAV");
    ui->labelPrequestAmount->setText(QString::fromStdString(amount));

    // Status
    ui->labelPrequestStatus->setText(QString::fromStdString(std::to_string(prequest.fState)));

    // Yes Votes
    ui->labelPrequestYes->setText(QString::fromStdString(std::to_string(prequest.nVotesYes)));

    // No Votes
    ui->labelPrequestNo->setText(QString::fromStdString(std::to_string(prequest.nVotesNo)));

    // Payment Request Hash
    ui->labelPrequestHash->setText(QString::fromStdString(prequest.hash.ToString()));

    // Transaction Block Hash
    ui->labelPrequestTransactionBlockHash->setText(QString::fromStdString(prequest.blockhash.ToString()));

    // Transaction Hash
    ui->labelPrequestTransactionHash->setText(QString::fromStdString(prequest.txblockhash.ToString()));

    // Version Number
    ui->labelPrequestVersionNo->setText(QString::fromStdString(std::to_string(prequest.nVersion)));

    // Voting Cycle
    ui->labelPrequestVotingCycle->setText(QString::fromStdString(std::to_string(prequest.nVotingCycle)));

    // Proposal Hash
    ui->labelPrequestProposalHash->setText(QString::fromStdString(prequest.proposalhash.ToString()));

    // Payment Hash
    ui->labelPrequestPaymentHash->setText(QString::fromStdString(prequest.paymenthash.ToString()));

    // Link
    ui->labelPrequestLink->setText(QString::fromStdString("https://www.navexplorer.com/community-fund/payment-request/" + prequest.hash.ToString()));

    /*
    uint64_t deadline_d = std::floor(prequest.nDeadline/86400);
    uint64_t deadline_h = std::floor((proposal.nDeadline-deadline_d*86400)/3600);
    uint64_t deadline_m = std::floor((proposal.nDeadline-(deadline_d*86400 + deadline_h*3600))/60);
    std::string s_deadline = std::to_string(deadline_d) + std::string(" Days ") + std::to_string(deadline_h) + std::string(" Hours ") + std::to_string(deadline_m) + std::string(" Minutes");

    uint64_t proptime = 0;
    if (mapBlockIndex.count(proposal.blockhash) > 0) {
        proptime = mapBlockIndex[proposal.blockhash]->GetBlockTime();
    }

    ui->labelDeadline->setText(QString::fromStdString(s_deadline));
    if (proposal.fState == CFund::NIL) {
        std::string expiry = std::to_string(7 - proposal.nVotingCycle) +  " voting cycles";
        ui->labelExpiresIn->setText(QString::fromStdString(expiry));
    }
    if (proposal.fState == CFund::ACCEPTED) {
        uint64_t deadline = proptime + proposal.nDeadline - pindexBestHeader->GetBlockTime();

        uint64_t deadline_d = std::floor(deadline/86400);
        uint64_t deadline_h = std::floor((deadline-deadline_d*86400)/3600);
        uint64_t deadline_m = std::floor((deadline-(deadline_d*86400 + deadline_h*3600))/60);

        std::string s_deadline = "";
        if(deadline_d >= 14)
        {
            s_deadline = std::to_string(deadline_d) + std::string(" Days");
        }
        else
        {
            s_deadline = std::to_string(deadline_d) + std::string(" Days ") + std::to_string(deadline_h) + std::string(" Hours ") + std::to_string(deadline_m) + std::string(" Minutes");
        }
        ui->labelExpiresIn->setText(QString::fromStdString(s_deadline));
    }
    if (proposal.fState == CFund::REJECTED) {
        std::string expiry_title = "Rejected on: ";
        std::time_t t = static_cast<time_t>(proptime);
        std::stringstream ss;
        ss << std::put_time(std::gmtime(&t), "%c %Z");
        ui->labelExpiresInTitle->setText(QString::fromStdString(expiry_title));
        ui->labelExpiresIn->setText(QString::fromStdString(ss.str().erase(10, 9)));
    }
    if (proposal.fState == CFund::EXPIRED || proposal.GetState(pindexBestHeader->GetBlockTime()).find("expired") != string::npos) {
        if (proposal.fState == CFund::EXPIRED) {
            std::string expiry_title = "Expired on: ";
            std::time_t t = static_cast<time_t>(proptime);
            std::stringstream ss;
            ss << std::put_time(std::gmtime(&t), "%c %Z");
            ui->labelExpiresInTitle->setText(QString::fromStdString(expiry_title));
            ui->labelExpiresIn->setText(QString::fromStdString(ss.str().erase(10, 9)));
        }
        else {
            std::string expiry_title = "Expires: ";
            std::string expiry = "At end of voting period";
            ui->labelExpiresInTitle->setText(QString::fromStdString(expiry_title));
            ui->labelExpiresIn->setText(QString::fromStdString(expiry));
        }
    }
    ui->labelStatus->setText(QString::fromStdString(proposal.GetState(pindexBestHeader->GetBlockTime())));
    ui->labelNumberOfYesVotes->setText(QString::fromStdString(std::to_string(proposal.nVotesYes)));
    ui->labelNumberOfNoVotes->setText(QString::fromStdString(std::to_string(proposal.nVotesNo)));

    ui->labelTransactionBlockHash->setText(QString::fromStdString(proposal.blockhash.ToString()));
    ui->labelTransactionHash->setText(QString::fromStdString(proposal.txblockhash.ToString()));
    ui->labelVersionNumber->setText(QString::fromStdString(std::to_string(proposal.nVersion)));
    ui->labelVotingCycleNumber->setText(QString::fromStdString(std::to_string(proposal.nVotingCycle)));
    ui->labelLinkToProposal->setText(QString::fromStdString("https://navcommunity.net/view-proposal/" + proposal.hash.ToString()));
    ui->labelProposalHash->setText(QString::fromStdString(proposal.hash.ToString()));


    //set hyperlink for navcommunity proposal view
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
    */
}

void CommunityFundDisplayPaymentRequestDetailed::on_click_buttonBoxYesNoVote(QAbstractButton *button)
{
    //cast the vote
    bool duplicate = false;

    if (ui->buttonBoxYesNoVote_2->buttonRole(button) == QDialogButtonBox::YesRole)
    {
        ui->buttonBoxYesNoVote_2->button(QDialogButtonBox::Yes)->setStyleSheet("background-color: #35db03;");
        ui->buttonBoxYesNoVote_2->button(QDialogButtonBox::No)->setStyleSheet("background-color: #F3F4F6;");
        CFund::VotePaymentRequest(prequest.hash.ToString(), true, duplicate);
    }
    else if(ui->buttonBoxYesNoVote_2->buttonRole(button) == QDialogButtonBox::NoRole)
    {
        ui->buttonBoxYesNoVote_2->button(QDialogButtonBox::Yes)->setStyleSheet("background-color: #F3F4F6;");
        ui->buttonBoxYesNoVote_2->button(QDialogButtonBox::No)->setStyleSheet("background-color: #de1300;");
        CFund::VotePaymentRequest(prequest.hash.ToString(), false, duplicate);
    }
    else if(ui->buttonBoxYesNoVote_2->buttonRole(button) == QDialogButtonBox::RejectRole)
    {
        ui->buttonBoxYesNoVote_2->button(QDialogButtonBox::Yes)->setStyleSheet("background-color: #F3F4F6;");
        ui->buttonBoxYesNoVote_2->button(QDialogButtonBox::No)->setStyleSheet("background-color: #F3F4F6;");
        CFund::RemoveVotePaymentRequest(prequest.hash.ToString());
    }
    else {
        return;
    }
}

CommunityFundDisplayPaymentRequestDetailed::~CommunityFundDisplayPaymentRequestDetailed()
{
    delete ui;
}
