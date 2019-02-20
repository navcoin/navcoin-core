#ifndef COMMUNITYFUNDSUCCESSDIALOG_H
#define COMMUNITYFUNDSUCCESSDIALOG_H

#include <QWidget>

namespace Ui {
class CommunityFundSuccessDialog;
}

class CommunityFundSuccessDialog : public QWidget
{
    Q_OBJECT

public:
    explicit CommunityFundSuccessDialog(QWidget *parent = 0);
    ~CommunityFundSuccessDialog();

private:
    Ui::CommunityFundSuccessDialog *ui;
};

#endif // COMMUNITYFUNDSUCCESSDIALOG_H
