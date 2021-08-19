#include "database.h"
#include <sstream>
#include <cassert>
#include <utility>


std::wstring esc(const std::wstring& inp) {
	char* data = sqlite3_mprintf("%q", utf8_encode(inp).c_str());
	std::wstring escaped_string = utf8_decode(data);
	sqlite3_free(data);
	return escaped_string;
}

static int null_callback(void* notused, int argc, char** argv, char** col_name) {
	return 0;
}

static int opened_book_callback(void* res_vector, int argc, char** argv, char** col_name) {
	std::vector<OpenedBookState>* res = (std::vector<OpenedBookState>*) res_vector;

	if (argc != 3) {
		std::cerr << "Error in file " << __FILE__ << " " << "Line: " << __LINE__ << std::endl;
	}

	float zoom_level = atof(argv[0]);
	float offset_x = atof(argv[1]);
	float offset_y = atof(argv[2]);

	res->push_back(OpenedBookState{ zoom_level, offset_x, offset_y });
	return 0;
}

static int prev_doc_callback(void* res_vector, int argc, char** argv, char** col_name) {
	std::vector<std::wstring>* res = (std::vector<std::wstring>*) res_vector;

	if (argc != 1) {
		std::cerr << "Error in file " << __FILE__ << " " << "Line: " << __LINE__ << std::endl;
	}

	res->push_back(utf8_decode(argv[0]));
	return 0;
}

static int mark_select_callback(void* res_vector, int argc, char** argv, char** col_name) {

	std::vector<Mark>* res = (std::vector<Mark>*)res_vector;
	assert(argc == 2);

	char symbol = argv[0][0];
	float offset_y = atof(argv[1]);

	res->push_back({ offset_y, symbol });
	return 0;
}

static int global_mark_select_callback(void* res_vector, int argc, char** argv, char** col_name) {

	std::vector<std::pair<std::wstring, float>>* res = (std::vector<std::pair<std::wstring, float>>*)res_vector;
	assert(argc == 2);

	//char symbol = argv[0][0];
	std::wstring path = utf8_decode(argv[0]);
	float offset_y = atof(argv[1]);

	res->push_back(std::make_pair(path, offset_y));
	return 0;
}

static int global_bookmark_select_callback(void* res_vector, int argc, char** argv, char** col_name) {

	std::vector<std::pair<std::wstring, BookMark>>* res = (std::vector<std::pair<std::wstring, BookMark>>*)res_vector;
	assert(argc == 3);

	std::wstring path = utf8_decode(argv[0]);
	std::wstring desc = utf8_decode(argv[1]);
	float offset_y = atof(argv[2]);

	BookMark bm;
	bm.description = desc;
	bm.y_offset = offset_y;
	res->push_back(std::make_pair(path, bm));
	return 0;
}

static int global_highlight_select_callback(void* res_vector, int argc, char** argv, char** col_name) {

	std::vector<std::pair<std::wstring, Highlight>>* res = (std::vector<std::pair<std::wstring, Highlight>>*)res_vector;
	assert(argc == 7);

	std::wstring path = utf8_decode(argv[0]);
	std::wstring desc = utf8_decode(argv[1]);
	char type = argv[2][0];
	float begin_x = atof(argv[3]);
	float begin_y = atof(argv[4]);
	float end_x = atof(argv[5]);
	float end_y = atof(argv[6]);

	Highlight highlight;
	highlight.description = desc;
	highlight.type = type;
	highlight.selection_begin.x = begin_x;
	highlight.selection_begin.y = begin_y;

	highlight.selection_end.x = end_x;
	highlight.selection_end.y = end_y;
	res->push_back(std::make_pair(path, highlight));
	return 0;
}

static int bookmark_select_callback(void* res_vector, int argc, char** argv, char** col_name) {

	std::vector<BookMark>* res = (std::vector<BookMark>*)res_vector;
	assert(argc == 2);

	std::wstring desc = utf8_decode(argv[0]);
	float offset_y = atof(argv[1]);

	res->push_back({ offset_y, desc });
	return 0;
}

static int highlight_select_callback(void* res_vector, int argc, char** argv, char** col_name) {

	std::vector<Highlight>* res = (std::vector<Highlight>*)res_vector;
	assert(argc == 6);

	std::wstring desc = utf8_decode(argv[0]);
	float begin_x = atof(argv[1]);
	float begin_y = atof(argv[2]);
	float end_x = atof(argv[3]);
	float end_y = atof(argv[4]);
	char type = argv[5][0];

	Highlight highlight;
	highlight.description = desc;
	highlight.type = type;
	highlight.selection_begin = { begin_x, begin_y };
	highlight.selection_end = { end_x, end_y };
	res->push_back(highlight);

	return 0;
}

static int link_select_callback(void* res_vector, int argc, char** argv, char** col_name) {

	std::vector<Link>* res = (std::vector<Link>*)res_vector;
	assert(argc == 5);

	std::wstring dst_path = utf8_decode(argv[0]);
	float src_offset_y = atof(argv[1]);
	float dst_offset_x = atof(argv[2]);
	float dst_offset_y = atof(argv[3]);
	float dst_zoom_level = atof(argv[4]);

	Link link;
	link.dst.document_path = dst_path;
	link.src_offset_y = src_offset_y;
	link.dst.book_state.offset_x = dst_offset_x;
	link.dst.book_state.offset_y = dst_offset_y;
	link.dst.book_state.zoom_level = dst_zoom_level;

	res->push_back(link);
	return 0;
}

bool handle_error(int error_code, char* error_message) {
	if (error_code != SQLITE_OK) {
		std::cerr << "SQL Error: " << error_message << std::endl;
		sqlite3_free(error_message);
		return false;
	}
	return true;
}

bool create_opened_books_table(sqlite3* db) {

	const char* create_opened_books_sql = "CREATE TABLE IF NOT EXISTS opened_books ("\
	"id INTEGER PRIMARY KEY AUTOINCREMENT,"\
	"path TEXT UNIQUE,"\
	"zoom_level REAL,"\
	"offset_x REAL,"\
	"offset_y REAL,"\
	"last_access_time TEXT);";

	char* error_message = nullptr;
	return handle_error(
		sqlite3_exec(db, create_opened_books_sql, null_callback, 0, &error_message),
		error_message);
}

bool create_marks_table(sqlite3* db) {
	const char* create_marks_sql = "CREATE TABLE IF NOT EXISTS marks ("\
	"id INTEGER PRIMARY KEY AUTOINCREMENT," \
	"document_path TEXT,"\
	"symbol CHAR,"\
	"offset_y real,"\
	"UNIQUE(document_path, symbol));";

	char* error_message = nullptr;
	return handle_error(
		sqlite3_exec(db, create_marks_sql, null_callback, 0, &error_message),
		error_message);
}

bool create_bookmarks_table(sqlite3* db) {
	const char* create_bookmarks_sql = "CREATE TABLE IF NOT EXISTS bookmarks ("\
		"id INTEGER PRIMARY KEY AUTOINCREMENT," \
		"document_path TEXT,"\
		"desc TEXT,"\
		"offset_y real);";

	char* error_message = nullptr;
	return handle_error(
		sqlite3_exec(db, create_bookmarks_sql, null_callback, 0, &error_message),
		error_message);
}

bool create_highlights_table(sqlite3* db) {
	const char* create_highlights_sql = "CREATE TABLE IF NOT EXISTS highlights ("\
		"id INTEGER PRIMARY KEY AUTOINCREMENT," \
		"document_path TEXT,"\
		"desc TEXT,"\
		"type char,"\
		"begin_x real,"\
		"begin_y real,"\
		"end_x real,"\
		"end_y real);";

	char* error_message = nullptr;
	return handle_error(
		sqlite3_exec(db, create_highlights_sql, null_callback, 0, &error_message),
		error_message);
}

bool create_links_table(sqlite3* db) {
	const char* create_marks_sql = "CREATE TABLE IF NOT EXISTS links ("\
		"id INTEGER PRIMARY KEY AUTOINCREMENT," \
		"src_document TEXT,"\
		"dst_document TEXT,"\
		"src_offset_y REAL,"\
		"dst_offset_x REAL,"\
		"dst_offset_y REAL,"\
		"dst_zoom_level REAL);";

	char* error_message = nullptr;
	return handle_error(
		sqlite3_exec(db, create_marks_sql, null_callback, 0, &error_message),
		error_message);
}

bool insert_book(sqlite3* db, const std::wstring& path, float zoom_level, float offset_x, float offset_y) {
	const char* insert_books_sql = ""\
		"INSERT INTO opened_books (PATH, zoom_level, offset_x, offset_y, last_access_time) VALUES ";

	std::wstringstream ss;
	ss << insert_books_sql << "'" << esc(path) << "', " << zoom_level << ", " << offset_x << ", " << offset_y << ", datetime('now');";
	
	char* error_message = nullptr;
	return handle_error(
		sqlite3_exec(db, utf8_encode(ss.str()).c_str(), null_callback, 0, &error_message),
		error_message);
}

bool update_book(sqlite3* db, const std::wstring& path, float zoom_level, float offset_x, float offset_y) {

	std::wstringstream ss;
	ss << "insert or replace into opened_books(path, zoom_level, offset_x, offset_y, last_access_time) values ('" <<
		esc(path) << "', " << zoom_level << ", " << offset_x << ", " << offset_y << ", datetime('now'));";

	char* error_message = nullptr;
	return handle_error(
		sqlite3_exec(db, utf8_encode(ss.str()).c_str(), null_callback, 0, &error_message),
		error_message);
}

bool insert_mark(sqlite3* db, const std::wstring& document_path, char symbol, float offset_y) {

	//todo: probably should escape symbol too
	std::wstringstream ss;
	ss << "INSERT INTO marks (document_path, symbol, offset_y) VALUES ('" << esc(document_path) << "', '" << symbol << "', " << offset_y << ");";
	char* error_message = nullptr;

	return handle_error(
		sqlite3_exec(db, utf8_encode(ss.str()).c_str(), null_callback, 0, &error_message),
		error_message);
}

bool delete_mark_with_symbol(sqlite3* db, char symbol) {

	std::wstringstream ss;
	ss << "DELETE FROM marks where symbol='" << symbol << "';";
	char* error_message = nullptr;

	return handle_error(
		sqlite3_exec(db, utf8_encode(ss.str()).c_str(), null_callback, 0, &error_message),
		error_message);
}

bool insert_bookmark(sqlite3* db, const std::wstring& document_path, const std::wstring& desc, float offset_y) {

	std::wstringstream ss;
	ss << "INSERT INTO bookmarks (document_path, desc, offset_y) VALUES ('" << esc(document_path) << "', '" << esc(desc) << "', " << offset_y << ");";
	char* error_message = nullptr;

	return handle_error(
		sqlite3_exec(db, utf8_encode(ss.str()).c_str(), null_callback, 0, &error_message),
		error_message);
}

bool insert_highlight(sqlite3* db,
	const std::wstring& document_path,
	const std::wstring& desc,
	float begin_x,
	float begin_y,
	float end_x,
	float end_y,
	char type) {

	std::wstringstream ss;
	ss << "INSERT INTO highlights (document_path, desc, type, begin_x, begin_y, end_x, end_y) VALUES ('" << esc(document_path) << "', '" << esc(desc) << "', '" <<
		type << "' , " <<
		begin_x << " , " <<
		begin_y << " , " <<
		end_x << " , " <<
		end_y << ");";
	char* error_message = nullptr;

	return handle_error(
		sqlite3_exec(db, utf8_encode(ss.str()).c_str(), null_callback, 0, &error_message),
		error_message);
}

bool insert_link(sqlite3* db, const std::wstring& src_document_path, const std::wstring& dst_document_path, float dst_offset_x, float dst_offset_y, float dst_zoom_level, float src_offset_y) {

	std::wstringstream ss;
	ss << "INSERT INTO links (src_document, dst_document, src_offset_y, dst_offset_x, dst_offset_y, dst_zoom_level) VALUES ('" <<
		esc(src_document_path) << "', '" << esc(dst_document_path) << "', " << src_offset_y << ", " << dst_offset_x << ", " << dst_offset_y << ", " << dst_zoom_level << ");";
	char* error_message = nullptr;

	return handle_error(
		sqlite3_exec(db, utf8_encode(ss.str()).c_str(), null_callback, 0, &error_message),
		error_message);
}

bool update_link(sqlite3* db, const std::wstring& src_document_path, float dst_offset_x, float dst_offset_y, float dst_zoom_level, float src_offset_y) {

	std::wstringstream ss;
	ss << "UPDATE links SET dst_offset_x=" << dst_offset_x << ", dst_offset_y=" << dst_offset_y <<
		", dst_zoom_level=" << dst_zoom_level << " WHERE src_document='" <<
		esc(src_document_path) << "' AND src_offset_y=" << src_offset_y << ";";
	char* error_message = nullptr;

	return handle_error(
		sqlite3_exec(db, utf8_encode(ss.str()).c_str(), null_callback, 0, &error_message),
		error_message);
}

bool delete_link(sqlite3* db, const std::wstring& src_document_path, float src_offset_y) {

	std::wstringstream ss;
	ss << "DELETE FROM links where src_document='" << esc(src_document_path) << "'AND src_offset_y=" << src_offset_y << ";";
	char* error_message = nullptr;

	return handle_error(
		sqlite3_exec(db, utf8_encode(ss.str()).c_str(), null_callback, 0, &error_message),
		error_message);
}

bool delete_bookmark(sqlite3* db, const std::wstring& src_document_path, float src_offset_y) {

	std::wstringstream ss;
	ss << "DELETE FROM bookmarks where document_path='" << esc(src_document_path) << "'AND offset_y=" << src_offset_y << ";";
	char* error_message = nullptr;

	return handle_error(
		sqlite3_exec(db, utf8_encode(ss.str()).c_str(), null_callback, 0, &error_message),
		error_message);
}

bool delete_highlight(sqlite3* db, const std::wstring& src_document_path, float begin_x, float begin_y, float end_x, float end_y) {

	std::wstringstream ss;
	ss << "DELETE FROM highlights where document_path='" << esc(src_document_path) <<
		"'AND begin_x=" << begin_x <<
		" AND begin_y=" << begin_y <<
		" AND end_x=" << end_x <<
		" AND end_y=" << end_y << ";";
	char* error_message = nullptr;

	return handle_error(
		sqlite3_exec(db, utf8_encode(ss.str()).c_str(), null_callback, 0, &error_message),
		error_message);
}

bool update_mark(sqlite3* db, const std::wstring& document_path, char symbol, float offset_y) {

	std::wstringstream ss;
	ss << "UPDATE marks set offset_y=" << offset_y << " where document_path='" << esc(document_path) << "' AND symbol='" << symbol << "';";

	char* error_message = nullptr;
	return handle_error(
		sqlite3_exec(db, utf8_encode(ss.str()).c_str(), null_callback, 0, &error_message),
		error_message);
}


bool select_opened_book(sqlite3* db, const std::wstring& book_path, std::vector<OpenedBookState> &out_result) {
		std::wstringstream ss;
		ss << "select zoom_level, offset_x, offset_y from opened_books where path='" << esc(book_path) << "'";
		char* error_message = nullptr;
		return handle_error(
			sqlite3_exec(db, utf8_encode(ss.str()).c_str(), opened_book_callback, &out_result, &error_message),
			error_message);
}

//bool delete_mark_with_symbol(sqlite3* db, char symbol) {
//
//	std::wstringstream ss;
//	ss << "DELETE FROM marks where symbol='" << symbol << "';";
//	char* error_message = nullptr;
//
//	return handle_error(
//		sqlite3_exec(db, utf8_encode(ss.str()).c_str(), null_callback, 0, &error_message),
//		error_message);
//}

bool delete_opened_book(sqlite3* db, const std::wstring& book_path) {
		std::wstringstream ss;
		ss << "DELETE FROM opened_books where path='" << esc(book_path) << "'";
		char* error_message = nullptr;
		return handle_error(
			sqlite3_exec(db, utf8_encode(ss.str()).c_str(), null_callback, 0, &error_message),
			error_message);
}


bool select_prev_docs(sqlite3* db,  std::vector<std::wstring> &out_result) {
		std::wstringstream ss;
		ss << "SELECT path FROM opened_books order by datetime(last_access_time) desc;";
		char* error_message = nullptr;
		return handle_error(
			sqlite3_exec(db, utf8_encode(ss.str()).c_str(), prev_doc_callback, &out_result, &error_message),
			error_message);
}

bool select_mark(sqlite3* db, const std::wstring& book_path, std::vector<Mark> &out_result) {
		std::wstringstream ss;
		ss << "select symbol, offset_y from marks where document_path='" << esc(book_path) << "';";

		char* error_message = nullptr;
		return handle_error(
			sqlite3_exec(db, utf8_encode(ss.str()).c_str(), mark_select_callback, &out_result, &error_message),
			error_message);
}

bool select_global_mark(sqlite3* db, char symbol, std::vector<std::pair<std::wstring, float>> &out_result) {
		std::wstringstream ss;
		ss << "select document_path, offset_y from marks where symbol='" << symbol << "';";

		char* error_message = nullptr;
		return handle_error(
			sqlite3_exec(db, utf8_encode(ss.str()).c_str(), global_mark_select_callback, &out_result, &error_message),
			error_message);
}

bool select_bookmark(sqlite3* db, const std::wstring& book_path, std::vector<BookMark> &out_result) {
		std::wstringstream ss;
		ss << "select desc, offset_y from bookmarks where document_path='" << esc(book_path) << "';";

		char* error_message = nullptr;
		return handle_error(
			sqlite3_exec(db, utf8_encode(ss.str()).c_str(), bookmark_select_callback, &out_result, &error_message),
			error_message);
}

bool select_highlight(sqlite3* db, const std::wstring& book_path, std::vector<Highlight> &out_result) {
		std::wstringstream ss;
		ss << "select desc, begin_x, begin_y, end_x, end_y, type from highlights where document_path='" << esc(book_path) << "';";

		char* error_message = nullptr;
		return handle_error(
			sqlite3_exec(db, utf8_encode(ss.str()).c_str(), highlight_select_callback, &out_result, &error_message),
			error_message);
}

bool select_highlight_with_type(sqlite3* db, const std::wstring& book_path, char type, std::vector<Highlight> &out_result) {
		std::wstringstream ss;
		ss << "select desc, begin_x, begin_y, end_x, end_y, type from highlights where document_path='" << esc(book_path) << "' AND type='" << type << "';";

		char* error_message = nullptr;
		return handle_error(
			sqlite3_exec(db, utf8_encode(ss.str()).c_str(), highlight_select_callback, &out_result, &error_message),
			error_message);
}

bool global_select_highlight(sqlite3* db,  std::vector<std::pair<std::wstring, Highlight>> &out_result) {
		std::wstringstream ss;
		ss << "select document_path, desc, type, begin_x, begin_y, end_x, end_y from highlights;";

		char* error_message = nullptr;
		return handle_error(
			sqlite3_exec(db, utf8_encode(ss.str()).c_str(), global_highlight_select_callback, &out_result, &error_message),
			error_message);
}

bool global_select_bookmark(sqlite3* db,  std::vector<std::pair<std::wstring, BookMark>> &out_result) {
		std::wstringstream ss;
		ss << "select document_path, desc, offset_y from bookmarks;";

		char* error_message = nullptr;
		return handle_error(
			sqlite3_exec(db, utf8_encode(ss.str()).c_str(), global_bookmark_select_callback, &out_result, &error_message),
			error_message);
}

bool select_links(sqlite3* db, const std::wstring& src_document_path, std::vector<Link> &out_result) {
		std::wstringstream ss;
		ss << "select dst_document, src_offset_y, dst_offset_x, dst_offset_y, dst_zoom_level from links where src_document='" << esc(src_document_path) << "';";

		char* error_message = nullptr;
		return handle_error(
			sqlite3_exec(db, utf8_encode(ss.str()).c_str(), link_select_callback, &out_result, &error_message),
			error_message);
}

void create_tables(sqlite3* db) {
	create_opened_books_table(db);
	create_marks_table(db);
	create_bookmarks_table(db);
	create_highlights_table(db);
	create_links_table(db);
}
