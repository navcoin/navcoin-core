// Copyright (c) 2019 The NavCoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "daoproposeanswer.h"

DaoProposeAnswer::DaoProposeAnswer(QWidget *parent, CConsultation consultation, ValidatorFunc validator) :
    layout(new QVBoxLayout),
    consultation(consultation),
    answerInput(new QLineEdit),
    validatorFunc(validator)
{
    this->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    this->setLayout(layout);

    CStateViewCache view(pcoinsTip);

    auto *bottomBox = new QFrame;
    auto *bottomBoxLayout = new QHBoxLayout;
    bottomBoxLayout->setContentsMargins(QMargins());
    bottomBox->setLayout(bottomBoxLayout);

    QPushButton* proposeBtn = new QPushButton(tr("Submit"));
    connect(proposeBtn, SIGNAL(clicked()), this, SLOT(onPropose()));

    QPushButton* closeBtn = new QPushButton(tr("Close"));
    connect(closeBtn, SIGNAL(clicked()), this, SLOT(onClose()));

    bottomBoxLayout->addStretch(1);
    bottomBoxLayout->addWidget(proposeBtn);
    bottomBoxLayout->addWidget(closeBtn);

    warningLbl = new QLabel("");
    warningLbl->setObjectName("warning");
    warningLbl->setVisible(false);

    QString fee = QString::fromStdString(FormatMoney(GetConsensusParameter(Consensus::CONSENSUS_PARAM_CONSULTATION_ANSWER_MIN_FEE, view)));

    layout->addSpacing(15);
    layout->addWidget(new QLabel(tr("Submit an answer proposal for:<br>%1").arg(QString::fromStdString(consultation.strDZeel))));
    layout->addSpacing(15);
    layout->addWidget(answerInput);
    layout->addWidget(warningLbl);
    layout->addSpacing(15);
    layout->addWidget(new QLabel(tr("By submitting the proposal a %1 NAV deduction will occur from your wallet").arg(fee)));
    layout->addWidget(bottomBox);
    layout->addSpacing(15);

}

void DaoProposeAnswer::setModel(WalletModel *model)
{
    this->model = model;
}

void DaoProposeAnswer::showWarning(QString text)
{
    warningLbl->setText(text);
    warningLbl->setVisible(text == "" ? false : true);
    adjustSize();
}

void DaoProposeAnswer::onPropose()
{
    if(!model)
        return;

    LOCK2(cs_main, pwalletMain->cs_wallet);
    CStateViewCache view(pcoinsTip);

    CNavCoinAddress address("NQFqqMUD55ZV3PJEJZtaKCsQmjLT6JkjvJ"); // Dummy address

    CWalletTx wtx;
    bool fSubtractFeeFromAmount = false;

    std::string sAnswer = answerInput->text().toStdString();

    if (!(validatorFunc)(QString::fromStdString(sAnswer)))
    {
        showWarning(tr("Entry not valid"));
        return;
    }

    UniValue strDZeel(UniValue::VOBJ);
    uint64_t nVersion = CConsultationAnswer::BASE_VERSION;

    showWarning("");

    sAnswer = consultation.IsAboutConsensusParameter() ? RemoveFormatConsensusParameter((Consensus::ConsensusParamsPos)consultation.nMin, sAnswer) : sAnswer;

    strDZeel.pushKV("h",consultation.hash.ToString());
    strDZeel.pushKV("a",sAnswer);
    strDZeel.pushKV("v",(uint64_t)nVersion);

    wtx.strDZeel = strDZeel.write();
    wtx.nCustomVersion = CTransaction::ANSWER_VERSION;

    // Ensure wallet is unlocked
    WalletModel::UnlockContext ctx(model->requestUnlock());
    if(!ctx.isValid())
    {
        // Unlock wallet was cancelled
        return;
    }

    // Check balance
    CAmount curBalance = pwalletMain->GetBalance();
    if (curBalance <= GetConsensusParameter(Consensus::CONSENSUS_PARAM_CONSULTATION_ANSWER_MIN_FEE, view)) {
        QMessageBox msgBox(this);
        string fee = FormatMoney(GetConsensusParameter(Consensus::CONSENSUS_PARAM_CONSULTATION_ANSWER_MIN_FEE, view));
        std::string str = tr("You require at least %1 NAV mature and available to propose an answer.\n").arg(QString::fromStdString(fee)).toStdString();
        msgBox.setText(tr(str.c_str()));
        msgBox.addButton(tr("Ok"), QMessageBox::AcceptRole);
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.setWindowTitle("Insufficient NAV");
        msgBox.exec();
        return;
    }

    CScript scriptPubKey;
    SetScriptForCommunityFundContribution(scriptPubKey);

    // Create and send the transaction
    CReserveKey reservekey(pwalletMain);
    CAmount nFeeRequired;
    std::string strError;
    vector<CRecipient> vecSend;
    int nChangePosRet = -1;
    CAmount nValue = GetConsensusParameter(Consensus::CONSENSUS_PARAM_CONSULTATION_ANSWER_MIN_FEE, view);
    CRecipient recipient = {scriptPubKey, nValue, fSubtractFeeFromAmount};
    vecSend.push_back(recipient);

    bool created = true;

    if (!pwalletMain->CreateTransaction(vecSend, wtx, reservekey, nFeeRequired, nChangePosRet, strError, NULL, true)) {
        if (!fSubtractFeeFromAmount && nValue + nFeeRequired > pwalletMain->GetBalance()) {
            created = false;
        }
    }
    if (!pwalletMain->CommitTransaction(wtx, reservekey)) {
        created = false;
    }

    if (created) {
        // Display success UI and close current dialog
        if (QMessageBox::Yes == QMessageBox(QMessageBox::Information,
                                            tr("Success!"),
                                            tr("Your answer proposal has been correctly created.")+"<br><br>"+tr("Now you need to find support from stakers so it is included in the voting!")+"<br><br>"+tr("Do you want to support your own canswer?"),
                                            QMessageBox::Yes|QMessageBox::No).exec())
        {
            bool duplicate;
            CConsultationAnswer answer;
            if (TxToConsultationAnswer(wtx.strDZeel, wtx.GetHash(), uint256(), answer))
            {
                Support(answer.hash, duplicate);
            }
        }
        QDialog::accept();
        return;
    }
    else {
        // Display something went wrong UI
        QMessageBox msgBox(this);
        msgBox.setText(tr("Answer proposal failed"));
        msgBox.addButton(tr("Ok"), QMessageBox::AcceptRole);
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.setWindowTitle("Error");
        msgBox.exec();
        return;
    }
}

void DaoProposeAnswer::onClose()
{
    close();
}
