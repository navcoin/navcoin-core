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

CommunityFundPage::CommunityFundPage(const PlatformStyle *platformStyle, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::CommunityFundPage),
    clientModel(0),
    walletModel(0),
    flag(CFund::NIL)
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
    connect(ui->radioButtonYourVote, SIGNAL(clicked()), this, SLOT(on_click_radioButtonAll()));
    connect(ui->radioButtonPending, SIGNAL(clicked()), this, SLOT(on_click_radioButtonAll()));
    connect(ui->radioButtonAccepted, SIGNAL(clicked()), this, SLOT(on_click_radioButtonAll()));
    connect(ui->radioButtonRejected, SIGNAL(clicked()), this, SLOT(on_click_radioButtonAll()));

    //fetch cfund info
    Refresh();
}

void CommunityFundPage::Refresh()
{
    stringstream a;
    a << fixed << setprecision(2) << pindexBestHeader->nCFSupply;
    string available = a.str();
    available.append(" NAV");
    ui->labelAvailableAmount->setText(QString::fromStdString(available));

    stringstream l;
    l << fixed << setprecision(2) << pindexBestHeader->nCFLocked;
    string locked = l.str();
    locked.append(" NAV");
    ui->labelLockedAmount->setText(QString::fromStdString(locked));

    {
    std::vector<CFund::CProposal> vec;
    if(pblocktree->GetProposalIndex(vec))
    {
        BOOST_FOREACH(const CFund::CProposal& proposal, vec) {
            if (proposal.fState != flag)
                continue;
            std::cout << proposal.fState << "\n";
        }
    }
    }

    {
    std::vector<CFund::CPaymentRequest> vec;
    if(pblocktree->GetPaymentRequestIndex(vec))
    {
        BOOST_FOREACH(const CFund::CPaymentRequest& prequest, vec) {
            if (prequest.fState != flag)
                continue;
            std::cout << prequest.fState << "\n";
        }
    }
    }
}

void CommunityFundPage::on_click_pushButtonProposals()
{
    ui->pushButtonProposals->setStyleSheet("QPushButton { background-color: #DBE0E8; }");
    ui->pushButtonPaymentRequests->setStyleSheet("QPushButton { background-color: #EDF0F3; }");
}

void CommunityFundPage::on_click_pushButtonPaymentRequests()
{
    ui->pushButtonProposals->setStyleSheet("QPushButton { background-color: #EDF0F3; }");
    ui->pushButtonPaymentRequests->setStyleSheet("QPushButton { background-color: #DBE0E8; }");
}

void CommunityFundPage::on_click_radioButtonAll()
{
    flag = CFund::NIL;
}

void CommunityFundPage::on_click_radioButtonYourVote()
{
    flag = CFund::NIL;
}

void CommunityFundPage::on_click_radioButtonPending()
{
    flag = CFund::PENDING_VOTING_PREQ;
}

void CommunityFundPage::on_click_radioButtonAccepted()
{
    flag = CFund::ACCEPTED;
}

void CommunityFundPage::on_click_radioButtonRejected()
{
    flag = CFund::REJECTED;
}

CommunityFundPage::~CommunityFundPage()
{
    delete ui;
}
