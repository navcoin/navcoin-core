#include <qt/communityfundcreateproposaldialog.h>
#include <qt/communityfundsuccessdialog.h>
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

    string fee = std::to_string(Params().GetConsensus().nProposalMinimalFee / COIN);
    string warning = "By submitting the proposal a " + fee + " NAV deduction will occur from your wallet ";
    ui->labelWarning->setText(QString::fromStdString(warning));
}

void CommunityFundCreateProposalDialog::setModel(WalletModel *model)
{
    this->model = model;
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
    size_t desc_length = ui->plainTextEditDescription->toPlainText().toStdString().length();
    if(desc_length >= 1024 || desc_length == 0)
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

        strDZeel.pushKV("n",nReqAmount);
        strDZeel.pushKV("a",Address);
        strDZeel.pushKV("d",nDeadline);
        strDZeel.pushKV("s",sDesc);
        strDZeel.pushKV("v",IsReducedCFundQuorumEnabled(chainActive.Tip(), Params().GetConsensus()) ? CFund::CProposal::CURRENT_VERSION : 2);

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
        if (curBalance <= Params().GetConsensus().nProposalMinimalFee) {
            QMessageBox msgBox(this);
            string fee = std::to_string(Params().GetConsensus().nProposalMinimalFee / COIN);
            std::string str = "You require at least " + fee + " NAV mature and available to create a proposal\n";
            msgBox.setText(tr(str.c_str()));
            msgBox.addButton(tr("Ok"), QMessageBox::AcceptRole);
            msgBox.setIcon(QMessageBox::Warning);
            msgBox.setWindowTitle("Insufficient NAV");
            msgBox.exec();
            return;
        }

        // Create partial proposal object with all nessesary display fields from input and create confirmation dialog
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
                return;
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
        if(ui->plainTextEditDescription->toPlainText() == QString("") || ui->plainTextEditDescription->toPlainText().size() <= 0)
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
