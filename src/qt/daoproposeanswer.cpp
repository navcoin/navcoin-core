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
    this->setStyleSheet(Skinize());

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

    layout->addSpacing(15);
    layout->addWidget(new QLabel(tr("Submit an answer proposal for:<br>%1").arg(QString::fromStdString(consultation.strDZeel))));
    layout->addSpacing(15);
    layout->addWidget(answerInput);
    layout->addWidget(warningLbl);
    layout->addSpacing(15);
    layout->addWidget(new QLabel(tr("By submitting the proposal a %1 NAV deduction will occur from your wallet").arg(QString::fromStdString(FormatMoney(Params().GetConsensus().nProposalMinimalFee)))));
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

    CNavCoinAddress address("NQFqqMUD55ZV3PJEJZtaKCsQmjLT6JkjvJ"); // Dummy address

    CWalletTx wtx;
    bool fSubtractFeeFromAmount = false;

    std::string sAnswer = answerInput->text().toStdString();

    if (!(*validatorFunc)(QString::fromStdString(sAnswer)))
    {
        showWarning(tr("Entry not valid"));
        return;
    }

    UniValue strDZeel(UniValue::VOBJ);
    uint64_t nVersion = CConsultationAnswer::BASE_VERSION;

    showWarning("");

    strDZeel.push_back(Pair("h",consultation.hash.ToString()));
    strDZeel.push_back(Pair("a",sAnswer));
    strDZeel.push_back(Pair("v",(uint64_t)nVersion));

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
    if (curBalance <= Params().GetConsensus().nConsultationAnswerMinimalFee) {
        QMessageBox msgBox(this);
        string fee = FormatMoney(Params().GetConsensus().nConsultationAnswerMinimalFee);
        std::string str = "You require at least " + fee + " NAV mature and available to propose an answer.\n";
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
    CAmount nValue = Params().GetConsensus().nConsultationAnswerMinimalFee;
    CRecipient recipient = {scriptPubKey, nValue, fSubtractFeeFromAmount, ""};
    vecSend.push_back(recipient);

    bool created = true;

    if (!pwalletMain->CreateTransaction(vecSend, wtx, reservekey, nFeeRequired, nChangePosRet, strError, NULL, true, "")) {
        if (!fSubtractFeeFromAmount && nValue + nFeeRequired > pwalletMain->GetBalance()) {
            created = false;
        }
    }
    if (!pwalletMain->CommitTransaction(wtx, reservekey)) {
        created = false;
    }

    if (created) {
        // Display success UI and close current dialog
        QMessageBox msgBox(this);
        msgBox.setText(tr("The answer proposal has been correctly submitted."));
        msgBox.addButton(tr("Ok"), QMessageBox::AcceptRole);
        msgBox.setIcon(QMessageBox::Information);
        msgBox.setWindowTitle("Success!");
        msgBox.exec();
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
