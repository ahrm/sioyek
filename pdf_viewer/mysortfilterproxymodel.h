#pragma once
#include <QString>
#include <qsortfilterproxymodel.h>
#include <map>
#include <qmap.h>

extern "C" {
    #include "fzf/fzf.h"
}

class MySortFilterProxyModel : public QSortFilterProxyModel {
    Q_OBJECT

public:
    QString filterString;
    mutable std::vector<int> scores;
    mutable QString last_update_string;
    bool is_fuzzy = false;
    fzf_slab_t* slab;
    mutable QMap<QModelIndex, int> index_map;
    bool is_tree = false;

    MySortFilterProxyModel(bool fuzzy, bool is_tree);
    ~MySortFilterProxyModel();
    bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const;
    bool filter_accepts_row_column(int row, int col, const QModelIndex& source_parent) const;
    Q_INVOKABLE void setFilterCustom(const QString& filterString);

    //void setFilterFixedString(const QString &pattern) override;
    bool lessThan(const QModelIndex& left, const QModelIndex& right) const;

    int compute_score(QString filter_string, QString item_string) const;
    int compute_score(fzf_pattern_t* pattern, QString item_string) const;
    int update_scores_for_index(fzf_pattern_t* pattern, const QModelIndex& index, int col) const;
    void ensure_scores() const;
    void update_scores() const;

};
