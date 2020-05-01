#pragma once
#include <vector>
#include <string>
#include <optional>
#include <iostream>

#include <Windows.h>

#include <mupdf/fitz.h>
#include "sqlite3.h"

#include "database.h"
#include "utils.h"
#include "book.h"

class Document {
private:
	vector<Mark> marks;
	vector<BookMark> bookmarks;
	vector<Link> links;
	sqlite3* db;
	vector<TocNode*> top_level_toc_nodes;
	vector<wstring> flat_toc_names;
	vector<int> flat_toc_pages;

	optional<int> cached_num_pages;

	fz_context* context;
	wstring file_name;
	unordered_map<int, fz_link*> cached_page_links;
	fz_outline* cached_outline;

	vector<float> accum_page_heights;
	vector<float> page_heights;
	vector<float> page_widths;

	int get_mark_index(char symbol);
	fz_outline* get_toc_outline();
	void load_document_metadata_from_db();
	void create_toc_tree(vector<TocNode*>& toc);

	Document(fz_context* context, wstring file_name, sqlite3* db);
public:
	fz_document* doc;

	void add_bookmark(wstring desc, float y_offset);
	void add_link(Link link, bool insert_into_database = true);
	wstring get_path();
	BookMark* find_closest_bookmark(float to_offset_y, int* index = nullptr);
	void delete_closest_bookmark(float to_y_offset);
	Link* find_closest_link(float to_offset_y, int* index = nullptr);
	void delete_closest_link(float to_offset_y);
	const vector<BookMark>& get_bookmarks() const;
	fz_link* get_page_links(int page_number);
	void add_mark(char symbol, float y_offset);
	bool get_mark_location_if_exists(char symbol, float* y_offset);
	~Document();
	const vector<TocNode*>& get_toc();
	const vector<wstring>& get_flat_toc_names();
	const vector<int>& get_flat_toc_pages();
	bool open();
	float get_page_height(int page_index);
	float get_page_width(int page_index);
	float get_accum_page_height(int page_index);
	const vector<float>& get_page_heights();
	const vector<float>& get_page_widths();
	const vector<float>& get_accum_page_heights();
	void load_page_dimensions();
	int num_pages();
	fz_rect get_page_absolute_rect(int page);
	void absolute_to_page_pos(float absolute_x, float absolute_y, float* doc_x, float* doc_y, int* doc_page);
	//void absolute_to_page_rects(fz_rect absolute_rect,
	//	vector<fz_rect>& resulting_rects,
	//	vector<int>& resulting_pages,
	//	vector<fz_rect>* complete_rects);
	void page_pos_to_absolute_pos(int page, float page_x, float page_y, float* abs_x, float* abs_y);
	fz_rect page_rect_to_absolute_rect(int page, fz_rect page_rect);
	friend class DocumentManager;
};

class DocumentManager {
private:
	fz_context* mupdf_context;
	sqlite3* database;
	unordered_map<wstring, Document*> cached_documents;
public:

	DocumentManager(fz_context* mupdf_context, sqlite3* database);

	Document* get_document(wstring path);
	const unordered_map<wstring, Document*>& get_cached_documents();;
};
