#pragma once

#include <QWidget>
#include <QQuickWidget>
#include <QQmlContext>
#include <QQuickItem>


class TouchRectangleSelectUI : public QWidget{
    Q_OBJECT
public:
    TouchRectangleSelectUI(bool initial_enabled, float initial_x, float initial_y, float initial_width, float intitial_height, QWidget* parent=nullptr);
    void resizeEvent(QResizeEvent* resize_event) override;

public slots:
    void handleRectangleSelected(bool, qreal, qreal, qreal, qreal);

signals:
    void rectangleSelected(bool, qreal, qreal, qreal, qreal);

private:
    QQuickWidget* quick_widget = nullptr;

};
