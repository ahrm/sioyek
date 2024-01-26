//#include <Windows.h>
#include <cwctype>

#ifdef SIOYEK_ANDROID
#include <unistd.h>
#endif

#include <cmath>
#include <cassert>
#include "utils.h"
#include <optional>
#include <cstring>
#include <sstream>
#include <string>
#include <qclipboard.h>
#include <qguiapplication.h>
#include <qprocess.h>
#include <qdesktopservices.h>
#include <qurl.h>
#include <qmessagebox.h>
#include <qbytearray.h>
#include <qdatastream.h>
#include <qstringlist.h>
#include <qcommandlineparser.h>
#include <qdir.h>
#include <qurl.h>
#include <qnetworkaccessmanager.h>
#include <qnetworkrequest.h>
#include <qnetworkreply.h>
#include <qscreen.h>
#include <qjsonarray.h>
#include <quuid.h>

#ifdef SIOYEK_ANDROID
#include <QtCore/private/qandroidextras_p.h>
#include <qjniobject.h>
#endif

#include <mupdf/pdf.h>

extern std::wstring LIBGEN_ADDRESS;
extern std::wstring GOOGLE_SCHOLAR_ADDRESS;
extern std::ofstream LOG_FILE;
extern int STATUS_BAR_FONT_SIZE;
extern float STATUS_BAR_COLOR[3];
extern float STATUS_BAR_TEXT_COLOR[3];
extern float UI_SELECTED_TEXT_COLOR[3];
extern float UI_SELECTED_BACKGROUND_COLOR[3];
extern bool NUMERIC_TAGS;

extern QString EPUB_TEMPLATE;
extern float EPUB_LINE_SPACING;
extern float EPUB_WIDTH;
extern float EPUB_HEIGHT;
extern float EPUB_FONT_SIZE;
extern std::wstring EPUB_CSS;
extern float HIGHLIGHT_COLORS[26 * 3];
extern float BLACK_COLOR[3];
extern bool PAPER_DOWNLOAD_AUTODETECT_PAPER_NAME;

extern float UI_BACKGROUND_COLOR[3];
extern float UI_TEXT_COLOR[3];

extern std::wstring PAPER_SEARCH_URL_PATH;
extern std::wstring PAPER_SEARCH_TILE_PATH;
extern std::wstring PAPER_SEARCH_CONTRIB_PATH;
extern std::wstring UI_FONT_FACE_NAME;
extern std::wstring STATUS_FONT_FACE_NAME;

extern bool VERBOSE;


#ifdef Q_OS_WIN
#include <windows.h>
#endif


std::wstring to_lower(const std::wstring& inp) {
    std::wstring res;
    for (char c : inp) {
        res.push_back(::tolower(c));
    }
    return res;
}

void get_flat_toc(const std::vector<TocNode*>& roots, std::vector<std::wstring>& output, std::vector<int>& pages) {
    // Enumerate ToC nodes in DFS order

    for (const auto& root : roots) {
        output.push_back(root->title);
        pages.push_back(root->page);
        get_flat_toc(root->children, output, pages);
    }
}

TocNode* get_toc_node_from_indices_helper(const std::vector<TocNode*>& roots, const std::vector<int>& indices, int pointer) {
    assert(pointer >= 0);

    if (pointer == 0) {
        return roots[indices[pointer]];
    }

    return get_toc_node_from_indices_helper(roots[indices[pointer]]->children, indices, pointer - 1);
}

TocNode* get_toc_node_from_indices(const std::vector<TocNode*>& roots, const std::vector<int>& indices) {
    // Get a table of contents item by recursively indexing using `indices`
    return get_toc_node_from_indices_helper(roots, indices, indices.size() - 1);
}


QStandardItem* get_item_tree_from_toc_helper(const std::vector<TocNode*>& children, QStandardItem* parent) {

    for (const auto* child : children) {
        QStandardItem* child_item = new QStandardItem(QString::fromStdWString(child->title));
        QStandardItem* page_item = new QStandardItem("[ " + QString::number(child->page) + " ]");
        child_item->setData(child->page);
        page_item->setTextAlignment(Qt::AlignVCenter | Qt::AlignRight);

        get_item_tree_from_toc_helper(child->children, child_item);
        parent->appendRow(QList<QStandardItem*>() << child_item << page_item);
    }
    return parent;
}


QStandardItemModel* get_model_from_toc(const std::vector<TocNode*>& roots) {

    QStandardItemModel* model = new QStandardItemModel();
    get_item_tree_from_toc_helper(roots, model->invisibleRootItem());
    return model;
}


int mod(int a, int b)
{
    // compute a mod b handling negative numbers "correctly"
    return (a % b + b) % b;
}

ParsedUri parse_uri(fz_context* mupdf_context, fz_document* document, std::string uri) {
    fz_link_dest dest = fz_resolve_link_dest(mupdf_context, document, uri.c_str());
    int target_page = fz_page_number_from_location(mupdf_context, document, dest.loc) + 1;
    if (dest.type != FZ_LINK_DEST_XYZ) {
        float x = dest.x + dest.w / 2;;
        float y = dest.y + dest.h / 2;
        return { target_page, x, y };
    }
    return { target_page, dest.x, dest.y };
}

char get_symbol(int key, bool is_shift_pressed, const std::vector<char>& special_symbols) {

    if (key >= 'A' && key <= 'Z') {
        if (is_shift_pressed) {
            return key;
        }
        else {
            return key + 'a' - 'A';
        }
    }

    if (key >= '0' && key <= '9') {
        return key;
    }

    if (std::find(special_symbols.begin(), special_symbols.end(), key) != special_symbols.end()) {
        return key;
    }

    return 0;
}

void rect_to_quad(fz_rect rect, float quad[8]) {
    quad[0] = rect.x0;
    quad[1] = rect.y0;
    quad[2] = rect.x1;
    quad[3] = rect.y0;
    quad[4] = rect.x0;
    quad[5] = rect.y1;
    quad[6] = rect.x1;
    quad[7] = rect.y1;
}

void copy_to_clipboard(const std::wstring& text, bool selection) {
    auto clipboard = QGuiApplication::clipboard();
    auto qtext = QString::fromStdWString(text);
    if (!selection) {
        clipboard->setText(qtext);
    }
    else {
        clipboard->setText(qtext, QClipboard::Selection);
    }
}

#define OPEN_KEY(parent, name, ptr) \
	RegCreateKeyExA(parent, name, 0, 0, 0, KEY_WRITE, 0, &ptr, 0)

#define SET_KEY(parent, name, value) \
	RegSetValueExA(parent, name, 0, REG_SZ, (const BYTE *)(value), (DWORD)strlen(value) + 1)

void install_app(const char* argv0)
{
#ifdef Q_OS_WIN
    char buf[512];
    HKEY software, classes, testpdf, dotpdf;
    HKEY shell, open, command, supported_types;
    HKEY pdf_progids;

    OPEN_KEY(HKEY_CURRENT_USER, "Software", software);
    OPEN_KEY(software, "Classes", classes);
    OPEN_KEY(classes, ".pdf", dotpdf);
    OPEN_KEY(dotpdf, "OpenWithProgids", pdf_progids);
    OPEN_KEY(classes, "Sioyek", testpdf);
    OPEN_KEY(testpdf, "SupportedTypes", supported_types);
    OPEN_KEY(testpdf, "shell", shell);
    OPEN_KEY(shell, "open", open);
    OPEN_KEY(open, "command", command);

    sprintf(buf, "\"%s\" \"%%1\"", argv0);

    SET_KEY(open, "FriendlyAppName", "Sioyek");
    SET_KEY(command, "", buf);
    SET_KEY(supported_types, ".pdf", "");
    SET_KEY(pdf_progids, "sioyek", "");

    RegCloseKey(dotpdf);
    RegCloseKey(testpdf);
    RegCloseKey(classes);
    RegCloseKey(software);
#endif
}

int get_f_key(std::wstring name) {
    if (name[0] == '<') {
        name = name.substr(1, name.size() - 2);
    }
    if (name[0] != 'f') {
        return 0;
    }
    name = name.substr(1, name.size() - 1);
    if (!isdigit(name[0])) {
        return 0;
    }

    int num;
    std::wstringstream ss(name);
    ss >> num;
    return  num;
}

void show_error_message(const std::wstring& error_message) {
    QMessageBox msgBox;
    msgBox.setText(QString::fromStdWString(error_message));
    msgBox.setStandardButtons(QMessageBox::Ok);
    msgBox.setDefaultButton(QMessageBox::Ok);
    msgBox.exec();
}

std::wstring utf8_decode(const std::string& encoded_str) {
    std::wstring res;
    utf8::utf8to32(encoded_str.begin(), encoded_str.end(), std::back_inserter(res));
    return res;
}

std::string utf8_encode(const std::wstring& decoded_str) {
    std::string res;
    utf8::utf32to8(decoded_str.begin(), decoded_str.end(), std::back_inserter(res));
    return res;
}

bool is_rtl(int c) {
    if (
        (c == 0x05BE) || (c == 0x05C0) || (c == 0x05C3) || (c == 0x05C6) ||
        ((c >= 0x05D0) && (c <= 0x05F4)) ||
        (c == 0x0608) || (c == 0x060B) || (c == 0x060D) ||
        ((c >= 0x061B) && (c <= 0x064A)) ||
        ((c >= 0x066D) && (c <= 0x066F)) ||
        ((c >= 0x0671) && (c <= 0x06D5)) ||
        ((c >= 0x06E5) && (c <= 0x06E6)) ||
        ((c >= 0x06EE) && (c <= 0x06EF)) ||
        ((c >= 0x06FA) && (c <= 0x0710)) ||
        ((c >= 0x0712) && (c <= 0x072F)) ||
        ((c >= 0x074D) && (c <= 0x07A5)) ||
        ((c >= 0x07B1) && (c <= 0x07EA)) ||
        ((c >= 0x07F4) && (c <= 0x07F5)) ||
        ((c >= 0x07FA) && (c <= 0x0815)) ||
        (c == 0x081A) || (c == 0x0824) || (c == 0x0828) ||
        ((c >= 0x0830) && (c <= 0x0858)) ||
        ((c >= 0x085E) && (c <= 0x08AC)) ||
        (c == 0x200F) || (c == 0xFB1D) ||
        ((c >= 0xFB1F) && (c <= 0xFB28)) ||
        ((c >= 0xFB2A) && (c <= 0xFD3D)) ||
        ((c >= 0xFD50) && (c <= 0xFDFC)) ||
        ((c >= 0xFE70) && (c <= 0xFEFC)) ||
        ((c >= 0x10800) && (c <= 0x1091B)) ||
        ((c >= 0x10920) && (c <= 0x10A00)) ||
        ((c >= 0x10A10) && (c <= 0x10A33)) ||
        ((c >= 0x10A40) && (c <= 0x10B35)) ||
        ((c >= 0x10B40) && (c <= 0x10C48)) ||
        ((c >= 0x1EE00) && (c <= 0x1EEBB))
        ) return true;
    return false;
}

std::wstring reverse_wstring(const std::wstring& inp) {
    std::wstring res;
    for (int i = inp.size() - 1; i >= 0; i--) {
        res.push_back(inp[i]);
    }
    return res;
}

bool parse_search_command(const std::wstring& search_command, int* out_begin, int* out_end, std::wstring* search_text) {
    std::wstringstream ss(search_command);
    if (search_command[0] == '<') {
        wchar_t dummy;
        ss >> dummy;
        ss >> *out_begin;
        ss >> dummy;
        ss >> *out_end;
        ss >> dummy;
        std::getline(ss, *search_text);
        return true;
    }
    else {
        *search_text = ss.str();
        return false;
    }
}

float dist_squared(fz_point p1, fz_point p2) {
    return (p1.x - p2.x) * (p1.x - p2.x) + 100 * (p1.y - p2.y) * (p1.y - p2.y);
}

bool is_text_rtl(const std::wstring& text) {
    int score = 0;
    for (int chr : text) {
        if (is_rtl(chr)) {
            score += 1;
        }
        else {
            score -= 1;
        }
    }
    return score > 0;
}

bool is_stext_line_rtl(fz_stext_line* line) {

    float rtl_count = 0.0f;
    float total_count = 0.0f;
    LL_ITER(ch, line->first_char) {
        if (is_rtl(ch->c)) {
            rtl_count += 1.0f;
        }
        total_count += 1.0f;
    }
    return ((rtl_count / total_count) > 0.5f);
}
bool is_stext_page_rtl(fz_stext_page* stext_page) {

    float rtl_count = 0.0f;
    float total_count = 0.0f;

    LL_ITER(block, stext_page->first_block) {
        if (block->type == FZ_STEXT_BLOCK_TEXT) {
            LL_ITER(line, block->u.t.first_line) {
                LL_ITER(ch, line->first_char) {
                    if (is_rtl(ch->c)) {
                        rtl_count += 1.0f;
                    }
                    total_count += 1.0f;
                }
            }
        }
    }
    return ((rtl_count / total_count) > 0.5f);
}

std::vector<fz_stext_char*> reorder_stext_line(fz_stext_line* line) {

    std::vector<fz_stext_char*> reordered_chars;

    bool rtl = is_stext_line_rtl(line);

    LL_ITER(ch, line->first_char) {
        reordered_chars.push_back(ch);
    }

    if (rtl) {
        std::sort(reordered_chars.begin(), reordered_chars.end(), [](fz_stext_char* lhs, fz_stext_char* rhs) {
            return lhs->quad.lr.x > rhs->quad.lr.x;
            });
    }
    else {
        std::sort(reordered_chars.begin(), reordered_chars.end(), [](fz_stext_char* lhs, fz_stext_char* rhs) {
            return (lhs->quad.lr.x <= rhs->quad.lr.x) && (lhs->quad.ll.x < rhs->quad.ll.x);
            });
    }
    return reordered_chars;
}

void get_flat_chars_from_block(fz_stext_block* block, std::vector<fz_stext_char*>& flat_chars, bool dehyphenate) {
    if (block->type == FZ_STEXT_BLOCK_TEXT) {
        LL_ITER(line, block->u.t.first_line) {
            std::vector<fz_stext_char*> reordered_chars = reorder_stext_line(line);
            for (auto ch : reordered_chars) {
                if (ch->c == 65533) {
                    // unicode replacement character https://www.fileformat.info/info/unicode/char/fffd/index.htm
                    ch->c = ' ';
                }

                if (dehyphenate) {
                    if (ch->c == '-' && (ch->next == nullptr)) {
                        continue;
                    }
                }

                flat_chars.push_back(ch);
            }
        }
    }
}

bool is_index_reverse_reference_number(const std::vector<fz_stext_char*>& flat_chars, int index, int* range_begin, int* range_end) {

    if (flat_chars[index]->next != nullptr) return false;
    if (flat_chars[index]->c > 128) return false;
    if (!std::isdigit(flat_chars[index]->c)) return false;

    std::vector<char> chars_between_last_dot_and_index;

    int current_index = index-1;
    bool reached_dot = false;

    while (current_index >= 0) {
        if ((flat_chars[current_index]->c == '.') || (flat_chars[current_index]->next == nullptr)) {
            break;
        }
        chars_between_last_dot_and_index.push_back(flat_chars[current_index]->c);
        current_index--;
    }
    int n_chars = chars_between_last_dot_and_index.size();
    if (n_chars > 0 && n_chars < 20) {
        for (int i = 0; i < n_chars; i++) {
            if ((chars_between_last_dot_and_index[i] > 0) && (chars_between_last_dot_and_index[i] < 128) && std::isalpha(chars_between_last_dot_and_index[i])) {
                return false;
            }
        }
        *range_begin = current_index;
        *range_end = index;
        return true;
    }
    return false;

    //int last_dot_index 

}


void get_flat_chars_from_stext_page_for_bib_detection(fz_stext_page* stext_page, std::vector<DocumentCharacter>& flat_chars) {
    LL_ITER(block, stext_page->first_block) {
        if (block->type == FZ_STEXT_BLOCK_TEXT) {
            LL_ITER(line, block->u.t.first_line) {
                std::vector<fz_stext_char*> current_line_chars;
                std::optional<DocumentCharacter> phantom_space = {};

                LL_ITER(chr, line->first_char) {
                    current_line_chars.push_back(chr);
                }

                //if (current_line_chars.size() < 5) {
                //    bool found_dot = false;
                //    for (auto c : current_line_chars) {
                //        if (c->c == '.') {
                //            found_dot = true;
                //            break;
                //        }
                //    }
                //    if (!found_dot) {
                //        continue;
                //    }
                //}
                if (current_line_chars.size() > 0) { // remove inverse references from end of lines
                    if (current_line_chars.back()->c <= 128 && (std::isdigit(current_line_chars.back()->c))) {
                        while (current_line_chars.size() > 0 && current_line_chars.back()->c != '.') {
                            current_line_chars.pop_back();
                        }
                    }
                }

                if (current_line_chars.size() > 0) { //dehyphenate
                    if (current_line_chars.back()->c == '-') {
                        current_line_chars.pop_back();
                    }
                    else {
                        DocumentCharacter ps;
                        ps.c = ' ';
                        ps.rect = fz_rect_from_quad(current_line_chars.back()->quad);
                        ps.stext_block = block;
                        ps.stext_line = line;
                        ps.stext_char = nullptr;
                        ps.is_final = true;
                        phantom_space = ps;
                    }
                }
                for (int i = 0; i < current_line_chars.size(); i++) {
                    DocumentCharacter dc;
                    dc.c = current_line_chars[i]->c;
                    dc.rect = fz_rect_from_quad(current_line_chars[i]->quad);
                    dc.stext_block = block;
                    dc.stext_line = line;
                    dc.stext_char = current_line_chars[i];
                    if (i == current_line_chars.size() - 1) {
                        if (!phantom_space.has_value()) {
                            dc.is_final = true;
                        }
                    }
                    flat_chars.push_back(dc);
                }
                if (phantom_space) {
                    flat_chars.push_back(phantom_space.value());
                }
            }
        }
    }
    //std::vector<fz_stext_char*> temp_flat_chars;
    //get_flat_chars_from_stext_page(stext_page, temp_flat_chars, true);
    //std::vector<std::pair<int, int>> ranges_to_remove;

    //for (int i = 0; i < temp_flat_chars.size(); i++) {
    //    int begin_index = -1;
    //    int end_index = -1;
    //    if (is_index_reverse_reference_number(temp_flat_chars, i, &begin_index, &end_index)) {
    //        ranges_to_remove.push_back(std::make_pair(begin_index, end_index));
    //    }
    //}

    //int current_range_index = -1;
    //if (ranges_to_remove.size() > 0) current_range_index = 0;

    //for (int i = 0; i < temp_flat_chars.size(); i++) {
    //    if ((current_range_index < ranges_to_remove.size() - 1) && (i > ranges_to_remove[current_range_index].second)) {
    //        current_range_index += 1;
    //    }

    //    if ((current_range_index == -1) || !(i > ranges_to_remove[current_range_index].first && i <= ranges_to_remove[current_range_index].second)) {
    //        flat_chars.push_back(temp_flat_chars[i]);
    //    }

    //}


}

void get_flat_chars_from_stext_page(fz_stext_page* stext_page, std::vector<fz_stext_char*>& flat_chars, bool dehyphenate) {

    LL_ITER(block, stext_page->first_block) {
        get_flat_chars_from_block(block, flat_chars, dehyphenate);
    }
}

bool is_delimeter(int c) {
    std::vector<char> delimeters = { ' ', '\n', ';', ',' };
    return std::find(delimeters.begin(), delimeters.end(), c) != delimeters.end();
}

float get_character_height(fz_stext_char* c) {
    return std::abs(c->quad.ul.y - c->quad.ll.y);
}

float get_character_width(fz_stext_char* c) {
    return std::abs(c->quad.ul.x - c->quad.ur.x);
}

bool is_start_of_new_line(fz_stext_char* prev_char, fz_stext_char* current_char) {
    float height = get_character_height(current_char);
    float threshold = height / 2;
    if (std::abs(prev_char->quad.ll.y - current_char->quad.ll.y) > threshold) {
        return true;
    }
    return false;
}
bool is_start_of_new_word(fz_stext_char* prev_char, fz_stext_char* current_char) {
    if (is_delimeter(prev_char->c)) {
        return true;
    }

    return is_start_of_new_line(prev_char, current_char);
}

PagelessDocumentRect create_word_rect(const std::vector<PagelessDocumentRect>& chars) {
    PagelessDocumentRect res;
    res.x0 = res.x1 = res.y0 = res.y1 = 0;
    if (chars.size() == 0) return res;
    res = chars[0];

    for (size_t i = 1; i < chars.size(); i++) {
        if (res.x0 > chars[i].x0) res.x0 = chars[i].x0;
        if (res.x1 < chars[i].x1) res.x1 = chars[i].x1;
        if (res.y0 > chars[i].y0) res.y0 = chars[i].y0;
        if (res.y1 < chars[i].y1) res.y1 = chars[i].y1;
    }

    return res;
}

std::vector<PagelessDocumentRect> create_word_rects_multiline(const std::vector<PagelessDocumentRect>& chars) {
    std::vector<PagelessDocumentRect> res;
    std::vector<PagelessDocumentRect> current_line_chars;

    if (chars.size() == 0) return res;
    current_line_chars.push_back(chars[0]);

    for (size_t i = 1; i < chars.size(); i++) {
        if (chars[i].x0 < chars[i - 1].x0) { // a new line has begun
            res.push_back(create_word_rect(current_line_chars));
            current_line_chars.clear();
            current_line_chars.push_back(chars[i]);
        }
        else {
            current_line_chars.push_back(chars[i]);
        }
    }
    if (current_line_chars.size() > 0) {
        res.push_back(create_word_rect(current_line_chars));
    }
    return res;
}

PagelessDocumentRect create_word_rect(const std::vector<fz_stext_char*>& chars) {
    PagelessDocumentRect res;
    res.x0 = res.x1 = res.y0 = res.y1 = 0;
    if (chars.size() == 0) return res;
    res = fz_rect_from_quad(chars[0]->quad);

    for (size_t i = 1; i < chars.size(); i++) {
        PagelessDocumentRect current_char_rect = fz_rect_from_quad(chars[i]->quad);
        if (res.x0 > current_char_rect.x0) res.x0 = current_char_rect.x0;
        if (res.x1 < current_char_rect.x1) res.x1 = current_char_rect.x1;
        if (res.y0 > current_char_rect.y0) res.y0 = current_char_rect.y0;
        if (res.y1 < current_char_rect.y1) res.y1 = current_char_rect.y1;
    }

    return res;
}

void get_flat_words_from_flat_chars(const std::vector<fz_stext_char*>& flat_chars, std::vector<PagelessDocumentRect>& flat_word_rects, std::vector<std::vector<PagelessDocumentRect>>* out_char_rects) {

    if (flat_chars.size() == 0) return;

    std::vector<std::wstring> res;
    std::vector<fz_stext_char*> pending_word;
    pending_word.push_back(flat_chars[0]);

    for (size_t i = 1; i < flat_chars.size(); i++) {
        if (is_start_of_new_word(flat_chars[i - 1], flat_chars[i])) {
            flat_word_rects.push_back(create_word_rect(pending_word));
            if (out_char_rects != nullptr) {
                std::vector<PagelessDocumentRect> chars;
                for (auto c : pending_word) {
                    chars.push_back(fz_rect_from_quad(c->quad));
                }
                out_char_rects->push_back(chars);
            }
            if (is_start_of_new_line(flat_chars[i - 1], flat_chars[i])) {
                fz_rect new_rect = fz_rect_from_quad(flat_chars[i - 1]->quad);

                new_rect.x0 = (new_rect.x0 + 2 * new_rect.x1) / 3;
                flat_word_rects.push_back(new_rect);
                if (out_char_rects != nullptr) {
                    out_char_rects->push_back({ new_rect });
                }
            }
            pending_word.clear();
            pending_word.push_back(flat_chars[i]);
        }
        else {
            pending_word.push_back(flat_chars[i]);
        }
    }
}

void get_word_rect_list_from_flat_chars(const std::vector<fz_stext_char*>& flat_chars,
    std::vector<std::wstring>& words,
    std::vector<std::vector<PagelessDocumentRect>>& flat_word_rects) {

    if (flat_chars.size() == 0) return;

    std::vector<fz_stext_char*> pending_word;
    pending_word.push_back(flat_chars[0]);

    auto get_rects = [&]() {
        std::vector<PagelessDocumentRect> res;
        for (auto chr : pending_word) {
            res.push_back(fz_rect_from_quad(chr->quad));
        }
        return res;
    };

    auto get_word = [&]() {
        std::wstring res;
        for (auto chr : pending_word) {
            res.push_back(chr->c);
        }
        return res;
    };

    for (size_t i = 1; i < flat_chars.size(); i++) {
        if (is_start_of_new_word(flat_chars[i - 1], flat_chars[i])) {
            flat_word_rects.push_back(get_rects());
            words.push_back(get_word());

            pending_word.clear();
            pending_word.push_back(flat_chars[i]);
        }
        else {
            pending_word.push_back(flat_chars[i]);
        }
    }
}

int get_num_tag_digits(int n) {
    int res = 1;
    while (n > 26) {
        n = n / 26;
        res++;
    }
    return res;
}

std::string get_aplph_tag(int n, int max_n) {
    int n_digits = get_num_tag_digits(max_n);
    std::string tag;
    for (int i = 0; i < n_digits; i++) {
        tag.push_back('a' + (n % 26));
        n = n / 26;
    }
    std::reverse(tag.begin(), tag.end());
    return tag;
}

std::vector<std::string> get_tags(int n) {
    std::vector<std::string> res;
    int n_digits = get_num_tag_digits(n);
    for (int i = 0; i < n; i++) {
        int current_n = i;
        std::string tag;
        if (!NUMERIC_TAGS) {
            for (int i = 0; i < n_digits; i++) {
                tag.push_back('a' + (current_n % 26));
                current_n = current_n / 26;
            }
        }
        else {
            tag = std::to_string(i);
        }
        res.push_back(tag);
    }
    return res;
}

int get_index_from_tag(std::string tag, bool reversed) {
    if (reversed) {
        std::reverse(tag.begin(), tag.end());
    }

    int res = 0;
    int mult = 1;

    if (!NUMERIC_TAGS) {
        for (size_t i = 0; i < tag.size(); i++) {
            res += (tag[i] - 'a') * mult;
            mult = mult * 26;
        }
    }
    else {
        res = std::stoi(tag);
    }
    return res;
}

fz_stext_char* find_closest_char_to_document_point(const std::vector<fz_stext_char*> flat_chars, fz_point document_point, int* location_index) {
    float min_distance = std::numeric_limits<float>::infinity();
    fz_stext_char* res = nullptr;

    int index = 0;
    for (auto current_char : flat_chars) {

        fz_point quad_center;

        quad_center.x = (current_char->quad.ll.x + current_char->quad.lr.x) / 2;
        quad_center.y = (current_char->quad.ll.y + current_char->quad.ul.y) / 2;

        float distance = dist_squared(document_point, quad_center);
        if (distance < min_distance) {
            min_distance = distance;
            res = current_char;
            *location_index = index;
        }
        index++;
    }

    return res;
}

bool is_line_separator(fz_stext_char* last_char, fz_stext_char* current_char) {
    if (last_char == nullptr) {
        return false;
    }
    float dist = abs(last_char->quad.ll.y - current_char->quad.ll.y);
    if (dist > 1.0f) {
        return true;
    }
    return false;
}

bool is_separator(fz_stext_char* last_char, fz_stext_char* current_char) {
    if (last_char == nullptr) {
        return false;
    }

    if (current_char->c == ' ') {
        return true;
    }
    float dist = abs(last_char->quad.ll.y - current_char->quad.ll.y);
    if (dist > 1.0f) {
        return true;
    }
    return false;
}


std::wstring get_string_from_stext_block(fz_stext_block* block) {
    if (block->type == FZ_STEXT_BLOCK_TEXT) {
        std::wstring res;
        LL_ITER(line, block->u.t.first_line) {
            res += get_string_from_stext_line(line);
            if (line->next && res.size() > 0) {
                if (res.back() == '-') {
                    res.pop_back();
                }
                else {
                    res.push_back(' ');
                }
            }
        }
        return res;
    }
    else {
        return L"";
    }
}
std::wstring get_string_from_stext_line(fz_stext_line* line) {

    std::wstring res;
    LL_ITER(ch, line->first_char) {
        res.push_back(ch->c);
    }
    return res;
}

std::vector<PagelessDocumentRect> get_char_rects_from_stext_line(fz_stext_line* line) {
    std::vector<PagelessDocumentRect> res;
    LL_ITER(ch, line->first_char) {
        res.push_back(fz_rect_from_quad(ch->quad));
    }
    return res;
}

bool is_consequtive(fz_rect rect1, fz_rect rect2) {

    float xdist = abs(rect1.x1 - rect2.x1);
    float ydist1 = abs(rect1.y0 - rect2.y0);
    float ydist2 = abs(rect1.y1 - rect2.y1);
    float ydist = std::min(ydist1, ydist2);

    float rect1_width = rect1.x1 - rect1.x0;
    float rect2_width = rect2.x1 - rect2.x0;
    float average_width = (rect1_width + rect2_width) / 2.0f;

    float rect1_height = rect1.y1 - rect1.y0;
    float rect2_height = rect2.y1 - rect2.y0;
    float average_height = (rect1_height + rect2_height) / 2.0f;

    if (xdist < 3 * average_width && ydist < 2 * average_height) {
        return true;
    }

    return false;

}

fz_rect bound_rects(const std::vector<fz_rect>& rects) {
    // find the bounding box of some rects

    fz_rect res = rects[0];

    float average_y0 = 0.0f;
    float average_y1 = 0.0f;

    for (auto rect : rects) {
        if (res.x1 < rect.x1) {
            res.x1 = rect.x1;
        }
        if (res.x0 > rect.x0) {
            res.x0 = rect.x0;
        }

        average_y0 += rect.y0;
        average_y1 += rect.y1;
    }

    average_y0 /= rects.size();
    average_y1 /= rects.size();

    res.y0 = average_y0;
    res.y1 = average_y1;

    return res;

}


int next_path_separator_position(const std::wstring& path) {
    wchar_t sep1 = '/';
    wchar_t sep2 = '\\';
    int index1 = path.find(sep1);
    int index2 = path.find(sep2);
    if (index2 == -1) {
        return index1;
    }

    if (index1 == -1) {
        return index2;
    }
    return std::min(index1, index2);
}

void split_path(std::wstring path, std::vector<std::wstring>& res) {

    size_t loc = -1;
    // overflows
    while ((loc = next_path_separator_position(path)) != (size_t)-1) {

        int skiplen = loc + 1;
        if (loc != 0) {
            std::wstring part = path.substr(0, loc);
            res.push_back(part);
        }
        path = path.substr(skiplen, path.size() - skiplen);
    }
    if (path.size() > 0) {
        res.push_back(path);
    }
}


std::vector<std::wstring> split_whitespace(std::wstring const& input) {
    std::wistringstream buffer(input);
    std::vector<std::wstring> ret((std::istream_iterator<std::wstring, wchar_t>(buffer)),
        std::istream_iterator<std::wstring, wchar_t>());
    return ret;
}

void split_key_string(std::wstring haystack, const std::wstring& needle, std::vector<std::wstring>& res) {
    //todo: we can significantly reduce string allocations in this function if it turns out to be a
    //performance bottleneck.

    if (haystack == needle) {
        res.push_back(L"-");
        return;
    }

    size_t loc = -1;
    size_t needle_size = needle.size();
    while ((loc = haystack.find(needle)) != (size_t)-1) {


        int skiplen = loc + needle_size;
        if (loc != 0) {
            std::wstring part = haystack.substr(0, loc);
            res.push_back(part);
        }
        if ((loc < (haystack.size() - 1)) && (haystack.substr(needle.size(), needle.size()) == needle)) {
            // if needle is repeated, one of them is added as a token for example
            // <C-->
            // means [C, -]
            res.push_back(needle);
        }
        haystack = haystack.substr(skiplen, haystack.size() - skiplen);
    }
    if (haystack.size() > 0) {
        res.push_back(haystack);
    }
}


void run_command(std::wstring command, QStringList parameters, bool wait) {


#ifdef Q_OS_WIN
    std::wstring parameters_string = parameters.join(" ").toStdWString();
    SHELLEXECUTEINFO ShExecInfo = { 0 };
    ShExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
    if (wait) {
        ShExecInfo.fMask = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_NO_CONSOLE;
    }
    else {
        ShExecInfo.fMask = SEE_MASK_ASYNCOK | SEE_MASK_NO_CONSOLE;
    }

    ShExecInfo.hwnd = NULL;
    ShExecInfo.lpVerb = NULL;
    ShExecInfo.lpFile = command.c_str();
    ShExecInfo.lpParameters = NULL;
    ShExecInfo.lpDirectory = NULL;
    ShExecInfo.nShow = SW_HIDE;
    ShExecInfo.hInstApp = NULL;
    ShExecInfo.lpParameters = parameters_string.c_str();

    ShellExecuteExW(&ShExecInfo);
    if (wait) {
        WaitForSingleObject(ShExecInfo.hProcess, INFINITE);
        CloseHandle(ShExecInfo.hProcess);
    }

#else
    // todo: use setProcessChanellMode to use the same console as the parent process
    QProcess* process = new QProcess;
    QString qcommand = QString::fromStdWString(command);
    QStringList qparameters;

    QObject::connect(process, &QProcess::errorOccurred, [process](QProcess::ProcessError error) {
        auto msg = process->errorString().toStdWString();
        show_error_message(msg);
        });

    QObject::connect(process, qOverload<int, QProcess::ExitStatus >(&QProcess::finished), [process](int exit_code, QProcess::ExitStatus stat) {
        process->deleteLater();
        });

    for (int i = 0; i < parameters.size(); i++) {
        qparameters.append(parameters[i]);
    }
    //qparameters.append(QString::fromStdWString(parameters));

    if (wait) {
        process->start(qcommand, qparameters);
        process->waitForFinished();
    }
    else {
        process->startDetached(qcommand, qparameters);
    }
#endif

}


void open_file_url(const QString& url_string, bool show_fail_message) {
    if (!QDesktopServices::openUrl(QUrl::fromLocalFile(url_string))) {
        show_error_message(("Could not open address: " + url_string).toStdWString());
    }
}

void open_file_url(const std::wstring& url_string, bool show_fail_message) {
    QString qurl_string = QString::fromStdWString(url_string);
    open_file_url(qurl_string, show_fail_message);
}

void open_web_url(const QString& url_string) {
    QDesktopServices::openUrl(QUrl(url_string));
}

void open_web_url(const std::wstring& url_string) {
    QString qurl_string = QString::fromStdWString(url_string);
    open_web_url(qurl_string);
}


void search_custom_engine(const std::wstring& search_string, const std::wstring& custom_engine_url) {

    if (search_string.size() > 0) {
        QString qurl_string = QString::fromStdWString(custom_engine_url + search_string);
        open_web_url(qurl_string);
    }
}

void search_google_scholar(const std::wstring& search_string) {
    search_custom_engine(search_string, GOOGLE_SCHOLAR_ADDRESS);
}

void search_libgen(const std::wstring& search_string) {
    search_custom_engine(search_string, LIBGEN_ADDRESS);
}



//void open_url(const std::wstring& url_string) {
//
//	if (url_string.size() > 0) {
//		QString qurl_string = QString::fromStdWString(url_string);
//		open_url(qurl_string);
//	}
//}
//
void create_file_if_not_exists(const std::wstring& path) {
    std::string path_utf8 = utf8_encode(path);
    if (!QFile::exists(QString::fromStdWString(path))) {
        std::ofstream outfile(path_utf8);
        outfile << "";
        outfile.close();
    }
}


void open_file(const std::wstring& path, bool show_fail_message) {
    std::wstring canon_path = get_canonical_path(path);
    open_file_url(canon_path, show_fail_message);

}

void get_text_from_flat_chars(const std::vector<fz_stext_char*>& flat_chars, std::wstring& string_res, std::vector<int>& indices) {

    string_res.clear();
    indices.clear();

    for (size_t i = 0; i < flat_chars.size(); i++) {
        fz_stext_char* ch = flat_chars[i];

        if (ch->next == nullptr) { // add a space after the last character in a line, igonre hyphenated characters
            if (ch->c != '-') {
                string_res.push_back(ch->c);
                indices.push_back(i);

                string_res.push_back(' ');
                indices.push_back(-1);
            }
            continue;
        }
        string_res.push_back(ch->c);
        indices.push_back(i);
    }
}


std::wstring find_first_regex_match(const std::wstring& haystack, const std::wstring& regex_string) {
    std::wregex regex(regex_string);
    std::wsmatch match;
    if (std::regex_search(haystack, match, regex)) {
        return match.str();
    }
    return L"";
}

std::vector<std::wstring> find_all_regex_matches(std::wstring haystack,
    const std::wstring& regex_string,
    std::vector<std::pair<int, int>>* match_ranges) {

    std::wregex regex(regex_string);
    std::wsmatch match;
    std::vector<std::wstring> res;
    int skipped_length = 0;

    while (std::regex_search(haystack, match, regex)) {
        for (size_t i = 0; i < match.size(); i++) {
            if (match[i].matched) {
                res.push_back(match[i].str());
                if (match_ranges) {
                    int begin_index = match[i].first - haystack.begin();
                    int match_length = match[i].length();
                    match_ranges->push_back(std::make_pair(skipped_length + begin_index, skipped_length + begin_index + match_length-1));
                }
            }
        }
        skipped_length += match.prefix().length() + match.length();
        haystack = match.suffix();
    }
    return res;

}

void find_regex_matches_in_stext_page(const std::vector<fz_stext_char*>& flat_chars,
    const std::wregex& regex,
    std::vector<std::pair<int, int>>& match_ranges, std::vector<std::wstring>& match_texts) {

    std::wstring page_string;
    std::vector<int> indices;

    get_text_from_flat_chars(flat_chars, page_string, indices);

    std::wsmatch match;

    int offset = 0;
    while (std::regex_search(page_string, match, regex)) {
        int start_index = offset + match.position();
        int end_index = start_index + match.length() - 1;
        match_ranges.push_back(std::make_pair(indices[start_index], indices[end_index]));
        match_texts.push_back(match.str());

        int old_length = page_string.size();
        page_string = match.suffix();
        int new_length = page_string.size();

        offset += (old_length - new_length);
    }
}

bool are_stext_chars_far_enough_for_equation(fz_stext_char* first, fz_stext_char* second) {
    float second_width = second->quad.lr.x - second->quad.ll.x;

    if (second_width < 0) {
        return false;
    }

    return (second->origin.x - first->origin.x) > (5 * second_width);
}

bool is_whitespace(int chr) {
    if ((chr == ' ') || (chr == '\n') || (chr == '\t')) {
        return true;
    }
    return false;
}

std::wstring strip_string(std::wstring& input_string) {

    std::wstring result;
    int start_index = 0;
    int end_index = input_string.size() - 1;
    if (input_string.size() == 0) {
        return L"";
    }

    while (is_whitespace(input_string[start_index])) {
        start_index++;
        if ((size_t)start_index == input_string.size()) {
            return L"";
        }
    }

    while (is_whitespace(input_string[end_index])) {
        end_index--;
    }
    return input_string.substr(start_index, end_index - start_index + 1);

}

void index_generic(const std::vector<fz_stext_char*>& flat_chars, int page_number, std::vector<IndexedData>& indices) {

    std::wstring page_string;
    std::vector<std::optional<fz_rect>> page_rects;

    for (auto ch : flat_chars) {
        page_string.push_back(ch->c);
        page_rects.push_back(fz_rect_from_quad(ch->quad));
        if (ch->next == nullptr) {
            page_string.push_back('\n');
            page_rects.push_back({});
        }
    }

    std::wregex index_dst_regex(L"(^|\n)[A-Z][a-zA-Z]{2,}\.?[ \t]+[0-9]+(\.[0-9]+)*");
    //std::wregex index_dst_regex(L"(^|\n)[A-Z][a-zA-Z]{2,}[ \t]+[0-9]+(\-[0-9]+)*");
    //std::wregex index_src_regex(L"[a-zA-Z]{3,}[ \t]+[0-9]+(\.[0-9]+)*");
    std::wsmatch match;


    int offset = 0;
    while (std::regex_search(page_string, match, index_dst_regex)) {

        IndexedData new_data;
        new_data.page = page_number;
        std::wstring match_string = match.str();
        new_data.text = strip_string(match_string);
        new_data.y_offset = 0.0f;

        int match_start_index = match.position();
        int match_size = match_string.size();
        for (int i = 0; i < match_size; i++) {
            int index = offset + match_start_index + i;
            if (page_rects[index]) {
                new_data.y_offset = page_rects[index].value().y0;
                break;
            }
        }
        offset += match_start_index + match_size;
        page_string = match.suffix();

        indices.push_back(new_data);
    }
}

void index_equations(const std::vector<fz_stext_char*>& flat_chars, int page_number, std::map<std::wstring, std::vector<IndexedData>>& indices) {
    std::wregex regex(L"\\([0-9]+(\\.[0-9]+)*\\)");
    std::vector<std::pair<int, int>> match_ranges;
    std::vector<std::wstring> match_texts;

    find_regex_matches_in_stext_page(flat_chars, regex, match_ranges, match_texts);

    for (size_t i = 0; i < match_ranges.size(); i++) {
        auto [start_index, end_index] = match_ranges[i];
        if (start_index == -1 || end_index == -1) {
            break;
        }


        // we expect the equation reference to be sufficiently separated from the rest of the text
        if (((start_index > 0) && are_stext_chars_far_enough_for_equation(flat_chars[start_index - 1], flat_chars[start_index]))) {

            std::wstring match_text = match_texts[i].substr(1, match_texts[i].size() - 2);
            IndexedData indexed_equation;
            indexed_equation.page = page_number;
            indexed_equation.text = match_text;
            indexed_equation.y_offset = flat_chars[start_index]->quad.ll.y;
            if (indices.find(match_text) == indices.end()) {
                indices[match_text] = std::vector<IndexedData>();
                indices[match_text].push_back(indexed_equation);
            }
            else {
                indices[match_text].push_back(indexed_equation);
            }
        }
    }

}

void index_references(fz_stext_page* page, int page_number, std::map<std::wstring, IndexedData>& indices) {

    char start_char = '[';
    char end_char = ']';
    char delim_char = ',';

    bool started = false;
    std::vector<IndexedData> temp_indices;
    std::wstring current_text = L"";

    LL_ITER(block, page->first_block) {
        if (block->type != FZ_STEXT_BLOCK_TEXT) continue;

        LL_ITER(line, block->u.t.first_line) {
            int chars_in_line = 0;
            LL_ITER(ch, line->first_char) {
                chars_in_line++;
                if (ch->c == ' ') {
                    continue;
                }
                //if (ch->c == '.') {
                //	started = false;
                //	temp_indices.clear();
                //	current_text.clear();
                //}

                // references are usually at the beginning of the line, we consider the possibility
                // of up to 3 extra characters before the reference (e.g. numbers, extra spaces, etc.)
                if ((ch->c == start_char) && (chars_in_line < 4)) {
                    temp_indices.clear();
                    current_text.clear();
                    started = true;
                    continue;
                }
                if (ch->c == end_char) {
                    started = false;

                    IndexedData index_data;
                    index_data.page = page_number;
                    index_data.y_offset = ch->quad.ll.y;
                    index_data.text = current_text;

                    temp_indices.push_back(index_data);
                    //indices[text] = index_data;

                    for (auto index : temp_indices) {
                        indices[index.text] = index;
                    }
                    current_text.clear();
                    temp_indices.clear();
                    continue;
                }
                if (started && (ch->c == delim_char)) {
                    IndexedData index_data;
                    index_data.page = page_number;
                    index_data.y_offset = ch->quad.ll.y;
                    index_data.text = current_text;
                    current_text.clear();
                    temp_indices.push_back(index_data);
                    continue;
                }
                if (started) {
                    current_text.push_back(ch->c);
                }
            }

        }
    }
}

void sleep_ms(unsigned int ms) {
#ifdef Q_OS_WIN
    Sleep(ms);
#else
    struct timespec ts = { ms / 1000, (ms % 1000) * 1000 * 1000 };
    nanosleep(&ts, NULL);
#endif
}

void get_pixmap_pixel(fz_pixmap* pixmap, int x, int y, unsigned char* r, unsigned char* g, unsigned char* b) {
    if (
        (x < 0) ||
        (y < 0) ||
        (x >= pixmap->w) ||
        (y >= pixmap->h)
        ) {
        *r = 0;
        *g = 0;
        *b = 0;
        return;
    }

    (*r) = pixmap->samples[y * pixmap->w * 3 + x * 3 + 0];
    (*g) = pixmap->samples[y * pixmap->w * 3 + x * 3 + 1];
    (*b) = pixmap->samples[y * pixmap->w * 3 + x * 3 + 2];
}

int find_max_horizontal_line_length_at_pos(fz_pixmap* pixmap, int pos_x, int pos_y) {
    int min_x = pos_x;
    int max_x = pos_x;

    while (true) {
        unsigned char r, g, b;
        get_pixmap_pixel(pixmap, min_x, pos_y, &r, &g, &b);
        if ((r != 255) || (g != 255) || (b != 255)) {
            break;
        }
        else {
            min_x--;
        }
    }
    while (true) {
        unsigned char r, g, b;
        get_pixmap_pixel(pixmap, max_x, pos_y, &r, &g, &b);
        if ((r != 255) || (g != 255) || (b != 255)) {
            break;
        }
        else {
            max_x++;
        }
    }
    return max_x - min_x;
}

bool largest_contigous_ones(std::vector<int>& arr, int* start_index, int* end_index) {
    arr.push_back(0);

    bool has_at_least_one_one = false;

    for (auto x : arr) {
        if (x == 1) {
            has_at_least_one_one = true;
        }
    }
    if (!has_at_least_one_one) {
        return false;
    }


    int max_count = 0;
    int max_start_index = -1;
    int max_end_index = -1;

    int count = 0;

    for (size_t i = 0; i < arr.size(); i++) {
        if (arr[i] == 1) {
            count++;
        }
        else {
            if (count > max_count) {
                max_count = count;
                max_end_index = i - 1;
                max_start_index = i - count;
            }
            count = 0;
        }
    }

    *start_index = max_start_index;
    *end_index = max_end_index;
    return true;
}

std::vector<unsigned int> get_max_width_histogram_from_pixmap(fz_pixmap* pixmap) {
    std::vector<unsigned int> res;

    for (int j = 0; j < pixmap->h; j++) {
        unsigned int x_value = 0;
        for (int i = 0; i < pixmap->w; i++) {
            unsigned char r, g, b;
            get_pixmap_pixel(pixmap, i, j, &r, &g, &b);
            float lightness = (static_cast<float>(r) + static_cast<float>(g) + static_cast<float>(b)) / 3.0f;

            if (lightness > 150) {
                x_value += 1;
            }
        }
        res.push_back(x_value);
    }

    return res;
}

template<typename T>
float average_value(std::vector<T> values) {
    T sum = 0;
    for (auto x : values) {
        sum += x;
    }
    return static_cast<float>(sum) / values.size();
}

template<typename T>
float standard_deviation(std::vector<T> values, float average_value) {
    T sum = 0;
    for (auto x : values) {
        sum += (x - average_value) * (x - average_value);
    }
    return std::sqrt(static_cast<float>(sum) / values.size());
}

//std::vector<unsigned int> get_line_ends_from_histogram(std::vector<unsigned int> histogram) {
void get_line_begins_and_ends_from_histogram(std::vector<unsigned int> histogram, std::vector<unsigned int>& res_begins, std::vector<unsigned int>& res) {

    float mean_width = average_value(histogram);
    float std = standard_deviation(histogram, mean_width);

    std::vector<float> normalized_histogram;

    if (std < 0.00001f) {
        return;
    }

    for (auto x : histogram) {
        normalized_histogram.push_back((x - mean_width) / std);
    }

    size_t i = 0;

    while (i < histogram.size()) {

        while ((i < histogram.size()) && (normalized_histogram[i] > 0.2f)) i++;
        res_begins.push_back(i);
        while ((i < histogram.size()) && (normalized_histogram[i] <= 0.21f)) i++;
        if (i == histogram.size()) break;
        res.push_back(i);
    }

    while (res_begins.size() > res.size()) {
        res_begins.pop_back();
    }

    float additional_distance = 0.0f;

    if (res.size() > 5) {
        std::vector<float> line_distances;

        for (size_t i = 0; i < res.size() - 1; i++) {
            line_distances.push_back(res[i + 1] - res[i]);
        }
        std::nth_element(line_distances.begin(), line_distances.begin() + line_distances.size() / 2, line_distances.end());
        additional_distance = line_distances[line_distances.size() / 2];

        for (size_t i = 0; i < res.size(); i++) {
            res[i] += static_cast<unsigned int>(additional_distance / 5.0f);
            res_begins[i] -= static_cast<unsigned int>(additional_distance / 5.0f);
        }

    }

}

int find_best_vertical_line_location(fz_pixmap* pixmap, int doc_x, int doc_y) {


    int search_height = 5;

    float min_candid_y = doc_y;
    float max_candid_y = doc_y + search_height;

    std::vector<int> max_possible_widths;

    for (int candid_y = min_candid_y; candid_y <= max_candid_y; candid_y++) {
        int current_width = find_max_horizontal_line_length_at_pos(pixmap, doc_x, candid_y);
        max_possible_widths.push_back(current_width);
    }

    int max_width_value = -1;

    for (auto w : max_possible_widths) {
        if (w > max_width_value) {
            max_width_value = w;
        }
    }
    std::vector<int> is_max_list;

    for (auto x : max_possible_widths) {
        if (x == max_width_value) {
            is_max_list.push_back(1);
        }
        else {
            is_max_list.push_back(0);
        }
    }

    int start_index, end_index;
    largest_contigous_ones(is_max_list, &start_index, &end_index);

    //return doc_y + (start_index + end_index) / 2;
    return doc_y + start_index;
}

bool is_string_numeric_(const std::wstring& str) {
    if (str.size() == 0) {
        return false;
    }
    for (auto ch : str) {
        if (!std::isdigit(ch)) {
            return false;
        }
    }
    return true;
}

bool is_string_numeric(const std::wstring& str) {
    if (str.size() == 0) {
        return false;
    }

    if (str[0] == '-') {
        return is_string_numeric_(str.substr(1, str.length() - 1));
    }
    else {
        return is_string_numeric_(str);
    }
}


bool is_string_numeric_float(const std::wstring& str) {
    if (str.size() == 0) {
        return false;
    }
    int dot_count = 0;

    for (size_t i = 0; i < str.size(); i++) {
        if (i == 0) {
            if (str[i] == '-') continue;
        }
        else {
            if (str[i] == '.') {
                dot_count++;
                if (dot_count >= 2) return false;
            }
            else if (!std::isdigit(str[i])) {
                return false;
            }
        }

    }
    return true;
}

QByteArray serialize_string_array(const QStringList& string_list) {
    QByteArray result;
    QDataStream stream(&result, QIODevice::WriteOnly);
    stream << static_cast<int>(string_list.size());
    for (int i = 0; i < string_list.size(); i++) {
        stream << string_list.at(i);
    }
    return result;
}


QStringList deserialize_string_array(const QByteArray& byte_array) {
    QStringList result;
    QDataStream stream(byte_array);

    int size;
    stream >> size;

    for (int i = 0; i < size; i++) {
        QString string;
        stream >> string;
        result.append(string);
    }

    return result;
}




char* get_argv_value(int argc, char** argv, std::string key) {
    for (int i = 0; i < argc - 1; i++) {
        if (key == argv[i]) {
            return argv[i + 1];
        }
    }
    return nullptr;
}

bool has_arg(int argc, char** argv, std::string key) {
    for (int i = 0; i < argc; i++) {
        if (key == argv[i]) {
            return true;
        }
    }
    return false;
}
bool should_reuse_instance(int argc, char** argv) {
    for (int i = 0; i < argc; i++) {
        if (std::strcmp(argv[i], "--reuse-instance") == 0) return true;

    }
    return false;
}

bool should_new_instance(int argc, char** argv) {
    for (int i = 0; i < argc; i++) {
        if (std::strcmp(argv[i], "--new-instance") == 0) return true;

    }
    return false;
}

QCommandLineParser* get_command_line_parser() {

    QCommandLineParser* parser = new QCommandLineParser();

    parser->setApplicationDescription("Sioyek is a PDF reader designed for reading research papers and technical books.");
    //parser->addVersionOption();

    //QCommandLineOption reuse_instance_option("reuse-instance");
    //reuse_instance_option.setDescription("When opening a new file, reuse the previous instance of sioyek instead of opening a new window.");
    //parser->addOption(reuse_instance_option);

    //QCommandLineOption new_instance_option("new-instance");
    //new_instance_option.setDescription("When opening a new file, create a new instance of sioyek.");
    //parser->addOption(new_instance_option);

    QCommandLineOption new_window_option("new-window");
    new_window_option.setDescription("Open the file in a new window but within the same sioyek instance (reuses the previous window if a sioyek window with the same file already exists).");
    parser->addOption(new_window_option);

    QCommandLineOption reuse_window_option("reuse-window");
    reuse_window_option.setDescription("Force sioyek to reuse the current window even when should_launch_new_window is set.");
    parser->addOption(reuse_window_option);

    QCommandLineOption nofocus_option("nofocus");
    nofocus_option.setDescription("Do not bring the sioyek instance to foreground.");
    parser->addOption(nofocus_option);

    QCommandLineOption version_option("version");
    version_option.setDescription("Print sioyek version.");
    parser->addOption(version_option);


    QCommandLineOption page_option("page", "Which page to open.", "page");
    parser->addOption(page_option);

    QCommandLineOption focus_option("focus-text", "Set a visual mark on line including <text>.", "focus-text");
    parser->addOption(focus_option);

    QCommandLineOption focus_page_option("focus-text-page", "Specifies the page which is used for focus-text", "focus-text-page");
    parser->addOption(focus_page_option);


    QCommandLineOption inverse_search_option("inverse-search", "The command to execute when performing inverse search.\
 In <command>, %1 is filled with the file name and %2 is filled with the line number.", "command");
    parser->addOption(inverse_search_option);

    QCommandLineOption command_option("execute-command", "The command to execute on running instance of sioyek", "execute-command");
    parser->addOption(command_option);

    QCommandLineOption command_data_option("execute-command-data", "Optional data for execute-command command", "execute-command-data");
    parser->addOption(command_data_option);

    QCommandLineOption forward_search_file_option("forward-search-file", "Perform forward search on file <file>. You must also include --forward-search-line to specify the line", "file");
    parser->addOption(forward_search_file_option);

    QCommandLineOption forward_search_line_option("forward-search-line", "Perform forward search on line <line>. You must also include --forward-search-file to specify the file", "line");
    parser->addOption(forward_search_line_option);

    QCommandLineOption forward_search_column_option("forward-search-column", "Perform forward search on column <column>. You must also include --forward-search-file to specify the file", "column");
    parser->addOption(forward_search_column_option);

    QCommandLineOption zoom_level_option("zoom", "Set zoom level to <zoom>.", "zoom");
    parser->addOption(zoom_level_option);

    QCommandLineOption xloc_option("xloc", "Set x position within page to <xloc>.", "xloc");
    parser->addOption(xloc_option);

    QCommandLineOption yloc_option("yloc", "Set y position within page to <yloc>.", "yloc");
    parser->addOption(yloc_option);

    QCommandLineOption window_id_option("window-id", "Apply command to window with id <window-id>", "window-id");
    parser->addOption(window_id_option);

    QCommandLineOption shared_database_path_option("shared-database-path", "Specify which file to use for shared data (bookmarks, highlights, etc.)", "path");
    parser->addOption(shared_database_path_option);

    QCommandLineOption verbose_option("verbose", "Print extra information in commnad line.");
    parser->addOption(verbose_option);

    QCommandLineOption wait_for_response_option("wait-for-response", "Wait for the command to finish before returning.");
    parser->addOption(wait_for_response_option);

    QCommandLineOption no_auto_config_option("no-auto-config", "Disables all config files except the ones next to the executable. Used mainly for testing.");
    parser->addOption(no_auto_config_option);

    parser->addHelpOption();

    return parser;
}


std::wstring concatenate_path(const std::wstring& prefix, const std::wstring& suffix) {
    std::wstring result = prefix;
#ifdef Q_OS_WIN
    wchar_t separator = '\\';
#else
    wchar_t separator = '/';
#endif
    if (prefix == L"") {
        return suffix;
    }

    if (result[result.size() - 1] != separator) {
        result.push_back(separator);
    }
    result.append(suffix);
    return result;
}

std::wstring get_canonical_path(const std::wstring& path) {
#ifdef SIOYEK_ANDROID
    if (path.size() > 0) {
        if (path[0] == ':') { // it is a resouce file
            return path;
        }
        if (path.substr(0, 9) == L"content:/") {
            return path;
        }
        else {
            QDir dir(QString::fromStdWString(path));
            return std::move(dir.absolutePath().toStdWString());
        }
    }
    else {
        return L"";
    }
#else
    QDir dir(QString::fromStdWString(path));
    //return std::move(dir.canonicalPath().toStdWString());
    return std::move(dir.absolutePath().toStdWString());
#endif

}

std::wstring add_redundant_dot_to_path(const std::wstring& path) {
    std::vector<std::wstring> parts;
    split_path(path, parts);

    std::wstring last = parts[parts.size() - 1];
    parts.erase(parts.begin() + parts.size() - 1);
    parts.push_back(L".");
    parts.push_back(last);

    std::wstring res = L"";
    if (path[0] == '/') {
        res = L"/";
    }

    for (size_t i = 0; i < parts.size(); i++) {
        res.append(parts[i]);
        if (i < parts.size() - 1) {
            res.append(L"/");
        }
    }
    return res;
}

float manhattan_distance(float x1, float y1, float x2, float y2) {
    return abs(x1 - x2) + abs(y1 - y2);
}

float manhattan_distance(fvec2 v1, fvec2 v2) {
    return manhattan_distance(v1.x(), v1.y(), v2.x(), v2.y());
}

QWidget* get_top_level_widget(QWidget* widget) {
    while (widget->parent() != nullptr) {
        widget = widget->parentWidget();
    }
    return widget;
}

float type_name_similarity_score(std::wstring name1, std::wstring name2) {
    name1 = to_lower(name1);
    name2 = to_lower(name2);
    size_t common_prefix_index = 0;

    while (name1[common_prefix_index] == name2[common_prefix_index]) {
        common_prefix_index++;
        if ((common_prefix_index == name1.size()) || (common_prefix_index == name2.size())) {
            return common_prefix_index;
        }
    }
    return common_prefix_index;
}

void check_for_updates(QWidget* parent, std::string current_version) {

    return;
    //QString url = "https://github.com/ahrm/sioyek/releases/latest";
    //QNetworkAccessManager* manager = new QNetworkAccessManager;

    //QObject::connect(manager, &QNetworkAccessManager::finished, [=](QNetworkReply *reply) {
    //	std::string response_text = reply->readAll().toStdString();
    //	int first_index = response_text.find("\"");
    //	int last_index = response_text.rfind("\"");
    //	std::string url_string = response_text.substr(first_index + 1, last_index - first_index - 1);

    //	std::vector<std::wstring> parts;
    //	split_path(utf8_decode(url_string), parts);
    //	if (parts.size() > 0) {
    //		std::string version_string = utf8_encode(parts.back().substr(1, parts.back().size() - 1));

    //		if (version_string != current_version) {
    //			int ret = QMessageBox::information(parent, "Update", QString::fromStdString("Do you want to update from " + current_version + " to " + version_string + "?"),
    //				QMessageBox::Ok | QMessageBox::Cancel,
    //				QMessageBox::Cancel);
    //			if (ret == QMessageBox::Ok) {
    //				open_web_url(url);
    //			}
    //		}

    //	}
    //	});
    //manager->get(QNetworkRequest(QUrl(url)));
}

QString expand_home_dir(QString path) {
    if (path.size() > 0) {
        if (path.at(0) == '~') {
            return QDir::homePath() + QDir::separator() + path.remove(0, 2);
        }
    }
    return path;
}

void split_root_file(QString path, QString& out_root, QString& out_partial) {

    QChar sep = QDir::separator();
    if (path.indexOf(sep) == -1) {
        sep = '/';
    }

    QStringList parts = path.split(sep);

    if (path.size() > 0) {
        if (path.at(path.size() - 1) == sep) {
            out_root = parts.join(sep);
        }
        else {
            if ((parts.size() == 2) && (path.at(0) == '/')) {
                out_root = "/";
                out_partial = parts.at(1);
            }
            else {
                out_partial = parts.at(parts.size() - 1);
                parts.pop_back();
                out_root = parts.join(QDir::separator());
            }
        }
    }
    else {
        out_partial = "";
        out_root = "";
    }
}


std::wstring lowercase(const std::wstring& input) {

    std::wstring res;
    for (int i = 0; i < input.size(); i++) {
        if ((input[i] >= 'A') && (input[i] <= 'Z')) {
            res.push_back(input[i] + 'a' - 'A');
        }
        else {
            res.push_back(input[i]);
        }
    }
    return res;
}

int hex2int(int hex) {
    if (hex >= '0' && hex <= '9') {
        return hex - '0';
    }
    else {
        return (hex - 'a') + 10;
    }
}
float get_color_component_from_hex(std::wstring hexcolor) {
    hexcolor = lowercase(hexcolor);

    if (hexcolor.size() < 2) {
        return 0;
    }
    return static_cast<float>(hex2int(hexcolor[0]) * 16 + hex2int(hexcolor[1])) / 255.0f;
}

QString get_color_hexadecimal(float color) {
    QString hex_map[] = { "0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "a", "b", "c", "d", "e", "f" };
    int val = static_cast<int>(color * 255);
    int high = val / 16;
    int low = val % 16;
    return QString("%1%2").arg(hex_map[high], hex_map[low]);

}

QString get_color_qml_string(float r, float g, float b) {
    QString res = QString("#%1%2%3").arg(get_color_hexadecimal(r), get_color_hexadecimal(g), get_color_hexadecimal(b));
    return res;
}

void hexademical_to_normalized_color(std::wstring color_string, float* color, int n_components) {
    if (color_string[0] == '#') {
        color_string = color_string.substr(1, color_string.size() - 1);
    }

    for (int i = 0; i < n_components; i++) {
        *(color + i) = get_color_component_from_hex(color_string.substr(i * 2, 2));
    }
}

void copy_file(std::wstring src_path, std::wstring dst_path) {
    std::ifstream  src(utf8_encode(src_path), std::ios::binary);
    std::ofstream  dst(utf8_encode(dst_path), std::ios::binary);

    dst << src.rdbuf();
}

fz_quad quad_from_rect(fz_rect r)
{
    fz_quad q;
    q.ul = fz_make_point(r.x0, r.y0);
    q.ur = fz_make_point(r.x1, r.y0);
    q.ll = fz_make_point(r.x0, r.y1);
    q.lr = fz_make_point(r.x1, r.y1);
    return q;
}


std::wifstream open_wifstream(const std::wstring& file_name) {

#ifdef Q_OS_WIN
    return std::move(std::wifstream(file_name));
#else
    std::string encoded_file_name = utf8_encode(file_name);
    return std::move(std::wifstream(encoded_file_name.c_str()));
#endif
}

std::wofstream open_wofstream(const std::wstring& file_name) {

#ifdef Q_OS_WIN
    return std::move(std::wofstream(file_name));
#else
    std::string encoded_file_name = utf8_encode(file_name);
    return std::move(std::wofstream(encoded_file_name.c_str()));
#endif
}

std::wstring truncate_string(const std::wstring& inp, int size) {
    if (inp.size() <= (size_t)size) {
        return inp;
    }
    else {
        return inp.substr(0, size - 3) + L"...";
    }

}

std::wstring get_page_formatted_string(int page) {
    std::wstringstream ss;
    ss << L"[ " << page << L" ]";
    return ss.str();
}

bool is_string_titlish(const std::wstring& str) {
    if (str.size() <= 5 || str.size() >= 60) {
        return false;
    }
    std::wregex regex(L"([0-9IVXC]+\\.)+([0-9IVXC]+)*");
    std::wsmatch match;

    std::regex_search(str, match, regex);
    int pos = match.position();
    int size = match.length();
    return (size > 0) && (pos == 0);
}

bool is_title_parent_of(const std::wstring& parent_title, const std::wstring& child_title, bool* are_same) {
    int count = std::min(parent_title.size(), child_title.size());

    *are_same = false;

    for (int i = 0; i < count; i++) {
        if (parent_title.at(i) == ' ') {
            if (child_title.at(i) == ' ') {
                *are_same = true;
                return false;
            }
            else {
                return true;
            }
        }
        if (child_title.at(i) != parent_title.at(i)) {
            return false;
        }
    }

    return true;
}

struct Range {
    float begin;
    float end;

    float size() {
        return end - begin;
    }
};

struct Range merge_range(struct Range range1, struct Range range2) {
    struct Range res;
    res.begin = std::min(range1.begin, range2.begin);
    res.end = std::max(range1.end, range2.end);
    return res;
}

float line_num_penalty(int num) {
    return 1.0f;
    //if (num == 1) {
    //	return 1.0f;
    //}
    //return 1.0f + static_cast<float>(num) / 5.0f;
}

float height_increase_penalty(float ratio) {
    return  50 * ratio;
}

float width_increase_bonus(float ratio) {
    return -50 * ratio;
}

int find_best_merge_index_for_line_index(const std::vector<fz_stext_line*>& lines,
    const std::vector<PagelessDocumentRect>& line_rects,
    const std::vector<int> char_counts,
    int index) {

    return index;
    int max_merged_lines = 40;
    //Range current_range = { lines[index]->bbox.y0, lines[index]->bbox.y1 };
    //Range current_range_x = { lines[index]->bbox.x0, lines[index]->bbox.x1 };
    struct Range current_range = { line_rects[index].y0, line_rects[index].y1 };
    struct Range current_range_x = { line_rects[index].x0, line_rects[index].x1 };
    float maximum_height = current_range.size();
    float maximum_width = current_range_x.size();
    float min_cost = current_range.size() * line_num_penalty(1) / current_range_x.size();
    int min_index = index;

    for (size_t j = index + 1; (j < lines.size()) && ((j - index) < (size_t)max_merged_lines); j++) {
        float line_height = line_rects[j].y1 - line_rects[j].y0;
        float line_width = line_rects[j].x1 - line_rects[j].x0;
        if (line_height > maximum_height) {
            maximum_height = line_height;
        }
        if (line_width > maximum_width) {
            maximum_width = line_width;
        }
        if (char_counts[j] > 10) {
            current_range = merge_range(current_range, { line_rects[j].y0, line_rects[j].y1 });
        }

        current_range_x = merge_range(current_range, { line_rects[j].x0, line_rects[j].x1 });

        float cost = current_range.size() / (j - index + 1) +
            line_num_penalty(j - index + 1) / current_range_x.size() +
            height_increase_penalty(current_range.size() / maximum_height) +
            width_increase_bonus(current_range_x.size() / maximum_width);
        if (cost < min_cost) {
            min_cost = cost;
            min_index = j;
        }
    }
    return min_index;
}


fz_rect get_line_rect(fz_stext_line* line) {
    fz_rect res;
    res.x0 = res.x1 = res.y0 = res.y1 = 0;

    if (line == nullptr || line->first_char == nullptr) {
        return res;
    }

    res = fz_rect_from_quad(line->first_char->quad);

    std::vector<float> char_x_begins;
    std::vector<float> char_x_ends;
    std::vector<float> char_y_begins;
    std::vector<float> char_y_ends;

    int num_chars = 0;
    LL_ITER(chr, line->first_char) {
        fz_rect char_rect = fz_union_rect(res, fz_rect_from_quad(chr->quad));
        char_x_ends.push_back(char_rect.x1);
        char_y_begins.push_back(char_rect.y0);
        char_y_ends.push_back(char_rect.y1);

        if (char_rect.x0 > 0) {
            char_x_begins.push_back(char_rect.x0);
        }
        num_chars++;
    }


    int percentile_index = static_cast<int>(0.5f * num_chars);
    int first_percentile_index = percentile_index;
    int last_percentile_index = num_chars - percentile_index;

    if (last_percentile_index >= num_chars) {
        last_percentile_index = num_chars - 1;
    }

    if (char_x_begins.size() > 0) {
        res.x0 = *std::min_element(char_x_begins.begin(), char_x_begins.end());
        res.x1 = *std::max_element(char_x_ends.begin(), char_x_ends.end());
    }
    if (char_y_begins.size() > 0) {
        std::nth_element(char_y_begins.begin(), char_y_begins.begin() + first_percentile_index, char_y_begins.end());
        std::nth_element(char_y_ends.begin(), char_y_ends.begin() + last_percentile_index, char_y_ends.end());
        res.y0 = *(char_y_begins.begin() + first_percentile_index);
        res.y1 = *(char_y_ends.begin() + last_percentile_index);
    }

    return res;
}

int line_num_chars(fz_stext_line* line) {
    int res = 0;
    LL_ITER(chr, line->first_char) {
        res++;
    }
    return res;
}


void merge_lines(
    std::vector<fz_stext_line*> lines,
    std::vector<PagelessDocumentRect>& out_rects,
    std::vector<std::wstring>& out_texts,
    std::vector<std::vector<PagelessDocumentRect>>* out_line_chars) {

    std::vector<PagelessDocumentRect> temp_rects;
    std::vector<std::wstring> temp_texts;
    std::vector<std::vector<PagelessDocumentRect>> temp_line_chars;

    std::vector<PagelessDocumentRect> custom_line_rects;
    std::vector<int> char_counts;

    std::vector<int> indices_to_delete;
    for (size_t i = 0; i < lines.size(); i++) {
        if (line_num_chars(lines[i]) < 5) {
            indices_to_delete.push_back(i);
        }
    }

    for (int i = indices_to_delete.size() - 1; i >= 0; i--) {
        lines.erase(lines.begin() + indices_to_delete[i]);
    }

    for (auto line : lines) {
        custom_line_rects.push_back(get_line_rect(line));
        char_counts.push_back(line_num_chars(line));
    }

    for (size_t i = 0; i < lines.size(); i++) {
        PagelessDocumentRect rect = custom_line_rects[i];
        int best_index = find_best_merge_index_for_line_index(lines, custom_line_rects, char_counts, i);
        std::wstring text = get_string_from_stext_line(lines[i]);
        std::vector<PagelessDocumentRect> line_chars;
        if (out_line_chars) {
            line_chars = get_char_rects_from_stext_line(lines[i]);
        }

        for (int j = i + 1; j <= best_index; j++) {
            rect = fz_union_rect(rect, lines[j]->bbox);
            text = text + get_string_from_stext_line(lines[j]);
            if (out_line_chars) {
                std::vector<PagelessDocumentRect> merged_line_chars = get_char_rects_from_stext_line(lines[j]);
                line_chars.insert(line_chars.end(), merged_line_chars.begin(), merged_line_chars.end());
            }
        }

        temp_rects.push_back(rect);
        temp_texts.push_back(text);
        if (out_line_chars) {
            temp_line_chars.push_back(line_chars);
        }
        i = best_index;
    }
    for (size_t i = 0; i < temp_rects.size(); i++) {
        if (i > 0 && out_rects.size() > 0) {
            fz_rect prev_rect = out_rects[out_rects.size() - 1];
            fz_rect current_rect = temp_rects[i];
            if ((std::abs(prev_rect.y0 - current_rect.y0) < 1.0f) || (std::abs(prev_rect.y1 - current_rect.y1) < 1.0f)) {
                out_rects[out_rects.size() - 1].x0 = std::min(prev_rect.x0, current_rect.x0);
                out_rects[out_rects.size() - 1].x1 = std::max(prev_rect.x1, current_rect.x1);

                out_rects[out_rects.size() - 1].y0 = std::min(prev_rect.y0, current_rect.y0);
                out_rects[out_rects.size() - 1].y1 = std::max(prev_rect.y1, current_rect.y1);
                out_texts[out_texts.size() - 1] = out_texts[out_texts.size() - 1] + temp_texts[i];
                if (out_line_chars) {
                    (*out_line_chars)[out_line_chars->size()-1].insert((*out_line_chars)[out_line_chars->size()-1].end(), temp_line_chars[i].begin(), temp_line_chars[i].end());
                }
                continue;
            }
        }
        out_rects.push_back(temp_rects[i]);
        out_texts.push_back(temp_texts[i]);
        if (out_line_chars) {
            out_line_chars->push_back(temp_line_chars[i]);
        }
    }
}


int lcs(const char* X, const char* Y, int m, int n)
{
    //int L[m + 1][n + 1];
    std::vector<std::vector<int>> L;
    for (int i = 0; i < m + 1; i++) {
        L.push_back(std::vector<int>(n + 1));
    }

    int i, j;

    /* Following steps build L[m+1][n+1] in bottom up fashion. Note
      that L[i][j] contains length of LCS of X[0..i-1] and Y[0..j-1] */
    for (i = 0; i <= m; i++) {
        for (j = 0; j <= n; j++) {
            if (i == 0 || j == 0)
                L[i][j] = 0;

            else if (X[i - 1] == Y[j - 1])
                L[i][j] = L[i - 1][j - 1] + 1;

            else
                L[i][j] = std::max(L[i - 1][j], L[i][j - 1]);
        }
    }

    /* L[m][n] contains length of LCS for X[0..n-1] and Y[0..m-1] */
    return L[m][n];
}

bool command_requires_text(const std::wstring& command) {
    if ((command.find(L"%5") != -1) || (command.find(L"command_text") != -1)) {
        return true;
    }
    return false;
}

bool command_requires_rect(const std::wstring& command) {
    if (command.find(L"%{selected_rect}") != -1) {
        return true;
    }
    return false;
}

void parse_command_string(std::wstring command_string, std::string& command_name, std::wstring& command_data) {
    int lindex = command_string.find(L"(");
    int rindex = command_string.rfind(L")");
    if (lindex < rindex) {
        command_name = utf8_encode(command_string.substr(0, lindex));
        command_data = command_string.substr(lindex + 1, rindex - lindex - 1);
    }
    else {
        command_data = L"";
        command_name = utf8_encode(command_string);
    }
}

void parse_color(std::wstring color_string, float* out_color, int n_components) {
    if (color_string.size() > 0) {
        if (color_string[0] == '#') {
            hexademical_to_normalized_color(color_string, out_color, n_components);
        }
        else {
            std::wstringstream ss(color_string);

            for (int i = 0; i < n_components; i++) {
                ss >> *(out_color + i);
            }
        }
    }
}

int get_status_bar_height() {
    if (STATUS_BAR_FONT_SIZE > 0) {
        return STATUS_BAR_FONT_SIZE + 5;
    }
    else {
        return 20;
    }
}

void flat_char_prism(const std::vector<fz_stext_char*>& chars, int page, std::wstring& output_text, std::vector<int>& pages, std::vector<PagelessDocumentRect>& rects) {
    fz_stext_char* last_char = nullptr;

    for (int j = 0; j < chars.size(); j++) {
        if (is_line_separator(last_char, chars[j])) {
            if (last_char->c == '-') {
                pages.pop_back();
                rects.pop_back();
                output_text.pop_back();
            }
            else {
                pages.push_back(page);
                rects.push_back(fz_rect_from_quad(chars[j]->quad));
                output_text.push_back(' ');
            }
        }
        pages.push_back(page);
        rects.push_back(fz_rect_from_quad(chars[j]->quad));
        output_text.push_back(chars[j]->c);
        last_char = chars[j];
    }
}

QString get_color_stylesheet(float* bg_color, float* text_color, bool nofont, int font_size) {
    if ((!nofont) && (STATUS_BAR_FONT_SIZE > -1 || font_size > -1)) {
        int size = font_size > 0 ? font_size : STATUS_BAR_FONT_SIZE;
        QString	font_size_stylesheet = QString("font-size: %1px").arg(size);
        return QString("background-color: %1; color: %2; border: 0; %3;").arg(
            get_color_qml_string(bg_color[0], bg_color[1], bg_color[2]),
            get_color_qml_string(text_color[0], text_color[1], text_color[2]),
            font_size_stylesheet
        );
    }
    else {
        return QString("background-color: %1; color: %2; border: 0;").arg(
            get_color_qml_string(bg_color[0], bg_color[1], bg_color[2]),
            get_color_qml_string(text_color[0], text_color[1], text_color[2])
        );
    }
}

QString get_ui_stylesheet(bool nofont, int font_size) {
    return get_color_stylesheet(UI_BACKGROUND_COLOR, UI_TEXT_COLOR, nofont, font_size);
}

QString get_status_stylesheet(bool nofont, int font_size) {
    return get_color_stylesheet(STATUS_BAR_COLOR, STATUS_BAR_TEXT_COLOR, nofont, font_size);
}

QString get_list_item_stylesheet() {
    return QString("background-color: red; padding-bottom: 20px; padding-top: 20px;");
}

QString get_selected_stylesheet(bool nofont) {
    if ((!nofont) && STATUS_BAR_FONT_SIZE > -1) {
        QString	font_size_stylesheet = QString("font-size: %1px").arg(STATUS_BAR_FONT_SIZE);
        return QString("background-color: %1; color: %2; border: 0; %3;").arg(
            get_color_qml_string(UI_SELECTED_BACKGROUND_COLOR[0], UI_SELECTED_BACKGROUND_COLOR[1], UI_SELECTED_BACKGROUND_COLOR[2]),
            get_color_qml_string(UI_SELECTED_TEXT_COLOR[0], UI_SELECTED_TEXT_COLOR[1], UI_SELECTED_TEXT_COLOR[2]),
            font_size_stylesheet
        );
    }
    else {
        return QString("background-color: %1; color: %2; border: 0;").arg(
            get_color_qml_string(UI_SELECTED_BACKGROUND_COLOR[0], UI_SELECTED_BACKGROUND_COLOR[1], UI_SELECTED_BACKGROUND_COLOR[2]),
            get_color_qml_string(UI_SELECTED_TEXT_COLOR[0], UI_SELECTED_TEXT_COLOR[1], UI_SELECTED_TEXT_COLOR[2])
        );
    }
}

void convert_color4(float* in_color, int* out_color) {
    out_color[0] = (int)(in_color[0] * 255);
    out_color[1] = (int)(in_color[1] * 255);
    out_color[2] = (int)(in_color[2] * 255);
    out_color[3] = (int)(in_color[3] * 255);
}

void convert_color3(float* in_color, int* out_color) {
    out_color[0] = (int)(in_color[0] * 255);
    out_color[1] = (int)(in_color[1] * 255);
    out_color[2] = (int)(in_color[2] * 255);
}

#ifdef SIOYEK_ANDROID

QJniObject parseUriString(const QString& uriString) {
    return QJniObject::callStaticObjectMethod
    ("android/net/Uri", "parse",
        "(Ljava/lang/String;)Landroid/net/Uri;",
        QJniObject::fromString(uriString).object());
}

QString android_file_uri_from_content_uri(QString uri) {

    //    mainActivityObj = QtAndroid::androidActivity();
    QJniObject activity = QNativeInterface::QAndroidApplication::context();
    QJniObject uri_object = parseUriString(uri);

    QJniObject file_uri_object = QJniObject::callStaticObjectMethod("info/sioyek/sioyek/SioyekActivity",
        "getPathFromUri",
        "(Landroid/content/Context;Landroid/net/Uri;)Ljava/lang/String;", activity.object(), uri_object.object());
    return file_uri_object.toString();
}
#endif

fz_document* open_document_with_file_name(fz_context* context, std::wstring file_name) {

#ifdef SIOYEK_ANDROID

    QFile pdf_qfile(QString::fromStdWString(file_name));

    pdf_qfile.open(QIODeviceBase::ReadOnly);
    int qfile_handle = pdf_qfile.handle();
    fz_stream* stream = nullptr;


    if (qfile_handle != -1) {
        FILE* file_ptr = fdopen(dup(qfile_handle), "rb");
        stream = fz_open_file_ptr_no_close(context, file_ptr);
    }
    else {
        QByteArray bytes = pdf_qfile.readAll();
        int size = bytes.size();
        unsigned char* new_buffer = new unsigned char[size];
        memcpy(new_buffer, bytes.data(), bytes.size());
        stream = fz_open_memory(context, new_buffer, bytes.size());
    }

    //return fz_open_document_with_stream(context, "application/pdf", stream);
    std::string file_name_str = utf8_encode(file_name);
    return fz_open_document_with_stream(context, file_name_str.c_str(), stream);
#else
    fz_document* doc = fz_open_document(context, utf8_encode(file_name).c_str());
    if (fz_is_document_reflowable(context, doc)) {

        if (EPUB_CSS.size() > 0) {
            std::string css = utf8_encode(EPUB_CSS);
            fz_set_user_css(context, css.c_str());
        }

        fz_layout_document(context, doc, EPUB_WIDTH, EPUB_HEIGHT, EPUB_FONT_SIZE);

        //int a = 2;
    }
    return doc;
#endif
}

void convert_qcolor_to_float3(const QColor& color, float* out_floats) {
    *(out_floats + 0) = static_cast<float>(color.red()) / 255.0f;
    *(out_floats + 1) = static_cast<float>(color.green()) / 255.0f;
    *(out_floats + 2) = static_cast<float>(color.blue()) / 255.0f;
}

QColor convert_float3_to_qcolor(const float* floats) {
    return QColor(get_color_qml_string(floats[0], floats[1], floats[2]));
}

void convert_qcolor_to_float4(const QColor& color, float* out_floats) {
    *(out_floats + 0) = static_cast<float>(color.red()) / 255.0f;
    *(out_floats + 1) = static_cast<float>(color.green()) / 255.0f;
    *(out_floats + 2) = static_cast<float>(color.blue()) / 255.0f;
    *(out_floats + 3) = static_cast<float>(color.alpha()) / 255.0f;
}

#ifdef SIOYEK_ANDROID

#include "main_widget.h"
extern std::vector<MainWidget*> windows;

// modified from https://github.com/mahdize/CrossQFile/blob/main/CrossQFile.cpp


QString android_file_name_from_uri(QString uri) {

    //    mainActivityObj = QtAndroid::androidActivity();
    QJniObject activity = QNativeInterface::QAndroidApplication::context();

    QJniObject contentResolverObj = activity.callObjectMethod
    ("getContentResolver", "()Landroid/content/ContentResolver;");


    //	QAndroidJniObject cursorObj {contentResolverObj.callObjectMethod
    //		("query",
    //		 "(Landroid/net/Uri;[Ljava/lang/String;Landroid/os/Bundle;Landroid/os/CancellationSignal;)Landroid/database/Cursor;",
    //		 parseUriString(fileName()).object(), QAndroidJniObject().object(), QAndroidJniObject().object(),
    //		 QAndroidJniObject().object(), QAndroidJniObject().object())};

    QJniObject cursorObj{ contentResolverObj.callObjectMethod
        ("query",
         "(Landroid/net/Uri;[Ljava/lang/String;Landroid/os/Bundle;Landroid/os/CancellationSignal;)Landroid/database/Cursor;",
         parseUriString(uri).object(), QJniObject().object(), QJniObject().object(),
         QJniObject().object(), QJniObject().object()) };

    cursorObj.callMethod<jboolean>("moveToFirst");

    QJniObject retObj{ cursorObj.callObjectMethod
        ("getString","(I)Ljava/lang/String;", cursorObj.callMethod<jint>
         ("getColumnIndex","(Ljava/lang/String;)I",
          QJniObject::getStaticObjectField<jstring>
          ("android/provider/OpenableColumns","DISPLAY_NAME").object())) };

    QString ret{ retObj.toString() };
    return ret;
}

void check_pending_intents(const QString workingDirPath)
{
    //    QJniObject activity = QNativeInterface::QAndroidApplication::context();
    QJniObject activity = QNativeInterface::QAndroidApplication::context();
    if (activity.isValid()) {
        // create a Java String for the Working Dir Path
        QJniObject jniWorkingDir = QJniObject::fromString(workingDirPath);
        if (!jniWorkingDir.isValid()) {
            //            emit shareError(0, tr("Share: an Error occured\nWorkingDir not valid"));
            return;
        }
        activity.callMethod<void>("checkPendingIntents", "(Ljava/lang/String;)V", jniWorkingDir.object<jstring>());
        return;
    }
}


void setFileUrlReceived(const QString& url)
{
    if (windows.size() > 0) {
        windows[0]->open_document(url.toStdWString());
    }
}

extern "C" {
    JNIEXPORT void JNICALL
        Java_info_sioyek_sioyek_SioyekActivity_setFileUrlReceived(JNIEnv* env,
            jobject obj,
            jstring url)
    {
        const char* urlStr = env->GetStringUTFChars(url, NULL);
        Q_UNUSED(obj)
            setFileUrlReceived(urlStr);
        env->ReleaseStringUTFChars(url, urlStr);
        return;
    }

    JNIEXPORT void JNICALL
        Java_info_sioyek_sioyek_SioyekActivity_qDebug(JNIEnv* env,
            jobject obj,
            jstring url)
    {
        const char* urlStr = env->GetStringUTFChars(url, NULL);
        qDebug() << urlStr;
        Q_UNUSED(obj)
            env->ReleaseStringUTFChars(url, urlStr);
        return;
    }

    //JNIEXPORT void JNICALL
    //  Java_org_ekkescorner_examples_sharex_QShareActivity_setFileReceivedAndSaved(JNIEnv *env,
    //                                        jobject obj,
    //                                        jstring url)
    //{
    //    const char *urlStr = env->GetStringUTFChars(url, NULL);
    //    Q_UNUSED (obj)
    //    setFileReceivedAndSaved(urlStr);
    //    env->ReleaseStringUTFChars(url, urlStr);
    //    return;
    //}
}
#endif

float dampen_velocity(float v, float dt) {
    if (v == 0) return 0;
    dt = -dt;

    float accel = 3000.0f;
    if (v > 0) {
        v -= accel * dt;
        if (v < 0) {
            v = 0;
        }
    }
    else {
        v += accel * dt;
        if (v > 0) {
            v = 0;
        }
    }
    return v;
}

fz_irect get_index_irect(fz_rect original, int index, fz_matrix transform, int num_h_slices, int num_v_slices) {
    fz_rect transformed = fz_transform_rect(original, transform);
    fz_irect rounded = fz_round_rect(transformed);

    int vi = index / num_h_slices;
    int hi = index % num_h_slices;

    int slice_width = (rounded.x1 - rounded.x0) / num_h_slices;
    int slice_height = (rounded.y1 - rounded.y0) / num_v_slices;

    int x0 = hi * slice_width;
    int y0 = vi * slice_height;

    int x1 = (hi == (num_h_slices - 1)) ? rounded.x1 : (hi + 1) * slice_width;
    int y1 = (vi == (num_v_slices - 1)) ? rounded.y1 : (vi + 1) * slice_height;

    fz_irect res;
    res.x0 = x0;
    res.x1 = x1;

    res.y0 = y0;
    res.y1 = y1;

    return res;


}

void translate_index(int index, int* h_index, int* v_index, int num_h_slices, int num_v_slices) {
    *v_index = index / num_h_slices;
    *h_index = index % num_h_slices;
}

void get_slice_size(float* width, float* height, fz_rect original, int num_h_slices, int num_v_slices) {
    *width = (original.x1 - original.x0) / num_h_slices;
    *height = (original.y1 - original.y0) / num_v_slices;
}

fz_rect get_index_rect(fz_rect original, int index, int num_h_slices, int num_v_slices) {

    int h_index, v_index;
    translate_index(index, &h_index, &v_index, num_h_slices, num_v_slices);

    float slice_height, slice_width;
    get_slice_size(&slice_width, &slice_height, original, num_h_slices, num_v_slices);

    fz_rect new_rect;
    new_rect.x0 = original.x0 + h_index * slice_width;
    new_rect.x1 = original.x0 + (h_index + 1) * slice_width;
    new_rect.y0 = original.y0 + v_index * slice_height;
    new_rect.y1 = original.y0 + (v_index + 1) * slice_height;
    return new_rect;
}

QStandardItemModel* create_table_model(const std::vector<std::vector<std::wstring>> column_texts) {
    QStandardItemModel* model = new QStandardItemModel();
    if (column_texts.size() == 0) {
        return model;
    }
    int num_rows = column_texts[0].size();
    for (int i = 1; i < column_texts.size(); i++) {
        assert(column_texts[i].size() == num_rows);
    }

    for (int i = 0; i < num_rows; i++) {
        QList<QStandardItem*> items;
        for (int j = 0; j < column_texts.size(); j++) {
            QStandardItem* item = new QStandardItem(QString::fromStdWString(column_texts[j][i]));

            if (j == (column_texts.size() - 1)) {
                item->setTextAlignment(Qt::AlignVCenter | Qt::AlignRight);
            }
            else {
                item->setTextAlignment(Qt::AlignVCenter);
            }
            items.append(item);
        }
        model->appendRow(items);
    }
    return model;
}


QStandardItemModel* create_table_model(std::vector<std::wstring> lefts, std::vector<std::wstring> rights) {
    QStandardItemModel* model = new QStandardItemModel();

    assert(lefts.size() == rights.size());

    for (size_t i = 0; i < lefts.size(); i++) {
        QStandardItem* name_item = new QStandardItem(QString::fromStdWString(lefts[i]));
        QStandardItem* key_item = new QStandardItem(QString::fromStdWString(rights[i]));
        key_item->setTextAlignment(Qt::AlignVCenter | Qt::AlignRight);
        model->appendRow(QList<QStandardItem*>() << name_item << key_item);
    }
    return model;
}

float vec3_distance_squared(float* v1, float* v2) {
    float distance = 0;

    float dx = *(v1 + 0) - *(v2 + 0);
    float dy = *(v1 + 1) - *(v2 + 1);
    float dz = *(v1 + 2) - *(v2 + 2);

    return dx * dx + dy * dy + dz * dz;
}

char get_highlight_color_type(float color[3]) {
    float min_distance = 1000;
    int min_index = -1;

    for (int i = 0; i < 26; i++) {
        float dist = vec3_distance_squared(color, &HIGHLIGHT_COLORS[i * 3]);
        if (dist < min_distance) {
            min_distance = dist;
            min_index = i;
        }
    }

    return 'a' + min_index;
}

float* get_highlight_type_color(char type) {
    if (type == '_') {
        return &BLACK_COLOR[0];
    }
    if (type >= 'a' && type <= 'z') {
        return &HIGHLIGHT_COLORS[(type - 'a') * 3];
    }
    if (type >= 'A' && type <= 'Z') {
        return &HIGHLIGHT_COLORS[(type - 'A') * 3];
    }
    return &HIGHLIGHT_COLORS[0];
}

void lighten_color(float input[3], float output[3]) {
    QColor color = qRgb(
        static_cast<int>(input[0] * 255),
        static_cast<int>(input[1] * 255),
        static_cast<int>(input[2] * 255)
    );
    float prev_lightness = static_cast<float>(color.lightness()) / 255.0f;
    int lightness_increase = static_cast<int>((0.9f / prev_lightness) * 100);

    QColor lighter = color;
    if (lightness_increase > 100) {
        lighter = color.lighter(lightness_increase);
    }

    output[0] = lighter.redF();
    output[1] = lighter.greenF();
    output[2] = lighter.blueF();
}


QString trim_text_after(QString source, QString needle) {
    int needle_index = source.indexOf(needle);
    if (needle_index != -1) {
        return source.left(needle_index);
    }
    return source;
}

std::wstring clean_link_source_text(std::wstring link_source_text) {
    QString text = QString::fromStdWString(link_source_text);

    text = trim_text_after(text, "et. al.");
    text = trim_text_after(text, "et al");
    text = trim_text_after(text, "etal.");

    std::vector<char> garbage_chars = { '[', ']', '.', ',' };
    while ((text.size() > 0) && (std::find(garbage_chars.begin(), garbage_chars.end(), text.at(0)) != garbage_chars.end())) {
        text = text.right(text.size() - 1);
    }

    while ((text.size() > 0) && (std::find(garbage_chars.begin(), garbage_chars.end(), text.at(text.size() - 1)) != garbage_chars.end())) {
        text = text.left(text.size() - 1);
    }
    return text.toStdWString();
}

std::wstring clean_bib_item(std::wstring bib_item) {
    QString bib_item_qstring = QString::fromStdWString(bib_item);
    int bracket_index = bib_item_qstring.indexOf("]");
    if (bracket_index >= 0 && bracket_index < 10) {
        bib_item_qstring = bib_item_qstring.right(bib_item_qstring.size() - bracket_index - 1);
    }

    int arxiv_index = bib_item_qstring.toLower().indexOf("arxiv");
    if (arxiv_index >= 0) {
        bib_item_qstring = bib_item_qstring.left(arxiv_index);
    }

    std::wstring candid = bib_item_qstring.toStdWString();
    while (candid.size() > 0 && ((candid[candid.size() - 1] > 128) || !std::isalpha(candid[candid.size() - 1]))) {
        candid = candid.substr(0, candid.size() - 1);
    }
    return candid;
}

struct Line2D {
    float nx;
    float ny;
    float c;
};

float point_distance_from_line(AbsoluteDocumentPos point, Line2D line) {
    return std::abs(line.nx * point.x + line.ny * point.y - line.c);
}

Line2D line_from_points(AbsoluteDocumentPos p1, AbsoluteDocumentPos p2) {
    float dx = p2.x - p1.x;
    float dy = p2.y - p1.y;
    float nx = -dy;
    float ny = dx;
    float size = std::sqrt(nx * nx + ny * ny);

    nx = nx / size;
    ny = ny / size;

    Line2D res;
    res.nx = nx;
    res.ny = ny;
    res.c = nx * p1.x + ny * p1.y;
    return res;
}

std::vector<FreehandDrawingPoint> prune_freehand_drawing_points(const std::vector<FreehandDrawingPoint>& points) {

    if (points.size() < 3) {
        return points;
    }

    std::vector<FreehandDrawingPoint> pruned_points;
    pruned_points.push_back(points[0]);
    int candid_index = 1;

    while (candid_index < points.size() - 1) {
        int next_index = candid_index + 1;
        if ((points[candid_index].pos.x == pruned_points.back().pos.x) && (points[candid_index].pos.y == pruned_points.back().pos.y)) {
            candid_index++;
            continue;
        }

        float dx0 = points[candid_index].pos.x - pruned_points.back().pos.x;
        float dy0 = points[candid_index].pos.y - pruned_points.back().pos.y;

        float dx1 = points[next_index].pos.x - points[candid_index].pos.x;
        float dy1 = points[next_index].pos.y - points[candid_index].pos.y;
        float dot_product = dx0 * dx1 + dy0 * dy1;

        Line2D line = line_from_points(pruned_points.back().pos, points[next_index].pos);
        float thickness_factor = std::min(points[candid_index].thickness, 1.0f);
        if ((dot_product < 0) || (point_distance_from_line(points[candid_index].pos, line) > (0.2f * thickness_factor))) {
            pruned_points.push_back(points[candid_index]);
        }


        candid_index++;
    }

    pruned_points.push_back(points[points.size() - 1]);


    return pruned_points;
}

float rect_area(fz_rect rect) {
    if (rect.x1 < rect.x0 || rect.y1 < rect.y0) return 0;
    return std::abs(rect.x1 - rect.x0) * std::abs(rect.y1 - rect.y0);
}

bool are_rects_same(fz_rect r1, fz_rect r2) {
    float r1_area = rect_area(r1);
    float r2_area = rect_area(r2);
    float max_area = std::max(r1_area, r2_area);
    if (r2_area == 0) {
        return (std::abs(r1.x0 - r2.x0) < 0.01f) && (std::abs(r1.y0 - r2.y0) < 0.01f);
    }
    fz_rect intersection = fz_intersect_rect(r1, r2);
    if (rect_area(intersection) > max_area * 0.9f) {
        return true;
    }
    return false;
}

bool is_new_word(fz_stext_char* old_char, fz_stext_char* new_char) {
    if (old_char == nullptr) return true;
    if (new_char->c == ' ' || new_char->c == '\n') return true;
    return std::abs(new_char->quad.ll.x - old_char->quad.ll.x) > 5 * std::abs(old_char->quad.lr.x - old_char->quad.ll.x);
}

std::optional<DocumentRect> find_shrinking_rect_word(bool before, fz_stext_page* page, DocumentRect rect){
    bool found = false;
    std::optional<DocumentRect> last_before_space_rect = {};
    std::optional<DocumentRect> before_rect = {};
    fz_stext_char* old_char = nullptr;


    bool should_return_next_char = false;
    bool was_last_character_space = true;

    LL_ITER(block, page->first_block) {
        if (block->type == FZ_STEXT_BLOCK_TEXT) {
            LL_ITER(line, block->u.t.first_line) {
                LL_ITER(ch, line->first_char) {
                    DocumentRect cr = DocumentRect(fz_rect_from_quad(ch->quad), rect.page);
                    if (should_return_next_char) {
                        return cr;
                    }
                    if ((!before) && (is_new_word(old_char, ch)) && found) {
                        return last_before_space_rect;
                    }
                    if (is_new_word(old_char, ch)) {
                        last_before_space_rect = before_rect;
                        was_last_character_space = true;
                        if (found && before) {
                            should_return_next_char = true;
                        }
                    }
                    if (are_rects_same(cr.rect, rect.rect)) {
                        found = true;
                    }
                    before_rect = cr;
                    old_char = ch;
                }
            }
        }
    }
    return {};
}

std::vector<DocumentRect> find_expanding_rect_word(bool before, fz_stext_page* page, DocumentRect page_rect) {
    std::vector<DocumentRect> res;
    std::vector<fz_stext_char*> chars;
    get_flat_chars_from_stext_page(page, chars);
    int index = -1;
    for (int i = 0; i < chars.size(); i++) {
        DocumentRect cr = DocumentRect(fz_rect_from_quad(chars[i]->quad), page_rect.page);
        if (are_rects_same(cr.rect, page_rect.rect)) {
            index = i;
            break;
        }
    }
    int original_index = index;
    int prev_index = original_index;

    if (index != -1) {
        if (before) {
            index--;
            while (index >= 0) {
                res.push_back(DocumentRect(fz_rect_from_quad(chars[index]->quad), page_rect.page));
                //if (chars[index]->c == ' ' || chars[index]->c == '\n') {
                if (is_new_word(chars[prev_index], chars[index])) {
                    if (std::abs(original_index - index) > 1) {
                        res.pop_back();
                        break;
                    }
                }
                prev_index = index;
                index--;
            }
        }
        else {
            index++;
            while (index < chars.size()) {
                res.push_back(DocumentRect(fz_rect_from_quad(chars[index]->quad), page_rect.page));
                if (is_new_word(chars[prev_index], chars[index])) {
                    if (std::abs(original_index - index) > 1) {
                        res.pop_back();
                        break;
                    }
                }
                prev_index = index;
                index++;
            }
        }
    }
    return res;

}

//std::vector<fz_rect> find_expanding_rect_word(bool before, fz_stext_page* page, fz_rect page_rect) {
//	bool found = false;
//	std::optional<fz_rect> last_after_space_rect = {};
//	std::optional<fz_rect> before_rect = {};
//	std::vector<fz_rect> res;
//
//	bool was_last_character_space = true;
//
//	LL_ITER(block, page->first_block) {
//		if (block->type == FZ_STEXT_BLOCK_TEXT) {
//			LL_ITER(line, block->u.t.first_line) {
//				LL_ITER(ch, line->first_char) {
//					fz_rect cr = fz_rect_from_quad(ch->quad);
//
//					if (was_last_character_space) {
//						was_last_character_space = false;
//						last_after_space_rect = cr;
//					}
//
//					if (before) {
//						res.push_back(cr);
//					}
//					if (ch->c == ' ' || ch->c == '\n') {
//						was_last_character_space = true;
//						if (before) {
//							res.clear();
//						}
//						if (found) {
//							return res;
//						}
//					}
//					if (found) {
//						res.push_back(cr);
//					}
//
//					if (are_rects_same(cr, page_rect)) {
//						if (before) {
//							return res;
//						}
//						found = true;
//					}
//
//					before_rect = cr;
//
//				}
//			}
//		}
//	}
//
//	return {};
//}

std::optional<DocumentRect> find_expanding_rect(bool before, fz_stext_page* page, DocumentRect page_rect) {
    bool found = false;
    std::optional<DocumentRect> before_rect = {};

    LL_ITER(block, page->first_block) {
        if (block->type == FZ_STEXT_BLOCK_TEXT) {
            LL_ITER(line, block->u.t.first_line) {
                LL_ITER(ch, line->first_char) {
                    DocumentRect cr = DocumentRect(fz_rect_from_quad(ch->quad), page_rect.page);
                    if (found) {
                        return cr;
                    }
                    if (are_rects_same(cr.rect, page_rect.rect)) {
                        if (before) {
                            return before_rect;
                        }
                        found = true;
                    }
                    before_rect = cr;
                }
            }
        }
    }

    return {};
}


QStringList extract_paper_data_from_json_response(QJsonValue json_object, const std::vector<QString>& path) {

    if (path.size() == 0) {
        if (json_object.isArray()) {
            QJsonArray array = json_object.toArray();
            QStringList list;
            for (int i = 0; i < array.size(); i++) {
                list.append(array.at(i).toString());
            }
            return QStringList() << list.join(", ");
        }
        else {
            return { json_object.toString() };
        }
    }


    QString current_path = path[0];
    if (current_path.indexOf("[]") != -1) {
        QJsonArray array = json_object.toObject().value(current_path.left(current_path.size() - 2)).toArray();
        QStringList res;

        for (int i = 0; i < array.size(); i++) {
            QStringList temp_objects = extract_paper_data_from_json_response(array.at(i), std::vector<QString>(path.begin() + 1, path.end()));
            for (int i = 0; i < temp_objects.size(); i++) {
                res.push_back(temp_objects.at(i));
            }
        }
        return res;
    }
    else if (current_path.indexOf("[") != -1) {
        QString index_string = current_path.mid(current_path.indexOf("[") + 1, current_path.indexOf("]") - current_path.indexOf("[") - 1);
        QString key_string = current_path.left(current_path.indexOf("["));
        int index = index_string.toInt();
        return extract_paper_data_from_json_response(json_object.toObject().value(key_string).toArray().at(index),
            std::vector<QString>(path.begin() + 1, path.end()));

    }
    else {
        if (json_object.isArray()) {
            QJsonArray array = json_object.toObject().value(current_path.left(current_path.size() - 2)).toArray();
            QStringList res;

            for (int i = 0; i < array.size(); i++) {
                QStringList temp_objects = extract_paper_data_from_json_response(array.at(i), std::vector<QString>(path.begin() + 1, path.end()));
                res.push_back(temp_objects.join(", "));
            }
            return res;
        }
        else {
            return extract_paper_data_from_json_response(json_object.toObject().value(current_path),
                std::vector<QString>(path.begin() + 1, path.end()));
        }
    }
}

QStringList extract_paper_string_from_json_response(QJsonObject json_object, std::wstring path) {
    std::vector<QString> parts;
    QStringList parts_string_list = QString::fromStdWString(path).split(".");
    for (int i = 0; i < parts_string_list.size(); i++) {
        parts.push_back(parts_string_list.at(i));
    }
    return extract_paper_data_from_json_response(json_object, parts);
}

QString file_size_to_human_readable_string(int file_size) {
    if (file_size < 1000) {
        return QString::number(file_size);
    }
    else if (file_size < 1000 * 1000) {
        return QString::number(file_size / 1000) + "K";
    }
    else if (file_size < 1000 * 1000 * 1000) {
        return QString::number(file_size / (1000 * 1000)) + "M";
    }
    else if (file_size < 1000 * 1000 * 1000 * 1000) {
        return QString::number(file_size / (1000 * 1000 * 1000)) + "G";
    }
    else {
        return QString("inf");
    }
}

std::wstring new_uuid() {
    return QUuid::createUuid().toString().toStdWString();
}

std::string new_uuid_utf8() {
    return QUuid::createUuid().toString().toStdString();
}

bool are_same(float f1, float f2) {
    return std::abs(f1 - f2) < 0.01;
}

bool are_same(const FreehandDrawing& lhs, const FreehandDrawing& rhs) {
    if (lhs.points.size() != rhs.points.size()) {
        return false;
    }
    if (lhs.type != rhs.type) {
        return false;
    }
    for (int i = 0; i < lhs.points.size(); i++) {
        if (!are_same(lhs.points[i].pos, rhs.points[i].pos)) {
            return false;
        }
    }
    return true;

}

PagelessDocumentRect get_range_rect_union(const std::vector<PagelessDocumentRect>& rects, int first_index, int last_index) {
    PagelessDocumentRect res = rects[first_index];
    for (int i = first_index + 1; i <= last_index; i++) {
        res = fz_union_rect(res, rects[i]);
    }
    return res;
}


int get_largest_quote_size(const std::wstring& text, int* begin_index, int* end_index) {
    bool is_in_quote = false;
    int largest_size = -1;
    int largest_begin_index = -1;
    int largest_end_index = -1;

    int current_size = 0;

    int current_begin_index = -1;

    for (int i = 0; i < text.size(); i++) {
        if ((text[i] == '"') || (text[i] == 8220) || (text[i] == 8221)) {
            if (is_in_quote) {
                is_in_quote = false;
                if (current_size > largest_size) {
                    largest_size = current_size;
                    largest_begin_index = current_begin_index;
                    largest_end_index = i;
                }
                current_size = 0;
            }
            else {
                is_in_quote = true;
                current_begin_index = i;
                current_size = 0;
            }
        }

        if (is_in_quote) {
            current_size++;
        }

    }
    *begin_index = largest_begin_index;
    *end_index = largest_end_index;
    
    return largest_size;
}

bool is_quote_reference(const std::wstring& text, int* begin_index, int* end_index) {
    return get_largest_quote_size(text, begin_index, end_index) > 15;
}

std::wstring strip_garbage_from_paper_name(std::wstring paper_name) {
    std::vector<int> garbage_characters = { '.', ',', ':', '"', '\'', ' ', 8220, 8221 };
    int first_index = 0;
    int last_index = paper_name.size()-1;

    while (std::find(garbage_characters.begin(), garbage_characters.end(), paper_name[first_index]) != garbage_characters.end()) {
        first_index++;
    }

    while (std::find(garbage_characters.begin(), garbage_characters.end(), paper_name[last_index]) != garbage_characters.end()) {
        last_index--;
    }
    if (last_index > first_index) {
        return paper_name.substr(first_index, last_index - first_index + 1);
    }
    return L"";
}

std::wstring get_paper_name_from_reference_text(std::wstring reference_text) {
    if (PAPER_DOWNLOAD_AUTODETECT_PAPER_NAME) {

        int quote_begin_index, quote_end_index;
        if (is_quote_reference(reference_text, &quote_begin_index, &quote_end_index)) {
            return strip_garbage_from_paper_name(reference_text.substr(quote_begin_index, quote_end_index - quote_begin_index));
        }

        QString str = QString::fromStdWString(reference_text);
        //QRegularExpression reference_ending_dot_regex = QRegularExpression("(\.\w*In )|(\.\w*[aA]r[xX]iv )|(\.\w*[aA]r[X]iv )");
        QRegularExpression reference_ending_dot_regex = QRegularExpression("(\.\w*In )|(\.\w*[aA]r[xX]iv )");

        int ending_index = str.lastIndexOf(reference_ending_dot_regex);
        if (ending_index == -1) {
            int last_dot_index = str.lastIndexOf(".");
            // igonre if the last dot is close to the end
            if (str.size() - last_dot_index < 8) {
                str = str.left(last_dot_index - 1);
            }
            ending_index = str.lastIndexOf(".") + 1;
        }

        while (ending_index > -1) {
            str = str.left(ending_index - 1);
            int starting_index = str.lastIndexOf(".");
            QString res = str.right(str.size() - starting_index - 1).trimmed();
            //if (res.size() > 0 && res[0] == ':') {
            //    res = res.right(res.size() - 1);
            //}
            if (res.size() > 10) {
                return strip_garbage_from_paper_name(res.toStdWString());
            }
            else {
                ending_index = starting_index;
            }
        }
    }

    return reference_text;

}

fz_rect get_first_page_size(fz_context* ctx, const std::wstring& document_path) {
    std::string path = utf8_encode(document_path);
    bool failed = false;
    
    fz_rect bounds;

    fz_try(ctx) {
        fz_document* doc = fz_open_document(ctx, path.c_str());

        fz_page* first_page = fz_load_page(ctx, doc, 0);
        bounds = fz_bound_page(ctx, first_page);

        fz_drop_page(ctx, first_page);
        fz_drop_document(ctx, doc);
    }
    fz_catch(ctx) {
        failed = true;
    }
    if (failed) {
        return fz_rect{ 0,0, 100, 100 };
    }

    return bounds;
}

QString get_direct_pdf_url_from_archive_url(QString url) {
    if (url.indexOf("web.archive.org") == -1) return url;

    int index = url.lastIndexOf("http") - 1;
    if (index > 4) {

        QString prefix = url.left(index);
        QString suffix = url.right(url.size() - index);
        return prefix + "if_" + suffix;
    }
    return url;

}

QString get_original_url_from_archive_url(QString url) {
    return url.right(url.size() - url.lastIndexOf("http"));
}

bool does_paper_name_match_query(std::wstring query, std::wstring paper_name) {
    std::string query_encoded = QString::fromStdWString(query).toLower().toStdString();
    std::string paper_name_encoded = QString::fromStdWString(paper_name).toLower().toStdString();

    int score = lcs(query_encoded.c_str(), paper_name_encoded.c_str(), query_encoded.size(), paper_name_encoded.size());
    int threshold = static_cast<int>(static_cast<float>(std::max(query_encoded.size(), paper_name_encoded.size())) * 0.9f);
    return score >= threshold;
}


bool is_dot_index_end_of_a_reference(const std::vector<DocumentCharacter>& flat_chars, int dot_index) {
    int next_non_whitespace_index = -1;
    int prev_index = dot_index - 1;
    int context_begin = dot_index - 10;
    int context_end = dot_index + 10;

    if (dot_index >= flat_chars.size()-2) {
        return true;
    }

    for (int candid = dot_index; candid < std::min((int)flat_chars.size(), dot_index+4); candid++) {
        if (flat_chars[candid].is_final) {
            next_non_whitespace_index = candid + 1;
            if (next_non_whitespace_index == flat_chars.size()) next_non_whitespace_index = -1;
            break;
        }
        fz_rect candid_rect = flat_chars[candid].rect;
        fz_rect dot_rect = flat_chars[dot_index].rect;
        if (candid_rect.y0 > dot_rect.y1) {
            next_non_whitespace_index = candid;
            break;
        }
    }
    if (next_non_whitespace_index == -1) {
        for (int candid = dot_index; candid < std::min((int)flat_chars.size(), dot_index + 2); candid++) {
            fz_rect candid_rect = flat_chars[candid].rect;
            fz_rect dot_rect = flat_chars[dot_index].rect;
            if (candid_rect.y0 > dot_rect.y1) {
                next_non_whitespace_index = candid;
                break;
            }
        }
    }
    if (next_non_whitespace_index > -1 && prev_index > -1) {
        if (context_begin >= 0 && context_end < flat_chars.size()) {
            std::wstring context;
            for (int i = context_begin; i < context_end; i++) {
                context.push_back(flat_chars[i].c);
            }
            int a = 2;
            //qDebug() << QString::fromStdWString(context) << "!!" << QString(QChar(flat_chars[next_non_whitespace_index]->c));
        }
        fz_rect dot_rect = flat_chars[prev_index].rect;
        fz_rect next_rect = flat_chars[next_non_whitespace_index].rect;
        float height = std::abs(next_rect.y1 - next_rect.y0);

        if ((next_rect.y0 > (dot_rect.y0 + height / 2)) && (next_rect.y1 > (dot_rect.y1 + height / 2))) {
            return true;
        }
        if (std::abs(next_rect.y0 - dot_rect.y0) > 5 * height) {
            return true;
        }
    }
    return false;
}

std::wstring remove_et_al(std::wstring ref) {
    int index = ref.find(L"et al.");
    if (index != -1) {
        return ref.substr(0, index) + ref.substr(index + 6);
    }
    else {
        return ref;
    }
}

bool is_year(QString str) {
    if (str.size() == 0) return false;

    for (int i = 0; i < str.size(); i++) {
        if (!str[i].isDigit()) {
            return false;
        }
    }
    int n = str.toInt();
    if (n > 1600 && n < 2100) {
        return true;
    }
    return false;
}

bool is_text_refernce_rather_than_paper_name(std::wstring text) {
    text = strip_garbage_from_paper_name(text);

    if (text.size() > 50) {
        return false;
    }
    if ((text.find(L"et al") != -1) || (text.find(L"et. al") != -1)) {
        return true;
    }
    if (text.back() >= 0 && text.back() <= 128 && std::isdigit(text.back())) {
        return true;
    }

    QStringList parts = QString::fromStdWString(text).split(QRegularExpression("[ \(\)]"));
    for (int i = 0; i < parts.size(); i++) {
        if (is_year(parts[i])) {
            return true;
        }
    }
    return false;

}

QJsonObject rect_to_json(fz_rect rect) {
    QJsonObject res;
    res["x0"] = rect.x0;
    res["y0"] = rect.y0;
    res["x1"] = rect.x1;
    res["y1"] = rect.y1;
    return res;
}

bool pred_case_sensitive(const wchar_t& c1, const wchar_t& c2) {
    return c1 == c2;
}

bool pred_case_insensitive(const wchar_t& c1, const wchar_t& c2) {
    return std::tolower(c1) == std::tolower(c2);
}

// a function to return a pred based on case sensitivity
std::function<bool(const wchar_t&, const wchar_t&)> get_pred(SearchCaseSensitivity cs, const std::wstring& query) {
    if (cs == SearchCaseSensitivity::CaseSensitive) return pred_case_sensitive;
    if (cs == SearchCaseSensitivity::CaseInsensitive) return pred_case_insensitive;
    if (QString::fromStdWString(query).isLower()) return pred_case_insensitive;
    return pred_case_sensitive;
}

std::vector<SearchResult> search_text_with_index(const std::wstring& super_fast_search_index,
    const std::vector<int>& super_fast_search_index_pages,
    const std::vector<PagelessDocumentRect>& super_fast_search_rects,
    std::wstring query,
    SearchCaseSensitivity case_sensitive,
    int begin_page,
    int min_page,
    int max_page) {

    std::vector<SearchResult> output;

    std::vector<SearchResult> before_results;
    bool is_before = true;

    //auto pred = case_sensitive == SearchCaseSensitivity::CaseSensitive ? pred_case_sensitive : pred_case_insensitive;
    auto pred = get_pred(case_sensitive, query);
    auto searcher = std::default_searcher(query.begin(), query.end(), pred);
    auto it = std::search(
        super_fast_search_index.begin(),
        super_fast_search_index.end(),
        searcher);

    for (; it != super_fast_search_index.end(); it = std::search(it + 1, super_fast_search_index.end(), searcher)) {
        int start_index = it - super_fast_search_index.begin();
        std::deque<fz_rect> match_rects;
        std::vector<fz_rect> compressed_match_rects;

        int match_page = super_fast_search_index_pages[start_index];

        if (match_page >= begin_page) {
            is_before = false;
        }

        int end_index = start_index + query.size();


        for (int j = start_index; j < end_index; j++) {
            fz_rect rect = super_fast_search_rects[j];
            match_rects.push_back(rect);
        }

        merge_selected_character_rects(match_rects, compressed_match_rects);
        SearchResult res{ compressed_match_rects, match_page };

        if (!((match_page < min_page) || (match_page > max_page))) {
            if (is_before) {
                before_results.push_back(res);
            }
            else {
                output.push_back(res);
            }
        }
    }
    output.insert(output.end(), before_results.begin(), before_results.end());
    return output;
}

void search_text_with_index_single_page(const std::wstring& super_fast_search_index,
    const std::vector<PagelessDocumentRect>& super_fast_search_rects,
    std::wstring query,
    SearchCaseSensitivity case_sensitive,
    int page_number,
    std::vector<SearchResult>* output
    ){

    auto pred = get_pred(case_sensitive, query);
    auto searcher = std::default_searcher(query.begin(), query.end(), pred);
    auto it = std::search(
        super_fast_search_index.begin(),
        super_fast_search_index.end(),
        searcher);

    for (; it != super_fast_search_index.end(); it = std::search(it + 1, super_fast_search_index.end(), searcher)) {
        int start_index = it - super_fast_search_index.begin();
        std::deque<fz_rect> match_rects;
        std::vector<fz_rect> compressed_match_rects;

        int end_index = start_index + query.size();

        for (int j = start_index; j < end_index; j++) {
            fz_rect rect = super_fast_search_rects[j];
            match_rects.push_back(rect);
        }

        merge_selected_character_rects(match_rects, compressed_match_rects);
        SearchResult res{ compressed_match_rects, page_number };

        output->push_back(res);
    }

}

std::vector<SearchResult> search_regex_with_index(const std::wstring& super_fast_search_index,
    const std::vector<int>& super_fast_search_index_pages,
    const std::vector<PagelessDocumentRect>& super_fast_search_rects,
    std::wstring query,
    SearchCaseSensitivity case_sensitive,
    int begin_page,
    int min_page,
    int max_page) {
    std::vector<SearchResult> output;
    search_regex_with_index_( super_fast_search_index,
        super_fast_search_index_pages,
        super_fast_search_rects,
        query,
        case_sensitive,
        begin_page,
        min_page,
        max_page,
        &output);
    return output;
}

void search_regex_with_index_(const std::wstring& super_fast_search_index,
    const std::vector<int>& super_fast_search_index_pages,
    const std::vector<PagelessDocumentRect>& super_fast_search_rects,
    std::wstring query,
    SearchCaseSensitivity case_sensitive,
    int begin_page,
    int min_page,
    int max_page,
    std::vector<SearchResult>* output)
{

    std::wregex regex;
    try {
        if (case_sensitive != SearchCaseSensitivity::CaseSensitive) {
            regex = std::wregex(query, std::regex_constants::icase);
        }
        else {
            regex = std::wregex(query);
        }
    }
    catch (const std::regex_error&) {
        return;
    }


    std::vector<SearchResult> before_results;
    bool is_before = true;

    int offset = 0;

    std::wstring::const_iterator search_start(super_fast_search_index.begin());
    std::wsmatch match;
    int empty_tolerance = 1000;


    while (std::regex_search(search_start, super_fast_search_index.cend(), match, regex)) {
        std::deque<fz_rect> match_rects;
        std::vector<fz_rect> compressed_match_rects;

        int match_page = super_fast_search_index_pages[offset + match.position()];

        if (match_page >= begin_page) {
            is_before = false;
        }

        int start_index = offset + match.position();
        int end_index = offset + match.position() + match.length();
        if (start_index < end_index) {


            for (int j = start_index; j < end_index; j++) {
                fz_rect rect = super_fast_search_rects[j];
                match_rects.push_back(rect);
            }

            merge_selected_character_rects(match_rects, compressed_match_rects);
            SearchResult res{ compressed_match_rects, match_page };

            if (!((match_page < min_page) || (match_page > max_page))) {
                if (is_before) {
                    before_results.push_back(res);
                }
                else {
                    output->push_back(res);
                }
            }
        }
        else {
            empty_tolerance--;
            if (empty_tolerance == 0) {
                break;
            }
        }

        offset = end_index;
        search_start = match.suffix().first;
    }
    output->insert(output->end(), before_results.begin(), before_results.end());

}

std::vector<std::wstring> get_path_unique_prefix(const std::vector<std::wstring>& paths) {
    std::vector<QStringList> path_parts;
    QChar separator = '/';

    int max_depth = -1;
    for (auto p : paths) {
        path_parts.push_back(QString::fromStdWString(p).split(separator));
        int current_depth = path_parts.back().size();
        if (current_depth > max_depth) max_depth = current_depth;
    }

    std::vector<std::wstring> res;

    for (int depth = 1; depth <= max_depth; depth++) {

        for (auto parts : path_parts) {
            if (depth < parts.size()) {
                res.push_back(parts.mid(parts.size() - depth, depth).join(separator).toStdWString());
            }
            else {
                res.push_back(parts.join(separator).toStdWString());
            }
        }
        std::vector<std::wstring> res_copy = res;
        std::sort(res_copy.begin(), res_copy.end());
        bool found_duplicate = false;

        for (int i = 0; i < res_copy.size() - 1; i++) {
            if (res_copy[i] == res_copy[i + 1]) found_duplicate = true;
        }

        if (!found_duplicate) break;
        res.clear();
    }

    return res;
}

bool is_block_vertical(fz_stext_block* block) {

    int num_vertical = 0;
    int num_horizontal = 0;
    LL_ITER(line, block->u.t.first_line) {

        LL_ITER(ch, line->first_char) {
            if (ch->next != nullptr) {
                float horizontal_diff = std::abs(ch->quad.ll.x - ch->next->quad.ll.x);
                float vertical_diff = std::abs(ch->quad.ll.y - ch->next->quad.ll.y);
                if (vertical_diff > horizontal_diff) {
                    num_vertical++;
                }
                else {
                    num_horizontal++;
                }
            }
        }
    }
    return num_vertical > num_horizontal;
}
QString get_file_name_from_paper_name(QString paper_name) {
    if (paper_name.size() > 0) {
        QStringList parts = paper_name.split(' ');
        QString new_file_name;
        for (int i = 0; i < parts.size(); i++) {
            new_file_name += parts[i].toLower();
            if (i < parts.size() - 1) {
                new_file_name += '_';
            }
        }

        new_file_name.remove(".");
        new_file_name.remove("\\");
        return new_file_name;
    }

    return "";
}


void rgb2hsv(float* rgb_color, float* hsv_color) {
    int rgb_255_color[3];
    convert_color3(rgb_color, rgb_255_color);
    QColor qcolor(rgb_255_color[0], rgb_255_color[1], rgb_255_color[2]);
    QColor hsv_qcolor = qcolor.toHsv();
    hsv_color[0] = hsv_qcolor.hsvHueF();
    hsv_color[1] = hsv_qcolor.hsvSaturationF();
    hsv_color[2] = hsv_qcolor.lightnessF();
    if (hsv_color[0] < 0) hsv_color[0] += 1.0f;
}

void hsv2rgb(float* hsv_color, float* rgb_color) {
    QColor qcolor;
    qcolor.setHsvF(hsv_color[0], hsv_color[1], hsv_color[2]);
    rgb_color[0] = qcolor.redF();
    rgb_color[1] = qcolor.greenF();
    rgb_color[2] = qcolor.blueF();
}

bool operator==(const fz_rect& lhs, const fz_rect& rhs) {
    return lhs.x0 == rhs.x0 &&
        lhs.y0 == rhs.y0 &&
        lhs.x1 == rhs.x1 &&
        lhs.y1 == rhs.y1;
}

bool is_bright(float color[3]){
    return (color[0] + color[1] + color[2]) > 1.5f;
}


bool is_abbreviation(const std::wstring& txt){
    int n_upper = 0;
    int n_lower = 0;

    for (auto c : txt){
        // prevent crash on non-ascii chars
        if (c <= 0 || c > 128) continue;

        if (isupper(c)){
            n_upper++;
        }
        else if (islower(c)){
            n_lower++;
        }
    }

    return n_upper > n_lower;
}

bool is_in(char c, std::vector<char> candidates){
    return std::find(candidates.begin(), candidates.end(), c) != candidates.end();
}


bool is_doc_valid(fz_context* ctx, std::string path) {
    bool is_valid = false;

    fz_try(ctx) {
        fz_document* doc = fz_open_document(ctx, path.c_str());
        if (doc) {
            int n_pages = fz_count_pages(ctx, doc);
            is_valid = n_pages > 0;
            fz_drop_document(ctx, doc);
        }
    }
    fz_catch(ctx) {
        is_valid = false;
    }

    return is_valid;

}

QString get_ui_font_face_name() {
    if (UI_FONT_FACE_NAME.empty()) {
        return "";
    }
    else {
        return QString::fromStdWString(UI_FONT_FACE_NAME);
    }
}

QString get_status_font_face_name() {
    if (STATUS_FONT_FACE_NAME.empty()) {
        return "Monospace";
    }
    else {
        return QString::fromStdWString(STATUS_FONT_FACE_NAME);
    }
}
