#pragma once

#include <vector>
#include <string>
#include <assert.h>
#include <utility>
#include <algorithm>
#include <thread>
#include <optional>
#include <deque>

#include <mupdf/fitz.h>
#include "sqlite3.h"

#include "coordinates.h"
#include "book.h"

extern float ZOOM_INC_FACTOR;
extern const int PAGE_PADDINGS;

class CachedChecksummer;
class Document;
class DatabaseManager;
class DocumentManager;
class ConfigManager;

class DocumentView {
protected:

    DatabaseManager* db_manager = nullptr;
    DocumentManager* document_manager = nullptr;
    CachedChecksummer* checksummer;
    Document* current_document = nullptr;

    float zoom_level = 0.0f;
    float offset_x = 0.0f;
    float offset_y = 0.0f;

    // absolute rect of the current ruler if this is {} then ruler_pos is used instead
    std::optional<AbsoluteRect> ruler_rect;
    float ruler_pos = 0;

    // index of the current highlighted line in ruler mode
    int line_index = -1;

    int view_width = 0;
    int view_height = 0;
    bool is_relenting = false;

    // in auto resize mode, we automatically set the zoom level to fit the page when resizing the document
    bool is_auto_resize_mode = true;
    bool is_ruler_mode_ = false;
    std::optional<int> presentation_page_number;

public:
    // list of selected characters (e.g. using mouse select) to be highlighted
    std::deque<AbsoluteRect> selected_character_rects;
    // whether we should show a keyboard text selection marker at the end/begin of current
    // text selection (depending on `mark_end` value)
    bool should_show_text_selection_marker = false;
    bool mark_end = true;

    DocumentView(DatabaseManager* db_manager, DocumentManager* document_manager, CachedChecksummer* checksummer);
    ~DocumentView();
    float get_zoom_level();
    DocumentViewState get_state();
    PortalViewState get_checksum_state();
    //void set_opened_book_state(const OpenedBookState& state);
    void handle_escape();
    void set_book_state(OpenedBookState state);
    virtual bool set_offsets(float new_offset_x, float new_offset_y, bool force = false);
    bool set_pos(AbsoluteDocumentPos pos);
    Document* get_document();
    bool is_ruler_mode();
    void exit_ruler_mode();

    // find the closest portal to the current position
    // if limit is true, we only search for portals near the current location and not all portals
    std::optional<Portal> find_closest_portal(bool limit = false);
    std::optional<BookMark> find_closest_bookmark();
    void goto_portal(Portal* link);
    void delete_closest_portal();
    void delete_closest_bookmark();
    Highlight get_highlight_with_index(int index);
    void delete_highlight_with_index(int index);
    void delete_highlight(Highlight hl);
    void delete_all_highlights();
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
    void get_text_selection(AbsoluteDocumentPos selection_begin, AbsoluteDocumentPos selection_end, bool is_word_selection, std::deque<AbsoluteRect>& selected_characters, std::wstring& text_selection);
    void add_mark(char symbol);
    std::string add_bookmark(std::wstring desc);
    std::string add_highlight(AbsoluteDocumentPos selection_begin, AbsoluteDocumentPos selection_end, char type);
    void on_view_size_change(int new_width, int new_height);
    //void absolute_to_window_pos(float absolute_x, float absolute_y, float* window_x, float* window_y);
    NormalizedWindowPos absolute_to_window_pos(AbsoluteDocumentPos absolute_pos);

    NormalizedWindowRect absolute_to_window_rect(AbsoluteRect doc_rect);
    NormalizedWindowPos document_to_window_pos(DocumentPos pos);
    WindowPos absolute_to_window_pos_in_pixels(AbsoluteDocumentPos abs_pos);
    WindowPos document_to_window_pos_in_pixels_uncentered(DocumentPos doc_pos);
    WindowPos document_to_window_pos_in_pixels_banded(DocumentPos doc_pos);
    NormalizedWindowRect document_to_window_rect(DocumentRect doc_rect);
    WindowRect document_to_window_irect(DocumentRect);
    NormalizedWindowRect document_to_window_rect_pixel_perfect(DocumentRect doc_rect, int pixel_width, int pixel_height, bool banded = false);
    DocumentPos window_to_document_pos(WindowPos window_pos);
    DocumentPos window_to_document_pos_uncentered(WindowPos window_pos);
    AbsoluteDocumentPos window_to_absolute_document_pos(WindowPos window_pos);
    NormalizedWindowPos window_to_normalized_window_pos(WindowPos window_pos);
    WindowPos normalized_window_to_window_pos(NormalizedWindowPos normalized_window_pos);
    WindowRect normalized_to_window_rect(NormalizedWindowRect normalized_rect);
    void goto_mark(char symbol);
    void goto_end();

    void goto_left();
    void goto_left_smart();

    void goto_right();
    void goto_right_smart();

    float get_max_valid_x(bool relenting);
    float get_min_valid_x(bool relenting);

    virtual float set_zoom_level(float zl, bool should_exit_auto_resize_mode);
    virtual float zoom_in(float zoom_factor = ZOOM_INC_FACTOR);
    virtual float zoom_out(float zoom_factor = ZOOM_INC_FACTOR);
    float zoom_in_cursor(WindowPos mouse_pos, float zoom_factor = ZOOM_INC_FACTOR);
    float zoom_out_cursor(WindowPos mouse_pos, float zoom_factor = ZOOM_INC_FACTOR);
    bool move_absolute(float dx, float dy, bool force = false);
    bool move(float dx, float dy, bool force = false);
    void get_absolute_delta_from_doc_delta(float doc_dx, float doc_dy, float* abs_dx, float* abs_dy);
    int get_center_page_number();
    void get_visible_pages(int window_height, std::vector<int>& visible_pages);
    void move_pages(int num_pages);
    void move_screens(int num_screens);
    void reset_doc_state();
    void open_document(const std::wstring& doc_path, bool* invalid_flag, bool load_prev_state = true, std::optional<OpenedBookState> prev_state = {}, bool foce_load_dimensions = false);
    float get_page_offset(int page);
    void goto_offset_within_page(int page, float offset_y);
    void goto_page(int page);
    void fit_to_page_width(bool smart = false, bool ratio = false);
    void fit_to_page_height(bool smart = false);
    void fit_to_page_height_and_width_smart();
    void fit_to_page_height_width_minimum(int statusbar_height);
    void persist(bool persist_drawings = false);
    std::wstring get_current_chapter_name();
    std::optional<AbsoluteRect> get_control_rect();
    std::optional<std::pair<int, int>> get_current_page_range();
    int get_current_chapter_index();
    void goto_chapter(int diff);
    void get_page_chapter_index(int page, std::vector<TocNode*> toc_nodes, std::vector<int>& res);
    std::vector<int> get_current_chapter_recursive_index();
    float view_height_in_document_space();
    void set_vertical_line_pos(float pos);
    bool has_ruler_rect();
    std::optional<AbsoluteRect> get_ruler_rect();
    //float get_vertical_line_pos();
    float get_ruler_pos();

    //float get_vertical_line_window_y();
    float get_ruler_window_y();
    std::optional<NormalizedWindowRect> get_ruler_window_rect();

    void goto_vertical_line_pos();
    int get_page_offset();
    void set_page_offset(int new_offset);
    void rotate();
    void goto_top_of_page();
    void goto_bottom_of_page();
    int get_line_index_of_vertical_pos();
    int get_line_index_of_pos(DocumentPos docpos);
    int get_line_index();
    void set_line_index(int index, int page);
    int get_vertical_line_page();
    bool goto_definition();
    std::vector<SmartViewCandidate> find_line_definitions();
    std::optional<std::wstring> get_selected_line_text();
    bool get_is_auto_resize_mode();
    void disable_auto_resize_mode();
    void readjust_to_screen();
    float get_half_screen_offset();
    void scroll_mid_to_top();
    void get_visible_links(std::vector<PdfLink>& visible_page_links);
    void set_text_mark(bool is_begin);
    void toggle_text_mark();
    void get_rects_from_ranges(int page_number, const std::vector<PagelessDocumentRect>& line_char_rects, const std::vector<std::pair<int, int>>& ranges, std::vector<PagelessDocumentRect>& out_rects);
    std::optional<AbsoluteRect> expand_selection(bool is_begin, bool word);
    std::optional<AbsoluteRect> shrink_selection(bool is_begin, bool word);
    std::deque<AbsoluteRect>* get_selected_character_rects();

    std::vector<int> get_visible_highlight_indices();
    void set_presentation_page_number(std::optional<int> page);
    std::optional<int> get_presentation_page_number();
    bool is_presentation_mode();

};


class ScratchPad : public DocumentView {
private:
    std::vector<FreehandDrawing> all_drawings;
    std::vector<FreehandDrawing> non_compiled_drawings;
    bool is_compile_valid = false;
public:

    std::vector<PixmapDrawing> pixmaps;
    std::optional<CompiledDrawingData> cached_compiled_drawing_data = {};

    ScratchPad();
    bool set_offsets(float new_offset_x, float new_offset_y, bool force = false);
    float set_zoom_level(float zl, bool should_exit_auto_resize_mode);
    float zoom_in(float zoom_factor = ZOOM_INC_FACTOR);
    float zoom_out(float zoom_factor = ZOOM_INC_FACTOR);

    std::vector<int> get_intersecting_drawing_indices(AbsoluteRect selection);
    std::vector<int> get_intersecting_pixmap_indices(AbsoluteRect selection);
    std::vector<SelectedObjectIndex> get_intersecting_objects(AbsoluteRect selection);
    void delete_intersecting_drawings(AbsoluteRect selection);
    void delete_intersecting_pixmaps(AbsoluteRect selection);
    void delete_intersecting_objects(AbsoluteRect selection);
    void get_selected_objects_with_indices(const std::vector<SelectedObjectIndex>& indices, std::vector<FreehandDrawing>& freehand_drawings, std::vector<PixmapDrawing>& pixmap_drawings);
    void add_pixmap(QPixmap pixmap);
    AbsoluteRect get_bounding_box();

    const std::vector<FreehandDrawing>& get_all_drawings();
    const std::vector<FreehandDrawing>& get_non_compiled_drawings();
    void on_compile();
    void invalidate_compile(bool force=false);
    void add_drawing(FreehandDrawing drawing);
    void clear();
    bool is_compile_invalid();

};
