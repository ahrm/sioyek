#include "document_view.h"
#include <cmath>

extern float MOVE_SCREEN_PERCENTAGE;
extern float FIT_TO_PAGE_WIDTH_RATIO;
extern float RULER_PADDING;
extern float RULER_X_PADDING;
extern bool EXACT_HIGHLIGHT_SELECT;
extern bool IGNORE_STATUSBAR_IN_PRESENTATION_MODE;
extern bool VERBOSE;


DocumentView::DocumentView(fz_context* mupdf_context,
    DatabaseManager* db_manager,
    DocumentManager* document_manager,
    ConfigManager* config_manager,
    CachedChecksummer* checksummer) :
    mupdf_context(mupdf_context),
    db_manager(db_manager),
    config_manager(config_manager),
    document_manager(document_manager),
    checksummer(checksummer)
{

}
DocumentView::~DocumentView() {
}

DocumentView::DocumentView(fz_context* mupdf_context,
    DatabaseManager* db_manager,
    DocumentManager* document_manager,
    ConfigManager* config_manager,
    CachedChecksummer* checksummer,
    bool* invalid_flag,
    std::wstring path,
    int view_width,
    int view_height,
    float offset_x,
    float offset_y) :
    DocumentView(mupdf_context,
        db_manager,
        document_manager,
        config_manager,
        checksummer)

{
    on_view_size_change(view_width, view_height);
    open_document(path, invalid_flag);
    set_offsets(offset_x, offset_y);
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
    is_ruler_mode_ = state.ruler_mode;
    ruler_pos = state.ruler_pos;
    ruler_rect = state.ruler_rect;
    line_index = state.line_index;
}

bool DocumentView::set_offsets(float new_offset_x, float new_offset_y, bool force) {
    // if move was truncated
    bool truncated = false;

    if (current_document == nullptr) return false;

    int num_pages = current_document->num_pages();
    if (num_pages == 0) return false;

    float max_y_offset = current_document->get_accum_page_height(num_pages - 1) + current_document->get_page_height(num_pages - 1);
    float min_y_offset = 0;
    float min_x_offset = get_min_valid_x();
    float max_x_offset = get_max_valid_x();

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

//void DocumentView::render_highlight_window( GLuint program, fz_rect window_rect) {
//	float quad_vertex_data[] = {
//		window_rect.x0, window_rect.y1,
//		window_rect.x1, window_rect.y1,
//		window_rect.x0, window_rect.y0,
//		window_rect.x1, window_rect.y0
//	};
//	glEnable(GL_BLEND);
//	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
//	glUseProgram(program);
//	glEnableVertexAttribArray(0);
//	glEnableVertexAttribArray(1);
//	glBufferData(GL_ARRAY_BUFFER, sizeof(quad_vertex_data), quad_vertex_data, GL_DYNAMIC_DRAW);
//	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
//	glDisable(GL_BLEND);
//}

//void DocumentView::render_highlight_absolute( GLuint program, fz_rect absolute_document_rect) {
//	fz_rect window_rect = absolute_to_window_rect(absolute_document_rect);
//	render_highlight_window(program, window_rect);
//}
//
//void DocumentView::render_highlight_document( GLuint program, int page, fz_rect doc_rect) {
//	fz_rect window_rect = document_to_window_rect(page, doc_rect);
//	render_highlight_window(program, window_rect);
//}

std::optional<PdfLink> DocumentView::get_link_in_pos(WindowPos pos) {
    if (!current_document) return {};

    UncenteredDocumentPos doc_pos = window_to_document_pos_uncentered(pos);
    return current_document->get_link_in_pos(doc_pos);
}

int DocumentView::get_highlight_index_in_pos(WindowPos window_pos) {
    auto [view_x, view_y] = window_to_absolute_document_pos(window_pos);

    fz_point pos = { view_x, view_y };

    if (current_document->can_use_highlights()) {
        const std::vector<Highlight>& highlights = current_document->get_highlights();

        for (size_t i = 0; i < highlights.size(); i++) {
            for (size_t j = 0; j < highlights[i].highlight_rects.size(); j++) {
                if (fz_is_point_inside_rect(pos, highlights[i].highlight_rects[j])) {
                    return i;
                }
            }
        }
    }
    return -1;
}

void DocumentView::add_mark(char symbol) {
    //assert(current_document);
    if (current_document) {
        current_document->add_mark(symbol, offset_y);
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
        std::deque<fz_rect> selected_characters;
        std::vector<fz_rect> merged_characters;
        std::wstring selected_text;

        get_text_selection(selection_begin, selection_end, !EXACT_HIGHLIGHT_SELECT, selected_characters, selected_text);
        merge_selected_character_rects(selected_characters, merged_characters);
        if (selected_text.size() > 0) {
            return current_document->add_highlight(selected_text, merged_characters, selection_begin, selection_end, type);
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

fz_rect DocumentView::absolute_to_window_rect(fz_rect doc_rect) {
    fz_rect res;
    NormalizedWindowPos p1 = absolute_to_window_pos({ doc_rect.x0, doc_rect.y0 });
    NormalizedWindowPos p2 = absolute_to_window_pos({ doc_rect.x1, doc_rect.y1 });

    res.x0 = p1.x;
    res.y0 = p1.y;
    res.x1 = p2.x;
    res.y1 = p2.y;

    return res;
}

NormalizedWindowPos DocumentView::document_to_window_pos(UncenteredDocumentPos doc_pos) {

    if (current_document) {
        WindowPos window_pos = document_to_window_pos_in_pixels_uncentered(doc_pos);
        double halfwidth = static_cast<double>(view_width) / 2;
        double halfheight = static_cast<double>(view_height) / 2;

        float window_x = static_cast<float>((window_pos.x - halfwidth) / halfwidth);
        float window_y = static_cast<float>((window_pos.y - halfheight) / halfheight);
        return { window_x, -window_y };
    }
}

NormalizedWindowPos DocumentView::document_to_window_pos(CenteredDocumentPos doc_pos) {

    if (current_document) {
        WindowPos window_pos = document_to_window_pos_in_pixels_centered(doc_pos);
        double halfwidth = static_cast<double>(view_width) / 2;
        double halfheight = static_cast<double>(view_height) / 2;

        float window_x = static_cast<float>((window_pos.x - halfwidth) / halfwidth);
        float window_y = static_cast<float>((window_pos.y - halfheight) / halfheight);
        return { window_x, -window_y };
    }
}

//WindowPos DocumentView::document_to_window_pos_in_pixels(UncenteredDocumentPos doc_pos) {
//    return document_to_window_pos_in_pixels(DocumentPos { doc_pos.page, doc_pos.x, doc_pos.y });
//}

WindowPos DocumentView::absolute_to_window_pos_in_pixels(AbsoluteDocumentPos abspos, int page) {
    WindowPos window_pos;
    window_pos.y = (abspos.y - offset_y) * zoom_level + view_height / 2;
    window_pos.x = (abspos.x + offset_x) * zoom_level + view_width / 2;
    return window_pos;
}

WindowPos DocumentView::document_to_window_pos_in_pixels_uncentered(UncenteredDocumentPos doc_pos) {
    AbsoluteDocumentPos abspos = current_document->document_to_absolute_pos(doc_pos);
    return absolute_to_window_pos_in_pixels(abspos, doc_pos.page);
}

WindowPos DocumentView::document_to_window_pos_in_pixels_centered(CenteredDocumentPos doc_pos) {
    AbsoluteDocumentPos abspos = current_document->document_to_absolute_pos(doc_pos);
    return absolute_to_window_pos_in_pixels(abspos, doc_pos.page);
}

WindowPos DocumentView::document_to_window_pos_in_pixels_banded(UncenteredDocumentPos doc_pos) {
    AbsoluteDocumentPos abspos = current_document->document_to_absolute_pos(doc_pos);
    WindowPos window_pos;
    window_pos.y = static_cast<int>(std::roundf((abspos.y - offset_y) * zoom_level + static_cast<float>(view_height) / 2.0f));
    window_pos.x = static_cast<int>(std::roundf((abspos.x + offset_x) * zoom_level + static_cast<float>(view_width) / 2.0f));
    //window_pos.x = static_cast<int>(std::roundf((abspos.x + offset_x - current_document->get_page_width(doc_pos.page) / 2) * zoom_level + static_cast<float>(view_width) / 2));
    return window_pos;
}

fz_irect DocumentView::document_to_window_irect(int page, fz_rect doc_rect) {

    fz_irect window_rect;

    WindowPos bottom_left = document_to_window_pos_in_pixels_uncentered(UncenteredDocumentPos { page, doc_rect.x0, doc_rect.y0 });
    WindowPos top_right = document_to_window_pos_in_pixels_uncentered(UncenteredDocumentPos { page, doc_rect.x1, doc_rect.y1 });
    window_rect.x0 = bottom_left.x;
    window_rect.y0 = bottom_left.y;
    window_rect.x1 = top_right.x;
    window_rect.y1 = top_right.y;
    return window_rect;
}

fz_rect DocumentView::document_to_window_rect(int page, fz_rect doc_rect) {
    fz_rect res;

    NormalizedWindowPos p0 = document_to_window_pos(UncenteredDocumentPos { page, doc_rect.x0, doc_rect.y0 });
    NormalizedWindowPos p1 = document_to_window_pos(UncenteredDocumentPos { page, doc_rect.x1, doc_rect.y1 });

    res.x0 = p0.x;
    res.x1 = p1.x;
    res.y0 = p0.y;
    res.y1 = p1.y;

    return res;
}

fz_rect DocumentView::document_to_window_rect_pixel_perfect(int page, fz_rect doc_rect, int pixel_width, int pixel_height, bool banded) {

    if ((pixel_width <= 0) || (pixel_height <= 0)) {
        return document_to_window_rect(page, doc_rect);
    }


    WindowPos w0, w1;
    if (banded) {
        w0 = document_to_window_pos_in_pixels_banded({ page, doc_rect.x0, doc_rect.y0 });
        w1 = document_to_window_pos_in_pixels_banded({ page, doc_rect.x1, doc_rect.y1 });
    }
    else {
        w0 = document_to_window_pos_in_pixels_uncentered(UncenteredDocumentPos { page, doc_rect.x0, doc_rect.y0 });
        w1 = document_to_window_pos_in_pixels_uncentered(UncenteredDocumentPos { page, doc_rect.x1, doc_rect.y1 });
    }

    w1.x -= ((w1.x - w0.x) - pixel_width);
    if (!banded) {
        //w1.y -= ((w1.y - w0.y) - pixel_height);
        w1.y -= ((w1.y - w0.y) - pixel_height);
    }

    NormalizedWindowPos p0 = window_to_normalized_window_pos(w0);
    NormalizedWindowPos p1 = window_to_normalized_window_pos(w1);

    fz_rect res;
    res.x0 = p0.x;
    res.x1 = p1.x;
    res.y0 = -p0.y;
    res.y1 = -p1.y;

    return res;
}

UncenteredDocumentPos DocumentView::window_to_document_pos_uncentered(WindowPos window_pos) {
    if (current_document) {
        return current_document->absolute_to_page_pos_uncentered(
            { (window_pos.x - view_width / 2) / zoom_level - offset_x,
            (window_pos.y - view_height / 2) / zoom_level + offset_y });
    }
    else {
        return { -1, 0, 0 };
    }
}

UncenteredDocumentPos DocumentView::window_to_document_pos(WindowPos window_pos) {
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
    float normal_y = 2 * (static_cast<float>(window_pos.y) - view_height / 2.0f) / view_height;
    return { normal_x, normal_y };
}


void DocumentView::goto_mark(char symbol) {
    if (current_document) {
        float new_y_offset = 0.0f;
        if (current_document->get_mark_location_if_exists(symbol, &new_y_offset)) {
            set_offset_y(new_y_offset);
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

//void DocumentView::search_text(const wchar_t* text) {
//	if (!current_document) return;
//
//	search_results_mutex.lock();
//	search_results.clear();
//	current_search_result_index = 0;
//	search_results_mutex.unlock();
//
//	is_searching = true;
//	pdf_renderer->add_request(current_document->get_path(), 
//		get_current_page_number(), text, &search_results, &percent_done, &is_searching, &search_results_mutex);
//}

//void DocumentView::goto_search_result(int offset) {
//	if (!current_document) return;
//
//	search_results_mutex.lock();
//	if (search_results.size() == 0) {
//		search_results_mutex.unlock();
//		return;
//	}
//
//	int target_index = mod(current_search_result_index + offset, search_results.size());
//	current_search_result_index = target_index;
//
//	int target_page = search_results[target_index].page;
//
//	fz_rect rect = search_results[target_index].rect;
//
//	float new_offset_y = rect.y0 + current_document->get_accum_page_height(target_page);
//
//	set_offset_y(new_offset_y);
//	search_results_mutex.unlock();
//}

void DocumentView::get_visible_pages(int window_height, std::vector<int>& visible_pages) {
    if (!current_document) return;

    float window_y_range_begin = offset_y - window_height / (1.5 * zoom_level);
    float window_y_range_end = offset_y + window_height / (1.5 * zoom_level);
    window_y_range_begin -= 1;
    window_y_range_end += 1;

    current_document->get_visible_pages(window_y_range_begin, window_y_range_end, visible_pages);

    //float page_begin = 0.0f;

    //const vector<float>& page_heights = current_document->get_page_heights();
    //for (int i = 0; i < page_heights.size(); i++) {
    //	float page_end = page_begin + page_heights[i];

    //	if (intersects(window_y_range_begin, window_y_range_end, page_begin, page_end)) {
    //		visible_pages.push_back(i);
    //	}
    //	page_begin = page_end;
    //}
    //cout << "num visible pages:" << visible_pages.size() << endl;
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

void DocumentView::goto_offset_within_page(CenteredDocumentPos pos) {
    set_offsets(pos.x, get_page_offset(pos.page) + pos.y);
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

void DocumentView::fit_to_page_height_width_minimum() {
    int cp = get_center_page_number();
    if (cp == -1) return;

    int page_width = current_document->get_page_width(cp);
    int page_height = current_document->get_page_height(cp);

    float x_zoom_level = static_cast<float>(view_width) / page_width;
    float y_zoom_level;
    if (IGNORE_STATUSBAR_IN_PRESENTATION_MODE) {
        y_zoom_level = (static_cast<float>(view_height)) / page_height;
    }
    else {
        y_zoom_level = (static_cast<float>(view_height) - get_status_bar_height()) / page_height;
    }

    set_offset_x(0);
    set_zoom_level(std::min(x_zoom_level, y_zoom_level), true);

}

void DocumentView::persist(bool persist_drawings) {
    if (!current_document) return;
    db_manager->update_book(current_document->get_checksum(), zoom_level, offset_x, offset_y);
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

void DocumentView::set_vertical_line_rect(fz_rect rect) {
    ruler_rect = rect;
    is_ruler_mode_ = true;
}

//float DocumentView::get_vertical_line_pos() {
//	return vertical_line_pos;
//}

float DocumentView::get_ruler_pos() {
    if (ruler_rect.has_value()) {
        return ruler_rect.value().y1;
    }
    else {
        return ruler_pos;
    }
}

std::optional<fz_rect> DocumentView::get_ruler_rect() {
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

std::optional<fz_rect> DocumentView::get_ruler_window_rect() {
    if (has_ruler_rect()) {
        fz_rect absol_ruler_rect = get_ruler_rect().value();

        absol_ruler_rect.y0 -= RULER_PADDING;
        absol_ruler_rect.y1 += RULER_PADDING;

        absol_ruler_rect.x0 -= RULER_X_PADDING;
        absol_ruler_rect.x1 += RULER_X_PADDING;
        return absolute_to_window_rect(absol_ruler_rect);
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
    std::deque<fz_rect>& selected_characters,
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

float DocumentView::get_max_valid_x() {
    float page_width = current_document->get_page_width(get_center_page_number());
    return std::abs(-view_width / zoom_level / 2 + page_width / 2);
}

float DocumentView::get_min_valid_x() {
    float page_width = current_document->get_page_width(get_center_page_number());
    return -std::abs(-view_width / zoom_level / 2 + page_width / 2);
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
    UncenteredDocumentPos line_doc_pos = current_document->absolute_to_page_pos_uncentered({ 0, get_ruler_pos() });
    auto rects = current_document->get_page_lines(line_doc_pos.page);
    int index = 0;
    while ((size_t)index < rects.size() && rects[index].y0 < get_ruler_pos()) {
        index++;
    }
    return index - 1;
}

int DocumentView::get_line_index_of_pos(UncenteredDocumentPos line_doc_pos) {
    fz_point document_point = { line_doc_pos.x, line_doc_pos.y };
    auto rects = current_document->get_page_lines(line_doc_pos.page, nullptr);
    int page_width = current_document->get_page_width(line_doc_pos.page);

    for (int i = 0; i < rects.size(); i++) {
        rects[i] = current_document->absolute_to_page_rect(rects[i], nullptr);
    }
    for (int i = 0; i < rects.size(); i++) {
        if (fz_is_point_inside_rect(document_point, rects[i])) {
            return i;
        }
    }
    return -1;
}

int DocumentView::get_vertical_line_page() {
    return current_document->absolute_to_page_pos({ 0, get_ruler_pos() }).page;
}

std::optional<std::wstring> DocumentView::get_selected_line_text() {
    if (line_index >= 0) {
        std::vector<std::wstring> lines;
        //std::vector<fz_rect> line_rects = current_document->get_page_lines(get_center_page_number(), &lines);
        std::vector<fz_rect> line_rects = current_document->get_page_lines(get_vertical_line_page(), &lines);
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

void DocumentView::get_rects_from_ranges(int page_number, const std::vector<fz_rect>& line_char_rects, const std::vector<std::pair<int, int>>& ranges, std::vector<fz_rect>& out_rects) {
    for (int i = 0; i < ranges.size(); i++) {
        auto [first, last] = ranges[i];
        fz_rect current_source_rect = get_range_rect_union(line_char_rects, first, last);
        current_source_rect = current_document->document_to_absolute_rect(page_number, current_source_rect);
        out_rects.push_back(current_source_rect);
    }
}

//std::vector<std::pair<DocumentPos, fz_rect>> DocumentView::find_line_definitions() {
std::vector<SmartViewCandidate> DocumentView::find_line_definitions() {
    //todo: remove duplicate code from this function, this just needs to find the location of the
    // reference, the rest can be handled by find_definition_of_location

    //std::vector<std::pair<DocumentPos, fz_rect>> result;
    std::vector<SmartViewCandidate> result;

    if (line_index > 0) {
        std::vector<std::wstring> lines;
        std::vector<std::vector<fz_rect>> line_char_rects;

        int line_page_number = get_vertical_line_page();

        std::vector<fz_rect> line_rects = current_document->get_page_lines(line_page_number, &lines, &line_char_rects);
        for (int i = 0; i < lines.size(); i++) {
            assert(lines[i].size() == line_char_rects[i].size());
        }
        if ((size_t)line_index < lines.size()) {
            std::wstring content = lines[line_index];

            //todo: deduplicate this code

            fz_rect line_rect = line_rects[line_index];
            float mid_y = (line_rect.y1 + line_rect.y0) / 2.0f;
            line_rect.y0 = line_rect.y0 = mid_y;

            std::vector<PdfLink> pdf_links = current_document->get_links_in_page_rect(get_vertical_line_page(), line_rect);
            if (pdf_links.size() > 0) {

                for (auto link : pdf_links) {
                    auto parsed_uri = parse_uri(mupdf_context, link.uri);
                    SmartViewCandidate candid;
                    candid.doc = get_document();
                    candid.source_rect = current_document->document_to_absolute_rect(line_page_number, link.rects[0]);
                    candid.source_text = get_document()->get_pdf_link_text(link);
                    candid.target_pos = UncenteredDocumentPos{ parsed_uri.page - 1, parsed_uri.x, parsed_uri.y };
                    result.push_back(candid);
                    //result.push_back(
                    //    std::make_pair(
                    //        DocumentPos{ parsed_uri.page - 1, parsed_uri.x, parsed_uri.y },
                    //        current_document->document_to_absolute_rect(line_page_number, link.rects[0], true)
                    //    )
                    //);
                    //if (src_rects) src_rects->push_back(link.rect);
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

            std::vector<fz_rect> generic_item_rects;
            std::vector<fz_rect> reference_rects;
            std::vector<fz_rect> equation_rects;

            int ruler_page = get_vertical_line_page();
            get_rects_from_ranges(ruler_page, line_char_rects[line_index], generic_item_ranges, generic_item_rects);
            get_rects_from_ranges(ruler_page, line_char_rects[line_index], reference_ranges, reference_rects);
            get_rects_from_ranges(ruler_page, line_char_rects[line_index], equation_ranges, equation_rects);

            //std::vector<std::pair<DocumentPos, fz_rect>> generic_positions;
            //std::vector<std::pair<DocumentPos, fz_rect>> reference_positions;
            //std::vector<std::pair<DocumentPos, fz_rect>> equation_positions;

            std::vector<SmartViewCandidate> generic_positions;
            std::vector<SmartViewCandidate> reference_positions;
            std::vector<SmartViewCandidate> equation_positions;


            //for (auto generic_item_text : generic_item_texts) {
            for (int i = 0; i < generic_item_texts.size(); i++) {
                std::vector<IndexedData> possible_targets = current_document->find_generic_with_string(generic_item_texts[i], ruler_page);
                for (int j = 0; j < possible_targets.size(); j++) {
                    SmartViewCandidate candid;
                    candid.doc = get_document();
                    candid.source_rect = generic_item_rects[i];
                    candid.source_text = generic_item_texts[i];
                    candid.target_pos = UncenteredDocumentPos{ possible_targets[j].page, 0, possible_targets[j].y_offset };
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
                            std::vector<fz_rect> subrects;
                            get_rects_from_ranges(ruler_page, line_char_rects[line_index], {std::make_pair(rect_range_begin, rect_range_end)}, subrects);

                            SmartViewCandidate candid;
                            candid.doc = get_document();
                            candid.source_rect = subrects[0];
                            candid.source_text = parts[j].trimmed().toStdWString();
                            candid.target_pos = UncenteredDocumentPos{ index[0].page, 0, index[0].y_offset };
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
                    candid.target_pos = UncenteredDocumentPos{ index[0].page, 0, index[0].y_offset };
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
                    candid.target_pos = UncenteredDocumentPos{ index[0].page, 0, index[0].y_offset };
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
        UncenteredDocumentPos docpos = defloc[0].get_docpos(this);
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
            ParsedUri parsed_uri = parse_uri(mupdf_context, link.uri);
            fz_rect window_rect = document_to_window_rect(page, link.rects[0]);
            if ((window_rect.x0 >= -1) && (window_rect.x0 <= 1) && (window_rect.y0 >= -1) && (window_rect.y0 <= 1)) {
                visible_page_links.push_back(link);
            }
        }
    }
}

std::deque<fz_rect>* DocumentView::get_selected_character_rects() {
    return &this->selected_character_rects;
}

std::optional<fz_rect> DocumentView::get_control_rect() {
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

std::optional<fz_rect> DocumentView::shrink_selection(bool is_begin, bool word) {
    if (selected_character_rects.size() > 1) {
        if (word) {
            int page;
            int index = is_begin ? 0 : selected_character_rects.size() - 1;
            fz_rect page_rect = current_document->absolute_to_page_rect(selected_character_rects[index], &page);
            if (page >= 0) {
                fz_stext_page* stext_page = current_document->get_stext_with_page_number(page);
                std::optional<fz_rect> new_rect_ = find_shrinking_rect_word(is_begin, stext_page, page_rect);
                if (new_rect_) {
                    fz_rect new_rect = current_document->document_to_absolute_rect(page, new_rect_.value());

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

std::optional<fz_rect> DocumentView::expand_selection(bool is_begin, bool word) {
    //current_document->get_stext_with_page_number()
    if (selected_character_rects.size() > 0) {
        int page;
        int index = is_begin ? 0 : selected_character_rects.size() - 1;

        fz_rect page_rect = current_document->absolute_to_page_rect(selected_character_rects[index], &page);
        if (page >= 0) {
            fz_stext_page* stext_page = current_document->get_stext_with_page_number(page);
            std::optional<fz_rect> next_rect = {};
            if (word) {
                std::vector<fz_rect> next_rects = find_expanding_rect_word(is_begin, stext_page, page_rect);
                for (int i = 0; i < next_rects.size(); i++) {
                    next_rects[i] = current_document->document_to_absolute_rect(page, next_rects[i]);
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
                fz_rect next_rect_abs = current_document->document_to_absolute_rect(page, next_rect.value());
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

fz_irect DocumentView::normalized_to_window_rect(fz_rect normalized_rect) {
    fz_irect res;
    res.x0 = static_cast<int>(normalized_rect.x0 * view_width / 2 + view_width / 2);
    res.x1 = static_cast<int>(normalized_rect.x1 * view_width / 2 + view_width / 2);
    res.y0 = static_cast<int>(-normalized_rect.y0 * view_height / 2 + view_height / 2);
    res.y1 = static_cast<int>(-normalized_rect.y1 * view_height / 2 + view_height / 2);

    return res;
}

bool DocumentView::is_ruler_mode() {
    return is_ruler_mode_;
}

Document* SmartViewCandidate::get_document(DocumentView* view) {
    if (doc) return doc;
    return view->get_document();

}

UncenteredDocumentPos SmartViewCandidate::get_docpos(DocumentView* view) {
    if (std::holds_alternative<UncenteredDocumentPos>(target_pos)) {
        return std::get<UncenteredDocumentPos>(target_pos);
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
        return get_document(view)->document_to_absolute_pos(std::get<UncenteredDocumentPos>(target_pos));
    }
}
