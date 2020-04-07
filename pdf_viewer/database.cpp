#include "database.h"
#include <sstream>
#include <cassert>


using namespace std;

static int null_callback(void* notused, int argc, char** argv, char** col_name) {
	return 0;
}

static int opened_book_callback(void* res_vector, int argc, char** argv, char** col_name) {
	vector<OpenedBookState>* res = (vector<OpenedBookState>*) res_vector;

	if (argc != 3) {
		cout << "this should not happen!" << endl;
	}

	float zoom_level = atof(argv[0]);
	float offset_x = atof(argv[1]);
	float offset_y = atof(argv[2]);

	res->push_back(OpenedBookState{ zoom_level, offset_x, offset_y });
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

static int bookmark_select_callback(void* res_vector, int argc, char** argv, char** col_name) {

	vector<BookMark>* res = (vector<BookMark>*)res_vector;
	assert(argc == 2);

	string desc = argv[0];
	float offset_y = atof(argv[1]);

	res->push_back({ offset_y, desc });
	return 0;
}

bool handle_error(int error_code, char* error_message) {
	if (error_code != SQLITE_OK) {
		cout << "SQL Error: " << error_message << endl;
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
	"offset_y REAL);";

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

bool insert_book(sqlite3* db, string path, float zoom_level, float offset_x, float offset_y) {
	const char* insert_books_sql = ""\
		"INSERT INTO opened_books (PATH, zoom_level, offset_x, offset_y) VALUES ";

	stringstream ss;
	ss << insert_books_sql << "'" << path << "', " << zoom_level << ", " << offset_x << ", " << offset_y << ";";
	
	char* error_message = nullptr;
	return handle_error(
		sqlite3_exec(db, ss.str().c_str(), null_callback, 0, &error_message),
		error_message);
}

bool update_book(sqlite3* db, string path, float zoom_level, float offset_x, float offset_y) {

	stringstream ss;
	ss << "insert or replace into opened_books(path, zoom_level, offset_x, offset_y) values ('" <<
		path << "', " << zoom_level << ", " << offset_x << ", " << offset_y << ");";

	
	char* error_message = nullptr;
	return handle_error(
		sqlite3_exec(db, ss.str().c_str(), null_callback, 0, &error_message),
		error_message);
}

bool insert_mark(sqlite3* db, string document_path, char symbol, float offset_y) {

	stringstream ss;
	ss << "INSERT INTO marks (document_path, symbol, offset_y) VALUES ('" << document_path << "', '" << symbol << "', " << offset_y << ");";
	char* error_message = nullptr;

	return handle_error(
		sqlite3_exec(db, ss.str().c_str(), null_callback, 0, &error_message),
		error_message);
}

bool insert_bookmark(sqlite3* db, string document_path, string desc, float offset_y) {

	stringstream ss;
	ss << "INSERT INTO bookmarks (document_path, desc, offset_y) VALUES ('" << document_path << "', '" << desc << "', " << offset_y << ");";
	char* error_message = nullptr;

	return handle_error(
		sqlite3_exec(db, ss.str().c_str(), null_callback, 0, &error_message),
		error_message);
}

bool update_mark(sqlite3* db, string document_path, char symbol, float offset_y) {

	stringstream ss;
	ss << "UPDATE marks set offset_y=" << offset_y << " where document_path='" << document_path << "' AND symbol='" << symbol << "';";

	char* error_message = nullptr;
	return handle_error(
		sqlite3_exec(db, ss.str().c_str(), null_callback, 0, &error_message),
		error_message);
}


bool select_opened_book(sqlite3* db, string book_path, vector<OpenedBookState> &out_result) {
		stringstream ss;
		ss << "select zoom_level, offset_x, offset_y from opened_books where path='" << book_path << "'";
		char* error_message = nullptr;
		return handle_error(
			sqlite3_exec(db, ss.str().c_str(), opened_book_callback, &out_result, &error_message),
			error_message);
}

bool select_mark(sqlite3* db, string book_path, vector<Mark> &out_result) {
		stringstream ss;
		ss << "select symbol, offset_y from marks where document_path='" << book_path << "';";

		char* error_message = nullptr;
		return handle_error(
			sqlite3_exec(db, ss.str().c_str(), mark_select_callback, &out_result, &error_message),
			error_message);
}

bool select_bookmark(sqlite3* db, string book_path, vector<BookMark> &out_result) {
		stringstream ss;
		ss << "select desc, offset_y from bookmarks where document_path='" << book_path << "';";

		char* error_message = nullptr;
		return handle_error(
			sqlite3_exec(db, ss.str().c_str(), bookmark_select_callback, &out_result, &error_message),
			error_message);
}
