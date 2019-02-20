#ifndef COMMUNITYFUNDDISPLAYPAYMENTREQUEST_H
#define COMMUNITYFUNDDISPLAYPAYMENTREQUEST_H

#include <QWidget>
#include <QAbstractButton>
#include "consensus/cfund.h"

namespace Ui {
class CommunityFundDisplayPaymentRequest;
}

class CommunityFundDisplayPaymentRequest : public QWidget
{
    Q_OBJECT

public:
    CommunityFundDisplayPaymentRequest(QWidget *parent = 0, CFund::CPaymentRequest prequest = CFund::CPaymentRequest());
    void refresh();
    ~CommunityFundDisplayPaymentRequest();

private:
    Ui::CommunityFundDisplayPaymentRequest *ui;
    CFund::CPaymentRequest prequest;

public Q_SLOTS:
    void on_click_buttonBoxVote(QAbstractButton *button);
    void on_click_pushButtonDetails();
};

#endif // COMMUNITYFUNDDISPLAYPAYMENTREQUEST_H
