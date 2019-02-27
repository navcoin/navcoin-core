#ifndef COMMUNITYFUNDCREATEPROPOSALDIALOG_H
#define COMMUNITYFUNDCREATEPROPOSALDIALOG_H

#include <QDialog>
#include "qvalidatedspinbox.h"

namespace Ui {
class CommunityFundCreateProposalDialog;
}

class CommunityFundCreateProposalDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CommunityFundCreateProposalDialog(QWidget *parent = 0);
    ~CommunityFundCreateProposalDialog();

private:
    Ui::CommunityFundCreateProposalDialog *ui;
    bool validate();

private Q_SLOTS:
    bool click_pushButtonCreateProposal();
    void click_spinBox();

};

#endif // COMMUNITYFUNDCREATEPROPOSALDIALOG_H
