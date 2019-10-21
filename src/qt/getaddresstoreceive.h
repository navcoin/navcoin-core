#ifndef GETADDRESSTORECEIVE_H
#define GETADDRESSTORECEIVE_H

#include <qt/walletmodel.h>
#include <QWidget>

namespace Ui {
class getAddressToReceive;
}

class getAddressToReceive : public QWidget
{
    Q_OBJECT

public:
    explicit getAddressToReceive(QWidget *parent = 0);
    ~getAddressToReceive();
    void setModel(WalletModel *model);

public Q_SLOTS:
    void getNewAddress();
    void getColdStakingAddress();
    void showQR();

private:
    Ui::getAddressToReceive *ui;
    WalletModel *model;
    QString address;

Q_SIGNALS:
    void requestPayment();
    void requestAddressHistory();

private Q_SLOTS:
    void showRequestPayment();
    void copyToClipboard();
    void showAddressHistory();
};

#endif // GETADDRESSTORECEIVE_H
