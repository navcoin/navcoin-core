#include <qt/communityfundcreateproposaldialog.h>
#include <qt/communityfundsuccessdialog.h>
#include <qt/optionsmodel.h>
#include <qt/sendcommunityfunddialog.h>
#include <ui_communityfundcreateproposaldialog.h>

#include <QDialog>
#include <QMessageBox>
#include <QSpinBox>
#include <QTextListFormat>
#include <string>

#include <base58.h>
#include <qt/guiconstants.h>
#include <qt/guiutil.h>
#include <main.h>
#include <qt/qvalidatedspinbox.h>
#include <sync.h>
#include <wallet/wallet.h>
#include <qt/walletmodel.h>

CommunityFundCreateProposalDialog::CommunityFundCreateProposalDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CommunityFundCreateProposalDialog),
    model(0)
{
    ui->setupUi(this);
    GUIUtil::setupAddressWidget(ui->lineEditNavcoinAddress, this);

    CStateViewCache view(pcoinsTip);

    ui->spinBoxDays->setRange(0, 999999999);
    ui->spinBoxHours->setRange(0, 24);
    ui->spinBoxMinutes->setRange(0, 60);

    connect(ui->pushButtonClose, SIGNAL(clicked()), this, SLOT(reject()));
    connect(ui->pushButtonCreateProposal, SIGNAL(clicked()), this, SLOT(click_pushButtonCreateProposal()));
    connect(ui->spinBoxDays, SIGNAL(clickedSpinBox()), this, SLOT(click_spinBox()));
    connect(ui->spinBoxMinutes, SIGNAL(clickedSpinBox()), this, SLOT(click_spinBox()));
    connect(ui->spinBoxHours, SIGNAL(clickedSpinBox()), this, SLOT(click_spinBox()));

    // Allow rollover of spinboxes
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

    string fee = FormatMoney(GetConsensusParameter(Consensus::CONSENSUS_PARAM_PROPOSAL_MIN_FEE, view));
    QString warning = tr("By submitting the proposal a contribution of %1 NAV to the Community Fund will occur from your wallet.").arg(QString::fromStdString(fee));
    ui->labelWarning->setText(warning);
}

void CommunityFundCreateProposalDialog::setModel(WalletModel *model)
{
    this->model = model;

    ui->lineEditRequestedAmount->setDisplayUnit(model->getOptionsModel()->getDisplayUnit());
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
    size_t desc_length = ui->plainTextEditDescription->text().toStdString().length();
    if(desc_length >= 1024 || desc_length == 0)
    {
        isValid = false;
    }

    if(ui->spinBoxDays->value()*24*60*60 + ui->spinBoxHours->value()*60*60 + ui->spinBoxMinutes->value()*60 <= 0)
    {
        ui->spinBoxDays->setValid(false);
        ui->spinBoxHours->setValid(false);
        ui->spinBoxMinutes->setValid(false);
        isValid = false;
    }

    return isValid;
}

// Q_SLOTS
void CommunityFundCreateProposalDialog::click_spinBox() {
    ui->spinBoxDays->setValid(true);
    ui->spinBoxHours->setValid(true);
    ui->spinBoxMinutes->setValid(true);
}

void CommunityFundCreateProposalDialog::click_pushButtonCreateProposal()
{
    if(!model)
        return;

    if(this->validate())
    {
        LOCK2(cs_main, pwalletMain->cs_wallet);

        CStateViewCache view(pcoinsTip);

        CNavcoinAddress address("NQFqqMUD55ZV3PJEJZtaKCsQmjLT6JkjvJ"); // Dummy address

        CWalletTx wtx;
        bool fSubtractFeeFromAmount = false;

        // Address
        string Address = ui->lineEditNavcoinAddress->text().toStdString().c_str();

        // Requested Amount
        CAmount nReqAmount = ui->lineEditRequestedAmount->value();

        // Deadline
        int64_t nDeadline = ui->spinBoxDays->value()*24*60*60 + ui->spinBoxHours->value()*60*60 + ui->spinBoxMinutes->value()*60;

        // Description
        string sDesc = ui->plainTextEditDescription->text().toStdString();

        bool fSuper = ui->superProposalCheckbox->isChecked();

        UniValue strDZeel(UniValue::VOBJ);
        uint64_t nVersion = CProposal::BASE_VERSION;

        if (IsReducedCFundQuorumEnabled(chainActive.Tip(), Params().GetConsensus()))
            nVersion |= CProposal::REDUCED_QUORUM_VERSION;

        if (IsDAOEnabled(chainActive.Tip(), Params().GetConsensus()))
            nVersion |= CProposal::ABSTAIN_VOTE_VERSION;

        if (IsExcludeEnabled(chainActive.Tip(), Params().GetConsensus()))
            nVersion |= CProposal::EXCLUDE_VERSION;

        if (fSuper && IsDaoSuperEnabled(chainActive.Tip(), Params().GetConsensus()))
            nVersion |= CProposal::SUPER_VERSION;

        strDZeel.pushKV("n",nReqAmount);
        strDZeel.pushKV("a",Address);
        strDZeel.pushKV("d",nDeadline);
        strDZeel.pushKV("s",sDesc);
        strDZeel.pushKV("v",nVersion);

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
            return;
        }

        // Ensure wallet is unlocked
        WalletModel::UnlockContext ctx(model->requestUnlock());
        if(!ctx.isValid())
        {
            // Unlock wallet was cancelled
            return;
        }

        // Check balance
        CAmount curBalance = pwalletMain->GetBalance();
        if (curBalance <= GetConsensusParameter(Consensus::CONSENSUS_PARAM_PROPOSAL_MIN_FEE, view)) {
            QMessageBox msgBox(this);
            string fee = FormatMoney(GetConsensusParameter(Consensus::CONSENSUS_PARAM_PROPOSAL_MIN_FEE, view));
            std::string str = tr("You require at least %1 NAV mature and available to create a proposal\n").arg(QString::fromStdString(fee)).toStdString();
            msgBox.setText(tr(str.c_str()));
            msgBox.addButton(tr("Ok"), QMessageBox::AcceptRole);
            msgBox.setIcon(QMessageBox::Warning);
            msgBox.setWindowTitle("Insufficient NAV");
            msgBox.exec();
            return;
        }

        // Create partial proposal object with all nessesary display fields from input and create confirmation dialog
        {
            CProposal *proposal = new CProposal();
            proposal->ownerAddress = Address;
            proposal->nAmount = nReqAmount;
            proposal->strDZeel = sDesc;
            proposal->nDeadline = nDeadline;

            SendCommunityFundDialog dlg(this, proposal, 10);
            if(dlg.exec() == QDialog::Rejected)
            {
                // User Declined to make the proposal
                return;
            }
            else {
                // User accepted making the proposal
                CScript scriptPubKey = GetScriptForDestination(address.Get());
                SetScriptForCommunityFundContribution(scriptPubKey);

                // Create and send the transaction
                CReserveKey reservekey(pwalletMain);
                CAmount nFeeRequired;
                std::string strError;
                vector<CRecipient> vecSend;
                int nChangePosRet = -1;
                CAmount nValue = GetConsensusParameter(Consensus::CONSENSUS_PARAM_PROPOSAL_MIN_FEE, view);
                CRecipient recipient = {scriptPubKey, nValue, fSubtractFeeFromAmount};
                vecSend.push_back(recipient);

                bool created_proposal = true;

                std::vector<shared_ptr<CReserveBLSCTBlindingKey>> reserveBLSCTKey;

                if (!pwalletMain->CreateTransaction(vecSend, wtx, reservekey, reserveBLSCTKey, nFeeRequired, nChangePosRet, strError, false, nullptr, true)) {
                    if (!fSubtractFeeFromAmount && nValue + nFeeRequired > pwalletMain->GetBalance()) {
                        created_proposal = false;
                    }
                }
                if (!pwalletMain->CommitTransaction(wtx, reservekey, reserveBLSCTKey)) {
                    created_proposal = false;
                }

                // If the proposal was successfully made, confirm to the user it was made
                if (created_proposal) {
                    // Display success UI and close current dialog
                    CommunityFundSuccessDialog dlg(wtx.GetHash(), this, proposal);
                    dlg.exec();
                    QDialog::accept();
                    return;
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
                    return;
                }
            }
        }
    }
    else
    {
        QMessageBox msgBox(this);
        QString str = QString(tr("Please enter a valid:\n"));
        if(!ui->lineEditNavcoinAddress->isValid() || (ui->lineEditNavcoinAddress->text() == QString("")))
            str += QString(tr("- Address\n"));
        if(!ui->lineEditRequestedAmount->validate())
            str += QString(tr("- Requested Amount\n"));
        if(ui->plainTextEditDescription->text() == QString("") || ui->plainTextEditDescription->text().size() <= 0)
            str += QString(tr("- Description\n"));
        if((ui->spinBoxDays->value()*24*60*60 + ui->spinBoxHours->value()*60*60 + ui->spinBoxMinutes->value()*60) <= 0)
            str += QString(tr("- Duration\n"));

        msgBox.setText(str);
        msgBox.addButton(tr("Ok"), QMessageBox::AcceptRole);
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.setWindowTitle("Please enter valid fields");
        msgBox.exec();
        return;
    }
    return;
}

CommunityFundCreateProposalDialog::~CommunityFundCreateProposalDialog()
{
    delete ui;
}
