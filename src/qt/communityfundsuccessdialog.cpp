#include "communityfundsuccessdialog.h"
#include "ui_communityfundsuccessdialog.h"

CommunityFundSuccessDialog::CommunityFundSuccessDialog(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::CommunityFundSuccessDialog)
{
    ui->setupUi(this);
}

CommunityFundSuccessDialog::~CommunityFundSuccessDialog()
{
    delete ui;
}
