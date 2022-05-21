
#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <optional>
#include <memory>


#include <qscrollarea.h>
#include <qabstractitemmodel.h>
#include <qapplication.h>
#include <qboxlayout.h>
#include <qdatetime.h>
#include <qdesktopwidget.h>
#include <qkeyevent.h>
#include <qlabel.h>
#include <qlineedit.h>
#include <qlistview.h>
#include <qopenglfunctions.h>
#include <qpushbutton.h>
#include <qsortfilterproxymodel.h>
#include <qstringlistmodel.h>
#include <qtextedit.h>
#include <qtimer.h>
#include <qtreeview.h>
#include <qwindow.h>
#include <qstandardpaths.h>
#include <qprocess.h>
#include <qguiapplication.h>
#include <qmimedata.h>


#include "input.h"
#include "database.h"
#include "book.h"
#include "utils.h"
#include "ui.h"
#include "pdf_renderer.h"
#include "document.h"
#include "document_view.h"
#include "pdf_view_opengl_widget.h"
#include "config.h"
#include "utf8.h"
#include "synctex/synctex_parser.h"
#include "path.h"

#include "main_widget.h"

#ifndef MAX_PATH
#define MAX_PATH PATH_MAX
#endif

extern bool SHOULD_USE_MULTIPLE_MONITORS;
extern bool SORT_BOOKMARKS_BY_LOCATION;
extern bool FLAT_TABLE_OF_CONTENTS;
extern bool HOVER_OVERVIEW;
extern bool WHEEL_ZOOM_ON_CURSOR;
extern float MOVE_SCREEN_PERCENTAGE;
extern std::wstring LIBGEN_ADDRESS;
extern std::wstring INVERSE_SEARCH_COMMAND;
extern float VISUAL_MARK_NEXT_PAGE_FRACTION;
extern float VISUAL_MARK_NEXT_PAGE_THRESHOLD;
extern float SMALL_PIXMAP_SCALE;
extern std::wstring EXECUTE_COMMANDS[26];
extern int STATUS_BAR_FONT_SIZE;
extern Path default_config_path;
extern Path default_keys_path;
extern std::vector<Path> user_config_paths;
extern std::vector<Path> user_keys_paths;
extern Path database_file_path;
extern Path tutorial_path;
extern Path last_opened_file_address_path;
extern Path auto_config_path;
extern std::wstring ITEM_LIST_PREFIX;
extern std::wstring SEARCH_URLS[26];
extern std::wstring MIDDLE_CLICK_SEARCH_ENGINE;
extern std::wstring SHIFT_MIDDLE_CLICK_SEARCH_ENGINE;
extern float DISPLAY_RESOLUTION_SCALE;
extern float STATUS_BAR_COLOR[3];
extern float STATUS_BAR_TEXT_COLOR[3];
extern int MAIN_WINDOW_SIZE[2];
extern int HELPER_WINDOW_SIZE[2];
extern int MAIN_WINDOW_MOVE[2];
extern int HELPER_WINDOW_MOVE[2];
extern float TOUCHPAD_SENSITIVITY;
extern int SINGLE_MAIN_WINDOW_SIZE[2];
extern int SINGLE_MAIN_WINDOW_MOVE[2];
extern float OVERVIEW_SIZE[2];
extern float OVERVIEW_OFFSET[2];
extern bool IGNORE_WHITESPACE_IN_PRESENTATION_MODE;
extern std::vector<MainWidget*> windows;

bool MainWidget::main_document_view_has_document()
{
    return (main_document_view != nullptr) && (doc() != nullptr);
}

void MainWidget::resizeEvent(QResizeEvent* resize_event) {
    QWidget::resizeEvent(resize_event);

    main_window_width = size().width();
    main_window_height = size().height();

    if (text_command_line_edit_container != nullptr) {
        text_command_line_edit_container->move(0, 0);
        text_command_line_edit_container->resize(main_window_width, 30);
    }

    if (status_label != nullptr) {
        int status_bar_height = get_status_bar_height();
        status_label->move(0, main_window_height - status_bar_height);
        status_label->resize(main_window_width, status_bar_height);
        status_label->show();
    }

    if ((main_document_view->get_document() != nullptr) && (main_document_view->get_zoom_level() == 0)) {
        main_document_view->fit_to_page_width();
        update_current_history_index();
    }

}


void MainWidget::set_overview_position(int page, float offset) {
    if (page >= 0) {
		float page_height = main_document_view->get_document()->get_page_height(page);
		opengl_widget->set_overview_page(OverviewState{ page, offset, page_height });
		invalidate_render();
    }
}

void MainWidget::set_overview_link(PdfLink link) {

    auto [page, offset_x, offset_y] = parse_uri(link.uri);
    if (page >= 1) {
        set_overview_position(page - 1, offset_y);
    }
}

void MainWidget::mouseMoveEvent(QMouseEvent* mouse_event) {

    if (is_rotated()) {
        // we don't handle mouse events while document is rotated becausae proper handling
        // would increase the code complexity too much to be worth it
        return;
    }

    //int x = mouse_event->pos().x();
    //int y = mouse_event->pos().y();
    WindowPos mpos = { mouse_event->pos().x(), mouse_event->pos().y() };

    std::optional<PdfLink> link = {};

    NormalizedWindowPos normal_mpos = main_document_view->window_to_normalized_window_pos(mpos);

    if (overview_resize_data) {
        // if we are resizing overview page, set the selected side of the overview window to the mosue position
        //float offset_diff_x = normal_x - overview_resize_data.value().original_mouse_pos.first;
        //float offset_diff_y = normal_y - overview_resize_data.value().original_mouse_pos.second;
        fvec2 offset_diff = fvec2(normal_mpos) - fvec2(overview_resize_data.value().original_normal_mouse_pos);
        opengl_widget->set_overview_side_pos(
            overview_resize_data.value().side_index,
            overview_resize_data.value().original_rect,
            offset_diff);
        validate_render();
        return;
    }

    if (overview_move_data) {
        fvec2 offset_diff = fvec2(normal_mpos) - fvec2(overview_move_data.value().original_normal_mouse_pos);
        offset_diff[1] = -offset_diff[1];
        fvec2 new_offsets = overview_move_data.value().original_offsets + offset_diff;
        opengl_widget->set_overview_offsets(new_offsets);
        validate_render();
        return;
    }

    if (opengl_widget->is_window_point_in_overview(normal_mpos)) {
        link = doc()->get_link_in_pos(opengl_widget->window_pos_to_overview_pos(normal_mpos));
        if (link) {
			setCursor(Qt::PointingHandCursor);
        }
        else {
			setCursor(Qt::ArrowCursor);
        }
        return;
    }

    if (main_document_view && (link = main_document_view->get_link_in_pos(mpos))) {
        // show hand cursor when hovering over links
        setCursor(Qt::PointingHandCursor);

        // if hover_overview config is set, we show an overview of links while hovering over them
        if (HOVER_OVERVIEW) {
            set_overview_link(link.value());
        }
    }
    else {
        setCursor(Qt::ArrowCursor);
        if (HOVER_OVERVIEW) {
            opengl_widget->set_overview_page({});
            invalidate_render();
        }
    }

    if (is_dragging) {
        ivec2 diff = ivec2(mpos) - ivec2(last_mouse_down_window_pos);

        fvec2 diff_doc = diff / main_document_view->get_zoom_level();

        main_document_view->set_offsets(last_mouse_down_document_offset.x + diff_doc.x(),
            last_mouse_down_document_offset.y - diff_doc.y());
        validate_render();
    }

    if (is_selecting) {

        // When selecting, we occasionally update selected text
        //todo: maybe have a timer event that handles this periodically
        if (last_text_select_time.msecsTo(QTime::currentTime()) > 16) {

            AbsoluteDocumentPos document_pos = main_document_view->window_to_absolute_document_pos(mpos);

            selection_begin = last_mouse_down;
            selection_end = document_pos;
            //fz_point selection_begin = { last_mouse_down.x(), last_mouse_down.y()};
            //fz_point selection_end = { document_x, document_y };

            main_document_view->get_text_selection(selection_begin,
                selection_end,
                is_word_selecting,
                opengl_widget->selected_character_rects,
                selected_text);

            validate_render();
            last_text_select_time = QTime::currentTime();
        }
    }

}

void MainWidget::persist() {
    main_document_view->persist();

    // write the address of the current document in a file so that the next time
    // we launch the application, we open this document
    if (main_document_view->get_document()) {
        std::ofstream last_path_file(last_opened_file_address_path.get_path_utf8());

        std::string encoded_file_name_str = utf8_encode(main_document_view->get_document()->get_path());
        last_path_file << encoded_file_name_str.c_str() << std::endl;
        last_path_file.close();
    }
}
void MainWidget::closeEvent(QCloseEvent* close_event) {
    handle_close_event();
}

MainWidget::MainWidget(MainWidget* other) : MainWidget(other->mupdf_context, other->db_manager, other->document_manager, other->config_manager, other->input_handler, other->checksummer, other->should_quit) {

}

MainWidget::MainWidget(fz_context* mupdf_context,
    DatabaseManager* db_manager,
    DocumentManager* document_manager,
    ConfigManager* config_manager,
    InputHandler* input_handler,
    CachedChecksummer* checksummer,
    bool* should_quit_ptr,
    QWidget* parent):
    QWidget(parent),
    mupdf_context(mupdf_context),
    db_manager(db_manager),
    document_manager(document_manager),
    config_manager(config_manager),
    input_handler(input_handler),
    checksummer(checksummer),
    should_quit(should_quit_ptr)
{
    setMouseTracking(true);
    setAcceptDrops(true);
    setAttribute(Qt::WA_DeleteOnClose);


    inverse_search_command = INVERSE_SEARCH_COMMAND;
    int first_screen_width = QApplication::desktop()->screenGeometry(0).width();

    if (DISPLAY_RESOLUTION_SCALE <= 0){
        pdf_renderer = new PdfRenderer(4, should_quit_ptr, mupdf_context, QApplication::desktop()->devicePixelRatioF());
    }
    else {
        pdf_renderer = new PdfRenderer(4, should_quit_ptr, mupdf_context, DISPLAY_RESOLUTION_SCALE);

    }
    pdf_renderer->start_threads();


    main_document_view = new DocumentView(mupdf_context, db_manager, document_manager, config_manager, checksummer);
    opengl_widget = new PdfViewOpenGLWidget(main_document_view, pdf_renderer, config_manager, false, this);

    helper_document_view = new DocumentView(mupdf_context, db_manager, document_manager, config_manager, checksummer);
    helper_opengl_widget = new PdfViewOpenGLWidget(helper_document_view, pdf_renderer, config_manager, true);

    status_label = new QLabel(this);
    status_label->setStyleSheet(get_status_stylesheet());
    status_label->setFont(QFont("Monaco"));

    // automatically open the helper window in second monitor
    int num_screens = QGuiApplication::screens().size();

    if ((num_screens > 1) && (HELPER_WINDOW_SIZE[0] > 0)) {
        apply_window_params_for_two_window_mode();
    }
    else {
        apply_window_params_for_one_window_mode();
    }

    helper_opengl_widget->register_on_link_edit_listener([this](OpenedBookState state) {
        this->update_closest_link_with_opened_book_state(state);
        });

    text_command_line_edit_container = new QWidget(this);
    text_command_line_edit_container->setStyleSheet("background-color: black; color: white; border: none;");

    QHBoxLayout* text_command_line_edit_container_layout = new QHBoxLayout();

    text_command_line_edit_label = new QLabel();
    text_command_line_edit = new QLineEdit();

    text_command_line_edit_label->setFont(QFont("Monaco"));
    text_command_line_edit->setFont(QFont("Monaco"));

    text_command_line_edit_container_layout->addWidget(text_command_line_edit_label);
    text_command_line_edit_container_layout->addWidget(text_command_line_edit);
    text_command_line_edit_container_layout->setContentsMargins(10, 0, 10, 0);

    text_command_line_edit_container->setLayout(text_command_line_edit_container_layout);
    text_command_line_edit_container->hide();

    on_command_done = [&](std::string command_name) {
        const Command* command = command_manager.get_command_with_name(command_name);
        handle_command_types(command, 1);
    };

    // when pdf renderer's background threads finish rendering a page or find a new search result
    // we need to update the ui
    QObject::connect(pdf_renderer, &PdfRenderer::render_advance, this, &MainWidget::invalidate_render);
    QObject::connect(pdf_renderer, &PdfRenderer::search_advance, this, &MainWidget::invalidate_ui);
    // we check periodically to see if the ui needs updating
    // this is done so that thousands of search results only trigger
    // a few rerenders
    // todo: make interval time configurable
    QTimer* timer = new QTimer(this);
    unsigned int INTERVAL_TIME = 200;
    timer->setInterval(INTERVAL_TIME);
    connect(timer, &QTimer::timeout, [&, INTERVAL_TIME]() {


        if (is_render_invalidated) {
            validate_render();
        }
        else if (is_ui_invalidated) {
            validate_ui();
        }
        // detect if the document file has changed and if so, reload the document
        if (main_document_view != nullptr) {
            Document* doc = nullptr;
            if ((doc = main_document_view->get_document()) != nullptr) {

                // Wait until a safe amount of time has passed since the last time the file was updated on the filesystem
                // this is because LaTeX software frequently puts PDF files in an invalid state while it is being made in
                // multiple passes.

                if ((doc->get_milies_since_last_edit_time() < (2 * INTERVAL_TIME)) &&
                    doc->get_milies_since_last_edit_time() > INTERVAL_TIME &&
                    doc->get_milies_since_last_document_update_time() > doc->get_milies_since_last_edit_time()
                    ) {

                //if (doc->get_milies_since_last_document_update_time() > doc->get_milies_since_last_edit_time()) {
                    doc->reload();
                    pdf_renderer->clear_cache();
                    invalidate_render();
                }
            }
        }
        });
    timer->start();


    QVBoxLayout* layout = new QVBoxLayout;
    layout->setSpacing(0);
    layout->setContentsMargins(0, 0, 0, 0);
    opengl_widget->setAttribute(Qt::WA_TransparentForMouseEvents);
    layout->addWidget(opengl_widget);
    setLayout(layout);

    setFocus();
}

MainWidget::~MainWidget() {
    for (int i = 0; i < windows.size(); i++) {
        if (windows[i] == this) {
            windows.erase(windows.begin() + i);
            break;
        }
    }

    if (windows.size() == 0) {
        *should_quit = true;
		pdf_renderer->join_threads();
    }

    // todo: use a reference counting pointer for document so we can delete main_doc
    // and helper_doc in DocumentView's destructor, not here.
    // ideally this function should just become:
    //		delete main_document_view;
    //		if(helper_document_view != main_document_view) delete helper_document_view;
    if (main_document_view != nullptr) {
        //Document* main_doc = main_document_view->get_document();
        //if (main_doc != nullptr) delete main_doc;

        //Document* helper_doc = nullptr;
        //if (helper_document_view != nullptr) {
        //    helper_doc = helper_document_view->get_document();
        //}
        //if (helper_doc != nullptr && helper_doc != main_doc) delete helper_doc;
    }

    if (helper_document_view != nullptr && helper_document_view != main_document_view) {
        delete helper_document_view;
    }
}

bool MainWidget::is_pending_link_source_filled() {
    return (pending_link && pending_link.value().first);
}

std::wstring MainWidget::get_status_string() {

    std::wstringstream ss;
    if (main_document_view->get_document() == nullptr) return L"";
    std::wstring chapter_name = main_document_view->get_current_chapter_name();

    ss << "Page " << get_current_page_number() + 1 << " / " << main_document_view->get_document()->num_pages();
    if (chapter_name.size() > 0) {
        ss << " [ " << chapter_name << " ] ";
    }
    int num_search_results = opengl_widget->get_num_search_results();
    float progress = -1;
    if (opengl_widget->get_is_searching(&progress)) {

        // show the 0th result if there are no results and the index + 1 otherwise
        int result_index = opengl_widget->get_num_search_results() > 0 ? opengl_widget->get_current_search_result_index() + 1 : 0;
        ss << " | showing result " << result_index << " / " << num_search_results;
        if (progress > 0) {
            ss << " (" << ((int)(progress * 100)) << "%%" << ")";
        }
    }
    if (is_pending_link_source_filled()) {
        ss << " | linking ...";
    }
    if (link_to_edit) {
        ss << " | editing link ...";
    }
    if (current_pending_command && current_pending_command->requires_symbol) {
        std::wstring wcommand_name = utf8_decode(current_pending_command->name.c_str());
        ss << " | " << wcommand_name << " waiting for symbol";
    }
    if (main_document_view != nullptr && main_document_view->get_document() != nullptr &&
        main_document_view->get_document()->get_is_indexing()) {
        ss << " | indexing ... ";
    }
    if (this->synctex_mode) {
        ss << " [ synctex ]";
    }
    if (this->mouse_drag_mode) {
        ss << " [ drag ]";
    }
    if (opengl_widget->is_presentation_mode()) {
        ss << " [ presentation ] ";
    }

    if (visual_scroll_mode) {
        ss << " [ visual scroll ] ";
    }

    if (horizontal_scroll_locked) {
        ss << " [ locked horizontal scroll ] ";
    }
    ss << " [ h:" << select_highlight_type << " ] ";

  //  if (last_command != nullptr) {
		//ss << " [ " << last_command->name.c_str() << " ] ";
  //  }

    return ss.str();
}

void MainWidget::handle_escape() {

    // add high escape priority to overview and search, if any of them are escaped, do not escape any further
    if (opengl_widget) {
        bool should_return = false;
        if (opengl_widget->get_overview_page()) {
            opengl_widget->set_overview_page({});
            should_return = true;
        }
        else if (opengl_widget->get_is_searching(nullptr)) {
            opengl_widget->cancel_search();
            should_return = true;
        }
        if (should_return) {
            validate_render();
            setFocus();
            return;
        }
    }

    text_command_line_edit->setText("");
    pending_link = {};
    current_pending_command = nullptr;

    if (current_widget != nullptr) {
        delete current_widget;
        current_widget = nullptr;
    }


    if (main_document_view) {
        main_document_view->handle_escape();
        opengl_widget->handle_escape();
    }
    
    if (opengl_widget) {
		bool done_anything = false;
        if (opengl_widget->get_overview_page()) {
            done_anything = true;
        }
        if (opengl_widget->selected_character_rects.size() > 0) {
            done_anything = true;
        }

        opengl_widget->set_overview_page({});
		opengl_widget->selected_character_rects.clear();
		selected_text.clear();

        if (!done_anything) {
            opengl_widget->set_should_draw_vertical_line(false);
        }
    }
    //if (opengl_widget) opengl_widget->set_should_draw_vertical_line(false);

    text_command_line_edit_container->hide();

    validate_render();
    setFocus();
}

void MainWidget::keyPressEvent(QKeyEvent* kevent) {
    key_event(false, kevent);
}

void MainWidget::keyReleaseEvent(QKeyEvent* kevent) {
    key_event(true, kevent);
}

void MainWidget::validate_render() {
    if (!isVisible()) {
        return;
    }
    if (last_smart_fit_page) {
        int current_page = get_current_page_number();
        if (current_page != last_smart_fit_page) {
            main_document_view->fit_to_page_width(true);
            last_smart_fit_page = current_page;
        }
    }
    if (opengl_widget->is_presentation_mode()) {
        int current_page = get_current_page_number();
        opengl_widget->set_visible_page_number(current_page);
        main_document_view->set_offset_y(main_document_view->get_document()->get_accum_page_height(current_page) + main_document_view->get_document()->get_page_height(current_page)/2);
        if (IGNORE_WHITESPACE_IN_PRESENTATION_MODE) {
            main_document_view->fit_to_page_height_smart();
        }
        else {
			main_document_view->fit_to_page_height_width_minimum();
        }
    }

    if (main_document_view && main_document_view->get_document()) {
        std::optional<Link> link = main_document_view->find_closest_link();
        if (link) {
            helper_document_view->goto_link(&link.value());
        }
        else {
            helper_document_view->set_null_document();
        }
    }
    validate_ui();
    update();
    opengl_widget->update();
    helper_opengl_widget->update();
    is_render_invalidated = false;
}

void MainWidget::validate_ui() {
    status_label->setText(QString::fromStdWString(get_status_string()));
    is_ui_invalidated = false;
}

void MainWidget::move_document(float dx, float dy) {
    if (main_document_view_has_document()) {
        main_document_view->move(dx, dy);
        //float prev_vertical_line_pos = opengl_widget->get_vertical_line_pos();
        //float new_vertical_line_pos = prev_vertical_line_pos - dy;
        //opengl_widget->set_vertical_line_pos(new_vertical_line_pos);
    }
}

void MainWidget::move_document_screens(int num_screens) {
    int view_height = opengl_widget->height();
    float move_amount = num_screens * view_height * MOVE_SCREEN_PERCENTAGE;
    move_document(0, move_amount);
}

QString MainWidget::get_status_stylesheet() {
    if (STATUS_BAR_FONT_SIZE > -1) {
        QString	font_size_stylesheet = QString("font-size: %1px").arg(STATUS_BAR_FONT_SIZE);
        return QString("background-color: %1; color: %2; border: 0; %3").arg(
            get_color_qml_string(STATUS_BAR_COLOR[0], STATUS_BAR_COLOR[1], STATUS_BAR_COLOR[2]),
            get_color_qml_string(STATUS_BAR_TEXT_COLOR[0], STATUS_BAR_TEXT_COLOR[1], STATUS_BAR_TEXT_COLOR[2]),
            font_size_stylesheet
        );
    }
    else{
        return QString("background-color: %1; color: %2; border: 0").arg(
            get_color_qml_string(STATUS_BAR_COLOR[0], STATUS_BAR_COLOR[1], STATUS_BAR_COLOR[2]),
            get_color_qml_string(STATUS_BAR_TEXT_COLOR[0], STATUS_BAR_TEXT_COLOR[1], STATUS_BAR_TEXT_COLOR[2])
        );
    }
}

int MainWidget::get_status_bar_height() {
    if (STATUS_BAR_FONT_SIZE > 0) {
        return STATUS_BAR_FONT_SIZE + 5;
    }
    else {
        return 20;
    }
}

void MainWidget::on_config_file_changed(ConfigManager* new_config) {

    status_label->setStyleSheet(get_status_stylesheet());

    int status_bar_height = get_status_bar_height();
    status_label->move(0, main_window_height - status_bar_height);
    status_label->resize(main_window_width, status_bar_height);

    text_command_line_edit_container->setStyleSheet("background-color: black; color: white; border: none;");
}

//void MainWidget::toggle_dark_mode()
//{
//	this->dark_mode = !this->dark_mode;
//	if (this->opengl_widget) {
//		this->opengl_widget->set_dark_mode(this->dark_mode);
//	}
//	if (this->helper_opengl_widget) {
//		this->helper_opengl_widget->set_dark_mode(this->dark_mode);
//	}
//}

void MainWidget::toggle_mouse_drag_mode(){
    this->mouse_drag_mode = !this->mouse_drag_mode;
}

void MainWidget::do_synctex_forward_search(const Path& pdf_file_path, const Path& latex_file_path, int line) {

    std::wstring latex_file_path_with_redundant_dot = add_redundant_dot_to_path(latex_file_path.get_path());

    std::string latex_file_string = latex_file_path.get_path_utf8();
    std::string latex_file_with_redundant_dot_string = utf8_encode(latex_file_path_with_redundant_dot);
    std::string pdf_file_string = pdf_file_path.get_path_utf8();

    synctex_scanner_t scanner = synctex_scanner_new_with_output_file(pdf_file_string.c_str(), nullptr, 1);

    int stat = synctex_display_query(scanner, latex_file_string.c_str(), line, 0);
    int target_page = -1;

    if (stat <= 0) {
        stat = synctex_display_query(scanner, latex_file_with_redundant_dot_string.c_str(), line, 0);
    }

    if (stat > 0) {
        synctex_node_t node;

        std::vector<std::pair<int, fz_rect>> highlight_rects;

        std::optional<fz_rect> first_rect = {};

        while ((node = synctex_next_result(scanner))) {
            int page = synctex_node_page(node);
            target_page = page-1;

            float x = synctex_node_box_visible_h(node);
            float y = synctex_node_box_visible_v(node);
            float w = synctex_node_box_visible_width(node);
            float h = synctex_node_box_visible_height(node);

            fz_rect doc_rect;
            doc_rect.x0 = x;
            doc_rect.y0 = y;
            doc_rect.x1 = x + w;
            doc_rect.y1 = y - h;

            if (!first_rect) {
                first_rect = doc_rect;
            }

            highlight_rects.push_back(std::make_pair(target_page, doc_rect));

            break; // todo: handle this properly
        }
        if (target_page != -1) {

            if ((main_document_view->get_document() == nullptr) ||
                (pdf_file_path.get_path() != main_document_view->get_document()->get_path())) {

                open_document(pdf_file_path);

            }

            opengl_widget->set_synctex_highlights(highlight_rects);
            if (highlight_rects.size() == 0) {
                main_document_view->goto_page(target_page);
            }
            else {
                main_document_view->goto_offset_within_page({ target_page, main_document_view->get_offset_x(), first_rect.value().y0 });
            }
        }

    }
    synctex_scanner_free(scanner);
}


void MainWidget::update_link_with_opened_book_state(Link lnk, const OpenedBookState& new_state) {
    std::wstring docpath = main_document_view->get_document()->get_path();
    Document* link_owner = document_manager->get_document(docpath);

    lnk.dst.book_state = new_state;

    if (link_owner) {
        link_owner->update_link(lnk);
    }

    db_manager->update_link(link_owner->get_checksum(),
        new_state.offset_x, new_state.offset_y, new_state.zoom_level, lnk.src_offset_y);

    link_to_edit = {};
}

void MainWidget::update_closest_link_with_opened_book_state(const OpenedBookState& new_state) {
    std::optional<Link> closest_link = main_document_view->find_closest_link();
    if (closest_link) {
        update_link_with_opened_book_state(closest_link.value(), new_state);
    }
}

void MainWidget::invalidate_render() {
    invalidate_ui();
    is_render_invalidated = true;
}

void MainWidget::invalidate_ui() {
    is_render_invalidated = true;
}

void MainWidget::handle_command_with_symbol(const Command* command, char symbol) {
    assert(symbol);
    assert(command->requires_symbol);

    if (command->name == "set_mark") {
        assert(main_document_view);

        // it is a global mark, we delete other marks with the same symbol from database and add the new mark
        if (isupper(symbol)) {
            db_manager->delete_mark_with_symbol(symbol);
            // we should also delete the cached marks
            document_manager->delete_global_mark(symbol);
            main_document_view->add_mark(symbol);
        }
        else{
            main_document_view->add_mark(symbol);
            validate_render();
        }

    }
    else if (command->name == "set_select_highlight_type") {
        select_highlight_type = symbol;
    }
    else if (command->name == "add_highlight") {
        if (opengl_widget->selected_character_rects.size() > 0) {
            main_document_view->add_highlight(selection_begin, selection_end, symbol);
            opengl_widget->selected_character_rects.clear();
            selected_text.clear();
        }
        else if (selected_highlight_index != -1) {
            Highlight new_highlight = main_document_view->get_highlight_with_index(selected_highlight_index);
            main_document_view->delete_highlight_with_index(selected_highlight_index);
            main_document_view->add_highlight(new_highlight.selection_begin, new_highlight.selection_end, symbol);
            selected_highlight_index = -1;
        }
    }
    else if (command->name == "external_search") {
        if ((symbol >= 'a') && (symbol <= 'z')) {
            search_custom_engine(selected_text, SEARCH_URLS[symbol - 'a']);
        }
        //if (opengl_widget->selected_character_rects.size() > 0) {
        //	main_document_view->add_highlight({ selection_begin_x, selection_begin_y }, { selection_end_x, selection_end_y }, symbol);
        //	opengl_widget->selected_character_rects.clear();
        //	selected_text.clear();
        //}
    }
    else if (command->name == "execute_predefined_command" ) {
        if ((symbol >= 'a') && (symbol <= 'z')) {
            execute_command(EXECUTE_COMMANDS[symbol - 'a']);
        }
    }
    else if (command->name == "goto_mark") {
        assert(main_document_view);

        if (symbol == '`' || symbol == '\'') {
            return_to_last_visual_mark();
        }
        else if (isupper(symbol)) { // global mark
            std::vector<std::pair<std::string, float>> mark_vector;
            db_manager->select_global_mark(symbol, mark_vector);
            if (mark_vector.size() > 0) {
                assert(mark_vector.size() == 1); // we can not have more than one global mark with the same name
                std::wstring doc_path = checksummer->get_path(mark_vector[0].first).value();
                open_document(doc_path, {}, mark_vector[0].second);
            }

        }
        else{
            main_document_view->goto_mark(symbol);
        }
    }
}

void MainWidget::open_document(const LinkViewState& lvs) {
    DocumentViewState dvs;
    auto path = checksummer->get_path(lvs.document_checksum);
    if (path) {
        dvs.book_state = lvs.book_state;
        dvs.document_path = path.value();
        open_document(dvs);
    }
}

void MainWidget::open_document_with_hash(const std::string& path, std::optional<float> offset_x, std::optional<float> offset_y, std::optional<float> zoom_level) {
    std::optional<std::wstring> maybe_path = checksummer->get_path(path);
    if (maybe_path) {
        Path path(maybe_path.value());
        open_document(path, offset_x, offset_y, zoom_level);
    }
}

void MainWidget::open_document(const Path& path, std::optional<float> offset_x, std::optional<float> offset_y, std::optional<float> zoom_level) {

    //save the previous document state
    if (main_document_view) {
        main_document_view->persist();
        update_history_state();
    }

    main_document_view->on_view_size_change(main_window_width, main_window_height);
    main_document_view->open_document(path.get_path(), &this->is_render_invalidated);
    bool has_document = main_document_view_has_document();

    if (has_document) {
        //setWindowTitle(QString::fromStdWString(path.get_path()));
        if (path.filename().has_value()) {
            setWindowTitle(QString::fromStdWString(path.filename().value()));
        }
        else {
            setWindowTitle(QString::fromStdWString(path.get_path()));
        }

        push_state();
    }

    if ((path.get_path().size() > 0) && (!has_document)) {
        show_error_message(L"Could not open file: " + path.get_path());
    }

    if (offset_x) {
        main_document_view->set_offset_x(offset_x.value());
    }
    if (offset_y) {
        main_document_view->set_offset_y(offset_y.value());
    }

    if (zoom_level) {
        main_document_view->set_zoom_level(zoom_level.value());
    }

    // reset smart fit when changing documents
    last_smart_fit_page = {};
    opengl_widget->on_document_view_reset();
    show_password_prompt_if_required();


}

void MainWidget::open_document_at_location(const Path& path_,
    int page,
    std::optional<float> x_loc,
    std::optional<float> y_loc,
    std::optional<float> zoom_level)
{
    //save the previous document state
    if (main_document_view) {
        main_document_view->persist();
        update_history_state();
    }
    std::wstring path = path_.get_path();

    open_document(path, &this->is_render_invalidated, true, {}, true);
    bool has_document = main_document_view_has_document();

    if (has_document) {
        //setWindowTitle(QString::fromStdWString(path));
        push_state();
    }

    if ((path.size() > 0) && (!has_document)) {
        show_error_message(L"Could not open file: " + path);
    }

    main_document_view->on_view_size_change(main_window_width, main_window_height);


    float absolute_x_loc, absolute_y_loc;
    main_document_view->get_document()->page_pos_to_absolute_pos(page,
        x_loc.value_or(0),
        y_loc.value_or(0),
        &absolute_x_loc,
        &absolute_y_loc);

    if (x_loc) {
        main_document_view->set_offset_x(absolute_x_loc);
    }
    main_document_view->set_offset_y(absolute_y_loc);

    if (zoom_level) {
        main_document_view->set_zoom_level(zoom_level.value());
    }

    // reset smart fit when changing documents
    last_smart_fit_page = {};
    opengl_widget->on_document_view_reset();
    show_password_prompt_if_required();
}

void MainWidget::open_document(const DocumentViewState& state)
{
    open_document(state.document_path, state.book_state.offset_x, state.book_state.offset_y, state.book_state.zoom_level);
}

void MainWidget::handle_command_with_file_name(const Command* command, std::wstring file_name) {
    assert(command->requires_file_name);
    if (command->name == "open_document") {
        open_document(file_name);
    }
}

bool MainWidget::is_waiting_for_symbol() {
    return (current_pending_command && current_pending_command->requires_symbol);
}

void MainWidget::handle_command_types(const Command* command, int num_repeats) {

    if (command == nullptr) return;

    last_command = command;

    if (command->requires_symbol) {
        current_pending_command = command;
        return;
    }
    if (command->requires_file_name) {
        std::wstring file_name = select_document_file_name();
        if (file_name.size() > 0) {
            handle_command_with_file_name(command, file_name);
        }
        else {
            std::cerr << "File select failed\n";
        }
        return;
    }
    else {
        handle_command(command, num_repeats);
    }
}

void MainWidget::key_event(bool released, QKeyEvent* kevent) {
    validate_render();


    if (released == false) {

        if (kevent->key() == Qt::Key::Key_Escape) {
            handle_escape();
        }

        if (kevent->key() == Qt::Key::Key_Return) {
            if (text_command_line_edit_container->isVisible()) {
                text_command_line_edit_container->hide();
                setFocus();
                handle_pending_text_command(text_command_line_edit->text().toStdWString());
                return;
            }
        }

        std::vector<int> ignored_codes = {
            Qt::Key::Key_Shift,
            Qt::Key::Key_Control
        };
        if (std::find(ignored_codes.begin(), ignored_codes.end(), kevent->key()) != ignored_codes.end()) {
            return;
        }
        if (is_waiting_for_symbol()) {

            char symb = get_symbol(kevent->key(), kevent->modifiers() & Qt::ShiftModifier, current_pending_command->special_symbols);
            if (symb) {
                handle_command_with_symbol(current_pending_command, symb);
                current_pending_command = nullptr;
            }
            return;
        }
        int num_repeats = 0;
        bool is_control_pressed = (kevent->modifiers() & Qt::ControlModifier) || (kevent->modifiers() & Qt::MetaModifier);
        std::vector<const Command*> commands = input_handler->handle_key(
            kevent->key(),
            kevent->modifiers() & Qt::ShiftModifier,
            is_control_pressed,
            kevent->modifiers() & Qt::AltModifier,
            &num_repeats);

        for (auto command : commands) {
            handle_command_types(command, num_repeats);
        }
    }

}

void MainWidget::handle_right_click(WindowPos click_pos, bool down) {

    if (is_rotated()) {
        return;
    }

    if ((down == true) && opengl_widget->get_overview_page()) {
        opengl_widget->set_overview_page({});
        //main_document_view->set_line_index(-1);
        invalidate_render();
        return;
    }

    if ((main_document_view->get_document() != nullptr) && (opengl_widget != nullptr)) {

        // disable visual mark and overview window when we are in synctex mode
        // because we probably don't need them (we are editing our own document after all)
        // we can always use middle click to jump to a destination which is probably what we
        // need anyway
        if (down == true && (!this->synctex_mode)) {
            if (current_pending_command && (current_pending_command->name == "goto_mark")) {
                return_to_last_visual_mark();
                return;
            }

            if (overview_under_pos(click_pos)) {
                return;
            }

            visual_mark_under_pos(click_pos);

        }
        else {
            if (this->synctex_mode) {
                auto [page, doc_x, doc_y] = main_document_view->window_to_document_pos(click_pos);
                std::wstring docpath = main_document_view->get_document()->get_path();
                std::string docpath_utf8 = utf8_encode(docpath);
                synctex_scanner_t scanner = synctex_scanner_new_with_output_file(docpath_utf8.c_str(), nullptr, 1);

                int stat = synctex_edit_query(scanner, page + 1, doc_x, doc_y);

                if (stat > 0) {
                    synctex_node_t node;
                    while ((node = synctex_next_result(scanner))) {
                        int line = synctex_node_line(node);
                        int column = synctex_node_column(node);
                        if (column < 0) column = 0;
                        int tag = synctex_node_tag(node);
                        const char* file_name = synctex_scanner_get_name(scanner, tag);

                        std::string line_string = std::to_string(line);
                        std::string column_string = std::to_string(column);

                        if (inverse_search_command.size() > 0) {
							QString command = QString::fromStdWString(inverse_search_command).arg(file_name, line_string.c_str(), column_string.c_str());
							QProcess::startDetached(command);
                        }
                        else{
                            show_error_message(L"inverse_search_command is not set in prefs_user.config");
                        }

                    }

                }
                synctex_scanner_free(scanner);

            }
        }

    }

}

void MainWidget::handle_left_click(WindowPos click_pos, bool down) {

    if (is_rotated()) {
        return;
    }

    AbsoluteDocumentPos abs_doc_pos = main_document_view->window_to_absolute_document_pos(click_pos);

    auto [normal_x, normal_y] = main_document_view->window_to_normalized_window_pos(click_pos);

    if (opengl_widget) opengl_widget->set_should_draw_vertical_line(false);

    if (down == true) {

        PdfViewOpenGLWidget::OverviewSide border_index = static_cast<PdfViewOpenGLWidget::OverviewSide>(-1);
        if (opengl_widget->is_window_point_in_overview_border(normal_x, normal_y, &border_index)) {
            PdfViewOpenGLWidget::OverviewResizeData resize_data;
            resize_data.original_normal_mouse_pos = NormalizedWindowPos{ normal_x, normal_y };
            resize_data.original_rect = opengl_widget->get_overview_rect();
            resize_data.side_index = border_index;
            overview_resize_data = resize_data;
            return;
        }
        if (opengl_widget->is_window_point_in_overview({ normal_x, normal_y })) {
            float original_offset_x, original_offset_y;

            PdfViewOpenGLWidget::OverviewMoveData move_data;
            opengl_widget->get_overview_offsets(&original_offset_x, &original_offset_y);
            move_data.original_normal_mouse_pos = NormalizedWindowPos{ normal_x, normal_y };
            move_data.original_offsets = fvec2{ original_offset_x, original_offset_y };
            overview_move_data = move_data;
            return;
        }

        selection_begin = abs_doc_pos;
        //selection_begin_x = x_;
        //selection_begin_y = y_;

        last_mouse_down = abs_doc_pos;
        //last_mouse_down_x = x_;
        //last_mouse_down_y = y_;
        last_mouse_down_window_pos = click_pos;
        last_mouse_down_document_offset = main_document_view->get_offsets();
        //last_mouse_down_window_x = x;
        //last_mouse_down_window_y = y;

        opengl_widget->selected_character_rects.clear();

        if (!mouse_drag_mode) {
            is_selecting = true;
        }
        else {
            is_dragging = true;
        }
    }
    else {
        selection_end = abs_doc_pos;

        is_selecting = false;
        is_dragging = false;

        bool was_overview_mode = overview_move_data.has_value() || overview_resize_data.has_value();

        overview_move_data = {};
        overview_resize_data = {};

        //if (was_overview_mode) {
        //    return;
        //}

        if ((!was_overview_mode) && (!mouse_drag_mode) && (manhattan_distance(fvec2(last_mouse_down), fvec2(abs_doc_pos)) > 5)) {

            //fz_point selection_begin = { last_mouse_down_x, last_mouse_down_y };
            //fz_point selection_end = { x_, y_ };

            main_document_view->get_text_selection(last_mouse_down,
                abs_doc_pos,
                is_word_selecting,
                opengl_widget->selected_character_rects,
                selected_text);
            is_word_selecting = false;
        }
        else {
            handle_click(click_pos);
            opengl_widget->selected_character_rects.clear();
            selected_text.clear();
        }
        validate_render();
    }
}

void MainWidget::update_history_state() {
    if (!main_document_view_has_document()) return; // we don't add empty document to history
    if (history.size() == 0) return; // this probably should never execute

    DocumentViewState dvs = main_document_view->get_state();
    history[current_history_index] = dvs;
}

void MainWidget::push_state() {

    if (!main_document_view_has_document()) return; // we don't add empty document to history

    DocumentViewState dvs = main_document_view->get_state();

    //if (history.size() > 0) { // this check should always be true
    //	history[history.size() - 1] = dvs;
    //}
    //// don't add the same place in history multiple times
    //// todo: we probably don't need this check anymore
    //if (history.size() > 0) {
    //	DocumentViewState last_history = history.back();
    //	if (last_history == dvs) return;
    //}

    // delete all history elements after the current history point
    history.erase(history.begin() + (1 + current_history_index), history.end());
    history.push_back(dvs);
    current_history_index = static_cast<int>(history.size() - 1);
}

void MainWidget::next_state() {
    update_current_history_index();
    if (current_history_index < (static_cast<int>(history.size())-1)) {
        current_history_index++;
        set_main_document_view_state(history[current_history_index]);

    }
}

void MainWidget::prev_state() {
    update_current_history_index();
    if (current_history_index > 0) {
        current_history_index--;

        /*
        Goto previous history
        In order to edit a link, we set the link to edit and jump to the link location, when going back, we
        update the link with the current location of document, therefore, we must check to see if a link
        is being edited and if so, we should update its destination position
        */
        if (link_to_edit) {

            //std::wstring link_document_path = checksummer->get_path(link_to_edit.value().dst.document_checksum).value();
            std::wstring link_document_path = history[current_history_index].document_path;
            Document* link_owner = document_manager->get_document(link_document_path);

            OpenedBookState state = main_document_view->get_state().book_state;
            link_to_edit.value().dst.book_state = state;

            if (link_owner) {
                link_owner->update_link(link_to_edit.value());
            }

            db_manager->update_link(checksummer->get_checksum(history[current_history_index].document_path),
                state.offset_x, state.offset_y, state.zoom_level, link_to_edit->src_offset_y);
            link_to_edit = {};
        }

        set_main_document_view_state(history[current_history_index]);
    }
}

void MainWidget::update_current_history_index() {
    if (main_document_view_has_document()) {
        DocumentViewState current_state = main_document_view->get_state();
        history[current_history_index] = current_state;

    }
}

void MainWidget::set_main_document_view_state(DocumentViewState new_view_state) {

    if (main_document_view->get_document()->get_path() != new_view_state.document_path) {
        open_document(new_view_state.document_path, &this->is_ui_invalidated);
        //setWindowTitle(QString::fromStdWString(new_view_state.document_path));
    }

    main_document_view->on_view_size_change(main_window_width, main_window_height);
    main_document_view->set_book_state(new_view_state.book_state);
}

void MainWidget::handle_click(WindowPos click_pos) {

    if (!main_document_view_has_document()) {
        return;
    }

    auto [normal_x, normal_y] = main_document_view->window_to_normalized_window_pos(click_pos);

    if (opengl_widget->is_window_point_in_overview({ normal_x, normal_y })) {
        auto [doc_page, doc_x, doc_y] = opengl_widget->window_pos_to_overview_pos({ normal_x, normal_y });
        auto link = main_document_view->get_document()->get_link_in_pos(doc_page, doc_x, doc_y);
        if (link) {
            handle_link_click(link.value());
        }
        return;
    }

    auto link = main_document_view->get_link_in_pos(click_pos);
    selected_highlight_index = main_document_view->get_highlight_index_in_pos(click_pos);


    if (link.has_value()) {
        handle_link_click(link.value());
    }
}
bool MainWidget::find_location_of_text_under_pointer(WindowPos pointer_pos, int* out_page, float* out_offset) {

    auto [page, offset_x, offset_y] = main_document_view->window_to_document_pos(pointer_pos);
    int current_page_number = get_current_page_number();

    fz_stext_page* stext_page = main_document_view->get_document()->get_stext_with_page_number(page);
    std::vector<fz_stext_char*> flat_chars;
    get_flat_chars_from_stext_page(stext_page, flat_chars);

    std::optional<std::pair<std::wstring, std::wstring>> generic_pair =\
        main_document_view->get_document()->get_generic_link_name_at_position(flat_chars, offset_x, offset_y);

    //std::optional<std::wstring> text_on_pointer = main_document_view->get_document()->get_text_at_position(flat_chars, offset_x, offset_y);
    std::optional<std::wstring> paper_name_on_pointer = main_document_view->get_document()->get_paper_name_at_position(flat_chars, offset_x, offset_y);
    std::optional<std::wstring> reference_text_on_pointer = main_document_view->get_document()->get_reference_text_at_position(flat_chars, offset_x, offset_y);
    std::optional<std::wstring> equation_text_on_pointer = main_document_view->get_document()->get_equation_text_at_position(flat_chars, offset_x, offset_y);

    if (generic_pair) {
        return main_document_view->get_document()->find_generic_location(generic_pair.value().first,
            generic_pair.value().second,
            out_page,
            out_offset);
    }
    if (equation_text_on_pointer) {
         std::optional<IndexedData> eqdata_ = main_document_view->get_document()->find_equation_with_string(equation_text_on_pointer.value(), current_page_number);
         if (eqdata_) {
             IndexedData refdata = eqdata_.value();
             *out_page = refdata.page;
             *out_offset = refdata.y_offset;
             return true;
         }
    }

    if (reference_text_on_pointer) {
         std::optional<IndexedData> refdata_ = main_document_view->get_document()->find_reference_with_string(reference_text_on_pointer.value());
         if (refdata_) {
             IndexedData refdata = refdata_.value();
             *out_page = refdata.page;
             *out_offset = refdata.y_offset;
             return true;
         }

    }

    return false;
}

void MainWidget::mouseReleaseEvent(QMouseEvent* mevent) {

	if (is_rotated()) {
		return;
	}

    if (mevent->button() == Qt::MouseButton::LeftButton) {
        handle_left_click({ mevent->pos().x(), mevent->pos().y() }, false);
        if (is_select_highlight_mode && (opengl_widget->selected_character_rects.size() > 0)) {
            main_document_view->add_highlight(selection_begin, selection_end, select_highlight_type);
        }
        if (opengl_widget->selected_character_rects.size() > 0) {
            copy_to_clipboard(selected_text, true);
        }
    }

    if (mevent->button() == Qt::MouseButton::RightButton) {
        handle_right_click({ mevent->pos().x(), mevent->pos().y() }, false);
    }

    if (mevent->button() == Qt::MouseButton::MiddleButton) {
        smart_jump_under_pos({ mevent->pos().x(), mevent->pos().y() });
    }

}

void MainWidget::mouseDoubleClickEvent(QMouseEvent* mevent) {
    if (mevent->button() == Qt::MouseButton::LeftButton) {
        is_selecting = true;
        is_word_selecting = true;
    }
}

void MainWidget::mousePressEvent(QMouseEvent* mevent) {

    if (mevent->button() == Qt::MouseButton::LeftButton) {
        handle_left_click({ mevent->pos().x(), mevent->pos().y() }, true);
    }

    if (mevent->button() == Qt::MouseButton::RightButton) {
        handle_right_click({ mevent->pos().x(), mevent->pos().y() }, true);
    }

    if (mevent->button() == Qt::MouseButton::XButton1) {
        handle_command(command_manager.get_command_with_name("prev_state"), 0);
    }

    if (mevent->button() == Qt::MouseButton::XButton2) {
        handle_command(command_manager.get_command_with_name("next_state"), 0);
    }
}

void MainWidget::wheelEvent(QWheelEvent* wevent) {

    const Command* command = nullptr;
    //bool is_touchpad = wevent->source() == Qt::MouseEventSource::MouseEventSynthesizedBySystem;
    //bool is_touchpad = true;
    float vertical_move_amount = VERTICAL_MOVE_AMOUNT * TOUCHPAD_SENSITIVITY;
    float horizontal_move_amount = HORIZONTAL_MOVE_AMOUNT * TOUCHPAD_SENSITIVITY;

    //if (is_touchpad) {
    //	vertical_move_amount *= TOUCHPAD_SENSITIVITY;
    //	horizontal_move_amount *= TOUCHPAD_SENSITIVITY;
    //}

    bool is_control_pressed = QApplication::queryKeyboardModifiers().testFlag(Qt::ControlModifier) ||
        QApplication::queryKeyboardModifiers().testFlag(Qt::MetaModifier);

    bool is_shift_pressed = QApplication::queryKeyboardModifiers().testFlag(Qt::ShiftModifier);
    bool is_visual_mark_mode = opengl_widget->get_should_draw_vertical_line() && visual_scroll_mode;


    int x = wevent->pos().x();
    int y = wevent->pos().y();
    WindowPos mouse_window_pos = { x, y };
    auto [normal_x, normal_y] = main_document_view->window_to_normalized_window_pos(mouse_window_pos);

	int num_repeats = abs(wevent->delta() / 120);
    if (num_repeats == 0) {
        num_repeats = 1;
    }

    if ((!is_control_pressed) && (!is_shift_pressed)) {
        if (opengl_widget->is_window_point_in_overview({ normal_x, normal_y })) {
            if (wevent->angleDelta().y() > 0) {
                scroll_overview_up();
            }
            if (wevent->angleDelta().y() < 0) {
                scroll_overview_down();
            }
            validate_render();
        }
        else {

            if (wevent->angleDelta().y() > 0) {

                if (is_visual_mark_mode) {
                    command = command_manager.get_command_with_name("move_visual_mark_up");
                }
                else {
                    move_vertical(-72.0f * vertical_move_amount * num_repeats);
                    return;
                }
            }
            if (wevent->angleDelta().y() < 0) {

                if (is_visual_mark_mode) {
                    command = command_manager.get_command_with_name("move_visual_mark_down");
                }
                else {
                    move_vertical(72.0f * vertical_move_amount * num_repeats);
                    return;
                }
            }

            if (wevent->angleDelta().x() > 0) {
                move_horizontal(-72.0f * horizontal_move_amount);
                return;
            }
            if (wevent->angleDelta().x() < 0) {
                move_horizontal(72.0f * horizontal_move_amount);
                return;
            }
        }
    }

    if (is_control_pressed) {
        if (wevent->angleDelta().y() > 0) {
            if (WHEEL_ZOOM_ON_CURSOR) {
				command = command_manager.get_command_with_name("zoom_in_cursor");
            }
			else {
				command = command_manager.get_command_with_name("zoom_in");
            }
        }
        if (wevent->angleDelta().y() < 0) {
            if (WHEEL_ZOOM_ON_CURSOR) {
				command = command_manager.get_command_with_name("zoom_out_cursor");
            }
            else {
				command = command_manager.get_command_with_name("zoom_out");
            }
        }

    }
    if (is_shift_pressed) {
        if (wevent->angleDelta().y() > 0) {
            command = command_manager.get_command_with_name("move_left");
        }
        if (wevent->angleDelta().y() < 0) {
            command = command_manager.get_command_with_name("move_right");
        }

    }

    if (command) {
        handle_command(command, num_repeats);
    }
}

void MainWidget::show_textbar(const std::wstring& command_name, bool should_fill_with_selected_text) {
    text_command_line_edit->clear();
    if (should_fill_with_selected_text) {
        text_command_line_edit->setText(QString::fromStdWString(selected_text));
    }
    text_command_line_edit_label->setText(QString::fromStdWString(command_name));
    text_command_line_edit_container->show();
    text_command_line_edit->setFocus();
}

bool MainWidget::helper_window_overlaps_main_window() {

    QRect main_window_rect = get_main_window_rect();
    QRect helper_window_rect = get_helper_window_rect();
    QRect intersection = main_window_rect.intersected(helper_window_rect);
    return intersection.width() > 50;
}

void MainWidget::toggle_window_configuration() {

    QWidget* helper_window = get_top_level_widget(helper_opengl_widget);
    QWidget* main_window = get_top_level_widget(opengl_widget);

    if (helper_opengl_widget->isVisible()) {
        apply_window_params_for_one_window_mode();
    }
    else {
        apply_window_params_for_two_window_mode();
        if (helper_window_overlaps_main_window()) {
            helper_window->activateWindow();
        }
        else {
            main_window->activateWindow();
        }


    }
}

void MainWidget::toggle_two_window_mode() {

    //main_widget.resize(window_width, window_height);

    QWidget* helper_window = get_top_level_widget(helper_opengl_widget);

    if (helper_opengl_widget->isHidden()) {

        QPoint pos = helper_opengl_widget->pos();
        QSize size = helper_opengl_widget->size();
        helper_window->show();
        helper_window->resize(size);
        helper_window->move(pos);

        if (!helper_window_overlaps_main_window()) {
            activateWindow();
        }
    }
    else {
        helper_window->hide();
    }
}

void MainWidget::handle_command(const Command* command, int num_repeats) {
    if (command == nullptr) return;

    if (command->requires_text) {
        current_pending_command = command;
        bool should_fill_text_bar_with_selected_text = false;
        if (command->name == "search" || command->name == "chapter_search" || command->name == "ranged_search" || command->name == "add_bookmark") {
            should_fill_text_bar_with_selected_text = true;
        }
        if (command->name == "open_link") {
            opengl_widget->set_highlight_links(true, true);
        }
        if ((command->name == "keyboard_select") || (command->name == "keyboard_smart_jump") || (command->name == "keyboard_overview")) {
            highlight_words();
        }
        show_textbar(utf8_decode(command->name.c_str()), should_fill_text_bar_with_selected_text);
        if (command->name == "chapter_search") {
            std::optional<std::pair<int, int>> chapter_range = main_document_view->get_current_page_range();
            if (chapter_range) {
                std::stringstream search_range_string;
                search_range_string << "<" << chapter_range.value().first << "," << chapter_range.value().second << ">";
                text_command_line_edit->setText(search_range_string.str().c_str() + text_command_line_edit->text());
            }
        }
        return;
    }
    if (command->name == "goto_begining") {
        if (num_repeats) {
            main_document_view->goto_page(num_repeats - 1 + main_document_view->get_page_offset());
        }
        else {
            main_document_view->set_offset_y(0.0f);
        }
    }

    if (command->name == "goto_end") {
        if (num_repeats) {
            main_document_view->goto_page(num_repeats - 1 + main_document_view->get_page_offset());
        }
        else {
			main_document_view->goto_end();
        }
    }

    if (command->name == "toggle_select_highlight") {
        is_select_highlight_mode = !is_select_highlight_mode;
    }
    if (command->name == "copy") {
        copy_to_clipboard(selected_text);
    }
    if (command->name == "copy_window_size_config") {
        copy_to_clipboard(get_window_configuration_string());
    }

    if (command->name == "highlight_links") {
        opengl_widget->set_highlight_links(true, false);
    }

    int rp = std::max(num_repeats, 1);

    if (!opengl_widget->is_presentation_mode()) {
        if (command->name == "screen_down") {
            move_document_screens(1 * rp);
        }
        if (command->name == "screen_up") {
            move_document_screens(-1 * rp);
        }
        if (opengl_widget->get_overview_page()) {
            // if overview page is opened, scroll in overview
			if (command->name == "move_down") {
                scroll_overview_down();
			}
			if (command->name == "move_up") {
                scroll_overview_up();
			}
        }
        else {
			
			if (command->name == "move_down") {
				move_document(0.0f, 72.0f * rp * VERTICAL_MOVE_AMOUNT);
			}
			if (command->name == "move_up") {
				move_document(0.0f, -72.0f * rp * VERTICAL_MOVE_AMOUNT);
			}
        }

        if (command->name == "move_right") {
            if (!horizontal_scroll_locked) {
                main_document_view->move(72.0f * rp * HORIZONTAL_MOVE_AMOUNT, 0.0f);
                last_smart_fit_page = {};
            }
        }

        if (command->name == "move_left") {
            if (!horizontal_scroll_locked) {
                main_document_view->move(-72.0f * rp * HORIZONTAL_MOVE_AMOUNT, 0.0f);
                last_smart_fit_page = {};
            }
        }
    }
    else {
        if (command->name == "screen_down") {
            main_document_view->move_pages(1);
        }
        if (command->name == "screen_up") {
            main_document_view->move_pages(-1);
        }
        if (command->name == "move_down") {
            main_document_view->move_pages(1);
        }
        if (command->name == "move_up") {
            main_document_view->move_pages(-1);
        }
        if (command->name == "move_left") {
            main_document_view->move_pages(1);
        }
        if (command->name == "move_right") {
            main_document_view->move_pages(-1);
        }
    }


    if (command->name == "link" || command->name == "portal") {
        handle_link();
    }
    else if (command->name == "new_window") {

		MainWidget* new_widget = new MainWidget(mupdf_context,
            db_manager,
            document_manager,
            config_manager,
            input_handler,
            checksummer,
            should_quit);
        new_widget->open_document(main_document_view->get_state());
        windows.push_back(new_widget);
    }

    else if (command->name == "goto_link" || command->name == "goto_portal") {
        std::optional<Link> link = main_document_view->find_closest_link();
        if (link) {
            open_document(link->dst);
        }
    }
    else if (command->name == "edit_link" || command->name == "edit_portal") {
        std::optional<Link> link = main_document_view->find_closest_link();
        if (link) {
            link_to_edit = link;
            open_document(link->dst);
        }
    }

    else if (command->name == "zoom_in") {
		main_document_view->zoom_in();
        last_smart_fit_page = {};
    }

    else if (command->name == "zoom_out") {
		main_document_view->zoom_out();
        last_smart_fit_page = {};
    }

    else if (command->name == "zoom_in_cursor") {
		QPoint mouse_pos = mapFromGlobal(QCursor::pos());
        main_document_view->zoom_in_cursor({ mouse_pos.x(), mouse_pos.y() });
		last_smart_fit_page = {};
    }

    else if (command->name == "zoom_out_cursor") {
		QPoint mouse_pos = mapFromGlobal(QCursor::pos());
        main_document_view->zoom_out_cursor({ mouse_pos.x(), mouse_pos.y() });
		last_smart_fit_page = {};
    }

    else if (command->name == "fit_to_page_width") {
        main_document_view->fit_to_page_width();
        last_smart_fit_page = {};
    }
    else if (command->name == "fit_to_page_width_ratio") {
        main_document_view->fit_to_page_width(false, true);
        last_smart_fit_page = {};
    }
    else if (command->name == "fit_to_page_width_smart") {
        main_document_view->fit_to_page_width(true);
        int current_page = get_current_page_number();
        last_smart_fit_page = current_page;
    }
    else if (command->name == "fit_to_page_height_smart") {
        main_document_view->fit_to_page_height_smart();
    }

    else if (command->name == "next_state") {
        next_state();
    }
    else if (command->name == "prev_state") {
        prev_state();
    }

    else if (command->name == "next_item") {
        if (num_repeats == 0) num_repeats++;
        opengl_widget->goto_search_result(num_repeats);
    }

    else if (command->name == "previous_item") {
        if (num_repeats == 0) num_repeats++;
        opengl_widget->goto_search_result(-num_repeats);
    }
    else if (command->name == "push_state") {
        push_state();
    }
    else if (command->name == "pop_state") {
        prev_state();
    }

    else if (command->name == "next_page") {
        main_document_view->move_pages(1 + num_repeats);
    }
    else if (command->name == "previous_page") {
        main_document_view->move_pages(-1 - num_repeats);
    }
    else if (command->name == "goto_top_of_page") {
        main_document_view->goto_top_of_page();
    }
    else if (command->name == "goto_bottom_of_page") {
        main_document_view->goto_bottom_of_page();
    }
    else if (command->name == "next_chapter") {
        main_document_view->goto_chapter(rp);
    }
    else if (command->name == "prev_chapter") {
        main_document_view->goto_chapter(-rp);
    }
    else if (command->name == "goto_definition") {
        if (main_document_view->goto_definition()) {
            opengl_widget->set_should_draw_vertical_line(false);
        }
    }
    else if (command->name == "overview_definition") {
		if (!opengl_widget->get_overview_page()) {
			std::optional<DocumentPos> defpos = main_document_view->find_line_definition();
			if (defpos) {
				set_overview_position(defpos.value().page, defpos.value().y);
			}
        }
        else {
			opengl_widget->set_overview_page({});
        }
    }
    else if (command->name == "portal_to_definition") {
		std::optional<DocumentPos> defpos = main_document_view->find_line_definition();
        if (defpos) {
            AbsoluteDocumentPos abspos;
            doc()->page_pos_to_absolute_pos(defpos.value().page, defpos.value().x, defpos.value().y, &abspos.x, &abspos.y);
            Link link;
            link.dst.document_checksum = doc()->get_checksum();
            link.dst.book_state.offset_x = abspos.x;
            link.dst.book_state.offset_y = abspos.y;
            link.dst.book_state.zoom_level = main_document_view->get_zoom_level();
            link.src_offset_y = main_document_view->get_ruler_pos();
            doc()->add_link(link, true);
        }
    }


    else if (command->name == "goto_toc") {
        if (main_document_view->get_document()->has_toc()) {
            if (FLAT_TABLE_OF_CONTENTS) {
                std::vector<std::wstring> flat_toc;
                std::vector<int> current_document_toc_pages;
                get_flat_toc(main_document_view->get_document()->get_toc(), flat_toc, current_document_toc_pages);
                set_current_widget(new FilteredSelectWindowClass<int>(flat_toc, current_document_toc_pages, [&](int* page_value) {
                    if (page_value) {
                        validate_render();
                        update_history_state();
                        main_document_view->goto_page(*page_value);
                        push_state();
                    }
                    }, this));
                current_widget->show();
            }
            else {

                std::vector<int> selected_index = main_document_view->get_current_chapter_recursive_index();
                set_current_widget(new FilteredTreeSelect<int>(main_document_view->get_document()->get_toc_model(),
                    [&](const std::vector<int>& indices) {
                        TocNode* toc_node = get_toc_node_from_indices(main_document_view->get_document()->get_toc(),
                            indices);
                        if (toc_node) {
                            validate_render();
                            update_history_state();
                            //main_document_view->goto_page(toc_node->page);
                            main_document_view->goto_offset_within_page({ toc_node->page, toc_node->x, toc_node->y });
                            push_state();
                        }
                    }, this, selected_index));
                current_widget->show();
            }

        }
        else {
            show_error_message(L"This document doesn't have a table of contents");
        }
    }
    else if (command->name == "open_last_document") {

        auto last_opened_file = get_last_opened_file_checksum();
        if (last_opened_file) {
            open_document_with_hash(last_opened_file.value());
        }
    }
    else if (command->name == "open_prev_doc") {
        //std::vector<std::pair<std::wstring, std::wstring>> opened_docs_hash_path_pairs;
        std::vector<std::wstring> opened_docs_names;
        std::vector<std::wstring> opened_docs_hashes_;
        std::vector<std::string> opened_docs_hashes;

        db_manager->select_opened_books_path_values(opened_docs_hashes_);

        for (const auto& doc_hash_ : opened_docs_hashes_) {
            std::optional<std::wstring> path = checksummer->get_path(utf8_encode(doc_hash_));
            if (path) {
                opened_docs_names.push_back(Path(path.value()).filename().value_or(L"<ERROR>"));
                opened_docs_hashes.push_back(utf8_encode(doc_hash_));
            }
        }
        //db_manager->get_prev_path_hash_pairs(opened_docs_hash_path_pairs);

        //for (const auto& [path, hash] : opened_docs_hash_path_pairs) {
        //	opened_docs_names.push_back(Path(path).filename().value_or(L"<ERROR>"));
        //	opened_docs_hashes.push_back(utf8_encode(hash));
        //}

        if (opened_docs_hashes.size() > 0) {
            set_current_widget(new FilteredSelectWindowClass<std::string>(opened_docs_names,
                opened_docs_hashes,
                [&](std::string* doc_hash) {
                    if (doc_hash->size() > 0) {
                        validate_render();
                        open_document_with_hash(*doc_hash);
                    }
                },
                    this,
                    [&](std::string* doc_hash) {
                    db_manager->delete_opened_book(*doc_hash);
                }));
            current_widget->show();
        }
    }
    else if (command->name == "open_document_embedded") {

        set_current_widget(new FileSelector(
            [&](std::wstring doc_path) {
                validate_render();
                open_document(doc_path);
            }, this, ""));
        current_widget->show();
    }
    else if (command->name == "open_document_embedded_from_current_path") {
        std::wstring last_file_name = get_current_file_name().value_or(L"");

        set_current_widget(new FileSelector(
            [&](std::wstring doc_path) {
                validate_render();
                open_document(doc_path);
            }, this, QString::fromStdWString(last_file_name)));
        current_widget->show();
    }
    else if (command->name == "goto_bookmark") {
        std::vector<std::wstring> option_names;
        std::vector<std::wstring> option_location_strings;
        std::vector<float> option_locations;
        std::vector<BookMark> bookmarks;
        if (SORT_BOOKMARKS_BY_LOCATION) {
            bookmarks = main_document_view->get_document()->get_sorted_bookmarks();
        }
        else {
            bookmarks = main_document_view->get_document()->get_bookmarks();
        }

        for (auto bookmark : bookmarks){
            option_names.push_back(ITEM_LIST_PREFIX + L" " + bookmark.description);
            option_locations.push_back(bookmark.y_offset);
            auto [page, _, __] = main_document_view->get_document()->absolute_to_page_pos({ 0, bookmark.y_offset });
            option_location_strings.push_back(get_page_formatted_string(page + 1));
        }

        int closest_bookmark_index = main_document_view->get_document()->find_closest_sorted_bookmark_index(bookmarks, main_document_view->get_offset_y());

        set_current_widget(new FilteredSelectTableWindowClass<float>(
            option_names,
            option_location_strings,
            option_locations,
            closest_bookmark_index,
            [&](float* offset_value) {
                if (offset_value) {
                    validate_render();
                    update_history_state();
                    main_document_view->set_offset_y(*offset_value);
                    push_state();
                }
            },
            this,
                [&](float* offset_value) {
                if (offset_value) {
                    main_document_view->delete_closest_bookmark_to_offset(*offset_value);
                }
            }));
        current_widget->show();
    }
    else if (command->name == "add_highlight_with_current_type") {
        if (opengl_widget->selected_character_rects.size() > 0) {
            main_document_view->add_highlight(selection_begin, selection_end, select_highlight_type);
            opengl_widget->selected_character_rects.clear();
            selected_text.clear();
        }
    }
    else if (command->name == "goto_next_highlight") {
		auto next_highlight = main_document_view->get_document()->get_next_highlight(main_document_view->get_offset_y());
        if (next_highlight.has_value()) {
			long_jump_to_destination(next_highlight.value().selection_begin.y);
        }
	}
    else if (command->name == "goto_next_highlight_of_type") {
		auto next_highlight = main_document_view->get_document()->get_next_highlight(main_document_view->get_offset_y(), select_highlight_type);
        if (next_highlight.has_value()) {
			long_jump_to_destination(next_highlight.value().selection_begin.y);
        }
	}
    else if (command->name == "goto_prev_highlight") {
		auto prev_highlight = main_document_view->get_document()->get_prev_highlight(main_document_view->get_offset_y());
        if (prev_highlight.has_value()) {
			long_jump_to_destination(prev_highlight.value().selection_begin.y);
        }
	}
    else if (command->name == "goto_prev_highlight_of_type") {
		auto prev_highlight = main_document_view->get_document()->get_prev_highlight(main_document_view->get_offset_y(), select_highlight_type);
        if (prev_highlight.has_value()) {
			long_jump_to_destination(prev_highlight.value().selection_begin.y);
        }
	}

    else if (command->name == "goto_highlight") {
        std::vector<std::wstring> option_names;
        std::vector<std::wstring> option_location_strings;
        std::vector<std::vector<float>> option_locations;
        const std::vector<Highlight>& highlights = main_document_view->get_document()->get_highlights_sorted();

        int closest_highlight_index = main_document_view->get_document()->find_closest_sorted_highlight_index(highlights, main_document_view->get_offset_y());

        for (auto highlight : highlights){
            std::wstring type_name = L"a";
            type_name[0] = highlight.type;
            option_names.push_back(L"[" + type_name + L"] " + highlight.description + L"]");
            option_locations.push_back({highlight.selection_begin.x, highlight.selection_begin.y, highlight.selection_end.x, highlight.selection_end.y});
            auto [page, _, __] = main_document_view->get_document()->absolute_to_page_pos(highlight.selection_begin);
            option_location_strings.push_back(get_page_formatted_string(page + 1));
        }

        set_current_widget(new FilteredSelectTableWindowClass<std::vector<float>>(
            option_names,
            option_location_strings,
            option_locations,
            closest_highlight_index,
            [&](std::vector<float>* offset_values) {
                if (offset_values) {
                    validate_render();
                    update_history_state();
                    main_document_view->set_offset_y((*offset_values)[1]);
                    push_state();
                }
            },
            this,
                [&](std::vector<float>* offset_values) {
                if (offset_values) {
                    float begin_x = (*(offset_values))[0];
                    float begin_y = (*(offset_values))[1];
                    float end_x = (*(offset_values))[2];
                    float end_y = (*(offset_values))[3];
                    main_document_view->delete_highlight_with_offsets(begin_x, begin_y, end_x, end_y);
                }
            }));
        current_widget->show();
    }
    else if (command->name == "goto_bookmark_g") {
        std::vector<std::pair<std::string, BookMark>> global_bookmarks;
        db_manager->global_select_bookmark(global_bookmarks);
        std::vector<std::wstring> descs;
        std::vector<std::wstring> file_names;
        std::vector<BookState> book_states;

        for (const auto& desc_bm_pair : global_bookmarks) {
            std::string checksum = desc_bm_pair.first;
            std::optional<std::wstring> path = checksummer->get_path(checksum);
            if (path) {
                BookMark bm = desc_bm_pair.second;
                std::wstring file_name = Path(path.value()).filename().value_or(L"");
                descs.push_back(ITEM_LIST_PREFIX + L" " + bm.description);
                file_names.push_back(truncate_string(file_name, 50));
                book_states.push_back({ path.value(), bm.y_offset });
            }
        }
        set_current_widget(new FilteredSelectTableWindowClass<BookState>(
            descs,
            file_names,
            book_states,
            -1,
            [&](BookState* book_state) {
                if (book_state) {
                    validate_render();
                    open_document(book_state->document_path, 0.0f, book_state->offset_y);
                }
            },
            this,
            [&](BookState* book_state) {
                if (book_state) {
                    db_manager->delete_bookmark(checksummer->get_checksum(book_state->document_path), book_state->offset_y);
                }
            }));
        current_widget->show();

    }
    else if (command->name == "goto_highlight_g") {
        std::vector<std::pair<std::string, Highlight>> global_highlights;
        db_manager->global_select_highlight(global_highlights);
        std::vector<std::wstring> descs;
        std::vector<std::wstring> file_names;
        std::vector<BookState> book_states;

        for (const auto& desc_hl_pair : global_highlights) {
            std::string checksum = desc_hl_pair.first;
            std::optional<std::wstring> path = checksummer->get_path(checksum);
            if (path) {
                Highlight hl = desc_hl_pair.second;

                std::wstring file_name = Path(path.value()).filename().value_or(L"");

                std::wstring highlight_type_string = L"a";
                highlight_type_string[0] = hl.type;

                //descs.push_back(L"[" + highlight_type_string + L"]" + hl.description + L" {" + file_name + L"}");
                descs.push_back(L"[" + highlight_type_string + L"]" + hl.description);

				file_names.push_back(truncate_string(file_name, 50));

                book_states.push_back({ path.value(), hl.selection_begin.y });

            }
        }
        set_current_widget(new FilteredSelectTableWindowClass<BookState>(
            descs,
            file_names,
            book_states,
            -1,
            [&](BookState* book_state) {
                if (book_state) {
                    validate_render();
                    open_document(book_state->document_path, 0.0f, book_state->offset_y);
                }
            },
                this));
        current_widget->show();

    }

    else if (command->name == "toggle_visual_scroll") {
        toggle_visual_scroll_mode();
    }
    else if (command->name == "toggle_fullscreen") {
        toggle_fullscreen();
    }
    else if (command->name == "toggle_presentation_mode") {
        toggle_presentation_mode();
    }
    else if (command->name == "toggle_one_window") {
        toggle_two_window_mode();
    }
    else if (command->name == "toggle_window_configuration") {
        toggle_window_configuration();
    }

    else if (command->name == "toggle_highlight") {
        opengl_widget->toggle_highlight_links();
    }
    else if (command->name == "toggle_mouse_drag_mode") {
        toggle_mouse_drag_mode();
    }

    else if (command->name == "toggle_synctex") {
        toggle_synctex_mode();
    }

    else if (command->name == "delete_link" || command->name == "delete_portal") {

        main_document_view->delete_closest_link();
        validate_render();
    }

    else if (command->name == "delete_highlight") {

        if (selected_highlight_index != -1) {
            main_document_view->delete_highlight_with_index(selected_highlight_index);
            selected_highlight_index = -1;
        }
        validate_render();
    }

    else if (command->name == "delete_bookmark") {

        main_document_view->delete_closest_bookmark();
        validate_render();
    }
    //todo: check if still works after wstring
    else if (command->name == "search_selected_text_in_google_scholar") {
        search_google_scholar(selected_text);
    }
    else if (command->name == "open_selected_url") {
        open_web_url((selected_text).c_str());
    }
    else if (command->name == "search_selected_text_in_libgen") {
        search_libgen(selected_text);
    }
    else if (command->name == "toggle_dark_mode") {
        this->opengl_widget->toggle_dark_mode();
        helper_opengl_widget->toggle_dark_mode();
    }
    else if (command->name == "toggle_custom_color") {
        this->opengl_widget->toggle_custom_color_mode();
        helper_opengl_widget->toggle_custom_color_mode();
    }
    else if (command->name == "quit" || command->name == "q") {
		handle_close_event();
        QApplication::quit();
        return;
    }
    else if (command->name == "command") {
        QStringList command_names = command_manager.get_all_command_names();
        set_current_widget(new CommandSelector(
            &on_command_done, this, command_names, input_handler->get_command_key_mappings()));
        current_widget->show();
    }
    else if (command->name == "keys") {
        open_file(default_keys_path.get_path());
    }
    else if (command->name == "keys_user") {
        std::optional<Path> key_file_path = input_handler->get_or_create_user_keys_path();
        if (key_file_path) {
            open_file(key_file_path.value().get_path());
        }
    }
    else if (command->name == "prefs") {
        open_file(default_config_path.get_path());
    }
    else if (command->name == "prefs_user") {
        std::optional<Path> pref_file_path = config_manager->get_or_create_user_config_file();
        if (pref_file_path) {
            open_file(pref_file_path.value().get_path());
        }
    }
    else if (command->name == "prefs_user_all") {

        std::vector<Path> prefs_paths = config_manager->get_all_user_config_files();
        std::vector<std::wstring> prefs_paths_wstring;
        for (auto path : prefs_paths) {
            prefs_paths_wstring.push_back(path.get_path());
        }

        set_current_widget(new FilteredSelectWindowClass<std::wstring>(
            prefs_paths_wstring,
            prefs_paths_wstring,
            [&](std::wstring* path) {
                if (path) {
                    open_file(*path);
                }
            },
                this));
        current_widget->show();
    }
    else if (command->name == "keys_user_all") {

        std::vector<Path> keys_paths = input_handler->get_all_user_keys_paths();
        std::vector<std::wstring> keys_paths_wstring;
        for (auto path : keys_paths) {
            keys_paths_wstring.push_back(path.get_path());
        }

        set_current_widget(new FilteredSelectWindowClass<std::wstring>(
            keys_paths_wstring,
            keys_paths_wstring,
            [&](std::wstring* path) {
                if (path) {
                    open_file(*path);
                }
            },
                this));
        current_widget->show();
    }
    else if (command->name == "embed_annotations") {
        std::wstring embedded_pdf_file_name = select_new_pdf_file_name();
        if (embedded_pdf_file_name.size() > 0) {
            main_document_view->get_document()->embed_annotations(embedded_pdf_file_name);
        }
    }
    else if (command->name == "export") {
        std::wstring export_file_name = select_new_json_file_name();
        db_manager->export_json(export_file_name, checksummer);
    }
    else if (command->name == "import") {
        std::wstring import_file_name = select_json_file_name();
        db_manager->import_json(import_file_name, checksummer);
    }
    else if (command->name == "goto_left") {
		main_document_view->goto_left();
    }
    else if (command->name == "goto_left_smart") {
		main_document_view->goto_left_smart();
    }
    else if (command->name == "goto_right") {
		main_document_view->goto_right();
    }
    else if (command->name == "goto_right_smart") {
		main_document_view->goto_right_smart();
    }
    else if (command->name == "rotate_clockwise") {
		main_document_view->rotate();
        opengl_widget->rotate_clockwise();
    }
    else if (command->name == "rotate_counterclockwise") {
		main_document_view->rotate();
        opengl_widget->rotate_counterclockwise();
    }
    else if (command->name == "debug") {
    status_label->show();
    int swidth = status_label->width();
    int sheight = status_label->height();
    }
    else if (command->name == "toggle_fastread") {
		opengl_widget->toggle_fastread_mode();
	}
    else if (command->name == "smart_jump_under_cursor") {
        QPoint mouse_pos = mapFromGlobal(QCursor::pos());
        smart_jump_under_pos({ mouse_pos.x(), mouse_pos.y() });
    }
    else if (command->name == "overview_under_cursor") {
        QPoint mouse_pos = mapFromGlobal(QCursor::pos());
        overview_under_pos({ mouse_pos.x(), mouse_pos.y() });
    }
    else if (command->name == "close_overview") {
        opengl_widget->set_overview_page({});
    }
    else if (command->name == "visual_mark_under_cursor") {
        QPoint mouse_pos = mapFromGlobal(QCursor::pos());
        visual_mark_under_pos({ mouse_pos.x(), mouse_pos.y() });
    }
    else if (command->name == "enter_visual_mark_mode") {
        visual_mark_under_pos({ width()/2, height()/2});
    }
    else if (command->name == "close_visual_mark") {
        opengl_widget->set_should_draw_vertical_line(false);
    }
    else if (command->name == "toggle_horizontal_scroll_lock") {
        horizontal_scroll_locked = !horizontal_scroll_locked;
    }
    else if (command->name == "move_visual_mark_down") {
		if (opengl_widget->get_overview_page()) {
            scroll_overview_down();
        }
		else if (is_visual_mark_mode()) {
            move_visual_mark_down();
        }
        else {
            move_document(0.0f, 72.0f * rp * VERTICAL_MOVE_AMOUNT);
        }
		validate_render();
    }
    else if (command->name == "move_visual_mark_up") {
		if (opengl_widget->get_overview_page()) {
            scroll_overview_up();
        }
		else if (is_visual_mark_mode()) {
            move_visual_mark_up();
        }
        else {
            move_document(0.0f, -72.0f * rp * VERTICAL_MOVE_AMOUNT);
        }
		validate_render();
    }

    if (command->pushes_state) {
        push_state();
    }

    validate_render();
}

void MainWidget::smart_jump_under_pos(WindowPos pos){
    if (!main_document_view_has_document()) {
        return;
    }


    Qt::KeyboardModifiers modifiers = QGuiApplication::queryKeyboardModifiers();
    bool is_shift_pressed = modifiers.testFlag(Qt::ShiftModifier);

    auto [normal_x, normal_y] = main_document_view->window_to_normalized_window_pos(pos);

    // if overview page is open and we middle click on a paper name, search it in a search engine
    if (opengl_widget->is_window_point_in_overview({ normal_x, normal_y })) {
        auto [doc_page, doc_x, doc_y] = opengl_widget->window_pos_to_overview_pos({ normal_x, normal_y });
        std::optional<std::wstring> paper_name = main_document_view->get_document()->get_paper_name_at_position(doc_page, doc_x, doc_y);
        if (paper_name) {
            handle_paper_name_on_pointer(paper_name.value(), is_shift_pressed);
        }
        return;
    }

    auto [page, offset_x, offset_y] = main_document_view->window_to_document_pos(pos);

    fz_stext_page* stext_page = main_document_view->get_document()->get_stext_with_page_number(page);
    std::vector<fz_stext_char*> flat_chars;
    get_flat_chars_from_stext_page(stext_page, flat_chars);

    std::optional<std::pair<std::wstring, std::wstring>> generic_pair =\
            main_document_view->get_document()->get_generic_link_name_at_position(flat_chars, offset_x, offset_y);

    //std::optional<std::wstring> text_on_pointer = main_document_view->get_document()->get_text_at_position(flat_chars, offset_x, offset_y);
    std::optional<std::wstring> paper_name_on_pointer = main_document_view->get_document()->get_paper_name_at_position(flat_chars, offset_x, offset_y);
    std::optional<std::wstring> reference_text_on_pointer = main_document_view->get_document()->get_reference_text_at_position(flat_chars, offset_x, offset_y);
    std::optional<std::wstring> equation_text_on_pointer = main_document_view->get_document()->get_equation_text_at_position(flat_chars, offset_x, offset_y);

    if (generic_pair) {
        int page;
        float y_offset;

        if (main_document_view->get_document()->find_generic_location(generic_pair.value().first,
                                                                      generic_pair.value().second,
                                                                      &page,
                                                                      &y_offset)) {

            long_jump_to_destination(page, y_offset);
            return;
        }
    }
    if (equation_text_on_pointer) {
        std::optional<IndexedData> eqdata_ = main_document_view->get_document()->find_equation_with_string(
                    equation_text_on_pointer.value(),
                    get_current_page_number());
        if (eqdata_) {
            IndexedData refdata = eqdata_.value();
            long_jump_to_destination(refdata.page, refdata.y_offset);
            return;
        }
    }

    if (reference_text_on_pointer) {
        std::optional<IndexedData> refdata_ = main_document_view->get_document()->find_reference_with_string(reference_text_on_pointer.value());
        if (refdata_) {
            IndexedData refdata = refdata_.value();
            long_jump_to_destination(refdata.page, refdata.y_offset);
            return;
        }

    }
    if (paper_name_on_pointer) {
        handle_paper_name_on_pointer(paper_name_on_pointer.value(), is_shift_pressed);
    }
}

void MainWidget::visual_mark_under_pos(WindowPos pos){
    //float doc_x, doc_y;
    //int page;
    DocumentPos document_pos = main_document_view->window_to_document_pos(pos);
    if (document_pos.page != -1) {
        opengl_widget->set_should_draw_vertical_line(true);
        fz_pixmap* pixmap = main_document_view->get_document()->get_small_pixmap(document_pos.page);
        std::vector<unsigned int> hist = get_max_width_histogram_from_pixmap(pixmap);
        std::vector<unsigned int> line_locations;
        std::vector<unsigned int> _;
        get_line_begins_and_ends_from_histogram(hist, _, line_locations);
        int small_doc_x = static_cast<int>(document_pos.x * SMALL_PIXMAP_SCALE);
        int small_doc_y = static_cast<int>(document_pos.y * SMALL_PIXMAP_SCALE);
        int best_vertical_loc = find_best_vertical_line_location(pixmap, small_doc_x, small_doc_y);
        //int best_vertical_loc = line_locations[find_nth_larger_element_in_sorted_list(line_locations, static_cast<unsigned int>(small_doc_y), 2)];
        float best_vertical_loc_doc_pos = best_vertical_loc / SMALL_PIXMAP_SCALE;
        WindowPos window_pos = main_document_view->document_to_window_pos_in_pixels({document_pos.page, 0, best_vertical_loc_doc_pos});
        auto [abs_doc_x, abs_doc_y] = main_document_view->window_to_absolute_document_pos(window_pos);
        main_document_view->set_vertical_line_pos(abs_doc_y);
        main_document_view->set_line_index(main_document_view->get_line_index_of_vertical_pos());
        validate_render();
    }
}


bool MainWidget::overview_under_pos(WindowPos pos){

    std::optional<PdfLink> link;
    if (main_document_view && (link = main_document_view->get_link_in_pos(pos))) {
        set_overview_link(link.value());
        return true;
    }

    int autoreference_page;
    float autoreference_offset;
    if (find_location_of_text_under_pointer(pos, &autoreference_page, &autoreference_offset)) {
        set_overview_position(autoreference_page, autoreference_offset);
        return true;
    }

    return false;
}

void MainWidget::toggle_synctex_mode(){
    this->synctex_mode = !this->synctex_mode;
    if (this->synctex_mode){
        // we disable overview image in synctex mode
        this->opengl_widget->set_overview_page({});
    }
}

void MainWidget::handle_link() {
    if (!main_document_view_has_document()) return;

    if (is_pending_link_source_filled()) {
        auto [source_path, pl] = pending_link.value();
        pl.dst = main_document_view->get_checksum_state();
        pending_link = {};

        if (source_path == main_document_view->get_document()->get_path()) {
            main_document_view->get_document()->add_link(pl);
        }
        else {
            const std::unordered_map<std::wstring, Document*> cached_documents = document_manager->get_cached_documents();
            for (auto [doc_path, doc] : cached_documents) {
                if (source_path == doc_path) {
                    doc->add_link(pl, false);
                }
            }

            db_manager->insert_link(checksummer->get_checksum(source_path.value()),
                pl.dst.document_checksum,
                pl.dst.book_state.offset_x,
                pl.dst.book_state.offset_y,
                pl.dst.book_state.zoom_level,
                pl.src_offset_y);
        }
    }
    else {
        pending_link = std::make_pair<std::wstring, Link>(main_document_view->get_document()->get_path(),
            Link::with_src_offset(main_document_view->get_offset_y()));
    }
    validate_render();
}

void MainWidget::handle_pending_text_command(std::wstring text) {
    if (current_pending_command->name == "search" ||
        current_pending_command->name == "ranged_search" ||
        current_pending_command->name == "chapter_search" ) {

        // When searching, the start position before search is saved in a mark named '0'
        main_document_view->add_mark('/');

        int range_begin, range_end;
        std::wstring search_term;
        std::optional<std::pair<int, int>> search_range = {};
        if (parse_search_command(text, &range_begin, &range_end, &search_term)) {
            search_range = std::make_pair(range_begin, range_end);
        }

        if (search_term.size() > 0) {
            // in mupdf RTL documents are reversed, so we reverse the search string
            //todo: better (or any!) handling of mixed RTL and LTR text
            if (is_rtl(search_term[0])) {
                search_term = reverse_wstring(search_term);
            }
        }

        opengl_widget->search_text(search_term, search_range);
    }

    if (current_pending_command->name == "enter_password") {
        std::string password = utf8_encode(text);
        bool success = main_document_view->get_document()->apply_password(password.c_str());
        pdf_renderer->add_password(main_document_view->get_document()->get_path(), password);
    }

    if (current_pending_command->name == "add_bookmark") {
        main_document_view->add_bookmark(text);
    }
    if (current_pending_command->name == "execute") {

        execute_command(text);
    }

    if (current_pending_command->name == "open_link") {
        std::vector<int> visible_pages;
        std::vector<std::pair<int, fz_link*>> visible_page_links;

        if (is_string_numeric(text)) {

            int link_index = std::stoi(text);

            main_document_view->get_visible_pages(main_document_view->get_view_height(), visible_pages);
            for (auto page : visible_pages){
                fz_link* link = main_document_view->get_document()->get_page_links(page);
                while (link) {
                    visible_page_links.push_back(std::make_pair(page, link));
                    link = link->next;
                }
            }
            if ((link_index >= 0) && (link_index < static_cast<int>(visible_page_links.size()))) {
                auto [selected_page, selected_link] = visible_page_links[link_index];
                auto [page, offset_x, offset_y] = parse_uri(selected_link->uri);
                long_jump_to_destination(page-1, offset_y);
            }
        }
        opengl_widget->set_highlight_links(false, false);
    }
    if (current_pending_command->name == "keyboard_select") {

        QStringList parts = QString::fromStdWString(text).split(' ');

        if (parts.size() == 2) {

            fz_irect srect = get_tag_window_rect(parts.at(0).toStdString());
            fz_irect erect = get_tag_window_rect(parts.at(1).toStdString());

            handle_left_click({ srect.x0 + 5, (srect.y0 + srect.y1) / 2 }, true);
            handle_left_click({ erect.x0 - 5 , (erect.y0 + erect.y1) / 2 }, false);
            opengl_widget->set_should_highlight_words(false);
		}
	}
    if (current_pending_command->name == "keyboard_smart_jump") {
		fz_irect rect = get_tag_window_rect(utf8_encode(text));
        smart_jump_under_pos({ (rect.x0 + rect.x1) / 2, (rect.y0 + rect.y1) / 2 });
		opengl_widget->set_should_highlight_words(false);
	}

    if (current_pending_command->name == "keyboard_overview") {
        fz_irect rect = get_tag_window_rect(utf8_encode(text));
        overview_under_pos({(rect.x0 + rect.x1) / 2, (rect.y0 + rect.y1) / 2});
		opengl_widget->set_should_highlight_words(false);
	}

    if (current_pending_command->name == "goto_page_with_page_number") {

        if (is_string_numeric(text.c_str()) && text.size() < 6) { // make sure the page number is valid
            int dest = std::stoi(text.c_str()) - 1;
            main_document_view->goto_page(dest + main_document_view->get_page_offset());
        }
    }

    if (current_pending_command->name == "set_page_offset") {

        if (is_string_numeric(text.c_str()) && text.size() < 6) { // make sure the page number is valid
            main_document_view->set_page_offset(std::stoi(text.c_str()));
        }
    }

    if (current_pending_command->name == "command") {
        //Path standard_data_path(QStandardPaths::standardLocations(QStandardPaths::AppDataLocation).at(0).toStdWString());

        if (text == L"q") {
            close();
        }
        else if (text == L"keys") {
            open_file(default_keys_path.get_path());
        }
        else if (text == L"keys_user") {
            std::optional<Path> key_file_path = input_handler->get_or_create_user_keys_path();
            if (key_file_path) {
                open_file(key_file_path.value().get_path());
            }
        }
        else if (text == L"prefs") {
            open_file(default_config_path.get_path());
        }
        else if (text == L"prefs_user") {
            std::optional<Path> pref_file_path = config_manager->get_or_create_user_config_file();
            if (pref_file_path) {
                open_file(pref_file_path.value().get_path());
            }
        }
        else if (text == L"export") {
            std::wstring export_file_name = select_new_json_file_name();
            db_manager->export_json(export_file_name, checksummer);
        }
        else if (text == L"import") {
            std::wstring import_file_name = select_json_file_name();
            db_manager->import_json(import_file_name, checksummer);
        }
        else{
            const Command* command = command_manager.get_command_with_name(utf8_encode(text));
            if (command != nullptr) {
                handle_command(command, 1);
            }
        }
    }
}

void MainWidget::toggle_fullscreen() {
    if (isFullScreen()) {
        helper_opengl_widget->setWindowState(Qt::WindowState::WindowMaximized);
        setWindowState(Qt::WindowState::WindowMaximized);
    }
    else {
        helper_opengl_widget->setWindowState(Qt::WindowState::WindowFullScreen);
        setWindowState(Qt::WindowState::WindowFullScreen);
    }
}

void MainWidget::toggle_presentation_mode() {
    if (opengl_widget->is_presentation_mode()) {
        opengl_widget->set_visible_page_number({});
    }
    else {
        opengl_widget->set_visible_page_number(get_current_page_number());
    }
}

void MainWidget::complete_pending_link(const LinkViewState& destination_view_state) {
    Link& pl = pending_link.value().second;
    pl.dst = destination_view_state;
    main_document_view->get_document()->add_link(pl);
    pending_link = {};
}

void MainWidget::long_jump_to_destination(int page, float offset_y) {
    long_jump_to_destination({ page, main_document_view->get_offset_x(), offset_y });
}


void MainWidget::long_jump_to_destination(float abs_offset_y) {

    auto [page, _, offset_y] = main_document_view->get_document()->absolute_to_page_pos(
        { main_document_view->get_offset_x(), abs_offset_y });
    long_jump_to_destination(page, offset_y);
}

void MainWidget::long_jump_to_destination(DocumentPos pos) {
    if (!is_pending_link_source_filled()) {
        update_history_state();
        main_document_view->goto_offset_within_page({ pos.page, pos.x, pos.y });
        push_state();
    }
    else {
        // if we press the link button and then click on a pdf link, we automatically link to the
        // link's destination

        LinkViewState dest_state;
        dest_state.document_checksum = main_document_view->get_document()->get_checksum();
        dest_state.book_state.offset_x = pos.x;
        dest_state.book_state.offset_y = main_document_view->get_page_offset(pos.page) + pos.y;
        dest_state.book_state.zoom_level = main_document_view->get_zoom_level();

        complete_pending_link(dest_state);
    }
    invalidate_render();
}

void MainWidget::set_current_widget(QWidget* new_widget) {
    if (current_widget != nullptr) {
        garbage_widgets.push_back(current_widget);
    }
    current_widget = new_widget;

    if (garbage_widgets.size() > 2) {
        delete garbage_widgets[0];
        garbage_widgets.erase(garbage_widgets.begin());
    }
}

bool MainWidget::focus_on_visual_mark_pos(bool moving_down) {
    //float window_x, window_y;

    if (!main_document_view->get_ruler_window_rect().has_value()) {
		return false;
    }

    float thresh = 1 - VISUAL_MARK_NEXT_PAGE_THRESHOLD;
    fz_rect ruler_window_rect = main_document_view->get_ruler_window_rect().value();
    //main_document_view->absolute_to_window_pos(0, main_document_view->get_vertical_line_pos(), &window_x, &window_y);
    //if ((window_y < -thresh) || (window_y > thresh)) {
    if (ruler_window_rect.y0 < -1 || ruler_window_rect.y1 > 1) {
        main_document_view->goto_vertical_line_pos();
        return true;
    }
    if ((moving_down && (ruler_window_rect.y0 < -thresh)) || ((!moving_down) && (ruler_window_rect.y1 > thresh))) {
        main_document_view->goto_vertical_line_pos();
        //float distance = (main_document_view->get_view_height() / main_document_view->get_zoom_level()) * VISUAL_MARK_NEXT_PAGE_FRACTION;
        //main_document_view->move_absolute(0, -distance);
        return true;
    }
    return false;
}

void MainWidget::toggle_visual_scroll_mode() {
    visual_scroll_mode = !visual_scroll_mode;
}

std::optional<std::wstring> MainWidget::get_current_file_name() {
    if (main_document_view) {
        if (main_document_view->get_document()) {
            return main_document_view->get_document()->get_path();
        }
    }
    return {};
}

CommandManager* MainWidget::get_command_manager(){
    return &command_manager;
}

void MainWidget::toggle_dark_mode() {
    this->opengl_widget->toggle_dark_mode();
}

void MainWidget::execute_command(std::wstring command) {

    std::wstring file_path = main_document_view->get_document()->get_path();
    QString qfile_path = QString::fromStdWString(file_path);
    std::vector<std::wstring> path_parts;
    split_path(file_path, path_parts);
    std::wstring file_name = path_parts.back();
    QString qfile_name = QString::fromStdWString(file_name);

    QString qtext = QString::fromStdWString(command);

    qtext.arg(qfile_path);

    QStringList command_parts = qtext.split(QRegExp("\\s+"), QString::SkipEmptyParts);
    if (command_parts.size() > 0) {
        QString command_name = command_parts[0];
        QStringList command_args;

        command_parts.takeFirst();

        // you would think
        // "command %2".arg("first", "second");
        // would expand into
        // "command second"
        // and you would be wrong, for some reason qt decided the lowest numbered
        // %n should be filled with the first argument and so on. what follows is a hack to get around this

        for (int i = 0; i < command_parts.size(); i++) {
            command_parts[i].replace("%1", qfile_path);
            command_parts[i].replace("%2", qfile_name);
            command_parts[i].replace("%3", QString::fromStdWString(selected_text));
            command_parts[i].replace("%4", QString::number(get_current_page_number()));
            command_args.push_back(command_parts[i]);

            //bool part_requires_only_second = (command_parts[i].arg("%1", "%2") != command_parts[i]);
            //if (part_requires_only_second) {
            //    command_args.push_back(command_parts.at(i).arg(qfile_name));
            //}
            //else {
            //    command_args.push_back(command_parts.at(i).arg(qfile_path, qfile_name));
            //}
        }

        run_command(command_name.toStdWString(), command_args, false);
    }

}
void MainWidget::handle_paper_name_on_pointer(std::wstring paper_name, bool is_shift_pressed) {
    if (paper_name.size() > 5) {
        char type;
        if (is_shift_pressed) {
            type = SHIFT_MIDDLE_CLICK_SEARCH_ENGINE[0];
        }
        else {
            type = MIDDLE_CLICK_SEARCH_ENGINE[0];
        }
        if ((type >= 'a') && (type <= 'z')) {
            search_custom_engine(paper_name, SEARCH_URLS[type - 'a']);
        }
    }

}
void MainWidget::move_vertical(float amount) {
    move_document(0, amount);
    validate_render();

}
void MainWidget::move_horizontal(float amount){
    if (!horizontal_scroll_locked) {
        move_document(amount, 0);
        validate_render();
    }
}

std::optional<std::string> MainWidget::get_last_opened_file_checksum() {

    std::vector<std::wstring> opened_docs_hashes;
    std::wstring current_checksum = L"";
    if (main_document_view_has_document()) {
        current_checksum = utf8_decode(main_document_view->get_document()->get_checksum());
    }

    db_manager->select_opened_books_path_values(opened_docs_hashes);

    size_t index = 0;
    while (index < opened_docs_hashes.size()) {
        if (opened_docs_hashes[index] == current_checksum) {
            index++;
        }
        else {
            return utf8_encode(opened_docs_hashes[index]);
        }
    }

    return {};
}
void MainWidget::get_window_params_for_one_window_mode(int* main_window_size, int* main_window_move){
    if (SINGLE_MAIN_WINDOW_SIZE[0] >= 0) {
        main_window_size[0] = SINGLE_MAIN_WINDOW_SIZE[0];
        main_window_size[1] = SINGLE_MAIN_WINDOW_SIZE[1];
        main_window_move[0] = SINGLE_MAIN_WINDOW_MOVE[0];
        main_window_move[1] = SINGLE_MAIN_WINDOW_MOVE[1];
    }
    else{
        int window_width = QApplication::desktop()->screenGeometry(0).width();
        int window_height = QApplication::desktop()->screenGeometry(0).height();
        main_window_size[0] = window_width;
        main_window_size[1] = window_height;
        main_window_move[0] = 0;
        main_window_move[1] = 0;
    }
}
void MainWidget::get_window_params_for_two_window_mode(int* main_window_size, int* main_window_move, int* helper_window_size, int* helper_window_move) {
    if (MAIN_WINDOW_SIZE[0] >= 0) {
        main_window_size[0] = MAIN_WINDOW_SIZE[0];
        main_window_size[1] = MAIN_WINDOW_SIZE[1];
        main_window_move[0] = MAIN_WINDOW_MOVE[0];
        main_window_move[1] = MAIN_WINDOW_MOVE[1];
        helper_window_size[0] = HELPER_WINDOW_SIZE[0];
        helper_window_size[1] = HELPER_WINDOW_SIZE[1];
        helper_window_move[0] = HELPER_WINDOW_MOVE[0];
        helper_window_move[1] = HELPER_WINDOW_MOVE[1];
    }
    else {
        int num_screens = QApplication::desktop()->numScreens();
        int main_window_width = QApplication::desktop()->screenGeometry(0).width();
        int main_window_height = QApplication::desktop()->screenGeometry(0).height();
        if (num_screens > 1) {
            int second_window_width = QApplication::desktop()->screenGeometry(1).width();
            int second_window_height = QApplication::desktop()->screenGeometry(1).height();
            main_window_size[0] = main_window_width;
            main_window_size[1] = main_window_height;
            main_window_move[0] = 0;
            main_window_move[1] = 0;
            helper_window_size[0] = second_window_width;
            helper_window_size[1] = second_window_height;
            helper_window_move[0] = main_window_width;
            helper_window_move[1] = 0;
        }
        else {
            main_window_size[0] = main_window_width / 2;
            main_window_size[1] = main_window_height;
            main_window_move[0] = 0;
            main_window_move[1] = 0;
            helper_window_size[0] = main_window_width / 2;
            helper_window_size[1] = main_window_height;
            helper_window_move[0] = main_window_width / 2;
            helper_window_move[1] = 0;
        }
    }
}

void MainWidget::apply_window_params_for_one_window_mode(bool force_resize){

    QWidget* main_window = get_top_level_widget(opengl_widget);

    int main_window_width = QApplication::desktop()->screenGeometry(0).width();

    int main_window_size[2];
    int main_window_move[2];

    get_window_params_for_one_window_mode(main_window_size, main_window_move);

    bool should_maximize = main_window_width == main_window_size[0];

    if (should_maximize) {
        main_window->hide();
        if (force_resize) {
			main_window->resize(main_window_size[0], main_window_size[1]);
        }
        main_window->showMaximized();
    }
    else {
        main_window->move(main_window_move[0], main_window_move[1]);
        main_window->resize(main_window_size[0], main_window_size[1]);
    }


    if (helper_opengl_widget != nullptr) {
        helper_opengl_widget->hide();
        helper_opengl_widget->move(0, 0);
        helper_opengl_widget->resize(main_window_size[0], main_window_size[1]);
    }
}

void MainWidget::apply_window_params_for_two_window_mode() {
    QWidget* main_window = get_top_level_widget(opengl_widget);
    QWidget* helper_window = get_top_level_widget(helper_opengl_widget);

    int main_window_width = QApplication::desktop()->screenGeometry(0).width();

    int main_window_size[2];
    int main_window_move[2];
    int helper_window_size[2];
    int helper_window_move[2];

    get_window_params_for_two_window_mode(main_window_size, main_window_move, helper_window_size, helper_window_move);

    bool should_maximize = main_window_width == main_window_size[0];


    if (helper_opengl_widget != nullptr) {
        helper_window->move(helper_window_move[0], helper_window_move[1]);
        helper_window->resize(helper_window_size[0], helper_window_size[1]);
        helper_window->show();
    }

    if (should_maximize) {
        main_window->hide();
        main_window->showMaximized();
    }
    else {
		main_window->move(main_window_move[0], main_window_move[1]);
		main_window->resize(main_window_size[0], main_window_size[1]);
    }
}

QRect MainWidget::get_main_window_rect() {
    QPoint main_window_pos = pos();
    QSize main_window_size = size();
    return QRect(main_window_pos, main_window_size);
}

QRect MainWidget::get_helper_window_rect() {
    QPoint helper_window_pos = helper_opengl_widget->pos();
    QSize helper_window_size = helper_opengl_widget->size();
    return QRect(helper_window_pos, helper_window_size);
}

void MainWidget::open_document(const std::wstring& doc_path,
    bool* invalid_flag,
    bool load_prev_state,
    std::optional<OpenedBookState> prev_state,
    bool force_load_dimensions) {
    main_document_view->open_document(doc_path, invalid_flag, load_prev_state, prev_state, force_load_dimensions);

    std::optional<std::wstring> filename = Path(doc_path).filename();
    if (filename) {
        setWindowTitle(QString::fromStdWString(filename.value()));
    }
}

#ifndef Q_OS_MACOS
void MainWidget::dragEnterEvent(QDragEnterEvent* e)
{
	e->acceptProposedAction();

}

void MainWidget::dropEvent(QDropEvent* event)
{
    if (event->mimeData()->hasUrls()) {
        auto urls = event->mimeData()->urls();
        std::wstring path = urls.at(0).toString().toStdWString();
        // ignore file:/// at the begining of the URL
#ifdef Q_OS_WIN
        path = path.substr(8, path.size() - 8);
#else
        path = path.substr(7, path.size() - 7);
#endif
        //handle_args(QStringList() << QApplication::applicationFilePath() << QString::fromStdWString(path));
        open_document(path, &is_render_invalidated);
    }
}
#endif

void MainWidget::highlight_words() {

    int page = get_current_page_number();
    fz_stext_page* stext_page = main_document_view->get_document()->get_stext_with_page_number(page);
    std::vector<fz_stext_char*> flat_chars;
    std::vector<fz_rect> word_rects;
    std::vector<std::pair<fz_rect, int>> word_rects_with_page;

    get_flat_chars_from_stext_page(stext_page, flat_chars);
    get_flat_words_from_flat_chars(flat_chars, word_rects);
    for (auto rect : word_rects) {
        word_rects_with_page.push_back(std::make_pair(rect, page));
    }
    opengl_widget->set_highlight_words(word_rects_with_page);
    opengl_widget->set_should_highlight_words(true);

}

std::vector<fz_rect> MainWidget::get_flat_words() {
    int page = get_current_page_number();
    return main_document_view->get_document()->get_page_flat_words(page);
}

fz_irect MainWidget::get_tag_window_rect(std::string tag) {

    int page = get_current_page_number();
    fz_rect rect = get_tag_rect(tag);

    fz_irect window_rect;

    WindowPos bottom_left =  main_document_view->document_to_window_pos_in_pixels({ page, rect.x0, rect.y0 });
    WindowPos top_right = main_document_view->document_to_window_pos_in_pixels({ page, rect.x1, rect.y1 });
    window_rect.x0 = bottom_left.x;
    window_rect.y0 = bottom_left.y;
    window_rect.x1 = top_right.x;
    window_rect.y1 = top_right.y;

    return window_rect;
}

fz_rect MainWidget::get_tag_rect(std::string tag) {
	std::vector<fz_rect> word_rects = get_flat_words();
	int index = get_index_from_tag(tag);
    return word_rects[index];

}

bool MainWidget::is_rotated() {
    return opengl_widget->is_rotated();
}

void MainWidget::show_password_prompt_if_required() {
	if (main_document_view && (main_document_view->get_document() != nullptr)) {
		if (main_document_view->get_document()->needs_authentication()) {
			if (current_pending_command == nullptr || current_pending_command->name != "enter_password") {
				handle_command(command_manager.get_command_with_name("enter_password"), 1);
			}
		}
	}
}

void MainWidget::on_new_paper_added(const std::wstring& file_path) {
    if (is_pending_link_source_filled()) {
        LinkViewState dst_view_state;

        dst_view_state.book_state.offset_x = 0;
        dst_view_state.book_state.offset_y = 0;
        dst_view_state.book_state.zoom_level = 1;
        Document* new_doc = document_manager->get_document(file_path);
        new_doc->open(nullptr, false, "", true);
        fz_rect first_page_rect = new_doc->get_page_rect_no_cache(0);
        document_manager->free_document(new_doc);
        float first_page_width = first_page_rect.x1 - first_page_rect.x0;
        float first_page_height = first_page_rect.y1 - first_page_rect.y0;

        dst_view_state.document_checksum = checksummer->get_checksum(file_path);

        if (helper_document_view) {
            float helper_view_width = helper_document_view->get_view_width();
            float helper_view_height = helper_document_view->get_view_height();
            float zoom_level = helper_view_width / first_page_width; 
            dst_view_state.book_state.zoom_level = zoom_level;
			dst_view_state.book_state.offset_y = -std::abs(-helper_view_height / zoom_level / 2 + first_page_height / 2);
        }
        complete_pending_link(dst_view_state);
        invalidate_render();
    }
}
void MainWidget::handle_link_click(const PdfLink& link) {

	if (link.uri.substr(0, 4).compare("http") == 0) {
		open_web_url(utf8_decode(link.uri));
		return;
	}

	auto [page, offset_x, offset_y] = parse_uri(link.uri);

	// convert one indexed page to zero indexed page
	page--;

	// we usually just want to center the y offset and not the x offset (otherwise for example
	// a link at the right side of the screen will be centered, causing most of screen state to be empty)
	offset_x = main_document_view->get_offset_x();

    long_jump_to_destination({ page, offset_x, offset_y });
}

void MainWidget::save_auto_config() {
    std::wofstream outfile(auto_config_path.get_path_utf8());
    outfile << get_serialized_configuration_string();
    outfile.close();
}

std::wstring MainWidget::get_serialized_configuration_string() {
    float overview_size[2];
    float overview_offset[2];
    opengl_widget->get_overview_offsets(&overview_offset[0], &overview_offset[1]);
    opengl_widget->get_overview_size(&overview_size[0], &overview_size[1]);

    QString overview_config = "overview_size %1 %2\noverview_offset %3 %4\n";
    std::wstring overview_config_string = overview_config.arg(QString::number(overview_size[0]),
        QString::number(overview_size[1]),
        QString::number(overview_offset[0]),
        QString::number(overview_offset[1])).toStdWString();
    return overview_config_string + get_window_configuration_string();
}
std::wstring MainWidget::get_window_configuration_string() {
	
	QString config_string_multi = "main_window_size    %1 %2\nmain_window_move     %3 %4\nhelper_window_size    %5 %6\nhelper_window_move     %7 %8";
	QString config_string_single = "single_main_window_size    %1 %2\nsingle_main_window_move     %3 %4";

	QString main_window_size_w = QString::number(size().width());
	QString main_window_size_h = QString::number(size().height());
	QString helper_window_size_w = QString::number(-1);
	QString helper_window_size_h = QString::number(-1);
	QString main_window_move_x = QString::number(pos().x());
	QString main_window_move_y = QString::number(pos().y());
	QString helper_window_move_x = QString::number(-1);
	QString helper_window_move_y = QString::number(-1);

	if (helper_opengl_widget->isVisible()) {
		helper_window_size_w = QString::number(helper_opengl_widget->size().width());
		helper_window_size_h = QString::number(helper_opengl_widget->size().height());
		helper_window_move_x = QString::number(helper_opengl_widget->pos().x());
		helper_window_move_y = QString::number(helper_opengl_widget->pos().y());
		return (config_string_multi.arg(main_window_size_w,
			main_window_size_h,
			main_window_move_x,
			main_window_move_y,
			helper_window_size_w,
			helper_window_size_h,
			helper_window_move_x,
			helper_window_move_y).toStdWString());
	}
	else {
		return (config_string_single.arg(main_window_size_w,
			main_window_size_h,
			main_window_move_x,
			main_window_move_y).toStdWString());
	}
}

void MainWidget::handle_close_event() {
	save_auto_config();
	persist();

	// we need to delete this here (instead of destructor) to ensure that application
	// closes immediately after the main window is closed
	delete helper_opengl_widget;
}

Document* MainWidget::doc() {
    return main_document_view->get_document();
}

void MainWidget::return_to_last_visual_mark() {
	main_document_view->goto_vertical_line_pos();
	opengl_widget->set_should_draw_vertical_line(true);
	current_pending_command = nullptr;
	validate_render();
}

void MainWidget::changeEvent(QEvent* event) {
    if (event->type() == QEvent::WindowStateChange) {
        if (isMaximized()) {
			main_window_width = QApplication::desktop()->screenGeometry(0).width();
			main_window_height = QApplication::desktop()->screenGeometry(0).height();
		}
    }
    QWidget::changeEvent(event);
}

void MainWidget::move_visual_mark_down() {

    int prev_line_index = main_document_view->get_line_index();
    int new_line_index, new_page;
    int vertical_line_page = main_document_view->get_vertical_line_page();
    fz_rect ruler_rect = doc()->get_ith_next_line_from_absolute_y(vertical_line_page, prev_line_index, 1, true, &new_line_index, &new_page);
    main_document_view->set_line_index(new_line_index);
	main_document_view->set_vertical_line_rect(ruler_rect);
	if (focus_on_visual_mark_pos(true)) {
		float distance = (main_document_view->get_view_height() / main_document_view->get_zoom_level()) * VISUAL_MARK_NEXT_PAGE_FRACTION / 2;
		main_document_view->move_absolute(0, distance);
	}
}

void MainWidget::move_visual_mark_up() {
    int prev_line_index = main_document_view->get_line_index();
    int new_line_index, new_page;
    int vertical_line_page = main_document_view->get_vertical_line_page();
    fz_rect ruler_rect = doc()->get_ith_next_line_from_absolute_y(vertical_line_page, prev_line_index, -1, true, &new_line_index, &new_page);
    main_document_view->set_line_index(new_line_index);
	main_document_view->set_vertical_line_rect(ruler_rect);
	if (focus_on_visual_mark_pos(false)) {
		float distance = (main_document_view->get_view_height() / main_document_view->get_zoom_level()) * VISUAL_MARK_NEXT_PAGE_FRACTION / 2;
		main_document_view->move_absolute(0, distance);
	}
}
//void MainWidget::move_visual_mark_up() {
//	float new_pos, new_begin_pos;
//	doc()->get_ith_next_line_from_absolute_y(main_document_view->get_vertical_line_end_pos(), -2, true, &new_begin_pos, &new_pos);
//	main_document_view->set_vertical_line_pos(new_pos, new_begin_pos);
//	if (focus_on_visual_mark_pos(false)) {
//		float distance = (main_document_view->get_view_height() / main_document_view->get_zoom_level()) * VISUAL_MARK_NEXT_PAGE_FRACTION / 2;
//		main_document_view->move_absolute(0, -distance);
//	}
//}

bool MainWidget::is_visual_mark_mode() {
    return opengl_widget->get_should_draw_vertical_line();
}

void MainWidget::scroll_overview_down() {
    float vertical_move_amount = VERTICAL_MOVE_AMOUNT * TOUCHPAD_SENSITIVITY;
	OverviewState state = opengl_widget->get_overview_page().value();
	state.offset_y += 36.0f * vertical_move_amount;
	opengl_widget->set_overview_page(state);
}

void MainWidget::scroll_overview_up() {
    float vertical_move_amount = VERTICAL_MOVE_AMOUNT * TOUCHPAD_SENSITIVITY;
	OverviewState state = opengl_widget->get_overview_page().value();
	state.offset_y -= 36.0f * vertical_move_amount;
	opengl_widget->set_overview_page(state);

}

int MainWidget::get_current_page_number() const {
    // 
    if (opengl_widget->get_should_draw_vertical_line()) {
        return main_document_view->get_vertical_line_page();
    }
    else {
        return main_document_view->get_center_page_number();
    }
}

void MainWidget::set_inverse_search_command(const std::wstring& new_command) {
    inverse_search_command = new_command;
}
