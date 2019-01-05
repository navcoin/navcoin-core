#include "cfund_voting.h"

#include "txdb.h"
#include "consensus/cfund.h"
#include "main.h"

CFund_Voting::CFund_Voting(QWidget *parent, bool fPaymentRequests) :
    QDialog(parent),
    ui(new Ui::CFund_Voting),
    fSettings(fPaymentRequests)
{
    ui->setupUi(this);

    connect(ui->closeBtn, SIGNAL(clicked(bool)), this, SLOT(closeDialog()));
    connect(ui->voteyesBtn, SIGNAL(clicked(bool)), this, SLOT(voteYes()));
    connect(ui->votenoBtn, SIGNAL(clicked(bool)), this, SLOT(voteNo()));
    connect(ui->stopvotingBtn, SIGNAL(clicked(bool)), this, SLOT(stopVoting()));
    connect(ui->viewdetailsBtn, SIGNAL(clicked(bool)), this, SLOT(viewDetails()));
    connect(ui->switchBtn, SIGNAL(clicked(bool)), this, SLOT(switchView()));
    connect(ui->votingyesList, SIGNAL(itemPressed(QListWidgetItem*)), this, SLOT(selectedFromYes(QListWidgetItem*)));
    connect(ui->votingnoList, SIGNAL(itemPressed(QListWidgetItem*)), this, SLOT(selectedFromNo(QListWidgetItem*)));
    connect(ui->notvotingList, SIGNAL(itemPressed(QListWidgetItem*)), this, SLOT(selectedFromNotVoting(QListWidgetItem*)));
    Refresh();
}

void CFund_Voting::closeDialog() {
    close();
}

void CFund_Voting::voteYes() {
    if (selected == "")
        return;
    bool d = false;
    if (!fSettings) {
        CFund::VoteProposal(selected.toStdString(), true, d);
    } else {
        CFund::VotePaymentRequest(selected.toStdString(), true, d);
    }
    setSelection("");
    Refresh();
}

void CFund_Voting::voteNo() {
    if (selected == "")
        return;
    bool d = false;
    if (!fSettings) {
        CFund::VoteProposal(selected.toStdString(), false, d);
    } else {
        CFund::VotePaymentRequest(selected.toStdString(), false, d);
    }
    setSelection("");
    Refresh();
}

void CFund_Voting::stopVoting() {
    if (selected == "")
        return;
    if (!fSettings) {
        CFund::RemoveVoteProposal(selected.toStdString());
    } else {
        CFund::RemoveVotePaymentRequest(selected.toStdString());
    }
    setSelection("");
    Refresh();
}

void CFund_Voting::viewDetails() {
    if (selected == "")
        return;
    QString link = QString("https://www.navexplorer.com/community-fund/") + (fSettings ? QString("payment-request") : QString("proposal")) + QString("/") + selected;
    QDesktopServices::openUrl(QUrl(link));
}

void CFund_Voting::switchView() {
    setSelection("");
    fSettings = !fSettings;
    Refresh();
}

void CFund_Voting::selectedFromYes(QListWidgetItem* item) {
   ui->notvotingList->clearSelection();
   ui->votingnoList->clearSelection();

   setSelection(item->data(1).toString());
   ui->voteyesBtn->setEnabled(false);
}

void CFund_Voting::selectedFromNo(QListWidgetItem* item) {
   ui->notvotingList->clearSelection();
   ui->votingyesList->clearSelection();

   setSelection(item->data(1).toString());
   ui->votenoBtn->setEnabled(false);
}

void CFund_Voting::selectedFromNotVoting(QListWidgetItem* item) {
    ui->votingnoList->clearSelection();
    ui->votingyesList->clearSelection();

    setSelection(item->data(1).toString());
    ui->stopvotingBtn->setEnabled(false);
}

void CFund_Voting::setSelection(QString selection) {
    selected = selection;
    enableDisableButtons();
}

void CFund_Voting::enableDisableButtons() {

    if(selected == "") {
        ui->voteyesBtn->setEnabled(false);
        ui->votenoBtn->setEnabled(false);
        ui->stopvotingBtn->setEnabled(false);
        ui->viewdetailsBtn->setEnabled(false);
    } else {
        ui->viewdetailsBtn->setEnabled(true);
        ui->voteyesBtn->setEnabled(true);
        ui->votenoBtn->setEnabled(true);
        ui->stopvotingBtn->setEnabled(true);
    }
}

void CFund_Voting::Refresh()
{
    setWindowTitle(tr("Community Fund: ") + (fSettings ? tr("Payment Requests") : tr("Proposals")));

    ui->votingnoList->clear();
    ui->votingyesList->clear();
    ui->notvotingList->clear();
    ui->switchBtn->setText(fSettings ? tr("Switch to Proposal View") : tr("Switch to Payment Request View"));
    ui->viewdetailsBtn->setText(fSettings ? tr("View Payment Request Details") : tr("View Proposal Details"));
    ui->windowMainTitle->setText(fSettings ? tr("Payment Request Voting") : tr("Proposal Voting"));

    enableDisableButtons();

    int nCount = 0;

    {
    std::vector<CFund::CProposal> vec;
    if(pblocktree->GetProposalIndex(vec))
    {
        BOOST_FOREACH(const CFund::CProposal& proposal, vec) {
            if (proposal.fState != CFund::NIL)
                continue;
            QListWidget* whereToAdd = ui->notvotingList;
            auto it = std::find_if( vAddedProposalVotes.begin(), vAddedProposalVotes.end(),
                                    [&proposal](const std::pair<std::string, bool>& element){ return element.first == proposal.hash.ToString();} );
            if (it != vAddedProposalVotes.end()) {
                if (it->second)
                    whereToAdd = ui->votingyesList;
                else
                    whereToAdd = ui->votingnoList;
            }
            if (!fSettings)
            {
                QListWidgetItem *item = new QListWidgetItem(QString::fromStdString(proposal.strDZeel), whereToAdd);
                item->setData(1, QString::fromStdString(proposal.hash.ToString()));
            } else {
                if (whereToAdd == ui->notvotingList)
                    nCount++;
            }
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
            QListWidget* whereToAdd = ui->notvotingList;
            auto it = std::find_if( vAddedPaymentRequestVotes.begin(), vAddedPaymentRequestVotes.end(),
                                    [&prequest](const std::pair<std::string, int>& element){ return element.first == prequest.hash.ToString();} );
            if (it != vAddedPaymentRequestVotes.end()) {
                if (it->second)
                    whereToAdd = ui->votingyesList;
                else
                    whereToAdd = ui->votingnoList;
            }
            if (fSettings)
            {
                QListWidgetItem *item = new QListWidgetItem(QString::fromStdString(prequest.strDZeel), whereToAdd);
                item->setData(1, QString::fromStdString(prequest.hash.ToString()));
            } else {
                if (whereToAdd == ui->notvotingList)
                    nCount++;
            }
        }
    }
    }

    if (nCount > 0)
        ui->otherViewLabel->setText(fSettings ? tr("There are %1 new Proposals on the other view.").arg(nCount) : tr("There are %1 new Payment Requests on the other view.").arg(nCount));
    else
        ui->otherViewLabel->setText("");

}

CFund_Voting::~CFund_Voting()
{
    delete ui;
}
