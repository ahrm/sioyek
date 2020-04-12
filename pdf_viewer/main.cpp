//todo: cleanup the code
//todo: visibility test is still buggy??
//todo: threading
//todo: remove some O(n) things from page height computations
//todo: add fuzzy search
//todo: improve speed and code of document change (cache documents?)
//todo: copy
//todo: handle document memory leak (because documents are not deleted since adding state history)
//todo: tests!
//todo: handle mouse in menues
//todo: stop creating DocumentViews!

#include "imgui.h"
#include "imgui_impl_sdl.h"
#include "imgui_impl_opengl3.h"

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

#include <SDL.h>
#include <gl/glew.h>
#include <SDL_opengl.h>
#include <gl/GLU.h>

#include <Windows.h>
#include <mupdf/fitz.h>
#include "sqlite3.h"
#include <filesystem>

#include "input.h"
#include "database.h"
#include "book.h"
#include "utils.h"
#include "ui.h"
#include "pdf_renderer.h"

#define FTS_FUZZY_MATCH_IMPLEMENTATION
#include "fts_fuzzy_match.h"
#undef FTS_FUZZY_MATCH_IMPLEMENTATION


extern const float ZOOM_INC_FACTOR = 1.2f;
extern const unsigned int cache_invalid_milies = 1000;
extern const int page_paddings = 0;
extern const int max_pending_requests = 31;
extern const char* last_path_file_absolute_location = "C:\\Users\\Lion\\source\\repos\\pdf_viewer\\pdf_viewer\\last_document_path.txt";


using namespace std;

mutex mupdf_mutexes[FZ_LOCK_MAX];

void lock_mutex(void* user, int lock) {
	mutex* mut = (mutex*)user;
	(mut + lock)->lock();
}

void unlock_mutex(void* user, int lock) {
	mutex* mut = (mutex*)user;
	(mut + lock)->unlock();
}

GLfloat g_quad_vertex[] = {
	-1.0f, -1.0f,
	1.0f, -1.0f,
	-1.0f, 1.0f,
	1.0f, 1.0f
};

GLfloat g_quad_uvs[] = {
	0.0f, 0.0f,
	1.0f, 0.0f,
	0.0f, 1.0f,
	1.0f, 1.0f
};


void window_to_uv_scale(int window_width, int window_height, int document_width, int document_height, float* uv_x, float* uv_y) {
	*uv_x = ((float)(window_width)) / document_width;
	*uv_y = ((float)(window_height)) / document_height;
}


PdfRenderer* global_pdf_renderer;


class Document {
private:
	vector<Mark> marks;
	vector<BookMark> bookmarks;
	vector<Link> links;
	sqlite3* db;
	vector<TocNode*> top_level_toc_nodes;

	int get_mark_index(char symbol) {
		for (int i = 0; i < marks.size(); i++) {
			if (marks[i].symbol == symbol) {
				return i;
			}
		}
		return -1;
	}

public:
	fz_context* context;
	string file_name;
	fz_document* doc;
	vector<fz_rect> page_rects;

	void add_bookmark(string desc, float y_offset) {
		BookMark res;
		res.description = desc;
		res.y_offset = y_offset;
		bookmarks.push_back(res);
		insert_bookmark(db, file_name, desc, y_offset);
	}

	string get_path() {
		return file_name;
	}


	void add_link(Document* dst_document, float dst_y_offset, float dst_x_offset, float src_y_offset) {
		Link link;
		link.document_path = dst_document->get_path();
		link.dest_offset_x = dst_x_offset;
		link.dest_offset_y = dst_y_offset;
		link.src_offset_y = src_y_offset;
		add_link(link);
	}
	void add_link(Link link, bool insert_into_database=true) {
		links.push_back(link);
		if (insert_into_database) {
			insert_link(db, get_path(), link.document_path, link.dest_offset_x, link.dest_offset_y, link.src_offset_y);
		}
	}


	BookMark* find_closest_bookmark(float to_offset_y, int* index=nullptr) {
		float min_diff = 1000000.0f;
		int min_index = -1;
		if (index != nullptr) {
			*index = min_index;
		}

		for (int i = 0; i < bookmarks.size(); i++) {
			float diff = abs(bookmarks[i].y_offset - to_offset_y);
			if (diff < min_diff) {
				min_diff = diff;
				min_index = i;
			}
		}
		if (min_index >= 0) {
			if (index != nullptr) {
				*index = min_index;
			}
			return &bookmarks[min_index];
		}
		return nullptr;
	}

	void delete_closest_bookmark(float to_y_offset) {
		int closest_index = -1;
		find_closest_bookmark(to_y_offset, &closest_index);
		if (closest_index != -1) {
			delete_bookmark(db, get_path(), bookmarks[closest_index].y_offset);
			bookmarks.erase(bookmarks.begin() + closest_index);
		}
	}


	Link* find_closest_link(float to_offset_y, int* index=nullptr) {
		float min_diff = 1000000.0f;
		int min_index = -1;
		if (index != nullptr) {
			*index = min_index;
		}

		for (int i = 0; i < links.size(); i++) {
			float diff = abs(links[i].src_offset_y - to_offset_y);
			if (diff < min_diff) {
				min_diff = diff;
				min_index = i;
			}
		}
		if (min_index >= 0) {
			if (index != nullptr) {
				*index = min_index;
			}
			return &links[min_index];
		}
		return nullptr;
	}

	void delete_closest_link(float to_offset_y) {
		int closest_index = -1;
		find_closest_link(to_offset_y, &closest_index);
		if (closest_index != -1) {
			delete_link(db, get_path(), links[closest_index].src_offset_y);
			links.erase(links.begin() + closest_index);
		}

	}

	const vector<BookMark>& get_bookmarks() const {
		return bookmarks;
	}

	fz_link* get_page_links(int page_number) {
		fz_link* res = nullptr;
		fz_try(context) {
			fz_page* page = fz_load_page(context, doc, page_number);
			res = fz_load_links(context, page);
			fz_drop_page(context, page);
		}
		fz_catch(context) {
			cout << "Error: Could not load links" << endl;
			res = nullptr;
		}
		return res;
	}

	void add_mark(char symbol, float y_offset) {
		int current_mark_index = get_mark_index(symbol);
		if (current_mark_index == -1) {
			marks.push_back({ y_offset, symbol });
			insert_mark(db, file_name, symbol, y_offset);
		}
		else {
			marks[current_mark_index].y_offset = y_offset;
			update_mark(db, file_name, symbol, y_offset);
		}
	}

	void create_toc_tree(vector<TocNode*>& toc) {
		fz_try(context) {
			fz_outline* outline = fz_load_outline(context, doc);
			convert_toc_tree(outline, toc);

		}
		fz_catch(context) {
			cout << "Error: Could not load outline ... " << endl;
		}
	}

	void load_marks_from_database() {
		marks.clear();
		bookmarks.clear();
		select_mark(db, file_name, marks);
		select_bookmark(db, file_name, bookmarks);
		select_links(db, file_name, links);
	}

	bool get_mark_location_if_exists(char symbol, float* y_offset) {
		int mark_index = get_mark_index(symbol);
		if (mark_index == -1) {
			return false;
		}
		*y_offset = marks[mark_index].y_offset;
	}

	Document(fz_context* context, string file_name, sqlite3* db) : context(context), file_name(file_name), doc(nullptr), db(db) {
	}


	~Document() {
		if (doc != nullptr) {
			fz_try(context) {
				fz_drop_document(context, doc);
			}
			fz_catch(context) {
				cout << "Error: could not drop documnet" << endl;
			}
		}
	}

	const vector<TocNode*>& get_toc() {
		return top_level_toc_nodes;
	}

	bool open() {
		if (doc == nullptr) {
			fz_try(context) {
				doc = fz_open_document(context, file_name.c_str());
			}
			fz_catch(context) {
				cout << "could not open " << file_name << endl;
			}
			if (doc != nullptr) {

				load_marks_from_database();
				create_toc_tree(top_level_toc_nodes);

				return true;
			}


			return false;
		}
		else {
			cout << "warning! calling open() on an open document" << endl;
			return false;
		}
	}

	int num_pages() {
		int pages = -1;
		fz_try(context) {
			pages = fz_count_pages(context, doc);
		}
		fz_catch(context) {
			cout << "could not count pages" << endl;
		}
		return pages;
	}

	fz_pixmap* get_page_pixmap(int page, float zoom_level = 1.0f) {
		fz_pixmap* res = nullptr;
		fz_try(context) {
			fz_matrix transform_matrix = fz_pre_scale(fz_identity, zoom_level, zoom_level);
			res = fz_new_pixmap_from_page_number(context, doc, page, transform_matrix, fz_device_rgb(context), 0);
		}
		fz_catch(context) {
			cout << "could not render pixmap for page " << page << endl;
		}
		return res;
	}
};


class DocumentView {
	fz_context* mupdf_context;
	sqlite3* database;

	Document* current_document;
	string document_path;

	vector<float> page_heights;
	vector<float> page_widths;

	float zoom_level;
	float offset_x;
	float offset_y;

	bool render_is_invalid;

	int view_width;
	int view_height;

	vector<SearchResult> search_results;
	int current_search_result_index;

public:
	DocumentView(fz_context* mupdf_context, sqlite3* db) : 
		mupdf_context(mupdf_context),
		database(db)
	{

	}

	float get_zoom_level() {
		return zoom_level;
	}

	DocumentViewState get_state() {
		DocumentViewState res;
		res.document_view = this;
		res.offset_x = get_offset_x();
		res.offset_y = get_offset_y();
		res.zoom_level = get_zoom_level();
		return res;
	}

	void handle_escape() {
		search_results.clear();
		current_search_result_index = 0;
	}
	inline void set_offsets(float new_offset_x, float new_offset_y) {
		offset_x = new_offset_x;
		offset_y = new_offset_y;
		render_is_invalid = true;
	}
	
	Document* get_document() {
		return current_document;
	}

	int get_num_search_results() {
		return search_results.size();
	}

	int get_current_search_result_index() {
		return current_search_result_index;
	}


	Link* find_closest_link() {
		if (current_document) {
			return current_document->find_closest_link(offset_y);
		}
		return nullptr;
	}

	void delete_closest_link() {
		current_document->delete_closest_link(offset_y);
	}

	void delete_closest_bookmark() {
		current_document->delete_closest_bookmark(offset_y);
	}

	float get_offset_x() {
		return offset_x;
	}

	float get_offset_y() {
		return offset_y;
	}

	inline void set_offset_x(float new_offset_x) {
		set_offsets(new_offset_x, offset_y);
	}

	void render_highlight_absolute(GLuint program, fz_rect absolute_document_rect) {
		fz_rect window_rect = document_to_window_rect_absolute(absolute_document_rect);
		float quad_vertex_data[] = {
			window_rect.x0, window_rect.y1,
			window_rect.x1, window_rect.y1,
			window_rect.x0, window_rect.y0,
			window_rect.x1, window_rect.y0
		};
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glUseProgram(program);
		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);
		glBufferData(GL_ARRAY_BUFFER, sizeof(quad_vertex_data), quad_vertex_data, GL_DYNAMIC_DRAW);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		glDisable(GL_BLEND);
	}

	optional<PdfLink> get_link_in_pos(int view_x, int view_y) {
		float doc_x, doc_y;
		int page;
		window_to_document_pos(view_x, view_y, &doc_x, &doc_y, &page);
		fz_link* links = current_document->get_page_links(page);
		fz_link* links_root = links;

		fz_point point;
		point.x = doc_x;
		point.y = doc_y;

		PdfLink res;
		bool found = false;
		while (links != nullptr) {
			if (fz_is_point_inside_rect(point, links->rect)) {
				res = { links->rect, links->uri };
				found = true;
				break;
			}
			links = links->next;
		}

		fz_try(mupdf_context) {
			//todo: maybe we should loop and drop links?
			fz_drop_link(mupdf_context, links_root);
		}
		fz_catch(mupdf_context) {
			cout << "Error: Could not drop link" << endl;
		}
		if (found) return res;
		return {};
	}


	inline void set_offset_y(float new_offset_y) {
		set_offsets(offset_x, new_offset_y);
	}

	void get_text_selection(fz_rect document_space_rect, vector<fz_rect> &selected_characters) {
		float bottom_y, top_y, left_x, right_x;
		int page1, page2;
		absolute_to_page_pos(document_space_rect.x0, document_space_rect.y0, &left_x, &bottom_y, &page1);
		absolute_to_page_pos(document_space_rect.x1, document_space_rect.y1, &right_x, &top_y, &page2);
		fz_rect page_rect;

		page_rect.x0 = left_x;
		page_rect.x1 = right_x;
		page_rect.y0 = bottom_y;
		page_rect.y1 = top_y;

		fz_page* page = nullptr;
		fz_stext_page* stext_page = nullptr;

		string selected_text;
		for (int i = page1; i <= page2; i++) {
			fz_try(mupdf_context) {
				page = fz_load_page(mupdf_context, current_document->doc, i);
				stext_page = fz_new_stext_page_from_page(mupdf_context, page, nullptr);
			}
			fz_catch(mupdf_context) {
				cout << "Error: could not load page for selection" << endl;
			}


			fz_stext_block* current_block = stext_page->first_block;
			while (current_block) {

				if (current_block->type != FZ_STEXT_BLOCK_TEXT) {
					continue;
				}

				fz_stext_line* current_line = current_block->u.t.first_line;
				while (current_line) {
					fz_stext_char* current_char = current_line->first_char;
					while (current_char) {
						fz_rect charrect = page_rect_to_absolute_rect(i, fz_rect_from_quad(current_char->quad));
						//if (includes_rect(document_space_rect, charrect)) {
						if ((document_space_rect.y0 <= charrect.y0 && document_space_rect.y1 >= charrect.y1) || 
							(document_space_rect.y1 <= charrect.y1 && document_space_rect.y1 >= charrect.y0 && document_space_rect.x1 >= charrect.x1) || 
							(document_space_rect.y0 <= charrect.y1 && document_space_rect.y0 >= charrect.y0 && document_space_rect.x0 <= charrect.x0)
							) {
							selected_text.push_back(current_char->c);
							selected_characters.push_back(charrect);
						}
						current_char = current_char->next;
					}
					//some_text.push_back('\n');
					current_line = current_line->next;
				}

				current_block = current_block->next;
			}

			fz_drop_stext_page(mupdf_context, stext_page);
			fz_drop_page(mupdf_context, page);
		}

		cout << "selecting pages " << selected_text << page1 << " " << page2 << endl;
	}

	void add_mark(char symbol) {
		assert(current_document);
		current_document->add_mark(symbol, offset_y);
	}

	void add_bookmark(string desc) {
		assert(current_document);
		current_document->add_bookmark(desc, offset_y);
	}

	void get_relative_rect(int page_width, int page_height, float output_vertices[8], int additional_offset) {
		float x0 = 2 * static_cast<float>(offset_x * zoom_level - page_width / 2) / view_width;
		float x1 = 2 * static_cast<float>(offset_x * zoom_level + page_width / 2) / view_width;
		float y0 = 2 * static_cast<float>(offset_y * zoom_level + additional_offset) / view_height;
		float y1 = 2 * static_cast<float>(offset_y * zoom_level + additional_offset - page_height) / view_height;

		output_vertices[0] = x0;
		output_vertices[1] = y0;

		output_vertices[2] = x1;
		output_vertices[3] = y0;

		output_vertices[4] = x0;
		output_vertices[5] = y1;

		output_vertices[6] = x1;
		output_vertices[7] = y1;
	}

	void on_view_size_change(int new_width, int new_height) {
		view_width = new_width;
		view_height = new_height;
		render_is_invalid = true;
	}

	bool should_rerender() {
		return render_is_invalid;
	}

	fz_rect document_to_window_rect_absolute(fz_rect doc_rect) {


		float half_width = static_cast<float>(view_width) / zoom_level / 2;
		float half_height = static_cast<float>(view_height) / zoom_level / 2;

		fz_rect transformed_doc_rect;
		transformed_doc_rect.x0 = doc_rect.x0 + offset_x;
		transformed_doc_rect.x1 = doc_rect.x1 + offset_x;

		transformed_doc_rect.y0 = -doc_rect.y0 + offset_y;
		transformed_doc_rect.y1 = -doc_rect.y1 + offset_y;

		transformed_doc_rect.x0 /= half_width;
		transformed_doc_rect.x1 /= half_width;

		transformed_doc_rect.y0 /= half_height;
		transformed_doc_rect.y1 /= half_height;

		return transformed_doc_rect;
	}

	fz_rect document_to_window_rect(int page, fz_rect doc_rect) {

		double doc_rect_y_offset = 0.0f;
		for (int i = 0; i < page; i++) {
			doc_rect_y_offset -= page_heights[i];
		}

		doc_rect.y0 = page_heights[page] - doc_rect.y0;
		doc_rect.y1 = page_heights[page] - doc_rect.y1;

		doc_rect.y0 += doc_rect_y_offset;
		doc_rect.y1 += doc_rect_y_offset;

		float half_width = static_cast<float>(view_width) / zoom_level / 2;
		float half_height = static_cast<float>(view_height) / zoom_level / 2;

		fz_rect transformed_doc_rect;
		transformed_doc_rect.x0 = doc_rect.x0 + offset_x - page_widths[page] / 2;
		transformed_doc_rect.x1 = doc_rect.x1 + offset_x - page_widths[page] / 2;

		transformed_doc_rect.y0 = doc_rect.y0 + offset_y - page_heights[page];
		transformed_doc_rect.y1 = doc_rect.y1 + offset_y - page_heights[page];

		transformed_doc_rect.x0 /= half_width;
		transformed_doc_rect.x1 /= half_width;

		transformed_doc_rect.y0 /= half_height;
		transformed_doc_rect.y1 /= half_height;

		return transformed_doc_rect;
	}

	void absolute_to_page_pos(float absolute_x, float absolute_y, float* doc_x, float* doc_y, int* doc_page) {
		// bottom being the highest widnow_y
		float test_y = absolute_y;
		float remaining_y = test_y;
		int page = 0;
		int i = 0;
		for (i = 0; i < page_heights.size(); i++) {
			if ((remaining_y - page_heights[i]) < 0) {
				break;
			}
			remaining_y -= page_heights[i];
		}

		float page_width = page_widths[i];
		float document_x = page_width / 2 + absolute_x;
		//float document_y = page_heights[i] - remaining_y;
		float document_y = remaining_y;

		*doc_x = document_x;
		*doc_y = document_y;
		*doc_page = i;
	}

	void window_to_document_pos(float window_x, float window_y, float* doc_x, float* doc_y, int* doc_page) {
		absolute_to_page_pos(
			(window_x - view_width/2) / zoom_level + offset_x,
			(window_y - view_height/2) / zoom_level + offset_y, doc_x, doc_y, doc_page);
	}

	void window_to_absolute_document_pos(float window_x, float window_y, float* doc_x, float* doc_y) {
		*doc_x = (window_x - view_width / 2) / zoom_level + offset_x;
		*doc_y = (window_y - view_height / 2) / zoom_level + offset_y;
	}

	void page_pos_to_absolute_pos(int page, float page_x, float page_y, float* abs_x, float* abs_y) {
		for (int i = 0; i < page; i++) {
			page_y += page_heights[i];
		}
		*abs_x = page_x - page_widths[page]/2;
		*abs_y = page_y;
	}

	fz_rect page_rect_to_absolute_rect(int page, fz_rect page_rect) {
		float new_x0, new_x1, new_y0, new_y1;
		page_pos_to_absolute_pos(page, page_rect.x0, page_rect.y0, &new_x0, &new_y0);
		page_pos_to_absolute_pos(page, page_rect.x1, page_rect.y1, &new_x1, &new_y1);
		fz_rect res;
		res.x0 = new_x0;
		res.x1 = new_x1;
		res.y0 = new_y0;
		res.y1 = new_y1;
		return res;
	}


	//void window_to_document_pos(float window_x, float window_y, float* doc_x, float* doc_y, int* doc_page) {
	//	// bottom being the highest widnow_y
	//	float test_y = (window_y - view_height / 2) / zoom_level + offset_y;
	//	float remaining_y = test_y;
	//	int page = 0;
	//	int i = 0;
	//	for (i = 0; i < page_heights.size(); i++) {
	//		if ((remaining_y - page_heights[i]) < 0) {
	//			break;
	//		}
	//		remaining_y -= page_heights[i];
	//	}

	//	float page_width = page_widths[i];
	//	float document_x = page_width / 2 + offset_x + (window_x - view_width / 2) / zoom_level;
	//	//float document_y = page_heights[i] - remaining_y;
	//	float document_y = remaining_y;

	//	*doc_x = document_x;
	//	*doc_y = document_y;
	//	*doc_page = i;
	//}

	float get_page_additional_offset(int page_number) {
		double offset = 0;
		for (int i = 0; i < page_number; i++) {
			float real_page_height = (page_heights[i] * zoom_level);
			offset -= real_page_height;
			offset -= page_paddings;
		}
		return offset;
	}

	void goto_mark(char symbol) {
		assert(current_document);
		float new_y_offset = 0.0f;
		if (current_document->get_mark_location_if_exists(symbol, &new_y_offset)) {
			set_offset_y(new_y_offset);
		}
	}

	void goto_end() {
		float cum_offset_y = 0.0f;
		for (auto height : page_heights) {
			cum_offset_y += height;
		}

		set_offset_y(cum_offset_y);
	}

	float set_zoom_level(float zl) {
		zoom_level = zl;
		render_is_invalid = true;
		return zoom_level;
	}

	float zoom_in() {
		return set_zoom_level(zoom_level * ZOOM_INC_FACTOR);
	}

	float zoom_out() {
		return set_zoom_level(zoom_level / ZOOM_INC_FACTOR);
	}

	void move_absolute(float dx, float dy) {
		set_offsets(offset_x + dx, offset_y + dy);
	}

	void move(float dx, float dy) {
		int abs_dx = (dx / zoom_level);
		int abs_dy = (dy / zoom_level);
		move_absolute(abs_dx, abs_dy);
	}

	int get_current_page_number() {
		vector<int> visible_pages;
		int window_height, window_width;
		get_visible_pages(100, visible_pages);
		if (visible_pages.size() > 0) {
			return visible_pages[0];
		}
		return -1;
	}

	int search_text(const char* text) {
		search_results.clear();

		int num_pages = current_document->num_pages();
		int total_results = 0;

		for (int i = 0; i < num_pages; i++) {
			fz_page* page = fz_load_page(mupdf_context, current_document->doc, i);

			const int max_hits_per_page = 20;
			fz_quad hitboxes[max_hits_per_page];
			int num_results = fz_search_page(mupdf_context, page, text, hitboxes, max_hits_per_page);

			for (int j = 0; j < num_results; j++) {
				search_results.push_back(SearchResult{ hitboxes[j], i });
			}

			total_results += num_results;

			fz_drop_page(mupdf_context, page);
		}
		return total_results;
	}

	void goto_search_result(int offset) {
		if (search_results.size() == 0) {
			return;
		}

		int target_index = mod(current_search_result_index + offset, search_results.size());
		current_search_result_index = target_index;

		int target_page = search_results[target_index].page;

		fz_quad quad = search_results[target_index].quad;

		float new_offset_y = quad.ll.y;
		for (int i = 0; i < target_page; i++) {
			new_offset_y += page_heights[i] + page_paddings;
		}

		set_offset_y(new_offset_y);
	}

	void get_visible_pages(int window_height, vector<int>& visible_pages) {
		float window_y_range_begin = offset_y - window_height / (zoom_level);
		float window_y_range_end = offset_y + window_height / (zoom_level);
		window_y_range_begin -= 1;
		window_y_range_end += 1;

		float page_begin = 0.0f;

		for (int i = 0; i < page_heights.size(); i++) {
			float page_end = page_begin + page_heights[i];

			if (intersects(window_y_range_begin, window_y_range_end, page_begin, page_end)) {
				visible_pages.push_back(i);
			}
			page_begin = page_end;
		}
	}

	void move_pages(int num_pages) {
		int current_page = get_current_page_number();
		if (current_page == -1) {
			current_page = 0;
		}
		move_absolute(0, num_pages * (page_heights[current_page] + page_paddings));
	}

	void reset_doc_state() {
		zoom_level = 1.0f;
		set_offsets(0.0f, 0.0f);
	}

	void open_document(string doc_path) {

		string cannonical_path = std::filesystem::canonical(doc_path).string();
		document_path = cannonical_path;
		vector<OpenedBookState> prev_state;

		if (select_opened_book(database, cannonical_path, prev_state)) {
			if (prev_state.size() > 1) {
				cout << "more than one file with one path, this should not happen!" << endl;
			}
		}



		if (current_document != nullptr) {
			//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
			//todo: not doing this is a memory leak
			//delete current_document;
		}


		current_document = new Document(mupdf_context, doc_path, database);
		if (!current_document->open()) {
			//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
			//todo: not doing this is a memory leak
			//delete current_document;
			current_document = nullptr;
		}
		reset_doc_state();
		if (prev_state.size() > 0) {
			OpenedBookState previous_state = prev_state[0];
			zoom_level = previous_state.zoom_level;
			offset_x = previous_state.offset_x;
			offset_y = previous_state.offset_y;
			set_offsets(previous_state.offset_x, previous_state.offset_y);
		}

		page_heights.clear();
		page_widths.clear();

		if (current_document) {

			int page_count = current_document->num_pages();

			//todo: better (or any!) error handling
			for (int i = 0; i < page_count; i++) {
				fz_page* page = fz_load_page(mupdf_context, current_document->doc, i);
				fz_rect page_rect = fz_bound_page(mupdf_context, page);
				page_heights.push_back(page_rect.y1 - page_rect.y0);
				page_widths.push_back(page_rect.x1 - page_rect.x0);
				fz_drop_page(mupdf_context, page);
			}

		}

		//int window_width, window_height;
		//SDL_GetWindowSize(main_window, &window_width, &window_height);
		//offset_x = window_width / (2*zoom_level);
		//offset_y = window_height / zoom_level;

	}

	float get_page_offset(int page) {

		int max_page = current_document->num_pages();
		if (page > max_page) {
			page = max_page;
		}
		float new_offset_y = 0.0f;

		for (int i = 0; i < page; i++) {

			new_offset_y += page_heights[i];
		}
		return new_offset_y;
	}

	void goto_offset_within_page(int page, float offset_x, float offset_y) {
		set_offsets(offset_x, get_page_offset(page-1) + offset_y);
	}

	void goto_page(int page) {
		set_offset_y(get_page_offset(page));
	}

	void render_page(int page_number, GLuint rendered_program, GLuint unrendered_program) {

		int page_width, page_height;
		GLuint texture = global_pdf_renderer->find_rendered_page(document_path, page_number, zoom_level, &page_width, &page_height);
		if (texture == 0) {
			page_width = static_cast<int>(page_widths[page_number] * zoom_level);
			page_height = static_cast<int>(page_heights[page_number] * zoom_level);
		}

		int additional_offset = get_page_additional_offset(page_number);
		float page_vertices[4 * 2];
		get_relative_rect(page_width, page_height, page_vertices, additional_offset);

		if (texture != 0) {
			glUseProgram(rendered_program);
			glEnableVertexAttribArray(0);
			glEnableVertexAttribArray(1);
			//glBindVertexArray(vertex_array_object);
			//glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer_object);
			glBufferData(GL_ARRAY_BUFFER, sizeof(page_vertices), page_vertices, GL_DYNAMIC_DRAW);

			glBindTexture(GL_TEXTURE_2D, texture);

			glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		}
		else {
			glUseProgram(unrendered_program);
			glEnableVertexAttribArray(0);
			glEnableVertexAttribArray(1);
			//glBindVertexArray(vertex_array_object);
			//glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer_object);
			glBufferData(GL_ARRAY_BUFFER, sizeof(page_vertices), page_vertices, GL_DYNAMIC_DRAW);
			glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		}
	}

	void render(GLuint rendered_program, GLuint unrendered_program, GLuint highlight_program) {

		vector<int> visible_pages;
		get_visible_pages(view_height, visible_pages);
		render_is_invalid = false;

		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);


		for (int j = 0; j < visible_pages.size(); j++) {
			render_page(visible_pages[j], rendered_program, unrendered_program);
			fz_link* links = current_document->get_page_links(visible_pages[j]);
			while (links != nullptr) {
				fz_rect window_rect = document_to_window_rect(visible_pages[j], links->rect);
				float quad_vertex_data[] = {
					window_rect.x0, window_rect.y1,
					window_rect.x1, window_rect.y1,
					window_rect.x0, window_rect.y0,
					window_rect.x1, window_rect.y0
				};
				glEnable(GL_BLEND);
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
				glUseProgram(highlight_program);
				glEnableVertexAttribArray(0);
				glEnableVertexAttribArray(1);
				glBufferData(GL_ARRAY_BUFFER, sizeof(quad_vertex_data), quad_vertex_data, GL_DYNAMIC_DRAW);
				glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
				glDisable(GL_BLEND);

				links = links->next;
			}
		}
		//if (page_heights.size() > 0){
		//	fz_rect debug_document_rect;
		//	float doc_pos_x, doc_pos_y;
		//	int clicked_page;
		//	window_to_document_pos(debug_last_clicked_x, debug_last_clicked_y, &doc_pos_x, &doc_pos_y, &clicked_page);
		//	debug_document_rect.x0 = doc_pos_x - 50;
		//	debug_document_rect.x1 = doc_pos_x + 50;
		//	debug_document_rect.y0 = doc_pos_y - 50;
		//	debug_document_rect.y1 = doc_pos_y + 50;
		//	fz_rect debug_window_rect = document_to_window_rect(clicked_page, debug_document_rect);
		//	float quad_vertex_data[] = {
		//		debug_window_rect.x0, debug_window_rect.y1,
		//		debug_window_rect.x1, debug_window_rect.y1,
		//		debug_window_rect.x0, debug_window_rect.y0,
		//		debug_window_rect.x1, debug_window_rect.y0
		//	};
		//	glEnable(GL_BLEND);
		//	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		//	glUseProgram(highlight_program);
		//	glEnableVertexAttribArray(0);
		//	glEnableVertexAttribArray(1);
		//	glBufferData(GL_ARRAY_BUFFER, sizeof(quad_vertex_data), quad_vertex_data, GL_DYNAMIC_DRAW);
		//	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		//	glDisable(GL_BLEND);
		//}

		if (search_results.size() > 0) {
			SearchResult current_search_result = search_results[current_search_result_index];
			fz_quad result_quad = current_search_result.quad;
			fz_rect result_rect;
			result_rect.x0 = result_quad.ul.x;
			result_rect.x1 = result_quad.ur.x;

			result_rect.y0 = result_quad.ul.y;
			result_rect.y1 = result_quad.ll.y;

			fz_rect window_rect = document_to_window_rect(current_search_result.page, result_rect);
			float quad_vertex_data[] = {
				window_rect.x0, window_rect.y1,
				window_rect.x1, window_rect.y1,
				window_rect.x0, window_rect.y0,
				window_rect.x1, window_rect.y0
			};

			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			glUseProgram(highlight_program);
			glEnableVertexAttribArray(0);
			glEnableVertexAttribArray(1);
			//glBindVertexArray(vertex_array_object);
			//glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer_object);
			glBufferData(GL_ARRAY_BUFFER, sizeof(quad_vertex_data), quad_vertex_data, GL_DYNAMIC_DRAW);
			glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
			glDisable(GL_BLEND);

		}
	}

	void persist() {
		update_book(database, document_path, zoom_level, offset_x, offset_y);
	}
};


class WindowState {
private:
	DocumentView* main_document_view;
	DocumentView* helper_document_view;
	DocumentView* current_document_view;
	//Document* current_document;
	SDL_Window* main_window;
	SDL_Window* helper_window;
	SDL_GLContext* opengl_context;
	fz_context* mupdf_context;
	sqlite3* database;
	const Command* pending_text_command;
	FilteredSelect<int>* current_toc_select;
	FilteredSelect<string>* current_opened_doc_select;
	FilteredSelect<float>* current_bookmark_select;
	FilteredSelect<BookState>* current_global_bookmark_select;

	//float zoom_level;
	GLuint vertex_array_object;
	GLuint vertex_buffer_object;
	GLuint uv_buffer_object;
	GLuint gl_program;
	GLuint gl_debug_program;
	GLuint gl_unrendered_program;

	string pending_link_source_document_path;
	Link pending_link;
	bool pending_link_source_filled;

	int current_history_index;
	vector<DocumentViewState> history;
	vector<DocumentView*> cached_document_views;

	Link* link_to_edit;

	float last_mouse_down_x;
	float last_mouse_down_y;
	optional<fz_rect> selected_rect;

	void set_main_document_view_state(DocumentViewState new_view_state) {
		main_document_view = new_view_state.document_view;
		int window_width, window_height;
		SDL_GetWindowSize(main_window, &window_width, &window_height);
		main_document_view->on_view_size_change(window_width, window_height);
		main_document_view->set_offsets(new_view_state.offset_x, new_view_state.offset_y);
		main_document_view->set_zoom_level(new_view_state.zoom_level);
		current_document_view = main_document_view;
	}


	//void pop_state() {
	//	if (history.size() > 0) {
	//		DocumentViewState new_state = history[history.size() - 1];
	//		history.pop_back();
	//		set_main_document_view_state(new_state);
	//	}
	//}

	void prev_state() {
		if (current_history_index > 0) {
			if (current_history_index == history.size()) {
				push_state();
				current_history_index = history.size()-1;
			}
			current_history_index--;

			if (link_to_edit) {
				float link_new_offset_x = current_document_view->get_offset_x();
				float link_new_offset_y = current_document_view->get_offset_y();
				link_to_edit->dest_offset_x = link_new_offset_x;
				link_to_edit->dest_offset_y = link_new_offset_y;
				update_link(database, history[current_history_index].document_view->get_document()->get_path(),
					link_new_offset_x, link_new_offset_y, link_to_edit->src_offset_y);
				link_to_edit = nullptr;
			}

			set_main_document_view_state(history[current_history_index]);
		}
	}

	void next_state() {
		if (current_history_index+1 < history.size()) {
			current_history_index++;
			set_main_document_view_state(history[current_history_index]);
		}
	}

	void push_state() {
		DocumentViewState dvs;
		dvs.document_view = main_document_view;
		dvs.offset_x = main_document_view->get_offset_x();
		dvs.offset_y = main_document_view->get_offset_y();
		dvs.zoom_level = main_document_view->get_zoom_level();


		if (history.size() > 0) {
			DocumentViewState last_history = history[history.size() - 1];
			if (last_history.document_view == main_document_view && last_history.offset_x == dvs.offset_x && last_history.offset_y == dvs.offset_y) {
				return;
			}
		}

		if (current_history_index == history.size()) {
			history.push_back(dvs);
			current_history_index = history.size();
		}
		else {
			//todo: should probably do some reference counting garbage collection here

			history.erase(history.begin() + current_history_index, history.end());
			history.push_back(dvs);
			current_history_index = history.size();
		}
	}

	//vector<float> page_heights;
	//vector<float> page_widths;

	//string current_document_path;

	// offset of the center of screen in the current document space
	//float offset_x;
	//float offset_y;


	//int main_window_prev_width;
	//int main_window_prev_height;

	unsigned int last_tick_time;

	//vector<CachedPage> cached_pages;

	bool is_showing_ui;
	bool is_showing_textbar;
	char text_command_buffer[100];
	vector<fz_rect> selected_character_rects;

public:
	bool render_is_invalid;
	WindowState(SDL_Window* main_window,
		SDL_Window* helper_window,
		fz_context* mupdf_context,
		SDL_GLContext* opengl_context,
		sqlite3* database
	) :
		mupdf_context(mupdf_context),
		opengl_context(opengl_context),
		main_window(main_window),
		helper_window(helper_window),
		database(database),
		render_is_invalid(true),
		is_showing_ui(false),
		is_showing_textbar(false),
		pending_text_command(nullptr),
		current_toc_select(nullptr),
		current_bookmark_select(nullptr)
	{

		main_document_view = new DocumentView(mupdf_context, database);
		helper_document_view = new DocumentView(mupdf_context, database);

		cached_document_views.push_back(main_document_view);

		current_document_view = main_document_view;

		gl_program = LoadShaders("shaders\\simple.vertex", "shaders\\simple.fragment");
		if (gl_program == 0) {
			cout << "Error: could not compile shaders" << endl;
		}

		gl_debug_program = LoadShaders("shaders\\simple.vertex", "shaders\\debug.fragment");
		if (gl_debug_program == 0) {
			cout << "Error: could not compile debug shaders" << endl;
		}

		gl_unrendered_program = LoadShaders("shaders\\simple.vertex", "shaders\\unrendered_page.fragment");
		if (gl_unrendered_program == 0) {
			cout << "Error: could not compile debug shaders" << endl;
		}


		//unsigned char test_texture_data[] = {
		//	255, 0, 0, 0, 255, 0,
		//	0, 0, 255, 255, 0, 0
		//};

		//glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 2, 2, 0, GL_RGB, GL_UNSIGNED_BYTE, test_texture_data);

		glGenVertexArrays(1, &vertex_array_object);
		glBindVertexArray(vertex_array_object);

		glGenBuffers(1, &vertex_buffer_object);
		glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer_object);
		glBufferData(GL_ARRAY_BUFFER, sizeof(g_quad_vertex), g_quad_vertex, GL_DYNAMIC_DRAW);
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);

		glGenBuffers(1, &uv_buffer_object);
		glBindBuffer(GL_ARRAY_BUFFER, uv_buffer_object);
		glBufferData(GL_ARRAY_BUFFER, sizeof(g_quad_uvs), g_quad_uvs, GL_DYNAMIC_DRAW);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0);

		glDisable(GL_CULL_FACE);


		int main_window_width, main_window_height;
		SDL_GetWindowSize(main_window, &main_window_width, &main_window_height);
		main_document_view->on_view_size_change(main_window_width, main_window_height);

		int helper_window_width, helper_window_height;
		SDL_GetWindowSize(helper_window, &helper_window_width, &helper_window_height);
		helper_document_view->on_view_size_change(helper_window_width, helper_window_height);

		last_tick_time = SDL_GetTicks();
	}

	void handle_link() {
		if (pending_link_source_filled) {
			pending_link.dest_offset_x = main_document_view->get_offset_x();
			pending_link.dest_offset_y = main_document_view->get_offset_y();
			pending_link.document_path = main_document_view->get_document()->get_path();
			if (pending_link_source_document_path == main_document_view->get_document()->get_path()) {
				main_document_view->get_document()->add_link(pending_link);
			}
			else {
				for (int i = 0; i < cached_document_views.size(); i++) {
					if (cached_document_views[i]->get_document()) {
						if (cached_document_views[i]->get_document()->get_path() == pending_link_source_document_path) {
							cached_document_views[i]->get_document()->add_link(pending_link, false);
						}
					}
				}
				insert_link(database,
					pending_link_source_document_path,
					pending_link.document_path,
					pending_link.dest_offset_x,
					pending_link.dest_offset_y,
					pending_link.src_offset_y);
			}
			render_is_invalid = true;
			pending_link_source_filled = false;
		}
		else {
			pending_link.src_offset_y = main_document_view->get_offset_y();
			pending_link_source_document_path = main_document_view->get_document()->get_path();
			pending_link_source_filled = true;
		}
		invalidate_render();
	}


	void on_focus_gained(Uint32 window_id) {
		if (window_id == SDL_GetWindowID(main_window)) {
			current_document_view = main_document_view;
		}
		if (window_id == SDL_GetWindowID(helper_window)) {
			current_document_view = helper_document_view;
		}
	}

	void handle_command_with_symbol(const Command* command, char symbol) {
		assert(symbol);
		assert(command->requires_symbol);
		if (command->name == "set_mark") {
			assert(current_document_view);
			current_document_view->add_mark(symbol);
			invalidate_render();
		}
		else if (command->name == "goto_mark") {
			assert(current_document_view);
			current_document_view->goto_mark(symbol);
		}
		else if (command->name == "delete") {
			if (symbol == 'y') {
				main_document_view->delete_closest_link();
				invalidate_render();
			}
			else if (symbol == 'b') {
				main_document_view->delete_closest_bookmark();
				invalidate_render();
			}
		}
	}

	void handle_command_with_file_name(const Command* command, string file_name) {
		assert(command->requires_file_name);
		//cout << "handling " << command->name << " with file " << file_name << endl;
		if (command->name == "open_document") {
			//current_document_view->open_document(file_name);
			open_document(file_name);
		}
	}


	//void handle_command_with_text(const Command* command, string text) {
	//	assert(command->requires_text);
	//	pending_text_command = command;
	//	is_showing_textbar = true;
	//	is_showing_ui = true;
	//}


	void handle_command(const Command* command, int num_repeats) {
		if (command->requires_text) {
			pending_text_command = command;
			show_textbar();
			return;
		}
		if (command->pushes_state) {
			push_state();
		}
		if (command->name == "goto_begining") {
			if (num_repeats) {
				current_document_view->goto_page(num_repeats);
			}
			else {
				current_document_view->set_offset_y(0.0f);
			}
		}

		if (command->name == "goto_end") {
			current_document_view->goto_end();
		}
		//if (command->name == "search") {
		//	show_searchbar();
		//}
		int rp = max(num_repeats, 1);

		if (command->name == "move_down") {
			current_document_view->move(0.0f, 72.0f * rp);
		}
		else if (command->name == "move_up") {
			current_document_view->move(0.0f, -72.0f * rp);
		}

		else if (command->name == "move_right") {
			current_document_view->move(72.0f * rp, 0.0f);
		}
		else if (command->name == "link") {
			handle_link();
		}

		else if (command->name == "move_left") {
			current_document_view->move(-72.0f * rp, 0.0f);
		}

		else if (command->name == "goto_link") {
			Link* link = main_document_view->find_closest_link();
			if (link) {

				//if trying to go to the link we are currently viewing, go back to the previous state instead
				//this is so that the user can tap tab button to quickly change between the main view and the link

				if (link->dest_offset_x == main_document_view->get_offset_x() &&
					link->dest_offset_y == main_document_view->get_offset_y() &&
					link->document_path == main_document_view->get_document()->get_path()
					) {
					prev_state();
				}
				else {
					push_state();
					main_document_view = new DocumentView(mupdf_context, database);
					current_document_view = main_document_view;
					int window_width, window_height;
					SDL_GetWindowSize(main_window, &window_width, &window_height);
					main_document_view->on_view_size_change(window_width, window_height);
					main_document_view->open_document(link->document_path);
					main_document_view->set_offsets(link->dest_offset_x, link->dest_offset_y);
					invalidate_render();
				}
			}
		}
		else if (command->name == "edit_link") {
			Link* link = main_document_view->find_closest_link();
			if (link) {
				push_state();
				link_to_edit = link;
				main_document_view = new DocumentView(mupdf_context, database);
				current_document_view = main_document_view;
				int window_width, window_height;
				SDL_GetWindowSize(main_window, &window_width, &window_height);
				main_document_view->on_view_size_change(window_width, window_height);
				main_document_view->open_document(link->document_path);
				main_document_view->set_offsets(link->dest_offset_x, link->dest_offset_y);
				invalidate_render();
			}
		}

		else if (command->name == "zoom_in") {
			current_document_view->zoom_in();
		}

		else if (command->name == "zoom_out") {
			current_document_view->zoom_out();
		}

		else if (command->name == "next_state") {
			next_state();
			cout << "next state" << endl;
		}
		else if (command->name == "prev_state") {
			prev_state();
			cout << "prev state" << endl;
		}

		else if (command->name == "next_item") {
			current_document_view->goto_search_result(1 + num_repeats);
		}

		else if (command->name == "previous_item") {
			current_document_view->goto_search_result(-1 - num_repeats);
		}
		else if (command->name == "push_state") {
			push_state();
		}
		else if (command->name == "pop_state") {
			prev_state();
		}

		else if (command->name == "open_document") {
			cout << "can not open document right now!" << endl;
		}
		else if (command->name == "next_page") {
			current_document_view->move_pages(1 + num_repeats);
		}
		else if (command->name == "previous_page") {
			current_document_view->move_pages(-1 - num_repeats);
		}
		else if (command->name == "goto_toc") {
			vector<string> flat_toc;
			vector<int> current_document_toc_pages;
			get_flat_toc(current_document_view->get_document()->get_toc(), flat_toc, current_document_toc_pages);
			if (current_document_toc_pages.size() > 0) {
				current_toc_select = new FilteredSelect<int>(flat_toc, current_document_toc_pages);
				is_showing_ui = true;
			}
		}
		else if (command->name == "open_prev_doc") {
			vector<string> opened_docs_paths;
			vector<string> opened_docs_names;
			select_prev_docs(database, opened_docs_paths);

			for (const auto& p : opened_docs_paths) {
				opened_docs_names.push_back(std::filesystem::path(p).filename().string());
			}

			if (opened_docs_paths.size() > 0) {
				current_opened_doc_select = new FilteredSelect<string>(opened_docs_names, opened_docs_paths);
				is_showing_ui = true;
			}
		}
		else if (command->name == "goto_bookmark") {
			is_showing_ui = true;
			vector<string> option_names;
			vector<float> option_locations;
			for (int i = 0; i < current_document_view->get_document()->get_bookmarks().size(); i++) {
				option_names.push_back(current_document_view->get_document()->get_bookmarks()[i].description);
				option_locations.push_back(current_document_view->get_document()->get_bookmarks()[i].y_offset);
			}
			current_bookmark_select = new FilteredSelect<float>(option_names, option_locations);
			cout << "bookmark" << endl;
		}
		else if (command->name == "goto_bookmark_g") {
			is_showing_ui = true;
			vector<pair<string, BookMark>> global_bookmarks;
			global_select_bookmark(database, global_bookmarks);
			vector<string> descs;
			vector<BookState> book_states;

			for (const auto& desc_bm_pair : global_bookmarks) {
				string path = desc_bm_pair.first;
				BookMark bm = desc_bm_pair.second;
				descs.push_back(bm.description);
				book_states.push_back({ path, bm.y_offset });
			}
			current_global_bookmark_select = new FilteredSelect<BookState>(descs, book_states);

		}
		else if (command->name == "debug") {
			cout << "debug" << endl;
		}

	}


	bool should_render() {
		if (render_is_invalid) {
			return true;
		}
		if (is_showing_ui) {
			return true;
		}
		if (current_document_view->should_rerender()) {
			return true;
		}
		return false;
	}

	void handle_return() {
		is_showing_textbar = false;
		is_showing_ui = false;
		render_is_invalid = true;
		//current_search_result_index = 0;
		pending_text_command = nullptr;
	}


	void handle_pending_text_command(string text) {
		if (pending_text_command->name == "search") {
			//search_results.clear();
			//search_text(text.c_str(), search_results);
			current_document_view->search_text(text.c_str());
		}

		if (pending_text_command->name == "add_bookmark") {
			current_document_view->add_bookmark(text);
		}
	}





	void on_window_size_changed(Uint32 window_id) {
		if (window_id == SDL_GetWindowID(main_window)) {
			int new_width, new_height;
			SDL_GetWindowSize(main_window, &new_width, &new_height);
			main_document_view->on_view_size_change(new_width, new_height);
		}

		if (window_id == SDL_GetWindowID(helper_window)) {
			int new_width, new_height;
			SDL_GetWindowSize(helper_window, &new_width, &new_height);
			helper_document_view->on_view_size_change(new_width, new_height);
		}

		//if ((new_width != main_window_prev_width) || (new_height != main_window_prev_height)) {
		//	main_window_prev_width = new_width;
		//	main_window_prev_height = new_height;
		//}
		render_is_invalid = true;
	}



	void tick(bool force = false) {
		unsigned int now = SDL_GetTicks();
		if (force || (now - last_tick_time) > 1000) {
			last_tick_time = now;
			//update_book(database, current_document_path, zoom_level, offset_x, offset_y);
			main_document_view->persist();
		}
	}

	void update_window_title() {
		//if (current_document) {
		//	stringstream title;
		//	title << current_document_path << " ( page " << get_current_page_number() << " / " << current_document->num_pages() << " )";
		//	if (search_results.size() > 0) {
		//		title << " [Showing result " << current_search_result_index + 1 << " / " << search_results.size() << " ]";
		//	}
		//	SDL_SetWindowTitle(main_window, title.str().c_str());
		//}
	}

	string get_status_string(const Command* pending_command) {
		stringstream ss;
		if (main_document_view->get_document() == nullptr) return "";
		ss << "Page " << main_document_view->get_current_page_number() << " / " << main_document_view->get_document()->num_pages();
		int num_search_results = main_document_view->get_num_search_results();
		if (num_search_results > 0) {
			ss << " | showing result " << main_document_view->get_current_search_result_index() + 1 << " / " << num_search_results;
		}
		if (pending_link_source_filled) {
			ss << " | linking ...";
		}
		if (link_to_edit) {
			ss << " | editing link ...";
		}
		if (pending_command && pending_command->requires_symbol) {
			ss << " | " << pending_command->name <<" waiting for symbol";
		}
		return ss.str();
	}

	void invalidate_render() {
		render_is_invalid = true;
	}


	void show_textbar() {
		is_showing_ui = true;
		is_showing_textbar = true;
		ZeroMemory(text_command_buffer, sizeof(text_command_buffer));
	}

	void open_document(string path) {
		//current_document_view->open_document(path);
		int window_width, window_height;
		SDL_GetWindowSize(main_window, &window_width, &window_height);

		main_document_view = new DocumentView(mupdf_context, database);
		main_document_view->open_document(path);
		main_document_view->on_view_size_change(window_width, window_height);
		current_document_view = main_document_view;
		cached_document_views.push_back(main_document_view);

		if (path.size() > 0) {
			// this part should be moved to WindowState
			// ------------------------------------------
			ofstream last_path_file(last_path_file_absolute_location);
			//cout << "!!! " << last_path_file.is_open() << endl;
			last_path_file << path << endl;
			last_path_file.close();
			// ------------------------------------------
		}
	}

	void handle_escape() {
		is_showing_ui = false;
		is_showing_textbar = false;
		ZeroMemory(text_command_buffer, sizeof(text_command_buffer));
		pending_link_source_document_path = "";
		pending_link_source_filled = false;
		pending_text_command = nullptr;
		if (current_toc_select) {
			delete current_toc_select;
			current_toc_select = nullptr;
		}
		if (current_opened_doc_select) {
			delete current_opened_doc_select;
			current_opened_doc_select = nullptr;
		}
		if (current_bookmark_select) {
			delete current_bookmark_select;
			current_bookmark_select = nullptr;
		}
		if (current_global_bookmark_select) {
			delete current_global_bookmark_select;
			current_global_bookmark_select = nullptr;
		}
		if (main_document_view) {
			main_document_view->handle_escape();
		}

		if (helper_document_view) {
			helper_document_view->handle_escape();
		}
		invalidate_render();
	}

	void handle_click(int pos_x, int pos_y) {
		auto link_ = main_document_view->get_link_in_pos(pos_x, pos_y);
		if (link_.has_value()) {
			PdfLink link = link_.value();
			int page;
			float offset_x, offset_y;
			parse_uri(link.uri, &page, &offset_x, &offset_y);
			if (!pending_link_source_filled) {
				push_state();
				main_document_view->goto_offset_within_page(page, offset_x, offset_y);
			}
			else {
				pending_link.dest_offset_x = offset_x;
				pending_link.dest_offset_y = main_document_view->get_page_offset(page-1) + offset_y;
				pending_link.document_path = main_document_view->get_document()->get_path();
				main_document_view->get_document()->add_link(pending_link);
				render_is_invalid = true;
				pending_link_source_filled = false;
			}

		}
	}

	void handle_left_click(float x, float y, bool down) {
		float x_, y_;
		main_document_view->window_to_absolute_document_pos(x, y, &x_, &y_);

		if (down == true) {
			last_mouse_down_x = x_;
			last_mouse_down_y = y_;
		}
		else {
			if ((abs(last_mouse_down_x - x_) + abs(last_mouse_down_y - y_)) > 20) {
				fz_rect sr;
				sr.x0 = min(last_mouse_down_x, x_);
				sr.x1 = max(last_mouse_down_x, x_);

				sr.y0 = min(last_mouse_down_y, y_);
				sr.y1 = max(last_mouse_down_y, y_);
				selected_rect = sr;

				main_document_view->get_text_selection(sr, selected_character_rects);
				invalidate_render();
			}
			else {
				selected_rect = {};
				handle_click(x, y);
				selected_character_rects.clear();
				invalidate_render();
			}
		}
	}

	void render(const Command* pending_command) {
		if (should_render()) {
			cout << "rendering ..." << endl;

			//current_document_view->get_document()->find_closest_link(current_document_view->)
			Link* link = main_document_view->find_closest_link();
			if (link) {
				string helper_document_path = link->document_path;
				float helper_document_offset_x = link->dest_offset_x;
				float helper_document_offset_y = link->dest_offset_y;

				if (helper_document_view->get_document() && 
					helper_document_view->get_document()->get_path() == helper_document_path) {
					helper_document_view->set_offsets(helper_document_offset_x, helper_document_offset_y);
				}
				else {
					delete helper_document_view;
					int helper_window_width, helper_window_height;
					SDL_GetWindowSize(helper_window, &helper_window_width, &helper_window_height);

					helper_document_view = new DocumentView(mupdf_context, database);
					helper_document_view->on_view_size_change(helper_window_width, helper_window_height);
					helper_document_view->open_document(helper_document_path);
					helper_document_view->set_offsets(helper_document_offset_x, helper_document_offset_y);
					cached_document_views.push_back(main_document_view);
				}
			}
			render_is_invalid = false;


			ImGuiIO& io = ImGui::GetIO(); (void)io;

			update_window_title();

			SDL_GL_MakeCurrent(main_window, *opengl_context);
			int window_width, window_height;
			SDL_GetWindowSize(main_window, &window_width, &window_height);

			glViewport(0, 0, window_width, window_height);
			glBindVertexArray(vertex_array_object);
			glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer_object);

			main_document_view->render(gl_program, gl_unrendered_program, gl_debug_program);

			{
				ImGui_ImplOpenGL3_NewFrame();
				ImGui_ImplSDL2_NewFrame(main_window);
				ImGui::NewFrame();

				if (is_showing_textbar) {

					ImGui::SetNextWindowPos(ImVec2(0, 0));
					ImGui::SetNextWindowSize(ImVec2(window_width, 60));
					ImGui::Begin(pending_text_command->name.c_str());
					//ImGui::SetKeyboardFocusHere();
					ImGui::SetKeyboardFocusHere();
					if (ImGui::InputText(
						pending_text_command->name.c_str(),
						text_command_buffer,
						sizeof(text_command_buffer),
						ImGuiInputTextFlags_EnterReturnsTrue)
						) {
						//search_results.clear();
						//search_text(search_string, search_results);
						//handle_return();
						handle_pending_text_command(text_command_buffer);
						current_document_view->goto_search_result(0);
						handle_return();
						io.WantCaptureKeyboard = false;
					}
					ImGui::End();
				}
				if (current_toc_select) {
					//vector<string> select_items = { "ali", "alo", "mos", "tafavi" };
					//render_select(select_items, nullptr);
					if (current_toc_select->render()) {
						//cout << "selected option " << current_select->get_selected_option() << endl;
						//int selected_index = current_toc_select->get_selected_index();
						//goto_page(current_document_toc_pages[selected_index]);
						int* page_value = current_toc_select->get_value();
						if (page_value) {
							push_state();
							current_document_view->goto_page(*page_value);
						}
						delete current_toc_select;
						current_toc_select = nullptr;
						is_showing_ui = false;
					}
				}
				if (current_opened_doc_select) {
					if (current_opened_doc_select->render()) {
						string doc_path = *current_opened_doc_select->get_value();
						delete current_opened_doc_select;
						current_opened_doc_select = nullptr;
						is_showing_ui = false;

						if (doc_path.size() > 0) {
							push_state();
							//current_document_view->goto_page(*page_value);

							main_document_view = new DocumentView(mupdf_context, database);
							main_document_view->open_document(doc_path);
							int window_width, window_height;
							SDL_GetWindowSize(main_window, &window_width, &window_height);
							main_document_view->on_view_size_change(window_width, window_height);
							current_document_view = main_document_view;
							cached_document_views.push_back(main_document_view);
							invalidate_render();
						}
					}
				}
				if (current_bookmark_select) {
					if (current_bookmark_select->render()) {
						float* offset_value = current_bookmark_select->get_value();
						if (offset_value) {
							push_state();
							current_document_view->set_offset_y(*offset_value);
						}
						delete current_bookmark_select;
						current_bookmark_select = nullptr;
						is_showing_ui = false;
						render_is_invalid = true;
					}
				}
				if (current_global_bookmark_select) {
					if (current_global_bookmark_select->render()) {
						BookState* offset_value = current_global_bookmark_select->get_value();
						if (offset_value) {
							push_state();
							main_document_view = new DocumentView(mupdf_context, database);
							main_document_view->open_document(offset_value->document_path);
							main_document_view->set_offset_y(offset_value->offset_y);
							int window_width, window_height;
							SDL_GetWindowSize(main_window, &window_width, &window_height);
							main_document_view->on_view_size_change(window_width, window_height);
							cached_document_views.push_back(main_document_view);

						}
						delete current_global_bookmark_select;
						current_global_bookmark_select = nullptr;
						is_showing_ui = false;
						render_is_invalid = true;
					}
				}

				ImGui::GetStyle().WindowRounding = 0.0f;
				ImGui::GetStyle().WindowBorderSize = 0.0f;
				ImGui::SetNextWindowSize(ImVec2(window_width, 20));
				ImGui::SetNextWindowPos(ImVec2(0, window_height - 25));
				ImGui::Begin("some test", 0, ImGuiWindowFlags_NoDecoration);
				ImGui::Text(get_status_string(pending_command).c_str());
				ImGui::End();

				ImGui::Render();
				ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

			}
			for (auto rect : selected_character_rects) {
				main_document_view->render_highlight_absolute(gl_debug_program, rect);
			}

			if (selected_rect.has_value()) {
				fz_rect sr = selected_rect.value();
				//main_document_view->render_highlight_absolute(gl_debug_program, sr);
				//int window_width, window_height;
				//SDL_GetWindowSize(main_window, &window_width, &window_height);

				//sr.x0 = (sr.x0 - window_width / 2) / (window_width / 2);
				//sr.x1 = (sr.x1 - window_width / 2) / (window_width / 2);

				//sr.y0 = -(sr.y0 - window_height / 2) / (window_height / 2);
				//sr.y1 = -(sr.y1 - window_height / 2) / (window_height / 2);

				//float quad_vertex_data[] = {
				//	sr.x0, sr.y1,
				//	sr.x1, sr.y1,
				//	sr.x0, sr.y0,
				//	sr.x1, sr.y0
				//};
				//glEnable(GL_BLEND);
				//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
				//glUseProgram(gl_debug_program);
				//glEnableVertexAttribArray(0);
				//glEnableVertexAttribArray(1);
				//glBufferData(GL_ARRAY_BUFFER, sizeof(quad_vertex_data), quad_vertex_data, GL_DYNAMIC_DRAW);
				//glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
				//glDisable(GL_BLEND);

			}

			SDL_GL_SwapWindow(main_window);

			SDL_GL_MakeCurrent(helper_window, *opengl_context);

			SDL_GetWindowSize(helper_window, &window_width, &window_height);
			glViewport(0, 0, window_width, window_height);

			glClearColor(1.0f, 0.0f, 0.0f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT);
			helper_document_view->render(gl_program, gl_unrendered_program, gl_debug_program);
			SDL_GL_SwapWindow(helper_window);
			global_pdf_renderer->delete_old_pages();
		}
	}
};


int main(int argc, char* args[]) {

	sqlite3* db;
	char* error_message = nullptr;
	int rc;

	rc = sqlite3_open("test.db", &db);
	if (rc) {
		cout << "could not open database" << sqlite3_errmsg(db) << endl;
	}

	create_opened_books_table(db);
	create_marks_table(db);
	create_bookmarks_table(db);
	create_links_table(db);


	fz_locks_context locks;
	locks.user = mupdf_mutexes;
	locks.lock = lock_mutex;
	locks.unlock = unlock_mutex;

	fz_context* mupdf_context = fz_new_context(nullptr, &locks, FZ_STORE_UNLIMITED);
	//fz_context* mupdf_context = fz_new_context(nullptr, nullptr, FZ_STORE_UNLIMITED);
	if (!mupdf_context) {
		cout << "could not create mupdf context" << endl;
		return -1;
	}
	bool fail = false;
	fz_try(mupdf_context) {
		fz_register_document_handlers(mupdf_context);
	}
	fz_catch(mupdf_context) {
		cout << "could not register document handlers" << endl;
		fail = true;
	}

	if (fail) {
		return -1;
	}

	global_pdf_renderer = new PdfRenderer(mupdf_context);
	//global_pdf_renderer = new PdfRenderer();
	//global_pdf_renderer->init();

	thread worker([]() {
		global_pdf_renderer->run();
		});


	SDL_Window* window = nullptr;

	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		cout << "could not initialize SDL" << endl;
		return -1;
	}

	CommandManager command_manager;

	SDL_Rect display_rect;
	SDL_GetDisplayBounds(0, &display_rect);

	SDL_Window* window2 = SDL_CreateWindow("Pdf Viewer2", display_rect.w, SDL_WINDOWPOS_UNDEFINED, 500, 500, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_MAXIMIZED);
	window = SDL_CreateWindow("Pdf Viewer", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 500, 500, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_MAXIMIZED);

	if (window == nullptr) {
		cout << "could not create SDL window" << endl;
		return -1;
	}
	if (window2 == nullptr) {
		cout << "could not create the second window" << endl;
		return -1;
	}

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 1);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

	SDL_GLContext gl_context = SDL_GL_CreateContext(window);

	if (gl_context == nullptr) {
		cout << SDL_GetError() << endl;
		cout << "could not create opengl context" << endl;
		return -1;
	}

	glewExperimental = true;
	GLenum glew_error = glewInit();
	if (glew_error != GLEW_OK) {
		cout << "could not initialize glew" << endl;
		return -1;
	}

	if (SDL_GL_SetSwapInterval(1) < 0) {
		cout << "could not enable vsync" << endl;
		return -1;
	}

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	ImGui::StyleColorsDark();
	ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
	ImGui_ImplOpenGL3_Init("#version 400");

	WindowState window_state(window, window2, mupdf_context, &gl_context, db);

	//char file_path[MAX_PATH] = { 0 };
	string file_path;
	ifstream last_state_file(last_path_file_absolute_location);
	//last_state_file.read(file_path, MAX_PATH);
	getline(last_state_file, file_path);
	//last_state_file >> initial_document;
	last_state_file.close();

	window_state.open_document(file_path);

	bool quit = false;

	global_pdf_renderer->set_invalidate_pointer(&window_state.render_is_invalid);
	InputHandler input_handler("keys.config");

	bool is_waiting_for_symbol = false;
	const Command* current_pending_command = nullptr;
	while (!quit) {
		SDL_Event event;

		while (SDL_PollEvent(&event)) {
			ImGui_ImplSDL2_ProcessEvent(&event);

			// retarded hack to deal with imgui one frame lag
			if ((event.key.type == SDL_KEYDOWN || event.key.type == SDL_KEYUP) && event.key.keysym.scancode == SDL_SCANCODE_ESCAPE) {
				current_pending_command = nullptr;
				is_waiting_for_symbol = false;
				window_state.handle_escape();
			}

			else if ((event.key.type == SDL_KEYDOWN) && io.WantCaptureKeyboard) {
				continue;
			}
			if (event.type == SDL_MOUSEBUTTONDOWN) {
				if (event.button.button == SDL_BUTTON_LEFT) {
					//window_state.handle_click(event.button.x, event.button.y);
					window_state.handle_left_click(event.button.x, event.button.y, true);
				}

				if (event.button.button == SDL_BUTTON_X1) {
					window_state.handle_command(command_manager.get_command_with_name("next_state"), 0);
				}

				if (event.button.button == SDL_BUTTON_X2) {
					window_state.handle_command(command_manager.get_command_with_name("prev_state"), 0);
				}

			}

			if (event.type == SDL_MOUSEBUTTONUP) {
				if (event.button.button == SDL_BUTTON_LEFT) {
					//window_state.handle_click(event.button.x, event.button.y);
					window_state.handle_left_click(event.button.x, event.button.y, false);
				}

			}

			//render_invalidated = true;
			if (event.type == SDL_QUIT) {
				quit = true;
			}
			if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE) {
				quit = true;
			}
			if (event.type == SDL_WINDOWEVENT && ((event.window.event == SDL_WINDOWEVENT_RESIZED)
				|| (event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED))) {
				window_state.on_window_size_changed(event.window.windowID);
			}
			if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_FOCUS_GAINED) {
				window_state.on_focus_gained(event.window.windowID);
			}
			if (event.type == SDL_KEYDOWN) {
				vector<SDL_Scancode> ignored_codes = {
					SDL_SCANCODE_LSHIFT,
					SDL_SCANCODE_RSHIFT,
					SDL_SCANCODE_LCTRL,
					SDL_SCANCODE_RCTRL
				};
				if (find(ignored_codes.begin(), ignored_codes.end(), event.key.keysym.scancode) != ignored_codes.end()) {
					continue;
				}
				if (is_waiting_for_symbol) {
					char symb = get_symbol(event.key.keysym.scancode, (event.key.keysym.mod & KMOD_SHIFT != 0));
					if (symb) {
						window_state.handle_command_with_symbol(current_pending_command, symb);
						current_pending_command = nullptr;
						is_waiting_for_symbol = false;
					}
					continue;
				}
				int num_repeats = 0;
				const Command* command = input_handler.handle_key(
					SDL_GetKeyFromScancode(event.key.keysym.scancode),
					(event.key.keysym.mod & KMOD_SHIFT) != 0,
					(event.key.keysym.mod & KMOD_CTRL) != 0, &num_repeats);
				if (command) {
					if (command->requires_symbol) {
						is_waiting_for_symbol = true;
						current_pending_command = command;
						window_state.invalidate_render();
						continue;
					}
					if (command->requires_file_name) {
						char file_name[MAX_PATH];
						if (select_pdf_file_name(file_name, MAX_PATH)) {
							window_state.handle_command_with_file_name(command, file_name);
						}
						else {
							cout << "File select failed" << endl;
						}
						continue;
					}
					else {
						window_state.handle_command(command, num_repeats);
					}
				}
			}
			if (event.type == SDL_MOUSEWHEEL) {
				int num_repeats = 1;
				const Command* command;
				if (event.wheel.y > 0) {
					command = input_handler.handle_key(SDLK_UP, false, false, &num_repeats);
				}
				if (event.wheel.y < 0) {
					command = input_handler.handle_key(SDLK_DOWN, false, false, &num_repeats);
				}
				
				window_state.handle_command(command, abs(event.wheel.y));
			}

		}
		window_state.render(current_pending_command);

		SDL_Delay(16);
		window_state.tick();
	}

	sqlite3_close(db);
	worker.join();
	return 0;
}
