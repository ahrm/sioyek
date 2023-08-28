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
