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
