#ifndef COMMUNITYFUNDDISPLAYPAYMENTREQUESTDETAILED_H
#define COMMUNITYFUNDDISPLAYPAYMENTREQUESTDETAILED_H

#include <QWidget>

namespace Ui {
class CommunityFundDisplayPaymentRequestDetailed;
}

class CommunityFundDisplayPaymentRequestDetailed : public QWidget
{
    Q_OBJECT

public:
    explicit CommunityFundDisplayPaymentRequestDetailed(QWidget *parent = 0);
    ~CommunityFundDisplayPaymentRequestDetailed();

private:
    Ui::CommunityFundDisplayPaymentRequestDetailed *ui;
};

#endif // COMMUNITYFUNDDISPLAYPAYMENTREQUESTDETAILED_H
