#ifndef DVTTECHSETUP_H
#define DVTTECHSETUP_H

#include "wallet/dvttech.h"

#include <QDialog>
#include <QVBoxLayout>

namespace Ui {
class dvttechsetup;
}

class dvttechsetup : public QDialog
{
    Q_OBJECT

public:
    explicit dvttechsetup(QWidget *parent = 0);
    ~dvttechsetup();

private:
    Ui::dvttechsetup *ui;

public Q_SLOTS:
    void reloadNavtechServers();
    void addNavtechServer();
    void removeNavtechServer();
    void getinfoNavtechServer();
    void showButtons(bool show=true);
    void showNavtechIntro();

};

#endif // DVTTECHSETUP_H
