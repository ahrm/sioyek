#pragma once

#include <vector>
#include <string>
#include <assert.h>
#include <utility>
#include <algorithm>
#include <thread>
#include <optional>

#include <qapplication.h>
#include <qpushbutton.h>
#include <qopenglwidget.h>
#include <qopenglextrafunctions.h>
#include <qopenglfunctions.h>
#include <qopengl.h>

#include <mupdf/fitz.h>
#include "sqlite3.h"

#include "pdf_renderer.h"
#include "document.h"
#include "utils.h"
#include "config.h"
#include "ui.h"
#include "checksum.h"

extern float ZOOM_INC_FACTOR;
extern const int PAGE_PADDINGS;


class DocumentView {
protected:

private:
	fz_context* mupdf_context = nullptr;
	DatabaseManager* db_manager = nullptr;
	ConfigManager* config_manager = nullptr;
	DocumentManager* document_manager = nullptr;
	CachedChecksummer* checksummer;
	Document* current_document = nullptr;

	float zoom_level = 0;
	float offset_x = 0;
	float offset_y = 0;

	//float vertical_line_begin_pos = 0;
	std::optional<fz_rect> ruler_rect;
	float ruler_pos = 0;
	int line_index = -1;

	int view_width = 0;
	int view_height = 0;
	bool is_auto_resize_mode = true;


public:
	std::vector<fz_rect> selected_character_rects;

	DocumentView( fz_context* mupdf_context, DatabaseManager* db_manager,  DocumentManager* document_manager, ConfigManager* config_manager, CachedChecksummer* checksummer);
	DocumentView( fz_context* mupdf_context, DatabaseManager* db_manager,  DocumentManager* document_manager, ConfigManager* config_manager, CachedChecksummer* checksummer, bool* invalid_flag,
		std::wstring path, int view_width, int view_height, float offset_x, float offset_y);
	~DocumentView();
	float get_zoom_level();
	DocumentViewState get_state();
	PortalViewState get_checksum_state();
	void set_opened_book_state(const OpenedBookState& state);
	void handle_escape();
	void set_book_state(OpenedBookState state);
	void set_offsets(float new_offset_x, float new_offset_y);
	Document* get_document();
	std::optional<Portal> find_closest_portal(bool limit=false);
	std::optional<BookMark> find_closest_bookmark();
	void goto_link(Portal* link);
	void delete_closest_portal();
	void delete_closest_bookmark();
	Highlight get_highlight_with_index(int index);
	void delete_highlight_with_index(int index);
	void delete_highlight(Highlight hl);
	void delete_closest_bookmark_to_offset(float offset);
	float get_offset_x();
	float get_offset_y();
	AbsoluteDocumentPos get_offsets();
	int get_view_height();
	int get_view_width();
	void set_null_document();
	void set_offset_x(float new_offset_x);
	void set_offset_y(float new_offset_y);
	std::optional<PdfLink> get_link_in_pos(WindowPos pos);
	int get_highlight_index_in_pos(WindowPos pos);
	void get_text_selection(AbsoluteDocumentPos selection_begin, AbsoluteDocumentPos selection_end, bool is_word_selection, std::vector<fz_rect>& selected_characters, std::wstring& text_selection);
	void add_mark(char symbol);
	void add_bookmark(std::wstring desc);
	void add_highlight(AbsoluteDocumentPos selection_begin, AbsoluteDocumentPos selection_end, char type);
	void on_view_size_change(int new_width, int new_height);
	void absolute_to_window_pos(float absolute_x, float absolute_y, float* window_x, float* window_y);
	//void absolute_to_window_pos_pixels(float absolute_x, float absolute_y, float* window_x, float* window_y);
	fz_rect absolute_to_window_rect(fz_rect doc_rect);
	NormalizedWindowPos document_to_window_pos(DocumentPos pos);
	WindowPos document_to_window_pos_in_pixels(DocumentPos doc_pos);
	fz_rect document_to_window_rect(int page, fz_rect doc_rect);
	fz_irect document_to_window_irect(int page, fz_rect doc_rect);
	fz_rect document_to_window_rect_pixel_perfect(int page, fz_rect doc_rect, int pixel_width, int pixel_height);
	DocumentPos window_to_document_pos(WindowPos window_pos);
	AbsoluteDocumentPos window_to_absolute_document_pos(WindowPos window_pos);
	NormalizedWindowPos window_to_normalized_window_pos(WindowPos window_pos);
	void goto_mark(char symbol);
	void goto_end();

	void goto_left();
	void goto_left_smart();

	void goto_right();
	void goto_right_smart();

	float get_max_valid_x();
	float get_min_valid_x();

	float set_zoom_level(float zl, bool should_exit_auto_resize_mode);
	float zoom_in(float zoom_factor = ZOOM_INC_FACTOR);
	float zoom_out(float zoom_factor = ZOOM_INC_FACTOR);
	float zoom_in_cursor(WindowPos mouse_pos, float zoom_factor = ZOOM_INC_FACTOR);
	float zoom_out_cursor(WindowPos mouse_pos, float zoom_factor = ZOOM_INC_FACTOR);
	void move_absolute(float dx, float dy);
	void move(float dx, float dy);
	void get_absolute_delta_from_doc_delta(float doc_dx, float doc_dy, float* abs_dx, float* abs_dy);
	int get_center_page_number();
	void get_visible_pages(int window_height, std::vector<int>& visible_pages);
	void move_pages(int num_pages);
	void move_screens(int num_screens);
	void reset_doc_state();
	void open_document(const std::wstring& doc_path, bool* invalid_flag, bool load_prev_state = true, std::optional<OpenedBookState> prev_state = {}, bool foce_load_dimensions=false);
	float get_page_offset(int page);
	void goto_offset_within_page(DocumentPos pos);
	void goto_offset_within_page(int page, float offset_y);
	void goto_page(int page);
	void fit_to_page_width(bool smart=false, bool ratio=false);
	void fit_to_page_height(bool smart=false);
	void fit_to_page_height_width_minimum();
	void persist();
	std::wstring get_current_chapter_name();
	std::optional<std::pair<int,int>> get_current_page_range();
	int get_current_chapter_index();
	void goto_chapter(int diff);
	void get_page_chapter_index(int page, std::vector<TocNode*> toc_nodes, std::vector<int>& res);
	std::vector<int> get_current_chapter_recursive_index();
	float view_height_in_document_space();
	void set_vertical_line_pos(float pos);
	void set_vertical_line_rect(fz_rect rect);
	bool has_ruler_rect();
	std::optional<fz_rect> get_ruler_rect();
	//float get_vertical_line_pos();
	float get_ruler_pos();

	//float get_vertical_line_window_y();
	float get_ruler_window_y();
	std::optional<fz_rect> get_ruler_window_rect();

	void goto_vertical_line_pos();
	int get_page_offset();
	void set_page_offset(int new_offset);
	void rotate();
	void goto_top_of_page();
	void goto_bottom_of_page();
	int get_line_index_of_vertical_pos();
	int get_line_index_of_pos(DocumentPos docpos);
	int get_line_index();
	void set_line_index(int index);
	int get_vertical_line_page();
	bool goto_definition();
	std::vector<DocumentPos> find_line_definitions();
	std::optional<std::wstring> get_selected_line_text();
	bool get_is_auto_resize_mode();
	void disable_auto_resize_mode();
	void readjust_to_screen();
	float get_half_screen_offset();
	void scroll_mid_to_top();
	void get_visible_links(std::vector<std::pair<int, fz_link*>>& visible_page_links);

	std::vector<fz_rect>* get_selected_character_rects();
};
