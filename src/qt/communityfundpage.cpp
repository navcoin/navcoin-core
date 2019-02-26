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

    // Connect push buttons to functions
    connect(ui->radioButtonAll, SIGNAL(clicked()), this, SLOT(click_radioButtonAll()));
    connect(ui->radioButtonYourVote, SIGNAL(clicked()), this, SLOT(click_radioButtonYourVote()));
    connect(ui->radioButtonPending, SIGNAL(clicked()), this, SLOT(click_radioButtonPending()));
    connect(ui->radioButtonAccepted, SIGNAL(clicked()), this, SLOT(click_radioButtonAccepted()));
    connect(ui->radioButtonRejected, SIGNAL(clicked()), this, SLOT(click_radioButtonRejected()));
    connect(ui->radioButtonExpired, SIGNAL(clicked()), this, SLOT(click_radioButtonExpired()));
    connect(ui->pushButtonCreateProposal, SIGNAL(clicked()), this , SLOT(click_pushButtonCreateProposal()));
    connect(ui->pushButtonCreatePaymentRequest, SIGNAL(clicked()), this, SLOT(click_pushButtonCreatePaymentRequest()));

    refresh(true, true);
}

void CommunityFundPage::setWalletModel(WalletModel *model)
{
    this->walletModel = model;
    refresh(true, true);
    ui->radioButtonAll->setChecked(true);
}

void CommunityFundPage::refreshTab()
{
    if(ui->radioButtonAll->isChecked())
        refresh(true, viewing_proposals);
    else
        refresh(false, viewing_proposals);
}

void CommunityFundPage::deleteChildWidgets(QLayoutItem *item) {
  QLayout *layout = item->layout();
  if (layout)
  {
    int itemCount = ui->gridLayout->count();
    for (int index = 0; index < itemCount; index++)
    {
      deleteChildWidgets(ui->gridLayout->itemAt(index));
    }
  }
  delete item->widget();
}

void CommunityFundPage::reset()
{
    for (int index = ui->gridLayout->count() - 1; index >= 0; --index)
    {
      int row, column, rowSpan, columnSpan;
      ui->gridLayout->getItemPosition(index, &row, &column, &rowSpan, &columnSpan);
        QLayoutItem *item = ui->gridLayout->takeAt(index);
        deleteChildWidgets(item);
        delete item;
    }
}

void CommunityFundPage::append(QWidget* widget)
{
    int index = ui->gridLayout->count();
    int row, column, rowSpan, columnSpan;
    ui->gridLayout->getItemPosition(index, &row, &column, &rowSpan, &columnSpan);
    row = int(index/2);
    column = index%2;
    ui->gridLayout->addWidget(widget, row, column);
}

void CommunityFundPage::refresh(bool all, bool proposal)
{
    reset();

    // Format avaliable amount in the community fund
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

    // Format locked amount in the community fund
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

    {
        int64_t spent_nav = 0;
        std::vector<CFund::CPaymentRequest> vec;
        if(pblocktree->GetPaymentRequestIndex(vec))
        {
            BOOST_FOREACH(const CFund::CPaymentRequest& prequest, vec)
            {
                if(prequest.fState == CFund::ACCEPTED)
                {
                    spent_nav = spent_nav + prequest.nAmount;
                }
            }
            stringstream s;
            s.imbue(std::locale(""));
            s << fixed << setprecision(8) << spent_nav/100000000.0;
            string spent = s.str();
            spent.erase(spent.find_last_not_of("0") + 1, std::string::npos );
            if(spent.at(spent.length()-1) == '.')
            {
                spent = spent.substr(0, spent.size()-1);
            }
            spent.append(" NAV");
            ui->labelSpentAmount->setText(QString::fromStdString(spent));
        }
    }

    // Propulate proposal grid
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
                    if (viewing_voted)
                    {
                        if (proposal.fState == CFund::NIL && proposal.GetState(pindexBestHeader->GetBlockTime()).find("expired") == string::npos)
                        {
                            auto it = std::find_if( vAddedProposalVotes.begin(), vAddedProposalVotes.end(),
                                                    [&proposal](const std::pair<std::string, bool>& element){ return element.first == proposal.hash.ToString();} );
                            if (it != vAddedProposalVotes.end())
                            {
                                append(new CommunityFundDisplay(0, proposal));
                            } else
                            {
                                continue;
                            }
                        }
                        else
                        {
                            continue;
                        }
                    }
                    else
                    {
                        // If the flag is expired, be sure to display proposals without the expired state if they have expired before the end of the voting cycle
                        if (proposal.fState != CFund::EXPIRED && proposal.GetState(pindexBestHeader->GetBlockTime()).find("expired") != string::npos && flag == CFund::EXPIRED)
                        {
                            append(new CommunityFundDisplay(0, proposal));
                        }
                        // If the proposal is accepted and waiting for funds or the end of the voting cycle, show in the accepted filter
                        if (flag == CFund::ACCEPTED && proposal.fState != CFund::ACCEPTED && proposal.GetState(pindexBestHeader->GetBlockTime()).find("accepted") != string::npos) {
                            append(new CommunityFundDisplay(0, proposal));
                        }
                        // If the proposal is rejected and waiting for funds or the end of the voting cycle, show in the rejected filter
                        if (flag == CFund::REJECTED && proposal.fState != CFund::REJECTED && proposal.GetState(pindexBestHeader->GetBlockTime()).find("rejected") != string::npos) {
                            append(new CommunityFundDisplay(0, proposal));
                        }
                        // Display proposals with the appropriate flag and have not expired before the voting cycle has ended
                        if (proposal.fState != flag || (flag != CFund::EXPIRED && proposal.GetState(pindexBestHeader->GetBlockTime()).find("expired") != string::npos ||
                                                        flag != CFund::ACCEPTED && proposal.GetState(pindexBestHeader->GetBlockTime()).find("accepted") != string::npos ||
                                                         flag != CFund::REJECTED && proposal.GetState(pindexBestHeader->GetBlockTime()).find("rejected") != string::npos))
                            continue;
                        append(new CommunityFundDisplay(0, proposal));
                    }
                }
            }
        }
    }
    else
    { //Payment request listings
        std::vector<CFund::CPaymentRequest> vec;
        if(pblocktree->GetPaymentRequestIndex(vec))
        {
            BOOST_FOREACH(const CFund::CPaymentRequest& prequest, vec) {
                // If wanting to view all prequests
                if (all)
                {
                    append(new CommunityFundDisplayPaymentRequest(0, prequest));
                }
                else
                {
                    // If the filter is set to my vote, filter to only pending proposals which have been voted for
                    if (viewing_voted)
                    {
                        if (prequest.fState == CFund::NIL && prequest.GetState().find("expired") == string::npos)
                        {
                            auto it = std::find_if( vAddedPaymentRequestVotes.begin(), vAddedPaymentRequestVotes.end(),
                                                    [&prequest](const std::pair<std::string, bool>& element){ return element.first == prequest.hash.ToString();} );
                            if (it != vAddedPaymentRequestVotes.end())
                            {
                                append(new CommunityFundDisplayPaymentRequest(0, prequest));
                            } else
                            {
                                continue;
                            }
                        }
                        else
                        {
                            continue;
                        }
                    }
                    else
                    {
                        // If the flag is expired, be sure to display prequests without the expired state if they have expired before the end of the voting cycle
                        if (prequest.fState != CFund::EXPIRED && prequest.GetState().find("expired") != string::npos && flag == CFund::EXPIRED)
                        {
                            append(new CommunityFundDisplayPaymentRequest(0, prequest));
                        }
                        // If the prequest is accepted and waiting for funds or the end of the voting cycle, show in the accepted filter
                        if (flag == CFund::ACCEPTED && prequest.fState != CFund::ACCEPTED && prequest.GetState().find("accepted") != string::npos) {
                            append(new CommunityFundDisplayPaymentRequest(0, prequest));
                        }
                        // If the prequest is rejected and waiting for funds or the end of the voting cycle, show in the rejected filter
                        if (flag == CFund::REJECTED && prequest.fState != CFund::REJECTED && prequest.GetState().find("rejected") != string::npos) {
                            append(new CommunityFundDisplayPaymentRequest(0, prequest));
                        }
                        // Display prequests with the appropriate flag and have not expired before the voting cycle has ended
                        if (prequest.fState != flag || (flag != CFund::EXPIRED && prequest.GetState().find("expired") != string::npos ||
                                                        flag != CFund::ACCEPTED && prequest.GetState().find("accepted") != string::npos ||
                                                        flag != CFund::REJECTED && prequest.GetState().find("rejected") != string::npos))
                            continue;
                        }
                        append(new CommunityFundDisplayPaymentRequest(0, prequest));
                    }
                }
            }
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

    refresh(ui->radioButtonAll->isChecked(), true);
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

    refresh(ui->radioButtonAll->isChecked(), false);
}

void CommunityFundPage::click_radioButtonAll()
{
    flag = CFund::NIL;
    viewing_voted = false;
    refresh(true, viewing_proposals);
}

void CommunityFundPage::click_radioButtonYourVote()
{
    flag = CFund::NIL;
    viewing_voted = true;
    refresh(false, viewing_proposals);
}

void CommunityFundPage::click_radioButtonPending()
{
    flag = CFund::NIL;
    viewing_voted = false;
    refresh(false, viewing_proposals);
}

void CommunityFundPage::click_radioButtonAccepted()
{
    flag = CFund::ACCEPTED;
    viewing_voted = false;
    refresh(false, viewing_proposals);
}

void CommunityFundPage::click_radioButtonRejected()
{
    flag = CFund::REJECTED;
    viewing_voted = false;
    refresh(false, viewing_proposals);
}

void CommunityFundPage::click_radioButtonExpired()
{
    flag = CFund::EXPIRED;
    viewing_voted = false;
    refresh(false, viewing_proposals);
}

void CommunityFundPage::click_pushButtonCreateProposal()
{
    CommunityFundCreateProposalDialog dlg(this);
    dlg.exec();  
    refresh(ui->radioButtonAll->isChecked(), true);
}

void CommunityFundPage::click_pushButtonCreatePaymentRequest()
{
    CommunityFundCreatePaymentRequestDialog dlg(this);
    dlg.exec();
    refresh(ui->radioButtonAll->isChecked(), false);
}

CommunityFundPage::~CommunityFundPage()
{
    delete ui;
}
