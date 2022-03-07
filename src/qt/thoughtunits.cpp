// Copyright (c) 2011-2015 The Bitcoin Core developers
// Copyright (c) 2014-2020 The Dash Core developers
// Copyright (c) 2018-2022 Thought Network Ltd
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/thoughtunits.h>
#include <chainparams.h>
#include <primitives/transaction.h>

#include <QSettings>
#include <QStringList>

ThoughtUnits::ThoughtUnits(QObject *parent):
        QAbstractListModel(parent),
        unitlist(availableUnits())
{
}

QList<ThoughtUnits::Unit> ThoughtUnits::availableUnits()
{
    QList<ThoughtUnits::Unit> unitlist;
    unitlist.append(THT);
    unitlist.append(mTHT);
    unitlist.append(uTHT);
    unitlist.append(notions);
    return unitlist;
}

bool ThoughtUnits::valid(int unit)
{
    switch(unit)
    {
    case THT:
    case mTHT:
    case uTHT:
    case notions:
        return true;
    default:
        return false;
    }
}

QString ThoughtUnits::name(int unit)
{
    if(Params().NetworkIDString() == CBaseChainParams::MAIN)
    {
        switch(unit)
        {
            case THT: return QString("THT");
            case mTHT: return QString("mTHT");
            case uTHT: return QString::fromUtf8("μTHT");
            case notions: return QString("notions");
            default: return QString("???");
        }
    }
    else
    {
        switch(unit)
        {
            case THT: return QString("tTHT");
            case mTHT: return QString("mtTHT");
            case uTHT: return QString::fromUtf8("μtTHT");
            case notions: return QString("tnotions");
            default: return QString("???");
        }
    }
}

QString ThoughtUnits::description(int unit)
{
    if(Params().NetworkIDString() == CBaseChainParams::MAIN)
    {
        switch(unit)
        {
            case THT: return QString("Thought");
            case mTHT: return QString("Milli-Thought (1 / 1" THIN_SP_UTF8 "000)");
            case uTHT: return QString("Micro-Thought (1 / 1" THIN_SP_UTF8 "000" THIN_SP_UTF8 "000)");
            case notions: return QString("Ten Nano-Thought (1 / 100" THIN_SP_UTF8 "000" THIN_SP_UTF8 "000)");
            default: return QString("???");
        }
    }
    else
    {
        switch(unit)
        {
            case THT: return QString("TestThought");
            case mTHT: return QString("Milli-TestThought (1 / 1" THIN_SP_UTF8 "000)");
            case uTHT: return QString("Micro-TestThought (1 / 1" THIN_SP_UTF8 "000" THIN_SP_UTF8 "000)");
            case notions: return QString("Ten Nano-TestThought (1 / 100" THIN_SP_UTF8 "000" THIN_SP_UTF8 "000)");
            default: return QString("???");
        }
    }
}

qint64 ThoughtUnits::factor(int unit)
{
    switch(unit)
    {
    case THT:  return 100000000;
    case mTHT: return 100000;
    case uTHT: return 100;
    case notions: return 1;
    default:   return 100000000;
    }
}

int ThoughtUnits::decimals(int unit)
{
    switch(unit)
    {
    case THT: return 8;
    case mTHT: return 5;
    case uTHT: return 2;
    case notions: return 0;
    default: return 0;
    }
}

QString ThoughtUnits::format(int unit, const CAmount& nIn, bool fPlus, SeparatorStyle separators)
{
    // Note: not using straight sprintf here because we do NOT want
    // localized number formatting.
    if(!valid(unit))
        return QString(); // Refuse to format invalid unit
    qint64 n = (qint64)nIn;
    qint64 coin = factor(unit);
    int num_decimals = decimals(unit);
    qint64 n_abs = (n > 0 ? n : -n);
    qint64 quotient = n_abs / coin;
    qint64 remainder = n_abs % coin;
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

    if (num_decimals <= 0)
        return quotient_str;

    return quotient_str + QString(".") + remainder_str;
}


// NOTE: Using formatWithUnit in an HTML context risks wrapping
// quantities at the thousands separator. More subtly, it also results
// in a standard space rather than a thin space, due to a bug in Qt's
// XML whitespace canonicalisation
//
// Please take care to use formatHtmlWithUnit instead, when
// appropriate.

QString ThoughtUnits::formatWithUnit(int unit, const CAmount& amount, bool plussign, SeparatorStyle separators)
{
    return format(unit, amount, plussign, separators) + QString(" ") + name(unit);
}

QString ThoughtUnits::formatHtmlWithUnit(int unit, const CAmount& amount, bool plussign, SeparatorStyle separators)
{
    QString str(formatWithUnit(unit, amount, plussign, separators));
    str.replace(QChar(THIN_SP_CP), QString(THIN_SP_HTML));
    return QString("<span style='white-space: nowrap;'>%1</span>").arg(str);
}

QString ThoughtUnits::floorWithUnit(int unit, const CAmount& amount, bool plussign, SeparatorStyle separators)
{
    QSettings settings;
    int digits = settings.value("digits").toInt();

    QString result = format(unit, amount, plussign, separators);
    if(decimals(unit) > digits) result.chop(decimals(unit) - digits);

    return result + QString(" ") + name(unit);
}

QString ThoughtUnits::floorHtmlWithUnit(int unit, const CAmount& amount, bool plussign, SeparatorStyle separators)
{
    QString str(floorWithUnit(unit, amount, plussign, separators));
    str.replace(QChar(THIN_SP_CP), QString(THIN_SP_HTML));
    return QString("<span style='white-space: nowrap;'>%1</span>").arg(str);
}

bool ThoughtUnits::parse(int unit, const QString &value, CAmount *val_out)
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

QString ThoughtUnits::getAmountColumnTitle(int unit)
{
    QString amountTitle = QObject::tr("Amount");
    if (ThoughtUnits::valid(unit))
    {
        amountTitle += " ("+ThoughtUnits::name(unit) + ")";
    }
    return amountTitle;
}

int ThoughtUnits::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return unitlist.size();
}

QVariant ThoughtUnits::data(const QModelIndex &index, int role) const
{
    return data(index.row(), role);
}

QVariant ThoughtUnits::data(const int &row, int role) const
{
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

CAmount ThoughtUnits::maxMoney()
{
    return MAX_MONEY;
}
