#pragma once

#include <QWidget>
#include <QQuickWidget>
#include <QQmlContext>
#include <QQuickItem>


class TouchRangeSelectUI : public QWidget{
    Q_OBJECT
public:
    TouchRangeSelectUI(float initial_top, float initial_bottom, QWidget* parent=nullptr);
    void resizeEvent(QResizeEvent* resize_event) override;

public slots:
    void handleRangeSelected(qreal, qreal);
    void handleRangeCanceled();

signals:
    void rangeSelected(qreal, qreal);
    void rangeCanceled();

private:
    QQuickWidget* quick_widget = nullptr;

};
