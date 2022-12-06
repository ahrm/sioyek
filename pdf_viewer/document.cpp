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

#include <mupdf/pdf.h>

#include "checksum.h"

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
	}
	else {
		auto checksum_thread = std::thread([&]() {
				std::string checksum = get_checksum();
				db_manager->insert_document_hash(get_path(), checksum);
			});
		checksum_thread.detach();
		//checksum_thread.join();
	}
}


void Document::add_bookmark(const std::wstring& desc, float y_offset) {
	BookMark bookmark{ y_offset, desc };
	bookmarks.push_back(bookmark);
	db_manager->insert_bookmark(get_checksum(), desc, y_offset);
}

void Document::fill_highlight_rects(fz_context* ctx, fz_document* doc_) {
	// Fill the `highlight_rects` attribute of all highlights with a vector of merged highlight rects of
	// individual characters in the highlighted area (we merge the rects of characters in a single line
	// which makes the result look nicer)

	for (size_t i = 0; i < highlights.size(); i++) {
		const Highlight highlight = highlights[i];
		std::vector<fz_rect> highlight_rects;
		std::vector<fz_rect> merged_rects;
		std::wstring highlight_text;
		get_text_selection(ctx, highlight.selection_begin, highlight.selection_end, true, highlight_rects, highlight_text, doc_);
		merge_selected_character_rects(highlight_rects, merged_rects);

		if (i < highlights.size()) {
			highlights[i].highlight_rects = std::move(merged_rects);
		}
		else {
			break;
		}
	}
}


void Document::add_highlight(const std::wstring& desc,
	const std::vector<fz_rect>& highlight_rects,
	AbsoluteDocumentPos selection_begin,
	AbsoluteDocumentPos selection_end,
	char type)
{
	if (type > 'z' || type < 'a') {
		type = 'a';
	}

	Highlight highlight;
	highlight.description = desc;
	highlight.selection_begin = selection_begin;
	highlight.selection_end = selection_end;
	highlight.type = type;
	highlight.highlight_rects = highlight_rects;

	highlights.push_back(highlight);
	db_manager->insert_highlight(
		get_checksum(),
		desc,
		selection_begin.x,
		selection_begin.y,
		selection_end.x,
		selection_end.y,
		highlight.type);
}

bool Document::get_is_indexing() {
	return is_indexing;
}

void Document::add_portal(Portal portal, bool insert_into_database) {
	portals.push_back(portal);
	if (insert_into_database) {
		db_manager->insert_portal(
			get_checksum(),
			portal.dst.document_checksum,
			portal.dst.book_state.offset_x,
			portal.dst.book_state.offset_y,
			portal.dst.book_state.zoom_level,
			portal.src_offset_y);
	}
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
		return abs(bm.y_offset - to_offset_y);
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
		db_manager->delete_bookmark( get_checksum(), bookmarks[closest_index].y_offset);
		bookmarks.erase(bookmarks.begin() + closest_index);
	}
}

void Document::delete_highlight_with_index(int index) {
	Highlight highlight_to_delete = highlights[index];

	db_manager->delete_highlight(
		get_checksum(),
		highlight_to_delete.selection_begin.x,
		highlight_to_delete.selection_begin.y,
		highlight_to_delete.selection_end.x,
		highlight_to_delete.selection_end.y);
	highlights.erase(highlights.begin() + index);
}

void Document::delete_highlight(Highlight hl) {
	for (size_t i = (highlights.size()-1); i >= 0; i--) {
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
		db_manager->delete_link( get_checksum(), portals[closest_index].src_offset_y);
		portals.erase(portals.begin() + closest_index);
	}
}

const std::vector<BookMark>& Document::get_bookmarks() const {
	return bookmarks;
}

std::vector<BookMark> Document::get_sorted_bookmarks() const {
	std::vector<BookMark> res = bookmarks;
	std::sort(res.begin(), res.end(), [](const BookMark& lhs, const BookMark& rhs) {return lhs.y_offset < rhs.y_offset; });
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
		marks.push_back({ y_offset, symbol });
		db_manager->insert_mark( get_checksum(), symbol, y_offset);
	}
	else {
		marks[current_mark_index].y_offset = y_offset;
		db_manager->update_mark( get_checksum(), symbol, y_offset);
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
	doc(nullptr){
	last_update_time = QDateTime::currentDateTime();
}

void Document::count_chapter_pages(std::vector<int> &page_counts) {
	int num_chapters = fz_count_chapters(context, doc);

	for (int i = 0; i < num_chapters; i++) {
		int num_pages = fz_count_chapter_pages(context, doc, i);
		page_counts.push_back(num_pages);
	}
}

void Document::count_chapter_pages_accum(std::vector<int> &accum_page_counts) {
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
	int end_index = histogram.size()-1;

	// find the first index in both directions where the histogram is nonzero
	while ((start_index < end_index) && (histogram[start_index++] < nonzero_threshold));
	while ((start_index < end_index) && (histogram[end_index--] < nonzero_threshold));


	int border = 10;
	start_index = std::max<int>(start_index - border, 0);
	end_index = std::min<int>(end_index + border, histogram.size()-1);

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

fz_link* Document::get_page_links(int page_number) {
	if (cached_page_links.find(page_number) != cached_page_links.end()) {
		return cached_page_links.at(page_number);
	}
	//std::cerr << "getting links .... for " << page_number << std::endl;

	fz_link* res = nullptr;
	fz_try(context) {
		fz_page* page = fz_load_page(context, doc, page_number);
		res = fz_load_links(context, page);
		cached_page_links[page_number] = res;
		fz_drop_page(context, page);
	}

	fz_catch(context) {
		std::cerr << "Error: Could not load links" << std::endl;
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
			std::cerr << "Error: could not drop documnet" << std::endl;
		}
	}
}
void Document::reload(std::string password) {
	fz_drop_document(context, doc);
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
	clear_toc_nodes();

	doc = nullptr;

	open(invalid_flag_pointer, false, password);
}

bool Document::open(bool* invalid_flag, bool force_load_dimensions, std::string password, bool temp) {
	last_update_time = QDateTime::currentDateTime();
	if (doc == nullptr) {
		fz_try(context) {
			doc = fz_open_document(context, utf8_encode(file_name).c_str());
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
			std::wcerr << "could not open " << file_name << std::endl;
		}
		if ((doc != nullptr) && (!temp)) {
			//load_document_metadata_from_db();
			load_page_dimensions(force_load_dimensions);
			create_toc_tree(top_level_toc_nodes);
			get_flat_toc(top_level_toc_nodes, flat_toc_names, flat_toc_pages);
			invalid_flag_pointer = invalid_flag;

			// we don't need to index figures in helper documents
			index_document(invalid_flag);
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

		// clone the main context for use in the background thread
		fz_context* context_ = fz_clone_context(context);
		fz_try(context_) {
			fz_document* doc_ = fz_open_document(context_, utf8_encode(file_name).c_str());
			//fz_layout_document(context_, doc, 600, 800, 20);
			load_document_metadata_from_db();

			float acc_height_ = 0.0f;
			for (int i = 0; i < n; i++) {
				fz_page* page = fz_load_page(context_, doc_, i);
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
}


fz_rect Document::get_page_absolute_rect(int page) {
	std::lock_guard guard(page_dims_mutex);

	fz_rect res = {0, 0, 1, 1};

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
		std::cerr << "could not count pages" << std::endl;
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
	int page_number = -1;
	DocumentPos bottom_left = absolute_to_page_pos({ absolute_rect.x0, absolute_rect.y0 });
	DocumentPos top_right = absolute_to_page_pos({ absolute_rect.x1, absolute_rect.y1 });
	if (page != nullptr) {
		*page = page_number;
	}
	fz_rect res;
	res.x0 = bottom_left.x;
	res.x1 = top_right.x;
	res.y0 = bottom_left.y;
	res.y1 = top_right.y;
	return res;
}

DocumentPos Document::absolute_to_page_pos(AbsoluteDocumentPos absp){

	std::lock_guard guard(page_dims_mutex);
	if (accum_page_heights.size() == 0) {
		return {0, 0.0f, 0.0f};
	}

	int i = (std::lower_bound(
		accum_page_heights.begin(),
		accum_page_heights.end(), absp.y) -  accum_page_heights.begin()) - 1;
	i = std::max(0, i);

	if (i < accum_page_heights.size()) {
		float acc_page_heights_i = accum_page_heights[i];
		float page_width = page_widths[i];
		float remaining_y = absp.y - acc_page_heights_i;

		return {i, page_width / 2 + absp.x, remaining_y};
	}
	else {
		return {-1, 0.0f, 0.0f};
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

			fz_document* doc_ = fz_open_document(context_, utf8_encode(file_name).c_str());

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



std::optional<IndexedData> Document::find_reference_with_string(std::wstring reference_name) {
	if (reference_indices.find(reference_name) != reference_indices.end()) {
		return reference_indices[reference_name];
	}

	return {};
}

std::optional<IndexedData> Document::find_equation_with_string(std::wstring equation_name, int page_number) {
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
		return res;
	}

	return {};
}



std::optional<std::wstring> Document::get_equation_text_at_position(const std::vector<fz_stext_char*>& flat_chars, float offset_x, float offset_y) {


	std::wregex regex(L"\\([0-9]+(\\.[0-9]+)*\\)");
	std::optional<std::wstring> match = get_regex_match_at_position(regex, flat_chars, offset_x, offset_y);

	if (match) {
		return match.value().substr(1, match.value().size() - 2);
	}
	else {
		return {};
	}
}

std::optional<std::wstring> Document::get_regex_match_at_position(const std::wregex& regex, const std::vector<fz_stext_char*>& flat_chars, float offset_x, float offset_y){
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
				DocumentPos pos {generic_indices[i].page, 0, generic_indices[i].y_offset};
				pos_scores.push_back(std::make_pair(score, pos));
				//if (score > best_score) {
				//	best_page = generic_indices[i].page;
				//	best_y_offset = generic_indices[i].y_offset;
				//	best_score = score;
				//}
			}

		}
	}

	auto  by_score = [](std::pair<int, DocumentPos> const & a, std::pair<int, DocumentPos> const & b)  {
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

std::optional<std::pair<std::wstring, std::wstring>> Document::get_generic_link_name_at_position(const std::vector<fz_stext_char*>& flat_chars, float offset_x, float offset_y) {
	std::wregex regex(L"[a-zA-Z]{3,}(\.){0,1}[ \t]+[0-9]+(\.[0-9]+)*");
	std::optional<std::wstring> match_string = get_regex_match_at_position(regex, flat_chars, offset_x, offset_y);
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

std::optional<std::wstring> Document::get_reference_text_at_position(const std::vector<fz_stext_char*>& flat_chars, float offset_x, float offset_y) {
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

	for (auto ch : flat_chars) {
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
			return selected_text;
		}
		if (started && (!reached) && (ch->c == end_char)) {
			started = false;
			selected_text.clear();
		}

		if (started && (!done)) {
			if (ch->c != ' ') {
				selected_text.push_back(ch->c);
			}
		}

		if ((started) && (!reached) && (ch->c == delim)) {
			selected_text.clear();
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
	std::vector<fz_rect>& selected_characters,
	std::wstring& selected_text) {
	get_text_selection(context, selection_begin, selection_end, is_word_selection, selected_characters, selected_text);
}
void Document::get_text_selection(fz_context* ctx, AbsoluteDocumentPos selection_begin,
	AbsoluteDocumentPos selection_end,
	bool is_word_selection,
	std::vector<fz_rect>& selected_characters,
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
	fz_output* output_file = fz_new_output_with_path(context, new_file_path_utf8.c_str(), 0);

	pdf_document* pdf_doc = pdf_specifics(context, doc);
	const std::vector<Highlight>& doc_highlights = get_highlights();
	const std::vector<BookMark>& doc_bookmarks = get_bookmarks();

	for (auto highlight : doc_highlights) {
		int page_number = get_offset_page_number(highlight.selection_begin.y);

		std::vector<fz_rect> selected_characters;
		std::vector<fz_rect> merged_characters;
		std::vector<fz_rect> selected_characters_page_rects;
		std::wstring selected_text;

		get_text_selection(highlight.selection_begin, highlight.selection_end, true, selected_characters, selected_text);
		merge_selected_character_rects(selected_characters, merged_characters);

		for (auto absrect : merged_characters) {
			selected_characters_page_rects.push_back(absolute_to_page_rect(absrect, nullptr));
		}
		//absolute_to_page_pos
		std::vector<fz_quad> selected_character_quads = quads_from_rects(selected_characters_page_rects);

		fz_page* page = load_cached_page(page_number);
		pdf_page* pdf_page = pdf_page_from_fz_page(context, page);
		pdf_annot* highlight_annot = pdf_create_annot(context, pdf_page, PDF_ANNOT_HIGHLIGHT);
		float color[] = { 1.0f, 0.0f, 0.0f };
		color[0] = HIGHLIGHT_COLORS[(highlight.type - 'a') * 3 + 0];
		color[1] = HIGHLIGHT_COLORS[(highlight.type - 'a') * 3 + 1];
		color[2] = HIGHLIGHT_COLORS[(highlight.type - 'a') * 3 + 2];

		pdf_set_annot_color(context, highlight_annot, 3, color);
		pdf_set_annot_quad_points(context, highlight_annot, selected_character_quads.size(), &selected_character_quads[0]);
		pdf_update_annot(context, highlight_annot);

		created_annotations.push_back(std::make_pair(pdf_page, highlight_annot));

	}

	for (auto bookmark : doc_bookmarks) {
		auto [page_number, doc_x, doc_y] = absolute_to_page_pos({ 0, bookmark.y_offset });

		fz_page* page = load_cached_page(page_number);
		pdf_page* pdf_page = pdf_page_from_fz_page(context, page);
		pdf_annot* bookmark_annot = pdf_create_annot(context, pdf_page, PDF_ANNOT_TEXT);
		std::string encoded_bookmark_text = utf8_encode(bookmark.description);

		fz_rect annot_rect;
		annot_rect.x0 = 10;
		annot_rect.x1 = 20;
		annot_rect.y0 = doc_y;
		annot_rect.y1 = doc_y + 10;

		pdf_set_annot_rect(context, bookmark_annot, annot_rect);
		pdf_set_annot_contents(context, bookmark_annot, encoded_bookmark_text.c_str());
		pdf_update_annot(context, bookmark_annot);

		created_annotations.push_back(std::make_pair(pdf_page, bookmark_annot));
	}

	pdf_write_options pwo {};
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

std::optional<std::wstring> Document::get_reference_text_at_position(int page, float offset_x, float offset_y) {
	fz_stext_page* stext_page = get_stext_with_page_number(page);
	std::vector<fz_stext_char*> flat_chars;
	get_flat_chars_from_stext_page(stext_page, flat_chars);
	return get_reference_text_at_position(flat_chars, offset_x, offset_y);
}

std::optional<std::wstring> Document::get_equation_text_at_position(int page, float offset_x, float offset_y) {
	fz_stext_page* stext_page = get_stext_with_page_number(page);
	std::vector<fz_stext_char*> flat_chars;
	get_flat_chars_from_stext_page(stext_page, flat_chars);
	return get_equation_text_at_position(flat_chars, offset_x, offset_y);
}

std::optional<std::pair<std::wstring, std::wstring>>  Document::get_generic_link_name_at_position(int page, float offset_x, float offset_y) {
	fz_stext_page* stext_page = get_stext_with_page_number(page);
	std::vector<fz_stext_char*> flat_chars;
	get_flat_chars_from_stext_page(stext_page, flat_chars);
	return get_generic_link_name_at_position(flat_chars, offset_x, offset_y);
}

std::optional<std::wstring> Document::get_regex_match_at_position(const std::wregex& regex, int page, float offset_x, float offset_y) {
	fz_stext_page* stext_page = get_stext_with_page_number(page);
	std::vector<fz_stext_char*> flat_chars;
	get_flat_chars_from_stext_page(stext_page, flat_chars);
	return get_regex_match_at_position(regex, flat_chars, offset_x, offset_y);
}

std::vector<std::vector<fz_rect>> Document::get_page_flat_word_chars(int page){
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
	if ((size_t) (index+offset) < sorted_highlights.size()) {
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
	if ((index+offset) >= 0) {
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
	fz_rect res {};
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

std::optional<PdfLink> Document::get_link_in_page_rect(int page, fz_rect rect) {
	if (!doc) return {};

	//rect.x0 += page_widths[page] / 2;
	//rect.x1 += page_widths[page] / 2;
	int rect_page;
	fz_rect doc_rect = absolute_to_page_rect(rect, &rect_page);

	if (page != -1) {
		fz_link* portals = get_page_links(page);
		std::optional<PdfLink> res = {};
		while (portals != nullptr) {
			if (rects_intersect(doc_rect, portals->rect))
			{
				res = { portals->rect, portals->uri };
				return res;
			}
			portals = portals->next;
		}
	}

	return {};
}

std::optional<PdfLink> Document::get_link_in_pos(int page, float doc_x, float doc_y){
	if (!doc) return {};

	if (page != -1) {
		fz_link* portals = get_page_links(page);
		fz_point point = { doc_x, doc_y };
		std::optional<PdfLink> res = {};
		while (portals != nullptr) {
			if (fz_is_point_inside_rect(point, portals->rect)) {
				res = { portals->rect, portals->uri };
				return res;
			}
			portals = portals->next;
		}

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
	AbsoluteDocumentPos res = {doc_pos.x, absolute_y};
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
	if ((new_index >= 0) && ((size_t) new_index < line_rects.size())) {
		*out_page = page;
		*out_index = new_index;
		return line_rects[new_index];
	}
	else {
		if (!cont) {
                        if (line_index > 0 && (size_t) line_index < line_rects.size()) {
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

const std::vector<fz_rect>& Document::get_page_lines(int page, std::vector<std::wstring>* out_line_texts) {


	if (cached_page_line_rects.find(page) != cached_page_line_rects.end()) {
		if (out_line_texts != nullptr) {
			*out_line_texts = cached_line_texts[page];
		}
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
			merge_lines(flat_lines, line_rects, line_texts);
			for (size_t i = 0; i < line_rects.size(); i++) {
				line_rects[i].x0 = line_rects[i].x0 - page_widths[page] / 2;
				line_rects[i].x1 = line_rects[i].x1 - page_widths[page] / 2;
				line_rects[i].y0 = document_to_absolute_y(page, line_rects[i].y0);
				line_rects[i].y1 = document_to_absolute_y(page, line_rects[i].y1);
			}

			std::vector<fz_rect> line_rects_;
			std::vector<std::wstring> line_texts_;

			for (int i = 0; i < line_rects.size(); i++) {
				if (fz_contains_rect(bound, line_rects[i])) {
					line_rects_.push_back(line_rects[i]);
					line_texts_.push_back(line_texts[i]);
				}
			}


			cached_page_line_rects[page] = line_rects_;
			cached_line_texts[page] = line_texts_;

			if (out_line_texts != nullptr) {
				*out_line_texts = line_texts_;
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

bool pred_case_sensitive(const wchar_t& c1, const wchar_t& c2) {
	return c1 == c2;
}

bool pred_case_insensitive(const wchar_t& c1, const wchar_t& c2) {
	return std::tolower(c1) == std::tolower(c2);
}

std::vector<SearchResult> Document::search_text(std::wstring query, bool case_sensitive, int begin_page, int min_page, int max_page) {
	std::vector<SearchResult> output;

	std::vector<SearchResult> before_results;
	bool is_before = true;

        auto pred = case_sensitive ? pred_case_sensitive : pred_case_insensitive;
        auto searcher = std::default_searcher(query.begin(), query.end(), pred);
        auto it = std::search(
		super_fast_search_index.begin(),
		super_fast_search_index.end(),
                searcher);

	for ( ; it != super_fast_search_index.end(); it = std::search(it+1, super_fast_search_index.end(), searcher)) {
		int start_index = it - super_fast_search_index.begin();
		std::vector<fz_rect> match_rects;
		std::vector<fz_rect> compressed_match_rects;

		int match_page = super_fast_search_index_pages[start_index];

		if (match_page >= begin_page) {
			is_before = false;
		}

		int end_index = start_index + query.size();


		for (int j = start_index; j < end_index; j++) {
			fz_rect rect = super_fast_search_rects[j];
			match_rects.push_back(rect);
		}

		merge_selected_character_rects(match_rects, compressed_match_rects);
		SearchResult res { compressed_match_rects, match_page };

		if (!((match_page < min_page) || (match_page > max_page))) {
			if (is_before) {
				before_results.push_back(res);
			}
			else {
				output.push_back(res);
			}
		}
	}
	output.insert(output.end(), before_results.begin(), before_results.end());
	return output;
}

std::vector<SearchResult> Document::search_regex(std::wstring query, bool case_sensitive, int begin_page, int min_page, int max_page)
{
	std::vector<SearchResult> output;

	std::wregex regex;
	try {
		if (case_sensitive) {
			regex = std::wregex(query);
		} else {
			regex = std::wregex(query, std::regex_constants::icase);
		}
	}
	catch (const std::regex_error&) {
		return output;
	}


	std::vector<SearchResult> before_results;
	bool is_before = true;

	int offset = 0;

	std::wstring::const_iterator search_start(super_fast_search_index.begin());
	std::wsmatch match;
	int empty_tolerance = 1000;


	while (std::regex_search(search_start, super_fast_search_index.cend(), match, regex)) {
		std::vector<fz_rect> match_rects;
		std::vector<fz_rect> compressed_match_rects;

		int match_page = super_fast_search_index_pages[offset + match.position()];

		if (match_page >= begin_page) {
			is_before = false;
		}

		int start_index = offset + match.position();
		int end_index = offset + match.position() + match.length();
		if (start_index < end_index) {


			for (int j = start_index; j < end_index; j++) {
				fz_rect rect = super_fast_search_rects[j];
				match_rects.push_back(rect);
			}

			merge_selected_character_rects(match_rects, compressed_match_rects);
			SearchResult res { compressed_match_rects, match_page };

			if (!((match_page < min_page) || (match_page > max_page))) {
				if (is_before) {
					before_results.push_back(res);
				}
				else {
					output.push_back(res);
				}
			}
		}
		else {
			empty_tolerance--;
			if (empty_tolerance == 0) {
				break;
			}
		}

		offset = end_index;
		search_start = match.suffix().first;
	}
	output.insert(output.end(), before_results.begin(), before_results.end());
	return output;

}

float Document::max_y_offset() {
	int np = num_pages();

	return get_accum_page_height(np - 1) + get_page_height(np - 1);

}
