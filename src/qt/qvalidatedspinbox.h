#ifndef QVALIDATEDSPINBOX_H
#define QVALIDATEDSPINBOX_H

#include <QSpinBox>

class QValidatedSpinBox : public QSpinBox
{
    Q_OBJECT

public:
    QValidatedSpinBox(QWidget *parent);
    void clear();
    void setCheckValidator(const QValidator *v);
    bool isValid();

protected:
    void focusInEvent(QFocusEvent *evt);
    void focusOutEvent(QFocusEvent *evt);

private:
    bool valid;
    const QValidator *checkValidator;

public Q_SLOTS:
    void setValid(bool valid);
    void setEnabled(bool enabled);

Q_SIGNALS:
    void validationDidChange(QValidatedSpinBox *validatedSpinBox);

private Q_SLOTS:
    void markValid();
    void checkValidity();
};

#endif // QVALIDATEDSPINBOX_H
