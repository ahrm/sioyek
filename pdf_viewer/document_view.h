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

	float vertical_line_pos = 0;

	int view_width = 0;
	int view_height = 0;

public:
	DocumentView( fz_context* mupdf_context, DatabaseManager* db_manager,  DocumentManager* document_manager, ConfigManager* config_manager, CachedChecksummer* checksummer);
	DocumentView( fz_context* mupdf_context, DatabaseManager* db_manager,  DocumentManager* document_manager, ConfigManager* config_manager, CachedChecksummer* checksummer, bool* invalid_flag,
		std::wstring path, int view_width, int view_height, float offset_x, float offset_y);
	~DocumentView();
	float get_zoom_level();
	DocumentViewState get_state();
	LinkViewState get_checksum_state();
	void set_opened_book_state(const OpenedBookState& state);
	void handle_escape();
	void set_book_state(OpenedBookState state);
	void set_offsets(float new_offset_x, float new_offset_y);
	Document* get_document();
	std::optional<Link> find_closest_link();
	void goto_link(Link* link);
	void delete_closest_link();
	void delete_closest_bookmark();
	void delete_highlight_with_index(int index);
	void delete_highlight_with_offsets(float begin_x, float begin_y, float end_x, float end_y);
	void delete_closest_bookmark_to_offset(float offset);
	float get_offset_x();
	float get_offset_y();
	int get_view_height();
	int get_view_width();
	void set_null_document();
	void set_offset_x(float new_offset_x);
	void set_offset_y(float new_offset_y);
	std::optional<PdfLink> get_link_in_pos(int view_x, int view_y);
	int get_highlight_index_in_pos(int view_x, int view_y);
	void get_text_selection(fz_point selection_begin, fz_point selection_end, bool is_word_selection, std::vector<fz_rect>& selected_characters, std::wstring& text_selection);
	void add_mark(char symbol);
	void add_bookmark(std::wstring desc);
	void add_highlight(fz_point selection_begin, fz_point selection_end, char type);
	void on_view_size_change(int new_width, int new_height);
	void absolute_to_window_pos(float absolute_x, float absolute_y, float* window_x, float* window_y);
	//void absolute_to_window_pos_pixels(float absolute_x, float absolute_y, float* window_x, float* window_y);
	fz_rect absolute_to_window_rect(fz_rect doc_rect);
	void document_to_window_pos(int page, float doc_x, float doc_y, float* window_x, float* window_y);
	void document_to_window_pos_in_pixels(int page, float doc_x, float doc_y, int* window_x, int* window_y);
	fz_rect document_to_window_rect(int page, fz_rect doc_rect);
	void window_to_document_pos(float window_x, float window_y, float* doc_x, float* doc_y, int* doc_page);
	void window_to_absolute_document_pos(float window_x, float window_y, float* doc_x, float* doc_y);
	void goto_mark(char symbol);
	void goto_end();
	float set_zoom_level(float zl);
	float zoom_in();
	float zoom_out();
	void move_absolute(float dx, float dy);
	void move(float dx, float dy);
	void get_absolute_delta_from_doc_delta(float doc_dx, float doc_dy, float* abs_dx, float* abs_dy);
	int get_current_page_number();
	void get_visible_pages(int window_height, std::vector<int>& visible_pages);
	void move_pages(int num_pages);
	void move_screens(int num_screens);
	void reset_doc_state();
	void open_document(const std::wstring& doc_path, bool* invalid_flag, bool load_prev_state = true, std::optional<OpenedBookState> prev_state = {}, bool foce_load_dimensions=false);
	float get_page_offset(int page);
	void goto_offset_within_page(int page, float offset_x, float offset_y);
	void goto_offset_within_page(int page, float offset_y);
	void goto_page(int page);
	void fit_to_page_width(bool smart=false);
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
	float get_vertical_line_pos();
	float get_vertical_line_window_y();
	void goto_vertical_line_pos();
};
