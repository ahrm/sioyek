#include "coordinates.h"
#include "utils.h"
#include "document.h"
#include "document_view.h"

WindowPos::WindowPos(float x_, float y_) {
    x = static_cast<int>(x_);
    y = static_cast<int>(y_);
}

WindowPos::WindowPos(int x_, int y_) {
    x = x_;
    y = y_;
}

WindowPos::WindowPos() {
    x = 0;
    y = 0;
}

WindowPos::WindowPos(QPoint pos) {
    x = pos.x();
    y = pos.y();
}

bool are_same(const AbsoluteDocumentPos& lhs, const AbsoluteDocumentPos& rhs) {
    return are_same(lhs.x, rhs.x) && are_same(lhs.y, rhs.y);
}

//bool operator==(const AbsoluteDocumentPos& lhs, const AbsoluteDocumentPos& rhs)
//{
//	return are_same(lhs.x, rhs.x) && are_same(lhs.y, rhs.y);
//}

AbsoluteDocumentPos DocumentPos::to_absolute(Document* doc) {
    return doc->document_to_absolute_pos(*this);
}

NormalizedWindowPos DocumentPos::to_window_normalized(DocumentView* document_view) {
    return document_view->document_to_window_pos(*this);
}

WindowPos DocumentPos::to_window(DocumentView* document_view) {
    return document_view->document_to_window_pos_in_pixels_uncentered(*this);
}

DocumentPos AbsoluteDocumentPos::to_document(Document* doc) {
    return doc->absolute_to_page_pos(*this);
}

NormalizedWindowPos AbsoluteDocumentPos::to_window_normalized(DocumentView* document_view) {
    return document_view->absolute_to_window_pos(*this);
}

WindowPos AbsoluteDocumentPos::to_window(DocumentView* document_view) {
    return document_view->absolute_to_window_pos_in_pixels(*this);
}

DocumentPos NormalizedWindowPos::to_document(DocumentView* document_view) {
    WindowPos window_pos = document_view->normalized_window_to_window_pos(*this);
    return document_view->window_to_document_pos(window_pos);
}

AbsoluteDocumentPos NormalizedWindowPos::to_absolute(DocumentView* document_view) {
    WindowPos window_pos = document_view->normalized_window_to_window_pos(*this);
    return document_view->window_to_absolute_document_pos(window_pos);
}

WindowPos NormalizedWindowPos::to_window(DocumentView* document_view) {
    return document_view->normalized_window_to_window_pos(*this);
}

DocumentPos WindowPos::to_document(DocumentView* document_view) {
    return document_view->window_to_document_pos(*this);
}

AbsoluteDocumentPos WindowPos::to_absolute(DocumentView* document_view) {
    return document_view->window_to_absolute_document_pos(*this);
}

NormalizedWindowPos WindowPos::to_window_normalized(DocumentView* document_view) {
    return document_view->window_to_normalized_window_pos(*this);
}
AbsoluteRect::AbsoluteRect(fz_rect r) : rect(r) {
}

AbsoluteRect DocumentRect::to_absolute(Document* doc) {
    return AbsoluteRect{ doc->document_to_absolute_rect(page, rect) };
}

DocumentRect AbsoluteRect::to_document(Document* doc) {
    int page = -1;
    fz_rect document_rect = doc->absolute_to_page_rect(rect, &page);
    return DocumentRect{ document_rect, page };
}

void AbsoluteRect::operator=(const fz_rect& r) {
    rect = r;
}

AbsoluteDocumentPos AbsoluteRect::center() {
    return AbsoluteDocumentPos { (rect.x0 + rect.x1) / 2.0f, (rect.y0 + rect.y1) / 2.0f};
}

int WindowPos::manhattan(const WindowPos& other) {
    return std::abs(x - other.x) + std::abs(y - other.y);
}

AbsoluteRect::AbsoluteRect(AbsoluteDocumentPos top_left, AbsoluteDocumentPos bottom_right) {
    rect.x0 = top_left.x;
    rect.y0 = top_left.y;
    rect.x1 = bottom_right.x;
    rect.y1 = bottom_right.y;
}

NormalizedWindowRect AbsoluteRect::to_window_normalized(DocumentView* document_view) {
    return NormalizedWindowRect{ document_view->absolute_to_window_rect(rect) };
}

NormalizedWindowRect DocumentRect::to_window_normalized(DocumentView* document_view) {
    return NormalizedWindowRect{ document_view->document_to_window_rect(page, rect) };
}

DocumentPos DocumentRect::top_left() {
    return DocumentPos{ page, rect.x0, rect.y0 };
}

DocumentPos DocumentRect::bottom_right() {
    return DocumentPos{ page, rect.x1, rect.y1 };
}

NormalizedWindowRect::NormalizedWindowRect(NormalizedWindowPos top_left, NormalizedWindowPos bottom_right) {
    rect.x0 = top_left.x;
    rect.y0 = top_left.y;
    rect.x1 = bottom_right.x;
    rect.y1 = bottom_right.y;
}

NormalizedWindowRect::NormalizedWindowRect(fz_rect r) : rect(r) {

}

AbsoluteRect::AbsoluteRect() : rect(fz_empty_rect) {

}

AbsoluteDocumentPos AbsoluteRect::top_left() {
    return AbsoluteDocumentPos{ rect.x0, rect.y0 };
}

AbsoluteDocumentPos AbsoluteRect::bottom_right() {
    return AbsoluteDocumentPos{ rect.x1, rect.y1 };
}

DocumentRect::DocumentRect() : rect(fz_empty_rect), page(-1){

}

DocumentRect::DocumentRect(fz_rect r, int p) : rect(r), page(p) {

}
DocumentRect::DocumentRect(DocumentPos top_left, DocumentPos bottom_right, int p) {
    rect.x0 = top_left.x;
    rect.y0 = top_left.y;
    rect.x1 = bottom_right.x;
    rect.y1 = bottom_right.y;
    page = p;
}

NormalizedWindowRect::NormalizedWindowRect() : rect(fz_empty_rect) {

}

bool AbsoluteRect::contains(const AbsoluteDocumentPos& point) {
    return fz_is_point_inside_rect(fz_point{ point.x, point.y }, rect);
}

fvec2 operator-(const AbsoluteDocumentPos& lhs, const AbsoluteDocumentPos& rhs) {
    return fvec2( lhs.x - rhs.x, lhs.y - rhs.y );
}

fvec2 operator-(const DocumentPos& lhs, const DocumentPos& rhs) {
    return fvec2( lhs.x - rhs.x, lhs.y - rhs.y );
}

fvec2 operator-(const NormalizedWindowPos& lhs, const NormalizedWindowPos& rhs) {
    return fvec2( lhs.x - rhs.x, lhs.y - rhs.y );
}

ivec2 operator-(const WindowPos& lhs, const WindowPos& rhs) {
    return ivec2( lhs.x - rhs.x, lhs.y - rhs.y );
}

AbsoluteDocumentPos operator+(const AbsoluteDocumentPos& lhs, const fvec2& rhs) {
    return AbsoluteDocumentPos{ lhs.x + rhs[0], lhs.y + rhs[1]};
}

DocumentPos operator+(const DocumentPos& lhs, const fvec2& rhs) {
    return DocumentPos{ lhs.page, lhs.x + rhs[0], lhs.y + rhs[1]};
}

NormalizedWindowPos operator+(const NormalizedWindowPos& lhs, const fvec2& rhs) {
    return NormalizedWindowPos{ lhs.x + rhs[0], lhs.y + rhs[1]};
}

WindowPos operator+(const WindowPos& lhs, const ivec2& rhs) {
    return WindowPos{ lhs.x + rhs[0], lhs.y + rhs[1]};
}

WindowRect DocumentRect::to_window(DocumentView* document_view) {
    return WindowRect(document_view->document_to_window_irect(page, rect));
}
