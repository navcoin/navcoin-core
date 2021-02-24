// Copyright (c) 2011-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/transactionfilterproxy.h>

#include <qt/transactiontablemodel.h>
#include <qt/transactionrecord.h>

#include <cstdlib>

// Earliest date that can be represented (far in the past)
const QDateTime TransactionFilterProxy::MIN_DATE = QDateTime::fromTime_t(0);
// Last date that can be represented (far in the future)
const QDateTime TransactionFilterProxy::MAX_DATE = QDateTime::fromTime_t(0xFFFFFFFF);

TransactionFilterProxy::TransactionFilterProxy(QObject *parent) : QSortFilterProxyModel(parent) { }

bool TransactionFilterProxy::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    QModelIndex index = sourceModel()->index(sourceRow, 0, sourceParent);

    if (!showInactive && TransactionStatus::Conflicted == index.data(TransactionTableModel::StatusRole).toInt())
        return false;

    if (typeFilter != ALL_TYPES && !(TYPE(index.data(TransactionTableModel::TypeRole).toInt()) & typeFilter))
        return false;

    if (watchOnlyFilter != WatchOnlyFilter_All && (watchOnlyFilter == WatchOnlyFilter_No) == index.data(TransactionTableModel::WatchonlyRole).toBool())
        return false;

    if (minAmount != 0 && llabs(index.data(TransactionTableModel::AmountRole).toLongLong()) < minAmount)
        return false;

    if (dateFrom != MIN_DATE || dateTo != MAX_DATE) {
        QDateTime datetime = index.data(TransactionTableModel::DateRole).toDateTime();
        if (datetime < dateFrom || datetime > dateTo)
            return false;
    }

    if (searchString != "") {
        QString address = index.data(TransactionTableModel::AddressRole).toString();
        QString label = index.data(TransactionTableModel::LabelRole).toString();
        QString txid = index.data(TransactionTableModel::TxHashRole).toString();
        if (!address.contains(searchString, Qt::CaseInsensitive) &&
                !  label.contains(searchString, Qt::CaseInsensitive) &&
                !   txid.contains(searchString, Qt::CaseInsensitive)) {
            return false;
        }
    }

    return true;
}

void TransactionFilterProxy::setDateRange(const QDateTime &from, const QDateTime &to)
{
    this->dateFrom = from;
    this->dateTo = to;
    invalidateFilter();
}

void TransactionFilterProxy::setSearchString(const QString &search_string)
{
    if (searchString == search_string) return;
    searchString = search_string;
    invalidateFilter();
}

void TransactionFilterProxy::setTypeFilter(quint32 modes)
{
    this->typeFilter = modes;
    invalidateFilter();
}

void TransactionFilterProxy::setMinAmount(const CAmount& minimum)
{
    this->minAmount = minimum;
    invalidateFilter();
}

void TransactionFilterProxy::setWatchOnlyFilter(WatchOnlyFilter filter)
{
    this->watchOnlyFilter = filter;
    invalidateFilter();
}

void TransactionFilterProxy::setLimit(int limit)
{
    this->limitRows = limit;
}

void TransactionFilterProxy::setShowInactive(bool _showInactive)
{
    this->showInactive = _showInactive;
    invalidateFilter();
}

int TransactionFilterProxy::rowCount(const QModelIndex &parent) const
{
    if(limitRows != -1)
        return std::min(QSortFilterProxyModel::rowCount(parent), limitRows);

    return QSortFilterProxyModel::rowCount(parent);
}
