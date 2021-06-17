#pragma once

#include <vector>
#include <string>
#include <assert.h>
#include <utility>
#include <algorithm>
#include <filesystem>

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

extern float ZOOM_INC_FACTOR;
extern const int page_paddings;

class DocumentView {
protected:

private:
	fz_context* mupdf_context;
	sqlite3* database;
	ConfigManager* config_manager;
	DocumentManager* document_manager;
	Document* current_document;

	float zoom_level;
	//float offset_x;
	//float offset_y;
	float offset_x;
	float offset_y;

	int view_width;
	int view_height;

public:
	DocumentView( fz_context* mupdf_context, sqlite3* db,  DocumentManager* document_manager, ConfigManager* config_manager);
	DocumentView( fz_context* mupdf_context, sqlite3* db,  DocumentManager* document_manager, ConfigManager* config_manager,
		wstring path, int view_width, int view_height, float offset_x, float offset_y);
	float get_zoom_level();
	DocumentViewState get_state();
	void handle_escape();
	inline void set_offsets(float new_offset_x, float new_offset_y);
	Document* get_document();
	Link* find_closest_link();
	void delete_closest_link();
	void delete_closest_bookmark();
	float get_offset_x();
	float get_offset_y();
	int get_view_height();
	int get_view_width();
	void set_null_document();
	void set_offset_x(float new_offset_x);
	void set_offset_y(float new_offset_y);
	optional<PdfLink> get_link_in_pos(int view_x, int view_y);
	void get_text_selection(fz_point selection_begin, fz_point selection_end, vector<fz_rect>& selected_characters, wstring& text_selection);
	void add_mark(char symbol);
	void add_bookmark(wstring desc);
	void on_view_size_change(int new_width, int new_height);
	void absolute_to_window_pos(float absolute_x, float absolute_y, float* window_x, float* window_y);
	fz_rect absolute_to_window_rect(fz_rect doc_rect);
	void document_to_window_pos(int page, float doc_x, float doc_y, float* window_x, float* window_y);
	fz_rect document_to_window_rect(int page, fz_rect doc_rect);
	void window_to_document_pos(float window_x, float window_y, float* doc_x, float* doc_y, int* doc_page);
	void window_to_absolute_document_pos(float window_x, float window_y, float* doc_x, float* doc_y);
	bool get_block_at_window_position(float window_x, float window_y, int* page, fz_rect* rect);
	void goto_mark(char symbol);
	void goto_end();
	float set_zoom_level(float zl);
	float zoom_in();
	float zoom_out();
	void move_absolute(float dx, float dy);
	void move(float dx, float dy);
	int get_current_page_number();
	void get_visible_pages(int window_height, vector<int>& visible_pages);
	void move_pages(int num_pages);
	void move_screens(int num_screens);
	void reset_doc_state();
	void open_document(wstring doc_path, bool load_prev_state = true);
	float get_page_offset(int page);
	void goto_offset_within_page(int page, float offset_x, float offset_y);
	void goto_page(int page);
	void fit_to_page_width();
	void persist();
	wstring get_current_chapter_name();
	optional<pair<int,int>> get_current_page_range();
	int get_current_chapter_index();
	void goto_chapter(int diff);
	float view_height_in_document_space();
};
