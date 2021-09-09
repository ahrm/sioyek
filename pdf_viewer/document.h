#pragma once
#include <vector>
#include <string>
#include <optional>
#include <iostream>
#include <thread>
#include <map>
#include <unordered_map>

//#include <Windows.h>
#include <qstandarditemmodel.h>
#include <qdatetime.h>

#include <qobject.h>

#include <mupdf/fitz.h>
#include "sqlite3.h"

#include "database.h"
#include "utils.h"
#include "book.h"
#include "checksum.h"

class Document {

private:

	std::vector<Mark> marks;
	std::vector<BookMark> bookmarks;
	std::vector<Highlight> highlights;
	std::vector<Link> links;
	DatabaseManager* db_manager = nullptr;
	std::vector<TocNode*> top_level_toc_nodes;
	std::vector<std::wstring> flat_toc_names;
	std::vector<int> flat_toc_pages;


	// number of pages in the document
	std::optional<int> cached_num_pages = {};

	std::vector<std::pair<int, fz_stext_page*>> cached_stext_pages;
	std::vector<std::pair<int, fz_pixmap*>> cached_small_pixmaps;

	fz_context* context = nullptr;
	std::wstring file_name;
	std::unordered_map<int, fz_link*> cached_page_links;
	QStandardItemModel* cached_toc_model = nullptr;

	std::vector<float> accum_page_heights;
	std::vector<float> page_heights;
	std::vector<float> page_widths;
	std::mutex page_dims_mutex;

	// These are a heuristic index of all figures and references in the document
	// The reason that we use a hashmap for reference_indices and a vector for figures is that
	// the reference we are looking for is usually the last reference with that name, but this is not
	// necessarily true for the figures.
	std::vector<IndexedData> generic_indices;
	std::map<std::wstring, IndexedData> reference_indices;
	std::map<std::wstring, IndexedData> equation_indices;

	std::mutex figure_indices_mutex;
	std::optional<std::thread> figure_indexing_thread = {};
	bool is_figure_indexing_required = true;
	bool is_indexing = false;
	bool are_highlights_loaded = false;

	QDateTime last_update_time;
	CachedChecksummer* checksummer;

	// we do some of the document processing in a background thread (for example indexing all the
	// figures/indices and computing page heights. we use this pointer to notify the main thread when
	// processing is complete.
	bool* invalid_flag_pointer = nullptr;

	int get_mark_index(char symbol);
	fz_outline* get_toc_outline();

	// load marks, bookmarks, links, etc.
	void load_document_metadata_from_db();

	// convetr the fz_outline structure to our own TocNode structure
	void create_toc_tree(std::vector<TocNode*>& toc);

	Document(fz_context* context, std::wstring file_name, DatabaseManager* db_manager, CachedChecksummer* checksummer);
public:
	fz_document* doc = nullptr;

	void add_bookmark(const std::wstring& desc, float y_offset);
	void add_highlight(const std::wstring& desc, const std::vector<fz_rect>& highlight_rects, fz_point selection_begin, fz_point selection_end, char type);
	void fill_highlight_rects(fz_context* ctx);
	void count_chapter_pages(std::vector<int> &page_counts);
	void convert_toc_tree(fz_outline* root, std::vector<TocNode*>& output);
	void count_chapter_pages_accum(std::vector<int> &page_counts);
	bool get_is_indexing();
	fz_stext_page* get_stext_with_page_number(fz_context* ctx, int page_number);
	fz_stext_page* get_stext_with_page_number(int page_number);
	void add_link(Link link, bool insert_into_database = true);
	std::wstring get_path();
	std::string get_checksum();
	int find_closest_bookmark_index(float to_offset_y);
	std::optional<Link> find_closest_link(float to_offset_y, int* index = nullptr);
	bool update_link(Link new_link);
	void delete_closest_bookmark(float to_y_offset);
	void delete_highlight_with_index(int index);
	void delete_highlight_with_offsets(float begin_x, float begin_y, float end_x, float end_y);
	void delete_closest_link(float to_offset_y);
	const std::vector<BookMark>& get_bookmarks() const;
	const std::vector<Highlight>& get_highlights() const;
	const std::vector<Highlight> get_highlights_sorted() const;
	fz_link* get_page_links(int page_number);
	void add_mark(char symbol, float y_offset);
	bool remove_mark(char symbol);
	bool get_mark_location_if_exists(char symbol, float* y_offset);
	~Document();
	const std::vector<TocNode*>& get_toc();
	bool has_toc();
	const std::vector<std::wstring>& get_flat_toc_names();
	const std::vector<int>& get_flat_toc_pages();
	bool open(bool* invalid_flag, bool force_load_dimensions=false);
	void reload();
	QDateTime get_last_edit_time();
	unsigned int get_milies_since_last_document_update_time();
	unsigned int get_milies_since_last_edit_time();
	float get_page_height(int page_index);
	fz_pixmap* get_small_pixmap(int page);
	float get_page_width(int page_index);
	float get_page_width_smart(int page_index, float* left_ratio, float* right_ratio, int* normal_page_width);
	float get_accum_page_height(int page_index);
	//const vector<float>& get_page_heights();
	//const vector<float>& get_page_widths();
	//const vector<float>& get_accum_page_heights();
	void get_visible_pages(float doc_y_range_begin, float doc_y_range_end, std::vector<int>& visible_pages);
	void load_page_dimensions(bool force_load_now);
	int num_pages();
	fz_rect get_page_absolute_rect(int page);
	void absolute_to_page_pos(float absolute_x, float absolute_y, float* doc_x, float* doc_y, int* doc_page);
	QStandardItemModel* get_toc_model();
	void page_pos_to_absolute_pos(int page, float page_x, float page_y, float* abs_x, float* abs_y);
	fz_rect page_rect_to_absolute_rect(int page, fz_rect page_rect);
	int get_offset_page_number(float y_offset);
	void index_figures(bool* invalid_flag);
	void stop_indexing();
	std::optional<IndexedData> find_reference_with_string(std::wstring reference_name);
	std::optional<IndexedData> find_equation_with_string(std::wstring equation_name);

	std::optional<std::wstring> get_text_at_position(const std::vector<fz_stext_char*>& flat_chars, float offset_x, float offset_y);
	std::optional<std::wstring> get_reference_text_at_position(const std::vector<fz_stext_char*>& flat_chars, float offset_x, float offset_y);
	std::optional<std::wstring> get_paper_name_at_position(const std::vector<fz_stext_char*>& flat_chars, float offset_x, float offset_y);
	std::optional<std::wstring> get_equation_text_at_position(const std::vector<fz_stext_char*>& flat_chars, float offset_x, float offset_y);
	std::optional<std::pair<std::wstring, std::wstring>> get_generic_link_name_at_position(const std::vector<fz_stext_char*>& flat_chars, float offset_x, float offset_y);
	std::optional<std::wstring> get_regex_match_at_position(const std::wregex& regex, const std::vector<fz_stext_char*>& flat_chars, float offset_x, float offset_y);
	bool find_generic_location(const std::wstring& type, const std::wstring& name, int* page, float* y_offset);
	bool can_use_highlights();

	void get_text_selection(fz_point selection_begin,
		fz_point selection_end,
		bool is_word_selection, // when in word select mode, we select entire words even if the range only partially includes the word
		std::vector<fz_rect>& selected_characters,
		std::wstring& selected_text);
	void get_text_selection(fz_context* ctx, fz_point selection_begin,
		fz_point selection_end,
		bool is_word_selection,
		std::vector<fz_rect>& selected_characters,
		std::wstring& selected_text);

	friend class DocumentManager;
};

class DocumentManager {
private:
	fz_context* mupdf_context = nullptr;
	DatabaseManager* db_manager = nullptr;
	CachedChecksummer* checksummer;
	std::unordered_map<std::wstring, Document*> cached_documents;
	std::unordered_map<std::string, std::wstring> hash_to_path;
public:

	DocumentManager(fz_context* mupdf_context, DatabaseManager* db_manager, CachedChecksummer* checksummer);

	Document* get_document(const std::wstring& path);
	const std::unordered_map<std::wstring, Document*>& get_cached_documents();
	void delete_global_mark(char symbol);
};
