#ifndef COMMUNITYFUNDCREATEPROPOSALDIALOG_H
#define COMMUNITYFUNDCREATEPROPOSALDIALOG_H

#include <qt/qvalidatedspinbox.h>
#include <qt/walletmodel.h>

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

    void setModel(WalletModel *model);

private:
    Ui::CommunityFundCreateProposalDialog *ui;
    WalletModel *model;
    bool validate();

private Q_SLOTS:
    void click_pushButtonCreateProposal();
    void click_spinBox();
};

#endif // COMMUNITYFUNDCREATEPROPOSALDIALOG_H
