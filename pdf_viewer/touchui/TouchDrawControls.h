#pragma once

#include <QWidget>
#include <QQuickWidget>
#include <QQmlContext>
#include <QQuickItem>


class TouchDrawControls : public QWidget {
    Q_OBJECT
public:
    TouchDrawControls(float pen_size, char selected_symbol, QWidget* parent = nullptr);
    void resizeEvent(QResizeEvent* resize_event) override;
    void setDrawType(char type);
    void set_pen_size(float size);

public slots:
    void handleExitDrawMode();
    void handleEnablePenDrawMode();
    void handleDisablePenDrawMode();
    void handleChangeColor(int);
    void handleEraser();
    void handlePenSizeChanged(qreal size);

signals:
    void exitDrawModePressed();
    void enablePenDrawModePressed();
    void disablePenDrawModePressed();
    void changeColorPressed(int);
    void eraserPressed();
    void penSizeChanged(qreal size);

private:
    QQuickWidget* quick_widget = nullptr;

};
