#include <qt/qvalidatedspinbox.h>
#include <qt/guiconstants.h>

#include <QDialog>

QValidatedSpinBox::QValidatedSpinBox(QDialog *parent) :
    QSpinBox(parent),
    valid(true)
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
    Q_EMIT clickedSpinBox();
    QSpinBox::focusInEvent(evt);
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
