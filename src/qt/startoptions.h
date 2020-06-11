//
// Created by Kolby on 6/19/2019.
//
#pragma once

#include <QWidget>

namespace Ui {
class StartOptions;
}

/** Dialog to ask for passphrases. Used for encryption only
 */
class StartOptions : public QWidget {
    Q_OBJECT

public:
    explicit StartOptions(QWidget *parent = nullptr);
    ~StartOptions();
    int getRows();

private:
    int rows;
    Ui::StartOptions *ui;
};
