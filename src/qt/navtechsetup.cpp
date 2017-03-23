#include "navtechsetup.h"
#include "navtechitem.h"
#include "ui_navtechsetup.h"
#include "net.h"
#include "skinize.h"
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

    ui->saveButton->setVisible(false);

    this->setWindowTitle("Navtech");

    connect(ui->addNewButton,SIGNAL(clicked()),this,SLOT(addNavtechServer()));
    connect(ui->getInfoButton,SIGNAL(clicked()),this,SLOT(getinfoNavtechServer()));
    connect(ui->removeButton,SIGNAL(clicked()),this,SLOT(removeNavtechServer()));
    connect(ui->serversListWidget,SIGNAL(itemClicked(QListWidgetItem*)),this,SLOT(showButtons()));
    connect(ui->saveButton,SIGNAL(clicked()),this,SLOT(close()));

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

void navtechsetup::showNavtechIntro()
{
    setWindowIcon(QIcon(":icons/navcoin"));
    setStyleSheet(Skinize());

    ui->saveButton->setVisible(true);

    exec();
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

    showButtons(false);
    reloadNavtechServers();
}

void navtechsetup::showButtons(bool show)
{
    ui->getInfoButton->setVisible(show);
    ui->removeButton->setVisible(show);
}

void navtechsetup::getinfoNavtechServer()
{
    QString serverToCheck = ui->serversListWidget->currentItem()->text();

    if(serverToCheck == "")
        return;

    QStringList server = serverToCheck.split(':');

    if(server.length() != 2)
        return;

    std::string readBuffer;

    curl_global_init(CURL_GLOBAL_ALL);
    curl = curl_easy_init();

    if (curl) {

      std::string serverURL = "https://" + server.at(0).toStdString() + ":" + server.at(1).toStdString() + "/api/check-node";

      curl_easy_setopt(curl, CURLOPT_URL, serverURL.c_str());
      curl_easy_setopt(curl, CURLOPT_POSTFIELDS, "num_addresses=0");
      curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, CurlWriteResponse);
      curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
      curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
      curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

      res = curl_easy_perform(curl);

      if (res != CURLE_OK) {
          QMessageBox::critical(this, windowTitle(),
              tr("Could not stablish a connection to the server %1.").arg(server.at(0)),
              QMessageBox::Ok, QMessageBox::Ok);
        curl_easy_cleanup(curl);
        curl_global_cleanup();
      } else {
          QJsonDocument jsonDoc =  QJsonDocument::fromJson(QString::fromStdString(readBuffer).toUtf8());
          QJsonObject jsonObject = jsonDoc.object();

          QString type = jsonObject["type"].toString();

          if (type != "SUCCESS") {
            QMessageBox::critical(this, windowTitle(),
                tr("Could not connect to the server.") + "<br><br>" + type + "<br><br>" + QString::fromStdString(readBuffer),
                QMessageBox::Ok, QMessageBox::Ok);
          } else {
            QJsonObject jsonData = jsonObject["data"].toObject();
            QString minAmount = jsonData["min_amount"].toString();
            QString maxAmount = jsonData["max_amount"].toString();
            QString txFee = jsonData["transaction_fee"].toString();

            QMessageBox::critical(this, windowTitle(),
                tr("Navtech server") + "<br><br>" +  tr("Address: ") + server.at(0) + "<br>" + tr("Min amount: ") + minAmount + " <br>" + tr("Max amount: ") + maxAmount + "<br>" + tr("Tx fee: ") + txFee,
                QMessageBox::Ok, QMessageBox::Ok);
          }
          curl_easy_cleanup(curl);
          curl_global_cleanup();
        }
      } else {
        curl_easy_cleanup(curl);
        curl_global_cleanup();
        QMessageBox::critical(this, windowTitle(),
          tr("Could not stablish a connection to the server %1.").arg(server.at(0)),
          QMessageBox::Ok, QMessageBox::Ok);
    }
}
