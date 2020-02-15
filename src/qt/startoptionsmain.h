//
// Created by Kolby on 6/19/2019.
//
#pragma once

#include <QDialog>
#include <QWidget>

#include "startoptions.h"
#include <startoptionsrestore.h>
#include <startoptionsrevealed.h>
#include <startoptionssort.h>

#include "mnemonic/mnemonic.h"

class WalletModel;

namespace Ui {
class StartOptionsMain;
}

/** Dialog to ask for passphrases. Used for encryption only
 */
class StartOptionsMain : public QDialog {
    Q_OBJECT

public:
    enum Pages { StartPage, CreateOrRestorePage, OrderWordsPage, CheckWordsPage };
    explicit StartOptionsMain(QWidget *parent);
    ~StartOptionsMain();

    std::string getWords() { return wordsDone; }

public Q_SLOTS:
    void on_RestoreWallet_clicked();
    void on_Next_clicked();
    void on_Back_clicked();
    void on_NewWallet_clicked();

private:
    Ui::StartOptionsMain *ui;
    Pages pageNum = StartPage;
    int rows;
    QStringList qWordList;

    std::string wordsDone;
    std::vector<std::string> words;
    std::vector<std::string> wordsList;
    std::string mnemonic;

    StartOptions *startOptions;
    StartOptionsRevealed *startOptionsRevealed;
    StartOptionsSort *startOptionsSort;
    StartOptionsRestore *startOptionsRestore;
    std::string words_empty_str;
    std::string words_mnemonic;
};
