#include "sendcommunityfunddialog.h"
#include "ui_sendcommunityfunddialog.h"

#include <QSettings>
#include <guiutil.h>
#include <vector>
#include <string>
#include <sstream>
#include "consensus/dao.h"
#include "main.h"
#include "base58.h"
#include "chain.h"


SendCommunityFundDialog::SendCommunityFundDialog(QWidget *parent, CProposal* proposal, int secDelay) :
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
    ui->labelRequestedAmount->setText(QString("%1 NAV / ").arg(proposal->nAmount/COIN).append("%1 EUR / ").arg(proposal->nAmount / settings.value("eurFactor", 0).toFloat()).append("%2 USD / ").arg(proposal->nAmount / settings.value("usdFactor", 0).toFloat()).append("%3 BTC").arg(proposal->nAmount / settings.value("btcFactor", 0).toFloat()));

    // Format long descriptions
    std::string description = proposal->strDZeel.c_str();
    std::istringstream buf(description);
    std::istream_iterator<std::string> beg(buf), end;
    std::string finalDescription = "";
    std::vector<std::string> words(beg, end);
    for(std::string word : words) {
        unsigned int count = 0;
        while(count < word.length()-1) {
            if (count % 40 == 0 && count != 0) {
                word.insert(count, "\n");
            }
            count++;
        }
        finalDescription += word + " ";
    }

    ui->labelDescription->setText(QString::fromStdString(finalDescription));
    ui->labelDuration->setText(GUIUtil::formatDurationStr(int(proposal->nDeadline)));

    string fee = FormatMoney(Params().GetConsensus().nProposalMinimalFee);
    string warning = "By submitting the proposal a " + fee + " NAV deduction will occur from your wallet ";
    ui->labelWarning->setText(QString::fromStdString(warning));
}

SendCommunityFundDialog::SendCommunityFundDialog(QWidget *parent, CPaymentRequest* prequest, int secDelay) :
    QDialog(parent),
    ui(new Ui::SendCommunityFundDialog),
    proposal(0),
    prequest(prequest),
    secDelay(secDelay)
{
    ui->setupUi(this);

    QDialog::setWindowTitle("Confirm Payment Request Details");

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
    ui->labelRequestedAmount->setText(QString("%1 NAV / ").arg(prequest->nAmount/COIN).append("%1 EUR / ").arg(prequest->nAmount / settings.value("eurFactor", 0).toFloat()).append("%2 USD / ").arg(prequest->nAmount / settings.value("usdFactor", 0).toFloat()).append("%3 BTC").arg(prequest->nAmount / settings.value("btcFactor", 0).toFloat()));

    // Format long descriptions
    std::string description = prequest->strDZeel.c_str();
    std::istringstream buf(description);
    std::istream_iterator<std::string> beg(buf), end;
    std::string finalDescription = "";
    std::vector<std::string> words(beg, end);
    for(std::string word : words) {
        unsigned int count = 0;
        while(count < word.length()-1) {
            if (count % 40 == 0 && count != 0) {
                word.insert(count, "\n");
            }
            count++;
        }
        finalDescription += word + " ";
    }

    ui->labelDescription->setText(QString::fromStdString(finalDescription));
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
        countDownTimer.stop();
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
