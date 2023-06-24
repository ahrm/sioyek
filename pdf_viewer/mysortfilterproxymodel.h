#pragma once
#include <QString>
#include <QSortFilterProxyModel>

class MySortFilterProxyModel : public QSortFilterProxyModel {
    Q_OBJECT

public:
    QString filterString;
    std::vector<int> scores;
    bool is_fuzzy = false;

    MySortFilterProxyModel(bool fuzzy);
    bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const;
    bool filter_accepts_row_column(int row, int col, const QModelIndex& source_parent) const;
    Q_INVOKABLE void setFilterCustom(const QString& filterString);

    //void setFilterFixedString(const QString &pattern) override;
    bool lessThan(const QModelIndex& left, const QModelIndex& right) const;

    void update_scores();

};
