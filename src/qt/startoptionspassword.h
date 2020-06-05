#pragma once

#include <QWidget>

namespace Ui
{
    class StartOptionsPassword;
}

/** Dialog to ask for passphrases. Used for encryption only
*/
class StartOptionsPassword : public QWidget
{
    Q_OBJECT

    public:
        explicit StartOptionsPassword(QWidget *parent = nullptr);
        ~StartOptionsPassword();
        QString getPass();
        QString getPassConf();

    private:
        Ui::StartOptionsPassword *ui;
};
