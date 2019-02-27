#ifndef QVALIDATEDSPINBOX_H
#define QVALIDATEDSPINBOX_H

#include <QSpinBox>
#include <iostream>

class QValidatedSpinBox : public QSpinBox
{
    Q_OBJECT

public:
    QValidatedSpinBox(QDialog *parent);
    void clear();

protected:
    void focusInEvent(QFocusEvent *evt);

private:
    bool valid;

public Q_SLOTS:
    void setValid(bool valid);

Q_SIGNALS:
    void clickedSpinBox();

private Q_SLOTS:
    void markValid();
};

#endif // QVALIDATEDSPINBOX_H
