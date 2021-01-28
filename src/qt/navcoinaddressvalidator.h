// Copyright (c) 2011-2014 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef NAVCOIN_QT_NAVCOINADDRESSVALIDATOR_H
#define NAVCOIN_QT_NAVCOINADDRESSVALIDATOR_H

#include <QValidator>

/** Base58 entry widget validator, checks for valid characters and
 * removes some whitespace.
 */
class NavcoinAddressEntryValidator : public QValidator
{
    Q_OBJECT

public:
    explicit NavcoinAddressEntryValidator(QObject *parent);

    State validate(QString &input, int &pos) const;
};

/** Navcoin address widget validator, checks for a valid navcoin address.
 */
class NavcoinAddressCheckValidator : public QValidator
{
    Q_OBJECT

public:
    explicit NavcoinAddressCheckValidator(QObject *parent);

    State validate(QString &input, int &pos) const;
};

#endif // NAVCOIN_QT_NAVCOINADDRESSVALIDATOR_H
