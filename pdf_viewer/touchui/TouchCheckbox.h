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

signals:
    void itemSelected(bool);

private:
    QQuickWidget* quick_widget = nullptr;

};
