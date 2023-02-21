#pragma once

#ifndef MAIN_WIDGET_DEFINE
#define MAIN_WIDGET_DEFINE

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
#include <qscrollbar.h>

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

public:
	fz_context* mupdf_context = nullptr;
	DatabaseManager* db_manager = nullptr;
	DocumentManager* document_manager = nullptr;
	CommandManager* command_manager = nullptr;
	ConfigManager* config_manager = nullptr;
	PdfRenderer* pdf_renderer = nullptr;
	InputHandler* input_handler = nullptr;
	CachedChecksummer* checksummer = nullptr;

	PdfViewOpenGLWidget* opengl_widget = nullptr;
	PdfViewOpenGLWidget* helper_opengl_widget = nullptr;
	QScrollBar* scroll_bar = nullptr;

	//sgd::optional<Command> current_pending_command;
	std::unique_ptr<Command> pending_command_instance = nullptr;

	DocumentView* main_document_view = nullptr;
	DocumentView* helper_document_view = nullptr;

	// current widget responsible for selecting an option (for example toc or bookmarks)
	QWidget* current_widget = nullptr;
	std::vector<QWidget*> garbage_widgets;

	std::function<void(std::string)> on_command_done = nullptr;
	std::vector<DocumentViewState> history;
	int current_history_index = -1;

	std::wstring custom_status_message = L"";

	bool* should_quit = nullptr;
	// last position when mouse was clicked in absolute document space
	AbsoluteDocumentPos last_mouse_down;
	AbsoluteDocumentPos last_mouse_down_document_offset;

	//int last_mouse_down_window_x = 0;
	//int last_mouse_down_window_y = 0;
	WindowPos last_mouse_down_window_pos;

	AbsoluteDocumentPos selection_begin;
	AbsoluteDocumentPos selection_end;

	bool rect_select_mode = false;
	std::optional<AbsoluteDocumentPos> rect_select_begin = {};
	std::optional<AbsoluteDocumentPos> rect_select_end = {};
 // when set, mouse wheel moves the visual mark
	bool visual_scroll_mode = false;
	bool debug_mode = false;

	bool horizontal_scroll_locked = false;

	// is the user currently selecing text? (happens when we left click and move the cursor)
	bool is_selecting = false;
	// is the user in word select mode? (happens when we double left click and move the cursor)
	bool is_word_selecting = false;
	std::wstring selected_text;

	bool is_select_highlight_mode = false;
	char select_highlight_type = 'a';

	bool smooth_scroll_mode = false;
	float smooth_scroll_speed = 0.0f;

	QTimer* validation_interval_timer = nullptr;

	std::optional<Portal> link_to_edit = {};
	int selected_highlight_index = -1;

	std::optional<std::pair<std::optional<std::wstring>, Portal>> pending_link;

	bool mouse_drag_mode = false;
	bool synctex_mode = false;
	bool is_dragging = false;

	bool should_show_status_label = true;

	std::optional<CharacterAddress> typing_location;

	int main_window_width = 0;
	int main_window_height = 0;

	QWidget* text_command_line_edit_container = nullptr;
	QLabel* text_command_line_edit_label = nullptr;
	QLineEdit* text_command_line_edit = nullptr;
	QLabel* status_label = nullptr;

	bool is_render_invalidated = false;
	bool is_ui_invalidated = false;

	bool should_show_last_command = false;

	//std::optional<std::pair<std::wstring, int>> last_smart_fit_state = {};
	std::optional<int> last_smart_fit_page = {};

	std::wstring inverse_search_command;

	std::optional<PdfViewOpenGLWidget::OverviewMoveData> overview_move_data = {};
	std::optional<PdfViewOpenGLWidget::OverviewResizeData> overview_resize_data = {};

	std::optional<char> command_to_be_executed_symbol = {};

	std::vector<DocumentPos> smart_view_candidates;
	int index_into_candidates = 0;

	QTime last_text_select_time = QTime::currentTime();
	QTime last_speed_update_time = QTime::currentTime();

	bool main_document_view_has_document();
	std::optional<std::string> get_last_opened_file_checksum();

	void open_document(const std::wstring& doc_path, bool* invalid_flag, bool load_prev_state = true, std::optional<OpenedBookState> prev_state = {}, bool foce_load_dimensions=false);



	void toggle_statusbar();
	void toggle_titlebar();
	void handle_paper_name_on_pointer(std::wstring paper_name, bool is_shift_pressed);
	//void paintEvent(QPaintEvent* paint_event) override;
	void persist() ;
	bool is_pending_link_source_filled();
	std::wstring get_status_string();
	void handle_escape();
	bool is_waiting_for_symbol();
	void key_event(bool released, QKeyEvent* kevent);
	void handle_left_click(WindowPos click_pos, bool down, bool is_shift_pressed, bool is_control_pressed, bool is_alt_pressed);
	void handle_right_click(WindowPos click_pos, bool down, bool is_shift_pressed, bool is_control_pressed, bool is_alt_pressed);

	void next_state();
	void prev_state();
	void update_current_history_index();

	void set_main_document_view_state(DocumentViewState new_view_state);
	void handle_click(WindowPos pos);

	//bool eventFilter(QObject* obj, QEvent* event) override;
	void show_textbar(const std::wstring& command_name, bool should_fill_with_selected_text = false);
	void toggle_two_window_mode();
	void toggle_window_configuration();
	void handle_portal();
	void add_portal(std::wstring source_path, Portal new_link);
	void toggle_fullscreen();
	void toggle_presentation_mode();
	void set_presentation_mode(bool mode);
    void set_synctex_mode(bool mode);
    void toggle_synctex_mode();
    void complete_pending_link(const PortalViewState& destination_view_state);
	void long_jump_to_destination(DocumentPos pos);
	void long_jump_to_destination(int page, float offset_y);
	void long_jump_to_destination(float abs_offset_y);
	void execute_command(std::wstring command, std::wstring text=L"", bool wait=false);
	//QString get_status_stylesheet();
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
	bool is_visual_mark_mode();
	void reload();

	QString get_font_face_name(); 

	void reset_highlight_links();
	void set_rect_select_mode(bool mode);
	void clear_selected_rect();
	void clear_selected_text();

	std::optional<fz_rect> get_selected_rect_absolute();
	bool get_selected_rect_document(int& out_page, fz_rect& out_rect);
	Document* doc();

	MainWidget(
		fz_context* mupdf_context,
		DatabaseManager* db_manager,
		DocumentManager* document_manager,
		ConfigManager* config_manager,
		CommandManager* command_manager,
		InputHandler* input_handler,
		CachedChecksummer* checksummer,
		bool* should_quit_ptr,
		QWidget* parent=nullptr
	);
	MainWidget(MainWidget* other);

	~MainWidget();

	//void handle_command(NewCommand* command, int num_repeats);
	void handle_command_types(std::unique_ptr<Command> command, int num_repeats);
	void handle_pending_text_command(std::wstring text);

	void invalidate_render();
	void invalidate_ui();
	void open_document(const Path& path, std::optional<float> offset_x = {}, std::optional<float> offset_y = {}, std::optional<float> zoom_level = {});
	void open_document_with_hash(const std::string& hash, std::optional<float> offset_x = {}, std::optional<float> offset_y = {}, std::optional<float> zoom_level = {});
	void open_document_at_location(const Path& path, int page, std::optional<float> x_loc, std::optional<float> y_loc, std::optional<float> zoom_level);
	void open_document(const DocumentViewState& state);
	void open_document(const PortalViewState& checksum);
	void validate_render();
	void validate_ui();
	void zoom(WindowPos pos, float zoom_factor, bool zoom_in);
	void move_document(float dx, float dy);
	void move_document_screens(int num_screens);
	void focus_text(int page, const std::wstring& text);

	void move_visual_mark(int offset);
	void on_config_file_changed(ConfigManager* new_config) override;
	void toggle_mouse_drag_mode();
	void toggle_dark_mode();
	void do_synctex_forward_search(const Path& pdf_file_path,const Path& latex_file_path, int line, int column);
	//void handle_args(const QStringList &arguments);
	void update_link_with_opened_book_state(Portal lnk, const OpenedBookState& new_state);
	void update_closest_link_with_opened_book_state(const OpenedBookState& new_state);
	void set_current_widget(QWidget* new_widget);
	bool focus_on_visual_mark_pos(bool moving_down);
	void toggle_visual_scroll_mode();
	void set_overview_link(PdfLink link);
	void set_overview_position(int page, float offset);
	bool find_location_of_text_under_pointer(WindowPos pos, int* out_page, float* out_offset, bool update_candidates=false);
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

	std::vector<fz_rect> get_flat_words(std::vector<std::vector<fz_rect>>* flat_word_chars=nullptr);

	std::optional<fz_rect> get_tag_rect(std::string tag, std::vector<fz_rect>* word_chars=nullptr);
	std::optional<fz_irect> get_tag_window_rect(std::string tag, std::vector<fz_irect>* char_rects=nullptr);

	bool is_rotated();
	void on_new_paper_added(const std::wstring& file_path);
	void scroll_overview(int amount);
	int get_current_page_number() const;
	void set_inverse_search_command(const std::wstring& new_command);
	int get_current_monitor_width(); int get_current_monitor_height();
	void synctex_under_pos(WindowPos position);
	std::optional<std::wstring> get_paper_name_under_cursor();
	void set_status_message(std::wstring new_status_string);
	void remove_self_from_windows();
	//void handle_additional_command(std::wstring command_name, bool wait=false);
	std::optional<DocumentPos> get_overview_position();
	void handle_keyboard_select(const std::wstring& text);
	//void run_multiple_commands(const std::wstring& commands);
	void push_state(bool update=true);
	void toggle_scrollbar();
	void update_scrollbar();
	void handle_portal_overview_update();
	void goto_overview();
	bool is_rect_visible(int page, fz_rect rect);
	void set_mark_in_current_location(char symbol);
	void goto_mark(char symbol);
	void advance_command(std::unique_ptr<Command> command);
	void perform_search(std::wstring text, bool is_regex=false);
	void overview_to_definition();
	void portal_to_definition();
	void move_visual_mark_command(int amount);

	void handle_vertical_move(int amount);
	void handle_horizontal_move(int amount);
	void handle_goto_bookmark();
	void handle_goto_bookmark_global();
	void handle_add_highlight(char symbol);
	void handle_goto_highlight();
	void handle_goto_highlight_global();
	void handle_goto_toc();
	void handle_open_prev_doc();
	void handle_move_screen(int amount);
	void handle_new_window();
	void handle_open_link(const std::wstring& text, bool copy=false);
	void handle_overview_link(const std::wstring& text);
	void handle_portal_to_link(const std::wstring& text);
	void handle_keys_user_all();
	void handle_prefs_user_all();
	void handle_portal_to_overview();
	void handle_focus_text(const std::wstring& text);
	void handle_goto_window();
	void handle_toggle_smooth_scroll_mode();
	void handle_overview_to_portal();
	void handle_toggle_typing_mode();
	void handle_delete_highlight_under_cursor();
	void synchronize_pending_link();
	void refresh_all_windows();
	std::optional<std::pair<int, fz_link*>> get_selected_link(const std::wstring& text);

	int num_visible_links();

	protected:
	void focusInEvent(QFocusEvent* ev);
	void resizeEvent(QResizeEvent* resize_event) override;
	void changeEvent(QEvent* event) override;
	void mouseMoveEvent(QMouseEvent* mouse_event) override;

	// we already handle drag and drop on macos elsewhere
#ifndef Q_OS_MACOS
	void dragEnterEvent(QDragEnterEvent* e);
	void dropEvent(QDropEvent* event);
#endif

	void closeEvent(QCloseEvent* close_event) override;
	void keyPressEvent(QKeyEvent* kevent) override;
	void keyReleaseEvent(QKeyEvent* kevent) override;
	void mouseReleaseEvent(QMouseEvent* mevent) override;
	void mousePressEvent(QMouseEvent* mevent) override;
	void mouseDoubleClickEvent(QMouseEvent* mevent) override;
	void wheelEvent(QWheelEvent* wevent) override;

};

#endif
