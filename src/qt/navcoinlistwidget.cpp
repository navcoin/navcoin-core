// Copyright (c) 2019 The NavCoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "navcoinlistwidget.h"

NavCoinListWidget::NavCoinListWidget(QWidget *parent, QString title, ValidatorFunc validator) :
    QWidget(parent),
    listWidget(new QListWidget),
    addInput(new QLineEdit),
    removeBtn( new QPushButton),
    warningLbl(new QLabel),
    validatorFunc(validator)
{
    QVBoxLayout* layout = new QVBoxLayout();
    QHBoxLayout* addLayout = new QHBoxLayout();
    QFrame* add = new QFrame();

    add->setLayout(addLayout);

    QPushButton* addBtn = new QPushButton(tr("Add"));
    removeBtn = new QPushButton(tr("Remove"));

    removeBtn->setVisible(false);

    addLayout->addWidget(addInput);
    addLayout->addWidget(addBtn);
    addLayout->addWidget(removeBtn);

    this->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    this->setLayout(layout);

    QLabel* titleLbl = new QLabel(title);

    warningLbl->setObjectName("warning");
    warningLbl->setVisible(false);

    layout->addWidget(titleLbl);
    layout->addWidget(listWidget);
    layout->addWidget(add);
    layout->addWidget(warningLbl);

    connect(addBtn, SIGNAL(clicked()), this, SLOT(onInsert()));
    connect(removeBtn, SIGNAL(clicked()), this, SLOT(onRemove()));
    connect(listWidget,
            SIGNAL(currentItemChanged(QListWidgetItem *, QListWidgetItem *)),
            this, SLOT(onSelect(QListWidgetItem *)));
}

void NavCoinListWidget::onRemove()
{
    listWidget->takeItem(listWidget->row(listWidget->currentItem()));
}

void NavCoinListWidget::onSelect(QListWidgetItem* item)
{
    removeBtn->setVisible(item != nullptr);
}

void NavCoinListWidget::onInsert()
{
    QString itemText = addInput->text();

    for(int row = 0; row < listWidget->count(); row++)
    {
             QListWidgetItem *item = listWidget->item(row);
             if (item->text() == itemText)
             {
                 warningLbl->setText(tr("Duplicated entry"));
                 warningLbl->setVisible(true);
                 return;
             }
    }

    if (!(*validatorFunc)(itemText))
    {
        warningLbl->setText(tr("Entry not valid"));
        warningLbl->setVisible(true);
        return;
    }

    warningLbl->setVisible(false);

    addInput->setText("");

    QListWidgetItem *newItem = new QListWidgetItem;
    newItem->setText(itemText);

    int row = listWidget->row(listWidget->currentItem());
    listWidget->insertItem(row, newItem);
}

QStringList NavCoinListWidget::getEntries()
{
    QStringList ret;

    for(int row = 0; row < listWidget->count(); row++)
    {
             QListWidgetItem *item = listWidget->item(row);
             ret << item->text();
    }

    return ret;
}
