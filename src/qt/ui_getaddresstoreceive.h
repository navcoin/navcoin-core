/********************************************************************************
** Form generated from reading UI file 'getaddresstoreceive.ui'
**
** Created by: Qt User Interface Compiler version 5.10.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_GETADDRESSTORECEIVE_H
#define UI_GETADDRESSTORECEIVE_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>
#include "receiverequestdialog.h"

QT_BEGIN_NAMESPACE

class Ui_getAddressToReceive
{
public:
    QGridLayout *gridLayout;
    QVBoxLayout *verticalLayout;
    QHBoxLayout *horizontalLayout;
    QSpacerItem *horizontalSpacer;
    QLabel *label;
    QSpacerItem *horizontalSpacer_2;
    QSpacerItem *verticalSpacer;
    QSpacerItem *verticalSpacer_2;
    QHBoxLayout *horizontalLayout_8;
    QSpacerItem *horizontalSpacer_4;
    QHBoxLayout *horizontalLayout_2;
    QSpacerItem *horizontalSpacer_16;
    QVBoxLayout *verticalLayout_3;
    QRImageWidget *lblQRCode;
    QSpacerItem *horizontalSpacer_15;
    QSpacerItem *horizontalSpacer_3;
    QSpacerItem *verticalSpacer_3;
    QHBoxLayout *horizontalLayout_7;
    QSpacerItem *horizontalSpacer_13;
    QPushButton *copyClipboardButton;
    QPushButton *newAddressButton;
    QPushButton *requestPaymentButton;
    QPushButton *requestNewAddressButton;
    QSpacerItem *horizontalSpacer_14;
    QSpacerItem *verticalSpacer_4;

    void setupUi(QWidget *getAddressToReceive)
    {
        if (getAddressToReceive->objectName().isEmpty())
            getAddressToReceive->setObjectName(QStringLiteral("getAddressToReceive"));
        getAddressToReceive->setWindowModality(Qt::NonModal);
        getAddressToReceive->resize(1497, 666);
        QSizePolicy sizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(getAddressToReceive->sizePolicy().hasHeightForWidth());
        getAddressToReceive->setSizePolicy(sizePolicy);
        gridLayout = new QGridLayout(getAddressToReceive);
        gridLayout->setObjectName(QStringLiteral("gridLayout"));
        verticalLayout = new QVBoxLayout();
        verticalLayout->setObjectName(QStringLiteral("verticalLayout"));
        verticalLayout->setSizeConstraint(QLayout::SetNoConstraint);
        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setObjectName(QStringLiteral("horizontalLayout"));
        horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout->addItem(horizontalSpacer);

        label = new QLabel(getAddressToReceive);
        label->setObjectName(QStringLiteral("label"));
        QFont font;
        font.setPointSize(11);
        label->setFont(font);

        horizontalLayout->addWidget(label);

        horizontalSpacer_2 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout->addItem(horizontalSpacer_2);


        verticalLayout->addLayout(horizontalLayout);

        verticalSpacer = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Maximum);

        verticalLayout->addItem(verticalSpacer);

        verticalSpacer_2 = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout->addItem(verticalSpacer_2);

        horizontalLayout_8 = new QHBoxLayout();
        horizontalLayout_8->setObjectName(QStringLiteral("horizontalLayout_8"));
        horizontalSpacer_4 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_8->addItem(horizontalSpacer_4);

        horizontalLayout_2 = new QHBoxLayout();
        horizontalLayout_2->setObjectName(QStringLiteral("horizontalLayout_2"));
        horizontalSpacer_16 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_2->addItem(horizontalSpacer_16);

        verticalLayout_3 = new QVBoxLayout();
        verticalLayout_3->setObjectName(QStringLiteral("verticalLayout_3"));
        lblQRCode = new QRImageWidget(getAddressToReceive);
        lblQRCode->setObjectName(QStringLiteral("lblQRCode"));
        QSizePolicy sizePolicy1(QSizePolicy::Expanding, QSizePolicy::Fixed);
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(lblQRCode->sizePolicy().hasHeightForWidth());
        lblQRCode->setSizePolicy(sizePolicy1);
        lblQRCode->setMinimumSize(QSize(300, 320));
        lblQRCode->setStyleSheet(QStringLiteral(""));
        lblQRCode->setTextFormat(Qt::PlainText);
        lblQRCode->setAlignment(Qt::AlignCenter);
        lblQRCode->setWordWrap(true);

        verticalLayout_3->addWidget(lblQRCode);


        horizontalLayout_2->addLayout(verticalLayout_3);

        horizontalSpacer_15 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_2->addItem(horizontalSpacer_15);


        horizontalLayout_8->addLayout(horizontalLayout_2);

        horizontalSpacer_3 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_8->addItem(horizontalSpacer_3);


        verticalLayout->addLayout(horizontalLayout_8);

        verticalSpacer_3 = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout->addItem(verticalSpacer_3);

        horizontalLayout_7 = new QHBoxLayout();
        horizontalLayout_7->setObjectName(QStringLiteral("horizontalLayout_7"));
        horizontalSpacer_13 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_7->addItem(horizontalSpacer_13);

        copyClipboardButton = new QPushButton(getAddressToReceive);
        copyClipboardButton->setObjectName(QStringLiteral("copyClipboardButton"));

        horizontalLayout_7->addWidget(copyClipboardButton);

        newAddressButton = new QPushButton(getAddressToReceive);
        newAddressButton->setObjectName(QStringLiteral("newAddressButton"));

        horizontalLayout_7->addWidget(newAddressButton);

        requestPaymentButton = new QPushButton(getAddressToReceive);
        requestPaymentButton->setObjectName(QStringLiteral("requestPaymentButton"));

        horizontalLayout_7->addWidget(requestPaymentButton);

        requestNewAddressButton = new QPushButton(getAddressToReceive);
        requestNewAddressButton->setObjectName(QStringLiteral("requestNewAddressButton"));

        horizontalLayout_7->addWidget(requestNewAddressButton);

        horizontalSpacer_14 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_7->addItem(horizontalSpacer_14);


        verticalLayout->addLayout(horizontalLayout_7);

        verticalSpacer_4 = new QSpacerItem(20, 80, QSizePolicy::Minimum, QSizePolicy::MinimumExpanding);

        verticalLayout->addItem(verticalSpacer_4);


        gridLayout->addLayout(verticalLayout, 0, 0, 1, 1);


        retranslateUi(getAddressToReceive);

        QMetaObject::connectSlotsByName(getAddressToReceive);
    } // setupUi

    void retranslateUi(QWidget *getAddressToReceive)
    {
        getAddressToReceive->setWindowTitle(QApplication::translate("getAddressToReceive", "Form", nullptr));
        label->setText(QApplication::translate("getAddressToReceive", "Use the following address to receive NavCoins:", nullptr));
#ifndef QT_NO_TOOLTIP
        lblQRCode->setToolTip(QApplication::translate("getAddressToReceive", "QR Code", nullptr));
#endif // QT_NO_TOOLTIP
        copyClipboardButton->setText(QApplication::translate("getAddressToReceive", "Copy to clipboard", nullptr));
        newAddressButton->setText(QApplication::translate("getAddressToReceive", "Generate a new address", nullptr));
        requestPaymentButton->setText(QApplication::translate("getAddressToReceive", "Request payment", nullptr));
        requestNewAddressButton->setText(QApplication::translate("getAddressToReceive", "List old addresses", nullptr));
    } // retranslateUi

};

namespace Ui {
    class getAddressToReceive: public Ui_getAddressToReceive {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_GETADDRESSTORECEIVE_H
