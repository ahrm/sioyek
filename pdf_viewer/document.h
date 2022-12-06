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
#include <qnetworkreply.h>
#include <qjsondocument.h>
#include <qurlquery.h>

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
	std::vector<Portal> portals;
	DatabaseManager* db_manager = nullptr;
	std::vector<TocNode*> top_level_toc_nodes;
	std::vector<TocNode*> created_top_level_toc_nodes;
	std::vector<std::wstring> flat_toc_names;
	std::vector<int> flat_toc_pages;
	std::map<int, std::vector<fz_rect>> cached_page_line_rects;
	std::map<int, std::vector<std::wstring>> cached_line_texts;

	bool super_fast_search_index_ready = false;
	std::wstring super_fast_search_index;
	std::vector<int> super_fast_search_index_pages;
	std::vector<fz_rect> super_fast_search_rects;

	int page_offset = 0;


	// number of pages in the document
	std::optional<int> cached_num_pages = {};

	std::vector<std::pair<int, fz_stext_page*>> cached_stext_pages;
	std::vector<std::pair<int, fz_pixmap*>> cached_small_pixmaps;
	std::map<int, std::optional<std::string>> cached_fastread_highlights;

	fz_context* context = nullptr;
	std::wstring file_name;
	std::unordered_map<int, fz_link*> cached_page_links;
	std::unordered_map<int, std::vector<fz_rect>> cached_flat_words;
	std::unordered_map<int, std::vector<std::vector<fz_rect>>> cached_flat_word_chars;
	QStandardItemModel* cached_toc_model = nullptr;

	std::vector<float> accum_page_heights;
	std::vector<float> page_heights;
	std::vector<float> page_widths;
	std::mutex page_dims_mutex;
	std::string correct_password = "";
	bool password_was_correct = false;
	bool document_needs_password = false;

	// These are a heuristic index of all figures and references in the document
	// The reason that we use a hashmap for reference_indices and a vector for figures is that
	// the reference we are looking for is usually the last reference with that name, but this is not
	// necessarily true for the figures.
	std::vector<IndexedData> generic_indices;
	std::map<std::wstring, IndexedData> reference_indices;
	//std::map<std::wstring, IndexedData> equation_indices;
	std::map<std::wstring, std::vector<IndexedData>> equation_indices;

	std::mutex document_indexing_mutex;
	std::optional<std::thread> document_indexing_thread = {};
	bool is_document_indexing_required = true;
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
	void clear_toc_nodes();
	void clear_toc_node(TocNode* node);
public:
	fz_document* doc = nullptr;

	void add_bookmark(const std::wstring& desc, float y_offset);
	//void add_bookmark_annotation(const BookMark& bookmark);
	//void delete_bookmark_annotation(const BookMark& bookmark);
	void add_highlight(const std::wstring& desc, const std::vector<fz_rect>& highlight_rects, AbsoluteDocumentPos selection_begin, AbsoluteDocumentPos selection_end, char type);
	//void add_highlight_annotation(const Highlight& highlight, const std::vector<fz_rect>& selected_rects);
	void delete_highlight_with_index(int index);
	void delete_highlight(Highlight hl);
	//void delete_highlight_annotation(const Highlight& highlight);

	void fill_highlight_rects(fz_context* ctx, fz_document* doc);
	void count_chapter_pages(std::vector<int> &page_counts);
	void convert_toc_tree(fz_outline* root, std::vector<TocNode*>& output);
	void count_chapter_pages_accum(std::vector<int> &page_counts);
	bool get_is_indexing();
	fz_stext_page* get_stext_with_page_number(fz_context* ctx, int page_number, fz_document* doc=nullptr);
	fz_stext_page* get_stext_with_page_number(int page_number);
	void add_portal(Portal link, bool insert_into_database = true);
	std::wstring get_path();
	std::string get_checksum();
	std::optional<std::string> get_checksum_fast();
	//int find_closest_bookmark_index(float to_offset_y);

	int find_closest_bookmark_index(const std::vector<BookMark>& sorted_bookmarks, float to_offset_y) const;
	int find_closest_highlight_index(const std::vector<Highlight>& sorted_highlights, float to_offset_y) const;

	std::optional<Portal> find_closest_portal(float to_offset_y, int* index = nullptr);
	bool update_portal(Portal new_link);
	void delete_closest_bookmark(float to_y_offset);
	void delete_closest_portal(float to_offset_y);
	const std::vector<BookMark>& get_bookmarks() const;
	std::vector<BookMark> get_sorted_bookmarks() const;
	const std::vector<Highlight>& get_highlights() const;
	const std::vector<Highlight> get_highlights_of_type(char type) const;
	const std::vector<Highlight> get_highlights_sorted(char type=0) const;

	std::optional<Highlight> get_next_highlight(float abs_y, char type=0, int offset=0) const;
	std::optional<Highlight> get_prev_highlight(float abs_y, char type=0, int offset=0) const;

	fz_link* get_page_links(int page_number);
	void add_mark(char symbol, float y_offset);
	bool remove_mark(char symbol);
	bool get_mark_location_if_exists(char symbol, float* y_offset);
	~Document();
	const std::vector<TocNode*>& get_toc();
	bool has_toc();
	const std::vector<std::wstring>& get_flat_toc_names();
	const std::vector<int>& get_flat_toc_pages();
	bool open(bool* invalid_flag, bool force_load_dimensions=false, std::string password="", bool temp=false);
	void reload(std::string password="");
	QDateTime get_last_edit_time();
	unsigned int get_milies_since_last_document_update_time();
	unsigned int get_milies_since_last_edit_time();
	float get_page_height(int page_index);
	fz_pixmap* get_small_pixmap(int page);
	float get_page_width(int page_index);
	//float get_page_width_smart(int page_index, float* left_ratio, float* right_ratio, int* normal_page_width);
	float get_page_size_smart(bool width, int page_index, float* left_ratio, float* right_ratio, int* normal_page_width);
	float get_accum_page_height(int page_index);
	void rotate();
	void get_visible_pages(float doc_y_range_begin, float doc_y_range_end, std::vector<int>& visible_pages);
	void load_page_dimensions(bool force_load_now);
	int num_pages();
	fz_rect get_page_absolute_rect(int page);
	DocumentPos absolute_to_page_pos(AbsoluteDocumentPos absolute_pos);
	fz_rect absolute_to_page_rect(const fz_rect& absolute_rect, int* page);
	QStandardItemModel* get_toc_model();
	int get_offset_page_number(float y_offset);
	void index_document(bool* invalid_flag);
	void stop_indexing();
	std::optional<IndexedData> find_reference_with_string(std::wstring reference_name);
	std::optional<IndexedData> find_equation_with_string(std::wstring equation_name, int page_number);

	std::optional<std::wstring> get_text_at_position(const std::vector<fz_stext_char*>& flat_chars, float offset_x, float offset_y);
	std::optional<std::wstring> get_reference_text_at_position(const std::vector<fz_stext_char*>& flat_chars, float offset_x, float offset_y);
	std::optional<std::wstring> get_paper_name_at_position(const std::vector<fz_stext_char*>& flat_chars, float offset_x, float offset_y);
	std::optional<std::wstring> get_equation_text_at_position(const std::vector<fz_stext_char*>& flat_chars, float offset_x, float offset_y);
	std::optional<std::pair<std::wstring, std::wstring>> get_generic_link_name_at_position(const std::vector<fz_stext_char*>& flat_chars, float offset_x, float offset_y);
	std::optional<std::wstring> get_regex_match_at_position(const std::wregex& regex, const std::vector<fz_stext_char*>& flat_chars, float offset_x, float offset_y);
	std::optional<std::wstring> get_text_at_position(int page, float offset_x, float offset_y);
	std::optional<std::wstring> get_reference_text_at_position(int page, float offset_x, float offset_y);
	std::optional<std::wstring> get_paper_name_at_position(int page, float offset_x, float offset_y);
	std::optional<std::wstring> get_equation_text_at_position(int page, float offset_x, float offset_y);
	std::optional<std::pair<std::wstring, std::wstring>> get_generic_link_name_at_position(int page, float offset_x, float offset_y);
	std::optional<std::wstring> get_regex_match_at_position(const std::wregex& regex, int page, float offset_x, float offset_y);
	std::vector<DocumentPos> find_generic_locations(const std::wstring& type, const std::wstring& name);
	bool can_use_highlights();

	void get_text_selection(AbsoluteDocumentPos selection_begin,
		AbsoluteDocumentPos selection_end,
		bool is_word_selection, // when in word select mode, we select entire words even if the range only partially includes the word
		std::vector<fz_rect>& selected_characters,
		std::wstring& selected_text);
	void get_text_selection(fz_context* ctx, AbsoluteDocumentPos selection_begin,
		AbsoluteDocumentPos selection_end,
		bool is_word_selection,
		std::vector<fz_rect>& selected_characters,
		std::wstring& selected_text,
		fz_document* doc=nullptr);

	int get_page_offset();
	void set_page_offset(int new_offset);
	void embed_annotations(std::wstring new_file_path);
	std::vector<fz_rect> get_page_flat_words(int page);
	std::vector<std::vector<fz_rect>> get_page_flat_word_chars(int page);

	bool needs_password();
	bool needs_authentication();
	bool apply_password(const char* password);
	//std::optional<std::string> get_page_fastread_highlights(int page);
	std::vector<fz_rect> get_highlighted_character_masks(int page);
	fz_rect get_page_rect_no_cache(int page);
	std::optional<PdfLink> get_link_in_pos(int page, float x, float y);
	std::optional<PdfLink> get_link_in_pos(const DocumentPos& pos);
	std::optional<PdfLink> get_link_in_page_rect(int page, fz_rect rect);

	//void create_table_of_contents(std::vector<TocNode*>& top_nodes);
	int add_stext_page_to_created_toc(fz_stext_page* stext_page,
		int page_number,
		std::vector<TocNode*>& toc_node_stack,
		std::vector<TocNode*>& top_level_node);

	float document_to_absolute_y(int page, float doc_y);
	AbsoluteDocumentPos document_to_absolute_pos(DocumentPos, bool center_mid=false);
	fz_rect document_to_absolute_rect(int page, fz_rect doc_rect, bool center_mid=false);

	//void get_ith_next_line_from_absolute_y(float absolute_y, int i, bool cont, float* out_begin, float* out_end);
	fz_rect get_ith_next_line_from_absolute_y(int page, int line_index, int i, bool cont, int* out_index, int* out_page);
	const std::vector<fz_rect>& get_page_lines(int page, std::vector<std::wstring>* line_texts=nullptr);

	bool is_super_fast_index_ready();
	std::vector<SearchResult> search_text(std::wstring query, bool case_sensitive, int begin_page, int min_page, int max_page);
	std::vector<SearchResult> search_regex(std::wstring query, bool case_sensitive, int begin_page, int min_page, int max_page);
	float max_y_offset();

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
	void free_document(Document* document);
	const std::unordered_map<std::wstring, Document*>& get_cached_documents();
	void delete_global_mark(char symbol);
	~DocumentManager();
};
