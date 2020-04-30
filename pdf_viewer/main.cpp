//todo: cleanup the code
//todo: visibility test is still buggy??
//todo: add fuzzy search
//todo: handle document memory leak (because documents are not deleted since adding state history)
//todo: tests!
//todo: handle right to left documents

//#include "imgui.h"
//#include "imgui_impl_sdl.h"
//#include "imgui_impl_opengl3.h"

#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <thread>
#include <mutex>
#include <optional>
#include <utility>
#include <memory>

#include <qapplication.h>
#include <qpushbutton.h>
#include <qopenglwidget.h>
#include <qopenglextrafunctions.h>
#include <qopenglfunctions.h>
#include <qopengl.h>
#include <qwindow.h>
#include <qkeyevent.h>
#include <qlineedit.h>
#include <qtreeview.h>
#include <qsortfilterproxymodel.h>
#include <qabstractitemmodel.h>
#include <qopenglshaderprogram.h>
#include <qtimer.h>
#include <qdatetime.h>
#include <qstackedwidget.h>
#include <qboxlayout.h>
#include <qlistview.h>
#include <qstringlistmodel.h>
#include <qlabel.h>
#include <qtextedit.h>
#include <qfilesystemwatcher.h>
#include <qdesktopwidget.h>

#include <Windows.h>
#include <mupdf/fitz.h>
#include "sqlite3.h"
#include <filesystem>

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

#define FTS_FUZZY_MATCH_IMPLEMENTATION
#include "fts_fuzzy_match.h"
#undef FTS_FUZZY_MATCH_IMPLEMENTATION


extern float background_color[3] = { 1.0f, 1.0f, 1.0f };
extern float ZOOM_INC_FACTOR = 1.2f;
extern float vertical_move_amount = 1.0f;
extern float horizontal_move_amount = 1.0f;
extern const unsigned int cache_invalid_milies = 1000;
extern const int persist_milies = 1000 * 60;
extern const int page_paddings = 0;
extern const int max_pending_requests = 31;

wstring last_path_file_absolute_location;
extern filesystem::path parent_path = "";


using namespace std;

mutex mupdf_mutexes[FZ_LOCK_MAX];

void lock_mutex(void* user, int lock) {
	mutex* mut = (mutex*)user;
	(mut + lock)->lock();
}

void unlock_mutex(void* user, int lock) {
	mutex* mut = (mutex*)user;
	(mut + lock)->unlock();
}




class MainWidget : public QWidget {

private:
	PdfViewOpenGLWidget* opengl_widget;
	PdfViewOpenGLWidget* helper_opengl_widget;
	bool is_waiting_for_symbol = false;

	//todo: see if these two can be merged
	const Command* current_pending_command = nullptr;
	const Command* pending_text_command = nullptr;

	InputHandler* input_handler;
	DocumentView* main_document_view;
	DocumentView* helper_document_view;
	// current widget responsible for selecting an option (for example toc or bookmarks)
	//QWidget* current_widget;
	unique_ptr<QWidget> current_widget;

	fz_context* mupdf_context;
	sqlite3* db;
	DocumentManager* document_manager;
	CommandManager command_manager;
	ConfigManager* config_manager;
	PdfRenderer* pdf_renderer;

	vector<DocumentViewState> history;
	int current_history_index;

	// last position when mouse was clicked in absolute document space
	float last_mouse_down_x;
	float last_mouse_down_y;
	bool is_selecting;
	optional<fz_rect> selected_rect;
	//vector<fz_rect> selected_character_rects;

	wstring selected_text;

	Link* link_to_edit;

	wstring pending_link_source_document_path;
	Link pending_link;
	bool pending_link_source_filled;

	int main_window_width, main_window_height;

	QLineEdit* text_command_line_edit;
	QTextEdit* status_label;

	bool is_render_invalidated = false;
	bool is_ui_invalidated = false;

	bool should_delete_old_pages = false;

	QTimer garbage_collect_timer;

protected:


	void paintEvent(QPaintEvent* paint_event) override {
		QWidget::paintEvent(paint_event);

		// we only try to delete old pages after a render event has occured to ensure we
		// don't delete any immediately-useful pages
		should_delete_old_pages = true;
	}

	void resizeEvent(QResizeEvent* resize_event) override {
		main_window_width = size().width();
		main_window_height = size().height();

		text_command_line_edit->move(0, 0);
		text_command_line_edit->resize(main_window_width, 30);

		status_label->move(0, main_window_height - 20);
		status_label->resize(main_window_width, 20);
	}


	void closeEvent(QCloseEvent* close_event) override {
		main_document_view->persist();
		ofstream last_path_file(last_path_file_absolute_location);
		last_path_file << utf8_encode(main_document_view->get_document()->get_path()) << endl;
		last_path_file.close();
		delete helper_opengl_widget;
	}

	bool dsqp;
public:

	MainWidget(
		fz_context* mupdf_context,
		sqlite3* db,
		DocumentManager* document_manager,
		ConfigManager* config_manager,
		InputHandler* input_handler,
		bool* should_quit_ptr,
		QWidget* parent=nullptr
	) : QWidget(parent),
		mupdf_context(mupdf_context),
		db(db),
		document_manager(document_manager),
		config_manager(config_manager),
		input_handler(input_handler)
	{ 
		int num_screens = QApplication::desktop()->numScreens();
		int first_screen_width = QApplication::desktop()->screenGeometry(0).width();

		pdf_renderer = new PdfRenderer(4, should_quit_ptr, mupdf_context);
		//pdf_renderer->set_invalidate_pointer(&dsqp);
		pdf_renderer->start_threads();

		garbage_collect_timer.setInterval(1000);
		garbage_collect_timer.start();

		QObject::connect(&garbage_collect_timer, &QTimer::timeout, [&]() {
			if (should_delete_old_pages) {
				pdf_renderer->delete_old_pages();
				should_delete_old_pages = false;
			}
			});

		//pdf_renderer->set_on_render_invalidate_function([&]() {
		//	invalidate_render();
		//	});

		//pdf_renderer->set_on_search_invalidate_function([&]() {
		//	invalidate_render();
		//	});
		QObject::connect(pdf_renderer, &PdfRenderer::render_advance, this, &MainWidget::invalidate_render);
		QObject::connect(pdf_renderer, &PdfRenderer::search_advance, this, &MainWidget::invalidate_ui);


		main_document_view = new DocumentView(mupdf_context, db, document_manager, config_manager);
		opengl_widget = new PdfViewOpenGLWidget(main_document_view, pdf_renderer, config_manager, this);

		helper_document_view = new DocumentView(mupdf_context, db, document_manager, config_manager);
		helper_opengl_widget = new PdfViewOpenGLWidget(helper_document_view, pdf_renderer, config_manager);

		if (num_screens > 1) {
			helper_opengl_widget->move(first_screen_width, 0);
			helper_opengl_widget->showMaximized();
		}


		status_label = new QTextEdit(this);
		status_label->setReadOnly(true);
		status_label->setVerticalScrollBarPolicy(Qt::ScrollBarPolicy::ScrollBarAlwaysOff);

		status_label->setStyleSheet("background-color: black; color: white; border: 0");
		status_label->setFontFamily("Monaco");

		//status_label->show();

		text_command_line_edit = new QLineEdit(this);
		text_command_line_edit->hide();

		QObject::connect(text_command_line_edit, &QLineEdit::returnPressed, [&]() {
			handle_pending_text_command(text_command_line_edit->text().toStdWString());
			text_command_line_edit->hide();
			setFocus();
			});

		QTimer* timer = new QTimer(this);
		timer->setInterval(200);
		connect(timer, &QTimer::timeout, [&]() {
			if (is_render_invalidated) {
				validate_render();
			}
			else if (is_ui_invalidated) {
				validate_ui();
			}
			});
		timer->start();

		QVBoxLayout* layout = new QVBoxLayout;
		layout->setSpacing(0);
		layout->setContentsMargins(0, 0, 0, 0);
		layout->addWidget(opengl_widget);
		setLayout(layout);

		setFocus();
	}

	~MainWidget() {
		cout << "Waiting for background threads ..." << endl;
		pdf_renderer->join_threads();
	}

	string get_status_string() {
		stringstream ss;
		if (main_document_view->get_document() == nullptr) return "";
		ss << "Page " << main_document_view->get_current_page_number()+1 << " / " << main_document_view->get_document()->num_pages();
		int num_search_results = opengl_widget->get_num_search_results();
		float progress = -1;
		if (num_search_results > 0 || opengl_widget->get_is_searching(&progress)) {
			opengl_widget->get_is_searching(&progress);

			// show the 0th result if there are no results and the index + 1 otherwise
			int result_index = opengl_widget->get_num_search_results() > 0 ? opengl_widget->get_current_search_result_index() + 1 : 0;
			ss << " | showing result " << result_index << " / " << num_search_results;
			if (progress > 0) {
				ss << " (" << ((int)(progress * 100)) << "%%" << ")";
			}
		}
		if (pending_link_source_filled) {
			ss << " | linking ...";
		}
		if (link_to_edit) {
			ss << " | editing link ...";
		}
		if (current_pending_command && current_pending_command->requires_symbol) {
			ss << " | " << current_pending_command->name <<" waiting for symbol";
		}
		return ss.str();
	}

	void handle_escape() {
		text_command_line_edit->setText("");
		pending_link_source_document_path = L"";
		pending_link_source_filled = false;
		pending_text_command = nullptr;

		if (current_widget) {
			current_widget = nullptr;
		}
		if (main_document_view) {
			main_document_view->handle_escape();
			opengl_widget->handle_escape();
		}

		text_command_line_edit->hide();

		validate_render();
	}

	void keyPressEvent(QKeyEvent* kevent) override {
		key_event(false, kevent);
	}

	void keyReleaseEvent(QKeyEvent* kevent) override {
		key_event(true, kevent);
	}

	void validate_render() {
		if (main_document_view && main_document_view->get_document()) {
			Link* link = main_document_view->find_closest_link();
			if (link) {
				if (helper_document_view->get_document() &&
					helper_document_view->get_document()->get_path() == link->document_path) {

					helper_document_view->set_offsets(link->dest_offset_x, link->dest_offset_y);
				}
				else {
					//delete helper_document_view;
					//helper_document_view = new DocumentView(mupdf_context, database, pdf_renderer, document_manager, config_manager, link->document_path,
					//	helper_window_width, helper_window_height, link->dest_offset_x, link->dest_offset_y);
					helper_document_view->open_document(link->document_path);
					helper_document_view->set_offsets(link->dest_offset_x, link->dest_offset_y);
				}
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

	void validate_ui() {
		status_label->setText(QString::fromStdString(get_status_string()));
		is_ui_invalidated = false;
	}

	void invalidate_render() {
		invalidate_ui();
		is_render_invalidated = true;
	}

	void invalidate_ui() {
		is_render_invalidated = true;
	}

	void handle_command_with_symbol(const Command* command, char symbol) {
		assert(symbol);
		assert(command->requires_symbol);
		if (command->name == "set_mark") {
			assert(main_document_view);
			main_document_view->add_mark(symbol);
			validate_render();
		}
		else if (command->name == "goto_mark") {
			assert(main_document_view);
			main_document_view->goto_mark(symbol);
		}
		else if (command->name == "delete") {
			if (symbol == 'y') {
				main_document_view->delete_closest_link();
				validate_render();
			}
			else if (symbol == 'b') {
				main_document_view->delete_closest_bookmark();
				validate_render();
			}
		}
	}

	void open_document(wstring path, optional<float> offset_x = {}, optional<float> offset_y = {}) {

		//save the previous document state
		if (main_document_view) {
			main_document_view->persist();
		}

		main_document_view->open_document(path);
		//opengl_widget->set_document_view(main_document_view);

		if (path.size() > 0 && main_document_view->get_document() == nullptr) {
			show_error_message(L"Could not open file: " + path);
		}
		main_document_view->on_view_size_change(main_window_width, main_window_height);

		if (offset_x) {
			main_document_view->set_offset_x(offset_x.value());
		}
		if (offset_y) {
			main_document_view->set_offset_y(offset_y.value());
		}

		//if (path.size() > 0) {
		//	ofstream last_path_file(last_path_file_absolute_location);
		//	last_path_file << utf8_encode(path) << endl;
		//	last_path_file.close();
		//}
	}
	void handle_command_with_file_name(const Command* command, wstring file_name) {
		assert(command->requires_file_name);
		if (command->name == "open_document") {
			//current_document_view->open_document(file_name);
			open_document(file_name);
		}
	}

	void key_event(bool released, QKeyEvent* kevent) {
		validate_render();

		if (kevent->key() == Qt::Key::Key_Escape) {
			current_pending_command = nullptr;
			is_waiting_for_symbol = false;
			//todo: merge here and there
			handle_escape();
		}

		if (released == false) {
			vector<int> ignored_codes = {
				Qt::Key::Key_Shift,
				Qt::Key::Key_Control
			};
			if (std::find(ignored_codes.begin(), ignored_codes.end(), kevent->key()) != ignored_codes.end()) {
				return;
			}
			if (is_waiting_for_symbol) {

				char symb = get_symbol(kevent->key(), kevent->modifiers() & Qt::ShiftModifier);
				if (symb) {
					handle_command_with_symbol(current_pending_command, symb);
					current_pending_command = nullptr;
					is_waiting_for_symbol = false;
				}
				return;
			}
			int num_repeats = 0;
			const Command* command = input_handler->handle_key(
				kevent->key(),
				kevent->modifiers() & Qt::ShiftModifier,
				kevent->modifiers() & Qt::ControlModifier,
				&num_repeats);

			if (command) {
				if (command->requires_symbol) {
					is_waiting_for_symbol = true;
					current_pending_command = command;
					return;
				}
				if (command->requires_file_name) {
					wchar_t file_name[MAX_PATH];
					if (select_pdf_file_name(file_name, MAX_PATH)) {
						handle_command_with_file_name(command, file_name);
					}
					else {
						cerr << "File select failed" << endl;
					}
					return;
				}
				else {
					handle_command(command, num_repeats);
				}
			}
		}

	}

	void handle_left_click(float x, float y, bool down) {
		//if (ImGui::GetIO().WantCaptureMouse) {
		//	return;
		//}
		float x_, y_;
		main_document_view->window_to_absolute_document_pos(x, y, &x_, &y_);

		if (down == true) {
			last_mouse_down_x = x_;
			last_mouse_down_y = y_;
			opengl_widget->selected_character_rects.clear();
			is_selecting = true;
		}
		else {
			is_selecting = false;
			if ((abs(last_mouse_down_x - x_) + abs(last_mouse_down_y - y_)) > 20) {
				//fz_rect sr;
				//sr.x0 = min(last_mouse_down_x, x_);
				//sr.x1 = max(last_mouse_down_x, x_);

				//sr.y0 = min(last_mouse_down_y, y_);
				//sr.y1 = max(last_mouse_down_y, y_);
				//selected_rect = sr;
				fz_point selection_begin = { last_mouse_down_x, last_mouse_down_y };
				fz_point selection_end = { x_, y_ };

				main_document_view->get_text_selection(selection_begin,
					selection_end,
					opengl_widget->selected_character_rects,
					selected_text);
				validate_render();
			}
			else {
				selected_rect = {};
				handle_click(x, y);
				opengl_widget->selected_character_rects.clear();
				validate_render();
			}
		}
	}

	void push_state() {
		DocumentViewState dvs = main_document_view->get_state();

		// do not add the same place to history multiple times
		if (history.size() > 0) {
			DocumentViewState last_history = history[history.size() - 1];
			if (last_history.document_path == main_document_view->get_document()->get_path() && last_history.offset_x == dvs.offset_x && last_history.offset_y == dvs.offset_y) {
				return;
			}
		}

		if (current_history_index == history.size()) {
			history.push_back(dvs);
			current_history_index = history.size();
		}
		else {
			//todo: should probably do some reference counting garbage collection here

			history.erase(history.begin() + current_history_index, history.end());
			history.push_back(dvs);
			current_history_index = history.size();
		}
	}

	void next_state() {
		if (current_history_index+1 < history.size()) {
			current_history_index++;
			set_main_document_view_state(history[current_history_index]);
		}
	}

	void prev_state() {

		/*
		Goto previous history
		In order to edit a link, we set the link to edit and jump to the link location, when going back, we
		update the link with the current location of document, therefore, we must check to see if a link
		is being edited and if so, we should update its destination position
		*/

		if (current_history_index > 0) {
			if (current_history_index == history.size()) {
				push_state();
				current_history_index = history.size()-1;
			}
			current_history_index--;

			if (link_to_edit) {
				float link_new_offset_x = main_document_view->get_offset_x();
				float link_new_offset_y = main_document_view->get_offset_y();
				link_to_edit->dest_offset_x = link_new_offset_x;
				link_to_edit->dest_offset_y = link_new_offset_y;
				//update_link(db, history[current_history_index].document_view->get_document()->get_path(),
				//	link_new_offset_x, link_new_offset_y, link_to_edit->src_offset_y);

				update_link(db, history[current_history_index].document_path,
					link_new_offset_x, link_new_offset_y, link_to_edit->src_offset_y);
				link_to_edit = nullptr;
			}

			set_main_document_view_state(history[current_history_index]);
		}
	}

	void set_main_document_view_state(DocumentViewState new_view_state) {
		//main_document_view = new_view_state.document_view;
		//opengl_widget->set_document_view(main_document_view);
		if (main_document_view->get_document()->get_path() != new_view_state.document_path) {
			main_document_view->open_document(new_view_state.document_path);
		}

		main_document_view->on_view_size_change(main_window_width, main_window_height);
		main_document_view->set_offsets(new_view_state.offset_x, new_view_state.offset_y);
		main_document_view->set_zoom_level(new_view_state.zoom_level);
	}

	void handle_click(int pos_x, int pos_y) {
		auto link_ = main_document_view->get_link_in_pos(pos_x, pos_y);
		if (link_.has_value()) {
			PdfLink link = link_.value();
			int page;
			float offset_x, offset_y;
			parse_uri(link.uri, &page, &offset_x, &offset_y);

			if (!pending_link_source_filled) {
				push_state();
				main_document_view->goto_offset_within_page(page, offset_x, offset_y);
			}
			else {
				// if we press the link button and then click on a pdf link, we automatically link to the
				// link's destination

				pending_link.dest_offset_x = offset_x;
				pending_link.dest_offset_y = main_document_view->get_page_offset(page-1) + offset_y;
				pending_link.document_path = main_document_view->get_document()->get_path();
				main_document_view->get_document()->add_link(pending_link);
				pending_link_source_filled = false;
			}
		}
	}

	void mouseReleaseEvent(QMouseEvent* mevent) override {

		if (mevent->button() == Qt::MouseButton::LeftButton) {
			//window_state.handle_click(event.button.x, event.button.y);
			handle_left_click(mevent->pos().x(), mevent->pos().y(), false);
		}

	}

	void mousePressEvent(QMouseEvent* mevent) override{
		if (mevent->button() == Qt::MouseButton::LeftButton) {
			//window_state.handle_click(event.button.x, event.button.y);
			handle_left_click(mevent->pos().x(), mevent->pos().y(), true);
		}

		if (mevent->button() == Qt::MouseButton::XButton1) {
			handle_command(command_manager.get_command_with_name("next_state"), 0);
		}

		if (mevent->button() == Qt::MouseButton::XButton2) {
			handle_command(command_manager.get_command_with_name("prev_state"), 0);
		}
	}

	void wheelEvent(QWheelEvent* wevent) override {
		int num_repeats = 1;
		const Command* command = nullptr;
		if (wevent->delta() > 0) {
			command = input_handler->handle_key(Qt::Key::Key_Up, false, false, &num_repeats);
		}
		if (wevent->delta() < 0) {
			command = input_handler->handle_key(Qt::Key::Key_Down, false, false, &num_repeats);
		}

		if (command) {
			handle_command(command, abs(wevent->delta() / 120));
		}
	}

	void show_textbar() {
		text_command_line_edit->clear();
		text_command_line_edit->show();
		text_command_line_edit->setFixedSize(main_window_width, 30);
		text_command_line_edit->setFocus();
	}

	void toggle_two_window_mode() {
		if (helper_opengl_widget->isHidden()) {
			helper_opengl_widget->show();
			activateWindow();
		}
		else {
			helper_opengl_widget->hide();
		}
	}
	void handle_command(const Command* command, int num_repeats) {

		if (command->requires_text) {
			pending_text_command = command;
			show_textbar();
			return;
		}
		if (command->pushes_state) {
			push_state();
		}
		if (command->name == "goto_begining") {
			if (num_repeats) {
				main_document_view->goto_page(num_repeats);
			}
			else {
				main_document_view->set_offset_y(0.0f);
			}
		}

		if (command->name == "goto_end") {
			main_document_view->goto_end();
		}
		if (command->name == "copy") {
			copy_to_clipboard(selected_text);
		}

		int rp = max(num_repeats, 1);

		if (command->name == "move_down") {
			main_document_view->move(0.0f, 72.0f * rp * vertical_move_amount);
		}
		else if (command->name == "move_up") {
			main_document_view->move(0.0f, -72.0f * rp * vertical_move_amount);
		}

		else if (command->name == "move_right") {
			main_document_view->move(72.0f * rp * horizontal_move_amount, 0.0f);
		}

		else if (command->name == "move_left") {
			main_document_view->move(-72.0f * rp * horizontal_move_amount, 0.0f);
		}

		else if (command->name == "link") {
			handle_link();
		}

		else if (command->name == "goto_link") {
			Link* link = main_document_view->find_closest_link();
			if (link) {

				//todo: add a feature where we can tap tab button to switch between main view and helper view
				push_state();
				open_document(link->document_path, link->dest_offset_x, link->dest_offset_y);
			}
		}
		else if (command->name == "edit_link") {
			Link* link = main_document_view->find_closest_link();
			if (link) {
				push_state();
				link_to_edit = link;
				open_document(link->document_path, link->dest_offset_x, link->dest_offset_y);
			}
		}

		else if (command->name == "zoom_in") {
			main_document_view->zoom_in();
		}

		else if (command->name == "zoom_out") {
			main_document_view->zoom_out();
		}

		else if (command->name == "next_state") {
			next_state();
		}
		else if (command->name == "prev_state") {
			prev_state();
		}

		else if (command->name == "next_item") {
			opengl_widget->goto_search_result(1 + num_repeats);
		}

		else if (command->name == "previous_item") {
			opengl_widget->goto_search_result(-1 - num_repeats);
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
		else if (command->name == "goto_toc") {
			vector<wstring> flat_toc;
			vector<int> current_document_toc_pages;
			get_flat_toc(main_document_view->get_document()->get_toc(), flat_toc, current_document_toc_pages);
			if (current_document_toc_pages.size() > 0) {
				current_widget = make_unique<FilteredSelectWindowClass<int>>(flat_toc, current_document_toc_pages, [&](void* page_pointer) {
					int* page_value = (int*)page_pointer;
					if (page_value) {
						push_state();
						validate_render();
						main_document_view->goto_page(*page_value);
					}
					}, this);
				current_widget->show();
			}
		}
		else if (command->name == "open_prev_doc") {
			vector<wstring> opened_docs_paths;
			vector<wstring> opened_docs_names;
			select_prev_docs(db, opened_docs_paths);

			for (const auto& p : opened_docs_paths) {
				opened_docs_names.push_back(std::filesystem::path(p).filename().wstring());
			}

			if (opened_docs_paths.size() > 0) {
				current_widget = make_unique<FilteredSelectWindowClass<wstring>>(opened_docs_names, opened_docs_paths, [&](void* string_pointer) {
					wstring doc_path = *(wstring*)string_pointer;
					if (doc_path.size() > 0) {
						push_state();
						validate_render();
						open_document(doc_path);
					}
					}, this);
				current_widget->show();
			}
		}
		else if (command->name == "goto_bookmark") {
			vector<wstring> option_names;
			vector<float> option_locations;
			for (int i = 0; i < main_document_view->get_document()->get_bookmarks().size(); i++) {
				option_names.push_back(main_document_view->get_document()->get_bookmarks()[i].description);
				option_locations.push_back(main_document_view->get_document()->get_bookmarks()[i].y_offset);
			}
			current_widget = make_unique<FilteredSelectWindowClass<float>>(option_names, option_locations, [&](void* float_pointer) {

				float* offset_value = (float*)float_pointer;
				if (offset_value) {
					push_state();
					validate_render();
					main_document_view->set_offset_y(*offset_value);
				}
				}, this);
			current_widget->show();
		}
		else if (command->name == "goto_bookmark_g") {
			vector<pair<wstring, BookMark>> global_bookmarks;
			global_select_bookmark(db, global_bookmarks);
			vector<wstring> descs;
			vector<BookState> book_states;

			for (const auto& desc_bm_pair : global_bookmarks) {
				wstring path = desc_bm_pair.first;
				BookMark bm = desc_bm_pair.second;
				descs.push_back(bm.description);
				book_states.push_back({ path, bm.y_offset });
			}
			current_widget = make_unique<FilteredSelectWindowClass<BookState>>(descs, book_states, [&](void* book_p) {
				BookState* offset_value = (BookState*)book_p;
				if (offset_value) {
					push_state();
					validate_render();
					open_document(offset_value->document_path, 0.0f, offset_value->offset_y);
				}
				}, this);
			current_widget->show();

		}

		else if (command->name == "toggle_fullscreen") {
			toggle_fullscreen();
		}
		else if (command->name == "toggle_one_window") {
			toggle_two_window_mode();
		}

		else if (command->name == "toggle_highlight") {
			opengl_widget->toggle_highlight_links();
		}

		//todo: check if still works after wstring
		else if (command->name == "search_selected_text_in_google_scholar") {

			ShellExecuteW(0, 0, (*config_manager->get_config<wstring>(L"google_scholar_address") + (selected_text)).c_str() , 0, 0, SW_SHOW);
		}
		else if (command->name == "search_selected_text_in_libgen") {
			ShellExecuteW(0, 0, (*config_manager->get_config<wstring>(L"libgen_address") + (selected_text)).c_str() , 0, 0, SW_SHOW);
		}
		else if (command->name == "debug") {
			cout << "debug" << endl;
		}
		validate_render();
	}

	void handle_link() {
		if (pending_link_source_filled) {
			pending_link.dest_offset_x = main_document_view->get_offset_x();
			pending_link.dest_offset_y = main_document_view->get_offset_y();
			pending_link.document_path = main_document_view->get_document()->get_path();
			if (pending_link_source_document_path == main_document_view->get_document()->get_path()) {
				main_document_view->get_document()->add_link(pending_link);
			}
			else {
				const unordered_map<wstring, Document*> cached_documents = document_manager->get_cached_documents();
				for (auto [doc_path, doc] : cached_documents) {
					if (pending_link_source_document_path == doc_path) {
						doc->add_link(pending_link, false);
					}
				}

				insert_link(db,
					pending_link_source_document_path,
					pending_link.document_path,
					pending_link.dest_offset_x,
					pending_link.dest_offset_y,
					pending_link.src_offset_y);
			}
			pending_link_source_filled = false;
		}
		else {
			pending_link.src_offset_y = main_document_view->get_offset_y();
			pending_link_source_document_path = main_document_view->get_document()->get_path();
			pending_link_source_filled = true;
		}
		validate_render();
	}

	void handle_pending_text_command(wstring text) {
		if (pending_text_command->name == "search") {

			// in mupdf RTL documents are reversed, so we reverse the search string
			//todo: better (or any!) handling of mixed RTL and LTR text
			if (is_rtl(text[0])) {
				text = reverse_wstring(text);
			}
			opengl_widget->search_text(text.c_str());
		}

		if (pending_text_command->name == "add_bookmark") {
			main_document_view->add_bookmark(text);
		}
		if (pending_text_command->name == "command") {
			wcout << text << endl;
		}
	}

	void toggle_fullscreen() {
		if (isFullScreen()) {
			helper_opengl_widget->setWindowState(Qt::WindowState::WindowMaximized);
			setWindowState(Qt::WindowState::WindowMaximized);
		}
		else {
			helper_opengl_widget->setWindowState(Qt::WindowState::WindowFullScreen);
			setWindowState(Qt::WindowState::WindowFullScreen);
		}
	}
};



int main(int argc, char* args[]) {

	char exe_file_name[MAX_PATH];
	GetModuleFileNameA(NULL, exe_file_name, sizeof(exe_file_name));
	filesystem::path exe_path = exe_file_name;

	// parent path is the directory in which config files and required shaders are located
	// in debug mode this is the same directory as the source file but in release mode it is
	// the directory in which the executable is located
	parent_path = exe_path.parent_path();

#ifdef NDEBUG
	install_app(exe_file_name);
#else
	filesystem::path source_file_path = __FILE__;
	parent_path = source_file_path.parent_path();
#endif

	last_path_file_absolute_location = (parent_path / "last_document_path.txt").wstring();

	filesystem::path config_path = parent_path / "prefs.config";
	ConfigManager config_manager(config_path.wstring());

	sqlite3* db;
	char* error_message = nullptr;
	int rc;

	rc = sqlite3_open((parent_path / "test.db").string().c_str(), &db);
	if (rc) {
		cerr << "could not open database" << sqlite3_errmsg(db) << endl;
	}

	create_tables(db);

	fz_locks_context locks;
	locks.user = mupdf_mutexes;
	locks.lock = lock_mutex;
	locks.unlock = unlock_mutex;

	fz_context* mupdf_context = fz_new_context(nullptr, &locks, FZ_STORE_UNLIMITED);

	if (!mupdf_context) {
		cerr << "could not create mupdf context" << endl;
		return -1;
	}
	bool fail = false;
	fz_try(mupdf_context) {
		fz_register_document_handlers(mupdf_context);
	}
	fz_catch(mupdf_context) {
		cerr << "could not register document handlers" << endl;
		fail = true;
	}

	if (fail) {
		return -1;
	}

	bool quit = false;

	bool one_window_mode = false;
	int num_displays = SDL_GetNumVideoDisplays();
	if (num_displays == 1) {
		one_window_mode = true;
	}


	InputHandler input_handler((parent_path / "keys.config").wstring());

	//char file_path[MAX_PATH] = { 0 };
	wstring file_path;
	string file_path_;
	ifstream last_state_file(last_path_file_absolute_location);
	getline(last_state_file, file_path_);
	file_path = utf8_decode(file_path_);
	last_state_file.close();

	bool is_waiting_for_symbol = false;
	const Command* current_pending_command = nullptr;

	DocumentManager document_manager(mupdf_context, db);

	QFileSystemWatcher pref_file_watcher;
	pref_file_watcher.addPath(QString::fromStdWString(config_path));

	// live reload the config file
	QObject::connect(&pref_file_watcher, &QFileSystemWatcher::fileChanged, [&]() {
		wifstream config_file(config_path);
		config_manager.deserialize(config_file);
		config_file.close();
		});


	QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts, true);
	QApplication app(argc, args);

	MainWidget main_widget(mupdf_context, db, &document_manager, &config_manager, &input_handler, &quit);
	main_widget.open_document(file_path);
	main_widget.resize(500, 500);
	main_widget.showMaximized();


	app.exec();

	quit = true;
	return 0;
}
