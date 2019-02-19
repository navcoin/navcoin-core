#include "sendcommunityfunddialog.h"
#include "ui_sendcommunityfunddialog.h"

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
