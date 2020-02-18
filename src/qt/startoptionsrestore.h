//
// Created by Kolby on 6/19/2019.
//
#pragma once

#include <QLineEdit>
#include <QString>
#include <QWidget>
#include <list>

namespace Ui {
class StartOptionsRestore;
}

/** Dialog to ask for passphrases. Used for encryption only
 */
class StartOptionsRestore : public QWidget {
    Q_OBJECT

public:
    explicit StartOptionsRestore(QStringList wordList, int rows,
                                 QWidget *parent = nullptr);
    ~StartOptionsRestore();
    std::vector<std::string> getOrderedStrings();

private Q_SLOTS:
    void textChanged(const QString &text);

private:
    Ui::StartOptionsRestore *ui;
    std::list<QLineEdit *> editList;

    QStringList wordList;
};
