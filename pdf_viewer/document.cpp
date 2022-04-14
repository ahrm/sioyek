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
//extern bool AUTO_EMBED_ANNOTATIONS;

int Document::get_mark_index(char symbol) {
	LOG("Document::get_mark_index");
	for (int i = 0; i < marks.size(); i++) {
		if (marks[i].symbol == symbol) {
			return i;
		}
	}
	return -1;
}

void Document::load_document_metadata_from_db() {
	LOG("Document::load_document_metadata_from_db");
	std::string checksum = get_checksum();
	db_manager->insert_document_hash(get_path(), checksum);
	marks.clear();
	bookmarks.clear();
	highlights.clear();
	links.clear();
	links.clear();
	db_manager->select_mark(checksum, marks);
	db_manager->select_bookmark(checksum, bookmarks);
	db_manager->select_highlight(checksum, highlights);
	db_manager->select_links(checksum, links);
}


void Document::add_bookmark(const std::wstring& desc, float y_offset) {
	LOG("Document::add_bookmark");
	BookMark bookmark{ y_offset, desc };
	bookmarks.push_back(bookmark);
	db_manager->insert_bookmark(get_checksum(), desc, y_offset);
	//if (AUTO_EMBED_ANNOTATIONS) {
	//	add_bookmark_annotation(bookmark);
	//}
}

void Document::fill_highlight_rects(fz_context* ctx) {
	LOG("Document::fill_highlight_rects");

	for (int i = 0; i < highlights.size(); i++) {

		const Highlight& highlight = highlights[i];
		std::vector<fz_rect> highlight_rects;
		std::vector<fz_rect> merged_rects;
		std::wstring highlight_text;
		get_text_selection(ctx, highlight.selection_begin, highlight.selection_end, true, highlight_rects, highlight_text);

		merge_selected_character_rects(highlight_rects, merged_rects);

		highlights[i].highlight_rects = std::move(merged_rects);
	}
}


void Document::add_highlight(const std::wstring& desc,
	const std::vector<fz_rect>& highlight_rects,
	fz_point selection_begin,
	fz_point selection_end,
	char type)
{
	LOG("Document::add_highlight");
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

	//if (AUTO_EMBED_ANNOTATIONS) {
	//	add_highlight_annotation(highlight, highlight_rects);
	//}
}

bool Document::get_is_indexing() {
	LOG("Document::get_is_indexing");
	return is_indexing;
}

void Document::add_link(Link link, bool insert_into_database) {
	LOG("Document::add_link");
	links.push_back(link);
	if (insert_into_database) {
		db_manager->insert_link(
			get_checksum(),
			link.dst.document_checksum,
			link.dst.book_state.offset_x,
			link.dst.book_state.offset_y,
			link.dst.book_state.zoom_level,
			link.src_offset_y);
	}
}

std::wstring Document::get_path() {
	LOG("Document::get_path");

	return file_name;
}

std::string Document::get_checksum() {
	LOG("Document::get_checksum");

	return checksummer->get_checksum(get_path());
}

int Document::find_closest_bookmark_index(float to_offset_y) {
	LOG("Document::find_closest_bookmark_index");

	int min_index = argminf<BookMark>(bookmarks, [to_offset_y](BookMark bm) {
		return abs(bm.y_offset - to_offset_y);
		});

	return min_index;
}

int Document::find_closest_sorted_bookmark_index(const std::vector<BookMark>& sorted_bookmarks, float to_offset_y) const {

	int min_index = argminf<BookMark>(sorted_bookmarks, [to_offset_y](BookMark bm) {
		return abs(bm.y_offset - to_offset_y);
		});

	return min_index;
}

int Document::find_closest_sorted_highlight_index(const std::vector<Highlight>& sorted_highlights, float to_offset_y) const {

	int min_index = argminf<Highlight>(sorted_highlights, [to_offset_y](Highlight hl) {
		return abs(hl.selection_begin.y - to_offset_y);
		});

	return min_index;
}

void Document::delete_closest_bookmark(float to_y_offset) {
	LOG("Document::delete_closest_bookmark");
	int closest_index = find_closest_bookmark_index(to_y_offset);
	if (closest_index > -1) {
		BookMark deleted_bookmark = bookmarks[closest_index];
		db_manager->delete_bookmark( get_checksum(), bookmarks[closest_index].y_offset);
		bookmarks.erase(bookmarks.begin() + closest_index);
		//if (AUTO_EMBED_ANNOTATIONS) {
		//	delete_bookmark_annotation(deleted_bookmark);
		//}
	}
}

void Document::delete_highlight_with_index(int index) {
	LOG("Document::delete_highlight_with_index");
	Highlight highlight_to_delete = highlights[index];

	db_manager->delete_highlight(
		get_checksum(),
		highlight_to_delete.selection_begin.x,
		highlight_to_delete.selection_begin.y,
		highlight_to_delete.selection_end.x,
		highlight_to_delete.selection_end.y);
	highlights.erase(highlights.begin() + index);
	//if (AUTO_EMBED_ANNOTATIONS) {
	//	delete_highlight_annotation(highlight_to_delete);
	//}
}

void Document::delete_highlight_with_offsets(float begin_x, float begin_y, float end_x, float end_y) {
	LOG("Document::delete_highlight_with_offsets");
	int index_to_delete = -1;
	for (int i = 0; i < highlights.size(); i++) {
		if (
			(highlights[i].selection_begin.x == begin_x) &&
			(highlights[i].selection_begin.y == begin_y) &&
			(highlights[i].selection_end.x == end_x) &&
			(highlights[i].selection_end.y == end_y)
			) {
			index_to_delete = i;
		}
	}
	if (index_to_delete != -1) {
		delete_highlight_with_index(index_to_delete);
	}

}

std::optional<Link> Document::find_closest_link(float to_offset_y, int* index) {
	LOG("Document::find_closest_link");
	int min_index = argminf<Link>(links, [to_offset_y](Link l) {
		return abs(l.src_offset_y - to_offset_y);
		});

	if (min_index >= 0) {
		if (index) *index = min_index;
		return links[min_index];
	}
	return {};
}

bool Document::update_link(Link new_link) {
	LOG("Document::update_link");
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
	LOG("Document::delete_closest_link");
	int closest_index = -1;
	if (find_closest_link(to_offset_y, &closest_index)) {
		db_manager->delete_link( get_checksum(), links[closest_index].src_offset_y);
		links.erase(links.begin() + closest_index);
	}
}

const std::vector<BookMark>& Document::get_bookmarks() const {
	LOG("std::vector<BookMark>& Document::get_bookmarks");
	return bookmarks;
}

std::vector<BookMark> Document::get_sorted_bookmarks() const {
	LOG("Document::get_sorted_bookmarks");
	std::vector<BookMark> res = bookmarks;
	std::sort(res.begin(), res.end(), [](const BookMark& lhs, const BookMark& rhs) {return lhs.y_offset < rhs.y_offset; });
	return res;
}

const std::vector<Highlight>& Document::get_highlights() const {
	LOG("std::vector<Highlight>& Document::get_highlights");
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
	LOG("std::vector<Highlight> Document::get_highlights_sorted");
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
	LOG("Document::add_mark");
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
	LOG("Document::remove_mark");
	for (int i = 0; i < marks.size(); i++) {
		if (marks[i].symbol == symbol) {
			marks.erase(marks.begin() + i);
			return true;
		}
	}
	return false;
}

bool Document::get_mark_location_if_exists(char symbol, float* y_offset) {
	LOG("Document::get_mark_location_if_exists");
	int mark_index = get_mark_index(symbol);
	if (mark_index == -1) {
		return false;
	}
	*y_offset = marks[mark_index].y_offset;
	return true;
}

Document::Document(fz_context* context, std::wstring file_name, DatabaseManager* db, CachedChecksummer* checksummer) :
	context(context),
	file_name(file_name),
	doc(nullptr),
	db_manager(db),
	checksummer(checksummer){
	last_update_time = QDateTime::currentDateTime();

	network_access_manager = new QNetworkAccessManager;

    QObject::connect(network_access_manager, &QNetworkAccessManager::finished, [&](QNetworkReply *reply) {
		QJsonDocument responsedoc = QJsonDocument::fromJson(reply->readAll());
		std::string page_string = responsedoc.object().value("page").toString().toStdString();
		int page = atoi(page_string.c_str());
		std::string highlights = responsedoc.object().value("text").toString().toStdString();
		cached_fastread_highlights[page] = highlights;
        reply->deleteLater();
        });
}

void Document::count_chapter_pages(std::vector<int> &page_counts) {
	LOG("Document::count_chapter_pages");
	int num_chapters = fz_count_chapters(context, doc);

	for (int i = 0; i < num_chapters; i++) {
		int num_pages = fz_count_chapter_pages(context, doc, i);
		page_counts.push_back(num_pages);
	}

}

void Document::count_chapter_pages_accum(std::vector<int> &accum_page_counts) {
	LOG("Document::count_chapter_pages_accum");
	std::vector<int> raw_page_count;
	count_chapter_pages(raw_page_count);

	int accum = 0;

	for (int i = 0; i < raw_page_count.size(); i++) {
		accum_page_counts.push_back(accum);
		accum += raw_page_count[i];
	}

}

const std::vector<TocNode*>& Document::get_toc() {
	LOG("std::vector<TocNode*>& Document::get_toc");
	return top_level_toc_nodes;
}

bool Document::has_toc() {
	LOG("Document::has_toc");
	return top_level_toc_nodes.size() > 0;
}

const std::vector<std::wstring>& Document::get_flat_toc_names() {
	LOG("std::vector<std::wstring>& Document::get_flat_toc_names");
	return flat_toc_names;
}

const std::vector<int>& Document::get_flat_toc_pages() {
	LOG("std::vector<int>& Document::get_flat_toc_pages");
	return flat_toc_pages;
}

float Document::get_page_height(int page_index) {
	LOG("Document::get_page_height");
	std::lock_guard guard(page_dims_mutex);
	if ((page_index >= 0) && (page_index < page_heights.size())) {
		return page_heights[page_index];
	}
	else {
		return -1.0f;
	}
}

float Document::get_page_width(int page_index) {
	LOG("Document::get_page_width");
	if ((page_index >= 0) && (page_index < page_widths.size())) {
		return page_widths[page_index];
	}
	else {
		return -1.0f;
	}
}

float Document::get_page_width_smart(int page_index, float* left_ratio, float* right_ratio, int* normal_page_width) {
	LOG("Document::get_page_width_smart");
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

float Document::get_accum_page_height(int page_index) {
	LOG("Document::get_accum_page_height");
	std::lock_guard guard(page_dims_mutex);
	if (page_index < 0 || (page_index >= accum_page_heights.size())) {
		return 0.0f;
	}
	return accum_page_heights[page_index];
}


fz_outline* Document::get_toc_outline() {
	LOG("Document::get_toc_outline");
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
	LOG("Document::create_toc_tree");
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
	LOG("Document::convert_toc_tree");
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
	LOG("Document::get_page_links");
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
	LOG("Document::get_last_edit_time");
	QFileInfo info(QString::fromStdWString(get_path()));
	return info.lastModified();
}

unsigned int Document::get_milies_since_last_document_update_time() {
	LOG("int Document::get_milies_since_last_document_update_time");
	QDateTime now = QDateTime::currentDateTime();
	return last_update_time.msecsTo(now);

}

unsigned int Document::get_milies_since_last_edit_time() {
	LOG("int Document::get_milies_since_last_edit_time");
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
	delete network_access_manager;
	//this->figure_indexing_thread.join();
}
void Document::reload(std::string password) {
	LOG("Document::reload");
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

	open(invalid_flag_pointer, false, password);
}

bool Document::open(bool* invalid_flag, bool force_load_dimensions, std::string password) {
	LOG("Document::open");
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
		if (doc != nullptr) {
			//load_document_metadata_from_db();
			load_page_dimensions(force_load_dimensions);
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
	LOG("Document::get_visible_pages");

	std::lock_guard guard(page_dims_mutex);

	if (page_heights.size() == 0) {
		return;
	}

	float page_begin = 0.0f;

	for (int i = 0; i < page_heights.size(); i++) {
		float page_end = page_begin + page_heights[i];

		if (range_intersects(doc_y_range_begin, doc_y_range_end, page_begin, page_end)) {
			visible_pages.push_back(i);
		}
		page_begin = page_end;
	}
}

void Document::load_page_dimensions(bool force_load_now) {
	LOG("Document::load_page_dimensions");
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
			for (int i = 0; i < n; i++) {
				int height = bounds.y1 - bounds.y0;
				int width = bounds.x1 - bounds.x0;

				page_heights.push_back(height);
				accum_page_heights.push_back(acc_height);
				acc_height += height;
				page_widths.push_back(width);
			}
		}
		fz_catch(context) {
			std::wcout << L"could not load sample page dimensions\n";
		}
	}

	auto load_page_dimensions_function = [this, n]() {
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

			fz_drop_document(context_, doc_);
		}
		fz_catch(context_) {
			std::wcout << L"Error: could not load page dimensions\n";
		}

		page_dims_mutex.lock();

		page_heights = std::move(page_heights_);
		accum_page_heights = std::move(accum_page_heights_);
		page_widths = std::move(page_widths_);

		if (invalid_flag_pointer) {
			*invalid_flag_pointer = true;
		}
		page_dims_mutex.unlock();

		fill_highlight_rects(context_);
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
	LOG("Document::get_page_absolute_rect");
	std::lock_guard guard(page_dims_mutex);

	fz_rect res;

	if (page >= page_widths.size()) {
		res.x0 = 0;
		res.y0 = 0;
		res.x1 = 1;
		res.y1 = 1;
		return res;
	}

	res.x0 = -page_widths[page] / 2;
	res.x1 = page_widths[page] / 2;

	res.y0 = accum_page_heights[page];
	res.y1 = accum_page_heights[page] + page_heights[page];
	return res;
}

int Document::num_pages() {
	LOG("Document::num_pages");
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
	LOG("DocumentManager::get_document");
	if (cached_documents.find(path) != cached_documents.end()) {
		return cached_documents.at(path);
	}
	Document* new_doc = new Document(mupdf_context, path, db_manager, checksummer);
	cached_documents[path] = new_doc;
	return new_doc;
}

const std::unordered_map<std::wstring, Document*>& DocumentManager::get_cached_documents() {
	LOG("std::unordered_map<std::wstring, Document*>& DocumentManager::get_cached_documents");
	return cached_documents;
}

void DocumentManager::delete_global_mark(char symbol) {
	LOG("DocumentManager::delete_global_mark");
	for (auto [path, doc] : cached_documents) {
		doc->remove_mark(symbol);
	}
}


fz_stext_page* Document::get_stext_with_page_number(int page_number) {
	LOG("Document::get_stext_with_page_number");
	return get_stext_with_page_number(context, page_number);
}

fz_stext_page* Document::get_stext_with_page_number(fz_context* ctx, int page_number) {
	LOG("Document::get_stext_with_page_number");
	const int MAX_CACHED_STEXT_PAGES = 10;

	for (auto [page, cached_stext_page] : cached_stext_pages) {
		if (page == page_number) {
			return cached_stext_page;
		}
	}

	fz_stext_page* stext_page = nullptr;

	fz_try(ctx) {
		//if (needs_authentication()) {
		//	fz_authenticate_password(context, doc, correct_password.c_str());
		//}
		stext_page = fz_new_stext_page_from_page_number(ctx, doc, page_number, nullptr);
	}
	fz_catch(ctx) {

	}

	if (stext_page != nullptr) {

		if (cached_stext_pages.size() == MAX_CACHED_STEXT_PAGES) {
			fz_drop_stext_page(ctx, cached_stext_pages[0].second);
			cached_stext_pages.erase(cached_stext_pages.begin());
		}
		cached_stext_pages.push_back(std::make_pair(page_number, stext_page));
		return stext_page;
	}

	return nullptr;
}

int Document::get_page_offset() {
	LOG("Document::get_page_offset");
	return page_offset;
}

void Document::set_page_offset(int new_offset) {
	LOG("Document::set_page_offset");
	page_offset = new_offset;
}

fz_rect Document::absolute_to_page_rect(const fz_rect& absolute_rect, int* page) {
	float page_x0, page_x1, page_y0, page_y1;
	int page_number = -1;
	absolute_to_page_pos(absolute_rect.x0, absolute_rect.y0, &page_x0, &page_y0, &page_number);
	absolute_to_page_pos(absolute_rect.x1, absolute_rect.y1, &page_x1, &page_y1, &page_number);
	if (page != nullptr) {
		*page = page_number;
	}
	fz_rect res;
	res.x0 = page_x0;
	res.x1 = page_x1;
	res.y0 = page_y0;
	res.y1 = page_y1;
	return res;
}

void Document::absolute_to_page_pos(float absolute_x, float absolute_y, float* doc_x, float* doc_y, int* doc_page) {
	LOG("Document::absolute_to_page_pos");

	std::lock_guard guard(page_dims_mutex);
	if (accum_page_heights.size() == 0) {
		*doc_x = 0;
		*doc_y = 0;
		*doc_page = 0;
		return;
	}

	int i = (std::lower_bound(
		accum_page_heights.begin(),
		accum_page_heights.end(), absolute_y) -  accum_page_heights.begin()) - 1;
	i = std::max(0, i);

	float acc_page_heights_i = 0.0f;
	float page_width_i = 0.0f;
	if (i < accum_page_heights.size()) {
		acc_page_heights_i = accum_page_heights[i];
		page_width_i = page_widths[i];
		float remaining_y = absolute_y - acc_page_heights_i;
		float page_width = page_width_i;

		*doc_x = page_width / 2 + absolute_x;
		*doc_y = remaining_y;
		*doc_page = i;
	}
	else {
		*doc_page = -1;
	}
}

QStandardItemModel* Document::get_toc_model() {
	LOG("Document::get_toc_model");
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
	LOG("Document::page_pos_to_absolute_pos");
	std::lock_guard guard(page_dims_mutex);
	if ((page_widths.size() == 0) || (page >= page_widths.size())) {
		*abs_x = 0;
		*abs_y = 0;
		return;
	}

	*abs_x = page_x - page_widths[page] / 2;
	*abs_y = page_y + accum_page_heights[page];
}

fz_rect Document::page_rect_to_absolute_rect(int page, fz_rect page_rect) {
	LOG("Document::page_rect_to_absolute_rect");
	fz_rect res;
	page_pos_to_absolute_pos(page, page_rect.x0, page_rect.y0, &res.x0, &res.y0);
	page_pos_to_absolute_pos(page, page_rect.x1, page_rect.y1, &res.x1, &res.y1);

	return res;
}

int Document::get_offset_page_number(float y_offset) {
	LOG("Document::get_offset_page_number");
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

void Document::index_figures(bool* invalid_flag) {
	LOG("Document::index_figures");
	int n = num_pages();

	if (this->figure_indexing_thread.has_value()) {
		// if we are already indexing figures, we should wait for the previous thread to finish
		this->figure_indexing_thread.value().join();
	}

	is_figure_indexing_required = true;
	is_indexing = true;

	this->figure_indexing_thread = std::thread([this, n, invalid_flag]() {
		std::wcout << "starting index thread ..." << std::endl;
		std::vector<IndexedData> local_generic_data;
		std::map<std::wstring, IndexedData> local_reference_data;
		std::map<std::wstring, std::vector<IndexedData>> local_equation_data;

		fz_context* context_ = fz_clone_context(context);
		fz_try(context_) {

			fz_document* doc_ = fz_open_document(context_, utf8_encode(file_name).c_str());

			if (document_needs_password) {
				fz_authenticate_password(context_, doc_, correct_password.c_str());
			}

			bool focus_next = false;
			for (int i = 0; i < n; i++) {
				// when we close a document before its indexing is finished, we should stop indexing as soon as posible
				if (!is_figure_indexing_required) {
					break;
				}

				// we don't use get_stext_with_page_number here on purpose because it would lead to many unnecessary allocations
				fz_stext_options options;
				options.flags = FZ_STEXT_PRESERVE_IMAGES;
				fz_stext_page* stext_page = fz_new_stext_page_from_page_number(context_, doc_, i, &options);

				std::vector<fz_stext_char*> flat_chars;
				get_flat_chars_from_stext_page(stext_page, flat_chars);

				index_references(stext_page, i, local_reference_data);
				index_equations(flat_chars, i, local_equation_data);
				index_generic(flat_chars, i, local_generic_data);

				fz_drop_stext_page(context_, stext_page);
			}


			fz_drop_document(context_, doc_);
		}
		fz_catch(context_) {
			std::wcout << L"There was an error in indexing thread.\n";
		}

		fz_drop_context(context_);

		figure_indices_mutex.lock();

		reference_indices = std::move(local_reference_data);
		equation_indices = std::move(local_equation_data);
		generic_indices = std::move(local_generic_data);

		figure_indices_mutex.unlock();
		std::wcout << "figure indexing finished ... " << std::endl;
		is_indexing = false;
		if (invalid_flag) {
			*invalid_flag = true;
		}
		});
	//thread.detach();
}

void Document::stop_indexing() {
	LOG("Document::stop_indexing");
	is_figure_indexing_required = false;
}



std::optional<IndexedData> Document::find_reference_with_string(std::wstring reference_name) {
	LOG("Document::find_reference_with_string");
	if (reference_indices.find(reference_name) != reference_indices.end()) {
		return reference_indices[reference_name];
	}

	return {};
}

std::optional<IndexedData> Document::find_equation_with_string(std::wstring equation_name, int page_number) {
	LOG("Document::find_equation_with_string");
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
	LOG("Document::get_equation_text_at_position");


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
	LOG("Document::get_regex_match_at_position");
	fz_rect selected_rect;
	selected_rect.x0 = offset_x - 0.1f;
	selected_rect.x1 = offset_x + 0.1f;
	selected_rect.y0 = offset_y - 0.1f;
	selected_rect.y1 = offset_y + 0.1f;

	std::vector<std::pair<int, int>> match_ranges;
	std::vector<std::wstring> match_texts;

	find_regex_matches_in_stext_page(flat_chars, regex, match_ranges, match_texts);

	for (int i = 0; i < match_ranges.size(); i++) {
		auto [start_index, end_index] = match_ranges[i];
		for (int index = start_index; index <= end_index; index++) {
			if (fz_contains_rect(fz_rect_from_quad(flat_chars[index]->quad), selected_rect)) {
				return match_texts[i];
			}
		}
	}
	return {};
}

bool Document::find_generic_location(const std::wstring& type, const std::wstring& name, int* page, float* y_offset) {
	LOG("Document::find_generic_location");
	int best_page = -1;
	int best_y_offset = 0.0f;
	float best_score = -1000;

	for (int i = 0; i < generic_indices.size(); i++) {
		std::vector<std::wstring> parts = split_whitespace(generic_indices[i].text);

		if (parts.size() == 2) {
			std::wstring current_type = parts[0];
			std::wstring current_name = parts[1];

			if (current_name == name) {
				int score = type_name_similarity_score(current_type, type);
				if (score > best_score) {
					best_page = generic_indices[i].page;
					best_y_offset = generic_indices[i].y_offset;
					best_score = score;
				}
			}

		}
	}

	if (best_page != -1) {
		*page = best_page;
		*y_offset = best_y_offset;
		return true;
	}

	return false;
}

bool Document::can_use_highlights() {
	LOG("Document::can_use_highlights");
	return are_highlights_loaded;
}

std::optional<std::pair<std::wstring, std::wstring>> Document::get_generic_link_name_at_position(const std::vector<fz_stext_char*>& flat_chars, float offset_x, float offset_y) {
	LOG("std::wstring>> Document::get_generic_link_name_at_position");
	std::wregex regex(L"[a-zA-Z]{3,}[ \t]+[0-9]+(\.[0-9]+)*");
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
	LOG("Document::get_reference_text_at_position");
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
	LOG("get_matches");
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
	LOG("Document::get_text_at_position");

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
	LOG("Document::get_paper_name_at_position");
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
	LOG("Document::get_small_pixmap");
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

void Document::get_text_selection(fz_point selection_begin,
	fz_point selection_end,
	bool is_word_selection, // when in word select mode, we select entire words even if the range only partially includes the word
	std::vector<fz_rect>& selected_characters,
	std::wstring& selected_text) {
	LOG("Document::get_text_selection");
	get_text_selection(context, selection_begin, selection_end, is_word_selection, selected_characters, selected_text);
}
void Document::get_text_selection(fz_context* ctx, fz_point selection_begin,
	fz_point selection_end,
	bool is_word_selection,
	std::vector<fz_rect>& selected_characters,
	std::wstring& selected_text) {
	LOG("Document::get_text_selection");

	// selected_characters are in absolute document space
	int page_begin, page_end;
	fz_rect page_rect;

	selected_characters.clear();
	selected_text.clear();

	fz_point page_point1;
	fz_point page_point2;

	absolute_to_page_pos(selection_begin.x, selection_begin.y, &page_point1.x, &page_point1.y, &page_begin);
	absolute_to_page_pos(selection_end.x, selection_end.y, &page_point2.x, &page_point2.y, &page_end);

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
		fz_stext_page* stext_page = get_stext_with_page_number(ctx, i);
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
					fz_rect charrect = page_rect_to_absolute_rect(i, fz_rect_from_quad(current_char->quad));
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
	int page_count = num_pages();

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
		int page_number;
		float doc_x, doc_y;
		absolute_to_page_pos(0, bookmark.y_offset, &doc_x, &doc_y, &page_number);

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

	pdf_write_options pwo = {0};
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

std::vector<fz_rect> Document::get_page_flat_words(int page) {
	if (cached_flat_words.find(page) != cached_flat_words.end()) {
		return cached_flat_words[page];
	}

	fz_stext_page* stext_page = get_stext_with_page_number(page);
	std::vector<fz_stext_char*> flat_chars;
	std::vector<fz_rect> word_rects;
	std::vector<std::pair<fz_rect, int>> word_rects_with_page;
	get_flat_chars_from_stext_page(stext_page, flat_chars);
	get_flat_words_from_flat_chars(flat_chars, word_rects);
	cached_flat_words[page] = word_rects;
    return word_rects;
}

void Document::rotate() {
	std::swap(page_heights, page_widths);
	float acc_height = 0;
	for (int i = 0; i < page_heights.size(); i++) {
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
	if ((index+offset) < sorted_highlights.size()) {
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

	auto fastread_highlights_ = get_page_fastread_highlights(page);
	if (fastread_highlights_) {
		auto fastread_highlights = fastread_highlights_.value();

		if (fastread_highlights.size() != flat_chars.size()) {
			std::wcout << L"invalid highlight received\n";
			return res;
		}

		std::vector<fz_rect> highlighted_characters;
		for (int i = 0; i < flat_chars.size(); i++) {
			if (fastread_highlights[i] == '0') {
				if (highlighted_characters.size() > 0) {
					auto word_rects = create_word_rects_multiline(highlighted_characters);
					for (auto rect : word_rects) {
						res.push_back(rect);
					}
					highlighted_characters.clear();
				}
			}
			else {
				highlighted_characters.push_back(fz_rect_from_quad(flat_chars[i]->quad));
			}
		}
		return res;
	}
	else {
		if (!USE_HEURISTIC_IF_TEXT_SUMMARY_NOT_AVAILABLE) return res;

		std::vector<std::wstring> words;
		std::vector<std::vector<fz_rect>> word_rects;
		get_word_rect_list_from_flat_chars(flat_chars, words, word_rects);


		for (int i = 0; i < words.size(); i++) {

			std::vector<fz_rect> highlighted_characters;

			int num_highlighted = static_cast<int>(std::ceil(words[i].size() * 0.3f));
			for (int j = 0; j < num_highlighted; j++) {
				highlighted_characters.push_back(word_rects[i][j]);
			}

			res.push_back(create_word_rect(highlighted_characters));
		}
		return res;
	}

}

std::optional<std::string> Document::get_page_fastread_highlights(int page) {
	if (cached_fastread_highlights.find(page) != cached_fastread_highlights.end()) {
		return cached_fastread_highlights[page];
	}
	else {
		cached_fastread_highlights[page] = {};
		std::vector<fz_stext_char*> chars;
		auto stext_page = get_stext_with_page_number(page);
		std::wstring page_text;
		get_flat_chars_from_stext_page(stext_page, chars);
		for (auto chr : chars) {
			page_text.push_back(chr->c);
		}

		const QUrl url = QString::fromStdWString(TEXT_HIGHLIGHT_URL);
		QNetworkRequest request(url);
        request.setHeader(QNetworkRequest::ContentTypeHeader,
            "application/x-www-form-urlencoded");
        QUrlQuery postData;
        postData.addQueryItem("text", QString::fromStdWString(page_text));
        postData.addQueryItem("page", QString::number(page));

		if (TEXT_SUMMARY_HIGHLIGHT_SHOULD_REFINE) {
			postData.addQueryItem("refine", QString::number(1));
		}
		else {
			postData.addQueryItem("refine", QString::number(0));
		}
		
		if (TEXT_SUMMARY_HIGHLIGHT_SHOULD_FILL) {
			postData.addQueryItem("fill", QString::number(1));
		}
		else {
			postData.addQueryItem("fill", QString::number(0));
		}
		postData.addQueryItem("context_size", QString::number(TEXT_SUMMARY_CONTEXT_SIZE));


        network_access_manager->post(request, postData.toString(QUrl::FullyEncoded).toUtf8());
		return {};
	}
}
