#ifndef CFUND_VOTING_H
#define CFUND_VOTING_H
#include "ui_cfund_voting.h"

#include <QDialog>
#include <QUrl>
#include <QDesktopServices>

namespace Ui {
class CFund_Voting;
}

class CFund_Voting : public QDialog
{
    Q_OBJECT

public:
    explicit CFund_Voting(QWidget *parent = 0, bool fPaymentRequests = false);
    ~CFund_Voting();

    void Refresh();

    QString selected;

public Q_SLOTS:
    void closeDialog();
    void voteYes();
    void voteNo();
    void stopVoting();
    void viewDetails();
    void switchView();
    void selectedFromYes(QListWidgetItem* item);
    void selectedFromNo(QListWidgetItem* item);
    void selectedFromNotVoting(QListWidgetItem* item);
    void enableDisableButtons();
    void setSelection(QString selection);

private:
    Ui::CFund_Voting *ui;
    bool fSettings;
};

#endif // CFUND_VOTING_H
