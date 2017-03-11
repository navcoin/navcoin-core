#include "navtechsetup.h"
#include "navtechitem.h"
#include "ui_navtechsetup.h"
#include "net.h"
#include "util.h"

#include <QVBoxLayout>
#include <QListWidgetItem>
#include <QListWidget>
#include <QMessageBox>
#include <QtNetwork>
#include <QWidget>

navtechsetup::navtechsetup(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::navtechsetup)
{
    ui->setupUi(this);
    ui->getInfoButton->setVisible(false);
    ui->removeButton->setVisible(false);

    this->setWindowTitle("Navtech");

    connect(ui->addNewButton,SIGNAL(clicked()),this,SLOT(addNavtechServer()));
    connect(ui->getInfoButton,SIGNAL(clicked()),this,SLOT(getinfoNavtechServer()));
    connect(ui->removeButton,SIGNAL(clicked()),this,SLOT(removeNavtechServer()));
    connect(ui->serversListWidget,SIGNAL(itemClicked(QListWidgetItem*)),this,SLOT(showButtons()));

    reloadNavtechServers();
}

navtechsetup::~navtechsetup()
{
    delete ui;
}

void navtechsetup::addNavtechServer()
{
    QString serverToAdd = ui->addServerText->text();

    if(serverToAdd == "")
        return;

    serverToAdd = serverToAdd.remove(' ').remove('\n').remove('\r').remove('\t');

    LOCK(cs_vAddedAnonServers);
    std::vector<std::string>::iterator it = vAddedAnonServers.begin();
    for(; it != vAddedAnonServers.end(); it++)
        if (serverToAdd.toStdString() == *it)
            break;

    WriteConfigFile("addanonserver", serverToAdd.toStdString());
    if (it == vAddedAnonServers.end())
        vAddedAnonServers.push_back(serverToAdd.toStdString());

    ui->addServerText->clear();

    reloadNavtechServers();
}

void navtechsetup::reloadNavtechServers()
{
    ui->serversListWidget->clear();

    const std::vector<std::string>& confAnonServers = mapMultiArgs["-addanonserver"];

    BOOST_FOREACH(std::string confAnonServer, confAnonServers) {
        ui->serversListWidget->addItem(QString::fromStdString(confAnonServer));
    }

    BOOST_FOREACH(std::string vAddedAnonServer, vAddedAnonServers) {
        ui->serversListWidget->addItem(QString::fromStdString(vAddedAnonServer));
    }
}

void navtechsetup::removeNavtechServer()
{
    QString serverToRemove = ui->serversListWidget->currentItem()->text();

    if(serverToRemove == "")
        return;

    QMessageBox::StandardButton btnRetVal = QMessageBox::question(this, tr("Remove Navtech server"),
        tr("You are about to remove the following Navtech server: ") + "<br><br>" + serverToRemove + "<br><br>" + tr("Are you sure?"),
        QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel);

    if(btnRetVal == QMessageBox::Cancel)
        return;

    LOCK(cs_vAddedAnonServers);
    std::vector<std::string>::iterator it = vAddedAnonServers.begin();

    RemoveConfigFile("addanonserver", serverToRemove.toStdString());
    for(; it != vAddedAnonServers.end(); it++)
        if (serverToRemove == QString::fromStdString(*it))
            break;

    if (it != vAddedAnonServers.end())
    {
      vAddedAnonServers.erase(it);
      QMessageBox::critical(this, windowTitle(),
          tr("Removed."),
          QMessageBox::Ok, QMessageBox::Ok);
    }

    reloadNavtechServers();
}

void navtechsetup::showButtons()
{
    ui->getInfoButton->setVisible(true);
    ui->removeButton->setVisible(true);
}

void navtechsetup::getinfoNavtechServer()
{
    QString serverToCheck = ui->serversListWidget->currentItem()->text();

    if(serverToCheck == "")
        return;

    QStringList server = serverToCheck.split(':');

    if(server.length() != 2)
        return;

    QSslSocket *socket = new QSslSocket(this);
    socket->setPeerVerifyMode(socket->VerifyNone);
    socket->connectToHostEncrypted(server.at(0), server.at(1).toInt());

    if(!socket->waitForEncrypted())
    {
        QMessageBox::critical(this, windowTitle(),
            tr("Could not connect to the server."),
            QMessageBox::Ok, QMessageBox::Ok);
    }
    else
    {
        QString reqString = QString("POST /api/check-node HTTP/1.1\r\n" \
                            "Host: %1\r\n" \
                            "Content-Type: application/x-www-form-urlencoded\r\n" \
                            "Content-Length: 15\r\n" \
                            "Connection: Close\r\n\r\n" \
                            "num_addresses=0\r\n").arg(server.at(0));

        socket->write(reqString.toUtf8());

        while (socket->waitForReadyRead()){

            while(socket->canReadLine()){
                //read all the lines
                QString line = socket->readLine();
            }

            QString rawReply = socket->readAll();

            QJsonDocument jsonDoc =  QJsonDocument::fromJson(rawReply.toUtf8());
            QJsonObject jsonObject = jsonDoc.object();

            QString type = jsonObject["type"].toString();

            if (type != "SUCCESS") {
                QMessageBox::critical(this, windowTitle(),
                    tr("Could not connect to the server."),
                    QMessageBox::Ok, QMessageBox::Ok);
            }

            QJsonObject jsonData = jsonObject["data"].toObject();
            QString minAmount = jsonData["min_amount"].toString();
            QString maxAmount = jsonData["max_amount"].toString();
            QString txFee = jsonData["transaction_fee"].toString();

            QMessageBox::critical(this, windowTitle(),
                tr("Navtech server") + "<br><br>" + tr("Address: ") + server.at(0) + "<br>" + tr("Min amount: ") + minAmount + " <br>" + tr("Max amount: ") + maxAmount + "<br>" + tr("Tx fee: ") + txFee,
                QMessageBox::Ok, QMessageBox::Ok);
        }
    }

}
