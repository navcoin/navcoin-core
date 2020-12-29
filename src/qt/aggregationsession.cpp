// Copyright (c) 2020 The NavCoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "aggregationsession.h"

AggregationSessionDialog::AggregationSessionDialog(QWidget *parent)
    :
      QDialog(parent), layout(new QVBoxLayout), topStatus(0), bottomStatus(0), bottom2Status(0), fReady(false)
{
    this->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    this->setLayout(layout);

    topStatus = new QLabel(tr("Announcing mix session to other peers."));
    topStatus->setStyleSheet("font-weight: bold");
    topStatus->setAlignment(Qt::AlignCenter);
    QProgressBar* progressBar = new QProgressBar();
    progressBar->setMaximum(0);
    progressBar->setMinimum(0);
    progressBar->setValue(0);
    bottomStatus = new QLabel(tr("Please wait..."));
    bottomStatus->setAlignment(Qt::AlignCenter);
    bottom2Status = new QLabel("");
    bottom2Status->setAlignment(Qt::AlignCenter);

    layout->setContentsMargins(20 * GUIUtil::scale(), 20 * GUIUtil::scale(), 20 * GUIUtil::scale(), 20 * GUIUtil::scale());
    layout->addWidget(topStatus);
    layout->addWidget(progressBar);
    layout->addWidget(bottomStatus);
    layout->addWidget(bottom2Status);

    if (!pwalletMain)
        ShowError(tr("Wallet is not loaded."));

    if (!pwalletMain->aggSession)
        ShowError(tr("Error internal structures."));

    auto nCount = 0;

    if (pwalletMain && pwalletMain->aggSession)
        nCount = pwalletMain->aggSession->GetTransactionCandidates().size();
    else
    {
        fReady = true;
        this->done(false);
        return;
    }

    if (nCount > GetArg("-defaultmixin", DEFAULT_TX_MIXCOINS)*2)
    {
        fReady = true;
    }
    else
    {
        if (!pwalletMain->aggSession->Start())
        {
            pwalletMain->aggSession->Stop();
            if (!pwalletMain->aggSession->Start())
            {
                ShowError(tr("Could not start mix session, try restarting your wallet."));
            }
        }
    }

    nStart = GetTime();


    QTimer *timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(CheckStatus()));
    timer->start(1000);
}

void AggregationSessionDialog::setClientModel(ClientModel *clientModel)
{
    this->clientModel = clientModel;

    if (clientModel) {
        connect(clientModel, SIGNAL(newAggregationSession(std::string)), this, SLOT(NewAggregationSession(std::string)));
    }
}

void AggregationSessionDialog::NewAggregationSession(std::string hs)
{
    if (pwalletMain && pwalletMain->aggSession && pwalletMain->aggSession->GetHiddenService() == hs)
    {
        QSettings settings;
        SetTopLabel(tr("Session started."));
        SetTopLabel(tr("Waiting for coin candidates..."));
        fReady = true;
        nExpiresAt = GetTime()+settings.value("aggregationSessionWait", 15).toInt();
    }
}

void AggregationSessionDialog::CheckStatus()
{
    if (!fReady && vNodes.size() > 0)
    {
        if (GetTime() > nStart + 60)
        {
            fReady = false;
            this->done(false);
        }

        SetBottomLabel(QString(tr("Waiting %1 seconds more...").arg(nStart + 60 - GetTime())));

        return;
    }

    fReady = false;

    size_t nCount = 0;

    if (pwalletMain && pwalletMain->aggSession && vNodes.size() > 0)
        nCount = pwalletMain->aggSession->GetTransactionCandidates().size();
    else
    {
        fReady = true;
        this->done(false);
        return;
    }

    if (nExpiresAt < GetTime() || nCount > GetArg("-defaultmixin", DEFAULT_TX_MIXCOINS)*2)
    {
        SetTopLabel("We have enough coins to mix.");
        SetBottomLabel("Aggregating coin inputs...");
        SetBottom2Label("");
        bool ret = true;
        {
            LOCK(cs_main);
            if (!pwalletMain->aggSession->SelectCandidates(this->selectedCoins))
            {
                QMessageBox::critical(this, tr("Something failed"), tr("There was an error trying to validate the coins after mixing."));
                ret = false;
            }
        }
        this->done(ret);
        fReady = ret;
        return;
    }
    else if (nCount > 0)
    {
        SetBottomLabel(QString(tr("We have %1 coin candidate(s)...").arg(nCount)));
    }
    SetBottom2Label(QString(tr("Waiting %1 seconds more...").arg(nExpiresAt-GetTime())));
    fReady = true;
}

CandidateTransaction AggregationSessionDialog::GetSelectedCoins()
{
    return selectedCoins;
}

void AggregationSessionDialog::setWalletModel(WalletModel *model)
{
    this->walletModel = model;
}

void AggregationSessionDialog::ShowError(QString string)
{
    QMessageBox::critical(this, tr("Something failed"), string);
    this->done(0);
}

void AggregationSessionDialog::SetTopLabel(QString string)
{
    topStatus->setText(string);
}

void AggregationSessionDialog::SetBottomLabel(QString string)
{
    bottomStatus->setText(string);
}

void AggregationSessionDialog::SetBottom2Label(QString string)
{
    bottom2Status->setText(string);
}
