// Copyright (c) 2019 The NavCoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "daoconsultationcreate.h"

DaoConsultationCreate::DaoConsultationCreate(QWidget *parent) :
    layout(new QVBoxLayout),
    questionInput(new QLineEdit),
    answerIsNumbersBtn(new QRadioButton),
    answerIsFromListBtn(new QRadioButton),
    minBox(new QSpinBox),
    maxBox(new QSpinBox),
    minLbl(new QLabel),
    maxLbl(new QLabel),
    warningLbl(new QLabel),
    listWidget(new NavCoinListWidget(nullptr, "", [](QString s)->bool{return !s.isEmpty();})),
    moreAnswersBox(new QCheckBox)
{
    this->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    this->setLayout(layout);
    this->setStyleSheet(Skinize());

    auto *topBox = new QFrame;
    auto *topBoxLayout = new QHBoxLayout;
    topBoxLayout->setContentsMargins(QMargins());
    topBox->setLayout(topBoxLayout);

    QLabel* title = new QLabel(tr("Submit a new consultation"));

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

    auto *moreAnswers = new QFrame;
    auto *moreAnswersBoxLayout = new QHBoxLayout;
    moreAnswersBoxLayout->setContentsMargins(QMargins());
    moreAnswers->setLayout(moreAnswersBoxLayout);

    moreAnswersBox = new QCheckBox(tr("Stakers can propose additional answers"));

    moreAnswersBoxLayout->addStretch();
    moreAnswersBoxLayout->addWidget(moreAnswersBox);
    moreAnswersBoxLayout->addStretch();

    minLbl = new QLabel(tr("Answer must be between"));
    maxLbl = new QLabel(tr("Maximum amount of answers stakers can select at the same time"));
    rangeFrameLayout->addWidget(minLbl);
    rangeFrameLayout->addWidget(minBox);
    rangeFrameLayout->addWidget(maxLbl);
    rangeFrameLayout->addWidget(maxBox);
    rangeFrameLayout->addStretch(1);

    minLbl->setVisible(false);
    minBox->setVisible(false);

    minBox->setMinimum(0);
    maxBox->setMinimum(1);
    maxBox->setMaximum(pow(2,24));

    QPushButton* createBtn = new QPushButton(tr("Submit"));
    connect(createBtn, SIGNAL(clicked()), this, SLOT(onCreate()));

    answerIsNumbersBtn = new QRadioButton(tr("The possible answers are numbers"), this);
    answerIsFromListBtn = new QRadioButton(tr("The possible answers are from a list"), this);
    connect(answerIsNumbersBtn, SIGNAL(toggled(bool)), this, SLOT(onRange(bool)));
    connect(answerIsFromListBtn, SIGNAL(toggled(bool)), this, SLOT(onList(bool)));

    answerIsFromListBtn->setChecked(true);

    bottomBoxLayout->addStretch(1);
    bottomBoxLayout->addWidget(createBtn);

    warningLbl = new QLabel("");
    warningLbl->setObjectName("warning");
    warningLbl->setVisible(false);

    listWidget = new NavCoinListWidget(this, tr("Possible answers"), [](QString s)->bool{return !s.isEmpty();});

    layout->addWidget(topBox);
    layout->addSpacing(15);
    layout->addWidget(new QLabel(tr("Question")));
    layout->addWidget(questionInput);
    layout->addSpacing(15);
    layout->addWidget(answerIsNumbersBtn);
    layout->addSpacing(15);
    layout->addWidget(answerIsFromListBtn);
    layout->addWidget(listWidget);
    layout->addWidget(moreAnswers);
    layout->addSpacing(15);
    layout->addWidget(rangeFrame);
    layout->addSpacing(15);
    layout->addWidget(warningLbl);
    layout->addWidget(bottomBox);
}

DaoConsultationCreate::DaoConsultationCreate(QWidget *parent, QString title, int consensuspos) :
    layout(new QVBoxLayout),
    questionInput(new QLineEdit),
    answerIsNumbersBtn(new QRadioButton),
    answerIsFromListBtn(new QRadioButton),
    minBox(new QSpinBox),
    maxBox(new QSpinBox),
    minLbl(new QLabel),
    maxLbl(new QLabel),
    warningLbl(new QLabel),
    listWidget(new NavCoinListWidget(nullptr, "", [](QString s)->bool{return !s.isEmpty();})),
    moreAnswersBox(new QCheckBox),
    cpos(consensuspos),
    title(title)
{
    this->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    this->setLayout(layout);
    this->setStyleSheet(Skinize());

    auto *bottomBox = new QFrame;
    auto *bottomBoxLayout = new QHBoxLayout;
    bottomBoxLayout->setContentsMargins(QMargins());
    bottomBox->setLayout(bottomBoxLayout);

    minLbl->setVisible(false);
    minBox->setVisible(false);

    QPushButton* createBtn = new QPushButton(tr("Submit"));
    connect(createBtn, SIGNAL(clicked()), this, SLOT(onCreateConsensus()));

    bottomBoxLayout->addStretch(1);
    bottomBoxLayout->addWidget(createBtn);

    warningLbl = new QLabel("");
    warningLbl->setObjectName("warning");
    warningLbl->setVisible(false);

    listWidget = new NavCoinListWidget(this, tr("Possible answers"), [this](QString s)->bool{
            return IsValidConsensusParameterProposal((Consensus::ConsensusParamsPos)cpos, RemoveFormatConsensusParameter((Consensus::ConsensusParamsPos)cpos, s.toStdString()), chainActive.Tip());
    });

    layout->addWidget(new QLabel(tr("Propose a new value for:")));
    layout->addSpacing(15);
    layout->addWidget(new QLabel(title));
    layout->addSpacing(15);
    layout->addWidget(listWidget);
    layout->addSpacing(15);
    layout->addWidget(warningLbl);
    layout->addWidget(bottomBox);
}

void DaoConsultationCreate::onCreate()
{
    if(!model)
        return;

    bool fRange = answerIsNumbersBtn->isChecked();
    int64_t nMin = minBox->value();
    int64_t nMax = maxBox->value();

    if (!fRange && (nMax < 1 || nMax > 16))
    {
        showWarning(tr("Choose a smaller number of answers."));
        return;
    }
    else if(fRange && !(nMin >= 0 && nMax < (uint64_t)-5 && nMax > nMin))
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

    QStringList listAnswers = listWidget->getEntries();

    UniValue strDZeel(UniValue::VOBJ);
    uint64_t nVersion = CConsultation::BASE_VERSION;

    if (fRange)
        nVersion |= CConsultation::ANSWER_IS_A_RANGE_VERSION;

    if (moreAnswersBox->isChecked())
        nVersion |= CConsultation::MORE_ANSWERS_VERSION;
    else if (listAnswers.size() < 2 && !fRange)
    {
        showWarning("You need to indicate at least two possible answers.");
        return;
    }

    showWarning("");

    UniValue answers(UniValue::VARR);

    for(auto &a: listAnswers)
    {
        answers.push_back(a.toStdString());
    }

    strDZeel.pushKV("q",sQuestion);
    strDZeel.pushKV("m",nMin);
    strDZeel.pushKV("n",nMax);
    strDZeel.pushKV("a",answers);
    strDZeel.pushKV("v",(uint64_t)nVersion);

    wtx.strDZeel = strDZeel.write();
    wtx.nCustomVersion = CTransaction::CONSULTATION_VERSION;

    CAmount nMinFee = GetConsensusParameter(Consensus::CONSENSUS_PARAM_CONSULTATION_MIN_FEE) + GetConsensusParameter(Consensus::CONSENSUS_PARAM_CONSULTATION_ANSWER_MIN_FEE) * answers.size();

    // Ensure wallet is unlocked
    WalletModel::UnlockContext ctx(model->requestUnlock());
    if(!ctx.isValid())
    {
        // Unlock wallet was cancelled
        return;
    }

    // Check balance
    CAmount curBalance = pwalletMain->GetBalance();
    if (curBalance <= GetConsensusParameter(Consensus::CONSENSUS_PARAM_CONSULTATION_MIN_FEE)) {
        QMessageBox msgBox(this);
        string fee = FormatMoney(nMinFee);
        std::string str = tr("You require at least %1 NAV mature and available to create a consultation.\n").arg(QString::fromStdString(fee)).toStdString();
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
    CAmount nValue = nMinFee;
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
        if (QMessageBox::Yes == QMessageBox(QMessageBox::Information,
                                            tr("Success!"),
                                            tr("Your consultation has been correctly created.")+"<br><br>"+tr("Now you need to find support from stakers so the voting can start!")+"<br><br>"+tr("Do you want to support your own consultation/answers?"),
                                            QMessageBox::Yes|QMessageBox::No).exec())
        {
            bool duplicate;
            if (fRange)
                Support(wtx.hash, duplicate);
            else {
                CConsultation consultation;
                std::vector<CConsultationAnswer> vAnswers;
                if (TxToConsultation(wtx.strDZeel, wtx.GetHash(), uint256(), consultation, vAnswers))
                {
                    for (auto &it: vAnswers)
                    {
                        Support(it.hash, duplicate);
                    }
                }
            }
        }
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

void DaoConsultationCreate::setModel(WalletModel *model)
{
    this->model = model;
}

void DaoConsultationCreate::onCreateConsensus()
{
    if(!model)
        return;

    bool fRange = answerIsNumbersBtn->isChecked();
    int64_t nMin = cpos;
    int64_t nMax = 1;

    LOCK2(cs_main, pwalletMain->cs_wallet);

    CNavCoinAddress address("NQFqqMUD55ZV3PJEJZtaKCsQmjLT6JkjvJ"); // Dummy address

    CWalletTx wtx;
    bool fSubtractFeeFromAmount = false;

    std::string sQuestion = "Consensus change for: " + title.toStdString();

    if (sQuestion == "")
    {
        showWarning("The question can not be empty!");
        return;
    }

    if (cpos < 0 || cpos >= Consensus::MAX_CONSENSUS_PARAMS)
    {
        showWarning("Something went wrong!");
        return;
    }

    QStringList listAnswers = listWidget->getEntries();

    QMutableStringListIterator it(listAnswers);
    while (it.hasNext()) {
        QString& current = it.next();
        it.setValue(QString::fromStdString(RemoveFormatConsensusParameter((Consensus::ConsensusParamsPos)nMin, current.toStdString())));
    }

    UniValue strDZeel(UniValue::VOBJ);
    uint64_t nVersion = CConsultation::BASE_VERSION | CConsultation::MORE_ANSWERS_VERSION | CConsultation::CONSENSUS_PARAMETER_VERSION;

    if (listAnswers.size() < 1)
    {
        showWarning("You need to indicate at least one possible answer.");
        return;
    }

    showWarning("");

    UniValue answers(UniValue::VARR);

    for(auto &a: listAnswers)
    {
        answers.push_back(a.toStdString());
    }

    strDZeel.pushKV("q",sQuestion);
    strDZeel.pushKV("m",nMin);
    strDZeel.pushKV("n",nMax);
    strDZeel.pushKV("a",answers);
    strDZeel.pushKV("v",(uint64_t)nVersion);

    wtx.strDZeel = strDZeel.write();
    wtx.nCustomVersion = CTransaction::CONSULTATION_VERSION;

    CAmount nMinFee = GetConsensusParameter(Consensus::CONSENSUS_PARAM_CONSULTATION_MIN_FEE) + GetConsensusParameter(Consensus::CONSENSUS_PARAM_CONSULTATION_ANSWER_MIN_FEE) * answers.size();

    // Ensure wallet is unlocked
    WalletModel::UnlockContext ctx(model->requestUnlock());
    if(!ctx.isValid())
    {
        // Unlock wallet was cancelled
        return;
    }

    // Check balance
    CAmount curBalance = pwalletMain->GetBalance();
    if (curBalance <= nMinFee) {
        QMessageBox msgBox(this);
        string fee = FormatMoney(nMinFee);
        std::string str = tr("You require at least %1 NAV mature and available to create a proposal for a consensus change.\n").arg(QString::fromStdString(fee)).toStdString();
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
    CAmount nValue = nMinFee;
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
        if (QMessageBox::Yes == QMessageBox(QMessageBox::Information,
                                            tr("Success!"),
                                            tr("Your proposal for a consensus change has been correctly created.")+"<br><br>"+tr("Now you need to find support from stakers so the voting can start!")+"<br><br>"+tr("Do you want to support your own consultation?"),
                                            QMessageBox::Yes|QMessageBox::No).exec())
        {
            bool duplicate;
            CConsultation consultation;
            std::vector<CConsultationAnswer> vAnswers;
            if (TxToConsultation(wtx.strDZeel, wtx.GetHash(), uint256(), consultation, vAnswers))
            {
                for (auto &it: vAnswers)
                {
                    Support(it.hash, duplicate);
                }
            }
        }
        QDialog::accept();
        return;
    }
    else {
        // Display something went wrong UI
        QMessageBox msgBox(this);
        msgBox.setText(tr("Proposal creation failed."));
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
    adjustSize();
}

void DaoConsultationCreate::onRange(bool fChecked)
{
    if (fChecked)
    {
        minLbl->setVisible(true);
        minBox->setVisible(true);
        listWidget->setVisible(false);
        moreAnswersBox->setVisible(false);
        maxLbl->setText(tr("and"));
    }
    else
    {
        minLbl->setVisible(false);
        minBox->setVisible(false);
        listWidget->setVisible(true);
        moreAnswersBox->setVisible(true);
        maxLbl->setText(tr("Maximum amount of answers stakers can select at the same time"));

    }
    adjustSize();
}

void DaoConsultationCreate::onList(bool fChecked)
{

}
