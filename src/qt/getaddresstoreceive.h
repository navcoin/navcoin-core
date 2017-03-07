#ifndef GETADDRESSTORECEIVE_H
#define GETADDRESSTORECEIVE_H

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

private:
    Ui::getAddressToReceive *ui;
};

#endif // GETADDRESSTORECEIVE_H
