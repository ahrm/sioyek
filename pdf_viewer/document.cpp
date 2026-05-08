#include "document.h"
#include <algorithm>
#include <thread>
#include <cmath>
#include "coordinates.h"
#include "utf8.h"
#include <qfileinfo.h>
#include <qdatetime.h>
#include <map>
#include <regex>
#include <qcryptographichash.h>
#include <qjsondocument.h>
#include "path.h"
#include <qjsonarray.h>
#include <qjsonobject.h>
#include <qjsondocument.h>
#include <qdir.h>
#include <qstandardpaths.h>
#include <set>
#include <cwctype>
#include <tuple>

#include <mupdf/pdf.h>

#include "sqlite3.h"
#include "checksum.h"
#include "database.h"
#include "utf8/checked.h"
#include "utils.h"

extern bool SHOULD_RENDER_PDF_ANNOTATIONS;

const int WINDOW_SIZE = 10;
const int EMBEDDING_DIM = 10;

extern float SMALL_PIXMAP_SCALE;
extern float HIGHLIGHT_COLORS[26 * 3];
extern std::wstring TEXT_HIGHLIGHT_URL;
extern bool TEXT_SUMMARY_HIGHLIGHT_SHOULD_REFINE;
extern bool TEXT_SUMMARY_HIGHLIGHT_SHOULD_FILL;
extern int TEXT_SUMMARY_CONTEXT_SIZE;
extern bool USE_HEURISTIC_IF_TEXT_SUMMARY_NOT_AVAILABLE;
extern bool ENABLE_EXPERIMENTAL_FEATURES;
extern bool CREATE_TABLE_OF_CONTENTS_IF_NOT_EXISTS;
extern int MAX_CREATED_TABLE_OF_CONTENTS_SIZE;
extern bool FORCE_CUSTOM_LINE_ALGORITHM;
extern bool SUPER_FAST_SEARCH;
extern float EPUB_WIDTH;
extern float EPUB_HEIGHT;
extern QString EPUB_TEMPLATE;
extern float EPUB_FONT_SIZE;
extern std::wstring EPUB_CSS;
//extern std::vector<float> embedding_weights;
//extern std::vector<float> linear_weights;
extern float EPUB_LINE_SPACING;
extern Path standard_data_path;
extern bool VERBOSE;
extern float FREETEXT_BOOKMARK_COLOR[3];
extern float FREETEXT_BOOKMARK_FONT_SIZE;
extern std::wstring SHARED_DATABASE_PATH;
extern bool DEBUG;
extern bool EXACT_HIGHLIGHT_SELECT;
extern std::wstring ANNOTATIONS_DIR_PATH;
extern bool LIGHTEN_COLORS_WHEN_EMBEDDING_ANNOTATIONS;

namespace {

struct TocTextLine {
    std::wstring text;
    fz_rect rect;
    float font_size = 0.0f;
    bool bold = false;
};

struct CreatedTocCandidate {
    std::wstring title;
    int page = -1;
    float x = 0.0f;
    float y = 0.0f;
    float page_height = 0.0f;
    int level = 0;
    float score = 0.0f;
    bool structural = false;
    bool margin = false;
};

struct PrintedTocCandidate {
    std::wstring title;
    std::wstring page_label;
    int source_page = -1;
    fz_rect rect = fz_empty_rect;
    float x = 0.0f;
    float y = 0.0f;
    int level = 0;
    bool has_dot_leader = false;
    bool has_link_target = false;
    int link_target_page = -1;
    float link_target_x = 0.0f;
    float link_target_y = 0.0f;
};

struct ResolvedPrintedTocEntry {
    PrintedTocCandidate printed;
    int target_page = -1;
    float target_x = 0.0f;
    float target_y = 0.0f;
};

static bool toc_is_space(wchar_t c) {
    return c == L' ' || c == L'\t' || c == L'\n' || c == L'\r' || c == L'\f' || c == L'\v';
}

static std::wstring toc_trim(const std::wstring& str) {
    size_t begin = 0;
    while (begin < str.size() && toc_is_space(str[begin])) {
        begin++;
    }

    size_t end = str.size();
    while (end > begin && toc_is_space(str[end - 1])) {
        end--;
    }
    return str.substr(begin, end - begin);
}

static std::wstring toc_to_lower(const std::wstring& str) {
    std::wstring res;
    res.reserve(str.size());
    for (wchar_t c : str) {
        res.push_back(std::towlower(c));
    }
    return res;
}

static std::wstring toc_normalize_spaces(const std::wstring& str) {
    std::wstring res;
    bool last_was_space = true;
    for (wchar_t c : str) {
        if (toc_is_space(c)) {
            if (!last_was_space) {
                res.push_back(L' ');
            }
            last_was_space = true;
        }
        else {
            res.push_back(c);
            last_was_space = false;
        }
    }
    return toc_trim(res);
}

static bool toc_has_alpha(const std::wstring& str) {
    for (wchar_t c : str) {
        if (std::iswalpha(c)) {
            return true;
        }
    }
    return false;
}

static bool toc_is_all_digits(const std::wstring& str) {
    if (str.empty()) {
        return false;
    }
    for (wchar_t c : str) {
        if (!std::iswdigit(c)) {
            return false;
        }
    }
    return true;
}

static bool toc_is_roman_numeral(const std::wstring& str) {
    if (str.empty() || str.size() > 12) {
        return false;
    }
    for (wchar_t c : toc_to_lower(str)) {
        if (c != L'i' && c != L'v' && c != L'x' && c != L'l' && c != L'c' && c != L'd' && c != L'm') {
            return false;
        }
    }
    return true;
}

static int toc_roman_value(wchar_t c) {
    switch (std::towlower(c)) {
    case L'i': return 1;
    case L'v': return 5;
    case L'x': return 10;
    case L'l': return 50;
    case L'c': return 100;
    case L'd': return 500;
    case L'm': return 1000;
    default: return 0;
    }
}

static int toc_parse_roman(const std::wstring& str) {
    if (!toc_is_roman_numeral(str)) {
        return -1;
    }

    int res = 0;
    int prev = 0;
    for (int i = static_cast<int>(str.size()) - 1; i >= 0; i--) {
        int val = toc_roman_value(str[i]);
        if (val < prev) {
            res -= val;
        }
        else {
            res += val;
            prev = val;
        }
    }
    return res;
}

static int toc_parse_int(const std::wstring& str) {
    if (!toc_is_all_digits(str)) {
        return -1;
    }

    bool ok = false;
    int val = QString::fromStdWString(str).toInt(&ok);
    return ok ? val : -1;
}

static int toc_count_words(const std::wstring& str) {
    return static_cast<int>(split_whitespace(str).size());
}

static bool toc_is_plain_page_label_sequence(const std::vector<std::wstring>& page_labels) {
    if (page_labels.empty()) {
        return true;
    }
    for (int i = 0; i < static_cast<int>(page_labels.size()); i++) {
        if (toc_normalize_spaces(page_labels[i]) != QString::number(i + 1).toStdWString()) {
            return false;
        }
    }
    return true;
}

static bool toc_starts_with(const std::wstring& str, const std::wstring& prefix) {
    return str.size() >= prefix.size() && str.compare(0, prefix.size(), prefix) == 0;
}

static bool toc_contains(const std::wstring& str, const std::wstring& needle) {
    return str.find(needle) != std::wstring::npos;
}

static std::wstring toc_strip_edge_punctuation(const std::wstring& str) {
    int begin = 0;
    int end = static_cast<int>(str.size());
    while (begin < end && !std::iswalnum(str[begin]) && str[begin] != L'§') {
        begin++;
    }
    while (end > begin && !std::iswalnum(str[end - 1]) && str[end - 1] != L'§') {
        end--;
    }
    return str.substr(begin, end - begin);
}

static int toc_count_outline_components(std::wstring label) {
    label = toc_strip_edge_punctuation(label);
    if (label.empty()) {
        return 0;
    }

    int components = 0;
    int i = 0;
    while (i < static_cast<int>(label.size())) {
        bool saw_digit = false;
        while (i < static_cast<int>(label.size()) && std::iswdigit(label[i])) {
            saw_digit = true;
            i++;
        }
        if (!saw_digit) {
            break;
        }
        components++;
        if (i >= static_cast<int>(label.size()) || label[i] != L'.') {
            break;
        }
        i++;
    }
    return components;
}

static bool toc_match_prefix_regex(
    const std::wstring& text,
    const std::wregex& regex,
    int level,
    int* out_level,
    int* out_prefix_end,
    bool* out_bare_marker) {

    std::wsmatch match;
    if (!std::regex_search(text, match, regex) || match.position() != 0) {
        return false;
    }

    int prefix_end = static_cast<int>(match.length());
    std::wstring rest = toc_trim(text.substr(prefix_end));
    *out_level = level;
    *out_prefix_end = prefix_end;
    *out_bare_marker = !toc_has_alpha(rest);
    return true;
}

static bool toc_match_structured_heading_prefix(
    const std::wstring& raw_text,
    int* out_level,
    int* out_prefix_end,
    bool* out_bare_marker) {

    std::wstring text = toc_normalize_spaces(raw_text);
    if (text.empty()) {
        return false;
    }

    static const std::wregex chapter_regex(
        L"^(chapter|chap\\.?|ch\\.?|part|book|lecture)\\s+([0-9]+|[ivxlcdm]+)\\b\\s*[:\\.\\-–—]?\\s*",
        std::regex_constants::icase);
    static const std::wregex appendix_regex(
        L"^(appendix)\\s+([a-z]|[0-9]+|[ivxlcdm]+)\\b\\s*[:\\.\\-–—]?\\s*",
        std::regex_constants::icase);
    static const std::wregex section_regex(
        L"^(§+|section|sec\\.?)\\s*([0-9]+(\\.[0-9]+)*\\.?|[ivxlcdm]+\\b\\.?)\\s*[:\\.\\-–—]?\\s*",
        std::regex_constants::icase);
    static const std::wregex arabic_outline_regex(
        L"^([0-9]+(\\.[0-9]+)*\\.?)\\s+",
        std::regex_constants::icase);
    static const std::wregex roman_outline_regex(
        L"^([ivxlcdm]+)\\.\\s+",
        std::regex_constants::icase);

    if (toc_match_prefix_regex(text, chapter_regex, 0, out_level, out_prefix_end, out_bare_marker) ||
        toc_match_prefix_regex(text, appendix_regex, 0, out_level, out_prefix_end, out_bare_marker)) {
        return true;
    }

    std::wsmatch match;
    if (std::regex_search(text, match, section_regex) && match.position() == 0) {
        int components = toc_count_outline_components(match.str(2));
        *out_level = std::max(1, components);
        *out_prefix_end = static_cast<int>(match.length());
        *out_bare_marker = !toc_has_alpha(toc_trim(text.substr(*out_prefix_end)));
        return true;
    }

    if (std::regex_search(text, match, arabic_outline_regex) && match.position() == 0) {
        int components = toc_count_outline_components(match.str(1));
        if (components > 0) {
            *out_level = std::min(components - 1, 5);
            *out_prefix_end = static_cast<int>(match.length());
            *out_bare_marker = !toc_has_alpha(toc_trim(text.substr(*out_prefix_end)));
            return true;
        }
    }

    if (std::regex_search(text, match, roman_outline_regex) && match.position() == 0) {
        std::wstring roman = toc_strip_edge_punctuation(match.str(1));
        if (toc_parse_roman(roman) > 0) {
            *out_level = 0;
            *out_prefix_end = static_cast<int>(match.length());
            *out_bare_marker = !toc_has_alpha(toc_trim(text.substr(*out_prefix_end)));
            return true;
        }
    }

    return false;
}

static bool toc_is_contents_heading(const std::wstring& text) {
    std::wstring lower = toc_to_lower(toc_normalize_spaces(text));
    return lower == L"contents" ||
        lower == L"table of contents" ||
        lower == L"toc" ||
        lower == L"detailed contents";
}

static bool toc_is_unhelpful_heading(const std::wstring& text) {
    std::wstring lower = toc_to_lower(toc_normalize_spaces(text));
    return lower == L"contents" ||
        lower == L"table of contents" ||
        lower == L"list of figures" ||
        lower == L"list of tables";
}

static std::wstring toc_strip_leading_number(const std::wstring& text) {
    std::wstring str = toc_normalize_spaces(text);
    int level = 0;
    int prefix_end = 0;
    bool bare_marker = false;
    if (toc_match_structured_heading_prefix(str, &level, &prefix_end, &bare_marker) && prefix_end < static_cast<int>(str.size())) {
        std::wstring stripped = toc_trim(str.substr(prefix_end));
        if (!stripped.empty()) {
            return stripped;
        }
    }

    return str;
}

static std::wstring toc_title_match_key(const std::wstring& text) {
    std::wstring stripped = toc_strip_leading_number(text);
    std::wstring lower = toc_to_lower(stripped);
    std::wstring res;
    bool last_was_space = true;

    for (wchar_t c : lower) {
        if (std::iswalnum(c)) {
            res.push_back(c);
            last_was_space = false;
        }
        else if (!last_was_space) {
            res.push_back(L' ');
            last_was_space = true;
        }
    }
    return toc_trim(res);
}

static float toc_lcs_similarity(const std::wstring& lhs, const std::wstring& rhs) {
    if (lhs.empty() || rhs.empty()) {
        return 0.0f;
    }

    std::vector<int> prev(rhs.size() + 1, 0);
    std::vector<int> cur(rhs.size() + 1, 0);
    for (size_t i = 1; i <= lhs.size(); i++) {
        for (size_t j = 1; j <= rhs.size(); j++) {
            if (lhs[i - 1] == rhs[j - 1]) {
                cur[j] = prev[j - 1] + 1;
            }
            else {
                cur[j] = std::max(prev[j], cur[j - 1]);
            }
        }
        std::swap(prev, cur);
        std::fill(cur.begin(), cur.end(), 0);
    }

    return static_cast<float>(prev[rhs.size()]) / static_cast<float>(std::max(lhs.size(), rhs.size()));
}

static float toc_token_similarity(const std::wstring& lhs, const std::wstring& rhs) {
    std::set<std::wstring> lhs_tokens;
    std::set<std::wstring> rhs_tokens;

    for (const auto& token : split_whitespace(lhs)) {
        if (token.size() > 1) {
            lhs_tokens.insert(token);
        }
    }
    for (const auto& token : split_whitespace(rhs)) {
        if (token.size() > 1) {
            rhs_tokens.insert(token);
        }
    }

    if (lhs_tokens.empty() || rhs_tokens.empty()) {
        return 0.0f;
    }

    int intersection = 0;
    for (const auto& token : lhs_tokens) {
        if (rhs_tokens.find(token) != rhs_tokens.end()) {
            intersection++;
        }
    }
    return static_cast<float>(intersection) / static_cast<float>(std::max(lhs_tokens.size(), rhs_tokens.size()));
}

static float toc_title_similarity(const std::wstring& lhs_title, const std::wstring& rhs_title) {
    std::wstring lhs = toc_title_match_key(lhs_title);
    std::wstring rhs = toc_title_match_key(rhs_title);
    if (lhs.empty() || rhs.empty()) {
        return 0.0f;
    }
    if (lhs == rhs) {
        return 1.0f;
    }
    if ((lhs.size() > 5 && toc_contains(rhs, lhs)) || (rhs.size() > 5 && toc_contains(lhs, rhs))) {
        return 0.9f;
    }
    return 0.65f * toc_lcs_similarity(lhs, rhs) + 0.35f * toc_token_similarity(lhs, rhs);
}

static fz_rect toc_line_rect(fz_stext_line* line) {
    fz_rect rect = fz_empty_rect;
    bool has_rect = false;
    LL_ITER(ch, line->first_char) {
        fz_rect ch_rect = fz_rect_from_quad(ch->quad);
        if (!has_rect) {
            rect = ch_rect;
            has_rect = true;
        }
        else {
            rect = fz_union_rect(rect, ch_rect);
        }
    }
    return has_rect ? rect : line->bbox;
}

static std::vector<TocTextLine> toc_extract_text_lines(fz_stext_page* stext_page) {
    std::vector<TocTextLine> lines;
    if (!stext_page) {
        return lines;
    }

    LL_ITER(block, stext_page->first_block) {
        if (block->type != FZ_STEXT_BLOCK_TEXT) {
            continue;
        }

        LL_ITER(line, block->u.t.first_line) {
            TocTextLine toc_line;
            toc_line.text = toc_normalize_spaces(get_string_from_stext_line(line, true));
            if (toc_line.text.empty()) {
                continue;
            }

            toc_line.rect = toc_line_rect(line);

            float size_sum = 0.0f;
            int size_count = 0;
            int bold_count = 0;
            LL_ITER(ch, line->first_char) {
                size_sum += ch->size;
                size_count++;
                if ((ch->flags & FZ_STEXT_BOLD) != 0) {
                    bold_count++;
                }
            }
            if (size_count > 0) {
                toc_line.font_size = size_sum / size_count;
                toc_line.bold = bold_count > (size_count / 3);
            }
            lines.push_back(toc_line);
        }
    }

    std::sort(lines.begin(), lines.end(), [](const TocTextLine& lhs, const TocTextLine& rhs) {
        if (std::abs(lhs.rect.y0 - rhs.rect.y0) > 2.0f) {
            return lhs.rect.y0 < rhs.rect.y0;
        }
        return lhs.rect.x0 < rhs.rect.x0;
    });

    return lines;
}

static float toc_estimate_body_font_size(const std::vector<TocTextLine>& lines) {
    std::vector<float> sizes;
    for (const auto& line : lines) {
        if (line.font_size > 0.0f && line.text.size() >= 20 && toc_count_words(line.text) >= 4) {
            sizes.push_back(line.font_size);
        }
    }
    if (sizes.empty()) {
        for (const auto& line : lines) {
            if (line.font_size > 0.0f) {
                sizes.push_back(line.font_size);
            }
        }
    }
    if (sizes.empty()) {
        return 10.0f;
    }

    std::nth_element(sizes.begin(), sizes.begin() + sizes.size() / 2, sizes.end());
    return sizes[sizes.size() / 2];
}

static bool toc_get_numbered_level(const std::wstring& text, int* out_level) {
    int prefix_end = 0;
    bool bare_marker = false;
    if (toc_match_structured_heading_prefix(text, out_level, &prefix_end, &bare_marker)) {
        return true;
    }

    return false;
}

static bool toc_is_major_heading_keyword(const std::wstring& text) {
    std::wstring lower = toc_to_lower(toc_normalize_spaces(text));
    return toc_starts_with(lower, L"chapter ") ||
        toc_starts_with(lower, L"chap. ") ||
        toc_starts_with(lower, L"ch. ") ||
        toc_starts_with(lower, L"part ") ||
        toc_starts_with(lower, L"book ") ||
        toc_starts_with(lower, L"lecture ") ||
        toc_starts_with(lower, L"appendix ") ||
        lower == L"preface" ||
        lower == L"foreword" ||
        lower == L"introduction" ||
        lower == L"conclusion" ||
        lower == L"references" ||
        lower == L"bibliography" ||
        lower == L"index" ||
        lower == L"acknowledgments" ||
        lower == L"acknowledgements";
}

static int toc_infer_level(const std::wstring& title, float x, float min_x) {
    int numbered_level = 0;
    if (toc_get_numbered_level(title, &numbered_level)) {
        return std::max(0, numbered_level);
    }

    std::wstring lower = toc_to_lower(toc_normalize_spaces(title));
    if (toc_starts_with(lower, L"part ") || toc_starts_with(lower, L"book ") || toc_starts_with(lower, L"lecture ")) {
        return 0;
    }
    if (toc_is_major_heading_keyword(title)) {
        return 0;
    }

    float indent = x - min_x;
    if (indent > 60.0f) {
        return 2;
    }
    if (indent > 25.0f) {
        return 1;
    }
    return 0;
}

static bool toc_looks_like_sentence(const std::wstring& text) {
    std::wstring trimmed = toc_trim(text);
    if (trimmed.empty()) {
        return false;
    }
    int words = toc_count_words(trimmed);
    wchar_t last = trimmed.back();
    return words > 8 && (last == L'.' || last == L',' || last == L';' || last == L':');
}

static bool toc_is_bare_structural_heading(const std::wstring& text) {
    int level = 0;
    int prefix_end = 0;
    bool bare_marker = false;
    return toc_match_structured_heading_prefix(text, &level, &prefix_end, &bare_marker) && bare_marker;
}

static bool toc_line_is_title_fragment(const TocTextLine& line) {
    std::wstring title = toc_normalize_spaces(line.text);
    if (title.size() < 3 || title.size() > 110 || !toc_has_alpha(title)) {
        return false;
    }
    if (toc_is_unhelpful_heading(title) || toc_looks_like_sentence(title)) {
        return false;
    }
    if (toc_is_all_digits(title) || toc_is_roman_numeral(title)) {
        return false;
    }
    return toc_count_words(title) <= 14;
}

static fz_rect toc_union_line_rects(fz_rect lhs, fz_rect rhs) {
    if (fz_is_empty_rect(lhs)) {
        return rhs;
    }
    if (fz_is_empty_rect(rhs)) {
        return lhs;
    }
    return fz_union_rect(lhs, rhs);
}

static std::vector<TocTextLine> toc_add_combined_heading_lines(
    const std::vector<TocTextLine>& lines,
    float page_height) {

    std::vector<TocTextLine> augmented = lines;
    for (int i = 0; i + 1 < static_cast<int>(lines.size()); i++) {
        const TocTextLine& first = lines[i];
        if (!toc_is_bare_structural_heading(first.text)) {
            continue;
        }

        int next_index = i + 1;
        while (next_index < static_cast<int>(lines.size()) && toc_trim(lines[next_index].text).empty()) {
            next_index++;
        }
        if (next_index >= static_cast<int>(lines.size())) {
            continue;
        }

        const TocTextLine& second = lines[next_index];
        if (!toc_line_is_title_fragment(second)) {
            continue;
        }

        float gap = second.rect.y0 - first.rect.y1;
        float font = std::max(first.font_size, second.font_size);
        if (font <= 0.0f) {
            font = 12.0f;
        }
        if (gap < -2.0f || gap > std::max(36.0f, font * 3.0f)) {
            continue;
        }
        if (page_height > 0.0f && first.rect.y0 > page_height * 0.55f) {
            continue;
        }
        if (std::abs(first.rect.x0 - second.rect.x0) > 90.0f && second.rect.x0 < first.rect.x0) {
            continue;
        }

        TocTextLine combined;
        std::wstring separator = first.text.find(L'.') == std::wstring::npos ? L". " : L" ";
        combined.text = toc_normalize_spaces(first.text + separator + second.text);
        combined.rect = toc_union_line_rects(first.rect, second.rect);
        combined.font_size = std::max(first.font_size, second.font_size);
        combined.bold = first.bold || second.bold;
        augmented.push_back(combined);
    }

    std::sort(augmented.begin(), augmented.end(), [](const TocTextLine& lhs, const TocTextLine& rhs) {
        if (std::abs(lhs.rect.y0 - rhs.rect.y0) > 2.0f) {
            return lhs.rect.y0 < rhs.rect.y0;
        }
        if (std::abs(lhs.rect.x0 - rhs.rect.x0) > 2.0f) {
            return lhs.rect.x0 < rhs.rect.x0;
        }
        return lhs.text.size() > rhs.text.size();
    });
    return augmented;
}

static std::vector<CreatedTocCandidate> toc_detect_heading_candidates(
    const std::vector<TocTextLine>& lines,
    int page_number,
    float page_height) {

    std::vector<CreatedTocCandidate> candidates;
    std::vector<TocTextLine> heading_lines = toc_add_combined_heading_lines(lines, page_height);
    float body_size = toc_estimate_body_font_size(lines);

    for (const auto& line : heading_lines) {
        std::wstring title = toc_normalize_spaces(line.text);
        if (title.size() < 3 || title.size() > 140 || !toc_has_alpha(title)) {
            continue;
        }
        if (toc_is_unhelpful_heading(title) || toc_looks_like_sentence(title)) {
            continue;
        }
        if (toc_is_all_digits(title) || toc_is_roman_numeral(title)) {
            continue;
        }

        int numbered_level = 0;
        bool structured = toc_get_numbered_level(title, &numbered_level);
        bool numbered = structured || is_string_titlish(title);
        bool keyword = toc_is_major_heading_keyword(title);
        bool bare_structural = toc_is_bare_structural_heading(title);
        if (bare_structural) {
            continue;
        }

        float score = 0.0f;
        if (numbered) {
            score += 3.0f;
        }
        if (keyword) {
            score += 3.0f;
        }
        if (line.font_size >= body_size * 1.25f) {
            score += 3.0f;
        }
        else if (line.font_size >= body_size * 1.12f) {
            score += 2.0f;
        }
        else if (line.font_size >= body_size * 1.05f) {
            score += 1.0f;
        }
        if (line.bold) {
            score += 1.0f;
        }
        if (line.rect.y0 < page_height * 0.30f) {
            score += 1.0f;
        }
        if (toc_count_words(title) <= 8) {
            score += 1.0f;
        }
        if (!numbered && !keyword && title.size() > 90) {
            score -= 1.0f;
        }
        if (!numbered && !keyword && toc_to_lower(title) == title) {
            score -= 1.0f;
        }

        if (score < 3.0f) {
            continue;
        }

        CreatedTocCandidate candidate;
        candidate.title = title;
        candidate.page = page_number;
        candidate.x = line.rect.x0;
        candidate.y = line.rect.y0;
        candidate.page_height = page_height;
        candidate.level = numbered ? numbered_level : toc_infer_level(title, line.rect.x0, 0.0f);
        candidate.score = score;
        candidate.structural = structured || keyword;
        candidate.margin = page_height > 0.0f && (line.rect.y0 < page_height * 0.09f || line.rect.y0 > page_height * 0.92f);
        candidates.push_back(candidate);
    }

    return candidates;
}

static bool toc_is_page_label_char(wchar_t c) {
    return std::iswalnum(c) || c == L'-';
}

static bool toc_is_page_label_text(const std::wstring& text) {
    std::wstring label = toc_strip_edge_punctuation(toc_normalize_spaces(text));
    return toc_is_all_digits(label) || toc_is_roman_numeral(label);
}

static bool toc_parse_printed_toc_line(const TocTextLine& line, PrintedTocCandidate* out_candidate) {
    std::wstring text = toc_normalize_spaces(line.text);
    if (text.size() < 6 || text.size() > 180 || !toc_has_alpha(text)) {
        return false;
    }
    if (toc_is_contents_heading(text)) {
        return false;
    }

    int end = static_cast<int>(text.size()) - 1;
    while (end >= 0 && toc_is_space(text[end])) {
        end--;
    }
    int start = end;
    while (start >= 0 && toc_is_page_label_char(text[start])) {
        start--;
    }

    if (start == end) {
        return false;
    }

    std::wstring page_label = toc_strip_edge_punctuation(text.substr(start + 1, end - start));
    if (!toc_is_page_label_text(page_label)) {
        return false;
    }

    std::wstring title = text.substr(0, start + 1);
    bool has_dot_leader = false;
    int trailing_dots = 0;
    while (!title.empty()) {
        wchar_t c = title.back();
        if (c == L'.' || c == L'·' || c == L'•' || c == L'…') {
            trailing_dots++;
            has_dot_leader = true;
            title.pop_back();
        }
        else if (toc_is_space(c)) {
            title.pop_back();
        }
        else {
            break;
        }
    }

    title = toc_normalize_spaces(title);
    if (title.size() < 3 || title.size() > 140 || !toc_has_alpha(title)) {
        return false;
    }
    if (toc_is_unhelpful_heading(title)) {
        return false;
    }
    if (!has_dot_leader && trailing_dots < 2 && toc_count_words(title) < 2 && !toc_is_major_heading_keyword(title)) {
        return false;
    }

    PrintedTocCandidate candidate;
    candidate.title = title;
    candidate.page_label = page_label;
    candidate.rect = line.rect;
    candidate.x = line.rect.x0;
    candidate.y = line.rect.y0;
    candidate.has_dot_leader = has_dot_leader;
    candidate.level = toc_infer_level(title, line.rect.x0, 0.0f);
    *out_candidate = candidate;
    return true;
}

static float toc_line_center_y(const TocTextLine& line) {
    return (line.rect.y0 + line.rect.y1) * 0.5f;
}

static TocTextLine toc_merge_printed_row_window(const std::vector<TocTextLine>& row_lines, int begin, int end) {
    TocTextLine merged = row_lines[begin];
    for (int i = begin + 1; i <= end; i++) {
        merged.text = toc_normalize_spaces(merged.text + L" " + row_lines[i].text);
        merged.rect = toc_union_line_rects(merged.rect, row_lines[i].rect);
        merged.font_size = std::max(merged.font_size, row_lines[i].font_size);
        merged.bold = merged.bold || row_lines[i].bold;
    }
    return merged;
}

static bool toc_parse_printed_toc_row(const std::vector<TocTextLine>& row_lines, TocTextLine* out_probe_line) {
    if (row_lines.size() < 2) {
        return false;
    }

    int label_index = static_cast<int>(row_lines.size()) - 1;
    if (!toc_is_page_label_text(row_lines[label_index].text)) {
        return false;
    }

    TocTextLine title_line = toc_merge_printed_row_window(row_lines, 0, label_index - 1);
    std::wstring title = toc_normalize_spaces(title_line.text);
    if (title.size() < 3 || !toc_has_alpha(title) || toc_is_unhelpful_heading(title)) {
        return false;
    }

    TocTextLine probe_line = title_line;
    probe_line.text = toc_normalize_spaces(title + L" " + toc_strip_edge_punctuation(row_lines[label_index].text));
    probe_line.rect = toc_union_line_rects(title_line.rect, row_lines[label_index].rect);
    probe_line.font_size = std::max(title_line.font_size, row_lines[label_index].font_size);
    probe_line.bold = title_line.bold || row_lines[label_index].bold;

    PrintedTocCandidate ignored;
    if (!toc_parse_printed_toc_line(probe_line, &ignored)) {
        return false;
    }

    *out_probe_line = probe_line;
    return true;
}

static std::vector<TocTextLine> toc_build_printed_toc_probe_lines(const std::vector<TocTextLine>& lines) {
    std::vector<TocTextLine> probes = lines;
    if (lines.empty()) {
        return probes;
    }

    std::vector<std::vector<TocTextLine>> rows;
    for (const auto& line : lines) {
        bool inserted = false;
        float center_y = toc_line_center_y(line);
        for (auto& row : rows) {
            float row_y = toc_line_center_y(row.front());
            float tolerance = std::max(3.0f, std::max(row.front().font_size, line.font_size) * 0.65f);
            if (std::abs(center_y - row_y) <= tolerance) {
                row.push_back(line);
                inserted = true;
                break;
            }
        }
        if (!inserted) {
            rows.push_back({ line });
        }
    }

    std::sort(rows.begin(), rows.end(), [](const std::vector<TocTextLine>& lhs, const std::vector<TocTextLine>& rhs) {
        return toc_line_center_y(lhs.front()) < toc_line_center_y(rhs.front());
    });

    for (auto& row : rows) {
        std::sort(row.begin(), row.end(), [](const TocTextLine& lhs, const TocTextLine& rhs) {
            return lhs.rect.x0 < rhs.rect.x0;
        });

        TocTextLine row_probe;
        if (toc_parse_printed_toc_row(row, &row_probe)) {
            probes.push_back(row_probe);
        }
    }

    for (int i = 0; i + 1 < static_cast<int>(rows.size()); i++) {
        if (toc_is_page_label_text(rows[i].back().text) || !toc_is_page_label_text(rows[i + 1].back().text)) {
            continue;
        }
        float row_gap = rows[i + 1].front().rect.y0 - rows[i].front().rect.y1;
        float font = std::max(rows[i].front().font_size, rows[i + 1].front().font_size);
        if (row_gap < -2.0f || row_gap > std::max(18.0f, font * 2.0f)) {
            continue;
        }
        if (std::abs(rows[i].front().rect.x0 - rows[i + 1].front().rect.x0) > 35.0f) {
            continue;
        }

        std::vector<TocTextLine> wrapped_row = rows[i];
        wrapped_row.insert(wrapped_row.end(), rows[i + 1].begin(), rows[i + 1].end());
        std::sort(wrapped_row.begin(), wrapped_row.end(), [](const TocTextLine& lhs, const TocTextLine& rhs) {
            if (std::abs(toc_line_center_y(lhs) - toc_line_center_y(rhs)) > 2.0f) {
                return toc_line_center_y(lhs) < toc_line_center_y(rhs);
            }
            return lhs.rect.x0 < rhs.rect.x0;
        });

        TocTextLine wrapped_probe;
        if (toc_parse_printed_toc_row(wrapped_row, &wrapped_probe)) {
            probes.push_back(wrapped_probe);
        }
    }

    for (int i = 0; i + 1 < static_cast<int>(rows.size()); i++) {
        if (!toc_is_page_label_text(rows[i].back().text) || toc_is_page_label_text(rows[i + 1].back().text)) {
            continue;
        }
        if (rows[i].size() < 2) {
            continue;
        }

        TocTextLine first_title = toc_merge_printed_row_window(rows[i], 0, static_cast<int>(rows[i].size()) - 2);
        TocTextLine second_title = toc_merge_printed_row_window(rows[i + 1], 0, static_cast<int>(rows[i + 1].size()) - 1);
        if (!toc_is_bare_structural_heading(first_title.text) || !toc_line_is_title_fragment(second_title)) {
            continue;
        }

        float row_gap = rows[i + 1].front().rect.y0 - rows[i].front().rect.y1;
        float font = std::max(rows[i].front().font_size, rows[i + 1].front().font_size);
        if (row_gap < -2.0f || row_gap > std::max(18.0f, font * 2.0f)) {
            continue;
        }
        if (std::abs(rows[i].front().rect.x0 - rows[i + 1].front().rect.x0) > 45.0f) {
            continue;
        }

        std::vector<TocTextLine> expanded_row;
        expanded_row.insert(expanded_row.end(), rows[i].begin(), rows[i].end() - 1);
        expanded_row.insert(expanded_row.end(), rows[i + 1].begin(), rows[i + 1].end());
        expanded_row.push_back(rows[i].back());

        TocTextLine expanded_probe;
        if (toc_parse_printed_toc_row(expanded_row, &expanded_probe)) {
            probes.push_back(expanded_probe);
        }
    }

    return probes;
}

static std::vector<PrintedTocCandidate> toc_detect_printed_toc_candidates(
    const std::vector<TocTextLine>& lines,
    int page_number,
    bool printed_toc_already_started,
    bool* out_page_looks_like_toc) {

    std::vector<PrintedTocCandidate> candidates;
    std::vector<TocTextLine> probe_lines = toc_build_printed_toc_probe_lines(lines);
    bool has_contents_title = false;
    int dot_leader_count = 0;
    float min_x = 1000000.0f;

    for (const auto& line : lines) {
        if (toc_is_contents_heading(line.text)) {
            has_contents_title = true;
        }
        min_x = std::min(min_x, line.rect.x0);
    }
    if (min_x == 1000000.0f) {
        min_x = 0.0f;
    }

    std::set<std::tuple<std::wstring, std::wstring, int>> seen_candidates;
    for (const auto& line : probe_lines) {
        PrintedTocCandidate candidate;
        if (toc_parse_printed_toc_line(line, &candidate)) {
            if (toc_is_bare_structural_heading(candidate.title)) {
                continue;
            }
            candidate.source_page = page_number;
            candidate.level = toc_infer_level(candidate.title, candidate.x, min_x);
            auto key = std::make_tuple(toc_title_match_key(candidate.title), toc_to_lower(candidate.page_label), candidate.source_page);
            if (seen_candidates.find(key) != seen_candidates.end()) {
                continue;
            }
            seen_candidates.insert(key);
            if (candidate.has_dot_leader) {
                dot_leader_count++;
            }
            candidates.push_back(candidate);
        }
    }

    bool looks_like_toc = has_contents_title ||
        candidates.size() >= 5 ||
        (printed_toc_already_started && candidates.size() >= 2) ||
        (candidates.size() >= 3 && dot_leader_count >= 2);

    *out_page_looks_like_toc = looks_like_toc;
    if (!looks_like_toc) {
        candidates.clear();
    }
    return candidates;
}

static fz_rect toc_expand_rect(fz_rect rect, float amount) {
    rect.x0 -= amount;
    rect.x1 += amount;
    rect.y0 -= amount;
    rect.y1 += amount;
    return rect;
}

static float toc_intersection_area(fz_rect lhs, fz_rect rhs) {
    fz_rect intersection = fz_intersect_rect(lhs, rhs);
    if (fz_is_empty_rect(intersection)) {
        return 0.0f;
    }
    return std::max(0.0f, intersection.x1 - intersection.x0) *
        std::max(0.0f, intersection.y1 - intersection.y0);
}

static void toc_attach_link_targets_to_printed_candidates(
    fz_context* context,
    fz_document* document,
    fz_link* links,
    std::vector<PrintedTocCandidate>& candidates,
    int num_pages) {

    for (auto& candidate : candidates) {
        fz_rect candidate_rect = toc_expand_rect(candidate.rect, 3.0f);
        fz_link* best_link = nullptr;
        float best_overlap = 0.0f;

        for (fz_link* link = links; link; link = link->next) {
            if (!link->uri || fz_is_empty_rect(link->rect)) {
                continue;
            }
            float overlap = toc_intersection_area(candidate_rect, link->rect);
            if (overlap > best_overlap) {
                best_overlap = overlap;
                best_link = link;
            }
        }

        if (!best_link || best_overlap <= 0.0f) {
            continue;
        }

        float target_x = 0.0f;
        float target_y = 0.0f;
        fz_try(context) {
            fz_location loc = fz_resolve_link(context, document, best_link->uri, &target_x, &target_y);
            int target_page = fz_page_number_from_location(context, document, loc);
            if (target_page >= 0 && target_page < num_pages) {
                candidate.has_link_target = true;
                candidate.link_target_page = target_page;
                candidate.link_target_x = std::isnan(target_x) ? 0.0f : target_x;
                candidate.link_target_y = std::isnan(target_y) ? 0.0f : target_y;
            }
        }
        fz_catch(context) {
        }
    }
}

static int toc_resolve_page_label(const std::vector<std::wstring>& page_labels, const std::wstring& label, int num_pages) {
    std::wstring normalized_label = toc_to_lower(toc_normalize_spaces(label));
    for (int i = 0; i < static_cast<int>(page_labels.size()); i++) {
        if (toc_to_lower(toc_normalize_spaces(page_labels[i])) == normalized_label) {
            return i;
        }
    }

    int numeric = toc_parse_int(label);
    if (numeric > 0) {
        int page = numeric - 1;
        if (page >= 0 && page < num_pages) {
            return page;
        }
    }

    int roman = toc_parse_roman(label);
    if (roman > 0) {
        int page = roman - 1;
        if (page >= 0 && page < num_pages) {
            return page;
        }
    }

    return -1;
}

static void toc_clear_nodes(std::vector<TocNode*>& nodes) {
    for (auto node : nodes) {
        if (!node) {
            continue;
        }
        toc_clear_nodes(node->children);
        delete node;
    }
    nodes.clear();
}

static int toc_count_nodes(const std::vector<TocNode*>& nodes) {
    int count = 0;
    for (auto node : nodes) {
        if (!node) {
            continue;
        }
        count += 1 + toc_count_nodes(node->children);
    }
    return count;
}

static void toc_flatten_nodes(const std::vector<TocNode*>& nodes, std::vector<TocNode*>& output) {
    for (auto node : nodes) {
        if (!node) {
            continue;
        }
        output.push_back(node);
        toc_flatten_nodes(node->children, output);
    }
}

static float toc_quality_score(const std::vector<TocNode*>& nodes, int num_pages) {
    std::vector<TocNode*> flat;
    toc_flatten_nodes(nodes, flat);
    if (flat.empty() || num_pages <= 0) {
        return 0.0f;
    }

    std::set<int> unique_pages;
    int valid_pages = 0;
    int nonmonotonic = 0;
    int previous_page = -1;
    int min_page = num_pages;
    int max_page = 0;
    int useful_titles = 0;

    for (auto node : flat) {
        if (!node) {
            continue;
        }
        if (node->page >= 0 && node->page < num_pages) {
            valid_pages++;
            unique_pages.insert(node->page);
            min_page = std::min(min_page, node->page);
            max_page = std::max(max_page, node->page);
            if (previous_page > node->page) {
                nonmonotonic++;
            }
            previous_page = node->page;
        }
        std::wstring title_key = toc_title_match_key(node->title);
        if (title_key.size() >= 3 && toc_has_alpha(title_key)) {
            useful_titles++;
        }
    }

    if (valid_pages == 0 || useful_titles == 0) {
        return 0.0f;
    }

    float coverage = static_cast<float>(max_page - min_page + 1) / static_cast<float>(num_pages);
    float monotonic_penalty = static_cast<float>(nonmonotonic) / static_cast<float>(std::max(1, valid_pages - 1));
    float unique_ratio = static_cast<float>(unique_pages.size()) / static_cast<float>(valid_pages);

    return static_cast<float>(flat.size()) * 1.5f +
        static_cast<float>(unique_pages.size()) * 1.25f +
        coverage * 12.0f +
        unique_ratio * 8.0f -
        monotonic_penalty * 12.0f;
}

static int toc_nonmonotonic_count(const std::vector<TocNode*>& nodes) {
    std::vector<TocNode*> flat;
    toc_flatten_nodes(nodes, flat);

    int nonmonotonic = 0;
    int previous_page = -1;
    for (auto node : flat) {
        if (!node || node->page < 0) {
            continue;
        }
        if (previous_page > node->page) {
            nonmonotonic++;
        }
        previous_page = node->page;
    }
    return nonmonotonic;
}

static bool toc_nodes_are_printed_sensible(const std::vector<TocNode*>& nodes, int num_pages) {
    std::vector<TocNode*> flat;
    toc_flatten_nodes(nodes, flat);
    int count = static_cast<int>(flat.size());
    if (count < 3 || num_pages <= 0) {
        return false;
    }

    std::set<int> unique_pages;
    int valid_pages = 0;
    int useful_titles = 0;
    for (auto node : flat) {
        if (!node) {
            continue;
        }
        if (node->page >= 0 && node->page < num_pages) {
            valid_pages++;
            unique_pages.insert(node->page);
        }
        std::wstring title_key = toc_title_match_key(node->title);
        if (title_key.size() >= 3 && toc_has_alpha(title_key)) {
            useful_titles++;
        }
    }

    if (valid_pages < std::max(3, count * 2 / 3) || useful_titles < std::max(3, count * 2 / 3)) {
        return false;
    }
    if (unique_pages.size() < static_cast<size_t>(std::max(2, count / 4))) {
        return false;
    }
    if (toc_nonmonotonic_count(nodes) > std::max(1, count / 5)) {
        return false;
    }
    return toc_quality_score(nodes, num_pages) > static_cast<float>(count) * 2.0f;
}

static bool toc_is_scrappy(const std::vector<TocNode*>& nodes, int num_pages) {
    std::vector<TocNode*> flat;
    toc_flatten_nodes(nodes, flat);
    if (flat.empty()) {
        return true;
    }

    std::set<int> unique_pages;
    int valid_pages = 0;
    int min_page = num_pages;
    int max_page = 0;
    int nonmonotonic = 0;
    int previous_page = -1;

    for (auto node : flat) {
        if (!node) {
            continue;
        }
        if (node->page >= 0 && node->page < num_pages) {
            valid_pages++;
            unique_pages.insert(node->page);
            min_page = std::min(min_page, node->page);
            max_page = std::max(max_page, node->page);
            if (previous_page > node->page) {
                nonmonotonic++;
            }
            previous_page = node->page;
        }
    }

    if (valid_pages == 0) {
        return true;
    }
    if (num_pages >= 80 && flat.size() <= 2) {
        return true;
    }
    if (flat.size() > 1 && unique_pages.size() <= 1) {
        return true;
    }

    float coverage = static_cast<float>(max_page - min_page + 1) / static_cast<float>(std::max(1, num_pages));
    if (num_pages >= 100 && flat.size() < 5 && coverage < 0.20f) {
        return true;
    }
    if (flat.size() >= 5 && nonmonotonic > static_cast<int>(flat.size() / 3)) {
        return true;
    }

    return false;
}

static void toc_filter_heading_candidates(std::vector<CreatedTocCandidate>& candidates, int num_pages) {
    std::map<std::wstring, int> title_counts;
    std::map<std::wstring, int> margin_title_counts;
    for (const auto& candidate : candidates) {
        std::wstring key = toc_title_match_key(candidate.title);
        title_counts[key]++;
        if (candidate.margin) {
            margin_title_counts[key]++;
        }
    }

    int repeat_threshold = std::max(3, num_pages / 30);
    std::vector<CreatedTocCandidate> filtered;
    std::set<std::pair<int, std::wstring>> seen_on_page;
    std::set<std::wstring> emitted_repeated_titles;
    for (size_t i = 0; i < candidates.size(); i++) {
        const auto& candidate = candidates[i];
        std::wstring key = toc_title_match_key(candidate.title);
        if (key.empty()) {
            continue;
        }
        if (candidate.margin && title_counts[key] > repeat_threshold && margin_title_counts[key] >= title_counts[key] / 2) {
            continue;
        }
        if (toc_is_bare_structural_heading(candidate.title)) {
            bool has_richer_heading_nearby = false;
            for (size_t j = 0; j < candidates.size(); j++) {
                if (i == j || candidates[j].page != candidate.page) {
                    continue;
                }
                if (std::abs(candidates[j].y - candidate.y) > 90.0f) {
                    continue;
                }
                if (!toc_is_bare_structural_heading(candidates[j].title) && toc_contains(toc_to_lower(candidates[j].title), toc_to_lower(candidate.title))) {
                    has_richer_heading_nearby = true;
                    break;
                }
            }
            if (has_richer_heading_nearby) {
                continue;
            }
        }
        if (title_counts[key] > repeat_threshold) {
            if (emitted_repeated_titles.find(key) != emitted_repeated_titles.end()) {
                continue;
            }
            emitted_repeated_titles.insert(key);
        }
        auto page_key = std::make_pair(candidate.page, key);
        if (seen_on_page.find(page_key) != seen_on_page.end()) {
            continue;
        }
        seen_on_page.insert(page_key);
        filtered.push_back(candidate);
    }
    candidates = std::move(filtered);
}

static TocNode* toc_node_from_candidate(const CreatedTocCandidate& candidate) {
    TocNode* node = new TocNode;
    node->title = candidate.title;
    node->page = candidate.page;
    node->x = candidate.x;
    node->y = candidate.y;
    return node;
}

static void toc_append_node_by_level(std::vector<TocNode*>& top_level_nodes, std::vector<std::pair<int, TocNode*>>& stack, int level, TocNode* node) {
    level = std::max(0, level);
    while (!stack.empty() && stack.back().first >= level) {
        stack.pop_back();
    }

    if (stack.empty()) {
        top_level_nodes.push_back(node);
    }
    else {
        stack.back().second->children.push_back(node);
    }
    stack.push_back(std::make_pair(level, node));
}

static std::vector<TocNode*> toc_build_heading_tree(std::vector<CreatedTocCandidate> candidates, int max_entries) {
    std::vector<TocNode*> top_level_nodes;
    std::vector<std::pair<int, TocNode*>> stack;
    std::sort(candidates.begin(), candidates.end(), [](const CreatedTocCandidate& lhs, const CreatedTocCandidate& rhs) {
        if (lhs.page != rhs.page) {
            return lhs.page < rhs.page;
        }
        return lhs.y < rhs.y;
    });

    int added = 0;
    for (const auto& candidate : candidates) {
        if (added >= max_entries) {
            break;
        }
        TocNode* node = toc_node_from_candidate(candidate);
        toc_append_node_by_level(top_level_nodes, stack, candidate.level, node);
        added++;
    }
    return top_level_nodes;
}

static std::optional<CreatedTocCandidate> toc_find_best_heading_match(
    const PrintedTocCandidate& printed,
    const std::vector<CreatedTocCandidate>& headings,
    int label_page,
    int num_pages) {

    float best_score = 0.0f;
    float best_similarity = 0.0f;
    std::optional<CreatedTocCandidate> best = {};
    for (const auto& heading : headings) {
        if (heading.page <= printed.source_page) {
            continue;
        }
        float similarity = toc_title_similarity(printed.title, heading.title);
        if (similarity < 0.58f) {
            continue;
        }

        float score = similarity;
        if (label_page >= 0) {
            int offset = heading.page - label_page;
            int max_reasonable_offset = std::max(80, num_pages / 3);
            if (offset < -3 || offset > max_reasonable_offset) {
                continue;
            }
            score -= static_cast<float>(std::min(std::abs(offset), 80)) * 0.001f;
        }

        if (score > best_score) {
            best_score = score;
            best_similarity = similarity;
            best = heading;
        }
    }

    if (best && best_similarity >= 0.72f) {
        return best;
    }
    return {};
}

static int toc_median_offset(std::vector<int> offsets) {
    if (offsets.empty()) {
        return 0;
    }
    std::nth_element(offsets.begin(), offsets.begin() + offsets.size() / 2, offsets.end());
    return offsets[offsets.size() / 2];
}

static std::string toc_make_internal_link_uri(int page, float x, float y) {
    (void)x;
    if (std::isnan(y) || y < 0.0f) {
        return QString("#page=%1").arg(page + 1).toStdString();
    }
    return QString("#page=%1&view=FitH,%2")
        .arg(page + 1)
        .arg(static_cast<double>(y))
        .toStdString();
}

static std::vector<ResolvedPrintedTocEntry> toc_resolve_printed_toc_entries(
    const std::vector<PrintedTocCandidate>& printed_candidates,
    const std::vector<CreatedTocCandidate>& headings,
    const std::vector<std::wstring>& page_labels,
    int num_pages) {

    std::vector<std::optional<CreatedTocCandidate>> matched_headings;
    std::vector<int> label_pages;
    std::vector<int> offsets;
    matched_headings.reserve(printed_candidates.size());
    label_pages.reserve(printed_candidates.size());

    for (const auto& printed : printed_candidates) {
        int label_page = toc_resolve_page_label(page_labels, printed.page_label, num_pages);
        label_pages.push_back(label_page);
        std::optional<CreatedTocCandidate> match = {};
        if (!printed.has_link_target) {
            match = toc_find_best_heading_match(printed, headings, label_page, num_pages);
        }
        matched_headings.push_back(match);
        if (match && label_page >= 0) {
            offsets.push_back(match->page - label_page);
        }
    }

    int page_offset = 0;
    if (!offsets.empty()) {
        page_offset = toc_median_offset(offsets);
    }
    else if (toc_is_plain_page_label_sequence(page_labels)) {
        int max_source_page = -1;
        int min_printed_arabic_label = num_pages + 1;
        for (const auto& printed : printed_candidates) {
            max_source_page = std::max(max_source_page, printed.source_page);
            int printed_label = toc_parse_int(toc_strip_edge_punctuation(printed.page_label));
            if (printed_label > 0) {
                min_printed_arabic_label = std::min(min_printed_arabic_label, printed_label);
            }
        }
        if (max_source_page >= 0 && min_printed_arabic_label <= max_source_page + 1) {
            page_offset = max_source_page + 1;
        }
    }

    std::vector<ResolvedPrintedTocEntry> resolved;
    std::set<std::pair<std::wstring, std::wstring>> seen_entries;

    for (size_t i = 0; i < printed_candidates.size(); i++) {
        const auto& printed = printed_candidates[i];
        std::wstring key = toc_title_match_key(printed.title);
        std::wstring entry_key_page = toc_to_lower(toc_normalize_spaces(printed.page_label));
        auto entry_key = std::make_pair(key, entry_key_page);
        if (key.empty() || seen_entries.find(entry_key) != seen_entries.end()) {
            continue;
        }

        int target_page = -1;
        float target_x = 0.0f;
        float target_y = 0.0f;
        if (printed.has_link_target) {
            target_page = printed.link_target_page;
            target_x = printed.link_target_x;
            target_y = printed.link_target_y;
        }
        else if (matched_headings[i]) {
            target_page = matched_headings[i]->page;
            target_x = matched_headings[i]->x;
            target_y = matched_headings[i]->y;
        }
        else {
            int label_page = label_pages[i];
            if (label_page >= 0) {
                target_page = label_page + page_offset;
            }
        }

        if (target_page < 0 || target_page >= num_pages) {
            continue;
        }

        seen_entries.insert(entry_key);
        ResolvedPrintedTocEntry entry;
        entry.printed = printed;
        entry.target_page = target_page;
        entry.target_x = target_x;
        entry.target_y = target_y;
        resolved.push_back(entry);
    }

    return resolved;
}

static std::vector<TocNode*> toc_build_printed_tree(
    const std::vector<ResolvedPrintedTocEntry>& resolved_entries,
    int max_entries) {

    std::vector<TocNode*> top_level_nodes;
    std::vector<std::pair<int, TocNode*>> stack;

    int added = 0;
    for (const auto& entry : resolved_entries) {
        if (added >= max_entries) {
            break;
        }

        TocNode* node = new TocNode;
        node->title = entry.printed.title;
        node->page = entry.target_page;
        node->x = entry.target_x;
        node->y = entry.target_y;
        toc_append_node_by_level(top_level_nodes, stack, entry.printed.level, node);
        added++;
    }

    return top_level_nodes;
}

static std::map<int, std::vector<PdfLink>> toc_build_printed_toc_links(
    const std::vector<ResolvedPrintedTocEntry>& resolved_entries,
    int max_entries) {

    std::map<int, std::vector<PdfLink>> links_by_page;
    int added = 0;
    for (const auto& entry : resolved_entries) {
        if (added >= max_entries) {
            break;
        }
        if (entry.printed.source_page < 0 || entry.target_page < 0 || fz_is_empty_rect(entry.printed.rect)) {
            continue;
        }

        fz_rect rect = entry.printed.rect;
        rect.x0 -= 2.0f;
        rect.x1 += 2.0f;
        rect.y0 -= 2.0f;
        rect.y1 += 2.0f;

        PdfLink link;
        link.source_page = entry.printed.source_page;
        link.rects.push_back(rect);
        link.uri = toc_make_internal_link_uri(entry.target_page, entry.target_x, entry.target_y);
        links_by_page[entry.printed.source_page].push_back(link);
        added++;
    }
    return links_by_page;
}

}

int Document::get_mark_index(char symbol) {
    for (size_t i = 0; i < marks.size(); i++) {
        if (marks[i].symbol == symbol) {
            return i;
        }
    }
    return -1;
}

CharacterIterator::CharacterIterator(fz_stext_page* page) {
    block = page->first_block;
    while (block != nullptr && (block->type == FZ_STEXT_BLOCK_IMAGE)) block = block->next;
    if (block != nullptr) {
        line = block->u.t.first_line;
        chr = line->first_char;
    }
    else {
        line = nullptr;
        chr = nullptr;
    }
}

CharacterIterator::CharacterIterator(fz_stext_block* b, fz_stext_line* l, fz_stext_char* c) {
    block = b;
    line = l;
    chr = c;
}

CharacterIterator& CharacterIterator::operator++() {
    if (chr->next != nullptr) {
        chr = chr->next;
        return *this;
    }
    if (line->next != nullptr) {
        line = line->next;
        chr = line->first_char;
        return *this;
    }
    if (block->next != nullptr) {
        block = block->next;
        while (block != nullptr && (block->type == FZ_STEXT_BLOCK_IMAGE)) block = block->next;
        if (block == nullptr) {
            line = nullptr;
            chr = nullptr;
        }
        else {
            line = block->u.t.first_line;
            chr = line->first_char;
        }
        return *this;
    }

    block = nullptr;
    line = nullptr;
    chr = nullptr;

    return *this;
}

CharacterIterator CharacterIterator::operator++(int) {
    CharacterIterator tmp = *this;
    ++(*this);
    return tmp;
}

bool CharacterIterator::operator==(const CharacterIterator& other) const {
    return chr == other.chr;
}

bool CharacterIterator::operator!=(const CharacterIterator& other) const {
    return chr != other.chr;
}

std::tuple<fz_stext_block*, fz_stext_line*, fz_stext_char*> CharacterIterator::operator*() const {
    return std::make_tuple(block, line, chr);
}


PageIterator::PageIterator(fz_stext_page* page) : page(page) {

}

CharacterIterator PageIterator::begin() const {
    return CharacterIterator(page);
}

CharacterIterator PageIterator::end() const {
    return CharacterIterator(nullptr, nullptr, nullptr);
}

void Document::load_document_metadata_from_db() {

    marks.clear();
    bookmarks.clear();
    highlights.clear();
    portals.clear();
    study_objects.clear();
    region_highlights.clear();

    std::optional<std::string> checksum_ = get_checksum_fast();
    if (checksum_) {
        std::string checksum = checksum_.value();
        db_manager->select_mark(checksum, marks);
        db_manager->select_bookmark(checksum, bookmarks);
        db_manager->select_highlight(checksum, highlights);
        db_manager->select_links(checksum, portals);
        db_manager->select_study_objects(checksum, study_objects);
        db_manager->select_region_highlights(checksum, region_highlights);
        should_reload_annotations = false;
    }
    else {
        auto checksum_thread = std::thread([&]() {
            std::string checksum = get_checksum();
            if ((checksummer->num_docs_with_checksum(checksum) > 1) || annotations_file_exists()) {
                if (marks.size() == 0 && bookmarks.size() == 0 && highlights.size() == 0 && portals.size() == 0 && study_objects.size() == 0 && region_highlights.size() == 0) {
                    db_manager->select_mark(checksum, marks);
                    db_manager->select_bookmark(checksum, bookmarks);
                    db_manager->select_highlight(checksum, highlights);
                    db_manager->select_links(checksum, portals);
                    db_manager->select_study_objects(checksum, study_objects);
                    db_manager->select_region_highlights(checksum, region_highlights);
                }
                // we already have a document with the same hash so there might be
                // annotations that are not loaded
                should_reload_annotations = true;
            }
            db_manager->insert_document_hash(get_path(), checksum);
            });
        checksum_thread.detach();
        //checksum_thread.join();
    }
}

std::string Document::add_study_object(const std::wstring& type,
    const std::wstring& title,
    const std::wstring& note,
    int page,
    float offset_x,
    float offset_y,
    float page_offset_y,
    float zoom_level,
    std::optional<AbsoluteDocumentPos> selection_begin,
    std::optional<AbsoluteDocumentPos> selection_end) {

    if (!is_valid_study_object_type(type) || title.empty()) {
        return "";
    }

    StudyObject study_object;
    study_object.id = new_uuid_utf8();
    study_object.document_checksum = get_checksum();
    study_object.document_path = get_path();
    study_object.type = type;
    study_object.title = title;
    study_object.note = note;
    study_object.page = page;
    study_object.offset_x = offset_x;
    study_object.offset_y = offset_y;
    study_object.page_offset_y = page_offset_y;
    study_object.zoom_level = zoom_level;
    study_object.selection_begin = selection_begin;
    study_object.selection_end = selection_end;
    study_object.created_at = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss").toStdString();
    study_object.updated_at = study_object.created_at;

    db_manager->insert_document_hash(get_path(), study_object.document_checksum);
    if (!db_manager->insert_study_object(study_object)) {
        return "";
    }

    study_objects.push_back(study_object);
    return study_object.id;
}


std::string Document::add_bookmark(const std::wstring& desc, float y_offset) {
    BookMark bookmark;
    bookmark.y_offset_ = y_offset;
    bookmark.description = desc;
    bookmark.uuid = new_uuid_utf8();
    bookmark.update_creation_time();
    bookmarks.push_back(bookmark);
    db_manager->insert_bookmark(get_checksum(), desc, y_offset, utf8_decode(bookmark.uuid));
    is_annotations_dirty = true;
    return bookmark.uuid;
}

std::string Document::add_marked_bookmark(const std::wstring& desc, AbsoluteDocumentPos pos) {
    BookMark bookmark;
    bookmark.description = desc;
    bookmark.y_offset_ = pos.y;
    bookmark.begin_x = pos.x;
    bookmark.begin_y = pos.y;
    bookmark.uuid = new_uuid_utf8();
    bookmark.update_creation_time();

    if (db_manager->insert_bookmark_marked(get_checksum(), desc, pos.x, pos.y, utf8_decode(bookmark.uuid))) {
        bookmarks.push_back(bookmark);
        is_annotations_dirty = true;
    }
    return bookmark.uuid;
}

int Document::add_incomplete_bookmark(BookMark incomplete_bookmark){
    incomplete_bookmark.uuid = new_uuid_utf8();
    bookmarks.push_back(incomplete_bookmark);
    return bookmarks.size() - 1;
}

std::string Document::add_pending_bookmark(int index, const std::wstring& desc) {
    BookMark& bookmark = bookmarks[index];
    bookmark.description = desc;
    bookmark.font_size = FREETEXT_BOOKMARK_FONT_SIZE;
    bookmark.update_creation_time();

    if (!db_manager->insert_bookmark_freetext(get_checksum(), bookmark)) {
        undo_pending_bookmark(index);
    }
    else {
        is_annotations_dirty = true;
    }
    return bookmark.uuid;
}

void Document::undo_pending_bookmark(int index) {
    if (index >= 0 && index < bookmarks.size()) {
        bookmarks.erase(bookmarks.begin() + index);
    }
}

void Document::add_freetext_bookmark_with_color(const std::wstring& desc, AbsoluteRect absrect, float* color, float font_size) {
    BookMark bookmark;
    bookmark.description = desc;
    bookmark.y_offset_ = absrect.y0;

    bookmark.begin_x = absrect.x0;
    bookmark.begin_y = absrect.y0;
    bookmark.end_x = absrect.x1;
    bookmark.end_y = absrect.y1;

    bookmark.color[0] = color[0];
    bookmark.color[1] = color[1];
    bookmark.color[2] = color[2];
    bookmark.font_size = font_size < 0 ? FREETEXT_BOOKMARK_FONT_SIZE : font_size;
    bookmark.uuid = new_uuid_utf8();
    bookmark.update_creation_time();

    if (db_manager->insert_bookmark_freetext(get_checksum(), bookmark)) {
        bookmarks.push_back(bookmark);
        is_annotations_dirty = true;
    }
}
void Document::add_freetext_bookmark(const std::wstring& desc, AbsoluteRect absrect) {
    add_freetext_bookmark_with_color(desc, absrect, FREETEXT_BOOKMARK_COLOR);
}

void Document::fill_index_highlight_rects(int highlight_index, fz_context* thread_context, fz_document* thread_document) {

    if (highlight_index >= highlights.size()) {
        return;
    }
    if (thread_context == nullptr) thread_context = context;
    if (thread_document == nullptr) thread_document = doc;

    const Highlight highlight = highlights[highlight_index];
    std::deque<AbsoluteRect> highlight_rects;
    std::vector<AbsoluteRect> merged_rects;
    std::wstring highlight_text;
    get_text_selection(thread_context, highlight.selection_begin, highlight.selection_end, !EXACT_HIGHLIGHT_SELECT, highlight_rects, highlight_text, thread_document);
    merge_selected_character_rects(highlight_rects, merged_rects, highlights[highlight_index].type != '_');
    highlights[highlight_index].highlight_rects = std::move((std::vector<AbsoluteRect>&)merged_rects);
}

void Document::fill_highlight_rects(fz_context* ctx, fz_document* doc_) {
    // Fill the `highlight_rects` attribute of all highlights with a vector of merged highlight rects of
    // individual characters in the highlighted area (we merge the rects of characters in a single line
    // which makes the result look nicer)

    for (size_t i = 0; i < highlights.size(); i++) {
        fill_index_highlight_rects(i, ctx, doc_);
    }
}


std::string Document::add_highlight(const std::wstring& annot, AbsoluteDocumentPos selection_begin, AbsoluteDocumentPos selection_end, char type) {
    std::deque<AbsoluteRect> selected_characters;
    std::vector<AbsoluteRect> merged_rects;
    std::wstring selected_text;
    get_text_selection(selection_begin, selection_end, true, selected_characters, selected_text);

    if ((type != '_') && (!((type <= 'z' && type >= 'a') || (type <= 'Z' && type >= 'A')))) {
        type = 'a';
    }

    merge_selected_character_rects<AbsoluteRect>(selected_characters, merged_rects);
    Highlight highlight;
    highlight.description = selected_text;
    highlight.text_annot = annot;
    highlight.selection_begin = selection_begin;
    highlight.selection_end = selection_end;
    highlight.type = type;
    highlight.highlight_rects = merged_rects;
    highlight.uuid = new_uuid_utf8();
    highlight.update_creation_time();

    if (db_manager->insert_highlight_with_annotation(
        get_checksum(),
        selected_text,
        annot,
        selection_begin.x,
        selection_begin.y,
        selection_end.x,
        selection_end.y,
        highlight.type,
        utf8_decode(highlight.uuid))) {
        highlights.push_back(highlight);
        is_annotations_dirty = true;
    }

    return highlight.uuid;
}

std::string Document::add_highlight(const std::wstring& desc,
    const std::vector<AbsoluteRect>& highlight_rects,
    AbsoluteDocumentPos selection_begin,
    AbsoluteDocumentPos selection_end,
    char type)
{
    //if (type > 'z' || type < 'a') {
    if ((type != '_') && (!((type <= 'z' && type >= 'a') || (type <= 'Z' && type >= 'A')))) {
        type = 'a';
    }

    Highlight highlight;
    highlight.description = desc;
    highlight.selection_begin = selection_begin;
    highlight.selection_end = selection_end;
    highlight.type = type;
    highlight.highlight_rects = highlight_rects;
    highlight.uuid = new_uuid_utf8();
    highlight.update_creation_time();

    highlights.push_back(highlight);
    db_manager->insert_highlight(
        get_checksum(),
        desc,
        selection_begin.x,
        selection_begin.y,
        selection_end.x,
        selection_end.y,
        highlight.type,
        utf8_decode(highlight.uuid));
    is_annotations_dirty = true;
    return highlight.uuid;
}

bool Document::get_is_indexing() {
    return is_indexing;
}

int Document::add_portal(Portal portal, bool insert_into_database) {
    portal.uuid = new_uuid_utf8();
    portal.update_creation_time();
    portals.push_back(portal);
    int index = portals.size() - 1;
    if (insert_into_database) {
        if (portal.is_visible()) {
            bool res = db_manager->insert_visible_portal(
                get_checksum(),
                portal.dst.document_checksum,
                portal.dst.book_state.offset_x,
                portal.dst.book_state.offset_y,
                portal.dst.book_state.zoom_level,
                portal.src_offset_x.value(),
                portal.src_offset_y,
                utf8_decode(portal.uuid));
        }
        else {
            db_manager->insert_portal(
                get_checksum(),
                portal.dst.document_checksum,
                portal.dst.book_state.offset_x,
                portal.dst.book_state.offset_y,
                portal.dst.book_state.zoom_level,
                portal.src_offset_y,
                utf8_decode(portal.uuid));
        }
        is_annotations_dirty = true;
    }
    return index;
}


std::wstring Document::get_path() {

    return file_name;
}

std::string Document::get_checksum() {

    return checksummer->get_checksum(get_path());
}

std::optional<std::string> Document::get_checksum_fast() {

    return checksummer->get_checksum_fast(get_path());
}

int Document::find_closest_bookmark_index(const std::vector<BookMark>& sorted_bookmarks, float to_offset_y) const {

    int min_index = argminf<BookMark>(sorted_bookmarks, [to_offset_y](BookMark bm) {
        return abs(bm.get_y_offset() - to_offset_y);
        });

    return min_index;
}

int Document::find_closest_portal_index(const std::vector<Portal>& sorted_portals, float to_offset_y) const {

    int min_index = argminf<Portal>(sorted_portals, [to_offset_y](Portal portal) {
        return abs(portal.src_offset_y - to_offset_y);
        });

    return min_index;
}

int Document::find_closest_highlight_index(const std::vector<Highlight>& sorted_highlights, float to_offset_y) const {

    int min_index = argminf<Highlight>(sorted_highlights, [to_offset_y](Highlight hl) {
        return abs(hl.selection_begin.y - to_offset_y);
        });

    return min_index;
}

void Document::delete_closest_bookmark(float to_y_offset) {
    int closest_index = find_closest_bookmark_index(bookmarks, to_y_offset);
    if (closest_index > -1) {
        db_manager->delete_bookmark(bookmarks[closest_index].uuid);
        is_annotations_dirty = true;
        bookmarks.erase(bookmarks.begin() + closest_index);
    }
}

void Document::delete_bookmark(int index) {

    if ((index != -1) && (index < bookmarks.size())) {
        if (db_manager->delete_bookmark(bookmarks[index].uuid)) {
            bookmarks.erase(bookmarks.begin() + index);
            is_annotations_dirty = true;
        }
    }
}

void Document::delete_highlight_with_index(int index) {
    Highlight highlight_to_delete = highlights[index];

    db_manager->delete_highlight(highlight_to_delete.uuid);
    highlights.erase(highlights.begin() + index);
    is_annotations_dirty = true;
}

void Document::delete_bookmark_with_index(int index) {
    BookMark bookmark_to_delete = bookmarks[index];

    db_manager->delete_bookmark(bookmark_to_delete.uuid);
    bookmarks.erase(bookmarks.begin() + index);
    is_annotations_dirty = true;
}

void Document::delete_highlight(Highlight hl) {
    for (size_t i = (highlights.size() - 1); i >= 0; i--) {
        if (highlights[i] == hl) {
            delete_highlight_with_index(i);
            return;
        }
    }
}

std::optional<Portal> Document::find_closest_portal(float to_offset_y, int* index) {
    int min_index = argminf<Portal>(portals, [to_offset_y](Portal l) {
        return abs(l.src_offset_y - to_offset_y);
        });

    if (min_index >= 0) {
        if (index) *index = min_index;
        return portals[min_index];
    }
    return {};
}

bool Document::update_portal(Portal new_portal) {
    for (auto& portal : portals) {
        if (portal.src_offset_y == new_portal.src_offset_y) {
            portal.dst.book_state = new_portal.dst.book_state;
            return true;
        }
    }
    return false;
}

void Document::delete_closest_portal(float to_offset_y) {
    int closest_index = -1;
    if (find_closest_portal(to_offset_y, &closest_index)) {
        db_manager->delete_portal(portals[closest_index].uuid);
        portals.erase(portals.begin() + closest_index);
        is_annotations_dirty = true;
    }
}

int Document::get_portal_index_with_uuid(const std::string& uuid) {
    for (int i = 0; i < portals.size(); i++) {
        if (portals[i].uuid == uuid) {
            return i;
        }
    }
    return -1;
}

void Document::delete_portal_with_uuid(const std::string& uuid) {
    int index = get_portal_index_with_uuid(uuid);
    if (index > -1) {
        db_manager->delete_portal(uuid);
        portals.erase(portals.begin() + index);
    }
}

std::vector<BookMark>& Document::get_bookmarks() {
    return bookmarks;
}

std::vector<BookMark> Document::get_sorted_bookmarks() const {
    std::vector<BookMark> res = bookmarks;
    std::sort(res.begin(), res.end(), [](const BookMark& lhs, const BookMark& rhs) {return lhs.get_y_offset() < rhs.get_y_offset(); });
    return res;
}

std::vector<Portal> Document::get_sorted_portals() const {
    std::vector<Portal> res = portals;
    std::sort(res.begin(), res.end(), [](const Portal& lhs, const Portal& rhs) {return lhs.src_offset_y < rhs.src_offset_y; });
    return res;
}

const std::vector<Highlight>& Document::get_highlights() const {
    return highlights;
}

const std::vector<StudyObject>& Document::get_study_objects() const {
    return study_objects;
}

std::vector<StudyObject> Document::get_study_objects_sorted() const {
    std::vector<StudyObject> res = study_objects;
    std::sort(res.begin(), res.end(), [](const StudyObject& lhs, const StudyObject& rhs) {
        if (lhs.page != rhs.page) {
            return lhs.page < rhs.page;
        }
        return lhs.offset_y < rhs.offset_y;
        });
    return res;
}

int Document::get_study_object_index_with_id(const std::string& id) const {
    for (int i = 0; i < study_objects.size(); i++) {
        if (study_objects[i].id == id) {
            return i;
        }
    }
    return -1;
}

int Document::find_closest_study_object_index(const std::vector<StudyObject>& sorted_study_objects, float to_offset_y) const {
    return argminf<StudyObject>(sorted_study_objects, [to_offset_y](StudyObject study_object) {
        return abs(study_object.offset_y - to_offset_y);
        });
}

bool Document::update_study_object_title(const std::string& id, const std::wstring& new_title) {
    if (new_title.empty()) {
        return false;
    }
    int index = get_study_object_index_with_id(id);
    if (index < 0) {
        return false;
    }
    if (!db_manager->update_study_object_title(id, new_title)) {
        return false;
    }
    study_objects[index].title = new_title;
    study_objects[index].updated_at = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss").toStdString();
    return true;
}

bool Document::update_study_object_type(const std::string& id, const std::wstring& new_type) {
    if (!is_valid_study_object_type(new_type)) {
        return false;
    }
    int index = get_study_object_index_with_id(id);
    if (index < 0) {
        return false;
    }
    if (!db_manager->update_study_object_type(id, new_type)) {
        return false;
    }
    study_objects[index].type = new_type;
    study_objects[index].updated_at = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss").toStdString();
    return true;
}

bool Document::delete_study_object(const std::string& id) {
    int index = get_study_object_index_with_id(id);
    if (index < 0) {
        return false;
    }
    if (!db_manager->delete_study_object(id)) {
        return false;
    }
    study_objects.erase(study_objects.begin() + index);
    return true;
}

static PagelessDocumentRect normalized_pageless_rect(PagelessDocumentRect rect) {
    if (rect.x0 > rect.x1) {
        std::swap(rect.x0, rect.x1);
    }
    if (rect.y0 > rect.y1) {
        std::swap(rect.y0, rect.y1);
    }
    return rect;
}

std::string Document::add_region_highlight(DocumentRect rect,
    const std::wstring& title,
    const std::wstring& note,
    const std::wstring& type) {

    rect.rect = normalized_pageless_rect(rect.rect);
    if (rect.page < 0 || rect.page >= num_pages()) {
        return "";
    }
    if (rect.rect.width() <= 0.0f || rect.rect.height() <= 0.0f) {
        return "";
    }

    RegionHighlight region_highlight;
    region_highlight.id = new_uuid_utf8();
    region_highlight.document_checksum = get_checksum();
    region_highlight.document_path = get_path();
    region_highlight.page = rect.page;
    region_highlight.rect = rect.rect;
    region_highlight.title = title;
    region_highlight.note = note;
    region_highlight.type = type.empty() ? L"region" : type;
    region_highlight.created_at = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss").toStdString();
    region_highlight.updated_at = region_highlight.created_at;

    db_manager->insert_document_hash(get_path(), region_highlight.document_checksum);
    if (!db_manager->insert_region_highlight(region_highlight)) {
        return "";
    }

    region_highlights.push_back(region_highlight);
    return region_highlight.id;
}

const std::vector<RegionHighlight>& Document::get_region_highlights() const {
    return region_highlights;
}

std::vector<RegionHighlight> Document::get_region_highlights_sorted() const {
    std::vector<RegionHighlight> res = region_highlights;
    std::sort(res.begin(), res.end(), [](const RegionHighlight& lhs, const RegionHighlight& rhs) {
        if (lhs.page != rhs.page) {
            return lhs.page < rhs.page;
        }
        if (lhs.rect.y0 != rhs.rect.y0) {
            return lhs.rect.y0 < rhs.rect.y0;
        }
        return lhs.rect.x0 < rhs.rect.x0;
        });
    return res;
}

int Document::get_region_highlight_index_with_id(const std::string& id) const {
    for (int i = 0; i < region_highlights.size(); i++) {
        if (region_highlights[i].id == id) {
            return i;
        }
    }
    return -1;
}

int Document::find_closest_region_highlight_index(const std::vector<RegionHighlight>& sorted_region_highlights, float to_offset_y) {
    return argminf<RegionHighlight>(sorted_region_highlights, [this, to_offset_y](RegionHighlight region_highlight) {
        float region_center_y = (region_highlight.rect.y0 + region_highlight.rect.y1) / 2.0f;
        return abs(document_to_absolute_y(region_highlight.page, region_center_y) - to_offset_y);
        });
}

bool Document::update_region_highlight_title(const std::string& id, const std::wstring& new_title) {
    int index = get_region_highlight_index_with_id(id);
    if (index < 0) {
        return false;
    }
    if (!db_manager->update_region_highlight_title(id, new_title)) {
        return false;
    }
    region_highlights[index].title = new_title;
    region_highlights[index].updated_at = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss").toStdString();
    return true;
}

bool Document::delete_region_highlight(const std::string& id) {
    int index = get_region_highlight_index_with_id(id);
    if (index < 0) {
        return false;
    }
    if (!db_manager->delete_region_highlight(id)) {
        return false;
    }
    region_highlights.erase(region_highlights.begin() + index);
    return true;
}

const std::vector<Highlight> Document::get_highlights_of_type(char type) const {
    std::vector<Highlight> res;

    for (auto hl : highlights) {
        if (hl.type == type) {
            res.push_back(hl);
        }
    }
    return res;
}

const std::vector<Highlight> Document::get_highlights_sorted(char type) const {
    std::vector<Highlight> res;

    if (type == 0) {
        res = highlights;
    }
    else {
        res = get_highlights_of_type(type);
    }

    std::sort(res.begin(), res.end(), [](const Highlight& hl1, const Highlight& hl2) {
        return hl1.selection_begin.y < hl2.selection_begin.y;
        });

    return res;
}


void Document::add_mark(char symbol, float y_offset, std::optional<float> x_offset, std::optional<float> zoom_level) {
    int current_mark_index = get_mark_index(symbol);
    if (current_mark_index == -1) {
        Mark m;
        m.y_offset = y_offset;
        m.symbol = symbol;
        m.update_creation_time();
        m.x_offset = x_offset;
        m.zoom_level = zoom_level;
        marks.push_back(m);
        db_manager->insert_mark(get_checksum(), symbol, y_offset, new_uuid(), x_offset, zoom_level);
        is_annotations_dirty = true;
    }
    else {
        marks[current_mark_index].y_offset = y_offset;
        marks[current_mark_index].x_offset = x_offset;
        marks[current_mark_index].zoom_level = zoom_level;
        marks[current_mark_index].update_modification_time();
        db_manager->update_mark(get_checksum(), symbol, y_offset, x_offset, zoom_level);
        is_annotations_dirty = true;
    }
}

bool Document::remove_mark(char symbol) {
    for (size_t i = 0; i < marks.size(); i++) {
        if (marks[i].symbol == symbol) {
            marks.erase(marks.begin() + i);
            return true;
        }
    }
    return false;
}

std::optional<Mark> Document::get_mark_if_exists(char symbol){
    int mark_index = get_mark_index(symbol);
    if (mark_index == -1) {
        return {};
    }
    return marks[mark_index];
}

Document::Document(fz_context* context, std::wstring file_name, DatabaseManager* db, CachedChecksummer* checksummer) :
    db_manager(db),
    context(context),
    file_name(file_name),
    checksummer(checksummer),
    doc(nullptr) {
    last_update_time = QDateTime::currentDateTime();
    should_render_annotations = SHOULD_RENDER_PDF_ANNOTATIONS;
}

void Document::count_chapter_pages(std::vector<int>& page_counts) {
    int num_chapters = fz_count_chapters(context, doc);

    for (int i = 0; i < num_chapters; i++) {
        int num_pages = fz_count_chapter_pages(context, doc, i);
        page_counts.push_back(num_pages);
    }
}

void Document::count_chapter_pages_accum(std::vector<int>& accum_page_counts) {
    std::vector<int> raw_page_count;
    count_chapter_pages(raw_page_count);

    int accum = 0;

    for (size_t i = 0; i < raw_page_count.size(); i++) {
        accum_page_counts.push_back(accum);
        accum += raw_page_count[i];
    }
}

const std::vector<TocNode*>& Document::get_toc() {
    if (created_top_level_toc_nodes.size() > 0 && (should_use_created_toc || top_level_toc_nodes.size() == 0)) {
        return created_top_level_toc_nodes;
    }
    else {
        return top_level_toc_nodes;
    }
}

bool Document::has_toc() {
    return top_level_toc_nodes.size() > 0 || created_top_level_toc_nodes.size() > 0;
}

const std::vector<std::wstring>& Document::get_flat_toc_names() {
    return flat_toc_names;
}

const std::vector<int>& Document::get_flat_toc_pages() {
    return flat_toc_pages;
}

float Document::get_page_height(int page_index) {
    std::lock_guard guard(page_dims_mutex);
    if ((page_index >= 0) && (page_index < page_heights.size())) {
        return page_heights[page_index];
    }
    else {
        return -1.0f;
    }
}

float Document::get_page_width(int page_index) {
    if ((page_index >= 0) && (page_index < page_widths.size())) {
        return page_widths[page_index];
    }
    else {
        return -1.0f;
    }
}

float Document::get_page_width_median() {
    if (page_widths.size() == 0) {
        return 500.0f;
    }

    std::vector<float> sorted_widths = page_widths;
    std::nth_element(sorted_widths.begin(), sorted_widths.begin() + sorted_widths.size() / 2, sorted_widths.end());
    return sorted_widths[sorted_widths.size() / 2];
}

float Document::get_page_size_smart(bool width, int page_index, float* left_ratio, float* right_ratio, int* normal_page_width) {

    fz_pixmap* pixmap = get_small_pixmap(page_index);

    // project the small pixmap into a histogram (the `width` parameter determines whether we do this
    // horizontally or vertically)
    std::vector<float> histogram;

    if (width) {
        histogram.resize(pixmap->w);
    }
    else {
        histogram.resize(pixmap->h);
    }

    float total_nonzero = 0.0f;
    for (int i = 0; i < pixmap->w; i++) {
        for (int j = 0; j < pixmap->h; j++) {
            int index = 3 * pixmap->w * j + 3 * i;

            int r = pixmap->samples[index];
            int g = pixmap->samples[index + 1];
            int b = pixmap->samples[index + 2];

            float brightness = static_cast<float>(r + g + b) / 3.0f;
            if (brightness < 250) {
                if (width) {
                    histogram[i] += 1;
                }
                else {
                    histogram[j] += 1;
                }
                total_nonzero += 1;
            }
        }
    }

    float average_nonzero = total_nonzero / histogram.size();
    float nonzero_threshold = average_nonzero / 3.0f;

    int start_index = 0;
    int end_index = histogram.size() - 1;

    // find the first index in both directions where the histogram is nonzero
    while ((start_index < end_index) && (histogram[start_index++] < nonzero_threshold));
    while ((start_index < end_index) && (histogram[end_index--] < nonzero_threshold));


    int border = 10;
    start_index = std::max<int>(start_index - border, 0);
    end_index = std::min<int>(end_index + border, histogram.size() - 1);

    *left_ratio = static_cast<float>(start_index) / histogram.size();
    *right_ratio = static_cast<float>(end_index) / histogram.size();

    float standard_size;
    if (width) {
        standard_size = page_widths[page_index];
    }
    else {
        standard_size = page_heights[page_index];
    }

    float ratio = static_cast<float>(end_index - start_index) / histogram.size();

    *normal_page_width = standard_size;

    return ratio * standard_size;
}

float Document::get_accum_page_height(int page_index) {
    std::lock_guard guard(page_dims_mutex);
    if (page_index < 0 || (page_index >= accum_page_heights.size())) {
        return 0.0f;
    }
    return accum_page_heights[page_index];
}


fz_outline* Document::get_toc_outline() {
    fz_outline* res = nullptr;
    fz_try(context) {
        res = fz_load_outline(context, doc);
    }
    fz_catch(context) {
        //std::cerr << "Error: Could not load outline ... " << std::endl;
    }
    return res;
}

void Document::create_toc_tree(std::vector<TocNode*>& toc) {
    fz_try(context) {
        fz_outline* outline = get_toc_outline();
        if (outline) {
            convert_toc_tree(outline, toc);
            fz_drop_outline(context, outline);
        }
        else {
            //create_table_of_contents(toc);
        }
    }
    fz_catch(context) {
        //std::cerr << "Error: Could not load outline ... " << std::endl;
    }
}

void Document::convert_toc_tree(fz_outline* root, std::vector<TocNode*>& output) {
    // convert an fz_outline structure to a tree of TocNodes

    try {
        std::vector<int> accum_chapter_pages;
        count_chapter_pages_accum(accum_chapter_pages);
        do {
            if (root == nullptr || root->title == nullptr) {
                break;
            }

                TocNode* current_node = new TocNode;
                current_node->title = utf8_decode(root->title);
                current_node->x = root->x;
                current_node->y = root->y;
            if (root->page.page == -1) {
                float xp, yp;
                fz_location loc = fz_resolve_link(context, doc, root->uri, &xp, &yp);
                int chapter_page = 0;
                if (loc.chapter >= 0 && loc.chapter < accum_chapter_pages.size()) {
                    chapter_page = accum_chapter_pages[loc.chapter];
                }
                current_node->page = chapter_page + loc.page;
            }
            else {
                current_node->page = root->page.page;
            }
            convert_toc_tree(root->down, current_node->children);


            output.push_back(current_node);
        } while ((root = root->next));
    }
    catch(utf8::exception& e) {
        std::cerr << "could not convert toc node: " << e.what() << std::endl;
    }
}

PdfLink Document::merge_links(const std::vector<PdfLink>& links_to_merge) {
    PdfLink merged_link;
    merged_link.uri = links_to_merge[0].uri;
    merged_link.source_page = links_to_merge[0].source_page;
    std::vector<PagelessDocumentRect> rects;
    PagelessDocumentRect current_rect = links_to_merge[0].rects[0];
    for (int i = 1; i < links_to_merge.size(); i++) {
        PagelessDocumentRect new_rect = links_to_merge[i].rects[0];
        float height = std::abs(current_rect.y1 - current_rect.y0);
        if (std::abs(current_rect.y1 - new_rect.y1) < height / 5) {
            current_rect.x0 = std::min(current_rect.x0, new_rect.x0);
            current_rect.x1 = std::max(current_rect.x1, new_rect.x1);
        }
        else {
            rects.push_back(current_rect);
            current_rect = new_rect;
        }
    }
    rects.push_back(current_rect);
    merged_link.rects = rects;
    return merged_link;
}

const std::vector<PdfLink>& Document::get_page_merged_pdf_links(int page_number) {
    if (cached_merged_pdf_links.find(page_number) != cached_merged_pdf_links.end()) {
        return cached_merged_pdf_links[page_number];
    }

    std::vector<PdfLink> res;

    std::vector<PdfLink> links_to_merge;

    fz_link* current_link = get_page_links(page_number);

    auto should_add_link = [&]() {
        if (links_to_merge.size() == 0) {
            return false;
        }
        if (links_to_merge.back().uri == current_link->uri) {
            if (links_to_merge.back().rects.size() > 0) {
                if (std::abs(links_to_merge.back().rects[0].y1 - current_link->rect.y1) > 2 * links_to_merge.back().rects[0].height()) {
                    return true;
                }
            }
            return false;
        }
        else {
            return true;
        }

        };

    while (current_link) {
        if (should_add_link()) {
            PdfLink merged_link = merge_links(links_to_merge);
            //merged_link.uri = links_to_merge[0].uri;
            //merged_link.source_page = links_to_merge[0].source_page;
            //for (int i = 0; i < links_to_merge.size(); i++) {
            //    merged_link.rects.push_back(links_to_merge[i].rects[0]);
            //}
            res.push_back(merged_link);
            links_to_merge.clear();
        }
        links_to_merge.push_back(pdf_link_from_fz_link(page_number, current_link));

        current_link = current_link->next;
    }

    if (links_to_merge.size() > 0) {
        res.push_back(merge_links(links_to_merge));
    }

    auto generated_it = generated_page_links.find(page_number);
    if (generated_it != generated_page_links.end()) {
        for (const auto& generated_link : generated_it->second) {
            bool overlaps_existing_link = false;
            for (const auto& existing_link : res) {
                for (const auto& existing_rect : existing_link.rects) {
                    for (const auto& generated_rect : generated_link.rects) {
                        if (rects_intersect(existing_rect, generated_rect)) {
                            overlaps_existing_link = true;
                            break;
                        }
                    }
                    if (overlaps_existing_link) {
                        break;
                    }
                }
                if (overlaps_existing_link) {
                    break;
                }
            }
            if (!overlaps_existing_link) {
                res.push_back(generated_link);
            }
        }
    }

    cached_merged_pdf_links[page_number] = res;
    return cached_merged_pdf_links[page_number];
}

fz_link* Document::get_page_links(int page_number) {
    if (cached_page_links.find(page_number) != cached_page_links.end()) {
        return cached_page_links.at(page_number);
    }

    fz_link* res = nullptr;
    fz_try(context) {
        fz_page* page = fz_load_page(context, doc, page_number);
        res = fz_load_links(context, page);
        cached_page_links[page_number] = res;
        fz_drop_page(context, page);
    }

    fz_catch(context) {
        LOG(std::cerr << "Error: Could not load links" << std::endl);
        res = nullptr;
    }
    return res;
}

QDateTime Document::get_last_edit_time() {
    QFileInfo info(QString::fromStdWString(get_path()));
    return info.lastModified();
}

unsigned int Document::get_milies_since_last_document_update_time() {
    QDateTime now = QDateTime::currentDateTime();
    return last_update_time.msecsTo(now);

}

unsigned int Document::get_milies_since_last_edit_time() {
    QDateTime last_modified_time = get_last_edit_time();
    QDateTime now = QDateTime::currentDateTime();
    return last_modified_time.msecsTo(now);
}

Document::~Document() {
    if (document_indexing_thread.has_value()) {
        stop_indexing();
        document_indexing_thread.value().join();
    }

    if (doc != nullptr) {
        fz_try(context) {

            for (auto [_, stext_page] : cached_stext_pages) {
                fz_drop_stext_page(context, stext_page);
            }

            for (auto [_, small_pixmap] : cached_small_pixmaps) {
                fz_drop_pixmap(context, small_pixmap);
            }

            for (auto [_, link] : cached_page_links) {
                fz_drop_link(context, link);
            }

            fz_drop_document(context, doc);
        }
        fz_catch(context) {
            LOG(std::cerr << "Error: could not drop document" << std::endl);
        }
    }
}
void Document::reload(std::string password) {

    fz_drop_document(context, doc);
    clear_document_caches();

    doc = nullptr;

    open(invalid_flag_pointer, false, password);
}

bool Document::open(bool* invalid_flag, bool force_load_dimensions, std::string password, bool temp) {
    load_extras();

    last_update_time = QDateTime::currentDateTime();
    if (doc == nullptr) {
        fz_try(context) {
            //			doc = fz_open_document(context, utf8_encode(file_name).c_str());
            doc = open_document_with_file_name(context, file_name);
            document_needs_password = fz_needs_password(context, doc);
            if (password.size() > 0) {
                int auth_res = fz_authenticate_password(context, doc, password.c_str());
                if (auth_res > 0) {
                    password_was_correct = true;
                }
            }
            //fz_layout_document(context, doc, 600, 800, 9);
        }
        fz_catch(context) {
            LOG(std::wcerr << "could not open " << file_name << std::endl);
        }
        if ((doc != nullptr) && (!temp)) {
            //load_document_metadata_from_db();
            load_document_caches(invalid_flag, force_load_dimensions);
            return true;
        }


        return false;
    }
    else {
        //std::cerr << "warning! calling open() on an open document" << std::endl;
        return true;
    }
}


//const vector<float>& get_page_heights();
//const vector<float>& get_page_widths();
//const vector<float>& get_accum_page_heights();

void Document::get_visible_pages(float doc_y_range_begin, float doc_y_range_end, std::vector<int>& visible_pages) {

    std::lock_guard guard(page_dims_mutex);

    if (page_heights.size() == 0) {
        return;
    }

    float page_begin = 0.0f;

    for (size_t i = 0; i < page_heights.size(); i++) {
        float page_end = page_begin + page_heights[i];

        if (range_intersects(doc_y_range_begin, doc_y_range_end, page_begin, page_end)) {
            visible_pages.push_back(i);
        }
        page_begin = page_end;
    }
}

void Document::load_page_dimensions(bool force_load_now) {
    page_heights.clear();
    accum_page_heights.clear();
    page_widths.clear();
    page_labels.clear();

    int n = num_pages();
    float acc_height = 0.0f;
    // initially assume all pages have the same dimensions, correct these heights
    // when the background thread is done
    if (n > 0) {
        fz_try(context) {
            fz_page* page = fz_load_page(context, doc, n / 2);
            PagelessDocumentRect bounds = fz_bound_page(context, page);
            fz_drop_page(context, page);
            int height = bounds.y1 - bounds.y0;
            int width = bounds.x1 - bounds.x0;

            for (int i = 0; i < n; i++) {
                page_heights.push_back(height);
                accum_page_heights.push_back(acc_height);
                page_widths.push_back(width);
                acc_height += height;
            }
        }
        fz_catch(context) {
            std::wcout << L"Error: could not load sample page dimensions\n";
        }
    }

    auto load_page_dimensions_function = [this, n, force_load_now]() {
        std::vector<float> accum_page_heights_;
        std::vector<float> page_heights_;
        std::vector<float> page_widths_;
        std::vector<std::wstring> page_labels_;
        const int N = 20;
        char label_buffer[N];

        // clone the main context for use in the background thread
        fz_context* context_ = fz_clone_context(context);
        fz_try(context_) {
            //			fz_document* doc_ = fz_open_document(context_, utf8_encode(file_name).c_str());
            fz_document* doc_ = open_document_with_file_name(context_, file_name);
            //fz_layout_document(context_, doc, 600, 800, 20);
            load_document_metadata_from_db();

            float acc_height_ = 0.0f;
            for (int i = 0; i < n; i++) {
                fz_page* page = fz_load_page(context_, doc_, i);
                fz_page_label(context_, page, label_buffer, N);
                page_labels_.push_back(utf8_decode(label_buffer));
                PagelessDocumentRect page_rect = fz_bound_page(context_, page);

                float page_height = page_rect.y1 - page_rect.y0;
                float page_width = page_rect.x1 - page_rect.x0;

                accum_page_heights_.push_back(acc_height_);
                page_heights_.push_back(page_height);
                page_widths_.push_back(page_width);
                acc_height_ += page_height;

                fz_drop_page(context_, page);
            }


            page_dims_mutex.lock();

            page_heights = std::move(page_heights_);
            accum_page_heights = std::move(accum_page_heights_);
            page_widths = std::move(page_widths_);
            page_labels = std::move(page_labels_);

            if (invalid_flag_pointer) {
                *invalid_flag_pointer = true;
            }
            page_dims_mutex.unlock();

            fill_highlight_rects(context_, doc_);
            detected_paper_name = detect_paper_name(context_, doc_);
            //db_manager->set_actual_document_name(get_checksum(), detected_paper_name);

            fz_drop_document(context_, doc_);
        }
        fz_catch(context_) {
            std::wcout << L"Error: could not load page dimensions\n";
        }

        fz_drop_context(context_);

        are_highlights_loaded = true;
        if (invalid_flag_pointer) {
            *invalid_flag_pointer = true;
        }

    };

    if (force_load_now) {
        load_page_dimensions_function();
    }
    else {
        auto background_page_dimensions_loading_thread = std::thread(load_page_dimensions_function);
        background_page_dimensions_loading_thread.detach();
    }
    load_drawings_async();
}


AbsoluteRect Document::get_page_absolute_rect(int page) {
    std::lock_guard guard(page_dims_mutex);

    AbsoluteRect res({0, 0, 1, 1});

    if (page >= page_widths.size()) {
        return res;
    }

    res.x0 = -page_widths[page] / 2;
    res.x1 = page_widths[page] / 2;

    res.y0 = accum_page_heights[page];
    res.y1 = accum_page_heights[page] + page_heights[page];
    return res;
}

int Document::num_pages() {
    if (cached_num_pages.has_value()) {
        return cached_num_pages.value();
    }

    int pages = -1;
    fz_try(context) {
        pages = fz_count_pages(context, doc);
        cached_num_pages = pages;
    }
    fz_catch(context) {
        LOG(std::cerr << "could not count pages" << std::endl);
    }
    return pages;
}

DocumentManager::DocumentManager(fz_context* mupdf_context, DatabaseManager* db, CachedChecksummer* checksummer) :
    mupdf_context(mupdf_context),
    db_manager(db),
    checksummer(checksummer)
{
    //get_prev_path_hash_pairs(database, const std::string& path, std::vector<std::pair<std::wstring, std::wstring>>& out_pairs);
}


Document* DocumentManager::get_document(const std::wstring& path) {
    if (cached_documents.find(path) != cached_documents.end()) {
        return cached_documents.at(path);
    }
    Document* new_doc = new Document(mupdf_context, path, db_manager, checksummer);
    cached_documents[path] = new_doc;
    return new_doc;
}

const std::unordered_map<std::wstring, Document*>& DocumentManager::get_cached_documents() {
    return cached_documents;
}

void DocumentManager::delete_global_mark(char symbol) {
    for (auto [path, doc] : cached_documents) {
        doc->remove_mark(symbol);
    }
}


fz_stext_page* Document::get_stext_with_page_number(int page_number) {
    return get_stext_with_page_number(context, page_number);
}

fz_stext_page* Document::get_stext_with_page_number(fz_context* ctx, int page_number, fz_document* doc_) {
    // shouldn't cache when the request is coming from the worker thread
    // as the stext pages generated in the other thread is not usable in
    // main thread's context
    bool nocache = false;
    if (doc_ == nullptr) {
        doc_ = doc;
    }
    else {
        if (doc_ != doc) {
            nocache = true;
        }
    }

    const int MAX_CACHED_STEXT_PAGES = 10;

    if (!nocache) {
        for (auto [page, cached_stext_page] : cached_stext_pages) {
            if (page == page_number) {
                return cached_stext_page;
            }
        }
    }

    fz_stext_page* stext_page = nullptr;

    bool failed = false;
    if (!doc_) {
        return nullptr;
    }

    fz_try(ctx) {
        fz_stext_options options;
        options.flags = FZ_STEXT_PRESERVE_IMAGES;
        options.scale = 0.0f;
        stext_page = fz_new_stext_page_from_page_number(ctx, doc_, page_number, &options);
    }
    fz_catch(ctx) {
        failed = true;
    }
    if (failed) {
        return nullptr;
    }

    if (stext_page != nullptr) {

        if (!nocache) {
            if (cached_stext_pages.size() == MAX_CACHED_STEXT_PAGES) {
                fz_drop_stext_page(ctx, cached_stext_pages[0].second);
                cached_stext_pages.erase(cached_stext_pages.begin());
            }
            cached_stext_pages.push_back(std::make_pair(page_number, stext_page));
        }
        return stext_page;
    }

    return nullptr;
}

int Document::get_page_offset() {
    return page_offset;
}

void Document::set_page_offset(int new_offset) {
    page_offset = new_offset;
}

DocumentRect Document::absolute_to_page_rect(AbsoluteRect abs_rect) {
    DocumentPos top_left = abs_rect.top_left().to_document(this);
    DocumentPos bottom_right = abs_rect.bottom_right().to_document(this);
    return DocumentRect(top_left, bottom_right, top_left.page);
}

DocumentPos Document::absolute_to_page_pos(AbsoluteDocumentPos absp) {

    std::lock_guard guard(page_dims_mutex);
    if (accum_page_heights.size() == 0) {
        return { 0, 0.0f, 0.0f };
    }

    int i = (std::lower_bound(
        accum_page_heights.begin(),
        accum_page_heights.end(), absp.y) - accum_page_heights.begin()) - 1;
    i = std::max(0, i);

    if (i < accum_page_heights.size()) {
        float acc_page_heights_i = accum_page_heights[i];
        float page_width = page_widths[i];
        float remaining_y = absp.y - acc_page_heights_i;

        return { i, page_width / 2 + absp.x, remaining_y };
    }
    else {
        return { -1, 0.0f, 0.0f };
    }
}

DocumentPos Document::absolute_to_page_pos_uncentered(AbsoluteDocumentPos absolute_pos) {
    return absolute_to_page_pos(absolute_pos);
}

QStandardItemModel* Document::get_toc_model() {
    if (cached_toc_model && cached_toc_model_generation != toc_model_generation) {
        delete cached_toc_model;
        cached_toc_model = nullptr;
    }
    if (!cached_toc_model) {
        cached_toc_model = get_model_from_toc(get_toc());
        cached_toc_model_generation = toc_model_generation;
    }
    return cached_toc_model;
}


int Document::get_offset_page_number(float y_offset) {
    std::lock_guard guard(page_dims_mutex);

    if (accum_page_heights.size() == 0) {
        return -1;
    }

    auto it = std::lower_bound(accum_page_heights.begin(), accum_page_heights.end(), y_offset);

    // std::lower_bound returns an iterator pointing to the first element of the vector not less than y_offset,
    // but we are looking for the last element of vector that is less than y_offset
    if (it > accum_page_heights.begin()) {
        it--;
    }

    return (it - accum_page_heights.begin());
}

void Document::index_document(bool* invalid_flag) {
    int n = num_pages();

    if (this->document_indexing_thread.has_value()) {
        // if we are already indexing figures, we should wait for the previous thread to finish
        this->document_indexing_thread.value().join();
    }

    is_document_indexing_required = true;
    is_indexing = true;

    this->document_indexing_thread = std::thread([this, n, invalid_flag]() {
        std::vector<IndexedData> local_generic_data;
        std::map<std::wstring, IndexedData> local_reference_data;
        std::map<std::wstring, std::vector<IndexedData>> local_equation_data;

        std::wstring local_super_fast_search_index;
        std::vector<int> local_page_begin_indices;

        const bool native_toc_is_scrappy = toc_is_scrappy(top_level_toc_nodes, n);
        const bool should_build_created_toc = CREATE_TABLE_OF_CONTENTS_IF_NOT_EXISTS && (native_toc_is_scrappy || n >= 20);
        const int printed_toc_scan_limit = std::min(n, std::max(24, std::min(90, n / 5 + 20)));
        const size_t heading_candidate_limit = static_cast<size_t>(std::max(MAX_CREATED_TABLE_OF_CONTENTS_SIZE * 8, 2000));
        bool printed_toc_started = false;
        int last_printed_toc_page = -1;
        std::vector<PrintedTocCandidate> printed_toc_candidates;
        std::vector<CreatedTocCandidate> heading_toc_candidates;
        std::vector<std::wstring> indexed_page_labels;
        std::map<int, std::vector<PdfLink>> local_generated_page_links;
        if (should_build_created_toc) {
            indexed_page_labels.reserve(n);
        }

        fz_context* context_ = fz_clone_context(context);
        fz_try(context_) {

            //			fz_document* doc_ = fz_open_document(context_, utf8_encode(file_name).c_str());
            fz_document* doc_ = open_document_with_file_name(context_, file_name);

            if (document_needs_password) {
                fz_authenticate_password(context_, doc_, correct_password.c_str());
            }
            for (int i = 0; i < n; i++) {
                // when we close a document before its indexing is finished, we should stop indexing as soon as posible
                if (!is_document_indexing_required) {
                    break;
                }

                // we don't use get_stext_with_page_number here on purpose because it would lead to many unnecessary allocations
                fz_stext_page* stext_page = fz_new_stext_page_from_page_number(context_, doc_, i, nullptr);

                std::vector<fz_stext_char*> flat_chars;
                get_flat_chars_from_stext_page(stext_page, flat_chars);

                if (SUPER_FAST_SEARCH) {
                    flat_char_prism2(flat_chars, i, local_super_fast_search_index, local_page_begin_indices);
                }

                index_references(stext_page, i, local_reference_data);
                index_equations(flat_chars, i, local_equation_data);
                index_generic(flat_chars, i, local_generic_data);

                if (should_build_created_toc) {
                    const int label_buffer_size = 20;
                    char label_buffer[label_buffer_size];
                    label_buffer[0] = '\0';
                    fz_page* label_page = fz_load_page(context_, doc_, i);
                    fz_page_label(context_, label_page, label_buffer, label_buffer_size);
                    fz_drop_page(context_, label_page);

                    std::wstring current_page_label = utf8_decode(label_buffer);
                    if (current_page_label.empty()) {
                        current_page_label = QString::number(i + 1).toStdWString();
                    }
                    indexed_page_labels.push_back(current_page_label);

                    std::vector<TocTextLine> page_lines = toc_extract_text_lines(stext_page);
                    float page_height = stext_page->mediabox.y1 - stext_page->mediabox.y0;
                    if (page_height <= 0.0f) {
                        page_height = 800.0f;
                    }

                    if (heading_toc_candidates.size() < heading_candidate_limit) {
                        std::vector<CreatedTocCandidate> page_heading_candidates = toc_detect_heading_candidates(page_lines, i, page_height);
                        heading_toc_candidates.insert(
                            heading_toc_candidates.end(),
                            page_heading_candidates.begin(),
                            page_heading_candidates.end());
                    }

                    bool should_try_printed_toc =
                        i < printed_toc_scan_limit ||
                        (printed_toc_started && i <= last_printed_toc_page + 2);

                    if (should_try_printed_toc) {
                        bool page_looks_like_toc = false;
                        std::vector<PrintedTocCandidate> page_printed_candidates =
                            toc_detect_printed_toc_candidates(page_lines, i, printed_toc_started, &page_looks_like_toc);

                        if (page_looks_like_toc) {
                            fz_link* page_links = nullptr;
                            fz_page* link_page = fz_load_page(context_, doc_, i);
                            page_links = fz_load_links(context_, link_page);
                            fz_drop_page(context_, link_page);
                            if (page_links) {
                                toc_attach_link_targets_to_printed_candidates(
                                    context_,
                                    doc_,
                                    page_links,
                                    page_printed_candidates,
                                    n);
                                fz_drop_link(context_, page_links);
                            }
                            printed_toc_started = true;
                            last_printed_toc_page = i;
                            printed_toc_candidates.insert(
                                printed_toc_candidates.end(),
                                page_printed_candidates.begin(),
                                page_printed_candidates.end());
                        }
                    }
                }

                fz_drop_stext_page(context_, stext_page);
            }


            fz_drop_document(context_, doc_);
        }
        fz_catch(context_) {
            std::wcout << L"There was an error in indexing thread.\n";
        }

        fz_drop_context(context_);

        document_indexing_mutex.lock();

        reference_indices = std::move(local_reference_data);
        equation_indices = std::move(local_equation_data);
        generic_indices = std::move(local_generic_data);

        super_fast_search_index = std::move(local_super_fast_search_index);
        super_fast_page_begin_indices = std::move(local_page_begin_indices);
        if (SUPER_FAST_SEARCH) {
            super_fast_search_index_ready = true;
        }

        std::vector<TocNode*> selected_created_toc_nodes;
        bool prefer_created_toc = false;

        if (should_build_created_toc) {
            std::vector<std::wstring> local_page_labels;
            {
                std::lock_guard guard(page_dims_mutex);
                local_page_labels = page_labels;
            }
            if (local_page_labels.size() != static_cast<size_t>(n) && indexed_page_labels.size() == static_cast<size_t>(n)) {
                local_page_labels = indexed_page_labels;
            }

            toc_filter_heading_candidates(heading_toc_candidates, n);

            std::vector<TocNode*> heading_nodes = toc_build_heading_tree(
                heading_toc_candidates,
                MAX_CREATED_TABLE_OF_CONTENTS_SIZE);
            std::vector<ResolvedPrintedTocEntry> resolved_printed_entries = toc_resolve_printed_toc_entries(
                printed_toc_candidates,
                heading_toc_candidates,
                local_page_labels,
                n);
            std::vector<TocNode*> printed_nodes = toc_build_printed_tree(
                resolved_printed_entries,
                MAX_CREATED_TABLE_OF_CONTENTS_SIZE);

            int heading_count = toc_count_nodes(heading_nodes);
            int printed_count = toc_count_nodes(printed_nodes);
            bool printed_is_sensible = toc_nodes_are_printed_sensible(printed_nodes, n);

            if (printed_is_sensible) {
                selected_created_toc_nodes = std::move(printed_nodes);
                toc_clear_nodes(heading_nodes);
            }
            else if (heading_count >= 2) {
                selected_created_toc_nodes = std::move(heading_nodes);
                toc_clear_nodes(printed_nodes);
            }
            else if (printed_count > 0) {
                selected_created_toc_nodes = std::move(printed_nodes);
                toc_clear_nodes(heading_nodes);
            }
            else {
                toc_clear_nodes(heading_nodes);
                toc_clear_nodes(printed_nodes);
            }
            if (printed_is_sensible) {
                local_generated_page_links = toc_build_printed_toc_links(
                    resolved_printed_entries,
                    MAX_CREATED_TABLE_OF_CONTENTS_SIZE);
            }

            float generated_quality = toc_quality_score(selected_created_toc_nodes, n);
            float native_quality = toc_quality_score(top_level_toc_nodes, n);
            int generated_count = toc_count_nodes(selected_created_toc_nodes);
            bool generated_is_useful = generated_count >= 2 && generated_quality > 0.0f;
            prefer_created_toc = generated_is_useful &&
                (top_level_toc_nodes.size() == 0 || native_toc_is_scrappy || generated_quality > native_quality + 4.0f);
        }

        toc_clear_nodes(created_top_level_toc_nodes);
        created_top_level_toc_nodes = std::move(selected_created_toc_nodes);
        should_use_created_toc = prefer_created_toc;
        generated_page_links = std::move(local_generated_page_links);
        cached_merged_pdf_links.clear();
        flat_toc_names.clear();
        flat_toc_pages.clear();
        get_flat_toc(get_toc(), flat_toc_names, flat_toc_pages);
        toc_model_generation++;

        document_indexing_mutex.unlock();
        is_indexing = false;
        if (is_document_indexing_required && invalid_flag) {
            *invalid_flag = true;
        }
        });
    //thread.detach();
}

void Document::stop_indexing() {
    is_document_indexing_required = false;
}



std::vector<IndexedData> Document::find_reference_with_string(std::wstring reference_name, int page_number) {
    if (reference_name.size() > 1 && reference_name[0] == '[') {
        reference_name = reference_name.substr(1, reference_name.size() - 2);
    }

    if (reference_indices.find(reference_name) != reference_indices.end()) {
        return { reference_indices[reference_name] };
    }

    return {};
}

std::vector<IndexedData> Document::find_equation_with_string(std::wstring equation_name, int page_number) {
    if (equation_name.size() > 1 && (equation_name[0] == '(' || equation_name[0] == '[')) {
        equation_name = equation_name.substr(1, equation_name.size() - 2);
    }
    if (equation_indices.find(equation_name) != equation_indices.end()) {
        const std::vector<IndexedData> equations = equation_indices[equation_name];
        int min_distance = 10000;
        std::optional<IndexedData> res = {};

        for (const auto& index : equations) {
            int distance = std::abs(index.page - page_number);

            if (distance < min_distance) {
                min_distance = distance;
                res = index;
            }
        }

        if (res) {
            return { res.value() };
        }
    }

    return {};
}

std::vector<IndexedData> Document::find_generic_with_string(std::wstring equation_name, int page_number) {
    QString qtext = QString::fromStdWString(equation_name);

    std::vector<IndexedData> res;

    QStringList parts = qtext.split(' ');
    if (parts.size() == 2) {
        std::wstring type = parts.at(0).toStdWString();
        std::wstring ref = parts.at(1).toStdWString();

        std::vector<DocumentPos> positions = find_generic_locations(type, ref);
        for (auto pos : positions) {
            IndexedData index;
            index.page = pos.page;
            index.text = equation_name;
            index.y_offset = pos.y;
            res.push_back(index);
        }
    }
    return res;
}


std::optional<std::wstring> Document::get_equation_text_at_position(
    const std::vector<fz_stext_char*>& flat_chars,
    PagelessDocumentPos position,
    std::pair<int,
    int>* out_range) {


    std::wregex regex(L"\\([0-9]+(\\.[0-9]+)*\\)");
    std::optional<std::wstring> match = get_regex_match_at_position(regex, flat_chars, position, out_range);

    if (match) {
        return match.value().substr(1, match.value().size() - 2);
    }
    else {
        return {};
    }
}

std::optional<std::wstring> Document::get_regex_match_at_position(const std::wregex& regex, const std::vector<fz_stext_char*>& flat_chars, PagelessDocumentPos pos, std::pair<int, int>* out_range) {
    std::vector<std::pair<int, int>> match_ranges;
    std::vector<std::wstring> match_texts;

    find_regex_matches_in_stext_page(flat_chars, regex, match_ranges, match_texts);

    for (size_t i = 0; i < match_ranges.size(); i++) {
        auto [start_index, end_index] = match_ranges[i];
        for (int index = start_index; index <= end_index; index++) {
            if (rect_from_quad(flat_chars[index]->quad).contains(pos)) {
                if (out_range) {
                    *out_range = std::make_pair(start_index, end_index);
                }

                return match_texts[i];
            }
        }
    }
    return {};
}

std::vector<DocumentPos> Document::find_generic_locations(const std::wstring& type, const std::wstring& name) {
    std::vector<std::pair<int, DocumentPos>> pos_scores;

    for (size_t i = 0; i < generic_indices.size(); i++) {
        std::vector<std::wstring> parts = split_whitespace(generic_indices[i].text);

        if (parts.size() == 2) {
            std::wstring current_type = parts[0];
            std::wstring current_name = parts[1];

            if (current_name == name) {
                int score = type_name_similarity_score(current_type, type);
                DocumentPos pos{ generic_indices[i].page, 0, generic_indices[i].y_offset };
                pos_scores.push_back(std::make_pair(score, pos));
                //if (score > best_score) {
                //	best_page = generic_indices[i].page;
                //	best_y_offset = generic_indices[i].y_offset;
                //	best_score = score;
                //}
            }

        }
    }

    auto  by_score = [](std::pair<int, DocumentPos> const& a, std::pair<int, DocumentPos> const& b) {
        return a.first < b.first;
    };

    std::sort(pos_scores.begin(), pos_scores.end(), by_score);

    std::vector<DocumentPos> res;
    for (int i = pos_scores.size() - 1; i >= 0; i--) {
        res.push_back(pos_scores[i].second);
    }
    return res;

    //if (best_page != -1) {
    //	*page = best_page;
    //	*y_offset = best_y_offset;
    //	return true;
    //}

    //return false;
}

bool Document::can_use_highlights() {
    return are_highlights_loaded;
}

std::optional<std::pair<std::wstring, std::wstring>> Document::get_generic_link_name_at_position(
    const std::vector<fz_stext_char*>& flat_chars,
    PagelessDocumentPos position,
    std::pair<int,
    int>* out_range) {

    std::wregex regex(L"[a-zA-Z]{3,}(\\.){0,1}[ \t]+[0-9]+(\\.[0-9]+)*");
    std::optional<std::wstring> match_string = get_regex_match_at_position(regex, flat_chars, position, out_range);
    if (match_string) {
        std::vector<std::wstring> parts = split_whitespace(match_string.value());
        if (parts.size() != 2) {
            return {};
        }
        else {
            std::wstring type = parts[0];
            std::wstring name = parts[1];
            return std::make_pair(type, name);
        }
    }

    else {
        return {};
    }

    return {};
}

std::optional<std::wstring> Document::get_reference_text_at_position(const std::vector<fz_stext_char*>& flat_chars, PagelessDocumentPos position, std::pair<int, int>* out_range) {

    char start_char = '[';
    char end_char = ']';
    char delim = ',';

    bool reached = false; // true when we reach selected character
    bool started = false; // true when we reach [
    bool done = false; // true when whe reach a delimeter after the click location
    //	for example suppose we click here in  [124,253,432]
    //                                              ^      
    // then `done` will be true when we reach the second comma

    std::wstring selected_text = L"";

    int index = -1;
    int begin_index = -1;
    int end_index = -1;

    for (auto ch : flat_chars) {
        index++;

        if (rect_from_quad(ch->quad).contains(position)) {
            if (started) {
                reached = true;
            }
        }
        if (ch->c == start_char) {
            started = true;
            continue;
        }

        if (started && reached && (ch->c == delim)) {
            done = true;
            continue;
        }
        if (started && reached && (ch->c == end_char)) {
            if (out_range) {
                *out_range = std::make_pair(begin_index, end_index);
            }
            return selected_text;
        }
        if (started && (!reached) && (ch->c == end_char)) {
            started = false;
            selected_text.clear();
            begin_index = index;
        }

        if (started && (!done)) {
            if (ch->c != ' ') {
                if (selected_text.size() == 0) {
                    begin_index = index;
                }
                selected_text.push_back(ch->c);
                end_index = index;
            }
        }

        if ((started) && (!reached) && (ch->c == delim)) {
            selected_text.clear();
            begin_index = index;
        }

    }

    return {};
}

void get_matches(std::wstring haystack, const std::wregex& reg, std::vector<std::pair<int, int>>& indices) {
    std::wsmatch match;

    int offset = 0;
    while (std::regex_search(haystack, match, reg)) {
        int start_index = offset + match.position();
        int end_index = start_index + match.length();
        indices.push_back(std::make_pair(start_index, end_index));

        int old_length = haystack.size();
        haystack = match.suffix();
        int new_length = haystack.size();

        offset += (old_length - new_length);
    }
}

std::optional<std::wstring> Document::get_text_at_position(const std::vector<fz_stext_char*>& flat_chars, PagelessDocumentPos position){

    std::wstring selected_string;
    bool reached = false;

    for (auto ch : flat_chars) {
        if (iswspace(ch->c) || (ch->next == nullptr)) {
            if (reached) {
                return selected_string;
            }
            selected_string.clear();
        }
        else {
            selected_string.push_back(ch->c);
        }
        if (rect_from_quad(ch->quad).contains(position)) {
            reached = true;
        }
    }


    return {};
}

std::wstring clean_bib_string_quotations(std::wstring bib_string){
    int start_index= bib_string.find(L"“");
    int end_index= bib_string.find(L"”");
    if ((start_index != -1) && (end_index != -1)){
        if ((end_index - start_index) > bib_string.size() / 2){
            return bib_string.substr(start_index+1, end_index - start_index-1);
        }
    }
    return bib_string;
}

std::optional<std::wstring> Document::get_paper_name_at_position(const std::vector<fz_stext_char*>& flat_chars, PagelessDocumentPos position) {
    std::wstring selected_string = L"";
    bool reached = false;

    for (auto ch : flat_chars) {
        if (ch->c == '.') {
            if (!reached) {
                selected_string = L"";
            }
            else {
                return clean_bib_string_quotations(selected_string);
            }
        }

        if (rect_from_quad(ch->quad).contains(position)) {
            reached = true;
        }
        if ((ch->c == '-') && (ch->next == nullptr)) continue;

        if (ch->c != '.') {
            selected_string.push_back(ch->c);
        }
        if (ch->next == nullptr) {
            selected_string.push_back(' ');
        }

    }

    return {};
}

fz_pixmap* Document::get_small_pixmap(int page) {
    for (auto [cached_page, pixmap] : cached_small_pixmaps) {
        if (cached_page == page) {
            return pixmap;
        }
    }
    if (!doc) return nullptr;

    //fz_matrix ctm = fz_scale(0.5f, 0.5f);
    fz_matrix ctm = fz_scale(SMALL_PIXMAP_SCALE, SMALL_PIXMAP_SCALE);
    fz_pixmap* res = fz_new_pixmap_from_page_number(context, doc, page, ctm, fz_device_rgb(context), 0);
    cached_small_pixmaps.push_back(std::make_pair(page, res));
    unsigned int SMALL_PIXMAP_CACHE_SIZE = 5;

    if (cached_small_pixmaps.size() > SMALL_PIXMAP_CACHE_SIZE) {
        fz_drop_pixmap(context, cached_small_pixmaps[0].second);
        cached_small_pixmaps.erase(cached_small_pixmaps.begin());
    }

    return res;
}

void Document::get_text_selection(AbsoluteDocumentPos selection_begin,
    AbsoluteDocumentPos selection_end,
    bool is_word_selection, // when in word select mode, we select entire words even if the range only partially includes the word
    std::deque<AbsoluteRect>& selected_characters,
    std::wstring& selected_text) {
    get_text_selection(context, selection_begin, selection_end, is_word_selection, selected_characters, selected_text);
}

std::wstring Document::get_raw_text_selection(AbsoluteDocumentPos selection_begin, AbsoluteDocumentPos selection_end) {
    DocumentPos page_pos1 = absolute_to_page_pos_uncentered(selection_begin);
    DocumentPos page_pos2 = absolute_to_page_pos_uncentered(selection_end);

    fz_stext_page* page = get_stext_with_page_number(page_pos1.page);
    fz_stext_char* closest_char_to_begin = nullptr;
    float closest_char_to_begin_distance = 1000000;
    fz_stext_char* closest_char_to_end = nullptr;
    float closest_char_to_end_distance = 1000000;

    for (auto [block, line, chr] : PageIterator(page)) {
        fz_rect rect = fz_rect_from_quad(chr->quad);
        float center_x = (rect.x0 + rect.x1) / 2;
        float center_y = (rect.y0 + rect.y1) / 2;
        DocumentPos docpos = DocumentPos{ page_pos1.page, center_x, center_y };
        float begin_distance = std::abs(docpos.x - page_pos1.x) + std::abs(docpos.y - page_pos1.y);
        float end_distance = std::abs(docpos.x - page_pos2.x) + std::abs(docpos.y - page_pos2.y);
        if (begin_distance < closest_char_to_begin_distance) {
            closest_char_to_begin = chr;
            closest_char_to_begin_distance = begin_distance;
        }

        if (end_distance < closest_char_to_end_distance) {
            closest_char_to_end = chr;
            closest_char_to_end_distance = end_distance;
        }
    }

    std::wstring result;
    bool reached_begin = false;
    for (auto [block, line, chr] : PageIterator(page)) {
        if (chr == closest_char_to_end) break;
        if (chr == closest_char_to_begin) {
            reached_begin = true;
        }
        if (reached_begin) {
            result.push_back(chr->c);
            if (chr->next == nullptr) {
                result.push_back('\n');
            }
        }
    }
    return result;

}

void Document::get_line_selection(AbsoluteDocumentPos selection_begin,
    AbsoluteDocumentPos selection_end,
    std::deque<AbsoluteRect>& selected_characters,
    std::wstring& selected_text) {

    selected_characters.clear();
    selected_text.clear();

    DocumentPos begin_doc_pos = selection_begin.to_document(this);
    DocumentPos end_doc_pos = selection_end.to_document(this);

    bool begun = false;

    fz_point begin_fz_point = { begin_doc_pos.x, begin_doc_pos.y };
    fz_point end_fz_point = { end_doc_pos.x, end_doc_pos.y };

    fz_stext_page* begin_page = get_stext_with_page_number(begin_doc_pos.page);
    fz_stext_page* end_page = get_stext_with_page_number(end_doc_pos.page);

    int begin_index, end_index;
    fz_stext_line* closest_line_to_begin = find_closest_line_to_document_point(begin_page, begin_fz_point, &begin_index);
    fz_stext_line* closest_line_to_end = find_closest_line_to_document_point(end_page, end_fz_point, &end_index);

    if ((begin_page == end_page && begin_index > end_index)) {
        std::swap(closest_line_to_begin, closest_line_to_end);
    }
    else if (begin_page > end_page){
        std::swap(closest_line_to_begin, closest_line_to_end);
        std::swap(begin_doc_pos, end_doc_pos);
    }

    for (int page_number = begin_doc_pos.page; page_number <= end_doc_pos.page; page_number++) {


        fz_stext_page* stext_page = get_stext_with_page_number(page_number);

        if (closest_line_to_begin == nullptr || closest_line_to_end == nullptr) return;

        LL_ITER(block, stext_page->first_block) {
            if (block->type == FZ_STEXT_BLOCK_TEXT) {
                LL_ITER(line, block->u.t.first_line) {
                    if (line == closest_line_to_begin) {
                        begun = true;
                    }

                    if (begun) {
                        for (fz_stext_char* chr = line->first_char; chr; chr = chr->next) {
                            selected_characters.push_back(to_absolute(page_number, chr->quad));
                            selected_text.push_back(chr->c);
                        }
                    }

                    if (line == closest_line_to_end) {
                        return;
                    }
                }
            }
        }

    }

}

void Document::get_text_selection(fz_context* ctx, AbsoluteDocumentPos selection_begin,
    AbsoluteDocumentPos selection_end,
    bool is_word_selection,
    std::deque<AbsoluteRect>& selected_characters,
    std::wstring& selected_text,
    fz_document* doc_) {

    selected_characters.clear();
    selected_text.clear();

    if (doc_ == nullptr) {
        doc_ = doc;
    }


    DocumentPos page_pos1 = absolute_to_page_pos_uncentered(selection_begin);
    DocumentPos page_pos2 = absolute_to_page_pos_uncentered(selection_end);

    int page_begin, page_end;
    fz_point page_point1;
    fz_point page_point2;

    page_begin = page_pos1.page;
    page_end = page_pos2.page;
    page_point1.x = page_pos1.x;
    page_point2.x = page_pos2.x;
    page_point1.y = page_pos1.y;
    page_point2.y = page_pos2.y;

    if ((page_begin == -1) || (page_end == -1)) {
        return;
    }

    if (page_end < page_begin) {
        std::swap(page_begin, page_end);
        std::swap(page_point1, page_point2);
    }

    selected_text.clear();

    bool word_selecting = false;
    bool selecting = false;
    if (is_word_selection) {
        selecting = true;
    }

    for (int i = page_begin; i <= page_end; i++) {

        // for now, let's assume there is only one page
        fz_stext_page* stext_page = get_stext_with_page_number(ctx, i, doc_);
        if (!stext_page) continue;

        std::vector<fz_stext_char*> flat_chars;
        get_flat_chars_from_stext_page(stext_page, flat_chars);


        int location_index1, location_index2;
        fz_stext_char* char_begin = nullptr;
        fz_stext_char* char_end = nullptr;
        if (i == page_begin) {
            char_begin = find_closest_char_to_document_point(flat_chars, page_point1, &location_index1);
        }
        if (i == page_end) {
            char_end = find_closest_char_to_document_point(flat_chars, page_point2, &location_index2);
        }

        if (char_begin == char_end && (!is_word_selection) && (char_begin != nullptr)) {
            selected_text.push_back(char_begin->c);
            selected_characters.push_back(to_absolute(i, char_begin->quad));
            return;
        }

        if (flat_chars.size() > 0) {
            if (char_begin == nullptr) {
                char_begin = flat_chars[0];
            }
            if (char_end == nullptr) {
                char_end = flat_chars[flat_chars.size() - 1];
            }
        }

        if ((char_begin == nullptr) || (char_end == nullptr)) {
            return;
        }

        while ((char_begin->c == ' ') && (char_begin->next != nullptr)) {
            char_begin = char_begin->next;
        }

        if (char_begin && char_end) {
            // swap the locations if end happends before begin
            if (page_begin == page_end && location_index1 > location_index2) {
                std::swap(char_begin, char_end);
            }
        }


        for (auto current_char : flat_chars) {
            if (!is_word_selection) {
                if (current_char == char_begin) {
                    selecting = true;
                }
                //if (current_char == char_end) {
                //	selecting = false;
                //}
            }
            else {
                if (word_selecting == false && is_separator(char_begin, current_char)) {
                    selected_text.clear();
                    selected_characters.clear();
                    continue;
                }
                if (current_char == char_begin) {
                    word_selecting = true;
                }
                if (current_char == char_end) {
                    selecting = false;
                }
                if (word_selecting == true && is_separator(char_end, current_char) && selecting == false) {
                    word_selecting = false;
                    return;
                }
            }

            if (selecting || word_selecting) {
                if (!(current_char->c == ' ' && selected_text.size() == 0)) {
                    selected_text.push_back(current_char->c);
                    selected_characters.push_back(to_absolute(i, current_char->quad));
                }
                if ((current_char->next == nullptr)) {
                    if (current_char->c != '-')
                    {
                        selected_text.push_back(' ');
                        if (is_word_selection) {
                            if (!word_selecting) {
                                selected_text.clear();
                                selected_characters.clear();
                            }
                            else if (!selecting){
                                return;
                            }
                        }
                    }
                    else {
                        selected_text.pop_back();
                    }
                }

            }
            if (!is_word_selection) {
                if (current_char == char_end) {
                    selecting = false;
                }
            }
        }
    }
}

void Document::get_pdf_annotations(std::vector<BookMark>& pdf_bookmarks, std::vector<Highlight>& pdf_highlights, std::vector<FreehandDrawing>& pdf_drawings) {
    for (int p = 0; p < num_pages(); p++) {
        fz_page* page = fz_load_page(context, doc, p);
        pdf_page* pdf_page = pdf_page_from_fz_page(context, page);

        pdf_annot* annot = pdf_first_annot(context, pdf_page);
        while (annot) {
            enum pdf_annot_type annot_type = pdf_annot_type(context, annot);

            if (annot_type == pdf_annot_type::PDF_ANNOT_INK) {
                int num_strokes = pdf_annot_ink_list_count(context, annot);
                for (int stroke_index = 0; stroke_index < num_strokes; stroke_index++) {
                    FreehandDrawing drawing;

                    int vertex_count = pdf_annot_ink_list_stroke_count(context, annot, stroke_index);
                    int n_channels;
                    float color[4];
                    pdf_annot_color(context, annot, &n_channels, color);
                    char type = get_highlight_color_type(color);
                    float thickness = pdf_annot_border(context, annot);
                    drawing.type = type;

                    for (int vertex_index = 0; vertex_index < vertex_count; vertex_index++) {
                        fz_point vertex = pdf_annot_ink_list_stroke_vertex(context, annot, stroke_index, vertex_index);
                        FreehandDrawingPoint point;
                        DocumentPos docpos;
                        docpos.page = p;
                        docpos.x = vertex.x;
                        docpos.y = vertex.y;
                        point.pos = document_to_absolute_pos(docpos);
                        point.thickness = thickness;
                        drawing.points.push_back(point);
                    }
                    pdf_drawings.push_back(drawing);
                }
            }
            if (annot_type == pdf_annot_type::PDF_ANNOT_TEXT) {
                PagelessDocumentRect rect = pdf_bound_annot(context, annot);
                AbsoluteRect absrect = to_absolute(p, rect);
                // get text of annotation
                const char* txt = pdf_annot_contents(context, annot);
                //new_bookmark.description = utf8_decode(txt);
                BookMark new_bookmark;
                new_bookmark.description = utf8_decode(txt);
                new_bookmark.y_offset_ = (absrect.y0 + absrect.y1) / 2;
                new_bookmark.begin_x = (absrect.x0 + absrect.x1) / 2;
                new_bookmark.begin_y = new_bookmark.y_offset_;
                pdf_bookmarks.push_back(new_bookmark);

            }

            if (annot_type == pdf_annot_type::PDF_ANNOT_FREE_TEXT) {
                PagelessDocumentRect rect = pdf_bound_annot(context, annot);
                AbsoluteRect absrect = to_absolute(p, rect);
                // get text of annotation
                const char* txt = pdf_annot_contents(context, annot);
                //new_bookmark.description = utf8_decode(txt);
                BookMark new_bookmark;
                new_bookmark.description = utf8_decode(txt);
                new_bookmark.y_offset_ = absrect.y0;
                new_bookmark.begin_x = absrect.x0;
                new_bookmark.begin_y = absrect.y0;
                new_bookmark.end_x = absrect.x1;
                new_bookmark.end_y = absrect.y1;
                char font_name[100] = { 0 };
                const char* font_name_addr[] = { font_name };
                float font_size;
                float color[4];
                int n_channels;

                pdf_annot_default_appearance(context, annot, &font_name_addr[0], &font_size, &n_channels, color);
                new_bookmark.font_face = utf8_decode(font_name);
                new_bookmark.font_size = font_size;
                new_bookmark.color[0] = color[0];
                new_bookmark.color[1] = color[1];
                new_bookmark.color[2] = color[2];

                pdf_bookmarks.push_back(new_bookmark);

            }

            if ((annot_type == pdf_annot_type::PDF_ANNOT_HIGHLIGHT) || (annot_type == pdf_annot_type::PDF_ANNOT_UNDERLINE) || (annot_type == pdf_annot_type::PDF_ANNOT_STRIKE_OUT)) {
                int num_quads = pdf_annot_quad_point_count(context, annot);

                if (num_quads > 0) {
                    fz_quad first_vertex = pdf_annot_quad_point(context, annot, 0);
                    fz_quad last_vertex = pdf_annot_quad_point(context, annot, num_quads - 1);
                    const char* txt = pdf_annot_contents(context, annot);
                    float color[4];
                    int n_channels = -1;
                    pdf_annot_color(context, annot, &n_channels, color);

                    DocumentPos begin_pos, end_pos;
                    begin_pos.page = p;
                    begin_pos.x = first_vertex.ul.x;
                    begin_pos.y = (first_vertex.ul.y + first_vertex.ll.y) / 2;
                    end_pos.page = p;
                    end_pos.x = last_vertex.ur.x;
                    end_pos.y = (last_vertex.ur.y + last_vertex.lr.y) / 2;

                    AbsoluteDocumentPos begin_abspos = document_to_absolute_pos(begin_pos);
                    AbsoluteDocumentPos end_abspos = document_to_absolute_pos(end_pos);

                    Highlight new_highlight;
                    new_highlight.selection_begin = begin_abspos;
                    new_highlight.selection_end = end_abspos;
                    new_highlight.text_annot = utf8_decode(txt);
                    new_highlight.type = get_highlight_color_type(color);

                    if (annot_type == pdf_annot_type::PDF_ANNOT_UNDERLINE) {
                        new_highlight.type = new_highlight.type + 'A' - 'a';
                    }
                    if (annot_type == pdf_annot_type::PDF_ANNOT_STRIKE_OUT) {
                        new_highlight.type = '_';
                    }

                    pdf_highlights.push_back(new_highlight);

                }
            }
            annot = pdf_next_annot(context, annot);
        }

        fz_drop_page(context, page);
    }
}

void Document::import_annotations() {
    std::vector<BookMark> pdf_bookmarks;
    std::vector<Highlight> pdf_highlights;
    std::vector<FreehandDrawing> pdf_drawings;

    get_pdf_annotations(pdf_bookmarks, pdf_highlights, pdf_drawings);


    //           int count[1] = { static_cast<int>(points.size()) };
       //		pdf_set_annot_border(context, drawing_annot, drawing.points[0].thickness);
       //		pdf_set_annot_ink_list(context, drawing_annot, 1, count, &points[0]);
       //		pdf_update_annot(context, drawing_annot);
       //	}
    for (auto bookmark : pdf_bookmarks) {
        if (is_bookmark_new(bookmark)) {

            if (bookmark.end_y > 0) { // it is a freetext bookmark
                AbsoluteRect absrect;
                absrect.x0 = bookmark.begin_x;
                absrect.x1 = bookmark.end_x;
                absrect.y0 = bookmark.begin_y;
                absrect.y1 = bookmark.end_y;
                add_freetext_bookmark_with_color(bookmark.description, absrect, bookmark.color, bookmark.font_size);
            }
            else {
                add_marked_bookmark(bookmark.description, { bookmark.begin_x, bookmark.begin_y });
            }
        }
    }
    for (auto highlight : pdf_highlights) {
        if (is_highlight_new(highlight)) {
            add_highlight(highlight.text_annot, highlight.selection_begin, highlight.selection_end, highlight.type);
        }
    }
    for (auto drawing : pdf_drawings) {
        if (is_drawing_new(drawing)) {
            add_freehand_drawing(drawing);
        }
    }
}

void Document::embed_annotations(std::wstring new_file_path) {

    std::unordered_map<int, fz_page*> cached_pages;
    std::vector<std::pair<pdf_page*, pdf_annot*>> created_annotations;

    auto load_cached_page = [&](int page_number) {
        if (cached_pages.find(page_number) != cached_pages.end()) {
            return cached_pages[page_number];
        }
        else {
            fz_page* page = fz_load_page(context, doc, page_number);
            cached_pages[page_number] = page;
            return page;
        }
    };

    std::string new_file_path_utf8 = utf8_encode(new_file_path);
    fz_output* output_file = nullptr;

    fz_try(context) {
        output_file = fz_new_output_with_path(context, new_file_path_utf8.c_str(), 0);
    }
    fz_catch(context) {
        output_file = nullptr;
    }

    if (output_file == nullptr) {
        show_error_message(L"Could not open the output file. Make sure the file is not opened in sioyek or another application.");
        return;
    }

    pdf_document* pdf_doc = pdf_specifics(context, doc);
    std::vector<Highlight> pdf_highlights;
    std::vector<BookMark> pdf_bookmarks;
    std::vector<FreehandDrawing> pdf_drawings;

    get_pdf_annotations(pdf_bookmarks, pdf_highlights, pdf_drawings);

    const std::vector<Highlight>& doc_highlights = get_new_sioyek_highlights(pdf_highlights);
    const std::vector<BookMark>& doc_bookmarks = get_new_sioyek_bookmarks(pdf_bookmarks);

    for (auto highlight : doc_highlights) {
        int page_number = get_offset_page_number(highlight.selection_begin.y);

        std::deque<AbsoluteRect> selected_characters;
        std::vector<AbsoluteRect> merged_characters;
        std::vector<PagelessDocumentRect> selected_characters_page_rects;
        std::wstring selected_text;

        get_text_selection(highlight.selection_begin, highlight.selection_end, !EXACT_HIGHLIGHT_SELECT, selected_characters, selected_text);
        merge_selected_character_rects(selected_characters, merged_characters, highlight.type != '_');

        for (auto absrect : merged_characters) {
            selected_characters_page_rects.push_back(absrect.to_document(this).rect);
        }
        //absolute_to_page_pos
        std::vector<fz_quad> selected_character_quads = quads_from_rects(selected_characters_page_rects);

        fz_page* page = load_cached_page(page_number);
        pdf_page* pdf_page = pdf_page_from_fz_page(context, page);
        pdf_annot* highlight_annot;
        if (highlight.type == '_') {
            highlight_annot = pdf_create_annot(context, pdf_page, PDF_ANNOT_STRIKE_OUT);
        }
        else if (std::isupper(highlight.type)) {
            highlight_annot = pdf_create_annot(context, pdf_page, PDF_ANNOT_UNDERLINE);
        }
        else {
            highlight_annot = pdf_create_annot(context, pdf_page, PDF_ANNOT_HIGHLIGHT);
        }
        float* color_ = get_highlight_type_color(highlight.type);
        float color[3];
        if (LIGHTEN_COLORS_WHEN_EMBEDDING_ANNOTATIONS) {
            // lighten highlight colors before embedding (because we use alpha to make it lighter but
            // the color doesn't take it into accout). Also see: https://github.com/ahrm/sioyek/issues/667
            lighten_color(color_, color);
        }
        else {
            color[0] = color_[0];
            color[1] = color_[1];
            color[2] = color_[2];
        }

        pdf_set_annot_color(context, highlight_annot, 3, color);
        if (highlight.text_annot.size() > 0) {
            std::string encoded_highlight_text = utf8_encode(highlight.text_annot);
            pdf_set_annot_contents(context, highlight_annot, encoded_highlight_text.c_str());
        }

        pdf_set_annot_quad_points(context, highlight_annot, selected_character_quads.size(), &selected_character_quads[0]);
        pdf_update_annot(context, highlight_annot);

        created_annotations.push_back(std::make_pair(pdf_page, highlight_annot));

    }

    for (auto bookmark : doc_bookmarks) {
        auto [page_number, doc_x, doc_y] = absolute_to_page_pos({ 0, bookmark.get_y_offset()});

        fz_page* page = load_cached_page(page_number);
        pdf_page* pdf_page = pdf_page_from_fz_page(context, page);
        pdf_annot* bookmark_annot;
        if (bookmark.is_freetext() && (!bookmark.is_box())) {
            bookmark_annot = pdf_create_annot(context, pdf_page, PDF_ANNOT_FREE_TEXT);
        }
        else if (bookmark.is_box()) {
            bookmark_annot = pdf_create_annot(context, pdf_page, PDF_ANNOT_SQUARE);

            float default_color[3] = { 0, 0, 0 };
            float* color = &default_color[0];
            std::optional<char> bm_type = bookmark.get_type();
            if (bm_type) {
                color = get_highlight_type_color(bm_type.value());
            }

            pdf_set_annot_color(context, bookmark_annot, 3, color);
        }
        else {
            bookmark_annot = pdf_create_annot(context, pdf_page, PDF_ANNOT_TEXT);
        }

        std::string encoded_bookmark_text = utf8_encode(bookmark.description);

        PagelessDocumentRect annot_rect;
        if (bookmark.is_freetext()) {
            annot_rect = bookmark.rect().to_document(this).rect;

            std::string encoded_font_face = utf8_encode(bookmark.font_face);
            const char* font_face = bookmark.font_face.size() == 0 ? "Times New Roman" : encoded_font_face.c_str();
            pdf_set_annot_default_appearance(context, bookmark_annot, font_face, bookmark.font_size, 3, bookmark.color);
        }
        else if (bookmark.is_marked()) {
            //DocumentPos begin_page_pos = absolute_to_page_pos_uncentered({ bookmark.begin_x, bookmark.begin_y });
            DocumentPos begin_page_pos = bookmark.begin_pos().to_document(this);

            annot_rect.x0 = begin_page_pos.x - 6;
            annot_rect.x1 = begin_page_pos.x + 6;
            annot_rect.y0 = begin_page_pos.y - 6;
            annot_rect.y1 = begin_page_pos.y + 6;
        }
        else {
            annot_rect.x0 = 10;
            annot_rect.x1 = 20;
            annot_rect.y0 = doc_y;
            annot_rect.y1 = doc_y + 10;
        }

        pdf_set_annot_rect(context, bookmark_annot, annot_rect);
        pdf_set_annot_contents(context, bookmark_annot, encoded_bookmark_text.c_str());
        pdf_update_annot(context, bookmark_annot);

        created_annotations.push_back(std::make_pair(pdf_page, bookmark_annot));
    }

    for (auto [page_number, drawings] : page_freehand_drawings) {
        for (auto drawing : drawings) {
            fz_page* page = load_cached_page(page_number);
            pdf_page* pdf_page = pdf_page_from_fz_page(context, page);
            pdf_annot* drawing_annot = pdf_create_annot(context, pdf_page, PDF_ANNOT_INK);
            std::vector<fz_point> points;

            for (auto point : drawing.points) {
                DocumentPos docpos = absolute_to_page_pos_uncentered(point.pos);
                if (docpos.page == page_number) {
                    points.push_back(fz_point{ docpos.x, docpos.y });
                }
            }

            int count[1] = { static_cast<int>(points.size()) };
            pdf_set_annot_border(context, drawing_annot, drawing.points[0].thickness);
            pdf_set_annot_ink_list(context, drawing_annot, 1, count, &points[0]);
            if (drawing.type >= 'a' && drawing.type <= 'z') {
                pdf_set_annot_color(context, drawing_annot, 3, &HIGHLIGHT_COLORS[3 * (drawing.type - 'a')]);
            }
            pdf_update_annot(context, drawing_annot);
        }
    }


    pdf_write_options pwo{};
    pdf_write_document(context, pdf_doc, output_file, &pwo);
    fz_close_output(context, output_file);
    fz_drop_output(context, output_file);

    for (auto [page, annot] : created_annotations) {
        pdf_delete_annot(context, page, annot);
        pdf_drop_annot(context, annot);
    }
    for (auto [num, page] : cached_pages) {
        fz_drop_page(context, page);
    }
}

std::optional<std::wstring> Document::get_text_at_position(DocumentPos position) {
    fz_stext_page* stext_page = get_stext_with_page_number(position.page);
    std::vector<fz_stext_char*> flat_chars;
    get_flat_chars_from_stext_page(stext_page, flat_chars);
    return get_text_at_position(flat_chars, position.pageless());
}

std::optional<std::wstring> Document::get_paper_name_at_position(DocumentPos position) {
    fz_stext_page* stext_page = get_stext_with_page_number(position.page);
    std::vector<fz_stext_char*> flat_chars;
    get_flat_chars_from_stext_page(stext_page, flat_chars);
    return get_paper_name_at_position(flat_chars, position.pageless());
}

std::optional<std::wstring> Document::get_reference_text_at_position(DocumentPos pos, std::pair<int, int>* out_range) {
    fz_stext_page* stext_page = get_stext_with_page_number(pos.page);
    std::vector<fz_stext_char*> flat_chars;
    get_flat_chars_from_stext_page(stext_page, flat_chars);
    return get_reference_text_at_position(flat_chars, pos.pageless(), out_range);
}

std::optional<std::wstring> Document::get_equation_text_at_position(DocumentPos pos, std::pair<int, int>* out_range) {
    fz_stext_page* stext_page = get_stext_with_page_number(pos.page);
    std::vector<fz_stext_char*> flat_chars;
    get_flat_chars_from_stext_page(stext_page, flat_chars);
    return get_equation_text_at_position(flat_chars, pos.pageless(), out_range);
}

std::optional<std::pair<std::wstring, std::wstring>>  Document::get_generic_link_name_at_position(DocumentPos pos, std::pair<int, int>* out_range) {
    fz_stext_page* stext_page = get_stext_with_page_number(pos.page);
    std::vector<fz_stext_char*> flat_chars;
    get_flat_chars_from_stext_page(stext_page, flat_chars);
    return get_generic_link_name_at_position(flat_chars,  pos.pageless(), out_range);
}

std::optional<std::wstring> Document::get_regex_match_at_position(const std::wregex& regex, DocumentPos position, std::pair<int, int>* out_range) {
    fz_stext_page* stext_page = get_stext_with_page_number(position.page);
    std::vector<fz_stext_char*> flat_chars;
    get_flat_chars_from_stext_page(stext_page, flat_chars);
    return get_regex_match_at_position(regex, flat_chars, position.pageless(), out_range);
}

std::vector<std::vector<PagelessDocumentRect>> Document::get_page_flat_word_chars(int page) {
    // warning: this function should only be called after get_page_flat_words has already cached the chars
    if (cached_flat_word_chars.find(page) != cached_flat_word_chars.end()) {
        return cached_flat_word_chars[page];
    }
    return {};
}

std::vector<PagelessDocumentRect> Document::get_page_flat_words(int page) {
    if (cached_flat_words.find(page) != cached_flat_words.end()) {
        return cached_flat_words[page];
    }

    fz_stext_page* stext_page = get_stext_with_page_number(page);
    std::vector<fz_stext_char*> flat_chars;
    std::vector<PagelessDocumentRect> word_rects;
    std::vector<std::vector<PagelessDocumentRect>> word_char_rects;
    get_flat_chars_from_stext_page(stext_page, flat_chars);
    get_flat_words_from_flat_chars(flat_chars, word_rects, &word_char_rects);
    cached_flat_words[page] = word_rects;
    cached_flat_word_chars[page] = word_char_rects;
    return word_rects;
}

void Document::rotate() {
    std::swap(page_heights, page_widths);
    float acc_height = 0;
    for (size_t i = 0; i < page_heights.size(); i++) {
        accum_page_heights[i] = acc_height;
        acc_height += page_heights[i];
    }
}

std::optional<Highlight> Document::get_next_highlight(float abs_y, char type, int offset) const {

    int index = 0;
    auto sorted_highlights = get_highlights_sorted(type);

    for (auto hl : sorted_highlights) {
        if (hl.selection_begin.y <= abs_y) {
            index++;
        }
        else {
            break;
        }
    }

    // now index points the the next highlight
    if ((size_t)(index + offset) < sorted_highlights.size()) {
        return sorted_highlights[index + offset];
    }

    return {};
}

std::optional<Highlight> Document::get_prev_highlight(float abs_y, char type, int offset) const {

    int index = -1;
    auto sorted_highlights = get_highlights_sorted(type);

    for (auto hl : sorted_highlights) {
        if (hl.selection_begin.y < abs_y) {
            index++;
        }
        else {
            break;
        }
    }

    // now index points the the previous highlight
    if ((index + offset) >= 0) {
        return sorted_highlights[index + offset];
    }

    return {};
}

bool Document::needs_password() {
    return document_needs_password;
}


bool Document::apply_password(const char* password) {

    if (context && doc) {
        reload(password);
        if (password_was_correct) {
            correct_password = password;
        }
        return password_was_correct;
    }
    return false;
}

bool Document::needs_authentication() {
    if (needs_password()) {
        return !password_was_correct;
    }
    else {
        return false;
    }
}

std::vector<PagelessDocumentRect> Document::get_highlighted_character_masks(int page) {
    fz_stext_page* stext_page = get_stext_with_page_number(page);
    std::vector<fz_stext_char*> flat_chars;
    get_flat_chars_from_stext_page(stext_page, flat_chars);
    std::vector<PagelessDocumentRect> res;
    std::vector<std::wstring> words;
    std::vector<std::vector<PagelessDocumentRect>> word_rects;
    get_word_rect_list_from_flat_chars(flat_chars, words, word_rects);

    for (size_t i = 0; i < words.size(); i++) {
        std::vector<PagelessDocumentRect> highlighted_characters;
        int num_highlighted = static_cast<int>(std::ceil(words[i].size() * 0.3f));
        for (int j = 0; j < num_highlighted; j++) {
            highlighted_characters.push_back(word_rects[i][j]);
        }
        res.push_back(create_word_rect(highlighted_characters));
    }

    return res;
}


PagelessDocumentRect Document::get_page_rect_no_cache(int page_number) {
    PagelessDocumentRect res{};
    fz_try(context) {
        int n_pages = num_pages();
        if (page_number < n_pages) {
            fz_page* page = fz_load_page(context, doc, page_number);
            PagelessDocumentRect bound = fz_bound_page(context, page);
            res = bound;
            fz_drop_page(context, page);
        }
    }
    fz_catch(context) {

    }
    return res;
}

void DocumentManager::free_document(Document* document) {
    bool found = false;
    std::wstring path_to_erase;

    for (auto [path, doc] : cached_documents) {
        if (doc == document) {
            found = true;
            path_to_erase = path;
        }
    }
    if (found) {
        cached_documents.erase(path_to_erase);
    }

    delete document;
}

std::optional<PdfLink> Document::get_link_in_pos(const DocumentPos& pos) {
    return get_link_in_pos(pos.page, pos.x, pos.y);
}

std::wstring Document::get_pdf_link_text(PdfLink link) {
    int page = link.source_page;

    std::wstring res;
    for (int rect_index = 0; rect_index < link.rects.size(); rect_index++) {

        PagelessDocumentRect current_link_rect = link.rects[rect_index];

        for (auto [block, line, chr] : page_iterator(page)) {
            PagelessDocumentRect charrect = fz_rect_from_quad(chr->quad);
            float y = (charrect.y0 + charrect.y1) / 2;
            float x = (charrect.x0 + charrect.x1) / 2;
            float height = charrect.y1 - charrect.y0;
            float width = charrect.x1 - charrect.x0;

            charrect.y0 = y - height / 5;
            charrect.y1 = y + height / 5;
            charrect.x0 = x;
            charrect.x1 = x;

            if (rects_intersect(charrect, current_link_rect)) {
                res.push_back(chr->c);
            }
        }
    }
    return res;
}

std::vector<PdfLink> Document::get_links_in_page_rect(int page, AbsoluteRect rect) {
    if (!doc) return {};

    std::vector<PdfLink> res;
    DocumentRect doc_rect = rect.to_document(this);

    if (page != -1) {
        const std::vector<PdfLink>& links = get_page_merged_pdf_links(page);
        for (auto link : links) {
            for (auto link_rect : link.rects) {
                if (rects_intersect(doc_rect.rect, link_rect)) {
                    res.push_back(link);
                    break;
                }
            }
        }
    }

    return res;
}

PdfLink Document::pdf_link_from_fz_link(int page, fz_link* link) {
    return PdfLink{ {link->rect}, page, link->uri };
}

std::optional<PdfLink> Document::get_link_in_pos(int page, float doc_x, float doc_y) {
    if (!doc) return {};

    if (page != -1) {
        //fz_link* page_links = get_page_links(page);
        const std::vector<PdfLink>& page_links = get_page_merged_pdf_links(page);
        fz_point point = { doc_x, doc_y };
        std::optional<PdfLink> res = {};
        for (auto link : page_links) {
            for (auto link_rect : link.rects) {
                if (fz_is_point_inside_rect(point, link_rect)) {
                    return link;
                }
            }
        }
        //while (page_links != nullptr) {
        //    if (fz_is_point_inside_rect(point, page_links->rect)) {
        //        res = { {page_links->rect}, page, page_links->uri };
        //        return res;
        //    }
        //    page_links = page_links->next;
        //}

    }
    return {};
}

int Document::add_stext_page_to_created_toc(fz_stext_page* stext_page,
    int page_number,
    std::vector<TocNode*>& toc_node_stack,
    std::vector<TocNode*>& top_level_nodes) {

    int num_new_entries = 0;
    auto add_toc_node = [&](TocNode* node) {
        if (toc_node_stack.size() == 0) {
            top_level_nodes.push_back(node);
            toc_node_stack.push_back(node);
        }
        else {
            bool are_same = false;
            // pop until we reach a parent of stack is empty
            while ((toc_node_stack.size() > 0) &&
                (!is_title_parent_of(toc_node_stack[toc_node_stack.size() - 1]->title, node->title, &are_same)) &&
                (!are_same)) { // ignore items with the same name as current parent (happens when title of chapter is written on top of pages)
                toc_node_stack.pop_back();
            }

            if (are_same) return;
            num_new_entries += 1;

            if (toc_node_stack.size() > 0) {
                toc_node_stack[toc_node_stack.size() - 1]->children.push_back(node);
            }
            else {
                toc_node_stack.push_back(node);
                top_level_nodes.push_back(node);
            }
        }
    };

    LL_ITER(block, stext_page->first_block) {
        std::vector<fz_stext_char*> chars;
        get_flat_chars_from_block(block, chars);
        if (chars.size() > 0) {
            std::wstring block_string;
            std::vector<int> indices;
            get_text_from_flat_chars(chars, block_string, indices);
            if (is_string_titlish(block_string)) {
                TocNode* new_node = new TocNode;
                new_node->page = page_number;
                new_node->title = block_string;
                new_node->x = 0;
                new_node->y = block->bbox.y0;
                add_toc_node(new_node);
            }
        }
    }
    return num_new_entries;
}

float Document::document_to_absolute_y(int page, float doc_y) {
    if ((page < accum_page_heights.size()) && (page >= 0)) {
        return doc_y + accum_page_heights[page];
    }
    return 0;
}

AbsoluteDocumentPos Document::document_to_absolute_pos(DocumentPos doc_pos) {
    float absolute_y = document_to_absolute_y(doc_pos.page, doc_pos.y);
    AbsoluteDocumentPos res = { doc_pos.x, absolute_y };
    if (doc_pos.page < page_widths.size()) {
        res.x -= page_widths[doc_pos.page] / 2;
    }
    return res;
}

AbsoluteRect Document::document_to_absolute_rect(DocumentRect doc_rect){
    AbsoluteDocumentPos top_left = doc_rect.top_left().to_absolute(this);
    AbsoluteDocumentPos bottom_right = doc_rect.bottom_right().to_absolute(this);
    return AbsoluteRect(top_left, bottom_right);
}


AbsoluteRect Document::get_ith_next_line_from_absolute_y(int page, int line_index, int i, bool continue_to_next_page, int* out_index, int* out_page) {
    auto line_rects = get_page_lines(page);

    if (line_index < 0) {
        line_index = line_index + line_rects.size();
    }

    int new_index = line_index + i;

    if ((new_index >= 0) && ((size_t)new_index < line_rects.size())) {
        *out_page = page;
        *out_index = new_index;
        return line_rects[new_index];
    }
    else {
        if (!continue_to_next_page) {
            if (line_index > 0 && (size_t)line_index < line_rects.size()) {
                *out_page = page;
                *out_index = line_index;
                return line_rects[line_index];
            }
            else {
                AbsoluteRect page_absrect = get_page_absolute_rect(page);
                page_absrect.y1 = page_absrect.y0;

                *out_index = 0;
                *out_page = page;
                return page_absrect;
            }
        }

        int tries = 2;
        int n_pages = num_pages();
        for (int j = 1; j < tries+1; j++) {
            int next_page = i > 0 ? page + j : page - j;
            int next_line_index = i > 0 ? 0 : -1;
            if (next_page < 0 || (next_page >= n_pages)) break;
            if (get_page_lines(next_page).size() > 0) {
                return get_ith_next_line_from_absolute_y(next_page, next_line_index, 0, false, out_index, out_page);
            }
        }

        *out_page = page;
        *out_index = line_index;
        return line_rects[line_index];
    }
}

const std::vector<AbsoluteRect>& Document::get_page_lines(
    int page,
    std::vector<std::wstring>* out_line_texts,
    std::vector<std::vector<PagelessDocumentRect>>* out_line_rects){


    if ((out_line_rects == nullptr) && (cached_page_line_rects.find(page) != cached_page_line_rects.end())) {
        if (out_line_texts) {
            *out_line_texts = cached_line_texts[page];
        }
        return cached_page_line_rects[page];
    }
    else {
        fz_stext_page* stext_page = get_stext_with_page_number(page);
        if (stext_page && stext_page_has_lines(stext_page) && (!FORCE_CUSTOM_LINE_ALGORITHM)) {

            PagelessDocumentRect bound = get_page_absolute_rect(page);

            std::vector<PagelessDocumentRect> line_rects;
            std::vector<std::wstring> line_texts;
            std::vector<fz_stext_line*> flat_lines;

            LL_ITER(block, stext_page->first_block) {
                if (block->type == FZ_STEXT_BLOCK_TEXT) {
                    LL_ITER(line, block->u.t.first_line) {
                        flat_lines.push_back(line);
                    }
                }
            }

            std::vector<std::vector<PagelessDocumentRect>> line_char_rects_;

            merge_lines(flat_lines, line_rects, line_texts, &line_char_rects_);

            std::vector<AbsoluteRect> line_rects_;
            std::vector<std::wstring> line_texts_;

            for (size_t i = 0; i < line_rects.size(); i++) {
                //line_rects[i] = DocumentRect(line_rects[i], page).to_absolute(this);
                AbsoluteRect line_rect_absolute = DocumentRect(line_rects[i], page).to_absolute(this);
                if (fz_contains_rect(bound, line_rect_absolute)) {
                    line_rects_.push_back(line_rect_absolute);
                    line_texts_.push_back(line_texts[i]);
                    if (out_line_rects) {
                        out_line_rects->push_back(line_char_rects_[i]);
                    }
                }
            }

            cached_page_line_rects[page] = line_rects_;
            cached_line_texts[page] = line_texts_;

            if (out_line_texts != nullptr) {
                *out_line_texts = line_texts_;
            }

        }
        else {
            fz_pixmap* pixmap = get_small_pixmap(page);
            static const std::vector<AbsoluteRect> empty = {};
            if (pixmap == nullptr) return empty;
            std::vector<unsigned int> hist = get_max_width_histogram_from_pixmap(pixmap);
            std::vector<unsigned int> line_locations;
            std::vector<unsigned int> line_locations_begins;
            get_line_begins_and_ends_from_histogram(hist, line_locations_begins, line_locations);

            std::vector<AbsoluteRect> line_rects;
            for (size_t i = 0; i < line_locations_begins.size(); i++) {
                AbsoluteRect line_rect;
                line_rect.x0 = 0 - page_widths[page] / 2;
                line_rect.x1 = static_cast<float>(pixmap->w) / SMALL_PIXMAP_SCALE - page_widths[page] / 2;
                line_rect.y0 = document_to_absolute_y(page, static_cast<float>(line_locations_begins[i]) / SMALL_PIXMAP_SCALE);
                line_rect.y1 = document_to_absolute_y(page, static_cast<float>(line_locations[i]) / SMALL_PIXMAP_SCALE);
                line_rects.push_back(line_rect);
            }
            cached_page_line_rects[page] = line_rects;
        }
        return cached_page_line_rects[page];
    }
}
void Document::clear_toc_nodes() {
    for (auto node : top_level_toc_nodes) {
        clear_toc_node(node);
    }
    top_level_toc_nodes.clear();
    for (auto node : created_top_level_toc_nodes) {
        clear_toc_node(node);
    }
    created_top_level_toc_nodes.clear();
    should_use_created_toc = false;
    generated_page_links.clear();
    cached_merged_pdf_links.clear();
    toc_model_generation++;
}

void Document::clear_toc_node(TocNode* node) {
    for (auto child : node->children) {
        clear_toc_node(child);
    }
    delete node;
}

DocumentManager::~DocumentManager() {
    for (auto [path, doc] : cached_documents) {
        delete doc;
    }
    cached_documents.clear();
}

bool Document::is_super_fast_index_ready() {
    return super_fast_search_index_ready;
}


std::vector<SearchResult> Document::search_text(std::wstring query, SearchCaseSensitivity case_sensitive, int begin_page, int min_page, int max_page) {

    return search_text_with_index(
        super_fast_search_index,
        super_fast_page_begin_indices,
        query,
        case_sensitive,
        begin_page,
        min_page,
        max_page);

}

std::vector<SearchResult> Document::search_regex(std::wstring query, SearchCaseSensitivity case_sensitive, int begin_page, int min_page, int max_page)
{

    return search_regex_with_index(super_fast_search_index,
        super_fast_page_begin_indices,
        query,
        case_sensitive,
        begin_page,
        min_page,
        max_page);
}

float Document::max_y_offset() {
    int np = num_pages();

    return get_accum_page_height(np - 1) + get_page_height(np - 1);

}

std::wstring Document::get_page_label(int page_index) {
    if (page_index >= 0 && page_index < page_labels.size()) {
        return page_labels[page_index];
    }
    else {
        return QString::number(page_index + 1).toStdWString();
    }
}

int Document::get_page_number_with_label(std::wstring page_label) {
    if (page_labels.size() > 0) {
        for (int i = 0; i < page_labels.size(); i++) {
            if (page_labels[i] == page_label) {
                return i;
            }
        }
        return QString::fromStdWString(page_label).toInt() - 1;
    }
    else {
        return QString::fromStdWString(page_label).toInt() - 1;
    }
}


bool Document::is_reflowable() {
    if (doc != nullptr) {
        return fz_is_document_reflowable(context, doc);
    }
    return false;
}

void Document::clear_document_caches() {
    cached_num_pages = {};
    cached_fastread_highlights.clear();
    cached_line_texts.clear();
    cached_page_line_rects.clear();
    cached_page_index.clear();


    for (auto [_, cached_small_pixmap] : cached_small_pixmaps) {
        fz_drop_pixmap(context, cached_small_pixmap);
    }
    cached_small_pixmaps.clear();

    for (auto [_, cached_stext_page] : cached_stext_pages) {
        fz_drop_stext_page(context, cached_stext_page);
    }
    cached_stext_pages.clear();

    for (auto page_link_pair : cached_page_links) {
        fz_drop_link(context, page_link_pair.second);
    }
    cached_page_links.clear();
    generated_page_links.clear();
    cached_merged_pdf_links.clear();

    delete cached_toc_model;
    cached_toc_model = nullptr;
    cached_toc_model_generation = -1;

    super_fast_search_index.clear();
    super_fast_page_begin_indices.clear();
    super_fast_search_index_ready = false;

    clear_toc_nodes();
    flat_toc_names.clear();
    flat_toc_pages.clear();
}


void Document::load_document_caches(bool* invalid_flag, bool force_now) {

    load_page_dimensions(force_now);
    create_toc_tree(top_level_toc_nodes);
    get_flat_toc(top_level_toc_nodes, flat_toc_names, flat_toc_pages);
    invalid_flag_pointer = invalid_flag;

    // we don't need to index figures in helper documents
    index_document(invalid_flag);
}

int Document::reflow(int page) {

    set_extra("epub_width", EPUB_WIDTH);
    set_extra("epub_height", EPUB_HEIGHT);
    persist_extras();

    fz_bookmark last_position_bookmark = fz_make_bookmark(context,
        doc, fz_location_from_page_number(context, doc, page));

    clear_document_caches();

    //if (EPUB_CSS.size() > -1) {
    //	std::string encoded = utf7_encode(EPUB_CSS);
    //	fz_set_user_css(context, encoded.c_str());
    //}
    //else {
    //	QString temp = EPUB_TEMPLATE;
    //	std::string encoded = temp.replace("%{line_spacing}", QString::number(EPUB_LINE_SPACING)).toStdString();
    //	fz_set_user_css(context, encoded.c_str());
    //}

    if (EPUB_CSS.size() > 0) {
        std::string css = utf8_encode(EPUB_CSS);
        fz_set_user_css(context, css.c_str());
    }

    fz_layout_document(context, doc, EPUB_WIDTH, EPUB_HEIGHT, EPUB_FONT_SIZE);

    bool flag = false;
    load_document_caches(&flag, false);

    fz_location loc = fz_lookup_bookmark(context, doc, last_position_bookmark);
    int new_page = fz_page_number_from_location(context, doc, loc);
    return new_page;
}

//void push_embedding(int index, std::vector<float>& target) {
//    int offset = index * EMBEDDING_DIM;
//    for (int i = 0; i < EMBEDDING_DIM; i++) {
//        target.push_back(embedding_weights[offset + i]);
//    }
//}

//std::vector<float> apply_linear_weights(std::vector<float> embeddings) {
//
//    std::vector<float> outputs;
//
//    for (int j = 0; j < 2; j++) {
//        float res = 0;
//        for (int i = 0; i < embeddings.size(); i++) {
//            res += embeddings[i] * linear_weights[i + j * embeddings.size()];
//        }
//        outputs.push_back(res);
//    }
//    return outputs;
//}

std::vector<std::wstring> Document::get_page_bib_candidates(int page_number, std::vector<PagelessDocumentRect>* out_rects) {
    fz_stext_page* stext_page = get_stext_with_page_number(page_number);
    std::vector<DocumentCharacter> flat_chars;

    //get_flat_chars_from_stext_page(stext_page, flat_chars, true);
    get_flat_chars_from_stext_page_for_bib_detection(stext_page, flat_chars);

    std::vector<PagelessDocumentRect> char_rects;

    float page_width = page_widths[page_number];
    float page_height = page_heights[page_number];

    std::vector<int> dot_indices;
    std::vector<int> end_indices;

    std::vector<int> bracket_end_indices;
    std::vector<int> indented_end_indices;

    std::wstring raw_text;
    for (int i = 0; i < flat_chars.size(); i++) {
        if (flat_chars[i].c == '.') {
            dot_indices.push_back(i);
        }
        else if (flat_chars[i].is_final && i < (flat_chars.size() - 1) && (flat_chars[i + 1].c == '[')) {
            dot_indices.push_back(i);
        }

        char_rects.push_back(flat_chars[i].rect);
        raw_text.push_back(flat_chars[i].c);
    }

    for (int dot_index : dot_indices) {
        if (is_dot_index_end_of_a_reference(flat_chars, dot_index)) {
            end_indices.push_back(dot_index);

            if (dot_index + 1 < flat_chars.size() && (flat_chars[dot_index + 1].c == '[')) {
                bracket_end_indices.push_back(dot_index);
            }

            if (dot_index + 1 < flat_chars.size()){
                int next_index = dot_index + 1;

                // skip phantom spaces
                if (flat_chars[next_index].stext_char == nullptr) next_index++;

                if (next_index == flat_chars.size()) {
                    indented_end_indices.push_back(dot_index);
                }
                else {
                    PagelessDocumentRect dot_line_begin_rect = rect_from_quad(flat_chars[dot_index].stext_line->first_char->quad);
                    PagelessDocumentRect next_line_rect = flat_chars[next_index].rect;
                    float next_x = next_line_rect.center().x;

                    if (next_x < dot_line_begin_rect.x0) {
                        indented_end_indices.push_back(dot_index);
                    }
                    else if (std::abs(next_line_rect.y0 - dot_line_begin_rect.y0) > (5 * next_line_rect.height())) {
                        indented_end_indices.push_back(dot_index);
                    }
                }
            }

        }

    }
    if (bracket_end_indices.size() > 3) {
        end_indices = bracket_end_indices;
    }

    if (indented_end_indices.size() > 3 && indented_end_indices.size() > bracket_end_indices.size()) {
        end_indices = indented_end_indices;
    }

    std::vector<PagelessDocumentRect> res;
    for (int i = 0; i < end_indices.size(); i++) {
        res.push_back(char_rects[end_indices[i]]);
    }

    std::vector<std::wstring> reference_texts;
    if (end_indices.size() == 0) {
        return reference_texts;
    }

    reference_texts.push_back(raw_text.substr(0, end_indices[0]));
    for (int i = 1; i < end_indices.size(); i++) {
        int length = end_indices[i] - end_indices[i - 1];
        reference_texts.push_back(raw_text.substr(end_indices[i-1] + 1, length));
    }

    // try to remove the texts before the first bib item

    int bib_index = -1;

    for (int i = 0; i < std::min<int>(5, reference_texts.size()); i++) {
        int last_bib_index = QString::fromStdWString(reference_texts[i]).toLower().lastIndexOf("bibliography");
        int last_ref_index = QString::fromStdWString(reference_texts[i]).toLower().lastIndexOf("references");
        int len = -1;

        if (last_bib_index != -1) {
            len = std::string("bibligraphy").size();
        }
        if (last_ref_index != -1) {
            len = std::string("references").size();
        }

        int last_index = std::max(last_bib_index, last_ref_index);
        if (last_index != -1) {
            bib_index = i;
            reference_texts[i] = reference_texts[i].substr(last_index + len, reference_texts[i].size() - last_index - len);
            break;

        }
    }

    for (int i = 0; i < bib_index; i++) {
        reference_texts.erase(reference_texts.begin());
    }

    if (out_rects) {

        for (int i = 0; i < end_indices.size(); i++) {
            out_rects->push_back(char_rects[end_indices[i]]);
        }

    }

    return reference_texts;

}

std::optional<std::pair<std::wstring, PagelessDocumentRect>> Document::get_page_bib_with_reference(int page_number, std::wstring reference_text) {
    //todo: use the reference offset as well as the page to more accurately get the reference 

    reference_text = remove_et_al(reference_text);
    std::vector<PagelessDocumentRect> bib_rects;
    std::vector<std::wstring> bib_texts_ = get_page_bib_candidates(page_number, &bib_rects);
    std::vector<std::wstring> bib_text_prefixes;
    for (auto bib : bib_texts_) {
        bib_text_prefixes.push_back(bib.substr(0, reference_text.size() + 5));
    }

    QString reference_text_qstring = QString::fromStdWString(reference_text);
    std::string encoded_reference_text = utf8_encode(reference_text);

    for (int i = 0; i < bib_text_prefixes.size(); i++) {
        if (QString::fromStdWString(bib_text_prefixes[i]).indexOf(reference_text_qstring) != -1) {
            return std::make_pair(bib_texts_[i], bib_rects[i]);
        }
    }

    int score = -1;
    int max_score = encoded_reference_text.size();
    int max_score_index = -1;

    for (int i = 0; i < bib_text_prefixes.size(); i++) {
        std::string encoded_bib_text = utf8_encode(bib_text_prefixes[i]);
        int current_score = lcs(&encoded_bib_text[0], &encoded_reference_text[0], encoded_bib_text.size(), encoded_reference_text.size());
        if (current_score == max_score) {
            return std::make_pair(bib_texts_[i], bib_rects[i]);
        }
        if (current_score > score) {
            score = current_score;
            max_score_index = i;
        }
    }
    if (max_score_index >= 0) {
        return std::make_pair(bib_texts_[max_score_index], bib_rects[max_score_index]);
    }

    return {};
}


void Document::add_freehand_drawing(FreehandDrawing new_drawing) {
    if (new_drawing.points.size() > 0) {
        DocumentPos docpos = absolute_to_page_pos_uncentered(new_drawing.points[0].pos);
        drawings_mutex.lock();
        page_freehand_drawings[docpos.page].push_back(new_drawing);
        drawings_mutex.unlock();
        is_drawings_dirty = true;
    }
}

void Document::undo_freehand_drawing() {
    int most_recent_page_index = -1;
    QDateTime most_recent_page_time;

    drawings_mutex.lock();
    for (auto& [page, drawings] : page_freehand_drawings) {
        if (drawings.size() > 0) {
            if (most_recent_page_index == -1) {
                most_recent_page_index = page;
                most_recent_page_time = drawings[drawings.size() - 1].creattion_time;
            }
            else {
                if (drawings[drawings.size() - 1].creattion_time.secsTo(most_recent_page_time) > 0) {
                    most_recent_page_index = page;
                    most_recent_page_time = drawings[drawings.size() - 1].creattion_time;
                }
            }
        }
    }
    if (most_recent_page_index >= 0) {
        is_drawings_dirty = true;
        page_freehand_drawings[most_recent_page_index].pop_back();
    }
    drawings_mutex.unlock();
}

const std::vector<FreehandDrawing>& Document::get_page_drawings(int page) {
    return page_freehand_drawings[page];
}

std::vector<SelectedObjectIndex> Document::get_page_intersecting_drawing_indices(int page, AbsoluteRect absolute_rect, bool mask[26]) {
    std::vector<SelectedObjectIndex> indices;
    std::vector<FreehandDrawing>& page_drawings = page_freehand_drawings[page];

    for (int i = 0; i < page_drawings.size(); i++) {
        if (!mask[page_drawings[i].type - 'a']) {
            continue;
        }
        for (auto point : page_drawings[i].points) {
            fz_point absolute_point = fz_point{ point.pos.x, point.pos.y };
            if (absolute_rect.contains(point.pos)) {
                indices.push_back(SelectedObjectIndex{ i, SelectedObjectType::Drawing });
                break;
            }
        }
    }
    return indices;

}

void Document::delete_all_page_drawings(int page) {
    page_freehand_drawings[page].clear();
    is_drawings_dirty = true;
}

void Document::delete_all_drawings() {
    page_freehand_drawings.clear();
    is_drawings_dirty = true;
}

void Document::delete_page_intersecting_drawings(int page, AbsoluteRect absolute_rect, bool mask[26]) {
    std::vector<FreehandDrawing>& page_drawings = page_freehand_drawings[page];
    std::vector<SelectedObjectIndex> indices_to_delete = get_page_intersecting_drawing_indices(page, absolute_rect, mask);

    for (int j = indices_to_delete.size() - 1; j >= 0; j--) {
        //page_freehand_drawings.erase(page_freehand_drawings.begin() + indices_to_delete[j]);
        page_freehand_drawings[page].erase(page_freehand_drawings[page].begin() + indices_to_delete[j].index);
    }
    is_drawings_dirty = true;
}

std::wstring Document::get_addtional_sioyek_file_path(QString type) {
    if (ANNOTATIONS_DIR_PATH.size() == 0) {
        Path path = Path(file_name);
        QString filename = QString::fromStdWString(path.filename().value());
        QString drawing_file_name = filename + ".sioyek." + type;
        return path.file_parent().slash(drawing_file_name.toStdWString()).get_path();
    }
    else {
        Path parent_path(ANNOTATIONS_DIR_PATH);
        std::wstring escaped_path = (QString::fromStdWString(file_name).replace("/", "_").replace("\\", "_").replace(":", "_") + ".sioyek." + type).toStdWString();
        return parent_path.slash(escaped_path).get_path();

    }
}

std::wstring Document::get_drawings_file_path() {
    Path path = Path(file_name);
#ifdef SIOYEK_ANDROID
    if (file_name == L":/tutorial.pdf") {
        QString parent_path = QStandardPaths::standardLocations(QStandardPaths::AppDataLocation).at(0);
        return Path(parent_path.toStdWString()).slash(L"tutorial.pdf.sioyek.drawings").get_path();
    }
#endif
    return get_addtional_sioyek_file_path("drawings");
}

std::wstring Document::get_scratchpad_file_path() {
    Path path = Path(file_name);
#ifdef SIOYEK_ANDROID
    if (file_name == L":/tutorial.pdf") {
        QString parent_path = QStandardPaths::standardLocations(QStandardPaths::AppDataLocation).at(0);
        return Path(parent_path.toStdWString()).slash(L"tutorial.pdf.sioyek.scratchpad").get_path();
    }
#endif
    return get_addtional_sioyek_file_path("scratchpad");
}

std::wstring Document::get_extras_file_path() {
    return get_path_extras_file_name(file_name);
}

bool Document::annotations_file_exists() {
    std::wstring file_path = get_annotations_file_path();
    return QFileInfo(QString::fromStdWString(file_path)).exists();
}

bool Document::annotations_file_is_newer_than_database() {
    std::wstring file_path = get_annotations_file_path();
    QFileInfo file_info(QString::fromStdWString(file_path));
    QFileInfo database_file_info(QString::fromStdWString(SHARED_DATABASE_PATH));

    if (file_info.exists() && database_file_info.exists()) {
        return file_info.lastModified() > database_file_info.lastModified();
    }

    return false;
}

std::wstring Document::get_annotations_file_path() {
    Path path = Path(file_name);
#ifdef SIOYEK_ANDROID
    if (file_name == L":/tutorial.pdf") {
        QString parent_path = QStandardPaths::standardLocations(QStandardPaths::AppDataLocation).at(0);
        return Path(parent_path.toStdWString()).slash(L"tutorial.pdf.sioyek.annotations").get_path();
    }
#endif
    return get_addtional_sioyek_file_path("annotations");
}


void Document::load_drawings_async() {
    drawings_mutex.lock();
    auto drawing_load_thread = std::thread([&]() {
        load_drawings();
        });
    drawing_load_thread.detach();
    drawings_mutex.unlock();
}

//void Document::persist_drawings_async() {
//	if (!is_drawings_dirty) {
//		return;
//	}
//
//	drawings_mutex.lock();
//	auto drawing_persist_thread = std::thread([&]() {
//		persist_drawings();
//		});
//	drawing_persist_thread.detach();
//	drawings_mutex.unlock();
//}

void Document::load_annotations(bool sync) {

    //if (document_indexing_thread.has_value() && document_indexing_thread->)
    should_reload_annotations = false;
    if (highlights.size() > 0 && (highlights[0].highlight_rects.size() == 0)) {
        fill_highlight_rects(context, doc);
    }

    if (!are_highlights_loaded || !get_checksum_fast().has_value()) {
        return;
    }

    std::wstring annotation_fille_path = get_annotations_file_path();

    QFile json_file(QString::fromStdWString(annotation_fille_path));
    if (json_file.open(QFile::ReadOnly)) {
        QJsonDocument json_document = QJsonDocument().fromJson(json_file.readAll());
        json_file.close();

        QJsonObject root = json_document.object();
        std::vector<BookMark> file_bookmarks = load_from_json_array<BookMark>(root["bookmarks"].toArray());
        std::vector<Highlight> file_highlights = load_from_json_array<Highlight>(root["highlights"].toArray());
        std::vector<Mark> file_marks = load_from_json_array<Mark>(root["marks"].toArray());
        std::vector<Portal> file_portals = load_from_json_array<Portal>(root["portals"].toArray());

        std::string current_checksum = get_checksum();

        auto portals_array = root["portals"].toArray();

        // set the destination of self-referencing portals to be the current
        // document and not the original doucment
        for (int i = 0; i < portals_array.size(); i++) {
            if (portals_array[i].toObject()["same"].toBool()) {
                file_portals[i].dst.document_checksum = current_checksum;
            }
        }

        std::vector<Annotation*> new_annotations;
        std::vector<Annotation*> updated_annotations;
        std::vector<Annotation*> deleted_annotations;

        std::map<std::string, int> bookmark_index_map = annotation_prism(file_bookmarks, bookmarks, new_annotations, updated_annotations, deleted_annotations);
        std::map<std::string, int> highlight_index_map = annotation_prism(file_highlights, highlights, new_annotations, updated_annotations, deleted_annotations);
        std::map<std::string, int> mark_index_map = annotation_prism(file_marks, marks, new_annotations, updated_annotations, deleted_annotations);
        std::map<std::string, int> portal_index_map = annotation_prism(file_portals, portals, new_annotations, updated_annotations, deleted_annotations);


        for (int i = 0; i < updated_annotations.size(); i++) {
            bool success = db_manager->update_annotation(updated_annotations[i]);
            if (success) {
                if (dynamic_cast<BookMark*>(updated_annotations[i])) {
                    bookmarks[bookmark_index_map[updated_annotations[i]->uuid]] = *dynamic_cast<BookMark*>(updated_annotations[i]);
                }

                if (dynamic_cast<Highlight*>(updated_annotations[i])) {
                    highlights[highlight_index_map[updated_annotations[i]->uuid]] = *dynamic_cast<Highlight*>(updated_annotations[i]);
                    fill_index_highlight_rects(highlight_index_map[updated_annotations[i]->uuid]);
                }

                if (dynamic_cast<Mark*>(updated_annotations[i])) {
                    marks[mark_index_map[updated_annotations[i]->uuid]] = *dynamic_cast<Mark*>(updated_annotations[i]);
                }

                if (dynamic_cast<Portal*>(updated_annotations[i])) {
                    portals[portal_index_map[updated_annotations[i]->uuid]] = *dynamic_cast<Portal*>(updated_annotations[i]);
                }

            }
        }

        for (int i = 0; i < new_annotations.size(); i++) {
            bool success = db_manager->insert_annotation(new_annotations[i], get_checksum());

            if (success) {
                if (dynamic_cast<BookMark*>(new_annotations[i])) {
                    bookmarks.push_back(*dynamic_cast<BookMark*>(new_annotations[i]));
                }

                if (dynamic_cast<Highlight*>(new_annotations[i])) {
                    highlights.push_back(*dynamic_cast<Highlight*>(new_annotations[i]));
                    fill_index_highlight_rects(highlights.size() - 1);
                }

                if (dynamic_cast<Mark*>(new_annotations[i])) {
                    marks.push_back(*dynamic_cast<Mark*>(new_annotations[i]));
                }

                if (dynamic_cast<Portal*>(new_annotations[i])) {
                    portals.push_back(*dynamic_cast<Portal*>(new_annotations[i]));
                }
            }
        }

        if (sync) {
            for (int i = 0; i < deleted_annotations.size(); i++) {
                bool success = db_manager->delete_annotation(deleted_annotations[i]);
            }
            bookmarks = file_bookmarks;
            highlights = file_highlights;
            marks = file_marks;
            portals = file_portals;

            for (int i = 0; i < highlights.size(); i++) {
                fill_index_highlight_rects(i);
            }

        }

    }

}

void Document::load_drawings() {

    std::wstring drawing_file_path = get_drawings_file_path();

    QFile json_file(QString::fromStdWString(drawing_file_path));
    if (json_file.open(QFile::ReadOnly)) {
        QJsonDocument json_document = QJsonDocument().fromJson(json_file.readAll());
        json_file.close();

        QJsonObject root = json_document.object();
        QJsonObject drawings_object = root["drawings"].toObject();
        page_freehand_drawings.clear();
        is_drawings_dirty = false;

        for (auto& page_str : drawings_object.keys()) {
            int page_number = page_str.toInt();
            QJsonArray page_drawings_array = drawings_object[page_str].toArray();
            for (auto val : page_drawings_array) {
                QJsonObject drawing_object = val.toObject();
                FreehandDrawing drawing;

                drawing.creattion_time = QDateTime::fromString(drawing_object["creation_time"].toString(), Qt::ISODate);
                drawing.type = drawing_object["type"].toInt();

                if (drawing_object.contains("alpha")) {
                    drawing.alpha = drawing_object["alpha"].toDouble();
                }

                QJsonArray x_array = drawing_object["point_xs"].toArray();
                QJsonArray y_array = drawing_object["point_ys"].toArray();
                QJsonArray t_array = drawing_object["point_thicknesses"].toArray();

                for (int i = 0; i < x_array.size(); i++) {
                    FreehandDrawingPoint point;
                    point.pos.x = x_array.at(i).toDouble();
                    point.pos.y = y_array.at(i).toDouble();
                    point.thickness = t_array.at(i).toDouble();
                    drawing.points.push_back(point);
                }
                page_freehand_drawings[page_number].push_back(drawing);
            }
        }

    }

}

void Document::persist_annotations(bool force) {

    if ((!is_annotations_dirty) && (!force)) {
        return;
    }

    if (!annotations_file_exists() && (!force)) {
        return;
    }

    std::string current_checksum = get_checksum();

    QJsonArray json_bookmarks = export_array(bookmarks, current_checksum);
    QJsonArray json_highlights = export_array(highlights, current_checksum);
    QJsonArray json_marks = export_array(marks, current_checksum);
    QJsonArray json_portals = export_array(portals, current_checksum);


    QJsonObject book_object;
    book_object["bookmarks"] = json_bookmarks;
    book_object["marks"] = json_marks;
    book_object["highlights"] = json_highlights;
    book_object["portals"] = json_portals;

    QJsonDocument json_document(book_object);

    QFile output_file(QString::fromStdWString(get_annotations_file_path()));
    output_file.open(QFile::WriteOnly);
    output_file.write(json_document.toJson());
    output_file.close();

    is_annotations_dirty = false;
}

void Document::persist_drawings(bool force) {
    if ((!force) && (!is_drawings_dirty)) {
        return;
    }

    std::wstring drawing_file_path = get_drawings_file_path();
    QJsonObject root_object;
    QJsonObject drawings_object;

    for (auto& [page, drawings] : page_freehand_drawings) {
        QJsonArray page_drawings_array;

        for (auto& drawing : drawings) {
            QJsonObject drawing_object;
            drawing_object["creation_time"] = drawing.creattion_time.toString(Qt::ISODate);
            drawing_object["type"] = drawing.type;
            drawing_object["alpha"] = drawing.alpha;
            //QJsonArray points;
            QJsonArray points_xs;
            QJsonArray points_ys;
            QJsonArray points_thicknesses;

            for (auto& p : drawing.points) {
                points_xs.append(p.pos.x);
                points_ys.append(p.pos.y);
                points_thicknesses.append(p.thickness);
            }
            drawing_object["point_xs"] = points_xs;
            drawing_object["point_ys"] = points_ys;
            drawing_object["point_thicknesses"] = points_thicknesses;

            page_drawings_array.append(drawing_object);
        }
        drawings_object[QString::number(page)] = page_drawings_array;
    }
    root_object["drawings"] = drawings_object;


    QJsonDocument json_doc;
    json_doc.setObject(root_object);

    QFile json_file(QString::fromStdWString(drawing_file_path));
    json_file.open(QFile::WriteOnly);
    json_file.write(json_doc.toJson());
    json_file.close();

    is_drawings_dirty = false;
}

std::vector<std::wstring> DocumentManager::get_loaded_document_paths() {
    std::vector<std::wstring> res;

    for (auto& [path, doc] : cached_documents) {
        res.push_back(path);
    }
    return res;
}

std::optional<Document*> DocumentManager::get_cached_document(const std::wstring& path) {
    if (cached_documents.find(path) != cached_documents.end()) {
        return cached_documents[path];
    }
    return {};
}

int Document::find_highlight_index_with_uuid(const std::string& uuid) {
    for (int i = 0; i < highlights.size(); i++) {
        if (highlights[i].uuid == uuid) return i;
    }
    return -1;
}

void Document::update_highlight_add_text_annotation(const std::string& uuid, const std::wstring& text_annot) {
    int highlight_index = find_highlight_index_with_uuid(uuid);
    if (highlight_index > -1) {
        db_manager->update_highlight_add_annotation(uuid, text_annot);
        is_annotations_dirty = true;
        highlights[highlight_index].text_annot = text_annot;
        highlights[highlight_index].update_modification_time();

    }
}

void Document::update_highlight_type(const std::string& uuid, char new_type) {
    db_manager->update_highlight_type(uuid, new_type);
    is_annotations_dirty = true;
}

void Document::update_highlight_type(int index, char new_type) {
    update_highlight_type(highlights[index].uuid, new_type);
    highlights[index].type = new_type;
    highlights[index].update_modification_time();
}

int Document::get_portal_index_at_pos(AbsoluteDocumentPos abspos) {
    for (int i = 0; i < portals.size(); i++) {
        if (portals[i].src_offset_x.has_value()) {
            //if (fz_is_point_inside_rect({abspos.x, abspos.y}, portals[i].get_rectangle())) {
            if (portals[i].get_rectangle().contains(abspos)) {
                return i;
            }
        }
    }
    return -1;
}

int Document::get_bookmark_index_at_pos(AbsoluteDocumentPos abspos) {
    for (int i = 0; i < bookmarks.size(); i++) {
        if (bookmarks[i].begin_y != -1) {
            if (bookmarks[i].end_y == -1) {

                //if (fz_is_point_inside_rect({abspos.x, abspos.y}, bookmarks[i].get_rectangle())) {
                if (bookmarks[i].get_rectangle().contains(abspos)) {
                    return i;
                }
            }
            else {
                AbsoluteRect bookmark_rect;
                bookmark_rect.x0 = bookmarks[i].begin_x;
                bookmark_rect.y0 = bookmarks[i].begin_y;
                bookmark_rect.x1 = bookmarks[i].end_x;
                bookmark_rect.y1 = bookmarks[i].end_y;

                if (fz_is_point_inside_rect({ abspos.x, abspos.y }, bookmark_rect)) {
                    return i;
                }
            }
        }
    }
    return -1;
}

void Document::update_bookmark_text(int index, const std::wstring& new_text, float new_font_size) {
    if ((index >= 0) && (index < bookmarks.size())) {
        if (db_manager->update_bookmark_change_text(bookmarks[index].uuid, new_text, new_font_size)) {
            bookmarks[index].description = new_text;
            bookmarks[index].update_modification_time();
            is_annotations_dirty = true;
        }
    }
}

void Document::update_bookmark_position(int index, AbsoluteDocumentPos new_begin_position, AbsoluteDocumentPos new_end_position) {
    if ((index >= 0) && (index < bookmarks.size())) {
        if (db_manager->update_bookmark_change_position(bookmarks[index].uuid, new_begin_position, new_end_position)) {
            bookmarks[index].y_offset_ = new_begin_position.y;
            bookmarks[index].begin_x = new_begin_position.x;
            bookmarks[index].begin_y = new_begin_position.y;
            bookmarks[index].end_x = new_end_position.x;
            bookmarks[index].end_y = new_end_position.y;
            bookmarks[index].update_modification_time();
            is_annotations_dirty = true;
        }
    }
}

void Document::update_portal_src_position(int index, AbsoluteDocumentPos new_position){
    if ((index >= 0) && (index < portals.size())) {
        if (db_manager->update_portal_change_src_position(portals[index].uuid, new_position)) {
            portals[index].src_offset_x = new_position.x;
            portals[index].src_offset_y = new_position.y;
            portals[index].update_modification_time();
            is_annotations_dirty = true;
        }
    }
}

bool Document::is_bookmark_new(const BookMark& new_bookmark) {
    for (auto bookmark : bookmarks) {
        if (are_same(bookmark, new_bookmark)) {
            return false;
        }
    }
    return true;
}


bool Document::is_highlight_new(const Highlight& new_highlight) {
    for (auto highlight : highlights) {
        if (are_same(highlight, new_highlight)) {
            return false;
        }
    }
    return true;
}

bool Document::is_drawing_new(const FreehandDrawing& drawing) {
    if (drawing.points.size() == 0) return false;

    int drawing_page = absolute_to_page_pos(drawing.points[0].pos).page;
    for (auto page_drawing : page_freehand_drawings[drawing_page]) {
        if (are_same(page_drawing, drawing)) {
            return false;
        }
    }
    return true;

}

bool Document::should_render_pdf_annotations() {
    return should_render_annotations;
}

void Document::set_should_render_pdf_annotations(bool val) {
    should_render_annotations = val;
}

bool Document::get_should_render_pdf_annotations() {
    return should_render_annotations;
}

std::vector<BookMark> Document::get_new_sioyek_bookmarks(const std::vector<BookMark>& pdf_bookmarks) {
    std::vector<BookMark> res;

    for (auto bookmark : bookmarks) {
        bool exists_in_pdf = false;

        for (auto pdf_bookmark : pdf_bookmarks) {
            if (are_same(bookmark, pdf_bookmark)) {
                exists_in_pdf = true;
                break;
            }
        }
        if (!exists_in_pdf) {
            res.push_back(bookmark);
        }
    }
    return res;
}

std::vector<Highlight> Document::get_new_sioyek_highlights(const std::vector<Highlight>& pdf_highlights) {
    std::vector<Highlight> res;

    for (auto highlight : highlights) {
        bool exists_in_pdf = false;

        for (auto pdf_highlight : pdf_highlights) {
            if (are_same(highlight, pdf_highlight)) {
                exists_in_pdf = true;
                break;
            }
        }
        if (!exists_in_pdf) {
            res.push_back(highlight);
        }
    }
    return res;
}
int Document::num_freehand_drawings() {
    int res = 0;

    for (auto [page, drawings] : page_freehand_drawings) {
        res += drawings.size();
    }
    return res;
}

void Document::get_page_freehand_drawings_with_indices(int page, const std::vector<SelectedObjectIndex>& indices, std::vector<FreehandDrawing>&freehand_drawings, std::vector<PixmapDrawing>&pixmap_drawings){
    //std::vector<FreehandDrawing> results;
    const std::vector<FreehandDrawing>& page_drawings = page_freehand_drawings[page];
    for (auto index : indices) {
        freehand_drawings.push_back(page_drawings[index.index]);
        //results.push_back(page_drawings[index]);
    }
    //return results;
}
int Document::get_highlight_index_with_uuid(std::string uuid) {
    for (int i = 0; i < highlights.size(); i++) {
        if (highlights[i].uuid == uuid) {
            return i;
        }
    }
    return -1;
}

int Document::get_bookmark_index_with_uuid(std::string uuid) {
    for (int i = 0; i < bookmarks.size(); i++) {
        if (bookmarks[i].uuid == uuid) {
            return i;
        }
    }
    return -1;
}

std::string Document::get_highlight_index_uuid(int index) {
    if ((index >= 0) && (index < highlights.size())) {
        return highlights[index].uuid;
    }
    return "";
}

std::string Document::get_bookmark_index_uuid(int index) {
    if ((index >= 0) && (index < bookmarks.size())) {
        return bookmarks[index].uuid;
    }
    return "";
}

bool Document::get_should_reload_annotations() {
    return should_reload_annotations;
}


void Document::reload_annotations_on_new_checksum() {
    if (annotations_file_exists()) {
        load_annotations();
    }
    else {
        load_document_metadata_from_db();
        fill_highlight_rects(context, doc);
    }
}
std::vector<Portal>& Document::get_portals() {
    return portals;
}

std::optional<std::wstring> DocumentManager::get_path_from_hash(const std::string& checksum) {
    if (hash_to_path.find(checksum) != hash_to_path.end()) {
        return hash_to_path[checksum];
    }
    std::vector<std::wstring> paths;

    db_manager->get_path_from_hash(checksum, paths);

    if (paths.size() > 0) {
        hash_to_path[checksum] = paths[0];
        return paths[0];
    }

    return {};
}

Document* DocumentManager::get_document_with_checksum(const std::string& checksum) {


    std::optional<std::wstring> path = get_path_from_hash(checksum);
    if (path) {
        return get_document(path.value());
    }
    return nullptr;

    //db_manager->get_path_from_hash();
    //if (hash_to_path.find(checksum) != hash_to_path.end()) {
    //    return get_document(hash_to_path[checksum]);
    //}
}

std::vector<Portal> Document::get_intersecting_visible_portals(float absrange_begin, float absrange_end) {
    std::vector<Portal> res;

    for (auto portal : portals) {
        if (portal.is_visible()) {
            if (portal.src_offset_y >= absrange_begin && portal.src_offset_y <= absrange_end) {
                res.push_back(portal);
            }
        }
    }

    return res;
}

std::optional<DocumentPos> Document::find_abbreviation(std::wstring abbr, std::vector<DocumentRect>& overview_highlight_rects){

    abbr = QString::fromStdWString(abbr).trimmed().toStdWString();
    if (abbr.size() == 0){
        return {};
    }

    std::wstring query = L"(" + abbr + L")";

    auto searcher = std::default_searcher(query.begin(), query.end(), pred_case_sensitive);
    auto it = std::search(
        super_fast_search_index.begin(),
        super_fast_search_index.end(),
        searcher);

    if (it == super_fast_search_index.end()){
        if (abbr.back() == 's'){
            abbr =  abbr.substr(0, abbr.size()-1);
            query = L"(" + abbr + L")";
        }
        else{
            abbr = abbr + L"s";
            query = L"(" + abbr + L")";
        }

        searcher = std::default_searcher(query.begin(), query.end(), pred_case_sensitive);
        it = std::search(
            super_fast_search_index.begin(),
            super_fast_search_index.end(),
            searcher);
    }

    std::vector<int> found_indices;

    if (it != super_fast_search_index.end()) {
        int index = it - super_fast_search_index.begin();
        int abbr_page = 0;

        while ((abbr_page < num_pages() - 1) && super_fast_page_begin_indices[abbr_page + 1] < index) {
            abbr_page++;
        }

        while (index > 0 && is_in(super_fast_search_index[index], {' ', '(', ')', '\n'})){
            index--;
        }

        // remaining_abbr = abbr with dots removed
        std::wstring remaining_abbr;
        for (int i = 0; i < abbr.size(); i++) {
            if (abbr[i] != '.') {
                remaining_abbr.push_back(abbr[i]);
            }
        }
        // if the abbreviation ends with a plural s, remove it
        if (remaining_abbr.size() > 0 && remaining_abbr.back() == 's') {
            remaining_abbr.pop_back();
        }

        std::deque<PagelessDocumentRect> raw_rects;
        std::vector<PagelessDocumentRect> merged_rects;
        CachedPageIndex& abbr_page_index = get_page_index(abbr_page);

        while (index > 0 && remaining_abbr.size() > 0){
            if (
                super_fast_search_index[index] == ' ' ||
                super_fast_search_index[index] == '\n' ||
                super_fast_search_index[index] == '-' ||
                (super_fast_search_index[index] > 127)
                ) {
                if (QChar(super_fast_search_index[index+1]).toLower() == QChar(remaining_abbr.back()).toLower()){
                    remaining_abbr.pop_back();
                    if (remaining_abbr.size() == 0) {
                        break;
                    }
                }
            }

            if (index - super_fast_page_begin_indices[abbr_page] < 0) {
                break;
            }
            PagelessDocumentRect rect = abbr_page_index.rects[index - super_fast_page_begin_indices[abbr_page]];
            /* overview_highlight_rects.push_back(DocumentRect(rect, super_fast_search_index_pages[index])); */
            raw_rects.push_back(rect);

            index--;
        }
        /* while (index > 0 && super_fast_search_index[index]) */

        if (raw_rects.size() > 0){
            merge_selected_character_rects(raw_rects, merged_rects, false);
            for (auto r : merged_rects){
                overview_highlight_rects.push_back(DocumentRect(r, abbr_page));
            }
            return DocumentPos{abbr_page, overview_highlight_rects[0].rect.x0, overview_highlight_rects[0].rect.y0};
        }

        return {};
    }

    return {};
}

int Document::find_reference_page_with_reference_text(std::wstring ref) {

    QStringList parts = QString::fromStdWString(ref).split(QRegularExpression("[ \\w\\(\\);,]"));
    QString largest_part = "";
    for (int i = 0; i < parts.size(); i++) {
        if (parts.at(i).size() > largest_part.size() ) {
            largest_part = parts.at(i);
        }
    }


    std::wstring query = largest_part.toStdWString();
    auto searcher = std::default_searcher(query.begin(), query.end(), pred_case_sensitive);

    std::vector<int> found_indices;

    auto it = std::search(
        super_fast_search_index.begin(),
        super_fast_search_index.end(),
        searcher);

    for (; it != super_fast_search_index.end(); it = std::search(it + 1, super_fast_search_index.end(), searcher)) {
        int index = it - super_fast_search_index.begin();
        found_indices.push_back(index);
    }

    std::vector<int> filtered_indices;

    for (auto index : found_indices) {
        const int context_prev_size = 50;
        const int context_next_size = 400;
        int context_first = std::max(index - context_prev_size, 0);
        int context_last = std::min(index + context_next_size, static_cast<int>(super_fast_search_index.size()-1));
        bool found_all = true;
        std::wstring context_substring = super_fast_search_index.substr(context_first, context_last - context_first);

        for (int i = 0; i < parts.size(); i++) {
            if (parts[i].size() < 4 && parts[i].startsWith("et")) {
                continue;
            }
            if (parts[i].size() < 5 && parts[i].startsWith("al")) {
                continue;
            }

            if (parts[i].size() > 2) {
                std::wstring subquery = parts[i].toStdWString();
                auto subsearcher = std::default_searcher(subquery.begin(), subquery.end(), pred_case_sensitive);
                auto last = super_fast_search_index.begin() + context_last + 1;
                if (std::search(super_fast_search_index.begin() + context_first, last, subsearcher) == last) {
                    found_all = false;
                    break;
                }
            }
        }
        if (found_all) {
            filtered_indices.push_back(index);
        }
    }
    if (filtered_indices.size() > 0) {

        int res_page = 0;
        while ((res_page < super_fast_page_begin_indices.size() - 1) && super_fast_page_begin_indices[res_page + 1] < filtered_indices.back()) {
            res_page++;
        }

        return res_page;
    }
    return -1;

}

QJsonArray Document::get_bookmarks_json() {
    return export_array(bookmarks, get_checksum());
}

QJsonArray Document::get_highlights_json() {
    return export_array(highlights, get_checksum());
}

QJsonArray Document::get_portals_json() {
    return export_array(portals, get_checksum());
}

QJsonArray Document::get_marks_json() {
    return export_array(portals, get_checksum());
}

int DocumentManager::get_tab_index(const std::wstring& path) {
    auto found = std::find(tabs.begin(), tabs.end(), path);
    if (found != tabs.end()) {
        return found - tabs.begin();
    }
    return -1;
}


int DocumentManager::add_tab(const std::wstring& path) {
    int tab_index = get_tab_index(path);
    if (tab_index == -1) {
        tabs.push_back(path);
    }
    return tab_index;
}

void DocumentManager::remove_tab(const std::wstring& path) {
    int index = get_tab_index(path);
    if (index != -1) {
        tabs.erase(tabs.begin() + index);
    }
}

std::vector<std::wstring> DocumentManager::get_tabs() {
    return tabs;
}

std::wstring Document::detect_paper_name() {
    return detect_paper_name(context, doc);
}

std::wstring Document::detect_paper_name(fz_context* context, fz_document* doc) {

    if (detected_paper_name.size() > 0) return detected_paper_name;

    fz_stext_page* stext_page = get_stext_with_page_number(context, 0, doc);
    if (stext_page) {

        //std::wstring max_block_text = L"";
        fz_stext_block* max_block = nullptr;
        float max_block_area = -1;

        LL_ITER(block, stext_page->first_block) {

            if (block->type == FZ_STEXT_BLOCK_TEXT) {
                std::wstring block_text;
                int num_chars_in_block = 0;
                float cum_block_char_volumes = 0;

                LL_ITER(line, block->u.t.first_line) {
                    LL_ITER(ch, line->first_char) {
                        if (ch->c < 128 && ch->c > 0) {
                            block_text.push_back(ch->c);
                            num_chars_in_block++;
                            PagelessDocumentRect char_rect = rect_from_quad(ch->quad);
                            float area = rect_area(char_rect);
                            cum_block_char_volumes += area;
                        }
                    }
                }

                //if (QString::fromStdWString(block_text).startsWith("arXiv")) continue;
                if (is_block_vertical(block)) continue;

                float average_area = cum_block_char_volumes / num_chars_in_block;
                if (num_chars_in_block > 10) {
                    if (average_area > max_block_area) {
                        max_block_area = average_area;
                        max_block = block;
                        //max_block_text = block_text;
                    }
                }
            }
        }
        if (max_block) {
            return get_string_from_stext_block(max_block, true);
        }
        else {
            char buffer[1000];
            fz_lookup_metadata(context, doc, FZ_META_INFO_TITLE, buffer, 1000);
            return utf8_decode(buffer);
        }
    }
    return L"";
}

PageIterator Document::page_iterator(int page_number) {
    fz_stext_page* page = get_stext_with_page_number(page_number);
    return PageIterator(page);
}

int Document::get_page_text_and_line_rects_after_rect(int page_number,
    AbsoluteRect after_,
    std::wstring& text,
    std::vector<PagelessDocumentRect>& line_rects,
    std::vector<PagelessDocumentRect>& char_rects){
    int index_into_page = 0;

    bool begun = false;
    DocumentRect after = after_.to_document(this);
    after.rect.y0 = after.rect.y1 = (after.rect.y0 + after.rect.y1) / 2;

    if (after.rect == fz_empty_rect) {
        begun = true;
    }

    for (auto [block, line, chr] : page_iterator(page_number)) {

        if (rects_intersect(after.rect, line->bbox)) {
            begun = true;
        }

        if (!begun) {
            index_into_page++;
        }

        if (chr->c > 0 && chr->c < 128) {
            if (begun) {
                if ((chr->next == nullptr) && (chr->c == '-')) continue;
                text.push_back(chr->c);

                line_rects.push_back(line->bbox);
                char_rects.push_back(fz_rect_from_quad(chr->quad));

                if ((chr->next == nullptr)) {
                    text.push_back(' ');

                    line_rects.push_back(line->bbox);
                    char_rects.push_back(fz_rect_from_quad(chr->quad));

                    if (line->next == nullptr) {
                        text.push_back('\n');

                        line_rects.push_back(line->bbox);
                        char_rects.push_back(fz_rect_from_quad(chr->quad));
                    }
                }
            }
        }

    }
    return index_into_page;
}

std::optional<AbsoluteRect> Document::get_rect_vertically(bool below, AbsoluteRect rect) {
    DocumentRect doc_rect = rect.to_document(this);
    if (doc_rect.page < 0) return {};

    float closest_distance = 100000;
    std::optional<DocumentRect> closest_rect = {};
    float page_rect_x = static_cast<float>(doc_rect.rect.center().x);

    for (auto [block, line, ch] : page_iterator(doc_rect.page)) {
        float h = std::abs(ch->quad.ul.y - ch->quad.ll.y);
        float current_y = below ? ch->quad.ll.y : ch->quad.ul.y;
        float threshold_y = below ? (doc_rect.rect.y1 + h / 2) : (doc_rect.rect.y0 - h / 2);
        bool threshold_reached = below ? (current_y > threshold_y) : (current_y < threshold_y);

        if (threshold_reached) {
            float distance = std::abs(ch->quad.lr.y - doc_rect.rect.y1) + std::abs(ch->quad.lr.x - page_rect_x);
            if (distance < closest_distance) {
                closest_distance = distance;
                closest_rect = DocumentRect(rect_from_quad(ch->quad), doc_rect.page);
            }
        }

    }
    if (closest_rect) {
        return closest_rect->to_absolute(this);
    }
    return {};
}

AbsoluteRect Document::to_absolute(int page, fz_quad quad) {
    return DocumentRect(rect_from_quad(quad), page).to_absolute(this);
}

AbsoluteRect Document::to_absolute(int page, PagelessDocumentRect rect) {
    return DocumentRect(rect, page).to_absolute(this);
}

fz_context* Document::get_mupdf_context(){
    return context;
}

CachedPageIndex& Document::get_page_index(int page) {
    for (auto& [pn, cached] : cached_page_index) {
        if (pn == page) return cached;
    }

    if (cached_page_index.size() > 20) {
        cached_page_index.erase(cached_page_index.begin(), cached_page_index.begin() + 10);
    }

    CachedPageIndex index;
    std::vector<int> dummy_pages;

    std::vector<fz_stext_char*> flat_chars;
    fz_stext_page* stext_page = get_stext_with_page_number(page);
    get_flat_chars_from_stext_page(stext_page, flat_chars);
    flat_char_prism(flat_chars, page, index.text, dummy_pages, index.rects);

    cached_page_index.push_back(std::make_pair(page, index));
    return cached_page_index.back().second;
}

void Document::fill_search_result(SearchResult* result) {
    if (result->rects.size() > 0) return;

    int page = result->page;
    CachedPageIndex& page_index = get_page_index(page);

    int begin_index = result->begin_index_in_page;
    int end_index = result->end_index_in_page;

    std::deque<fz_rect> raw_rects;
    std::vector<fz_rect> compressed_rects;
    end_index = std::min<int>(end_index, page_index.rects.size());

    for (int i = begin_index; i < end_index; i++) {
        raw_rects.push_back(page_index.rects[i]);
    }

    merge_selected_character_rects(raw_rects, compressed_rects);
    result->rects = compressed_rects;
}

int Document::get_page_offset_into_super_fast_index(int from){
    return super_fast_page_begin_indices[from];
}

QString Document::get_rest_of_document_pages_text(int from) {
    if ((from >= 0) && from < super_fast_page_begin_indices.size()) {
        return QString::fromStdWString(super_fast_search_index.substr(super_fast_page_begin_indices[from]));
    }
    return "";
}

int Document::get_page_from_character_offset(int offset){
    int res = 0;
    while ((res < super_fast_page_begin_indices.size() - 1) && (super_fast_page_begin_indices[res + 1] < offset)){
        res++;
    }
    return res;
}

bool Document::load_extras() {
    QString extras_file_path = QString::fromStdWString(get_extras_file_path());
    QFileInfo extras_file_info = QFileInfo(extras_file_path);
    if (extras_file_info.exists()) {
        QFile extras_file = QFile(extras_file_path);
        if (extras_file.open(QIODevice::ReadOnly)) {
            QByteArray data = extras_file.readAll();
            QJsonDocument json_doc = QJsonDocument::fromJson(data);
            extras = json_doc.object();
            extras_file.close();
            return true;
        }
    }
    return false;

}

bool Document::persist_extras() {
    QString extras_file_path = QString::fromStdWString(get_extras_file_path());
    QFile extras_file = QFile(extras_file_path);
    if (!extras.isEmpty()) {
        if (extras_file.open(QIODevice::WriteOnly)) {
            QJsonDocument doc;
            doc.setObject(extras);
            QByteArray data = doc.toJson();
            extras_file.write(data);
            extras_file.close();
            return true;
        }
    }

    return false;
}
std::optional<QVariant> Document::get_extra(QString name) {
    if (extras.find(name) != extras.end()) {
        return extras[name].toVariant();
    }
    return {};
}

std::wstring Document::get_detected_paper_name_if_exists() {
    return detected_paper_name;
}


std::wstring& Document::get_super_fast_search_index() {
    return super_fast_search_index;
}

std::vector<int>& Document::get_super_fast_search_page_indices() {
    return super_fast_page_begin_indices;
}


fz_stext_char* Document::get_next_char_after_selection(int page_number, fz_point document_point){
    fz_stext_page* stext_page = get_stext_with_page_number(context, page_number, doc);

    std::vector<fz_stext_char*> flat_chars;
    get_flat_chars_from_stext_page(stext_page, flat_chars);

// fz_stext_char* find_closest_char_to_document_point(const std::vector<fz_stext_char*> flat_chars, fz_point document_point, int* location_index) {
    int location_index = -1;
    fz_stext_char* char_begin = find_closest_char_to_document_point(flat_chars, document_point, &location_index);
    if (char_begin) {

        if (location_index + 1 < flat_chars.size()) {
            fz_stext_char* next_char = flat_chars[location_index + 1];
            return next_char;
        }
    }
    return nullptr;
}
