// Copyright (c) 2011-2014 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "navcoinaddressvalidator.h"

#include "base58.h"
#include "utils/dns_utils.h"
#include "util.h"

/* Base58 characters are:
     "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz"

  This is:
  - All numbers except for '0'
  - All upper-case letters except for 'I' and 'O'
  - All lower-case letters except for 'l'
*/

NavCoinAddressEntryValidator::NavCoinAddressEntryValidator(QObject *parent) :
    QValidator(parent)
{
}

QValidator::State NavCoinAddressEntryValidator::validate(QString &input, int &pos) const
{
    Q_UNUSED(pos);

    // Empty address is "intermediate" input
    if (input.isEmpty())
        return QValidator::Intermediate;

    // Correction
    for (int idx = 0; idx < input.size();)
    {
        bool removeChar = false;
        QChar ch = input.at(idx);
        // Corrections made are very conservative on purpose, to avoid
        // users unexpectedly getting away with typos that would normally
        // be detected, and thus sending to the wrong address.
        switch(ch.unicode())
        {
        // Qt categorizes these as "Other_Format" not "Separator_Space"
        case 0x200B: // ZERO WIDTH SPACE
        case 0xFEFF: // ZERO WIDTH NO-BREAK SPACE
            removeChar = true;
            break;
        default:
            break;
        }

        // Remove whitespace
        if (ch.isSpace())
            removeChar = true;

        // To next character
        if (removeChar)
            input.remove(idx, 1);
        else
            ++idx;
    }

    // Validation
    QValidator::State state = QValidator::Acceptable;
    for (int idx = 0; idx < input.size(); ++idx)
    {
        int ch = input.at(idx).unicode();

        if (((ch >= '0' && ch<='9') ||
            (ch >= 'a' && ch<='z') ||
            (ch >= 'A' && ch<='Z')) ||
            ch == '.' || ch == '@' )
        {
            // Alphanumeric and not a 'forbidden' character
        }
        else
        {
            state = QValidator::Invalid;
        }
    }

    return state;
}

NavCoinAddressCheckValidator::NavCoinAddressCheckValidator(QObject *parent) :
    QValidator(parent)
{
}

QValidator::State NavCoinAddressCheckValidator::validate(QString &input, int &pos) const
{
  Q_UNUSED(pos);
#ifdef HAVE_UNBOUND
  utils::DNSResolver* DNS = nullptr;

  // Validate the passed NavCoin address
  if(DNS->check_address_syntax(input.toStdString().c_str()))
  {

    bool dnssec_valid;
    std::vector<std::string> addresses = utils::dns_utils::addresses_from_url(input.toStdString().c_str(), dnssec_valid);

    if(addresses.empty() || (!dnssec_valid && GetBoolArg("-requirednssec",true)))
      return QValidator::Invalid;
    else
      return QValidator::Acceptable;

  }
  else
  {
#endif

    CNavCoinAddress addr(input.toStdString());
    if (addr.IsValid())
      return QValidator::Acceptable;

    return QValidator::Invalid;
#ifdef HAVE_UNBOUND
  }
#endif

}
