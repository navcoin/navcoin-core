#include "communityfundcreateproposaldialog.h"
#include "ui_communityfundcreateproposaldialog.h"
#include "guiconstants.h"
CommunityFundCreateProposalDialog::CommunityFundCreateProposalDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CommunityFundCreateProposalDialog)
{
    ui->setupUi(this);

    //maybe set as vecot rof QLineEdit instead?
    //set validator (input cleaning) for days, hours, minutes line edit
    ui->lineEditDays->setValidator( new QIntValidator(0, 100, this) );
    ui->lineEditHours->setValidator( new QIntValidator(0, 100, this) );
    ui->lineEditMinutes->setValidator( new QIntValidator(0, 100, this) );
    // Input Mask 9 = ASCII digit required. 0-9.
    ui->lineEditDays->setInputMask(QString("999"));
    ui->lineEditHours->setInputMask(QString("99"));
    ui->lineEditMinutes->setInputMask(QString("99"));

    ui->lineEditDays->setMaxLength(3);
    ui->lineEditHours->setMaxLength(2);
    ui->lineEditMinutes->setMaxLength(2);
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
    //validate days/hours/minutes QLineEdits
    //ui->lineEditDays;
    //ui->lineEditHours;
    //ui->lineEditMinutes;

    return true;
}

//Q_SLOTS

bool CommunityFundCreateProposalDialog::on_click_pushButtonCreateProposal()
{
    this->validate();
    return false;
}

CommunityFundCreateProposalDialog::~CommunityFundCreateProposalDialog()
{
    delete ui;
}
