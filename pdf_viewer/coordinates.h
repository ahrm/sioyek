#pragma once

#include <qpoint.h>
#include <mupdf/fitz.h>

class Document;
class DocumentView;

struct AbsoluteDocumentPos;
struct NormalizedWindowPos;
struct WindowPos;



struct PagelessDocumentPos {
    float x;
    float y;
};

struct DocumentPos {
    int page;
    float x;
    float y;

    PagelessDocumentPos pageless() const;
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
template<typename R, typename T>
struct EnhancedRect : public R {

    using S = decltype(R::x0);

    EnhancedRect() {
        R::x0 = 0;
        R::y0 = 0;
        R::x1 = 0;
        R::y1 = 0;
    }

    EnhancedRect(R r) : R(r) {

    }

    EnhancedRect(T top_left, T bottom_right) {
        R::x0 = top_left.x;
        R::y0 = top_left.y;
        R::x1 = bottom_right.x;
        R::y1 = bottom_right.y;
    }

    S width() {
        return R::x1 - R::x0;
    }

    S height() {
        return R::y1 - R::y0;
    }

    S area() {
        return (R::x1 - R::x0) * (R::y1 - R::y0);
    }

    T center() {
        return T{ (R::x0 + R::x1) / 2, (R::y0 + R::y1) / 2 };
    }

    T top_left() {
        return T{ R::x0, R::y0 };
    }

    T bottom_right() {
        return T{ R::x1, R::y1 };
    }

    void operator=(const R& r) {
        R::x0 = r.x0;
        R::y0 = r.y0;
        R::x1 = r.x1;
        R::y1 = r.y1;
    }

    bool contains(const T& point) const {
        return (point.x >= R::x0 && point.x < R::x1 && point.y >= R::y0 && point.y < R::y1);
    }
};

using PagelessDocumentRect = EnhancedRect<fz_rect, PagelessDocumentPos>;
using WindowRect = EnhancedRect<fz_irect, WindowPos>;

struct DocumentRect {
    EnhancedRect<fz_rect, PagelessDocumentPos> rect;
    int page;

    DocumentRect();
    DocumentRect(fz_rect r, int page);
    DocumentRect(DocumentPos top_left, DocumentPos bottom_right, int page);

    AbsoluteRect to_absolute(Document* doc);
    NormalizedWindowRect to_window_normalized(DocumentView* document_view);
    WindowRect to_window(DocumentView* document_view);

    DocumentPos top_left();
    DocumentPos bottom_right();
};

struct NormalizedWindowRect : public EnhancedRect<fz_rect, NormalizedWindowPos>  {
    NormalizedWindowRect(NormalizedWindowPos top_left, NormalizedWindowPos bottom_right);
    NormalizedWindowRect(fz_rect r);
    NormalizedWindowRect();
};


struct AbsoluteRect : public EnhancedRect<fz_rect, AbsoluteDocumentPos> {
    AbsoluteRect(AbsoluteDocumentPos top_left, AbsoluteDocumentPos bottom_right);
    AbsoluteRect(fz_rect r);
    AbsoluteRect();
    DocumentRect to_document(Document* doc) const;

    NormalizedWindowRect to_window_normalized(DocumentView* document_view);
};

//template<typename T, int dim>
//operator+ (Vec<T, dim> a, Vec<T, dim> b) {
//	Vec<T, dim> c;
//	for (int i = 0; i < dim; i++) {
//		c.values[i] = a.values[i] + b.values[i];
//	}
//	return c;
//}
bool are_same(const AbsoluteDocumentPos& lhs, const AbsoluteDocumentPos& rhs);

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

fvec2 operator-(const AbsoluteDocumentPos& lhs, const AbsoluteDocumentPos& rhs);
fvec2 operator-(const DocumentPos& lhs, const DocumentPos& rhs);
fvec2 operator-(const NormalizedWindowPos& lhs, const NormalizedWindowPos& rhs);
ivec2 operator-(const WindowPos& lhs, const WindowPos& rhs);

PagelessDocumentRect rect_from_quad(fz_quad quad);
AbsoluteDocumentPos operator+(const AbsoluteDocumentPos& lhs, const fvec2& rhs);
DocumentPos operator+(const DocumentPos& lhs, const fvec2& rhs);
NormalizedWindowPos operator+(const NormalizedWindowPos& lhs, const fvec2& rhs);
WindowPos operator+(const WindowPos& lhs, const ivec2& rhs);
