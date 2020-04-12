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
	optional<int> cached_num_pages;
	fz_context* context;
	string file_name;
	vector<fz_rect> page_rects;
	unordered_map<int, fz_link*> cached_page_links;
	fz_outline* cached_outline;

	int get_mark_index(char symbol);
	fz_outline* get_toc_outline();
	void load_document_metadata_from_db();
	void create_toc_tree(vector<TocNode*>& toc);

public:
	fz_document* doc;

	void add_bookmark(string desc, float y_offset);
	void add_link(Link link, bool insert_into_database = true);
	string get_path();
	BookMark* find_closest_bookmark(float to_offset_y, int* index = nullptr);
	void delete_closest_bookmark(float to_y_offset);
	Link* find_closest_link(float to_offset_y, int* index = nullptr);
	void delete_closest_link(float to_offset_y);
	const vector<BookMark>& get_bookmarks() const;
	fz_link* get_page_links(int page_number);
	void add_mark(char symbol, float y_offset);
	bool get_mark_location_if_exists(char symbol, float* y_offset);
	Document(fz_context* context, string file_name, sqlite3* db);
	~Document();
	const vector<TocNode*>& get_toc();
	bool open();
	int num_pages();
};
