#include "communityfunddisplaypaymentrequest.h"
#include "ui_communityfunddisplaypaymentrequest.h"
#include "main.h"
#include <QtWidgets/QDialogButtonBox>
#include "../txdb.h"
#include <iomanip>
#include <sstream>
#include <iostream>
#include <ctime>
#include "consensus/cfund.h"
#include <iostream>
#include "chain.h"

// Unsure about this one as yet
#include "communityfunddisplaydetailed.h"

CommunityFundDisplayPaymentRequest::CommunityFundDisplayPaymentRequest(QWidget *parent, CFund::CPaymentRequest prequest) :
    QWidget(parent),
    ui(new Ui::CommunityFundDisplayPaymentRequest),
    prequest(prequest)
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

    connect(ui->buttonBoxVote, SIGNAL(clicked( QAbstractButton*)), this, SLOT(on_click_buttonBoxVote(QAbstractButton*)));
    connect(ui->pushButtonDetails, SIGNAL(clicked()), this, SLOT(on_click_pushButtonDetails()));

    //set labels from community fund
    ui->title->setText(QString::fromStdString(prequest.strDZeel));
    ui->labelStatus->setText(QString::fromStdString(prequest.GetState()));
    stringstream n;
    n.imbue(std::locale(""));
    n << fixed << setprecision(8) << prequest.nAmount/100000000.0;
    string nav_amount = n.str();
    nav_amount.erase(nav_amount.find_last_not_of("0") + 1, std::string::npos );
    if(nav_amount.at(nav_amount.length()-1) == '.') {
        nav_amount = nav_amount.substr(0, nav_amount.size()-1);
    }
    nav_amount.append(" NAV");
    ui->labelRequested->setText(QString::fromStdString(nav_amount));

    uint64_t proptime = 0;
    if (mapBlockIndex.count(prequest.blockhash) > 0) {
        proptime = mapBlockIndex[prequest.blockhash]->GetBlockTime();
    }

    uint64_t deadline = proptime - pindexBestHeader->GetBlockTime();

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

    ui->labelDuration->setText(QString::fromStdString(s_deadline));

    // Hide ability to vote is the status is expired
    std::string status = ui->labelStatus->text().toStdString();
    if (status.find("expired") != string::npos) {
        ui->buttonBoxVote->setStandardButtons(QDialogButtonBox::NoButton);
    }

    // If prequest is pending show voting cycles left
    if (prequest.fState == CFund::NIL) {
        std::string duration_title = "Voting Cycle: ";
        std::string duration = std::to_string(prequest.nVotingCycle) +  " of 6";
        ui->labelTitleDuration->setText(QString::fromStdString(duration_title));
        ui->labelDuration->setText(QString::fromStdString(duration));
    }

    // If prequest is rejected, show when it was rejected
    if (prequest.fState == CFund::REJECTED) {
        std::string expiry_title = "Rejected on: ";
        std::time_t t = static_cast<time_t>(proptime);
        std::stringstream ss;
        ss << std::put_time(std::gmtime(&t), "%c %Z");
        ui->labelTitleDuration->setText(QString::fromStdString(expiry_title));
        ui->labelDuration->setText(QString::fromStdString(ss.str().erase(10, 9)));
    }

    // If expired show when it expired
    if (prequest.fState == CFund::EXPIRED || status.find("expired") != string::npos) {
        if (prequest.fState == CFund::EXPIRED) {
            std::string expiry_title = "Expired on: ";
            std::time_t t = static_cast<time_t>(proptime);
            std::stringstream ss;
            ss << std::put_time(std::gmtime(&t), "%c %Z");
            ui->labelTitleDuration->setText(QString::fromStdString(expiry_title));
            ui->labelDuration->setText(QString::fromStdString(ss.str().erase(10, 9)));
        }
        else {
            std::string expiry_title = "Expires: ";
            std::string expiry = "At end of voting period";
            ui->labelTitleDuration->setText(QString::fromStdString(expiry_title));
            ui->labelDuration->setText(QString::fromStdString(expiry));
        }
    }

    // Shade in yes/no buttons is user has voted
    // If the prequest is pending and not prematurely expired (ie can be voted on):
    if (prequest.fState == CFund::NIL && prequest.GetState().find("expired") == string::npos) {
        // Get prequest votes list
        auto it = std::find_if( vAddedPaymentRequestVotes.begin(), vAddedPaymentRequestVotes.end(),
                                [&prequest](const std::pair<std::string, bool>& element){ return element.first == prequest.hash.ToString();} );
        if (it != vAddedPaymentRequestVotes.end()) {
            if (it->second) {
                // Prequest was voted yes, shade in yes button and unshade no button
                ui->buttonBoxVote->button(QDialogButtonBox::Yes)->setStyleSheet("background-color: #35db03;");
                ui->buttonBoxVote->button(QDialogButtonBox::No)->setStyleSheet("background-color: #F3F4F6;");
            }
            else {
                // Prequest was noted no, shade in no button and unshade yes button
                ui->buttonBoxVote->button(QDialogButtonBox::Yes)->setStyleSheet("background-color: #F3F4F6;");
                ui->buttonBoxVote->button(QDialogButtonBox::No)->setStyleSheet("background-color: #de1300;");
            }
        }
        else {
            // Prequest was not voted on, reset shades of both buttons
            ui->buttonBoxVote->button(QDialogButtonBox::Yes)->setStyleSheet("background-color: #F3F4F6;");
            ui->buttonBoxVote->button(QDialogButtonBox::No)->setStyleSheet("background-color: #F3F4F6;");

        }
    }

    //hide ui voting elements on proposals which are not allowed vote states
    if(!prequest.CanVote())
    {
        ui->buttonBoxVote->setStandardButtons(QDialogButtonBox::NoButton);
    }

    std::string title_string = prequest.strDZeel;
    if (title_string.length() > 140) {
        title_string = title_string.substr(0, 140);
        title_string.append("...");
    }
    ui->title->setText(QString::fromStdString(title_string));

}

void CommunityFundDisplayPaymentRequest::on_click_buttonBoxVote(QAbstractButton *button)
{
    //cast the vote
    bool duplicate = false;

    if (ui->buttonBoxVote->buttonRole(button) == QDialogButtonBox::YesRole)
    {
        ui->buttonBoxVote->button(QDialogButtonBox::Yes)->setStyleSheet("background-color: #35db03;");
        ui->buttonBoxVote->button(QDialogButtonBox::No)->setStyleSheet("background-color: #F3F4F6;");
        CFund::VotePaymentRequest(prequest.hash.ToString(), true, duplicate);
    }
    else if(ui->buttonBoxVote->buttonRole(button) == QDialogButtonBox::NoRole)
    {
        ui->buttonBoxVote->button(QDialogButtonBox::Yes)->setStyleSheet("background-color: #F3F4F6;");
        ui->buttonBoxVote->button(QDialogButtonBox::No)->setStyleSheet("background-color: #de1300;");
        CFund::VotePaymentRequest(prequest.hash.ToString(), false, duplicate);
    }
    else if(ui->buttonBoxVote->buttonRole(button) == QDialogButtonBox::RejectRole)
    {
        ui->buttonBoxVote->button(QDialogButtonBox::Yes)->setStyleSheet("background-color: #F3F4F6;");
        ui->buttonBoxVote->button(QDialogButtonBox::No)->setStyleSheet("background-color: #F3F4F6;");
        CFund::RemoveVotePaymentRequest(prequest.hash.ToString());
    }
    else {
        return;
    }

    //update
}

void CommunityFundDisplayPaymentRequest::on_click_pushButtonDetails()
{
    // Wont work just yet due to needing to pass a payment request
    //CommunityFundDisplayDetailed dlg(this, proposal);
    //dlg.exec();
}

CommunityFundDisplayPaymentRequest::~CommunityFundDisplayPaymentRequest()
{
    delete ui;
}
