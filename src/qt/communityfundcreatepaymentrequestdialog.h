#ifndef COMMUNITYFUNDCREATEPAYMENTREQUESTDIALOG_H
#define COMMUNITYFUNDCREATEPAYMENTREQUESTDIALOG_H

#include <QDialog>

namespace Ui {
class CommunityFundCreatePaymentRequestDialog;
}

class CommunityFundCreatePaymentRequestDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CommunityFundCreatePaymentRequestDialog(QWidget *parent = 0);
    ~CommunityFundCreatePaymentRequestDialog();

private:
    Ui::CommunityFundCreatePaymentRequestDialog *ui;
    bool validate();

public Q_SLOTS:
    bool on_click_pushButtonSubmitPaymentRequest();
    void on_click_plainTextEditDescription();
};

#endif // COMMUNITYFUNDCREATEPAYMENTREQUESTDIALOG_H
