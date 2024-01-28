#include "ui.h"
#include <qfiledialog.h>

#include <QItemSelectionModel>
#include <QTapGesture>
#include <main_widget.h>

#include <QQuickView>
#include <QQmlComponent>
#include <QQuickItem>
#include <QQmlEngine>
#include <QQuickWidget>
#include <QFile>
#include "touchui/TouchSlider.h"
#include "touchui/TouchConfigMenu.h"
#include "touchui/TouchSettings.h"

extern std::wstring DEFAULT_OPEN_FILE_PATH;
extern float DARK_MODE_CONTRAST;
extern float BACKGROUND_COLOR[3];
extern bool RULER_MODE;
extern float VISUAL_MARK_NEXT_PAGE_FRACTION;
extern float VISUAL_MARK_NEXT_PAGE_THRESHOLD;
extern float HIGHLIGHT_COLORS[26 * 3];
extern float TTS_RATE;
extern bool EMACS_MODE;
extern float MENU_SCREEN_WDITH_RATIO;

std::wstring select_command_file_name(std::string command_name) {
    if (command_name == "open_document") {
        return select_document_file_name();
    }
    else if (command_name == "source_config") {
        return select_any_file_name();
    }
    else {
        return select_any_file_name();
    }
}

std::wstring select_command_folder_name() {
    QString dir_name = QFileDialog::getExistingDirectory(nullptr, "Select Folder");
    return dir_name.toStdWString();
}

std::wstring select_document_file_name() {
    if (DEFAULT_OPEN_FILE_PATH.size() == 0) {

        QString file_name = QFileDialog::getOpenFileName(nullptr, "Select Document", "", "Documents (*.pdf *.epub *.cbz)");
        return file_name.toStdWString();
    }
    else {

        QFileDialog fd = QFileDialog(nullptr, "Select Document", "", "Documents (*.pdf *.epub *.cbz)");
        fd.setDirectory(QString::fromStdWString(DEFAULT_OPEN_FILE_PATH));
        if (fd.exec()) {

            QString file_name = fd.selectedFiles().first();
            return file_name.toStdWString();
        }
        else {
            return L"";
        }
    }

}

std::wstring select_json_file_name() {
    QString file_name = QFileDialog::getSaveFileName(nullptr, "Select Document", "", "Documents (*.json )");
    return file_name.toStdWString();
}

std::wstring select_any_file_name() {
    QString file_name = QFileDialog::getSaveFileName(nullptr, "Select File", "", "Any (*)");
    return file_name.toStdWString();
}

std::wstring select_new_json_file_name() {
    QString file_name = QFileDialog::getSaveFileName(nullptr, "Select Document", "", "Documents (*.json )");
    return file_name.toStdWString();
}

std::wstring select_new_pdf_file_name() {
    QString file_name = QFileDialog::getSaveFileName(nullptr, "Select Document", "", "Documents (*.pdf )");
    return file_name.toStdWString();
}


std::vector<ConfigFileChangeListener*> ConfigFileChangeListener::registered_listeners;

ConfigFileChangeListener::ConfigFileChangeListener() {
    registered_listeners.push_back(this);
}

ConfigFileChangeListener::~ConfigFileChangeListener() {
    registered_listeners.erase(std::find(registered_listeners.begin(), registered_listeners.end(), this));
}

void ConfigFileChangeListener::notify_config_file_changed(ConfigManager* new_config_manager) {
    for (auto* it : ConfigFileChangeListener::registered_listeners) {
        it->on_config_file_changed(new_config_manager);
    }
}

bool HierarchialSortFilterProxyModel::filterAcceptsRow(int source_row, const QModelIndex& source_parent) const
{
    // custom behaviour :
#ifdef SIOYEK_QT6
    if (filterRegularExpression().pattern().size() == 0)
#else
    if (filterRegExp().isEmpty() == false)
#endif
    {
        // get source-model index for current row
        QModelIndex source_index = sourceModel()->index(source_row, this->filterKeyColumn(), source_parent);
        if (source_index.isValid())
        {
            // check current index itself :
            QString key = sourceModel()->data(source_index, filterRole()).toString();

#ifdef SIOYEK_QT6
            bool parent_contains = key.contains(filterRegularExpression());
#else
            bool parent_contains = key.contains(filterRegExp());
#endif

            if (parent_contains) return true;

            // if any of children matches the filter, then current index matches the filter as well
            int i, nb = sourceModel()->rowCount(source_index);
            for (i = 0; i < nb; ++i)
            {
                if (filterAcceptsRow(i, source_index))
                {
                    return true;
                }
            }
            return false;
        }
    }
    // parent call for initial behaviour
    return QSortFilterProxyModel::filterAcceptsRow(source_row, source_parent);
}

AndroidSelector::AndroidSelector(QWidget* parent) : QWidget(parent) {
    //     layout = new QVBoxLayout();

    main_widget = dynamic_cast<MainWidget*>(parent);
    int current_colorscheme_index = main_widget->get_current_colorscheme_index();
    bool horizontal_locked = main_widget->horizontal_scroll_locked;
    bool fullscreen = main_widget->isFullScreen();
    bool ruler = main_widget->is_visual_mark_mode();
    bool speaking = main_widget->is_reading;
    bool portaling = main_widget->is_pending_link_source_filled();
    bool fit_mode = main_widget->last_smart_fit_page.has_value();

    main_menu = new TouchMainMenu(fit_mode, portaling, fullscreen, ruler, speaking, horizontal_locked, current_colorscheme_index, this);


    //    set_rect_config_button = new QPushButton("Rect Config", this);
    //    goto_page_button = new QPushButton("Goto Page", this);
    //    fullscreen_button = new QPushButton("Fullscreen", this);
    //    select_text_button = new QPushButton("Select Text", this);
    //    open_document_button = new QPushButton("Open New Document", this);
    //    open_prev_document_button = new QPushButton("Open Previous Document", this);
    //    command_button = new QPushButton("Command", this);
    //    visual_mode_button = new QPushButton("Visual Mark Mode", this);
    //    search_button = new QPushButton("Search", this);
    //    set_background_color = new QPushButton("Background Color", this);
    //    set_dark_mode_contrast = new QPushButton("Dark Mode Contrast", this);
    //    set_ruler_mode = new QPushButton("Ruler Mode", this);
    //    restore_default_config_button = new QPushButton("Restore Default Config", this);
    //    toggle_dark_mode_button = new QPushButton("Toggle Dark Mode", this);
    //    ruler_mode_bounds_config_button = new QPushButton("Configure Ruler Mode", this);
    //    test_rectangle_select_ui = new QPushButton("Rectangle Select", this);

    //    layout->addWidget(set_rect_config_button);
    //    layout->addWidget(goto_page_button);
    //    layout->addWidget(fullscreen_button);
    //    layout->addWidget(select_text_button);
    //    layout->addWidget(open_document_button);
    //    layout->addWidget(open_prev_document_button);
    //    layout->addWidget(command_button);
    //    layout->addWidget(visual_mode_button);
    //    layout->addWidget(search_button);
    //    layout->addWidget(set_background_color);
    //    layout->addWidget(set_dark_mode_contrast);
    //    layout->addWidget(set_ruler_mode);
    //    layout->addWidget(restore_default_config_button);
    //    layout->addWidget(toggle_dark_mode_button);
    //    layout->addWidget(ruler_mode_bounds_config_button);
    //    layout->addWidget(test_rectangle_select_ui);


    QObject::connect(main_menu, &TouchMainMenu::fullscreenClicked, [&]() {
        //main_widget->current_widget = {};
        assert(main_widget->current_widget_stack.back() == this);
        main_widget->pop_current_widget();
        //deleteLater();
        main_widget->toggle_fullscreen();
        });

    QObject::connect(main_menu, &TouchMainMenu::selectTextClicked, [&]() {
        //main_widget->current_widget = {};
        //deleteLater();
        assert(main_widget->current_widget_stack.back() == this);
        main_widget->pop_current_widget();
        main_widget->handle_mobile_selection();
        main_widget->invalidate_render();
        });

    QObject::connect(main_menu, &TouchMainMenu::openNewDocClicked, [&]() {
        //main_widget->current_widget = {};
        //deleteLater();
        assert(main_widget->current_widget_stack.back() == this);
        main_widget->pop_current_widget();
        main_widget->run_command_with_name("open_document");
        });

    QObject::connect(main_menu, &TouchMainMenu::openPrevDocClicked, [&]() {
        main_widget->run_command_with_name("open_prev_doc", true);
        });

    QObject::connect(main_menu, &TouchMainMenu::highlightsClicked, [&]() {
        main_widget->run_command_with_name("goto_highlight", true);
        });

    QObject::connect(main_menu, &TouchMainMenu::tocClicked, [&]() {
        main_widget->run_command_with_name("goto_toc", true);
        });

    QObject::connect(main_menu, &TouchMainMenu::bookmarksClicked, [&]() {
        main_widget->run_command_with_name("goto_bookmark", true);
        });

    QObject::connect(main_menu, &TouchMainMenu::hintClicked, [&]() {
        //main_widget->run_command_with_name("toggle_rect_hints", true);

        main_widget->toggle_scratchpad_mode();
        main_widget->pop_current_widget();

        main_widget->invalidate_render();

        });

    QObject::connect(main_menu, &TouchMainMenu::commandsClicked, [&]() {
        main_widget->run_command_with_name("command", true);
        });

    QObject::connect(main_menu, &TouchMainMenu::drawingModeClicked, [&]() {
        main_widget->run_command_with_name("toggle_freehand_drawing_mode", true);
        });

    QObject::connect(main_menu, &TouchMainMenu::settingsClicked, [&]() {
        //TouchConfigMenu* config_menu = new TouchConfigMenu(main_widget);
        main_widget->show_touch_settings_menu();

        //TouchSettings* config_menu = new TouchSettings(main_widget);
        //assert(main_widget->current_widget_stack.back() == this);
        //main_widget->pop_current_widget();

        //main_widget->set_current_widget(config_menu);
        //main_widget->show_current_widget();

        });

    QObject::connect(main_menu, &TouchMainMenu::rulerModeClicked, [&]() {
        main_widget->android_handle_visual_mode();
        //main_widget->current_widget = {};
        //deleteLater();
        assert(main_widget->current_widget_stack.back() == this);
        main_widget->pop_current_widget();
        main_widget->invalidate_render();
        });

    QObject::connect(main_menu, &TouchMainMenu::searchClicked, [&]() {
        main_widget->run_command_with_name("search", true);
        });

    QObject::connect(main_menu, &TouchMainMenu::addBookmarkClicked, [&]() {
        main_widget->run_command_with_name("add_bookmark", true);
        });

    QObject::connect(main_menu, &TouchMainMenu::portalClicked, [&]() {
        main_widget->run_command_with_name("portal", true);
        });

    QObject::connect(main_menu, &TouchMainMenu::deletePortalClicked, [&]() {
        main_widget->run_command_with_name("delete_portal", true);
        });

    QObject::connect(main_menu, &TouchMainMenu::globalBookmarksClicked, [&]() {
        main_widget->run_command_with_name("goto_bookmark_g", true);
        });

    QObject::connect(main_menu, &TouchMainMenu::globalHighlightsClicked, [&]() {
        main_widget->run_command_with_name("goto_highlight_g", true);
        });

    QObject::connect(main_menu, &TouchMainMenu::ttsClicked, [&]() {
        main_widget->run_command_with_name("start_reading", true);
        });

    QObject::connect(main_menu, &TouchMainMenu::horizontalLockClicked, [&]() {
        main_widget->run_command_with_name("toggle_horizontal_scroll_lock", true);
        });

    //    QObject::connect(set_background_color, &QPushButton::pressed, [&](){

    //        auto command = main_widget->command_manager->get_command_with_name("setconfig_background_color");
    //        main_widget->current_widget = {};
    //        deleteLater();
    //        main_widget->handle_command_types(std::move(command), 0);
    //    });

    //    QObject::connect(set_rect_config_button, &QPushButton::pressed, [&](){

    //        auto command = main_widget->command_manager->get_command_with_name("setconfig_portrait_back_ui_rect");
    //        main_widget->current_widget = {};
    //        deleteLater();
    //        main_widget->handle_command_types(std::move(command), 0);
    //    });

    //    QObject::connect(set_dark_mode_contrast, &QPushButton::pressed, [&](){

    //        main_widget->current_widget = {};
    //        deleteLater();

    //        main_widget->current_widget = new FloatConfigUI(main_widget, &DARK_MODE_CONTRAST, 0.0f, 1.0f);
    //        main_widget->current_widget->show();
    //    });

    //    QObject::connect(set_ruler_mode, &QPushButton::pressed, [&](){

    //        auto command = main_widget->command_manager->get_command_with_name("setconfig_ruler_mode");
    //        main_widget->current_widget = {};
    //        deleteLater();
    //        main_widget->handle_command_types(std::move(command), 0);

    //    });

    //    QObject::connect(restore_default_config_button, &QPushButton::pressed, [&](){

    //        main_widget->current_widget = {};
    //        deleteLater();
    //        main_widget->restore_default_config();

    //    });

    QObject::connect(main_menu, &TouchMainMenu::darkColorschemeClicked, [&]() {
        main_widget->set_dark_mode();
        //main_widget->current_widget = {};
        //deleteLater();
        assert(main_widget->current_widget_stack.back() == this);
        main_widget->pop_current_widget();
        main_widget->invalidate_render();
        });

    QObject::connect(main_menu, &TouchMainMenu::lightColorschemeClicked, [&]() {
        main_widget->set_light_mode();
        //main_widget->current_widget = {};
        //deleteLater();
        assert(main_widget->current_widget_stack.back() == this);
        main_widget->pop_current_widget();
        main_widget->invalidate_render();
        });

    QObject::connect(main_menu, &TouchMainMenu::customColorschemeClicked, [&]() {
        main_widget->set_custom_color_mode();
        //main_widget->current_widget = {};
        //deleteLater();
        assert(main_widget->current_widget_stack.back() == this);
        main_widget->pop_current_widget();
        main_widget->invalidate_render();
        });

    QObject::connect(main_menu, &TouchMainMenu::downloadPaperClicked, [&]() {
        main_widget->pop_current_widget();
        main_widget->download_paper_under_cursor(true);
        main_widget->invalidate_render();
        });


    QObject::connect(main_menu, &TouchMainMenu::fitToPageWidthClicked, [&]() {

        if (main_widget->last_smart_fit_page) {
            main_widget->last_smart_fit_page = {};
            main_widget->pop_current_widget();
            main_widget->invalidate_render();
        }
        else {
            //main_widget->main_document_view->fit_to_page_width(true);
            main_widget->handle_fit_to_page_width(true);
            int current_page = main_widget->get_current_page_number();
            main_widget->last_smart_fit_page = current_page;

            main_widget->pop_current_widget();
            main_widget->invalidate_render();
        }
        });
    //    QObject::connect(ruler_mode_bounds_config_button, &QPushButton::pressed, [&](){
    ////        auto command = main_widget->command_manager->get_command_with_name("toggle_dark_mode");
    //        main_widget->current_widget = nullptr;
    //        deleteLater();
    //        RangeConfigUI* config_ui = new RangeConfigUI(main_widget, &VISUAL_MARK_NEXT_PAGE_FRACTION, &VISUAL_MARK_NEXT_PAGE_THRESHOLD);
    //        main_widget->current_widget = config_ui;
    //        main_widget->current_widget->show();
    ////        main_widget->handle_command_types(std::move(command), 0);
    //    });

    QObject::connect(main_menu, &TouchMainMenu::gotoPageClicked, [&]() {
        //main_widget->current_widget = nullptr;
        //deleteLater();
        assert(main_widget->current_widget_stack.back() == this);
        main_widget->pop_current_widget();
        PageSelectorUI* page_ui = new PageSelectorUI(main_widget,
            main_widget->get_current_page_number(),
            main_widget->current_document_page_count());

        main_widget->set_current_widget(page_ui);
        main_widget->show_current_widget();
        //main_widget->current_widget = page_ui;
        //main_widget->current_widget->show();
        });

    //    QObject::connect(test_rectangle_select_ui, &QPushButton::pressed, [&](){
    ////        auto command = main_widget->command_manager->get_command_with_name("toggle_dark_mode");
    ////        main_widget->current_widget = {};
    //        RectangleConfigUI* configui = new RectangleConfigUI(main_widget, &testrect);
    //        main_widget->current_widget = configui;
    //        configui->show();

    //        deleteLater();
    ////        main_widget->handle_command_types(std::move(command), 0);
    //    });

    //     layout->insertStretch(-1, 1);

    //     this->setLayout(layout);
}

void AndroidSelector::resizeEvent(QResizeEvent* resize_event) {
    QWidget::resizeEvent(resize_event);
    int parent_width = parentWidget()->width();
    int parent_height = parentWidget()->height();

    //float parent_width_in_centimeters = static_cast<float>(parent_width) / logicalDpiX() * 2.54f;
    //float parent_height_in_centimeters = static_cast<float>(parent_height) / logicalDpiY() * 2.54f;
    int ten_cm = static_cast<int>(12 * logicalDpiX() / 2.54f);

    int w = static_cast<int>(parent_width * 0.9f);
    int h = parent_height;

    w = std::min(w, ten_cm);
    h = std::min(h, ten_cm);

    main_menu->resize(w, h);
    setFixedSize(w, h);

    //    list_view->setFixedSize(parent_width * 0.9f, parent_height);
    move((parent_width - w) / 2, (parent_height - h) / 2);

}

//TextSelectionButtons::TextSelectionButtons(MainWidget* parent) : QWidget(parent) {

//    QHBoxLayout* layout = new QHBoxLayout();

//    main_widget = parent;
//    copy_button = new QPushButton("Copy");
//    search_in_scholar_button = new QPushButton("Search Scholar");
//    search_in_google_button = new QPushButton("Search Google");
//    highlight_button = new QPushButton("Highlight");

//    QObject::connect(copy_button, &QPushButton::clicked, [&](){
//        copy_to_clipboard(main_widget->selected_text);
//    });

//    QObject::connect(search_in_scholar_button, &QPushButton::clicked, [&](){
//        search_custom_engine(main_widget->selected_text, L"https://scholar.google.com/scholar?&q=");
//    });

//    QObject::connect(search_in_google_button, &QPushButton::clicked, [&](){
//        search_custom_engine(main_widget->selected_text, L"https://www.google.com/search?q=");
//    });

//    QObject::connect(highlight_button, &QPushButton::clicked, [&](){
//        main_widget->handle_touch_highlight();
//    });

//    layout->addWidget(copy_button);
//    layout->addWidget(search_in_scholar_button);
//    layout->addWidget(search_in_google_button);
//    layout->addWidget(highlight_button);

//    this->setLayout(layout);
//}

//void TextSelectionButtons::resizeEvent(QResizeEvent* resize_event) {
//    QWidget::resizeEvent(resize_event);
//    int parent_width = parentWidget()->width();
//    int parent_height = parentWidget()->height();

//    setFixedSize(parent_width, parent_height / 5);
////    list_view->setFixedSize(parent_width * 0.9f, parent_height);
//    move(0, 0);

//}

TouchTextSelectionButtons::TouchTextSelectionButtons(MainWidget* parent) : QWidget(parent) {
    main_widget = parent;
    buttons_ui = new TouchCopyOptions(this);
    this->setAttribute(Qt::WA_NoMousePropagation);

    QObject::connect(buttons_ui, &TouchCopyOptions::copyClicked, [&]() {
        copy_to_clipboard(main_widget->get_selected_text());
        main_widget->clear_selection_indicators();
        });

    QObject::connect(buttons_ui, &TouchCopyOptions::searchClicked, [&]() {
        main_widget->perform_search(main_widget->get_selected_text(), false);
        main_widget->show_search_buttons();
        main_widget->clear_selection_indicators();
        });

    QObject::connect(buttons_ui, &TouchCopyOptions::scholarClicked, [&]() {
        search_custom_engine(main_widget->get_selected_text(), L"https://scholar.google.com/scholar?&q=");
        main_widget->clear_selection_indicators();
        });

    QObject::connect(buttons_ui, &TouchCopyOptions::googleClicked, [&]() {
        search_custom_engine(main_widget->get_selected_text(), L"https://www.google.com/search?q=");
        main_widget->clear_selection_indicators();
        });

    QObject::connect(buttons_ui, &TouchCopyOptions::highlightClicked, [&]() {
        main_widget->handle_touch_highlight();
        main_widget->clear_selection_indicators();
        });

    QObject::connect(buttons_ui, &TouchCopyOptions::downloadClicked, [&]() {
        main_widget->download_selected_text();
        main_widget->clear_selection_indicators();
        });

    QObject::connect(buttons_ui, &TouchCopyOptions::smartJumpClicked, [&]() {
        main_widget->smart_jump_to_selected_text();
        main_widget->clear_selection_indicators();
        });
}

DrawControlsUI::DrawControlsUI(MainWidget* parent) : QWidget(parent) {
    main_widget = parent;
    controls_ui = new TouchDrawControls(parent->freehand_thickness, parent->get_current_freehand_type(), this);
    this->setAttribute(Qt::WA_NoMousePropagation);

    QObject::connect(controls_ui, &TouchDrawControls::exitDrawModePressed, [&]() {
        main_widget->exit_freehand_drawing_mode();
        });

    QObject::connect(controls_ui, &TouchDrawControls::changeColorPressed, [&](int color_index) {
        main_widget->current_freehand_type = 'a' + color_index;
        });

    QObject::connect(controls_ui, &TouchDrawControls::enablePenDrawModePressed, [&]() {
        main_widget->set_pen_drawing_mode(true);
        });

    QObject::connect(controls_ui, &TouchDrawControls::screenshotPressed, [&]() {
        if (main_widget->is_scratchpad_mode()) {
            main_widget->run_command_with_name("copy_drawings_from_scratchpad");
        }
        else {
            main_widget->run_command_with_name("copy_screenshot_to_scratchpad");
        }
        });

    QObject::connect(controls_ui, &TouchDrawControls::saveScratchpadPressed, [&]() {
        main_widget->run_command_with_name("save_scratchpad");
        main_widget->invalidate_render();
        });

    QObject::connect(controls_ui, &TouchDrawControls::loadScratchpadPressed, [&]() {
        main_widget->run_command_with_name("load_scratchpad");
        main_widget->invalidate_render();
        });

    QObject::connect(controls_ui, &TouchDrawControls::movePressed, [&]() {
        main_widget->run_command_with_name("select_freehand_drawings");
        main_widget->invalidate_render();
        });

    QObject::connect(controls_ui, &TouchDrawControls::toggleScratchpadPressed, [&]() {
        main_widget->run_command_with_name("toggle_scratchpad_mode");
        main_widget->invalidate_render();
        });

    QObject::connect(controls_ui, &TouchDrawControls::disablePenDrawModePressed, [&]() {
        main_widget->set_hand_drawing_mode(true);
        });

    QObject::connect(controls_ui, &TouchDrawControls::eraserPressed, [&]() {
        main_widget->run_command_with_name("delete_freehand_drawings");
        });

    QObject::connect(controls_ui, &TouchDrawControls::penSizeChanged, [&](qreal val) {
        main_widget->set_freehand_thickness(val);
        });


}
void DrawControlsUI::resizeEvent(QResizeEvent* resize_event) {
    QWidget::resizeEvent(resize_event);

    int pwidth = parentWidget()->width();
    int width = parentWidget()->width() * 3 / 4;
    int height = std::max(parentWidget()->height() / 16, 50);

    controls_ui->move(0, 0);
    controls_ui->resize(width, height);
    move((pwidth - width) / 2, height);
    setFixedSize(width, height);

}

void TouchTextSelectionButtons::resizeEvent(QResizeEvent* resize_event) {
    QWidget::resizeEvent(resize_event);

    int pwidth = parentWidget()->width();
    int width = parentWidget()->width() * 3 / 4;
    int height = parentWidget()->height() / 16;

    buttons_ui->move(0, 0);
    buttons_ui->resize(width, height);
    move((pwidth - width) / 2, height);
    //    setFixedSize(0, 0);
    setFixedSize(width, height);

}

HighlightButtons::HighlightButtons(MainWidget* parent) : QWidget(parent) {
    main_widget = parent;
    //layout = new QHBoxLayout();

    //delete_highlight_button = new QPushButton("Delete");
    //buttons_widget = new 
    highlight_buttons = new TouchHighlightButtons(main_widget->get_current_selected_highlight_type(), this);

    QObject::connect(highlight_buttons, &TouchHighlightButtons::deletePressed, [&]() {
        main_widget->handle_delete_selected_highlight();
        hide();
        });

    QObject::connect(highlight_buttons, &TouchHighlightButtons::editPressed, [&]() {
        main_widget->run_command_with_name("edit_selected_highlight");
        //main_widget->handle_delete_selected_highlight();
        hide();
        });

    QObject::connect(highlight_buttons, &TouchHighlightButtons::changeColorPressed, [&](int index) {
        //float* color = &HIGHLIGHT_COLORS[3 * index];

        //main_widget->handle_delete_selected_highlight();
        if (index < 26) {
            main_widget->change_selected_highlight_type('a' + index);
        }
        else {
            main_widget->change_selected_highlight_type('A' + index - 26);
        }
        hide();
        main_widget->invalidate_render();
        //main_widget->highlight_buttons = nullptr;
        //deleteLater();
        });

    //layout->addWidget(delete_highlight_button);
    //this->setLayout(layout);
}

void HighlightButtons::resizeEvent(QResizeEvent* resize_event) {

    QWidget::resizeEvent(resize_event);

    int parent_width = parentWidget()->width();
    int parent_height = parentWidget()->height();

    int dpi = physicalDpiY();
    float parent_height_in_centimeters = static_cast<float>(parent_height) / dpi * 2.54f;

    //int w = static_cast<int>(parent_width / 5);
    int w = parent_width;
    int h = static_cast<int>(static_cast<float>(dpi) / 2.54f);
    w = std::max(w, h * 6);

    setFixedSize(w, h);
    highlight_buttons->resize(w, h);
    move((parent_width - w) / 2, parent_height / 5);
}


SearchButtons::SearchButtons(MainWidget* parent) : QWidget(parent) {
    main_widget = parent;
    //layout = new QHBoxLayout(this);

//    delete_highlight_button = new QPushButton("Delete");
//    QObject::connect(delete_highlight_button, &QPushButton::clicked, [&](){
//        main_widget->handle_delete_selected_highlight();
//        hide();
//        main_widget->highlight_buttons = nullptr;
//        deleteLater();
//    });
    //next_match_button = new QPushButton("next");
    //prev_match_button = new QPushButton("prev");
    //goto_initial_location_button = new QPushButton("initial");
    buttons_widget = new TouchSearchButtons(this);

    QObject::connect(buttons_widget, &TouchSearchButtons::nextPressed, [&]() {
        //main_widget->opengl_widget->goto_search_result(1);
        main_widget->run_command_with_name("next_item");
        main_widget->validate_render();
        });

    QObject::connect(buttons_widget, &TouchSearchButtons::previousPressed, [&]() {
        main_widget->run_command_with_name("previous_item");
        //main_widget->opengl_widget->goto_search_result(-1);
        main_widget->validate_render();
        });

    QObject::connect(buttons_widget, &TouchSearchButtons::initialPressed, [&]() {
        main_widget->goto_mark('/');
        main_widget->invalidate_render();
        });


    //layout->addWidget(next_match_button);
    //layout->addWidget(goto_initial_location_button);
    //layout->addWidget(prev_match_button);

    //this->setLayout(layout);
}

void SearchButtons::resizeEvent(QResizeEvent* resize_event) {

    QWidget::resizeEvent(resize_event);
    int parent_width = parentWidget()->width();
    int parent_height = parentWidget()->height();

    int width = 2 * parentWidget()->width() / 3;
    int height = parentWidget()->height() / 12;


    buttons_widget->resize(width, height);
    setFixedSize(width, height);
    //layout->update();
    move((parent_width - width) / 2, parent_height - 3 * height / 2);
}

//ConfigUI::ConfigUI(MainWidget* parent) : QQuickWidget(parent){
ConfigUI::ConfigUI(std::string name, MainWidget* parent) : QWidget(parent) {
    main_widget = parent;
    config_name = name;
}

void ConfigUI::on_change() {
    main_widget->on_config_changed(config_name);
}

void ConfigUI::set_should_persist(bool val) {
    this->should_persist = val;
}

void ConfigUI::resizeEvent(QResizeEvent* resize_event) {
    QWidget::resizeEvent(resize_event);
    int parent_width = parentWidget()->width();
    int parent_height = parentWidget()->height();

    setFixedSize(2 * parent_width / 3, parent_height / 2);
    move(parent_width / 6, parent_height / 4);
}

Color3ConfigUI::Color3ConfigUI(std::string name, MainWidget* parent, float* config_location_) : ConfigUI(name, parent) {
    color_location = config_location_;
    color_picker = new QColorDialog(this);
    color_picker->show();

    connect(color_picker, &QColorDialog::colorSelected, [&](const QColor& color) {
        convert_qcolor_to_float3(color, color_location);
        main_widget->invalidate_render();
        on_change();

        if (should_persist) {
            main_widget->persist_config();
        }
        });

    //    QQmlEngine* engine = new QQmlEngine();
    //    QQmlComponent* component = new QQmlComponent(engine, QUrl("qrc:/pdf_viewer/qml/MyColorPicker.qml"), this);
    //    QObject *object = component->create();
    //    QQuickItem *item = qobject_cast<QQuickItem*>(object);

    //    QFile source_file("qrc:/pdf_viewer/qml/MyColorPicker.qml");
    //    QString source = source_file.readAll();

    //    QQuickWidget* quick_widget(source, this);
    //    QUrl url("qrc:/pdf_viewer/qml/MyColorPicker.qml");
    //    color_picker = new QQuickWidget(url, this);
    //    color_picker->setSizePolicy(QSizePolicy::Policy::Maximum, QSizePolicy::Policy::Maximum);
    //    color_picker->show();
    //    TouchSlider* slider = new TouchSlider(0, 100, 25, this);
    //    QObject::connect(slider, &TouchSlider::itemSelected, [&](int selected_value){
    //        qDebug() << "selected " << selected_value << "\n";

    //    });


    //    QObject::connect(color_picker, SIGNAL()

    //    QQuickWidget
    //    QQuickView* view = new QQuickView();
    //    view->setSource(QUrl("qrc:/pdf_viewer/qml/MyColorPicker.qml"));
    //    view->show();


}

void Color3ConfigUI::resizeEvent(QResizeEvent* resize_event) {
    QWidget::resizeEvent(resize_event);
    int parent_width = parentWidget()->width();
    int parent_height = parentWidget()->height();

    setFixedSize(2 * parent_width / 3, parent_height / 2);
    color_picker->resize(width(), height());

    move(parent_width / 6, parent_height / 4);
}

Color4ConfigUI::Color4ConfigUI(std::string name, MainWidget* parent, float* config_location_) : ConfigUI(name, parent) {
    color_location = config_location_;
    color_picker = new QColorDialog(this);
    color_picker->show();

    connect(color_picker, &QColorDialog::colorSelected, [&](const QColor& color) {
        convert_qcolor_to_float4(color, color_location);
        main_widget->invalidate_render();
        on_change();
        if (should_persist) {
            main_widget->persist_config();
        }
        });
}

MacroConfigUI::MacroConfigUI(std::string name, MainWidget* parent, std::wstring* config_location, std::wstring initial_macro) : ConfigUI(name, parent) {

    macro_editor = new TouchMacroEditor(utf8_encode(initial_macro), this, parent);

    connect(macro_editor, &TouchMacroEditor::macroConfirmed, [&, config_location](std::string macro) {
        //convert_qcolor_to_float4(color, color_location);
        (*config_location) = utf8_decode(macro);
        on_change();
        if (should_persist) {
            main_widget->persist_config();
        }
        main_widget->pop_current_widget();
        });

    QObject::connect(macro_editor, &TouchMacroEditor::canceled, [&]() {
        main_widget->pop_current_widget();
        });


}

FloatConfigUI::FloatConfigUI(std::string name, MainWidget* parent, float* config_location, float min_value_, float max_value_) : ConfigUI(name, parent) {

    min_value = min_value_;
    max_value = max_value_;
    float_location = config_location;

    int current_value = static_cast<int>((*config_location - min_value) / (max_value - min_value) * 100);
    slider = new TouchSlider(0, 100, current_value, this);
    QObject::connect(slider, &TouchSlider::itemSelected, [&](int val) {
        float value = min_value + (static_cast<float>(val) / 100.0f) * (max_value - min_value);
        *float_location = value;
        on_change();
        main_widget->invalidate_render();
        main_widget->pop_current_widget();
        });

    QObject::connect(slider, &TouchSlider::canceled, [&]() {
        main_widget->pop_current_widget();
        });

}


IntConfigUI::IntConfigUI(std::string name, MainWidget* parent, int* config_location, int min_value_, int max_value_) : ConfigUI(name, parent) {

    min_value = min_value_;
    max_value = max_value_;
    int_location = config_location;

    int current_value = static_cast<int>((*config_location - min_value) / (max_value - min_value) * 100);
    slider = new TouchSlider(min_value, max_value, current_value, this);
    QObject::connect(slider, &TouchSlider::itemSelected, [&](int val) {
        *int_location = val;
        on_change();
        main_widget->invalidate_render();
        main_widget->pop_current_widget();
        });

    QObject::connect(slider, &TouchSlider::canceled, [&]() {
        main_widget->pop_current_widget();
        });

}


PageSelectorUI::PageSelectorUI(MainWidget* parent, int current, int num_pages) : ConfigUI("", parent) {

    page_selector = new TouchPageSelector(0, num_pages - 1, current, this);

    QObject::connect(page_selector, &TouchPageSelector::pageSelected, [&](int val) {
        main_widget->goto_page_with_page_number(val);
        main_widget->invalidate_render();
        });

}

void PageSelectorUI::resizeEvent(QResizeEvent* resize_event) {
    QWidget::resizeEvent(resize_event);
    int parent_width = parentWidget()->width();
    int parent_height = parentWidget()->height();

    int w = 2 * parent_width / 3;
    int h = parent_height / 6;
    page_selector->resize(w, h);

    setFixedSize(w, h);
    move((parent_width - w) / 2, parent_height - 2 * h);
}

AudioUI::AudioUI(MainWidget* parent) : ConfigUI("", parent) {

    buttons = new TouchAudioButtons(this);
    buttons->set_rate(TTS_RATE);

    QObject::connect(buttons, &TouchAudioButtons::playPressed, [&]() {
        //main_widget->main_document_view->goto_page(val);
        //main_widget->invalidate_render();
        main_widget->handle_play();
        });

    QObject::connect(buttons, &TouchAudioButtons::stopPressed, [&]() {
        //main_widget->main_document_view->goto_page(val);
        //main_widget->invalidate_render();
        main_widget->handle_stop_reading();
        });

    QObject::connect(buttons, &TouchAudioButtons::pausePressed, [&]() {
        //main_widget->main_document_view->goto_page(val);
        //main_widget->invalidate_render();
        main_widget->handle_pause();
        });


    QObject::connect(buttons, &TouchAudioButtons::rateChanged, [&](qreal rate) {
        TTS_RATE = rate;
        buttons->set_rate(TTS_RATE);
        main_widget->handle_pause();
        main_widget->handle_play();
        });
    //QObject::connect(buttons, &TouchAudioButtons::speedIncreasePressed, [&](){
    //    TTS_RATE += 0.1;
    //    if (TTS_RATE > 1.0f) {
    //        TTS_RATE = 1.0f;
    //    }
    //    buttons->set_rate(TTS_RATE);
    //});

    //QObject::connect(buttons, &TouchAudioButtons::speedDecreasePressed, [&](){
    //    TTS_RATE -= 0.1;
    //    if (TTS_RATE < -1.0f) {
    //        TTS_RATE = -1.0f;
    //    }
    //    buttons->set_rate(TTS_RATE);
    //});
}

void AudioUI::resizeEvent(QResizeEvent* resize_event) {
    QWidget::resizeEvent(resize_event);
    int parent_width = parentWidget()->width();
    int parent_height = parentWidget()->height();

    int w = parent_width;
    int h = parent_height / 6;
    buttons->resize(w, h);

    setFixedSize(w, h);
    move((parent_width - w) / 2, parent_height - h);
}



BoolConfigUI::BoolConfigUI(std::string name_, MainWidget* parent, bool* config_location, QString qname) : ConfigUI(name_, parent) {
    bool_location = config_location;

    checkbox = new TouchCheckbox(qname, *config_location, this);
    QObject::connect(checkbox, &TouchCheckbox::itemSelected, [&](bool new_state) {
        *bool_location = static_cast<bool>(new_state);
        main_widget->invalidate_render();
        on_change();
        if (should_persist) {
            main_widget->persist_config();
        }

        main_widget->pop_current_widget();
        });

    QObject::connect(checkbox, &TouchCheckbox::canceled, [&]() {
        main_widget->pop_current_widget();
        });
}

void BoolConfigUI::resizeEvent(QResizeEvent* resize_event) {
    QWidget::resizeEvent(resize_event);
    int parent_width = parentWidget()->width();
    int parent_height = parentWidget()->height();

    int five_cm = static_cast<int>(12 * logicalDpiX() / 2.54f);

    int w = 2 * parent_width / 3;
    int h = parent_height / 2;

    w = std::min(w, five_cm);
    h = std::min(h, five_cm);

    checkbox->resize(w, h);

    setFixedSize(w, h);
    move((parent_width - w) / 2, (parent_height - h) / 2);
}

void MacroConfigUI::resizeEvent(QResizeEvent* resize_event) {
    QWidget::resizeEvent(resize_event);
    int parent_width = parentWidget()->width();
    int parent_height = parentWidget()->height();

    int w = 2 * parent_width / 3;
    int h = parent_height / 2;
    macro_editor->resize(w, h);

    setFixedSize(w, h);
    move(parent_width / 6, parent_height / 4);
}

void FloatConfigUI::resizeEvent(QResizeEvent* resize_event) {
    QWidget::resizeEvent(resize_event);
    int parent_width = parentWidget()->width();
    int parent_height = parentWidget()->height();

    int w = 2 * parent_width / 3;
    int h = parent_height / 2;
    slider->resize(w, h);

    setFixedSize(w, h);
    move(parent_width / 6, parent_height / 4);
}

void IntConfigUI::resizeEvent(QResizeEvent* resize_event) {
    QWidget::resizeEvent(resize_event);
    int parent_width = parentWidget()->width();
    int parent_height = parentWidget()->height();

    int w = 2 * parent_width / 3;
    int h = parent_height / 2;
    slider->resize(w, h);

    setFixedSize(w, h);
    move(parent_width / 6, parent_height / 4);
}

//QWidget* color3_configurator_ui(MainWidget* main_widget, void* location){
//    return new Color3ConfigUI(main_widget, (float*)location);
//}

//QWidget* color4_configurator_ui(MainWidget* main_widget, void* location){
//    return new Color4ConfigUI(main_widget, (float*)location);
//}


TouchCommandSelector::TouchCommandSelector(bool is_fuzzy, const QStringList& commands, MainWidget* mw) : QWidget(mw) {
    main_widget = mw;
    list_view = new TouchListView(is_fuzzy, commands, -1, this);

    QObject::connect(list_view, &TouchListView::itemSelected, [&](QString val, int index) {
        //main_widget->current_widget = nullptr;
        main_widget->pop_current_widget();
        main_widget->on_command_done(val.toStdString(), val.toStdString());
        //deleteLater();
        });
}

void TouchCommandSelector::resizeEvent(QResizeEvent* resize_event) {
    QWidget::resizeEvent(resize_event);

    int parent_width = parentWidget()->size().width();
    int parent_height = parentWidget()->size().height();

    resize(parent_width * 0.9f, parent_height);
    move(parent_width * 0.05f, 0);
    list_view->resize(parent_width * 0.9f, parent_height);
}


RectangleConfigUI::RectangleConfigUI(std::string name, MainWidget* parent, UIRect* config_location) : ConfigUI(name, parent) {

    rect_location = config_location;

    bool current_enabled = config_location->enabled;

    float current_left = config_location->left;
    float current_right = config_location->right;
    float current_top = config_location->top;
    float current_bottom = config_location->bottom;

    //    layout = new QVBoxLayout();

    rectangle_select_ui = new TouchRectangleSelectUI(current_enabled,
        current_left,
        current_top,
        (current_right - current_left) / 2.0f,
        (current_bottom - current_top) / 2.0f,
        this);


    //    int current_value = static_cast<int>((*config_location - min_value) / (max_value - min_value) * 100);
    //    slider = new TouchSlider(0, 100, current_value, this);
    QObject::connect(rectangle_select_ui, &TouchRectangleSelectUI::rectangleSelected, [&](bool enabled, qreal left, qreal right, qreal top, qreal bottom) {

        rect_location->enabled = enabled;
        rect_location->left = left;
        rect_location->right = right;
        rect_location->top = top;
        rect_location->bottom = bottom;

        on_change();
        if (should_persist) {
            main_widget->persist_config();
        }
        main_widget->invalidate_render();
        //main_widget->current_widget = nullptr;
        //deleteLater();
        main_widget->pop_current_widget();
        });

}

//bool RectangleConfigUI::eventFilter(QObject *obj, QEvent *event){
//    if ((obj == parentWidget()) && (event->type() == QEvent::Type::Resize)){

//        QResizeEvent* resize_event = static_cast<QResizeEvent*>(event);
//        int parent_width = resize_event->size().width();
//        int parent_height = resize_event->size().height();

//        bool res = QObject::eventFilter(obj, event);

//        qDebug() << "resizing to " << parent_width << " " << parent_height;
//        rectangle_select_ui->resize(parent_width, parent_height);
//        setFixedSize(parent_width, parent_height);

////        setFixedSize(parent_width, parent_height);
//        move(0, 0);
//        return res;
//    }
//    else{
//        return QObject::eventFilter(obj, event);
//    }
//}

void RectangleConfigUI::resizeEvent(QResizeEvent* resize_event) {
    QWidget::resizeEvent(resize_event);
    move(0, 0);
    //    rectangle_select_ui->resize(resize_event->size().width(), resize_event->size().height());
    setFixedSize(parentWidget()->size());
    rectangle_select_ui->resize(parentWidget()->size());

}

RangeConfigUI::RangeConfigUI(std::string name, MainWidget* parent, float* top_config_location, float* bottom_config_location) : ConfigUI(name, parent) {

    //    range_location = config_location;
    top_location = top_config_location;
    bottom_location = bottom_config_location;

    float current_top = -(*top_location);
    float current_bottom = -(*bottom_location) + 1;


    range_select_ui = new TouchRangeSelectUI(current_top, current_bottom, this);

    // force a resize event in order to have correct sizes
    resize(parent->width(), parent->height());


    QObject::connect(range_select_ui, &TouchRangeSelectUI::rangeSelected, [&](qreal top, qreal bottom) {

        *top_location = -top;
        *bottom_location = -bottom + 1;

        on_change();
        if (should_persist) {
            main_widget->persist_config();
        }
        main_widget->invalidate_render();
        //main_widget->current_widget = nullptr;
        //deleteLater();
        main_widget->pop_current_widget();
        });

    QObject::connect(range_select_ui, &TouchRangeSelectUI::rangeCanceled, [&]() {
        main_widget->invalidate_render();
        //main_widget->current_widget = nullptr;
        //deleteLater();
        main_widget->pop_current_widget();
        });

}

void RangeConfigUI::resizeEvent(QResizeEvent* resize_event) {
    QWidget::resizeEvent(resize_event);
    move(0, 0);
    range_select_ui->resize(resize_event->size().width(), resize_event->size().height());

}
QList<QStandardItem*> CommandSelector::get_item(std::string command_name) {

    std::string command_key = "";

    if (key_map.find(command_name) != key_map.end()) {
        const std::vector<std::string>& command_keys = key_map[command_name];
        for (size_t i = 0; i < command_keys.size(); i++) {
            const std::string& ck = command_keys[i];
            if (i > 0) {
                command_key += " | ";
            }
            command_key += ck;
        }

    }
    QStandardItem* name_item = new QStandardItem(QString::fromStdString(command_name));
    QStandardItem* key_item = new QStandardItem(QString::fromStdString(command_key));
    key_item->setTextAlignment(Qt::AlignVCenter | Qt::AlignRight);
    return (QList<QStandardItem*>() << name_item << key_item);
}

QStandardItemModel* CommandSelector::get_standard_item_model(std::vector<std::string> command_names) {

    QStandardItemModel* res = new QStandardItemModel();

    for (size_t i = 0; i < command_names.size(); i++) {
        res->appendRow(get_item(command_names[i]));
    }
    return res;
}

QStandardItemModel* CommandSelector::get_standard_item_model(QStringList command_names) {

    std::vector<std::string> std_command_names;

    for (int i = 0; i < command_names.size(); i++) {
        std_command_names.push_back(command_names.at(i).toStdString());
    }
    return get_standard_item_model(std_command_names);
}


QString CommandSelector::get_view_stylesheet_type_name() {
    return "QTableView";
}

void CommandSelector::on_select(const QModelIndex& index) {
    bool is_numeric = false;
    line_edit->text().toInt(&is_numeric);
    std::string query = line_edit->text().toStdString();
    QString name = standard_item_model->data(index).toString();
    //hide();
    main_widget->pop_current_widget();
    parentWidget()->setFocus();
    if (!is_numeric) {
        (*on_done)(name.toStdString(), query);
    }
    else {
        (*on_done)(line_edit->text().toStdString(), query);
    }
}

CommandSelector::~CommandSelector() {
    fzf_free_slab(slab);
}

CommandSelector::CommandSelector(bool is_fuzzy, std::function<void(std::string, std::string)>* on_done,
    MainWidget* parent,
    QStringList elements,
    std::unordered_map<std::string,
    std::vector<std::string>> key_map) : BaseSelectorWidget(new QTableView(), is_fuzzy, nullptr, parent),
    key_map(key_map),
    on_done(on_done),
    main_widget(parent)
{
    slab = fzf_make_default_slab();
    string_elements = elements;
    standard_item_model = get_standard_item_model(string_elements);

    QTableView* table_view = dynamic_cast<QTableView*>(get_view());

    table_view->setSelectionMode(QAbstractItemView::SingleSelection);
    table_view->setSelectionBehavior(QAbstractItemView::SelectRows);
    table_view->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table_view->setModel(standard_item_model);
    table_view->setCurrentIndex(standard_item_model->index(0, 0));


    table_view->horizontalHeader()->setStretchLastSection(true);
    table_view->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    table_view->horizontalHeader()->hide();
    table_view->verticalHeader()->hide();

}

bool CommandSelector::on_text_change(const QString& text) {

    std::vector<std::string> matching_element_names;
    std::vector<int> scores;
    std::string search_text_string = text.toStdString();
    std::vector<std::pair<std::string, int>> match_score_pairs;

    std::string pattern_str = text.toStdString();
    fzf_pattern_t* pattern = fzf_parse_pattern(CaseSmart, false, (char*)pattern_str.c_str(), true);

    QString actual_text = text;
    if (actual_text.endsWith("?")){
        actual_text = actual_text.left(actual_text.size()-1);
        search_text_string = actual_text.toStdString();
    }
    if (actual_text.startsWith("?")){
        actual_text = actual_text.right(actual_text.size()-1);
        search_text_string = actual_text.toStdString();
    }

    for (int i = 0; i < string_elements.size(); i++) {
        std::string encoded = utf8_encode(string_elements.at(i).toStdWString());
        int score = 0;
        if (is_fuzzy) {
            //score = static_cast<int>(rapidfuzz::fuzz::partial_ratio(search_text_string, encoded));
            score = fzf_get_score(encoded.c_str(), pattern, slab);
        }
        else {
            fts::fuzzy_match(search_text_string.c_str(), encoded.c_str(), score);
        }
        match_score_pairs.push_back(std::make_pair(encoded, score));
    }
    std::sort(match_score_pairs.begin(), match_score_pairs.end(), [](std::pair<std::string, int> lhs, std::pair<std::string, int> rhs) {
        return lhs.second > rhs.second;
        });

    for (int i = 0; i < string_elements.size(); i++) {
        if (string_elements.at(i).startsWith(actual_text)) {
            matching_element_names.push_back(string_elements.at(i).toStdString());
        }
    }

    //if (matching_element_names.size() == 0) {
    for (auto [command, score] : match_score_pairs) {
        if (score > 60 && (!QString::fromStdString(command).startsWith(actual_text))) {
            matching_element_names.push_back(command);
        }
    }
    //}

    fzf_free_pattern(pattern);

    QStandardItemModel* new_standard_item_model = get_standard_item_model(matching_element_names);
    dynamic_cast<QTableView*>(get_view())->setModel(new_standard_item_model);
    delete standard_item_model;
    standard_item_model = new_standard_item_model;
    dynamic_cast<QTableView*>(get_view())->setCurrentIndex(standard_item_model->index(0, 0));

    return true;
}

BaseSelectorWidget::BaseSelectorWidget(QAbstractItemView* item_view, bool fuzzy, QStandardItemModel* item_model, QWidget* parent) : QWidget(parent) {

    bool is_tree = dynamic_cast<QTreeView*>(item_view) != nullptr;
    is_fuzzy = fuzzy;
    proxy_model = new MySortFilterProxyModel(fuzzy, is_tree);
    proxy_model->setFilterCaseSensitivity(Qt::CaseSensitivity::CaseInsensitive);

    if (item_model) {
        proxy_model->setSourceModel(item_model);
    }

    resize(300, 800);
    QVBoxLayout* layout = new QVBoxLayout;
    setLayout(layout);

    line_edit = new MyLineEdit;
    abstract_item_view = item_view;
    abstract_item_view->setParent(this);
    abstract_item_view->setModel(proxy_model);
    abstract_item_view->setEditTriggers(QAbstractItemView::NoEditTriggers);

    if (TOUCH_MODE) {
        QScroller::grabGesture(abstract_item_view->viewport(), QScroller::TouchGesture);
        abstract_item_view->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
        QObject::connect(abstract_item_view, &QListView::pressed, [&](const QModelIndex& index) {
            pressed_row = index.row();
            pressed_pos = QCursor::pos();
            });

        QObject::connect(abstract_item_view, &QListView::clicked, [&](const QModelIndex& index) {
            QPoint current_pos = QCursor::pos();
            if (index.row() == pressed_row) {
                if ((current_pos - pressed_pos).manhattanLength() < 10) {
                    on_select(index);
                }
            }
            });
    }

    QTreeView* tree_view = dynamic_cast<QTreeView*>(abstract_item_view);

    if (tree_view) {
        int n_columns = item_model->columnCount();
        tree_view->expandAll();
        tree_view->setHeaderHidden(true);
        tree_view->resizeColumnToContents(0);
        tree_view->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    }
    if (proxy_model) {
        proxy_model->setRecursiveFilteringEnabled(true);
    }

    layout->addWidget(line_edit);
    layout->addWidget(abstract_item_view);

    line_edit->installEventFilter(this);
    line_edit->setFocus();

    if (!TOUCH_MODE) {
        QObject::connect(abstract_item_view, &QAbstractItemView::activated, [&](const QModelIndex& index) {
            on_select(index);
            });
    }

    QObject::connect(line_edit, &QLineEdit::textChanged, [&](const QString& text) {
        on_text_changed(text);
        });

    if (TOUCH_MODE) {
        QScroller::grabGesture(abstract_item_view, QScroller::TouchGesture);
        abstract_item_view->setHorizontalScrollMode(QAbstractItemView::ScrollMode::ScrollPerPixel);
        abstract_item_view->setVerticalScrollMode(QAbstractItemView::ScrollMode::ScrollPerPixel);
    }

    QString font_face;
    if (font_face.size() > 0) {
        abstract_item_view->setFont(get_ui_font_face_name());
        line_edit->setFont(get_ui_font_face_name());
    }
}

void BaseSelectorWidget::on_text_changed(const QString& text) {
    if (!on_text_change(text)) {
        // generic text change handling when we don't explicitly handle text change events
        //proxy_model->setFilterFixedString(text);
        proxy_model->setFilterCustom(text);
        QTreeView* t_view = dynamic_cast<QTreeView*>(get_view());
        if (t_view) {
            t_view->expandAll();
        }
        else {
            get_view()->setCurrentIndex(get_view()->model()->index(0, 0));
        }
    }
}

QAbstractItemView* BaseSelectorWidget::get_view() {
    return abstract_item_view;
}
void BaseSelectorWidget::on_delete(const QModelIndex& source_index, const QModelIndex& selected_index) {}
void BaseSelectorWidget::on_edit(const QModelIndex& source_index, const QModelIndex& selected_index) {}

void BaseSelectorWidget::on_return_no_select(const QString& text) {
    if (get_view()->model()->hasIndex(0, 0)) {
        on_select(get_view()->model()->index(0, 0));
    }
}

bool BaseSelectorWidget::on_text_change(const QString& text) {
    return false;
}

void BaseSelectorWidget::set_filter_column_index(int index) {
    proxy_model->setFilterKeyColumn(index);
}

std::optional<QModelIndex> BaseSelectorWidget::get_selected_index() {
    QModelIndexList selected_index_list = get_view()->selectionModel()->selectedIndexes();

    if (selected_index_list.size() > 0) {
        QModelIndex selected_index = selected_index_list.at(0);
        return selected_index;
    }
    return {};
}

std::wstring BaseSelectorWidget::get_selected_text() {
    return L"";
}

bool BaseSelectorWidget::eventFilter(QObject* obj, QEvent* event) {
    if (obj == line_edit) {
#ifdef SIOYEK_QT6
        if (event->type() == QEvent::KeyRelease) {
            QKeyEvent* key_event = static_cast<QKeyEvent*>(event);
            if (key_event->key() == Qt::Key_Delete) {
                handle_delete();
            }
            else if (key_event->key() == Qt::Key_Insert) {
                handle_edit();
            }
        }
#endif
        if (event->type() == QEvent::InputMethod) {
            if (TOUCH_MODE) {
                QInputMethodEvent* input_event = static_cast<QInputMethodEvent*>(event);
                QString text = input_event->preeditString();
                if (input_event->commitString().size() > 0) {
                    text = input_event->commitString();
                }
                if (text.size() > 0) {
                    on_text_changed(text);
                }
            }
        }
        if ((event->type() == QEvent::KeyPress)) {
            QKeyEvent* key_event = static_cast<QKeyEvent*>(event);
            bool is_control_pressed = key_event->modifiers().testFlag(Qt::ControlModifier) || key_event->modifiers().testFlag(Qt::MetaModifier);
            bool is_alt_pressed = key_event->modifiers().testFlag(Qt::AltModifier);

            if (TOUCH_MODE) {
                if (key_event->key() == Qt::Key_Back) {
                    return false;
                }
            }
            if (key_event->key() == Qt::Key_Down ||
                key_event->key() == Qt::Key_Up ||
                key_event->key() == Qt::Key_Left ||
                key_event->key() == Qt::Key_Right
                ) {
#ifdef SIOYEK_QT6
                QKeyEvent* newEvent = key_event->clone();
#else
                QKeyEvent* newEvent = new QKeyEvent(*key_event);
#endif
                QCoreApplication::postEvent(get_view(), newEvent);
                //QCoreApplication::postEvent(tree_view, key_event);
                return true;
            }
            if (key_event->key() == Qt::Key_Tab) {
                QKeyEvent* new_key_event = new QKeyEvent(key_event->type(), Qt::Key_Down, key_event->modifiers());
                QCoreApplication::postEvent(get_view(), new_key_event);
                return true;
            }
            if (EMACS_MODE) {
                if (((key_event->key() == Qt::Key_V)) && is_control_pressed) {
                    QKeyEvent* new_key_event = new QKeyEvent(key_event->type(), Qt::Key_Up, key_event->modifiers());
                    QCoreApplication::postEvent(get_view(), new_key_event);
                    return true;
                }
                if (((key_event->key() == Qt::Key_V)) && is_alt_pressed) {
                    QKeyEvent* new_key_event = new QKeyEvent(key_event->type(), Qt::Key_Down, key_event->modifiers());
                    QCoreApplication::postEvent(get_view(), new_key_event);
                    return true;
                }
            }
            if (((key_event->key() == Qt::Key_N) || (key_event->key() == Qt::Key_J)) && is_control_pressed) {
                simulate_move_down();
                return true;
            }
            if (((key_event->key() == Qt::Key_P) || (key_event->key() == Qt::Key_K)) && (is_control_pressed && !EMACS_MODE)) {
                simulate_move_up();
                return true;
            }
            if ((key_event->key() == Qt::Key_J) && is_alt_pressed) {
                QKeyEvent* new_key_event = new QKeyEvent(key_event->type(), Qt::Key_End, Qt::KeyboardModifier::NoModifier);
                QCoreApplication::postEvent(get_view(), new_key_event);
                return true;
            }
            if ((key_event->key() == Qt::Key_K) && is_alt_pressed) {
                QKeyEvent* new_key_event = new QKeyEvent(key_event->type(), Qt::Key_Home, Qt::KeyboardModifier::NoModifier);
                QCoreApplication::postEvent(get_view(), new_key_event);
                return true;
            }
            if ((key_event->key() == Qt::Key_PageDown)) {
                QKeyEvent* new_key_event = new QKeyEvent(key_event->type(), Qt::Key_PageDown, key_event->modifiers());
                QCoreApplication::postEvent(get_view(), new_key_event);
                return true;
            }
            if ((key_event->key() == Qt::Key_PageUp)) {
                QKeyEvent* new_key_event = new QKeyEvent(key_event->type(), Qt::Key_PageUp, key_event->modifiers());
                QCoreApplication::postEvent(get_view(), new_key_event);
                return true;
            }
            if (key_event->key() == Qt::Key_Backtab) {
                QKeyEvent* new_key_event = new QKeyEvent(key_event->type(), Qt::Key_Up, key_event->modifiers());
                QCoreApplication::postEvent(get_view(), new_key_event);
                return true;
            }
            if (((key_event->key() == Qt::Key_C) && is_control_pressed)) {
                std::wstring text = get_selected_text();
                if (text.size() > 0) {
                    copy_to_clipboard(text);
                }
                return true;
            }
            if (key_event->key() == Qt::Key_Return || key_event->key() == Qt::Key_Enter) {
                std::optional<QModelIndex> selected_index = get_selected_index();
                if (selected_index) {
                    on_select(selected_index.value());
                }
                else {
                    on_return_no_select(line_edit->text());
                }
                return true;
            }

        }
    }
    return false;
}

void BaseSelectorWidget::simulate_move_down() {
    QModelIndex next_index = get_view()->model()->index(get_view()->currentIndex().row() + 1, 0);
    int nrows = get_view()->model()->rowCount();

    if (next_index.row() > nrows || next_index.row() < 0) {
        next_index = get_view()->model()->index(0, 0);
    }

    get_view()->setCurrentIndex(next_index);
    get_view()->scrollTo(next_index, QAbstractItemView::ScrollHint::EnsureVisible);
}

void BaseSelectorWidget::simulate_move_up() {
    QModelIndex next_index = get_view()->model()->index(get_view()->currentIndex().row() - 1, 0);
    int nrows = get_view()->model()->rowCount();

    if (next_index.row() > nrows || next_index.row() < 0) {
        next_index = get_view()->model()->index(get_view()->model()->rowCount() - 1, 0);
    }

    get_view()->setCurrentIndex(next_index);
    get_view()->scrollTo(next_index, QAbstractItemView::ScrollHint::EnsureVisible);
}

QString BaseSelectorWidget::get_selected_item() {
    if (get_selected_index()) {
        return get_view()->model()->data(get_selected_index().value()).toString();
    }
    return "";
}

void BaseSelectorWidget::simulate_select() {
    std::optional<QModelIndex> selected_index = get_selected_index();
    if (selected_index) {
        on_select(selected_index.value());
    }
    else {
        on_return_no_select(line_edit->text());
    }
}

void BaseSelectorWidget::handle_delete() {
    QModelIndexList selected_index_list = get_view()->selectionModel()->selectedIndexes();
    if (selected_index_list.size() > 0) {
        QModelIndex selected_index = selected_index_list.at(0);
        if (proxy_model->hasIndex(selected_index.row(), selected_index.column())) {
            QModelIndex source_index = proxy_model->mapToSource(selected_index);
            on_delete(source_index, selected_index);
        }
    }
}

void BaseSelectorWidget::handle_edit() {
    QModelIndexList selected_index_list = get_view()->selectionModel()->selectedIndexes();
    if (selected_index_list.size() > 0) {
        QModelIndex selected_index = selected_index_list.at(0);
        if (proxy_model->hasIndex(selected_index.row(), selected_index.column())) {
            QModelIndex source_index = proxy_model->mapToSource(selected_index);
            on_edit(source_index, selected_index);
        }
    }
}

#ifndef SIOYEK_QT6
    void BaseSelectorWidget::keyReleaseEvent(QKeyEvent* event) {
        if (event->key() == Qt::Key_Delete) {
            handle_delete();
        }
        QWidget::keyReleaseEvent(event);
    }
#endif

void BaseSelectorWidget::on_config_file_changed() {
    QString font_size_stylesheet = "";
    if (FONT_SIZE > 0) {
        font_size_stylesheet = QString("font-size: %1px").arg(FONT_SIZE);
    }

    setStyleSheet(get_ui_stylesheet(true) + font_size_stylesheet);
    get_view()->setStyleSheet(get_view_stylesheet_type_name() + "::item::selected{" + get_selected_stylesheet() + "}");
}

void BaseSelectorWidget::resizeEvent(QResizeEvent* resize_event) {
    QWidget::resizeEvent(resize_event);
    int parent_width = parentWidget()->width();
    int parent_height = parentWidget()->height();
    setFixedSize(parent_width * MENU_SCREEN_WDITH_RATIO, parent_height);
    move(parent_width * (1 - MENU_SCREEN_WDITH_RATIO) / 2, 0);
    on_config_file_changed();
}


MyLineEdit::MyLineEdit(QWidget* parent) : QLineEdit(parent) {
}

int MyLineEdit::get_next_word_position() {
    int current_position = cursorPosition();

    int whitespace_position = text().indexOf(QRegularExpression("\\s"), current_position);
    if (whitespace_position == -1) {
        return text().size();
    }
    int next_word_position = text().indexOf(QRegularExpression("\\S"), whitespace_position);

    if (next_word_position == -1) {
        return text().size();
    }
    return next_word_position;
}

int MyLineEdit::get_prev_word_position() {
    int current_position = qMax(cursorPosition() -1, 0);

    int whitespace_position = text().lastIndexOf(QRegularExpression("\\s"), current_position);
    if (whitespace_position == -1) {
        return 0;
    }
    int prev_word_position = text().lastIndexOf(QRegularExpression("\\S"), whitespace_position);

    if (prev_word_position == -1) {
        return 0;
    }

    return qMin(prev_word_position + 1, text().size());
}

void MyLineEdit::keyPressEvent(QKeyEvent* event) {

    if ((event->key() == Qt::Key_Up)) {
        emit prev_suggestion();
        event->accept();
        return;
    }

    if ((event->key() == Qt::Key_Down)) {
        emit next_suggestion();
        event->accept();
        return;
    }

    if (!EMACS_MODE) {
        return QLineEdit::keyPressEvent(event);
    }
    else {
        bool is_alt_pressed = event->modifiers() & Qt::AltModifier;
        bool is_control_pressed = event->modifiers() & Qt::ControlModifier;

        if (is_control_pressed && (event->key() == Qt::Key_A)) {
            setCursorPosition(0);
            event->accept();
            return;
        }

        if (is_control_pressed && (event->key() == Qt::Key_E)) {
            setCursorPosition(text().size());
            event->accept();
            return;
        }

        if (is_control_pressed && (event->key() == Qt::Key_K)) {
            QString current_text = text();
            setText(current_text.left(cursorPosition()));
            event->accept();
            return;
        }

        if (is_control_pressed && (event->key() == Qt::Key_F)) {
            int next_position = cursorPosition() + 1;
            if (next_position <= text().size()) {
                setCursorPosition(next_position);
            }
            event->accept();
            return;
        }

        if (is_control_pressed && (event->key() == Qt::Key_B)) {
            int next_position = cursorPosition() - 1;
            if (next_position >= 0) {
                setCursorPosition(next_position);
            }
            event->accept();
            return;
        }

        if (is_control_pressed && (event->key() == Qt::Key_H)) {
            int current_position = cursorPosition();
            if (current_position > 0) {
                QString new_text = text().left(current_position - 1) + text().right(text().size() - current_position);
                setText(new_text);
                setCursorPosition(current_position - 1);
            }
            event->accept();
            return;
        }

        if (is_control_pressed && (event->key() == Qt::Key_W)) {
            int current_position = cursorPosition();
            if (current_position > 0) {
                int prev_word_position = get_prev_word_position();
                QString new_text = text().left(prev_word_position) + text().right(text().size() - current_position);
                setText(new_text);
                setCursorPosition(prev_word_position);
            }
            event->accept();
            return;
        }

        if (is_control_pressed && (event->key() == Qt::Key_D)) {
            int current_position = cursorPosition();
            if (current_position < text().size()) {
                QString new_text = text().left(current_position) + text().right(text().size() - current_position - 1);
                setText(new_text);
                setCursorPosition(current_position);
            }
            event->accept();
            return;
        }

        if (is_alt_pressed && (event->key() == Qt::Key_F)) {
            setCursorPosition(get_next_word_position());
            event->accept();
            return;
        }

        if (is_alt_pressed && (event->key() == Qt::Key_B)) {
            setCursorPosition(get_prev_word_position());
            event->accept();
            return;
        }
        QLineEdit::keyPressEvent(event);
    }

}
