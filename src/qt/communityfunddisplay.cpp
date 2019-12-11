#include <qt/communityfunddisplay.h>
#include <ui_communityfunddisplay.h>

#include <QtWidgets/QDialogButtonBox>
#include <main.h>
#include <qt/../txdb.h>
#include <iomanip>
#include <sstream>
#include <ctime>

#include <wallet/wallet.h>
#include <base58.h>
#include <consensus/cfund.h>
#include <chain.h>
#include <qt/guiutil.h>
#include <ui_interface.h>

#include <qt/communityfunddisplaydetailed.h>
#include <qt/communityfundpage.h>

CommunityFundDisplay::CommunityFundDisplay(QWidget *parent, CFund::CProposal proposal) :
    QWidget(parent),
    ui(new Ui::CommunityFundDisplay),
    proposal(proposal)
{
    ui->setupUi(this);

    QFont f_title("Sans Serif", 10.5, QFont::Bold);
    QFont f_label_title("Sans Serif", 10, QFont::Bold);
    QFont f_label("Sans Serif", 10, QFont::Normal);

    ui->title->setFont(f_title);
    ui->labelTitleDuration->setFont(f_label_title);
    ui->labelDuration->setFont(f_label);
    ui->labelTitleRequested->setFont(f_label_title);
    ui->labelRequested->setFont(f_label);
    ui->labelTitleStatus->setFont(f_label_title);
    ui->labelStatus->setFont(f_label);
    ui->pushButtonDetails->setFont(f_label);
    ui->buttonBoxVote->setFont(f_label);

    connect(ui->buttonBoxVote, SIGNAL(clicked( QAbstractButton*)), this, SLOT(click_buttonBoxVote(QAbstractButton*)));
    connect(ui->pushButtonDetails, SIGNAL(clicked()), this, SLOT(click_pushButtonDetails()));

    refresh();
}

void CommunityFundDisplay::refresh()
{
    // Update the dao new counter
    uiInterface.UpdateDaoNewCount();

    // Set labels from community fund
    ui->title->setText(QString::fromStdString(proposal.strDZeel));
    ui->labelStatus->setText(QString::fromStdString(proposal.GetState(pindexBestHeader->GetBlockTime())));

    string nav_amount;
    nav_amount = wallet->formatDisplayAmount(proposal.nAmount);
    ui->labelRequested->setText(QString::fromStdString(nav_amount));

    uint64_t proptime = 0;
    CBlockIndex* pblockindex = proposal.GetLastStateBlockIndex();
    if (pblockindex) {
        proptime = pblockindex->GetBlockTime();
    }

    uint64_t deadline = proptime + proposal.nDeadline - pindexBestHeader->GetBlockTime();
    uint64_t deadline_d = std::floor(deadline/86400);
    uint64_t deadline_h = std::floor((deadline-deadline_d*86400)/3600);
    uint64_t deadline_m = std::floor((deadline-(deadline_d*86400 + deadline_h*3600))/60);

    // Show appropriate amount of figures
    std::string s_deadline = "";
    if(deadline_d >= 14)
        s_deadline = std::to_string(deadline_d) + std::string(" Days");
    else
        s_deadline = std::to_string(deadline_d) + std::string(" Days ") + std::to_string(deadline_h) + std::string(" Hours ") + std::to_string(deadline_m) + std::string(" Minutes");

    ui->labelDuration->setText(QString::fromStdString(s_deadline));

    // Hide ability to vote is the status is expired
    std::string status = ui->labelStatus->text().toStdString();
    if (status.find("expired") != string::npos) {
        ui->buttonBoxVote->setStandardButtons(QDialogButtonBox::NoButton);
    }

    flags fLastState = proposal.GetLastState();

    // If proposal is pending show voting cycles left
    if (fLastState == CFund::NIL)
    {
        std::string duration_title = "Voting Cycle: ";
        std::string duration = std::to_string(proposal.nVotingCycle) +  " of " + std::to_string(Params().GetConsensus().nCyclesProposalVoting);
        ui->labelTitleDuration->setText(QString::fromStdString(duration_title));
        ui->labelDuration->setText(QString::fromStdString(duration));
    }

    // If proposal is rejected, show when it was rejected
    if (fLastState == CFund::REJECTED)
    {
        std::string expiry_title = "Rejected on: ";
        std::time_t t = static_cast<time_t>(proptime);
        std::stringstream ss;
        char buf[48];
        if (strftime(buf, sizeof(buf), "%c %Z", std::gmtime(&t)))
	    ss << buf;
        ui->labelTitleDuration->setText(QString::fromStdString(expiry_title));
        ui->labelDuration->setText(QString::fromStdString(ss.str().erase(10, 9)));
    }

    // If expired show when it expired
    if (fLastState == CFund::EXPIRED || status.find("expired") != string::npos)
    {
        if (fLastState == CFund::EXPIRED)
        {
            std::string expiry_title = "Expired on: ";
            std::time_t t = static_cast<time_t>(proptime);
            std::stringstream ss;
            char buf[48];
            if (strftime(buf, sizeof(buf), "%c %Z", std::gmtime(&t)))
                ss << buf;
            ui->labelTitleDuration->setText(QString::fromStdString(expiry_title));
            ui->labelDuration->setText(QString::fromStdString(ss.str().erase(10, 9)));
        }
        else
        {
            std::string expiry_title = "Expires: ";
            std::string expiry = "At end of voting period";
            ui->labelTitleDuration->setText(QString::fromStdString(expiry_title));
            ui->labelDuration->setText(QString::fromStdString(expiry));
        }
    }

    // Shade in yes/no buttons is user has voted
    // If the proposal is pending and not prematurely expired (ie can be voted on):
    if (fLastState == CFund::NIL && proposal.GetState(pindexBestHeader->GetBlockTime()).find("expired") == string::npos)
    {
        // Get proposal votes list
        CFund::CProposal prop = this->proposal;
        auto it = std::find_if( vAddedProposalVotes.begin(), vAddedProposalVotes.end(),
                                [&prop](const std::pair<std::string, bool>& element){ return element.first == prop.hash.ToString();} );
        if (it != vAddedProposalVotes.end())
        {
            if (it->second)
            {
                // Proposal was voted yes, shade in yes button and unshade no button
                ui->buttonBoxVote->setStandardButtons(QDialogButtonBox::No|QDialogButtonBox::Yes|QDialogButtonBox::Cancel);
                ui->buttonBoxVote->button(QDialogButtonBox::Yes)->setStyleSheet(COLOR_VOTE_YES);
                ui->buttonBoxVote->button(QDialogButtonBox::No)->setStyleSheet(COLOR_VOTE_NEUTRAL);
            }
            else
            {
                // Proposal was noted no, shade in no button and unshade yes button
                ui->buttonBoxVote->setStandardButtons(QDialogButtonBox::No|QDialogButtonBox::Yes|QDialogButtonBox::Cancel);
                ui->buttonBoxVote->button(QDialogButtonBox::Yes)->setStyleSheet(COLOR_VOTE_NEUTRAL);
                ui->buttonBoxVote->button(QDialogButtonBox::No)->setStyleSheet(COLOR_VOTE_NO);
            }
        }
        else
        {
            // Proposal was not voted on, reset shades of both buttons
            ui->buttonBoxVote->setStandardButtons(QDialogButtonBox::No|QDialogButtonBox::Yes);
            ui->buttonBoxVote->button(QDialogButtonBox::Yes)->setStyleSheet(COLOR_VOTE_NEUTRAL);
            ui->buttonBoxVote->button(QDialogButtonBox::No)->setStyleSheet(COLOR_VOTE_NEUTRAL);
        }
    }

    // If a proposal is expired pending voting of payment requests, change the expiry label text
    if(status.find("expired pending voting of payment requests") != string::npos) {
        ui->labelDuration->setText(QString::fromStdString("After payment request voting"));
    }

    {
        LOCK(cs_main);
        //hide ui voting elements on proposals which are not allowed vote states
        if(!proposal.CanVote())
            ui->buttonBoxVote->setStandardButtons(QDialogButtonBox::NoButton);
    }

    // Prevent overflow of title
    std::string title_string = proposal.strDZeel;
    std::replace( title_string.begin(), title_string.end(), '\n', ' ');
    if (title_string.length() > 140)
    {
        title_string = title_string.substr(0, 140);
        title_string.append("...");
    }
    ui->title->setText(QString::fromStdString(title_string));

    // Hide expiry label is proposal is accepted and waiting for coins
    if(status.find("accepted waiting for enough coins in fund") != string::npos) {
        ui->labelDuration->setVisible(false);
        ui->labelTitleDuration->setVisible(false);
    }
}

void CommunityFundDisplay::click_buttonBoxVote(QAbstractButton *button)
{
    // Make sure we have a lock when voting
    LOCK(cs_main);

    // Cast the vote
    bool duplicate = false;

    CFund::CProposal p;
    if (!pcoinsTip->GetProposal(uint256S(proposal.hash.ToString()), p))
    {
        return;
    }

    if (ui->buttonBoxVote->buttonRole(button) == QDialogButtonBox::YesRole)
    {
        CFund::VoteProposal(p, true, duplicate);
        refresh();
    }
    else if(ui->buttonBoxVote->buttonRole(button) == QDialogButtonBox::NoRole)
    {
        CFund::VoteProposal(p, false, duplicate);
        refresh();
    }
    else if(ui->buttonBoxVote->buttonRole(button) == QDialogButtonBox::RejectRole)
    {
        CFund::RemoveVoteProposal(p.hash.ToString());
        refresh();
    }
    else
    {
        refresh();
        return;
    }
}

void CommunityFundDisplay::click_pushButtonDetails()
{
    CommunityFundDisplayDetailed dlg(this, proposal);
    dlg.exec();
    refresh();
}

CommunityFundDisplay::~CommunityFundDisplay()
{
    delete ui;
}
