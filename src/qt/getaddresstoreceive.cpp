#include "getaddresstoreceive.h"
#include "ui_getaddresstoreceive.h"

getAddressToReceive::getAddressToReceive(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::getAddressToReceive)
{
    ui->setupUi(this);
}

getAddressToReceive::~getAddressToReceive()
{
    delete ui;
}
