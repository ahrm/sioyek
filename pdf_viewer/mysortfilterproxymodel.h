#pragma once
#include <QString>
#include <QSortFilterProxyModel>

class MySortFilterProxyModel : public QSortFilterProxyModel {
    QString filterString;
public:
    MySortFilterProxyModel();
    bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const;
    void setFilterCustom(QString filterString);
    bool lessThan(const QModelIndex& left, const QModelIndex& right) const;

};
