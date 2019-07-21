#ifndef COMMUNITYFUNDSUCCESSDIALOG_H
#define COMMUNITYFUNDSUCCESSDIALOG_H

#include <QWidget>
#include <QPushButton>
#include <QDialog>
#include "../consensus/dao.h"

namespace Ui {
class CommunityFundSuccessDialog;
}

class CommunityFundSuccessDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CommunityFundSuccessDialog(uint256 hash, QWidget *parent = 0, CPaymentRequest* prequest = 0);
    explicit CommunityFundSuccessDialog(uint256 hash, QWidget *parent = 0, CProposal* proposal = 0);
    ~CommunityFundSuccessDialog();

private:
    Ui::CommunityFundSuccessDialog *ui;
    CProposal* proposal;
    CPaymentRequest* prequest;
};

#endif // COMMUNITYFUNDSUCCESSDIALOG_H
