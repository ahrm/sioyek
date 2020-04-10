#pragma once

#include <string>
#include <mupdf/fitz.h>

using namespace std;

class DocumentView;

struct BookState {
	string document_path;
	float offset_y;
};

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

struct PdfLink {
	fz_rect rect;
	string uri;
};

struct DocumentViewState {
	DocumentView* document_view;
	float offset_x;
	float offset_y;
	float zoom_level;
};
