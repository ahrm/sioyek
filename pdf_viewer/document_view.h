#pragma once

#include <vector>
#include <string>
#include <assert.h>
#include <utility>
#include <algorithm>
#include <filesystem>

#include <mupdf/fitz.h>
#include "sqlite3.h"

#include "pdf_renderer.h"
#include "document.h"
#include "utils.h"

extern const float ZOOM_INC_FACTOR;
extern const int page_paddings;

class DocumentView {
	fz_context* mupdf_context;
	sqlite3* database;
	PdfRenderer* pdf_renderer;
	DocumentManager* document_manager;

	Document* current_document;

	float zoom_level;
	float offset_x;
	float offset_y;

	bool render_is_invalid;

	int view_width;
	int view_height;

	mutex search_results_mutex;
	float percent_done;
	bool is_searching;
	vector<SearchResult> search_results;
	int current_search_result_index;

public:
	DocumentView(fz_context* mupdf_context, sqlite3* db, PdfRenderer* pdf_renderer, DocumentManager* document_manager);
	DocumentView(fz_context* mupdf_context, sqlite3* db, PdfRenderer* pdf_renderer, DocumentManager* document_manager,
		string path, int view_width, int view_height, float offset_x, float offset_y);
	float get_zoom_level();
	DocumentViewState get_state();
	void handle_escape();
	inline void set_offsets(float new_offset_x, float new_offset_y);
	Document* get_document();
	int get_num_search_results();
	int get_current_search_result_index();
	Link* find_closest_link();
	void delete_closest_link();
	void delete_closest_bookmark();
	float get_offset_x();
	float get_offset_y();
	void set_offset_x(float new_offset_x);
	void set_offset_y(float new_offset_y);
	void render_highlight_window(GLuint program, fz_rect window_rect);
	void render_highlight_absolute(GLuint program, fz_rect absolute_document_rect);
	void render_highlight_document(GLuint program, int page, fz_rect doc_rect);
	optional<PdfLink> get_link_in_pos(int view_x, int view_y);
	void get_text_selection(fz_point selection_begin, fz_point selection_end, vector<fz_rect>& selected_characters, string& text_selection);
	void add_mark(char symbol);
	void add_bookmark(string desc);
	void on_view_size_change(int new_width, int new_height);
	bool should_rerender();
	bool get_is_searching(float* prog);


	void absolute_to_window_pos(float absolute_x, float absolute_y, float* window_x, float* window_y);
	fz_rect absolute_to_window_rect(fz_rect doc_rect);
	void document_to_window_pos(int page, float doc_x, float doc_y, float* window_x, float* window_y);
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
	int get_current_page_number();
	void search_text(const char* text);
	int search_text2(const char* text);
	void goto_search_result(int offset);
	void get_visible_pages(int window_height, vector<int>& visible_pages);
	void move_pages(int num_pages);
	void reset_doc_state();
	void open_document(string doc_path);
	float get_page_offset(int page);
	void goto_offset_within_page(int page, float offset_x, float offset_y);
	void goto_page(int page);
	void render_page(int page_number, GLuint rendered_program, GLuint unrendered_program);
	void render(GLuint rendered_program, GLuint unrendered_program, GLuint highlight_program);
	void persist();
};
