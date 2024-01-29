#include <cmath>

#include "document_view.h"
#include "checksum.h"
#include "database.h"
#include "document.h"
#include "qlogging.h"
#include "utils.h"
#include "config.h"
#include "ui.h"

extern float MOVE_SCREEN_PERCENTAGE;
extern float FIT_TO_PAGE_WIDTH_RATIO;
extern float RULER_PADDING;
extern float RULER_X_PADDING;
extern bool EXACT_HIGHLIGHT_SELECT;
extern bool VERBOSE;


DocumentView::DocumentView(DatabaseManager* db_manager,
    DocumentManager* document_manager,
    CachedChecksummer* checksummer) :
    db_manager(db_manager),
    document_manager(document_manager),
    checksummer(checksummer)
{

}
DocumentView::~DocumentView() {
}

float DocumentView::get_zoom_level() {
    return zoom_level;
}

DocumentViewState DocumentView::get_state() {
    DocumentViewState res;

    if (current_document) {
        res.document_path = current_document->get_path();
        res.book_state.offset_x = get_offset_x();
        res.book_state.offset_y = get_offset_y();
        res.book_state.zoom_level = get_zoom_level();
        res.book_state.ruler_mode = is_ruler_mode_;
        res.book_state.ruler_pos = ruler_pos;
        res.book_state.ruler_rect = ruler_rect;
        res.book_state.line_index = line_index;
        res.book_state.presentation_page = presentation_page_number;
    }
    return res;
}

PortalViewState DocumentView::get_checksum_state() {
    PortalViewState res;

    if (current_document) {
        res.document_checksum = current_document->get_checksum();
        res.book_state.offset_x = get_offset_x();
        res.book_state.offset_y = get_offset_y();
        res.book_state.zoom_level = get_zoom_level();
    }
    return res;
}

//void DocumentView::set_opened_book_state(const OpenedBookState& state) {
//    set_offsets(state.offset_x, state.offset_y);
//    set_zoom_level(state.zoom_level, true);
//    is_ruler_mode_ = state.ruler_mode;
//    ruler_pos = state.ruler_pos;
//    ruler_rect = state.ruler_rect;
//
//}


void DocumentView::handle_escape() {
}

void DocumentView::exit_ruler_mode() {
    is_ruler_mode_ = false;
}

void DocumentView::set_book_state(OpenedBookState state) {
    set_offsets(state.offset_x, state.offset_y);
    set_zoom_level(state.zoom_level, true);
    presentation_page_number = state.presentation_page;
    is_ruler_mode_ = state.ruler_mode;
    ruler_pos = state.ruler_pos;
    ruler_rect = state.ruler_rect;
    line_index = state.line_index;
}

bool DocumentView::set_pos(AbsoluteDocumentPos pos) {
    return set_offsets(pos.x, pos.y);
}

bool DocumentView::set_offsets(float new_offset_x, float new_offset_y, bool force) {
    // if move was truncated
    bool truncated = false;

    if (current_document == nullptr) return false;

    int num_pages = current_document->num_pages();
    if (num_pages == 0) return false;

    float max_y_offset = current_document->get_accum_page_height(num_pages - 1) + current_document->get_page_height(num_pages - 1);
    float min_y_offset = 0;
    float min_x_offset_normal = get_min_valid_x(false);
    float max_x_offset_normal = get_max_valid_x(false);
    float min_x_offset_relenting = get_min_valid_x(true);
    float max_x_offset_relenting = get_max_valid_x(true);
    float max_x_offset = is_relenting ? max_x_offset_relenting : max_x_offset_normal;
    float min_x_offset = is_relenting ? min_x_offset_relenting : min_x_offset_normal;
    float relent_threshold = view_width / 4 / zoom_level;

    if (TOUCH_MODE) {
        if ((new_offset_x - max_x_offset_normal > relent_threshold) || (min_x_offset_normal - new_offset_x > relent_threshold)) {
            is_relenting = true;
        }
        else {
            is_relenting = false;
        }
    }

    if (!force) {
        if (new_offset_y > max_y_offset) { new_offset_y = max_y_offset; truncated = true; }
        if (new_offset_y < min_y_offset) { new_offset_y = min_y_offset; truncated = true; }
        if (new_offset_x > max_x_offset) { new_offset_x = max_x_offset; truncated = true; }
        if (new_offset_x < min_x_offset) { new_offset_x = min_x_offset; truncated = true; }
    }

    offset_x = new_offset_x;
    offset_y = new_offset_y;
    return truncated;
}

Document* DocumentView::get_document() {
    return current_document;
}

//int DocumentView::get_num_search_results() {
//	search_results_mutex.lock();
//	int num = search_results.size();
//	search_results_mutex.unlock();
//	return num;
//}
//
//int DocumentView::get_current_search_result_index() {
//	return current_search_result_index;
//}

std::optional<Portal> DocumentView::find_closest_portal(bool limit) {
    if (current_document) {
        auto res = current_document->find_closest_portal(offset_y);
        if (res) {
            if (!limit) {
                return res;
            }
            else {
                if (std::abs(res.value().src_offset_y - offset_y) < 500.0f) {
                    return res;
                }
            }
        }
    }
    return {};
}

std::optional<BookMark> DocumentView::find_closest_bookmark() {

    if (current_document) {
        int bookmark_index = current_document->find_closest_bookmark_index(current_document->get_bookmarks(), offset_y);
        const std::vector<BookMark>& bookmarks = current_document->get_bookmarks();
        if ((bookmark_index >= 0) && (bookmark_index < bookmarks.size())) {
            if (std::abs(bookmarks[bookmark_index].get_y_offset() - offset_y) < 1000.0f) {
                return bookmarks[bookmark_index];
            }
        }
    }
    return {};
}

void DocumentView::goto_portal(Portal* link) {
    if (link) {
        if (get_document() &&
            get_document()->get_checksum() == link->dst.document_checksum) {
            set_book_state(link->dst.book_state);
        }
        else {
            auto destination_path = checksummer->get_path(link->dst.document_checksum);
            if (destination_path) {
                open_document(destination_path.value(), nullptr);
                set_book_state(link->dst.book_state);
            }
        }
    }
}

void DocumentView::delete_closest_portal() {
    if (current_document) {
        current_document->delete_closest_portal(offset_y);
    }
}

void DocumentView::delete_closest_bookmark() {
    if (current_document) {
        delete_closest_bookmark_to_offset(offset_y);
    }
}

// todo: these should be in Document not here
Highlight DocumentView::get_highlight_with_index(int index) {
    return current_document->get_highlights()[index];
}

void DocumentView::delete_highlight_with_index(int index) {
    current_document->delete_highlight_with_index(index);
}

void DocumentView::delete_highlight(Highlight hl) {
    current_document->delete_highlight(hl);
}

void DocumentView::delete_all_highlights() {
    current_document->delete_all_highlights();
}

void DocumentView::delete_closest_bookmark_to_offset(float offset) {
    current_document->delete_closest_bookmark(offset);
}

float DocumentView::get_offset_x() {
    return offset_x;
}

float DocumentView::get_offset_y() {
    return offset_y;
}

AbsoluteDocumentPos DocumentView::get_offsets() {
    return { offset_x, offset_y };
}

int DocumentView::get_view_height() {
    return view_height;
}

int DocumentView::get_view_width() {
    return view_width;
}

void DocumentView::set_null_document() {
    current_document = nullptr;
}

void DocumentView::set_offset_x(float new_offset_x) {
    set_offsets(new_offset_x, offset_y);
}

void DocumentView::set_offset_y(float new_offset_y) {
    set_offsets(offset_x, new_offset_y);
}

std::optional<PdfLink> DocumentView::get_link_in_pos(WindowPos pos) {
    if (!current_document) return {};

    DocumentPos doc_pos = window_to_document_pos_uncentered(pos);
    return current_document->get_link_in_pos(doc_pos);
}

int DocumentView::get_highlight_index_in_pos(WindowPos window_pos) {
    //auto [view_x, view_y] = window_to_absolute_document_pos(window_pos);

    //fz_point pos = { view_x, view_y };
    AbsoluteDocumentPos pos = window_pos.to_absolute(this);

    // if multiple highlights contain the position, we return the smallest highlight
    // see: https://github.com/ahrm/sioyek/issues/773
    int smallest_containing_highlight_index = -1;
    int min_length = INT_MAX;

    if (current_document->can_use_highlights()) {
        const std::vector<Highlight>& highlights = current_document->get_highlights();

        for (size_t i = 0; i < highlights.size(); i++) {
            for (size_t j = 0; j < highlights[i].highlight_rects.size(); j++) {
                //if (fz_is_point_inside_rect(pos, highlights[i].highlight_rects[j])) {
                if (highlights[i].highlight_rects[j].contains(pos)) {
                    int length = highlights[i].description.size();
                    if (length < min_length) {
                        min_length = length;
                        smallest_containing_highlight_index = i;
                    }
                }
            }
        }
    }
    return smallest_containing_highlight_index;
}

void DocumentView::add_mark(char symbol) {
    //assert(current_document);
    if (current_document) {
        current_document->add_mark(symbol, offset_y, offset_x, zoom_level);
    }
}

std::string DocumentView::add_bookmark(std::wstring desc) {
    //assert(current_document);
    if (current_document) {
        return current_document->add_bookmark(desc, offset_y);
    }
    return "";
}

std::string DocumentView::add_highlight(AbsoluteDocumentPos selection_begin, AbsoluteDocumentPos selection_end, char type) {

    if (current_document) {
        std::deque<AbsoluteRect> selected_characters;
        std::vector<AbsoluteRect> merged_characters;
        std::wstring selected_text;

        get_text_selection(selection_begin, selection_end, !EXACT_HIGHLIGHT_SELECT, selected_characters, selected_text);
        merge_selected_character_rects(selected_characters, merged_characters);
        if (selected_text.size() > 0) {
            return current_document->add_highlight(selected_text, (std::vector<AbsoluteRect>&)merged_characters, selection_begin, selection_end, type);
        }
    }

    return "";
}

void DocumentView::on_view_size_change(int new_width, int new_height) {
    view_width = new_width;
    view_height = new_height;
}

//void DocumentView::absolute_to_window_pos_pixels(float absolute_x, float absolute_y, float* window_x, float* window_y) {
//
//}

NormalizedWindowPos DocumentView::absolute_to_window_pos(AbsoluteDocumentPos abs) {
    NormalizedWindowPos res;
    float half_width = static_cast<float>(view_width) / zoom_level / 2;
    float half_height = static_cast<float>(view_height) / zoom_level / 2;

    res.x = (abs.x + offset_x) / half_width;
    res.y = (-abs.y + offset_y) / half_height;
    return res;

}

NormalizedWindowRect DocumentView::absolute_to_window_rect(AbsoluteRect doc_rect) {
    NormalizedWindowPos top_left = doc_rect.top_left().to_window_normalized(this);
    NormalizedWindowPos bottom_right = doc_rect.bottom_right().to_window_normalized(this);

    return NormalizedWindowRect(top_left, bottom_right);
}

NormalizedWindowPos DocumentView::document_to_window_pos(DocumentPos doc_pos) {

    if (current_document) {
        WindowPos window_pos = document_to_window_pos_in_pixels_uncentered(doc_pos);
        double halfwidth = static_cast<double>(view_width) / 2;
        double halfheight = static_cast<double>(view_height) / 2;

        float window_x = static_cast<float>((window_pos.x - halfwidth) / halfwidth);
        float window_y = static_cast<float>((window_pos.y - halfheight) / halfheight);
        return { window_x, -window_y };
    }
}

WindowPos DocumentView::absolute_to_window_pos_in_pixels(AbsoluteDocumentPos abspos) {
    WindowPos window_pos;
    window_pos.y = (abspos.y - offset_y) * zoom_level + view_height / 2;
    window_pos.x = (abspos.x + offset_x) * zoom_level + view_width / 2;
    return window_pos;
}

WindowPos DocumentView::document_to_window_pos_in_pixels_uncentered(DocumentPos doc_pos) {
    AbsoluteDocumentPos abspos = current_document->document_to_absolute_pos(doc_pos);
    return absolute_to_window_pos_in_pixels(abspos);
}

WindowPos DocumentView::document_to_window_pos_in_pixels_banded(DocumentPos doc_pos) {
    AbsoluteDocumentPos abspos = current_document->document_to_absolute_pos(doc_pos);
    WindowPos window_pos;
    window_pos.y = static_cast<int>(std::roundf((abspos.y - offset_y) * zoom_level + static_cast<float>(view_height) / 2.0f));
    window_pos.x = static_cast<int>(std::roundf((abspos.x + offset_x) * zoom_level + static_cast<float>(view_width) / 2.0f));
    return window_pos;
}

WindowRect DocumentView::document_to_window_irect(DocumentRect doc_rect) {
    WindowPos top_left = doc_rect.top_left().to_window(this);
    WindowPos bottom_right = doc_rect.bottom_right().to_window(this);
    return WindowRect(top_left, bottom_right);
}

NormalizedWindowRect DocumentView::document_to_window_rect(DocumentRect doc_rect) {
    NormalizedWindowPos top_left = doc_rect.top_left().to_window_normalized(this);
    NormalizedWindowPos bottom_right = doc_rect.bottom_right().to_window_normalized(this);

    return NormalizedWindowRect(top_left, bottom_right);
}

NormalizedWindowRect DocumentView::document_to_window_rect_pixel_perfect(DocumentRect doc_rect, int pixel_width, int pixel_height, bool banded) {

    if ((pixel_width <= 0) || (pixel_height <= 0)) {
        return doc_rect.to_window_normalized(this);
    }


    WindowPos top_left, bottom_right;
    if (banded) {
        top_left = document_to_window_pos_in_pixels_banded(doc_rect.top_left());
        bottom_right = document_to_window_pos_in_pixels_banded(doc_rect.bottom_right());
    }
    else {
        top_left = document_to_window_pos_in_pixels_uncentered(doc_rect.top_left());
        bottom_right = document_to_window_pos_in_pixels_uncentered(doc_rect.bottom_right());
    }

    bottom_right.x -= ((bottom_right.x - top_left.x) - pixel_width);
    if (!banded) {
        //w1.y -= ((w1.y - w0.y) - pixel_height);
        bottom_right.y -= ((bottom_right.y - top_left.y) - pixel_height);
    }

    NormalizedWindowPos top_left_normalized = top_left.to_window_normalized(this);
    NormalizedWindowPos bottom_right_normalized = bottom_right.to_window_normalized(this);

    return NormalizedWindowRect(top_left_normalized, bottom_right_normalized);
}

DocumentPos DocumentView::window_to_document_pos_uncentered(WindowPos window_pos) {
    if (current_document) {
        return current_document->absolute_to_page_pos_uncentered(
            { (window_pos.x - view_width / 2) / zoom_level - offset_x,
            (window_pos.y - view_height / 2) / zoom_level + offset_y });
    }
    else {
        return { -1, 0, 0 };
    }
}

DocumentPos DocumentView::window_to_document_pos(WindowPos window_pos) {
    if (current_document) {
        return current_document->absolute_to_page_pos_uncentered(
            { (window_pos.x - view_width / 2) / zoom_level - offset_x,
            (window_pos.y - view_height / 2) / zoom_level + offset_y });
    }
    else {
        return { -1, 0, 0 };
    }
}

AbsoluteDocumentPos DocumentView::window_to_absolute_document_pos(WindowPos window_pos) {
    float doc_x = (window_pos.x - view_width / 2) / zoom_level - offset_x;
    float doc_y = (window_pos.y - view_height / 2) / zoom_level + offset_y;
    return { doc_x, doc_y };
}

NormalizedWindowPos DocumentView::window_to_normalized_window_pos(WindowPos window_pos) {
    float normal_x = 2 * (static_cast<float>(window_pos.x) - view_width / 2.0f) / view_width;
    float normal_y = -2 * (static_cast<float>(window_pos.y) - view_height / 2.0f) / view_height;
    return { normal_x, normal_y };
}


void DocumentView::goto_mark(char symbol) {
    if (current_document) {
        float new_y_offset = 0.0f;
        std::optional<Mark> mark = current_document->get_mark_if_exists(symbol);
        if (mark) {
            set_offset_y(mark->y_offset);
            if (mark->x_offset) {
                set_offset_x(mark->x_offset.value());
                set_zoom_level(mark->zoom_level.value(), true);
            }
        }
    }
}
void DocumentView::goto_end() {
    if (current_document) {
        int last_page_index = current_document->num_pages() - 1;
        set_offset_y(current_document->get_accum_page_height(last_page_index) + current_document->get_page_height(last_page_index));
    }
}

void DocumentView::goto_left_smart() {

    float left_ratio, right_ratio;
    int normal_page_width;
    float page_width = current_document->get_page_size_smart(true, get_center_page_number(), &left_ratio, &right_ratio, &normal_page_width);
    float view_left_offset = (page_width / 2 - view_width / zoom_level / 2);

    set_offset_x(view_left_offset);
}

void DocumentView::goto_left() {
    float page_width = current_document->get_page_width(get_center_page_number());
    float view_left_offset = (page_width / 2 - view_width / zoom_level / 2);
    set_offset_x(view_left_offset);
}

void DocumentView::goto_right_smart() {

    float left_ratio, right_ratio;
    int normal_page_width;
    float page_width = current_document->get_page_size_smart(true, get_center_page_number(), &left_ratio, &right_ratio, &normal_page_width);
    float view_left_offset = -(page_width / 2 - view_width / zoom_level / 2);

    set_offset_x(view_left_offset);
}

void DocumentView::goto_right() {
    float page_width = current_document->get_page_width(get_center_page_number());
    float view_left_offset = -(page_width / 2 - view_width / zoom_level / 2);
    set_offset_x(view_left_offset);
}

float DocumentView::set_zoom_level(float zl, bool should_exit_auto_resize_mode) {
#ifdef SIOYEK_ANDROID
    const float max_zoom_level = 6.0f;
#else
    const float max_zoom_level = 10.0f;
#endif

    if (TOUCH_MODE){
        int page_number = get_center_page_number();
        if (page_number >= 0){
            float min_zoom_level = view_width / current_document->get_page_width(page_number);
            if (zl < min_zoom_level){
                zl = min_zoom_level;
            }
        }
    }

    if (zl > max_zoom_level) {
        zl = max_zoom_level;
    }
    if (should_exit_auto_resize_mode) {
        this->is_auto_resize_mode = false;
    }
    zoom_level = zl;
    this->readjust_to_screen();
    return zoom_level;
}

float DocumentView::zoom_in(float zoom_factor) {
#ifdef SIOYEK_ANDROID
    const float max_zoom_level = 6.0f;
#else
    const float max_zoom_level = 10.0f;
#endif
    float new_zoom_level = zoom_level * zoom_factor;

    if (new_zoom_level > max_zoom_level) {
        new_zoom_level = max_zoom_level;
    }

    return set_zoom_level(new_zoom_level, true);
}
float DocumentView::zoom_out(float zoom_factor) {
    return set_zoom_level(zoom_level / zoom_factor, true);
}

float DocumentView::zoom_in_cursor(WindowPos mouse_pos, float zoom_factor) {

    AbsoluteDocumentPos prev_doc_pos = window_to_absolute_document_pos(mouse_pos);

    float res = zoom_in(zoom_factor);

    AbsoluteDocumentPos new_doc_pos = window_to_absolute_document_pos(mouse_pos);

    move_absolute(-prev_doc_pos.x + new_doc_pos.x, prev_doc_pos.y - new_doc_pos.y);

    return res;
}

float DocumentView::zoom_out_cursor(WindowPos mouse_pos, float zoom_factor) {
    auto [prev_doc_x, prev_doc_y] = window_to_absolute_document_pos(mouse_pos);

    float res = zoom_out(zoom_factor);

    auto [new_doc_x, new_doc_y] = window_to_absolute_document_pos(mouse_pos);

    move_absolute(-prev_doc_x + new_doc_x, prev_doc_y - new_doc_y);
    return res;
}
bool DocumentView::move_absolute(float dx, float dy, bool force) {
    return set_offsets(offset_x + dx, offset_y + dy, force);
}

bool DocumentView::move(float dx, float dy, bool force) {
    float abs_dx = (dx / zoom_level);
    float abs_dy = (dy / zoom_level);
    return move_absolute(abs_dx, abs_dy, force);
}
void DocumentView::get_absolute_delta_from_doc_delta(float dx, float dy, float* abs_dx, float* abs_dy) {
    *abs_dx = (dx / zoom_level);
    *abs_dy = (dy / zoom_level);
}

int DocumentView::get_center_page_number() {
    if (current_document) {
        return current_document->get_offset_page_number(get_offset_y());
    }
    else {
        return -1;
    }
}

void DocumentView::get_visible_pages(int window_height, std::vector<int>& visible_pages) {
    if (!current_document) return;

    float window_y_range_begin = offset_y - window_height / (1.5 * zoom_level);
    float window_y_range_end = offset_y + window_height / (1.5 * zoom_level);
    window_y_range_begin -= 1;
    window_y_range_end += 1;

    current_document->get_visible_pages(window_y_range_begin, window_y_range_end, visible_pages);
}

void DocumentView::move_pages(int num_pages) {
    if (!current_document) return;
    int current_page = get_center_page_number();
    if (current_page == -1) {
        current_page = 0;
    }
    move_absolute(0, num_pages * (current_document->get_page_height(current_page) + PAGE_PADDINGS));
}

void DocumentView::move_screens(int num_screens) {
    float screen_height_in_doc_space = view_height / zoom_level;
    set_offset_y(get_offset_y() + num_screens * screen_height_in_doc_space * MOVE_SCREEN_PERCENTAGE);
    //return move_amount;
}

void DocumentView::reset_doc_state() {
    zoom_level = 1.0f;
    set_offsets(0.0f, 0.0f);
    is_ruler_mode_ = false;
    presentation_page_number = {};
}

void DocumentView::open_document(const std::wstring& doc_path,
    bool* invalid_flag,
    bool load_prev_state,
    std::optional<OpenedBookState> prev_state,
    bool force_load_dimensions) {

    std::wstring canonical_path = get_canonical_path(doc_path);

    if (canonical_path == L"") {
        current_document = nullptr;
        return;
    }

    //if (error_code) {
    //	current_document = nullptr;
    //	return;
    //}

    //document_path = cannonical_path;


    //current_document = new Document(mupdf_context, doc_path, database);
    //current_document = document_manager->get_document(doc_path);
    current_document = document_manager->get_document(canonical_path);
    //current_document->open();
    if (!current_document->open(invalid_flag, force_load_dimensions)) {
        current_document = nullptr;
    }

    reset_doc_state();

    if (prev_state) {
        zoom_level = prev_state.value().zoom_level;
        offset_x = prev_state.value().offset_x;
        offset_y = prev_state.value().offset_y;
        set_offsets(offset_x, offset_y);
        is_auto_resize_mode = false;
    }
    else if (load_prev_state) {

        std::optional<std::string> checksum = checksummer->get_checksum_fast(canonical_path);
        std::vector<OpenedBookState> prev_state;
        if (checksum && db_manager->select_opened_book(checksum.value(), prev_state)) {
            if (prev_state.size() > 1) {
                LOG(std::cerr << "more than one file with one path, this should not happen!" << std::endl);
            }
        }
        if (prev_state.size() > 0) {
            OpenedBookState previous_state = prev_state[0];
            zoom_level = previous_state.zoom_level;
            offset_x = previous_state.offset_x;
            offset_y = previous_state.offset_y;
            set_offsets(previous_state.offset_x, previous_state.offset_y);
            is_auto_resize_mode = false;
        }
        else {
            if (current_document) {
                // automatically adjust width
                fit_to_page_width();
                is_auto_resize_mode = true;
                set_offset_y(view_height / 2 / zoom_level);
            }
        }
    }
}

float DocumentView::get_page_offset(int page) {

    if (!current_document) return 0.0f;

    int max_page = current_document->num_pages() - 1;
    if (page > max_page) {
        page = max_page;
    }
    return current_document->get_accum_page_height(page);
}

void DocumentView::goto_offset_within_page(int page, float offset_y) {
    set_offsets(offset_x, get_page_offset(page) + offset_y);
}

void DocumentView::goto_page(int page) {
    set_offset_y(get_page_offset(page) + view_height_in_document_space() / 2);
}

//void DocumentView::goto_toc_link(std::variant<PageTocLink, ChapterTocLink> toc_link) {
//	int page = -1;
//
//	if (std::holds_alternative<PageTocLink>(toc_link)) {
//		PageTocLink l = std::get<PageTocLink>(toc_link);
//		page = l.page;
//	}
//	else{
//		ChapterTocLink l = std::get<ChapterTocLink>(toc_link);
//		std::vector<int> accum_chapter_page_counts;
//		current_document->count_chapter_pages_accum(accum_chapter_page_counts);
//		page = accum_chapter_page_counts[l.chapter] + l.page;
//	}
//	set_offset_y(get_page_offset(page) + view_height_in_document_space()/2);
//}

void DocumentView::fit_to_page_height_and_width_smart() {

    int cp = get_center_page_number();
    if (cp == -1) return;

    float top_ratio, bottom_ratio;
    int normal_page_height;

    float left_ratio, right_ratio;
    int normal_page_width;
    int page_height = current_document->get_page_size_smart(false, cp, &top_ratio, &bottom_ratio, &normal_page_height);
    int page_width = current_document->get_page_size_smart(true, cp, &left_ratio, &right_ratio, &normal_page_width);

    float bottom_leftover = 1.0f - bottom_ratio;
    float right_leftover = 1.0f - right_ratio;
    float height_imbalance = top_ratio - bottom_leftover;
    float width_imbalance = left_ratio - right_leftover;

    float height_zoom_level = static_cast<float>(view_height - 20) / page_height;
    float width_zoom_level = static_cast<float>(view_width - 20) / page_width;

    float best_zoom_level = 1.0f;

    set_zoom_level(qMin(width_zoom_level, height_zoom_level), true);
    goto_offset_within_page(cp, (height_imbalance / 2.0f + 0.5f) * normal_page_height);
    set_offset_x(-width_imbalance * normal_page_width / 2.0f);
}

void DocumentView::fit_to_page_height(bool smart) {
    int cp = get_center_page_number();
    if (cp == -1) return;

    float top_ratio, bottom_ratio;
    int normal_page_height;
    int page_height = current_document->get_page_size_smart(false, cp, &top_ratio, &bottom_ratio, &normal_page_height);
    float bottom_leftover = 1.0f - bottom_ratio;
    float imbalance = top_ratio - bottom_leftover;

    if (!smart) {
        page_height = current_document->get_page_height(cp);
        imbalance = 0;
    }

    set_zoom_level(static_cast<float>(view_height - 20) / page_height, true);
    //set_offset_y(-imbalance * normal_page_height / 2.0f);
    goto_offset_within_page(cp, (imbalance / 2.0f + 0.5f) * normal_page_height);
}

void DocumentView::fit_to_page_width(bool smart, bool ratio) {
    int cp = get_center_page_number();
    if (cp == -1) return;

    //int page_width = current_document->get_page_width(cp);
    if (smart) {

        float left_ratio, right_ratio;
        int normal_page_width;
        int page_width = current_document->get_page_size_smart(true, cp, &left_ratio, &right_ratio, &normal_page_width);
        float right_leftover = 1.0f - right_ratio;
        float imbalance = left_ratio - right_leftover;

        set_zoom_level(static_cast<float>(view_width) / page_width, false);
        set_offset_x(-imbalance * normal_page_width / 2.0f);
    }
    else {
        int page_width = current_document->get_page_width(cp);
        int virtual_view_width = view_width;
        if (ratio) {
            virtual_view_width = static_cast<int>(static_cast<float>(view_width) * FIT_TO_PAGE_WIDTH_RATIO);
        }
        set_offset_x(0);
        set_zoom_level(static_cast<float>(virtual_view_width) / page_width, true);
    }

}

void DocumentView::fit_to_page_height_width_minimum(int statusbar_height) {
    int cp = get_center_page_number();
    if (cp == -1) return;

    int page_width = current_document->get_page_width(cp);
    int page_height = current_document->get_page_height(cp);

    float x_zoom_level = static_cast<float>(view_width) / page_width;
    float y_zoom_level;
    y_zoom_level = (static_cast<float>(view_height) - statusbar_height) / page_height;

    set_offset_x(0);
    set_zoom_level(std::min(x_zoom_level, y_zoom_level), true);

}

void DocumentView::persist(bool persist_drawings) {
    if (!current_document) return;
    db_manager->update_book(current_document->get_checksum(), zoom_level, offset_x, offset_y, current_document->detect_paper_name());
    if (persist_drawings) {
        current_document->persist_drawings();
        current_document->persist_annotations();
    }
}

int DocumentView::get_current_chapter_index() {
    const std::vector<int>& chapter_pages = current_document->get_flat_toc_pages();

    if (chapter_pages.size() == 0) {
        return -1;
    }

    int cp = get_center_page_number();

    int current_chapter_index = 0;

    int index = 0;
    for (int p : chapter_pages) {
        if (p <= cp) {
            current_chapter_index = index;
        }
        index++;
    }

    return current_chapter_index;
}

std::wstring DocumentView::get_current_chapter_name() {
    const std::vector<std::wstring>& chapter_names = current_document->get_flat_toc_names();
    int current_chapter_index = get_current_chapter_index();
    if (current_chapter_index > 0) {
        return chapter_names[current_chapter_index];
    }
    return L"";
}

std::optional<std::pair<int, int>> DocumentView::get_current_page_range() {
    int ci = get_current_chapter_index();
    if (ci < 0) {
        return {};
    }
    const std::vector<int>& chapter_pages = current_document->get_flat_toc_pages();
    int range_begin = chapter_pages[ci];
    int range_end = current_document->num_pages() - 1;

    if ((size_t)ci < chapter_pages.size() - 1) {
        range_end = chapter_pages[ci + 1];
    }

    return std::make_pair(range_begin, range_end);
}

void DocumentView::get_page_chapter_index(int page, std::vector<TocNode*> nodes, std::vector<int>& res) {


    for (size_t i = 0; i < nodes.size(); i++) {
        if ((i == nodes.size() - 1) && (nodes[i]->page <= page)) {
            res.push_back(i);
            get_page_chapter_index(page, nodes[i]->children, res);
            return;
        }
        else {
            if ((nodes[i]->page <= page) && (nodes[i + 1]->page > page)) {
                res.push_back(i);
                get_page_chapter_index(page, nodes[i]->children, res);
                return;
            }
        }
    }

}
std::vector<int> DocumentView::get_current_chapter_recursive_index() {
    int curr_page = get_center_page_number();
    std::vector<TocNode*> nodes = current_document->get_toc();
    std::vector<int> res;
    get_page_chapter_index(curr_page, nodes, res);
    return res;
}

void DocumentView::goto_chapter(int diff) {
    const std::vector<int>& chapter_pages = current_document->get_flat_toc_pages();
    int curr_page = get_center_page_number();

    int index = 0;

    while ((size_t)index < chapter_pages.size() && chapter_pages[index] < curr_page) {
        index++;
    }

    int new_index = index + diff;
    if (new_index < 0) {
        goto_page(0);
    }
    else if ((size_t)new_index >= chapter_pages.size()) {
        goto_end();
    }
    else {
        goto_page(chapter_pages[new_index]);
    }
}

float DocumentView::view_height_in_document_space() {
    return static_cast<float>(view_height) / zoom_level;
}

void DocumentView::set_vertical_line_pos(float pos) {
    ruler_pos = pos;
    ruler_rect = {};
    is_ruler_mode_ = true;
}

float DocumentView::get_ruler_pos() {
    if (ruler_rect.has_value()) {
        return ruler_rect->y1;
    }
    else {
        return ruler_pos;
    }
}

std::optional<AbsoluteRect> DocumentView::get_ruler_rect() {
    return ruler_rect;
}

bool DocumentView::has_ruler_rect() {
    return ruler_rect.has_value();
}

float DocumentView::get_ruler_window_y() {

    float absol_end_y = get_ruler_pos();

    absol_end_y += RULER_PADDING;

    return absolute_to_window_pos({ 0.0f, absol_end_y }).y;
}

std::optional<NormalizedWindowRect> DocumentView::get_ruler_window_rect() {
    if (has_ruler_rect()) {
        AbsoluteRect absol_ruler_rect = get_ruler_rect().value();

        absol_ruler_rect.y0 -= RULER_PADDING;
        absol_ruler_rect.y1 += RULER_PADDING;

        absol_ruler_rect.x0 -= RULER_X_PADDING;
        absol_ruler_rect.x1 += RULER_X_PADDING;
        return NormalizedWindowRect(absolute_to_window_rect(absol_ruler_rect));
    }
    return {};
}

//float DocumentView::get_vertical_line_window_y() {
//
//	float absol_end_y = get_vertical_line_pos();
//
//	absol_end_y += RULER_PADDING;
//
//	float window_begin_x, window_begin_y;
//	float window_end_x, window_end_y;
//	absolute_to_window_pos(0.0, absol_end_y, &window_end_x, &window_end_y);
//
//	return window_end_y;
//}

void DocumentView::goto_vertical_line_pos() {
    if (current_document) {
        //float new_y_offset = vertical_line_pos;
        float new_y_offset = get_ruler_pos();
        set_offset_y(new_y_offset);
        is_ruler_mode_ = true;
    }
}

void DocumentView::get_text_selection(AbsoluteDocumentPos selection_begin,
    AbsoluteDocumentPos selection_end,
    bool is_word_selection, // when in word select mode, we select entire words even if the range only partially includes the word
    std::deque<AbsoluteRect>& selected_characters,
    std::wstring& selected_text) {

    if (current_document) {
        current_document->get_text_selection(selection_begin, selection_end, is_word_selection, selected_characters, selected_text);
    }

}

int DocumentView::get_page_offset() {
    return current_document->get_page_offset();
}

void DocumentView::set_page_offset(int new_offset) {
    current_document->set_page_offset(new_offset);
}

float DocumentView::get_max_valid_x(bool relenting) {
    float page_width = current_document->get_page_width(get_center_page_number());
    if (!relenting){
        return std::abs(-view_width / zoom_level / 2 + page_width / 2);
    }
    else{
        return std::abs(-view_width / zoom_level / 2 + 3 * page_width / 2);
    }
}

float DocumentView::get_min_valid_x(bool relenting) {
    float page_width = current_document->get_page_width(get_center_page_number());
    if (!relenting){
        return -std::abs(-view_width / zoom_level / 2 + page_width / 2);
    }
    else{
        return -std::abs(-view_width / zoom_level / 2 + 3 * page_width / 2);
    }
}

void DocumentView::rotate() {
    int current_page = get_center_page_number();

    current_document->rotate();
    float new_offset = current_document->get_accum_page_height(current_page) + current_document->get_page_height(current_page) / 2;
    set_offset_y(new_offset);
}

void DocumentView::goto_top_of_page() {
    int current_page = get_center_page_number();
    float offset_y = get_document()->get_accum_page_height(current_page) + static_cast<float>(view_height) / 2.0f / zoom_level;
    set_offset_y(offset_y);
}

void DocumentView::goto_bottom_of_page() {
    int current_page = get_center_page_number();
    float offset_y = get_document()->get_accum_page_height(current_page + 1) - static_cast<float>(view_height) / 2.0f / zoom_level;

    if (current_page + 1 == current_document->num_pages()) {
        offset_y = get_document()->get_accum_page_height(current_page) + get_document()->get_page_height(current_page) - static_cast<float>(view_height) / 2.0f / zoom_level;
    }
    set_offset_y(offset_y);
}

int DocumentView::get_line_index() {
    if (line_index == -1) {
        return get_line_index_of_vertical_pos();
    }
    else {
        return line_index;
    }
}

void DocumentView::set_line_index(int index, int page) {
    line_index = index;
    is_ruler_mode_ = true;
    if (page >= 0) {
        auto lines = get_document()->get_page_lines(page);
        if (index >= 0 && index < lines.size()) {
            ruler_rect = lines[index];
        }
    }

}

int DocumentView::get_line_index_of_vertical_pos() {
    DocumentPos line_doc_pos = current_document->absolute_to_page_pos_uncentered({ 0, get_ruler_pos() });
    auto rects = current_document->get_page_lines(line_doc_pos.page);
    int index = 0;
    while ((size_t)index < rects.size() && rects[index].y0 < get_ruler_pos()) {
        index++;
    }
    return index - 1;
}

int DocumentView::get_line_index_of_pos(DocumentPos line_doc_pos) {
    AbsoluteDocumentPos line_abs_pos = line_doc_pos.to_absolute(current_document);
    auto rects = current_document->get_page_lines(line_doc_pos.page, nullptr);
    int page_width = current_document->get_page_width(line_doc_pos.page);

    for (int i = 0; i < rects.size(); i++) {
        if (rects[i].contains(line_abs_pos)) return i;
    }
    return -1;
}

int DocumentView::get_vertical_line_page() {
    return current_document->absolute_to_page_pos({ 0, get_ruler_pos() }).page;
}

std::optional<std::wstring> DocumentView::get_selected_line_text() {
    if (line_index >= 0) {
        std::vector<std::wstring> lines;
        std::vector<AbsoluteRect> line_rects = current_document->get_page_lines(get_vertical_line_page(), &lines);
        if ((size_t)line_index < lines.size()) {
            std::wstring content = lines[line_index];
            return content;
        }
        else {
            return {};
        }
    }
    return {};
}

void DocumentView::get_rects_from_ranges(int page_number, const std::vector<PagelessDocumentRect>& line_char_rects, const std::vector<std::pair<int, int>>& ranges, std::vector<PagelessDocumentRect>& out_rects) {
    for (int i = 0; i < ranges.size(); i++) {
        auto [first, last] = ranges[i];
        PagelessDocumentRect current_source_rect = get_range_rect_union(line_char_rects, first, last);
        current_source_rect = current_document->document_to_absolute_rect(DocumentRect(current_source_rect, page_number));
        out_rects.push_back(current_source_rect);
    }
}

std::vector<SmartViewCandidate> DocumentView::find_line_definitions() {
    //todo: remove duplicate code from this function, this just needs to find the location of the
    // reference, the rest can be handled by find_definition_of_location

    std::vector<SmartViewCandidate> result;

    if (line_index > 0) {
        std::vector<std::wstring> lines;
        std::vector<std::vector<PagelessDocumentRect>> line_char_rects;

        int line_page_number = get_vertical_line_page();

        std::vector<AbsoluteRect> line_rects = current_document->get_page_lines(line_page_number, &lines, &line_char_rects);
        for (int i = 0; i < lines.size(); i++) {
            assert(lines[i].size() == line_char_rects[i].size());
        }
        if ((size_t)line_index < lines.size()) {
            std::wstring content = lines[line_index];

            //todo: deduplicate this code

            AbsoluteRect line_rect = line_rects[line_index];
            float mid_y = (line_rect.y1 + line_rect.y0) / 2.0f;
            line_rect.y0 = line_rect.y0 = mid_y;

            std::vector<PdfLink> pdf_links = current_document->get_links_in_page_rect(get_vertical_line_page(), line_rect);
            if (pdf_links.size() > 0) {

                for (auto link : pdf_links) {
                    auto parsed_uri = parse_uri(get_document()->get_mupdf_context(), get_document()->doc, link.uri);
                    SmartViewCandidate candid;
                    candid.doc = get_document();
                    candid.source_rect = current_document->document_to_absolute_rect(DocumentRect(link.rects[0], line_page_number));
                    candid.source_text = get_document()->get_pdf_link_text(link);
                    candid.target_pos = DocumentPos{ parsed_uri.page - 1, parsed_uri.x, parsed_uri.y };
                    result.push_back(candid);
                }

                return result;
            }

            std::wstring item_regex(L"[a-zA-Z]{2,}[ \t]+[0-9]+(\.[0-9]+)*");
            std::wstring reference_regex(L"\\[[a-zA-Z0-9, ]+\\]");
            std::wstring equation_regex(L"\\([0-9]+(\\.[0-9]+)*\\)");

            std::vector<std::pair<int, int>> generic_item_ranges;
            std::vector<std::pair<int, int>> reference_ranges;
            std::vector<std::pair<int, int>> equation_ranges;

            std::vector<std::wstring> generic_item_texts = find_all_regex_matches(content, item_regex, &generic_item_ranges);
            std::vector<std::wstring> reference_texts = find_all_regex_matches(content, reference_regex, &reference_ranges);
            std::vector<std::wstring> equation_texts = find_all_regex_matches(content, equation_regex, &equation_ranges);

            std::vector<PagelessDocumentRect> generic_item_rects;
            std::vector<PagelessDocumentRect> reference_rects;
            std::vector<PagelessDocumentRect> equation_rects;

            int ruler_page = get_vertical_line_page();
            get_rects_from_ranges(ruler_page, line_char_rects[line_index], generic_item_ranges, generic_item_rects);
            get_rects_from_ranges(ruler_page, line_char_rects[line_index], reference_ranges, reference_rects);
            get_rects_from_ranges(ruler_page, line_char_rects[line_index], equation_ranges, equation_rects);

            std::vector<SmartViewCandidate> generic_positions;
            std::vector<SmartViewCandidate> reference_positions;
            std::vector<SmartViewCandidate> equation_positions;

            for (int i = 0; i < generic_item_texts.size(); i++) {
                std::vector<IndexedData> possible_targets = current_document->find_generic_with_string(generic_item_texts[i], ruler_page);
                for (int j = 0; j < possible_targets.size(); j++) {
                    SmartViewCandidate candid;
                    candid.doc = get_document();
                    candid.source_rect = generic_item_rects[i];
                    candid.source_text = generic_item_texts[i];
                    candid.target_pos = DocumentPos{ possible_targets[j].page, 0, possible_targets[j].y_offset };
                    generic_positions.push_back(candid);
                    //generic_positions.push_back(
                    //    std::make_pair(DocumentPos{ possible_targets[j].page, 0, possible_targets[j].y_offset },
                    //        generic_item_rects[i])
                    //);
                }
            }
            for (int i = 0; i < reference_texts.size(); i++) {
                if (reference_texts[i].find(L",") != -1) {
                    // remove [ and ]
                    QString references_string = QString::fromStdWString(reference_texts[i].substr(1, reference_texts[i].size()-2));
                    QStringList parts = references_string.split(',');
                    int n_chars_seen = 1;
                    for (int j = 0; j < parts.size(); j++) {
                        auto index = current_document->find_reference_with_string(parts[j].trimmed().toStdWString(), ruler_page);

                        // range of the substring
                        int rect_range_begin = reference_ranges[i].first + n_chars_seen;
                        int rect_range_end = rect_range_begin + parts[j].size();


                        if (index.size() > 0) {
                            std::vector<PagelessDocumentRect> subrects;
                            get_rects_from_ranges(ruler_page, line_char_rects[line_index], {std::make_pair(rect_range_begin, rect_range_end)}, subrects);

                            SmartViewCandidate candid;
                            candid.doc = get_document();
                            candid.source_rect = subrects[0];
                            candid.source_text = parts[j].trimmed().toStdWString();
                            candid.target_pos = DocumentPos{ index[0].page, 0, index[0].y_offset };
                            reference_positions.push_back(candid);
                        }
                        n_chars_seen += parts[j].size() + 1;
                    }
                }
                auto index = current_document->find_reference_with_string(reference_texts[i], ruler_page);

                if (index.size() > 0) {

                    SmartViewCandidate candid;
                    candid.doc = get_document();
                    candid.source_rect = reference_rects[i];
                    candid.source_text = reference_texts[i];
                    candid.target_pos = DocumentPos{ index[0].page, 0, index[0].y_offset };
                    reference_positions.push_back(candid);
                    //reference_positions.push_back(
                    //    std::make_pair(
                    //        DocumentPos{ index[0].page, 0, index[0].y_offset },
                    //        reference_rects[i])
                    //);
                }
            }
            //for (auto equation_text : equation_texts) {
            for (int i = 0; i < equation_texts.size(); i++) {
                auto index = current_document->find_equation_with_string(equation_texts[i], get_vertical_line_page());

                if (index.size() > 0) {
                    SmartViewCandidate candid;
                    candid.doc = get_document();
                    candid.source_rect = equation_rects[i];
                    candid.source_text = equation_texts[i];
                    candid.target_pos = DocumentPos{ index[0].page, 0, index[0].y_offset };
                    equation_positions.push_back(candid);
                    //equation_positions.push_back(
                    //    std::make_pair(
                    //        DocumentPos { index[0].page, 0, index[0].y_offset },
                    //        equation_rects[i]
                    //    )
                    //);
                }
            }

            std::vector<std::vector<SmartViewCandidate>*> res_vectors = { &equation_positions, &reference_positions, &generic_positions };
            int index = 0;
            int max_size = 0;

            for (auto vec : res_vectors) {
                if (vec->size() > max_size) {
                    max_size = vec->size();
                }
            }
            // interleave the results
            for (int i = 0; i < max_size; i++) {
                for (auto vec : res_vectors) {
                    if (i < vec->size()) {
                        result.push_back(vec->at(i));
                    }
                }
            }

            return result;
        }
    }
    return result;
}

bool DocumentView::goto_definition() {
    std::vector<SmartViewCandidate> defloc = find_line_definitions();
    if (defloc.size() > 0) {
        //goto_offset_within_page(defloc[0].first.page, defloc[0].first.y);
        DocumentPos docpos = defloc[0].get_docpos(this);
        goto_offset_within_page(docpos.page, docpos.y);
        return true;
    }
    return false;
}

bool DocumentView::get_is_auto_resize_mode() {
    return is_auto_resize_mode;
}

void DocumentView::disable_auto_resize_mode() {
    this->is_auto_resize_mode = false;
}

void DocumentView::readjust_to_screen() {
    this->set_offsets(this->get_offset_x(), this->get_offset_y());
}

float DocumentView::get_half_screen_offset() {
    return (static_cast<float>(view_height) / 2.0f);
}

void DocumentView::scroll_mid_to_top() {
    float offset = get_half_screen_offset();
    move(0, offset);
}

void DocumentView::get_visible_links(std::vector<PdfLink>& visible_page_links) {

    std::vector<int> visible_pages;
    get_visible_pages(get_view_height(), visible_pages);
    for (auto page : visible_pages) {
        std::vector<PdfLink> links = get_document()->get_page_merged_pdf_links(page);
        for (auto link : links) {
            ParsedUri parsed_uri = parse_uri(get_document()->get_mupdf_context(), get_document()->doc, link.uri);
            NormalizedWindowRect window_rect = DocumentRect(link.rects[0], page).to_window_normalized(this);
            if (window_rect.is_visible()) {
                visible_page_links.push_back(link);
            }
        }
    }
}

std::deque<AbsoluteRect>* DocumentView::get_selected_character_rects() {
    return &this->selected_character_rects;
}

std::optional<AbsoluteRect> DocumentView::get_control_rect() {
    if (selected_character_rects.size() > 0) {
        if (mark_end) {
            return selected_character_rects[selected_character_rects.size() - 1];
        }
        else {
            return selected_character_rects[0];
        }
    }
    return {};
}

std::optional<AbsoluteRect> DocumentView::shrink_selection(bool is_begin, bool word) {
    if (selected_character_rects.size() > 1) {
        if (word) {
            int page;
            int index = is_begin ? 0 : selected_character_rects.size() - 1;
            DocumentRect page_rect = selected_character_rects[index].to_document(current_document);
            if (page >= 0) {
                fz_stext_page* stext_page = current_document->get_stext_with_page_number(page);
                std::optional<DocumentRect> new_rect_ = find_shrinking_rect_word(is_begin, stext_page, page_rect);
                if (new_rect_) {
                    AbsoluteRect new_rect = new_rect_->to_absolute(current_document);

                    if (is_begin) {
                        while (!are_rects_same(new_rect, selected_character_rects[0])) {
                            selected_character_rects.pop_front();
                            if (selected_character_rects.size() == 1) break;
                        }
                        return selected_character_rects[0];
                    }
                    else {
                        while (!are_rects_same(new_rect, selected_character_rects[selected_character_rects.size() - 1])) {
                            selected_character_rects.pop_back();
                            if (selected_character_rects.size() == 1) break;
                        }
                        return selected_character_rects[selected_character_rects.size() - 1];
                    }

                }
                return {};
            }

        }
        else {
            if (is_begin) {
                selected_character_rects.pop_front();
                return selected_character_rects[0];
            }
            else {
                selected_character_rects.pop_back();
                return selected_character_rects[selected_character_rects.size() - 1];
            }
        }
    }

    return {};
}

std::optional<AbsoluteRect> DocumentView::expand_selection(bool is_begin, bool word) {
    //current_document->get_stext_with_page_number()
    if (selected_character_rects.size() > 0) {
        int index = is_begin ? 0 : selected_character_rects.size() - 1;

        DocumentRect page_rect = selected_character_rects[index].to_document(current_document);

        if (page_rect.page >= 0) {
            fz_stext_page* stext_page = current_document->get_stext_with_page_number(page_rect.page);
            std::optional<DocumentRect> next_rect = {};
            if (word) {
                std::vector<DocumentRect> next_rects_document = find_expanding_rect_word(is_begin, stext_page, page_rect);
                std::vector<AbsoluteRect> next_rects;
                for (auto dr : next_rects_document) {
                    next_rects.push_back(dr.to_absolute(current_document));
                }
                if (is_begin) {
                    for (int i = 0; i < next_rects.size(); i++) {
                        selected_character_rects.push_front(next_rects[i]);
                    }
                    return selected_character_rects[0];
                }
                else {
                    for (int i = 0; i < next_rects.size(); i++) {
                        selected_character_rects.push_back(next_rects[i]);
                    }
                    return selected_character_rects[selected_character_rects.size() - 1];
                }
            }
            else {
                next_rect = find_expanding_rect(is_begin, stext_page, page_rect);
            }
            if (next_rect) {
                AbsoluteRect next_rect_abs = next_rect->to_absolute(current_document);
                if (is_begin) {
                    selected_character_rects.push_front(next_rect_abs);
                }
                else {
                    selected_character_rects.push_back(next_rect_abs);
                }
                return next_rect_abs;
            }
        }
    }
    return {};
}
void DocumentView::set_text_mark(bool is_begin) {
    if (is_begin) {
        mark_end = false;
    }
    else {
        mark_end = true;
    }
}

void DocumentView::toggle_text_mark() {
    set_text_mark(mark_end);
}


WindowPos DocumentView::normalized_window_to_window_pos(NormalizedWindowPos nwp) {
    int window_x0 = static_cast<int>(nwp.x * view_width / 2 + view_width / 2);
    int window_y0 = static_cast<int>(-nwp.y * view_height / 2 + view_height / 2);
    return { window_x0, window_y0 };
}

WindowRect DocumentView::normalized_to_window_rect(NormalizedWindowRect normalized_rect) {
    return WindowRect(normalized_rect.top_left().to_window(this), normalized_rect.bottom_right().to_window(this));
}

bool DocumentView::is_ruler_mode() {
    return is_ruler_mode_;
}

Document* SmartViewCandidate::get_document(DocumentView* view) {
    if (doc) return doc;
    return view->get_document();

}

DocumentPos SmartViewCandidate::get_docpos(DocumentView* view) {
    if (std::holds_alternative<DocumentPos>(target_pos)) {
        return std::get<DocumentPos>(target_pos);
    }
    else {
        return get_document(view)->absolute_to_page_pos_uncentered(std::get<AbsoluteDocumentPos>(target_pos));
    }
}

AbsoluteDocumentPos SmartViewCandidate::get_abspos(DocumentView* view) {
    if (std::holds_alternative<AbsoluteDocumentPos>(target_pos)) {
        return std::get<AbsoluteDocumentPos>(target_pos);
    }
    else {
        return get_document(view)->document_to_absolute_pos(std::get<DocumentPos>(target_pos));
    }
}

ScratchPad::ScratchPad() : DocumentView(nullptr, nullptr, nullptr) {
    zoom_level = 1;
}

bool ScratchPad::set_offsets(float new_offset_x, float new_offset_y, bool force) {
    offset_x = new_offset_x;
    offset_y = new_offset_y;
    return false;
}

float ScratchPad::set_zoom_level(float zl, bool should_exit_auto_resize_mode) {
    float min_zoom_level = 0.1f;
    float max_zoom_level = 1000.0f;

    if (zl < min_zoom_level) {
        zl = min_zoom_level;
    }
    if (zl > max_zoom_level) {
        zl = max_zoom_level;
    }

    zoom_level = zl;
    return zoom_level;
}

float ScratchPad::zoom_in(float zoom_factor) {
    return set_zoom_level(zoom_level * zoom_factor, true);
}

float ScratchPad::zoom_out(float zoom_factor) {
    return set_zoom_level(zoom_level / zoom_factor, true);
}

std::vector<int> ScratchPad::get_intersecting_drawing_indices(AbsoluteRect selection) {
    invalidate_compile();

    std::vector<int> res;

    for (int i = 0; i < all_drawings.size(); i++) {
        for (auto p : all_drawings[i].points) {
            if (selection.contains(p.pos)) {
                res.push_back(i);
                break;
            }
        }
    }
    return res;
}

void ScratchPad::delete_intersecting_drawings(AbsoluteRect selection) {
    invalidate_compile(true);

    std::vector<int> indices = get_intersecting_drawing_indices(selection);
    for (int i = 0; i < indices.size(); i++) {
        all_drawings.erase(all_drawings.begin() + indices[indices.size() - 1 - i]);
    }
}

std::vector<int> ScratchPad::get_intersecting_pixmap_indices(AbsoluteRect selection) {
    std::vector<int> res;
    for (int i = 0; i < pixmaps.size(); i++) {
        if (selection.intersects(pixmaps[i].rect)) {
            res.push_back(i);
        }
    }
    return res;
}

void ScratchPad::delete_intersecting_pixmaps(AbsoluteRect selection) {
    std::vector<int> indices = get_intersecting_pixmap_indices(selection);
    for (int i = 0; i < indices.size(); i++) {
        pixmaps.erase(pixmaps.begin() + indices[indices.size() - 1 - i]);
    }
}

void ScratchPad::delete_intersecting_objects(AbsoluteRect selection) {
    delete_intersecting_drawings(selection);
    delete_intersecting_pixmaps(selection);
}

void ScratchPad::get_selected_objects_with_indices(const std::vector<SelectedObjectIndex>&indices, std::vector<FreehandDrawing>&freehand_drawings, std::vector<PixmapDrawing>&pixmap_drawings){

    for (auto [index, type] : indices) {
        if (type == SelectedObjectType::Drawing) {
            freehand_drawings.push_back(all_drawings[index]);
        }
        else if (type == SelectedObjectType::Pixmap) {
            pixmap_drawings.push_back(pixmaps[index]);
        }
    }
}

void ScratchPad::add_pixmap(QPixmap pixmap) {
    AbsoluteDocumentPos top_abs_pos = get_bounding_box().bottom_right();
    int top_window_pos = top_abs_pos.to_window(this).y;

    float dpr = pixmap.devicePixelRatio();
    int pixmap_width = pixmap.width() / dpr;
    int pixmap_height = pixmap.height() / dpr;

    int pixmap_window_left = view_width / 2 - pixmap_width / 2;
    int pixmap_window_right = view_width / 2 + pixmap_width / 2;
    int pixmap_window_top = top_window_pos;
    int pixmap_window_bottom = top_window_pos + pixmap_height;

    WindowPos top_left = { pixmap_window_left, pixmap_window_top };
    WindowPos bottom_right = { pixmap_window_right, pixmap_window_bottom };

    AbsoluteDocumentPos top_left_abs = top_left.to_absolute(this);
    AbsoluteDocumentPos bottom_right_abs = bottom_right.to_absolute(this);
    AbsoluteRect pixmap_rect = AbsoluteRect(top_left_abs, bottom_right_abs);
    AbsoluteDocumentPos center_pos = pixmap_rect.center();
    offset_x = center_pos.x;
    offset_y = center_pos.y;

    pixmaps.push_back(PixmapDrawing{ pixmap, pixmap_rect });

}

AbsoluteRect ScratchPad::get_bounding_box() {
    AbsoluteRect res;
    res.x0 = res.x1 = res.y0 = res.y1 = 0;

    if (all_drawings.size() > 0) {
        res = all_drawings[0].bbox();
    }
    else if (pixmaps.size() > 0) {
        res = pixmaps[0].rect;
    }
    else {
        return res;
    }

    for (auto drawing : all_drawings) {
        res = res.union_rect(drawing.bbox());
    }
    for (auto [_, pixmap_rect] : pixmaps) {
        res = res.union_rect(pixmap_rect);
    }

    return res;
}

std::vector<SelectedObjectIndex> ScratchPad::get_intersecting_objects(AbsoluteRect selection) {
    std::vector<SelectedObjectIndex> selected_objects;

    std::vector<int> drawing_indices = get_intersecting_drawing_indices(selection);
    std::vector<int> pixmap_indices = get_intersecting_pixmap_indices(selection);

    for (auto index : drawing_indices) {
        selected_objects.push_back(SelectedObjectIndex{ index, SelectedObjectType::Drawing });
    }
    for (auto index : pixmap_indices) {
        selected_objects.push_back(SelectedObjectIndex{ index, SelectedObjectType::Pixmap });
    }

    return selected_objects;
}

const std::vector<FreehandDrawing>& ScratchPad::get_all_drawings() {
    return all_drawings;
}

const std::vector<FreehandDrawing>& ScratchPad::get_non_compiled_drawings() {
    return non_compiled_drawings;
}

void ScratchPad::on_compile() {
    is_compile_valid = true;
    non_compiled_drawings.clear();
}

void ScratchPad::invalidate_compile(bool force) {

    if (non_compiled_drawings.size() > 0) {
        is_compile_valid = false;
    }
    if (force) {
        is_compile_valid = false;
    }
}

void ScratchPad::add_drawing(FreehandDrawing drawing) {
    all_drawings.push_back(drawing);
    non_compiled_drawings.push_back(drawing);
}

void ScratchPad::clear() {
    all_drawings.clear();
    pixmaps.clear();
    non_compiled_drawings.clear();
    is_compile_valid = false;
}

bool ScratchPad::is_compile_invalid() {
    return !is_compile_valid;
}

std::vector<int> DocumentView::get_visible_highlight_indices() {

    const std::vector<Highlight>& highlights = get_document()->get_highlights();

    std::vector<int> res;

    for (size_t i = 0; i < highlights.size(); i++) {

        NormalizedWindowPos selection_begin_window_pos = absolute_to_window_pos(
            { highlights[i].selection_begin.x, highlights[i].selection_begin.y }
        );

        NormalizedWindowPos selection_end_window_pos = absolute_to_window_pos(
            { highlights[i].selection_end.x, highlights[i].selection_end.y }
        );

        if (selection_begin_window_pos.y > selection_end_window_pos.y) {
            std::swap(selection_begin_window_pos.y, selection_end_window_pos.y);
        }
        if (range_intersects(selection_begin_window_pos.y, selection_end_window_pos.y, -1.0f, 1.0f)) {
            res.push_back(i);
        }
    }

    return res;
}

void DocumentView::set_presentation_page_number(std::optional<int> page) {
    presentation_page_number = page;
}

std::optional<int> DocumentView::get_presentation_page_number() {
    return presentation_page_number;
}

bool DocumentView::is_presentation_mode() {
    return presentation_page_number.has_value();
}
