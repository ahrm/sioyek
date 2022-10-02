#include "book.h"


bool operator==(DocumentViewState& lhs, const DocumentViewState& rhs)
{
	return (lhs.book_state.offset_x == rhs.book_state.offset_x) &&
		(lhs.book_state.offset_y == rhs.book_state.offset_y) &&
		(lhs.book_state.zoom_level == rhs.book_state.zoom_level) &&
		(lhs.document_path == rhs.document_path);
}

bool operator==(const CachedPageData& lhs, const CachedPageData& rhs) {
	if (lhs.doc != rhs.doc) return false;
	if (lhs.page != rhs.page) return false;
	if (lhs.zoom_level != rhs.zoom_level) return false;
	return true;
}

Portal Portal::with_src_offset(float src_offset)
{
	Portal res = Portal();
	res.src_offset_y = src_offset;
	return res;
}

QJsonObject Mark::to_json() const
{
	QJsonObject res;
	res["y_offset"] = y_offset;
	res["symbol"] = symbol;
	return res;
}

void Mark::from_json(const QJsonObject& json_object)
{
	y_offset = json_object["y_offset"].toDouble();
	symbol = static_cast<char>(json_object["symbol"].toInt());
}

QJsonObject BookMark::to_json() const
{
	QJsonObject res;
	res["y_offset"] = y_offset;
	res["description"] = QString::fromStdWString(description);
	return res;

}

void BookMark::from_json(const QJsonObject& json_object)
{
	y_offset = json_object["y_offset"].toDouble();
	description = json_object["description"].toString().toStdWString();
}

QJsonObject Highlight::to_json() const
{
	QJsonObject res;
	res["selection_begin_x"] = selection_begin.x;
	res["selection_begin_y"] = selection_begin.y;
	res["selection_end_x"] = selection_end.x;
	res["selection_end_y"] = selection_end.y;
	res["description"] = QString::fromStdWString(description);
	res["type"] = type;
	return res;
}

void Highlight::from_json(const QJsonObject& json_object)
{
	selection_begin.x = json_object["selection_begin_x"].toDouble();
	selection_begin.y = json_object["selection_begin_y"].toDouble();
	selection_end.x = json_object["selection_end_x"].toDouble();
	selection_end.y = json_object["selection_end_y"].toDouble();
	description = json_object["description"].toString().toStdWString();
	type = static_cast<char>(json_object["type"].toInt());
}

QJsonObject Portal::to_json() const
{
	QJsonObject res;
	res["src_offset_y"] = src_offset_y;
	res["dst_checksum"] = QString::fromStdString(dst.document_checksum);
	res["dst_offset_x"] = dst.book_state.offset_x;
	res["dst_offset_y"] = dst.book_state.offset_y;
	res["dst_zoom_level"] = dst.book_state.zoom_level;
	return res;
}

void Portal::from_json(const QJsonObject& json_object)
{
	src_offset_y = json_object["src_offset_y"].toDouble();
	dst.document_checksum = json_object["dst_checksum"].toString().toStdString();
	dst.book_state.offset_x = json_object["dst_offset_x"].toDouble();
	dst.book_state.offset_y = json_object["dst_offset_y"].toDouble();
	dst.book_state.zoom_level = json_object["dst_zoom_level"].toDouble();
}

bool operator==(const Mark& lhs, const Mark& rhs)
{
	return (lhs.symbol == rhs.symbol) && (lhs.y_offset == rhs.y_offset);
}

bool operator==(const BookMark& lhs, const BookMark& rhs)
{
	return  (lhs.y_offset == rhs.y_offset) && (lhs.description == rhs.description);
}

bool operator==(const fz_point& lhs, const fz_point& rhs) {
	return (lhs.y == rhs.y) && (lhs.x == rhs.x);
}

bool operator==(const Highlight& lhs, const Highlight& rhs)
{
	return  (lhs.selection_begin.x == rhs.selection_begin.x) && (lhs.selection_end.x == rhs.selection_end.x) && 
		  (lhs.selection_begin.y == rhs.selection_begin.y) && (lhs.selection_end.y == rhs.selection_end.y) ;
}

bool operator==(const Portal& lhs, const Portal& rhs)
{
	return  (lhs.src_offset_y == rhs.src_offset_y) && (lhs.dst.document_checksum == rhs.dst.document_checksum);
}

