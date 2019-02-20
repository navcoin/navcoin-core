#include "communityfundsuccessdialog.h"
#include "ui_communityfundsuccessdialog.h"
#include <QDialog>

CommunityFundSuccessDialog::CommunityFundSuccessDialog(QWidget *parent, CFund::CPaymentRequest* prequest) :
    QDialog(parent),
    ui(new Ui::CommunityFundSuccessDialog),
    proposal(0),
    prequest(prequest)
{
    ui->setupUi(this);
    connect(ui->pushButtonClose, SIGNAL(clicked()), this, SLOT(reject()));

    // Generate label for payment request
    ui->label->setTextFormat(Qt::RichText);
    ui->label->setText(QString::fromStdString("You can now view your payment request in the "
                                                             "core wallet or <a href=\"https://www.navexplorer.com/community-fund/payment-request/"
                                              + prequest->hash.ToString() + "\">on the navexplorer</a>"));
    ui->label->setTextInteractionFlags(Qt::TextBrowserInteraction);
    ui->label->setOpenExternalLinks(true);
}

CommunityFundSuccessDialog::CommunityFundSuccessDialog(QWidget *parent, CFund::CProposal* proposal) :
    QDialog(parent),
    ui(new Ui::CommunityFundSuccessDialog),
    proposal(proposal),
    prequest(0)
{
    ui->setupUi(this);
    connect(ui->pushButtonClose, SIGNAL(clicked()), this, SLOT(reject()));

    // Generate label for proposal
    ui->label->setTextFormat(Qt::RichText);
    ui->label->setText(QString::fromStdString("You can now view your proposal in the "
                                                             "core wallet or <a href=\"https://www.navexplorer.com/community-fund/proposal/"
                                              + proposal->hash.ToString() + "\">on the navexplorer</a>"));
    ui->label->setTextInteractionFlags(Qt::TextBrowserInteraction);
    ui->label->setOpenExternalLinks(true);
}

CommunityFundSuccessDialog::~CommunityFundSuccessDialog()
{
    delete ui;
}
