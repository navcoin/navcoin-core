#include "communityfunddisplaypaymentrequestdetailed.h"
#include "ui_communityfunddisplaypaymentrequestdetailed.h"
#include "communityfundpage.h"
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
    connect(ui->buttonBoxYesNoVote_2, SIGNAL(clicked( QAbstractButton*)), this, SLOT(click_buttonBoxYesNoVote(QAbstractButton*)));
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

    //hide ui voting elements on prequests which are not allowed vote states
    if(!prequest.CanVote())
    {
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
    ui->labelPrequestStatus->setText(QString::fromStdString(prequest.GetState()));

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

    // Hide ability to vote is the status is expired
    std::string status = ui->labelPrequestStatus->text().toStdString();
    if (status.find("expired") != string::npos) {
        ui->buttonBoxYesNoVote_2->setStandardButtons(QDialogButtonBox::NoButton);
    }

    // Expiry
    uint64_t proptime = 0;
    if (mapBlockIndex.count(prequest.blockhash) > 0) {
        proptime = mapBlockIndex[prequest.blockhash]->GetBlockTime();
    }

    // If prequest is pending show voting cycles left
    if (prequest.fState == CFund::NIL) {
        std::string duration_title = "Expires in: ";
        std::string duration = std::to_string(8-prequest.nVotingCycle) +  " voting cycles";
        ui->labelPrequestExpiryTitle->setText(QString::fromStdString(duration_title));
        ui->labelPrequestExpiry->setText(QString::fromStdString(duration));
    }

    // If prequest is accepted, show when it was accepted
    if (prequest.fState == CFund::ACCEPTED) {
        std::string duration_title = "Accepted on: ";
        std::time_t t = static_cast<time_t>(proptime);
        std::stringstream ss;
        ss << std::put_time(std::gmtime(&t), "%c %Z");
        ui->labelPrequestExpiryTitle->setText(QString::fromStdString(duration_title));
        ui->labelPrequestExpiry->setText(QString::fromStdString(ss.str().erase(10, 9)));
    }

    // If prequest is rejected, show when it was rejected
    if (prequest.fState == CFund::REJECTED) {
        std::string expiry_title = "Rejected on: ";
        std::time_t t = static_cast<time_t>(proptime);
        std::stringstream ss;
        ss << std::put_time(std::gmtime(&t), "%c %Z");
        ui->labelPrequestExpiryTitle->setText(QString::fromStdString(expiry_title));
        ui->labelPrequestExpiry->setText(QString::fromStdString(ss.str().erase(10, 9)));
    }

    // If expired show when it expired
    if (prequest.fState == CFund::EXPIRED || status.find("expired") != string::npos) {
        if (prequest.fState == CFund::EXPIRED) {
            std::string expiry_title = "Expired on: ";
            std::time_t t = static_cast<time_t>(proptime);
            std::stringstream ss;
            ss << std::put_time(std::gmtime(&t), "%c %Z");
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
    if (prequest.fState == CFund::NIL) {
        ui->labelPrequestTransactionBlockHashTitle->setVisible(false);
        ui->labelPrequestTransactionBlockHash->setVisible(false);
    }

    // If the prequest is not accepted, hide the payment hash
    if (prequest.fState != CFund::ACCEPTED) {
        ui->labelPrequestPaymentHashTitle->setVisible(false);
        ui->labelPrequestPaymentHash->setVisible(false);
    }

    // Format long descriptions
    std::string description = prequest.strDZeel.c_str();
    std::istringstream buf(description);
    std::istream_iterator<std::string> beg(buf), end;
    std::string finalDescription = "";
    std::vector<std::string> words(beg, end);
    for(std::string word : words) {
        int count = 0;
        while(count < word.length()-1) {
            if (count % 40 == 0 && count != 0) {
                word.insert(count, "-");
            }
            count++;
        }
        finalDescription += word + " ";
    }

    ui->labelPaymentRequestTitle->setText(QString::fromStdString(finalDescription));
}

void CommunityFundDisplayPaymentRequestDetailed::click_buttonBoxYesNoVote(QAbstractButton *button)
{
    // Cast the vote
    bool duplicate = false;

    if (ui->buttonBoxYesNoVote_2->buttonRole(button) == QDialogButtonBox::YesRole)
    {
        ui->buttonBoxYesNoVote_2->setStandardButtons(QDialogButtonBox::No|QDialogButtonBox::Yes|QDialogButtonBox::Cancel);
        ui->buttonBoxYesNoVote_2->button(QDialogButtonBox::Yes)->setStyleSheet(COLOR_VOTE_YES);
        ui->buttonBoxYesNoVote_2->button(QDialogButtonBox::No)->setStyleSheet(COLOR_VOTE_NEUTRAL);
        CFund::VotePaymentRequest(prequest.hash.ToString(), true, duplicate);
    }
    else if(ui->buttonBoxYesNoVote_2->buttonRole(button) == QDialogButtonBox::NoRole)
    {
        ui->buttonBoxYesNoVote_2->setStandardButtons(QDialogButtonBox::No|QDialogButtonBox::Yes|QDialogButtonBox::Cancel);
        ui->buttonBoxYesNoVote_2->button(QDialogButtonBox::Yes)->setStyleSheet(COLOR_VOTE_NEUTRAL);
        ui->buttonBoxYesNoVote_2->button(QDialogButtonBox::No)->setStyleSheet(COLOR_VOTE_NO);
        CFund::VotePaymentRequest(prequest.hash.ToString(), false, duplicate);
    }
    else if(ui->buttonBoxYesNoVote_2->buttonRole(button) == QDialogButtonBox::RejectRole)
    {
        ui->buttonBoxYesNoVote_2->setStandardButtons(QDialogButtonBox::No|QDialogButtonBox::Yes);
        ui->buttonBoxYesNoVote_2->button(QDialogButtonBox::Yes)->setStyleSheet(COLOR_VOTE_NEUTRAL);
        ui->buttonBoxYesNoVote_2->button(QDialogButtonBox::No)->setStyleSheet(COLOR_VOTE_NEUTRAL);
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
