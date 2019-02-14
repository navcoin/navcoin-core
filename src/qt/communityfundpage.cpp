#include "communityfundpage.h"
#include "ui_communityfundpage.h"
#include <QAbstractScrollArea>
#include "communityfundpage.moc"
#include <iostream>
#include "main.h"
#include "txdb.h"
#include <string>
#include <iomanip>
#include <sstream>
#include "communityfunddisplay.h"

#include "communityfundcreateproposaldialog.h"
#include "communityfundcreatepaymentrequestdialog.h"

CommunityFundPage::CommunityFundPage(const PlatformStyle *platformStyle, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::CommunityFundPage),
    clientModel(0),
    walletModel(0),
    flag(CFund::NIL),
    viewing_proposals(true),
    viewing_voted(false)
{
    ui->setupUi(this);

    // Hide horizontal scrollArea scroll bar
    ui->scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    connect(ui->pushButtonProposals, SIGNAL(clicked()), this, SLOT(on_click_pushButtonProposals()));
    connect(ui->pushButtonPaymentRequests, SIGNAL(clicked()), this, SLOT(on_click_pushButtonPaymentRequests()));

    // Enable selection of pushButtonProposals by default
    ui->pushButtonProposals->setStyleSheet("QPushButton { background-color: #DBE0E8; }");
    ui->pushButtonPaymentRequests->setStyleSheet("QPushButton { background-color: #EDF0F3; }");

    connect(ui->radioButtonAll, SIGNAL(clicked()), this, SLOT(on_click_radioButtonAll()));
    connect(ui->radioButtonYourVote, SIGNAL(clicked()), this, SLOT(on_click_radioButtonYourVote()));
    connect(ui->radioButtonPending, SIGNAL(clicked()), this, SLOT(on_click_radioButtonPending()));
    connect(ui->radioButtonAccepted, SIGNAL(clicked()), this, SLOT(on_click_radioButtonAccepted()));
    connect(ui->radioButtonRejected, SIGNAL(clicked()), this, SLOT(on_click_radioButtonRejected()));
    connect(ui->radioButtonExpired, SIGNAL(clicked()), this, SLOT(on_click_radioButtonExpired()));
    connect(ui->pushButtonCreateProposal, SIGNAL(clicked()), this , SLOT(on_click_pushButtonCreateProposal()));
    connect(ui->pushButtonCreatePaymentRequest, SIGNAL(clicked()), this, SLOT(on_click_pushButtonCreatePaymentRequest()));

    //fetch cfund info
    Refresh(true, true);
}

void CommunityFundPage::setWalletModel(WalletModel *model)
{
    this->walletModel = model;

    Refresh(true, true);
    ui->radioButtonAll->setChecked(true);
}

void CommunityFundPage::refreshTab()
{
    Refresh(true, true);
    ui->radioButtonAll->setChecked(true);
    on_click_pushButtonProposals();
}

void CommunityFundPage::Refresh(bool all, bool proposal)
{
    for (int i = 0; i < ui->gridLayout->count(); ++i)
    {
      QWidget *widget = ui->gridLayout->itemAt(i)->widget();
      if (widget != NULL)
      {
        widget->setVisible(false);
      }
    }

    stringstream a;
    a.imbue(std::locale(""));
    a << fixed << setprecision(8) << pindexBestHeader->nCFSupply/100000000.0;
    string available = a.str();
    available.erase(available.find_last_not_of("0") + 1, std::string::npos );
    if(available.at(available.length()-1) == '.') {
        available = available.substr(0, available.size()-1);
    }
    available.append(" NAV");
    ui->labelAvailableAmount->setText(QString::fromStdString(available));

    stringstream l;
    l.imbue(std::locale(""));
    l << fixed << setprecision(8) << pindexBestHeader->nCFLocked/100000000.0;
    string locked = l.str();
    locked.erase(locked.find_last_not_of("0") + 1, std::string::npos );
    if(locked.at(locked.length()-1) == '.') {
        locked = locked.substr(0, locked.size()-1);
    }
    locked.append(" NAV");
    ui->labelLockedAmount->setText(QString::fromStdString(locked));

    if (proposal) {
        std::vector<CFund::CProposal> vec;
        if(pblocktree->GetProposalIndex(vec))
        {
            int r = 0;
            int c = 0;
            BOOST_FOREACH(const CFund::CProposal proposal, vec) {
                // If wanting to view all proposals
                if (all) {
                    ui->gridLayout->addWidget(new CommunityFundDisplay(0, proposal), r, c);
                    if(c == 1)
                    {
                        c = 0;
                        ++r;
                    }
                    else
                    {
                        ++c;
                    }
                }
                else {
                    // If the filter is set to my vote, filter to only pending proposals which have been voted for
                    if (viewing_voted) {
                        if (proposal.fState == CFund::NIL && proposal.GetState(pindexBestHeader->GetBlockTime()).find("expired") == string::npos) {
                            auto it = std::find_if( vAddedProposalVotes.begin(), vAddedProposalVotes.end(),
                                [&proposal](const std::pair<std::string, bool>& element){ return element.first == proposal.hash.ToString();} );
                            UniValue p(UniValue::VOBJ);
                            proposal.ToJson(p);
                            if (it != vAddedProposalVotes.end()) {
                                ui->gridLayout->addWidget(new CommunityFundDisplay(0, proposal), r, c);
                                if(c == 1)
                                {
                                    c = 0;
                                    ++r;
                                }
                                else
                                {
                                    ++c;
                                }
                            } else {
                                continue;
                            }
                        }
                    }

                    // If the flag is expired, be sure to display proposals without the expired state if they have expired before the end of the voting cycle
                    if (proposal.fState != CFund::EXPIRED && proposal.GetState(pindexBestHeader->GetBlockTime()).find("expired") != string::npos && flag == CFund::EXPIRED) {
                        ui->gridLayout->addWidget(new CommunityFundDisplay(0, proposal), r, c);
                        if(c == 1)
                        {
                            c = 0;
                            ++r;
                        }
                        else
                        {
                            ++c;
                        }
                    }
                    // Display proposals with the appropriate flag and have not expired before the voting cycle has ended
                    if (proposal.fState != flag || (flag != CFund::EXPIRED && proposal.GetState(pindexBestHeader->GetBlockTime()).find("expired") != string::npos))
                        continue;
                    ui->gridLayout->addWidget(new CommunityFundDisplay(0, proposal), r, c);
                    if(c == 1)
                    {
                        c = 0;
                        ++r;
                    }
                    else
                    {
                        ++c;
                    }
                }
            }
        }
    }
    else {
        std::vector<CFund::CPaymentRequest> vec;
        if(pblocktree->GetPaymentRequestIndex(vec))
        {
            BOOST_FOREACH(const CFund::CPaymentRequest& prequest, vec) {
                // List each payment request here
            }
        }
    }

    {
    int64_t spent_nav = 0;
    std::vector<CFund::CPaymentRequest> vec;
    if(pblocktree->GetPaymentRequestIndex(vec))
    {
        BOOST_FOREACH(const CFund::CPaymentRequest& prequest, vec) {
            if(prequest.fState == CFund::ACCEPTED) {
                spent_nav = spent_nav + prequest.nAmount;
            }
        }

        stringstream s;
        s.imbue(std::locale(""));
        s << fixed << setprecision(8) << spent_nav/100000000.0;
        string spent = s.str();
        spent.erase(spent.find_last_not_of("0") + 1, std::string::npos );
        if(spent.at(spent.length()-1) == '.') {
            spent = spent.substr(0, spent.size()-1);
        }
        spent.append(" NAV");
        ui->labelSpentAmount->setText(QString::fromStdString(spent));
    }
    }
}

void CommunityFundPage::on_click_pushButtonProposals()
{
    QFont font = ui->pushButtonProposals->property("font").value<QFont>();

    ui->pushButtonProposals->setStyleSheet("QPushButton { background-color: #DBE0E8; }");
    ui->pushButtonPaymentRequests->setStyleSheet("QPushButton { background-color: #EDF0F3; }");

    QFont f(font.family(), font.pointSize(), QFont::Bold);
    ui->pushButtonProposals->setFont(f);
    ui->pushButtonPaymentRequests->setFont(f);

    viewing_proposals = true;

    Refresh(true, true);
    ui->radioButtonAll->setChecked(true);
}

void CommunityFundPage::on_click_pushButtonPaymentRequests()
{
    QFont font = ui->pushButtonPaymentRequests->property("font").value<QFont>();

    ui->pushButtonProposals->setStyleSheet("QPushButton { background-color: #EDF0F3; }");
    ui->pushButtonPaymentRequests->setStyleSheet("QPushButton { background-color: #DBE0E8; }");

    QFont f(font.family(), font.pointSize(), QFont::Bold);
    ui->pushButtonProposals->setFont(f);
    ui->pushButtonPaymentRequests->setFont(f);

    viewing_proposals = false;

    Refresh(true, false);
    ui->radioButtonAll->setChecked(true);
}

void CommunityFundPage::on_click_radioButtonAll()
{
    flag = CFund::NIL;
    viewing_voted = false;
    Refresh(true, viewing_proposals);
}

void CommunityFundPage::on_click_radioButtonYourVote()
{
    flag = CFund::NIL;
    viewing_voted = true;
    Refresh(false, viewing_proposals);
}

void CommunityFundPage::on_click_radioButtonPending()
{
    flag = CFund::NIL;
    viewing_voted = false;
    Refresh(false, viewing_proposals);
}

void CommunityFundPage::on_click_radioButtonAccepted()
{
    flag = CFund::ACCEPTED;
    viewing_voted = false;
    Refresh(false, viewing_proposals);
}

void CommunityFundPage::on_click_radioButtonRejected()
{
    flag = CFund::REJECTED;
    viewing_voted = false;
    Refresh(false, viewing_proposals);
}

void CommunityFundPage::on_click_radioButtonExpired()
{
    flag = CFund::EXPIRED;
    viewing_voted = false;
    Refresh(false, viewing_proposals);
}

void CommunityFundPage::on_click_pushButtonCreateProposal()
{
        CommunityFundCreateProposalDialog dlg(this);
        dlg.exec();
}

void CommunityFundPage::on_click_pushButtonCreatePaymentRequest()
{
    CommunityFundCreatePaymentRequestDialog dlg(this);
    dlg.exec();
}

CommunityFundPage::~CommunityFundPage()
{
    delete ui;
}
