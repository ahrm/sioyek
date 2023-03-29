#pragma once

#include <QWidget>
#include <QQuickWidget>
#include <QQmlContext>
#include <QQuickItem>


class TouchRectangleSelectUI : public QWidget{
    Q_OBJECT
public:
    TouchRectangleSelectUI(bool initial_enabled=false, int initial_x=0, int initial_y=0, int initial_width=100, int intitial_height=100, QWidget* parent=nullptr);
    void resizeEvent(QResizeEvent* resize_event) override;

public slots:
    void handleRectangleSelected(bool, int, int, int, int);

signals:
    void rectangleSelected(bool, int, int, int, int);

private:
    QQuickWidget* quick_widget = nullptr;

};
