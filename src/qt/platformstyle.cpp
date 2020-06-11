// Copyright (c) 2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/platformstyle.h>

#include <qt/guiconstants.h>

#include <QApplication>
#include <QColor>
#include <QIcon>
#include <QImage>
#include <QPalette>
#include <QPixmap>

namespace {
/* Local functions for colorizing single-color images */

void MakeImage(QImage& img, const QColor& colorbase)
{
    img = img.convertToFormat(QImage::Format_ARGB32);
    for (int x = img.width(); x--; )
    {
        for (int y = img.height(); y--; )
        {
            const QRgb rgb = img.pixel(x, y);
            img.setPixel(x, y, qRgba(colorbase.red(), colorbase.green(), colorbase.blue(), qAlpha(rgb)));
        }
    }
}

QIcon ColorizeIcon(const QIcon& ico, const QColor& colorbase)
{
    QIcon new_ico;
    for(QSize sz: ico.availableSizes())
    {
        QImage img(ico.pixmap(sz).toImage());
        MakeImage(img, colorbase);
        new_ico.addPixmap(QPixmap::fromImage(img));
    }
    return new_ico;
}

QImage ColorizeImage(const QString& filename, const QColor& colorbase)
{
    QImage img(filename);
    MakeImage(img, colorbase);
    return img;
}

QIcon ColorizeIcon(const QString& filename, const QColor& colorbase)
{
    return QIcon(QPixmap::fromImage(ColorizeImage(filename, colorbase)));
}

}


PlatformStyle::PlatformStyle():
    singleColor(0,0,0),
    textColor(0,0,0)
{
    // Determine icon highlighting color
    singleColor = QColor(QApplication::palette().color(QPalette::Highlight));

    // Determine text color
    textColor = QColor(QApplication::palette().color(QPalette::WindowText));
}

QImage PlatformStyle::Image(const QString& filename) const
{
    return ColorizeImage(filename, SingleColor());
}

QIcon PlatformStyle::Icon(const QString& filename) const
{
    return ColorizeIcon(filename, SingleColor());
}

QIcon PlatformStyle::Icon(const QString& filename, const QString& colorbase) const
{
    return ColorizeIcon(filename, QColor(colorbase));
}

QIcon PlatformStyle::Icon(const QIcon& icon) const
{
    return ColorizeIcon(icon, SingleColor());
}

QIcon PlatformStyle::IconAlt(const QString& filename) const
{
    return ColorizeIcon(filename, TextColor());
}

QIcon PlatformStyle::IconAlt(const QIcon& icon) const
{
    return ColorizeIcon(icon, TextColor());
}

const PlatformStyle *PlatformStyle::instantiate()
{
    return new PlatformStyle();
}

