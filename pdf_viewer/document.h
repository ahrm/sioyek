#pragma once
#include <vector>
#include <string>
#include <optional>
#include <iostream>
#include <thread>
#include <mutex>
#include <map>
#include <unordered_map>
#include <deque>
#include <regex>

//#include <Windows.h>
#include <qstandarditemmodel.h>
#include <qdatetime.h>

#include <mupdf/fitz.h>
#include <qobject.h>
#include <qnetworkreply.h>
#include <qjsondocument.h>
#include <qurlquery.h>

#include "book.h"
#include "coordinates.h"

class CachedChecksummer;
class DatabaseManager;

class CharacterIterator {
    fz_stext_block* block = nullptr;
    fz_stext_line* line = nullptr;
    fz_stext_char* chr = nullptr;

public:
    CharacterIterator(fz_stext_page* page);
    CharacterIterator(fz_stext_block* b, fz_stext_line* l, fz_stext_char* c);
    CharacterIterator& operator++();
    CharacterIterator operator++(int);
    bool operator==(const CharacterIterator& other) const;
    bool operator!=(const CharacterIterator& other) const;
    std::tuple<fz_stext_block*, fz_stext_line*, fz_stext_char*> operator*() const;

    using difference_type = long;
    using value_type = std::tuple<fz_stext_block*, fz_stext_line*, fz_stext_char*>;
    using pointer = const std::tuple<fz_stext_block*, fz_stext_line*, fz_stext_char*>*;
    using reference = const std::tuple<fz_stext_block*, fz_stext_line*, fz_stext_char*>&;
    using iterator_category = std::forward_iterator_tag;
};

class PageIterator {
    fz_stext_page* page;
public:
    PageIterator(fz_stext_page* page);
    CharacterIterator begin() const;
    CharacterIterator end() const;
};

class Document {

private:

    std::mutex drawings_mutex;
    std::map<int, std::vector<FreehandDrawing>> page_freehand_drawings;
    // it means we have modified freehand drawings since the document was loaded
    // which means that when we exit, we must write the modified drawings to the drawings file
    bool is_drawings_dirty = false;
    bool is_annotations_dirty = false;

    std::vector<Mark> marks;
    std::vector<BookMark> bookmarks;
    std::vector<Highlight> highlights;
    std::vector<Portal> portals;
    DatabaseManager* db_manager = nullptr;
    std::vector<TocNode*> top_level_toc_nodes;
    //bool only_for_portal = true;

    // automatically generated table of contents entries
    std::vector<TocNode*> created_top_level_toc_nodes;
    // flattened table of contents entries when we don't want to (or can't)
    // show a tree view (e.g. due to performance reasons on PC and lack of availablity on mobile)
    std::vector<std::wstring> flat_toc_names;
    std::vector<int> flat_toc_pages;
    std::map<int, std::vector<AbsoluteRect>> cached_page_line_rects;
    std::map<int, std::vector<std::wstring>> cached_line_texts;

    bool super_fast_search_index_ready = false;
    // super fast index is the concatenated text of all pages along with two lists which map the
    // characters to pages and rects of those characters this index is built only if the 
    // super_fast_search config option is enabled
    std::wstring super_fast_search_index;
    std::vector<int> super_fast_search_index_pages;
    std::vector<PagelessDocumentRect> super_fast_search_rects;

    // DEPRECATED a page offset which could manually be set to make the page numbers correct
    // on PDF files with page numbers that start at a number other than 1. This is now
    // unnecessary because mupdf 1.22 allows us to get actual page labels
    int page_offset = 0;

    // number of pages in the document
    std::optional<int> cached_num_pages = {};

    std::vector<std::pair<int, fz_stext_page*>> cached_stext_pages;
    std::vector<std::pair<int, fz_pixmap*>> cached_small_pixmaps;
    std::map<int, std::optional<std::string>> cached_fastread_highlights;
    PdfLink merge_links(const std::vector<PdfLink>& links_to_merge);

    fz_context* context = nullptr;
    std::wstring file_name;
    std::unordered_map<int, fz_link*> cached_page_links;
    std::unordered_map<int, std::vector<PdfLink>> cached_merged_pdf_links;
    std::unordered_map<int, std::vector<PagelessDocumentRect>> cached_flat_words;
    std::unordered_map<int, std::vector<std::vector<PagelessDocumentRect>>> cached_flat_word_chars;
    QStandardItemModel* cached_toc_model = nullptr;

    // accumulated page heights (i.e. the height of the page plus the height of all the pages before it)
    std::vector<float> accum_page_heights;
    std::vector<float> page_heights;
    std::vector<float> page_widths;
    std::wstring detected_paper_name = L"";

    // label of the pages, e.g. "i", "ii", "iii", "1", "2", "3", etc.
    std::vector<std::wstring> page_labels;
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
    std::map<std::wstring, std::vector<IndexedData>> equation_indices;

    std::mutex document_indexing_mutex;
    std::optional<std::thread> document_indexing_thread = {};
    bool is_document_indexing_required = true;
    bool is_indexing = false;
    bool are_highlights_loaded = false;
    bool should_render_annotations = true;
    bool should_reload_annotations = false;

    QDateTime last_update_time;
    CachedChecksummer* checksummer;

    // we do some of the document processing in a background thread (for example indexing all the
    // figures/indices and computing page heights. we use this pointer to notify the main thread when
    // processing is complete.
    bool* invalid_flag_pointer = nullptr;

    int get_mark_index(char symbol);
    fz_outline* get_toc_outline();

    // load marks, bookmarks, links, etc.

    // convetr the fz_outline structure to our own TocNode structure
    void create_toc_tree(std::vector<TocNode*>& toc);

    Document(fz_context* context, std::wstring file_name, DatabaseManager* db_manager, CachedChecksummer* checksummer);
    void clear_toc_nodes();
    void clear_toc_node(TocNode* node);
    int find_highlight_index_with_uuid(const std::string& uuid);
public:
    fz_document* doc = nullptr;

    PageIterator page_iterator(int page_number);
    void get_page_text_and_line_rects_after_rect(int page_number,
        AbsoluteRect after,
        std::wstring& text,
        std::vector<PagelessDocumentRect>& line_rects,
        std::vector<PagelessDocumentRect>& char_rects);
    void load_document_metadata_from_db();
    std::string add_bookmark(const std::wstring& desc, float y_offset);
    std::string add_marked_bookmark(const std::wstring& desc, AbsoluteDocumentPos pos);
    int add_incomplete_freetext_bookmark(AbsoluteRect absrect);
    std::string add_pending_freetext_bookmark(int index, const std::wstring& desc);
    void undo_pending_bookmark(int index);
    void add_freetext_bookmark(const std::wstring& desc, AbsoluteRect absrect);
    void add_freetext_bookmark_with_color(const std::wstring& desc, AbsoluteRect absrect, float* color, float font_size = -1);
    std::string add_highlight(const std::wstring& desc, const std::vector<AbsoluteRect>& highlight_rects, AbsoluteDocumentPos selection_begin, AbsoluteDocumentPos selection_end, char type);
    std::string add_highlight(const std::wstring& annot, AbsoluteDocumentPos selection_begin, AbsoluteDocumentPos selection_end, char type);
    void delete_highlight_with_index(int index);
    void delete_highlight(Highlight hl);
    void delete_all_highlights();
    int get_bookmark_index_at_pos(AbsoluteDocumentPos abspos);
    int get_portal_index_at_pos(AbsoluteDocumentPos abspos);
    bool should_render_pdf_annotations();
    void set_should_render_pdf_annotations(bool val);
    bool get_should_render_pdf_annotations();
    std::vector<Portal> get_intersecting_visible_portals(float absrange_begin, float absrange_end);

    void fill_highlight_rects(fz_context* ctx, fz_document* doc);
    void fill_index_highlight_rects(int highlight_index, fz_context* thread_context = nullptr, fz_document* thread_document = nullptr);
    void count_chapter_pages(std::vector<int>& page_counts);
    void convert_toc_tree(fz_outline* root, std::vector<TocNode*>& output);
    void count_chapter_pages_accum(std::vector<int>& page_counts);
    bool get_is_indexing();
    fz_stext_page* get_stext_with_page_number(fz_context* ctx, int page_number, fz_document* doc = nullptr);
    fz_stext_page* get_stext_with_page_number(int page_number);
    int add_portal(Portal link, bool insert_into_database = true);
    std::wstring get_path();
    std::string get_checksum();
    std::optional<std::string> get_checksum_fast();
    //int find_closest_bookmark_index(float to_offset_y);

    int find_closest_bookmark_index(const std::vector<BookMark>& sorted_bookmarks, float to_offset_y) const;
    int find_closest_portal_index(const std::vector<Portal>& sorted_bookmarks, float to_offset_y) const;
    int find_closest_highlight_index(const std::vector<Highlight>& sorted_highlights, float to_offset_y) const;

    std::optional<Portal> find_closest_portal(float to_offset_y, int* index = nullptr);
    bool update_portal(Portal new_link);
    void delete_closest_bookmark(float to_y_offset);
    void delete_bookmark(int index);
    void delete_closest_portal(float to_offset_y);
    int get_portal_index_with_uuid(const std::string& uuid);
    void delete_portal_with_uuid(const std::string& uuid);
    std::vector<BookMark>& get_bookmarks();
    std::vector<Portal>& get_portals();
    std::vector<BookMark> get_sorted_bookmarks() const;
    std::vector<Portal> get_sorted_portals() const;
    const std::vector<Highlight>& get_highlights() const;
    int get_highlight_index_with_uuid(std::string uuid);
    int get_bookmark_index_with_uuid(std::string uuid);
    const std::vector<Highlight> get_highlights_of_type(char type) const;
    const std::vector<Highlight> get_highlights_sorted(char type = 0) const;

    std::optional<Highlight> get_next_highlight(float abs_y, char type = 0, int offset = 0) const;
    std::optional<Highlight> get_prev_highlight(float abs_y, char type = 0, int offset = 0) const;

    fz_link* get_page_links(int page_number);
    const std::vector<PdfLink>& get_page_merged_pdf_links(int page_number);
    PdfLink pdf_link_from_fz_link(int page, fz_link* link);
    void add_mark(char symbol, float y_offset, std::optional<float> x_offset, std::optional<float> zoom_level);
    bool remove_mark(char symbol);
    std::optional<Mark> get_mark_if_exists(char symbol);
    ~Document();
    const std::vector<TocNode*>& get_toc();
    bool has_toc();
    const std::vector<std::wstring>& get_flat_toc_names();
    const std::vector<int>& get_flat_toc_pages();
    bool open(bool* invalid_flag, bool force_load_dimensions = false, std::string password = "", bool temp = false);
    void reload(std::string password = "");
    QDateTime get_last_edit_time();
    unsigned int get_milies_since_last_document_update_time();
    unsigned int get_milies_since_last_edit_time();
    float get_page_height(int page_index);
    fz_pixmap* get_small_pixmap(int page);
    float get_page_width(int page_index);
    std::wstring get_page_label(int page_index);
    int get_page_number_with_label(std::wstring page_label);
    bool is_reflowable();
    //float get_page_width_smart(int page_index, float* left_ratio, float* right_ratio, int* normal_page_width);
    float get_page_size_smart(bool width, int page_index, float* left_ratio, float* right_ratio, int* normal_page_width);
    float get_accum_page_height(int page_index);
    void rotate();
    void get_visible_pages(float doc_y_range_begin, float doc_y_range_end, std::vector<int>& visible_pages);
    void load_page_dimensions(bool force_load_now);
    int num_pages();
    AbsoluteRect get_page_absolute_rect(int page);
    DocumentPos absolute_to_page_pos(AbsoluteDocumentPos absolute_pos);
    DocumentPos absolute_to_page_pos_uncentered(AbsoluteDocumentPos absolute_pos);
    DocumentRect absolute_to_page_rect(AbsoluteRect abs_rect);
    QStandardItemModel* get_toc_model();
    int get_offset_page_number(float y_offset);
    void index_document(bool* invalid_flag);
    void stop_indexing();
    void delete_page_intersecting_drawings(int page, AbsoluteRect absolute_rect, bool mask[26]);
    void delete_all_page_drawings(int page);
    void delete_all_drawings();
    std::vector<SelectedObjectIndex> get_page_intersecting_drawing_indices(int page, AbsoluteRect absolute_rect, bool mask[26]);

    std::vector<IndexedData> find_reference_with_string(std::wstring reference_name, int page_number);
    std::vector<IndexedData> find_equation_with_string(std::wstring equation_name, int page_number);
    std::vector<IndexedData> find_generic_with_string(std::wstring equation_name, int page_number);

    std::optional<std::wstring> get_text_at_position(const std::vector<fz_stext_char*>& flat_chars, PagelessDocumentPos position);
    std::optional<std::wstring> get_reference_text_at_position(const std::vector<fz_stext_char*>& flat_chars, PagelessDocumentPos position, std::pair<int, int>* out_range);
    std::optional<std::wstring> get_paper_name_at_position(const std::vector<fz_stext_char*>& flat_chars, PagelessDocumentPos position);
    std::optional<std::wstring> get_equation_text_at_position(const std::vector<fz_stext_char*>& flat_chars, PagelessDocumentPos position, std::pair<int, int>* out_range);
    std::optional<std::pair<std::wstring, std::wstring>> get_generic_link_name_at_position(const std::vector<fz_stext_char*>& flat_chars, PagelessDocumentPos position, std::pair<int, int>* out_range);
    std::optional<std::wstring> get_regex_match_at_position(const std::wregex& regex, const std::vector<fz_stext_char*>& flat_chars, PagelessDocumentPos position, std::pair<int, int>* out_range);
    std::optional<std::wstring> get_text_at_position(DocumentPos position);
    std::optional<std::wstring> get_reference_text_at_position(DocumentPos position, std::pair<int, int>* out_range);
    std::optional<std::wstring> get_paper_name_at_position(DocumentPos position);
    std::optional<std::wstring> get_equation_text_at_position(DocumentPos position, std::pair<int, int>* out_range);
    std::optional<std::pair<std::wstring, std::wstring>> get_generic_link_name_at_position(DocumentPos position, std::pair<int, int>* out_range);
    std::optional<std::wstring> get_regex_match_at_position(const std::wregex& regex, DocumentPos position, std::pair<int, int>* out_range);
    std::vector<DocumentPos> find_generic_locations(const std::wstring& type, const std::wstring& name);
    bool can_use_highlights();

    std::vector<std::wstring> get_page_bib_candidates(int page_number, std::vector<PagelessDocumentRect>* out_end_rects = nullptr);
    std::optional<std::pair<std::wstring, PagelessDocumentRect>> get_page_bib_with_reference(int page_number, std::wstring reference_text);

    void get_text_selection(AbsoluteDocumentPos selection_begin,
        AbsoluteDocumentPos selection_end,
        bool is_word_selection, // when in word select mode, we select entire words even if the range only partially includes the word
        std::deque<AbsoluteRect>& selected_characters,
        std::wstring& selected_text);
    void get_text_selection(fz_context* ctx, AbsoluteDocumentPos selection_begin,
        AbsoluteDocumentPos selection_end,
        bool is_word_selection,
        std::deque<AbsoluteRect>& selected_characters,
        std::wstring& selected_text,
        fz_document* doc = nullptr);

    bool is_bookmark_new(const BookMark& bookmark);
    bool is_highlight_new(const Highlight& highlight);
    bool is_drawing_new(const FreehandDrawing& drawing);
    int num_freehand_drawings();
    std::vector<BookMark> get_new_sioyek_bookmarks(const std::vector<BookMark>& pdf_bookmarks);
    std::vector<Highlight> get_new_sioyek_highlights(const std::vector<Highlight>& pdf_highlights);
    fz_context* get_mupdf_context();

    int get_page_offset();
    void set_page_offset(int new_offset);
    void embed_annotations(std::wstring new_file_path);
    void get_pdf_annotations(std::vector<BookMark>& pdf_bookmarks, std::vector<Highlight>& pdf_highlights, std::vector<FreehandDrawing>& pdf_drawings);
    void import_annotations();
    std::vector<PagelessDocumentRect> get_page_flat_words(int page);
    std::vector<std::vector<PagelessDocumentRect>> get_page_flat_word_chars(int page);
    void clear_document_caches();
    void load_document_caches(bool* invalid_flag, bool force_now);
    int reflow(int page);
    void update_highlight_add_text_annotation(const std::string& uuid, const std::wstring& text_annot);
    void update_highlight_type(const std::string& uuid, char new_type);
    void update_highlight_type(int index, char new_type);
    void update_bookmark_text(int index, const std::wstring& new_text, float new_font_size);
    void update_bookmark_position(int index, AbsoluteDocumentPos new_begin_position, AbsoluteDocumentPos new_end_position);
    void update_portal_src_position(int index, AbsoluteDocumentPos new_position);

    bool needs_password();
    bool needs_authentication();
    bool apply_password(const char* password);
    //std::optional<std::string> get_page_fastread_highlights(int page);
    std::vector<PagelessDocumentRect> get_highlighted_character_masks(int page);
    PagelessDocumentRect get_page_rect_no_cache(int page);
    std::optional<PdfLink> get_link_in_pos(int page, float x, float y);
    std::optional<PdfLink> get_link_in_pos(const DocumentPos& pos);
    std::vector<PdfLink> get_links_in_page_rect(int page, AbsoluteRect rect);
    std::wstring get_pdf_link_text(PdfLink link);
    std::string get_highlight_index_uuid(int index);
    std::string get_bookmark_index_uuid(int index);

    //void create_table_of_contents(std::vector<TocNode*>& top_nodes);
    int add_stext_page_to_created_toc(fz_stext_page* stext_page,
        int page_number,
        std::vector<TocNode*>& toc_node_stack,
        std::vector<TocNode*>& top_level_node);

    float document_to_absolute_y(int page, float doc_y);
    //AbsoluteDocumentPos document_to_absolute_pos(DocumentPos, bool center_mid = false);
    AbsoluteDocumentPos document_to_absolute_pos(DocumentPos docpos);

    AbsoluteRect document_to_absolute_rect(DocumentRect doc_rect);

    //void get_ith_next_line_from_absolute_y(float absolute_y, int i, bool cont, float* out_begin, float* out_end);
    AbsoluteRect get_ith_next_line_from_absolute_y(int page, int line_index, int i, bool continue_to_next_page, int* out_index, int* out_page);
    const std::vector<AbsoluteRect>& get_page_lines(
        int page,
        std::vector<std::wstring>* line_texts = nullptr,
        std::vector<std::vector<PagelessDocumentRect>>* out_line_rects = nullptr);

    std::wstring get_drawings_file_path();
    std::wstring get_scratchpad_file_path();
    std::wstring get_annotations_file_path();
    bool annotations_file_exists();
    bool annotations_file_is_newer_than_database();
    std::optional<AbsoluteRect> get_rect_vertically(bool below, AbsoluteRect rect);

    void persist_drawings(bool force = false);
    void persist_annotations(bool force = false);
    void load_drawings();
    void load_annotations(bool sync = false);
    void load_drawings_async();
    //void persist_drawings_async();

    std::wstring detect_paper_name(fz_context* context, fz_document* doc);
    std::wstring detect_paper_name();
    //void set_only_for_portal(bool val);
    //bool get_only_for_portal();

    bool is_super_fast_index_ready();
    std::vector<SearchResult> search_text(std::wstring query, SearchCaseSensitivity case_sensitive, int begin_page, int min_page, int max_page);
    std::vector<SearchResult> search_regex(std::wstring query, SearchCaseSensitivity case_sensitive, int begin_page, int min_page, int max_page);
    float max_y_offset();
    void add_freehand_drawing(FreehandDrawing new_drawing);
    void get_page_freehand_drawings_with_indices(int page, const std::vector<SelectedObjectIndex>& indices, std::vector<FreehandDrawing>& freehand_drawings, std::vector<PixmapDrawing>& pixmap_drawings);
    void undo_freehand_drawing();
    const std::vector<FreehandDrawing>& get_page_drawings(int page);
    AbsoluteRect to_absolute(int page, fz_quad quad);
    AbsoluteRect to_absolute(int page, PagelessDocumentRect rect);

    bool get_should_reload_annotations();
    void reload_annotations_on_new_checksum();
    int find_reference_page_with_reference_text(std::wstring query);
    std::optional<DocumentPos> find_abbreviation(std::wstring abbr, std::vector<DocumentRect>& overview_highlight_rects);

    QJsonArray get_bookmarks_json();
    QJsonArray get_highlights_json();
    QJsonArray get_portals_json();
    QJsonArray get_marks_json();

    friend class DocumentManager;
};

class DocumentManager {
private:
    fz_context* mupdf_context = nullptr;
    DatabaseManager* db_manager = nullptr;
    CachedChecksummer* checksummer;
    std::unordered_map<std::wstring, Document*> cached_documents;
    std::unordered_map<std::string, std::wstring> hash_to_path;
    std::vector<std::wstring> tabs;
public:

    DocumentManager(fz_context* mupdf_context, DatabaseManager* db_manager, CachedChecksummer* checksummer);

    int get_tab_index(const std::wstring& path);
    int add_tab(const std::wstring& path);
    void remove_tab(const std::wstring& path);
    std::vector<std::wstring> get_tabs();

    Document* get_document(const std::wstring& path);
    std::optional<std::wstring> get_path_from_hash(const std::string& checksum);

    Document* get_document_with_checksum(const std::string& checksum);
    std::optional<Document*> get_cached_document(const std::wstring& path);
    void free_document(Document* document);
    const std::unordered_map<std::wstring, Document*>& get_cached_documents();
    std::vector<std::wstring> get_loaded_document_paths();
    void delete_global_mark(char symbol);
    ~DocumentManager();
};
