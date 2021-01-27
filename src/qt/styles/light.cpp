// Copyright (C) 2017 by Juergen Skrotzky (JorgenVikingGod@gmail.com)
// Copyright (c) 2019-2020 The Navcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <guiconstants.h>
#include <styles/light.h>

StyleLight::StyleLight() : StyleLight(styleBase()) {}

StyleLight::StyleLight(QStyle *style) : QProxyStyle(style) {}

QStyle *StyleLight::styleBase(QStyle *style) const {
  static QStyle *base =
      !style ? QStyleFactory::create(QStringLiteral("Fusion")) : style;
  return base;
}

QStyle *StyleLight::baseStyle() const { return styleBase(); }

void StyleLight::polish(QPalette &palette) {
  // modify palette
  palette.setColor(QPalette::Window, QColor(239, 239, 239));
  palette.setColor(QPalette::WindowText, COLOR_BLACK);
  palette.setColor(QPalette::Disabled, QPalette::WindowText, COLOR_GREY);
  palette.setColor(QPalette::Base, QColor(250, 250, 250));
  palette.setColor(QPalette::AlternateBase, QColor(223, 223, 223));
  palette.setColor(QPalette::ToolTipBase, COLOR_BLACK);
  palette.setColor(QPalette::ToolTipText, QColor(239, 239, 239));
  palette.setColor(QPalette::Text, COLOR_BLACK);
  palette.setColor(QPalette::Disabled, QPalette::Text, COLOR_GREY);
  palette.setColor(QPalette::Dark, QColor(150, 150, 150));
  palette.setColor(QPalette::Shadow, QColor(170, 170, 170));
  palette.setColor(QPalette::Button, QColor(239, 239, 239));
  palette.setColor(QPalette::ButtonText, COLOR_BLACK);
  palette.setColor(QPalette::Disabled, QPalette::ButtonText, COLOR_GREY);
  palette.setColor(QPalette::BrightText, Qt::red);
  palette.setColor(QPalette::Link, COLOR_LINK);
  palette.setColor(QPalette::Highlight, QColor(COLOR_PURPLE));
  palette.setColor(QPalette::Disabled, QPalette::Highlight, QColor(80, 80, 80));
  palette.setColor(QPalette::HighlightedText, COLOR_BLACK);
  palette.setColor(QPalette::Disabled, QPalette::HighlightedText, COLOR_GREY);
}
