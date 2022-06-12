#pragma once

#include <string>
#include <memory>
#include <vector>
#include <optional>

#include <qwidget.h>
#include <qlineedit.h>
#include <qtextedit.h>
#include <qtimer.h>
#include <qbytearray.h>
#include <qdrag.h>

#include <mupdf/fitz.h>
#include "document_view.h"
#include "document.h"
#include "input.h"
#include "book.h"
#include "config.h"
#include "pdf_renderer.h"
#include "input.h"
#include "pdf_view_opengl_widget.h"
#include "path.h"
#include "checksum.h"

extern float VERTICAL_MOVE_AMOUNT;
extern float HORIZONTAL_MOVE_AMOUNT;

class MainWidget : public QWidget, ConfigFileChangeListener{

private:
	fz_context* mupdf_context = nullptr;
	DatabaseManager* db_manager = nullptr;
	DocumentManager* document_manager = nullptr;
	CommandManager command_manager;
	ConfigManager* config_manager = nullptr;
	PdfRenderer* pdf_renderer = nullptr;
	InputHandler* input_handler = nullptr;
	CachedChecksummer* checksummer = nullptr;

	PdfViewOpenGLWidget* opengl_widget = nullptr;
	PdfViewOpenGLWidget* helper_opengl_widget = nullptr;

	std::optional<Command> current_pending_command;

	DocumentView* main_document_view = nullptr;
	DocumentView* helper_document_view = nullptr;

	// current widget responsible for selecting an option (for example toc or bookmarks)
	QWidget* current_widget = nullptr;
	std::vector<QWidget*> garbage_widgets;

	std::function<void(std::string)> on_command_done = nullptr;
	std::vector<DocumentViewState> history;
	int current_history_index = -1;

	bool* should_quit = nullptr;
	// last position when mouse was clicked in absolute document space
	AbsoluteDocumentPos last_mouse_down;
	AbsoluteDocumentPos last_mouse_down_document_offset;

	//int last_mouse_down_window_x = 0;
	//int last_mouse_down_window_y = 0;
	WindowPos last_mouse_down_window_pos;

	AbsoluteDocumentPos selection_begin;
	AbsoluteDocumentPos selection_end;

	// when set, mouse wheel moves the visual mark
	bool visual_scroll_mode = false;

	bool horizontal_scroll_locked = false;

	// is the user currently selecing text? (happens when we left click and move the cursor)
	bool is_selecting = false;
	// is the user in word select mode? (happens when we double left click and move the cursor)
	bool is_word_selecting = false;
	std::wstring selected_text;

	bool is_select_highlight_mode = false;
	char select_highlight_type = 'a';

	std::optional<Link> link_to_edit = {};
	int selected_highlight_index = -1;

	std::optional<std::pair<std::optional<std::wstring>, Link>> pending_link;

	bool mouse_drag_mode = false;
	bool synctex_mode = false;
	bool is_dragging = false;

	bool should_show_status_label = true;

	int main_window_width = 0;
	int main_window_height = 0;

	QWidget* text_command_line_edit_container = nullptr;
	QLabel* text_command_line_edit_label = nullptr;
	QLineEdit* text_command_line_edit = nullptr;
	QLabel* status_label = nullptr;

	bool is_render_invalidated = false;
	bool is_ui_invalidated = false;

	const Command* last_command = nullptr;

	//std::optional<std::pair<std::wstring, int>> last_smart_fit_state = {};
	std::optional<int> last_smart_fit_page = {};

	std::wstring inverse_search_command;

	std::optional<PdfViewOpenGLWidget::OverviewMoveData> overview_move_data = {};
	std::optional<PdfViewOpenGLWidget::OverviewResizeData> overview_resize_data = {};

	std::optional<char> command_to_be_executed_symbol = {};

	QTime last_text_select_time = QTime::currentTime();

	bool main_document_view_has_document();
	std::optional<std::string> get_last_opened_file_checksum();

	void open_document(const std::wstring& doc_path, bool* invalid_flag, bool load_prev_state = true, std::optional<OpenedBookState> prev_state = {}, bool foce_load_dimensions=false);

protected:

	void focusInEvent(QFocusEvent* ev);

	void toggle_statusbar();
	void handle_paper_name_on_pointer(std::wstring paper_name, bool is_shift_pressed);
	//void paintEvent(QPaintEvent* paint_event) override;
	void resizeEvent(QResizeEvent* resize_event) override;
	void changeEvent(QEvent* event) override;
	void mouseMoveEvent(QMouseEvent* mouse_event) override;

	// we already handle drag and drop on macos elsewhere
#ifndef Q_OS_MACOS
	void dragEnterEvent(QDragEnterEvent* e);
	void dropEvent(QDropEvent* event);
#endif

	void closeEvent(QCloseEvent* close_event) override;
	void persist() ;
	bool is_pending_link_source_filled();
	std::wstring get_status_string();
	void handle_escape();
	void keyPressEvent(QKeyEvent* kevent) override;
	void keyReleaseEvent(QKeyEvent* kevent) override;
	bool handle_command_with_symbol(const Command* command, char symbol);
	void handle_command_with_file_name(const Command* command, std::wstring file_name);
	bool is_waiting_for_symbol();
	void key_event(bool released, QKeyEvent* kevent);
	void handle_left_click(WindowPos click_pos, bool down);
	void handle_right_click(WindowPos click_pos, bool down);

	void update_history_state();
	void push_state();
	void next_state();
	void prev_state();
	void update_current_history_index();

	void set_main_document_view_state(DocumentViewState new_view_state);
	void handle_click(WindowPos pos);
	void mouseReleaseEvent(QMouseEvent* mevent) override;
	void mousePressEvent(QMouseEvent* mevent) override;
	void mouseDoubleClickEvent(QMouseEvent* mevent) override;

	//bool eventFilter(QObject* obj, QEvent* event) override;
	void wheelEvent(QWheelEvent* wevent) override;
	void show_textbar(const std::wstring& command_name, bool should_fill_with_selected_text = false);
	void toggle_two_window_mode();
	void toggle_window_configuration(); void handle_command_types(const Command* command, int num_repeats);
	void handle_link();
	void handle_pending_text_command(std::wstring text);
	void toggle_fullscreen();
	void toggle_presentation_mode();
    void toggle_synctex_mode();
    void complete_pending_link(const LinkViewState& destination_view_state);
	void long_jump_to_destination(DocumentPos pos);
	void long_jump_to_destination(int page, float offset_y);
	void long_jump_to_destination(float abs_offset_y);
	void execute_command(std::wstring command, std::wstring text=L"");
	QString get_status_stylesheet();
	int get_status_bar_height();
    void smart_jump_under_pos(WindowPos pos);
    bool overview_under_pos(WindowPos pos);
    void visual_mark_under_pos(WindowPos pos);

	QRect get_main_window_rect();
	QRect get_helper_window_rect();


	void show_password_prompt_if_required();
	void handle_link_click(const PdfLink& link);

	std::wstring get_window_configuration_string();
	std::wstring get_serialized_configuration_string();
	void save_auto_config();

	void handle_close_event();
	void return_to_last_visual_mark();
	void move_visual_mark_down();
	void move_visual_mark_up();
	bool is_visual_mark_mode();

public:
	Document* doc();

	void handle_command(const Command* command, int num_repeats);

	MainWidget(
		fz_context* mupdf_context,
		DatabaseManager* db_manager,
		DocumentManager* document_manager,
		ConfigManager* config_manager,
		InputHandler* input_handler,
		CachedChecksummer* checksummer,
		bool* should_quit_ptr,
		QWidget* parent=nullptr
	);
	MainWidget(MainWidget* other);

	~MainWidget();

	void invalidate_render();
	void invalidate_ui();
	void open_document(const Path& path, std::optional<float> offset_x = {}, std::optional<float> offset_y = {}, std::optional<float> zoom_level = {});
	void open_document_with_hash(const std::string& hash, std::optional<float> offset_x = {}, std::optional<float> offset_y = {}, std::optional<float> zoom_level = {});
	void open_document_at_location(const Path& path, int page, std::optional<float> x_loc, std::optional<float> y_loc, std::optional<float> zoom_level);
	void open_document(const DocumentViewState& state);
	void open_document(const LinkViewState& checksum);
	void validate_render();
	void validate_ui();
	void move_document(float dx, float dy);
	void move_document_screens(int num_screens);

	void on_config_file_changed(ConfigManager* new_config) override;
	void toggle_mouse_drag_mode();
	void toggle_dark_mode();
	void do_synctex_forward_search(const Path& pdf_file_path,const Path& latex_file_path, int line, int column);
	//void handle_args(const QStringList &arguments);
	void update_link_with_opened_book_state(Link lnk, const OpenedBookState& new_state);
	void update_closest_link_with_opened_book_state(const OpenedBookState& new_state);
	void set_current_widget(QWidget* new_widget);
	bool focus_on_visual_mark_pos(bool moving_down);
	void toggle_visual_scroll_mode();
	void set_overview_link(PdfLink link);
	void set_overview_position(int page, float offset);
	bool find_location_of_text_under_pointer(WindowPos pos, int* out_page, float* out_offset);
	std::optional<std::wstring> get_current_file_name();
	CommandManager* get_command_manager();

	void move_vertical(float amount);
	void move_horizontal(float amount);
	void get_window_params_for_one_window_mode(int* main_window_size, int* main_window_move);
	void get_window_params_for_two_window_mode(int* main_window_size, int* main_window_move, int* helper_window_size, int* helper_window_move);
	void apply_window_params_for_one_window_mode(bool force_resize=false);
	void apply_window_params_for_two_window_mode();
	bool helper_window_overlaps_main_window();
	void highlight_words();

	std::vector<fz_rect> get_flat_words();

	fz_rect get_tag_rect(std::string tag);
	fz_irect get_tag_window_rect(std::string tag);

	bool is_rotated();
	void on_new_paper_added(const std::wstring& file_path);
	void scroll_overview_down();
	void scroll_overview_up();
	int get_current_page_number() const;
	void set_inverse_search_command(const std::wstring& new_command);
	void execute_predefined_command(char symbol);

};
