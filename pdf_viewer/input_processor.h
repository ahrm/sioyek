#pragma once
#include <qevent.h>
#include "coordinates.h"

class MainWidget;

class InputProcessor {
public:
    InputProcessor(MainWidget* main_widget);
    ~InputProcessor();

    void key_event(bool released, QKeyEvent* kevent);
    void handle_left_click(WindowPos click_pos, bool down, bool is_shift_pressed, bool is_control_pressed, bool is_alt_pressed);
    void handle_right_click(WindowPos click_pos, bool down, bool is_shift_pressed, bool is_control_pressed, bool is_alt_pressed);
    void mouseMoveEvent(QMouseEvent* mouse_event);
    void mousePressEvent(QMouseEvent* mevent);
    void mouseReleaseEvent(QMouseEvent* mevent);
    void mouseDoubleClickEvent(QMouseEvent* mevent);
    void wheelEvent(QWheelEvent* wevent);

private:
    MainWidget* main_widget;
};
