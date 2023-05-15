#pragma once

#include <QWidget>
#include <QQuickWidget>
#include <QQmlContext>
#include <QQuickItem>


class TouchDrawControls : public QWidget{
    Q_OBJECT
public:
    TouchDrawControls(char selected_symbol, QWidget* parent=nullptr);
    void resizeEvent(QResizeEvent* resize_event) override;
    void setDrawType(char type);

public slots:
    void handleExitDrawMode();
    void handleEnablePenDrawMode();
    void handleDisablePenDrawMode();
    void handleChangeColor(int);
    void handleEraser();

signals:
    void exitDrawModePressed();
    void enablePenDrawModePressed();
    void disablePenDrawModePressed();
    void changeColorPressed(int);
    void eraserPressed();

private:
    QQuickWidget* quick_widget = nullptr;

};
