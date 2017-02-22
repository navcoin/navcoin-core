// Copyright (c) 2011-2014 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef NAVCOIN_QT_NAVCOINADDRESSVALIDATOR_H
#define NAVCOIN_QT_NAVCOINADDRESSVALIDATOR_H

#include <QValidator>

/** Base58 entry widget validator, checks for valid characters and
 * removes some whitespace.
 */
class NavCoinAddressEntryValidator : public QValidator
{
    Q_OBJECT

public:
    explicit NavCoinAddressEntryValidator(QObject *parent);

    State validate(QString &input, int &pos) const;
};

/** NavCoin address widget validator, checks for a valid navcoin address.
 */
class NavCoinAddressCheckValidator : public QValidator
{
    Q_OBJECT

public:
    explicit NavCoinAddressCheckValidator(QObject *parent);

    State validate(QString &input, int &pos) const;
};

#endif // NAVCOIN_QT_NAVCOINADDRESSVALIDATOR_H
