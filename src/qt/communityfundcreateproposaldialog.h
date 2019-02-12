#ifndef COMMUNITYFUNDCREATEPROPOSALDIALOG_H
#define COMMUNITYFUNDCREATEPROPOSALDIALOG_H

#include <QDialog>

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
    bool validate() const;
private Q_SLOTS:

};

#endif // COMMUNITYFUNDCREATEPROPOSALDIALOG_H
