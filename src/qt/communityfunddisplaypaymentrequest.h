#ifndef COMMUNITYFUNDDISPLAYPAYMENTREQUEST_H
#define COMMUNITYFUNDDISPLAYPAYMENTREQUEST_H

#include <QWidget>
#include <QAbstractButton>
#include <consensus/cfund.h>
#include <wallet/wallet.h>

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
    CWallet *wallet;

public Q_SLOTS:
    void click_buttonBoxVote(QAbstractButton *button);
    void click_pushButtonDetails();
};

#endif // COMMUNITYFUNDDISPLAYPAYMENTREQUEST_H
