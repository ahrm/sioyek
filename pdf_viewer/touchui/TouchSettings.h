#pragma once

#include <QWidget>
#include <QQuickWidget>
#include <QQmlContext>
#include <QQuickItem>

class MainWidget;
class UIRect;

class TouchSettings : public QWidget{
    Q_OBJECT
public:
    TouchSettings(MainWidget* parent=nullptr);
    void resizeEvent(QResizeEvent* resize_event) override;

public slots:
    void handleLightApplicationBackground();
    void handleDarkApplicationBackground();
    void handleCustomApplicationBackground();
    void handleCustomPageText();
    void handleCustomPageBackground();
    void handleRulerModeBounds();
    void handleRulerModeColor();
    void handleRulerNext();
    void handleRulerPrev();
    void handleBack();
    void handleForward();
    void handleAllConfigs();
    void handleRestoreDefaults();
    //void handleGotoPage();
    //void handleToc();

signals:
    //void lightApplicationBackgroundClicked();
    //void gotoPageClicked();

private:
    void show_dialog_for_color_n(int n, float* color_location);
    void show_dialog_for_rect(UIRect* location);
    QQuickWidget* quick_widget = nullptr;
    MainWidget* main_widget;

};
