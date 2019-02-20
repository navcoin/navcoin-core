#ifndef COMMUNITYFUNDSUCCESSDIALOG_H
#define COMMUNITYFUNDSUCCESSDIALOG_H

#include <QWidget>
#include <QPushButton>
#include <QDialog>
#include "../consensus/cfund.h"

namespace Ui {
class CommunityFundSuccessDialog;
}

class CommunityFundSuccessDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CommunityFundSuccessDialog(QWidget *parent = 0, CFund::CPaymentRequest* prequest = 0);
    explicit CommunityFundSuccessDialog(QWidget *parent = 0, CFund::CProposal* proposal = 0);
    ~CommunityFundSuccessDialog();

private:
    Ui::CommunityFundSuccessDialog *ui;
    CFund::CProposal* proposal;
    CFund::CPaymentRequest* prequest;
};

#endif // COMMUNITYFUNDSUCCESSDIALOG_H
