#pragma once

#include <QWidget>
#include <QQuickWidget>
#include <QQmlContext>
#include <QQuickItem>


class TouchDrawControls : public QWidget {
    Q_OBJECT
public:
    bool is_in_scratchpad = false;

    TouchDrawControls(float pen_size, char selected_symbol, QWidget* parent = nullptr);
    void resizeEvent(QResizeEvent* resize_event) override;
    void setDrawType(char type);
    void set_pen_size(float size);
    void set_scratchpad_mode(bool mode);

public slots:
    void handleExitDrawMode();
    void handleEnablePenDrawMode();
    void handleDisablePenDrawMode();
    void handleChangeColor(int);
    void handleEraser();
    void handleScreenshot();
    void handleToggleScratchpad();
    void handlePenSizeChanged(qreal size);
    void handleSaveScratchpad();
    void handleLoadScratchpad();
    void handleMove();

signals:
    void exitDrawModePressed();
    void enablePenDrawModePressed();
    void disablePenDrawModePressed();
    void changeColorPressed(int);
    void eraserPressed();
    void penSizeChanged(qreal size);
    void screenshotPressed();
    void toggleScratchpadPressed();
    void saveScratchpadPressed();
    void loadScratchpadPressed();
    void movePressed();

private:
    QQuickWidget* quick_widget = nullptr;

};
