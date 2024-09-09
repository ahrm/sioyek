#pragma once

#include <QWidget>
#include <QQuickWidget>
#include <QQmlContext>
#include <QQuickItem>


class TouchPageSelector : public QWidget {
    Q_OBJECT
public:
    TouchPageSelector(int from, int to, int initial_value, QWidget* parent = nullptr);
    void resizeEvent(QResizeEvent* resize_event) override;

public slots:
    void handleSelect(int page);

signals:
    void pageSelected(int);

private:
    QQuickWidget* quick_widget = nullptr;

};
