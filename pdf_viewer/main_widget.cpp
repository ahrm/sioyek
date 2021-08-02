
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
#include <qstandardpaths.h>
#include <qprocess.h>


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

extern bool LAUNCHED_FROM_FILE_ICON;
extern bool SHOULD_USE_MULTIPLE_MONITORS;
extern bool FLAT_TABLE_OF_CONTENTS;
extern float MOVE_SCREEN_PERCENTAGE;
extern std::wstring LIBGEN_ADDRESS;
extern std::wstring INVERSE_SEARCH_COMMAND;

extern Path default_config_path;
extern Path default_keys_path;
extern Path user_config_path;
extern Path user_keys_path;
extern Path database_file_path;
extern Path tutorial_path;
extern Path last_opened_file_address_path;


bool MainWidget::main_document_view_has_document()
{
	return (main_document_view != nullptr) && (main_document_view->get_document() != nullptr);
}

void MainWidget::resizeEvent(QResizeEvent* resize_event) {
	QWidget::resizeEvent(resize_event);

	main_window_width = size().width();
	main_window_height = size().height();

	text_command_line_edit_container->move(0, 0);
	text_command_line_edit_container->resize(main_window_width, 30);

	status_label->move(0, main_window_height - 20);
	status_label->resize(main_window_width, 20);

	if ((main_document_view->get_document() != nullptr) && (main_document_view->get_zoom_level() == 0)) {
		main_document_view->fit_to_page_width();
	}

}

void MainWidget::mouseMoveEvent(QMouseEvent* mouse_event) {

	int x = mouse_event->pos().x();
	int y = mouse_event->pos().y();

	if (main_document_view && main_document_view->get_link_in_pos(x, y)) {
		// show hand cursor when hovering over links
		setCursor(Qt::PointingHandCursor);
	}
	else {
		setCursor(Qt::ArrowCursor);
	}

	if (is_selecting) {

		// When selecting, we occasionally update selected text
		//todo: maybe have a timer event that handles this periodically
		if (last_text_select_time.msecsTo(QTime::currentTime()) > 16) {

			float document_x, document_y;
			main_document_view->window_to_absolute_document_pos(x, y, &document_x, &document_y);

			fz_point selection_begin = { last_mouse_down_x, last_mouse_down_y };
			fz_point selection_end = { document_x, document_y };

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

	// write the address of the current document in a file so that the next time
	// we launch the application, we open this document
	if (main_document_view->get_document()) {
		std::ofstream last_path_file(last_opened_file_address_path.get_path_utf8());

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


	inverse_search_command = INVERSE_SEARCH_COMMAND;
	int first_screen_width = QApplication::desktop()->screenGeometry(0).width();

	pdf_renderer = new PdfRenderer(4, should_quit_ptr, mupdf_context);
	pdf_renderer->start_threads();


	main_document_view = new DocumentView(mupdf_context, db, document_manager, config_manager);
	opengl_widget = new PdfViewOpenGLWidget(main_document_view, pdf_renderer, config_manager, this);

	helper_document_view = new DocumentView(mupdf_context, db, document_manager, config_manager);
	helper_opengl_widget = new PdfViewOpenGLWidget(helper_document_view, pdf_renderer, config_manager);

	// automatically open the helper window in second monitor
	int num_screens = QApplication::desktop()->numScreens();
	if ((num_screens > 1) && SHOULD_USE_MULTIPLE_MONITORS) {
		helper_opengl_widget->move(first_screen_width, 0);
		if (!LAUNCHED_FROM_FILE_ICON) {
			helper_opengl_widget->showMaximized();
		}
		else{
			helper_opengl_widget->setWindowState(Qt::WindowState::WindowMaximized);
		}
	}


	status_label = new QLabel(this);
	status_label->setStyleSheet("background-color: black; color: white; border: 0");
	status_label->setFont(QFont("Monaco"));


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
					std::wcout << L"calling\n";
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

	// todo: use a reference counting pointer for document so we can delete main_doc
	// and helper_doc in DocumentView's destructor, not here.
	// ideally this function should just become:
	//		delete main_document_view;
	//		if(helper_document_view != main_document_view) delete helper_document_view;
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

	return ss.str();
}

void MainWidget::handle_escape() {
	text_command_line_edit->setText("");
	pending_link = {};
	current_pending_command = nullptr;
	current_widget = nullptr;

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
	if (last_smart_fit_page) {
		int current_page = main_document_view->get_current_page_number();
		if (current_page != last_smart_fit_page) {
			main_document_view->fit_to_page_width(true);
			last_smart_fit_page = current_page;
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

void MainWidget::move_document(float dx, float dy)
{
	if (main_document_view_has_document()) {
		main_document_view->move(dx, dy);
		//float prev_vertical_line_pos = opengl_widget->get_vertical_line_pos();
		//float new_vertical_line_pos = prev_vertical_line_pos - dy;
		//opengl_widget->set_vertical_line_pos(new_vertical_line_pos);
	}
}

void MainWidget::move_document_screens(int num_screens)
{
	int view_height = opengl_widget->height();
	float move_amount = num_screens * view_height * MOVE_SCREEN_PERCENTAGE;
	move_document(0, move_amount);
}

void MainWidget::on_config_file_changed(ConfigManager* new_config)
{
	//status_label->setStyleSheet(QString::fromStdWString(*config_manager->get_config<std::wstring>(L"status_label_stylesheet")));
	//text_command_line_edit_container->setStyleSheet(
	//	QString::fromStdWString(*config_manager->get_config<std::wstring>(L"text_command_line_stylesheet")));

	status_label->setStyleSheet("background-color: black; color: white; border: 0");
	text_command_line_edit_container->setStyleSheet("background-color: black; color: white; border: none;");
}

void MainWidget::toggle_dark_mode()
{
	this->dark_mode = !this->dark_mode;
	if (this->opengl_widget) {
		this->opengl_widget->set_dark_mode(this->dark_mode);
	}
	if (this->helper_opengl_widget) {
		this->helper_opengl_widget->set_dark_mode(this->dark_mode);
	}
}

void MainWidget::do_synctex_forward_search(const Path& pdf_file_path, const Path& latex_file_path, int line)
{

	//Path latex_file_path_with_redundant_dot = add_redundant_dot_to_path(latex_file_path);
	Path latex_file_path_with_redundant_dot = latex_file_path.add_redundant_dot();

	std::string latex_file_string = latex_file_path.get_path_utf8();
	std::string latex_file_with_redundant_dot_string = latex_file_path_with_redundant_dot.get_path_utf8();
	std::string pdf_file_string = pdf_file_path.get_path_utf8();

	//latex_file_string = "D:/phd/seventh/./pres.tex";
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

		while (node = synctex_next_result(scanner)) {
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

			if (pdf_file_path.get_path() != main_document_view->get_document()->get_path()) {
				open_document(pdf_file_path);
			}

			opengl_widget->set_synctex_highlights(highlight_rects);
			if (highlight_rects.size() == 0) {
				main_document_view->goto_page(target_page);
			}
			else {
				main_document_view->goto_offset_within_page(target_page, main_document_view->get_offset_x(), first_rect.value().y0);
			}
		}

	}
	synctex_scanner_free(scanner);
}

void MainWidget::on_new_instance_message(qint32 instance_id, QByteArray arguments_str)
{
	QStringList arguments = deserialize_string_array(arguments_str);

	handle_args(arguments);
}

//void MainWidget::handle_args(const std::vector<std::wstring> &arguments)
//{
//	std::optional<int> page = 0;
//	std::optional<float> x_loc, y_loc;
//	std::optional<float> zoom_level;
//
//	//todo: handle out of bounds error
//
//	std::wstring pdf_file_name = L"";
//
//	if (arguments.size() > 1) {
//		pdf_file_name = arguments[1];
//	}
//
//	std::optional<std::wstring> latex_file_name = {};
//	std::optional<int> latex_line = {};
//
//
//	for (int i = 0; i < arguments.size()-1; i++) {
//		if (arguments[i] == L"--page") {
//			if (is_string_numeric(arguments[i + 1])) {
//				page = std::stoi(arguments[i + 1].c_str());
//			}
//		}
//
//		if (arguments[i] == L"--inverse-search") {
//			inverse_search_command = arguments[i + 1];
//		}
//
//		if (arguments[i] == L"--forward-search") {
//			if (i + 3 < arguments.size()) {
//
//				latex_file_name = arguments[i + 1];
//				if (is_string_numeric(arguments[i + 2])) {
//					latex_line = std::stoi(arguments[i + 2].c_str());
//				}
//				pdf_file_name = arguments[i + 3];
//			}
//		}
//	}
//
//	if (arguments.size() > 1) {
//		open_document_at_location(pdf_file_name, page.value_or(0), x_loc, y_loc, zoom_level);
//
//		if (latex_file_name) {
//			do_synctex_forward_search(pdf_file_name, latex_file_name.value(), latex_line.value_or(0));
//		}
//
//		invalidate_render();
//	}
//	else {
//	}
//}

void MainWidget::handle_args(const QStringList& arguments)
{
	std::optional<int> page = -1;
	std::optional<float> x_loc, y_loc;
	std::optional<float> zoom_level;

	//todo: handle out of bounds error

	QCommandLineParser* parser = get_command_line_parser();

	if (!parser->parse(arguments)) {
		std::wcout << parser->errorText().toStdWString() << L"\n";
		return;
	}

	std::wstring pdf_file_name = L"";

	if (parser->positionalArguments().size() > 0) {
		pdf_file_name = parser->positionalArguments().at(0).toStdWString();
	}

	std::optional<std::wstring> latex_file_name = {};
	std::optional<int> latex_line = {};

	if (parser->isSet("forward-search-file")) {
		latex_file_name = parser->value("forward-search-file").toStdWString();
	}

	if (parser->isSet("forward-search-line")) {
		latex_line = parser->value("forward-search-line").toInt();
	}

	if (parser->isSet("page")) {
		page = parser->value("page").toInt();
	}

	if (parser->isSet("zoom")) {
		zoom_level = parser->value("zoom").toFloat();
	}

	if (parser->isSet("xloc")) {
		x_loc = parser->value("xloc").toFloat();
	}

	if (parser->isSet("yloc")) {
		y_loc = parser->value("yloc").toFloat();
	}

	if (parser->isSet("inverse-search")) {
		inverse_search_command =  parser->value("inverse-search").toStdWString();
	}

	// if no file is specified, use the previous file
	if (pdf_file_name == L"" && (main_document_view->get_document() != nullptr)) {
		pdf_file_name = main_document_view->get_document()->get_path();
	}

	if (page != -1) {
		open_document_at_location(pdf_file_name, page.value_or(0), x_loc, y_loc, zoom_level);
	}
	else if (latex_file_name) {
		do_synctex_forward_search(pdf_file_name, latex_file_name.value(), latex_line.value_or(0));
	}
	else {
		open_document(pdf_file_name);
	}

	invalidate_render();

	delete parser;
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
			delete_mark_with_symbol(db, symbol);
			// we should also delete the cached marks
			document_manager->delete_global_mark(symbol);
			main_document_view->add_mark(symbol);
		}
		else{
			main_document_view->add_mark(symbol);
			validate_render();
		}

	}
	else if (command->name == "goto_mark") {
		assert(main_document_view);

		if (isupper(symbol)) { // global mark
			std::vector<std::pair<std::wstring, float>> mark_vector;
			select_global_mark(db, symbol, mark_vector);
			if (mark_vector.size() > 0) {
				assert(mark_vector.size() == 1); // we can not have more than one global mark with the same name
				open_document(mark_vector[0].first, {}, mark_vector[0].second);
			}

		}
		else{
			main_document_view->goto_mark(symbol);
		}
	}
	//else if (command->name == "delete") {

	//	if (symbol == input_handler->create_link_sumbol) {
	//		main_document_view->delete_closest_link();
	//		validate_render();
	//	}
	//	else if (symbol == input_handler->create_bookmark_symbol) {
	//		main_document_view->delete_closest_bookmark();
	//		validate_render();
	//	}
	//}
}


void MainWidget::open_document(const Path& path, std::optional<float> offset_x, std::optional<float> offset_y, std::optional<float> zoom_level) {

	//save the previous document state
	if (main_document_view) {
		main_document_view->persist();
		update_history_state();
	}

	main_document_view->open_document(path.get_path(), &this->is_render_invalidated);
	bool has_document = main_document_view_has_document();

	if (has_document) {
		setWindowTitle(QString::fromStdWString(path.get_path()));
		push_state();
	}

	if ((path.get_path().size() > 0) && (!has_document)) {
		show_error_message(L"Could not open file: " + path.get_path());
	}
	main_document_view->on_view_size_change(main_window_width, main_window_height);

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

	main_document_view->open_document(path, &this->is_render_invalidated, true, {}, true);
	bool has_document = main_document_view_has_document();

	if (has_document) {
		setWindowTitle(QString::fromStdWString(path));
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
				std::wstring file_name = select_document_file_name();
				if (file_name.size() > 0) {
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

	if ((main_document_view->get_document() != nullptr) && (opengl_widget != nullptr)) {

		if (down == true) {
			if (current_pending_command && (current_pending_command->name == "goto_mark")) {
				main_document_view->goto_vertical_line_pos();
				current_pending_command = nullptr;
				validate_render();
				return;
			}

			float doc_x, doc_y;
			int page;
			main_document_view->window_to_document_pos(x, y, &doc_x, &doc_y, &page);
			opengl_widget->set_should_draw_vertical_line(true);
			float scale = 0.5f;
			fz_matrix ctm = fz_scale(scale, scale);
			fz_pixmap* pixmap = main_document_view->get_document()->get_small_pixmap(page);
			int small_doc_x = static_cast<int>(doc_x * scale);
			int small_doc_y = static_cast<int>(doc_y * scale);
			int best_vertical_loc = find_best_vertical_line_location(pixmap, small_doc_x, small_doc_y);
			float best_vertical_loc_doc_pos = best_vertical_loc / scale;
			int window_x, window_y;
			main_document_view->document_to_window_pos_in_pixels(page, doc_x, best_vertical_loc_doc_pos, &window_x, &window_y);
			float abs_doc_x, abs_doc_y;
			main_document_view->window_to_absolute_document_pos(window_x, window_y, &abs_doc_x, &abs_doc_y);
			main_document_view->set_vertical_line_pos(abs_doc_y);
			validate_render();

		}
		else {
			if (this->synctex_mode) {
				float doc_x, doc_y;
				int page;
				main_document_view->window_to_document_pos(x, y, &doc_x, &doc_y, &page);
				std::wstring docpath = main_document_view->get_document()->get_path();
				std::string docpath_utf8 = utf8_encode(docpath);
				synctex_scanner_t scanner = synctex_scanner_new_with_output_file(docpath_utf8.c_str(), nullptr, 1);

				int stat = synctex_edit_query(scanner, page + 1, doc_x, doc_y);

				if (stat > 0) {
					synctex_node_t node;
					while (node = synctex_next_result(scanner)) {
						int line = synctex_node_line(node);
						int column = synctex_node_column(node);
						if (column < 0) column = 0;
						int tag = synctex_node_tag(node);
						const char* file_name = synctex_scanner_get_name(scanner, tag);
						//std::wstringstream ss;

						//ss << L"\"C:\\Users\\Lion\\AppData\\Local\\Programs\\Microsoft VS Code\\code.exe\" --goto " << file_name << L":" << line << L":" << column;
						std::string line_string = std::to_string(line);
						std::string column_string = std::to_string(column);

						QString command = QString::fromStdWString(inverse_search_command).arg(file_name, line_string.c_str(), column_string.c_str());

						std::wcout << L"inverse search command: " << inverse_search_command << L"\n";
						std::wcout << L"executed command: " << command.toStdWString() << L"\n";

						QProcess::startDetached(command);

					}

				}
				synctex_scanner_free(scanner);

			}
		}

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

void MainWidget::update_history_state()
{
	if (!main_document_view_has_document()) return; // we don't add empty document to history
	if (history.size() == 0) return; // this probably should never execute

	DocumentViewState dvs = main_document_view->get_state();
	history[current_history_index] = dvs;
}

void MainWidget::push_state()
{

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

void MainWidget::next_state()
{
	update_current_history_index();
	if (current_history_index < history.size()-1) {
		current_history_index++;
		set_main_document_view_state(history[current_history_index]);

	}
}

void MainWidget::prev_state()
{
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

			Document* link_owner = document_manager->get_document(link_to_edit.value().dst.document_path);

			OpenedBookState state = main_document_view->get_state().book_state;
			link_to_edit.value().dst.book_state = state;

			if (link_owner) {
				link_owner->update_link(link_to_edit.value());
			}

			update_link(db, history[current_history_index].document_path,
				state.offset_x, state.offset_y, state.zoom_level, link_to_edit->src_offset_y);
			link_to_edit = {};
		}

		set_main_document_view_state(history[current_history_index]);
	}
}

void MainWidget::update_current_history_index()
{
	if (main_document_view_has_document()) {
		DocumentViewState current_state = main_document_view->get_state();
		history[current_history_index] = current_state;

	}
}

void MainWidget::set_main_document_view_state(DocumentViewState new_view_state) {

	if (main_document_view->get_document()->get_path() != new_view_state.document_path) {
		main_document_view->open_document(new_view_state.document_path, &this->is_ui_invalidated);
		setWindowTitle(QString::fromStdWString(new_view_state.document_path));
	}

	main_document_view->on_view_size_change(main_window_width, main_window_height);
	main_document_view->set_book_state(new_view_state.book_state);
}

void MainWidget::handle_click(int pos_x, int pos_y) {
	auto link_ = main_document_view->get_link_in_pos(pos_x, pos_y);
	if (link_.has_value()) {
		PdfLink link = link_.value();
		int page;
		float offset_x, offset_y;

		if (link.uri.substr(0, 4).compare("http") == 0) {
			open_url(link.uri.c_str());
			return;
		}

		parse_uri(link.uri, &page, &offset_x, &offset_y);

		// convert one indexed page to zero indexed page
		page--;

		// we usually just want to center the y offset and not the x offset (otherwise for example
		// a link at the right side of the screen will be centered, causing most of screen state to be empty)
		offset_x = main_document_view->get_offset_x();

		long_jump_to_destination(page, offset_x, offset_y);
	}
}

void MainWidget::mouseReleaseEvent(QMouseEvent* mevent) {

	if (mevent->button() == Qt::MouseButton::LeftButton) {
		handle_left_click(mevent->pos().x(), mevent->pos().y(), false);
	}

	if (mevent->button() == Qt::MouseButton::RightButton) {


		handle_right_click(mevent->pos().x(), mevent->pos().y(), false);
	}

	if (mevent->button() == Qt::MouseButton::MiddleButton) {

		int page;
		float offset_x, offset_y;

		Qt::KeyboardModifiers modifiers = QGuiApplication::queryKeyboardModifiers();
		bool is_shift_pressed = modifiers.testFlag(Qt::ShiftModifier);


		main_document_view->window_to_document_pos(mevent->pos().x(), mevent->pos().y(), &offset_x, &offset_y, &page);

		fz_stext_page* stext_page = main_document_view->get_document()->get_stext_with_page_number(page);
		std::vector<fz_stext_char*> flat_chars;
		get_flat_chars_from_stext_page(stext_page, flat_chars);


		std::optional<std::wstring> text_on_pointer = main_document_view->get_document()->get_text_at_position(flat_chars, offset_x, offset_y);
		std::optional<std::wstring> paper_name_on_pointer = main_document_view->get_document()->get_paper_name_at_position(flat_chars, offset_x, offset_y);
		std::optional<std::wstring> reference_text_on_pointer = main_document_view->get_document()->get_reference_text_at_position(flat_chars, offset_x, offset_y);
		std::optional<std::wstring> equation_text_on_pointer = main_document_view->get_document()->get_equation_text_at_position(flat_chars, offset_x, offset_y);

		bool was_figure = false;

		if (equation_text_on_pointer) {
			 std::optional<IndexedData> eqdata_ = main_document_view->get_document()->find_equation_with_string(equation_text_on_pointer.value());
			 if (eqdata_) {
				 IndexedData refdata = eqdata_.value();
				 update_history_state();

				 float page_height = main_document_view->get_document()->get_page_height(refdata.page);

				 main_document_view->goto_offset_within_page(refdata.page, main_document_view->get_offset_x(), refdata.y_offset);

				 push_state();
				 invalidate_render();
				 return;
			 }
		}

		if (reference_text_on_pointer) {
			 std::optional<IndexedData> refdata_ = main_document_view->get_document()->find_reference_with_string(reference_text_on_pointer.value());
			 if (refdata_) {
				 IndexedData refdata = refdata_.value();
				 update_history_state();

				 float page_height = main_document_view->get_document()->get_page_height(refdata.page);

				 main_document_view->goto_offset_within_page(refdata.page, main_document_view->get_offset_x(), refdata.y_offset);

				 push_state();
				 invalidate_render();
			 }

		}
		else {

			if (text_on_pointer) {

				std::wstring figure_string = get_figure_string_from_raw_string(text_on_pointer.value());
				if (figure_string.size() > 0) {
					was_figure = true;

					int fig_page;
					float fig_offset;
					if (main_document_view->get_document()->find_figure_with_string(figure_string, page, &fig_page, &fig_offset)) {
						float offset_x = main_document_view->get_offset_x();
						long_jump_to_destination(fig_page, offset_x, fig_offset);

					}
				}

			}
			if ((!was_figure) && paper_name_on_pointer) {
				if (paper_name_on_pointer.value().size() > 5) {
					if (is_shift_pressed) {
						search_libgen(paper_name_on_pointer.value());
					}
					else {
						search_google_scholar(paper_name_on_pointer.value());
					}
				}
			}
		}

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
		handle_left_click(mevent->pos().x(), mevent->pos().y(), true);
	}

	if (mevent->button() == Qt::MouseButton::RightButton) {
		handle_right_click(mevent->pos().x(), mevent->pos().y(), true);
	}

	if (mevent->button() == Qt::MouseButton::XButton1) {
		handle_command(command_manager.get_command_with_name("prev_state"), 0);
	}

	if (mevent->button() == Qt::MouseButton::XButton2) {
		handle_command(command_manager.get_command_with_name("next_state"), 0);
	}
}

void MainWidget::wheelEvent(QWheelEvent* wevent) {
	int num_repeats = 1;

	const Command* command = nullptr;

	bool is_control_pressed = QApplication::queryKeyboardModifiers().testFlag(Qt::ControlModifier);

	if (!is_control_pressed) {
		if (wevent->angleDelta().y() > 0) {
			command = input_handler->handle_key(Qt::Key::Key_Up, false, false, &num_repeats);
		}
		if (wevent->angleDelta().y() < 0) {
			command = input_handler->handle_key(Qt::Key::Key_Down, false, false, &num_repeats);
		}

		if (wevent->angleDelta().x() > 0) {
			command = input_handler->handle_key(Qt::Key::Key_Right, false, false, &num_repeats);
		}
		if (wevent->angleDelta().x() < 0) {
			command = input_handler->handle_key(Qt::Key::Key_Left, false, false, &num_repeats);
		}
	}

	if (is_control_pressed) {
		if (wevent->angleDelta().y() > 0) {
			command = command_manager.get_command_with_name("zoom_in");
		}
		if (wevent->angleDelta().y() < 0) {
			command = command_manager.get_command_with_name("zoom_out");
		}

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
	text_command_line_edit_container->show();
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
		if (command->name == "search" || command->name == "chapter_search" || command->name == "ranged_search" || command->name == "add_bookmark") {
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
	if (command->name == "goto_begining") {
		if (num_repeats) {
			main_document_view->goto_page(num_repeats-1);
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
		move_document(0.0f, 72.0f * rp * VERTICAL_MOVE_AMOUNT);
	}
	else if (command->name == "move_up") {
		move_document(0.0f, -72.0f * rp * VERTICAL_MOVE_AMOUNT);
	}

	else if (command->name == "move_right") {
		main_document_view->move(72.0f * rp * HORIZONTAL_MOVE_AMOUNT, 0.0f);
		last_smart_fit_page = {};
	}

	else if (command->name == "move_left") {
		main_document_view->move(-72.0f * rp * HORIZONTAL_MOVE_AMOUNT, 0.0f);
		last_smart_fit_page = {};
	}

	else if (command->name == "link") {
		handle_link();
	}

	else if (command->name == "goto_link") {
		std::optional<Link> link = main_document_view->find_closest_link();
		if (link) {
			open_document(link->dst);
		}
	}
	else if (command->name == "edit_link") {
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

	else if (command->name == "fit_to_page_width") {
		main_document_view->fit_to_page_width();
		last_smart_fit_page = {};
	}
	else if (command->name == "fit_to_page_width_smart") {
		main_document_view->fit_to_page_width(true);
		int current_page = main_document_view->get_current_page_number();
		last_smart_fit_page = current_page;
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
		if (main_document_view->get_document()->has_toc()) {
			if (FLAT_TABLE_OF_CONTENTS) {
				std::vector<std::wstring> flat_toc;
				std::vector<int> current_document_toc_pages;
				get_flat_toc(main_document_view->get_document()->get_toc(), flat_toc, current_document_toc_pages);
				current_widget = std::make_unique<FilteredSelectWindowClass<int>>(flat_toc, current_document_toc_pages, [&](int* page_value) {
					if (page_value) {
						validate_render();
						update_history_state();
						main_document_view->goto_page(*page_value);
						push_state();
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
							validate_render();
							update_history_state();
							main_document_view->goto_page(toc_node->page);
							push_state();
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
			opened_docs_names.push_back(Path(p).filename().value_or(L"<ERROR>"));
		}

		if (opened_docs_paths.size() > 0) {
			current_widget = std::make_unique<FilteredSelectWindowClass<std::wstring>>(opened_docs_names,
				opened_docs_paths,
				[&](std::wstring* doc_path) {
					if (doc_path->size() > 0) {
						validate_render();
						open_document(*doc_path);
					}
				},
				config_manager,
				this,
				[&](std::wstring* doc_path) {
					delete_opened_book(db, *doc_path);
				});
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
		current_widget = std::make_unique<FilteredSelectWindowClass<float>>(
			option_names,
			option_locations,
			[&](float* offset_value) {
				if (offset_value) {
					validate_render();
					update_history_state();
					main_document_view->set_offset_y(*offset_value);
					push_state();
				}
			},
			config_manager,
			this,
				[&](float* offset_value) {
				if (offset_value) {
					main_document_view->delete_closest_bookmark_to_offset(*offset_value);
				}
			});
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
		current_widget = std::make_unique<FilteredSelectWindowClass<BookState>>(
			descs,
			book_states,
			[&](BookState* book_state) {
				if (book_state) {
					validate_render();
					open_document(book_state->document_path, 0.0f, book_state->offset_y);
				}
			},
			config_manager,
			this,
			[&](BookState* book_state) {
				if (book_state) {
					delete_bookmark(db, book_state->document_path, book_state->offset_y);
				}
			});
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

	else if (command->name == "toggle_synctex") {
		this->synctex_mode = !this->synctex_mode;
	}

	else if (command->name == "delete_link") {

		main_document_view->delete_closest_link();
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
		open_url((selected_text).c_str());
	}
	else if (command->name == "search_selected_text_in_libgen") {
		search_libgen(selected_text);
	}
	else if (command->name == "toggle_dark_mode") {
		this->toggle_dark_mode();
	}
	else if (command->name == "debug") {
		wprintf(L"_________________________________\n");
		for (auto& x : history) {
			//std::wcout << x.document_path << "\n";
			wprintf(x.document_path.c_str());
			wprintf(L"\n");
		}
		wprintf(L"_________________________________\n");
		int b = 2;

	}

	if (command->pushes_state) {
		push_state();
	}

	validate_render();
}

void MainWidget::handle_link() {
	if (!main_document_view_has_document()) return;

	if (is_pending_link_source_filled()) {
		auto [source_path, pl] = pending_link.value();
		pl.dst = main_document_view->get_state();
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
				pl.dst.document_path,
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

	if (current_pending_command->name == "add_bookmark") {
		main_document_view->add_bookmark(text);
	}

	if (current_pending_command->name == "goto_page_with_page_number") {

		if (is_string_numeric(text.c_str()) && text.size() < 6) { // make sure the page number is valid
			int dest = std::stoi(text.c_str()) - 1;
			main_document_view->goto_page(dest);
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
			open_file(user_keys_path.get_path());
		}
		else if (text == L"prefs") {
			open_file(default_config_path.get_path());
		}
		else if (text == L"prefs_user") {
			open_file(user_config_path.get_path());
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

void MainWidget::complete_pending_link(const DocumentViewState& destination_view_state)
{
	Link& pl = pending_link.value().second;
	pl.dst = destination_view_state;
	main_document_view->get_document()->add_link(pl);
	pending_link = {};
}

void MainWidget::long_jump_to_destination(int page, float offset_x, float offset_y)
{
	if (!is_pending_link_source_filled()) {
		update_history_state();
		main_document_view->goto_offset_within_page(page, offset_x, offset_y);
		push_state();
	}
	else {
		// if we press the link button and then click on a pdf link, we automatically link to the
		// link's destination

		DocumentViewState dest_state;
		dest_state.document_path = main_document_view->get_document()->get_path();
		dest_state.book_state.offset_x = offset_x;
		dest_state.book_state.offset_y = main_document_view->get_page_offset(page) + offset_y;
		dest_state.book_state.zoom_level = main_document_view->get_zoom_level();

		complete_pending_link(dest_state);
	}
	invalidate_render();
}
