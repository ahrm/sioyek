#pragma once

#include <QWidget>
#include <QPaintEvent>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QRect>
#include <QString>
#include <string>
#include <vector>

class MainWidget;

class TabBarWidget : public QWidget {
    Q_OBJECT
public:
    TabBarWidget(MainWidget* parent);
    void update_tabs();

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void leaveEvent(QEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;

private:
    struct TabInfo {
        QRect rect;
        QRect close_rect;
        std::wstring path;
        QString label;
        bool is_active;
    };

    MainWidget* main_widget;
    std::vector<TabInfo> tab_infos;
    int hovered_index = -1;
    bool hover_on_close = false;
    int scroll_offset = 0;

    void recompute_layout(int w, int h);
    int tab_at(QPoint pos);
};
