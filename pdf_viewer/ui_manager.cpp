#include "ui_manager.h"
#include "main_widget.h"
#include "utils.h"
#include <qboxlayout.h>
#include <qfont.h>
#include <qtimer.h>

extern std::wstring UI_FONT_FACE_NAME;
extern int MAX_SCROLLBAR;

UIManager::UIManager(MainWidget* main_widget) : main_widget(main_widget) {

}

UIManager::~UIManager() {
    // Widgets are children of MainWidget so they are deleted automatically?
    // But we created them. If we pass `this` (MainWidget) as parent, they are managed by Qt.
}

QString get_font_face_name() {
    if (UI_FONT_FACE_NAME.empty()) {
        return "Monaco";
    }
    else {
        return QString::fromStdWString(UI_FONT_FACE_NAME);
    }
}

void UIManager::setup_ui() {
    status_label = new QLabel(main_widget);
    status_label->setStyleSheet(get_status_stylesheet());
    QFont label_font = QFont(get_font_face_name());
    label_font.setStyleHint(QFont::TypeWriter);
    status_label->setFont(label_font);

    text_command_line_edit_container = new QWidget(main_widget);
    text_command_line_edit_container->setStyleSheet(get_status_stylesheet());

    QHBoxLayout* text_command_line_edit_container_layout = new QHBoxLayout();

    text_command_line_edit_label = new QLabel();
    text_command_line_edit = new QLineEdit();

    text_command_line_edit_label->setFont(QFont(get_font_face_name()));
    text_command_line_edit->setFont(QFont(get_font_face_name()));

    text_command_line_edit_label->setStyleSheet(get_status_stylesheet());
    text_command_line_edit->setStyleSheet(get_status_stylesheet());

    text_command_line_edit_container_layout->addWidget(text_command_line_edit_label);
    text_command_line_edit_container_layout->addWidget(text_command_line_edit);
    text_command_line_edit_container_layout->setContentsMargins(10, 0, 10, 0);

    text_command_line_edit_container->setLayout(text_command_line_edit_container_layout);
    text_command_line_edit_container->hide();

    scroll_bar = new QScrollBar(main_widget);
    scroll_bar->setMinimum(0);
    scroll_bar->setMaximum(10000); // MAX_SCROLLBAR is 10000 in main_widget.cpp const

    scroll_bar->connect(scroll_bar, &QScrollBar::actionTriggered, [this](int action) {
        int value = scroll_bar->value();
        if (main_widget->main_document_view_has_document()) {
            float offset = main_widget->doc()->max_y_offset() * value / static_cast<float>(scroll_bar->maximum());
            main_widget->main_document_view->set_offset_y(offset);
            main_widget->validate_render();
        }
    });

    scroll_bar->hide();
}

void UIManager::resize_events() {
    int main_window_width = main_widget->size().width();
    int main_window_height = main_widget->size().height();

    if (text_command_line_edit_container != nullptr) {
        text_command_line_edit_container->move(0, 0);
        text_command_line_edit_container->resize(main_window_width, 30);
    }

    if (status_label != nullptr) {
        int status_bar_height = get_status_bar_height();
        status_label->move(0, main_window_height - status_bar_height);
        status_label->resize(main_window_width, status_bar_height);
        if (should_show_status_label) {
            status_label->show();
        }
    }
}

void UIManager::update_status_bar() {
    if (status_label) {
        status_label->setText(QString::fromStdWString(main_widget->get_status_string()));
    }
}

void UIManager::toggle_statusbar() {
    should_show_status_label = !should_show_status_label;

    if (!should_show_status_label) {
        status_label->hide();
    }
    else {
        status_label->show();
    }
}

void UIManager::show_textbar(const std::wstring& command_name, bool should_fill_with_selected_text) {
    text_command_line_edit->clear();
    if (should_fill_with_selected_text) {
        text_command_line_edit->setText(QString::fromStdWString(main_widget->selected_text));
    }
    text_command_line_edit_label->setText(QString::fromStdWString(command_name));
    text_command_line_edit_container->show();
    text_command_line_edit->setFocus();
}

void UIManager::toggle_scrollbar() {
    QTimer::singleShot(100, [this]() {
        if (scroll_bar->isVisible()) {
            scroll_bar->hide();
        }
        else {
            scroll_bar->show();
        }
        // This was in MainWidget, maybe we need to notify it or update layout
        // main_window_width = opengl_widget->width();
        // We probably don't need to update width here manually if layouts are used correctly,
        // but existing code did. For now let's skip updating width variable as it is member of MainWidget.
        // Or we can call resize event?
        main_widget->update();
    });
}

void UIManager::update_scrollbar() {
    if (main_widget->main_document_view_has_document()) {
        float offset = main_widget->main_document_view->get_offset_y();
        int scroll = static_cast<int>(10000 * offset / main_widget->doc()->max_y_offset());
        scroll_bar->setValue(scroll);
    }
}

void UIManager::on_config_file_changed() {
    status_label->setStyleSheet(get_status_stylesheet());
    status_label->setFont(QFont(get_font_face_name()));
    text_command_line_edit_container->setStyleSheet(get_status_stylesheet());
    text_command_line_edit->setFont(QFont(get_font_face_name()));

    text_command_line_edit_label->setStyleSheet(get_status_stylesheet());
    text_command_line_edit->setStyleSheet(get_status_stylesheet());

    int status_bar_height = get_status_bar_height();
    status_label->move(0, main_widget->size().height() - status_bar_height);
    status_label->resize(main_widget->size().width(), status_bar_height);
}

void UIManager::hide_textbar() {
    text_command_line_edit_container->hide();
}

bool UIManager::is_textbar_visible() {
    return text_command_line_edit_container->isVisible();
}

std::wstring UIManager::get_textbar_text() {
    return text_command_line_edit->text().toStdWString();
}

void UIManager::set_textbar_text(const std::wstring& text) {
    text_command_line_edit->setText(QString::fromStdWString(text));
}
