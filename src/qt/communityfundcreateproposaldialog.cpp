#include "communityfundcreateproposaldialog.h"
#include "ui_communityfundcreateproposaldialog.h"

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
    //connect
    connect(ui->pushButtonClose, SIGNAL(clicked()), this, SLOT(reject()));
}

// validate input fields
bool CommunityFundCreateProposalDialog::validate() const
{
    if(!ui->lineEditNavcoinAddress->isValid())
    {
        ui->lineEditNavcoinAddress->setValid(false);
    }

    //validate days/hours/minutes QLineEdits
    //ui->lineEditDays;
    //ui->lineEditHours;
    //ui->lineEditMinutes;
    ui->lineEditRequestedAmount->validate();
    return true;
}

CommunityFundCreateProposalDialog::~CommunityFundCreateProposalDialog()
{
    delete ui;
}
