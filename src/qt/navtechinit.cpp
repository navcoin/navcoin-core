#include "navtechinit.h"
#include "ui_navtechinit.h"
#include "net.h"
#include "util.h"
#include "skinize.h"

#include <string>
#include <iostream>

#include <QStringList>
#include <QUrl>

NavTechInit::NavTechInit(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::NavTechInit)
{
    ui->setupUi(this);
}

NavTechInit::~NavTechInit()
{
    delete ui;
}

void NavTechInit::ShowNavtechIntro(bool exitAfter)
{
    NavTechInit navtechinit;
    navtechinit.setWindowIcon(QIcon(":icons/navcoin"));
    navtechinit.setStyleSheet(Skinize());

    if(!navtechinit.exec())
    {
        if(exitAfter)
            exit(0);
        else
            return;
    }

    QString ServersToAdd = navtechinit.GetServers();
    QStringList ss = ServersToAdd.split('\n');

    for (int i = 0; i < ss.size(); ++i){
        QString Server = ss.at(i);
        QString strippedServer = Server.remove(' ');
        if(strippedServer != "")
        {
            WriteConfigFile("addanonserver",strippedServer.toStdString());
        }
    }
}

QString NavTechInit::GetServers()
{
    return ui->plainTextEdit->toPlainText();
}
