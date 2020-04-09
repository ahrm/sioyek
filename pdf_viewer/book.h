#pragma once

#include <string>
using namespace std;

class DocumentView;

struct OpenedBookState {
	float zoom_level;
	float offset_x;
	float offset_y;
};

struct Mark {
	float y_offset;
	char symbol;
};

struct BookMark {
	float y_offset;
	string description;
};

struct Link {
	string document_path;
	float dest_offset_x;
	float dest_offset_y;
	float src_offset_y;
};

struct DocumentViewState {
	DocumentView* document_view;
	float offset_x;
	float offset_y;
	float zoom_level;
};
