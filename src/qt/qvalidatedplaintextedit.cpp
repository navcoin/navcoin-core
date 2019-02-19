#include "qvalidatedplaintextedit.h"
#include "guiconstants.h"

QValidatedPlainTextEdit::QValidatedPlainTextEdit(QWidget *parent):
    QPlainTextEdit(parent),
    valid(true),
    checkValidator(0)
{
    connect(this, SIGNAL(textChanged(QString)), this, SLOT(markValid()));
}

void QValidatedPlainTextEdit::setValid(bool valid)
{
    if(valid == this->valid)
    {
        return;
    }

    if(valid)
    {
        setStyleSheet("");
    }
    else
    {
        setStyleSheet(STYLE_INVALID);
    }
    this->valid = valid;
}

void QValidatedPlainTextEdit::focusInEvent(QFocusEvent *evt)
{
    // Clear invalid flag on focus
    setValid(true);

    QPlainTextEdit::focusInEvent(evt);
}

void QValidatedPlainTextEdit::focusOutEvent(QFocusEvent *evt)
{
    checkValidity();

    QPlainTextEdit::focusOutEvent(evt);
}

void QValidatedPlainTextEdit::markValid()
{
    // As long as a user is typing ensure we display state as valid
    setValid(true);
}

void QValidatedPlainTextEdit::clear()
{
    setValid(true);
    QPlainTextEdit::clear();
}

void QValidatedPlainTextEdit::setEnabled(bool enabled)
{
    if (!enabled)
    {
        // A disabled QValidatedLineEdit should be marked valid
        setValid(true);
    }
    else
    {
        // Recheck validity when QValidatedLineEdit gets enabled
        checkValidity();
    }

    QPlainTextEdit::setEnabled(enabled);
}

void QValidatedPlainTextEdit::checkValidity()
{
    if (toPlainText().isEmpty())
    {
        setValid(true);

        // Check contents on focus out
        if (checkValidator)
        {
            QString address = toPlainText();
            int pos = 0;
            if (checkValidator->validate(address, pos) == QValidator::Acceptable)
                setValid(true);
            else
                setValid(false);
        }
    }
    else
        setValid(false);

    //Q_EMIT validationDidChange(this);
}

void QValidatedPlainTextEdit::setCheckValidator(const QValidator *v)
{
    checkValidator = v;
}

bool QValidatedPlainTextEdit::isValid()
{
    // use checkValidator in case the QValidatedLineEdit is disabled
    if (checkValidator)
    {
        QString address = toPlainText();
        int pos = 0;
        if (checkValidator->validate(address, pos) == QValidator::Acceptable)
            return true;
    }

    return valid;
}
