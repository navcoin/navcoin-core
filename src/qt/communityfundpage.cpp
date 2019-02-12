#include "communityfundpage.h"
#include "ui_communityfundpage.h"
#include <QAbstractScrollArea>
#include "communityfundpage.moc"
#include <iostream>
#include "main.h"
#include "txdb.h"
#include <string>
#include "communityfunddisplay.h"
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
    connect(ui->radioButtonYourVote, SIGNAL(clicked()), this, SLOT(on_click_radioButtonYourVote()));
    connect(ui->radioButtonPending, SIGNAL(clicked()), this, SLOT(on_click_radioButtonPending()));
    connect(ui->radioButtonAccepted, SIGNAL(clicked()), this, SLOT(on_click_radioButtonAccepted()));
    connect(ui->radioButtonRejected, SIGNAL(clicked()), this, SLOT(on_click_radioButtonRejected()));
    connect(ui->radioButtonExpired, SIGNAL(clicked()), this, SLOT(on_click_radioButtonExpired()));

    //fetch cfund info
    Refresh(true);
}

void CommunityFundPage::Refresh(bool all)
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
    a << fixed << setprecision(8) << pindexBestHeader->nCFSupply/100000000.0;
    string available = a.str();
    available.append(" NAV");
    ui->labelAvailableAmount->setText(QString::fromStdString(available));

    stringstream l;
    l << fixed << setprecision(8) << pindexBestHeader->nCFLocked/100000000.0;
    string locked = l.str();
    locked.append(" NAV");
    ui->labelLockedAmount->setText(QString::fromStdString(locked));

    {
    std::vector<CFund::CProposal> vec;
    if(pblocktree->GetProposalIndex(vec))
    {
        int r = 0;
        int c = 0;
        BOOST_FOREACH(const CFund::CProposal proposal, vec) {
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
                if (proposal.fState != flag)
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
        s << fixed << setprecision(8) << spent_nav/100000000.0;
        string spent = s.str();
        spent.append(" NAV");
        ui->labelSpentAmount->setText(QString::fromStdString(spent));
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
    Refresh(true);
}

void CommunityFundPage::on_click_radioButtonYourVote()
{
    flag = CFund::NIL;
    Refresh(true);
}

void CommunityFundPage::on_click_radioButtonPending()
{
    flag = CFund::NIL;
    Refresh(false);
}

void CommunityFundPage::on_click_radioButtonAccepted()
{
    flag = CFund::ACCEPTED;
    Refresh(false);
}

void CommunityFundPage::on_click_radioButtonRejected()
{
    flag = CFund::REJECTED;
    Refresh(false);
}

void CommunityFundPage::on_click_radioButtonExpired()
{
    flag = CFund::EXPIRED;
    Refresh(false);
}

CommunityFundPage::~CommunityFundPage()
{
    delete ui;
}
