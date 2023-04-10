#pragma once

#include <QWidget>
#include <QQuickWidget>
#include <QQmlContext>
#include <QQuickItem>


class TouchTextEdit : public QWidget{
    Q_OBJECT
public:
    TouchTextEdit(QString name, QString initial_value, QWidget* parent=nullptr);
    void resizeEvent(QResizeEvent* resize_event) override;

public slots:
    void handleConfirm(QString text);
    void handleCancel();

signals:
    void confirmed(QString text);
    void cancelled();

private:
    QQuickWidget* quick_widget = nullptr;

};
