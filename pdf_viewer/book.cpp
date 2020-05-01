#include "book.h"


bool operator==(const CachedPageData& lhs, const CachedPageData& rhs) {
	if (lhs.doc != rhs.doc) return false;
	if (lhs.page != rhs.page) return false;
	if (lhs.zoom_level != rhs.zoom_level) return false;
	return true;
}

Link Link::with_src_offset(float src_offset)
{
	Link res = Link();
	res.src_offset_y = src_offset;
	return res;
}
