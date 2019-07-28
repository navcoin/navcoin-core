// Copyright (c) 2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef NAVCOIN_QT_PLATFORMSTYLE_H
#define NAVCOIN_QT_PLATFORMSTYLE_H

#include <QIcon>
#include <QPixmap>
#include <QString>

/* Coin network-specific GUI style information */
class PlatformStyle
{
public:
    /** Get style associated with provided platform name, or 0 if not known */
    static const PlatformStyle *instantiate();

    const QString &getName() const { return name; }

    QColor TextColor() const { return textColor; }
    QColor SingleColor() const { return singleColor; }

    /** Colorize an image (given filename) with the icon color */
    QImage Image(const QString& filename) const;

    /** Colorize an icon (given filename) with the icon color */
    QIcon Icon(const QString& filename) const;
    QIcon Icon(const QString& filename, const QString& colorbase) const;

    /** Colorize an icon (given object) with the icon color */
    QIcon Icon(const QIcon& icon) const;

    /** Colorize an icon (given filename) with the text color */
    QIcon IconAlt(const QString& filename) const;

    /** Colorize an icon (given object) with the text color */
    QIcon IconAlt(const QIcon& icon) const;

private:
    PlatformStyle();

    QString name;
    QColor singleColor;
    QColor textColor;
};

#endif // NAVCOIN_QT_PLATFORMSTYLE_H

