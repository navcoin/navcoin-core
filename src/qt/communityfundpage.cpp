#include <qt/communityfundpage.h>
#include <qt/forms/ui_communityfundpage.h>
#include <qt/communityfundpage.moc>

#include <main.h>
#include <txdb.h>
#include <wallet/wallet.h>
#include <string>
#include <iomanip>
#include <sstream>
#include <QAbstractScrollArea>

#include <qt/communityfunddisplay.h>
#include <qt/communityfunddisplaypaymentrequest.h>
#include <qt/communityfundcreateproposaldialog.h>
#include <qt/communityfundcreatepaymentrequestdialog.h>

CommunityFundPage::CommunityFundPage(const PlatformStyle *platformStyle, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::CommunityFundPage),
    clientModel(0),
    walletModel(0),
    flag(CFund::NIL),
    viewing_proposals(true),
    viewing_voted(false),
    viewing_unvoted(false)
{
    ui->setupUi(this);

    // Hide horizontal scrollArea scroll bar
    ui->scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    connect(ui->pushButtonProposals, SIGNAL(clicked()), this, SLOT(click_pushButtonProposals()));
    connect(ui->pushButtonPaymentRequests, SIGNAL(clicked()), this, SLOT(click_pushButtonPaymentRequests()));

    // Enable selection of pushButtonProposals by default
    ui->pushButtonProposals->setStyleSheet(BTN_ACTIVE);
    ui->pushButtonPaymentRequests->setStyleSheet(BTN_NORMAL);

    // Connect push buttons to functions
    connect(ui->radioButtonAll, SIGNAL(clicked()), this, SLOT(click_radioButtonAll()));
    connect(ui->radioButtonYourVote, SIGNAL(clicked()), this, SLOT(click_radioButtonYourVote()));
    connect(ui->radioButtonPending, SIGNAL(clicked()), this, SLOT(click_radioButtonPending()));
    connect(ui->radioButtonAccepted, SIGNAL(clicked()), this, SLOT(click_radioButtonAccepted()));
    connect(ui->radioButtonRejected, SIGNAL(clicked()), this, SLOT(click_radioButtonRejected()));
    connect(ui->radioButtonExpired, SIGNAL(clicked()), this, SLOT(click_radioButtonExpired()));
    connect(ui->radioButtonNoVote, SIGNAL(clicked()), this, SLOT(click_radioButtonNoVote()));
    connect(ui->pushButtonCreateProposal, SIGNAL(clicked()), this , SLOT(click_pushButtonCreateProposal()));
    connect(ui->pushButtonCreatePaymentRequest, SIGNAL(clicked()), this, SLOT(click_pushButtonCreatePaymentRequest()));

    click_radioButtonPending();
    refresh(false, true);
}

void CommunityFundPage::setWalletModel(WalletModel *model)
{
    this->walletModel = model;
    flag = CFund::NIL;
    viewing_voted = false;
    viewing_unvoted = true;
    refresh(false, true);
    ui->radioButtonNoVote->setChecked(true);
}

void CommunityFundPage::refreshTab()
{
    refresh(ui->radioButtonAll->isChecked(), viewing_proposals);
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
    string available;
    available = wallet->formatDisplayAmount(pindexBestHeader->nCFSupply);
    ui->labelAvailableAmount->setText(QString::fromStdString(available));

    // Format locked amount in the community fund
    string locked;
    locked = wallet->formatDisplayAmount(pindexBestHeader->nCFLocked);
    ui->labelLockedAmount->setText(QString::fromStdString(locked));

    {
        int64_t spent_nav = 0;
        CPaymentRequestMap mapPaymentRequests;

        if(pcoinsTip->GetAllPaymentRequests(mapPaymentRequests))
        {
            for (CPaymentRequestMap::iterator it_ = mapPaymentRequests.begin(); it_ != mapPaymentRequests.end(); it_++)
            {
                CFund::CPaymentRequest prequest;

                if (!pcoinsTip->GetPaymentRequest(it_->first, prequest))
                    continue;

                if(prequest.GetLastState() == CFund::ACCEPTED)
                {
                    spent_nav = spent_nav + prequest.nAmount;
                }
            }

            string spent;
            spent = wallet->formatDisplayAmount(spent_nav);
            ui->labelSpentAmount->setText(QString::fromStdString(spent));
        }
    }

    // Propulate proposal grid
    if (proposal) {
        CProposalMap mapProposals;

        if(pcoinsTip->GetAllProposals(mapProposals))
        {
            for (CProposalMap::iterator it = mapProposals.begin(); it != mapProposals.end(); it++)
            {
                CFund::CProposal proposal;
                if (!pcoinsTip->GetProposal(it->first, proposal))
                    continue;

                // If wanting to view all proposals
                if (all) {
                    append(new CommunityFundDisplay(0, proposal));
                }
                else {
                    flags fLastState = proposal.GetLastState();
                    // If the filter is set to my vote, filter to only pending proposals which have been voted for
                    if (viewing_voted)
                    {
                        if (fLastState == CFund::NIL && proposal.GetState(pindexBestHeader->GetBlockTime()).find("expired") == string::npos)
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
                    // If the filter is set to no vote, filter to only pending proposals which have been not been voted for yet
                    else if (viewing_unvoted)
                    {
                        if (fLastState == CFund::NIL && proposal.GetState(pindexBestHeader->GetBlockTime()).find("expired") == string::npos)
                        {
                            auto it = std::find_if( vAddedProposalVotes.begin(), vAddedProposalVotes.end(),
                                                    [&proposal](const std::pair<std::string, bool>& element){ return element.first == proposal.hash.ToString();} );
                            if (it != vAddedProposalVotes.end())
                            {
                                continue;
                            } else
                            {
                                append(new CommunityFundDisplay(0, proposal));
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
                        if (fLastState != CFund::EXPIRED && proposal.GetState(pindexBestHeader->GetBlockTime()).find("expired") != string::npos && flag == CFund::EXPIRED)
                        {
                            append(new CommunityFundDisplay(0, proposal));
                        }
                        // If the proposal is accepted and waiting for funds or the end of the voting cycle, show in the accepted filter
                        if (flag == CFund::ACCEPTED && fLastState != CFund::ACCEPTED && proposal.GetState(pindexBestHeader->GetBlockTime()).find("accepted") != string::npos) {
                            append(new CommunityFundDisplay(0, proposal));
                        }
                        // If the proposal is rejected and waiting for funds or the end of the voting cycle, show in the rejected filter
                        if (flag == CFund::REJECTED && fLastState != CFund::REJECTED && proposal.GetState(pindexBestHeader->GetBlockTime()).find("rejected") != string::npos) {
                            append(new CommunityFundDisplay(0, proposal));
                        }
                        // Display proposals with the appropriate flag and have not expired before the voting cycle has ended
                        if (fLastState != flag || ((flag != CFund::EXPIRED && proposal.GetState(pindexBestHeader->GetBlockTime()).find("expired") != string::npos) ||
                                                        (flag != CFund::ACCEPTED && proposal.GetState(pindexBestHeader->GetBlockTime()).find("accepted") != string::npos) ||
                                                         (flag != CFund::REJECTED && proposal.GetState(pindexBestHeader->GetBlockTime()).find("rejected") != string::npos)))
                            continue;
                        append(new CommunityFundDisplay(0, proposal));
                    }
                }
            }
        }
    }
    else
    { //Payment request listings
        CPaymentRequestMap mapPaymentRequests;

        if(pcoinsTip->GetAllPaymentRequests(mapPaymentRequests))
        {
            for (CPaymentRequestMap::iterator it_ = mapPaymentRequests.begin(); it_ != mapPaymentRequests.end(); it_++)
            {
                CFund::CPaymentRequest prequest;

                if (!pcoinsTip->GetPaymentRequest(it_->first, prequest))
                    continue;

                // If wanting to view all prequests
                if (all)
                {
                    append(new CommunityFundDisplayPaymentRequest(0, prequest));
                }
                else
                {
                    flags fLastState = prequest.GetLastState();
                    // If the filter is set to my vote, filter to only pending prequests which have been voted for
                    if (viewing_voted)
                    {
                        if (fLastState == CFund::NIL && prequest.GetState().find("expired") == string::npos)
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
                    // If the filter is set to no vote, filter to only pending prequests which have not been voted for yet
                    else if (viewing_unvoted)
                    {
                        if (fLastState == CFund::NIL && prequest.GetState().find("expired") == string::npos)
                        {
                            auto it = std::find_if( vAddedPaymentRequestVotes.begin(), vAddedPaymentRequestVotes.end(),
                                                    [&prequest](const std::pair<std::string, bool>& element){ return element.first == prequest.hash.ToString();} );
                            if (it != vAddedPaymentRequestVotes.end())
                            {
                                continue;
                            } else
                            {
                                append(new CommunityFundDisplayPaymentRequest(0, prequest));
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
                        if (fLastState != CFund::EXPIRED && prequest.GetState().find("expired") != string::npos && flag == CFund::EXPIRED)
                        {
                            append(new CommunityFundDisplayPaymentRequest(0, prequest));
                        }
                        // If the prequest is accepted and waiting for funds or the end of the voting cycle, show in the accepted filter
                        if (flag == CFund::ACCEPTED && fLastState != CFund::ACCEPTED && prequest.GetState().find("accepted") != string::npos) {
                            append(new CommunityFundDisplayPaymentRequest(0, prequest));
                        }
                        // If the prequest is rejected and waiting for funds or the end of the voting cycle, show in the rejected filter
                        if (flag == CFund::REJECTED && fLastState != CFund::REJECTED && prequest.GetState().find("rejected") != string::npos) {
                            append(new CommunityFundDisplayPaymentRequest(0, prequest));
                        }
                        // Display prequests with the appropriate flag and have not expired before the voting cycle has ended
                        if (fLastState != flag || ((flag != CFund::EXPIRED && prequest.GetState().find("expired") != string::npos) ||
                                                        (flag != CFund::ACCEPTED && prequest.GetState().find("accepted") != string::npos) ||
                                                        (flag != CFund::REJECTED && prequest.GetState().find("rejected") != string::npos)))
                            continue;
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

    ui->pushButtonProposals->setStyleSheet(BTN_ACTIVE);
    ui->pushButtonPaymentRequests->setStyleSheet(BTN_NORMAL);

    QFont f(font.family(), font.pointSize(), QFont::Normal);
    ui->pushButtonProposals->setFont(f);
    ui->pushButtonPaymentRequests->setFont(f);

    viewing_proposals = true;
    refresh(ui->radioButtonAll->isChecked(), viewing_proposals);
}

void CommunityFundPage::click_pushButtonPaymentRequests()
{
    QFont font = ui->pushButtonProposals->property("font").value<QFont>();

    ui->pushButtonProposals->setStyleSheet(BTN_NORMAL);
    ui->pushButtonPaymentRequests->setStyleSheet(BTN_ACTIVE);

    QFont f(font.family(), font.pointSize(), QFont::Normal);
    ui->pushButtonProposals->setFont(f);
    ui->pushButtonPaymentRequests->setFont(f);

    viewing_proposals = false;
    refresh(ui->radioButtonAll->isChecked(), viewing_proposals);
}

void CommunityFundPage::click_radioButtonAll()
{
    flag = CFund::NIL;
    viewing_voted = false;
    viewing_unvoted = false;
    refresh(true, viewing_proposals);
}

void CommunityFundPage::click_radioButtonYourVote()
{
    flag = CFund::NIL;
    viewing_voted = true;
    viewing_unvoted = false;
    refresh(false, viewing_proposals);
}

void CommunityFundPage::click_radioButtonPending()
{
    flag = CFund::NIL;
    viewing_voted = false;
    viewing_unvoted = false;
    refresh(false, viewing_proposals);
}

void CommunityFundPage::click_radioButtonAccepted()
{
    flag = CFund::ACCEPTED;
    viewing_voted = false;
    viewing_unvoted = false;
    refresh(false, viewing_proposals);
}

void CommunityFundPage::click_radioButtonRejected()
{
    flag = CFund::REJECTED;
    viewing_voted = false;
    viewing_unvoted = false;
    refresh(false, viewing_proposals);
}

void CommunityFundPage::click_radioButtonExpired()
{
    flag = CFund::EXPIRED;
    viewing_voted = false;
    viewing_unvoted = false;
    refresh(false, viewing_proposals);
}

void CommunityFundPage::click_pushButtonCreateProposal()
{
    if (!Params().GetConsensus().fDaoClientActivated)
    {
        QMessageBox msgBox(this);
        QString str = tr("This function is temporarily disabled");
        msgBox.setText(str);
        msgBox.addButton(tr("Ok"), QMessageBox::AcceptRole);
        msgBox.setIcon(QMessageBox::Information);
        msgBox.setWindowTitle(str);
        msgBox.exec();
        return;
    }
    CommunityFundCreateProposalDialog dlg(this);
    dlg.setModel(walletModel);
    dlg.exec();
    refresh(ui->radioButtonAll->isChecked(), viewing_proposals);
}

void CommunityFundPage::click_pushButtonCreatePaymentRequest()
{
    if (!Params().GetConsensus().fDaoClientActivated)
    {
        QMessageBox msgBox(this);
        QString str = tr("This function is temporarily disabled");
        msgBox.setText(str);
        msgBox.addButton(tr("Ok"), QMessageBox::AcceptRole);
        msgBox.setIcon(QMessageBox::Information);
        msgBox.setWindowTitle(str);
        msgBox.exec();
        return;
    }
    CommunityFundCreatePaymentRequestDialog dlg(this);
    dlg.setModel(walletModel);
    dlg.exec();
    refresh(ui->radioButtonAll->isChecked(), viewing_proposals);
}

void CommunityFundPage::click_radioButtonNoVote()
{
    flag = CFund::NIL;
    viewing_voted = false;
    viewing_unvoted = true;
    refresh(false, viewing_proposals);
}

CommunityFundPage::~CommunityFundPage()
{
    delete ui;
}
