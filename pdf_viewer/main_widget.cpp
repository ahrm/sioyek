
#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <optional>
#include <memory>


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

#include "main_widget.h"

extern bool launched_from_file_icon;
extern bool flat_table_of_contents;
extern float move_screen_percentage;
extern std::filesystem::path parent_path;

void MainWidget::resizeEvent(QResizeEvent* resize_event) {
	QWidget::resizeEvent(resize_event);

	main_window_width = size().width();
	main_window_height = size().height();

	//text_command_line_edit->move(0, 0);
	//text_command_line_edit->resize(main_window_width, 30);
	text_command_line_edit_container->move(0, 0);
	text_command_line_edit_container->resize(main_window_width, 30);

	status_label->move(0, main_window_height - 20);
	status_label->resize(main_window_width, 20);

}

void MainWidget::mouseMoveEvent(QMouseEvent* mouse_event) {

	int x = mouse_event->pos().x();
	int y = mouse_event->pos().y();

	if (main_document_view && main_document_view->get_link_in_pos(x, y)) {
		setCursor(Qt::PointingHandCursor);
	}
	else {
		setCursor(Qt::ArrowCursor);
	}

	//if (opengl_widget->get_should_draw_vertical_line()) {
	//	opengl_widget->set_vertical_line_pos(y);
	//	validate_render();
	//}
	if (is_selecting) {

		// When selecting, we occasionally update selected text
		//todo: maybe have a timer event that handles this periodically
		if (last_text_select_time.msecsTo(QTime::currentTime()) > 100) {
			int x = mouse_event->pos().x();
			int y = mouse_event->pos().y();

			float x_, y_;
			main_document_view->window_to_absolute_document_pos(x, y, &x_, &y_);

			fz_point selection_begin = { last_mouse_down_x, last_mouse_down_y };
			fz_point selection_end = { x_, y_ };

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

void MainWidget::closeEvent(QCloseEvent* close_event) {
	main_document_view->persist();

	if (main_document_view->get_document()) {
		std::ofstream last_path_file(last_path_file_absolute_location);

		std::string encoded_file_name_str = utf8_encode(main_document_view->get_document()->get_path());
		last_path_file << encoded_file_name_str.c_str() << std::endl;
		last_path_file.close();
	}

	// we need to delete this here (instead of destructor) to ensure that application
	// closes immediately after the main window is closed
	delete helper_opengl_widget;
}

MainWidget::MainWidget(fz_context* mupdf_context, sqlite3* db, DocumentManager* document_manager, ConfigManager* config_manager, InputHandler* input_handler, bool* should_quit_ptr, QWidget* parent) : QWidget(parent),
mupdf_context(mupdf_context),
db(db),
document_manager(document_manager),
config_manager(config_manager),
input_handler(input_handler)
{
	setMouseTracking(true);

	int num_screens = QApplication::desktop()->numScreens();

	//todo: add an option so this is configurable
	num_screens = 1; //DEBUG!!!!!!!! remove this!
	int first_screen_width = QApplication::desktop()->screenGeometry(0).width();

	pdf_renderer = new PdfRenderer(4, should_quit_ptr, mupdf_context);
	pdf_renderer->start_threads();


	main_document_view = new DocumentView(mupdf_context, db, document_manager, config_manager);
	opengl_widget = new PdfViewOpenGLWidget(main_document_view, pdf_renderer, config_manager, this);

	helper_document_view = new DocumentView(mupdf_context, db, document_manager, config_manager);
	helper_opengl_widget = new PdfViewOpenGLWidget(helper_document_view, pdf_renderer, config_manager);

	if (num_screens > 1) {
		helper_opengl_widget->move(first_screen_width, 0);
		if (!launched_from_file_icon) {
			helper_opengl_widget->showMaximized();
		}
		else{
			helper_opengl_widget->setWindowState(Qt::WindowState::WindowMaximized);
		}
	}


	status_label = new QLabel(this);
	//status_label->setStyleSheet("background-color: black; color: white; border: 0");
	status_label->setStyleSheet(QString::fromStdWString(*config_manager->get_config<std::wstring>(L"status_label_stylesheet")));
	status_label->setFont(QFont("Monaco"));


	text_command_line_edit_container = new QWidget(this);
	//text_command_line_edit_container->setStyleSheet("background-color: black; color: white; border: 0");
	text_command_line_edit_container->setStyleSheet(QString::fromStdWString(*config_manager->get_config<std::wstring>(L"text_command_line_stylesheet")));

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

	QObject::connect(text_command_line_edit, &QLineEdit::returnPressed, [&]() {
		handle_pending_text_command(text_command_line_edit->text().toStdWString());
		text_command_line_edit_container->hide();
		setFocus();
		});

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
		if (continuous_render_mode || is_render_invalidated) {
			validate_render();
		}
		else if (is_ui_invalidated) {
			validate_ui();
		}
		if (main_document_view != nullptr) {
			Document* doc = nullptr;
			if ((doc = main_document_view->get_document()) != nullptr) {

				if (doc->get_milies_since_last_edit_time() < 2 * INTERVAL_TIME) {
					doc->reload();
					pdf_renderer->clear_cache();
					validate_render();
					validate_ui();
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
	pdf_renderer->join_threads();

	if (main_document_view != nullptr) {
		Document* main_doc = main_document_view->get_document();
		if (main_doc != nullptr) delete main_doc;

		Document* helper_doc = nullptr;
		if (helper_document_view != nullptr) {
			helper_doc = helper_document_view->get_document();
		}
		if (helper_doc != nullptr && helper_doc != main_doc) delete helper_doc;
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

	ss << "Page " << main_document_view->get_current_page_number() + 1 << " / " << main_document_view->get_document()->num_pages();
	if (chapter_name.size() > 0) {
		ss << " [ " << chapter_name << " ] ";
	}
	int num_search_results = opengl_widget->get_num_search_results();
	float progress = -1;
	if (opengl_widget->get_is_searching(&progress)) {
		opengl_widget->get_is_searching(&progress);

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
	return ss.str();
}

void MainWidget::handle_escape() {
	text_command_line_edit->setText("");
	pending_link = {};
	current_pending_command = nullptr;

	if (current_widget) {
		current_widget = nullptr;
	}
	if (main_document_view) {
		main_document_view->handle_escape();
		opengl_widget->handle_escape();
	}

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

	if (main_document_view && main_document_view->get_document()) {
		Link* link = main_document_view->find_closest_link();
		if (link) {
			if (helper_document_view->get_document() &&
				helper_document_view->get_document()->get_path() == link->document_path) {

				helper_document_view->set_offsets(link->dest_offset_x, link->dest_offset_y);
				helper_document_view->set_zoom_level(link->dest_zoom_level);
			}
			else {
				//delete helper_document_view;
				//helper_document_view = new DocumentView(mupdf_context, database, pdf_renderer, document_manager, config_manager, link->document_path,
				//	helper_window_width, helper_window_height, link->dest_offset_x, link->dest_offset_y);
				helper_document_view->open_document(link->document_path, &this->is_ui_invalidated);
				helper_document_view->set_offsets(link->dest_offset_x, link->dest_offset_y);
				helper_document_view->set_zoom_level(link->dest_zoom_level);
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

void MainWidget::validate_ui() {
	status_label->setText(QString::fromStdWString(get_status_string()));
	is_ui_invalidated = false;
}

void MainWidget::move_document(float dx, float dy)
{
	if (main_document_view) {
		main_document_view->move(dx, dy);
		int prev_vertical_line_pos = opengl_widget->get_vertical_line_pos();
		int new_vertical_line_pos = prev_vertical_line_pos - dy;
		opengl_widget->set_vertical_line_pos(new_vertical_line_pos);
	}
}

void MainWidget::move_document_screens(int num_screens)
{
	main_document_view->move_screens(1 * num_screens);
	int view_height = opengl_widget->height();
	float move_amount = num_screens * view_height * move_screen_percentage;
	if (opengl_widget) {
		int current_loc = opengl_widget->get_vertical_line_pos();
		int new_loc = current_loc - move_amount;
		opengl_widget->set_vertical_line_pos(new_loc);
	}
}

void MainWidget::on_config_file_changed(ConfigManager* new_config)
{
	status_label->setStyleSheet(QString::fromStdWString(*config_manager->get_config<std::wstring>(L"status_label_stylesheet")));
	text_command_line_edit_container->setStyleSheet(
		QString::fromStdWString(*config_manager->get_config<std::wstring>(L"text_command_line_stylesheet")));
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

void MainWidget::set_render_mode(bool continuous_render_mode_)
{
	continuous_render_mode = continuous_render_mode_;
}

void MainWidget::toggle_render_mode()
{
	set_render_mode(!continuous_render_mode);
}

void MainWidget::open_document(std::wstring path, std::optional<float> offset_x, std::optional<float> offset_y) {

	//save the previous document state
	if (main_document_view) {
		main_document_view->persist();
	}

	main_document_view->open_document(path, &this->is_ui_invalidated);
	if (main_document_view->get_document() != nullptr) {
		setWindowTitle(QString::fromStdWString(path));
	}

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

void MainWidget::key_event(bool released, QKeyEvent* kevent) {
	validate_render();

	if (kevent->key() == Qt::Key::Key_Escape) {
		handle_escape();
	}

	if (released == false) {
		std::vector<int> ignored_codes = {
			Qt::Key::Key_Shift,
			Qt::Key::Key_Control
		};
		if (std::find(ignored_codes.begin(), ignored_codes.end(), kevent->key()) != ignored_codes.end()) {
			return;
		}
		if (is_waiting_for_symbol()) {

			char symb = get_symbol(kevent->key(), kevent->modifiers() & Qt::ShiftModifier);
			if (symb) {
				handle_command_with_symbol(current_pending_command, symb);
				current_pending_command = nullptr;
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
				current_pending_command = command;
				return;
			}
			if (command->requires_file_name) {
				wchar_t file_name[MAX_PATH];
				if (select_document_file_name(file_name, MAX_PATH)) {
					handle_command_with_file_name(command, file_name);
				}
				else {
					std::cerr << "File select failed" << endl;
				}
				return;
			}
			else {
				handle_command(command, num_repeats);
			}
		}
	}

}

void MainWidget::handle_right_click(float x, float y, bool down) {
	//main_document_view->set
	if (opengl_widget) {
		opengl_widget->set_should_draw_vertical_line(true);
		opengl_widget->set_vertical_line_pos(y);
		validate_render();
		//set_render_mode(down);
	}

}

void MainWidget::handle_left_click(float x, float y, bool down) {

	if (opengl_widget) opengl_widget->set_should_draw_vertical_line(false);
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
			fz_point selection_begin = { last_mouse_down_x, last_mouse_down_y };
			fz_point selection_end = { x_, y_ };

			main_document_view->get_text_selection(selection_begin,
				selection_end,
				is_word_selecting,
				opengl_widget->selected_character_rects,
				selected_text);
		is_word_selecting = false;
		}
		else {
			handle_click(x, y);
			opengl_widget->selected_character_rects.clear();
			selected_text.clear();
		}
		validate_render();
	}
}

void MainWidget::push_state() {
	DocumentViewState dvs = main_document_view->get_state();

	// we do not need to add empty document to history
	if (main_document_view->get_document() == nullptr) {
		return;
	}

	// do not add the same place to history multiple times
	if (history.size() > 0) {
		DocumentViewState last_history = history[history.size() - 1];
		if (last_history.document_path == main_document_view->get_document()->get_path() && last_history.book_state.offset_x == dvs.book_state.offset_x && last_history.book_state.offset_y == dvs.book_state.offset_y) {
			return;
		}
	}

	if (current_history_index == history.size()) {
		history.push_back(dvs);
		current_history_index = history.size();
	}
	else {
		history.erase(history.begin() + current_history_index, history.end());
		history.push_back(dvs);
		current_history_index = history.size();
	}
}

void MainWidget::next_state() {
	if (current_history_index + 1 < history.size()) {
		current_history_index++;
		set_main_document_view_state(history[current_history_index]);
	}
}

void MainWidget::prev_state() {

	/*
	Goto previous history
	In order to edit a link, we set the link to edit and jump to the link location, when going back, we
	update the link with the current location of document, therefore, we must check to see if a link
	is being edited and if so, we should update its destination position
	*/

	if (current_history_index > 0) {
		if (current_history_index == history.size()) {
			push_state();
			current_history_index = history.size() - 1;
		}
		current_history_index--;

		if (link_to_edit) {
			float link_new_offset_x = main_document_view->get_offset_x();
			float link_new_offset_y = main_document_view->get_offset_y();
			float link_new_zoom_level = main_document_view->get_zoom_level();
			link_to_edit->dest_offset_x = link_new_offset_x;
			link_to_edit->dest_offset_y = link_new_offset_y;
			link_to_edit->dest_zoom_level = link_new_zoom_level;
			//update_link(db, history[current_history_index].document_view->get_document()->get_path(),
			//	link_new_offset_x, link_new_offset_y, link_to_edit->src_offset_y);

			update_link(db, history[current_history_index].document_path,
				link_new_offset_x, link_new_offset_y, link_new_zoom_level, link_to_edit->src_offset_y);
			link_to_edit = nullptr;
		}

		set_main_document_view_state(history[current_history_index]);
	}
}

void MainWidget::set_main_document_view_state(DocumentViewState new_view_state) {
	//main_document_view = new_view_state.document_view;
	//opengl_widget->set_document_view(main_document_view);
	if (main_document_view->get_document()->get_path() != new_view_state.document_path) {
		main_document_view->open_document(new_view_state.document_path, &this->is_ui_invalidated);
	}

	main_document_view->on_view_size_change(main_window_width, main_window_height);
	//main_document_view->set_offsets(new_view_state.offset_x, new_view_state.offset_y);
	//main_document_view->set_zoom_level(new_view_state.zoom_level);
	main_document_view->set_book_state(new_view_state.book_state);
}

void MainWidget::handle_click(int pos_x, int pos_y) {
	auto link_ = main_document_view->get_link_in_pos(pos_x, pos_y);
	if (link_.has_value()) {
		PdfLink link = link_.value();
		int page;
		float offset_x, offset_y;




		if (link.uri.substr(0, 4).compare("http") == 0) {
			ShellExecuteA(0, 0, link.uri.c_str(), 0, 0, SW_SHOW);
			return;
		}

		parse_uri(link.uri, &page, &offset_x, &offset_y);

		// we usually just want to center the y offset and not the x offset (otherwise for example
		// a link at the right side of the screen will be centered, causing most of screen state to be empty)
		offset_x = main_document_view->get_offset_x();

		if (!is_pending_link_source_filled()) {
			push_state();
			main_document_view->goto_offset_within_page(page, offset_x, offset_y);
		}
		else {
			// if we press the link button and then click on a pdf link, we automatically link to the
			// link's destination

			Link& pl = pending_link.value().second;
			pl.dest_offset_x = offset_x;
			pl.dest_offset_y = main_document_view->get_page_offset(page - 1) + offset_y;
			pl.dest_zoom_level = main_document_view->get_zoom_level();
			pl.document_path = main_document_view->get_document()->get_path();
			main_document_view->get_document()->add_link(pl);
			pending_link = {};
		}
	}
}

void MainWidget::mouseReleaseEvent(QMouseEvent* mevent) {

	if (mevent->button() == Qt::MouseButton::LeftButton) {
		handle_left_click(mevent->pos().x(), mevent->pos().y(), false);
	}

	if (mevent->button() == Qt::MouseButton::RightButton) {
		handle_right_click(mevent->pos().x(), mevent->pos().y(), false);
	}

//#ifdef _DEBUG
	if (mevent->button() == Qt::MouseButton::MiddleButton) {

		//int page;
		//fz_rect rect;
		//if (main_document_view->get_block_at_window_position(opengl_widget->pos().x(), mevent->pos().y(), &page, &rect)) {
		//	//optional<int> last_selected_block_page = {};
		//	//optional<fz_rect> last_selected_block = {};
		//	opengl_widget->last_selected_block_page = page;
		//	opengl_widget->last_selected_block = rect;
		//	invalidate_render();
		//}
		int page;
		float offset_x, offset_y;

		main_document_view->window_to_document_pos(mevent->pos().x(), mevent->pos().y(), &offset_x, &offset_y, &page);
		std::optional<std::wstring> text_on_pointer = main_document_view->get_document()->get_text_at_position(page, offset_x, offset_y);
		if (text_on_pointer) {
			std::wstring figure_string = get_figure_string_from_raw_string(text_on_pointer.value());
			if (figure_string.size() == 0) return;

			int fig_page;
			float fig_offset;
			if (main_document_view->get_document()->find_figure_with_string(figure_string, page, &fig_page, &fig_offset)) {
				push_state();
				main_document_view->goto_page(fig_page);
				invalidate_render();
			}
		}
		//if (text_on_pointer) {
		//	wcout << text_on_pointer.value() << endl;
		//}
	}
//#endif

}

void MainWidget::mouseDoubleClickEvent(QMouseEvent* mevent) {
	if (mevent->button() == Qt::MouseButton::LeftButton) {
		is_selecting = true;
		is_word_selecting = true;
	}
}

void MainWidget::mousePressEvent(QMouseEvent* mevent) {
	if (mevent->button() == Qt::MouseButton::LeftButton) {
		handle_left_click(mevent->pos().x(), mevent->pos().y(), true);
	}

	if (mevent->button() == Qt::MouseButton::RightButton) {
		handle_right_click(mevent->pos().x(), mevent->pos().y(), true);
	}

	if (mevent->button() == Qt::MouseButton::XButton1) {
		handle_command(command_manager.get_command_with_name("next_state"), 0);
	}

	if (mevent->button() == Qt::MouseButton::XButton2) {
		handle_command(command_manager.get_command_with_name("prev_state"), 0);
	}
}

void MainWidget::wheelEvent(QWheelEvent* wevent) {
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

void MainWidget::show_textbar(const std::wstring& command_name, bool should_fill_with_selected_text) {
	text_command_line_edit->clear();
	if (should_fill_with_selected_text) {
		text_command_line_edit->setText(QString::fromStdWString(selected_text));
	}
	text_command_line_edit_label->setText(QString::fromStdWString(command_name));
	//text_command_line_edit->show();
	text_command_line_edit_container->show();
	//text_command_line_edit_container->setFixedSize(main_window_width, 30);
	text_command_line_edit->setFocus();
}

void MainWidget::toggle_two_window_mode() {
	if (helper_opengl_widget->isHidden()) {
		helper_opengl_widget->show();
		activateWindow();
	}
	else {
		helper_opengl_widget->hide();
	}
}

void MainWidget::handle_command(const Command* command, int num_repeats) {

	if (command->requires_text) {
		current_pending_command = command;
		bool should_fill_text_bar_with_selected_text = false;
		if (command->name == "search" || command->name == "chapter_search" || command->name == "ranged_search") {
			should_fill_text_bar_with_selected_text = true;
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

	int rp = std::max(num_repeats, 1);

	if (command->name == "screen_down") {
		move_document_screens(1 * rp);
	}
	if (command->name == "screen_up") {
		move_document_screens(-1 * rp);
	}
	if (command->name == "move_down") {
		move_document(0.0f, 72.0f * rp * vertical_move_amount);
	}
	else if (command->name == "move_up") {
		move_document(0.0f, -72.0f * rp * vertical_move_amount);
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
			main_document_view->set_zoom_level(link->dest_zoom_level);
		}
	}
	else if (command->name == "edit_link") {
		Link* link = main_document_view->find_closest_link();
		if (link) {
			push_state();
			link_to_edit = link;
			open_document(link->document_path, link->dest_offset_x, link->dest_offset_y);
			main_document_view->set_zoom_level(link->dest_zoom_level);
		}
	}

	else if (command->name == "zoom_in") {
		main_document_view->zoom_in();
	}

	else if (command->name == "zoom_out") {
		main_document_view->zoom_out();
	}

	else if (command->name == "fit_to_page_width") {
		main_document_view->fit_to_page_width();
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
	else if (command->name == "next_chapter") {
		main_document_view->goto_chapter(rp);
	}
	else if (command->name == "prev_chapter") {
		main_document_view->goto_chapter(-rp);
	}

	else if (command->name == "goto_toc") {
		std::vector<std::wstring> flat_toc;
		std::vector<int> current_document_toc_pages;
		get_flat_toc(main_document_view->get_document()->get_toc(), flat_toc, current_document_toc_pages);
		if (current_document_toc_pages.size() > 0) {
			if (flat_table_of_contents) {
				current_widget = std::make_unique<FilteredSelectWindowClass<int>>(flat_toc, current_document_toc_pages, [&](void* page_pointer) {
					int* page_value = (int*)page_pointer;
					if (page_value) {
						push_state();
						validate_render();
						main_document_view->goto_page(*page_value);
					}
					}, config_manager, this);
				current_widget->show();
			}
			else {

				current_widget = std::make_unique<FilteredTreeSelect<int>>(main_document_view->get_document()->get_toc_model(),
					[&](const std::vector<int>& indices) {
						TocNode* toc_node = get_toc_node_from_indices(main_document_view->get_document()->get_toc(),
							indices);
						if (toc_node) {
							push_state();
							validate_render();
							main_document_view->goto_page(toc_node->page);
						}
					}, config_manager, this);
				current_widget->show();
			}

		}
	}
	else if (command->name == "open_prev_doc") {
		std::vector<std::wstring> opened_docs_paths;
		std::vector<std::wstring> opened_docs_names;
		select_prev_docs(db, opened_docs_paths);

		for (const auto& p : opened_docs_paths) {
			opened_docs_names.push_back(std::filesystem::path(p).filename().wstring());
		}

		if (opened_docs_paths.size() > 0) {
			current_widget = std::make_unique<FilteredSelectWindowClass<std::wstring>>(opened_docs_names, opened_docs_paths, [&](void* string_pointer) {
				std::wstring doc_path = *(std::wstring*)string_pointer;
				if (doc_path.size() > 0) {
					push_state();
					validate_render();
					open_document(doc_path);
				}
				}, config_manager, this);
			current_widget->show();
		}
	}
	else if (command->name == "goto_bookmark") {
		std::vector<std::wstring> option_names;
		std::vector<float> option_locations;
		for (int i = 0; i < main_document_view->get_document()->get_bookmarks().size(); i++) {
			option_names.push_back(main_document_view->get_document()->get_bookmarks()[i].description);
			option_locations.push_back(main_document_view->get_document()->get_bookmarks()[i].y_offset);
		}
		current_widget = std::make_unique<FilteredSelectWindowClass<float>>(option_names, option_locations, [&](void* float_pointer) {

			float* offset_value = (float*)float_pointer;
			if (offset_value) {
				push_state();
				validate_render();
				main_document_view->set_offset_y(*offset_value);
			}
			}, config_manager, this);
		current_widget->show();
	}
	else if (command->name == "goto_bookmark_g") {
		std::vector<std::pair<std::wstring, BookMark>> global_bookmarks;
		global_select_bookmark(db, global_bookmarks);
		std::vector<std::wstring> descs;
		std::vector<BookState> book_states;

		for (const auto& desc_bm_pair : global_bookmarks) {
			std::wstring path = desc_bm_pair.first;
			BookMark bm = desc_bm_pair.second;
			descs.push_back(bm.description);
			book_states.push_back({ path, bm.y_offset });
		}
		current_widget = std::make_unique<FilteredSelectWindowClass<BookState>>(descs, book_states, [&](void* book_p) {
			BookState* offset_value = (BookState*)book_p;
			if (offset_value) {
				push_state();
				validate_render();
				open_document(offset_value->document_path, 0.0f, offset_value->offset_y);
			}
			}, config_manager, this);
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
		ShellExecuteW(0, 0, (*config_manager->get_config<std::wstring>(L"google_scholar_address") + (selected_text)).c_str(), 0, 0, SW_SHOW);
	}
	else if (command->name == "open_selected_url") {
		ShellExecuteW(0, 0, (selected_text).c_str(), 0, 0, SW_SHOW);
	}
	else if (command->name == "search_selected_text_in_libgen") {
		ShellExecuteW(0, 0, (*config_manager->get_config<std::wstring>(L"libgen_address") + (selected_text)).c_str(), 0, 0, SW_SHOW);
	}
	else if (command->name == "debug") {
		//cout << "debug" << endl;
		//int page;
		//float y_offset;
		//if (main_document_view->get_document()->find_figure_with_string(L"2.2", &page, &y_offset)) {
		//	main_document_view->goto_page(page);
		//}
		main_document_view->get_document()->reload();
		pdf_renderer->clear_cache();

	}
	validate_render();
}

void MainWidget::handle_link() {
	if (is_pending_link_source_filled()) {
		auto [source_path, pl] = pending_link.value();
		pl.dest_offset_x = main_document_view->get_offset_x();
		pl.dest_offset_y = main_document_view->get_offset_y();
		pl.dest_zoom_level = main_document_view->get_zoom_level();
		pl.document_path = main_document_view->get_document()->get_path();
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

			insert_link(db,
				source_path.value(),
				pl.document_path,
				pl.dest_offset_x,
				pl.dest_offset_y,
				pl.dest_zoom_level,
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
		main_document_view->add_mark('0');

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
	//if (current_pending_command->name == "chapter_search" ) {
	//	//main_document_view->get_current_chapter_name
	//	opengl_widget->search_text(text);
	//}

	if (current_pending_command->name == "add_bookmark") {
		main_document_view->add_bookmark(text);
	}
	if (current_pending_command->name == "command") {
		if (text == L"q") {
			close();
			//QApplication::quit();
		}
		else if (text == L"ocr") {
			DocumentViewState current_state = main_document_view->get_state();
			OpenedBookState state = current_state.book_state;

			std::wstring docpathname = main_document_view->get_document()->get_path();
			std::filesystem::path docpath(docpathname);
			std::wstring file_name = docpath.filename().replace_extension("").wstring();
			std::wstring new_file_name = file_name + L"_ocr.pdf";
			std::wstring new_path = docpath.replace_filename(new_file_name).wstring();

			std::wstring command_params = (L"\"" + docpathname + L"\" \"" + new_path + L"\" --force-ocr");

			SHELLEXECUTEINFO ShExecInfo = { 0 };
			ShExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
			ShExecInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
			ShExecInfo.hwnd = NULL;
			ShExecInfo.lpVerb = NULL;
			ShExecInfo.lpFile = L"ocrmypdf.exe";
			ShExecInfo.lpParameters = command_params.c_str();
			ShExecInfo.lpDirectory = NULL;
			ShExecInfo.nShow = SW_SHOW;
			ShExecInfo.hInstApp = NULL;

			ShellExecuteExW(&ShExecInfo);
			WaitForSingleObject(ShExecInfo.hProcess, INFINITE);
			CloseHandle(ShExecInfo.hProcess);

			main_document_view->open_document(new_path, &this->is_ui_invalidated, false, state);
		}
		else if (text == L"keys") {
			ShellExecuteW(0, 0, (parent_path / "keys.config").wstring().c_str(), 0, 0, SW_SHOW);
		}
		else if (text == L"prefs") {
			ShellExecuteW(0, 0, (parent_path / "prefs.config").wstring().c_str(), 0, 0, SW_SHOW);
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
