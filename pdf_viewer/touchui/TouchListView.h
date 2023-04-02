#pragma once

#include <QWidget>
#include <QQuickWidget>
#include <QQmlContext>
#include <QQuickItem>
#include <QStringList>
#include <QStringListModel>
#include <QSortFilterProxyModel>
#include "mysortfilterproxymodel.h"


//class MySortFilterProxyModel;

class TouchListView : public QWidget{
    Q_OBJECT
public:
    TouchListView(QStringList elements, QWidget* parent=nullptr);
    void resizeEvent(QResizeEvent* resize_event) override;

public slots:
    void handleSelect(QString value, int index);

signals:
    void itemSelected(QString value, int index);

private:
    QQuickWidget* quick_widget = nullptr;
    QStringList items;
    QStringListModel model;
    MySortFilterProxyModel proxy_model;

};
