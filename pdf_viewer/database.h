#pragma once

#include <iostream>
#include <vector>
#include <iostream>
#include <string>
#include "sqlite3.h"
#include "book.h"
#include "utils.h"
#include "checksum.h"

class DatabaseManager {
private:
	sqlite3* local_db;
	sqlite3* global_db;
	bool create_opened_books_table();
	bool create_marks_table();
	bool create_bookmarks_table();
	bool create_links_table();
	void create_tables();
	bool create_document_hash_table();
	bool create_highlights_table();
public:
	bool open(const std::wstring& local_db_file_path, const std::wstring& global_db_file_path);
	bool select_opened_book(const std::string& book_path, std::vector<OpenedBookState>& out_result);
	bool insert_mark(const std::string& checksum, char symbol, float offset_y);
	bool update_mark(const std::string& checksum, char symbol, float offset_y);
	bool update_book(const std::string& path, float zoom_level, float offset_x, float offset_y);
	bool select_mark(const std::string& checksum, std::vector<Mark>& out_result);
	bool insert_bookmark(const std::string& checksum, const std::wstring& desc, float offset_y);
	bool select_bookmark(const std::string& checksum, std::vector<BookMark>& out_result);
	bool insert_portal(const std::string& src_checksum, const std::string& dst_checksum, float dst_offset_y, float dst_offset_x, float dst_zoom_level, float src_offset_y);
	bool select_links(const std::string& src_checksum, std::vector<Portal>& out_result);
	bool delete_link(const std::string& src_checksum, float src_offset_y);
	bool delete_bookmark(const std::string& src_checksum, float src_offset_y);
	bool global_select_bookmark(std::vector<std::pair<std::string, BookMark>>& out_result);
	bool global_select_highlight(std::vector<std::pair<std::string, Highlight>>& out_result);
	bool update_portal(const std::string& checksum, float dst_offset_x, float dst_offset_y, float dst_zoom_level, float src_offset_y);
	bool select_opened_books_path_values(std::vector<std::wstring>& out_result);
	bool delete_mark_with_symbol(char symbol);
	bool select_global_mark(char symbol, std::vector<std::pair<std::string, float>>& out_result);
	bool delete_opened_book(const std::string& book_path);
	bool delete_highlight(const std::string& checksum, float begin_x, float begin_y, float end_x, float end_y);
	bool select_highlight(const std::string& checksum, std::vector<Highlight>& out_result);
	bool select_highlight_with_type(const std::string& checksum, char type, std::vector<Highlight>& out_result);
	bool insert_highlight(const std::string& checksum,
		const std::wstring& desc,
		float begin_x,
		float begin_y,
		float end_x,
		float end_y,
		char type);
	bool get_path_from_hash(const std::string& checksum, std::vector<std::wstring>& out_paths);
	bool get_hash_from_path(const std::string& path, std::vector<std::wstring>& out_checksum);
	bool get_prev_path_hash_pairs(std::vector<std::pair<std::wstring, std::wstring>>& out_pairs);
	bool insert_document_hash(const std::wstring& path, const std::string& checksum);
	void upgrade_database_hashes();
	void split_database(const std::wstring& local_database_path, const std::wstring& global_database_path, bool was_using_hashes);
	void export_json(std::wstring json_file_path, CachedChecksummer* checksummer);
	void import_json(std::wstring json_file_path, CachedChecksummer* checksummer);
	void ensure_database_compatibility(const std::wstring& local_db_file_path, const std::wstring& global_db_file_path);
};

