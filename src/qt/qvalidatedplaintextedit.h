#ifndef QVALIDATEDPLAINTEXTEDIT_H
#define QVALIDATEDPLAINTEXTEDIT_H

#include <QWidget>
#include <QPlainTextEdit>
#include <QValidator>

class QValidatedPlainTextEdit : public QPlainTextEdit
{
public:
    explicit QValidatedPlainTextEdit(QWidget *parent);
    void clear();
    void setCheckValidator(const QValidator *v);
    bool isValid();

protected:
    void focusInEvent(QFocusEvent *evt);
    void focusOutEvent(QFocusEvent *evt);

private:
    bool valid;

public Q_SLOTS:
    void setValid(bool valid);
    void setEnabled(bool enabled);

//Q_SIGNALS:
    //void validationDidChange(QPlainTextEdit *validatedLineEdit);

private Q_SLOTS:
    void markValid();
    void checkValidity();
};

#endif // QVALIDATEDPLAINTEXTEDIT_H
