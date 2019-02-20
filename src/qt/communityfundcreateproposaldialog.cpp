#include "communityfundcreateproposaldialog.h"
#include "ui_communityfundcreateproposaldialog.h"
#include "sendcommunityfunddialog.h"

#include <QMessageBox>
#include <QTextListFormat>
#include <QDialog>

#include "guiconstants.h"
#include "guiutil.h"
#include "sync.h"
#include "wallet/wallet.h"
#include "base58.h"
#include "main.h"
#include "wallet/rpcwallet.h"
#include "wallet/rpcwallet.cpp"
#include <string>

CommunityFundCreateProposalDialog::CommunityFundCreateProposalDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CommunityFundCreateProposalDialog)
{
    ui->setupUi(this);
    GUIUtil::setupAddressWidget(ui->lineEditNavcoinAddress, this);

    ui->spinBoxDays->setRange(0, 999999999);
    ui->spinBoxHours->setRange(0, 59);
    ui->spinBoxMinutes->setRange(0, 59);

    connect(ui->pushButtonClose, SIGNAL(clicked()), this, SLOT(reject()));
    connect(ui->pushButtonCreateProposal, SIGNAL(clicked()), this, SLOT(on_click_pushButtonCreateProposal()));
}

// Validate input fields
bool CommunityFundCreateProposalDialog::validate()
{
    bool isValid = true;
    if(!ui->lineEditNavcoinAddress->isValid() || (ui->lineEditNavcoinAddress->text() == QString("")))
    {
        // styling must be done manually as an empty field returns valid (true)
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
bool CommunityFundCreateProposalDialog::on_click_pushButtonCreateProposal()
{
    if(this->validate())
    {
        LOCK2(cs_main, pwalletMain->cs_wallet);

        CNavCoinAddress address("NQFqqMUD55ZV3PJEJZtaKCsQmjLT6JkjvJ"); // Dummy address
        CWalletTx wtx;
        bool fSubtractFeeFromAmount = false;

        std::string Address = ui->lineEditNavcoinAddress->text().toStdString().c_str();

        CNavCoinAddress destaddress(Address);
        if (!destaddress.IsValid())
            return false;

        CAmount nReqAmount = ui->lineEditRequestedAmount->value();
        int64_t nDeadline = ui->spinBoxDays->value()*24*60*60 + ui->spinBoxHours->value()*60*60 + ui->spinBoxMinutes->value()*60;

        if(nDeadline <= 0)
            return false;

        string sDesc = ui->plainTextEditDescription->toPlainText().toStdString();

        UniValue strDZeel(UniValue::VOBJ);

        strDZeel.push_back(Pair("n",nReqAmount));
        strDZeel.push_back(Pair("a",Address));
        strDZeel.push_back(Pair("d",nDeadline));
        strDZeel.push_back(Pair("s",sDesc));
        strDZeel.push_back(Pair("v",IsReducedCFundQuorumEnabled(pindexBestHeader, Params().GetConsensus()) ? CFund::CProposal::CURRENT_VERSION : 2));

        wtx.strDZeel = strDZeel.write();
        wtx.nCustomVersion = CTransaction::PROPOSAL_VERSION;

        if(wtx.strDZeel.length() > 1024)
            return false;


        EnsureWalletIsUnlocked();

        bool donate = true;
        CAmount curBalance = pwalletMain->GetBalance();

        if (nReqAmount <= 0)
            return false;

        if (nReqAmount > curBalance)
            return false;

        // Parse NavCoin address (currently crashes wallet)
        CScript scriptPubKey = GetScriptForDestination(address.Get());

        if(donate)
          CFund::SetScriptForCommunityFundContribution(scriptPubKey);

        // Create and send the transaction
        CReserveKey reservekey(pwalletMain);
        CAmount nFeeRequired;
        std::string strError;
        vector<CRecipient> vecSend;
        int nChangePosRet = -1;
        CRecipient recipient = {scriptPubKey, nReqAmount, fSubtractFeeFromAmount, ""};
        vecSend.push_back(recipient);


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
        }

        // User accepted making the proposal
        // This boolean is a placeholder for the logic that creates the proposal and detects whether it was created successfully
        bool created_proposal = true;

        // If the proposal was successfully made, confirm to the user it was made
        if (created_proposal) {
            // Display success UI
            return true;
        }
        else {
            // Display something went wrong UI, ie does not have the 50 NAV to create the proposal
            return false;
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
        msgBox.exec();
        return false;
    }
    return true;
}

CommunityFundCreateProposalDialog::~CommunityFundCreateProposalDialog()
{
    delete ui;
}
