#pragma once

#include <QWidget>
#include <QQuickWidget>
#include <QQmlContext>
#include <QQuickItem>


class TouchHighlightButtons : public QWidget{
    Q_OBJECT
public:
    TouchHighlightButtons(QWidget* parent=nullptr);
    void resizeEvent(QResizeEvent* resize_event) override;

public slots:
    void handleDelete();
    void handleChangeColor(int);

signals:
    void deletePressed();
    void changeColorPressed(int);

private:
    QQuickWidget* quick_widget = nullptr;

};
