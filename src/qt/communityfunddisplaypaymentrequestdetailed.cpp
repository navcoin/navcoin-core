#include <qt/communityfunddisplaypaymentrequestdetailed.h>
#include <ui_communityfunddisplaypaymentrequestdetailed.h>
#include <qt/communityfundpage.h>
#include <main.h>
#include <iomanip>
#include <sstream>
#include <wallet/wallet.h>
#include <base58.h>

CommunityFundDisplayPaymentRequestDetailed::CommunityFundDisplayPaymentRequestDetailed(QWidget *parent, CFund::CPaymentRequest prequest) :
    QDialog(parent),
    ui(new Ui::CommunityFundDisplayPaymentRequestDetailed),
    prequest(prequest)
{
    ui->setupUi(this);

    //connect ui elements to functions
    connect(ui->buttonBoxYesNoVote_2, SIGNAL(clicked( QAbstractButton*)), this, SLOT(click_buttonBoxYesNoVote(QAbstractButton*)));
    connect(ui->pushButtonClose_2, SIGNAL(clicked()), this, SLOT(reject()));

    //update labels
    setPrequestLabels();

    auto fLastState = prequest.GetLastState();

    // Shade in yes/no buttons is user has voted
    // If the prequest is pending and not prematurely expired (ie can be voted on):
    if (fLastState == CFund::NIL && prequest.GetState().find("expired") == string::npos) {
        // Get prequest votes list
        auto it = std::find_if( vAddedPaymentRequestVotes.begin(), vAddedPaymentRequestVotes.end(),
                                [&prequest](const std::pair<std::string, bool>& element){ return element.first == prequest.hash.ToString();} );
        if (it != vAddedPaymentRequestVotes.end()) {
            if (it->second) {
                // Payment Request was voted yes, shade in yes button and unshade no button
                ui->buttonBoxYesNoVote_2->setStandardButtons(QDialogButtonBox::No|QDialogButtonBox::Yes|QDialogButtonBox::Cancel);
                ui->buttonBoxYesNoVote_2->button(QDialogButtonBox::Yes)->setStyleSheet(COLOR_VOTE_YES);
                ui->buttonBoxYesNoVote_2->button(QDialogButtonBox::No)->setStyleSheet(COLOR_VOTE_NEUTRAL);
            }
            else {
                // Payment Request was noted no, shade in no button and unshade yes button
                ui->buttonBoxYesNoVote_2->setStandardButtons(QDialogButtonBox::No|QDialogButtonBox::Yes|QDialogButtonBox::Cancel);
                ui->buttonBoxYesNoVote_2->button(QDialogButtonBox::Yes)->setStyleSheet(COLOR_VOTE_NEUTRAL);
                ui->buttonBoxYesNoVote_2->button(QDialogButtonBox::No)->setStyleSheet(COLOR_VOTE_NO);
            }
        }
        else {
            // Payment Request was not voted on, reset shades of both buttons
            ui->buttonBoxYesNoVote_2->setStandardButtons(QDialogButtonBox::No|QDialogButtonBox::Yes);
            ui->buttonBoxYesNoVote_2->button(QDialogButtonBox::Yes)->setStyleSheet(COLOR_VOTE_NEUTRAL);
            ui->buttonBoxYesNoVote_2->button(QDialogButtonBox::No)->setStyleSheet(COLOR_VOTE_NEUTRAL);

        }
    }

    {
        LOCK(cs_main);
        //hide ui voting elements on prequests which are not allowed vote states
        if(!prequest.CanVote(*pcoinsTip))
        {
            ui->buttonBoxYesNoVote_2->setStandardButtons(QDialogButtonBox::NoButton);
        }
    }
}

void CommunityFundDisplayPaymentRequestDetailed::setPrequestLabels() const
{
    // Title
    ui->labelPaymentRequestTitle->setText(QString::fromStdString(prequest.strDZeel));

    // Amount
    string amount;
    amount = wallet->formatDisplayAmount(prequest.nAmount);
    ui->labelPrequestAmount->setText(QString::fromStdString(amount));

    // Status
    ui->labelPrequestStatus->setText(QString::fromStdString(prequest.GetState()));

    // Yes Votes
    ui->labelPrequestYes->setText(QString::fromStdString(std::to_string(prequest.nVotesYes)));

    // No Votes
    ui->labelPrequestNo->setText(QString::fromStdString(std::to_string(prequest.nVotesNo)));

    // Payment Request Hash
    ui->labelPrequestHash->setText(QString::fromStdString(prequest.hash.ToString()));

    uint256 blockhash = uint256();
    CBlockIndex* pblockindex = prequest.GetLastStateBlockIndex();

    if (pblockindex)
        blockhash = pblockindex->GetBlockHash();

    // Transaction Block Hash
    ui->labelPrequestTransactionBlockHash->setText(QString::fromStdString(blockhash.ToString()));

    // Transaction Hash
    ui->labelPrequestTransactionHash->setText(QString::fromStdString(prequest.txblockhash.ToString()));

    // Version Number
    ui->labelPrequestVersionNo->setText(QString::fromStdString(std::to_string(prequest.nVersion)));

    // Voting Cycle
    ui->labelPrequestVotingCycle->setText(QString::fromStdString(std::to_string(prequest.nVotingCycle)));

    // Proposal Hash
    ui->labelPrequestProposalHash->setText(QString::fromStdString(prequest.proposalhash.ToString()));

    // Link
    ui->labelPrequestLink->setText(QString::fromStdString("https://www.navexplorer.com/community-fund/payment-request/" + prequest.hash.ToString()));

    // Hide ability to vote is the status is expired
    std::string status = ui->labelPrequestStatus->text().toStdString();
    if (status.find("expired") != string::npos) {
        ui->buttonBoxYesNoVote_2->setStandardButtons(QDialogButtonBox::NoButton);
    }

    // Expiry
    uint64_t proptime = 0;
    if (pblockindex)
        proptime = pblockindex->GetBlockTime();

    auto fLastState = prequest.GetLastState();

    // If prequest is pending show voting cycles left
    if (fLastState == CFund::NIL) {
        std::string duration_title = "Voting period finishes in: ";
        std::string duration = std::to_string(Params().GetConsensus().nCyclesPaymentRequestVoting-prequest.nVotingCycle) +  " voting cycles";
        ui->labelPrequestExpiryTitle->setText(QString::fromStdString(duration_title));
        ui->labelPrequestExpiry->setText(QString::fromStdString(duration));
    }

    // If prequest is accepted, show when it was accepted
    if (fLastState == CFund::ACCEPTED) {
        std::string duration_title = "Accepted on: ";
        std::time_t t = static_cast<time_t>(proptime);
        std::stringstream ss;
        char buf[48];
        if (strftime(buf, sizeof(buf), "%c %Z", std::gmtime(&t)))
            ss << buf;
        ui->labelPrequestExpiryTitle->setText(QString::fromStdString(duration_title));
        ui->labelPrequestExpiry->setText(QString::fromStdString(ss.str().erase(10, 9)));
    }

    // If prequest is rejected, show when it was rejected
    if (fLastState == CFund::REJECTED) {
        std::string expiry_title = "Rejected on: ";
        std::time_t t = static_cast<time_t>(proptime);
        std::stringstream ss;
        char buf[48];
        if (strftime(buf, sizeof(buf), "%c %Z", std::gmtime(&t)))
            ss << buf;
        ui->labelPrequestExpiryTitle->setText(QString::fromStdString(expiry_title));
        ui->labelPrequestExpiry->setText(QString::fromStdString(ss.str().erase(10, 9)));
    }

    // If expired show when it expired
    if (fLastState == CFund::EXPIRED || status.find("expired") != string::npos) {
        if (fLastState == CFund::EXPIRED) {
            std::string expiry_title = "Expired on: ";
            std::time_t t = static_cast<time_t>(proptime);
            std::stringstream ss;
            char buf[48];
            if (strftime(buf, sizeof(buf), "%c %Z", std::gmtime(&t)))
                ss << buf;
            ui->labelPrequestExpiryTitle->setText(QString::fromStdString(expiry_title));
            ui->labelPrequestExpiry->setText(QString::fromStdString(ss.str().erase(10, 9)));
        }
        else {
            std::string expiry_title = "Expires: ";
            std::string expiry = "At end of voting period";
            ui->labelPrequestExpiryTitle->setText(QString::fromStdString(expiry_title));
            ui->labelPrequestExpiry->setText(QString::fromStdString(expiry));
        }
    }

    //set hyperlink for navcommunity proposal view
    ui->labelPrequestLink->setTextFormat(Qt::RichText);
    ui->labelPrequestLink->setText("<a href=\"" + ui->labelPrequestLink->text() + "\">" + ui->labelPrequestLink->text() + "</a>");
    ui->labelPrequestLink->setTextInteractionFlags(Qt::TextBrowserInteraction);
    ui->labelPrequestLink->setOpenExternalLinks(true);

    // If prequest is pending, hide the transaction hash
    if (fLastState == CFund::NIL) {
        ui->labelPrequestTransactionBlockHashTitle->setVisible(false);
        ui->labelPrequestTransactionBlockHash->setVisible(false);
    }

    // If the prequest is not accepted, hide the payment hash
    ui->labelPrequestPaymentHashTitle->setVisible(false);
    ui->labelPrequestPaymentHash->setVisible(false);

    ui->labelPaymentRequestTitle->setText(QString::fromStdString(prequest.strDZeel.c_str()));

}

void CommunityFundDisplayPaymentRequestDetailed::click_buttonBoxYesNoVote(QAbstractButton *button)
{
    LOCK(cs_main);

    // Cast the vote
    bool duplicate = false;

    CFund::CPaymentRequest pr;
    if (!pcoinsTip->GetPaymentRequest(uint256S(prequest.hash.ToString()), pr))
    {
        return;
    }

    if (ui->buttonBoxYesNoVote_2->buttonRole(button) == QDialogButtonBox::YesRole)
    {
        ui->buttonBoxYesNoVote_2->setStandardButtons(QDialogButtonBox::No|QDialogButtonBox::Yes|QDialogButtonBox::Cancel);
        ui->buttonBoxYesNoVote_2->button(QDialogButtonBox::Yes)->setStyleSheet(COLOR_VOTE_YES);
        ui->buttonBoxYesNoVote_2->button(QDialogButtonBox::No)->setStyleSheet(COLOR_VOTE_NEUTRAL);
        CFund::VotePaymentRequest(pr, true, duplicate);
    }
    else if(ui->buttonBoxYesNoVote_2->buttonRole(button) == QDialogButtonBox::NoRole)
    {
        ui->buttonBoxYesNoVote_2->setStandardButtons(QDialogButtonBox::No|QDialogButtonBox::Yes|QDialogButtonBox::Cancel);
        ui->buttonBoxYesNoVote_2->button(QDialogButtonBox::Yes)->setStyleSheet(COLOR_VOTE_NEUTRAL);
        ui->buttonBoxYesNoVote_2->button(QDialogButtonBox::No)->setStyleSheet(COLOR_VOTE_NO);
        CFund::VotePaymentRequest(pr, false, duplicate);
    }
    else if(ui->buttonBoxYesNoVote_2->buttonRole(button) == QDialogButtonBox::RejectRole)
    {
        ui->buttonBoxYesNoVote_2->setStandardButtons(QDialogButtonBox::No|QDialogButtonBox::Yes);
        ui->buttonBoxYesNoVote_2->button(QDialogButtonBox::Yes)->setStyleSheet(COLOR_VOTE_NEUTRAL);
        ui->buttonBoxYesNoVote_2->button(QDialogButtonBox::No)->setStyleSheet(COLOR_VOTE_NEUTRAL);
        CFund::RemoveVotePaymentRequest(pr.hash.ToString());
    }
    else {
        return;
    }
}

CommunityFundDisplayPaymentRequestDetailed::~CommunityFundDisplayPaymentRequestDetailed()
{
    delete ui;
}
