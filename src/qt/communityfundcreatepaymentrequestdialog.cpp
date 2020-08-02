#include <qt/communityfundcreatepaymentrequestdialog.h>
#include <qt/communityfundsuccessdialog.h>
#include <qt/optionsmodel.h>
#include <qt/sendcommunityfunddialog.h>
#include <ui_communityfundcreatepaymentrequestdialog.h>

#include <QMessageBox>
#include <QComboBox>
#include <string>

#include <qt/navcoinunits.h>

#include <base58.h>
#include <consensus/dao.h>
#include <qt/guiconstants.h>
#include <qt/guiutil.h>
#include <main.cpp>
#include <main.h>
#include <sync.h>
#include <wallet/wallet.h>
#include <qt/walletmodel.h>

std::string random_str(size_t length)
{
    auto randchar = []() -> char
    {
        const char charset[] =
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";
        const size_t max_index = (sizeof(charset) - 1);
        return charset[ rand() % max_index ];
    };
    std::string str(length,0);
    std::generate_n( str.begin(), length, randchar );
    return str;
}

CommunityFundCreatePaymentRequestDialog::CommunityFundCreatePaymentRequestDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CommunityFundCreatePaymentRequestDialog),
    model(0)
{
    ui->setupUi(this);

    connect(ui->pushButtonClose, SIGNAL(clicked()), this, SLOT(reject()));
    connect(ui->pushButtonSubmitPaymentRequest, SIGNAL(clicked()), SLOT(click_pushButtonSubmitPaymentRequest()));

    CProposalMap proposalMap;

    {
    LOCK(cs_main);

    CStateViewCache coins(pcoinsTip);

    if (pcoinsTip != nullptr && coins.GetAllProposals(proposalMap))
    {
        for (auto& it: proposalMap)
        {
            if (it.second.GetLastState() != DAOFlags::ACCEPTED || it.second.GetAvailable(coins) == 0)
                continue;

            ui->comboBoxProposalHash->insertItem(0, QString::fromStdString(it.second.strDZeel).left(50) + (it.second.strDZeel.size() > 50 ? "..." : ""), QString::fromStdString(it.second.hash.ToString()));
        }
    }
    }
}

void CommunityFundCreatePaymentRequestDialog::setModel(WalletModel *model)
{
    this->model = model;

    ui->lineEditRequestedAmount->setDisplayUnit(model->getOptionsModel()->getDisplayUnit());
}

bool CommunityFundCreatePaymentRequestDialog::validate()
{
    bool isValid = true;

    // Proposal hash;
    if(!isActiveProposal(uint256S(ui->comboBoxProposalHash->currentData().toString().toStdString())))
    {
        isValid = false;
    }

    // Amount
    if(!ui->lineEditRequestedAmount->validate())
    {
        isValid = false;
        ui->lineEditRequestedAmount->setValid(false);
    }

    // Description
    size_t desc_size = ui->plainTextEditDescription->text().toStdString().length();
    if(desc_size >= 1024 || desc_size == 0)
    {
        isValid = false;
    }

    return isValid;
}

void CommunityFundCreatePaymentRequestDialog::click_pushButtonSubmitPaymentRequest()
{

    if(this->validate())
    {
        LOCK2(cs_main, pwalletMain->cs_wallet);

        // Get Proposal
        CProposal proposal;
        if(!pcoinsTip->GetProposal(uint256S(ui->comboBoxProposalHash->currentData().toString().toStdString()), proposal)) {
            QMessageBox msgBox(this);
            std::string str = "Proposal could not be found with that hash\n";
            msgBox.setText(tr(str.c_str()));
            msgBox.addButton(tr("Ok"), QMessageBox::AcceptRole);
            msgBox.setIcon(QMessageBox::Warning);
            msgBox.setWindowTitle("Proposal not found");
            msgBox.exec();
            return;
        }
        if(proposal.GetLastState() != DAOFlags::ACCEPTED) {
            QMessageBox msgBox(this);
            std::string str = "Proposals need to have been accepted to create a Payment Request for them\n";
            msgBox.setText(tr(str.c_str()));
            msgBox.addButton(tr("Ok"), QMessageBox::AcceptRole);
            msgBox.setIcon(QMessageBox::Warning);
            msgBox.setWindowTitle("Proposal not accepted");
            msgBox.exec();
            return;
        }

        // Get Address
        CNavCoinAddress address(proposal.GetOwnerAddress());
        if(!address.IsValid()) {
            QMessageBox msgBox(this);
            std::string str = "The address of the Proposal is not valid\n";
            msgBox.setText(tr(str.c_str()));
            msgBox.addButton(tr("Ok"), QMessageBox::AcceptRole);
            msgBox.setIcon(QMessageBox::Warning);
            msgBox.setWindowTitle("Address not valid");
            msgBox.exec();
            return;
        }

        // Get KeyID
        CKeyID keyID;
        if (!address.GetKeyID(keyID)) {
            QMessageBox msgBox(this);
            std::string str = "The address does not refer to a key\n";
            msgBox.setText(tr(str.c_str()));
            msgBox.addButton(tr("Ok"), QMessageBox::AcceptRole);
            msgBox.setIcon(QMessageBox::Warning);
            msgBox.setWindowTitle("Address not valid");
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

        // Get Key
        CKey key;
        if (!pwalletMain->GetKey(keyID, key)) {
            QMessageBox msgBox(this);
            std::string str = "You are not the owner of the Proposal\n";
            msgBox.setText(tr(str.c_str()));
            msgBox.addButton(tr("Ok"), QMessageBox::AcceptRole);
            msgBox.setIcon(QMessageBox::Warning);
            msgBox.setWindowTitle("Not the owner");
            msgBox.exec();
            return;
        }

        // Get fields from form
        CAmount nReqAmount = ui->lineEditRequestedAmount->value();
        std::string id = ui->plainTextEditDescription->text().toStdString();
        std::string sRandom = random_str(16);

        // Construct Secret
        std::string Secret = sRandom + "I kindly ask to withdraw " +
                std::to_string(nReqAmount) + "NAV from the proposal " +
                proposal.hash.ToString() + ". Payment request id: " + id;

        CHashWriter ss(SER_GETHASH, 0);
        ss << strMessageMagic;
        ss << Secret;

        // Attempt to sign
        vector<unsigned char> vchSig;
        if (!key.SignCompact(ss.GetHash(), vchSig)) {
            QMessageBox msgBox(this);
            std::string str = "Failed to sign\n";
            msgBox.setText(tr(str.c_str()));
            msgBox.addButton(tr("Ok"), QMessageBox::AcceptRole);
            msgBox.setIcon(QMessageBox::Warning);
            msgBox.setWindowTitle("Sign Failed");
            msgBox.exec();
            return;
        }

        // Create Signature
        std::string Signature = EncodeBase64(&vchSig[0], vchSig.size());

        CStateViewCache coins(pcoinsTip);

        // Validate requested amount
        if (nReqAmount <= 0 || nReqAmount > proposal.GetAvailable(coins, true)) {
            QMessageBox msgBox(this);
            QString str = tr("Requested amount must be greater than 0 NAV (Zero)\n");
            if (nReqAmount > proposal.GetAvailable(*pcoinsTip, true)) {
                str = tr("Requested amount %1 is more than avaible coins in the proposal (%2)\n")
                    .arg(
                        NavCoinUnits::formatWithUnit(NavCoinUnits::NAV, nReqAmount),
                        NavCoinUnits::formatWithUnit(NavCoinUnits::NAV, proposal.GetAvailable(*pcoinsTip, true))
                    );
            }
            msgBox.setText(str);
            msgBox.addButton(tr("Ok"), QMessageBox::AcceptRole);
            msgBox.setIcon(QMessageBox::Warning);
            msgBox.setWindowTitle(tr("Invalid Amount"));
            msgBox.exec();
            return;
        }

        // Create wtx
        CWalletTx wtx;
        bool fSubtractFeeFromAmount = false;

        UniValue strDZeel(UniValue::VOBJ);
        uint64_t nVersion = CPaymentRequest::BASE_VERSION;

        if (IsReducedCFundQuorumEnabled(chainActive.Tip(), Params().GetConsensus()))
            nVersion |= CPaymentRequest::REDUCED_QUORUM_VERSION;

        if (IsDAOEnabled(chainActive.Tip(), Params().GetConsensus()))
            nVersion |= CPaymentRequest::ABSTAIN_VOTE_VERSION;

        strDZeel.pushKV("h",ui->comboBoxProposalHash->currentData().toString().toStdString());
        strDZeel.pushKV("n",nReqAmount);
        strDZeel.pushKV("s",Signature);
        strDZeel.pushKV("r",sRandom);
        strDZeel.pushKV("i",id);
        strDZeel.pushKV("v",nVersion);

        wtx.strDZeel = strDZeel.write();
        wtx.nCustomVersion = CTransaction::PAYMENT_REQUEST_VERSION;

        // Validate wtx
        if(wtx.strDZeel.length() > 1024) {
            QMessageBox msgBox(this);
            std::string str = "Description too long\n";
            msgBox.setText(tr(str.c_str()));
            msgBox.addButton(tr("Ok"), QMessageBox::AcceptRole);
            msgBox.setIcon(QMessageBox::Warning);
            msgBox.setWindowTitle("Invalid String");
            msgBox.exec();
            return;
        }

        // Check balance
        CAmount curBalance = pwalletMain->GetBalance();
        if (curBalance <= 10000) {
            QMessageBox msgBox(this);
            string fee = FormatMoney(GetConsensusParameter(Consensus::CONSENSUS_PARAM_PAYMENT_REQUEST_MIN_FEE, coins));
            std::string str = tr("You require at least %1 NAV mature and available to create a payment request\n").arg(QString::fromStdString(fee)).toStdString();
            msgBox.setText(tr(str.c_str()));
            msgBox.addButton(tr("Ok"), QMessageBox::AcceptRole);
            msgBox.setIcon(QMessageBox::Warning);
            msgBox.setWindowTitle("Insufficient NAV");
            msgBox.exec();
            return;
        }

        // Create partial proposal object with all nessesary display fields from input and create confirmation dialog
        {
            // Create confirmation dialog
            CPaymentRequest* preq = new CPaymentRequest();
            preq->nAmount = ui->lineEditRequestedAmount->value();
            preq->proposalhash = proposal.hash;
            preq->strDZeel = ui->plainTextEditDescription->text().toStdString();

            SendCommunityFundDialog dlg(this, preq, 10);
            if(dlg.exec()== QDialog::Rejected) {
                // User Declined to make the prequest
                return;
            }
            else {
                // User accepted making the prequest
                // Parse NavCoin address
                CScript CFContributionScript;
                CScript scriptPubKey = GetScriptForDestination(address.Get());
                SetScriptForCommunityFundContribution(scriptPubKey);

                // Create and send the transaction
                CReserveKey reservekey(pwalletMain);
                CAmount nFeeRequired;
                std::string strError;
                vector<CRecipient> vecSend;
                int nChangePosRet = -1;
                CAmount nValue = 1000;
                CRecipient recipient = {scriptPubKey, nValue, fSubtractFeeFromAmount};
                vecSend.push_back(recipient);

                bool created_prequest = true;

                std::vector<shared_ptr<CReserveBLSCTBlindingKey>> reserveBLSCTKey;

                if (!pwalletMain->CreateTransaction(vecSend, wtx, reservekey, reserveBLSCTKey, nFeeRequired, nChangePosRet, strError, false, nullptr, true)) {
                    if (!fSubtractFeeFromAmount && nValue + nFeeRequired > pwalletMain->GetBalance()) {
                        created_prequest = false;
                    }
                }
                if (!pwalletMain->CommitTransaction(wtx, reservekey, reserveBLSCTKey)) {
                    created_prequest = false;
                }

                // If the proposal was successfully made, confirm to the user it was made
                if (created_prequest) {
                    // Display success UI
                    CommunityFundSuccessDialog dlg(wtx.GetHash(), this, preq);
                    dlg.exec();
                    QDialog::accept();
                    return;
                }
                else {
                    // Display something went wrong UI
                    QMessageBox msgBox(this);
                    std::string str = "Payment Request creation failed\n";
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
        QString str = tr("Please enter a valid:\n");
        if(!isActiveProposal(uint256S(ui->comboBoxProposalHash->currentData().toString().toStdString())))
            str += QString(tr("- Proposal Hash\n"));
        if(!ui->lineEditRequestedAmount->validate())
            str += QString(tr("- Requested Amount\n"));
        if(ui->plainTextEditDescription->text() == QString("") || ui->plainTextEditDescription->text().size() <= 0)
            str += QString(tr("- Description\n"));

        msgBox.setText(str);
        msgBox.addButton(tr("Ok"), QMessageBox::AcceptRole);
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.exec();
        return;
    }
}

bool CommunityFundCreatePaymentRequestDialog::isActiveProposal(uint256 hash)
{
    std::vector<CProposal> vec;
    CProposalMap mapProposals;

    if(pcoinsTip->GetAllProposals(mapProposals))
    {
        for (CProposalMap::iterator it = mapProposals.begin(); it != mapProposals.end(); it++)
        {
            CProposal proposal;
            if (!pcoinsTip->GetProposal(it->first, proposal))
                continue;
            vec.push_back(proposal);
        }

        if(std::find_if(vec.begin(), vec.end(), [&hash](CProposal& obj) {return obj.hash == hash;}) == vec.end())
        {
            return false;
        }
    }

    return true;
}

CommunityFundCreatePaymentRequestDialog::~CommunityFundCreatePaymentRequestDialog()
{
    delete ui;
}
