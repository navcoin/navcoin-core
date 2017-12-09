/********************************************************************************
** Form generated from reading UI file 'navtechinit.ui'
**
** Created by: Qt User Interface Compiler version 5.7.1
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_NAVTECHINIT_H
#define UI_NAVTECHINIT_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPlainTextEdit>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_NavTechInit
{
public:
    QWidget *verticalLayoutWidget;
    QVBoxLayout *verticalLayout;
    QLabel *label;
    QSpacerItem *verticalSpacer;
    QLabel *label_2;
    QSpacerItem *verticalSpacer_3;
    QPlainTextEdit *plainTextEdit;
    QWidget *horizontalLayoutWidget;
    QHBoxLayout *horizontalLayout_2;
    QDialogButtonBox *buttonBox_2;

    void setupUi(QDialog *NavTechInit)
    {
        if (NavTechInit->objectName().isEmpty())
            NavTechInit->setObjectName(QStringLiteral("NavTechInit"));
        NavTechInit->resize(651, 314);
        QFont font;
        font.setStyleStrategy(QFont::PreferAntialias);
        NavTechInit->setFont(font);
        verticalLayoutWidget = new QWidget(NavTechInit);
        verticalLayoutWidget->setObjectName(QStringLiteral("verticalLayoutWidget"));
        verticalLayoutWidget->setGeometry(QRect(10, 10, 631, 261));
        verticalLayout = new QVBoxLayout(verticalLayoutWidget);
        verticalLayout->setObjectName(QStringLiteral("verticalLayout"));
        verticalLayout->setContentsMargins(0, 0, 0, 0);
        label = new QLabel(verticalLayoutWidget);
        label->setObjectName(QStringLiteral("label"));
        label->setMaximumSize(QSize(600, 16777215));
        label->setAcceptDrops(false);
        label->setWordWrap(true);

        verticalLayout->addWidget(label);

        verticalSpacer = new QSpacerItem(20, 5, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout->addItem(verticalSpacer);

        label_2 = new QLabel(verticalLayoutWidget);
        label_2->setObjectName(QStringLiteral("label_2"));
        label_2->setMaximumSize(QSize(600, 16777215));
        label_2->setWordWrap(true);

        verticalLayout->addWidget(label_2);

        verticalSpacer_3 = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout->addItem(verticalSpacer_3);

        plainTextEdit = new QPlainTextEdit(verticalLayoutWidget);
        plainTextEdit->setObjectName(QStringLiteral("plainTextEdit"));

        verticalLayout->addWidget(plainTextEdit);

        horizontalLayoutWidget = new QWidget(NavTechInit);
        horizontalLayoutWidget->setObjectName(QStringLiteral("horizontalLayoutWidget"));
        horizontalLayoutWidget->setGeometry(QRect(10, 280, 631, 32));
        horizontalLayout_2 = new QHBoxLayout(horizontalLayoutWidget);
        horizontalLayout_2->setObjectName(QStringLiteral("horizontalLayout_2"));
        horizontalLayout_2->setContentsMargins(0, 0, 0, 0);
        buttonBox_2 = new QDialogButtonBox(horizontalLayoutWidget);
        buttonBox_2->setObjectName(QStringLiteral("buttonBox_2"));
        QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(buttonBox_2->sizePolicy().hasHeightForWidth());
        buttonBox_2->setSizePolicy(sizePolicy);
        buttonBox_2->setOrientation(Qt::Horizontal);
        buttonBox_2->setStandardButtons(QDialogButtonBox::Save);

        horizontalLayout_2->addWidget(buttonBox_2);


        retranslateUi(NavTechInit);
        QObject::connect(buttonBox_2, SIGNAL(accepted()), NavTechInit, SLOT(accept()));

        QMetaObject::connectSlotsByName(NavTechInit);
    } // setupUi

    void retranslateUi(QDialog *NavTechInit)
    {
        NavTechInit->setWindowTitle(QApplication::translate("NavTechInit", "NavTech Setup", Q_NULLPTR));
        label->setText(QApplication::translate("NavTechInit", "NavCoin uses an unique parallel cluster of nodes called NavTech to protect the privacy of your transactions.", Q_NULLPTR));
        label_2->setText(QApplication::translate("NavTechInit", "You will find below the list of the NavCoin Foundation Servers. Feel free to modify this list to include your prefered servers:", Q_NULLPTR));
        plainTextEdit->setPlainText(QApplication::translate("NavTechInit", "95.183.52.55:3000\n"
"95.183.52.28:3000\n"
"95.183.52.29:3000\n"
"95.183.53.184:3000\n"
"", Q_NULLPTR));
    } // retranslateUi

};

namespace Ui {
    class NavTechInit: public Ui_NavTechInit {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_NAVTECHINIT_H
