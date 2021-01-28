// Copyright (c) 2011-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/navcoinunits.h>

#include <primitives/transaction.h>
#include <util.h>

#include <math.h>

#include <QStringList>
#include <QSettings>

#include <sstream>
#include <string>
#include <iomanip>

NavcoinUnits::NavcoinUnits(QObject *parent):
    QAbstractListModel(parent),
    unitlist(availableUnits())
{
}

QList<NavcoinUnits::Unit> NavcoinUnits::availableUnits()
{
    QList<NavcoinUnits::Unit> unitlist;

    unitlist.append(NAV); // Navcoin
    unitlist.append(BTC); // Bitcoin
    unitlist.append(EUR); // Euro
    unitlist.append(USD); // United States Dollar
    unitlist.append(ARS); // Argentine Peso
    unitlist.append(AUD); // Australian dollar
    unitlist.append(BRL); // Brazilian real
    unitlist.append(CAD); // Canadian dollar
    unitlist.append(CHF); // Swiss franc
    unitlist.append(CLP); // Chilean peso
    unitlist.append(CZK); // Czech koruna
    unitlist.append(DKK); // Danish krone
    unitlist.append(GBP); // British pound
    unitlist.append(HKD); // Hong Kong dollar
    unitlist.append(HUF); // Hungarian forint
    unitlist.append(IDR); // Indonesian rupiah
    unitlist.append(ILS); // Israeli new shekel
    unitlist.append(INR); // Indian rupee
    unitlist.append(JPY); // Japanese Yen
    unitlist.append(KRW); // South Korean won
    unitlist.append(MXN); // Mexican peso
    unitlist.append(MYR); // Malaysian ringgit
    unitlist.append(NOK); // Norwegian krone
    unitlist.append(NZD); // New Zealand dollar
    unitlist.append(PHP); // Philippine peso
    unitlist.append(PKR); // Pakistani rupee
    unitlist.append(PLN); // Polish złoty
    unitlist.append(RUB); // Russian ruble
    unitlist.append(SEK); // Swedish krona
    unitlist.append(SGD); // Singapore dollar
    unitlist.append(THB); // Thai baht
    unitlist.append(TRYy); // Turkish lira
    unitlist.append(TWD); // New Taiwan dollar
    unitlist.append(ZAR); // South African rand

    return unitlist;
}

bool NavcoinUnits::valid(int unit)
{
    switch(unit)
    {
        case NAV:
        case BTC:
        case EUR:
        case USD:
        case ARS:
        case AUD:
        case BRL:
        case CAD:
        case CHF:
        case CLP:
        case CZK:
        case DKK:
        case GBP:
        case HKD:
        case HUF:
        case IDR:
        case ILS:
        case INR:
        case JPY:
        case KRW:
        case MXN:
        case MYR:
        case NOK:
        case NZD:
        case PHP:
        case PKR:
        case PLN:
        case RUB:
        case SEK:
        case SGD:
        case THB:
        case TRYy:
        case TWD:
        case ZAR:
            return true;
        default:
            return false;
    }
}

QString NavcoinUnits::name(int unit, bool fPrivate)
{
    switch(unit)
    {
        case NAV: if (fPrivate) return QString("xNAV"); else return QString("NAV");
        case BTC: return QString("BTC");
        case EUR: return QString("EUR");
        case USD: return QString("USD");
        case ARS: return QString("ARS");
        case AUD: return QString("AUD");
        case BRL: return QString("BRL");
        case CAD: return QString("CAD");
        case CHF: return QString("CHF");
        case CLP: return QString("CLP");
        case CZK: return QString("CZK");
        case DKK: return QString("DKK");
        case GBP: return QString("GBP");
        case HKD: return QString("HKD");
        case HUF: return QString("HUF");
        case IDR: return QString("IDR");
        case ILS: return QString("ILS");
        case INR: return QString("INR");
        case JPY: return QString("JPY");
        case KRW: return QString("KRW");
        case MXN: return QString("MXN");
        case MYR: return QString("MYR");
        case NOK: return QString("NOK");
        case NZD: return QString("NZD");
        case PHP: return QString("PHP");
        case PKR: return QString("PKR");
        case PLN: return QString("PLN");
        case RUB: return QString("RUB");
        case SEK: return QString("SEK");
        case SGD: return QString("SGD");
        case THB: return QString("THB");
        case TRYy: return QString("TRY");
        case TWD: return QString("TWD");
        case ZAR: return QString("ZAR");
        default: return QString("???");
    }
}

QString NavcoinUnits::description(int unit)
{
    switch(unit)
    {
        case NAV: return QString("Navcoin");
        case BTC: return QString("Bitcoin");
        case EUR: return QString("Euro");
        case USD: return QString("United States Dollar");
        case ARS: return QString("Argentine Peso");
        case AUD: return QString("Australian dollar");
        case BRL: return QString("Brazilian real");
        case CAD: return QString("Canadian dollar");
        case CHF: return QString("Swiss franc");
        case CLP: return QString("Chilean peso");
        case CZK: return QString("Czech koruna");
        case DKK: return QString("Danish krone");
        case GBP: return QString("British pound");
        case HKD: return QString("Hong Kong dollar");
        case HUF: return QString("Hungarian forint");
        case IDR: return QString("Indonesian rupiah");
        case ILS: return QString("Israeli new shekel");
        case INR: return QString("Indian rupee");
        case JPY: return QString("Japanese Yen");
        case KRW: return QString("South Korean won");
        case MXN: return QString("Mexican peso");
        case MYR: return QString("Malaysian ringgit");
        case NOK: return QString("Norwegian krone");
        case NZD: return QString("New Zealand dollar");
        case PHP: return QString("Philippine peso");
        case PKR: return QString("Pakistani rupee");
        case PLN: return QString("Polish złoty");
        case RUB: return QString("Russian ruble");
        case SEK: return QString("Swedish krona");
        case SGD: return QString("Singapore dollar");
        case THB: return QString("Thai baht");
        case TRYy: return QString("Turkish lira");
        case TWD: return QString("New Taiwan dollar");
        case ZAR: return QString("South African rand");
        default: return QString("???");
    }
}

qint64 NavcoinUnits::factor(int unit)
{

    QSettings settings;

    switch(unit)
    {
        case NAV: return 100000000;
        case BTC: return settings.value("btcFactor", 0).toFloat();
        case EUR: return settings.value("eurFactor", 0).toFloat();
        case USD: return settings.value("usdFactor", 0).toFloat();
        case ARS: return settings.value("arsFactor", 0).toFloat();
        case AUD: return settings.value("audFactor", 0).toFloat();
        case BRL: return settings.value("brlFactor", 0).toFloat();
        case CAD: return settings.value("cadFactor", 0).toFloat();
        case CHF: return settings.value("chfFactor", 0).toFloat();
        case CLP: return settings.value("clpFactor", 0).toFloat();
        case CZK: return settings.value("czkFactor", 0).toFloat();
        case DKK: return settings.value("dkkFactor", 0).toFloat();
        case GBP: return settings.value("gbpFactor", 0).toFloat();
        case HKD: return settings.value("hkdFactor", 0).toFloat();
        case HUF: return settings.value("hufFactor", 0).toFloat();
        case IDR: return settings.value("idrFactor", 0).toFloat();
        case ILS: return settings.value("ilsFactor", 0).toFloat();
        case INR: return settings.value("inrFactor", 0).toFloat();
        case JPY: return settings.value("jpyFactor", 0).toFloat();
        case KRW: return settings.value("krwFactor", 0).toFloat();
        case MXN: return settings.value("mxnFactor", 0).toFloat();
        case MYR: return settings.value("myrFactor", 0).toFloat();
        case NOK: return settings.value("nokFactor", 0).toFloat();
        case NZD: return settings.value("nzdFactor", 0).toFloat();
        case PHP: return settings.value("phpFactor", 0).toFloat();
        case PKR: return settings.value("pkrFactor", 0).toFloat();
        case PLN: return settings.value("plnFactor", 0).toFloat();
        case RUB: return settings.value("rubFactor", 0).toFloat();
        case SEK: return settings.value("sekFactor", 0).toFloat();
        case SGD: return settings.value("sgdFactor", 0).toFloat();
        case THB: return settings.value("thbFactor", 0).toFloat();
        case TRYy: return settings.value("tryFactor", 0).toFloat();
        case TWD: return settings.value("twdFactor", 0).toFloat();
        case ZAR: return settings.value("zarFactor", 0).toFloat();
        default:  return 100000000;
    }
}

int NavcoinUnits::decimals(int unit)
{
    switch(unit)
    {
        case NAV: return 8;
        case BTC: return 8;
        case EUR:
        case USD:
        case ARS:
        case AUD:
        case BRL:
        case CAD:
        case CHF:
        case CLP:
        case CZK:
        case DKK:
        case GBP:
        case HKD:
        case HUF:
        case IDR:
        case ILS:
        case INR:
        case JPY:
        case KRW:
        case MXN:
        case MYR:
        case NOK:
        case NZD:
        case PHP:
        case PKR:
        case PLN:
        case RUB:
        case SEK:
        case SGD:
        case THB:
        case TRYy:
        case TWD:
        case ZAR:
            return 6;
        default: return 0;
    }
}

QString NavcoinUnits::format(int unit, const CAmount& nIn, bool fPlus, SeparatorStyle separators, bool fPretty)
{
    // Note: not using straight sprintf here because we do NOT want
    // localized number formatting.
    if(!valid(unit))
        return QString(); // Refuse to format invalid unit
    qint64 n = (qint64)nIn;
    qint64 coin = factor(unit);
    int num_decimals = decimals(unit);
    qint64 n_abs = (n > 0 ? n : -n);
    qint64 quotient = 0;
    double quotientD = 0;
    qint64 remainder;

    // Check if we have a coin
    if (coin > 0) {
        quotient = n_abs / coin;
        quotientD = (double) n_abs / (double) coin;
    }

    std::ostringstream out;
    out << std::setprecision(num_decimals) << std::fixed
        << std::showpoint << quotientD;
    std::istringstream in(out.str());
    std::string wholePart;
    std::getline(in, wholePart, '.');
    in >> remainder;

    QString quotient_str = QString::number(quotient);
    QString remainder_str = QString::number(remainder).rightJustified(num_decimals, '0');

    // Use SI-style thin space separators as these are locale independent and can't be
    // confused with the decimal marker.
    QChar thin_sp(THIN_SP_CP);
    int q_size = quotient_str.size();
    if (separators == separatorAlways || (separators == separatorStandard && q_size > 4))
        for (int i = 3; i < q_size; i += 3)
            quotient_str.insert(q_size - i, thin_sp);

    if (n < 0)
        quotient_str.insert(0, '-');
    else if (fPlus && n > 0)
        quotient_str.insert(0, '+');

    // Check if we want auto adjust for decimals
    if (fPretty && (quotient >= 10 || quotientD == 0.0f)) {
        remainder_str.chop(4);
    }

    return quotient_str + QString(".") + remainder_str;
}

QString NavcoinUnits::pretty(int unit, const CAmount& nIn, bool fPlus, SeparatorStyle separators)
{
    return format(unit, nIn, fPlus, separators, true);
}

// NOTE: Using formatWithUnit in an HTML context risks wrapping
// quantities at the thousands separator. More subtly, it also results
// in a standard space rather than a thin space, due to a bug in Qt's
// XML whitespace canonicalisation
//
// Please take care to use formatHtmlWithUnit instead, when
// appropriate.
QString NavcoinUnits::formatWithUnit(int unit, const CAmount& amount, bool plussign, SeparatorStyle separators, bool fPrivate)
{
    return QString("%1 %2").arg(format(unit, amount, plussign, separators), name(unit, fPrivate));
}

QString NavcoinUnits::prettyWithUnit(int unit, const CAmount& amount, bool plussign, SeparatorStyle separators, bool fPrivate)
{
    return QString("%1 %2").arg(pretty(unit, amount, plussign, separators), name(unit, fPrivate));
}

QString NavcoinUnits::formatHtmlWithUnit(int unit, const CAmount& amount, bool plussign, SeparatorStyle separators, bool removeTrailing, bool fPrivate)
{
    QString str(formatWithUnit(unit, amount, plussign, separators, fPrivate));
    str.replace(QChar(THIN_SP_CP), QString(THIN_SP_HTML));
    return QString("<span style='white-space: nowrap;'>%1</span>").arg(str);
}


bool NavcoinUnits::parse(int unit, const QString &value, CAmount *val_out)
{
    if(!valid(unit) || value.isEmpty())
        return false; // Refuse to parse invalid unit or empty string
    int num_decimals = decimals(unit);

    // Ignore spaces and thin spaces when parsing
    QStringList parts = removeSpaces(value).split(".");

    if(parts.size() > 2)
    {
        return false; // More than one dot
    }
    QString whole = parts[0];
    QString decimals;

    if(parts.size() > 1)
    {
        decimals = parts[1];
    }
    if(decimals.size() > num_decimals)
    {
        return false; // Exceeds max precision
    }
    bool ok = false;
    QString str = whole + decimals.leftJustified(num_decimals, '0');

    if(str.size() > 18)
    {
        return false; // Longer numbers will exceed 63 bits
    }
    CAmount retvalue(str.toLongLong(&ok));
    if(val_out)
    {
        *val_out = retvalue;
    }
    return ok;
}

QString NavcoinUnits::getAmountColumnTitle(int unit)
{
    QString amountTitle = QObject::tr("Amount");
    if (NavcoinUnits::valid(unit))
    {
        amountTitle += " ("+NavcoinUnits::name(unit) + ")";
    }
    return amountTitle;
}

int NavcoinUnits::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return unitlist.size();
}

QVariant NavcoinUnits::data(const QModelIndex &index, int role) const
{
    int row = index.row();
    if(row >= 0 && row < unitlist.size())
    {
        Unit unit = unitlist.at(row);
        switch(role)
        {
            case Qt::EditRole:
            case Qt::DisplayRole:
                return QVariant(name(unit));
            case Qt::ToolTipRole:
                return QVariant(description(unit));
            case UnitRole:
                return QVariant(static_cast<int>(unit));
        }
    }
    return QVariant();
}

CAmount NavcoinUnits::maxMoney()
{
    return MAX_MONEY;
}
