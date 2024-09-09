#include "mysortfilterproxymodel.h"
#include <string>

#include "rapidfuzz_amalgamated.hpp"

bool MySortFilterProxyModel::filter_accepts_row_column(int row, int col, const QModelIndex& source_parent) const {
    if (filterString.size() == 0 || filterString == "<NULL>") return true;

    ensure_scores();
    if (is_tree) {
        QModelIndex current_index = sourceModel()->index(row, col, source_parent);
        return scores[index_map[current_index]];
    }
    else {
        return scores[row] > 50;
    }
}

bool MySortFilterProxyModel::filterAcceptsRow(int source_row,
    const QModelIndex& source_parent) const
{
    if (is_fuzzy) {

        int key_column = this->filterKeyColumn();

        if (key_column == -1) {
            int num_columns = sourceModel()->columnCount(source_parent);
            for (int i = 0; i < num_columns; i++) {
                if (filter_accepts_row_column(source_row, i, source_parent)) {
                    return true;
                }
            }

            return false;
        }
        else {
            return filter_accepts_row_column(source_row, key_column, source_parent);
            
        }
    }
    else {
        //QModelIndex source_index = sourceModel()->index(source_row, this->filterKeyColumn(), source_parent);
        //std::string key = sourceModel()->data(source_index, filterRole()).toString().toStdString();
        return QSortFilterProxyModel::filterAcceptsRow(source_row, source_parent);
    }
}


void MySortFilterProxyModel::setFilterCustom(const QString& filterString) {
    if (is_fuzzy) {
        this->filterString = filterString;
        this->setFilterFixedString(filterString);
        ensure_scores();
        this->invalidate();
        this->sort(0);
    }
    else {
        this->setFilterFixedString(filterString);
    }
}

bool MySortFilterProxyModel::lessThan(const QModelIndex& left,
    const QModelIndex& right) const
{
    if (is_fuzzy) {
        int left_score = scores[index_map[left]];
        int right_score = scores[index_map[right]];
        return left_score > right_score;
    }
    else {
        return QSortFilterProxyModel::lessThan(left, right);
    }
}

MySortFilterProxyModel::MySortFilterProxyModel(bool fuzzy, bool tree) {
    is_fuzzy = fuzzy;
    is_tree = tree;

    slab = fzf_make_default_slab();
    if (fuzzy) {
        setDynamicSortFilter(true);
    }
}

MySortFilterProxyModel::~MySortFilterProxyModel() {
    fzf_free_slab(slab);
}

int MySortFilterProxyModel::compute_score(QString filter_string, QString item_string) const{
    return static_cast<int>(rapidfuzz::fuzz::partial_ratio(filter_string.toStdWString(), item_string.toStdWString()));
}

int MySortFilterProxyModel::compute_score(fzf_pattern_t* pattern, QString item_string) const{
    std::string item_str = item_string.toStdString();
    return fzf_get_score(item_str.c_str(), pattern, slab);
}


int MySortFilterProxyModel::update_scores_for_index(fzf_pattern_t* pattern, const QModelIndex& index, int col) const{
    int n_children = sourceModel()->rowCount(index);

    int max_child_score = 0;

    for (int i = 0; i < n_children; i++) {
        QModelIndex child_index = sourceModel()->index(i, col, index);
        int child_score = update_scores_for_index(pattern, child_index, col);
        if (child_score > max_child_score) {
            max_child_score = child_score;
        }

    }

    int score = compute_score(pattern, sourceModel()->data(index).toString());
    scores.push_back(score);
    index_map[index] = scores.size() - 1;
    return std::max(score, max_child_score);
}


void MySortFilterProxyModel::ensure_scores() const {
    if ((scores.size() == 0) || (filterString != last_update_string)) {
        update_scores();
        last_update_string = filterString;
    }
}

void MySortFilterProxyModel::update_scores() const{
    scores.clear();

    int n_rows = sourceModel()->rowCount();
    int n_cols = sourceModel()->columnCount();
    int filter_column_index = filterKeyColumn();
    std::string filter_str = filterString.toStdString();
    fzf_pattern_t* pattern = fzf_parse_pattern(CaseSmart, false, (char*)filter_str.c_str(), true);

    if (is_tree) {
        for (int i = 0; i < n_rows; i++) {
            update_scores_for_index(pattern, sourceModel()->index(i, 0), filter_column_index);
        }
    }
    else {

        if ((n_cols == 1) || (filter_column_index >= 0)) {
            for (int i = 0; i < n_rows; i++) {
                QModelIndex current_index = sourceModel()->index(i, filter_column_index);

                QString row_data = sourceModel()->data(current_index).toString();
                int score = compute_score(filterString, row_data);
                scores.push_back(score);
                index_map[current_index] = scores.size() - 1;

            }
        }
        else {
            for (int i = 0; i < n_rows; i++) {
                int score = -1;

                for (int col_index = 0; col_index < n_cols; col_index++) {
                    QString rowcol_data = sourceModel()->data(sourceModel()->index(i, col_index)).toString();
                    int col_score = compute_score(filterString, rowcol_data);
                    if (col_score > score) score = col_score;
                }

                scores.push_back(score);
                index_map[sourceModel()->index(i, 0)] = scores.size() - 1;
            }
        }
    }

    fzf_free_pattern(pattern);

}
