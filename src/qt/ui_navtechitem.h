/********************************************************************************
** Form generated from reading UI file 'navtechitem.ui'
**
** Created by: Qt User Interface Compiler version 5.7.1
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_NAVTECHITEM_H
#define UI_NAVTECHITEM_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_navtechitem
{
public:
    QWidget *horizontalLayoutWidget;
    QHBoxLayout *horizontalLayout;
    QSpacerItem *horizontalSpacer_3;
    QLabel *serverNameLabel;
    QSpacerItem *horizontalSpacer_2;
    QLabel *serverHostLabel;
    QSpacerItem *horizontalSpacer;
    QPushButton *getInfoButton;
    QPushButton *deleteButton;
    QSpacerItem *horizontalSpacer_4;

    void setupUi(QWidget *navtechitem)
    {
        if (navtechitem->objectName().isEmpty())
            navtechitem->setObjectName(QStringLiteral("navtechitem"));
        navtechitem->resize(551, 48);
        QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(navtechitem->sizePolicy().hasHeightForWidth());
        navtechitem->setSizePolicy(sizePolicy);
        navtechitem->setMinimumSize(QSize(0, 36));
        horizontalLayoutWidget = new QWidget(navtechitem);
        horizontalLayoutWidget->setObjectName(QStringLiteral("horizontalLayoutWidget"));
        horizontalLayoutWidget->setGeometry(QRect(0, -4, 551, 41));
        horizontalLayout = new QHBoxLayout(horizontalLayoutWidget);
        horizontalLayout->setObjectName(QStringLiteral("horizontalLayout"));
        horizontalLayout->setContentsMargins(0, 0, 0, 0);
        horizontalSpacer_3 = new QSpacerItem(15, 20, QSizePolicy::Minimum, QSizePolicy::Minimum);

        horizontalLayout->addItem(horizontalSpacer_3);

        serverNameLabel = new QLabel(horizontalLayoutWidget);
        serverNameLabel->setObjectName(QStringLiteral("serverNameLabel"));
        QSizePolicy sizePolicy1(QSizePolicy::Preferred, QSizePolicy::Minimum);
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(serverNameLabel->sizePolicy().hasHeightForWidth());
        serverNameLabel->setSizePolicy(sizePolicy1);
        serverNameLabel->setMinimumSize(QSize(0, 25));

        horizontalLayout->addWidget(serverNameLabel);

        horizontalSpacer_2 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout->addItem(horizontalSpacer_2);

        serverHostLabel = new QLabel(horizontalLayoutWidget);
        serverHostLabel->setObjectName(QStringLiteral("serverHostLabel"));
        sizePolicy1.setHeightForWidth(serverHostLabel->sizePolicy().hasHeightForWidth());
        serverHostLabel->setSizePolicy(sizePolicy1);
        serverHostLabel->setMinimumSize(QSize(0, 25));

        horizontalLayout->addWidget(serverHostLabel);

        horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout->addItem(horizontalSpacer);

        getInfoButton = new QPushButton(horizontalLayoutWidget);
        getInfoButton->setObjectName(QStringLiteral("getInfoButton"));
        QSizePolicy sizePolicy2(QSizePolicy::Minimum, QSizePolicy::Minimum);
        sizePolicy2.setHorizontalStretch(0);
        sizePolicy2.setVerticalStretch(0);
        sizePolicy2.setHeightForWidth(getInfoButton->sizePolicy().hasHeightForWidth());
        getInfoButton->setSizePolicy(sizePolicy2);
        getInfoButton->setMinimumSize(QSize(0, 25));

        horizontalLayout->addWidget(getInfoButton);

        deleteButton = new QPushButton(horizontalLayoutWidget);
        deleteButton->setObjectName(QStringLiteral("deleteButton"));
        sizePolicy2.setHeightForWidth(deleteButton->sizePolicy().hasHeightForWidth());
        deleteButton->setSizePolicy(sizePolicy2);
        deleteButton->setMinimumSize(QSize(0, 25));

        horizontalLayout->addWidget(deleteButton);

        horizontalSpacer_4 = new QSpacerItem(15, 20, QSizePolicy::Minimum, QSizePolicy::Minimum);

        horizontalLayout->addItem(horizontalSpacer_4);


        retranslateUi(navtechitem);

        QMetaObject::connectSlotsByName(navtechitem);
    } // setupUi

    void retranslateUi(QWidget *navtechitem)
    {
        navtechitem->setWindowTitle(QApplication::translate("navtechitem", "Form", Q_NULLPTR));
        serverNameLabel->setText(QApplication::translate("navtechitem", "Server Name", Q_NULLPTR));
        serverHostLabel->setText(QApplication::translate("navtechitem", "127.0.0.1:3000", Q_NULLPTR));
        getInfoButton->setText(QApplication::translate("navtechitem", "Get Info", Q_NULLPTR));
        deleteButton->setText(QApplication::translate("navtechitem", "Delete", Q_NULLPTR));
    } // retranslateUi

};

namespace Ui {
    class navtechitem: public Ui_navtechitem {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_NAVTECHITEM_H
