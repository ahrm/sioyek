#pragma once

#include <iostream>
#include <vector>
#include <iostream>
#include <string>
#include "sqlite3.h"
#include "book.h"

using namespace std;

bool create_opened_books_table(sqlite3* db);
bool create_marks_table(sqlite3* db);
bool select_opened_book(sqlite3* db, string book_path, vector<OpenedBookState>& out_result);
bool insert_mark(sqlite3* db, string document_path, char symbol, float offset_y);
bool update_mark(sqlite3* db, string document_path, char symbol, float offset_y);
bool update_book(sqlite3* db, string path, float zoom_level, float offset_x, float offset_y);
bool select_mark(sqlite3* db, string book_path, vector<Mark>& out_result);
bool create_bookmarks_table(sqlite3* db);
bool insert_bookmark(sqlite3* db, string document_path, string desc, float offset_y);
bool select_bookmark(sqlite3* db, string book_path, vector<BookMark>& out_result);
