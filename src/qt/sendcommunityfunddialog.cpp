#include "sendcommunityfunddialog.h"
#include "ui_sendcommunityfunddialog.h"

#include <QSettings>
#include <guiutil.h>

//temp headers
#include <iostream>

SendCommunityFundDialog::SendCommunityFundDialog(QWidget *parent, CFund::CProposal* proposal, int secDelay) :
    QDialog(parent),
    ui(new Ui::SendCommunityFundDialog),
    proposal(proposal),
    prequest(0),
    secDelay(secDelay)
{
    ui->setupUi(this);

    connect(&countDownTimer, SIGNAL(timeout()), this, SLOT(countDown()));
    connect(ui->pushButtonYes, SIGNAL(clicked()), this, SLOT(accept()));
    connect(ui->pushButtonCancel, SIGNAL(clicked()), this, SLOT(reject()));

    ui->pushButtonCancel->setDefault(true);
    updateYesButton();

    // Set UI elements to proposal view
    ui->labelProposalHashTitle->setVisible(false);
    ui->labelProposalHash->setVisible(false);
    ui->labelAddress->setText(QString(proposal->Address.c_str()));

    // Amount label
    QSettings settings;
    ui->labelRequestedAmount->setText(QString("%1 NAV / ").arg(proposal->nAmount/100000000.0).append("%1 EUR / ").arg(proposal->nAmount / settings.value("eurFactor", 0).toFloat()).append("%2 USD / ").arg(proposal->nAmount / settings.value("usdFactor", 0).toFloat()).append("%3 BTC").arg(proposal->nAmount / settings.value("btcFactor", 0).toFloat()));

    ui->labelDescription->setText(QString(proposal->strDZeel.c_str()));
    ui->labelDuration->setText(GUIUtil::formatDurationStr(int(proposal->nDeadline)));
}

SendCommunityFundDialog::SendCommunityFundDialog(QWidget *parent, CFund::CPaymentRequest* prequest, int secDelay) :
    QDialog(parent),
    ui(new Ui::SendCommunityFundDialog),
    proposal(0),
    prequest(prequest),
    secDelay(secDelay)
{
    ui->setupUi(this);

    ui->pushButtonCancel->setDefault(true);
    updateYesButton();

    connect(&countDownTimer, SIGNAL(timeout()), this, SLOT(countDown()));
    connect(ui->pushButtonYes, SIGNAL(clicked()), this, SLOT(accept()));
    connect(ui->pushButtonCancel, SIGNAL(clicked()), this, SLOT(reject()));

    // Set UI elements to payment request view
    ui->labelQuestionTitle->setText(QString("Are you sure you would like to create the following payment request?"));
    ui->labelAddressTitle->setVisible(false);
    ui->labelAddress->setVisible(false);
    ui->labelDurationTitle->setVisible(false);
    ui->labelDuration->setVisible(false);
    ui->labelWarning->setVisible(false);

    ui->labelProposalHash->setText(QString(prequest->proposalhash.ToString().c_str()));
    ui->labelDescription->setText(QString(prequest->strDZeel.c_str()));

    // Amount label
    QSettings settings;
    ui->labelRequestedAmount->setText(QString("%1 NAV / ").arg(prequest->nAmount/100000000.0).append("%1 EUR / ").arg(prequest->nAmount / settings.value("eurFactor", 0).toFloat()).append("%2 USD / ").arg(prequest->nAmount / settings.value("usdFactor", 0).toFloat()).append("%3 BTC").arg(prequest->nAmount / settings.value("btcFactor", 0).toFloat()));
}

void SendCommunityFundDialog::updateYesButton()
{
    if(secDelay > 0)
    {
        ui->pushButtonYes->setEnabled(false);
        ui->pushButtonYes->setText(tr("Yes") + " (" + QString::number(secDelay) + ")");
    }
    else
    {
        ui->pushButtonYes->setEnabled(true);
        ui->pushButtonYes->setText(tr("Yes"));
    }
}

void SendCommunityFundDialog::countDown()
{
    secDelay--;
    updateYesButton();

    if(secDelay <= 0)
    {
        countDownTimer.stop();
    }
}

int SendCommunityFundDialog::exec()
{
    updateYesButton();
    countDownTimer.start(1000);
    return QDialog::exec();
}

SendCommunityFundDialog::~SendCommunityFundDialog()
{
    delete ui;
}
