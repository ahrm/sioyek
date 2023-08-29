#pragma once

#include <qpoint.h>
#include <mupdf/fitz.h>


class Document;
class DocumentView;

struct AbsoluteDocumentPos;
struct NormalizedWindowPos;
struct WindowPos;

struct DocumentPos {
    int page;
    float x;
    float y;

    AbsoluteDocumentPos to_absolute(Document* doc);
    NormalizedWindowPos to_window_normalized(DocumentView* document_view);
    WindowPos to_window(DocumentView* document_view);
};

struct AbsoluteDocumentPos {
    float x;
    // this is the concatenated y-coordinate of the current page (sum of all page heights up to current location)
    float y;

    DocumentPos to_document(Document* doc);
    NormalizedWindowPos to_window_normalized(DocumentView* document_view);
    WindowPos to_window(DocumentView* document_view);
};

// normalized window coordinates. x and y are in the range [-1, 1]
struct NormalizedWindowPos {
    float x;
    float y;

    DocumentPos to_document(DocumentView* document_view);
    AbsoluteDocumentPos to_absolute(DocumentView* document_view);
    WindowPos to_window(DocumentView* document_view);
};

// window coordinate in pixels
struct WindowPos {
    int x;
    int y;

    WindowPos(float x_, float y_);
    WindowPos(int x_, int y_);
    WindowPos();
    WindowPos(QPoint pos);

    DocumentPos to_document(DocumentView* document_view);
    AbsoluteDocumentPos to_absolute(DocumentView* document_view);
    NormalizedWindowPos to_window_normalized(DocumentView* document_view);
    int manhattan(const WindowPos& other);

};

struct DocumentRect;
struct AbsoluteRect;
struct NormalizedWindowRect;

//struct PagelessDocumentRect : public fz_rect {
//    PagelessDocumentRect();
//    PagelessDocumentRect(fz_rect r);
//};
template<typename T>
struct EnhancedRect : public fz_rect {

    EnhancedRect() : fz_rect(fz_empty_rect) {

    }

    EnhancedRect(fz_rect r) : fz_rect(r) {

    }

    T top_left() {
        return T{ x0, y0 };
    }

    T bottom_right() {
        return T{ x1, y1 };
    }

    float area() {
        return (x1 - x0) * (y1 - y0);
    }
};

struct DocumentRect {
    EnhancedRect<DocumentPos> rect;
    int page;

    DocumentRect();
    DocumentRect(fz_rect r, int page);
    DocumentRect(DocumentPos top_left, DocumentPos bottom_right, int page);

    AbsoluteRect to_absolute(Document* doc);
    NormalizedWindowRect to_window_normalized(DocumentView* document_view);

    DocumentPos top_left();
    DocumentPos bottom_right();
};

struct NormalizedWindowRect {
    EnhancedRect<NormalizedWindowPos> rect;

    NormalizedWindowRect(NormalizedWindowPos top_left, NormalizedWindowPos bottom_right);
    NormalizedWindowRect(fz_rect r);
    NormalizedWindowRect();
};


struct AbsoluteRect {
    EnhancedRect<AbsoluteDocumentPos> rect;

    AbsoluteRect(AbsoluteDocumentPos top_left, AbsoluteDocumentPos bottom_right);
    AbsoluteRect(fz_rect r);
    AbsoluteRect();
    DocumentRect to_document(Document* doc);
    AbsoluteDocumentPos center();
    AbsoluteDocumentPos top_left();
    AbsoluteDocumentPos bottom_right();
    NormalizedWindowRect to_window_normalized(DocumentView* document_view);
    void operator=(const fz_rect& r);
    bool contains(const AbsoluteDocumentPos& point);
};


template<typename T, int dim>
struct Vec {
    T values[dim];

    Vec(const QPoint& p) {
        values[0] = p.x();
        values[1] = p.y();
    }

    Vec(const NormalizedWindowPos& p) {
        values[0] = p.x;
        values[1] = p.y;
    }

    Vec(const WindowPos& p) {
        values[0] = p.x;
        values[1] = p.y;
    }

    Vec(const AbsoluteDocumentPos& p) {
        values[0] = p.x;
        values[1] = p.y;
    }

    Vec(T v1, T v2) {
        values[0] = v1;
        values[1] = v2;
    }

    Vec(T values_[]) : values(values_) {

    }

    Vec() {
        for (int i = 0; i < dim; i++) {
            values[i] = 0;
        }
    }

    T& operator[](int i) {
        return values[i];
    }

    const T& operator[](int i) const {
        return values[i];
    }

    T x() {
        return values[0];
    }

    T y() {
        return values[1];
    }

    T width() {
        return values[0];
    }

    T height() {
        return values[1];
    }

    NormalizedWindowPos to_normalized_window_pos() {
        return NormalizedWindowPos{ values[0], values[1] };
    }
};

using ivec2 = Vec<int, 2>;
using fvec2 = Vec<float, 2>;

template<typename T, int dim>
Vec<float, dim> operator/(const Vec<T, dim>& lhs, float rhs) {
    Vec<float, dim> res;
    for (int i = 0; i < dim; i++) {
        res[i] = lhs[i] / rhs;
    }
    return res;
}

template<typename T, int dim>
Vec<T, dim> operator+(const Vec<T, dim>& lhs, const Vec<T, dim>& rhs) {
    Vec<T, dim> res;
    for (int i = 0; i < dim; i++) {
        res[i] = lhs[i] + rhs[i];
    }
    return res;
}

template<typename T, int dim>
Vec<T, dim> operator-(const Vec<T, dim>& lhs, const Vec<T, dim>& rhs) {
    Vec<T, dim> res;
    for (int i = 0; i < dim; i++) {
        res[i] = lhs[i] - rhs[i];
    }
    return res;
}

//template<typename T, int dim>
//operator+ (Vec<T, dim> a, Vec<T, dim> b) {
//	Vec<T, dim> c;
//	for (int i = 0; i < dim; i++) {
//		c.values[i] = a.values[i] + b.values[i];
//	}
//	return c;
//}
bool are_same(const AbsoluteDocumentPos& lhs, const AbsoluteDocumentPos& rhs);
