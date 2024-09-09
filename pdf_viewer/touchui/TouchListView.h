#pragma once

#include <QWidget>
#include <QQuickWidget>
#include <QQmlContext>
#include <QQuickItem>
#include <QStringList>
#include <QStringListModel>
#include <QSortFilterProxyModel>

class MySortFilterProxyModel;

class TouchListView : public QWidget {
    Q_OBJECT
public:
    QAbstractItemModel* model;
    MySortFilterProxyModel* proxy_model;
    QQuickWidget* quick_widget = nullptr;
    TouchListView(bool is_fuzzy, QStringList elements, int selected_index, QWidget* parent = nullptr, bool deletable = false);
    TouchListView(bool is_fuzzy, QAbstractItemModel* elements, int selected_index, QWidget* parent = nullptr, bool deletable = false, bool move = true, bool is_tree = false);
    void initialize(int selected_index, bool deletable, bool is_tree = false);
    void resizeEvent(QResizeEvent* resize_event) override;
    void set_keyboard_focus();
    void keyPressEvent(QKeyEvent* kevent);
    void update_model();

public slots:
    void handleSelect(QString value, int index);
    void handlePressAndHold(QString value, int index);
    void handleDelete(QString value, int index);

signals:
    void itemSelected(QString value, int index);
    void itemPressAndHold(QString value, int index);
    void itemDeleted(QString value, int index);

};
