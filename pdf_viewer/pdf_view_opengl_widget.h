#pragma once

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
#include <qpainter.h>

#include "document_view.h"
#include "path.h"



struct OpenGLSharedResources {
	GLuint vertex_buffer_object;
	GLuint uv_buffer_object;
	GLuint rendered_program;
	GLuint rendered_dark_program;
	GLuint custom_color_program;
	GLuint unrendered_program;
	GLuint highlight_program;
	GLuint vertical_line_program;
	GLuint vertical_line_dark_program;

	GLint dark_mode_contrast_uniform_location;
	GLint highlight_color_uniform_location;
	GLint line_color_uniform_location;
	GLint line_time_uniform_location;
	GLint line_freq_uniform_location;

	GLint custom_color_background_uniform_location;
	GLint custom_color_text_uniform_location;

	bool is_initialized;
};

struct OverviewState {
	int page;
	float offset_y;
	float page_height;
};

class PdfViewOpenGLWidget : public QOpenGLWidget, protected QOpenGLExtraFunctions {
public:

	enum OverviewSide {
		bottom=0,
		top=1,
		left=2,
		right=3
	};

	struct OverviewResizeData {
		fz_rect original_rect;
		std::pair<float, float> original_mouse_pos;
		OverviewSide side_index;
	};

	struct OverviewMoveData {
		std::pair<float, float> original_offsets;
		std::pair<float, float> original_mouse_pos;
	};

	enum ColorPalette {
		Normal,
		Dark,
		Custom
	};

private:
	static OpenGLSharedResources shared_gl_objects;

	bool is_opengl_initialized = false;
	GLuint vertex_array_object;
	DocumentView* document_view = nullptr;
	PdfRenderer* pdf_renderer = nullptr;
	ConfigManager* config_manager = nullptr;
	std::vector<SearchResult> search_results;
	int current_search_result_index = -1;
	std::mutex search_results_mutex;
	bool is_search_cancelled = true;
	bool is_searching;
	bool should_highlight_links = false;
	ColorPalette color_mode = ColorPalette::Normal;
	bool is_helper = false;
	float percent_done = 0.0f;
	std::optional<int> visible_page_number = {};

	bool is_dragging = false;

	int last_mouse_down_window_x = 0;
	int last_mouse_down_window_y = 0;

	float last_mouse_down_document_offset_x = 0;
	float last_mouse_down_document_offset_y = 0;

	//float vertical_line_location;
	bool should_draw_vertical_line = false;
	QDateTime creation_time;

	std::optional<std::function<void(const OpenedBookState&)>> on_link_edit = {};
	std::optional<OverviewState> overview_page = {};

	float overview_half_width = 0.8f;
	float overview_half_height = 0.4f;

	float overview_offset_x = 0.0f;
	float overview_offset_y = 0.0f;

	GLuint LoadShaders(Path vertex_file_path_, Path fragment_file_path_);
protected:

	void initializeGL() override;
	void resizeGL(int w, int h) override;
	void render_highlight_window(GLuint program, fz_rect window_rect, bool draw_border=true);
	void render_line_window(GLuint program, float vertical_pos);
	void render_highlight_absolute(GLuint program, fz_rect absolute_document_rect, bool draw_border=true);
	void render_highlight_document(GLuint program, int page, fz_rect doc_rect);
	void paintGL() override;
	void render(QPainter* painter);

public:

#ifndef NDEBUG
	// properties for visualizing selected blocks, used only for debug
	std::optional<int> last_selected_block_page = {};
	std::optional<fz_rect> last_selected_block = {};
#endif

	std::vector<fz_rect> selected_character_rects;
	std::vector<std::pair<int, fz_rect>> synctex_highlights;

	PdfViewOpenGLWidget(DocumentView* document_view, PdfRenderer* pdf_renderer, ConfigManager* config_manager, bool is_helper, QWidget* parent = nullptr);
	~PdfViewOpenGLWidget();

	//void set_vertical_line_pos(float pos);
	//float get_vertical_line_pos();
	void set_should_draw_vertical_line(bool val);
	bool get_should_draw_vertical_line();
	void handle_escape();
	void toggle_highlight_links();
	void set_highlight_links(bool should_highlight);
	int get_num_search_results();
	int get_current_search_result_index();
	bool valid_document();
	void goto_search_result(int offset);
	void render_overview(OverviewState overview);
	void render_page(int page_number);
	bool get_is_searching(float* prog);
	void search_text(const std::wstring& text, std::optional<std::pair<int, int>> range = {});
	void set_dark_mode(bool mode);
	void toggle_dark_mode();
	void set_custom_color_mode(bool mode);
	void toggle_custom_color_mode();
	void set_synctex_highlights(std::vector<std::pair<int, fz_rect>> highlights);
	void on_document_view_reset();
	void mouseMoveEvent(QMouseEvent* mouse_event) override;
	void mousePressEvent(QMouseEvent* mevent) override;
	void mouseReleaseEvent(QMouseEvent* mevent) override;
	void wheelEvent(QWheelEvent* wevent) override;
	void register_on_link_edit_listener(std::function<void(const OpenedBookState&)> listener);
	void set_overview_page(std::optional<OverviewState> overview_page);
	std::optional<OverviewState> get_overview_page();
	void draw_empty_helper_message(QPainter* painter);
	void set_visible_page_number(std::optional<int> val);
	bool is_presentation_mode();
	fz_rect	get_overview_rect();
	std::vector<fz_rect> get_overview_border_rects();
	bool is_window_point_in_overview(float window_x, float window_y);
	bool is_window_point_in_overview_border(float window_x, float window_y, OverviewSide *which_border);

	void get_overview_offsets(float* offset_x, float* offset_y);

	float get_overview_side_pos(int index);
	void set_overview_side_pos(int index, fz_rect original_rect, float diff_x, float diff_y);
	void set_overview_rect(fz_rect rect);

	void set_overview_offsets(float offset_x, float offset_y);

	void bind_program();
};
