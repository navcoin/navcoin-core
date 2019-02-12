#include "communityfundcreateproposaldialog.h"
#include "ui_communityfundcreateproposaldialog.h"

CommunityFundCreateProposalDialog::CommunityFundCreateProposalDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CommunityFundCreateProposalDialog)
{
    ui->setupUi(this);
}

CommunityFundCreateProposalDialog::~CommunityFundCreateProposalDialog()
{
    delete ui;
}
