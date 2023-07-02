#pragma once

#include <QWidget>
#include <QQuickWidget>
#include <QQmlContext>
#include <QQuickItem>


class TouchSlider : public QWidget {
    Q_OBJECT
public:
    TouchSlider(int from, int to, int initial_value, QWidget* parent = nullptr);
    void resizeEvent(QResizeEvent* resize_event) override;

public slots:
    void handleSelect(int item);
    void handleCancel();

signals:
    void itemSelected(int);
    void canceled();

private:
    QQuickWidget* quick_widget = nullptr;

};
