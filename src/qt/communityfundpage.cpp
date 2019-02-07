#include "communityfundpage.h"
#include "ui_communityfundpage.h"
#include <QAbstractScrollArea>
#include "communityfundpage.moc"
#include <iostream>
#include "main.h"
#include "txdb.h"

CommunityFundPage::CommunityFundPage(const PlatformStyle *platformStyle, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::CommunityFundPage),
    clientModel(0),
    walletModel(0),
    flag(CFund::NIL)
{
    ui->setupUi(this);

    // Hide scrollArea scroll bar
    ui->scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    connect(ui->pushButtonProposals, SIGNAL(clicked()), this, SLOT(on_click_pushButtonProposals()));
    connect(ui->pushButtonPaymentRequests, SIGNAL(clicked()), this, SLOT(on_click_pushButtonPaymentRequests()));
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
    ui->pushButtonPaymentRequests->setEnabled(false);
}

void CommunityFundPage::on_click_pushButtonPaymentRequests()
{
    ui->pushButtonProposals->setEnabled(false);
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
