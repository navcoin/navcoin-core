// Copyright (c) 2011-2014 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef DEVAULT_QT_DEVAULTADDRESSVALIDATOR_H
#define DEVAULT_QT_DEVAULTADDRESSVALIDATOR_H

#include <QValidator>

/** Base58 entry widget validator, checks for valid characters and
 * removes some whitespace.
 */
class DeVaultAddressEntryValidator : public QValidator
{
    Q_OBJECT

public:
    explicit DeVaultAddressEntryValidator(QObject *parent);

    State validate(QString &input, int &pos) const;
};

/** DeVault address widget validator, checks for a valid devault address.
 */
class DeVaultAddressCheckValidator : public QValidator
{
    Q_OBJECT

public:
    explicit DeVaultAddressCheckValidator(QObject *parent);

    State validate(QString &input, int &pos) const;
};

#endif // DEVAULT_QT_DEVAULTADDRESSVALIDATOR_H
