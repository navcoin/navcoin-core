#include <qt/qvalidatedplaintextedit.h>
#include <qt/guiconstants.h>

QValidatedPlainTextEdit::QValidatedPlainTextEdit(QWidget *parent):
    QPlainTextEdit(parent),
    valid(true)
{
    connect(this, SIGNAL(textChanged(QString)), this, SLOT(markValid()));
}

void QValidatedPlainTextEdit::setValid(bool valid)
{
    if(valid == this->valid)
        return;
    if(valid)
        setStyleSheet("");
    else
        setStyleSheet(STYLE_INVALID);

    this->valid = valid;
}

void QValidatedPlainTextEdit::focusInEvent(QFocusEvent *evt)
{
    setValid(true);
    QPlainTextEdit::focusInEvent(evt);
}

void QValidatedPlainTextEdit::focusOutEvent(QFocusEvent *evt)
{
    QPlainTextEdit::focusOutEvent(evt);
}


void QValidatedPlainTextEdit::clear()
{
    setValid(true);
    QPlainTextEdit::clear();
}

void QValidatedPlainTextEdit::setEnabled(bool enabled)
{
    if (!enabled)
        setValid(true);

    QPlainTextEdit::setEnabled(enabled);
}

bool QValidatedPlainTextEdit::isValid()
{
    if(toPlainText().length() >= 1024)
        setValid(false);
    else
        setValid(true);
    //Q_EMIT validationDidChange(this);
}
