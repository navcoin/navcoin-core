#include <qt/communityfunddisplaydetailed.h>
#include <ui_communityfunddisplaydetailed.h>
#include <qt/communityfundpage.h>

#include <main.h>
#include <iomanip>
#include <sstream>
#include <wallet/wallet.h>
#include <base58.h>

CommunityFundDisplayDetailed::CommunityFundDisplayDetailed(QWidget *parent, CProposal proposal) :
    QDialog(parent),
    ui(new Ui::CommunityFundDisplayDetailed),
    proposal(proposal)
{
    ui->setupUi(this);
    ui->detailsWidget->setVisible(false);
    adjustSize();

    LOCK(cs_main);
    CStateViewCache view(pcoinsTip);

    //connect ui elements to functions
    connect(ui->buttonBoxYesNoVote, SIGNAL(clicked( QAbstractButton*)), this, SLOT(click_buttonBoxYesNoVote(QAbstractButton*)));
    connect(ui->pushButtonClose, SIGNAL(clicked()), this, SLOT(reject()));
    connect(ui->detailsBtn, SIGNAL(clicked()), this, SLOT(onDetails()));

    ui->buttonBoxYesNoVote->setStandardButtons(QDialogButtonBox::No|QDialogButtonBox::Yes|QDialogButtonBox::Ignore|QDialogButtonBox::Cancel);
    ui->buttonBoxYesNoVote->button(QDialogButtonBox::Ignore)->setText(tr("Abstain"));

    //update labels
    setProposalLabels();

    // Shade in yes/no buttons is user has voted
    // If the proposal is pending and not prematurely expired (ie can be voted on):
    if (proposal.GetLastState() == DAOFlags::NIL && proposal.GetState(pindexBestHeader->GetBlockTime(), view).find("expired") == string::npos) {
        // Get proposal votes list
        auto it = mapAddedVotes.find(proposal.hash);

        if (it != mapAddedVotes.end())
        {
            if (it->second == 1) {
                // Proposal was voted yes, shade in yes button and unshade no button
                ui->buttonBoxYesNoVote->setStandardButtons(QDialogButtonBox::No|QDialogButtonBox::Yes|QDialogButtonBox::Ignore|QDialogButtonBox::Cancel);
                ui->buttonBoxYesNoVote->button(QDialogButtonBox::Yes)->setStyleSheet(COLOR_VOTE_YES);
                ui->buttonBoxYesNoVote->button(QDialogButtonBox::No)->setStyleSheet(COLOR_VOTE_NEUTRAL);
                ui->buttonBoxYesNoVote->button(QDialogButtonBox::Ignore)->setStyleSheet(COLOR_VOTE_NEUTRAL);
            }
            else if (it->second == 0) {
                // Proposal was noted no, shade in no button and unshade yes button
                ui->buttonBoxYesNoVote->setStandardButtons(QDialogButtonBox::No|QDialogButtonBox::Yes|QDialogButtonBox::Ignore|QDialogButtonBox::Cancel);
                ui->buttonBoxYesNoVote->button(QDialogButtonBox::Yes)->setStyleSheet(COLOR_VOTE_NEUTRAL);
                ui->buttonBoxYesNoVote->button(QDialogButtonBox::No)->setStyleSheet(COLOR_VOTE_NO);
                ui->buttonBoxYesNoVote->button(QDialogButtonBox::Ignore)->setStyleSheet(COLOR_VOTE_NEUTRAL);
            }
            else if (it->second == -1) {
                // Proposal was noted abstain, shade in no button and unshade yes button
                ui->buttonBoxYesNoVote->setStandardButtons(QDialogButtonBox::No|QDialogButtonBox::Yes|QDialogButtonBox::Ignore|QDialogButtonBox::Cancel);
                ui->buttonBoxYesNoVote->button(QDialogButtonBox::Yes)->setStyleSheet(COLOR_VOTE_NEUTRAL);
                ui->buttonBoxYesNoVote->button(QDialogButtonBox::No)->setStyleSheet(COLOR_VOTE_NEUTRAL);
                ui->buttonBoxYesNoVote->button(QDialogButtonBox::Ignore)->setStyleSheet(COLOR_VOTE_ABSTAIN);
            }
        }
        else {
            // Proposal was not voted on, reset shades of both buttons
            ui->buttonBoxYesNoVote->setStandardButtons(QDialogButtonBox::No|QDialogButtonBox::Yes|QDialogButtonBox::Ignore);
            ui->buttonBoxYesNoVote->button(QDialogButtonBox::Yes)->setStyleSheet(COLOR_VOTE_NEUTRAL);
            ui->buttonBoxYesNoVote->button(QDialogButtonBox::No)->setStyleSheet(COLOR_VOTE_NEUTRAL);
            ui->buttonBoxYesNoVote->button(QDialogButtonBox::Ignore)->setStyleSheet(COLOR_VOTE_NEUTRAL);
        }
    }

    ui->buttonBoxYesNoVote->button(QDialogButtonBox::Ignore)->setText(tr("Abstain"));

    //hide ui voting elements on proposals which are not allowed vote states
    if(!proposal.CanVote(view))
    {
        ui->buttonBoxYesNoVote->setStandardButtons(QDialogButtonBox::NoButton);
    }

    // Hide ability to vote is the status is expired
    std::string status = ui->labelStatus->text().toStdString();
    if (status.find("expired") != string::npos) {
        ui->buttonBoxYesNoVote->setStandardButtons(QDialogButtonBox::NoButton);
    }
}

void CommunityFundDisplayDetailed::onDetails()
{
    ui->detailsWidget->setVisible(!ui->detailsWidget->isVisible());
    adjustSize();
}

void CommunityFundDisplayDetailed::setProposalLabels()
{
    CStateViewCache view(pcoinsTip);

    ui->labelProposalTitle->setText(QString::fromStdString(proposal.strDZeel));
    ui->labelAddress->setText(QString::fromStdString(proposal.GetPaymentAddress()));

    uint64_t deadline_d = std::floor(proposal.nDeadline/86400);
    uint64_t deadline_h = std::floor((proposal.nDeadline-deadline_d*86400)/3600);
    uint64_t deadline_m = std::floor((proposal.nDeadline-(deadline_d*86400 + deadline_h*3600))/60);
    ui->labelDeadline->setText(tr("%n Days", "", deadline_d) + tr(" %n Hours", "", deadline_h) + tr(" %n Minutes", "", deadline_m));

    uint64_t proptime = 0;
    CBlockIndex *pblockindex = proposal.GetLastStateBlockIndex();
    if (pblockindex) {
        proptime = pblockindex->GetBlockTime();
    }

    auto fLastState = proposal.GetLastState();

    if (fLastState == DAOFlags::NIL) {
        std::string expiry_title = "Voting period finishes in: ";
        ui->labelExpiresInTitle->setText(QString::fromStdString(expiry_title));
        std::string expiry = std::to_string(GetConsensusParameter(Consensus::CONSENSUS_PARAM_PROPOSAL_MAX_VOTING_CYCLES, view) - proposal.nVotingCycle) +  " voting cycles";
        ui->labelExpiresIn->setText(QString::fromStdString(expiry));
    }
    if (fLastState == DAOFlags::ACCEPTED) {
        uint64_t deadline = proptime + proposal.nDeadline - pindexBestHeader->GetBlockTime();

        uint64_t deadline_d = std::floor(deadline/86400);
        uint64_t deadline_h = std::floor((deadline-deadline_d*86400)/3600);
        uint64_t deadline_m = std::floor((deadline-(deadline_d*86400 + deadline_h*3600))/60);

        if(deadline_d >= 14)
            ui->labelExpiresIn->setText(tr("%n Days", "", deadline_d));
        else
            ui->labelExpiresIn->setText(tr("%n Days", "", deadline_d) + tr(" %n Hours", "", deadline_h) + tr(" %n Minutes", "", deadline_m));
    }

    if (fLastState == DAOFlags::REJECTED) {
        std::string expiry_title = "Rejected on: ";
        std::time_t t = static_cast<time_t>(proptime);
        std::stringstream ss;
        char buf[48];
        if (strftime(buf, sizeof(buf), "%c %Z", std::gmtime(&t)))
            ss << buf;
        ui->labelExpiresInTitle->setText(QString::fromStdString(expiry_title));
        ui->labelExpiresIn->setText(QString::fromStdString(ss.str().erase(10, 9)));
    }
    if (fLastState== DAOFlags::EXPIRED || proposal.GetState(pindexBestHeader->GetBlockTime(), view).find("expired") != string::npos) {
        if (fLastState == DAOFlags::EXPIRED) {
            std::string expiry_title = "Expired on: ";
            std::time_t t = static_cast<time_t>(proptime);
            std::stringstream ss;
            ss << std::put_time(std::gmtime(&t), "%c %Z");
            ui->labelExpiresInTitle->setText(tr("Expired on: "));
            ui->labelExpiresIn->setText(QString::fromStdString(ss.str()));
        }
        else {
            ui->labelExpiresInTitle->setText(tr("Expires: "));
            ui->labelExpiresIn->setText(tr("At end of voting period"));
        }
    }
    ui->labelStatus->setText(QString::fromStdString(proposal.GetState(pindexBestHeader->GetBlockTime(), view)));
    ui->labelNumberOfYesVotes->setText(QString::fromStdString(std::to_string(proposal.nVotesYes)));
    ui->labelNumberOfNoVotes->setText(QString::fromStdString(std::to_string(proposal.nVotesNo)));
    ui->labelTransactionBlockHash->setText(QString::fromStdString(pblockindex ? pblockindex->GetBlockHash().ToString() : ""));
    ui->labelTransactionHash->setText(QString::fromStdString(proposal.txblockhash.ToString()));
    ui->labelVersionNumber->setText(QString::fromStdString(std::to_string(proposal.nVersion)));
    ui->labelVotingCycleNumber->setText(QString::fromStdString(std::to_string(proposal.nVotingCycle)));
    ui->labelLinkToProposal->setText(QString::fromStdString("https://www.navexplorer.com/community-fund/proposal/" + proposal.hash.ToString()));
    ui->labelProposalHash->setText(QString::fromStdString(proposal.hash.ToString()));


    //set hyperlink for navcommunity proposal view
    ui->labelLinkToProposal->setText("<a href=\"" + ui->labelLinkToProposal->text() + "\">" + ui->labelLinkToProposal->text() + "</a>");
    ui->labelLinkToProposal->setTextInteractionFlags(Qt::TextBrowserInteraction);
    ui->labelLinkToProposal->setOpenExternalLinks(true);

    string amount;
    amount = wallet->formatDisplayAmount(proposal.nAmount);
    ui->labelAmount->setText(QString::fromStdString(amount));

    string fee;
    fee = wallet->formatDisplayAmount(proposal.nFee);
    ui->labelFee->setText(QString::fromStdString(fee));

    // If a proposal is expired pending voting of payment requests, change the expiry label text
    std::string status = ui->labelStatus->text().toStdString();
    if(status.find("expired pending voting of payment requests") != string::npos) {
        ui->labelExpiresIn->setText(QString::fromStdString("After payment request voting"));
    }

    // If proposal is pending, hide the transaction hash
    if (fLastState == DAOFlags::NIL) {
        ui->labelTransactionBlockHashTitle->setVisible(false);
        ui->labelTransactionBlockHash->setVisible(false);
    }

    ui->labelProposalTitle->setText(QString::fromStdString(proposal.strDZeel.c_str()));

    // Hide expiry label is proposal is accepted and waiting for coins
    if(status.find("accepted waiting for enough coins in fund") != string::npos) {
        ui->labelExpiresIn->setVisible(false);
        ui->labelExpiresInTitle->setVisible(false);
    }
    adjustSize();
}

void CommunityFundDisplayDetailed::click_buttonBoxYesNoVote(QAbstractButton *button)
{
    // Make sure we have a lock
    LOCK(cs_main);

    //cast the vote
    bool duplicate = false;

    CProposal p;
    if (!pcoinsTip->GetProposal(uint256S(proposal.hash.ToString()), p))
    {
        return;
    }

    if (ui->buttonBoxYesNoVote->buttonRole(button) == QDialogButtonBox::YesRole)
    {
        ui->buttonBoxYesNoVote->setStandardButtons(QDialogButtonBox::No|QDialogButtonBox::Yes|QDialogButtonBox::Ignore|QDialogButtonBox::Cancel);
        ui->buttonBoxYesNoVote->button(QDialogButtonBox::Yes)->setStyleSheet(COLOR_VOTE_YES);
        ui->buttonBoxYesNoVote->button(QDialogButtonBox::No)->setStyleSheet(COLOR_VOTE_NEUTRAL);
        ui->buttonBoxYesNoVote->button(QDialogButtonBox::Ignore)->setStyleSheet(COLOR_VOTE_NEUTRAL);
        Vote(p.hash, 1, duplicate);
        close();
    }
    else if(ui->buttonBoxYesNoVote->buttonRole(button) == QDialogButtonBox::NoRole)
    {
        ui->buttonBoxYesNoVote->setStandardButtons(QDialogButtonBox::No|QDialogButtonBox::Yes|QDialogButtonBox::Ignore|QDialogButtonBox::Cancel);
        ui->buttonBoxYesNoVote->button(QDialogButtonBox::Yes)->setStyleSheet(COLOR_VOTE_NEUTRAL);
        ui->buttonBoxYesNoVote->button(QDialogButtonBox::No)->setStyleSheet(COLOR_VOTE_NO);
        ui->buttonBoxYesNoVote->button(QDialogButtonBox::Ignore)->setStyleSheet(COLOR_VOTE_NEUTRAL);
        Vote(p.hash, 0, duplicate);
        close();
    }
    else if(ui->buttonBoxYesNoVote->standardButton(button) == QDialogButtonBox::Ignore)
    {
        ui->buttonBoxYesNoVote->setStandardButtons(QDialogButtonBox::No|QDialogButtonBox::Yes|QDialogButtonBox::Ignore|QDialogButtonBox::Cancel);
        ui->buttonBoxYesNoVote->button(QDialogButtonBox::Yes)->setStyleSheet(COLOR_VOTE_NEUTRAL);
        ui->buttonBoxYesNoVote->button(QDialogButtonBox::No)->setStyleSheet(COLOR_VOTE_NEUTRAL);
        ui->buttonBoxYesNoVote->button(QDialogButtonBox::Ignore)->setStyleSheet(COLOR_VOTE_ABSTAIN);
        Vote(p.hash, -1, duplicate);
        close();
    }
    else if(ui->buttonBoxYesNoVote->buttonRole(button) == QDialogButtonBox::RejectRole)
    {
        ui->buttonBoxYesNoVote->setStandardButtons(QDialogButtonBox::No|QDialogButtonBox::Yes|QDialogButtonBox::Ignore);
        ui->buttonBoxYesNoVote->button(QDialogButtonBox::Yes)->setStyleSheet(COLOR_VOTE_NEUTRAL);
        ui->buttonBoxYesNoVote->button(QDialogButtonBox::No)->setStyleSheet(COLOR_VOTE_NEUTRAL);
        ui->buttonBoxYesNoVote->button(QDialogButtonBox::Ignore)->setStyleSheet(COLOR_VOTE_NEUTRAL);
        RemoveVote(p.hash);
        close();
    }
    else {
        return;
    }
}

CommunityFundDisplayDetailed::~CommunityFundDisplayDetailed()
{
    delete ui;
}
