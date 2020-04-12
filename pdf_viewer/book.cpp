#include "book.h"

bool operator==(const RenderRequest& lhs, const RenderRequest& rhs) {
	if (rhs.path != lhs.path) {
		return false;
	}
	if (rhs.page != lhs.page) {
		return false;
	}
	if (rhs.zoom_level != lhs.zoom_level) {
		return false;
	}
	return true;
}

bool operator==(const CachedPageData& lhs, const CachedPageData& rhs) {
	if (lhs.doc != rhs.doc) return false;
	if (lhs.page != rhs.page) return false;
	if (lhs.zoom_level != rhs.zoom_level) return false;
	return true;
}