// Copyright (c) 2011-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef NAVCOIN_QT_GUICONSTANTS_H
#define NAVCOIN_QT_GUICONSTANTS_H

#include <QString>

/* Milliseconds between price updates */
static const int PRICE_UPDATE_DELAY = 300000;

/* Milliseconds between model updates */
static const int MODEL_UPDATE_DELAY = 250;

/* Milliseconds between error log refreshes */
static const int ERROR_LOG_UPDATE_DELAY = 2500;

/* Initial number of debug logs to parse error log entries from */
static const int ERROR_LOG_INITIAL_COUNT = 500;

/* AskPassphraseDialog -- Maximum passphrase length */
static const int MAX_PASSPHRASE_SIZE = 1024;

/* NavcoinGUI -- Size of icons in status bar */
static const int STATUSBAR_ICONSIZE = 18;

static const bool DEFAULT_SPLASHSCREEN = true;

/**
 * Wallet colors
 */
static const QString COLOR_WHITE = "#fafafa";
static const QString COLOR_PURPLE = "#7d5ab5";
static const QString COLOR_PURPLE_DARK = "#3e1e6e";
static const QString COLOR_PURPLE_DARKER = "#351565";
static const QString COLOR_BLUE = "#42b6eb";
static const QString COLOR_BLUE_LIGHT = "#46b1e8";
static const QString COLOR_MAGENTA = "#c42bb7";
static const QString COLOR_GREY = "#c1c1c1";
static const QString COLOR_GRAY = "#c1c1c1";
static const QString COLOR_LINK = "#2a82da";

/* Invalid field background style */
#define STYLE_INVALID "background:#FF8080"

/* Transaction list -- unconfirmed transaction */
#define COLOR_UNCONFIRMED QColor(128, 128, 128)
/* Transaction list -- negative amount */
#define COLOR_NEGATIVE QColor(173, 43, 43)
/* Transaction list -- positive amount */
#define COLOR_POSITIVE QColor(73, 156, 76)
/* Transaction list -- bare address (without label) */
#define COLOR_BAREADDRESS QColor(140, 140, 140)
/* Transaction list -- anon transaction */
#define COLOR_ANON QColor(245, 166, 35)
/* Transaction list -- TX status decoration - open until date */
#define COLOR_TX_STATUS_OPENUNTILDATE QColor(64, 64, 255)
/* Transaction list -- TX status decoration - offline */
#define COLOR_TX_STATUS_OFFLINE QColor(192, 192, 192)
/* Transaction list -- TX status decoration - danger, tx needs attention */
#define COLOR_TX_STATUS_DANGER QColor(200, 100, 100)
/* Transaction list -- TX status decoration - default color */
#define COLOR_BLACK QColor(0, 0, 0)
/* Transaction list -- Address - default color */
#define COLOR_TXLIST QColor(220,140,30)

/* Tooltips longer than this (in characters) are converted into rich text,
   so that they can be word-wrapped.
 */
static const int TOOLTIP_WRAP_THRESHOLD = 80;

/* Maximum allowed URI length */
static const int MAX_URI_LENGTH = 255;

/* QRCodeDialog -- size of exported QR Code image */
#define QR_IMAGE_SIZE 260

/* Number of frames in spinner animation */
#define SPINNER_FRAMES 36

#define QAPP_ORG_NAME "Navcoin"
#define QAPP_ORG_DOMAIN "navcoin.org"
#define QAPP_APP_NAME_DEFAULT "Navcoin-Qt"
#define QAPP_APP_NAME_TESTNET "Navcoin-Qt-testnet"

#endif // NAVCOIN_QT_GUICONSTANTS_H
