#include "database.h"
#include <sstream>
#include <cassert>
#include <utility>
#include <set>
#include <optional>
#include <unordered_map>
#include <variant>
#include <iomanip>

#include <qfile.h>
#include <qjsonarray.h>
#include <qjsondocument.h>
#include <qjsonobject.h>
#include <qjsonvalue.h>

#include "checksum.h"

extern bool DEBUG;
extern float HIGHLIGHT_DELETE_THRESHOLD;

std::wstring esc(const std::wstring& inp) {
	char* data = sqlite3_mprintf("%q", utf8_encode(inp).c_str());
	std::wstring escaped_string = utf8_decode(data);
	sqlite3_free(data);
	return escaped_string;
}

std::wstring esc(const std::string& inp) {
	return esc(utf8_decode(inp));
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

	std::vector<std::pair<std::string, float>>* res = (std::vector<std::pair<std::string, float>>*)res_vector;
	assert(argc == 2);

	//char symbol = argv[0][0];
	std::string path = argv[0];
	float offset_y = atof(argv[1]);

	res->push_back(std::make_pair(path, offset_y));
	return 0;
}

static int global_bookmark_select_callback(void* res_vector, int argc, char** argv, char** col_name) {

	std::vector<std::pair<std::string, BookMark>>* res = (std::vector<std::pair<std::string, BookMark>>*)res_vector;
	assert(argc == 3);

	std::string path = argv[0];
	std::wstring desc = utf8_decode(argv[1]);
	float offset_y = atof(argv[2]);

	BookMark bm;
	bm.description = desc;
	bm.y_offset = offset_y;
	res->push_back(std::make_pair(path, bm));
	return 0;
}

static int global_highlight_select_callback(void* res_vector, int argc, char** argv, char** col_name) {

	std::vector<std::pair<std::string, Highlight>>* res = (std::vector<std::pair<std::string, Highlight>>*)res_vector;
	assert(argc == 7);

	std::string path = argv[0];
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

static int wstring_select_callback(void* res_vector, int argc, char** argv, char** col_name) {

	std::vector<std::wstring>* res = (std::vector<std::wstring>*)res_vector;
	assert(argc == 1);

	std::wstring desc = utf8_decode(argv[0]);

	res->push_back(desc);
	return 0;
}

static int string_select_callback(void* res_vector, int argc, char** argv, char** col_name) {

	std::vector<std::string>* res = (std::vector<std::string>*)res_vector;
	assert(argc == 1);

	std::string desc = argv[0];

	res->push_back(desc);
	return 0;
}

static int wstring_pair_select_callback(void* res_vector, int argc, char** argv, char** col_name) {

	std::vector<std::pair<std::wstring, std::wstring>>* res = (std::vector<std::pair<std::wstring, std::wstring>>*)res_vector;
	assert(argc == 2);

	std::wstring first = utf8_decode(argv[0]);
	std::wstring second = utf8_decode(argv[1]);

	res->push_back(std::make_pair(first, second));
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

	std::vector<Portal>* res = (std::vector<Portal>*)res_vector;
	assert(argc == 5);

	std::string dst_path = argv[0];
	float src_offset_y = atof(argv[1]);
	float dst_offset_x = atof(argv[2]);
	float dst_offset_y = atof(argv[3]);
	float dst_zoom_level = atof(argv[4]);

	Portal link;
	link.dst.document_checksum = dst_path;
	link.src_offset_y = src_offset_y;
	link.dst.book_state.offset_x = dst_offset_x;
	link.dst.book_state.offset_y = dst_offset_y;
	link.dst.book_state.zoom_level = dst_zoom_level;

	res->push_back(link);
	return 0;
}

//template<typename T>
//T parse_single(char* inp) {
//	return T();
//}
//
//template<>
//int parse_single<int>(char* inp) {
//	return std::stoi(inp);
//}
//
//template<>
//float parse_single<float>(char* inp) {
//	return std::stof(inp);
//}
//
//template<>
//std::string parse_single<std::string>(char* inp) {
//	return std::string(inp);
//}
//
//template<>
//std::wstring parse_single<std::wstring>(char* inp) {
//	return utf8_decode(std::string(inp));
//}
//
//template<>
//char parse_single<char>(char* inp) {
//	return *inp;
//}
//
//template <size_t i>
//std::tuple<> unpack_helper(void* res_vector, int argc, char** argv, char** col_name) {
//	return std::tuple<>();
//}
//
//template <size_t i, typename T, typename... Types>
//std::tuple<T, Types...> unpack_helper(void* res_vector, int argc, char** argv, char** col_name) {
//	return std::tuple_cat(std::tuple<T>(parse_single<T>(argv[i])), unpack_helper<i + 1, Types...>(res_vector, argc, argv, col_name));
//}
//
//template<typename... Types>
//std::tuple<Types...> unpack(void* res_vector, int argc, char** argv, char** col_name) {
//	auto item = unpack_helper<0, Types...>(res_vector, argc, argv, col_name);
//	(static_cast<std::vector<std::tuple<Types...>>*>(res_vector))->push_back(item);
//}

//template<typename T>
//T::tuple_type unpack_object(void* res_vector, int argc, char** argv, char** col_name) {
//	//using std::tuple<Types...> = T::tuple_type;
//	auto item = unpack_helper<Types...>(res_vector, argc, argv, col_name);
//	(static_cast<std::vector<std::tuple<Types...>>*>(res_vector))->push_back(item);
//}

//template<typename T>
//T unpack(void* res_vector, int argc, char** argv, char** col_name) {
//	auto item = unpack_helper<Types...>(res_vector, argc, argv, col_name);
//	(static_cast<std::vector<std::tuple<Types...>>*>(res_vector))->push_back(item);
//}

bool handle_error(int error_code, char* error_message) {
	if (error_code != SQLITE_OK) {
		std::cerr << "SQL Error: " << error_message << std::endl;
		sqlite3_free(error_message);
		return false;
	}
	return true;
}

bool DatabaseManager::open(const std::wstring& local_db_file_path, const std::wstring& global_db_file_path) {

	std::string local_database_file_path_utf8 = utf8_encode(local_db_file_path);
	int local_rc = sqlite3_open(local_database_file_path_utf8.c_str(), &local_db);

	if (local_rc) {
		std::cerr << "could not create local database" << sqlite3_errmsg(local_db) << std::endl;
		local_db = nullptr;
		global_db = nullptr;
		return false;
	}

	if (local_db_file_path != global_db_file_path) {
		std::string global_database_file_path_utf8 = utf8_encode(global_db_file_path);
		int global_rc = sqlite3_open(global_database_file_path_utf8.c_str(), &global_db);

		if (global_rc) {
			std::cerr << "could not create global database" << sqlite3_errmsg(global_db) << std::endl;
			local_db = nullptr;
			global_db = nullptr;
			return false;
		}
	}
	else {
		global_db = local_db;
	}

	return true;
}

bool DatabaseManager::create_opened_books_table() {

	const char* create_opened_books_sql = "CREATE TABLE IF NOT EXISTS opened_books ("\
	"id INTEGER PRIMARY KEY AUTOINCREMENT,"\
	"path TEXT UNIQUE,"\
	"zoom_level REAL,"\
	"offset_x REAL,"\
	"offset_y REAL,"\
	"last_access_time TEXT);";

	char* error_message = nullptr;
	int error_code = sqlite3_exec(global_db, create_opened_books_sql, null_callback, 0, &error_message);
	return handle_error(error_code, error_message);
}

bool DatabaseManager::create_marks_table() {
	const char* create_marks_sql = "CREATE TABLE IF NOT EXISTS marks ("\
	"id INTEGER PRIMARY KEY AUTOINCREMENT," \
	"document_path TEXT,"\
	"symbol CHAR,"\
	"offset_y real,"\
	"UNIQUE(document_path, symbol));";

	char* error_message = nullptr;
	int error_code = sqlite3_exec(global_db, create_marks_sql, null_callback, 0, &error_message);
	return handle_error(
		error_code,
		error_message);
}

bool DatabaseManager::create_bookmarks_table() {
	const char* create_bookmarks_sql = "CREATE TABLE IF NOT EXISTS bookmarks ("\
		"id INTEGER PRIMARY KEY AUTOINCREMENT," \
		"document_path TEXT,"\
		"desc TEXT,"\
		"offset_y real);";

	char* error_message = nullptr;
	int error_code = sqlite3_exec(global_db, create_bookmarks_sql, null_callback, 0, &error_message);
	return handle_error(
		error_code,
		error_message);
}

bool DatabaseManager::create_highlights_table() {
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
	int error_code = sqlite3_exec(global_db, create_highlights_sql, null_callback, 0, &error_message);
	return handle_error(
		error_code,
		error_message);
}

bool DatabaseManager::create_document_hash_table() {
	const char* create_document_hash_sql = "CREATE TABLE IF NOT EXISTS document_hash ("\
		"id INTEGER PRIMARY KEY AUTOINCREMENT," \
		"path TEXT,"\
		"hash TEXT,"\
		"UNIQUE(path, hash));";

	char* error_message = nullptr;
	int error_code = sqlite3_exec(local_db, create_document_hash_sql, null_callback, 0, &error_message);
	return handle_error(
		error_code,
		error_message);
}

bool DatabaseManager::create_links_table() {
	const char* create_marks_sql = "CREATE TABLE IF NOT EXISTS links ("\
		"id INTEGER PRIMARY KEY AUTOINCREMENT," \
		"src_document TEXT,"\
		"dst_document TEXT,"\
		"src_offset_y REAL,"\
		"dst_offset_x REAL,"\
		"dst_offset_y REAL,"\
		"dst_zoom_level REAL);";

	char* error_message = nullptr;
	int error_code = sqlite3_exec(global_db, create_marks_sql, null_callback, 0, &error_message);
	return handle_error(
		error_code,
		error_message);
}

//bool DatabaseManager::insert_book(const std::string& path, float zoom_level, float offset_x, float offset_y) {
//	const char* insert_books_sql = ""\
//		"INSERT INTO opened_books (PATH, zoom_level, offset_x, offset_y, last_access_time) VALUES ";
//
//	std::wstringstream ss;
//	ss << insert_books_sql << "'" << esc(path) << "', " << zoom_level << ", " << offset_x << ", " << offset_y << ", datetime('now');";
//	
//	char* error_message = nullptr;
//	int error_code = sqlite3_exec(global_db, utf8_encode(ss.str()).c_str(), null_callback, 0, &error_message);
//	return handle_error(
//		error_code,
//		error_message);
//}

bool DatabaseManager::insert_document_hash(const std::wstring& path, const std::string& checksum){

	const char* delete_doc_sql = ""\
		"DELETE FROM document_hash WHERE path=";

	const char* insert_doc_hash_sql = ""\
		"INSERT INTO document_hash (path, hash) VALUES (";

	std::wstringstream insert_ss;
	insert_ss << insert_doc_hash_sql << "'" << esc(path) << "', '" << esc(checksum) << "');";

	std::wstringstream delete_ss;
	delete_ss << delete_doc_sql << "'" << esc(path) << "';";

	char* delete_error_message = nullptr;
	int delete_error_code = sqlite3_exec(local_db, utf8_encode(delete_ss.str()).c_str(), null_callback, 0, &delete_error_message);
	handle_error(delete_error_code, delete_error_message);
	
	char* insert_error_message = nullptr;
	int insert_error_code = sqlite3_exec(local_db, utf8_encode(insert_ss.str()).c_str(), null_callback, 0, &insert_error_message);
	return handle_error(insert_error_code, insert_error_message);
}

bool DatabaseManager::update_book(const std::string& path, float zoom_level, float offset_x, float offset_y) {

	std::wstringstream ss;
	ss << "insert or replace into opened_books(path, zoom_level, offset_x, offset_y, last_access_time) values ('" <<
		esc(path) << "', " << zoom_level << ", " << offset_x << ", " << offset_y << ", datetime('now'));";

	char* error_message = nullptr;
	int error_code = sqlite3_exec(global_db, utf8_encode(ss.str()).c_str(), null_callback, 0, &error_message);
	return handle_error(
		error_code,
		error_message);
}

bool DatabaseManager::insert_mark(const std::string& document_path, char symbol, float offset_y) {

	//todo: probably should escape symbol too
	std::wstringstream ss;
	ss << "INSERT INTO marks (document_path, symbol, offset_y) VALUES ('" << esc(document_path) << "', '" << symbol << "', " << offset_y << ");";
	char* error_message = nullptr;

	int error_code = sqlite3_exec(global_db, utf8_encode(ss.str()).c_str(), null_callback, 0, &error_message);
	return handle_error(
		error_code,
		error_message);
}

bool DatabaseManager::delete_mark_with_symbol(char symbol) {

	std::wstringstream ss;
	ss << "DELETE FROM marks where symbol='" << symbol << "';";
	char* error_message = nullptr;

	int error_code = sqlite3_exec(global_db, utf8_encode(ss.str()).c_str(), null_callback, 0, &error_message);
	return handle_error(
		error_code,
		error_message);
}

bool DatabaseManager::insert_bookmark(const std::string& document_path, const std::wstring& desc, float offset_y) {

	std::wstringstream ss;
	ss << "INSERT INTO bookmarks (document_path, desc, offset_y) VALUES ('" << esc(document_path) << "', '" << esc(desc) << "', " << offset_y << ");";
	char* error_message = nullptr;

	int error_code = sqlite3_exec(global_db, utf8_encode(ss.str()).c_str(), null_callback, 0, &error_message);
	return handle_error(
		error_code,
		error_message);
}

bool DatabaseManager::insert_highlight(const std::string& document_path,
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

	int error_code = sqlite3_exec(global_db, utf8_encode(ss.str()).c_str(), null_callback, 0, &error_message);
	return handle_error(
		error_code,
		error_message);
}

bool DatabaseManager::insert_portal(const std::string& src_document_path, const std::string& dst_document_path, float dst_offset_x, float dst_offset_y, float dst_zoom_level, float src_offset_y) {

	std::wstringstream ss;
	ss << "INSERT INTO links (src_document, dst_document, src_offset_y, dst_offset_x, dst_offset_y, dst_zoom_level) VALUES ('" <<
		esc(src_document_path) << "', '" << esc(dst_document_path) << "', " << src_offset_y << ", " << dst_offset_x << ", " << dst_offset_y << ", " << dst_zoom_level << ");";
	char* error_message = nullptr;

	int error_code = sqlite3_exec(global_db, utf8_encode(ss.str()).c_str(), null_callback, 0, &error_message);
	return handle_error(
		error_code,
		error_message);
}

bool DatabaseManager::update_portal(const std::string& src_document_path, float dst_offset_x, float dst_offset_y, float dst_zoom_level, float src_offset_y) {

	std::wstringstream ss;
	ss << "UPDATE links SET dst_offset_x=" << dst_offset_x << ", dst_offset_y=" << dst_offset_y <<
		", dst_zoom_level=" << dst_zoom_level << " WHERE src_document='" <<
		esc(src_document_path) << "' AND abs(src_offset_y-(" << src_offset_y << ")) < 0.01;";
	char* error_message = nullptr;

	int error_code = sqlite3_exec(global_db, utf8_encode(ss.str()).c_str(), null_callback, 0, &error_message);
	return handle_error(
		error_code,
		error_message);
}

bool DatabaseManager::delete_link(const std::string& src_document_path, float src_offset_y) {

	std::wstringstream ss;
	ss << "DELETE FROM links where src_document='" << esc(src_document_path) << "'AND abs(src_offset_y-(" << src_offset_y << ")) < 0.01;";
	char* error_message = nullptr;

	int error_code = sqlite3_exec(global_db, utf8_encode(ss.str()).c_str(), null_callback, 0, &error_message);
	return handle_error(
		error_code,
		error_message);
}

bool DatabaseManager::delete_bookmark(const std::string& src_document_path, float src_offset_y) {

	std::wstringstream ss;
	ss << "DELETE FROM bookmarks where document_path='" << esc(src_document_path) << "'AND abs(offset_y-(" << src_offset_y << ")) < 0.01;";
	char* error_message = nullptr;

	int error_code = sqlite3_exec(global_db, utf8_encode(ss.str()).c_str(), null_callback, 0, &error_message);
	return handle_error(
		error_code,
		error_message);
}

bool DatabaseManager::delete_highlight(const std::string& src_document_path, float begin_x, float begin_y, float end_x, float end_y) {

	std::wstringstream ss;
	std::wstring threshold = QString::number(HIGHLIGHT_DELETE_THRESHOLD).toStdWString();
	ss << std::setprecision(10) << "DELETE FROM highlights where document_path='" << esc(src_document_path) <<
		"'AND abs(begin_x-(" << begin_x << ")) < " << threshold <<
		" AND abs(begin_y-(" << begin_y << ")) < " << threshold <<
		" AND abs(end_x-(" << end_x << ")) < " << threshold << 
		" AND abs(end_y-(" << end_y << ")) < " << threshold ;
	char* error_message = nullptr;

	if (DEBUG) {
		std::wcout << ss.str() << L"\n";
	}

	int error_code = sqlite3_exec(global_db, utf8_encode(ss.str()).c_str(), null_callback, 0, &error_message);
	return handle_error(
		error_code,
		error_message);
}

bool DatabaseManager::update_mark(const std::string& document_path, char symbol, float offset_y) {

	std::wstringstream ss;
	ss << "UPDATE marks set offset_y=" << offset_y << " where document_path='" << esc(document_path) << "' AND symbol='" << symbol << "';";

	char* error_message = nullptr;
	int error_code = sqlite3_exec(global_db, utf8_encode(ss.str()).c_str(), null_callback, 0, &error_message);
	return handle_error(
		error_code,
		error_message);
}


bool DatabaseManager::select_opened_book(const std::string& book_path, std::vector<OpenedBookState> &out_result) {
		std::wstringstream ss;
		ss << "select zoom_level, offset_x, offset_y from opened_books where path='" << esc(book_path) << "'";
		char* error_message = nullptr;
		int error_code = sqlite3_exec(global_db, utf8_encode(ss.str()).c_str(), opened_book_callback, &out_result, &error_message);
		return handle_error(
			error_code,
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

bool DatabaseManager::delete_opened_book(const std::string& book_path) {
		std::wstringstream ss;
		ss << "DELETE FROM opened_books where path='" << esc(book_path) << "'";
		char* error_message = nullptr;
		int error_code = sqlite3_exec(global_db, utf8_encode(ss.str()).c_str(), null_callback, 0, &error_message);
		return handle_error(
			error_code,
			error_message);
}


bool DatabaseManager::select_opened_books_path_values( std::vector<std::wstring> &out_result) {
		std::wstringstream ss;
		ss << "SELECT path FROM opened_books order by datetime(last_access_time) desc;";
		char* error_message = nullptr;
		int error_code = sqlite3_exec(global_db, utf8_encode(ss.str()).c_str(), prev_doc_callback, &out_result, &error_message);
		return handle_error(
			error_code,
			error_message);
}

//bool DatabaseManager::select_opened_books_hashes_and_names(std::vector<std::pair<std::wstring, std::wstring>> &out_result) {
//	std::vector<std::wstring> hashes;
//	select_opened_books_path_values(hashes);
//	for (const auto& hash : hashes) {
//		std::vector<std::wstring> paths;
//		get_path_from_hash(utf8_encode(hash), paths);
//		if (paths.size() > 0) {
//			out_result.push_back(std::make_pair(hash, paths.back()));
//		}
//	}
//	return true;
//	//this->select_
//	//this->sele
//		//std::wstringstream ss;
//		//ss << "SELECT opened_books.path, document_hash.path FROM opened_books, document_hash where opened_books.path=document_hash.hash order by datetime(opened_books.last_access_time) desc;";
//		//char* error_message = nullptr;
//		//int error_code = sqlite3_exec(db, utf8_encode(ss.str()).c_str(), wstring_pair_select_callback, &out_result, &error_message);
//		//return handle_error(
//		//	error_code,
//		//	error_message);
//}

bool DatabaseManager::select_mark(const std::string& book_path, std::vector<Mark> &out_result) {
		std::wstringstream ss;
		ss << "select symbol, offset_y from marks where document_path='" << esc(book_path) << "';";

		char* error_message = nullptr;
		int error_code = sqlite3_exec(global_db, utf8_encode(ss.str()).c_str(), mark_select_callback, &out_result, &error_message);
		return handle_error(
			error_code,
			error_message);
}

bool DatabaseManager::select_global_mark(char symbol, std::vector<std::pair<std::string, float>> &out_result) {
		std::wstringstream ss;
		ss << "select document_path, offset_y from marks where symbol='" << symbol << "';";

		char* error_message = nullptr;
		int error_code = sqlite3_exec(global_db, utf8_encode(ss.str()).c_str(), global_mark_select_callback, &out_result, &error_message);
		return handle_error(
			error_code,
			error_message);
}

bool DatabaseManager::select_bookmark(const std::string& book_path, std::vector<BookMark> &out_result) {
		std::wstringstream ss;
		ss << "select desc, offset_y from bookmarks where document_path='" << esc(book_path) << "';";

		char* error_message = nullptr;
		int error_code = sqlite3_exec(global_db, utf8_encode(ss.str()).c_str(), bookmark_select_callback, &out_result, &error_message);
		return handle_error(
			error_code,
			error_message);
}

bool DatabaseManager::get_path_from_hash(const std::string& checksum, std::vector<std::wstring> &out_paths){
		std::wstringstream ss;
		ss << "select path from document_hash where hash='" << esc(checksum) << "';";

		char* error_message = nullptr;
		int error_code = sqlite3_exec(local_db, utf8_encode(ss.str()).c_str(), wstring_select_callback, &out_paths, &error_message);
		return handle_error(
			error_code,
			error_message);
}

bool DatabaseManager::get_hash_from_path(const std::string& path, std::vector<std::wstring> &out_checksum){
		std::wstringstream ss;
		ss << "select hash from document_hash where path='" << esc(path) << "';";

		char* error_message = nullptr;
		int error_code = sqlite3_exec(local_db, utf8_encode(ss.str()).c_str(), wstring_select_callback, &out_checksum, &error_message);
		return handle_error(
			error_code,
			error_message);
}

bool DatabaseManager::get_prev_path_hash_pairs(std::vector<std::pair<std::wstring, std::wstring>> &out_pairs){
		std::wstringstream ss;
		ss << "select path, hash from document_hash;";

		char* error_message = nullptr;
		int error_code = sqlite3_exec(local_db, utf8_encode(ss.str()).c_str(), wstring_pair_select_callback, &out_pairs, &error_message);
		return handle_error(
			error_code,
			error_message);
}

bool DatabaseManager::select_highlight(const std::string& book_path, std::vector<Highlight> &out_result) {
		std::wstringstream ss;
		ss << "select desc, begin_x, begin_y, end_x, end_y, type from highlights where document_path='" << esc(book_path) << "';";

		char* error_message = nullptr;
		int error_code = sqlite3_exec(global_db, utf8_encode(ss.str()).c_str(), highlight_select_callback, &out_result, &error_message);
		return handle_error(
			error_code,
			error_message);
}

bool DatabaseManager::select_highlight_with_type(const std::string& book_path, char type, std::vector<Highlight> &out_result) {
		std::wstringstream ss;
		ss << "select desc, begin_x, begin_y, end_x, end_y, type from highlights where document_path='" << esc(book_path) << "' AND type='" << type << "';";

		char* error_message = nullptr;
		int error_code = sqlite3_exec(global_db, utf8_encode(ss.str()).c_str(), highlight_select_callback, &out_result, &error_message);
		return handle_error(
			error_code,
			error_message);
}

bool DatabaseManager::global_select_highlight(std::vector<std::pair<std::string, Highlight>> &out_result) {
		std::wstringstream ss;
		ss << "select document_path, desc, type, begin_x, begin_y, end_x, end_y from highlights;";

		char* error_message = nullptr;
		int error_code = sqlite3_exec(global_db, utf8_encode(ss.str()).c_str(), global_highlight_select_callback, &out_result, &error_message);
		return handle_error(
			error_code,
			error_message);
}

bool DatabaseManager::global_select_bookmark(std::vector<std::pair<std::string, BookMark>> &out_result) {
		std::wstringstream ss;
		ss << "select document_path, desc, offset_y from bookmarks;";

		char* error_message = nullptr;
		int error_code = sqlite3_exec(global_db, utf8_encode(ss.str()).c_str(), global_bookmark_select_callback, &out_result, &error_message);
		return handle_error(
			error_code,
			error_message);
}

bool DatabaseManager::select_links(const std::string& src_document_path, std::vector<Portal> &out_result) {
		std::wstringstream ss;
		ss << "select dst_document, src_offset_y, dst_offset_x, dst_offset_y, dst_zoom_level from links where src_document='" << esc(src_document_path) << "';";

		char* error_message = nullptr;
		int error_code = sqlite3_exec(global_db, utf8_encode(ss.str()).c_str(), link_select_callback, &out_result, &error_message);
		return handle_error(
			error_code,
			error_message);
}

void DatabaseManager::create_tables() {
	create_opened_books_table();
	create_marks_table();
	create_bookmarks_table();
	create_highlights_table();
	create_links_table();
	create_document_hash_table();
}

bool update_string_value(sqlite3* db,
	const std::wstring& table_name,
	const std::wstring& field_name,
	const std::wstring& old_value,
	const std::wstring& new_value) {

	std::wstringstream ss;
	ss << "UPDATE " << table_name << " set " << field_name << "='" << esc(new_value) << "' where " << field_name << "='" << esc(old_value) << "';";

	char* error_message = nullptr;
	int error_code = sqlite3_exec(db, utf8_encode(ss.str()).c_str(), null_callback, 0, &error_message);
	return handle_error(
		error_code,
		error_message);

}
bool update_mark_path(sqlite3* db, const std::wstring& path, const std::wstring& new_path) {
	return update_string_value(db, L"marks", L"document_path", path, new_path);
}

bool update_opened_book_path(sqlite3* db, const std::wstring& path, const std::wstring& new_path) {
	return update_string_value(db, L"opened_books", L"path", path, new_path);
}

bool update_bookmark_path(sqlite3* db, const std::wstring& path, const std::wstring& new_path) {
	return update_string_value(db, L"bookmarks", L"document_path", path, new_path);
}

bool update_highlight_path(sqlite3* db, const std::wstring& path, const std::wstring& new_path) {
	return update_string_value(db, L"highlights", L"document_path", path, new_path);
}

bool update_portal_path(sqlite3* db, const std::wstring& path, const std::wstring& new_path) {
	return update_string_value(db, L"links", L"src_document", path, new_path) && update_string_value(db, L"links", L"dst_document", path, new_path);
}

void DatabaseManager::upgrade_database_hashes() {
	CachedChecksummer checksummer({});

	std::vector<std::wstring> prev_doc_paths;
	select_opened_books_path_values(prev_doc_paths);

	for (const auto& doc_path : prev_doc_paths) {
		std::string checksum = checksummer.get_checksum(doc_path);
		if (checksum.size() > 0) {
			std::wstring uchecksum = utf8_decode(checksum);
			insert_document_hash(doc_path, checksum);

			//update_mark_path(local_db, doc_path, uchecksum);
			//update_bookmark_path(local_db, doc_path, uchecksum);
			//update_highlight_path(local_db, doc_path, uchecksum);
			//update_portal_path(local_db, doc_path, uchecksum);
			//update_opened_book_path(local_db, doc_path, uchecksum);
		}
	}
}

void DatabaseManager::split_database(const std::wstring& local_database_path, const std::wstring& global_database_path, bool was_using_hashes) {

	//we should only split when we have the same local and global database
	assert(local_db == global_db);

	// ---------------------- EXPORT PREVIOUS DATABASE ----------------------------
	std::vector<std::pair<std::wstring, std::wstring>> path_hash;
	get_prev_path_hash_pairs(path_hash);
	std::vector<std::pair<std::string, OpenedBookState>> opened_book_states;
	std::vector<std::pair<std::string, Mark>> marks;
	std::vector<std::pair<std::string, BookMark>> bookmarks;
	std::vector<std::pair<std::string, Highlight>> highlights;
	std::vector<std::pair<std::string, Portal>> portals;

	std::unordered_map<std::wstring, std::wstring> path_to_hash;

	for (const auto& [path, hash] : path_hash) {
		path_to_hash[path] = hash;
	}

	for (const auto &[path, hash_] : path_hash) {
		std::string hash = utf8_encode(hash_);
		std::string key;
		if (was_using_hashes) {
			key = hash;
		}
		else {
			key = utf8_encode(path);
		}
		std::vector<OpenedBookState> current_book_state;
		std::vector<Mark> current_marks;
		std::vector<BookMark> current_bookmarks;
		std::vector<Highlight> current_highlights;
		std::vector<Portal> current_portals;

		select_opened_book(key, current_book_state);
		if (current_book_state.size() > 0) {
			opened_book_states.push_back(std::make_pair(hash, current_book_state[0]));
		}

		select_mark(key, current_marks);
		for (const auto& mark : current_marks) {
			marks.push_back(std::make_pair(hash, mark));
		}

		select_bookmark(key, current_bookmarks);
		for (const auto& bookmark : current_bookmarks) {
			bookmarks.push_back(std::make_pair(hash, bookmark));
		}

		select_highlight(key, current_highlights);
		for (const auto& highlight : current_highlights) {
			highlights.push_back(std::make_pair(hash, highlight));
		}

		select_links(key, current_portals);
		for (auto portal : current_portals) {
			if (!was_using_hashes) {
				if (path_to_hash.find(utf8_decode(portal.dst.document_checksum)) == path_to_hash.end()) {
					continue;
				}
				else {
					portal.dst.document_checksum = utf8_encode(path_to_hash[utf8_decode(portal.dst.document_checksum)]);
				}
			}

			portals.push_back(std::make_pair(hash, portal));
		}
	}
	sqlite3_close(local_db);


	// ---------------------- CREATE NEW DATABASE FILES ----------------------------

	if (!open(local_database_path, global_database_path)) {
		return;
	}

	// ---------------------- IMPORT PREVIOUS DATA ----------------------------
	create_tables();
	for (const auto &[path, hash] : path_hash) {
		insert_document_hash(path, utf8_encode(hash));
	}

	for (const auto &[hash, book_state] : opened_book_states) {
		update_book(hash, book_state.zoom_level, book_state.offset_x, book_state.offset_y);
	}
	for (const auto &[hash, mark] : marks) {
		insert_mark(hash, mark.symbol, mark.y_offset);
	}
	for (const auto &[hash, bookmark] : bookmarks) {
		insert_bookmark(hash, bookmark.description, bookmark.y_offset);
	}
	for (const auto &[hash, highlight] : highlights) {
		insert_highlight(
			hash,
			highlight.description,
			highlight.selection_begin.x,
			highlight.selection_begin.y,
			highlight.selection_end.x,
			highlight.selection_end.y,
			highlight.type
		);
	}
	for (const auto &[hash, portal] : portals) {
		insert_portal(
			hash,
			portal.dst.document_checksum,
			portal.dst.book_state.offset_x,
			portal.dst.book_state.offset_y,
			portal.dst.book_state.zoom_level,
			portal.src_offset_y
		);
	}

}

template<typename T>
QJsonArray export_array(std::vector<T> objects) {
	QJsonArray res;

	for (const T& obj : objects) {
		res.append(obj.to_json());
	}
	return res;
}


template <typename T>
std::vector<T> load_from_json_array(const QJsonArray& item_list) {

	std::vector<T> res;

	for (int i = 0; i < item_list.size(); i++) {
		QJsonObject current_json_object = item_list.at(i).toObject();
		T current_object;
		current_object.from_json(current_json_object);
		res.push_back(current_object);
	}
	return res;
}


void DatabaseManager::export_json(std::wstring json_file_path, CachedChecksummer* checksummer) {

	std::set<std::string> seen_checksums;

	std::vector<std::wstring> prev_doc_checksums;

	select_opened_books_path_values(prev_doc_checksums);

	QJsonArray document_data_array;

	for (size_t i = 0; i < prev_doc_checksums.size(); i++) {

		const auto& document_checksum = utf8_encode(prev_doc_checksums[i]);
		std::optional<std::wstring> path = checksummer->get_path(document_checksum);

		if ((!path) || (document_checksum.size() == 0) || (seen_checksums.find(document_checksum) != seen_checksums.end())) {
			continue;
		}

		std::vector<BookMark> bookmarks;
		std::vector<Highlight> highlights;
		std::vector<Mark> marks;
		std::vector<Portal> portals;
		std::vector<OpenedBookState> opened_book_state_;

		select_opened_book(document_checksum, opened_book_state_);
		if (opened_book_state_.size() != 1) {
			continue;
		}

		OpenedBookState opened_book_state = opened_book_state_[0];

		select_bookmark(document_checksum, bookmarks);
		select_mark(document_checksum, marks);
		select_highlight(document_checksum, highlights);
		select_links(document_checksum, portals);


		QJsonArray json_bookmarks = export_array(bookmarks);
		QJsonArray json_highlights = export_array(highlights);
		QJsonArray json_marks = export_array(marks);
		QJsonArray json_portals = export_array(portals);

		QJsonObject book_object;
		book_object["offset_x"] = opened_book_state.offset_x;
		book_object["offset_y"] = opened_book_state.offset_y;
		book_object["zoom_level"] = opened_book_state.zoom_level;
		book_object["checksum"] = QString::fromStdString(document_checksum);
		book_object["path"] = QString::fromStdWString(path.value());
		book_object["bookmarks"] = json_bookmarks;
		book_object["marks"] = json_marks;
		book_object["highlights"] = json_highlights;
		book_object["portals"] = json_portals;

		document_data_array.append(std::move(book_object));

		seen_checksums.insert(document_checksum);
	}

	QJsonObject exported_json;
	exported_json["documents"] = std::move(document_data_array);

	QJsonDocument json_document(exported_json);


	QFile output_file(QString::fromStdWString(json_file_path));
	output_file.open(QFile::WriteOnly);
	output_file.write(json_document.toJson());
	output_file.close();
}

template<typename T>
std::vector<T> get_new_elements(const std::vector<T>& prev_elements, const std::vector<T>& new_elements) {
	std::vector<T> res;
	for (const auto& new_elem : new_elements) {
		bool is_new = true;
		for (const auto& prev_elem : prev_elements) {
			if (new_elem == prev_elem) {
				is_new = false;
				break;
			}
		}
		if (is_new) {
			res.push_back(new_elem);
		}
	}
	return res;
}

void DatabaseManager::import_json(std::wstring json_file_path, CachedChecksummer* checksummer) {

	QFile json_file(QString::fromStdWString(json_file_path));
	json_file.open(QFile::ReadOnly);
	QJsonDocument json_document = QJsonDocument().fromJson(json_file.readAll());
	json_file.close();

	QJsonObject imported_json = json_document.object();
	QJsonArray documents_json_array = imported_json.value("documents").toArray();

	//std::vector<JsonDocumentData> imported_documents;

	for (int i = 0; i < documents_json_array.size(); i++) {

		QJsonObject current_json_doc = documents_json_array.at(i).toObject();

		std::string checksum = current_json_doc["checksum"].toString().toStdString();
		//std::wstring path = current_json_doc["path"].toString().toStdWString();
		float offset_x = current_json_doc["offset_x"].toDouble();
		float offset_y = current_json_doc["offset_y"].toDouble();
		float zoom_level = current_json_doc["zoom_level"].toDouble();

		auto bookmarks = std::move(load_from_json_array<BookMark>(current_json_doc["bookmarks"].toArray()));
		auto marks = load_from_json_array<Mark>(current_json_doc["marks"].toArray());
		auto highlights = load_from_json_array<Highlight>(current_json_doc["highlights"].toArray());
		auto portals = load_from_json_array<Portal>(current_json_doc["portals"].toArray());

		std::vector<BookMark> prev_bookmarks;
		std::vector<Mark> prev_marks;
		std::vector<Highlight> prev_highlights;
		std::vector<Portal> prev_portals;


		select_bookmark(checksum, prev_bookmarks);
		select_mark(checksum, prev_marks);
		select_highlight(checksum, prev_highlights);
		select_links(checksum, prev_portals);

		std::vector<BookMark> new_bookmarks = get_new_elements(prev_bookmarks, bookmarks);
		std::vector<Mark> new_marks = get_new_elements(prev_marks, marks);
		std::vector<Highlight> new_highlights = get_new_elements(prev_highlights, highlights);
		std::vector<Portal> new_portals = get_new_elements(prev_portals, portals);

		std::optional<std::wstring> path = checksummer->get_path(checksum);

		if (path) {
			//update_book(db, path.value(), zoom_level, offset_x, offset_y);
			update_book(checksum, zoom_level, offset_x, offset_y);
		}

		for (const auto& bm : new_bookmarks) {
			insert_bookmark(checksum, bm.description, bm.y_offset);
		}
		for (const auto& mark : new_marks) {
			insert_mark(checksum, mark.symbol, mark.y_offset);
		}
		for (const auto& hl : new_highlights) {
			insert_highlight(checksum,
				hl.description,
				hl.selection_begin.x,
				hl.selection_begin.y,
				hl.selection_end.x,
				hl.selection_end.y,
				hl.type);
		}
		for (const auto& portal : new_portals) {
			insert_portal(checksum,
				portal.dst.document_checksum,
				portal.dst.book_state.offset_x,
				portal.dst.book_state.offset_y,
				portal.dst.book_state.zoom_level,
				portal.src_offset_y);
		}

	}
}

std::string create_select_query(std::string table_name,
	std::vector<std::string> selections,
	std::unordered_map<std::string,std::variant<std::wstring, std::string, int, char, float>> values) {
	std::wstringstream ss;

	ss << L"SELECT ";
	for (size_t i = 0; i < selections.size(); i++) {
		ss << utf8_decode(selections[i]);
		if ((size_t) i < (selections.size() - 1)) {
			ss << ", ";
		}
	}
	ss << " FROM " << utf8_decode(table_name) << " WHERE ";

	int index = 0;
	for (const auto& [key, value] : values) {

		std::wstring ukey = utf8_decode(key);

		if (std::holds_alternative<std::wstring>(value)) {
			ss << ukey << L"='" << esc(std::get<std::wstring>(value)) << L"'";
		}

		if (std::holds_alternative<std::string>(value)) {
			ss << ukey << L"='" << esc(std::get<std::string>(value)) << L"'";
		}

		if (std::holds_alternative<char>(value)) {
			ss << ukey << L"='" << std::get<char>(value) << L"'";
		}

		if (std::holds_alternative<int>(value)) {
			ss << ukey << L"=" << std::get<int>(value) << L"";
		}

		if (std::holds_alternative<float>(value)) {
			ss << ukey << L"=" << std::get<float>(value) << L"";
		}

		index++;
		if ((size_t) index != values.size()) {
			ss << ", ";
		}
	}
	ss << L";";
	return utf8_encode(ss.str());
}

void DatabaseManager::ensure_database_compatibility(const std::wstring& local_db_file_path, const std::wstring& global_db_file_path) {
	create_tables();

	// if the database is still using absolute paths instead of checksums, update all paths to checksums
	std::vector<std::pair<std::wstring, std::wstring>> prev_path_hash_pairs;
	get_prev_path_hash_pairs(prev_path_hash_pairs);
	bool was_using_hashes = true;

	if (prev_path_hash_pairs.size() == 0) {
		was_using_hashes = false;
		upgrade_database_hashes();
	}

	//if we are still using a single database file instead of separate local and global database files, split the database.
	if (local_db == global_db) {
		split_database(local_db_file_path, global_db_file_path, was_using_hashes);
	}
}
