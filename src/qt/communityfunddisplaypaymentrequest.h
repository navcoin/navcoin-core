#ifndef COMMUNITYFUNDDISPLAYPAYMENTREQUEST_H
#define COMMUNITYFUNDDISPLAYPAYMENTREQUEST_H

#include <QWidget>

namespace Ui {
class CommunityFundDisplayPaymentRequest;
}

class CommunityFundDisplayPaymentRequest : public QWidget
{
    Q_OBJECT

public:
    explicit CommunityFundDisplayPaymentRequest(QWidget *parent = 0);
    ~CommunityFundDisplayPaymentRequest();

private:
    Ui::CommunityFundDisplayPaymentRequest *ui;
};

#endif // COMMUNITYFUNDDISPLAYPAYMENTREQUEST_H
