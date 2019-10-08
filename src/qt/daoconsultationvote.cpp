// Copyright (c) 2019 The NavCoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "daoconsultationvote.h"

DaoConsultationVote::DaoConsultationVote(QWidget *parent, CConsultation consultation) :
    layout(new QVBoxLayout),
    questionLbl(new QLabel),
    consultation(consultation),
    answersBox(new QGroupBox),
    warningLbl(new QLabel),
    amountBox(new QSpinBox),
    nChecked(0)
{
    this->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    this->setLayout(layout);
    this->setStyleSheet(Skinize());

    QFont subtitleFnt("Sans Serif", 18, QFont::Bold);

    auto *topBox = new QFrame;
    auto *topBoxLayout = new QHBoxLayout;
    topBoxLayout->setContentsMargins(QMargins());
    topBox->setLayout(topBoxLayout);

    questionLbl = new QLabel(QString::fromStdString(consultation.strDZeel));
    questionLbl->setFont(subtitleFnt);

    topBoxLayout->addSpacing(15);
    topBoxLayout->addWidget(questionLbl, 0, Qt::AlignCenter);
    topBoxLayout->addSpacing(15);

    auto *middleBox = new QFrame;
    auto *middleBoxLayout = new QHBoxLayout;
    middleBoxLayout->setContentsMargins(QMargins());
    middleBox->setLayout(middleBoxLayout);

    amountBox = new QSpinBox();
    amountBox->setMinimum(consultation.nMin);
    amountBox->setMaximum(consultation.nMax);

    middleBoxLayout->addSpacing(15);
    middleBoxLayout->addWidget(amountBox, 0, Qt::AlignCenter);
    middleBoxLayout->addSpacing(15);

    if (consultation.IsRange())
    {
        auto v = mapAddedVotes.find(consultation.hash);

        if (v != mapAddedVotes.end())
        {
            amountBox->setValue(v->second);
        }
        else
        {
            amountBox->setValue((consultation.nMax - consultation.nMin) / 2);
        }
    }

    answersBox = new QGroupBox("");
    QVBoxLayout *vbox = new QVBoxLayout;

    CConsultationAnswerMap consultationAnswerMap;

    if (pcoinsTip && pcoinsTip->GetAllConsultationAnswers(consultationAnswerMap))
    {
        CStateViewCache coins(pcoinsTip);

        for (auto& it: consultationAnswerMap)
        {
            CConsultationAnswer c = it.second;

            if (!c.CanBeVoted(coins))
                continue;

            if (c.parent != consultation.hash)
                continue;

            std::string sAnswer = c.sAnswer;

            if (consultation.IsAboutConsensusParameter())
                sAnswer = FormatConsensusParameter((Consensus::ConsensusParamsPos)consultation.nMin, c.sAnswer);

            QCheckBox* answer = new QCheckBox(QString::fromStdString(sAnswer));

            answers << answer;
            answer->setProperty("id", QString::fromStdString(c.hash.GetHex()));
            connect(answer, SIGNAL(toggled(bool)), this, SLOT(onAnswer(bool)));

            auto v = mapAddedVotes.find(c.hash);

            if (v != mapAddedVotes.end())
            {
                answer->setChecked(true);
            }

            vbox->addWidget(answer);
        }
    }

    vbox->addStretch(1);
    answersBox->setLayout(vbox);

    auto *bottomBox = new QFrame;
    auto *bottomBoxLayout = new QHBoxLayout;
    bottomBoxLayout->setContentsMargins(QMargins());
    bottomBox->setLayout(bottomBoxLayout);

    QPushButton* abstainBtn = new QPushButton(tr("Abstain"));
    connect(abstainBtn, SIGNAL(clicked()), this, SLOT(onAbstain()));

    QPushButton* stopBtn = new QPushButton(tr("Stop voting"));
    connect(stopBtn, SIGNAL(clicked()), this, SLOT(onRemove()));

    QPushButton* closeBtn = new QPushButton(tr("Save"));
    connect(closeBtn, SIGNAL(clicked()), this, SLOT(onClose()));

    bottomBoxLayout->addWidget(abstainBtn);
    bottomBoxLayout->addWidget(stopBtn);
    bottomBoxLayout->addStretch(1);
    bottomBoxLayout->addWidget(closeBtn);

    warningLbl = new QLabel(QString("You can only select a maximum of %1 answer(s).").arg(consultation.nMax));
    warningLbl->setObjectName("warning");
    warningLbl->setFont(subtitleFnt);
    warningLbl->setVisible(false);

    layout->addSpacing(15);
    layout->addWidget(topBox);
    layout->addSpacing(15);
    layout->addWidget(new QLabel(consultation.nMax == 1 || consultation.IsRange() ? tr("Your vote:") : tr("Your votes (you can select up to %1):").arg(consultation.nMax)));
    if (consultation.IsRange())
    {
        layout->addWidget(middleBox);
    }
    else
    {
        layout->addWidget(answersBox);
        layout->addWidget(warningLbl);
    }
    layout->addSpacing(15);
    layout->addWidget(bottomBox);
    layout->addSpacing(15);

}

void DaoConsultationVote::onAnswer(bool fChecked)
{
    LOCK(cs_main);

    QVariant idV = sender()->property("id");
    QCheckBox *clickedBox = qobject_cast<QCheckBox*>(sender());

    if (idV.isValid())
    {
        CStateViewCache coins(pcoinsTip);

        nChecked = nChecked + (fChecked ? 1 : -1);

        if (fChecked && nChecked > consultation.nMax)
        {
            clickedBox->setChecked(false);
            warningLbl->setVisible(true);
            adjustSize();
            return;
        }

        warningLbl->setVisible(false);
        adjustSize();

        uint256 hash = uint256S(idV.toString().toStdString());

        CConsultationAnswer answer;

        if (coins.HaveConsultationAnswer(hash) && coins.GetConsultationAnswer(hash, answer) &&
                answer.CanBeVoted(coins))
        {
            bool duplicate;
            if (fChecked)
                Vote(hash, 1, duplicate);
            else
                RemoveVote(hash);
        }
    }
}

void DaoConsultationVote::onAbstain()
{
    for (auto& it: answers)
    {
        it->setChecked(false);
    }

    bool duplicate;

    Vote(consultation.hash, -1, duplicate);

    close();
}

void DaoConsultationVote::onRemove()
{
    for (auto& it: answers)
    {
        it->setChecked(false);
    }

    bool duplicate;

    RemoveVote(consultation.hash);

    close();
}

void DaoConsultationVote::onClose()
{
    if (consultation.IsRange())
    {
        bool duplicate;
        if (consultation.IsValidVote(amountBox->value()))
            VoteValue(consultation.hash, amountBox->value(), duplicate);
    }
    close();
}
