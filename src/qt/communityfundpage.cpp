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
#include "communityfunddisplaypaymentrequest.h"
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

    connect(ui->pushButtonProposals, SIGNAL(clicked()), this, SLOT(click_pushButtonProposals()));
    connect(ui->pushButtonPaymentRequests, SIGNAL(clicked()), this, SLOT(click_pushButtonPaymentRequests()));

    // Enable selection of pushButtonProposals by default
    ui->pushButtonProposals->setStyleSheet("QPushButton { background-color: #DBE0E8; }");
    ui->pushButtonPaymentRequests->setStyleSheet("QPushButton { background-color: #EDF0F3; }");

    connect(ui->radioButtonAll, SIGNAL(clicked()), this, SLOT(click_radioButtonAll()));
    connect(ui->radioButtonYourVote, SIGNAL(clicked()), this, SLOT(click_radioButtonYourVote()));
    connect(ui->radioButtonPending, SIGNAL(clicked()), this, SLOT(click_radioButtonPending()));
    connect(ui->radioButtonAccepted, SIGNAL(clicked()), this, SLOT(click_radioButtonAccepted()));
    connect(ui->radioButtonRejected, SIGNAL(clicked()), this, SLOT(click_radioButtonRejected()));
    connect(ui->radioButtonExpired, SIGNAL(clicked()), this, SLOT(click_radioButtonExpired()));
    connect(ui->pushButtonCreateProposal, SIGNAL(clicked()), this , SLOT(click_pushButtonCreateProposal()));
    connect(ui->pushButtonCreatePaymentRequest, SIGNAL(clicked()), this, SLOT(click_pushButtonCreatePaymentRequest()));

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
    if(ui->radioButtonAll->isChecked()) {
        Refresh(true, viewing_proposals);
    }
    else {
        Refresh(false, viewing_proposals);
    }
}

void CommunityFundPage::deleteChildWidgets(QLayoutItem *item) {
  QLayout *layout = item->layout();
  if (layout) {
    // Process all child items recursively.
    int itemCount = ui->gridLayout->count();
    for (int i = 0; i < itemCount; i++) {
      deleteChildWidgets(ui->gridLayout->itemAt(i));
    }
  }
  delete item->widget();
}

void CommunityFundPage::reset()
{
    for (int i = ui->gridLayout->count() - 1; i >= 0; i--) {
      int r, c, rs, cs;
      ui->gridLayout->getItemPosition(i, &r, &c, &rs, &cs);
        // This layout item is subject to deletion.
        QLayoutItem *item = ui->gridLayout->takeAt(i);
        deleteChildWidgets(item);
        delete item;
      }
}
void CommunityFundPage::append(QWidget* widget)
{
    int index = ui->gridLayout->count();
    int* row = new int(0);
    int* column = new int(0);
    int* rowSpan = new int(0);
    int* columnSpan = new int(0);
    ui->gridLayout->getItemPosition(index, row, column, rowSpan,columnSpan);
    *row = int(index/2);
    *column = index%2;
    ui->gridLayout->addWidget(widget,*row,*column);
}

void CommunityFundPage::Refresh(bool all, bool proposal)
{
    reset();

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
            BOOST_FOREACH(const CFund::CProposal proposal, vec) {
                // If wanting to view all proposals
                if (all) {
                    append(new CommunityFundDisplay(0, proposal));
                }
                else {
                    // If the filter is set to my vote, filter to only pending proposals which have been voted for
                    if (viewing_voted) {
                        if (proposal.fState == CFund::NIL && proposal.GetState(pindexBestHeader->GetBlockTime()).find("expired") == string::npos) {
                            auto it = std::find_if( vAddedProposalVotes.begin(), vAddedProposalVotes.end(),
                                                    [&proposal](const std::pair<std::string, bool>& element){ return element.first == proposal.hash.ToString();} );
                            if (it != vAddedProposalVotes.end()) {
                                append(new CommunityFundDisplay(0, proposal));
                            } else {
                                continue;
                            }
                        }
                        else {
                            continue;
                        }
                    }
                    else {
                        // If the flag is expired, be sure to display proposals without the expired state if they have expired before the end of the voting cycle
                        if (proposal.fState != CFund::EXPIRED && proposal.GetState(pindexBestHeader->GetBlockTime()).find("expired") != string::npos && flag == CFund::EXPIRED) {
                            append(new CommunityFundDisplay(0, proposal));
                        }
                        // Display proposals with the appropriate flag and have not expired before the voting cycle has ended
                        if (proposal.fState != flag || (flag != CFund::EXPIRED && proposal.GetState(pindexBestHeader->GetBlockTime()).find("expired") != string::npos))
                            continue;
                        append(new CommunityFundDisplay(0, proposal));
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

                // If wanting to view all prequests
                if (all) {
                    append(new CommunityFundDisplayPaymentRequest(0, prequest));
                }
                else {
                    // If the filter is set to my vote, filter to only pending proposals which have been voted for
                    if (viewing_voted) {
                        if (prequest.fState == CFund::NIL && prequest.GetState().find("expired") == string::npos) {
                            auto it = std::find_if( vAddedPaymentRequestVotes.begin(), vAddedPaymentRequestVotes.end(),
                                                    [&prequest](const std::pair<std::string, bool>& element){ return element.first == prequest.hash.ToString();} );
                            if (it != vAddedPaymentRequestVotes.end()) {
                                append(new CommunityFundDisplayPaymentRequest(0, prequest));
                            } else {
                                continue;
                            }
                        }
                        else {
                            continue;
                        }
                    }
                    else {
                        // If the flag is expired, be sure to display prequests without the expired state if they have expired before the end of the voting cycle
                        if (prequest.fState != CFund::EXPIRED && prequest.GetState().find("expired") != string::npos && flag == CFund::EXPIRED) {
                            append(new CommunityFundDisplayPaymentRequest(0, prequest));
                        }
                        // Display prequests with the appropriate flag and have not expired before the voting cycle has ended
                        if (prequest.fState != flag || (flag != CFund::EXPIRED && prequest.GetState().find("expired") != string::npos))
                            continue;
                        append(new CommunityFundDisplayPaymentRequest(0, prequest));
                    }
                }
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

void CommunityFundPage::click_pushButtonProposals()
{
    QFont font = ui->pushButtonProposals->property("font").value<QFont>();

    ui->pushButtonProposals->setStyleSheet("QPushButton { background-color: #DBE0E8; }");
    ui->pushButtonPaymentRequests->setStyleSheet("QPushButton { background-color: #EDF0F3; }");

    QFont f(font.family(), font.pointSize(), QFont::Bold);
    ui->pushButtonProposals->setFont(f);
    ui->pushButtonPaymentRequests->setFont(f);

    viewing_proposals = true;

    if(ui->radioButtonAll->isChecked()) {
        Refresh(true, true);
    }
    else {
        Refresh(false, true);
    }
}

void CommunityFundPage::click_pushButtonPaymentRequests()
{
    QFont font = ui->pushButtonPaymentRequests->property("font").value<QFont>();

    ui->pushButtonProposals->setStyleSheet("QPushButton { background-color: #EDF0F3; }");
    ui->pushButtonPaymentRequests->setStyleSheet("QPushButton { background-color: #DBE0E8; }");

    QFont f(font.family(), font.pointSize(), QFont::Bold);
    ui->pushButtonProposals->setFont(f);
    ui->pushButtonPaymentRequests->setFont(f);

    viewing_proposals = false;

    if(ui->radioButtonAll->isChecked()) {
        Refresh(true, false);
    }
    else {
        Refresh(false, false);
    }
}

void CommunityFundPage::click_radioButtonAll()
{
    flag = CFund::NIL;
    viewing_voted = false;
    Refresh(true, viewing_proposals);
}

void CommunityFundPage::click_radioButtonYourVote()
{
    flag = CFund::NIL;
    viewing_voted = true;
    Refresh(false, viewing_proposals);
}

void CommunityFundPage::click_radioButtonPending()
{
    flag = CFund::NIL;
    viewing_voted = false;
    Refresh(false, viewing_proposals);
}

void CommunityFundPage::click_radioButtonAccepted()
{
    flag = CFund::ACCEPTED;
    viewing_voted = false;
    Refresh(false, viewing_proposals);
}

void CommunityFundPage::click_radioButtonRejected()
{
    flag = CFund::REJECTED;
    viewing_voted = false;
    Refresh(false, viewing_proposals);
}

void CommunityFundPage::click_radioButtonExpired()
{
    flag = CFund::EXPIRED;
    viewing_voted = false;
    Refresh(false, viewing_proposals);
}

void CommunityFundPage::click_pushButtonCreateProposal()
{
        CommunityFundCreateProposalDialog dlg(this);
        dlg.exec();
}

void CommunityFundPage::click_pushButtonCreatePaymentRequest()
{
    CommunityFundCreatePaymentRequestDialog dlg(this);
    dlg.exec();
}

CommunityFundPage::~CommunityFundPage()
{
    delete ui;
}
