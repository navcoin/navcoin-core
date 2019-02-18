#include "qvalidatedplaintextedit.h"
#include <iostream>

QValidatedPlainTextEdit::QValidatedPlainTextEdit(QWidget *parent):
    QPlainTextEdit(parent)
{
    std::cout << "droid\n";
}
