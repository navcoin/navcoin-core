#ifndef COMMUNITYFUNDDISPLAYPAYMENTREQUESTDETAILED_H
#define COMMUNITYFUNDDISPLAYPAYMENTREQUESTDETAILED_H

#include <QWidget>
#include <consensus/cfund.h>
#include <wallet/wallet.h>
#include <QDialog>
#include <QAbstractButton>

namespace Ui {
class CommunityFundDisplayPaymentRequestDetailed;
}

class CommunityFundDisplayPaymentRequestDetailed : public QDialog
{
    Q_OBJECT

public:
    explicit CommunityFundDisplayPaymentRequestDetailed(QWidget *parent = 0, CFund::CPaymentRequest prequest = CFund::CPaymentRequest());
    ~CommunityFundDisplayPaymentRequestDetailed();

private:
    Ui::CommunityFundDisplayPaymentRequestDetailed *ui;
    CFund::CPaymentRequest prequest;
    CWallet *wallet;
    void setPrequestLabels() const;

public Q_SLOTS:
    void click_buttonBoxYesNoVote(QAbstractButton *button);
};

#endif // COMMUNITYFUNDDISPLAYPAYMENTREQUESTDETAILED_H
