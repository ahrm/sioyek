#pragma once

#include <QWidget>
#include <QQuickWidget>
#include <QQmlContext>
#include <QQuickItem>


class MainWidget;

class TouchMacroEditor : public QWidget{
    Q_OBJECT
public:
    TouchMacroEditor(std::string macro, QWidget* parent, MainWidget* main_widget);
    void resizeEvent(QResizeEvent* resize_event) override;

public slots:
    void handleConfirm(QString macro);
    void handleCancel();

signals:
    void macroConfirmed(std::string);
    void canceled();

private:
    QQuickWidget* quick_widget = nullptr;

};
