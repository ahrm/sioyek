#include "database.h"
#include <sstream>
#include <cassert>
#include <utility>


using namespace std;

static int null_callback(void* notused, int argc, char** argv, char** col_name) {
	return 0;
}

static int opened_book_callback(void* res_vector, int argc, char** argv, char** col_name) {
	vector<OpenedBookState>* res = (vector<OpenedBookState>*) res_vector;

	if (argc != 3) {
		cerr << "Error in file " << __FILE__ << " " << "Line: " << __LINE__ << endl;
	}

	float zoom_level = atof(argv[0]);
	float offset_x = atof(argv[1]);
	float offset_y = atof(argv[2]);

	res->push_back(OpenedBookState{ zoom_level, offset_x, offset_y });
	return 0;
}

static int prev_doc_callback(void* res_vector, int argc, char** argv, char** col_name) {
	vector<wstring>* res = (vector<wstring>*) res_vector;

	if (argc != 1) {
		cerr << "Error in file " << __FILE__ << " " << "Line: " << __LINE__ << endl;
	}

	res->push_back(utf8_decode(argv[0]));
	return 0;
}

static int mark_select_callback(void* res_vector, int argc, char** argv, char** col_name) {

	vector<Mark>* res = (vector<Mark>*)res_vector;
	assert(argc == 2);

	char symbol = argv[0][0];
	float offset_y = atof(argv[1]);

	res->push_back({ offset_y, symbol });
	return 0;
}

static int global_bookmark_select_callback(void* res_vector, int argc, char** argv, char** col_name) {

	vector<pair<wstring, BookMark>>* res = (vector<pair<wstring, BookMark>>*)res_vector;
	assert(argc == 3);

	wstring path = utf8_decode(argv[0]);
	wstring desc = utf8_decode(argv[1]);
	float offset_y = atof(argv[2]);

	BookMark bm;
	bm.description = desc;
	bm.y_offset = offset_y;
	res->push_back(make_pair(path, bm));
	return 0;
}

static int bookmark_select_callback(void* res_vector, int argc, char** argv, char** col_name) {

	vector<BookMark>* res = (vector<BookMark>*)res_vector;
	assert(argc == 2);

	wstring desc = utf8_decode(argv[0]);
	float offset_y = atof(argv[1]);

	res->push_back({ offset_y, desc });
	return 0;
}

static int link_select_callback(void* res_vector, int argc, char** argv, char** col_name) {

	vector<Link>* res = (vector<Link>*)res_vector;
	assert(argc == 5);

	wstring dst_path = utf8_decode(argv[0]);
	float src_offset_y = atof(argv[1]);
	float dst_offset_x = atof(argv[2]);
	float dst_offset_y = atof(argv[3]);
	float dst_zoom_level = atof(argv[4]);

	Link link;
	link.document_path = dst_path;
	link.src_offset_y = src_offset_y;
	link.dest_offset_x = dst_offset_x;
	link.dest_offset_y = dst_offset_y;
	link.dest_zoom_level = dst_zoom_level;

	res->push_back(link);
	return 0;
}

bool handle_error(int error_code, char* error_message) {
	if (error_code != SQLITE_OK) {
		cerr << "SQL Error: " << error_message << endl;
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
	const char* create_marks_sql = "CREATE TABLE IF NOT EXISTS bookmarks ("\
		"id INTEGER PRIMARY KEY AUTOINCREMENT," \
		"document_path TEXT,"\
		"desc TEXT,"\
		"offset_y real);";

	char* error_message = nullptr;
	return handle_error(
		sqlite3_exec(db, create_marks_sql, null_callback, 0, &error_message),
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

bool insert_book(sqlite3* db, wstring path, float zoom_level, float offset_x, float offset_y) {
	const char* insert_books_sql = ""\
		"INSERT INTO opened_books (PATH, zoom_level, offset_x, offset_y, last_access_time) VALUES ";

	wstringstream ss;
	ss << insert_books_sql << "'" << path << "', " << zoom_level << ", " << offset_x << ", " << offset_y << ", datetime('now');";
	
	char* error_message = nullptr;
	return handle_error(
		sqlite3_exec(db, utf8_encode(ss.str()).c_str(), null_callback, 0, &error_message),
		error_message);
}

bool update_book(sqlite3* db, wstring path, float zoom_level, float offset_x, float offset_y) {

	wstringstream ss;
	ss << "insert or replace into opened_books(path, zoom_level, offset_x, offset_y, last_access_time) values ('" <<
		path << "', " << zoom_level << ", " << offset_x << ", " << offset_y << ", datetime('now'));";

	char* error_message = nullptr;
	return handle_error(
		sqlite3_exec(db, utf8_encode(ss.str()).c_str(), null_callback, 0, &error_message),
		error_message);
}

bool insert_mark(sqlite3* db, wstring document_path, char symbol, float offset_y) {

	wstringstream ss;
	ss << "INSERT INTO marks (document_path, symbol, offset_y) VALUES ('" << document_path << "', '" << symbol << "', " << offset_y << ");";
	char* error_message = nullptr;

	return handle_error(
		sqlite3_exec(db, utf8_encode(ss.str()).c_str(), null_callback, 0, &error_message),
		error_message);
}

bool insert_bookmark(sqlite3* db, wstring document_path, wstring desc, float offset_y) {

	wstringstream ss;
	ss << "INSERT INTO bookmarks (document_path, desc, offset_y) VALUES ('" << document_path << "', '" << desc << "', " << offset_y << ");";
	char* error_message = nullptr;

	return handle_error(
		sqlite3_exec(db, utf8_encode(ss.str()).c_str(), null_callback, 0, &error_message),
		error_message);
}

bool insert_link(sqlite3* db, wstring src_document_path, wstring dst_document_path, float dst_offset_x, float dst_offset_y, float dst_zoom_level, float src_offset_y) {

	wstringstream ss;
	ss << "INSERT INTO links (src_document, dst_document, src_offset_y, dst_offset_x, dst_offset_y, dst_zoom_level) VALUES ('" <<
		src_document_path << "', '" << dst_document_path << "', " << src_offset_y << ", " << dst_offset_x << ", " << dst_offset_y << ", " << dst_zoom_level << ");";
	char* error_message = nullptr;

	return handle_error(
		sqlite3_exec(db, utf8_encode(ss.str()).c_str(), null_callback, 0, &error_message),
		error_message);
}

bool update_link(sqlite3* db, wstring src_document_path, float dst_offset_x, float dst_offset_y, float dst_zoom_level, float src_offset_y) {

	wstringstream ss;
	ss << "UPDATE links SET dst_offset_x=" << dst_offset_x << ", dst_offset_y=" << dst_offset_y <<
		", dst_zoom_level=" << dst_zoom_level << " WHERE src_document='" <<
		src_document_path << "' AND src_offset_y=" << src_offset_y << ";";
	char* error_message = nullptr;

	return handle_error(
		sqlite3_exec(db, utf8_encode(ss.str()).c_str(), null_callback, 0, &error_message),
		error_message);
}

bool delete_link(sqlite3* db, wstring src_document_path, float src_offset_y) {

	wstringstream ss;
	ss << "DELETE FROM links where src_document='" << src_document_path << "'AND src_offset_y=" << src_offset_y << ";";
	char* error_message = nullptr;

	return handle_error(
		sqlite3_exec(db, utf8_encode(ss.str()).c_str(), null_callback, 0, &error_message),
		error_message);
}

bool delete_bookmark(sqlite3* db, wstring src_document_path, float src_offset_y) {

	wstringstream ss;
	ss << "DELETE FROM bookmarks where document_path='" << src_document_path << "'AND offset_y=" << src_offset_y << ";";
	char* error_message = nullptr;

	return handle_error(
		sqlite3_exec(db, utf8_encode(ss.str()).c_str(), null_callback, 0, &error_message),
		error_message);
}

bool update_mark(sqlite3* db, wstring document_path, char symbol, float offset_y) {

	wstringstream ss;
	ss << "UPDATE marks set offset_y=" << offset_y << " where document_path='" << document_path << "' AND symbol='" << symbol << "';";

	char* error_message = nullptr;
	return handle_error(
		sqlite3_exec(db, utf8_encode(ss.str()).c_str(), null_callback, 0, &error_message),
		error_message);
}


bool select_opened_book(sqlite3* db, wstring book_path, vector<OpenedBookState> &out_result) {
		wstringstream ss;
		ss << "select zoom_level, offset_x, offset_y from opened_books where path='" << book_path << "'";
		char* error_message = nullptr;
		return handle_error(
			sqlite3_exec(db, utf8_encode(ss.str()).c_str(), opened_book_callback, &out_result, &error_message),
			error_message);
}

bool select_prev_docs(sqlite3* db,  vector<wstring> &out_result) {
		wstringstream ss;
		ss << "SELECT path FROM opened_books order by datetime(last_access_time) desc;";
		char* error_message = nullptr;
		return handle_error(
			sqlite3_exec(db, utf8_encode(ss.str()).c_str(), prev_doc_callback, &out_result, &error_message),
			error_message);
}

bool select_mark(sqlite3* db, wstring book_path, vector<Mark> &out_result) {
		wstringstream ss;
		ss << "select symbol, offset_y from marks where document_path='" << book_path << "';";

		char* error_message = nullptr;
		return handle_error(
			sqlite3_exec(db, utf8_encode(ss.str()).c_str(), mark_select_callback, &out_result, &error_message),
			error_message);
}

bool select_bookmark(sqlite3* db, wstring book_path, vector<BookMark> &out_result) {
		wstringstream ss;
		ss << "select desc, offset_y from bookmarks where document_path='" << book_path << "';";

		char* error_message = nullptr;
		return handle_error(
			sqlite3_exec(db, utf8_encode(ss.str()).c_str(), bookmark_select_callback, &out_result, &error_message),
			error_message);
}

bool global_select_bookmark(sqlite3* db,  vector<pair<wstring, BookMark>> &out_result) {
		wstringstream ss;
		ss << "select document_path, desc, offset_y from bookmarks;";

		char* error_message = nullptr;
		return handle_error(
			sqlite3_exec(db, utf8_encode(ss.str()).c_str(), global_bookmark_select_callback, &out_result, &error_message),
			error_message);
}

bool select_links(sqlite3* db, wstring src_document_path, vector<Link> &out_result) {
		wstringstream ss;
		ss << "select dst_document, src_offset_y, dst_offset_x, dst_offset_y, dst_zoom_level from links where src_document='" << src_document_path << "';";

		char* error_message = nullptr;
		return handle_error(
			sqlite3_exec(db, utf8_encode(ss.str()).c_str(), link_select_callback, &out_result, &error_message),
			error_message);
}

void create_tables(sqlite3* db) {
	create_opened_books_table(db);
	create_marks_table(db);
	create_bookmarks_table(db);
	create_links_table(db);
}
