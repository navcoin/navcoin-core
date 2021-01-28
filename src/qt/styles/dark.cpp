// Copyright (C) 2017 by Juergen Skrotzky (JorgenVikingGod@gmail.com)
// Copyright (c) 2019-2020 The Navcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <guiconstants.h>
#include <styles/dark.h>

StyleDark::StyleDark() : StyleDark(styleBase()) {}

StyleDark::StyleDark(QStyle *style) : QProxyStyle(style) {}

QStyle *StyleDark::styleBase(QStyle *style) const {
  static QStyle *base =
      !style ? QStyleFactory::create(QStringLiteral("Fusion")) : style;
  return base;
}

QStyle *StyleDark::baseStyle() const { return styleBase(); }

void StyleDark::polish(QPalette &palette) {
  // modify palette
  palette.setColor(QPalette::Window, QColor(53, 53, 53));
  palette.setColor(QPalette::WindowText, COLOR_WHITE);
  palette.setColor(QPalette::Disabled, QPalette::WindowText, COLOR_GREY);
  palette.setColor(QPalette::Base, QColor(42, 42, 42));
  palette.setColor(QPalette::AlternateBase, QColor(66, 66, 66));
  palette.setColor(QPalette::ToolTipBase, COLOR_WHITE);
  palette.setColor(QPalette::ToolTipText, QColor(53, 53, 53));
  palette.setColor(QPalette::Text, COLOR_WHITE);
  palette.setColor(QPalette::Disabled, QPalette::Text, COLOR_GREY);
  palette.setColor(QPalette::Dark, QColor(35, 35, 35));
  palette.setColor(QPalette::Shadow, QColor(20, 20, 20));
  palette.setColor(QPalette::Button, QColor(53, 53, 53));
  palette.setColor(QPalette::ButtonText, COLOR_WHITE);
  palette.setColor(QPalette::Disabled, QPalette::ButtonText, COLOR_GREY);
  palette.setColor(QPalette::BrightText, Qt::red);
  palette.setColor(QPalette::Link, QColor(42, 130, 218));
  palette.setColor(QPalette::Highlight, QColor(COLOR_PURPLE));
  palette.setColor(QPalette::Disabled, QPalette::Highlight, QColor(80, 80, 80));
  palette.setColor(QPalette::HighlightedText, COLOR_WHITE);
  palette.setColor(QPalette::Disabled, QPalette::HighlightedText, COLOR_GREY);
}
