#include "qspinboxtriad.h"

QSpinBoxTriad::QSpinBoxTriad(QWidget *parent) : QWidget(parent)
{
    //create layout and days, hours, minutes spinboxes
    QBoxLayout* layout = new QBoxLayout(QBoxLayout::LeftToRight, nullptr);
    QSpinBox* days = new QSpinBox();
    QSpinBox* hours = new QSpinBox();
    QSpinBox* minutes = new QSpinBox();

    //add spinboxes to horizonal layout
    layout->addWidget(days);
    layout->addWidget(hours);
    layout->addWidget(minutes);

}
