#pragma once

#include <QWidget>
#include <QQuickWidget>
#include <QQmlContext>
#include <QQuickItem>
#include <QStringList>
#include <QStringListModel>
#include <QSortFilterProxyModel>
#include "mysortfilterproxymodel.h"
#include "config.h"


//class MySortFilterProxyModel;
class MainWidget;

class TouchConfigMenu : public QWidget{
    Q_OBJECT
public:
    //TouchConfigMenu(std::vector<Config>* configs, QWidget* parent = nullptr);
    TouchConfigMenu(MainWidget* main_widget);
    void resizeEvent(QResizeEvent* resize_event) override;

public slots:
    void handleColor3ConfigChanged(QString config_name, qreal r, qreal g, qreal b);
    void handleColor4ConfigChanged(QString config_name, qreal r, qreal g, qreal b, qreal a);
    void handleBoolConfigChanged(QString config_name, bool new_value);
    void handleFloatConfigChanged(QString config_name, qreal new_value);
    void handleIntConfigChanged(QString config_name, int new_value);
    void handleTextConfigChanged(QString config_name, QString new_value);
    void handleSetConfigPressed(QString config_name);
    void handleSaveButtonClicked();
    //void handlePressAndHold(QString value, int index);
    //void handleDelete(QString value, int index);

signals:
    //void itemSelected(QString value, int index);
    //void itemPressAndHold(QString value, int index);
    //void itemDeleted(QString value, int index);

private:
    QQuickWidget* quick_widget = nullptr;
    ConfigManager* config_manager;
    //QStringList items;
    //QStringListModel model;
    MainWidget* main_widget;
    ConfigModel config_model;
    MySortFilterProxyModel* proxy_model;

};
