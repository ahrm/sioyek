#include "config.h"
#include "utils.h"
#include <cassert>
#include <map>
#include <qdir.h>
//#include <ui.h>

int FONT_SIZE = -1;
int STATUS_BAR_FONT_SIZE = -1;
float BACKGROUND_COLOR[3] = { 0.97f, 0.97f, 0.97f };
float DARK_MODE_BACKGROUND_COLOR[3] = { 0.0f, 0.0f, 0.0f };
float CUSTOM_COLOR_MODE_EMPTY_BACKGROUND_COLOR[3] = { 0.0f, 0.0f, 0.0f };
float RULER_COLOR[3] = { 0.5f, 0.5f, 0.5f };
float RULER_MARKER_COLOR[3] = { 1.0f, 0.0f, 0.0f };
float CUSTOM_BACKGROUND_COLOR[3] = { 0.18f, 0.204f, 0.251f };
float CUSTOM_TEXT_COLOR[3] = { 0.847f, 0.871f, 0.914f };
float STATUS_BAR_COLOR[3] = { 0.0f, 0.0f, 0.0f };
float STATUS_BAR_TEXT_COLOR[3] = { 1.0f, 1.0f, 1.0f };
float UI_TEXT_COLOR[3] = { 1.0f, 1.0f, 1.0f };
float UI_BACKGROUND_COLOR[3] = { 0.0f, 0.0f, 0.0f };
float UI_SELECTED_TEXT_COLOR[3] = { 0.0f, 0.0f, 0.0f };
float UI_SELECTED_BACKGROUND_COLOR[3] = { 1.0f, 1.0f, 1.0f };
float UNSELECTED_SEARCH_HIGHLIGHT_COLOR[3] = { 0.0f, 0.5f, 0.5f };
float GAMMA = 1.0f;
bool DEBUG_DISPLAY_FREEHAND_POINTS = false;
bool DEBUG_SMOOTH_FREEHAND_DRAWINGS = true;
bool ADD_NEWLINES_WHEN_COPYING_TEXT = false;
bool ALWAYS_COPY_SELECTED_TEXT = false;
bool SHOW_STATUSBAR_ONLY_WHEN_MOUSE_OVER = false;
float PERSISTANCE_PERIOD = -1.0f;

#ifdef SIOYEK_MOBILE
bool TOUCH_MODE = true;
#else
bool TOUCH_MODE = false;
#endif

#ifdef SIOYEK_MOBILE
bool SLICED_RENDERING = true;
#else
bool SLICED_RENDERING = false;
#endif
int NUM_V_SLICES = 5;
int NUM_H_SLICES = 1;
bool SHOULD_RENDER_PDF_ANNOTATIONS = true;
bool AUTOMATICALLY_DOWNLOAD_MATCHING_PAPER_NAME = true;
bool NO_AUTO_CONFIG = false;
bool USE_RULER_TO_HIGHLIGHT_SYNCTEX_LINE = true;
bool HIDE_OVERLAPPING_LINK_LABELS = true;
bool DONT_FOCUS_IF_SYNCTEX_RECT_IS_VISIBLE = false;
bool GG_USES_LABELS = false;

// when the window rect size is not an exact integer, mupdf rounds up and 
// this can cause a pixel strip on documents with dark background color
// this is a workaround for which basically doesn't render the last pixel strip
bool BACKGROUND_PIXEL_FIX = false;

std::wstring SEARCH_URLS[26];
std::wstring EXECUTE_COMMANDS[26];
std::wstring TEXT_HIGHLIGHT_URL = L"http://localhost:5000/";
std::wstring PAPER_SEARCH_URL = L"https://search.fatcat.wiki/fatcat_release/_search?q=%{query}";

std::wstring PAPER_SEARCH_URL_PATH = L"hits.hits[]._source.best_pdf_url";
std::wstring PAPER_SEARCH_TILE_PATH = L"hits.hits[]._source.title";
std::wstring PAPER_SEARCH_CONTRIB_PATH = L"hits.hits[]._source.contrib_names";

std::wstring MIDDLE_CLICK_SEARCH_ENGINE = L"s";
std::wstring SHIFT_MIDDLE_CLICK_SEARCH_ENGINE = L"l";
std::wstring PAPERS_FOLDER_PATH = L"";
#ifndef SIOYEK_MOBILE
std::wstring STATUS_BAR_FORMAT = L"[ %{current_page} / %{num_pages} ]%{chapter_name}%{search_results}%{search_progress}%{link_status}%{waiting_for_symbol}%{indexing}%{preview_index}%{synctex}%{drag}%{presentation}%{visual_scroll}%{locked_scroll}%{highlight}%{freehand_drawing}%{closest_bookmark}%{close_portal}%{rect_select}%{custom_message}%{download}";
std::wstring RIGHT_STATUS_BAR_FORMAT = L"";
#else
std::wstring STATUS_BAR_FORMAT = L"# %{current_page} / %{num_pages}%{search_results}%{search_progress}%{link_status}%{indexing}%{current_requirement_desc}";
std::wstring RIGHT_STATUS_BAR_FORMAT = L"%{auto_name}";
#endif

float BLACK_COLOR[3] = { 0.0f, 0.0f, 0.0f };
float HIGHLIGHT_COLORS[26 * 3] = { \
0.94, 0.64, 1.00, \
0.00, 0.46, 0.86, \
0.60, 0.25, 0.00, \
0.30, 0.00, 0.36, \
0.10, 0.10, 0.10, \
0.00, 0.36, 0.19, \
0.17, 0.81, 0.28, \
1.00, 0.80, 0.60, \
0.50, 0.50, 0.50, \
0.58, 1.00, 0.71, \
0.56, 0.49, 0.00, \
0.62, 0.80, 0.00, \
0.76, 0.00, 0.53, \
0.00, 0.20, 0.50, \
1.00, 0.64, 0.02, \
1.00, 0.66, 0.73, \
0.26, 0.40, 0.00, \
1.00, 0.00, 0.06, \
0.37, 0.95, 0.95, \
0.00, 0.60, 0.56, \
0.88, 1.00, 0.40, \
0.45, 0.04, 1.00, \
0.60, 0.00, 0.00, \
1.00, 1.00, 0.50, \
1.00, 1.00, 0.00, \
1.00, 0.31, 0.02
};

float DARK_MODE_CONTRAST = 0.8f;
float ZOOM_INC_FACTOR = 1.2f;
float SCROLL_ZOOM_INC_FACTOR = 1.2f;
float VERTICAL_MOVE_AMOUNT = 1.0f;
float HORIZONTAL_MOVE_AMOUNT = 1.0f;
float MOVE_SCREEN_PERCENTAGE = 0.5f;
unsigned int CACHE_INVALID_MILIES = 1000;
int PERSIST_MILIES = 1000 * 60;
int PAGE_PADDINGS = 0;
int MAX_PENDING_REQUESTS = 31;
bool FLAT_TABLE_OF_CONTENTS = false;
bool SHOULD_USE_MULTIPLE_MONITORS = false;
bool SHOULD_CHECK_FOR_LATEST_VERSION_ON_STARTUP = false;
bool DEFAULT_DARK_MODE = false;
bool USE_SYSTEM_THEME = false;
bool USE_CUSTOM_COLOR_FOR_DARK_SYSTEM_THEME = false;
bool SORT_BOOKMARKS_BY_LOCATION = true;
bool SORT_HIGHLIGHTS_BY_LOCATION = true;
std::wstring LIBGEN_ADDRESS = L"";
std::wstring GOOGLE_SCHOLAR_ADDRESS = L"";
std::wstring INVERSE_SEARCH_COMMAND = L"";
std::wstring SHARED_DATABASE_PATH = L"";
std::wstring BOOK_SCAN_PATH = L"";
std::wstring UI_FONT_FACE_NAME = L"";
std::wstring STATUS_FONT_FACE_NAME = L"";
std::wstring DEFAULT_OPEN_FILE_PATH = L"";
std::wstring ANNOTATIONS_DIR_PATH = L"";
bool SHOULD_LOAD_TUTORIAL_WHEN_NO_OTHER_FILE = true;
bool SHOULD_LAUNCH_NEW_INSTANCE = false;
bool SHOULD_LAUNCH_NEW_WINDOW = false;
bool SHOULD_DRAW_UNRENDERED_PAGES = false;
bool PRESERVE_IMAGE_COLORS = false;
bool INVERTED_PRESERVED_IMAGE_COLORS = false;
bool HOVER_OVERVIEW = false;
bool RERENDER_OVERVIEW = true;
bool LINEAR_TEXTURE_FILTERING = false;
bool RULER_MODE = true;
bool SMALL_TOC = false;
bool WHEEL_ZOOM_ON_CURSOR = false;
bool TEXT_SUMMARY_HIGHLIGHT_SHOULD_REFINE = true;
bool TEXT_SUMMARY_HIGHLIGHT_SHOULD_FILL = true;
bool USE_HEURISTIC_IF_TEXT_SUMMARY_NOT_AVAILABLE = false;
int TEXT_SUMMARY_CONTEXT_SIZE = 49;
float VISUAL_MARK_NEXT_PAGE_FRACTION = 0.75;
float VISUAL_MARK_NEXT_PAGE_THRESHOLD = 0.25f;
float MENU_SCREEN_WDITH_RATIO = 0.9f;
float RULER_PADDING = 1.0f;
float RULER_X_PADDING = 5.0f;
std::wstring ITEM_LIST_PREFIX = L">";
float STRIKE_LINE_WIDTH = 1.0f;
int RULER_UNDERLINE_PIXEL_WIDTH = 2;
bool AUTO_RENAME_DOWNLOADED_PAPERS = false;
bool SHOW_MOST_RECENT_COMMANDS_FIRST = true;
bool ALLOW_HORIZONTAL_DRAG_WHEN_DOCUMENT_IS_SMALL = false;
bool INVERT_SELECTED_TEXT = false;
bool IGNORE_SCROLL_EVENTS = false;

#ifdef SIOYEK_MOBILE
std::wstring STARTUP_COMMANDS = L"toggle_mouse_drag_mode;toggle_fullscreen";
#else
std::wstring STARTUP_COMMANDS = L"";
#endif

bool ALIGN_LINK_DEST_TO_TOP = false;
int MAX_TAB_COUNT = 100;
float SMALL_PIXMAP_SCALE = 0.75f;
float DISPLAY_RESOLUTION_SCALE = -1;
float FIT_TO_PAGE_WIDTH_RATIO = 0.75;
int MAIN_WINDOW_SIZE[2] = { -1, -1 };
int HELPER_WINDOW_SIZE[2] = { -1, -1 };
int MAIN_WINDOW_MOVE[2] = { -1, -1 };
int HELPER_WINDOW_MOVE[2] = { -1, -1 };
float TOUCHPAD_SENSITIVITY = 1.0f;
int SINGLE_MAIN_WINDOW_SIZE[2] = { -1, -1 };
int SINGLE_MAIN_WINDOW_MOVE[2] = { -1, -1 };
bool ENABLE_EXPERIMENTAL_FEATURES = false;
bool CREATE_TABLE_OF_CONTENTS_IF_NOT_EXISTS = true;
int MAX_CREATED_TABLE_OF_CONTENTS_SIZE = 5000;
bool FORCE_CUSTOM_LINE_ALGORITHM = false;
float OVERVIEW_SIZE[2] = { 0.8f, 0.4f };
float OVERVIEW_OFFSET[2] = { 0.0f, 0.0f };
bool IGNORE_WHITESPACE_IN_PRESENTATION_MODE = false;
#ifdef SIOYEK_MOBILE
bool EXACT_HIGHLIGHT_SELECT = true;
#else
bool EXACT_HIGHLIGHT_SELECT = false;
#endif
bool SHOW_DOC_PATH = false;
float FASTREAD_OPACITY = 0.5f;
bool SHOULD_WARN_ABOUT_USER_KEY_OVERRIDE = true;
bool SINGLE_CLICK_SELECTS_WORDS = false;
bool USE_LEGACY_KEYBINDS = false;
bool MULTILINE_MENUS = true;
bool START_WITH_HELPER_WINDOW = false;
std::map<std::wstring, std::wstring> ADDITIONAL_COMMANDS;
bool LIGHTEN_COLORS_WHEN_EMBEDDING_ANNOTATIONS = true;


std::map<std::wstring, JsCommandInfo> ADDITIONAL_JAVASCRIPT_COMMANDS;
std::map<std::wstring, JsCommandInfo> ADDITIONAL_ASYNC_JAVASCRIPT_COMMANDS;
std::map<std::wstring, std::wstring> ADDITIONAL_MACROS;
std::vector<AdditionalKeymapData> ADDITIONAL_KEYMAPS;
bool PRERENDER_NEXT_PAGE = true;
bool HIGHLIGHT_MIDDLE_CLICK = false;
float HYPERDRIVE_SPEED_FACTOR = 10.0f;
float SMOOTH_SCROLL_SPEED = 3.0f;
float SMOOTH_SCROLL_DRAG = 3000.0f;
int PRERENDERED_PAGE_COUNT = 0;
bool SHOW_RIGHT_CLICK_CONTEXT_MENU = false;
bool ALLOW_MAIN_VIEW_SCROLL_WHILE_IN_OVERVIEW = false;
std::wstring CONTEXT_MENU_ITEMS = L"";
std::wstring CONTEXT_MENU_ITEMS_FOR_SELECTED_TEXT = L"copy|add_highlight(a)|add_highlight(b)|add_highlight(c)";
std::wstring CONTEXT_MENU_ITEMS_FOR_LINKS = L"";
std::wstring CONTEXT_MENU_ITEMS_FOR_HIGHLIGHTS = L"delete_highlight|edit_selected_highlight|add_highlight(a)|add_highlight(b)|add_highlight(c)";
std::wstring CONTEXT_MENU_ITEMS_FOR_BOOKMARKS = L"delete_visible_bookmark|edit_selected_bookmark|move_selected_bookmark";
std::wstring CONTEXT_MENU_ITEMS_FOR_OVERVIEW = L"";

bool RIGHT_CLICK_CONTEXT_MENU = false;
#ifdef SIOYEK_MOBILE
int NUM_CACHED_PAGES = 3;
#else
int NUM_CACHED_PAGES = 5;
#endif

float PAGE_SEPARATOR_WIDTH = 0.0f;
float PAGE_SEPARATOR_COLOR[3] = { 0.9f, 0.9f, 0.9f };
bool SUPER_FAST_SEARCH = true;
bool INCREMENTAL_SEARCH = false;
bool SHOW_CLOSEST_BOOKMARK_IN_STATUSBAR = false;
bool SHOW_CLOSE_PORTAL_IN_STATUSBAR = false;
bool CASE_SENSITIVE_SEARCH = false;
bool SMARTCASE_SEARCH = false;
bool SHOW_DOCUMENT_NAME_IN_STATUSBAR = false;
bool NUMERIC_TAGS = false;
bool SHOULD_HIGHLIGHT_LINKS = false;
bool SHOULD_HIGHLIGHT_UNSELECTED_SEARCH = false;
int KEYBOARD_SELECT_FONT_SIZE = 20;
int DOCUMENTATION_FONT_SIZE = 16;
bool FUZZY_SEARCHING = false;
bool INVERTED_HORIZONTAL_SCROLLING = false;
bool TOC_JUMP_ALIGN_TOP = false;
float CUSTOM_COLOR_CONTRAST = 0.5f;
float HIGHLIGHT_DELETE_THRESHOLD = 0.1f;
float SCROLL_VIEW_SENSITIVITY = 1.0f;
float KEYBOARD_SELECT_BACKGROUND_COLOR[] = { 0.9f , 0.75f, 0.0f, 1.0f };
float KEYBOARD_SELECT_TEXT_COLOR[] = { 0.0f , 0.0f, 0.5f, 1.0f };
float KEYBOARD_SELECTED_TAG_TEXT_COLOR[] = { 1.0f , 1.0f, 1.0f, 1.0f };
float KEYBOARD_SELECTED_TAG_BACKGROUND_COLRO[] = { 0.0f , 0.0f, 0.0f, 1.0f };

bool AUTOCENTER_VISUAL_SCROLL = false;
bool ALPHABETIC_LINK_TAGS = false;
bool VIMTEX_WSL_FIX = false;
float RULER_AUTO_MOVE_SENSITIVITY = 40.0f;
float TTS_RATE = 0.0f;
bool VERBOSE = true;
bool FILL_TEXTBAR_WITH_SELECTED_TEXT = true;
int NUM_PRERENDERED_NEXT_SLIDES = 1;
int NUM_PRERENDERED_PREV_SLIDES = 0;
float SMOOTH_MOVE_MAX_VELOCITY = 1500;
//float SMOOTH_MOVE_INITIAL_VELOCITY = 1500;
float PAGE_SPACE_X = 10.0f;
float PAGE_SPACE_Y = 10.0f;
bool USE_KEYBOARD_POINT_SELECTION = false;
std::wstring TAG_FONT_FACE = L"";
//UIRect TEST_UI_RECT = {true, -0.1f, 0.1f, -0.1f, 0.1f};

bool PAPER_DOWNLOAD_CREATE_PORTAL = true;
bool PAPER_DOWNLOAD_AUTODETECT_PAPER_NAME = true;
float DEFAULT_TEXT_HIGHLIGHT_COLOR[3] = { 1.0f, 1.0f, 0.0 };
float DEFAULT_VERTICAL_LINE_COLOR[4] = { 0.0f, 0.0f, 0.0f, 0.1f };
float DEFAULT_SEARCH_HIGHLIGHT_COLOR[3] = { 0.0f, 1.0f, 0.0f };
float DEFAULT_LINK_HIGHLIGHT_COLOR[3] = { 0.0f, 0.0f, 1.0f };
float DEFAULT_SYNCTEX_HIGHLIGHT_COLOR[3] = { 1.0f, 0.0f, 0.0f };
float HIDE_SYNCTEX_HIGHLIGHT_TIMEOUT = 1.0f;

float FREETEXT_BOOKMARK_COLOR[3] = { 0.0f, 0.0f, 0.0f };
float FREETEXT_BOOKMARK_FONT_SIZE = 8.0f;
bool RENDER_FREETEXT_BORDERS = false;
bool REAL_PAGE_SEPARATION = false;

float EPUB_WIDTH = 400;
float EPUB_HEIGHT = 700;
float EPUB_FONT_SIZE = 14;
float EPUB_LINE_SPACING = 2.0f;
int RELOAD_INTERVAL_MILISECONDS = 200;
bool ADJUST_ANNOTATION_COLORS_FOR_DARK_MODE = true;

#ifdef Q_OS_MACOS
float MACOS_TITLEBAR_COLOR[3] = { -1.0f, -1.0f, -1.0f };
bool MACOS_HIDE_TITLEBAR = false;
#endif

std::wstring RULER_DISPLAY_MODE = L"underline";
std::wstring EPUB_CSS = L"";
QString EPUB_TEMPLATE = "p {\
line-height: %{line_spacing}em!important;\
}";

UIRect PORTRAIT_EDIT_PORTAL_UI_RECT = { true, -0.2f, 0.2f, -1.0f, -0.7f };
UIRect LANDSCAPE_EDIT_PORTAL_UI_RECT = { true, -0.2f, 0.2f, -1.0f, -0.7f };

UIRect PORTRAIT_MIDDLE_LEFT_UI_RECT = { true, -1.0f, -0.7f, -0.3f, 0.3f };
UIRect PORTRAIT_MIDDLE_RIGHT_UI_RECT = { true, 0.7f, 1.0f, -0.3f, 0.3f };
UIRect LANDSCAPE_MIDDLE_LEFT_UI_RECT = { true, -1.0f, -0.7f, -0.3f, 0.3f };
UIRect LANDSCAPE_MIDDLE_RIGHT_UI_RECT = { true, 0.7f, 1.0f, -0.3f, 0.3f };

UIRect PORTRAIT_BACK_UI_RECT = { true, -1.0f, -0.7f, -1.0f, -0.7f };
UIRect PORTRAIT_FORWARD_UI_RECT = { true, 0.7f, 1.0f, -1.0f, -0.7 };
UIRect LANDSCAPE_BACK_UI_RECT = { true, -1.0f, -0.7f, -1.0f, -0.7f };
UIRect LANDSCAPE_FORWARD_UI_RECT = { true, 0.7f, 1.0f, -1.0f, -0.7 };

UIRect PORTRAIT_VISUAL_MARK_PREV = { true, -1.0f, -0.7f, 0.7f, 1.0f };
UIRect PORTRAIT_VISUAL_MARK_NEXT = { true, 0.7f, 1.0f, 0.7f, 1.0f };
UIRect LANDSCAPE_VISUAL_MARK_PREV = { true, -1.0f, -0.7f, 0.7f, 1.0f };
UIRect LANDSCAPE_VISUAL_MARK_NEXT = { true, 0.7f, 1.0f, 0.7f, 1.0f };
float BOOKMARK_RECT_SIZE = 8.0f;

std::wstring RESIZE_COMMAND = L"";
std::wstring SHIFT_CLICK_COMMAND = L"overview_under_cursor";
std::wstring CONTROL_CLICK_COMMAND = L"smart_jump_under_cursor";
std::wstring RIGHT_CLICK_COMMAND = L"";
std::wstring MIDDLE_CLICK_COMMAND = L"";
std::wstring SHIFT_RIGHT_CLICK_COMMAND = L"";
std::wstring CONTROL_RIGHT_CLICK_COMMAND = L"";
std::wstring ALT_CLICK_COMMAND = L"";
std::wstring ALT_RIGHT_CLICK_COMMAND = L"";
std::wstring HOLD_MIDDLE_CLICK_COMMAND = L"download_paper_under_cursor";
std::wstring TABLET_PEN_CLICK_COMMAND = L"[s]show_touch_draw_controls;[r]move_visual_mark_next";
//std::wstring TABLET_PEN_CLICK_COMMAND = L"[s]show_touch_main_menu;[r]move_visual_mark_next";
//std::wstring TABLET_PEN_DOUBLE_CLICK_COMMAND = L"toggle_scratchpad_mode";
std::wstring TABLET_PEN_DOUBLE_CLICK_COMMAND = L"load_scratchpad";
std::wstring VOLUME_DOWN_COMMAND = L"";
std::wstring VOLUME_UP_COMMAND = L"";

std::wstring BACK_RECT_TAP_COMMAND = L"prev_state";
std::wstring BACK_RECT_HOLD_COMMAND = L"goto_mark";
std::wstring FORWARD_RECT_TAP_COMMAND = L"next_state";
std::wstring FORWARD_RECT_HOLD_COMMAND = L"set_mark";
std::wstring TOP_CENTER_TAP_COMMAND = L"show_touch_main_menu";
std::wstring TOP_CENTER_HOLD_COMMAND = L"toggle_dark_mode";
std::wstring VISUAL_MARK_NEXT_TAP_COMMAND = L"";
std::wstring VISUAL_MARK_NEXT_HOLD_COMMAND = L"";
std::wstring VISUAL_MARK_PREV_TAP_COMMAND = L"";
std::wstring VISUAL_MARK_PREV_HOLD_COMMAND = L"";
std::wstring MIDDLE_LEFT_RECT_TAP_COMMAND = L"";
std::wstring MIDDLE_LEFT_RECT_HOLD_COMMAND = L"";
#ifdef SIOYEK_MOBILE
std::wstring MIDDLE_RIGHT_RECT_TAP_COMMAND = L"overview_definition";
#else
std::wstring MIDDLE_RIGHT_RECT_TAP_COMMAND = L"";
#endif
std::wstring MIDDLE_RIGHT_RECT_HOLD_COMMAND = L"";


bool UIRect::contains(NormalizedWindowPos window_pos) {
    return enabled && (window_pos.x >= left) && (window_pos.x <= right) && (-window_pos.y <= bottom) && (-window_pos.y >= top);
}

template<typename T>
void* generic_deserializer(std::wstringstream& stream, void* res_, bool* changed) {
    T* res = static_cast<T*>(res_);
    T prev_value = *res;
    stream >> *res;
    if (changed != nullptr) {
        if (prev_value != *res) {
            *changed = true;
        }
        else {
            *changed = false;
        }
    }
    return res;
}

void int_serializer(void* int_pointer, std::wstringstream& stream) {
    stream << *(int*)int_pointer;
}

void bool_serializer(void* bool_pointer, std::wstringstream& stream) {
    stream << *(bool*)bool_pointer;
}

void string_serializer(void* string_pointer, std::wstringstream& stream) {
    stream << *(std::wstring*)string_pointer;
}

void rect_serializer(void* rect_pointer, std::wstringstream& stream) {
    UIRect* rect = (UIRect*)rect_pointer;
    stream << rect->enabled << " " << rect->left << " " << rect->right << " " << rect->top << " " << rect->bottom;
}


void* string_deserializer(std::wstringstream& stream, void* res_, bool* changed) {
    assert(res_ != nullptr);
    //delete res_;

    std::wstring* res = static_cast<std::wstring*>(res_);
    std::wstring prev_value = *res;
    res->clear();
    std::getline(stream, *res);
    while (iswspace((*res)[0])) {
        res->erase(res->begin());

    }
    if (changed != nullptr) {
        if (prev_value != *res) {
            *changed = true;
        }
        else {
            *changed = false;
        }
    }
    return res;
}



template<int N, typename T>
void vec_n_serializer(void* vec_n_pointer, std::wstringstream& stream) {
    for (int i = 0; i < N; i++) {
        stream << *(((T*)vec_n_pointer) + i);
        if (i < N - 1) {
            stream << L" ";
        }
    }
}

//template<int N>
//void vec_n_serializer<N, float>(void* vec_n_pointer, std::wstringstream& stream) {
//	for (int i = 0; i < N; i++) {
//		stream << *(((T*)vec_n_pointer) + i);
//	}
//}

void* rect_deserializer(std::wstringstream& stream, void* res_, bool* changed) {
    assert(res_ != nullptr);
    UIRect* res = (UIRect*)res_;
    UIRect prev_value = *res;
    if (res == nullptr) {
        res = new UIRect;
    }
    stream >> res->enabled >> res->left >> res->right >> res->top >> res->bottom;
    if (changed) {
        UIRect new_value = *res;
        if ((prev_value.bottom != new_value.bottom) ||
            (prev_value.top != new_value.top) ||
            (prev_value.left != new_value.left) ||
            (prev_value.right != new_value.right) ||
            (prev_value.enabled != new_value.enabled)) {
            *changed = true;
        }
        else {
            *changed = false;
        }

    }

    return res;

}
template <int N, typename T>
bool are_vecs_different(T* vec1, T* vec2) {
    for (int i = 0; i < N; i++) {
        if (vec1[i] != vec2[i]) {
            return true;
        }
    }
    return false;

}

template<int N, typename T>
void* vec_n_deserializer(std::wstringstream& stream, void* res_, bool* changed) {
    assert(res_ != nullptr);
    T* res = (T*)res_;
    T prev_value[N];
    for (int i = 0; i < N; i++) {
        prev_value[i] = *(res + i);
    }

    if (res == nullptr) {
        res = new T[N];
        assert(false); // this should not happen
    }
    for (int i = 0; i < N; i++) {
        stream >> *(res + i);
    }
    if (changed) {
        *changed = are_vecs_different<N, T>(prev_value, res);

    }

    return res;
}

template <int N>
void* colorn_deserializer(std::wstringstream& stream, void* res_, bool* changed) {

    assert(res_ != nullptr);
    float* res = (float*)res_;
    float prev_value[N];
    for (int i = 0; i < N; i++) {
        prev_value[i] = *(res + i);
    }

    if (res == nullptr) {
        res = new float[N];
        assert(false); // this should not happen
    }
    while (std::isspace(stream.peek())) {
        stream.ignore();
    }
    if (stream.peek() == '#') {
        std::wstring rest;
        std::getline(stream, rest);
        hexademical_to_normalized_color(rest, res, N);
    }
    else {
        for (int i = 0; i < N; i++) {
            stream >> *(res + i);
        }
    }
    if (changed) {
        if (are_vecs_different<N, float>(prev_value, res)) {
            *changed = true;
        }
        else {
            *changed = false;
        }
    }

    return res;
}

void float_serializer(void* float_pointer, std::wstringstream& stream) {
    stream << *(float*)float_pointer;
}


void* Config::get_value() {
    return value;
}

Config* ConfigManager::get_mut_config_with_name(std::wstring config_name) {
    for (auto& it : configs) {
        if (it.name == config_name) return &it;
    }
    return nullptr;
}

bool ensure_between_0_and_1(const QStringList& parts) {
    bool ok = false;
    for (int i = 0; i < parts.size(); i++) {
        int val = parts.at(i).toFloat(&ok);
        if (!ok) {
            std::wcout << L"Error: invalid value for color component: " << parts.at(i).toStdWString() << "\n";
            return false;
        }
        if (val < 0 || val > 1) {
            std::wcout << L"Error: invalid value for color component (must be between 0 and 1): " << parts.at(i).toStdWString() << "\n";
            return false;
        }
    }
    return true;
}

template <int N>
bool colorn_validator(const std::wstring& str) {
    QString qstr = QString::fromStdWString(str);
    auto parts = qstr.trimmed().split(' ', Qt::SplitBehaviorFlags::SkipEmptyParts);
    if (parts.size() != N) {
        if (parts.size() == 1) {
            if (parts.at(0).at(0) == '#') {
                if (parts.at(0).size() == (1 + 2 * N)) {
                    return true;
                }
            }

        }
        std::wcout << L"Error: required " << N << " values for color, but got " << parts.size() << "\n";
        return false;
    }
    if (!ensure_between_0_and_1(parts)) {
        return false;
    }
    return true;
}
//bool color_3_validator(const std::wstring& str) {
//	QString qstr = QString::fromStdWString(str);
//	auto parts = qstr.trimmed().split(' ', Qt::SplitBehaviorFlags::SkipEmptyParts);
//	if (parts.size() != 3) {
//		std::wcout << L"Error: required 3 values for color, but got " << parts.size() << "\n";
//		return false;
//	}
//	if (!ensure_between_0_and_1(parts)) {
//		return false;
//	}
//	return true;
//}
//
//bool color_4_validator(const std::wstring& str) {
//	QString qstr = QString::fromStdWString(str);
//	auto parts = qstr.trimmed().split(' ', Qt::SplitBehaviorFlags::SkipEmptyParts);
//	if (parts.size() != 4) {
//		std::wcout << L"Error: required 4 values for color, but got " << parts.size() << "\n";
//		return false;
//	}
//	if (!ensure_between_0_and_1(parts)) {
//		return false;
//	}
//	return true;
//}

bool bool_validator(const std::wstring& str) {
    QString qstr = QString::fromStdWString(str);
    auto parts = qstr.trimmed().split(' ', Qt::SplitBehaviorFlags::SkipEmptyParts);
    std::wstring msg = L"Bool values should be either 0 or 1, but got ";
    if (parts.size() != 1) {
        std::wcout << msg << str << L"\n";
        return false;
    }
    if (parts.at(0).trimmed().toStdWString() == L"0" || parts.at(0).trimmed().toStdWString() == L"1") {
        return true;
    }
    std::wcout << msg << str << L"\n";
    return false;
}

auto ivec2_serializer = vec_n_serializer<2, int>;
auto ivec2_deserializer = vec_n_deserializer<2, int>;
auto fvec2_serializer = vec_n_serializer<2, float>;
auto fvec2_deserializer = vec_n_deserializer<2, float>;
auto vec3_serializer = vec_n_serializer<3, float>;
auto vec4_serializer = vec_n_serializer<4, float>;
auto vec3_deserializer = vec_n_deserializer<3, float>;
auto vec4_deserializer = vec_n_deserializer<4, float>;
auto float_deserializer = generic_deserializer<float>;
auto int_deserializer = generic_deserializer<int>;
auto bool_deserializer = generic_deserializer<bool>;
auto color3_deserializer = colorn_deserializer<3>;
auto color4_deserializer = colorn_deserializer<4>;
auto color_3_validator = colorn_validator<3>;
auto color_4_validator = colorn_validator<4>;

class ConfigBuilder{

private:
    std::wstring name;
    ConfigType config_type;
    void* value = nullptr;
    std::function<void(void*, std::wstringstream&)> serialize = nullptr;
    std::function<void*(std::wstringstream&, void* res, bool* changed)> deserialize = nullptr;
    std::function<bool(const std::wstring& value)> validator = nullptr;
    std::variant<FloatExtras, IntExtras, EmptyExtras> extras = EmptyExtras{};
    bool is_auto = false;

    std::function<void(void*, std::wstringstream&)> get_serializer(){
        if (serialize) return serialize;
        switch(config_type){
        case Int: return int_serializer;
        case Float: return float_serializer;
        case Color3: return vec3_serializer;
        case Color4: return vec4_serializer;
        case Bool: return bool_serializer;
        case String: return string_serializer;
        case FilePath: return string_serializer;
        case FolderPath: return string_serializer;
        case IVec2: return ivec2_serializer;
        case FVec2: return fvec2_serializer;
        case EnableRectangle: return rect_serializer;
        case Range: return fvec2_serializer;
        case Macro: return string_serializer;
        default: assert(false);
        }
    }

    std::function<void*(std::wstringstream&, void* res, bool* changed)> get_deserializer(){
        if (deserialize) return deserialize;
        switch(config_type){
        case Int: return int_deserializer;
        case Float: return float_deserializer;
        case Color3: return color3_deserializer;
        case Color4: return color4_deserializer;
        case Bool: return bool_deserializer;
        case String: return string_deserializer;
        case FilePath: return string_deserializer;
        case FolderPath: return string_deserializer;
        case IVec2: return ivec2_deserializer;
        case FVec2: return fvec2_deserializer;
        case EnableRectangle: return rect_deserializer;
        case Range: return fvec2_deserializer;
        case Macro: return string_deserializer;
        default: assert(false);
        }

    }

    std::function<bool(const std::wstring& value)> get_validator(){
        if (validator) return validator;
        switch(config_type){
        case Int: return nullptr;
        case Float: return nullptr;
        case Color3: return color_3_validator;
        case Color4: return color_4_validator;
        case Bool: return bool_validator;
        case String: return nullptr;
        case FilePath: return nullptr;
        case FolderPath: return nullptr;
        case IVec2: return nullptr;
        case FVec2: return nullptr;
        case EnableRectangle: return nullptr;
        case Range: return nullptr;
        case Macro: return nullptr;
        default: assert(false);
        }

    }

public:
    ConfigBuilder(std::wstring config_name, ConfigType ctype, void* cvalue){
        name = config_name;
        config_type = ctype;
        value = cvalue;
    }

    static ConfigBuilder color3(std::wstring config_name, float* val){
        return ConfigBuilder(config_name, ConfigType::Color3, val);
    }

    static ConfigBuilder color4(std::wstring config_name, float* val){
        return ConfigBuilder(config_name, ConfigType::Color4, val);
    }

    static ConfigBuilder boolean(std::wstring config_name, bool* val){
        return ConfigBuilder(config_name, ConfigType::Bool, val);
    }

    static ConfigBuilder floatc(std::wstring config_name, float* val, FloatExtras ex){
        return ConfigBuilder(config_name, ConfigType::Float, val).extra(ex);
    }

    static ConfigBuilder intc(std::wstring config_name, int* val, IntExtras ex){
        return ConfigBuilder(config_name, ConfigType::Int, val).extra(ex);
    }

    static ConfigBuilder string(std::wstring config_name, std::wstring* val){
        return ConfigBuilder(config_name, ConfigType::String, val);
    }

    static ConfigBuilder macro(std::wstring config_name, std::wstring* val){
        return ConfigBuilder(config_name, ConfigType::Macro, val);
    }

    static ConfigBuilder ivec2(std::wstring config_name, int* val){
        return ConfigBuilder(config_name, ConfigType::IVec2, val);
    }

    static ConfigBuilder fvec2(std::wstring config_name, float* val){
        return ConfigBuilder(config_name, ConfigType::FVec2, val);
    }

    static ConfigBuilder rect(std::wstring config_name, UIRect* val){
        return ConfigBuilder(config_name, ConfigType::EnableRectangle, val);
    }

    ConfigBuilder serializer(std::function<void(void*, std::wstringstream&)> ser){
        this->serialize = ser;
        return *this;
    }

    ConfigBuilder deserializer(std::function<void*(std::wstringstream&, void* res, bool* changed)> deser){
        this->deserialize = deser;
        return *this;
    }

    ConfigBuilder valid(std::function<bool(const std::wstring& value)> validator_){
        this->validator = validator_;
        return *this;
    }

    ConfigBuilder extra(std::variant<FloatExtras, IntExtras, EmptyExtras> extras_){
        this->extras = extras_;
        return *this;
    }

    ConfigBuilder auto_(){
        this->is_auto = true;
        return *this;
    }

    Config build(){
        Config res;
        res.name = name;
        res.config_type = config_type;
        res.value = value;
        res.serialize = get_serializer();
        res.deserialize = get_deserializer();
        res.validator = get_validator();
        res.extras = extras;
        res.is_auto = is_auto;
        res.load_default();
        return res;
    }

};

ConfigManager::ConfigManager(const Path& default_path, const Path& auto_path, const std::vector<Path>& user_paths) {

    user_config_paths = user_paths;

    auto add_color3 = [&](std::wstring name, float* location){
        configs.push_back(
            ConfigBuilder::color3(name, location).build()
            );
    };

    auto add_color4 = [&](std::wstring name, float* location){
        configs.push_back(
            ConfigBuilder::color4(name, location).build()
            );
    };

    auto add_bool = [&](std::wstring name, bool* location){
        configs.push_back(
            ConfigBuilder::boolean(name, location).build()
            );
    };

    auto add_string = [&](std::wstring name, std::wstring* location){
        configs.push_back(
            ConfigBuilder::string(name, location).build()
            );
    };

    auto add_macro = [&](std::wstring name, std::wstring* location){
        configs.push_back(
            ConfigBuilder::macro(name, location).build()
            );
    };

    auto add_ivec2 = [&](std::wstring name, int* location){
        configs.push_back(
            ConfigBuilder::ivec2(name, location).build()
            );
    };

    auto add_fvec2 = [&](std::wstring name, float* location){
        configs.push_back(
            ConfigBuilder::fvec2(name, location).build()
            );
    };

    auto add_rect = [&](std::wstring name, UIRect* location){
        configs.push_back(
            ConfigBuilder::rect(name, location).build()
            );
    };

    auto add_float = [&](std::wstring name, float* location, FloatExtras extras){
        configs.push_back(
            ConfigBuilder::floatc(name, location, extras).build()
            );
    };

    auto add_int = [&](std::wstring name, int* location, IntExtras extras){
        configs.push_back(
            ConfigBuilder::intc(name, location, extras).build()
            );
    };

    add_color3(L"text_highlight_color", DEFAULT_TEXT_HIGHLIGHT_COLOR);
    add_color3(L"search_highlight_color", DEFAULT_SEARCH_HIGHLIGHT_COLOR);
    add_color3(L"unselected_search_highlight_color", UNSELECTED_SEARCH_HIGHLIGHT_COLOR);
    add_color3(L"freetext_bookmark_color", FREETEXT_BOOKMARK_COLOR);
    add_color3(L"link_highlight_color", DEFAULT_LINK_HIGHLIGHT_COLOR);
    add_color3(L"synctex_highlight_color", DEFAULT_SYNCTEX_HIGHLIGHT_COLOR);
    add_color3(L"ruler_color", RULER_COLOR);
    add_color3(L"ruler_marker_color", RULER_MARKER_COLOR);
    add_color3(L"background_color", BACKGROUND_COLOR);
    add_color3(L"dark_mode_background_color", DARK_MODE_BACKGROUND_COLOR);
    add_color3(L"custom_color_mode_empty_background_color", CUSTOM_COLOR_MODE_EMPTY_BACKGROUND_COLOR);
    add_color3(L"custom_background_color", CUSTOM_BACKGROUND_COLOR);
    add_color3(L"custom_text_color", CUSTOM_TEXT_COLOR);
    add_color3(L"status_bar_color", STATUS_BAR_COLOR);
    add_color3(L"status_bar_text_color", STATUS_BAR_TEXT_COLOR);
    add_color3(L"page_separator_color", PAGE_SEPARATOR_COLOR);
    add_color3(L"ui_selected_background_color", UI_SELECTED_BACKGROUND_COLOR);
    add_color3(L"ui_text_color", UI_TEXT_COLOR);
    add_color3(L"ui_background_color", UI_BACKGROUND_COLOR);
    add_color3(L"ui_selected_text_color", UI_SELECTED_TEXT_COLOR);
    add_color3(L"ui_background_color", STATUS_BAR_COLOR);
    add_color4(L"vertical_line_color",DEFAULT_VERTICAL_LINE_COLOR);
    add_color4(L"visual_mark_color",DEFAULT_VERTICAL_LINE_COLOR);
    add_color4(L"keyboard_select_background_color", KEYBOARD_SELECT_BACKGROUND_COLOR);
    add_color4(L"keyboard_select_text_color", KEYBOARD_SELECT_TEXT_COLOR);
    add_color4(L"keyboard_selected_tag_text_color", KEYBOARD_SELECTED_TAG_TEXT_COLOR);
    add_color4(L"keyboard_selected_tag_background_color", KEYBOARD_SELECTED_TAG_BACKGROUND_COLRO);
    add_float(L"synctex_highlight_timeout", &HIDE_SYNCTEX_HIGHLIGHT_TIMEOUT, FloatExtras{-1.0f, 100.0f});
    add_float(L"dark_mode_contrast", &DARK_MODE_CONTRAST, FloatExtras{0.0f, 1.0f});
    add_float(L"freetext_bookmark_font_size", &FREETEXT_BOOKMARK_FONT_SIZE, FloatExtras{0.0f, 100.0f});
    add_float(L"custom_color_contrast", &CUSTOM_COLOR_CONTRAST, FloatExtras{0.0f, 1.0f});
    add_float(L"zoom_inc_factor", &ZOOM_INC_FACTOR, FloatExtras{1.0f, 2.0f});
    add_float(L"scroll_zoom_inc_factor", &SCROLL_ZOOM_INC_FACTOR, FloatExtras{1.0f, 2.0f});
    add_float(L"vertical_move_amount", &VERTICAL_MOVE_AMOUNT, FloatExtras{0.1f, 10.0f});
    add_float(L"horizontal_move_amount", &HORIZONTAL_MOVE_AMOUNT, FloatExtras{0.1f, 10.0f});
    add_float(L"move_screen_percentage", &MOVE_SCREEN_PERCENTAGE, FloatExtras{0.0f, 1.0f});
    add_float(L"strike_line_width", &STRIKE_LINE_WIDTH, FloatExtras{0.0f, 10.0f});
    add_float(L"move_screen_ratio", &MOVE_SCREEN_PERCENTAGE, FloatExtras{0.0f, 1.0f});
    add_float(L"visual_mark_next_page_fraction", &VISUAL_MARK_NEXT_PAGE_FRACTION, FloatExtras{-2.0f, 2.0f});
    add_float(L"visual_mark_next_page_threshold", &VISUAL_MARK_NEXT_PAGE_THRESHOLD, FloatExtras{-2.0f, 2.0f});
    add_float(L"touchpad_sensitivity", &TOUCHPAD_SENSITIVITY, FloatExtras{0.0f, 10.0f});
    add_float(L"scrollview_sensitivity", &SCROLL_VIEW_SENSITIVITY, FloatExtras{0.0f, 10.0f});
    add_float(L"page_separator_width", &PAGE_SEPARATOR_WIDTH, FloatExtras{0.0f, 10.0f});
    add_float(L"fit_to_page_width_ratio", &FIT_TO_PAGE_WIDTH_RATIO, FloatExtras{0.25f, 1.0f});
    add_float(L"ruler_padding", &RULER_PADDING, FloatExtras{0.0f, 10.0f});
    add_float(L"ruler_x_padding", &RULER_X_PADDING, FloatExtras{0.0f, 10.0f});
    add_float(L"ruler_auto_move_sensitivity", &RULER_AUTO_MOVE_SENSITIVITY, FloatExtras{0.0f, 200.0f});
    add_float(L"fastread_opacity", &FASTREAD_OPACITY, FloatExtras{0.0f, 1.0f});
    add_float(L"page_space_x", &PAGE_SPACE_X, FloatExtras{0.0f, 100.0f});
    add_float(L"page_space_y", &PAGE_SPACE_Y, FloatExtras{0.0f, 100.0f});
    add_float(L"menu_screen_width_ratio", &MENU_SCREEN_WDITH_RATIO, FloatExtras{0.0f, 1.0f});
    add_float(L"menu_screen_width_ratio", &MENU_SCREEN_WDITH_RATIO, FloatExtras{0.0f, 1.0f});
    add_float(L"smooth_scroll_speed", &SMOOTH_SCROLL_SPEED, FloatExtras{0.0f, 20.0f});
    add_float(L"smooth_scroll_drag", &SMOOTH_SCROLL_DRAG, FloatExtras{10.0f, 10000.0f});
    add_float(L"gamma", &GAMMA, FloatExtras{0.0f, 1.0f});
    add_float(L"highlight_delete_threshold", &HIGHLIGHT_DELETE_THRESHOLD, FloatExtras{0.0f, 0.1f});
    add_float(L"tts_rate", &TTS_RATE, FloatExtras{-1.0f, 1.0f});
    add_float(L"smooth_move_max_velocity", &SMOOTH_MOVE_MAX_VELOCITY, FloatExtras{0.0f, 100000.0f});
    add_float(L"epub_width", &EPUB_WIDTH, FloatExtras{0.0f, 1000.0f});
    add_float(L"epub_height", &EPUB_HEIGHT, FloatExtras{0.0f, 1000.0f});
    add_float(L"epub_font_size", &EPUB_FONT_SIZE, FloatExtras{0.0f, 100.0f});
    add_float(L"persistance_period", &PERSISTANCE_PERIOD, FloatExtras{-1.0f, 100000.0f});
    add_bool(L"default_dark_mode", &DEFAULT_DARK_MODE);
    add_bool(L"use_system_theme", &USE_SYSTEM_THEME);
    add_bool(L"use_custom_color_as_dark_system_theme", &USE_CUSTOM_COLOR_FOR_DARK_SYSTEM_THEME);
    add_bool(L"render_freetext_borders", &RENDER_FREETEXT_BORDERS);
    add_bool(L"flat_toc", &FLAT_TABLE_OF_CONTENTS);
    add_bool(L"adjust_annotation_colors_for_dark_mode", &ADJUST_ANNOTATION_COLORS_FOR_DARK_MODE);
    add_bool(L"right_click_context_menu", &SHOW_RIGHT_CLICK_CONTEXT_MENU);
    add_bool(L"preserve_image_colors_in_dark_mode", &PRESERVE_IMAGE_COLORS);
    add_bool(L"inverted_preserved_image_colors", &INVERTED_PRESERVED_IMAGE_COLORS);
    add_bool(L"invert_selected_text", &INVERT_SELECTED_TEXT);
    add_bool(L"ignore_scroll_events", &IGNORE_SCROLL_EVENTS);
    add_bool(L"dont_center_if_synctex_rect_is_visible", &DONT_FOCUS_IF_SYNCTEX_RECT_IS_VISIBLE);
    add_bool(L"gg_uses_labels", &GG_USES_LABELS);
    add_bool(L"should_use_multiple_monitors", &SHOULD_USE_MULTIPLE_MONITORS);
    add_bool(L"paper_download_should_create_portal", &PAPER_DOWNLOAD_CREATE_PORTAL);
    add_bool(L"paper_download_should_detect_paper_name", &PAPER_DOWNLOAD_AUTODETECT_PAPER_NAME);
    add_bool(L"automatically_download_matching_paper_name", &AUTOMATICALLY_DOWNLOAD_MATCHING_PAPER_NAME);
    add_bool(L"should_load_tutorial_when_no_other_file", &SHOULD_LOAD_TUTORIAL_WHEN_NO_OTHER_FILE);
    add_bool(L"should_launch_new_instance", &SHOULD_LAUNCH_NEW_INSTANCE);
    add_bool(L"should_launch_new_window", &SHOULD_LAUNCH_NEW_WINDOW);
    add_bool(L"should_draw_unrendered_pages", &SHOULD_DRAW_UNRENDERED_PAGES);
    add_bool(L"hide_overlapping_link_labels", &HIDE_OVERLAPPING_LINK_LABELS);
    add_bool(L"real_page_separation", &REAL_PAGE_SEPARATION);
    add_bool(L"fill_textbar_with_selected_text", &FILL_TEXTBAR_WITH_SELECTED_TEXT);
    add_bool(L"align_link_dest_to_top", &ALIGN_LINK_DEST_TO_TOP);
    add_bool(L"check_for_updates_on_startup", &SHOULD_CHECK_FOR_LATEST_VERSION_ON_STARTUP);
    add_bool(L"sort_bookmarks_by_location", &SORT_BOOKMARKS_BY_LOCATION);
    add_bool(L"sort_highlights_by_location", &SORT_HIGHLIGHTS_BY_LOCATION);
    add_bool(L"hover_overview", &HOVER_OVERVIEW);
    add_bool(L"rerender_overview", &RERENDER_OVERVIEW);
    add_bool(L"wheel_zoom_on_cursor", &WHEEL_ZOOM_ON_CURSOR);
    add_bool(L"linear_filter", &LINEAR_TEXTURE_FILTERING);
    add_bool(L"collapsed_toc", &SMALL_TOC);
    add_bool(L"render_pdf_annotations", &SHOULD_RENDER_PDF_ANNOTATIONS);
    add_bool(L"ruler_mode", &RULER_MODE);
    add_bool(L"use_ruler_to_highlight_synctex_line", &USE_RULER_TO_HIGHLIGHT_SYNCTEX_LINE);
    add_bool(L"show_most_recent_commands_first", &SHOW_MOST_RECENT_COMMANDS_FIRST);
    add_bool(L"keyboard_point_selection", &USE_KEYBOARD_POINT_SELECTION);
    add_bool(L"text_summary_should_refine", &TEXT_SUMMARY_HIGHLIGHT_SHOULD_REFINE);
    add_bool(L"text_summary_should_fill", &TEXT_SUMMARY_HIGHLIGHT_SHOULD_FILL);
    add_bool(L"use_heuristic_if_text_summary_not_available", &USE_HEURISTIC_IF_TEXT_SUMMARY_NOT_AVAILABLE);
    add_bool(L"enable_experimental_features", &ENABLE_EXPERIMENTAL_FEATURES);
    add_bool(L"create_table_of_contents_if_not_exists", &CREATE_TABLE_OF_CONTENTS_IF_NOT_EXISTS);
    add_bool(L"force_custom_line_algorithm", &FORCE_CUSTOM_LINE_ALGORITHM);
    add_bool(L"ignore_whitespace_in_presentation_mode", &IGNORE_WHITESPACE_IN_PRESENTATION_MODE);
    add_bool(L"exact_highlight_select", &EXACT_HIGHLIGHT_SELECT);
    add_bool(L"show_doc_path", &SHOW_DOC_PATH);
    add_bool(L"should_warn_about_user_key_override", &SHOULD_WARN_ABOUT_USER_KEY_OVERRIDE);
    add_bool(L"single_click_selects_words", &SINGLE_CLICK_SELECTS_WORDS);
    add_bool(L"allow_horizontal_drag_when_document_is_small", &ALLOW_HORIZONTAL_DRAG_WHEN_DOCUMENT_IS_SMALL);
    add_bool(L"right_click_context_menu", &RIGHT_CLICK_CONTEXT_MENU);
    add_bool(L"use_legacy_keybinds", &USE_LEGACY_KEYBINDS);
    add_bool(L"multiline_menus", &MULTILINE_MENUS);
    add_bool(L"start_with_helper_window", &START_WITH_HELPER_WINDOW);
    add_bool(L"lighten_colors_when_embedding_annotations", &LIGHTEN_COLORS_WHEN_EMBEDDING_ANNOTATIONS);
    add_bool(L"prerender_next_page_presentation", &PRERENDER_NEXT_PAGE);
    add_bool(L"highlight_middle_click", &HIGHLIGHT_MIDDLE_CLICK);
    add_bool(L"auto_rename_downloaded_papers", &AUTO_RENAME_DOWNLOADED_PAPERS);
    add_bool(L"super_fast_search", &SUPER_FAST_SEARCH);
    add_bool(L"incremental_search", &INCREMENTAL_SEARCH);
    add_bool(L"show_closest_bookmark_in_statusbar", &SHOW_CLOSEST_BOOKMARK_IN_STATUSBAR);
    add_bool(L"show_close_portal_in_statusbar", &SHOW_CLOSE_PORTAL_IN_STATUSBAR);
    add_bool(L"case_sensitive_search", &CASE_SENSITIVE_SEARCH);
    add_bool(L"smartcase_search", &SMARTCASE_SEARCH);
    add_bool(L"show_document_name_in_statusbar", &SHOW_DOCUMENT_NAME_IN_STATUSBAR);
    add_bool(L"numeric_tags", &NUMERIC_TAGS);
    add_bool(L"highlight_links", &SHOULD_HIGHLIGHT_LINKS);
    add_bool(L"should_highlight_unselected_search", &SHOULD_HIGHLIGHT_UNSELECTED_SEARCH);
    add_bool(L"fuzzy_searching", &FUZZY_SEARCHING);
    add_bool(L"inverted_horizontal_scrolling", &INVERTED_HORIZONTAL_SCROLLING);
    add_bool(L"toc_jump_align_top", &TOC_JUMP_ALIGN_TOP);
    add_bool(L"autocenter_visual_scroll", &AUTOCENTER_VISUAL_SCROLL);
    add_bool(L"alphabetic_link_tags", &ALPHABETIC_LINK_TAGS);
    add_bool(L"vimtex_wsl_fix", &VIMTEX_WSL_FIX);
    add_bool(L"sliced_rendering", &SLICED_RENDERING);
    add_bool(L"touch_mode", &TOUCH_MODE);
    add_bool(L"allow_main_view_scroll_while_in_overview", &ALLOW_MAIN_VIEW_SCROLL_WHILE_IN_OVERVIEW);
    add_bool(L"background_pixel_fix", &BACKGROUND_PIXEL_FIX);
    add_bool(L"add_newlines_when_copying_text", &ADD_NEWLINES_WHEN_COPYING_TEXT);
    add_bool(L"always_copy_selected_text", &ALWAYS_COPY_SELECTED_TEXT);
    add_bool(L"show_statusbar_only_when_hovered", &SHOW_STATUSBAR_ONLY_WHEN_MOUSE_OVER);

    add_string(L"google_scholar_address", &GOOGLE_SCHOLAR_ADDRESS);
    add_string(L"item_list_prefix", &ITEM_LIST_PREFIX);
    add_string(L"inverse_search_command", &INVERSE_SEARCH_COMMAND);
    add_string(L"libgen_address", &LIBGEN_ADDRESS);
    add_string(L"shared_database_path", &SHARED_DATABASE_PATH);
    add_string(L"ruler_display_mode", &RULER_DISPLAY_MODE);
    add_string(L"ui_font", &UI_FONT_FACE_NAME);
    add_string(L"status_font", &STATUS_FONT_FACE_NAME);
    add_string(L"middle_click_search_engine", &MIDDLE_CLICK_SEARCH_ENGINE);
    add_string(L"shift_middle_click_search_engine", &SHIFT_MIDDLE_CLICK_SEARCH_ENGINE);
    add_string(L"text_summary_url", &TEXT_HIGHLIGHT_URL);
    add_string(L"paper_search_url", &PAPER_SEARCH_URL);
    add_string(L"papers_folder_path", &PAPERS_FOLDER_PATH);
    add_string(L"scan_path", &BOOK_SCAN_PATH);
    add_string(L"context_menu_items", &CONTEXT_MENU_ITEMS);
    add_string(L"context_menu_items_for_links", &CONTEXT_MENU_ITEMS_FOR_LINKS);
    add_string(L"context_menu_items_for_selected_text", &CONTEXT_MENU_ITEMS_FOR_SELECTED_TEXT);
    add_string(L"context_menu_items_for_highlights", &CONTEXT_MENU_ITEMS_FOR_HIGHLIGHTS);
    add_string(L"context_menu_items_for_bookmarks", &CONTEXT_MENU_ITEMS_FOR_BOOKMARKS);
    add_string(L"context_menu_items_for_overview", &CONTEXT_MENU_ITEMS_FOR_OVERVIEW);
    add_string(L"paper_download_url_path", &PAPER_SEARCH_URL_PATH);
    add_string(L"paper_download_title_path", &PAPER_SEARCH_TILE_PATH);
    add_string(L"paper_download_contrib_path", &PAPER_SEARCH_CONTRIB_PATH);
    add_string(L"default_open_file_path", &DEFAULT_OPEN_FILE_PATH);
    add_string(L"annotations_directory", &ANNOTATIONS_DIR_PATH);
    add_string(L"status_bar_format", &STATUS_BAR_FORMAT);
    add_string(L"right_status_bar_format", &RIGHT_STATUS_BAR_FORMAT);
    add_string(L"epub_css", &EPUB_CSS);
    add_string(L"tag_font_face", &TAG_FONT_FACE);
    add_macro(L"startup_commands", &STARTUP_COMMANDS);
    add_macro(L"shift_click_command", &SHIFT_CLICK_COMMAND);
    add_macro(L"resize_command", &RESIZE_COMMAND);
    add_macro(L"control_click_command", &CONTROL_CLICK_COMMAND);
    add_macro(L"hold_middle_click_command", &HOLD_MIDDLE_CLICK_COMMAND);
    add_macro(L"back_rect_tap_command", &BACK_RECT_TAP_COMMAND);
    add_macro(L"tablet_pen_click_command", &TABLET_PEN_CLICK_COMMAND);
    add_macro(L"tablet_pen_double_click_command", &TABLET_PEN_DOUBLE_CLICK_COMMAND);
    add_macro(L"volume_down_command", &VOLUME_DOWN_COMMAND);
    add_macro(L"volume_down_command", &VOLUME_UP_COMMAND);
    add_macro(L"back_rect_hold_command", &BACK_RECT_HOLD_COMMAND);
    add_macro(L"forward_rect_tap_command", &FORWARD_RECT_TAP_COMMAND);
    add_macro(L"forward_rect_hold_command", &FORWARD_RECT_HOLD_COMMAND);
    add_macro(L"top_center_tap_command", &TOP_CENTER_TAP_COMMAND);
    add_macro(L"top_center_hold_command", &TOP_CENTER_HOLD_COMMAND);
    add_macro(L"middle_left_rect_tap_command", &MIDDLE_LEFT_RECT_TAP_COMMAND);
    add_macro(L"middle_left_rect_hold_command", &MIDDLE_LEFT_RECT_HOLD_COMMAND);
    add_macro(L"middle_right_rect_tap_command", &MIDDLE_RIGHT_RECT_TAP_COMMAND);
    add_macro(L"middle_right_rect_hold_command", &MIDDLE_RIGHT_RECT_HOLD_COMMAND);
    add_macro(L"visual_mark_next_tap_command", &VISUAL_MARK_NEXT_TAP_COMMAND);
    add_macro(L"visual_mark_next_hold_command", &VISUAL_MARK_NEXT_HOLD_COMMAND);
    add_macro(L"visual_mark_prev_tap_command", &VISUAL_MARK_PREV_TAP_COMMAND);
    add_macro(L"visual_mark_prev_hold_command", &VISUAL_MARK_PREV_HOLD_COMMAND);
    add_macro(L"right_click_command", &RIGHT_CLICK_COMMAND);
    add_macro(L"middle_click_command", &MIDDLE_CLICK_COMMAND);
    add_macro(L"shift_right_click_command", &SHIFT_RIGHT_CLICK_COMMAND);
    add_macro(L"control_right_click_command", &CONTROL_RIGHT_CLICK_COMMAND);
    add_macro(L"alt_click_command", &ALT_CLICK_COMMAND);
    add_macro(L"alt_right_click_command", &ALT_RIGHT_CLICK_COMMAND);
    add_int(L"font_size", &FONT_SIZE, IntExtras{1, 100});
    add_int(L"ruler_pixel_width", &RULER_UNDERLINE_PIXEL_WIDTH, IntExtras{1, 100});
    add_int(L"num_prerendered_next_slides", &NUM_PRERENDERED_NEXT_SLIDES, IntExtras{0, 5});
    add_int(L"num_cached_pages", &NUM_CACHED_PAGES, IntExtras{0, 100});
    add_int(L"num_prerendered_prev_slides", &NUM_PRERENDERED_PREV_SLIDES, IntExtras{0, 5});
    add_int(L"keyboard_select_font_size", &KEYBOARD_SELECT_FONT_SIZE, IntExtras{1, 100});
    add_int(L"documentation_font_size", &DOCUMENTATION_FONT_SIZE, IntExtras{1, 100});
    add_int(L"status_bar_font_size", &STATUS_BAR_FONT_SIZE, IntExtras{1, 100});
    add_int(L"text_summary_context_size", &TEXT_SUMMARY_CONTEXT_SIZE, IntExtras{1, 100});
    add_int(L"max_created_toc_size", &MAX_CREATED_TABLE_OF_CONTENTS_SIZE, IntExtras{1, 100000});
    add_int(L"prerendered_page_count", &PRERENDERED_PAGE_COUNT, IntExtras{0, 10});
    add_int(L"reload_interval_miliseconds", &RELOAD_INTERVAL_MILISECONDS, IntExtras{0, 10000});
    add_ivec2(L"main_window_size", MAIN_WINDOW_SIZE);
    add_ivec2(L"helper_window_size", HELPER_WINDOW_SIZE);
    add_ivec2(L"main_window_move", MAIN_WINDOW_MOVE);
    add_ivec2(L"helper_window_move", HELPER_WINDOW_MOVE);
    add_ivec2(L"single_main_window_size", SINGLE_MAIN_WINDOW_SIZE);
    add_ivec2(L"single_main_window_move", SINGLE_MAIN_WINDOW_MOVE);
    add_fvec2(L"overview_size", OVERVIEW_SIZE);
    add_fvec2(L"overview_offset", OVERVIEW_OFFSET);
    add_rect(L"portrait_back_ui_rect", &PORTRAIT_BACK_UI_RECT);
    add_rect(L"portrait_forward_ui_rect", &PORTRAIT_FORWARD_UI_RECT);
    add_rect(L"landscape_back_ui_rect", &LANDSCAPE_BACK_UI_RECT);
    add_rect(L"landscape_forward_ui_rect", &LANDSCAPE_FORWARD_UI_RECT);
    add_rect(L"portrait_visual_mark_next", &PORTRAIT_VISUAL_MARK_NEXT);
    add_rect(L"landscape_middle_right_rect", &LANDSCAPE_MIDDLE_RIGHT_UI_RECT);
    add_rect(L"landscape_middle_left_rect", &LANDSCAPE_MIDDLE_LEFT_UI_RECT);
    add_rect(L"portrait_middle_right_rect", &PORTRAIT_MIDDLE_RIGHT_UI_RECT);
    add_rect(L"portrait_middle_left_rect", &PORTRAIT_MIDDLE_LEFT_UI_RECT);
    add_rect(L"portrait_visual_mark_prev", &PORTRAIT_VISUAL_MARK_PREV);
    add_rect(L"landscape_visual_mark_next", &LANDSCAPE_VISUAL_MARK_NEXT);
    add_rect(L"landscape_visual_mark_prev", &LANDSCAPE_VISUAL_MARK_PREV);

#ifdef Q_OS_MACOS
    add_color3(L"macos_titlebar_color", MACOS_TITLEBAR_COLOR);
    add_bool(L"macos_hide_titlebar", &MACOS_HIDE_TITLEBAR);
#endif

    std::wstring highlight_config_string = L"highlight_color_a";
    std::wstring search_url_config_string = L"search_url_a";
    std::wstring execute_command_config_string = L"execute_command_a";

    for (char letter = 'a'; letter <= 'z'; letter++) {
        highlight_config_string[highlight_config_string.size() - 1] = letter;
        search_url_config_string[search_url_config_string.size() - 1] = letter;
        execute_command_config_string[execute_command_config_string.size() - 1] = letter;

        configs.push_back({ highlight_config_string, ConfigType::Color3, &HIGHLIGHT_COLORS[(letter - 'a') * 3], vec3_serializer, color3_deserializer, color_3_validator });
        configs.push_back({ search_url_config_string, ConfigType::String, &SEARCH_URLS[letter - 'a'], string_serializer, string_deserializer, nullptr });
        configs.push_back({ execute_command_config_string, ConfigType::String, &EXECUTE_COMMANDS[letter - 'a'], string_serializer, string_deserializer, nullptr });
    }

    // for (auto& config : configs) {
    //     config.save_value_into_default();
    // }

    deserialize(nullptr, default_path, auto_path, user_paths);
}

void ConfigManager::persist_config() {
#ifdef SIOYEK_ANDROID
    serialize(android_config_path);
#else
    //Path path(L"test.config");
    //serialize(path);
#endif

}

void ConfigManager::restore_default() {
#ifdef SIOYEK_ANDROID
    clear_file(android_config_path);
#endif
}

void ConfigManager::clear_file(const Path& path) {

    std::wofstream file = open_wofstream(path.get_path());
    file.close();
}

void ConfigManager::serialize_auto_configs(std::wofstream& stream) {
    std::vector<std::wstring> auto_config_names = get_auto_config_names();
    std::vector<std::wstring> exceptions = {
        L"main_window_size",
        L"single_main_window_size",
        L"main_window_move",
        L"single_main_window_move",
        L"helper_window_size",
        L"helper_window_move",
        L"overview_size",
        L"overview_offset",
    };

    auto is_exception = [&](const std::wstring& config_name) {
        return std::find(exceptions.begin(), exceptions.end(), config_name) != exceptions.end();
        };

    for (auto& conf : configs) {
        if (conf.is_auto && (!is_exception(conf.name))) {
            if (conf.is_empty_string()) {
                continue;
            }
            if (!conf.has_changed_from_default()) {
                continue;
            }

            std::wstringstream ss;
            stream << conf.name << " ";
            if (conf.get_value()) {
                conf.serialize(conf.get_value(), ss);
            }
            stream << ss.str() << std::endl;
        }
    }
}
void ConfigManager::serialize(const Path& path) {

    std::wofstream file = open_wofstream(path.get_path());

    for (auto it : configs) {

        if (it.is_empty_string()) {
            continue;
        }
        if (!it.has_changed_from_default()){
            continue;
        }

        std::wstringstream ss;
        file << it.name << " ";
        if (it.get_value()) {
            it.serialize(it.get_value(), ss);
        }
        file << ss.str() << std::endl;
    }

    file.close();
}

void ConfigManager::deserialize_file(std::vector<std::string>* changed_config_names, const Path& file_path, bool warn_if_not_exists, bool is_auto) {

    std::wstring line;
    std::wifstream default_file = open_wifstream(file_path.get_path());
    int line_number = 0;

    if (warn_if_not_exists && (!default_file.good())) {
        std::wcout << "Error: Could not open config file " << file_path.get_path() << std::endl;
    }

    while (std::getline(default_file, line)) {
        line_number++;

        if (line.size() == 0 || line[0] == '#') {
            continue;
        }

        std::wstringstream ss{ line };
        std::wstring conf_name;
        ss >> conf_name;
        //special handling for new_command 
        if (conf_name == L"source") {
            std::wstring path;
            std::getline(ss, path);
            path = strip_string(path);
            if (path.size() > 0) {
                if (path[0] == '.') {
                    auto parent_dir = QDir(QString::fromStdWString(file_path.file_parent().get_path()));
                    path = parent_dir.absoluteFilePath(QString::fromStdWString(path)).toStdWString();
                }
                if (path[0] == '~') {
                    path = QDir::homePath().toStdWString() + path.substr(1, path.size() - 1);
                }

                deserialize_file(changed_config_names, path, true);
            }
        }
        else if ((conf_name == L"new_command") || (conf_name == L"new_async_js_command") || (conf_name == L"new_macro") || (conf_name == L"new_js_command") || (conf_name == L"keymap")) {
            std::wstring config_value;
            std::getline(ss, config_value);
            config_value = strip_string(config_value);
            int space_index = config_value.find(L" ");

            if (conf_name == L"keymap") {
                AdditionalKeymapData keymap_data;
                keymap_data.file_name = file_path.get_path();
                keymap_data.line_number = line_number;
                keymap_data.keymap_string = QString::fromStdWString(config_value).trimmed().toStdWString();
                ADDITIONAL_KEYMAPS.push_back(keymap_data);
            }
            else {
                std::wstring new_command_name = config_value.substr(0, space_index);
                if (new_command_name[0] == '_') {
                    std::wstring command_value = config_value.substr(space_index + 1, config_value.size() - space_index - 1);

                    if (conf_name == L"new_command") {
                        ADDITIONAL_COMMANDS[new_command_name] = command_value;
                    }
                    if (conf_name == L"new_macro") {
                        ADDITIONAL_MACROS[new_command_name] = command_value;
                    }
                    if (conf_name == L"new_js_command" || conf_name == L"new_async_js_command") {
                        QString qcommand_value = QString::fromStdWString(command_value);
                        QStringList parts = qcommand_value.split("::");
                        bool has_entry_point = parts.size() >= 2;

                        if (conf_name == L"new_js_command") {
                            if (has_entry_point) {
                                ADDITIONAL_JAVASCRIPT_COMMANDS[new_command_name] = JsCommandInfo{
                                    file_path.get_path(), parts.first().toStdWString(), parts.last().toStdWString()};
                            }
                            else {
                                ADDITIONAL_JAVASCRIPT_COMMANDS[new_command_name] = JsCommandInfo{
                                    file_path.get_path(), command_value, {} };
                            }
                        }
                        if (conf_name == L"new_async_js_command") {
                            if (has_entry_point) {
                                ADDITIONAL_ASYNC_JAVASCRIPT_COMMANDS[new_command_name] = JsCommandInfo{
                                    file_path.get_path(), parts.first().toStdWString(), parts.last().toStdWString()};
                            }
                            else {
                                ADDITIONAL_ASYNC_JAVASCRIPT_COMMANDS[new_command_name] = JsCommandInfo{
                                    file_path.get_path(), command_value, {} };
                            }
                        }
                    }
                }
            }
        }
        else {

            Config* conf = get_mut_config_with_name(conf_name);
            if (conf == nullptr) {
                std::wcout << L"Error: " << conf_name << L" is not a valid configuration name\n";
                continue;
            }

            std::wstring config_value;
            std::getline(ss, config_value);

            std::wstringstream config_value_stream(config_value);

            if (is_auto) {
                conf->is_auto = true;
            }

            if ((conf != nullptr) && (conf->validator != nullptr)) {
                if (!conf->validator(config_value)) {
                    std::wcout << L"Error in config file " << file_path.get_path() << L" at line " << line_number << L" : " << line << L"\n";
                    continue;
                }
            }

            if (conf) {
                bool changed = false;
                auto deserialization_result = conf->deserialize(config_value_stream, conf->value, &changed);
                if (deserialization_result != nullptr) {
                    conf->value = deserialization_result;
                }
                else {
                    std::wcout << L"Error in config file " << file_path.get_path() << L" at line " << line_number << L" : " << line << L"\n";
                }
                if (changed && (changed_config_names != nullptr)) {
                    changed_config_names->push_back(utf8_encode(conf->name));
                }
            }
        }

    }
    default_file.close();
}

void ConfigManager::deserialize(std::vector<std::string>* changed_config_names, const Path& default_file_path, const Path& auto_path, const std::vector<Path>& user_file_paths) {

    ADDITIONAL_COMMANDS.clear();
    ADDITIONAL_JAVASCRIPT_COMMANDS.clear();
    ADDITIONAL_ASYNC_JAVASCRIPT_COMMANDS.clear();
    ADDITIONAL_MACROS.clear();
    ADDITIONAL_KEYMAPS.clear();

    deserialize_file(changed_config_names, default_file_path);

    if (!NO_AUTO_CONFIG) {
        deserialize_file(changed_config_names, auto_path, false, true);
    }

    for (const auto& user_file_path : user_file_paths) {
        deserialize_file(changed_config_names, user_file_path);
    }
}

std::optional<Path> ConfigManager::get_or_create_user_config_file() {
    if (user_config_paths.size() == 0) {
        return {};
    }

    for (int i = user_config_paths.size() - 1; i >= 0; i--) {
        if (user_config_paths[i].file_exists()) {
            return user_config_paths[i];
        }
    }
    user_config_paths.back().file_parent().create_directories();
    create_file_if_not_exists(user_config_paths.back().get_path());
    return user_config_paths.back();
}

std::vector<Path> ConfigManager::get_all_user_config_files() {
    std::vector<Path> res;
    for (int i = user_config_paths.size() - 1; i >= 0; i--) {
        if (user_config_paths[i].file_exists()) {
            res.push_back(user_config_paths[i]);
        }
    }
    return  res;
}

std::vector<Config> ConfigManager::get_configs() {
    return configs;
}


std::vector<Config>* ConfigManager::get_configs_ptr() {
    return &configs;
}

bool ConfigManager::deserialize_config(std::string config_name, std::wstring config_value) {

    std::wstringstream config_value_stream(config_value);
    Config* conf = get_mut_config_with_name(utf8_decode(config_name));
    bool changed = false;
    auto deserialization_result = conf->deserialize(config_value_stream, conf->value, &changed);
    if (deserialization_result != nullptr) {
        conf->value = deserialization_result;
    }
    return changed;
}

ConfigModel::ConfigModel(std::vector<Config>* configs, QObject* parent) : QAbstractTableModel(parent), configs(configs) {
}

int ConfigModel::rowCount(const QModelIndex& parent) const {
    return configs->size();
}

int ConfigModel::columnCount(const QModelIndex& parent) const {
    return 3;
}
QVariant ConfigModel::data(const QModelIndex& index, int role) const {
    if (role == Qt::DisplayRole) {
        int col = index.column();
        int row = index.row();
        if (col < 0 || row < 0) {
            //return QAbstractTableModel::data(index, role);
            return QVariant::fromValue(QString(""));
        }

        const Config* conf = &((*configs)[row]);
        ConfigType config_type = conf->config_type;
        std::wstring config_name = conf->name;
        if (col == 0) {
            return QVariant::fromValue(QString::fromStdWString(conf->get_type_string()));
        }
        if (col == 1) {
            return QVariant::fromValue(QString::fromStdWString(config_name));
        }
        if (col == 2) {
            //std::wstringstream config_serialized;
            //conf->serialize(conf->value, config_serialized);
            //QVariant::fromValue(QString::fromStdWString(config_serialized.str()));
            if (config_type == ConfigType::Bool) {
                return QVariant::fromValue(*(bool*)conf->value);
            }

            if (config_type == ConfigType::Float) {
                QList<float> vals;
                FloatExtras extras = std::get<FloatExtras>(conf->extras);
                vals << *(float*)conf->value;
                vals << extras.min_val << extras.max_val;
                return QVariant::fromValue(vals);
            }
            if (config_type == ConfigType::Int) {
                QList<int> vals;
                IntExtras extras = std::get<IntExtras>(conf->extras);
                vals << *(int*)conf->value;
                vals << extras.min_val << extras.max_val;
                return QVariant::fromValue(vals);
            }

            if ((config_type == ConfigType::String) || (config_type == ConfigType::Macro) || (config_type == ConfigType::FilePath) || (config_type == ConfigType::FolderPath)) {
                //QColor::from
                return QVariant::fromValue(QString::fromStdWString(*(std::wstring*)(conf->value)));
            }
            if (config_type == ConfigType::Color3) {
                //QColor::from
                int out_rgb[3];
                convert_color3((float*)conf->value, out_rgb);
                return QVariant::fromValue(QColor(out_rgb[0], out_rgb[1], out_rgb[2]));
            }

            if (config_type == ConfigType::Color4) {
                //QColor::from
                int out_rgb[4];
                convert_color4((float*)conf->value, out_rgb);
                return QVariant::fromValue(QColor(out_rgb[0], out_rgb[1], out_rgb[2], out_rgb[3]));
            }

            //if (config_type == ConfigType::String) {
            //	//QColor::from
            //	return QVariant::fromValue(QString::fromStdWString(*(std::wstring*)conf->value));
            //}
            return QVariant::fromValue(QString(""));
        }

        //conf->name

    }
    return QVariant::fromValue(QString(""));
}

void Config::save_value_into_default() {
    default_value_string = L"";
    std::wstringstream default_value_string_stream(default_value_string);
    serialize(value, default_value_string_stream);
    default_value_string = default_value_string_stream.str();
}

void Config::load_default() {
    if (default_value_string.size() > 0) {
        std::wstringstream default_value_string_stream(default_value_string);
        deserialize(default_value_string_stream, value, nullptr);
    }
}

void ConfigManager::restore_defaults_in_memory() {
    for (auto& config : configs) {
        config.load_default();
    }
}

std::wstring Config::get_type_string() const{
    if (config_type == ConfigType::Bool) return L"bool";
    if (config_type == ConfigType::Int) return L"int";
    if (config_type == ConfigType::Float) return L"float";
    if (config_type == ConfigType::Color3) return L"color3";
    if (config_type == ConfigType::Color4) return L"color4";
    if (config_type == ConfigType::String) return L"string";
    if (config_type == ConfigType::FilePath) return L"filepath";
    if (config_type == ConfigType::FolderPath) return L"folderpath";
    if (config_type == ConfigType::Macro) return L"macro";
    if (config_type == ConfigType::IVec2) return L"ivec2";
    if (config_type == ConfigType::FVec2) return L"fvec2";
    if (config_type == ConfigType::EnableRectangle) return L"enablerectangle";
    if (config_type == ConfigType::Range) return L"range";
}

QRect UIRect::to_window(int window_width, int window_height) {
    int top_window = (int)(window_height * (top + 1) / 2.0f);
    int bottom_window = (int)(window_height * (bottom + 1) / 2.0f);
    int left_window = (int)(window_width * (left + 1) / 2.0f);
    int right_window = (int)(window_width * (right + 1) / 2.0f);
    return QRect(left_window, top_window, right_window - left_window, bottom_window - top_window);
}

std::wstring Config::get_current_string(){
    std::wstringstream value_string;
    serialize(value, value_string);
    return value_string.str();
}

bool Config::has_changed_from_default(){
    return get_current_string() != default_value_string;
}

std::vector<std::wstring> ConfigManager::get_auto_config_names() {
    std::vector<std::wstring> res;

    for (auto& c : configs) {
        if (c.is_auto) {
            res.push_back(c.name);
        }
    }
    return res;
}

bool Config::is_empty_string() {
    if ((config_type == ConfigType::String) || (config_type == ConfigType::Macro) || (config_type == ConfigType::FilePath) || (config_type == ConfigType::FolderPath)) {
        if (((std::wstring*)value)->size() == 0) {
            return true;
        }
    }
    return false;
}
