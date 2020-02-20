//
// Copyright (c) 2019 DeVault developers
// Created by Kolby on 6/19/2019.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
//
#include <startoptionsdialog.h>
#include <startoptionsmain.h>
#include <ui_startoptionsmain.h>

#include <QDebug>
#include <QFileDialog>
#include <iostream>
#include <qt/guiutil.h>
#include <random.h>
#include <utilstrencodings.h>

#include <ui_interface.h>
#include <mnemonic/mnemonic.h>
#include <mnemonic/generateseed.h>

#include <QKeyEvent>
#include <QMessageBox>
#include <QPushButton>

inline bool is_not_space(int c) { return !std::isspace(c); }

StartOptionsMain::StartOptionsMain(QWidget *parent)
    : QDialog(parent), ui(new Ui::StartOptionsMain) {
    ui->setupUi(this);

    this->setWindowTitle(tr("Navcoin Wallet Setup"));

    dictionary dic = string_to_lexicon("english");
    for(unsigned long i=0; i< dic.size() ; i++){
        qWordList << dic[i];
    }

    this->setContentsMargins(0, 0, 0, 0);
    ui->QStackTutorialContainer->setContentsMargins(0, 0, 0, 10);
    ui->QStackTutorialContainer->setObjectName("startOptions");

    ui->Back->setVisible(false);
    ui->Next->setVisible(false);
    startOptions = new StartOptions(this);
    ui->QStackTutorialContainer->addWidget(startOptions);
    ui->QStackTutorialContainer->setCurrentWidget(startOptions);
}

StartOptionsMain::~StartOptionsMain() {
    delete ui;
}

void StartOptionsMain::on_NewWallet_clicked() {
    pageNum = CreateOrRestorePage;
    ui->NewWallet->setVisible(false);
    ui->RestoreWallet->setVisible(false);
    ui->Back->setVisible(true);
    ui->Next->setVisible(true);
    rows = startOptions->getRows();

    //Generate mnemonic phrase from fresh entropy
    mnemonic = "";
    navcoin::GenerateNewMnemonicSeed(mnemonic, "english");

    std::stringstream ss(mnemonic);
    std::istream_iterator<std::string> begin(ss);
    std::istream_iterator<std::string> end;
    words.clear();
    words = std::vector<std::string>(begin, end);


    startOptionsRevealed = new StartOptionsRevealed(words, rows, this);
    ui->QStackTutorialContainer->addWidget(startOptionsRevealed);
    ui->QStackTutorialContainer->setCurrentWidget(startOptionsRevealed);
}

void StartOptionsMain::on_RestoreWallet_clicked() {
    pageNum = CheckWordsPage;
    ui->NewWallet->setVisible(false);
    ui->RestoreWallet->setVisible(false);
    ui->Back->setVisible(true);
    ui->Next->setVisible(true);
    rows = startOptions->getRows();

    startOptionsRestore = new StartOptionsRestore(qWordList, rows, this);
    ui->QStackTutorialContainer->addWidget(startOptionsRestore);
    ui->QStackTutorialContainer->setCurrentWidget(startOptionsRestore);
}

void StartOptionsMain::on_Back_clicked() {
    /*  Pages
     * Page 1 : Main page were you select the amount of words and the option
     *      Path 1 : Create wallet
     *          Page 2 : Shows you your words
     *          Page 3 : Order the words
     *      Path 2 : Restore wallet
     *          Page 4 Enter words to restore
     */
    switch (pageNum) {
        case StartPage: {
            // does nothing as you can not click back on page one
        }
        case CreateOrRestorePage: {
            pageNum = StartPage;
            ui->NewWallet->setVisible(true);
            ui->RestoreWallet->setVisible(true);
            ui->Back->setVisible(false);
            ui->Next->setVisible(false);
            ui->QStackTutorialContainer->setCurrentWidget(startOptions);
            break;
        }
        case OrderWordsPage: {
            pageNum = CreateOrRestorePage;
            ui->QStackTutorialContainer->addWidget(startOptionsRevealed);
            ui->QStackTutorialContainer->setCurrentWidget(startOptionsRevealed);
            break;
        }
        case CheckWordsPage: {
            pageNum = StartPage;
            ui->NewWallet->setVisible(true);
            ui->RestoreWallet->setVisible(true);
            ui->Back->setVisible(false);
            ui->Next->setVisible(false);
            ui->QStackTutorialContainer->setCurrentWidget(startOptions);
            break;
        }
    }
}

void StartOptionsMain::on_Next_clicked() {
    auto rtrim = [](std::string& s) { s.erase(std::find_if(s.rbegin(), s.rend(), is_not_space).base(), s.end()); };
    /*  Pages
     * Page 1 : Main page were you select the amount of words and the option
     *      Path 1 : Create wallet
     *          Page 2 : Shows you your words
     *          Page 3 : Order the words
     *      Path 2 : Restore wallet
     *          Page 4 Enter words to restore
     */
    switch (pageNum) {
        case StartPage: {
            // does nothing as you can not click back on page one
        }

        case CreateOrRestorePage: {
            pageNum = OrderWordsPage;
            startOptionsSort = new StartOptionsSort(words, rows, this);
            ui->QStackTutorialContainer->addWidget(startOptionsSort);
            ui->QStackTutorialContainer->setCurrentWidget(startOptionsSort);
            break;
        }
        case OrderWordsPage: {
            words_empty_str = "";
            std::list<QString> word_str = startOptionsSort->getOrderedStrings();
            // reverses the lists order
            word_str.reverse();
            for (QString &q_word : word_str) {
                if (words_empty_str.empty())
                    words_empty_str = q_word.toStdString();
                else
                    words_empty_str += "" + q_word.toStdString();
            }

            words_mnemonic = "";
            for (std::string &q_word : words) {
                if (words_mnemonic.empty())
                    words_mnemonic = q_word;
                else
                    words_mnemonic += "" + q_word;
            }
            rtrim(words_empty_str);
            rtrim(words_mnemonic);
            if (words_empty_str != words_mnemonic) {
                QString error = tr("Unfortunately, your words are in the wrong "
                                "order. Please try again.");
                StartOptionsDialog dlg(error, this);
                dlg.exec();
            } else {
                wordsDone = join(words, " ");
                QApplication::quit();
            }
            break;
        }
        case CheckWordsPage: {
            std::vector<std::string> word_str = startOptionsRestore->getOrderedStrings();

            std::string seedphrase = "";
            for (std::string &q_word : word_str) {
                if (seedphrase.empty())
                    seedphrase = q_word;
                else
                    seedphrase += " " + q_word;
            }
            // reverses the lists order
            dictionary lexicon = string_to_lexicon("english");
            if (validate_mnemonic(sentence_to_word_list(seedphrase), lexicon)) {
                wordsDone = join(word_str, " ");
                QApplication::quit();
            } else {
                QString error =
                    tr("Unfortunately, your words seem to be invalid. This is "
                    "most likely because one or more are misspelled. Please "
                    "double check your spelling and word order. If you "
                    "continue to have issue your words may not currently be in "
                    "our dictionary.");
                StartOptionsDialog dlg(error, this);
                dlg.exec();
            }

            break;
        }
    }
}
