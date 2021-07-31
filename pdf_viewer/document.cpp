#include "document.h"
#include <algorithm>
#include <thread>
#include "utf8.h"
#include <qfileinfo.h>
#include <qdatetime.h>
#include <map>
#include <regex>

int Document::get_mark_index(char symbol) {
	for (int i = 0; i < marks.size(); i++) {
		if (marks[i].symbol == symbol) {
			return i;
		}
	}
	return -1;
}

void Document::load_document_metadata_from_db() {
	marks.clear();
	bookmarks.clear();
	select_mark(db, file_name, marks);
	select_bookmark(db, file_name, bookmarks);
	select_links(db, file_name, links);
}


void Document::add_bookmark(const std::wstring& desc, float y_offset) {
	bookmarks.push_back({y_offset, desc});
	insert_bookmark(db, file_name, desc, y_offset);
}

bool Document::get_is_indexing()
{
	return is_indexing;
}

void Document::add_link(Link link, bool insert_into_database) {
	links.push_back(link);
	if (insert_into_database) {
		insert_link(db,
			get_path(),
			link.dst.document_path,
			link.dst.book_state.offset_x,
			link.dst.book_state.offset_y,
			link.dst.book_state.zoom_level,
			link.src_offset_y);
	}
}

std::wstring Document::get_path() {

	return file_name;
}

int Document::find_closest_bookmark_index(float to_offset_y) {

	int min_index = argminf<BookMark>(bookmarks, [to_offset_y](BookMark bm) {
		return abs(bm.y_offset - to_offset_y);
		});

	return min_index;
}

void Document::delete_closest_bookmark(float to_y_offset) {
	int closest_index = find_closest_bookmark_index(to_y_offset);
	if (closest_index > -1) {
		delete_bookmark(db, get_path(), bookmarks[closest_index].y_offset);
		bookmarks.erase(bookmarks.begin() + closest_index);
	}
}

std::optional<Link> Document::find_closest_link(float to_offset_y, int* index) {
	int min_index = argminf<Link>(links, [to_offset_y](Link l) {
		return abs(l.src_offset_y - to_offset_y);
		});

	if (min_index >= 0) {
		if (index) *index = min_index;
		return links[min_index];
	}
	return {};
}

bool Document::update_link(Link new_link)
{
	for (auto& link : links) {
		if (link.src_offset_y == new_link.src_offset_y) {
			link.dst.book_state.offset_x = new_link.dst.book_state.offset_x;
			link.dst.book_state.offset_y = new_link.dst.book_state.offset_y;
			link.dst.book_state.zoom_level = new_link.dst.book_state.zoom_level;
			return true;
		}
	}
	return false;
}

void Document::delete_closest_link(float to_offset_y) {
	int closest_index = -1;
	if (find_closest_link(to_offset_y, &closest_index)) {
		delete_link(db, get_path(), links[closest_index].src_offset_y);
		links.erase(links.begin() + closest_index);
	}
}

const std::vector<BookMark>& Document::get_bookmarks() const {
	return bookmarks;
}

void Document::add_mark(char symbol, float y_offset) {
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

bool Document::remove_mark(char symbol)
{
	for (int i = 0; i < marks.size(); i++) {
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

Document::Document(fz_context* context, std::wstring file_name, sqlite3* db) : context(context), file_name(file_name), doc(nullptr), db(db) {
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

	for (int i = 0; i < raw_page_count.size(); i++) {
		accum_page_counts.push_back(accum);
		accum += raw_page_count[i];
	}

}

const std::vector<TocNode*>& Document::get_toc() {
	return top_level_toc_nodes;
}

bool Document::has_toc()
{
	return top_level_toc_nodes.size() > 0;
}

const std::vector<std::wstring>& Document::get_flat_toc_names()
{
	return flat_toc_names;
}

const std::vector<int>& Document::get_flat_toc_pages()
{
	return flat_toc_pages;
}

float Document::get_page_height(int page_index)
{
	std::lock_guard guard(page_dims_mutex);
	return page_heights[page_index];
}

float Document::get_page_width(int page_index)
{
	return page_widths[page_index];
}

float Document::get_page_width_smart(int page_index, float* left_ratio, float* right_ratio, int* normal_page_width)
{
	//fz_pixmap* rendered_pixmap = fz_new_pixmap_from_page_number(mupdf_context, doc, req.page, transform_matrix, fz_device_rgb(mupdf_context), 0);
	//fz_matrix ctm = fz_scale(0.3, 0.3);
	fz_matrix ctm = fz_identity;

	fz_pixmap* pixmap = fz_new_pixmap_from_page_number(context, doc, page_index, ctm, fz_device_rgb(context), 0);

	std::vector<float> vertical_histogram(pixmap->w);

	//float brightness_sum = 0.0f;
	//for (int i = 0; i < pixmap->w; i++) {
	//	for (int j = 0; j < pixmap->h; j++) {
	//		int index = 3 * pixmap->w * j + 3 * i;

	//		int r = pixmap->samples[index];
	//		int g = pixmap->samples[index + 1];
	//		int b = pixmap->samples[index + 2];

	//		float brightness = static_cast<float>(r + g + b) / 3.0f;
	//		vertical_histogram[i] += brightness;
	//		brightness_sum += brightness;
	//	}
	//}

	float total_nonzero = 0.0f;
	for (int i = 0; i < pixmap->w; i++) {
		for (int j = 0; j < pixmap->h; j++) {
			int index = 3 * pixmap->w * j + 3 * i;

			int r = pixmap->samples[index];
			int g = pixmap->samples[index + 1];
			int b = pixmap->samples[index + 2];

			float brightness = static_cast<float>(r + g + b) / 3.0f;
			if (brightness < 250) {
				vertical_histogram[i] += 1;
				total_nonzero += 1;
			}
		}
	}

	float average_nonzero = total_nonzero / vertical_histogram.size();
	float nonzero_threshold = average_nonzero / 3.0f;

	int start_index = 0;
	int end_index = vertical_histogram.size()-1;

	while ((start_index < end_index) && (vertical_histogram[start_index++] < nonzero_threshold));
	while ((start_index < end_index) && (vertical_histogram[end_index--] < nonzero_threshold));


	int border = 10;
	start_index = std::max<int>(start_index - border, 0);
	end_index = std::min<int>(end_index + border, vertical_histogram.size()-1);

	fz_drop_pixmap(context, pixmap);

	*left_ratio = static_cast<float>(start_index) / vertical_histogram.size();
	*right_ratio = static_cast<float>(end_index) / vertical_histogram.size();

	float standard_width = page_widths[page_index];
	float ratio = static_cast<float>(end_index - start_index) / vertical_histogram.size();

	*normal_page_width = standard_width;

	return ratio * standard_width;
}

float Document::get_accum_page_height(int page_index)
{
	std::lock_guard guard(page_dims_mutex);
	if (page_index < 0) {
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
		std::cerr << "Error: Could not load outline ... " << std::endl;
	}
	return res;
}

void Document::create_toc_tree(std::vector<TocNode*>& toc) {
	fz_try(context) {
		fz_outline* outline = get_toc_outline();
		convert_toc_tree(outline, toc);
		fz_drop_outline(context, outline);
	}
	fz_catch(context) {
		std::cerr << "Error: Could not load outline ... " << std::endl;
	}
}

void Document::convert_toc_tree(fz_outline* root, std::vector<TocNode*>& output) {
	// convert an fz_outline structure to a tree of TocNodes

	std::vector<int> accum_chapter_pages;
	count_chapter_pages_accum(accum_chapter_pages);

	do {
		if (root == nullptr) {
			break;
		}

		TocNode* current_node = new TocNode;
		current_node->title = utf8_decode(root->title);
		current_node->x = root->x;
		current_node->y = root->y;
		if (root->page == -1) {
			float xp, yp;
			fz_location loc = fz_resolve_link(context, doc, root->uri, &xp, &yp);
			int chapter_page = accum_chapter_pages[loc.chapter];
			current_node->page = chapter_page + loc.page;
		}
		else {
			current_node->page = root->page;
		}
		convert_toc_tree(root->down, current_node->children);


		output.push_back(current_node);
	} while (root = root->next);
}

fz_link* Document::get_page_links(int page_number) {
	if (cached_page_links.find(page_number) != cached_page_links.end()) {
		return cached_page_links.at(page_number);
	}
	std::cerr << "getting links .... for " << page_number << std::endl;

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
	if (figure_indexing_thread.has_value()) {
		stop_indexing();
		figure_indexing_thread.value().join();
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
	//this->figure_indexing_thread.join();
}
void Document::reload() {
	fz_drop_document(context, doc);
	cached_num_pages = {};

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

	doc = nullptr;

	open(invalid_flag_pointer);
}

bool Document::open(bool* invalid_flag, bool force_load_dimensions) {
	last_update_time = QDateTime::currentDateTime();
	if (doc == nullptr) {
		fz_try(context) {
			doc = fz_open_document(context, utf8_encode(file_name).c_str());
			//fz_layout_document(context, doc, 600, 800, 9);
		}
		fz_catch(context) {
			std::wcerr << "could not open " << file_name << std::endl;
		}
		if (doc != nullptr) {
			load_page_dimensions(force_load_dimensions);
			load_document_metadata_from_db();
			create_toc_tree(top_level_toc_nodes);
			get_flat_toc(top_level_toc_nodes, flat_toc_names, flat_toc_pages);
			invalid_flag_pointer = invalid_flag;

			// we don't need to index figures in helper documents
			index_figures(invalid_flag);
			return true;
		}


		return false;
	}
	else {
		std::cerr << "warning! calling open() on an open document" << std::endl;
		return true;
	}
}


//const vector<float>& get_page_heights();
//const vector<float>& get_page_widths();
//const vector<float>& get_accum_page_heights();

void Document::get_visible_pages(float doc_y_range_begin, float doc_y_range_end, std::vector<int>& visible_pages) {

	std::lock_guard guard(page_dims_mutex);
	float page_begin = 0.0f;

	for (int i = 0; i < page_heights.size(); i++) {
		float page_end = page_begin + page_heights[i];

		if (intersects(doc_y_range_begin, doc_y_range_end, page_begin, page_end)) {
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
		fz_page* page = fz_load_page(context, doc, n/2);
		fz_rect bounds = fz_bound_page(context, page);
		fz_drop_page(context, page);
		for (int i = 0; i < n; i++) {
			int height = bounds.y1 - bounds.y0;
			int width = bounds.x1 - bounds.x0;

			page_heights.push_back(height);
			accum_page_heights.push_back(acc_height);
			acc_height += height;
			page_widths.push_back(width);
		}
	}

	auto load_page_dimensions_function = [this, n]() {
		std::vector<float> accum_page_heights_;
		std::vector<float> page_heights_;
		std::vector<float> page_widths_;

		// clone the main context for use in the background thread
		fz_context* context_ = fz_clone_context(context);
		fz_document* doc_ = fz_open_document(context_, utf8_encode(file_name).c_str());
		//fz_layout_document(context_, doc, 600, 800, 20);

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

		fz_drop_document(context_, doc_);
		fz_drop_context(context_);

		page_dims_mutex.lock();

		page_heights = std::move(page_heights_);
		accum_page_heights = std::move(accum_page_heights_);
		page_widths = std::move(page_widths_);

		if (invalid_flag_pointer) {
			*invalid_flag_pointer = true;
		}

		//are_dimensions_correct = true;

		page_dims_mutex.unlock();
	};

	if (force_load_now) {
		load_page_dimensions_function();
	}
	else {
		auto background_page_dimensions_loading_thread = std::thread(load_page_dimensions_function);
		background_page_dimensions_loading_thread.detach();
	}
}


fz_rect Document::get_page_absolute_rect(int page)
{
	std::lock_guard guard(page_dims_mutex);

	fz_rect res;
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

DocumentManager::DocumentManager(fz_context* mupdf_context, sqlite3* database) : mupdf_context(mupdf_context), database(database)
{
}


Document* DocumentManager::get_document(std::filesystem::path path) {
	if (cached_documents.find(path.wstring()) != cached_documents.end()) {
		return cached_documents.at(path.wstring());
	}
	Document* new_doc = new Document(mupdf_context, path.wstring(), database);
	cached_documents[path.wstring()] = new_doc;
	return new_doc;
}

const std::unordered_map<std::wstring, Document*>& DocumentManager::get_cached_documents()
{
	return cached_documents;
}

void DocumentManager::delete_global_mark(char symbol)
{
	for (auto [path, doc] : cached_documents) {
		doc->remove_mark(symbol);
	}
}


fz_stext_page* Document::get_stext_with_page_number(int page_number)
{
	const int MAX_CACHED_STEXT_PAGES = 10;

	for (auto [page, cached_stext_page] : cached_stext_pages) {
		if (page == page_number) {
			return cached_stext_page;
		}
	}

	fz_stext_page* stext_page = fz_new_stext_page_from_page_number(context, doc, page_number, nullptr);

	if (stext_page != nullptr) {

		if (cached_stext_pages.size() == MAX_CACHED_STEXT_PAGES) {
			fz_drop_stext_page(context, cached_stext_pages[0].second);
			cached_stext_pages.erase(cached_stext_pages.begin());
		}
		cached_stext_pages.push_back(std::make_pair(page_number, stext_page));
		return stext_page;
	}

	return nullptr;
}

void Document::absolute_to_page_pos(float absolute_x, float absolute_y, float* doc_x, float* doc_y, int* doc_page) {

	std::lock_guard guard(page_dims_mutex);

	int i = (std::lower_bound(
		accum_page_heights.begin(),
		accum_page_heights.end(), absolute_y) -  accum_page_heights.begin()) - 1;
	i = std::max(0, i);

	float remaining_y = absolute_y - accum_page_heights[i];
	float page_width = page_widths[i];

	*doc_x = page_width / 2 + absolute_x;
	*doc_y = remaining_y;
	*doc_page = i;
}

QStandardItemModel* Document::get_toc_model()
{
	if (!cached_toc_model) {
		cached_toc_model = get_model_from_toc(get_toc());
	}
	return cached_toc_model;
}

//void Document::absolute_to_page_rects(fz_rect absolute_rect, vector<fz_rect>& resulting_rects, vector<int>& resulting_pages)
//{
//	fz_rect res;
//	int page_begin, page_end;
//	absolute_to_page_pos(absolute_rect.x0, absolute_rect.y0, &res.x0, &res.y0, &page_begin);
//	absolute_to_page_pos(absolute_rect.x1, absolute_rect.y1, &res.x1, &res.y1, &page_end);
//
//	for (int i = page_begin; i <= page_end; i++) {
//		fz_rect page_absolute_rect = get_page_absolute_rect(i);
//		fz_rect intersection = fz_intersect_rect(absolute_rect, page_absolute_rect);
//		intersection.y0 -= page_absolute_rect.y0;
//		intersection.y1 -= page_absolute_rect.y0;
//
//		resulting_rects.push_back(intersection);
//		resulting_pages.push_back(i);
//	}
//}

void Document::page_pos_to_absolute_pos(int page, float page_x, float page_y, float* abs_x, float* abs_y) {
	std::lock_guard guard(page_dims_mutex);
	*abs_x = page_x - page_widths[page] / 2;
	*abs_y = page_y + accum_page_heights[page];
}

fz_rect Document::page_rect_to_absolute_rect(int page, fz_rect page_rect) {
	fz_rect res;
	page_pos_to_absolute_pos(page, page_rect.x0, page_rect.y0, &res.x0, &res.y0);
	page_pos_to_absolute_pos(page, page_rect.x1, page_rect.y1, &res.x1, &res.y1);

	return res;
}

int Document::get_offset_page_number(float y_offset)
{
	std::lock_guard guard(page_dims_mutex);

	auto it = std::lower_bound(accum_page_heights.begin(), accum_page_heights.end(), y_offset);

	// std::lower_bound returns an iterator pointing to the first element of the vector not less than y_offset,
	// but we are looking for the last element of vector that is less than y_offset
	if (it > accum_page_heights.begin()) {
		it--;
	}

	return (it - accum_page_heights.begin());
}

void Document::index_figures(bool* invalid_flag)
{
	int n = num_pages();

	if (this->figure_indexing_thread.has_value()) {
		// if we are already indexing figures, we should wait for the previous thread to finish
		this->figure_indexing_thread.value().join();
	}

	is_figure_indexing_required = true;
	is_indexing = true;
	this->figure_indexing_thread = std::thread([this, n, invalid_flag]() {
		std::wcout << "starting index thread ..." << std::endl;
		std::vector<IndexedData> local_figure_data;
		std::map<std::wstring, IndexedData> local_reference_data;
		std::map<std::wstring, IndexedData> local_equation_data;

		fz_context* context_ = fz_clone_context(context);
		fz_document* doc_ = fz_open_document(context_, utf8_encode(file_name).c_str());

		bool focus_next = false;
		for (int i = 0; i < n; i++) {
			// when we close a document before its indexing is finished, we should stop indexing as soon as posible
			if (!is_figure_indexing_required) {
				break;
			}

			// we don't use get_stext_with_page_number here on purpose because it would lead to many unnecessary allocations
			fz_stext_page* stext_page = fz_new_stext_page_from_page_number(context_, doc_, i, nullptr);

			std::vector<fz_stext_char*> flat_chars;
			get_flat_chars_from_stext_page(stext_page, flat_chars);

			index_references(stext_page, i, local_reference_data);
			index_equations(flat_chars, i, local_equation_data);

			LL_ITER(block, stext_page->first_block) {
				if (does_stext_block_starts_with_string_case_insensitive(block, L"fig")) {
					std::wstring res;
					get_stext_block_string(block, res);
					std::wcout << res << "\n";
					local_figure_data.push_back({ i, block->bbox.y1, std::move(res) });
				}
			}

			fz_drop_stext_page(context_, stext_page);
		}


		fz_drop_document(context_, doc_);
		fz_drop_context(context_);

		figure_indices_mutex.lock();
		figure_indices = std::move(local_figure_data);
		reference_indices = std::move(local_reference_data);
		equation_indices = std::move(local_equation_data);
		figure_indices_mutex.unlock();
		std::wcout << "figure indexing finished ... " << std::endl;
		is_indexing = false;
		if (invalid_flag) {
			*invalid_flag = true;
		}
		});
	//thread.detach();
}

void Document::stop_indexing()
{
	is_figure_indexing_required = false;
}


//todo: convert to std::optional
bool Document::find_figure_with_string(std::wstring figure_name, int reference_page, int* page, float* y_offset)
{
	std::lock_guard guard(figure_indices_mutex);

	if (figure_name[figure_name.size() - 1] == '.') { // some books have an extra dot at the end of figure references
		figure_name = figure_name.substr(0, figure_name.size() - 1);
	}

	size_t min_index = 100000;
	float min_y = 0;
	int min_page = -1;
	float min_score = 1000000;

	for (const auto& [p, y, text] : figure_indices) {
		size_t pos = text.find(figure_name);
		int distance = abs(p - reference_page);
		float score = distance + pos * 10;
		if (pos != std::wstring::npos) {
			if (score < min_score) {
				min_index = pos;
				min_y = y;
				min_page = p;
				min_score = score;
			}
		}
	}
	if (min_page >= 0) {
		*page = min_page;
		*y_offset = min_y;
		return true;
	}
	return false;
}

std::optional<IndexedData> Document::find_reference_with_string(std::wstring reference_name)
{
	if (reference_indices.find(reference_name) != reference_indices.end()) {
		return reference_indices[reference_name];
	}

	return {};
}

std::optional<IndexedData> Document::find_equation_with_string(std::wstring equation_name)
{
	if (equation_indices.find(equation_name) != equation_indices.end()) {
		return equation_indices[equation_name];
	}

	return {};
}



std::optional<std::wstring> Document::get_equation_text_at_position(std::vector<fz_stext_char*> flat_chars, float offset_x, float offset_y) {
	fz_rect selected_rect;
	selected_rect.x0 = offset_x - 0.1f;
	selected_rect.x1 = offset_x + 0.1f;
	selected_rect.y0 = offset_y - 0.1f;
	selected_rect.y1 = offset_y + 0.1f;

	std::wregex regex(L"\\([0-9]+(\\.[0-9]+)*\\)");
	std::vector<std::pair<int, int>> match_ranges;
	std::vector<std::wstring> match_texts;

	find_regex_matches_in_stext_page(flat_chars, regex, match_ranges, match_texts);

	for (int i = 0; i < match_ranges.size(); i++) {
		auto [start_index, end_index] = match_ranges[i];
		for (int index = start_index; index <= end_index; index++) {
			if (fz_contains_rect(fz_rect_from_quad(flat_chars[index]->quad), selected_rect)) {
				return match_texts[i].substr(1, match_texts[i].size() - 2);
			}
		}
	}
	return {};
}

std::optional<std::wstring> Document::get_reference_text_at_position(std::vector<fz_stext_char*> flat_chars, float offset_x, float offset_y)
{
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
//std::optional<std::pair<std::wstring, std::wstring>> Document::get_all_text_objects_at_location(std::vector<fz_stext_char*> flat_chars, float offset_x, float offset_y) {
//
//	struct TextObjectType {
//		std::wregex main_regex;
//		std::optional<std::wregex> number_regex;
//		std::wstring name;
//	};
//
//	fz_rect selected_rect;
//
//	selected_rect.x0 = offset_x - 0.1f;
//	selected_rect.x1 = offset_x + 0.1f;
//
//	selected_rect.y0 = offset_y - 0.1f;
//	selected_rect.y1 = offset_y + 0.1f;
//
//	std::wregex reference_regex(L"\\[[^\\[\\]]+\\]");
//	std::wregex reference_name_regex(L"[^,]+,");
//
//	std::wregex figure_regex(L"(figure|Figure|fig\\.|Fig\\.) [0-9]+(\\.[0-9]+)*");
//	std::wregex figure_number_regex(L"[0-9]+(\\.[0-9]+)*");
//
//	std::wregex sentence_regex(L"[^\\.]{5,}\\.");
//
//	std::vector<TextObjectType>  regexes;
//
//	// sorted by priority
//	regexes.push_back({ reference_regex, {}, L"reference" });
//	regexes.push_back({ figure_regex, figure_number_regex, L"figure" });
//	regexes.push_back({ sentence_regex, {}, L"paper_name" });
//
//	//regexes.push_back(std::make_pair(reference_regex, L"reference"));
//	//regexes.push_back(std::make_pair(figure_regex, L"figure"));
//	//regexes.push_back(std::make_pair(sentence_regex, L"paper_name"));
//
//	std::wstring raw_string;
//	std::vector<int> indices;
//
//	for (int i = 0; i < flat_chars.size(); i++) {
//
//		fz_stext_char* ch = flat_chars[i];
//
//		raw_string.push_back(ch->c);
//		indices.push_back(i);
//
//		if (ch->next == nullptr) {
//			raw_string.push_back('\n');
//			indices.push_back(-1);
//		}
//	}
//
//	int offset = 0;
//
//	std::vector<std::pair<int, int>> reference_matches;
//	std::vector<std::pair<int, int>> figure_matches;
//	std::vector<std::pair<int, int>> paper_matches;
//
//	get_matches(raw_string, reference_regex, reference_matches);
//	get_matches(raw_string, figure_regex, figure_matches);
//	get_matches(raw_string, sentence_regex, paper_matches);
//
//	for (auto [begin, end] : reference_matches) {
//		for (int index = begin; index <= end; index++) {
//			int char_index = indices[index];
//			if (char_index < 0) continue;
//
//			if (fz_contains_rect(fz_rect_from_quad(flat_chars[char_index]->quad), selected_rect)) {
//
//			}
//		}
//	}
//
//	return {};
//}

std::optional<std::wstring> Document::get_text_at_position(std::vector<fz_stext_char*> flat_chars, float offset_x, float offset_y)
{

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

std::optional<std::wstring> Document::get_paper_name_at_position(std::vector<fz_stext_char*> flat_chars, float offset_x, float offset_y)
{
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

fz_pixmap* Document::get_small_pixmap(int page)
{
	for (auto [cached_page, pixmap] : cached_small_pixmaps) {
		if (cached_page == page) {
			return pixmap;
		}
	}

	fz_matrix ctm = fz_scale(0.5f, 0.5f);
	fz_pixmap* res = fz_new_pixmap_from_page_number(context, doc, page, ctm, fz_device_rgb(context), 0);
	cached_small_pixmaps.push_back(std::make_pair(page, res));
	unsigned int SMALL_PIXMAP_CACHE_SIZE = 5;

	if (cached_small_pixmaps.size() > SMALL_PIXMAP_CACHE_SIZE) {
		fz_drop_pixmap(context, cached_small_pixmaps[0].second);
		cached_small_pixmaps.erase(cached_small_pixmaps.begin());
	}

	return res;
}
