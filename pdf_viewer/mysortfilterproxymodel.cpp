#include "mysortfilterproxymodel.h"
#include <string>

#include "rapidfuzz_amalgamated.hpp"

bool MySortFilterProxyModel::filter_accepts_row_column(int row, int col, const QModelIndex& source_parent) const {
	if (filterString.size() == 0 || filterString == "<NULL>") return true;

    if (scores.size() > 0) {
		return scores[row] > 50;
    }
	QModelIndex source_index = sourceModel()->index(row, col, source_parent);

	QString key = sourceModel()->data(source_index, filterRole()).toString();
	std::wstring s1 = filterString.toStdWString();
	std::wstring s2 = key.toStdWString();
	int score = static_cast<int>(rapidfuzz::fuzz::partial_ratio(s1, s2));
    return score > 50;

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
        update_scores();
        this->invalidate();
        this->sort(0);
    }
    else {
        this->setFilterFixedString(filterString);
    }
}

//void MySortFilterProxyModel::setFilterFixedString(const QString& filterString) {
//    if (FUZZY_SEARCHING) {
//        this->filterString = filterString;
//        this->setFilterFixedString(filterString);
//        sort(0);
//    }
//    else {
//        this->setFilterFixedString(filterString);
//    }
//}

bool MySortFilterProxyModel::lessThan(const QModelIndex& left,
    const QModelIndex& right) const
{
    if (is_fuzzy) {

        QString leftData = sourceModel()->data(left).toString();
        QString rightData = sourceModel()->data(right).toString();

        int left_index = left.row();
        int right_index = right.row();
        int left_score = scores[left_index];
        int right_score = scores[right_index];
        //int left_score = static_cast<int>(rapidfuzz::fuzz::partial_ratio(filterString.toStdWString(), leftData.toStdWString()));
        //int right_score = static_cast<int>(rapidfuzz::fuzz::partial_ratio(filterString.toStdWString(), rightData.toStdWString()));

        return left_score > right_score;
    }
    else {
        return QSortFilterProxyModel::lessThan(left, right);
    }
}

 MySortFilterProxyModel::MySortFilterProxyModel(bool fuzzy) {
     is_fuzzy = fuzzy;

     if (fuzzy) {
         setDynamicSortFilter(true);
     }
}

 void MySortFilterProxyModel::update_scores() {
     scores.clear();

     int n_rows = sourceModel()->rowCount();
     int n_cols = sourceModel()->columnCount();
     int filter_column_index = filterKeyColumn();
     std::wstring filter_wstring = filterString.toStdWString();

     if ((n_cols == 1) || (filter_column_index >= 0)) {
		 for (int i = 0; i < n_rows; i++) {
			 QString row_data = sourceModel()->data(sourceModel()->index(i, filter_column_index)).toString();
             int score = static_cast<int>(rapidfuzz::fuzz::partial_ratio(filter_wstring, row_data.toStdWString()));
             scores.push_back(score);
		 }
     }
     else {
		 for (int i = 0; i < n_rows; i++) {
             int score = -1;

             for (int col_index = 0; col_index < n_cols; col_index++) {
				 QString rowcol_data = sourceModel()->data(sourceModel()->index(i, col_index)).toString();
				 int col_score = static_cast<int>(rapidfuzz::fuzz::partial_ratio(filter_wstring, rowcol_data.toStdWString()));
                 if (col_score > score) score = col_score;
             }

             scores.push_back(score);
		 }
     }

 }
