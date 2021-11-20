// Copyright (c) 2019-2020 The Navcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "daosupport.h"

DaoSupport::DaoSupport(QWidget *parent, CConsultation consultation) :
    layout(new QVBoxLayout),
    consultation(consultation),
    answerBox(new QGroupBox)
{
    this->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Expanding);
    this->setLayout(layout);

    auto *bottomBox = new QFrame;
    auto *bottomBoxLayout = new QHBoxLayout;
    bottomBoxLayout->setContentsMargins(QMargins());
    bottomBox->setLayout(bottomBoxLayout);

    QPushButton* closeBtn = new QPushButton(tr("Close"));
    connect(closeBtn, SIGNAL(clicked()), this, SLOT(onClose()));

    warningLbl = new QLabel("");
    warningLbl->setObjectName("warning");
    warningLbl->setVisible(false);

    answerBox = new QGroupBox("");
    QVBoxLayout *vbox = new QVBoxLayout;
    QVector<QCheckBox*> answers;

    CConsultationAnswerMap consultationAnswerMap;

    if (pcoinsTip && pcoinsTip->GetAllConsultationAnswers(consultationAnswerMap))
    {
        CStateViewCache coins(pcoinsTip);

        for (auto& it: consultationAnswerMap)
        {
            CConsultationAnswer c = it.second;

            if (c.parent != consultation.hash)
                continue;

            QString s = "";

            if (consultation.IsSuper())
            {
                for (int i = 0; i < c.vAnswer.size(); ++i) {
                    if (i != 0)
                        s = s + "\n";
                    s = s + QString::fromStdString(Consensus::sConsensusParamsDesc[consultation.vParameters[i]] + ": ");
                    s = s + QString::fromStdString(consultation.IsAboutConsensusParameter() ? FormatConsensusParameter((Consensus::ConsensusParamsPos) consultation.vParameters[i], c.vAnswer[i]) : c.vAnswer[i]);
                }
            }
            else
            {
                s = QString::fromStdString(consultation.IsAboutConsensusParameter() ? FormatConsensusParameter((Consensus::ConsensusParamsPos) consultation.nMin, c.sAnswer) : c.sAnswer);
            }

            QCheckBox* answer = new QCheckBox(s);

            answers << answer;
            answer->setProperty("id", QString::fromStdString(c.hash.GetHex()));
            connect(answer, SIGNAL(toggled(bool)), this, SLOT(onSupport(bool)));

            auto v = mapSupported.find(c.hash);

            if (v != mapSupported.end())
            {
                answer->setChecked(true);
            }

            vbox->addWidget(answer);
        }
    }

    if (answers.size() == 0)
        vbox->addWidget(new QLabel(tr("There are no proposed answers")));

    QPushButton* proposeBtn = new QPushButton(answers.size() == 0 ? tr("Propose an answer") : tr("Propose a different answer"));
    connect(proposeBtn, SIGNAL(clicked()), this, SLOT(onPropose()));

    bottomBoxLayout->addStretch(1);
    if (consultation.CanHaveNewAnswers())
        bottomBoxLayout->addWidget(proposeBtn);
    bottomBoxLayout->addWidget(closeBtn);

    vbox->addStretch(1);
    answerBox->setLayout(vbox);

    layout->addSpacing(15);
    QLabel* titleLabel = new QLabel(tr("Consultation:<br><br>%1").arg(QString::fromStdString(consultation.strDZeel)));
    titleLabel->setWordWrap(true) ;
    layout->addWidget(titleLabel);
    layout->addSpacing(15);
    layout->addWidget(new QLabel(tr("Possible answers:")));
    layout->addWidget(answerBox);
    if (answers.size() > 0)
    {
        layout->addWidget(new QLabel(tr("Check the box from the answers you consider valid for the aforementioned question in order to support them.")));
        layout->addWidget(new QLabel(tr("Consultations need to gather a minimal support from the network before being allowed to start with the voting.")));
        layout->addWidget(new QLabel(tr("Supporting an answer does not mean voting for it, but just agreeing with the answer being part of the final set of possible answers.")));
    }
    layout->addSpacing(15);
    layout->addWidget(warningLbl);
    layout->addWidget(bottomBox);
    layout->addSpacing(15);
}

void DaoSupport::setModel(WalletModel *model)
{
    this->model = model;
}

void DaoSupport::showWarning(QString text)
{
    warningLbl->setText(text);
    warningLbl->setVisible(text == "" ? false : true);
    adjustSize();
}

void DaoSupport::onSupport(bool fChecked)
{
    LOCK(cs_main);

    QVariant idV = sender()->property("id");
    QCheckBox *clickedBox = qobject_cast<QCheckBox*>(sender());

    if (idV.isValid())
    {
        CStateViewCache coins(pcoinsTip);

        warningLbl->setVisible(false);
        adjustSize();

        uint256 hash = uint256S(idV.toString().toStdString());

        CConsultationAnswer answer;

        if (coins.HaveConsultationAnswer(hash) && coins.GetConsultationAnswer(hash, answer) &&
                answer.CanBeSupported(coins))
        {
            bool duplicate;
            if (fChecked)
                Support(hash, duplicate);
            else
                RemoveSupport(hash.ToString());
        }
    }
}

void DaoSupport::onClose()
{
    close();
}

void DaoSupport::onPropose()
{
    DaoProposeAnswer dlg(this, consultation, [this](QString s)->bool{
        try
        {
            return consultation.IsAboutConsensusParameter() ? IsValidConsensusParameterProposal((Consensus::ConsensusParamsPos)consultation.nMin, RemoveFormatConsensusParameter((Consensus::ConsensusParamsPos)consultation.nMin, s.toStdString()), chainActive.Tip(), *pcoinsTip) : !s.isEmpty();
        }
        catch (...)
        {
            return false;
        }
    });
    dlg.setModel(model);
    dlg.exec();
    close();
}
