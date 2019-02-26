#include "qvalidatedspinbox.h"
#include "guiconstants.h"

QValidatedSpinBox::QValidatedSpinBox(QWidget *parent) :
    QSpinBox(parent),
    valid(true),
    checkValidator(0)
{
    connect(this, SIGNAL(textChanged(QString)), this, SLOT(markValid()));
}

void QValidatedSpinBox::setValid(bool valid)
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

void QValidatedSpinBox::focusInEvent(QFocusEvent *evt)
{
    // Clear invalid flag on focus
    setValid(true);

    QSpinBox::focusInEvent(evt);
}

void QValidatedSpinBox::focusOutEvent(QFocusEvent *evt)
{
    checkValidity();

    QSpinBox::focusOutEvent(evt);
}

void QValidatedSpinBox::markValid()
{
    // As long as a user is typing ensure we display state as valid
    setValid(true);
}

void QValidatedSpinBox::clear()
{
    setValid(true);
    QSpinBox::clear();
}

void QValidatedSpinBox::setEnabled(bool enabled)
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

    QSpinBox::setEnabled(enabled);
}

void QValidatedSpinBox::checkValidity()
{
    if (text().isEmpty())
    {
        setValid(true);
    }
    else if (hasAcceptableInput())
    {
        setValid(true);

        // Check contents on focus out
        if (checkValidator)
        {
            QString address = text();
            int pos = 0;
            if (checkValidator->validate(address, pos) == QValidator::Acceptable)
                setValid(true);
            else
                setValid(false);
        }
    }
    else
        setValid(false);

    Q_EMIT validationDidChange(this);
}

void QValidatedSpinBox::setCheckValidator(const QValidator *v)
{
    checkValidator = v;
}

bool QValidatedSpinBox::isValid()
{
    // use checkValidator in case the QValidatedLineEdit is disabled
    if (checkValidator)
    {
        QString address = text();
        int pos = 0;
        if (checkValidator->validate(address, pos) == QValidator::Acceptable)
            return true;
    }

    return valid;
}
