#include "communityfundcreateproposaldialog.h"
#include "ui_communityfundcreateproposaldialog.h"
#include "sendcommunityfunddialog.h"
#include "communityfundsuccessdialog.h"

#include <QMessageBox>
#include <QTextListFormat>
#include <QDialog>

#include "guiconstants.h"
#include "guiutil.h"
#include "sync.h"
#include "wallet/wallet.h"
#include "base58.h"
#include "main.h"
#include <string>

CommunityFundCreateProposalDialog::CommunityFundCreateProposalDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CommunityFundCreateProposalDialog)
{
    ui->setupUi(this);
    GUIUtil::setupAddressWidget(ui->lineEditNavcoinAddress, this);

    ui->spinBoxDays->setRange(0, 999999999);
    ui->spinBoxHours->setRange(0, 24);
    ui->spinBoxMinutes->setRange(0, 60);

    connect(ui->pushButtonClose, SIGNAL(clicked()), this, SLOT(reject()));
    connect(ui->pushButtonCreateProposal, SIGNAL(clicked()), this, SLOT(click_pushButtonCreateProposal()));
    connect(ui->spinBoxMinutes, QOverload<int>::of(&QSpinBox::valueChanged),
            [=](int minutes){
                if(minutes == 60)
                {
                    ui->spinBoxMinutes->setValue(0);
                    ui->spinBoxHours->setValue(ui->spinBoxHours->value()+1);
                }
    });
    connect(ui->spinBoxHours, QOverload<int>::of(&QSpinBox::valueChanged),
            [=](int hours){
                if(hours == 24)
                {
                    ui->spinBoxHours->setValue(0);
                    ui->spinBoxDays->setValue(ui->spinBoxDays->value()+1);
                }
    });
}

// Validate input fields
bool CommunityFundCreateProposalDialog::validate()
{
    bool isValid = true;
    if(!ui->lineEditNavcoinAddress->isValid() || (ui->lineEditNavcoinAddress->text() == QString("")))
    {
        // Styling must be done manually as an empty field returns valid (true)
        ui->lineEditNavcoinAddress->setStyleSheet(STYLE_INVALID);
        ui->lineEditNavcoinAddress->setValid(false);
        isValid = false;
    }
    if(!ui->lineEditRequestedAmount->validate())
    {
        ui->lineEditRequestedAmount->setValid(false);
        isValid = false;
    }
    size_t desc_size = ui->plainTextEditDescription->toPlainText().toStdString().length();
    if(desc_size >= 1024 || desc_size == 0)
    {
        isValid = false;
        ui->plainTextEditDescription->setValid(false);
    }
    else
    {
        ui->plainTextEditDescription->setValid(true);
    }
    if(ui->spinBoxDays->value()*24*60*60 + ui->spinBoxHours->value()*60*60 + ui->spinBoxMinutes->value()*60 <= 0)
    {
        isValid = false;
    }

    return isValid;
}

// Q_SLOTS
bool CommunityFundCreateProposalDialog::click_pushButtonCreateProposal()
{
    if(this->validate())
    {
        LOCK2(cs_main, pwalletMain->cs_wallet);

        CNavCoinAddress address("NQFqqMUD55ZV3PJEJZtaKCsQmjLT6JkjvJ"); // Dummy address

        CWalletTx wtx;
        bool fSubtractFeeFromAmount = false;

        // Address
        string Address = ui->lineEditNavcoinAddress->text().toStdString().c_str();

        // Requested Amount
        CAmount nReqAmount = ui->lineEditRequestedAmount->value();

        // Deadline
        int64_t nDeadline = ui->spinBoxDays->value()*24*60*60 + ui->spinBoxHours->value()*60*60 + ui->spinBoxMinutes->value()*60;

        // Description
        string sDesc = ui->plainTextEditDescription->toPlainText().toStdString();

        UniValue strDZeel(UniValue::VOBJ);

        strDZeel.push_back(Pair("n",nReqAmount));
        strDZeel.push_back(Pair("a",Address));
        strDZeel.push_back(Pair("d",nDeadline));
        strDZeel.push_back(Pair("s",sDesc));
        strDZeel.push_back(Pair("v",IsReducedCFundQuorumEnabled(pindexBestHeader, Params().GetConsensus()) ? CFund::CProposal::CURRENT_VERSION : 2));

        wtx.strDZeel = strDZeel.write();
        wtx.nCustomVersion = CTransaction::PROPOSAL_VERSION;

        if(wtx.strDZeel.length() > 1024) {
            QMessageBox msgBox(this);
            std::string str = "Please shorten your description\n";
            msgBox.setText(tr(str.c_str()));
            msgBox.addButton(tr("Ok"), QMessageBox::AcceptRole);
            msgBox.setIcon(QMessageBox::Warning);
            msgBox.setWindowTitle("Description too long");
            msgBox.exec();
            return false;
        }

        // Ensure wallet is unlocked
        if (pwalletMain->IsLocked()) {
            QMessageBox msgBox(this);
            std::string str = "Please unlock the wallet\n";
            msgBox.setText(tr(str.c_str()));
            msgBox.addButton(tr("Ok"), QMessageBox::AcceptRole);
            msgBox.setIcon(QMessageBox::Warning);
            msgBox.setWindowTitle("Error");
            msgBox.exec();
            return false;
        }
        if (fWalletUnlockStakingOnly) {
            QMessageBox msgBox(this);
            std::string str = "Wallet is unlocked for staking only\n";
            msgBox.setText(tr(str.c_str()));
            msgBox.addButton(tr("Ok"), QMessageBox::AcceptRole);
            msgBox.setIcon(QMessageBox::Warning);
            msgBox.setWindowTitle("Error");
            msgBox.exec();
            return false;
        }

        // Check balance
        CAmount curBalance = pwalletMain->GetBalance();
        if (curBalance <= Params().GetConsensus().nProposalMinimalFee) {
            QMessageBox msgBox(this);
            std::string str = "You require at least 50 NAV mature and available to create a proposal\n";
            msgBox.setText(tr(str.c_str()));
            msgBox.addButton(tr("Ok"), QMessageBox::AcceptRole);
            msgBox.setIcon(QMessageBox::Warning);
            msgBox.setWindowTitle("Insufficient NAV");
            msgBox.exec();
            return false;
        }

        //create partial proposal object with all nessesary display fields from input and create confirmation dialog
        {
            CFund::CProposal *proposal = new CFund::CProposal();
            proposal->Address = Address;
            proposal->nAmount = nReqAmount;
            proposal->strDZeel = sDesc;
            proposal->nDeadline = nDeadline;

            SendCommunityFundDialog dlg(this, proposal, 10);
            if(dlg.exec() == QDialog::Rejected)
            {
                // User Declined to make the proposal
                return false;
            }
            else {

                // User accepted making the proposal
                // Parse NavCoin address
                CScript CFContributionScript;
                CScript scriptPubKey = GetScriptForDestination(address.Get());
                CFund::SetScriptForCommunityFundContribution(scriptPubKey);

                // Create and send the transaction
                CReserveKey reservekey(pwalletMain);
                CAmount nFeeRequired;
                std::string strError;
                vector<CRecipient> vecSend;
                int nChangePosRet = -1;
                CAmount nValue = Params().GetConsensus().nProposalMinimalFee;
                CRecipient recipient = {scriptPubKey, nValue, fSubtractFeeFromAmount, ""};
                vecSend.push_back(recipient);

                bool created_proposal = true;

                if (!pwalletMain->CreateTransaction(vecSend, wtx, reservekey, nFeeRequired, nChangePosRet, strError, NULL, true, "")) {
                    if (!fSubtractFeeFromAmount && nValue + nFeeRequired > pwalletMain->GetBalance()) {
                        created_proposal = false;
                    }
                }
                if (!pwalletMain->CommitTransaction(wtx, reservekey)) {
                    created_proposal = false;
                }

                // If the proposal was successfully made, confirm to the user it was made
                if (created_proposal) {
                    // Display success UI
                    CommunityFundSuccessDialog dlg(wtx.GetHash(), this, proposal);
                    dlg.exec();
                    return true;
                }
                else {
                    // Display something went wrong UI
                    QMessageBox msgBox(this);
                    std::string str = "Proposal creation failed\n";
                    msgBox.setText(tr(str.c_str()));
                    msgBox.addButton(tr("Ok"), QMessageBox::AcceptRole);
                    msgBox.setIcon(QMessageBox::Warning);
                    msgBox.setWindowTitle("Error");
                    msgBox.exec();
                    return false;
                }
            }
        }      
    }
    else
    {
        QMessageBox msgBox(this);
        std::string str = "Please enter a valid:\n";
        if(!ui->lineEditNavcoinAddress->isValid() || (ui->lineEditNavcoinAddress->text() == QString("")))
            str += "- Address\n";
        if(!ui->lineEditRequestedAmount->validate())
            str += "- Requested Amount\n";
        if(ui->plainTextEditDescription->toPlainText() == QString("") || ui->plainTextEditDescription->toPlainText().size() <= 0)
            str += "- Description\n";
        if((ui->spinBoxDays->value()*24*60*60 + ui->spinBoxHours->value()*60*60 + ui->spinBoxMinutes->value()*60) <= 0)
            str += "- Duration\n";

        msgBox.setText(tr(str.c_str()));
        msgBox.addButton(tr("Ok"), QMessageBox::AcceptRole);
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.setWindowTitle("Please enter valid fields");
        msgBox.exec();
        return false;
    }
    return true;
}

CommunityFundCreateProposalDialog::~CommunityFundCreateProposalDialog()
{
    delete ui;
}
