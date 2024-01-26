#pragma once

#include <vector>
#include <string>
#include <variant>
#include <mupdf/fitz.h>
//#include <gl/glew.h>
#include <qopengl.h>
#include <variant>
#include <qjsonobject.h>
#include <qdatetime.h>
#include <qpixmap.h>

#include "coordinates.h"

class DocumentView;
class Document;

enum class SelectedObjectType {
    Drawing,
    Pixmap
};

struct SelectedObjectIndex {
    int index;
    SelectedObjectType type;
};

struct BookState {
    std::wstring document_path;
    float offset_y;
    std::string uuid;
};

struct OverviewState {
    float absolute_offset_y;
    float absolute_offset_x = 0;
    float zoom_level = -1;
    Document* doc = nullptr;
};

struct OpenedBookState {
    float zoom_level;
    float offset_x;
    float offset_y;
    bool ruler_mode = false;
    std::optional<int> presentation_page = {};
    std::optional<AbsoluteRect> ruler_rect = {};
    float ruler_pos = 0;
    int line_index = -1;
};

struct Annotation {
    std::string creation_time;
    std::string modification_time;
    std::string uuid;

    virtual QJsonObject to_json(std::string doc_checksum) const = 0;
    virtual void  add_to_tuples(std::vector<std::pair<std::string, QVariant>>& tuples) = 0;
    virtual std::vector<std::pair<std::string, QVariant>> to_tuples();
    virtual void from_json(const QJsonObject& json_object) = 0;

    void add_metadata_to_json(QJsonObject& obj) const;
    void load_metadata_from_json(const QJsonObject& obj);

    QDateTime get_creation_datetime() const;
    QDateTime get_modification_datetime() const;

    void update_creation_time();
    void update_modification_time();
};

/*
    A mark is a location in the document labeled with a symbol (which is a single character [a-z]). For example
    we can mark a location with symbol 'a' and later return to that location by going to the mark named 'a'.
    Lower case marks are local to the document and upper case marks are global.
*/
struct Mark : Annotation {
    float y_offset;
    char symbol;
    std::optional<float> x_offset = {};
    std::optional<float> zoom_level = {};

    QJsonObject to_json(std::string doc_checksum) const;
    void from_json(const QJsonObject& json_object);
    void add_to_tuples(std::vector<std::pair<std::string, QVariant>>& tuples) override;
};

/*
    A bookmark is similar to mark but instead of being indexed by a symbol, it has a description.
*/
struct BookMark : Annotation {
    float y_offset_ = -1;
    std::wstring description;

    float begin_x = -1;
    float begin_y = -1;
    float end_x = -1;
    float end_y = -1;

    float color[3] = { 0 };
    float font_size = -1;
    std::wstring font_face;

    AbsoluteDocumentPos begin_pos();
    AbsoluteDocumentPos end_pos();
    AbsoluteRect rect();
    QJsonObject to_json(std::string doc_checksum) const;
    void from_json(const QJsonObject& json_object);
    void add_to_tuples(std::vector<std::pair<std::string, QVariant>>& tuples) override;
    float get_y_offset() const;

    bool is_freetext() const;
    bool is_marked() const;

    AbsoluteRect get_rectangle() const;
};

struct Highlight : Annotation {
    AbsoluteDocumentPos selection_begin;
    AbsoluteDocumentPos selection_end;
    std::wstring description;
    std::wstring text_annot;
    char type;
    std::vector<AbsoluteRect> highlight_rects;

    QJsonObject to_json(std::string doc_checksum) const;
    void add_to_tuples(std::vector<std::pair<std::string, QVariant>>& tuples) override;
    void from_json(const QJsonObject& json_object);

};


struct PdfLink {
    //fz_rect rect;
    std::vector<PagelessDocumentRect> rects;
    int source_page;
    std::string uri;
};

enum OverviewSide {
    bottom = 0,
    top = 1,
    left = 2,
    right = 3
};

struct ParsedUri {
    int page;
    float x;
    float y;
};

struct FreehandDrawingPoint {
    AbsoluteDocumentPos pos;
    float thickness;
};

enum class SearchCaseSensitivity {
    CaseSensitive,
    CaseInsensitive,
    SmartCase
};

struct DocumentCharacter {
    int c;
    PagelessDocumentRect rect;
    bool is_final = false;
    fz_stext_block* stext_block;
    fz_stext_line* stext_line;
    fz_stext_char* stext_char;
};

struct FreehandDrawing {
    std::vector<FreehandDrawingPoint> points;
    char type;
    QDateTime creattion_time;
    AbsoluteRect bbox();
};

struct PixmapDrawing {
    QPixmap pixmap;
    AbsoluteRect rect;
};

struct CharacterAddress {
    int page;
    fz_stext_block* block;
    fz_stext_line* line;
    fz_stext_char* character;
    Document* doc;

    CharacterAddress* previous_character = nullptr;

    bool advance(char c);
    bool backspace();
    bool next_char();
    bool next_line();
    bool next_block();
    bool next_page();

    float focus_offset();

};

struct DocumentViewState {
    std::wstring document_path;
    OpenedBookState book_state;
};

struct PortalViewState {
    std::string document_checksum;
    OpenedBookState book_state;
};

struct OverviewResizeData {
    fz_rect original_rect;
    NormalizedWindowPos original_normal_mouse_pos;
    OverviewSide side_index;
};

struct OverviewMoveData {
    fvec2 original_offsets;
    NormalizedWindowPos original_normal_mouse_pos;
};

struct OverviewTouchMoveData {
    AbsoluteDocumentPos overview_original_pos_absolute;
    NormalizedWindowPos original_mouse_normalized_pos;
};

/*
    A link is a connection between two document locations. For example when reading a paragraph that is referencing a figure,
    we may want to link that paragraphs's location to the figure. We can then easily switch between the paragraph and the figure.
    Also if helper window is opened, it automatically displays the closest link to the current location.
    Note that this is different from PdfLink which is the built-in link functionality in PDF file format.
*/
struct Portal : Annotation {
    static Portal with_src_offset(float src_offset);

    PortalViewState dst;
    float src_offset_y;
    std::optional<float> src_offset_x = {};

    bool is_visible() const;

    QJsonObject to_json(std::string doc_checksum) const;
    void from_json(const QJsonObject& json_object);
    void add_to_tuples(std::vector<std::pair<std::string, QVariant>>& tuples) override;

    AbsoluteRect get_rectangle() const;
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

enum class ReferenceType {
    Generic,
    Equation,
    Reference,
    Abbreviation,
    None
};

struct SmartViewCandidate {
    Document* doc = nullptr;
    AbsoluteRect source_rect;
    std::wstring source_text;
    std::variant<DocumentPos, AbsoluteDocumentPos> target_pos;

    Document* get_document(DocumentView* view);
    DocumentPos get_docpos(DocumentView* view);
    AbsoluteDocumentPos get_abspos(DocumentView* view);
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

bool are_same(const BookMark& lhs, const BookMark& rhs);

bool are_same(const Highlight& lhs, const Highlight& rhs);
