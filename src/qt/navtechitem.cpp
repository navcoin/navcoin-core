#include "navtechitem.h"
#include "ui_navtechitem.h"

navtechitem::navtechitem(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::navtechitem),
    host(""),
    name("")
{
    ui->setupUi(this);
}

void navtechitem::setHost(QString hostStr)
{
    host = hostStr;
    ui->serverHostLabel->setText(hostStr);
}

void navtechitem::setName(QString nameStr)
{
    name = nameStr;
    ui->serverNameLabel->setText(nameStr);
}

navtechitem::~navtechitem()
{
    delete ui;
}
