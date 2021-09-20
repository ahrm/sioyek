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

	const Command* current_pending_command = nullptr;

	DocumentView* main_document_view = nullptr;
	DocumentView* helper_document_view = nullptr;

	// current widget responsible for selecting an option (for example toc or bookmarks)
	std::unique_ptr<QWidget> current_widget = nullptr;

	std::vector<DocumentViewState> history;
	int current_history_index = -1;

	// last position when mouse was clicked in absolute document space
	float last_mouse_down_x = 0;
	float last_mouse_down_y = 0;

	float last_mouse_down_document_x_offset = 0;
	float last_mouse_down_document_y_offset = 0;

	int last_mouse_down_window_x = 0;
	int last_mouse_down_window_y = 0;

	float selection_begin_x = 0;
	float selection_begin_y = 0;
	float selection_end_x = 0;
	float selection_end_y = 0;

	// is the user currently selecing text? (happens when we left click and move the cursor)
	bool is_selecting = false;
	// is the user in word select mode? (happens when we double left click and move the cursor)
	bool is_word_selecting = false;
	std::wstring selected_text;

	std::optional<Link> link_to_edit = {};
	int selected_highlight_index = -1;

	std::optional<std::pair<std::optional<std::wstring>, Link>> pending_link;

	bool dark_mode = false;
	bool mouse_drag_mode = false;
	bool synctex_mode = false;
	bool is_dragging = false;

	int main_window_width = 0;
	int main_window_height = 0;

	QWidget* text_command_line_edit_container = nullptr;
	QLabel* text_command_line_edit_label = nullptr;
	QLineEdit* text_command_line_edit = nullptr;
	QLabel* status_label = nullptr;

	bool is_render_invalidated = false;
	bool is_ui_invalidated = false;

	//std::optional<std::pair<std::wstring, int>> last_smart_fit_state = {};
	std::optional<int> last_smart_fit_page = {};

	std::wstring inverse_search_command;

	QTime last_text_select_time = QTime::currentTime();

	bool main_document_view_has_document();

protected:

	//void paintEvent(QPaintEvent* paint_event) override;
	void resizeEvent(QResizeEvent* resize_event) override;
	void mouseMoveEvent(QMouseEvent* mouse_event) override;
	void closeEvent(QCloseEvent* close_event) override;
	bool is_pending_link_source_filled();
	std::wstring get_status_string();
	void handle_escape();
	void keyPressEvent(QKeyEvent* kevent) override;
	void keyReleaseEvent(QKeyEvent* kevent) override;
	void invalidate_render();
	void invalidate_ui();
	void handle_command_with_symbol(const Command* command, char symbol);
	void handle_command_with_file_name(const Command* command, std::wstring file_name);
	bool is_waiting_for_symbol();
	void key_event(bool released, QKeyEvent* kevent);
	void handle_left_click(float x, float y, bool down);
	void handle_right_click(float x, float y, bool down);

	void update_history_state();
	void push_state();
	void next_state();
	void prev_state();
	void update_current_history_index();

	void set_main_document_view_state(DocumentViewState new_view_state);
	void handle_click(int pos_x, int pos_y);
	void mouseReleaseEvent(QMouseEvent* mevent) override;
	void mousePressEvent(QMouseEvent* mevent) override;
	void mouseDoubleClickEvent(QMouseEvent* mevent) override;

	//bool eventFilter(QObject* obj, QEvent* event) override;
	void wheelEvent(QWheelEvent* wevent) override;
	void show_textbar(const std::wstring& command_name, bool should_fill_with_selected_text = false);
	void toggle_two_window_mode();
	void handle_command(const Command* command, int num_repeats);
	void handle_link();
	void handle_pending_text_command(std::wstring text);
	void toggle_fullscreen();
	void toggle_presentation_mode();
	void complete_pending_link(const LinkViewState& destination_view_state);
	void long_jump_to_destination(int page, float offset_x, float offset_y);
	void long_jump_to_destination(int page, float offset_y);

public:

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

	~MainWidget();

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
	void toggle_dark_mode();
	void toggle_mouse_drag_mode();
	void do_synctex_forward_search(const Path& pdf_file_path,const Path& latex_file_path, int line);
	void on_new_instance_message(qint32 instance_id, QByteArray arguments);
	void handle_args(const QStringList &arguments);
	void update_link_with_opened_book_state(Link lnk, const OpenedBookState& new_state);
	void update_closest_link_with_opened_book_state(const OpenedBookState& new_state);
};
