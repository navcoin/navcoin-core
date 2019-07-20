// Copyright (c) 2019 The NavCoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "daosupport.h"

DaoSupport::DaoSupport(QWidget *parent, CConsultation consultation) :
    layout(new QVBoxLayout),
    consultation(consultation),
    answerBox(new QGroupBox)
{
    this->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    this->setLayout(layout);
    this->setStyleSheet(Skinize());

    auto *bottomBox = new QFrame;
    auto *bottomBoxLayout = new QHBoxLayout;
    bottomBoxLayout->setContentsMargins(QMargins());
    bottomBox->setLayout(bottomBoxLayout);

    QPushButton* closeBtn = new QPushButton(tr("Close"));
    connect(closeBtn, SIGNAL(clicked()), this, SLOT(onClose()));

    bottomBoxLayout->addStretch(1);
    bottomBoxLayout->addWidget(closeBtn);

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

            QCheckBox* answer = new QCheckBox(QString::fromStdString(c.sAnswer));

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
        vbox->addWidget(new QLabel(tr("This consultation has no proposed answers")));

    vbox->addStretch(1);
    answerBox->setLayout(vbox);

    layout->addSpacing(15);
    layout->addWidget(new QLabel(tr("Consultation:<br>%1").arg(QString::fromStdString(consultation.strDZeel))));
    layout->addSpacing(15);
    layout->addWidget(new QLabel(tr("Proposed answers (check the box to support):")));
    layout->addWidget(answerBox);
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
