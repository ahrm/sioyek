#pragma once

#include <vector>
#include <string>
#include <mupdf/fitz.h>
//#include <gl/glew.h>
#include <qopengl.h>
#include <mutex>
#include <variant>
#include <qjsonobject.h>

#include "coordinates.h"

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

/*
	A mark is a location in the document labeled with a symbol (which is a single character [a-z]). For example
	we can mark a location with symbol 'a' and later return to that location by going to the mark named 'a'.
	Lower case marks are local to the document and upper case marks are global.
*/
struct Mark {
	float y_offset;
	char symbol;

	QJsonObject to_json() const;
	void from_json(const QJsonObject& json_object);
};

/*
	A bookmark is similar to mark but instead of being indexed by a symbol, it has a description.
*/
struct BookMark {
	float y_offset;
	std::wstring description;

	QJsonObject to_json() const;
	void from_json(const QJsonObject& json_object);
};

struct Highlight {
	AbsoluteDocumentPos selection_begin;
	AbsoluteDocumentPos selection_end;
	std::wstring description;
	char type;
	std::vector<fz_rect> highlight_rects;

	QJsonObject to_json() const;
	void from_json(const QJsonObject& json_object);
};


struct PdfLink {
	fz_rect rect;
	std::string uri;
};

struct DocumentViewState {
	std::wstring document_path;
	OpenedBookState book_state;
};

struct PortalViewState {
	std::string document_checksum;
	OpenedBookState book_state;
};

/*
	A link is a connection between two document locations. For example when reading a paragraph that is referencing a figure,
	we may want to link that paragraphs's location to the figure. We can then easily switch between the paragraph and the figure.
	Also if helper window is opened, it automatically displays the closest link to the current location.
	Note that this is different from PdfLink which is the built-in link functionality in PDF file format.
*/
struct Portal {
	static Portal with_src_offset(float src_offset);

	PortalViewState dst;
	float src_offset_y;

	QJsonObject to_json() const;
	void from_json(const QJsonObject& json_object);
};


bool operator==(DocumentViewState& lhs, const DocumentViewState& rhs);

struct SearchResult {
	std::vector<fz_rect> rects;
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
	Document* doc = nullptr;
	int page;
	float zoom_level;
};

/*
	A cached page consists of cached_page_data which is the header that describes the rendered location
	and the actual rendered page. We have two different rendered formats: the pixmap we got from mupdf and
	the cached_page_texture which is an OpenGL texture. The reason we need both formats in the structure is because
	we render the pixmaps in a background mupdf thread, but in order to be able to use a texture in the main thread,
	the texture has to be created in the main thread. Therefore, we fill this structure's cached_page_pixmap value in the
	background thread and then send it to the main thread where we create the texture (which is a relatively fast operation
	so it doesn't matter that it is in the main thread). When cached_page_texture is created, we can safely delete the
	cached_page_pixmap, but the pixmap can only be deleted in the thread that it was created in, so we have to once again,
	send the cached_page_texture back to the background thread to be deleted.
*/
struct CachedPage {
	CachedPageData cached_page_data;
	fz_pixmap* cached_page_pixmap = nullptr;

	// last_access_time is used to garbage collect old pages
	unsigned int last_access_time;
	GLuint cached_page_texture;
};
bool operator==(const CachedPageData& lhs, const CachedPageData& rhs);


/*
	When a document does not have built-in links to the figures, we use a heuristic to find the figures
	and index them in FigureData structure. Using this, we can quickly find the figures when user clicks on the
	text descripbing the figure (for example 'Fig. 2.13')
*/
struct IndexedData {
	int page;
	float y_offset;
	std::wstring text;
};

bool operator==(const Mark& lhs, const Mark& rhs);
bool operator==(const BookMark& lhs, const BookMark& rhs);
bool operator==(const Highlight& lhs, const Highlight& rhs);
bool operator==(const Portal& lhs, const Portal& rhs);
