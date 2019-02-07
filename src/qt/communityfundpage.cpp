#include "communityfundpage.h"
#include "ui_communityfundpage.h"
#include <QAbstractScrollArea>
#include "communityfundpage.moc"
#include <iostream>
#include "main.h"
#include "txdb.h"
#include "consensus/cfund.h"

CommunityFundPage::CommunityFundPage(const PlatformStyle *platformStyle, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::CommunityFundPage),
    clientModel(0),
    walletModel(0)
{
    ui->setupUi(this);

    // Hide scrollArea scroll bar
    ui->scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    connect(ui->pushButtonProposals, SIGNAL(clicked()), this, SLOT(on_click_pushButtonProposals()));
    connect(ui->pushButtonPaymentRequests, SIGNAL(clicked()), this, SLOT(on_click_pushButtonPaymentRequests()));

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
            if (proposal.fState != CFund::NIL)
                continue;
            std::cout << "got";
        }
    }
    }

    {
    std::vector<CFund::CPaymentRequest> vec;
    if(pblocktree->GetPaymentRequestIndex(vec))
    {
        BOOST_FOREACH(const CFund::CPaymentRequest& prequest, vec) {
            if (prequest.fState != CFund::NIL)
                continue;
            std::cout << "got";
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

CommunityFundPage::~CommunityFundPage()
{
    delete ui;
}
