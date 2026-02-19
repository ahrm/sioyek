#pragma once
#include <string>
#include <qwidget.h>
#include <qlabel.h>
#include <qlineedit.h>
#include <qscrollbar.h>

class MainWidget;

class UIManager {
public:
    UIManager(MainWidget* main_widget);
    ~UIManager();

    void setup_ui();
    void resize_events();
    void update_status_bar();
    void toggle_statusbar();
    void show_textbar(const std::wstring& command_name, bool should_fill_with_selected_text = false);
    void toggle_scrollbar();
    void update_scrollbar();
    void on_config_file_changed();
    void hide_textbar();
    bool is_textbar_visible();
    std::wstring get_textbar_text();
    void set_textbar_text(const std::wstring& text);

    QLabel* status_label = nullptr;
    QWidget* text_command_line_edit_container = nullptr;
    QLabel* text_command_line_edit_label = nullptr;
    QLineEdit* text_command_line_edit = nullptr;
    QScrollBar* scroll_bar = nullptr;

    bool should_show_status_label = true;

private:
    MainWidget* main_widget;
};
