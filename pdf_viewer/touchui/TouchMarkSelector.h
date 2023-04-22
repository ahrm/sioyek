#pragma once

#include <QWidget>
#include <QQuickWidget>
#include <QQmlContext>
#include <QQuickItem>


class TouchMarkSelector : public QWidget{
    Q_OBJECT
public:
    TouchMarkSelector(QWidget* parent=nullptr);
    void resizeEvent(QResizeEvent* resize_event) override;

public slots:
    void handleMarkSelected(QString mark);

signals:
    void onMarkSelected(QString mark);

private:
    QQuickWidget* quick_widget = nullptr;

};
