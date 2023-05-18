#pragma once

#include <QWidget>
#include <QQuickWidget>
#include <QQmlContext>
#include <QQuickItem>


class TouchCheckbox : public QWidget{
    Q_OBJECT
public:
    TouchCheckbox(QString name, bool initial_value, QWidget* parent=nullptr);
    void resizeEvent(QResizeEvent* resize_event) override;

public slots:
    void handleSelect(bool item);
    void handleCancel();

signals:
    void itemSelected(bool);
    void canceled();

private:
    QQuickWidget* quick_widget = nullptr;

};
