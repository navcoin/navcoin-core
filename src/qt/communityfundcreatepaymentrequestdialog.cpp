#include "communityfundcreatepaymentrequestdialog.h"
#include "ui_communityfundcreatepaymentrequestdialog.h"

#include "sendcommunityfunddialog.h"
#include "consensus/cfund.h"
#include "main.h"
#include "main.cpp"
#include "guiconstants.h"
#include "skinize.h"
#include <QMessageBox>
#include <iostream>

std::string random_string_owo( size_t length )
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
    ui(new Ui::CommunityFundCreatePaymentRequestDialog)
{
    ui->setupUi(this);

    //connect
    connect(ui->pushButtonClose, SIGNAL(clicked()), this, SLOT(reject()));
    connect(ui->pushButtonSubmitPaymentRequest, SIGNAL(clicked()), SLOT(click_pushButtonSubmitPaymentRequest()));
}

bool CommunityFundCreatePaymentRequestDialog::validate()
{
    bool isValid = true;

    //proposal hash;
    if(!isActiveProposal(uint256S(ui->lineEditProposalHash->text().toStdString())))
    {
        isValid = false;
        ui->lineEditProposalHash->setValid(false);
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

bool CommunityFundCreatePaymentRequestDialog::click_pushButtonSubmitPaymentRequest()
{

    if(this->validate())
    {

        //create payment request
        std::cout << "0\n";
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

        //EnsureWalletIsUnlocked();

        CKey key;
        if (!pwalletMain->GetKey(keyID, key))
            return false;
        std::cout << "1\n";
        CAmount nReqAmount = ui->lineEditRequestedAmount->value();
        std::string id = ui->plainTextEditDescription->toPlainText().toStdString();
        std::string sRandom = random_string_owo(16);

        std::string Secret = sRandom + "I kindly ask to withdraw " +
                std::to_string(nReqAmount) + "NAV from the proposal " +
                proposal.hash.ToString() + ". Payment request id: " + id;

        CHashWriter ss(SER_GETHASH, 0);
        ss << strMessageMagic;
        ss << Secret;
        std::cout << "2\n";
        vector<unsigned char> vchSig;
        if (!key.SignCompact(ss.GetHash(), vchSig))
            return false;
        std::cout << "3\n";
        std::string Signature = EncodeBase64(&vchSig[0], vchSig.size());

        if (nReqAmount <= 0 || nReqAmount > proposal.GetAvailable(true))
            return false;

        CWalletTx wtx;
        bool fSubtractFeeFromAmount = false;

        UniValue strDZeel(UniValue::VOBJ);
        std::cout << "4\n";
        strDZeel.push_back(Pair("h",ui->lineEditProposalHash->text().toStdString()));
        strDZeel.push_back(Pair("n",nReqAmount));
        strDZeel.push_back(Pair("s",Signature));
        strDZeel.push_back(Pair("r",sRandom));
        strDZeel.push_back(Pair("i",id));
        strDZeel.push_back(Pair("v",IsReducedCFundQuorumEnabled(pindexBestHeader, Params().GetConsensus()) ? CFund::CPaymentRequest::CURRENT_VERSION : 2));
        std::cout << "5\n";
        wtx.strDZeel = strDZeel.write();
        wtx.nCustomVersion = CTransaction::PAYMENT_REQUEST_VERSION;

        if(wtx.strDZeel.length() > 1024)
            return false;

        //SendMoney(address.Get(), 10000, fSubtractFeeFromAmount, wtx, "", true);
        std::cout << "6\n";
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
        std::cout << "7\n";
        // Create and send the transaction
        CReserveKey reservekey(pwalletMain);
        CAmount nFeeRequired;
        std::string strError;
        vector<CRecipient> vecSend;
        int nChangePosRet = -1;
        CRecipient recipient = {scriptPubKey, nReqAmount, fSubtractFeeFromAmount, ""};
        vecSend.push_back(recipient);
        std::cout << "8\n";

        //create confirmation dialog
        {
        CFund::CPaymentRequest* preq = new CFund::CPaymentRequest();
        preq->nAmount = proposal.nAmount;
        preq->fState = proposal.fState;
        preq->strDZeel = proposal.strDZeel;
        SendCommunityFundDialog dlg(this, preq, 10);
        if(dlg.exec()== QDialog::Rejected)
            return false;
        }

        std::cout << "9\n";
        if (!pwalletMain->CreateTransaction(vecSend, wtx, reservekey, nFeeRequired, nChangePosRet, strError, NULL, true, strDZeel.get_str())) {
            if (!fSubtractFeeFromAmount && nReqAmount + nFeeRequired > pwalletMain->GetBalance());
        }
        std::cout << "10\n";
        if (!pwalletMain->CommitTransaction(wtx, reservekey));
        std::cout << "11\n";
        return true;

    }
    else
    {

        QMessageBox msgBox(this);
        std::string str = "Please enter a valid:\n";
        if(!isActiveProposal(uint256S(ui->lineEditProposalHash->text().toStdString())))
            str += "- Proposal Hash\n";
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
}
bool CommunityFundCreatePaymentRequestDialog::isActiveProposal(uint256 hash)
{
    std::vector<CFund::CProposal> vec;
    if(pblocktree->GetProposalIndex(vec))
    {
        if(std::find_if(vec.begin(), vec.end(), [&hash](CFund::CProposal& obj) {return obj.hash == hash;}) == vec.end())
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


