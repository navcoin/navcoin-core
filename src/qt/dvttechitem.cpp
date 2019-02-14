#include "dvttechitem.h"
#include "ui_dvttechitem.h"

dvttechitem::dvttechitem(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::dvttechitem),
    host(""),
    name("")
{
    ui->setupUi(this);
}

void dvttechitem::setHost(QString hostStr)
{
    host = hostStr;
    ui->serverHostLabel->setText(hostStr);
}

void dvttechitem::setName(QString nameStr)
{
    name = nameStr;
    ui->serverNameLabel->setText(nameStr);
}

dvttechitem::~dvttechitem()
{
    delete ui;
}
