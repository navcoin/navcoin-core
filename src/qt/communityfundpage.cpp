#include "communityfundpage.h"
#include "ui_communityfundpage.h"
#include "communityfundpage.moc"

CommunityFundPage::CommunityFundPage(const PlatformStyle *platformStyle, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::CommunityFundPage),
    clientModel(0),
    walletModel(0)
{
    ui->setupUi(this);
}

CommunityFundPage::~CommunityFundPage()
{
    delete ui;
}
