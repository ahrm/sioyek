#pragma once

#include <vector>
#include <string>
#include <mupdf/fitz.h>
#include <gl/glew.h>

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

struct SearchResult {
	fz_quad quad;
	int page;
};

struct RenderRequest {
	//fz_document* doc;
	string path;
	int page;
	float zoom_level;
};

struct RenderResponse {
	RenderRequest request;
	unsigned int last_access_time;
	fz_pixmap* pixmap;
	GLuint texture;
};

struct TocNode {
	vector<TocNode*> children;
	string title;
	int page;
	float y;
	float x;
};

bool operator==(const RenderRequest& lhs, const RenderRequest& rhs);

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
