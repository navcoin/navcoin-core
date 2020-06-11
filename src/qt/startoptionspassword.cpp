#include <startoptionspassword.h>
#include <ui_startoptionspassword.h>

StartOptionsPassword::StartOptionsPassword(QWidget *parent)
    : QWidget(parent), ui(new Ui::StartOptionsPassword)
{
    ui->setupUi(this);
}

StartOptionsPassword::~StartOptionsPassword()
{
    delete ui;
}

QString StartOptionsPassword::getPass()
{
    return ui->pass->text();
}

QString StartOptionsPassword::getPassConf()
{
    return ui->passConf->text();
}
