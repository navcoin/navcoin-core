// Copyright (C) 2017 by Juergen Skrotzky (JorgenVikingGod@gmail.com)
// Copyright (c) 2019-2020 The NavCoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef STYLE_LIGHT_HEADER
#define STYLE_LIGHT_HEADER

#include <QApplication>
#include <QFile>
#include <QFont>
#include <QProxyStyle>
#include <QStyleFactory>

class StyleLight : public QProxyStyle {
  Q_OBJECT

 public:
  StyleLight();
  explicit StyleLight(QStyle *style);

  QStyle *baseStyle() const;

  void polish(QPalette &palette) override;

 private:
  QStyle *styleBase(QStyle *style = Q_NULLPTR) const;
};

#endif  // STYLE_LIGHT_HEADER
