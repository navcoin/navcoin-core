// Copyright (C) 2017 by Juergen Skrotzky (JorgenVikingGod@gmail.com)
// Copyright (c) 2019-2020 The Navcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef STYLE_DARK_HEADER
#define STYLE_DARK_HEADER

#include <QApplication>
#include <QFile>
#include <QFont>
#include <QProxyStyle>
#include <QStyleFactory>

class StyleDark : public QProxyStyle {
  Q_OBJECT

 public:
  StyleDark();
  explicit StyleDark(QStyle *style);

  QStyle *baseStyle() const;

  void polish(QPalette &palette) override;

 private:
  QStyle *styleBase(QStyle *style = Q_NULLPTR) const;
};

#endif  // STYLE_DARK_HEADER
