#include "book.h"
#include "utils.h"

extern float BOOKMARK_RECT_SIZE;

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

QJsonObject Mark::to_json(std::string doc_checksum) const
{
    QJsonObject res;
    res["y_offset"] = y_offset;
    res["symbol"] = symbol;
    add_metadata_to_json(res);
    return res;
}

void Annotation::add_metadata_to_json(QJsonObject& obj) const {
    obj["creation_time"] = QString::fromStdString(creation_time);
    obj["modification_time"] = QString::fromStdString(modification_time);
    obj["uuid"] = QString::fromStdString(uuid);
}

void Annotation::load_metadata_from_json(const QJsonObject& obj) {
    if (obj.contains("creation_time")) {
        creation_time = obj["creation_time"].toString().toStdString();
    }
    if (obj.contains("modification_time")) {
        modification_time = obj["modification_time"].toString().toStdString();
    }
    if (obj.contains("uuid")) {
        uuid = obj["uuid"].toString().toStdString();
    }
    else {
        uuid = utf8_encode(new_uuid());
    }
}

std::vector<std::pair<std::string, QVariant>> Annotation::to_tuples() {
    std::vector<std::pair<std::string, QVariant>> res;

    add_to_tuples(res);

    res.push_back({ "uuid", QString::fromStdString(uuid) });
    res.push_back({ "creation_time", QString::fromStdString(creation_time) });
    res.push_back({ "modification_time", QString::fromStdString(modification_time) });

    return res;
}

QDateTime Annotation::get_creation_datetime() const {
    return QDateTime::fromString(QString::fromStdString(creation_time), "yyyy-MM-dd HH:mm:ss");
}

QDateTime Annotation::get_modification_datetime() const {
    return QDateTime::fromString(QString::fromStdString(modification_time), "yyyy-MM-dd HH:mm:ss");
}

void Annotation::update_creation_time() {
    creation_time = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss").toStdString();
    update_modification_time();
}

void Annotation::update_modification_time() {
    modification_time = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss").toStdString();
}

void Mark::from_json(const QJsonObject& json_object)
{
    y_offset = json_object["y_offset"].toDouble();
    symbol = static_cast<char>(json_object["symbol"].toInt());

    load_metadata_from_json(json_object);

}

void Mark::add_to_tuples(std::vector<std::pair<std::string, QVariant>>& tuples) {
    tuples.push_back({ "offset_y", y_offset });
    tuples.push_back({ "symbol", QChar(symbol) });
}

QJsonObject BookMark::to_json(std::string doc_checksum) const
{
    QJsonObject res;
    res["y_offset"] = y_offset_;
    res["description"] = QString::fromStdWString(description);
    res["begin_x"] = begin_x;
    res["begin_y"] = begin_y;
    res["end_x"] = end_x;
    res["end_y"] = end_y;

    if (is_freetext()) {
        res["color_red"] = color[0];
        res["color_green"] = color[1];
        res["color_blue"] = color[2];
        res["font_size"] = font_size;
        res["font_face"] = QString::fromStdWString(font_face);
    }

    add_metadata_to_json(res);

    return res;

}

void BookMark::add_to_tuples(std::vector<std::pair<std::string, QVariant>>& tuples) {
    tuples.push_back({ "offset_y", y_offset_ });
    tuples.push_back({ "desc",  QString::fromStdWString(description) });
    tuples.push_back({ "begin_x",  begin_x });
    tuples.push_back({ "begin_y",  begin_y });
    tuples.push_back({ "end_x",  end_x });
    tuples.push_back({ "end_y",  end_y });
    tuples.push_back({ "color_red", color[0] });
    tuples.push_back({ "color_green", color[1] });
    tuples.push_back({ "color_blue", color[2] });
    tuples.push_back({ "font_size", font_size });
    tuples.push_back({ "font_face", QString::fromStdWString(font_face) });
}

void BookMark::from_json(const QJsonObject& json_object)
{
    y_offset_ = json_object["y_offset"].toDouble();
    description = json_object["description"].toString().toStdWString();
    begin_x = json_object["begin_x"].toDouble();
    begin_y = json_object["begin_y"].toDouble();
    end_x = json_object["end_x"].toDouble();
    end_y = json_object["end_y"].toDouble();

    if (json_object.contains("color_red")) {
        color[0] = json_object["color_red"].toDouble();
        color[1] = json_object["color_green"].toDouble();
        color[2] = json_object["color_blue"].toDouble();
        font_size = json_object["font_size"].toDouble();
        font_face = json_object["font_face"].toString().toStdWString();
    }

    load_metadata_from_json(json_object);
}

bool BookMark::is_freetext() const {
    return (begin_y > -1) && (end_y > -1);
}

bool BookMark::is_marked() const {
    return (begin_y > -1) && (end_y == -1);
}

QJsonObject Highlight::to_json(std::string doc_checksum) const
{
    QJsonObject res;
    res["selection_begin_x"] = selection_begin.x;
    res["selection_begin_y"] = selection_begin.y;
    res["selection_end_x"] = selection_end.x;
    res["selection_end_y"] = selection_end.y;
    res["description"] = QString::fromStdWString(description);
    res["text_annot"] = QString::fromStdWString(text_annot);
    res["type"] = type;
    add_metadata_to_json(res);
    return res;
}

void Highlight::add_to_tuples(std::vector<std::pair<std::string, QVariant>>& tuples) {
    tuples.push_back({ "begin_x", selection_begin.x });
    tuples.push_back({ "begin_y", selection_begin.y });
    tuples.push_back({ "end_x", selection_end.x });
    tuples.push_back({ "end_y", selection_end.y });
    tuples.push_back({ "desc", QString::fromStdWString(description) });
    tuples.push_back({ "text_annot", QString::fromStdWString(text_annot) });
    tuples.push_back({ "type", QChar(type) });
}

void Highlight::from_json(const QJsonObject& json_object)
{
    selection_begin.x = json_object["selection_begin_x"].toDouble();
    selection_begin.y = json_object["selection_begin_y"].toDouble();
    selection_end.x = json_object["selection_end_x"].toDouble();
    selection_end.y = json_object["selection_end_y"].toDouble();
    description = json_object["description"].toString().toStdWString();
    type = static_cast<char>(json_object["type"].toInt());

    load_metadata_from_json(json_object);
}

QJsonObject Portal::to_json(std::string doc_checksum) const
{
    QJsonObject res;
    res["src_offset_y"] = src_offset_y;
    res["dst_checksum"] = QString::fromStdString(dst.document_checksum);
    res["dst_offset_x"] = dst.book_state.offset_x;
    res["dst_offset_y"] = dst.book_state.offset_y;
    res["dst_zoom_level"] = dst.book_state.zoom_level;

    if (src_offset_x) {
        res["src_offset_x"] = src_offset_x.value();
    }

    res["same"] = (doc_checksum == dst.document_checksum);
    add_metadata_to_json(res);
    return res;
}

void Portal::from_json(const QJsonObject& json_object)
{
    src_offset_y = json_object["src_offset_y"].toDouble();
    dst.document_checksum = json_object["dst_checksum"].toString().toStdString();
    dst.book_state.offset_x = json_object["dst_offset_x"].toDouble();
    dst.book_state.offset_y = json_object["dst_offset_y"].toDouble();
    dst.book_state.zoom_level = json_object["dst_zoom_level"].toDouble();

    if (json_object.contains("src_offset_x")) {

        src_offset_x = json_object["src_offset_x"].toDouble();
    }

    load_metadata_from_json(json_object);
}

void Portal::add_to_tuples(std::vector<std::pair<std::string, QVariant>>& tuples) {
    tuples.push_back({ "src_offset_y", src_offset_y });
    tuples.push_back({ "dst_document", QString::fromStdString(dst.document_checksum) });
    tuples.push_back({ "dst_offset_x", dst.book_state.offset_x });
    tuples.push_back({ "dst_offset_y", dst.book_state.offset_y });
    tuples.push_back({ "dst_zoom_level", dst.book_state.zoom_level });
    if (src_offset_x) {
        tuples.push_back({ "src_offset_x", src_offset_x.value()});
    }

}

bool operator==(const Mark& lhs, const Mark& rhs)
{
    return (lhs.symbol == rhs.symbol) && (lhs.y_offset == rhs.y_offset);
}

bool operator==(const BookMark& lhs, const BookMark& rhs)
{
    return  (lhs.y_offset_ == rhs.y_offset_) && (lhs.description == rhs.description);
}

bool operator==(const fz_point& lhs, const fz_point& rhs) {
    return (lhs.y == rhs.y) && (lhs.x == rhs.x);
}

bool operator==(const Highlight& lhs, const Highlight& rhs)
{
    return  (lhs.selection_begin.x == rhs.selection_begin.x) && (lhs.selection_end.x == rhs.selection_end.x) &&
        (lhs.selection_begin.y == rhs.selection_begin.y) && (lhs.selection_end.y == rhs.selection_end.y);
}

bool operator==(const Portal& lhs, const Portal& rhs)
{
    return  (lhs.src_offset_y == rhs.src_offset_y) && (lhs.dst.document_checksum == rhs.dst.document_checksum);
}

bool are_same(const BookMark& lhs, const BookMark& rhs) {
    return are_same(lhs.begin_x, rhs.begin_x) && are_same(lhs.begin_y, rhs.begin_y) && are_same(lhs.end_x, rhs.end_x) && are_same(lhs.end_y, rhs.end_y);
}

bool are_same(const Highlight& lhs, const Highlight& rhs) {
    return are_same(lhs.selection_begin, rhs.selection_begin) && are_same(lhs.selection_end, rhs.selection_end);
}

bool Portal::is_visible() const {
    return src_offset_x.has_value();
}

AbsoluteRect BookMark::get_rectangle() const{
    if (end_y > -1) {

        return AbsoluteRect(
            AbsoluteDocumentPos{ begin_x, begin_y },
            AbsoluteDocumentPos{ end_x, end_y }
        );
    }
    else {
        return AbsoluteRect(
            AbsoluteDocumentPos{ begin_x - BOOKMARK_RECT_SIZE, begin_y - BOOKMARK_RECT_SIZE },
            AbsoluteDocumentPos{ begin_x + BOOKMARK_RECT_SIZE, begin_y + BOOKMARK_RECT_SIZE }
        );
    }
}

AbsoluteRect Portal::get_rectangle() const{

    return AbsoluteRect(
        AbsoluteDocumentPos{ src_offset_x.value() - BOOKMARK_RECT_SIZE, src_offset_y - BOOKMARK_RECT_SIZE},
        AbsoluteDocumentPos{ src_offset_x.value() + BOOKMARK_RECT_SIZE, src_offset_y + BOOKMARK_RECT_SIZE}
    );
}

float BookMark::get_y_offset() const{
    if (begin_y != -1) return begin_y;
    return y_offset_;
}

AbsoluteDocumentPos BookMark::begin_pos() {
    return AbsoluteDocumentPos{ begin_x, begin_y };
}

AbsoluteDocumentPos BookMark::end_pos() {
    return AbsoluteDocumentPos{ end_x, end_y };
}

AbsoluteRect BookMark::rect() {
    return AbsoluteRect(begin_pos(), end_pos());
}

AbsoluteRect FreehandDrawing::bbox(){
    AbsoluteRect res;
    if (points.size() > 0) {
        res.x0 = points[0].pos.x;
        res.x1 = points[0].pos.x;
        res.y0 = points[0].pos.y;
        res.y1 = points[0].pos.y;
        for (int i = 1; i < points.size(); i++) {
            res.x0 = std::min(points[i].pos.x, res.x0);
            res.x1 = std::max(points[i].pos.x, res.x1);
            res.y0 = std::min(points[i].pos.y, res.y0);
            res.y1 = std::max(points[i].pos.y, res.y1);
        }
    }
    return res;
}
