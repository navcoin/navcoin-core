// Copyright (c) 2011-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <QDialog>
#include <support/allocators/secure.h>

class CWallet;

namespace Ui {
class StartOptionsDialog;
}

/** Dialog to ask for passphrases. Used for encryption only
 */
class StartOptionsDialog : public QDialog {
    Q_OBJECT

public:
    explicit StartOptionsDialog(const QString error_words, QWidget *parent);
    ~StartOptionsDialog();

    void accept() override;

private:
    Ui::StartOptionsDialog *ui;
};
