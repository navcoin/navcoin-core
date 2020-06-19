// Copyright (c) 2020 The NavCoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "aggregationsession.h"

AggregationSesionDialog::AggregationSesionDialog(QWidget *parent)
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

    {
        LOCK(pwalletMain->cs_wallet);
        if (!pwalletMain->aggSession)
            ShowError(tr("Error internal structures."));

        if (!pwalletMain->aggSession->Start())
        {
            pwalletMain->aggSession->Stop();
            if (!pwalletMain->aggSession->Start())
            {
                ShowError(tr("Could not start mix session, try restarting your wallet."));
            }
        }
    }

    QTimer *timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(CheckStatus()));
    timer->start(1000);
}

void AggregationSesionDialog::setClientModel(ClientModel *clientModel)
{
    this->clientModel = clientModel;

    if (clientModel) {
        connect(clientModel, SIGNAL(newAggregationSesion(std::string)), this, SLOT(NewAggregationSesion(std::string)));
    }
}

void AggregationSesionDialog::NewAggregationSesion(std::string hs)
{
    if (pwalletMain && pwalletMain->aggSession && pwalletMain->aggSession->GetHiddenService() == hs)
    {
        SetTopLabel(tr("Session started."));
        SetTopLabel(tr("Waiting for coin candidates..."));
        fReady = true;
        nExpiresAt = GetTime()+10;
    }
}

void AggregationSesionDialog::CheckStatus()
{
    if (!fReady)
        return;

    fReady = false;

    size_t nCount = 0;

    if (pwalletMain && pwalletMain->aggSession)
        nCount = pwalletMain->aggSession->GetTransactionCandidates().size();
    else
    {
        fReady = true;
        return;
    }

    if (nExpiresAt < GetTime())
    {
        SetTopLabel("Coins received");
        SetBottomLabel("Combining coins...");
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
        fReady = true;
    }
    else if (nCount > 0)
    {
        SetBottomLabel(QString(tr("Received %1 coin candidate(s)...").arg(nCount)));
    }
    SetBottom2Label(QString(tr("%1 seconds left...").arg(nExpiresAt-GetTime())));
    fReady = true;
}

CandidateTransaction AggregationSesionDialog::GetSelectedCoins()
{
    return selectedCoins;
}

void AggregationSesionDialog::setWalletModel(WalletModel *model)
{
    this->walletModel = model;
}

void AggregationSesionDialog::ShowError(QString string)
{
    QMessageBox::critical(this, tr("Something failed"), string);
    this->done(0);
}

void AggregationSesionDialog::SetTopLabel(QString string)
{
    topStatus->setText(string);
}

void AggregationSesionDialog::SetBottomLabel(QString string)
{
    bottomStatus->setText(string);
}

void AggregationSesionDialog::SetBottom2Label(QString string)
{
    bottom2Status->setText(string);
}
