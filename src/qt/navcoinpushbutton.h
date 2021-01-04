// Copyright (c) 2019-2020 The NavCoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef NAVCOINPUSHBUTTON_H
#define NAVCOINPUSHBUTTON_H

#include <QIcon>
#include <QPushButton>
#include <QPainter>
#include <QString>
#include <QWidget>

class NavCoinPushButton : public QPushButton
{
    Q_OBJECT

public:
    NavCoinPushButton(QString label);
    void paintEvent(QPaintEvent*);
    void setBadge(int nValue);

private:
    QIcon getBadgeIcon(int nValue);
};

#endif // NAVCOINPUSHBUTTON_H
