#include "document.h"
#include <algorithm>
#include <thread>
#include <cmath>
#include "utf8.h"
#include <qfileinfo.h>
#include <qdatetime.h>
#include <map>
#include <regex>
#include <qcryptographichash.h>
#include <qjsondocument.h>
#include "path.h"
#include <qjsonarray.h>
#include <qjsonobject.h>
#include <qjsondocument.h>
#include <qdir.h>
#include <qstandardpaths.h>
#include <set>

#include <mupdf/pdf.h>

#include "checksum.h"

extern bool SHOULD_RENDER_PDF_ANNOTATIONS;

const int WINDOW_SIZE = 10;
const int EMBEDDING_DIM = 10;

extern float SMALL_PIXMAP_SCALE;
extern float HIGHLIGHT_COLORS[26 * 3];
extern std::wstring TEXT_HIGHLIGHT_URL;
extern bool TEXT_SUMMARY_HIGHLIGHT_SHOULD_REFINE;
extern bool TEXT_SUMMARY_HIGHLIGHT_SHOULD_FILL;
extern int TEXT_SUMMARY_CONTEXT_SIZE;
extern bool USE_HEURISTIC_IF_TEXT_SUMMARY_NOT_AVAILABLE;
extern bool ENABLE_EXPERIMENTAL_FEATURES;
extern bool CREATE_TABLE_OF_CONTENTS_IF_NOT_EXISTS;
extern int MAX_CREATED_TABLE_OF_CONTENTS_SIZE;
extern bool FORCE_CUSTOM_LINE_ALGORITHM;
extern bool SUPER_FAST_SEARCH;
extern float EPUB_WIDTH;
extern float EPUB_HEIGHT;
extern QString EPUB_TEMPLATE;
extern float EPUB_FONT_SIZE;
extern std::wstring EPUB_CSS;
//extern std::vector<float> embedding_weights;
//extern std::vector<float> linear_weights;
extern float EPUB_LINE_SPACING;
extern Path standard_data_path;
extern bool VERBOSE;
extern float FREETEXT_BOOKMARK_COLOR[3];
extern float FREETEXT_BOOKMARK_FONT_SIZE;
extern std::wstring SHARED_DATABASE_PATH;
extern bool DEBUG;

int Document::get_mark_index(char symbol) {
    for (size_t i = 0; i < marks.size(); i++) {
        if (marks[i].symbol == symbol) {
            return i;
        }
    }
    return -1;
}

void Document::load_document_metadata_from_db() {

    marks.clear();
    bookmarks.clear();
    highlights.clear();
    portals.clear();
    portals.clear();

    std::optional<std::string> checksum_ = get_checksum_fast();
    if (checksum_) {
        std::string checksum = checksum_.value();
        db_manager->select_mark(checksum, marks);
        db_manager->select_bookmark(checksum, bookmarks);
        db_manager->select_highlight(checksum, highlights);
        db_manager->select_links(checksum, portals);
        should_reload_annotations = false;
    }
    else {
        auto checksum_thread = std::thread([&]() {
            std::string checksum = get_checksum();
            if ((checksummer->num_docs_with_checksum(checksum) > 1) || annotations_file_exists()) {
                if (marks.size() == 0 && bookmarks.size() == 0 && highlights.size() == 0 && portals.size() == 0) {
                    db_manager->select_mark(checksum, marks);
                    db_manager->select_bookmark(checksum, bookmarks);
                    db_manager->select_highlight(checksum, highlights);
                    db_manager->select_links(checksum, portals);
                }
                // we already have a document with the same hash so there might be
                // annotations that are not loaded
                should_reload_annotations = true;
            }
            db_manager->insert_document_hash(get_path(), checksum);
            });
        checksum_thread.detach();
        //checksum_thread.join();
    }
}


void Document::add_bookmark(const std::wstring& desc, float y_offset) {
    BookMark bookmark;
    bookmark.y_offset_ = y_offset;
    bookmark.description = desc;
    bookmark.uuid = new_uuid_utf8();
    bookmark.update_creation_time();
    bookmarks.push_back(bookmark);
    db_manager->insert_bookmark(get_checksum(), desc, y_offset, utf8_decode(bookmark.uuid));
    is_annotations_dirty = true;
}

void Document::add_marked_bookmark(const std::wstring& desc, AbsoluteDocumentPos pos) {
    BookMark bookmark;
    bookmark.description = desc;
    bookmark.y_offset_ = pos.y;
    bookmark.begin_x = pos.x;
    bookmark.begin_y = pos.y;
    bookmark.uuid = new_uuid_utf8();
    bookmark.update_creation_time();

    if (db_manager->insert_bookmark_marked(get_checksum(), desc, pos.x, pos.y, utf8_decode(bookmark.uuid))) {
        bookmarks.push_back(bookmark);
        is_annotations_dirty = true;
    }
}

int Document::add_incomplete_freetext_bookmark(fz_rect absrect) {
    BookMark bookmark;

    bookmark.begin_x = absrect.x0;
    bookmark.begin_y = absrect.y0;
    bookmark.end_x = absrect.x1;
    bookmark.end_y = absrect.y1;
    bookmark.color[0] = FREETEXT_BOOKMARK_COLOR[0];
    bookmark.color[1] = FREETEXT_BOOKMARK_COLOR[1];
    bookmark.color[2] = FREETEXT_BOOKMARK_COLOR[2];
    //bookmark.font_size = FREETEXT_BOOKMARK_FONT_SIZE;

    bookmark.uuid = new_uuid_utf8();
    bookmarks.push_back(bookmark);
    return bookmarks.size() - 1;
}

void Document::add_pending_freetext_bookmark(int index, const std::wstring& desc) {
    BookMark& bookmark = bookmarks[index];
    bookmark.description = desc;
    bookmark.font_size = FREETEXT_BOOKMARK_FONT_SIZE;
    bookmark.update_creation_time();

    if (!db_manager->insert_bookmark_freetext(get_checksum(), bookmark)) {
        undo_pending_bookmark(index);
    }
    else {
        is_annotations_dirty = true;
    }
}

void Document::undo_pending_bookmark(int index) {
    if (index >= 0 && index < bookmarks.size()) {
        bookmarks.erase(bookmarks.begin() + index);
    }
}

void Document::add_freetext_bookmark_with_color(const std::wstring& desc, fz_rect absrect, float* color, float font_size) {
    BookMark bookmark;
    bookmark.description = desc;
    bookmark.y_offset_ = absrect.y0;

    bookmark.begin_x = absrect.x0;
    bookmark.begin_y = absrect.y0;
    bookmark.end_x = absrect.x1;
    bookmark.end_y = absrect.y1;

    bookmark.color[0] = color[0];
    bookmark.color[1] = color[1];
    bookmark.color[2] = color[2];
    bookmark.font_size = font_size < 0 ? FREETEXT_BOOKMARK_FONT_SIZE : font_size;
    bookmark.uuid = new_uuid_utf8();
    bookmark.update_creation_time();

    if (db_manager->insert_bookmark_freetext(get_checksum(), bookmark)) {
        bookmarks.push_back(bookmark);
        is_annotations_dirty = true;
    }
}
void Document::add_freetext_bookmark(const std::wstring& desc, fz_rect absrect) {
    add_freetext_bookmark_with_color(desc, absrect, FREETEXT_BOOKMARK_COLOR);
}

void Document::fill_index_highlight_rects(int highlight_index, fz_context* thread_context, fz_document* thread_document) {

    if (highlight_index >= highlights.size()) {
        return;
    }
    if (thread_context == nullptr) thread_context = context;
    if (thread_document == nullptr) thread_document = doc;

    const Highlight highlight = highlights[highlight_index];
    std::deque<fz_rect> highlight_rects;
    std::vector<fz_rect> merged_rects;
    std::wstring highlight_text;
    get_text_selection(thread_context, highlight.selection_begin, highlight.selection_end, true, highlight_rects, highlight_text, thread_document);
    merge_selected_character_rects(highlight_rects, merged_rects, highlights[highlight_index].type != '_');
    highlights[highlight_index].highlight_rects = std::move(merged_rects);
}

void Document::fill_highlight_rects(fz_context* ctx, fz_document* doc_) {
    // Fill the `highlight_rects` attribute of all highlights with a vector of merged highlight rects of
    // individual characters in the highlighted area (we merge the rects of characters in a single line
    // which makes the result look nicer)

    for (size_t i = 0; i < highlights.size(); i++) {
        fill_index_highlight_rects(i, ctx, doc_);
    }
}


std::string Document::add_highlight(const std::wstring& annot, AbsoluteDocumentPos selection_begin, AbsoluteDocumentPos selection_end, char type) {
    std::deque<fz_rect> selected_characters;
    std::vector<fz_rect> merged_rects;
    std::wstring selected_text;
    get_text_selection(selection_begin, selection_end, true, selected_characters, selected_text);

    if ((type != '_') && (!((type <= 'z' && type >= 'a') || (type <= 'Z' && type >= 'A')))) {
        type = 'a';
    }

    merge_selected_character_rects(selected_characters, merged_rects);
    Highlight highlight;
    highlight.description = selected_text;
    highlight.text_annot = annot;
    highlight.selection_begin = selection_begin;
    highlight.selection_end = selection_end;
    highlight.type = type;
    highlight.highlight_rects = merged_rects;
    highlight.uuid = new_uuid_utf8();
    highlight.update_creation_time();

    if (db_manager->insert_highlight_with_annotation(
        get_checksum(),
        selected_text,
        annot,
        selection_begin.x,
        selection_begin.y,
        selection_end.x,
        selection_end.y,
        highlight.type,
        utf8_decode(highlight.uuid))) {
        highlights.push_back(highlight);
        is_annotations_dirty = true;
    }

    return highlight.uuid;
}

std::string Document::add_highlight(const std::wstring& desc,
    const std::vector<fz_rect>& highlight_rects,
    AbsoluteDocumentPos selection_begin,
    AbsoluteDocumentPos selection_end,
    char type)
{
    //if (type > 'z' || type < 'a') {
    if ((type != '_') && (!((type <= 'z' && type >= 'a') || (type <= 'Z' && type >= 'A')))) {
        type = 'a';
    }

    Highlight highlight;
    highlight.description = desc;
    highlight.selection_begin = selection_begin;
    highlight.selection_end = selection_end;
    highlight.type = type;
    highlight.highlight_rects = highlight_rects;
    highlight.uuid = new_uuid_utf8();
    highlight.update_creation_time();

    highlights.push_back(highlight);
    db_manager->insert_highlight(
        get_checksum(),
        desc,
        selection_begin.x,
        selection_begin.y,
        selection_end.x,
        selection_end.y,
        highlight.type,
        utf8_decode(highlight.uuid));
    is_annotations_dirty = true;
    return highlight.uuid;
}

bool Document::get_is_indexing() {
    return is_indexing;
}

int Document::add_portal(Portal portal, bool insert_into_database) {
    portal.uuid = new_uuid_utf8();
    portal.update_creation_time();
    portals.push_back(portal);
    int index = portals.size() - 1;
    if (insert_into_database) {
        if (portal.is_visible()) {
            bool res = db_manager->insert_visible_portal(
                get_checksum(),
                portal.dst.document_checksum,
                portal.dst.book_state.offset_x,
                portal.dst.book_state.offset_y,
                portal.dst.book_state.zoom_level,
                portal.src_offset_x.value(),
                portal.src_offset_y,
                utf8_decode(portal.uuid));
        }
        else {
            db_manager->insert_portal(
                get_checksum(),
                portal.dst.document_checksum,
                portal.dst.book_state.offset_x,
                portal.dst.book_state.offset_y,
                portal.dst.book_state.zoom_level,
                portal.src_offset_y,
                utf8_decode(portal.uuid));
        }
        is_annotations_dirty = true;
    }
    return index;
}


std::wstring Document::get_path() {

    return file_name;
}

std::string Document::get_checksum() {

    return checksummer->get_checksum(get_path());
}

std::optional<std::string> Document::get_checksum_fast() {

    return checksummer->get_checksum_fast(get_path());
}

int Document::find_closest_bookmark_index(const std::vector<BookMark>& sorted_bookmarks, float to_offset_y) const {

    int min_index = argminf<BookMark>(sorted_bookmarks, [to_offset_y](BookMark bm) {
        return abs(bm.get_y_offset() - to_offset_y);
        });

    return min_index;
}

int Document::find_closest_portal_index(const std::vector<Portal>& sorted_portals, float to_offset_y) const {

    int min_index = argminf<Portal>(sorted_portals, [to_offset_y](Portal portal) {
        return abs(portal.src_offset_y - to_offset_y);
        });

    return min_index;
}

int Document::find_closest_highlight_index(const std::vector<Highlight>& sorted_highlights, float to_offset_y) const {

    int min_index = argminf<Highlight>(sorted_highlights, [to_offset_y](Highlight hl) {
        return abs(hl.selection_begin.y - to_offset_y);
        });

    return min_index;
}

void Document::delete_closest_bookmark(float to_y_offset) {
    int closest_index = find_closest_bookmark_index(bookmarks, to_y_offset);
    if (closest_index > -1) {
        db_manager->delete_bookmark(bookmarks[closest_index].uuid);
        is_annotations_dirty = true;
        bookmarks.erase(bookmarks.begin() + closest_index);
    }
}

void Document::delete_bookmark(int index) {

    if ((index != -1) && (index < bookmarks.size())) {
        if (db_manager->delete_bookmark(bookmarks[index].uuid)) {
            bookmarks.erase(bookmarks.begin() + index);
            is_annotations_dirty = true;
        }
    }
}

void Document::delete_highlight_with_index(int index) {
    Highlight highlight_to_delete = highlights[index];

    db_manager->delete_highlight(highlight_to_delete.uuid);
    highlights.erase(highlights.begin() + index);
    is_annotations_dirty = true;
}

void Document::delete_highlight(Highlight hl) {
    for (size_t i = (highlights.size() - 1); i >= 0; i--) {
        if (highlights[i] == hl) {
            delete_highlight_with_index(i);
            return;
        }
    }
}

std::optional<Portal> Document::find_closest_portal(float to_offset_y, int* index) {
    int min_index = argminf<Portal>(portals, [to_offset_y](Portal l) {
        return abs(l.src_offset_y - to_offset_y);
        });

    if (min_index >= 0) {
        if (index) *index = min_index;
        return portals[min_index];
    }
    return {};
}

bool Document::update_portal(Portal new_portal) {
    for (auto& portal : portals) {
        if (portal.src_offset_y == new_portal.src_offset_y) {
            portal.dst.book_state = new_portal.dst.book_state;
            return true;
        }
    }
    return false;
}

void Document::delete_closest_portal(float to_offset_y) {
    int closest_index = -1;
    if (find_closest_portal(to_offset_y, &closest_index)) {
        db_manager->delete_portal(portals[closest_index].uuid);
        portals.erase(portals.begin() + closest_index);
        is_annotations_dirty = true;
    }
}

int Document::get_portal_index_with_uuid(const std::string& uuid) {
    for (int i = 0; i < portals.size(); i++) {
        if (portals[i].uuid == uuid) {
            return i;
        }
    }
    return -1;
}

void Document::delete_portal_with_uuid(const std::string& uuid) {
    int index = get_portal_index_with_uuid(uuid);
    if (index > -1) {
        db_manager->delete_portal(uuid);
        portals.erase(portals.begin() + index);
    }
}

std::vector<BookMark>& Document::get_bookmarks() {
    return bookmarks;
}

std::vector<BookMark> Document::get_sorted_bookmarks() const {
    std::vector<BookMark> res = bookmarks;
    std::sort(res.begin(), res.end(), [](const BookMark& lhs, const BookMark& rhs) {return lhs.get_y_offset() < rhs.get_y_offset(); });
    return res;
}

std::vector<Portal> Document::get_sorted_portals() const {
    std::vector<Portal> res = portals;
    std::sort(res.begin(), res.end(), [](const Portal& lhs, const Portal& rhs) {return lhs.src_offset_y < rhs.src_offset_y; });
    return res;
}

const std::vector<Highlight>& Document::get_highlights() const {
    return highlights;
}

const std::vector<Highlight> Document::get_highlights_of_type(char type) const {
    std::vector<Highlight> res;

    for (auto hl : highlights) {
        if (hl.type == type) {
            res.push_back(hl);
        }
    }
    return res;
}

const std::vector<Highlight> Document::get_highlights_sorted(char type) const {
    std::vector<Highlight> res;

    if (type == 0) {
        res = highlights;
    }
    else {
        res = get_highlights_of_type(type);
    }

    std::sort(res.begin(), res.end(), [](const Highlight& hl1, const Highlight& hl2) {
        return hl1.selection_begin.y < hl2.selection_begin.y;
        });

    return res;
}


void Document::add_mark(char symbol, float y_offset) {
    int current_mark_index = get_mark_index(symbol);
    if (current_mark_index == -1) {
        Mark m;
        m.y_offset = y_offset;
        m.symbol = symbol;
        m.update_creation_time();
        marks.push_back(m);
        db_manager->insert_mark(get_checksum(), symbol, y_offset, new_uuid());
        is_annotations_dirty = true;
    }
    else {
        marks[current_mark_index].y_offset = y_offset;
        marks[current_mark_index].update_modification_time();
        db_manager->update_mark(get_checksum(), symbol, y_offset);
        is_annotations_dirty = true;
    }
}

bool Document::remove_mark(char symbol) {
    for (size_t i = 0; i < marks.size(); i++) {
        if (marks[i].symbol == symbol) {
            marks.erase(marks.begin() + i);
            return true;
        }
    }
    return false;
}

bool Document::get_mark_location_if_exists(char symbol, float* y_offset) {
    int mark_index = get_mark_index(symbol);
    if (mark_index == -1) {
        return false;
    }
    *y_offset = marks[mark_index].y_offset;
    return true;
}

Document::Document(fz_context* context, std::wstring file_name, DatabaseManager* db, CachedChecksummer* checksummer) :
    db_manager(db),
    context(context),
    file_name(file_name),
    checksummer(checksummer),
    doc(nullptr) {
    last_update_time = QDateTime::currentDateTime();
    should_render_annotations = SHOULD_RENDER_PDF_ANNOTATIONS;
}

void Document::count_chapter_pages(std::vector<int>& page_counts) {
    int num_chapters = fz_count_chapters(context, doc);

    for (int i = 0; i < num_chapters; i++) {
        int num_pages = fz_count_chapter_pages(context, doc, i);
        page_counts.push_back(num_pages);
    }
}

void Document::count_chapter_pages_accum(std::vector<int>& accum_page_counts) {
    std::vector<int> raw_page_count;
    count_chapter_pages(raw_page_count);

    int accum = 0;

    for (size_t i = 0; i < raw_page_count.size(); i++) {
        accum_page_counts.push_back(accum);
        accum += raw_page_count[i];
    }
}

const std::vector<TocNode*>& Document::get_toc() {
    if (top_level_toc_nodes.size() > 0) {
        return top_level_toc_nodes;
    }
    else {
        return created_top_level_toc_nodes;
    }
}

bool Document::has_toc() {
    return top_level_toc_nodes.size() > 0 || created_top_level_toc_nodes.size() > 0;
}

const std::vector<std::wstring>& Document::get_flat_toc_names() {
    return flat_toc_names;
}

const std::vector<int>& Document::get_flat_toc_pages() {
    return flat_toc_pages;
}

float Document::get_page_height(int page_index) {
    std::lock_guard guard(page_dims_mutex);
    if ((page_index >= 0) && (page_index < page_heights.size())) {
        return page_heights[page_index];
    }
    else {
        return -1.0f;
    }
}

float Document::get_page_width(int page_index) {
    if ((page_index >= 0) && (page_index < page_widths.size())) {
        return page_widths[page_index];
    }
    else {
        return -1.0f;
    }
}

float Document::get_page_size_smart(bool width, int page_index, float* left_ratio, float* right_ratio, int* normal_page_width) {

    fz_pixmap* pixmap = get_small_pixmap(page_index);

    // project the small pixmap into a histogram (the `width` parameter determines whether we do this
    // horizontally or vertically)
    std::vector<float> histogram;

    if (width) {
        histogram.resize(pixmap->w);
    }
    else {
        histogram.resize(pixmap->h);
    }

    float total_nonzero = 0.0f;
    for (int i = 0; i < pixmap->w; i++) {
        for (int j = 0; j < pixmap->h; j++) {
            int index = 3 * pixmap->w * j + 3 * i;

            int r = pixmap->samples[index];
            int g = pixmap->samples[index + 1];
            int b = pixmap->samples[index + 2];

            float brightness = static_cast<float>(r + g + b) / 3.0f;
            if (brightness < 250) {
                if (width) {
                    histogram[i] += 1;
                }
                else {
                    histogram[j] += 1;
                }
                total_nonzero += 1;
            }
        }
    }

    float average_nonzero = total_nonzero / histogram.size();
    float nonzero_threshold = average_nonzero / 3.0f;

    int start_index = 0;
    int end_index = histogram.size() - 1;

    // find the first index in both directions where the histogram is nonzero
    while ((start_index < end_index) && (histogram[start_index++] < nonzero_threshold));
    while ((start_index < end_index) && (histogram[end_index--] < nonzero_threshold));


    int border = 10;
    start_index = std::max<int>(start_index - border, 0);
    end_index = std::min<int>(end_index + border, histogram.size() - 1);

    *left_ratio = static_cast<float>(start_index) / histogram.size();
    *right_ratio = static_cast<float>(end_index) / histogram.size();

    float standard_size;
    if (width) {
        standard_size = page_widths[page_index];
    }
    else {
        standard_size = page_heights[page_index];
    }

    float ratio = static_cast<float>(end_index - start_index) / histogram.size();

    *normal_page_width = standard_size;

    return ratio * standard_size;
}

float Document::get_accum_page_height(int page_index) {
    std::lock_guard guard(page_dims_mutex);
    if (page_index < 0 || (page_index >= accum_page_heights.size())) {
        return 0.0f;
    }
    return accum_page_heights[page_index];
}


fz_outline* Document::get_toc_outline() {
    fz_outline* res = nullptr;
    fz_try(context) {
        res = fz_load_outline(context, doc);
    }
    fz_catch(context) {
        //std::cerr << "Error: Could not load outline ... " << std::endl;
    }
    return res;
}

void Document::create_toc_tree(std::vector<TocNode*>& toc) {
    fz_try(context) {
        fz_outline* outline = get_toc_outline();
        if (outline) {
            convert_toc_tree(outline, toc);
            fz_drop_outline(context, outline);
        }
        else {
            //create_table_of_contents(toc);
        }
    }
    fz_catch(context) {
        //std::cerr << "Error: Could not load outline ... " << std::endl;
    }
}

void Document::convert_toc_tree(fz_outline* root, std::vector<TocNode*>& output) {
    // convert an fz_outline structure to a tree of TocNodes

    std::vector<int> accum_chapter_pages;
    count_chapter_pages_accum(accum_chapter_pages);

    do {
        if (root == nullptr || root->title == nullptr) {
            break;
        }

        TocNode* current_node = new TocNode;
        current_node->title = utf8_decode(root->title);
        current_node->x = root->x;
        current_node->y = root->y;
        if (root->page.page == -1) {
            float xp, yp;
            fz_location loc = fz_resolve_link(context, doc, root->uri, &xp, &yp);
            int chapter_page = accum_chapter_pages[loc.chapter];
            current_node->page = chapter_page + loc.page;
        }
        else {
            current_node->page = root->page.page;
        }
        convert_toc_tree(root->down, current_node->children);


        output.push_back(current_node);
    } while ((root = root->next));
}

PdfLink Document::merge_links(const std::vector<PdfLink>& links_to_merge) {
    PdfLink merged_link;
    merged_link.uri = links_to_merge[0].uri;
    merged_link.source_page = links_to_merge[0].source_page;
    std::vector<fz_rect> rects;
    fz_rect current_rect = links_to_merge[0].rects[0];
    for (int i = 1; i < links_to_merge.size(); i++) {
        fz_rect new_rect = links_to_merge[i].rects[0];
        float height = std::abs(current_rect.y1 - current_rect.y0);
        if (std::abs(current_rect.y1 - new_rect.y1) < height / 5) {
            current_rect.x0 = std::min(current_rect.x0, new_rect.x0);
            current_rect.x1 = std::max(current_rect.x1, new_rect.x1);
        }
        else {
            rects.push_back(current_rect);
            current_rect = new_rect;
        }
    }
    rects.push_back(current_rect);
    merged_link.rects = rects;
    return merged_link;
}

const std::vector<PdfLink>& Document::get_page_merged_pdf_links(int page_number) {
    if (cached_merged_pdf_links.find(page_number) != cached_merged_pdf_links.end()) {
        return cached_merged_pdf_links[page_number];
    }

    std::vector<PdfLink> res;

    std::vector<PdfLink> links_to_merge;

    fz_link* current_link = get_page_links(page_number);

    while (current_link) {
        if (links_to_merge.size() > 0 && (links_to_merge.back().uri != current_link->uri)) {
            PdfLink merged_link = merge_links(links_to_merge);
            //merged_link.uri = links_to_merge[0].uri;
            //merged_link.source_page = links_to_merge[0].source_page;
            //for (int i = 0; i < links_to_merge.size(); i++) {
            //    merged_link.rects.push_back(links_to_merge[i].rects[0]);
            //}
            res.push_back(merged_link);
            links_to_merge.clear();
        }
        links_to_merge.push_back(pdf_link_from_fz_link(page_number, current_link));

        current_link = current_link->next;
    }

    if (links_to_merge.size() > 0) {
        res.push_back(merge_links(links_to_merge));
    }

    cached_merged_pdf_links[page_number] = res;
    return cached_merged_pdf_links[page_number];
}

fz_link* Document::get_page_links(int page_number) {
    if (cached_page_links.find(page_number) != cached_page_links.end()) {
        return cached_page_links.at(page_number);
    }

    fz_link* res = nullptr;
    fz_try(context) {
        fz_page* page = fz_load_page(context, doc, page_number);
        res = fz_load_links(context, page);
        cached_page_links[page_number] = res;
        fz_drop_page(context, page);
    }

    fz_catch(context) {
        LOG(std::cerr << "Error: Could not load links" << std::endl);
        res = nullptr;
    }
    return res;
}

QDateTime Document::get_last_edit_time() {
    QFileInfo info(QString::fromStdWString(get_path()));
    return info.lastModified();
}

unsigned int Document::get_milies_since_last_document_update_time() {
    QDateTime now = QDateTime::currentDateTime();
    return last_update_time.msecsTo(now);

}

unsigned int Document::get_milies_since_last_edit_time() {
    QDateTime last_modified_time = get_last_edit_time();
    QDateTime now = QDateTime::currentDateTime();
    return last_modified_time.msecsTo(now);
}

Document::~Document() {
    if (document_indexing_thread.has_value()) {
        stop_indexing();
        document_indexing_thread.value().join();
    }

    if (doc != nullptr) {
        fz_try(context) {
            fz_drop_document(context, doc);
            //todo: implement rest of destructor
        }
        fz_catch(context) {
            LOG(std::cerr << "Error: could not drop documnet" << std::endl);
        }
    }
}
void Document::reload(std::string password) {

    fz_drop_document(context, doc);
    clear_document_caches();

    doc = nullptr;

    open(invalid_flag_pointer, false, password);
}

bool Document::open(bool* invalid_flag, bool force_load_dimensions, std::string password, bool temp) {
    last_update_time = QDateTime::currentDateTime();
    if (doc == nullptr) {
        fz_try(context) {
            //			doc = fz_open_document(context, utf8_encode(file_name).c_str());
            doc = open_document_with_file_name(context, file_name);
            document_needs_password = fz_needs_password(context, doc);
            if (password.size() > 0) {
                int auth_res = fz_authenticate_password(context, doc, password.c_str());
                if (auth_res > 0) {
                    password_was_correct = true;
                }
            }
            //fz_layout_document(context, doc, 600, 800, 9);
        }
        fz_catch(context) {
            LOG(std::wcerr << "could not open " << file_name << std::endl);
        }
        if ((doc != nullptr) && (!temp)) {
            //load_document_metadata_from_db();
            load_document_caches(invalid_flag, force_load_dimensions);
            return true;
        }


        return false;
    }
    else {
        //std::cerr << "warning! calling open() on an open document" << std::endl;
        return true;
    }
}


//const vector<float>& get_page_heights();
//const vector<float>& get_page_widths();
//const vector<float>& get_accum_page_heights();

void Document::get_visible_pages(float doc_y_range_begin, float doc_y_range_end, std::vector<int>& visible_pages) {

    std::lock_guard guard(page_dims_mutex);

    if (page_heights.size() == 0) {
        return;
    }

    float page_begin = 0.0f;

    for (size_t i = 0; i < page_heights.size(); i++) {
        float page_end = page_begin + page_heights[i];

        if (range_intersects(doc_y_range_begin, doc_y_range_end, page_begin, page_end)) {
            visible_pages.push_back(i);
        }
        page_begin = page_end;
    }
}

void Document::load_page_dimensions(bool force_load_now) {
    page_heights.clear();
    accum_page_heights.clear();
    page_widths.clear();
    page_labels.clear();

    int n = num_pages();
    float acc_height = 0.0f;
    // initially assume all pages have the same dimensions, correct these heights
    // when the background thread is done
    if (n > 0) {
        fz_try(context) {
            fz_page* page = fz_load_page(context, doc, n / 2);
            fz_rect bounds = fz_bound_page(context, page);
            fz_drop_page(context, page);
            int height = bounds.y1 - bounds.y0;
            int width = bounds.x1 - bounds.x0;

            for (int i = 0; i < n; i++) {
                page_heights.push_back(height);
                accum_page_heights.push_back(acc_height);
                page_widths.push_back(width);
                acc_height += height;
            }
        }
        fz_catch(context) {
            std::wcout << L"Error: could not load sample page dimensions\n";
        }
    }

    auto load_page_dimensions_function = [this, n, force_load_now]() {
        std::vector<float> accum_page_heights_;
        std::vector<float> page_heights_;
        std::vector<float> page_widths_;
        std::vector<std::wstring> page_labels_;
        const int N = 20;
        char label_buffer[N];

        // clone the main context for use in the background thread
        fz_context* context_ = fz_clone_context(context);
        fz_try(context_) {
            //			fz_document* doc_ = fz_open_document(context_, utf8_encode(file_name).c_str());
            fz_document* doc_ = open_document_with_file_name(context_, file_name);
            //fz_layout_document(context_, doc, 600, 800, 20);
            load_document_metadata_from_db();

            float acc_height_ = 0.0f;
            for (int i = 0; i < n; i++) {
                fz_page* page = fz_load_page(context_, doc_, i);
                fz_page_label(context_, page, label_buffer, N);
                page_labels_.push_back(utf8_decode(label_buffer));
                fz_rect page_rect = fz_bound_page(context_, page);

                float page_height = page_rect.y1 - page_rect.y0;
                float page_width = page_rect.x1 - page_rect.x0;

                accum_page_heights_.push_back(acc_height_);
                page_heights_.push_back(page_height);
                page_widths_.push_back(page_width);
                acc_height_ += page_height;

                fz_drop_page(context_, page);
            }


            page_dims_mutex.lock();

            page_heights = std::move(page_heights_);
            accum_page_heights = std::move(accum_page_heights_);
            page_widths = std::move(page_widths_);
            page_labels = std::move(page_labels_);

            if (invalid_flag_pointer) {
                *invalid_flag_pointer = true;
            }
            page_dims_mutex.unlock();

            fill_highlight_rects(context_, doc_);

            fz_drop_document(context_, doc_);
        }
        fz_catch(context_) {
            std::wcout << L"Error: could not load page dimensions\n";
        }

        fz_drop_context(context_);

        are_highlights_loaded = true;

    };

    if (force_load_now) {
        load_page_dimensions_function();
    }
    else {
        auto background_page_dimensions_loading_thread = std::thread(load_page_dimensions_function);
        background_page_dimensions_loading_thread.detach();
    }
    load_drawings_async();
}


fz_rect Document::get_page_absolute_rect(int page) {
    std::lock_guard guard(page_dims_mutex);

    fz_rect res = { 0, 0, 1, 1 };

    if (page >= page_widths.size()) {
        return res;
    }

    res.x0 = -page_widths[page] / 2;
    res.x1 = page_widths[page] / 2;

    res.y0 = accum_page_heights[page];
    res.y1 = accum_page_heights[page] + page_heights[page];
    return res;
}

int Document::num_pages() {
    if (cached_num_pages.has_value()) {
        return cached_num_pages.value();
    }

    int pages = -1;
    fz_try(context) {
        pages = fz_count_pages(context, doc);
        cached_num_pages = pages;
    }
    fz_catch(context) {
        LOG(std::cerr << "could not count pages" << std::endl);
    }
    return pages;
}

DocumentManager::DocumentManager(fz_context* mupdf_context, DatabaseManager* db, CachedChecksummer* checksummer) :
    mupdf_context(mupdf_context),
    db_manager(db),
    checksummer(checksummer)
{
    //get_prev_path_hash_pairs(database, const std::string& path, std::vector<std::pair<std::wstring, std::wstring>>& out_pairs);
}


Document* DocumentManager::get_document(const std::wstring& path) {
    if (cached_documents.find(path) != cached_documents.end()) {
        return cached_documents.at(path);
    }
    Document* new_doc = new Document(mupdf_context, path, db_manager, checksummer);
    cached_documents[path] = new_doc;
    return new_doc;
}

const std::unordered_map<std::wstring, Document*>& DocumentManager::get_cached_documents() {
    return cached_documents;
}

void DocumentManager::delete_global_mark(char symbol) {
    for (auto [path, doc] : cached_documents) {
        doc->remove_mark(symbol);
    }
}


fz_stext_page* Document::get_stext_with_page_number(int page_number) {
    return get_stext_with_page_number(context, page_number);
}

fz_stext_page* Document::get_stext_with_page_number(fz_context* ctx, int page_number, fz_document* doc_) {
    // shouldn't cache when the request is coming from the worker thread
    // as the stext pages generated in the other thread is not usable in
    // main thread's context
    bool nocache = false;
    if (doc_ == nullptr) {
        doc_ = doc;
    }
    else {
        nocache = true;
    }

    const int MAX_CACHED_STEXT_PAGES = 10;

    if (!nocache) {
        for (auto [page, cached_stext_page] : cached_stext_pages) {
            if (page == page_number) {
                return cached_stext_page;
            }
        }
    }

    fz_stext_page* stext_page = nullptr;

    bool failed = false;
    fz_try(ctx) {
        stext_page = fz_new_stext_page_from_page_number(ctx, doc_, page_number, nullptr);
    }
    fz_catch(ctx) {
        failed = true;
    }
    if (failed) {
        return nullptr;
    }

    if (stext_page != nullptr) {

        if (!nocache) {
            if (cached_stext_pages.size() == MAX_CACHED_STEXT_PAGES) {
                fz_drop_stext_page(ctx, cached_stext_pages[0].second);
                cached_stext_pages.erase(cached_stext_pages.begin());
            }
            cached_stext_pages.push_back(std::make_pair(page_number, stext_page));
        }
        return stext_page;
    }

    return nullptr;
}

int Document::get_page_offset() {
    return page_offset;
}

void Document::set_page_offset(int new_offset) {
    page_offset = new_offset;
}

fz_rect Document::absolute_to_page_rect(const fz_rect& absolute_rect, int* page) {
    DocumentPos bottom_left = absolute_to_page_pos({ absolute_rect.x0, absolute_rect.y0 });
    DocumentPos top_right = absolute_to_page_pos({ absolute_rect.x1, absolute_rect.y1 });
    if (page != nullptr) {
        *page = bottom_left.page;
    }
    fz_rect res;
    res.x0 = bottom_left.x;
    res.x1 = top_right.x;
    res.y0 = bottom_left.y;
    res.y1 = top_right.y;
    return res;
}

DocumentPos Document::absolute_to_page_pos(AbsoluteDocumentPos absp) {

    std::lock_guard guard(page_dims_mutex);
    if (accum_page_heights.size() == 0) {
        return { 0, 0.0f, 0.0f };
    }

    int i = (std::lower_bound(
        accum_page_heights.begin(),
        accum_page_heights.end(), absp.y) - accum_page_heights.begin()) - 1;
    i = std::max(0, i);

    if (i < accum_page_heights.size()) {
        float acc_page_heights_i = accum_page_heights[i];
        float page_width = page_widths[i];
        float remaining_y = absp.y - acc_page_heights_i;

        return { i, page_width / 2 + absp.x, remaining_y };
    }
    else {
        return { -1, 0.0f, 0.0f };
    }
}

QStandardItemModel* Document::get_toc_model() {
    if (!cached_toc_model) {
        cached_toc_model = get_model_from_toc(get_toc());
    }
    return cached_toc_model;
}


int Document::get_offset_page_number(float y_offset) {
    std::lock_guard guard(page_dims_mutex);

    if (accum_page_heights.size() == 0) {
        return -1;
    }

    auto it = std::lower_bound(accum_page_heights.begin(), accum_page_heights.end(), y_offset);

    // std::lower_bound returns an iterator pointing to the first element of the vector not less than y_offset,
    // but we are looking for the last element of vector that is less than y_offset
    if (it > accum_page_heights.begin()) {
        it--;
    }

    return (it - accum_page_heights.begin());
}

void Document::index_document(bool* invalid_flag) {
    int n = num_pages();

    if (this->document_indexing_thread.has_value()) {
        // if we are already indexing figures, we should wait for the previous thread to finish
        this->document_indexing_thread.value().join();
    }

    is_document_indexing_required = true;
    is_indexing = true;

    this->document_indexing_thread = std::thread([this, n, invalid_flag]() {
        std::vector<IndexedData> local_generic_data;
        std::map<std::wstring, IndexedData> local_reference_data;
        std::map<std::wstring, std::vector<IndexedData>> local_equation_data;

        std::wstring local_super_fast_search_index;
        std::vector<int> local_super_fast_search_pages;
        std::vector<fz_rect> local_super_fast_search_rects;

        std::vector<TocNode*> toc_stack;
        std::vector<TocNode*> top_level_nodes;
        int num_added_toc_entries = 0;

        fz_context* context_ = fz_clone_context(context);
        fz_try(context_) {

            //			fz_document* doc_ = fz_open_document(context_, utf8_encode(file_name).c_str());
            fz_document* doc_ = open_document_with_file_name(context_, file_name);

            if (document_needs_password) {
                fz_authenticate_password(context_, doc_, correct_password.c_str());
            }
            for (int i = 0; i < n; i++) {
                // when we close a document before its indexing is finished, we should stop indexing as soon as posible
                if (!is_document_indexing_required) {
                    break;
                }

                // we don't use get_stext_with_page_number here on purpose because it would lead to many unnecessary allocations
                fz_stext_page* stext_page = fz_new_stext_page_from_page_number(context_, doc_, i, nullptr);

                std::vector<fz_stext_char*> flat_chars;
                get_flat_chars_from_stext_page(stext_page, flat_chars);

                if (SUPER_FAST_SEARCH) {
                    flat_char_prism(flat_chars, i, local_super_fast_search_index, local_super_fast_search_pages, local_super_fast_search_rects);
                }

                index_references(stext_page, i, local_reference_data);
                index_equations(flat_chars, i, local_equation_data);
                index_generic(flat_chars, i, local_generic_data);

                // if the document doesn't have table of contents, try to create one
                if (CREATE_TABLE_OF_CONTENTS_IF_NOT_EXISTS && (top_level_toc_nodes.size() == 0)) {
                    if (num_added_toc_entries < MAX_CREATED_TABLE_OF_CONTENTS_SIZE) {
                        num_added_toc_entries += add_stext_page_to_created_toc(stext_page, i, toc_stack, top_level_nodes);
                    }
                }

                fz_drop_stext_page(context_, stext_page);
            }


            fz_drop_document(context_, doc_);
        }
        fz_catch(context_) {
            std::wcout << L"There was an error in indexing thread.\n";
        }

        fz_drop_context(context_);

        document_indexing_mutex.lock();

        reference_indices = std::move(local_reference_data);
        equation_indices = std::move(local_equation_data);
        generic_indices = std::move(local_generic_data);

        super_fast_search_index = std::move(local_super_fast_search_index);
        super_fast_search_index_pages = std::move(local_super_fast_search_pages);
        super_fast_search_rects = std::move(local_super_fast_search_rects);
        if (SUPER_FAST_SEARCH) {
            super_fast_search_index_ready = true;
        }

        created_top_level_toc_nodes = std::move(top_level_nodes);

        document_indexing_mutex.unlock();
        is_indexing = false;
        if (is_document_indexing_required && invalid_flag) {
            *invalid_flag = true;
        }
        });
    //thread.detach();
}

void Document::stop_indexing() {
    is_document_indexing_required = false;
}



std::vector<IndexedData> Document::find_reference_with_string(std::wstring reference_name, int page_number) {
    if (reference_name.size() > 1 && reference_name[0] == '[') {
        reference_name = reference_name.substr(1, reference_name.size() - 2);
    }

    if (reference_indices.find(reference_name) != reference_indices.end()) {
        return { reference_indices[reference_name] };
    }

    return {};
}

std::vector<IndexedData> Document::find_equation_with_string(std::wstring equation_name, int page_number) {
    if (equation_name.size() > 1 && (equation_name[0] == '(' || equation_name[0] == '[')) {
        equation_name = equation_name.substr(1, equation_name.size() - 2);
    }
    if (equation_indices.find(equation_name) != equation_indices.end()) {
        const std::vector<IndexedData> equations = equation_indices[equation_name];
        int min_distance = 10000;
        std::optional<IndexedData> res = {};

        for (const auto& index : equations) {
            int distance = std::abs(index.page - page_number);

            if (distance < min_distance) {
                min_distance = distance;
                res = index;
            }
        }

        if (res) {
            return { res.value() };
        }
    }

    return {};
}

std::vector<IndexedData> Document::find_generic_with_string(std::wstring equation_name, int page_number) {
    QString qtext = QString::fromStdWString(equation_name);

    std::vector<IndexedData> res;

    QStringList parts = qtext.split(' ');
    if (parts.size() == 2) {
        std::wstring type = parts.at(0).toStdWString();
        std::wstring ref = parts.at(1).toStdWString();

        std::vector<DocumentPos> positions = find_generic_locations(type, ref);
        for (auto pos : positions) {
            IndexedData index;
            index.page = pos.page;
            index.text = equation_name;
            index.y_offset = pos.y;
            res.push_back(index);
        }
    }
    return res;
}


std::optional<std::wstring> Document::get_equation_text_at_position(
    const std::vector<fz_stext_char*>& flat_chars,
    float offset_x,
    float offset_y,
    std::pair<int,
    int>* out_range) {


    std::wregex regex(L"\\([0-9]+(\\.[0-9]+)*\\)");
    std::optional<std::wstring> match = get_regex_match_at_position(regex, flat_chars, offset_x, offset_y, out_range);

    if (match) {
        return match.value().substr(1, match.value().size() - 2);
    }
    else {
        return {};
    }
}

std::optional<std::wstring> Document::get_regex_match_at_position(const std::wregex& regex, const std::vector<fz_stext_char*>& flat_chars, float offset_x, float offset_y, std::pair<int, int>* out_range) {
    fz_rect selected_rect;
    selected_rect.x0 = offset_x - 0.1f;
    selected_rect.x1 = offset_x + 0.1f;
    selected_rect.y0 = offset_y - 0.1f;
    selected_rect.y1 = offset_y + 0.1f;

    std::vector<std::pair<int, int>> match_ranges;
    std::vector<std::wstring> match_texts;

    find_regex_matches_in_stext_page(flat_chars, regex, match_ranges, match_texts);

    for (size_t i = 0; i < match_ranges.size(); i++) {
        auto [start_index, end_index] = match_ranges[i];
        for (int index = start_index; index <= end_index; index++) {
            if (fz_contains_rect(fz_rect_from_quad(flat_chars[index]->quad), selected_rect)) {
                if (out_range) {
                    *out_range = std::make_pair(start_index, end_index);
                }

                return match_texts[i];
            }
        }
    }
    return {};
}

std::vector<DocumentPos> Document::find_generic_locations(const std::wstring& type, const std::wstring& name) {
    //int best_page = -1;
    //int best_y_offset = 0.0f;
    //float best_score = -1000;

    std::vector<std::pair<int, DocumentPos>> pos_scores;

    for (size_t i = 0; i < generic_indices.size(); i++) {
        std::vector<std::wstring> parts = split_whitespace(generic_indices[i].text);

        if (parts.size() == 2) {
            std::wstring current_type = parts[0];
            std::wstring current_name = parts[1];

            if (current_name == name) {
                int score = type_name_similarity_score(current_type, type);
                DocumentPos pos{ generic_indices[i].page, 0, generic_indices[i].y_offset };
                pos_scores.push_back(std::make_pair(score, pos));
                //if (score > best_score) {
                //	best_page = generic_indices[i].page;
                //	best_y_offset = generic_indices[i].y_offset;
                //	best_score = score;
                //}
            }

        }
    }

    auto  by_score = [](std::pair<int, DocumentPos> const& a, std::pair<int, DocumentPos> const& b) {
        return a.first < b.first;
    };

    std::sort(pos_scores.begin(), pos_scores.end(), by_score);

    std::vector<DocumentPos> res;
    for (int i = pos_scores.size() - 1; i >= 0; i--) {
        res.push_back(pos_scores[i].second);
    }
    return res;

    //if (best_page != -1) {
    //	*page = best_page;
    //	*y_offset = best_y_offset;
    //	return true;
    //}

    //return false;
}

bool Document::can_use_highlights() {
    return are_highlights_loaded;
}

std::optional<std::pair<std::wstring, std::wstring>> Document::get_generic_link_name_at_position(
    const std::vector<fz_stext_char*>& flat_chars,
    float offset_x,
    float offset_y,
    std::pair<int,
    int>* out_range) {

    std::wregex regex(L"[a-zA-Z]{3,}(\.){0,1}[ \t]+[0-9]+(\.[0-9]+)*");
    std::optional<std::wstring> match_string = get_regex_match_at_position(regex, flat_chars, offset_x, offset_y, out_range);
    if (match_string) {
        std::vector<std::wstring> parts = split_whitespace(match_string.value());
        if (parts.size() != 2) {
            return {};
        }
        else {
            std::wstring type = parts[0];
            std::wstring name = parts[1];
            return std::make_pair(type, name);
        }
    }

    else {
        return {};
    }

    return {};
}

std::optional<std::wstring> Document::get_reference_text_at_position(const std::vector<fz_stext_char*>& flat_chars, float offset_x, float offset_y, std::pair<int, int>* out_range) {
    fz_rect selected_rect;

    selected_rect.x0 = offset_x - 0.1f;
    selected_rect.x1 = offset_x + 0.1f;

    selected_rect.y0 = offset_y - 0.1f;
    selected_rect.y1 = offset_y + 0.1f;

    char start_char = '[';
    char end_char = ']';
    char delim = ',';

    bool reached = false; // true when we reach selected character
    bool started = false; // true when we reach [
    bool done = false; // true when whe reach a delimeter after the click location
    //	for example suppose we click here in  [124,253,432]
    //                                              ^      
    // then `done` will be true when we reach the second comma

    std::wstring selected_text = L"";

    int index = -1;
    int begin_index = -1;
    int end_index = -1;

    for (auto ch : flat_chars) {
        index++;

        if (fz_contains_rect(fz_rect_from_quad(ch->quad), selected_rect)) {
            if (started) {
                reached = true;
            }
        }
        if (ch->c == start_char) {
            started = true;
            continue;
        }

        if (started && reached && (ch->c == delim)) {
            done = true;
            continue;
        }
        if (started && reached && (ch->c == end_char)) {
            if (out_range) {
                *out_range = std::make_pair(begin_index, end_index);
            }
            return selected_text;
        }
        if (started && (!reached) && (ch->c == end_char)) {
            started = false;
            selected_text.clear();
            begin_index = index;
        }

        if (started && (!done)) {
            if (ch->c != ' ') {
                if (selected_text.size() == 0) {
                    begin_index = index;
                }
                selected_text.push_back(ch->c);
                end_index = index;
            }
        }

        if ((started) && (!reached) && (ch->c == delim)) {
            selected_text.clear();
            begin_index = index;
        }

    }

    return {};
}

void get_matches(std::wstring haystack, const std::wregex& reg, std::vector<std::pair<int, int>>& indices) {
    std::wsmatch match;

    int offset = 0;
    while (std::regex_search(haystack, match, reg)) {
        int start_index = offset + match.position();
        int end_index = start_index + match.length();
        indices.push_back(std::make_pair(start_index, end_index));

        int old_length = haystack.size();
        haystack = match.suffix();
        int new_length = haystack.size();

        offset += (old_length - new_length);
    }
}

std::optional<std::wstring> Document::get_text_at_position(const std::vector<fz_stext_char*>& flat_chars, float offset_x, float offset_y) {

    fz_rect selected_rect;

    selected_rect.x0 = offset_x - 0.1f;
    selected_rect.x1 = offset_x + 0.1f;

    selected_rect.y0 = offset_y - 0.1f;
    selected_rect.y1 = offset_y + 0.1f;

    std::wstring selected_string;
    bool reached = false;

    for (auto ch : flat_chars) {
        if (iswspace(ch->c) || (ch->next == nullptr)) {
            if (reached) {
                return selected_string;
            }
            selected_string.clear();
        }
        else {
            selected_string.push_back(ch->c);
        }
        if (fz_contains_rect(fz_rect_from_quad(ch->quad), selected_rect)) {
            reached = true;
        }
    }


    return {};
}

fz_stext_block* Document::get_text_block_at_positition(fz_stext_page* page, float offset_x, float offset_y) {
    fz_stext_block* current_block = page->first_block;

    fz_point pos = { offset_x, offset_y };

    while (current_block) {
        if (current_block->type == FZ_STEXT_BLOCK_TEXT) {
            fz_stext_line* current_line = current_block->u.t.first_line;
            while (current_line) {
                fz_rect bbox = current_line->bbox;
                if (fz_is_point_inside_rect(pos, bbox)) {
                    return current_block;
                }
                current_line = current_line->next;
            }
        }
        current_block = current_block->next;
    }
    return nullptr;
}

std::optional<std::wstring> Document::get_paper_name_at_position(const std::vector<fz_stext_char*>& flat_chars, float offset_x, float offset_y) {
    fz_rect selected_rect;

    selected_rect.x0 = offset_x - 0.1f;
    selected_rect.x1 = offset_x + 0.1f;

    selected_rect.y0 = offset_y - 0.1f;
    selected_rect.y1 = offset_y + 0.1f;

    std::wstring selected_string = L"";
    bool reached = false;

    for (auto ch : flat_chars) {
        if (ch->c == '.') {
            if (!reached) {
                selected_string = L"";
            }
            else {
                return selected_string;
            }
        }
        if (fz_contains_rect(fz_rect_from_quad(ch->quad), selected_rect)) {
            reached = true;
        }
        if ((ch->c == '-') && (ch->next == nullptr)) continue;

        if (ch->c != '.') {
            selected_string.push_back(ch->c);
        }
        if (ch->next == nullptr) {
            selected_string.push_back(' ');
        }

    }

    return {};
}

fz_pixmap* Document::get_small_pixmap(int page) {
    for (auto [cached_page, pixmap] : cached_small_pixmaps) {
        if (cached_page == page) {
            return pixmap;
        }
    }

    //fz_matrix ctm = fz_scale(0.5f, 0.5f);
    fz_matrix ctm = fz_scale(SMALL_PIXMAP_SCALE, SMALL_PIXMAP_SCALE);
    fz_pixmap* res = fz_new_pixmap_from_page_number(context, doc, page, ctm, fz_device_rgb(context), 0);
    cached_small_pixmaps.push_back(std::make_pair(page, res));
    unsigned int SMALL_PIXMAP_CACHE_SIZE = 5;

    if (cached_small_pixmaps.size() > SMALL_PIXMAP_CACHE_SIZE) {
        fz_drop_pixmap(context, cached_small_pixmaps[0].second);
        cached_small_pixmaps.erase(cached_small_pixmaps.begin());
    }

    return res;
}

void Document::get_text_selection(AbsoluteDocumentPos selection_begin,
    AbsoluteDocumentPos selection_end,
    bool is_word_selection, // when in word select mode, we select entire words even if the range only partially includes the word
    std::deque<fz_rect>& selected_characters,
    std::wstring& selected_text) {
    get_text_selection(context, selection_begin, selection_end, is_word_selection, selected_characters, selected_text);
}
void Document::get_text_selection(fz_context* ctx, AbsoluteDocumentPos selection_begin,
    AbsoluteDocumentPos selection_end,
    bool is_word_selection,
    std::deque<fz_rect>& selected_characters,
    std::wstring& selected_text,
    fz_document* doc_) {

    selected_characters.clear();
    selected_text.clear();

    if (doc_ == nullptr) {
        doc_ = doc;
    }


    DocumentPos page_pos1 = absolute_to_page_pos(selection_begin);
    DocumentPos page_pos2 = absolute_to_page_pos(selection_end);

    int page_begin, page_end;
    fz_point page_point1;
    fz_point page_point2;

    page_begin = page_pos1.page;
    page_end = page_pos2.page;
    page_point1.x = page_pos1.x;
    page_point2.x = page_pos2.x;
    page_point1.y = page_pos1.y;
    page_point2.y = page_pos2.y;

    if ((page_begin == -1) || (page_end == -1)) {
        return;
    }

    if (page_end < page_begin) {
        std::swap(page_begin, page_end);
        std::swap(page_point1, page_point2);
    }

    selected_text.clear();

    bool word_selecting = false;
    bool selecting = false;
    if (is_word_selection) {
        selecting = true;
    }

    for (int i = page_begin; i <= page_end; i++) {

        // for now, let's assume there is only one page
        fz_stext_page* stext_page = get_stext_with_page_number(ctx, i, doc_);
        if (!stext_page) continue;

        std::vector<fz_stext_char*> flat_chars;
        get_flat_chars_from_stext_page(stext_page, flat_chars);


        int location_index1, location_index2;
        fz_stext_char* char_begin = nullptr;
        fz_stext_char* char_end = nullptr;
        if (i == page_begin) {
            char_begin = find_closest_char_to_document_point(flat_chars, page_point1, &location_index1);
        }
        if (i == page_end) {
            char_end = find_closest_char_to_document_point(flat_chars, page_point2, &location_index2);
        }
        if (flat_chars.size() > 0) {
            if (char_begin == nullptr) {
                char_begin = flat_chars[0];
            }
            if (char_end == nullptr) {
                char_end = flat_chars[flat_chars.size() - 1];
            }
        }

        if ((char_begin == nullptr) || (char_end == nullptr)) {
            return;
        }

        while ((char_begin->c == ' ') && (char_begin->next != nullptr)) {
            char_begin = char_begin->next;
        }

        if (char_begin && char_end) {
            // swap the locations if end happends before begin
            if (page_begin == page_end && location_index1 > location_index2) {
                std::swap(char_begin, char_end);
            }
        }


        for (auto current_char : flat_chars) {
            if (!is_word_selection) {
                if (current_char == char_begin) {
                    selecting = true;
                }
                //if (current_char == char_end) {
                //	selecting = false;
                //}
            }
            else {
                if (word_selecting == false && is_separator(char_begin, current_char)) {
                    selected_text.clear();
                    selected_characters.clear();
                    continue;
                }
                if (current_char == char_begin) {
                    word_selecting = true;
                }
                if (current_char == char_end) {
                    selecting = false;
                }
                if (word_selecting == true && is_separator(char_end, current_char) && selecting == false) {
                    word_selecting = false;
                    return;
                }
            }

            if (selecting || word_selecting) {
                if (!(current_char->c == ' ' && selected_text.size() == 0)) {
                    selected_text.push_back(current_char->c);
                    fz_rect charrect = document_to_absolute_rect(i, fz_rect_from_quad(current_char->quad), true);
                    selected_characters.push_back(charrect);
                }
                if ((current_char->next == nullptr)) {
                    if (current_char->c != '-')
                    {
                        selected_text.push_back(' ');
                    }
                    else {
                        selected_text.pop_back();
                    }
                }

            }
            if (!is_word_selection) {
                if (current_char == char_end) {
                    selecting = false;
                }
            }
        }
    }
}

void Document::get_pdf_annotations(std::vector<BookMark>& pdf_bookmarks, std::vector<Highlight>& pdf_highlights, std::vector<FreehandDrawing>& pdf_drawings) {
    for (int p = 0; p < num_pages(); p++) {
        fz_page* page = fz_load_page(context, doc, p);
        pdf_page* pdf_page = pdf_page_from_fz_page(context, page);

        pdf_annot* annot = pdf_first_annot(context, pdf_page);
        while (annot) {
            enum pdf_annot_type annot_type = pdf_annot_type(context, annot);

            if (annot_type == pdf_annot_type::PDF_ANNOT_INK) {
                int num_strokes = pdf_annot_ink_list_count(context, annot);
                for (int stroke_index = 0; stroke_index < num_strokes; stroke_index++) {
                    FreehandDrawing drawing;

                    int vertex_count = pdf_annot_ink_list_stroke_count(context, annot, stroke_index);
                    int n_channels;
                    float color[4];
                    pdf_annot_color(context, annot, &n_channels, color);
                    char type = get_highlight_color_type(color);
                    float thickness = pdf_annot_border(context, annot);
                    drawing.type = type;

                    for (int vertex_index = 0; vertex_index < vertex_count; vertex_index++) {
                        fz_point vertex = pdf_annot_ink_list_stroke_vertex(context, annot, stroke_index, vertex_index);
                        FreehandDrawingPoint point;
                        DocumentPos docpos;
                        docpos.page = p;
                        docpos.x = vertex.x;
                        docpos.y = vertex.y;
                        point.pos = document_to_absolute_pos(docpos, true);
                        point.thickness = thickness;
                        drawing.points.push_back(point);
                    }
                    pdf_drawings.push_back(drawing);
                }
            }
            if (annot_type == pdf_annot_type::PDF_ANNOT_TEXT) {
                fz_rect rect = pdf_bound_annot(context, annot);
                fz_rect absrect = document_to_absolute_rect(p, rect, true);
                // get text of annotation
                const char* txt = pdf_annot_contents(context, annot);
                //new_bookmark.description = utf8_decode(txt);
                BookMark new_bookmark;
                new_bookmark.description = utf8_decode(txt);
                new_bookmark.y_offset_ = (absrect.y0 + absrect.y1) / 2;
                new_bookmark.begin_x = (absrect.x0 + absrect.x1) / 2;
                new_bookmark.begin_y = new_bookmark.y_offset_;
                pdf_bookmarks.push_back(new_bookmark);

            }

            if (annot_type == pdf_annot_type::PDF_ANNOT_FREE_TEXT) {
                fz_rect rect = pdf_bound_annot(context, annot);
                fz_rect absrect = document_to_absolute_rect(p, rect, true);
                // get text of annotation
                const char* txt = pdf_annot_contents(context, annot);
                //new_bookmark.description = utf8_decode(txt);
                BookMark new_bookmark;
                new_bookmark.description = utf8_decode(txt);
                new_bookmark.y_offset_ = absrect.y0;
                new_bookmark.begin_x = absrect.x0;
                new_bookmark.begin_y = absrect.y0;
                new_bookmark.end_x = absrect.x1;
                new_bookmark.end_y = absrect.y1;
                char font_name[100] = { 0 };
                const char* font_name_addr[] = { font_name };
                float font_size;
                float color[4];
                int n_channels;

                pdf_annot_default_appearance(context, annot, &font_name_addr[0], &font_size, &n_channels, color);
                new_bookmark.font_face = utf8_decode(font_name);
                new_bookmark.font_size = font_size;
                new_bookmark.color[0] = color[0];
                new_bookmark.color[1] = color[1];
                new_bookmark.color[2] = color[2];

                pdf_bookmarks.push_back(new_bookmark);

            }

            if ((annot_type == pdf_annot_type::PDF_ANNOT_HIGHLIGHT) || (annot_type == pdf_annot_type::PDF_ANNOT_UNDERLINE) || (annot_type == pdf_annot_type::PDF_ANNOT_STRIKE_OUT)) {
                int num_quads = pdf_annot_quad_point_count(context, annot);

                if (num_quads > 0) {
                    fz_quad first_vertex = pdf_annot_quad_point(context, annot, 0);
                    fz_quad last_vertex = pdf_annot_quad_point(context, annot, num_quads - 1);
                    const char* txt = pdf_annot_contents(context, annot);
                    float color[4];
                    int n_channels = -1;
                    pdf_annot_color(context, annot, &n_channels, color);

                    DocumentPos begin_pos, end_pos;
                    begin_pos.page = p;
                    begin_pos.x = first_vertex.ul.x;
                    begin_pos.y = (first_vertex.ul.y + first_vertex.ll.y) / 2;
                    end_pos.page = p;
                    end_pos.x = last_vertex.ur.x;
                    end_pos.y = (last_vertex.ur.y + last_vertex.lr.y) / 2;

                    AbsoluteDocumentPos begin_abspos = document_to_absolute_pos(begin_pos, true);
                    AbsoluteDocumentPos end_abspos = document_to_absolute_pos(end_pos, true);

                    Highlight new_highlight;
                    new_highlight.selection_begin = begin_abspos;
                    new_highlight.selection_end = end_abspos;
                    new_highlight.text_annot = utf8_decode(txt);
                    new_highlight.type = get_highlight_color_type(color);

                    if (annot_type == pdf_annot_type::PDF_ANNOT_UNDERLINE) {
                        new_highlight.type = new_highlight.type + 'A' - 'a';
                    }
                    if (annot_type == pdf_annot_type::PDF_ANNOT_STRIKE_OUT) {
                        new_highlight.type = '_';
                    }

                    pdf_highlights.push_back(new_highlight);

                }
            }
            annot = pdf_next_annot(context, annot);
        }

        fz_drop_page(context, page);
    }
}

void Document::import_annotations() {
    std::vector<BookMark> pdf_bookmarks;
    std::vector<Highlight> pdf_highlights;
    std::vector<FreehandDrawing> pdf_drawings;

    get_pdf_annotations(pdf_bookmarks, pdf_highlights, pdf_drawings);


    //           int count[1] = { static_cast<int>(points.size()) };
       //		pdf_set_annot_border(context, drawing_annot, drawing.points[0].thickness);
       //		pdf_set_annot_ink_list(context, drawing_annot, 1, count, &points[0]);
       //		pdf_update_annot(context, drawing_annot);
       //	}
    for (auto bookmark : pdf_bookmarks) {
        if (is_bookmark_new(bookmark)) {

            if (bookmark.end_y > 0) { // it is a freetext bookmark
                fz_rect absrect;
                absrect.x0 = bookmark.begin_x;
                absrect.x1 = bookmark.end_x;
                absrect.y0 = bookmark.begin_y;
                absrect.y1 = bookmark.end_y;
                add_freetext_bookmark_with_color(bookmark.description, absrect, bookmark.color, bookmark.font_size);
            }
            else {
                add_marked_bookmark(bookmark.description, { bookmark.begin_x, bookmark.begin_y });
            }
        }
    }
    for (auto highlight : pdf_highlights) {
        if (is_highlight_new(highlight)) {
            add_highlight(highlight.text_annot, highlight.selection_begin, highlight.selection_end, highlight.type);
        }
    }
    for (auto drawing : pdf_drawings) {
        if (is_drawing_new(drawing)) {
            add_freehand_drawing(drawing);
        }
    }

    //pdf_document* pdf_doc = pdf_specifics(context, doc);

    //std::vector<BookMark> new_bookmarks;
    //std::vector<Highlight> new_highlights;

    //for (int p = 0; p < num_pages(); p++) {
    //	fz_page* page = fz_load_page(context, doc, p);
    //	pdf_page* pdf_page = pdf_page_from_fz_page(context, page);

    //	pdf_annot* annot = pdf_first_annot(context, pdf_page);
    //	while (annot) {
    //		enum pdf_annot_type annot_type = pdf_annot_type(context, annot);

    //		if (annot_type == pdf_annot_type::PDF_ANNOT_TEXT) {
    //			fz_rect rect = pdf_bound_annot(context, annot);
    //			fz_rect absrect = document_to_absolute_rect(p, rect, true);
    //			// get text of annotation
    //			const char* txt = pdf_annot_contents(context, annot);
    //			//new_bookmark.description = utf8_decode(txt);
    //			BookMark new_bookmark;
    //			new_bookmark.description = utf8_decode(txt);
    //			new_bookmark.y_offset = (absrect.y0 + absrect.y1) / 2;
    //			new_bookmark.begin_x = (absrect.x0 + absrect.x1) / 2;
    //			new_bookmark.begin_y = new_bookmark.y_offset;

    //			if (is_bookmark_new(new_bookmark)) {
    //				add_marked_bookmark(new_bookmark.description, { new_bookmark.begin_x, new_bookmark.begin_y });
    //			}

    //		}

    //		if (annot_type == pdf_annot_type::PDF_ANNOT_FREE_TEXT) {
    //			fz_rect rect = pdf_bound_annot(context, annot);
    //			fz_rect absrect = document_to_absolute_rect(p, rect, true);
    //			// get text of annotation
    //			const char* txt = pdf_annot_contents(context, annot);
    //			//new_bookmark.description = utf8_decode(txt);
    //			BookMark new_bookmark;
    //			new_bookmark.description = utf8_decode(txt);
    //			new_bookmark.y_offset = absrect.y0;
    //			new_bookmark.begin_x = absrect.x0;
    //			new_bookmark.begin_y = absrect.y0;
    //			new_bookmark.end_x = absrect.x1;
    //			new_bookmark.end_y = absrect.y1;
    //			char font_name[100] = {0};
    //			const char* font_name_addr[] = {font_name};
    //			float font_size;
    //			float color[4];
    //			int n_channels;

    //			pdf_annot_default_appearance(context, annot, &font_name_addr[0], &font_size, &n_channels, color);
    //			new_bookmark.font_face = utf8_decode(font_name);
    //			new_bookmark.font_size = font_size;
    //			new_bookmark.color[0] = color[0];
    //			new_bookmark.color[1] = color[1];
    //			new_bookmark.color[2] = color[2];

    //			if (is_bookmark_new(new_bookmark)) {
    //				add_freetext_bookmark_with_color(new_bookmark.description, absrect, new_bookmark.color);
    //			}

    //		}

    //		if ((annot_type == pdf_annot_type::PDF_ANNOT_HIGHLIGHT) || (annot_type == pdf_annot_type::PDF_ANNOT_UNDERLINE)) {
    //			int num_quads = pdf_annot_quad_point_count(context, annot);

    //			if (num_quads > 0) {
    //				fz_quad first_vertex = pdf_annot_quad_point(context, annot, 0);
    //				fz_quad last_vertex = pdf_annot_quad_point(context, annot, num_quads-1);
    //				const char* txt = pdf_annot_contents(context, annot);
    //				float color[4];
    //				int n_channels = -1;
    //				pdf_annot_color(context, annot, &n_channels, color);

    //				DocumentPos begin_pos, end_pos;
    //				begin_pos.page = p;
    //				begin_pos.x = first_vertex.ul.x;
    //				begin_pos.y = ( first_vertex.ul.y + first_vertex.ll.y ) / 2;
    //				end_pos.page = p;
    //				end_pos.x = last_vertex.ur.x;
    //				end_pos.y = ( last_vertex.ur.y + last_vertex.lr.y ) / 2;

    //				AbsoluteDocumentPos begin_abspos = document_to_absolute_pos(begin_pos, true);
    //				AbsoluteDocumentPos end_abspos = document_to_absolute_pos(end_pos, true);

    //				Highlight new_highlight;
    //				new_highlight.selection_begin = begin_abspos;
    //				new_highlight.selection_end = end_abspos;
    //				new_highlight.text_annot = utf8_decode(txt);
    //				new_highlight.type = get_highlight_color_type(color);

    //				if (annot_type == pdf_annot_type::PDF_ANNOT_UNDERLINE) {
    //					new_highlight.type = new_highlight.type + 'A' - 'a';
    //				}

    //				if (is_highlight_new(new_highlight)) {
    //					add_highlight(new_highlight.text_annot, begin_abspos, end_abspos, new_highlight.type);
    //				}

    //			}
    //		}
    //		annot = pdf_next_annot(context, annot);
    //	}


    //	fz_drop_page(context, page);
    //}
}

void Document::embed_annotations(std::wstring new_file_path) {

    std::unordered_map<int, fz_page*> cached_pages;
    std::vector<std::pair<pdf_page*, pdf_annot*>> created_annotations;

    auto load_cached_page = [&](int page_number) {
        if (cached_pages.find(page_number) != cached_pages.end()) {
            return cached_pages[page_number];
        }
        else {
            fz_page* page = fz_load_page(context, doc, page_number);
            cached_pages[page_number] = page;
            return page;
        }
    };

    std::string new_file_path_utf8 = utf8_encode(new_file_path);
    fz_output* output_file = nullptr;

    fz_try(context) {
        output_file = fz_new_output_with_path(context, new_file_path_utf8.c_str(), 0);
    }
    fz_catch(context) {
        output_file = nullptr;
    }

    if (output_file == nullptr) {
        show_error_message(L"Could not open the output file. Make sure the file is not opened in sioyek or another application.");
        return;
    }

    pdf_document* pdf_doc = pdf_specifics(context, doc);
    std::vector<Highlight> pdf_highlights;
    std::vector<BookMark> pdf_bookmarks;
    std::vector<FreehandDrawing> pdf_drawings;

    get_pdf_annotations(pdf_bookmarks, pdf_highlights, pdf_drawings);

    const std::vector<Highlight>& doc_highlights = get_new_sioyek_highlights(pdf_highlights);
    const std::vector<BookMark>& doc_bookmarks = get_new_sioyek_bookmarks(pdf_bookmarks);

    for (auto highlight : doc_highlights) {
        int page_number = get_offset_page_number(highlight.selection_begin.y);

        std::deque<fz_rect> selected_characters;
        std::vector<fz_rect> merged_characters;
        std::vector<fz_rect> selected_characters_page_rects;
        std::wstring selected_text;

        get_text_selection(highlight.selection_begin, highlight.selection_end, true, selected_characters, selected_text);
        merge_selected_character_rects(selected_characters, merged_characters, highlight.type != '_');

        for (auto absrect : merged_characters) {
            selected_characters_page_rects.push_back(absolute_to_page_rect(absrect, nullptr));
        }
        //absolute_to_page_pos
        std::vector<fz_quad> selected_character_quads = quads_from_rects(selected_characters_page_rects);

        fz_page* page = load_cached_page(page_number);
        pdf_page* pdf_page = pdf_page_from_fz_page(context, page);
        pdf_annot* highlight_annot;
        if (highlight.type == '_') {
            highlight_annot = pdf_create_annot(context, pdf_page, PDF_ANNOT_STRIKE_OUT);
        }
        else if (std::isupper(highlight.type)) {
            highlight_annot = pdf_create_annot(context, pdf_page, PDF_ANNOT_UNDERLINE);
        }
        else {
            highlight_annot = pdf_create_annot(context, pdf_page, PDF_ANNOT_HIGHLIGHT);
        }
        float* color = get_highlight_type_color(highlight.type);

        pdf_set_annot_color(context, highlight_annot, 3, color);
        if (highlight.text_annot.size() > 0) {
            std::string encoded_highlight_text = utf8_encode(highlight.text_annot);
            pdf_set_annot_contents(context, highlight_annot, encoded_highlight_text.c_str());
        }

        pdf_set_annot_quad_points(context, highlight_annot, selected_character_quads.size(), &selected_character_quads[0]);
        pdf_update_annot(context, highlight_annot);

        created_annotations.push_back(std::make_pair(pdf_page, highlight_annot));

    }

    for (auto bookmark : doc_bookmarks) {
        auto [page_number, doc_x, doc_y] = absolute_to_page_pos({ 0, bookmark.y_offset_ });

        fz_page* page = load_cached_page(page_number);
        pdf_page* pdf_page = pdf_page_from_fz_page(context, page);
        pdf_annot* bookmark_annot;
        if (bookmark.is_freetext()) {
            bookmark_annot = pdf_create_annot(context, pdf_page, PDF_ANNOT_FREE_TEXT);
        }
        else {
            bookmark_annot = pdf_create_annot(context, pdf_page, PDF_ANNOT_TEXT);
        }

        std::string encoded_bookmark_text = utf8_encode(bookmark.description);

        fz_rect annot_rect;
        if (bookmark.is_freetext()) {
            DocumentPos begin_page_pos = absolute_to_page_pos({ bookmark.begin_x, bookmark.begin_y });
            DocumentPos end_page_pos = absolute_to_page_pos({ bookmark.end_x, bookmark.end_y });

            annot_rect.x0 = begin_page_pos.x;
            annot_rect.x1 = end_page_pos.x;
            annot_rect.y0 = begin_page_pos.y;
            annot_rect.y1 = end_page_pos.y;

            std::string encoded_bookmark_text = utf8_encode(bookmark.description);
            const char* font_face = bookmark.font_face.size() == 0 ? "Times New Roman" : encoded_bookmark_text.c_str();
            pdf_set_annot_default_appearance(context, bookmark_annot, font_face, bookmark.font_size, 3, bookmark.color);
        }
        else if (bookmark.is_marked()) {
            DocumentPos begin_page_pos = absolute_to_page_pos({ bookmark.begin_x, bookmark.begin_y });

            annot_rect.x0 = begin_page_pos.x - 6;
            annot_rect.x1 = begin_page_pos.x + 6;
            annot_rect.y0 = begin_page_pos.y - 6;
            annot_rect.y1 = begin_page_pos.y + 6;
        }
        else {
            annot_rect.x0 = 10;
            annot_rect.x1 = 20;
            annot_rect.y0 = doc_y;
            annot_rect.y1 = doc_y + 10;
        }

        pdf_set_annot_rect(context, bookmark_annot, annot_rect);
        pdf_set_annot_contents(context, bookmark_annot, encoded_bookmark_text.c_str());
        pdf_update_annot(context, bookmark_annot);

        created_annotations.push_back(std::make_pair(pdf_page, bookmark_annot));
    }

    for (auto [page_number, drawings] : page_freehand_drawings) {
        for (auto drawing : drawings) {
            fz_page* page = load_cached_page(page_number);
            pdf_page* pdf_page = pdf_page_from_fz_page(context, page);
            pdf_annot* drawing_annot = pdf_create_annot(context, pdf_page, PDF_ANNOT_INK);
            std::vector<fz_point> points;

            for (auto point : drawing.points) {
                DocumentPos docpos = absolute_to_page_pos(point.pos);
                if (docpos.page == page_number) {
                    points.push_back(fz_point{ docpos.x, docpos.y });
                }
            }

            int count[1] = { static_cast<int>(points.size()) };
            pdf_set_annot_border(context, drawing_annot, drawing.points[0].thickness);
            pdf_set_annot_ink_list(context, drawing_annot, 1, count, &points[0]);
            if (drawing.type >= 'a' && drawing.type <= 'z') {
                pdf_set_annot_color(context, drawing_annot, 3, &HIGHLIGHT_COLORS[3 * (drawing.type - 'a')]);
            }
            pdf_update_annot(context, drawing_annot);
        }
    }


    pdf_write_options pwo{};
    pdf_write_document(context, pdf_doc, output_file, &pwo);
    fz_close_output(context, output_file);
    fz_drop_output(context, output_file);

    for (auto [page, annot] : created_annotations) {
        pdf_delete_annot(context, page, annot);
        pdf_drop_annot(context, annot);
    }
    for (auto [num, page] : cached_pages) {
        fz_drop_page(context, page);
    }
}


//void Document::add_highlight_annotation(const Highlight& highlight, const std::vector<fz_rect>& selected_rects) {
//
//
//	int page_number = get_offset_page_number(highlight.selection_begin.y);
//
//	//todo: refactor this and other instances of this code into a function
//	std::vector<fz_rect> merged_characters;
//	std::vector<fz_rect> page_characters;
//
//	merge_selected_character_rects(selected_rects, merged_characters);
//
//	for (auto absrect : merged_characters) {
//		page_characters.push_back(absolute_to_page_rect(absrect, nullptr));
//	}
//	std::vector<fz_quad> selected_character_quads = quads_from_rects(page_characters);
//
//	fz_page* page = fz_load_page(context, doc, page_number);
//	pdf_page* pdf_page = pdf_page_from_fz_page(context, page);
//
//	if (pdf_page) {
//		pdf_annot* highlight_annot = pdf_create_annot(context, pdf_page, PDF_ANNOT_HIGHLIGHT);
//		float color[] = { 1.0f, 0.0f, 0.0f };
//		color[0] = HIGHLIGHT_COLORS[(highlight.type - 'a') * 3 + 0];
//		color[1] = HIGHLIGHT_COLORS[(highlight.type - 'a') * 3 + 1];
//		color[2] = HIGHLIGHT_COLORS[(highlight.type - 'a') * 3 + 2];
//
//		pdf_set_annot_color(context, highlight_annot, 3, color);
//		pdf_set_annot_quad_points(context, highlight_annot, selected_character_quads.size(), &selected_character_quads[0]);
//		pdf_update_annot(context, highlight_annot);
//		pdf_update_page(context, pdf_page);
//	}
//
//	fz_drop_page(context, page);
//}
//
//void Document::delete_highlight_annotation(const Highlight& highlight) {
//
//}
//
//void Document::add_bookmark_annotation(const BookMark& bookmark) {
//
//}
//
//void Document::delete_bookmark_annotation(const BookMark& bookmark) {
//
//}

std::optional<std::wstring> Document::get_text_at_position(int page, float offset_x, float offset_y) {
    fz_stext_page* stext_page = get_stext_with_page_number(page);
    std::vector<fz_stext_char*> flat_chars;
    get_flat_chars_from_stext_page(stext_page, flat_chars);
    return get_text_at_position(flat_chars, offset_x, offset_y);
}

std::optional<std::wstring> Document::get_paper_name_at_position(int page, float offset_x, float offset_y) {
    fz_stext_page* stext_page = get_stext_with_page_number(page);
    std::vector<fz_stext_char*> flat_chars;
    get_flat_chars_from_stext_page(stext_page, flat_chars);
    return get_paper_name_at_position(flat_chars, offset_x, offset_y);
}

std::optional<std::wstring> Document::get_reference_text_at_position(int page, float offset_x, float offset_y, std::pair<int, int>* out_range) {
    fz_stext_page* stext_page = get_stext_with_page_number(page);
    std::vector<fz_stext_char*> flat_chars;
    get_flat_chars_from_stext_page(stext_page, flat_chars);
    return get_reference_text_at_position(flat_chars, offset_x, offset_y, out_range);
}

std::optional<std::wstring> Document::get_equation_text_at_position(int page, float offset_x, float offset_y, std::pair<int, int>* out_range) {
    fz_stext_page* stext_page = get_stext_with_page_number(page);
    std::vector<fz_stext_char*> flat_chars;
    get_flat_chars_from_stext_page(stext_page, flat_chars);
    return get_equation_text_at_position(flat_chars, offset_x, offset_y, out_range);
}

std::optional<std::pair<std::wstring, std::wstring>>  Document::get_generic_link_name_at_position(int page, float offset_x, float offset_y, std::pair<int, int>* out_range) {
    fz_stext_page* stext_page = get_stext_with_page_number(page);
    std::vector<fz_stext_char*> flat_chars;
    get_flat_chars_from_stext_page(stext_page, flat_chars);
    return get_generic_link_name_at_position(flat_chars, offset_x, offset_y, out_range);
}

std::optional<std::wstring> Document::get_regex_match_at_position(const std::wregex& regex, int page, float offset_x, float offset_y, std::pair<int, int>* out_range) {
    fz_stext_page* stext_page = get_stext_with_page_number(page);
    std::vector<fz_stext_char*> flat_chars;
    get_flat_chars_from_stext_page(stext_page, flat_chars);
    return get_regex_match_at_position(regex, flat_chars, offset_x, offset_y, out_range);
}

std::vector<std::vector<fz_rect>> Document::get_page_flat_word_chars(int page) {
    // warning: this function should only be called after get_page_flat_words has already cached the chars
    if (cached_flat_word_chars.find(page) != cached_flat_word_chars.end()) {
        return cached_flat_word_chars[page];
    }
    return {};
}

std::vector<fz_rect> Document::get_page_flat_words(int page) {
    if (cached_flat_words.find(page) != cached_flat_words.end()) {
        return cached_flat_words[page];
    }

    fz_stext_page* stext_page = get_stext_with_page_number(page);
    std::vector<fz_stext_char*> flat_chars;
    std::vector<fz_rect> word_rects;
    std::vector<std::vector<fz_rect>> word_char_rects;
    std::vector<std::pair<fz_rect, int>> word_rects_with_page;
    get_flat_chars_from_stext_page(stext_page, flat_chars);
    get_flat_words_from_flat_chars(flat_chars, word_rects, &word_char_rects);
    cached_flat_words[page] = word_rects;
    cached_flat_word_chars[page] = word_char_rects;
    return word_rects;
}

void Document::rotate() {
    std::swap(page_heights, page_widths);
    float acc_height = 0;
    for (size_t i = 0; i < page_heights.size(); i++) {
        accum_page_heights[i] = acc_height;
        acc_height += page_heights[i];
    }
}

std::optional<Highlight> Document::get_next_highlight(float abs_y, char type, int offset) const {

    int index = 0;
    auto sorted_highlights = get_highlights_sorted(type);

    for (auto hl : sorted_highlights) {
        if (hl.selection_begin.y <= abs_y) {
            index++;
        }
        else {
            break;
        }
    }

    // now index points the the next highlight
    if ((size_t)(index + offset) < sorted_highlights.size()) {
        return sorted_highlights[index + offset];
    }

    return {};
}

std::optional<Highlight> Document::get_prev_highlight(float abs_y, char type, int offset) const {

    int index = -1;
    auto sorted_highlights = get_highlights_sorted(type);

    for (auto hl : sorted_highlights) {
        if (hl.selection_begin.y < abs_y) {
            index++;
        }
        else {
            break;
        }
    }

    // now index points the the previous highlight
    if ((index + offset) >= 0) {
        return sorted_highlights[index + offset];
    }

    return {};
}

bool Document::needs_password() {
    return document_needs_password;
}


bool Document::apply_password(const char* password) {

    if (context && doc) {
        reload(password);
        if (password_was_correct) {
            correct_password = password;
        }
        return password_was_correct;
    }
    return false;
}

bool Document::needs_authentication() {
    if (needs_password()) {
        return !password_was_correct;
    }
    else {
        return false;
    }
}

std::vector<fz_rect> Document::get_highlighted_character_masks(int page) {
    fz_stext_page* stext_page = get_stext_with_page_number(page);
    std::vector<fz_stext_char*> flat_chars;
    get_flat_chars_from_stext_page(stext_page, flat_chars);
    std::vector<fz_rect> res;
    std::vector<std::wstring> words;
    std::vector<std::vector<fz_rect>> word_rects;
    get_word_rect_list_from_flat_chars(flat_chars, words, word_rects);

    for (size_t i = 0; i < words.size(); i++) {
        std::vector<fz_rect> highlighted_characters;
        int num_highlighted = static_cast<int>(std::ceil(words[i].size() * 0.3f));
        for (int j = 0; j < num_highlighted; j++) {
            highlighted_characters.push_back(word_rects[i][j]);
        }
        res.push_back(create_word_rect(highlighted_characters));
    }

    return res;
}


fz_rect Document::get_page_rect_no_cache(int page_number) {
    fz_rect res{};
    fz_try(context) {
        int n_pages = num_pages();
        if (page_number < n_pages) {
            fz_page* page = fz_load_page(context, doc, page_number);
            fz_rect bound = fz_bound_page(context, page);
            res = bound;
            fz_drop_page(context, page);
        }
    }
    fz_catch(context) {

    }
    return res;
}

void DocumentManager::free_document(Document* document) {
    bool found = false;
    std::wstring path_to_erase;

    for (auto [path, doc] : cached_documents) {
        if (doc == document) {
            found = true;
            path_to_erase = path;
        }
    }
    if (found) {
        cached_documents.erase(path_to_erase);
    }

    delete document;
}

std::optional<PdfLink> Document::get_link_in_pos(const DocumentPos& pos) {
    return get_link_in_pos(pos.page, pos.x, pos.y);
}

std::wstring Document::get_pdf_link_text(PdfLink link) {
    int page = link.source_page;
    fz_stext_page* stext_page = get_stext_with_page_number(page);
    std::vector<fz_stext_char*> flat_chars;
    get_flat_chars_from_stext_page(stext_page, flat_chars);
    std::vector<fz_rect> flat_chars_rects;
    std::vector<int> flat_chars_pages;
    std::wstring flat_chars_text;
    flat_char_prism(flat_chars,page, flat_chars_text, flat_chars_pages, flat_chars_rects);

    std::wstring res;
    for (int rect_index = 0; rect_index < link.rects.size(); rect_index++) {

        fz_rect current_link_rect = link.rects[rect_index];

        //for (int i = 0; i < flat_chars.size(); i++) {
        for (int i = 0; i < flat_chars_text.size(); i++) {
            fz_rect charrect = flat_chars_rects[i];
            float y = (charrect.y0 + charrect.y1) / 2;
            float x = (charrect.x0 + charrect.x1) / 2;
            float height = charrect.y1 - charrect.y0;
            float width = charrect.x1 - charrect.x0;
            charrect.y0 = y - height / 5;
            charrect.y1 = y + height / 5;

            charrect.x0 = x;
            charrect.x1 = x;
            //fz_rect intersection = fz_intersect_rect(charrect, link.rect);
            //if (std::abs(intersection.y1 - intersection.y0) > (0.9f * std::abs(charrect.y1 - charrect.y0))) {
            if (rects_intersect(charrect, current_link_rect)) {
                res.push_back(flat_chars_text[i]);
            }

        }
    }
    return res;
}

std::vector<PdfLink> Document::get_links_in_page_rect(int page, fz_rect rect) {
    if (!doc) return {};

    std::vector<PdfLink> res;
    int rect_page;
    fz_rect doc_rect = absolute_to_page_rect(rect, &rect_page);

    if (page != -1) {
        //fz_link* links = get_page_links(page);
        const std::vector<PdfLink>& links = get_page_merged_pdf_links(page);
        for (auto link : links) {
            for (auto link_rect : link.rects) {
                if (rects_intersect(doc_rect, link_rect)) {
                    res.push_back(link);
                    break;
                }
            }
        }
    }

    return res;
}

PdfLink Document::pdf_link_from_fz_link(int page, fz_link* link) {
    return PdfLink{ {link->rect}, page, link->uri };
}

std::optional<PdfLink> Document::get_link_in_pos(int page, float doc_x, float doc_y) {
    if (!doc) return {};

    if (page != -1) {
        //fz_link* page_links = get_page_links(page);
        const std::vector<PdfLink>& page_links = get_page_merged_pdf_links(page);
        fz_point point = { doc_x, doc_y };
        std::optional<PdfLink> res = {};
        for (auto link : page_links) {
            for (auto link_rect : link.rects) {
                if (fz_is_point_inside_rect(point, link_rect)) {
                    return link;
                }
            }
        }
        //while (page_links != nullptr) {
        //    if (fz_is_point_inside_rect(point, page_links->rect)) {
        //        res = { {page_links->rect}, page, page_links->uri };
        //        return res;
        //    }
        //    page_links = page_links->next;
        //}

    }
    return {};
}

int Document::add_stext_page_to_created_toc(fz_stext_page* stext_page,
    int page_number,
    std::vector<TocNode*>& toc_node_stack,
    std::vector<TocNode*>& top_level_nodes) {

    int num_new_entries = 0;
    auto add_toc_node = [&](TocNode* node) {
        if (toc_node_stack.size() == 0) {
            top_level_nodes.push_back(node);
            toc_node_stack.push_back(node);
        }
        else {
            bool are_same = false;
            // pop until we reach a parent of stack is empty
            while ((toc_node_stack.size() > 0) &&
                (!is_title_parent_of(toc_node_stack[toc_node_stack.size() - 1]->title, node->title, &are_same)) &&
                (!are_same)) { // ignore items with the same name as current parent (happens when title of chapter is written on top of pages)
                toc_node_stack.pop_back();
            }

            if (are_same) return;
            num_new_entries += 1;

            if (toc_node_stack.size() > 0) {
                toc_node_stack[toc_node_stack.size() - 1]->children.push_back(node);
            }
            else {
                toc_node_stack.push_back(node);
                top_level_nodes.push_back(node);
            }
        }
    };

    LL_ITER(block, stext_page->first_block) {
        std::vector<fz_stext_char*> chars;
        get_flat_chars_from_block(block, chars);
        if (chars.size() > 0) {
            std::wstring block_string;
            std::vector<int> indices;
            get_text_from_flat_chars(chars, block_string, indices);
            if (is_string_titlish(block_string)) {
                TocNode* new_node = new TocNode;
                new_node->page = page_number;
                new_node->title = block_string;
                new_node->x = 0;
                new_node->y = block->bbox.y0;
                add_toc_node(new_node);
            }
        }
    }
    return num_new_entries;
}

float Document::document_to_absolute_y(int page, float doc_y) {
    if ((page < accum_page_heights.size()) && (page >= 0)) {
        return doc_y + accum_page_heights[page];
    }
    return 0;
}

AbsoluteDocumentPos Document::document_to_absolute_pos(DocumentPos doc_pos, bool center_mid) {
    float absolute_y = document_to_absolute_y(doc_pos.page, doc_pos.y);
    AbsoluteDocumentPos res = { doc_pos.x, absolute_y };
    if (center_mid && (doc_pos.page < page_widths.size())) {
        res.x -= page_widths[doc_pos.page] / 2;
    }
    return res;
}

fz_rect Document::document_to_absolute_rect(int page, fz_rect doc_rect, bool center_mid) {
    fz_rect res;
    AbsoluteDocumentPos x0y0 = document_to_absolute_pos({ page, doc_rect.x0, doc_rect.y0 }, center_mid);
    AbsoluteDocumentPos x1y1 = document_to_absolute_pos({ page, doc_rect.x1, doc_rect.y1 }, center_mid);

    res.x0 = x0y0.x;
    res.y0 = x0y0.y;
    res.x1 = x1y1.x;
    res.y1 = x1y1.y;

    return res;
}

//void Document::get_ith_next_line_from_absolute_y(float absolute_y, int i, bool cont, float* out_begin, float* out_end) {
//	auto [page, doc_x, doc_y] = absolute_to_page_pos({ 0, absolute_y });
//
//	fz_pixmap* pixmap = get_small_pixmap(page);
//	std::vector<unsigned int> hist = get_max_width_histogram_from_pixmap(pixmap);
//	std::vector<unsigned int> line_locations;
//	std::vector<unsigned int> line_locations_begins;
//	get_line_begins_and_ends_from_histogram(hist, line_locations_begins, line_locations);
//	int small_doc_y = static_cast<int>(doc_y * SMALL_PIXMAP_SCALE);
//
//	int index = find_nth_larger_element_in_sorted_list(line_locations, static_cast<unsigned int>(small_doc_y - 0.3f), i);
//
//	if (index > -1) {
//		int best_vertical_loc = line_locations[index];
//		int best_vertical_loc_begin = line_locations_begins[index];
//
//		float best_vertical_loc_doc_pos = best_vertical_loc / SMALL_PIXMAP_SCALE;
//		float best_vertical_loc_begin_doc_pos = best_vertical_loc_begin / SMALL_PIXMAP_SCALE;
//
//		float abs_doc_y = document_to_absolute_y(page, best_vertical_loc_doc_pos);
//		float abs_doc_begin_y = document_to_absolute_y(page, best_vertical_loc_begin_doc_pos);
//		*out_begin = abs_doc_begin_y;
//		*out_end = abs_doc_y;
//	}
//	else {
//		if (!cont) {
//			*out_begin = absolute_y;
//			*out_end = absolute_y;
//			return;
//		}
//
//		int next_page;
//		if (i > 0) {
//			//next_page = main_document_view->get_current_page_number() + 1;
//			next_page = get_offset_page_number(absolute_y) + 1;
//			if (next_page < num_pages()) {
//				return get_ith_next_line_from_absolute_y(get_accum_page_height(next_page) + 0.5, 1, false, out_begin, out_end);
//			}
//		}
//		else {
//			next_page = get_offset_page_number(absolute_y);
//			if (next_page > 0) {
//				return get_ith_next_line_from_absolute_y(get_accum_page_height(next_page) - 0.5f, -1, false, out_begin, out_end);
//			}
//		}
//		*out_begin = absolute_y;
//		*out_end = absolute_y;
//		return;
//	}
//
//}

//void Document::get_ith_next_line_from_absolute_y(float absolute_y, int i, bool cont, float* out_begin, float* out_end) {
fz_rect Document::get_ith_next_line_from_absolute_y(int page, int line_index, int i, bool cont, int* out_index, int* out_page) {
    //auto [page, doc_x, doc_y] = absolute_to_page_pos({ 0, absolute_y });

    auto line_rects = get_page_lines(page);
    //int index = 0;
    //while ((index < line_rects.size()) && (line_rects[index].y0 < absolute_y)) {
    //	index++;
    //}
    if (line_index < 0) {
        line_index = line_index + line_rects.size();
    }

    int new_index = line_index + i;
    if ((new_index >= 0) && ((size_t)new_index < line_rects.size())) {
        *out_page = page;
        *out_index = new_index;
        return line_rects[new_index];
    }
    else {
        if (!cont) {
            if (line_index > 0 && (size_t)line_index < line_rects.size()) {
                *out_page = page;
                *out_index = line_index;
                return line_rects[line_index];
            }
            else {
                fz_rect res;
                //*out_begin = accum_page_heights[page];
                //*out_end = accum_page_heights[page];
                res.y0 = accum_page_heights[page];
                res.y1 = accum_page_heights[page];
                res.x0 = 0;
                res.x1 = page_widths[page];

                *out_index = 0;
                *out_page = page;
                return res;
            }
        }

        if (i > 0) {
            //next_page = main_document_view->get_current_page_number() + 1;
            int next_page = page + 1;
            if (next_page < num_pages()) {
                return get_ith_next_line_from_absolute_y(next_page, 0, 0, false, out_index, out_page);
            }
        }
        else {
            int next_page = page - 1;
            if (next_page >= 0) {
                return get_ith_next_line_from_absolute_y(next_page, -1, 0, false, out_index, out_page);
            }
        }
        *out_page = page;
        *out_index = line_index;
        return line_rects[line_index];
    }

    //else {

    //	int next_page;
    //	if (i > 0) {
    //		//next_page = main_document_view->get_current_page_number() + 1;
    //		next_page = get_offset_page_number(absolute_y) + 1;
    //		if (next_page < num_pages()) {
    //			return get_ith_next_line_from_absolute_y(get_accum_page_height(next_page) + 0.5, 1, false, out_begin, out_end);
    //		}
    //	}
    //	else {
    //		next_page = get_offset_page_number(absolute_y);
    //		if (next_page > 0) {
    //			return get_ith_next_line_from_absolute_y(get_accum_page_height(next_page) - 0.5f, -1, false, out_begin, out_end);
    //		}
    //	}
    //	*out_begin = absolute_y;
    //	*out_end = absolute_y;
    //	return;
    //}

}

const std::vector<fz_rect>& Document::get_page_lines(
    int page,
    std::vector<std::wstring>* out_line_texts,
    std::vector<std::vector<fz_rect>>* out_line_rects){


    if ((out_line_rects == nullptr) && (cached_page_line_rects.find(page) != cached_page_line_rects.end())) {
        if (out_line_texts) {
            *out_line_texts = cached_line_texts[page];
        }
        //if (out_line_character_rects) {
        //    *out_line_character_rects = cached_line_char_rects[page];
        //}
        return cached_page_line_rects[page];
    }
    else {
        fz_stext_page* stext_page = get_stext_with_page_number(page);
        if (stext_page && stext_page->first_block && (!FORCE_CUSTOM_LINE_ALGORITHM)) {

            fz_page* mupdf_page = fz_load_page(context, doc, page);
            fz_rect bound = fz_bound_page(context, mupdf_page);
            bound = document_to_absolute_rect(page, bound);
            int halfwidth = (bound.x1 - bound.x0) / 2;
            bound.x0 -= halfwidth;
            bound.x1 -= halfwidth;
            fz_drop_page(context, mupdf_page);

            std::vector<fz_rect> line_rects;
            std::vector<std::wstring> line_texts;
            std::vector<fz_stext_line*> flat_lines;

            LL_ITER(block, stext_page->first_block) {
                if (block->type == FZ_STEXT_BLOCK_TEXT) {
                    LL_ITER(line, block->u.t.first_line) {
                        flat_lines.push_back(line);
                    }
                }
            }
            merge_lines(flat_lines, line_rects, line_texts, out_line_rects);
            for (size_t i = 0; i < line_rects.size(); i++) {
                line_rects[i].x0 = line_rects[i].x0 - page_widths[page] / 2;
                line_rects[i].x1 = line_rects[i].x1 - page_widths[page] / 2;
                line_rects[i].y0 = document_to_absolute_y(page, line_rects[i].y0);
                line_rects[i].y1 = document_to_absolute_y(page, line_rects[i].y1);
            }

            std::vector<fz_rect> line_rects_;
            std::vector<std::wstring> line_texts_;
            std::vector<std::vector<fz_rect>> line_char_rects_;

            for (int i = 0; i < line_rects.size(); i++) {
                if (fz_contains_rect(bound, line_rects[i])) {
                    line_rects_.push_back(line_rects[i]);
                    line_texts_.push_back(line_texts[i]);
                    if (out_line_rects) {
                        line_char_rects_.push_back((*out_line_rects)[i]);
                    }
                }
            }


            cached_page_line_rects[page] = line_rects_;
            cached_line_texts[page] = line_texts_;

            if (out_line_texts != nullptr) {
                *out_line_texts = line_texts_;
            }
            if (out_line_rects) {
                *out_line_rects = line_char_rects_;
            }

        }
        else {
            fz_pixmap* pixmap = get_small_pixmap(page);
            std::vector<unsigned int> hist = get_max_width_histogram_from_pixmap(pixmap);
            std::vector<unsigned int> line_locations;
            std::vector<unsigned int> line_locations_begins;
            get_line_begins_and_ends_from_histogram(hist, line_locations_begins, line_locations);

            std::vector<fz_rect> line_rects;
            for (size_t i = 0; i < line_locations_begins.size(); i++) {
                fz_rect line_rect;
                line_rect.x0 = 0 - page_widths[page] / 2;
                line_rect.x1 = static_cast<float>(pixmap->w) / SMALL_PIXMAP_SCALE - page_widths[page] / 2;
                line_rect.y0 = document_to_absolute_y(page, static_cast<float>(line_locations_begins[i]) / SMALL_PIXMAP_SCALE);
                line_rect.y1 = document_to_absolute_y(page, static_cast<float>(line_locations[i]) / SMALL_PIXMAP_SCALE);
                line_rects.push_back(line_rect);
            }
            cached_page_line_rects[page] = line_rects;
        }
        return cached_page_line_rects[page];
    }
}
void Document::clear_toc_nodes() {
    for (auto node : top_level_toc_nodes) {
        clear_toc_node(node);
    }
    top_level_toc_nodes.clear();
}

void Document::clear_toc_node(TocNode* node) {
    for (auto child : node->children) {
        clear_toc_node(child);
    }
    delete node;
}

DocumentManager::~DocumentManager() {
    for (auto [path, doc] : cached_documents) {
        delete doc;
    }
    cached_documents.clear();
}

bool Document::is_super_fast_index_ready() {
    return super_fast_search_index_ready;
}


std::vector<SearchResult> Document::search_text(std::wstring query, SearchCaseSensitivity case_sensitive, int begin_page, int min_page, int max_page) {

    return search_text_with_index(
        super_fast_search_index,
        super_fast_search_index_pages,
        super_fast_search_rects,
        query,
        case_sensitive,
        begin_page,
        min_page,
        max_page);

}

std::vector<SearchResult> Document::search_regex(std::wstring query, SearchCaseSensitivity case_sensitive, int begin_page, int min_page, int max_page)
{
    return search_regex_with_index(super_fast_search_index,
        super_fast_search_index_pages,
        super_fast_search_rects,
        query,
        case_sensitive,
        begin_page,
        min_page,
        max_page);
}

float Document::max_y_offset() {
    int np = num_pages();

    return get_accum_page_height(np - 1) + get_page_height(np - 1);

}

std::wstring Document::get_page_label(int page_index) {
    if (page_index >= 0 && page_index < page_labels.size()) {
        return page_labels[page_index];
    }
    else {
        return QString::number(page_index + 1).toStdWString();
    }
}

int Document::get_page_number_with_label(std::wstring page_label) {
    if (page_labels.size() > 0) {
        for (int i = 0; i < page_labels.size(); i++) {
            if (page_labels[i] == page_label) {
                return i;
            }
        }
        return -1;
    }
    else {
        return QString::fromStdWString(page_label).toInt();
    }
}


bool Document::is_reflowable() {
    if (doc != nullptr) {
        return fz_is_document_reflowable(context, doc);
    }
    return false;
}

void Document::clear_document_caches() {
    cached_num_pages = {};
    cached_fastread_highlights.clear();
    cached_line_texts.clear();
    cached_page_line_rects.clear();

    for (auto [_, cached_small_pixmap] : cached_small_pixmaps) {
        fz_drop_pixmap(context, cached_small_pixmap);
    }
    cached_small_pixmaps.clear();

    for (auto [_, cached_stext_page] : cached_stext_pages) {
        fz_drop_stext_page(context, cached_stext_page);
    }
    cached_stext_pages.clear();

    for (auto page_link_pair : cached_page_links) {
        fz_drop_link(context, page_link_pair.second);
    }
    cached_page_links.clear();

    delete cached_toc_model;
    cached_toc_model = nullptr;

    super_fast_search_index.clear();
    super_fast_search_index_pages.clear();
    super_fast_search_rects.clear();
    super_fast_search_index_ready = false;

    clear_toc_nodes();
}


void Document::load_document_caches(bool* invalid_flag, bool force_now) {

    load_page_dimensions(force_now);
    create_toc_tree(top_level_toc_nodes);
    get_flat_toc(top_level_toc_nodes, flat_toc_names, flat_toc_pages);
    invalid_flag_pointer = invalid_flag;

    // we don't need to index figures in helper documents
    index_document(invalid_flag);
}

int Document::reflow(int page) {

    fz_bookmark last_position_bookmark = fz_make_bookmark(context,
        doc, fz_location_from_page_number(context, doc, page));

    clear_document_caches();

    //if (EPUB_CSS.size() > -1) {
    //	std::string encoded = utf7_encode(EPUB_CSS);
    //	fz_set_user_css(context, encoded.c_str());
    //}
    //else {
    //	QString temp = EPUB_TEMPLATE;
    //	std::string encoded = temp.replace("%{line_spacing}", QString::number(EPUB_LINE_SPACING)).toStdString();
    //	fz_set_user_css(context, encoded.c_str());
    //}

    fz_layout_document(context, doc, EPUB_WIDTH, EPUB_HEIGHT, EPUB_FONT_SIZE);

    bool flag = false;
    load_document_caches(&flag, false);

    fz_location loc = fz_lookup_bookmark(context, doc, last_position_bookmark);
    int new_page = fz_page_number_from_location(context, doc, loc);
    return new_page;
}

//void push_embedding(int index, std::vector<float>& target) {
//    int offset = index * EMBEDDING_DIM;
//    for (int i = 0; i < EMBEDDING_DIM; i++) {
//        target.push_back(embedding_weights[offset + i]);
//    }
//}

//std::vector<float> apply_linear_weights(std::vector<float> embeddings) {
//
//    std::vector<float> outputs;
//
//    for (int j = 0; j < 2; j++) {
//        float res = 0;
//        for (int i = 0; i < embeddings.size(); i++) {
//            res += embeddings[i] * linear_weights[i + j * embeddings.size()];
//        }
//        outputs.push_back(res);
//    }
//    return outputs;
//}

std::vector<std::wstring> Document::get_page_bib_candidates(int page_number, std::vector<fz_rect>* out_rects) {
    fz_stext_page* stext_page = get_stext_with_page_number(page_number);
    std::vector<DocumentCharacter> flat_chars;

    //get_flat_chars_from_stext_page(stext_page, flat_chars, true);
    get_flat_chars_from_stext_page_for_bib_detection(stext_page, flat_chars);

    std::vector<fz_rect> char_rects;
    std::vector<float> augumented_rect_data;

    float page_width = page_widths[page_number];
    float page_height = page_heights[page_number];

    std::vector<int> dot_indices;
    std::vector<int> end_indices;

    std::vector<int> bracket_end_indices;
    std::vector<int> indented_end_indices;

    std::wstring raw_text;
    //std::wstring spaced_text;
    //std::vector<int> raw_to_spaced_index;
    for (int i = 0; i < flat_chars.size(); i++) {
        if (flat_chars[i].c == '.') {
            dot_indices.push_back(i);
        }
        else if (flat_chars[i].is_final && i < (flat_chars.size() - 1) && (flat_chars[i + 1].c == '[')) {
            dot_indices.push_back(i);
        }

    //    else if (flat_chars[i].is_final && i < (flat_chars.size() - 1) && (flat_chars[i + 1].c == '[')) {
    //        dot_indices.push_back(i);
    //    }
        char_rects.push_back(flat_chars[i].rect);
        raw_text.push_back(flat_chars[i].c);
        //spaced_text.push_back(flat_chars[i].c);
        //raw_to_spaced_index.push_back(i);
    }

    //for (int i = 0; i < flat_chars.size(); i++) {
    //    if (flat_chars[i].c == '.') {
    //        dot_indices.push_back(i);
    //    }
    //    else if (flat_chars[i].is_final && i < (flat_chars.size() - 1) && (flat_chars[i + 1].c == '[')) {
    //        dot_indices.push_back(i);
    //    }

    //    raw_text.push_back(flat_chars[i].c);
    //    spaced_text.push_back(flat_chars[i].c);
    //    char_rects.push_back(flat_chars[i].rect);

    //    if (flat_chars[i].is_final && (flat_chars[i].c != '-')) {
    //        spaced_text.push_back(' ');
    //    }
    //    raw_to_spaced_index.push_back(spaced_text.size()-1);

    //}

    for (auto rect : char_rects) {
        get_rect_augument_data(rect, page_width, page_height, augumented_rect_data);
    }


    for (int dot_index : dot_indices) {
        //std::vector<float> embeddings;
        //int start_index = dot_index - WINDOW_SIZE;
        //int end_index = dot_index + WINDOW_SIZE;

        //for (int index = start_index; index < end_index; index++) {
        //    if (index >= 0 && index < flat_chars.size()) {
        //        //embeddings.push_back(augumented_rect_data[index]);
        //        push_embedding(std::min(flat_chars[index].c, 127), embeddings);
        //    }
        //    else {
        //        push_embedding(0, embeddings);
        //    }

        //}
        //for (int index = start_index; index < end_index; index++) {
        //    const int RECT_DIM = 4 * 8;
        //    int rect_data_start_index = index * RECT_DIM;
        //    if (index >= 0 && index < flat_chars.size()) {
        //        for (int i = 0; i < RECT_DIM; i++) {
        //            embeddings.push_back(augumented_rect_data[index * RECT_DIM + i]);
        //        }
        //    }
        //    else {
        //        for (int i = 0; i < RECT_DIM; i++) {
        //            embeddings.push_back(0.0f);
        //        }
        //    }


        //}

        //auto outputs = apply_linear_weights(embeddings);
        //if (outputs[1] > outputs[0]) {
        //    end_indices.push_back(dot_index);
        //}
        if (is_dot_index_end_of_a_reference(flat_chars, dot_index)) {
            end_indices.push_back(dot_index);
            if (dot_index + 1 < flat_chars.size() && (flat_chars[dot_index + 1].c == '[')) {
                bracket_end_indices.push_back(dot_index);
            }
            if (dot_index + 1 < flat_chars.size()){
                int next_index = dot_index + 1;

                // skip phantom spaces
                if (flat_chars[next_index].stext_char == nullptr) next_index++;

                if (next_index == flat_chars.size()) {
                    indented_end_indices.push_back(dot_index);
                }
                else {
                    fz_rect dot_line_begin_rect = fz_rect_from_quad(flat_chars[dot_index].stext_line->first_char->quad);
                    fz_rect next_line_rect = flat_chars[next_index].rect;
                    float next_x = (next_line_rect.x0 + next_line_rect.x1) / 2;
                    float height = std::abs(next_line_rect.y1 - next_line_rect.y0);

                    if (next_x < dot_line_begin_rect.x0) {
                        indented_end_indices.push_back(dot_index);
                    }
                    //else if (next_line_rect.y0 < dot_line_begin_rect.y1) {
                    else if (std::abs(next_line_rect.y0 - dot_line_begin_rect.y0) > (5 * height)) {
                        indented_end_indices.push_back(dot_index);
                    }
                }
            }

        }

    }
    if (bracket_end_indices.size() > 3) {
        end_indices = bracket_end_indices;
    }

    if (indented_end_indices.size() > 3 && indented_end_indices.size() > bracket_end_indices.size()) {
        end_indices = indented_end_indices;
    }

    std::vector<fz_rect> res;
    for (int i = 0; i < end_indices.size(); i++) {
        res.push_back(char_rects[end_indices[i]]);
    }

    std::vector<std::wstring> reference_texts;
    if (end_indices.size() == 0) {
        return reference_texts;
    }

    //reference_texts.push_back(raw_text.substr(0, end_indices[0]));
    reference_texts.push_back(raw_text.substr(0, end_indices[0]));
    for (int i = 1; i < end_indices.size(); i++) {
        //reference_texts.push_back(raw_text.substr(end_indices[i - 1] + 1, end_indices[i] - end_indices[i - 1]));
        int length = end_indices[i] - end_indices[i - 1];
        reference_texts.push_back(raw_text.substr(end_indices[i-1] + 1, length));
    }

    // try to remove the texts before the first bib item

    int bib_index = -1;

    for (int i = 0; i < std::min<int>(5, reference_texts.size()); i++) {
        int last_bib_index = QString::fromStdWString(reference_texts[i]).toLower().lastIndexOf("bibliography");
        int last_ref_index = QString::fromStdWString(reference_texts[i]).toLower().lastIndexOf("references");
        int len = -1;

        if (last_bib_index != -1) {
            len = std::string("bibligraphy").size();
        }
        if (last_ref_index != -1) {
            len = std::string("references").size();
        }

        int last_index = std::max(last_bib_index, last_ref_index);
        if (last_index != -1) {
            bib_index = i;
            reference_texts[i] = reference_texts[i].substr(last_index + len, reference_texts[i].size() - last_index - len);
            break;

        }
    }

    for (int i = 0; i < bib_index; i++) {
        reference_texts.erase(reference_texts.begin());
    }

    if (out_rects) {

        for (int i = 0; i < end_indices.size(); i++) {
            out_rects->push_back(char_rects[end_indices[i]]);
        }

    }

    return reference_texts;

}

std::optional<std::pair<std::wstring, fz_rect>> Document::get_page_bib_with_reference(int page_number, std::wstring reference_text) {
    //todo: use the reference offset as well as the page to more accurately get the reference 


    reference_text = remove_et_al(reference_text);
    std::vector<fz_rect> bib_rects;
    std::vector<std::wstring> bib_texts_ = get_page_bib_candidates(page_number, &bib_rects);
    std::vector<std::wstring> bib_text_prefixes;
    for (auto bib : bib_texts_) {
        bib_text_prefixes.push_back(bib.substr(0, reference_text.size() + 5));
    }

    QString reference_text_qstring = QString::fromStdWString(reference_text);
    std::string encoded_reference_text = utf8_encode(reference_text);

    for (int i = 0; i < bib_text_prefixes.size(); i++) {
        if (QString::fromStdWString(bib_text_prefixes[i]).indexOf(reference_text_qstring) != -1) {
            return std::make_pair(bib_texts_[i], bib_rects[i]);
        }
    }

    int score = -1;
    int max_score = encoded_reference_text.size();
    int max_score_index = -1;

    for (int i = 0; i < bib_text_prefixes.size(); i++) {
        std::string encoded_bib_text = utf8_encode(bib_text_prefixes[i]);
        int current_score = lcs(&encoded_bib_text[0], &encoded_reference_text[0], encoded_bib_text.size(), encoded_reference_text.size());
        if (current_score == max_score) {
            return std::make_pair(bib_texts_[i], bib_rects[i]);
        }
        if (current_score > score) {
            score = current_score;
            max_score_index = i;
        }
    }
    if (max_score_index >= 0) {
        return std::make_pair(bib_texts_[max_score_index], bib_rects[max_score_index]);
    }

    return {};
}

std::wstring Document::get_text_in_rect(int page, fz_rect doc_rect) {
    fz_stext_page* stext_page = get_stext_with_page_number(page);
    std::vector<fz_stext_char*> flat_chars;
    get_flat_chars_from_stext_page(stext_page, flat_chars);

    std::wstring res;

    for (auto ch : flat_chars) {

        fz_rect scaled_rect = fz_rect_from_quad(ch->quad);
        float width = scaled_rect.x1 - scaled_rect.x0;
        float height = scaled_rect.y1 - scaled_rect.y0;
        scaled_rect.x0 += width / 4;
        scaled_rect.x1 -= width / 4;
        scaled_rect.y0 += height / 4;
        scaled_rect.y1 -= height / 4;

        if (rects_intersect(doc_rect, scaled_rect)) {
            res.push_back(ch->c);
        }

    }
    return res;
}


void Document::add_freehand_drawing(FreehandDrawing new_drawing) {
    if (new_drawing.points.size() > 0) {
        DocumentPos docpos = absolute_to_page_pos(new_drawing.points[0].pos);
        drawings_mutex.lock();
        page_freehand_drawings[docpos.page].push_back(new_drawing);
        drawings_mutex.unlock();
        is_drawings_dirty = true;
    }
}

void Document::undo_freehand_drawing() {
    int most_recent_page_index = -1;
    QDateTime most_recent_page_time;

    drawings_mutex.lock();
    for (auto& [page, drawings] : page_freehand_drawings) {
        if (drawings.size() > 0) {
            if (most_recent_page_index == -1) {
                most_recent_page_index = page;
                most_recent_page_time = drawings[drawings.size() - 1].creattion_time;
            }
            else {
                if (drawings[drawings.size() - 1].creattion_time.secsTo(most_recent_page_time) > 0) {
                    most_recent_page_index = page;
                    most_recent_page_time = drawings[drawings.size() - 1].creattion_time;
                }
            }
        }
    }
    if (most_recent_page_index >= 0) {
        is_drawings_dirty = true;
        page_freehand_drawings[most_recent_page_index].pop_back();
    }
    drawings_mutex.unlock();
}

const std::vector<FreehandDrawing>& Document::get_page_drawings(int page) {
    return page_freehand_drawings[page];
}

std::vector<int> Document::get_page_intersecting_drawing_indices(int page, fz_rect absolute_rect, bool mask[26]) {
    std::vector<int> indices;
    std::vector<FreehandDrawing>& page_drawings = page_freehand_drawings[page];

    for (int i = 0; i < page_drawings.size(); i++) {
        if (!mask[page_drawings[i].type - 'a']) {
            continue;
        }
        for (auto point : page_drawings[i].points) {
            fz_point absolute_point = fz_point{ point.pos.x, point.pos.y };
            if (fz_is_point_inside_rect(absolute_point, absolute_rect)) {
                indices.push_back(i);
                break;
            }
        }
    }
    return indices;

}

void Document::delete_page_intersecting_drawings(int page, fz_rect absolute_rect, bool mask[26]) {
    std::vector<FreehandDrawing>& page_drawings = page_freehand_drawings[page];
    std::vector<int> indices_to_delete = get_page_intersecting_drawing_indices(page, absolute_rect, mask);

    for (int j = indices_to_delete.size() - 1; j >= 0; j--) {
        //page_freehand_drawings.erase(page_freehand_drawings.begin() + indices_to_delete[j]);
        page_freehand_drawings[page].erase(page_freehand_drawings[page].begin() + indices_to_delete[j]);
    }
    is_drawings_dirty = true;
}

std::wstring Document::get_drawings_file_path() {
    Path path = Path(file_name);
#ifdef SIOYEK_ANDROID
    if (file_name == L":/tutorial.pdf") {
        QString parent_path = QStandardPaths::standardLocations(QStandardPaths::AppDataLocation).at(0);
        return Path(parent_path.toStdWString()).slash(L"tutorial.pdf.sioyek.drawings").get_path();
    }
#endif
    QString filename = QString::fromStdWString(path.filename().value());
    QString drawing_file_name = filename + ".sioyek.drawings";
    return path.file_parent().slash(drawing_file_name.toStdWString()).get_path();
}

bool Document::annotations_file_exists() {
    std::wstring file_path = get_annotations_file_path();
    return QFileInfo(QString::fromStdWString(file_path)).exists();
}

bool Document::annotations_file_is_newer_than_database() {
    std::wstring file_path = get_annotations_file_path();
    QFileInfo file_info(QString::fromStdWString(file_path));
    QFileInfo database_file_info(QString::fromStdWString(SHARED_DATABASE_PATH));

    if (file_info.exists() && database_file_info.exists()) {
        return file_info.lastModified() > database_file_info.lastModified();
    }

    return false;
}

std::wstring Document::get_annotations_file_path() {
    Path path = Path(file_name);
#ifdef SIOYEK_ANDROID
    if (file_name == L":/tutorial.pdf") {
        QString parent_path = QStandardPaths::standardLocations(QStandardPaths::AppDataLocation).at(0);
        return Path(parent_path.toStdWString()).slash(L"tutorial.pdf.sioyek.annotations").get_path();
    }
#endif
    QString filename = QString::fromStdWString(path.filename().value());
    QString drawing_file_name = filename + ".sioyek.annotations";
    return path.file_parent().slash(drawing_file_name.toStdWString()).get_path();
}


void Document::load_drawings_async() {
    drawings_mutex.lock();
    auto drawing_load_thread = std::thread([&]() {
        load_drawings();
        });
    drawing_load_thread.detach();
    drawings_mutex.unlock();
}

//void Document::persist_drawings_async() {
//	if (!is_drawings_dirty) {
//		return;
//	}
//
//	drawings_mutex.lock();
//	auto drawing_persist_thread = std::thread([&]() {
//		persist_drawings();
//		});
//	drawing_persist_thread.detach();
//	drawings_mutex.unlock();
//}

void Document::load_annotations(bool sync) {

    //if (document_indexing_thread.has_value() && document_indexing_thread->)
    should_reload_annotations = false;
    if (highlights.size() > 0 && (highlights[0].highlight_rects.size() == 0)) {
        fill_highlight_rects(context, doc);
    }

    if (!are_highlights_loaded || !get_checksum_fast().has_value()) {
        return;
    }

    std::wstring annotation_fille_path = get_annotations_file_path();

    QFile json_file(QString::fromStdWString(annotation_fille_path));
    if (json_file.open(QFile::ReadOnly)) {
        QJsonDocument json_document = QJsonDocument().fromJson(json_file.readAll());
        json_file.close();

        QJsonObject root = json_document.object();
        std::vector<BookMark> file_bookmarks = load_from_json_array<BookMark>(root["bookmarks"].toArray());
        std::vector<Highlight> file_highlights = load_from_json_array<Highlight>(root["highlights"].toArray());
        std::vector<Mark> file_marks = load_from_json_array<Mark>(root["marks"].toArray());
        std::vector<Portal> file_portals = load_from_json_array<Portal>(root["portals"].toArray());

        std::string current_checksum = get_checksum();

        auto portals_array = root["portals"].toArray();

        // set the destination of self-referencing portals to be the current
        // document and not the original doucment
        for (int i = 0; i < portals_array.size(); i++) {
            if (portals_array[i].toObject()["same"].toBool()) {
                file_portals[i].dst.document_checksum = current_checksum;
            }
        }

        std::vector<Annotation*> new_annotations;
        std::vector<Annotation*> updated_annotations;
        std::vector<Annotation*> deleted_annotations;

        std::map<std::string, int> bookmark_index_map = annotation_prism(file_bookmarks, bookmarks, new_annotations, updated_annotations, deleted_annotations);
        std::map<std::string, int> highlight_index_map = annotation_prism(file_highlights, highlights, new_annotations, updated_annotations, deleted_annotations);
        std::map<std::string, int> mark_index_map = annotation_prism(file_marks, marks, new_annotations, updated_annotations, deleted_annotations);
        std::map<std::string, int> portal_index_map = annotation_prism(file_portals, portals, new_annotations, updated_annotations, deleted_annotations);


        for (int i = 0; i < updated_annotations.size(); i++) {
            bool success = db_manager->update_annotation(updated_annotations[i]);
            if (success) {
                if (dynamic_cast<BookMark*>(updated_annotations[i])) {
                    bookmarks[bookmark_index_map[updated_annotations[i]->uuid]] = *dynamic_cast<BookMark*>(updated_annotations[i]);
                }

                if (dynamic_cast<Highlight*>(updated_annotations[i])) {
                    highlights[highlight_index_map[updated_annotations[i]->uuid]] = *dynamic_cast<Highlight*>(updated_annotations[i]);
                    fill_index_highlight_rects(highlight_index_map[updated_annotations[i]->uuid]);
                }

                if (dynamic_cast<Mark*>(updated_annotations[i])) {
                    marks[mark_index_map[updated_annotations[i]->uuid]] = *dynamic_cast<Mark*>(updated_annotations[i]);
                }

                if (dynamic_cast<Portal*>(updated_annotations[i])) {
                    portals[portal_index_map[updated_annotations[i]->uuid]] = *dynamic_cast<Portal*>(updated_annotations[i]);
                }

            }
        }

        for (int i = 0; i < new_annotations.size(); i++) {
            bool success = db_manager->insert_annotation(new_annotations[i], get_checksum());

            if (success) {
                if (dynamic_cast<BookMark*>(new_annotations[i])) {
                    bookmarks.push_back(*dynamic_cast<BookMark*>(new_annotations[i]));
                }

                if (dynamic_cast<Highlight*>(new_annotations[i])) {
                    highlights.push_back(*dynamic_cast<Highlight*>(new_annotations[i]));
                    fill_index_highlight_rects(highlights.size() - 1);
                }

                if (dynamic_cast<Mark*>(new_annotations[i])) {
                    marks.push_back(*dynamic_cast<Mark*>(new_annotations[i]));
                }

                if (dynamic_cast<Portal*>(new_annotations[i])) {
                    portals.push_back(*dynamic_cast<Portal*>(new_annotations[i]));
                }
            }
        }

        if (sync) {
            for (int i = 0; i < deleted_annotations.size(); i++) {
                bool success = db_manager->delete_annotation(deleted_annotations[i]);
            }
            bookmarks = file_bookmarks;
            highlights = file_highlights;
            marks = file_marks;
            portals = file_portals;

            for (int i = 0; i < highlights.size(); i++) {
                fill_index_highlight_rects(i);
            }

        }

    }

}

void Document::load_drawings() {

    std::wstring drawing_file_path = get_drawings_file_path();

    QFile json_file(QString::fromStdWString(drawing_file_path));
    if (json_file.open(QFile::ReadOnly)) {
        QJsonDocument json_document = QJsonDocument().fromJson(json_file.readAll());
        json_file.close();

        QJsonObject root = json_document.object();
        QJsonObject drawings_object = root["drawings"].toObject();
        page_freehand_drawings.clear();
        is_drawings_dirty = false;

        for (auto& page_str : drawings_object.keys()) {
            int page_number = page_str.toInt();
            QJsonArray page_drawings_array = drawings_object[page_str].toArray();
            for (auto val : page_drawings_array) {
                QJsonObject drawing_object = val.toObject();
                FreehandDrawing drawing;

                drawing.creattion_time = QDateTime::fromString(drawing_object["creation_time"].toString(), Qt::ISODate);
                drawing.type = drawing_object["type"].toInt();
                QJsonArray x_array = drawing_object["point_xs"].toArray();
                QJsonArray y_array = drawing_object["point_ys"].toArray();
                QJsonArray t_array = drawing_object["point_thicknesses"].toArray();

                for (int i = 0; i < x_array.size(); i++) {
                    FreehandDrawingPoint point;
                    point.pos.x = x_array.at(i).toDouble();
                    point.pos.y = y_array.at(i).toDouble();
                    point.thickness = t_array.at(i).toDouble();
                    drawing.points.push_back(point);
                }
                page_freehand_drawings[page_number].push_back(drawing);
            }
        }

    }

}

void Document::persist_annotations(bool force) {

    if ((!is_annotations_dirty) && (!force)) {
        return;
    }

    if (!annotations_file_exists() && (!force)) {
        return;
    }

    std::string current_checksum = get_checksum();

    QJsonArray json_bookmarks = export_array(bookmarks, current_checksum);
    QJsonArray json_highlights = export_array(highlights, current_checksum);
    QJsonArray json_marks = export_array(marks, current_checksum);
    QJsonArray json_portals = export_array(portals, current_checksum);


    QJsonObject book_object;
    book_object["bookmarks"] = json_bookmarks;
    book_object["marks"] = json_marks;
    book_object["highlights"] = json_highlights;
    book_object["portals"] = json_portals;

    QJsonDocument json_document(book_object);

    QFile output_file(QString::fromStdWString(get_annotations_file_path()));
    output_file.open(QFile::WriteOnly);
    output_file.write(json_document.toJson());
    output_file.close();

    is_annotations_dirty = false;
}

void Document::persist_drawings(bool force) {
    if ((!force) && (!is_drawings_dirty)) {
        return;
    }

    std::wstring drawing_file_path = get_drawings_file_path();
    QJsonObject root_object;
    QJsonObject drawings_object;

    for (auto& [page, drawings] : page_freehand_drawings) {
        QJsonArray page_drawings_array;

        for (auto& drawing : drawings) {
            QJsonObject drawing_object;
            drawing_object["creation_time"] = drawing.creattion_time.toString(Qt::ISODate);
            drawing_object["type"] = drawing.type;
            //QJsonArray points;
            QJsonArray points_xs;
            QJsonArray points_ys;
            QJsonArray points_thicknesses;

            for (auto& p : drawing.points) {
                points_xs.append(p.pos.x);
                points_ys.append(p.pos.y);
                points_thicknesses.append(p.thickness);
            }
            drawing_object["point_xs"] = points_xs;
            drawing_object["point_ys"] = points_ys;
            drawing_object["point_thicknesses"] = points_thicknesses;

            page_drawings_array.append(drawing_object);
        }
        drawings_object[QString::number(page)] = page_drawings_array;
    }
    root_object["drawings"] = drawings_object;


    QJsonDocument json_doc;
    json_doc.setObject(root_object);

    QFile json_file(QString::fromStdWString(drawing_file_path));
    json_file.open(QFile::WriteOnly);
    json_file.write(json_doc.toJson());
    json_file.close();

    is_drawings_dirty = false;
}

std::vector<std::wstring> DocumentManager::get_loaded_document_paths() {
    std::vector<std::wstring> res;
    for (auto& [path, _] : cached_documents) {
        res.push_back(path);
    }
    return res;
}

std::optional<Document*> DocumentManager::get_cached_document(const std::wstring& path) {
    if (cached_documents.find(path) != cached_documents.end()) {
        return cached_documents[path];
    }
    return {};
}

int Document::find_highlight_index_with_uuid(const std::string& uuid) {
    for (int i = 0; i < highlights.size(); i++) {
        if (highlights[i].uuid == uuid) return i;
    }
    return -1;
}

void Document::update_highlight_add_text_annotation(const std::string& uuid, const std::wstring& text_annot) {
    int highlight_index = find_highlight_index_with_uuid(uuid);
    if (highlight_index > -1) {
        db_manager->update_highlight_add_annotation(uuid, text_annot);
        is_annotations_dirty = true;
        highlights[highlight_index].text_annot = text_annot;
        highlights[highlight_index].update_modification_time();

    }
}

void Document::update_highlight_type(const std::string& uuid, char new_type) {
    db_manager->update_highlight_type(uuid, new_type);
    is_annotations_dirty = true;
}

void Document::update_highlight_type(int index, char new_type) {
    update_highlight_type(highlights[index].uuid, new_type);
    highlights[index].type = new_type;
    highlights[index].update_modification_time();
}

int Document::get_portal_index_at_pos(AbsoluteDocumentPos abspos) {
    for (int i = 0; i < portals.size(); i++) {
        if (portals[i].src_offset_x.has_value()) {
            if (fz_is_point_inside_rect({abspos.x, abspos.y}, portals[i].get_rectangle())) {
                return i;
            }
        }
    }
    return -1;
}

int Document::get_bookmark_index_at_pos(AbsoluteDocumentPos abspos) {
    for (int i = 0; i < bookmarks.size(); i++) {
        if (bookmarks[i].begin_y != -1) {
            if (bookmarks[i].end_y == -1) {

                if (fz_is_point_inside_rect({abspos.x, abspos.y}, bookmarks[i].get_rectangle())) {
                    return i;
                }
            }
            else {
                fz_rect bookmark_rect;
                bookmark_rect.x0 = bookmarks[i].begin_x;
                bookmark_rect.y0 = bookmarks[i].begin_y;
                bookmark_rect.x1 = bookmarks[i].end_x;
                bookmark_rect.y1 = bookmarks[i].end_y;

                if (fz_is_point_inside_rect({ abspos.x, abspos.y }, bookmark_rect)) {
                    return i;
                }
            }
        }
    }
    return -1;
}

void Document::update_bookmark_text(int index, const std::wstring& new_text, float new_font_size) {
    if ((index >= 0) && (index < bookmarks.size())) {
        if (db_manager->update_bookmark_change_text(bookmarks[index].uuid, new_text, new_font_size)) {
            bookmarks[index].description = new_text;
            bookmarks[index].update_modification_time();
            is_annotations_dirty = true;
        }
    }
}

void Document::update_bookmark_position(int index, AbsoluteDocumentPos new_begin_position, AbsoluteDocumentPos new_end_position) {
    if ((index >= 0) && (index < bookmarks.size())) {
        if (db_manager->update_bookmark_change_position(bookmarks[index].uuid, new_begin_position, new_end_position)) {
            bookmarks[index].y_offset_ = new_begin_position.y;
            bookmarks[index].begin_x = new_begin_position.x;
            bookmarks[index].begin_y = new_begin_position.y;
            bookmarks[index].end_x = new_end_position.x;
            bookmarks[index].end_y = new_end_position.y;
            bookmarks[index].update_modification_time();
            is_annotations_dirty = true;
        }
    }
}

void Document::update_portal_src_position(int index, AbsoluteDocumentPos new_position){
    if ((index >= 0) && (index < portals.size())) {
        if (db_manager->update_portal_change_src_position(portals[index].uuid, new_position)) {
            portals[index].src_offset_x = new_position.x;
            portals[index].src_offset_y = new_position.y;
            portals[index].update_modification_time();
            is_annotations_dirty = true;
        }
    }
}

bool Document::is_bookmark_new(const BookMark& new_bookmark) {
    for (auto bookmark : bookmarks) {
        if (are_same(bookmark, new_bookmark)) {
            return false;
        }
    }
    return true;
}


bool Document::is_highlight_new(const Highlight& new_highlight) {
    for (auto highlight : highlights) {
        if (are_same(highlight, new_highlight)) {
            return false;
        }
    }
    return true;
}

bool Document::is_drawing_new(const FreehandDrawing& drawing) {
    if (drawing.points.size() == 0) return false;

    int drawing_page = absolute_to_page_pos(drawing.points[0].pos).page;
    for (auto page_drawing : page_freehand_drawings[drawing_page]) {
        if (are_same(page_drawing, drawing)) {
            return false;
        }
    }
    return true;

}

bool Document::should_render_pdf_annotations() {
    return should_render_annotations;
}

void Document::set_should_render_pdf_annotations(bool val) {
    should_render_annotations = val;
}

bool Document::get_should_render_pdf_annotations() {
    return should_render_annotations;
}

std::vector<BookMark> Document::get_new_sioyek_bookmarks(const std::vector<BookMark>& pdf_bookmarks) {
    std::vector<BookMark> res;

    for (auto bookmark : bookmarks) {
        bool exists_in_pdf = false;

        for (auto pdf_bookmark : pdf_bookmarks) {
            if (are_same(bookmark, pdf_bookmark)) {
                exists_in_pdf = true;
                break;
            }
        }
        if (!exists_in_pdf) {
            res.push_back(bookmark);
        }
    }
    return res;
}

std::vector<Highlight> Document::get_new_sioyek_highlights(const std::vector<Highlight>& pdf_highlights) {
    std::vector<Highlight> res;

    for (auto highlight : highlights) {
        bool exists_in_pdf = false;

        for (auto pdf_highlight : pdf_highlights) {
            if (are_same(highlight, pdf_highlight)) {
                exists_in_pdf = true;
                break;
            }
        }
        if (!exists_in_pdf) {
            res.push_back(highlight);
        }
    }
    return res;
}
int Document::num_freehand_drawings() {
    int res = 0;

    for (auto [page, drawings] : page_freehand_drawings) {
        res += drawings.size();
    }
    return res;
}

std::vector<FreehandDrawing> Document::get_page_freehand_drawings_with_indices(int page, const std::vector<int> indices) {
    std::vector<FreehandDrawing> results;
    const std::vector<FreehandDrawing>& page_drawings = page_freehand_drawings[page];
    for (auto index : indices) {
        results.push_back(page_drawings[index]);
    }
    return results;
}
int Document::get_highlight_index_with_uuid(std::string uuid) {
    for (int i = 0; i < highlights.size(); i++) {
        if (highlights[i].uuid == uuid) {
            return i;
        }
    }
    return -1;
}

int Document::get_bookmark_index_with_uuid(std::string uuid) {
    for (int i = 0; i < bookmarks.size(); i++) {
        if (bookmarks[i].uuid == uuid) {
            return i;
        }
    }
    return -1;
}

std::string Document::get_highlight_index_uuid(int index) {
    if ((index >= 0) && (index < highlights.size())) {
        return highlights[index].uuid;
    }
    return "";
}

std::string Document::get_bookmark_index_uuid(int index) {
    if ((index >= 0) && (index < bookmarks.size())) {
        return bookmarks[index].uuid;
    }
    return "";
}

bool Document::get_should_reload_annotations() {
    return should_reload_annotations;
}


void Document::reload_annotations_on_new_checksum() {
    if (annotations_file_exists()) {
        load_annotations();
    }
    else {
        load_document_metadata_from_db();
    }
}
std::vector<Portal>& Document::get_portals() {
    return portals;
}

std::optional<std::wstring> DocumentManager::get_path_from_hash(const std::string& checksum) {
    if (hash_to_path.find(checksum) != hash_to_path.end()) {
        return hash_to_path[checksum];
    }
    std::vector<std::wstring> paths;

    db_manager->get_path_from_hash(checksum, paths);

    if (paths.size() > 0) {
        hash_to_path[checksum] = paths[0];
        return paths[0];
    }

    return {};
}

Document* DocumentManager::get_document_with_checksum(const std::string& checksum) {


    std::optional<std::wstring> path = get_path_from_hash(checksum);
    if (path) {
        return get_document(path.value());
    }
    return nullptr;

    //db_manager->get_path_from_hash();
    //if (hash_to_path.find(checksum) != hash_to_path.end()) {
    //    return get_document(hash_to_path[checksum]);
    //}
}

std::vector<Portal> Document::get_intersecting_visible_portals(float absrange_begin, float absrange_end) {
    std::vector<Portal> res;

    for (auto portal : portals) {
        if (portal.is_visible()) {
            if (portal.src_offset_y >= absrange_begin && portal.src_offset_y <= absrange_end) {
                res.push_back(portal);
            }
        }
    }

    return res;
}


int Document::find_reference_page_with_reference_text(std::wstring ref) {
    QStringList parts = QString::fromStdWString(ref).split(QRegularExpression("[ \w\(\);,]"));
    QString largest_part = "";
    for (int i = 0; i < parts.size(); i++) {
        if (parts.at(i).size() > largest_part.size() ) {
            largest_part = parts.at(i);
        }
    }


    std::wstring query = largest_part.toStdWString();
    auto searcher = std::default_searcher(query.begin(), query.end(), pred_case_sensitive);

    std::vector<int> found_indices;

    auto it = std::search(
        super_fast_search_index.begin(),
        super_fast_search_index.end(),
        searcher);

    for (; it != super_fast_search_index.end(); it = std::search(it + 1, super_fast_search_index.end(), searcher)) {
        int index = it - super_fast_search_index.begin();
        found_indices.push_back(index);
    }

    std::vector<int> filtered_indices;

    for (auto index : found_indices) {
        int context_first = std::max(index - 100, 0);
        int context_last = std::min(index + 100, static_cast<int>(super_fast_search_index.size()-1));
        bool found_all = true;

        for (int i = 0; i < parts.size(); i++) {
            if (parts[i].size() < 4 && parts[i].startsWith("et")) {
                continue;
            }
            if (parts[i].size() < 5 && parts[i].startsWith("al")) {
                continue;
            }

            if (parts[i].size() > 2) {
                std::wstring subquery = parts[i].toStdWString();
                auto subsearcher = std::default_searcher(subquery.begin(), subquery.end(), pred_case_sensitive);
                auto last = super_fast_search_index.begin() + context_last + 1;
                if (std::search(super_fast_search_index.begin() + context_first, last, subsearcher) == last) {
                    found_all = false;
                    break;
                }
            }
        }
        if (found_all) {
            filtered_indices.push_back(index);
        }
    }
    if (filtered_indices.size() > 0) {
        return super_fast_search_index_pages[filtered_indices.back()];
        //std::wstring filtered_context = super_fast_search_index.substr(filtered_indices.back(), 200);
        //int a = 2;
    }
    return -1;

}
