#pragma once

#include <QWidget>
#include <QQuickWidget>
#include <QQmlContext>
#include <QQuickItem>


class TouchHighlightButtons : public QWidget{
    Q_OBJECT
public:
    TouchHighlightButtons(char selected_symbol, QWidget* parent=nullptr);
    void resizeEvent(QResizeEvent* resize_event) override;
    void setHighlightType(char type);

public slots:
    void handleDelete();
    void handleChangeColor(int);

signals:
    void deletePressed();
    void changeColorPressed(int);

private:
    QQuickWidget* quick_widget = nullptr;

};
