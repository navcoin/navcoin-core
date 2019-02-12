#include "communityfundcreateproposaldialog.h"
#include "ui_communityfundcreateproposaldialog.h"
#include "guiconstants.h"
#include "guiutil.h"

CommunityFundCreateProposalDialog::CommunityFundCreateProposalDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CommunityFundCreateProposalDialog)
{
    ui->setupUi(this);
    GUIUtil::setupAddressWidget(ui->lineEditNavcoinAddress, this);

    //maybe set as vecot rof QLineEdit instead?

    ui->spinBoxDays->setMinimum(0);
    ui->spinBoxDays->setMaximum(999999999);
    ui->spinBoxHours->setMinimum(0);
    ui->spinBoxHours->setMaximum(59);
    ui->spinBoxMinutes->setMinimum(0);
    ui->spinBoxMinutes->setMaximum(59);

    //connect
    connect(ui->pushButtonClose, SIGNAL(clicked()), this, SLOT(reject()));
    connect(ui->pushButtonCreateProposal, SIGNAL(clicked()), this, SLOT(on_click_pushButtonCreateProposal()));
}

// validate input fields
bool CommunityFundCreateProposalDialog::validate()
{
    bool isValid = true;
    if(!ui->lineEditNavcoinAddress->isValid())
    {
        ui->lineEditNavcoinAddress->setValid(false);
        isValid = false;
    }
    if(!ui->lineEditRequestedAmount->validate())
    {
        ui->lineEditRequestedAmount->setValid(false);
        isValid = false;
    }
    if(ui->plainTextEditDescription->toPlainText() == QString(""))
    {
        ui->plainTextEditDescription->setStyleSheet(STYLE_INVALID);
        isValid = false;
    } else {
        ui->plainTextEditDescription->setStyleSheet(styleSheet());
    }

    return isValid;
}

//Q_SLOTS

bool CommunityFundCreateProposalDialog::on_click_pushButtonCreateProposal()
{

}

CommunityFundCreateProposalDialog::~CommunityFundCreateProposalDialog()
{
    delete ui;
}
