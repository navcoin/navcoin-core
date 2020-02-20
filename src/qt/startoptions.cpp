//
// Created by Kolby on 6/19/2019.
//

#include <guiutil.h>

#include <startoptions.h>
#include <ui_startoptions.h>


#include <QKeyEvent>
#include <QMessageBox>
#include <QPushButton>

StartOptions::StartOptions(QWidget *parent)
    : QWidget(parent), ui(new Ui::StartOptions) {
    ui->setupUi(this);

    // Size of the icon
    QSize iconSize(400 * GUIUtil::scale(), 95 * GUIUtil::scale());

    // Load the icon
    QPixmap icon = QPixmap(":icons/navcoin_full").scaled(iconSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);

    // Add alt text
    ui->welcomeIcon->setToolTip(tr("Welcome to Navcoin!"));

    // Add the icon
    ui->welcomeIcon->setPixmap(icon);
}

int StartOptions::getRows() {
    rows = 4;
    return rows;
};

StartOptions::~StartOptions() {
    delete ui;
}
