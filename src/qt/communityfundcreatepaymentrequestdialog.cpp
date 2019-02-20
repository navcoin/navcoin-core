#include "communityfundcreatepaymentrequestdialog.h"
#include "ui_communityfundcreatepaymentrequestdialog.h"
#include "uint256.h"
#include "consensus/cfund.h"
#include "main.h"
#include "main.cpp"
#include "guiconstants.h"
#include "skinize.h"
#include <QMessageBox>

CommunityFundCreatePaymentRequestDialog::CommunityFundCreatePaymentRequestDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CommunityFundCreatePaymentRequestDialog)
{
    ui->setupUi(this);

    //connect
    connect(ui->pushButtonClose, SIGNAL(clicked()), this, SLOT(reject()));
    connect(ui->pushButtonSubmitPaymentRequest, SIGNAL(clicked()), SLOT(on_click_pushButtonSubmitPaymentRequest()));
}

bool CommunityFundCreatePaymentRequestDialog::validate()
{
    bool isValid = true;

    //uint256 proposal hash;
    std::vector<CFund::CProposal> vec;
    if(pblocktree->GetProposalIndex(vec))
    {
        uint256 input_hash = uint256S(ui->lineEditProposalHash->text().toStdString());\
        if(std::find_if(vec.begin(), vec.end(), [&input_hash](CFund::CProposal& obj) {return obj.hash == input_hash;}) == vec.end())
        {
            isValid = false;
            ui->lineEditProposalHash->setValid(false);
        }
    }

    //amount
    if(!ui->lineEditRequestedAmount->validate())
    {
        isValid = false;
        ui->lineEditRequestedAmount->setValid(false);
    }

    //desc
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

    return isValid;
}

bool CommunityFundCreatePaymentRequestDialog::on_click_pushButtonSubmitPaymentRequest()
{

    if(this->validate())
    {

        //create payment request
        /*
        LOCK2(cs_main, pwalletMain->cs_wallet);

        CFund::CProposal proposal = pblocktree->GetProposal(uint256S(ui->lineEditProposalHash->text().toStdString()));

        if(proposal.fState != CFund::ACCEPTED)
            return false;

        CNavCoinAddress address(proposal.Address);

        if(!address.IsValid())
            return false;

        CKeyID keyID;
        if (!address.GetKeyID(keyID))
            return false;

        EnsureWalletIsUnlocked();

        CKey key;
        if (!pwalletMain->GetKey(keyID, key))
            return false;

        CAmount nReqAmount = ui->lineEditRequestedAmount->value();
        std::string id = ui->plainTextEditDescription->toPlainText().toStdString();
        std::string sRandom = random_string(16);

        std::string Secret = sRandom + "I kindly ask to withdraw " +
                std::to_string(nReqAmount) + "NAV from the proposal " +
                proposal.hash.ToString() + ". Payment request id: " + id;

        CHashWriter ss(SER_GETHASH, 0);
        ss << strMessageMagic;
        ss << Secret;

        vector<unsigned char> vchSig;
        if (!key.SignCompact(ss.GetHash(), vchSig))
            return false;

        std::string Signature = EncodeBase64(&vchSig[0], vchSig.size());

        if (nReqAmount <= 0 || nReqAmount > proposal.GetAvailable(true))
            return false;

        CWalletTx wtx;
        bool fSubtractFeeFromAmount = false;

        UniValue strDZeel(UniValue::VOBJ);

        strDZeel.push_back(Pair("h",ui->lineEditProposalHash->text().toStdString()));
        strDZeel.push_back(Pair("n",nReqAmount));
        strDZeel.push_back(Pair("s",Signature));
        strDZeel.push_back(Pair("r",sRandom));
        strDZeel.push_back(Pair("i",id));
        strDZeel.push_back(Pair("v",IsReducedCFundQuorumEnabled(pindexBestHeader, Params().GetConsensus()) ? CFund::CPaymentRequest::CURRENT_VERSION : 2));

        wtx.strDZeel = strDZeel.write();
        wtx.nCustomVersion = CTransaction::PAYMENT_REQUEST_VERSION;

        if(wtx.strDZeel.length() > 1024)
            return false;

        //SendMoney(address.Get(), 10000, fSubtractFeeFromAmount, wtx, "", true);

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
        if (!pwalletMain->CreateTransaction(vecSend, wtx, reservekey, nFeeRequired, nChangePosRet, strError, NULL, true, strDZeel.get_str())) {
            if (!fSubtractFeeFromAmount && nReqAmount + nFeeRequired > pwalletMain->GetBalance());
        }
        if (!pwalletMain->CommitTransaction(wtx, reservekey));
        */
        return true;

    }
    else
    {

        QMessageBox msgBox(this);
        std::string str = "Please enter a valid:\n";
        std::vector<CFund::CProposal> vec;
        if(pblocktree->GetProposalIndex(vec))
        {
            uint256 input_hash = uint256S(ui->lineEditProposalHash->text().toStdString());\
            if(std::find_if(vec.begin(), vec.end(), [&input_hash](CFund::CProposal& obj) {return obj.hash == input_hash;}) == vec.end())
            {
                str += "- Proposal Hash\n";
            }
        }
        if(!ui->lineEditRequestedAmount->validate())
            str += "- Requested Amount\n";
        if(ui->plainTextEditDescription->toPlainText() == QString("") || ui->plainTextEditDescription->toPlainText().size() <= 0)
            str += "- Description\n";

        msgBox.setText(tr(str.c_str()));
        msgBox.addButton(tr("Ok"), QMessageBox::AcceptRole);
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.exec();

        return false;
    }
        return false;
}

CommunityFundCreatePaymentRequestDialog::~CommunityFundCreatePaymentRequestDialog()
{
    delete ui;
}
