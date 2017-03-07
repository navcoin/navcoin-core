#ifndef NAVTECHINIT_H
#define NAVTECHINIT_H

#include <QDialog>

namespace Ui {
class NavTechInit;
}

class NavTechInit : public QDialog
{
    Q_OBJECT

public:
    explicit NavTechInit(QWidget *parent = 0);
    ~NavTechInit();
    QString GetServers();
    void ShowNavtechIntro();

private:
    Ui::NavTechInit *ui;
};

#endif // NAVTECHINIT_H
