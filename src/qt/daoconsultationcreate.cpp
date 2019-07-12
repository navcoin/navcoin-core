// Copyright (c) 2019 The NavCoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "daoconsultationcreate.h"

DaoConsultationCreate::DaoConsultationCreate(QWidget *parent) :
    layout(new QVBoxLayout),
    questionInput(new QLineEdit),
    rangeBox(new QCheckBox),
    minBox(new QSpinBox),
    maxBox(new QSpinBox),
    minLbl(new QLabel),
    maxLbl(new QLabel),
    warningLbl(new QLabel)
{
    this->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    this->setLayout(layout);
    this->setStyleSheet(Skinize());

    QFont subtitleFnt("Sans Serif", 18, QFont::Normal);

    auto *topBox = new QFrame;
    auto *topBoxLayout = new QHBoxLayout;
    topBoxLayout->setContentsMargins(QMargins());
    topBox->setLayout(topBoxLayout);

    QLabel* title = new QLabel(tr("Submit a new consultation"));
    title->setFont(subtitleFnt);

    topBoxLayout->addWidget(title, 0, Qt::AlignCenter);
    topBoxLayout->addStretch(1);

    auto *bottomBox = new QFrame;
    auto *bottomBoxLayout = new QHBoxLayout;
    bottomBoxLayout->setContentsMargins(QMargins());
    bottomBox->setLayout(bottomBoxLayout);

    auto *rangeFrame = new QFrame;
    auto *rangeFrameLayout = new QHBoxLayout;
    rangeFrameLayout->setContentsMargins(QMargins());
    rangeFrame->setLayout(rangeFrameLayout);

    minLbl = new QLabel(tr("Answer must be between"));
    maxLbl = new QLabel(tr("Max number of answers"));
    rangeFrameLayout->addWidget(minLbl);
    rangeFrameLayout->addWidget(minBox);
    rangeFrameLayout->addWidget(maxLbl);
    rangeFrameLayout->addWidget(maxBox);
    rangeFrameLayout->addStretch(1);

    minLbl->setVisible(false);
    minBox->setVisible(false);

    QPushButton* createBtn = new QPushButton(tr("Submit"));
    connect(createBtn, SIGNAL(clicked()), this, SLOT(onCreate()));

    rangeBox = new QCheckBox(tr("The possible answers are numbers"));
    connect(rangeBox, SIGNAL(toggled(bool)), this, SLOT(onRange(bool)));

    bottomBoxLayout->addStretch(1);
    bottomBoxLayout->addWidget(createBtn);

    warningLbl = new QLabel("");
    warningLbl->setObjectName("warning");
    warningLbl->setFont(subtitleFnt);
    warningLbl->setVisible(false);

    layout->addWidget(topBox);
    layout->addSpacing(30);
    layout->addWidget(new QLabel(tr("Question")));
    layout->addWidget(questionInput);
    layout->addWidget(rangeBox);
    layout->addSpacing(30);
    layout->addWidget(rangeFrame);
    layout->addSpacing(30);
    layout->addWidget(warningLbl);
    layout->addWidget(bottomBox);

}

void DaoConsultationCreate::setModel(WalletModel *model)
{
    this->model = model;
}

void DaoConsultationCreate::onCreate()
{
    if(!model)
        return;

    bool fRange = rangeBox->isChecked();
    int64_t nMin = minBox->value();
    int64_t nMax = maxBox->value();

    if (!fRange && (nMax < 1 || nMax > 16))
    {
        showWarning(tr("Choose a smaller number of answers."));
        return;
    }
    else if(fRange && !(nMin >= 0 && nMax < (uint64_t)-1 && nMax > nMin))
    {
        showWarning(tr("Wrong range!"));
        return;
    }

    LOCK2(cs_main, pwalletMain->cs_wallet);

    CNavCoinAddress address("NQFqqMUD55ZV3PJEJZtaKCsQmjLT6JkjvJ"); // Dummy address

    CWalletTx wtx;
    bool fSubtractFeeFromAmount = false;

    std::string sQuestion = questionInput->text().toStdString();

    if (sQuestion == "")
    {
        showWarning("The question can not be empty!");
        return;
    }

    showWarning("");

    UniValue strDZeel(UniValue::VOBJ);
    uint64_t nVersion = CConsultation::BASE_VERSION;

    if (fRange)
        nVersion |= CConsultation::ANSWER_IS_A_RANGE_VERSION;

    strDZeel.push_back(Pair("q",sQuestion));
    strDZeel.push_back(Pair("m",nMin));
    strDZeel.push_back(Pair("n",nMax));
    strDZeel.push_back(Pair("v",(uint64_t)nVersion));

    wtx.strDZeel = strDZeel.write();
    wtx.nCustomVersion = CTransaction::CONSULTATION_VERSION;

    // Ensure wallet is unlocked
    WalletModel::UnlockContext ctx(model->requestUnlock());
    if(!ctx.isValid())
    {
        // Unlock wallet was cancelled
        return;
    }

    // Check balance
    CAmount curBalance = pwalletMain->GetBalance();
    if (curBalance <= Params().GetConsensus().nConsultationMinimalFee) {
        QMessageBox msgBox(this);
        string fee = std::to_string(Params().GetConsensus().nConsultationMinimalFee / COIN);
        std::string str = "You require at least " + fee + " NAV mature and available to create a consultation.\n";
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
    CAmount nValue = Params().GetConsensus().nConsultationMinimalFee;
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
        msgBox.setText(tr("The consultation has been correctly created."));
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
        msgBox.setText(tr("Consultation creation failed"));
        msgBox.addButton(tr("Ok"), QMessageBox::AcceptRole);
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.setWindowTitle("Error");
        msgBox.exec();
        return;
    }
}

void DaoConsultationCreate::showWarning(QString text)
{
    warningLbl->setText(text);
    warningLbl->setVisible(text == "" ? false : true);
}

void DaoConsultationCreate::onRange(bool fChecked)
{
    if (fChecked)
    {
        minLbl->setVisible(true);
        minBox->setVisible(true);
        maxLbl->setText(tr("and"));
    }
    else
    {
        minLbl->setVisible(false);
        minBox->setVisible(false);
        maxLbl->setText(tr("Max number of answers"));
    }
}
