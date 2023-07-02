#pragma once

#include <QWidget>
#include <QQuickWidget>
#include <QQmlContext>
#include <QQuickItem>


class TouchDeleteButton : public QWidget {
    Q_OBJECT
public:
    TouchDeleteButton(QWidget* parent = nullptr);
    void resizeEvent(QResizeEvent* resize_event) override;

public slots:
    void handleDelete();

signals:
    void deletePressed();

private:
    QQuickWidget* quick_widget = nullptr;

};
