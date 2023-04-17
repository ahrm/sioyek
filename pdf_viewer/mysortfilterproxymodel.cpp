#include "mysortfilterproxymodel.h"
#include <string>

#include "rapidfuzz_amalgamated.hpp"

extern bool FUZZY_SEARCHING;

bool MySortFilterProxyModel::filterAcceptsRow(int source_row,
    const QModelIndex& source_parent) const
{
    if (FUZZY_SEARCHING) {

        QModelIndex source_index = sourceModel()->index(source_row, this->filterKeyColumn(), source_parent);
        if (source_index.isValid())
        {
            // check current index itself :

            QString key = sourceModel()->data(source_index, filterRole()).toString();
            if (filterString.size() == 0) return true;
            std::wstring s1 = filterString.toStdWString();
            std::wstring s2 = key.toStdWString();
            int score = static_cast<int>(rapidfuzz::fuzz::partial_ratio(s1, s2));

            return score > 50;
        }
        else {
            return false;
        }
    }
    else {
        return QSortFilterProxyModel::filterAcceptsRow(source_row, source_parent);
    }
}

void MySortFilterProxyModel::setFilterCustom(const QString& filterString) {
    if (FUZZY_SEARCHING) {
        this->filterString = filterString;
        this->setFilterFixedString(filterString);
        sort(0);
    }
    else {
        this->setFilterFixedString(filterString);
    }
}

bool MySortFilterProxyModel::lessThan(const QModelIndex& left,
    const QModelIndex& right) const
{
    if (FUZZY_SEARCHING) {

        QString leftData = sourceModel()->data(left).toString();
        QString rightData = sourceModel()->data(right).toString();

        int left_score = static_cast<int>(rapidfuzz::fuzz::partial_ratio(filterString.toStdWString(), leftData.toStdWString()));
        int right_score = static_cast<int>(rapidfuzz::fuzz::partial_ratio(filterString.toStdWString(), rightData.toStdWString()));
        return left_score > right_score;
    }
    else {
        return QSortFilterProxyModel::lessThan(left, right);
    }
}
 MySortFilterProxyModel::MySortFilterProxyModel() {
     if (FUZZY_SEARCHING) {
         setDynamicSortFilter(true);
     }
}
