#pragma once

#include <vector>
#include <string>
#include <mupdf/fitz.h>
//#include <gl/glew.h>
#include <qopengl.h>
#include <mutex>
#include <variant>


class DocumentView;

struct BookState {
	std::wstring document_path;
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
	std::wstring description;
};

struct Link {
	static Link with_src_offset(float src_offset);

	std::wstring document_path;
	float dest_offset_x;
	float dest_offset_y;
	float dest_zoom_level;
	float src_offset_y;
};

struct PdfLink {
	fz_rect rect;
	std::string uri;
};

struct DocumentViewState {
	std::wstring document_path;
	OpenedBookState book_state;
};

bool operator==(DocumentViewState& lhs, const DocumentViewState& rhs);

struct SearchResult {
	fz_rect rect;
	int page;
};


struct TocNode {
	std::vector<TocNode*> children;
	std::wstring title;
	int page;

	float y;
	float x;
};


class Document;

struct CachedPageData {
	Document* doc;
	int page;
	float zoom_level;
};

struct CachedPage {
	CachedPageData cached_page_data;
	fz_pixmap* cached_page_pixmap;
	unsigned int last_access_time;
	GLuint cached_page_texture;
};
bool operator==(const CachedPageData& lhs, const CachedPageData& rhs);


struct PdfPortal {
	fz_rect absolute_document_rect;
	Document* doc;
};

struct FigureData {
	int page;
	float y_offset;
	std::wstring text;
};
