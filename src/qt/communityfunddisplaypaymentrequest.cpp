#include <qt/communityfunddisplaypaymentrequest.h>
#include <ui_communityfunddisplaypaymentrequest.h>
#include <qt/communityfundpage.h>
#include <qt/communityfunddisplaypaymentrequestdetailed.h>

#include <QtWidgets/QDialogButtonBox>
#include <main.h>
#include <qt/../txdb.h>
#include <iomanip>
#include <sstream>
#include <ctime>
#include <consensus/dao.h>
#include <wallet/wallet.h>
#include <base58.h>
#include <chain.h>

CommunityFundDisplayPaymentRequest::CommunityFundDisplayPaymentRequest(QWidget *parent, CPaymentRequest prequest) :
    QWidget(parent),
    ui(new Ui::CommunityFundDisplayPaymentRequest),
    prequest(prequest)
{
    ui->setupUi(this);

    QFont f_title("Sans Serif", 10.5, QFont::Bold);
    QFont f_label_title("Sans Serif", 10, QFont::Bold);
    QFont f_label("Sans Serif", 10, QFont::Normal);

    ui->buttonBoxVote->setStandardButtons(QDialogButtonBox::No|QDialogButtonBox::Yes|QDialogButtonBox::Ignore|QDialogButtonBox::Cancel);
    ui->buttonBoxVote->button(QDialogButtonBox::Ignore)->setText(tr("Abstain"));

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

void CommunityFundDisplayPaymentRequest::refresh()
{
    LOCK(cs_main);
    CStateViewCache coins(pcoinsTip);

    // Set labels from community fund
    ui->title->setText(QString::fromStdString(prequest.strDZeel));
    ui->labelStatus->setText(QString::fromStdString(prequest.GetState(coins)));

    std::string nav_amount;
    nav_amount = wallet->formatDisplayAmount(prequest.nAmount);
    ui->labelRequested->setText(QString::fromStdString(nav_amount));

    uint64_t proptime = 0;
    CBlockIndex* pblockindex = prequest.GetLastStateBlockIndex();
    if (pblockindex) {
        proptime = pblockindex->GetBlockTime();
    }

    uint64_t deadline = proptime - pindexBestHeader->GetBlockTime();
    uint64_t deadline_d = std::floor(deadline/86400);
    uint64_t deadline_h = std::floor((deadline-deadline_d*86400)/3600);
    uint64_t deadline_m = std::floor((deadline-(deadline_d*86400 + deadline_h*3600))/60);

    if(deadline_d >= 14)
        ui->labelDuration->setText(tr("%n Days", "", deadline_d));
    else
        ui->labelDuration->setText(tr("%n Days", "", deadline_d) + tr(" %n Hours", "", deadline_h) + tr(" %n Minutes", "", deadline_m));

    // Hide ability to vote is the status is expired
    std::string status = ui->labelStatus->text().toStdString();
    if (status.find("expired") != std::string::npos) {
        ui->buttonBoxVote->setStandardButtons(QDialogButtonBox::NoButton);
    }

    auto fLastState = prequest.GetLastState();

    // If prequest is accepted, show when it was accepted
    if (fLastState == DAOFlags::ACCEPTED)
    {
        std::time_t t = static_cast<time_t>(proptime);
        std::stringstream ss;
        ss << std::put_time(std::gmtime(&t), "%c %Z");
        ui->labelTitleDuration->setText(tr("Accepted on: "));
        ui->labelDuration->setText(QString::fromStdString(ss.str()));
    }

    // If prequest is pending show voting cycles left
    if (fLastState == DAOFlags::NIL)
    {
        ui->labelTitleDuration->setText(tr("Voting Cycle: "));
        ui->labelDuration->setText(QString::number(prequest.nVotingCycle) + tr(" of ") + QString::number(GetConsensusParameter(Consensus::CONSENSUS_PARAM_PAYMENT_REQUEST_MAX_VOTING_CYCLES, coins)));
    }

    // If prequest is rejected, show when it was rejected
    if (fLastState == DAOFlags::REJECTED)
    {
        std::time_t t = static_cast<time_t>(proptime);
        std::stringstream ss;
        ss << std::put_time(std::gmtime(&t), "%c %Z");
        ui->labelTitleDuration->setText(tr("Rejected on: "));
        ui->labelDuration->setText(QString::fromStdString(ss.str()));
    }

    // If expired show when it expired
    if (fLastState == DAOFlags::EXPIRED || status.find("expired") != std::string::npos)
    {
        if (fLastState == DAOFlags::EXPIRED)
        {
            std::time_t t = static_cast<time_t>(proptime);
            std::stringstream ss;
            ss << std::put_time(std::gmtime(&t), "%c %Z");
            ui->labelTitleDuration->setText(tr("Expired on: "));
            ui->labelDuration->setText(QString::fromStdString(ss.str()));
        }
        else
        {
            ui->labelTitleDuration->setText(tr("Expires: "));
            ui->labelDuration->setText(tr("At end of voting period"));
        }
    }

    // Shade in yes/no buttons is user has voted
    // If the prequest is pending and not prematurely expired (ie can be voted on):
    if (fLastState == DAOFlags::NIL && prequest.GetState(coins).find("expired") == std::string::npos)
    {
        // Get prequest votes list
        CPaymentRequest preq = prequest;
        auto it = mapAddedVotes.find(prequest.hash);
        if (it != mapAddedVotes.end())
        {
            if (it->second == 1)
            {
                // Prequest was voted yes, shade in yes button and unshade no button
                ui->buttonBoxVote->setStandardButtons(QDialogButtonBox::No|QDialogButtonBox::Yes|QDialogButtonBox::Ignore|QDialogButtonBox::Cancel);
                ui->buttonBoxVote->button(QDialogButtonBox::Yes)->setStyleSheet(COLOR_VOTE_YES);
                ui->buttonBoxVote->button(QDialogButtonBox::No)->setStyleSheet(COLOR_VOTE_NEUTRAL);
                ui->buttonBoxVote->button(QDialogButtonBox::Ignore)->setStyleSheet(COLOR_VOTE_NEUTRAL);
            }
            else if (it->second == 0)
            {
                // Prequest was noted no, shade in no button and unshade yes button
                ui->buttonBoxVote->setStandardButtons(QDialogButtonBox::No|QDialogButtonBox::Yes|QDialogButtonBox::Ignore|QDialogButtonBox::Cancel);
                ui->buttonBoxVote->button(QDialogButtonBox::Yes)->setStyleSheet(COLOR_VOTE_NEUTRAL);
                ui->buttonBoxVote->button(QDialogButtonBox::No)->setStyleSheet(COLOR_VOTE_NO);
                ui->buttonBoxVote->button(QDialogButtonBox::Ignore)->setStyleSheet(COLOR_VOTE_NEUTRAL);
            } else if (it->second == -1)
            {
                // Prequest was noted abstain, shade in no button and unshade yes button
                ui->buttonBoxVote->setStandardButtons(QDialogButtonBox::No|QDialogButtonBox::Yes|QDialogButtonBox::Ignore|QDialogButtonBox::Cancel);
                ui->buttonBoxVote->button(QDialogButtonBox::Yes)->setStyleSheet(COLOR_VOTE_NEUTRAL);
                ui->buttonBoxVote->button(QDialogButtonBox::No)->setStyleSheet(COLOR_VOTE_NO);
                ui->buttonBoxVote->button(QDialogButtonBox::Ignore)->setStyleSheet(COLOR_VOTE_ABSTAIN);
            }
        }
        else
        {
            // Prequest was not voted on, reset shades of both buttons
            ui->buttonBoxVote->setStandardButtons(QDialogButtonBox::No|QDialogButtonBox::Yes);
            ui->buttonBoxVote->button(QDialogButtonBox::Yes)->setStyleSheet(COLOR_VOTE_NEUTRAL);
            ui->buttonBoxVote->button(QDialogButtonBox::No)->setStyleSheet(COLOR_VOTE_NEUTRAL);
            ui->buttonBoxVote->button(QDialogButtonBox::Ignore)->setStyleSheet(COLOR_VOTE_NEUTRAL);
        }
    }

    //hide ui voting elements on proposals which are not allowed vote states
    if(!prequest.CanVote(coins))
        ui->buttonBoxVote->setStandardButtons(QDialogButtonBox::NoButton);

    std::string title_string = prequest.strDZeel;
    std::replace( title_string.begin(), title_string.end(), '\n', ' ');
    if (title_string.length() > 140) {
        title_string = title_string.substr(0, 140);
        title_string.append("...");
    }
    ui->title->setText(QString::fromStdString(title_string));
}

void CommunityFundDisplayPaymentRequest::click_buttonBoxVote(QAbstractButton *button)
{
    LOCK(cs_main);

    //cast the vote
    bool duplicate = false;

    CPaymentRequest pr;
    if (!pcoinsTip->GetPaymentRequest(uint256S(prequest.hash.ToString()), pr))
    {
        return;
    }

    if (ui->buttonBoxVote->buttonRole(button) == QDialogButtonBox::YesRole)
    {
        Vote(pr.hash, 1, duplicate);
        refresh();
    }
    else if(ui->buttonBoxVote->buttonRole(button) == QDialogButtonBox::NoRole)
    {
        Vote(pr.hash, 0, duplicate);
        refresh();
    }
    else if(ui->buttonBoxVote->standardButton(button) == QDialogButtonBox::Ignore)
    {
        Vote(pr.hash, -1, duplicate);
        refresh();
    }
    else if(ui->buttonBoxVote->buttonRole(button) == QDialogButtonBox::RejectRole)
    {
        RemoveVote(pr.hash);
        refresh();
    }
    else {
        refresh();
        return;
    }
}

void CommunityFundDisplayPaymentRequest::click_pushButtonDetails()
{
    CommunityFundDisplayPaymentRequestDetailed dlg(this, prequest);
    dlg.exec();
    refresh();
}

CommunityFundDisplayPaymentRequest::~CommunityFundDisplayPaymentRequest()
{
    delete ui;
}
