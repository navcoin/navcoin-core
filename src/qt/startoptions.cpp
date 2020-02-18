//
// Created by Kolby on 6/19/2019.
//

#include <startoptions.h>
#include <ui_startoptions.h>


#include <QKeyEvent>
#include <QMessageBox>
#include <QPushButton>

StartOptions::StartOptions(QWidget *parent)
    : QWidget(parent), ui(new Ui::StartOptions) {
    ui->setupUi(this);

    ui->welcomeLabel2->setStyleSheet(
            "QLabel{font-size: 30px; color: #7578A2;}");
    ui->welcomeLabel2->setText(tr(
        "Welcome to Navcoin!"));

}

int StartOptions::getRows() {
    rows = 4;
    return rows;
};

StartOptions::~StartOptions() {
    delete ui;
}
