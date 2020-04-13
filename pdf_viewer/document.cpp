#include "document.h"

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


 void Document::add_bookmark(string desc, float y_offset) {
	 BookMark res;
	 res.description = desc;
	 res.y_offset = y_offset;
	 bookmarks.push_back(res);
	 insert_bookmark(db, file_name, desc, y_offset);
 }

 void Document::add_link(Link link, bool insert_into_database) {
	 links.push_back(link);
	 if (insert_into_database) {
		 insert_link(db, get_path(), link.document_path, link.dest_offset_x, link.dest_offset_y, link.src_offset_y);
	 }
 }

 string Document::get_path() {
	 return file_name;
 }

BookMark* Document::find_closest_bookmark(float to_offset_y, int* index) {

	 int min_index = argminf<BookMark>(bookmarks, [to_offset_y](BookMark bm) {
		 return abs(bm.y_offset - to_offset_y);
		 });

	 if (min_index >= 0) {
		 if (index) *index = min_index;
		 return &bookmarks[min_index];
	 }
	 return nullptr;
 }

void Document::delete_closest_bookmark(float to_y_offset) {
	int closest_index = -1;
	if (find_closest_bookmark(to_y_offset, &closest_index)) {
		delete_bookmark(db, get_path(), bookmarks[closest_index].y_offset);
		bookmarks.erase(bookmarks.begin() + closest_index);
	}
}

//todo: sort the lins and perform a binary search
Link* Document::find_closest_link(float to_offset_y, int* index) {
	int min_index = argminf<Link>(links, [to_offset_y](Link l) {
		return abs(l.src_offset_y - to_offset_y);
		});

	if (min_index >= 0) {
		if (index) *index = min_index;
		return &links[min_index];
	}
	return nullptr;
}

void Document::delete_closest_link(float to_offset_y) {
	int closest_index = -1;
	if (find_closest_link(to_offset_y, &closest_index)) {
		delete_link(db, get_path(), links[closest_index].src_offset_y);
		links.erase(links.begin() + closest_index);
	}
}

const vector<BookMark>& Document::get_bookmarks() const {
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

bool Document::get_mark_location_if_exists(char symbol, float* y_offset) {
	int mark_index = get_mark_index(symbol);
	if (mark_index == -1) {
		return false;
	}
	*y_offset = marks[mark_index].y_offset;
	return true;
}

Document::Document(fz_context* context, string file_name, sqlite3* db) : context(context), file_name(file_name), doc(nullptr), db(db) {
}

const vector<TocNode*>& Document::get_toc() {
	return top_level_toc_nodes;
}

float Document::get_page_height(int page_index)
{
	return page_heights[page_index];
}

float Document::get_page_width(int page_index)
{
	return page_widths[page_index];
}

float Document::get_accum_page_height(int page_index)
{
	return accum_page_heights[page_index];
}

const vector<float>& Document::get_page_heights()
{
	return page_heights;
}

const vector<float>& Document::get_page_widths()
{
	return page_widths;
}

const vector<float>& Document::get_accum_page_heights()
{
	return accum_page_heights;
}

fz_outline* Document::get_toc_outline() {
	if (cached_outline) return cached_outline;
	fz_try(context) {
		cached_outline = fz_load_outline(context, doc);
	}
	fz_catch(context) {
		cout << "Error: Could not load outline ... " << endl;
	}
	return cached_outline;
}

void Document::create_toc_tree(vector<TocNode*>& toc) {
	fz_try(context) {
		fz_outline* outline = get_toc_outline();
		convert_toc_tree(outline, toc);
	}
	fz_catch(context) {
		cout << "Error: Could not load outline ... " << endl;
	}
}
fz_link* Document::get_page_links(int page_number) {
	if (cached_page_links.find(page_number) != cached_page_links.end()) {
		return cached_page_links.at(page_number);
	}
	cout << "getting links .... for " << page_number << endl;

	fz_link* res = nullptr;
	fz_try(context) {
		fz_page* page = fz_load_page(context, doc, page_number);
		res = fz_load_links(context, page);
		cached_page_links[page_number] = res;
		fz_drop_page(context, page);
	}

	fz_catch(context) {
		cout << "Error: Could not load links" << endl;
		res = nullptr;
	}
	return res;
}

Document::~Document() {
	if (doc != nullptr) {
		fz_try(context) {
			fz_drop_document(context, doc);
			//todo: implement rest of destructor
		}
		fz_catch(context) {
			cout << "Error: could not drop documnet" << endl;
		}
	}
}
bool Document::open() {
	if (doc == nullptr) {
		fz_try(context) {
			doc = fz_open_document(context, file_name.c_str());
		}
		fz_catch(context) {
			cout << "could not open " << file_name << endl;
		}
		if (doc != nullptr) {
			load_page_dimensions();
			load_document_metadata_from_db();
			create_toc_tree(top_level_toc_nodes);
			return true;
		}


		return false;
	}
	else {
		cout << "warning! calling open() on an open document" << endl;
		return false;
	}
}

void Document::load_page_dimensions() {
	page_heights.clear();
	accum_page_heights.clear();
	page_widths.clear();

	int n = num_pages();
	float acc_height = 0.0f;
	for (int i = 0; i < n; i++) {
		fz_page* page = fz_load_page(context, doc, i);
		fz_rect page_rect = fz_bound_page(context, page);
		float page_height = page_rect.y1 - page_rect.y0;
		float page_width = page_rect.x1 - page_rect.x0;

		accum_page_heights.push_back(acc_height);
		page_heights.push_back(page_height);
		page_widths.push_back(page_width);
		acc_height += page_height;
		fz_drop_page(context, page);
	}
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
		cout << "could not count pages" << endl;
	}
	return pages;
}

DocumentManager::DocumentManager(fz_context* mupdf_context, sqlite3* database) : mupdf_context(mupdf_context), database(database)
{
}

Document* DocumentManager::get_document(string path) {
	if (cached_documents.find(path) != cached_documents.end()) {
		return cached_documents.at(path);
	}
	Document* new_doc = new Document(mupdf_context, path, database);
	cached_documents[path] = new_doc;
	return new_doc;
}
