#include "communityfunddisplay.h"
#include "ui_communityfunddisplay.h"
#include "main.h"
#include <QtWidgets/QDialogButtonBox>
#include "../txdb.h"
#include <iomanip>
#include <sstream>

#include "consensus/cfund.h"
#include <iostream>
#include "chain.h"

#include "communityfunddisplaydetailed.h"

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

    connect(ui->buttonBoxVote, SIGNAL(clicked( QAbstractButton*)), this, SLOT(on_click_buttonBoxVote(QAbstractButton*)));
    connect(ui->pushButtonDetails, SIGNAL(clicked()), this, SLOT(on_click_pushButtonDetails()));

    //set labels from community fund
    ui->title->setText(QString::fromStdString(proposal.strDZeel));
    ui->labelStatus->setText(QString::fromStdString(proposal.GetState(pindexBestHeader->GetBlockTime())));
    stringstream n;
    n.imbue(std::locale(""));
    n << fixed << setprecision(8) << proposal.nAmount/100000000.0;
    string nav_amount = n.str();
    nav_amount.erase(nav_amount.find_last_not_of("0") + 1, std::string::npos );
    if(nav_amount.at(nav_amount.length()-1) == '.') {
        nav_amount = nav_amount.substr(0, nav_amount.size()-1);
    }
    nav_amount.append(" NAV");
    ui->labelRequested->setText(QString::fromStdString(nav_amount));

    uint64_t proptime = 0;
    if (mapBlockIndex.count(proposal.blockhash) > 0) {
        proptime = mapBlockIndex[proposal.blockhash]->GetBlockTime();
    }

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

    ui->labelDuration->setText(QString::fromStdString(s_deadline));

    // Hide ability to vote is the status is expired
    std::string status = ui->labelStatus->text().toStdString();
    if (status.find("expired") != string::npos) {
        ui->buttonBoxVote->setStandardButtons(QDialogButtonBox::NoButton);
    }

    //hide ui voting elements on proposals which are not allowed vote states
    if(!proposal.CanVote())
    {
        ui->buttonBoxVote->setStandardButtons(QDialogButtonBox::NoButton);
    }

    std::string title_string = proposal.strDZeel;
    if (title_string.length() > 140) {
        title_string = title_string.substr(0, 140);
        title_string.append("...");
    }
    ui->title->setText(QString::fromStdString(title_string));
}

void CommunityFundDisplay::on_click_buttonBoxVote(QAbstractButton *button)
{
    //cast the vote
    bool duplicate = false;

    //add a filter based on if proposal or payment request

    //currently doesnt work
    if (ui->buttonBoxVote->buttonRole(button) == QDialogButtonBox::YesRole)
    {
        //if !fSettings, ask what this does
        CFund::VoteProposal(proposal.blockhash, true, duplicate);
    }
    else if(ui->buttonBoxVote->buttonRole(button) == QDialogButtonBox::NoRole)
    {
            CFund::VoteProposal(proposal.blockhash, false, duplicate);
    }
    else {
        return;
    }

    //updat
}


void CommunityFundDisplay::on_click_pushButtonDetails()
{
    CommunityFundDisplayDetailed dlg(this, proposal);
    dlg.exec();
}


CommunityFundDisplay::~CommunityFundDisplay()
{
    delete ui;
}
