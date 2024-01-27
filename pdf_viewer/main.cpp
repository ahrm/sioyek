#include <iostream>
#include <vector>
#include <string>
#include <regex>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <thread>
#include <mutex>
#include <optional>
#include <utility>
#include <memory>
#include <filesystem>
#include <map>

#ifdef SIOYEK_ANDROID
#include <QtCore/private/qandroidextras_p.h>
#endif

#include <QDebug>
#include <qapplication.h>
#include <qpushbutton.h>
#ifndef SIOYEK_QT6
#include <qopenglwidget.h>
#endif
#include <qopenglextrafunctions.h>
#include <qopenglfunctions.h>
#include <qopengl.h>
#include <qwindow.h>
#include <qkeyevent.h>
#include <qlineedit.h>
#include <qtreeview.h>
#include <qsortfilterproxymodel.h>
#include <qabstractitemmodel.h>
#include <qopenglshaderprogram.h>
#include <qtimer.h>
#include <qdatetime.h>
#include <qstackedwidget.h>
#include <qboxlayout.h>
#include <qlistview.h>
#include <qstringlistmodel.h>
#include <qlabel.h>
#include <qtextedit.h>
#include <qfilesystemwatcher.h>

#ifndef SIOYEK_QT6
#include <qdesktopwidget.h>
#endif

#include <qfontdatabase.h>
#include <qstandarditemmodel.h>
#include <qscrollarea.h>
#include <qdesktopservices.h>
#include <qprocess.h>
#include <qstandardpaths.h>
#include <qcommandlineparser.h>
#include <qdir.h>
#include <qsurfaceformat.h>

#include <mupdf/fitz.h>
#include "sqlite3.h"


#include "input.h"
#include "database.h"
#include "book.h"
#include "utils.h"
#include "ui.h"
#include "pdf_renderer.h"
#include "document.h"
#include "document_view.h"
#include "pdf_view_opengl_widget.h"
#include "config.h"
#include "utf8.h"
#include "main_widget.h"
#include "path.h"
#include "RunGuard.h"
#include "checksum.h"
#include "OpenWithApplication.h"
#include "new_file_checker.h"

#define FTS_FUZZY_MATCH_IMPLEMENTATION
#include "fts_fuzzy_match.h"
#undef FTS_FUZZY_MATCH_IMPLEMENTATION

#ifndef MAX_PATH
#define MAX_PATH PATH_MAX
#endif

//#define LINUX_STANDARD_PATHS


int DATABASE_VERSION = 2;
std::wstring APPLICATION_NAME = L"sioyek";
std::string LOG_FILE_NAME = "sioyek_log.txt";
std::ofstream LOG_FILE;
int FONT_SIZE = -1;
int STATUS_BAR_FONT_SIZE = -1;
std::string APPLICATION_VERSION = "2.0.0";
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
bool DEBUG = false;
bool DEBUG_DISPLAY_FREEHAND_POINTS = false;
bool DEBUG_SMOOTH_FREEHAND_DRAWINGS = true;
#ifdef SIOYEK_ANDROID
bool TOUCH_MODE = true;
#else
bool TOUCH_MODE = false;
#endif

#ifdef SIOYEK_ANDROID
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
#ifndef SIOYEK_ANDROID
std::wstring STATUS_BAR_FORMAT = L"Page %{current_page} / %{num_pages}%{chapter_name}%{search_results}%{search_progress}%{link_status}%{waiting_for_symbol}%{indexing}%{preview_index}%{synctex}%{drag}%{presentation}%{visual_scroll}%{locked_scroll}%{highlight}%{freehand_drawing}%{closest_bookmark}%{close_portal}%{rect_select}%{custom_message}%{download}";
#else
std::wstring STATUS_BAR_FORMAT = L"# %{current_page} / %{num_pages}%{search_results}%{search_progress}%{link_status}%{indexing}";
#endif

int next_window_id = 0;
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
float VERTICAL_MOVE_AMOUNT = 1.0f;
float HORIZONTAL_MOVE_AMOUNT = 1.0f;
float MOVE_SCREEN_PERCENTAGE = 0.5f;
const unsigned int CACHE_INVALID_MILIES = 1000;
const int PERSIST_MILIES = 1000 * 60;
const int PAGE_PADDINGS = 0;
const int MAX_PENDING_REQUESTS = 31;
bool FLAT_TABLE_OF_CONTENTS = false;
bool SHOULD_USE_MULTIPLE_MONITORS = false;
bool SHOULD_CHECK_FOR_LATEST_VERSION_ON_STARTUP = false;
bool DEFAULT_DARK_MODE = false;
bool SORT_BOOKMARKS_BY_LOCATION = true;
std::wstring LIBGEN_ADDRESS = L"";
std::wstring GOOGLE_SCHOLAR_ADDRESS = L"";
std::wstring INVERSE_SEARCH_COMMAND = L"";
std::wstring SHARED_DATABASE_PATH = L"";
std::wstring BOOK_SCAN_PATH = L"";
std::wstring UI_FONT_FACE_NAME = L"";
std::wstring STATUS_FONT_FACE_NAME = L"";
std::wstring DEFAULT_OPEN_FILE_PATH = L"";
bool SHOULD_LOAD_TUTORIAL_WHEN_NO_OTHER_FILE = true;
bool SHOULD_LAUNCH_NEW_INSTANCE = false;
bool SHOULD_LAUNCH_NEW_WINDOW = false;
bool SHOULD_DRAW_UNRENDERED_PAGES = false;
bool PRESERVE_IMAGE_COLORS = false;
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

#ifdef SIOYEK_ANDROID
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
#ifdef SIOYEK_ANDROID
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
std::map<std::wstring, std::pair<std::wstring, std::wstring>> ADDITIONAL_JAVASCRIPT_COMMANDS;
std::map<std::wstring, std::pair<std::wstring, std::wstring>> ADDITIONAL_ASYNC_JAVASCRIPT_COMMANDS;
std::map<std::wstring, std::wstring> ADDITIONAL_MACROS;
std::vector<AdditionalKeymapData> ADDITIONAL_KEYMAPS;
bool PRERENDER_NEXT_PAGE = true;
bool EMACS_MODE = false;
bool HIGHLIGHT_MIDDLE_CLICK = false;
float HYPERDRIVE_SPEED_FACTOR = 10.0f;
float SMOOTH_SCROLL_SPEED = 3.0f;
float SMOOTH_SCROLL_DRAG = 3000.0f;
int PRERENDERED_PAGE_COUNT = 0;
std::wstring CONTEXT_MENU_ITEMS = L"";
bool RIGHT_CLICK_CONTEXT_MENU = false;

float PAGE_SEPARATOR_WIDTH = 0.0f;
float PAGE_SEPARATOR_COLOR[3] = { 0.9f, 0.9f, 0.9f };
bool SUPER_FAST_SEARCH = false;
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
bool AUTOCENTER_VISUAL_SCROLL = false;
bool ALPHABETIC_LINK_TAGS = false;
bool VIMTEX_WSL_FIX = false;
float RULER_AUTO_MOVE_SENSITIVITY = 40.0f;
float TTS_RATE = 0.0f;
bool VERBOSE = false;
bool FILL_TEXTBAR_WITH_SELECTED_TEXT = true;
int NUM_PRERENDERED_NEXT_SLIDES = 1;
int NUM_PRERENDERED_PREV_SLIDES = 0;
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

float EPUB_WIDTH = 400;
float EPUB_HEIGHT = 700;
float EPUB_FONT_SIZE = 14;
float EPUB_LINE_SPACING = 2.0f;
int RELOAD_INTERVAL_MILISECONDS = 200;
bool ADJUST_ANNOTATION_COLORS_FOR_DARK_MODE = true;

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

Path standard_data_path;
Path default_config_path(L"");
Path default_keys_path(L"");
std::vector<Path> user_config_paths = {};
std::vector<Path> user_keys_paths = {};
Path database_file_path(L"");
Path local_database_file_path(L"");
Path global_database_file_path(L"");
#ifdef SIOYEK_ANDROID
Path tutorial_path(L":/tutorial.pdf");
Path android_config_path(L"");
#else
Path tutorial_path(L"");
#endif
Path last_opened_file_address_path(L"");
Path shader_path(L"");
Path auto_config_path(L"");
Path downloaded_papers_path(L"");
ScratchPad global_scratchpad;

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

std::wstring BACK_RECT_TAP_COMMAND = L"history_back";
std::wstring BACK_RECT_HOLD_COMMAND = L"goto_mark";
std::wstring FORWARD_RECT_TAP_COMMAND = L"history_forward";
std::wstring FORWARD_RECT_HOLD_COMMAND = L"set_mark";
std::wstring EDIT_PORTAL_TAP_COMMAND = L"edit_portal";
std::wstring EDIT_PORTAL_HOLD_COMMAND = L"";
std::wstring VISUAL_MARK_NEXT_TAP_COMMAND = L"";
std::wstring VISUAL_MARK_NEXT_HOLD_COMMAND = L"";
std::wstring VISUAL_MARK_PREV_TAP_COMMAND = L"";
std::wstring VISUAL_MARK_PREV_HOLD_COMMAND = L"";
std::wstring MIDDLE_LEFT_RECT_TAP_COMMAND = L"";
std::wstring MIDDLE_LEFT_RECT_HOLD_COMMAND = L"";
#ifdef SIOYEK_ANDROID
std::wstring MIDDLE_RIGHT_RECT_TAP_COMMAND = L"overview_definition";
#else
std::wstring MIDDLE_RIGHT_RECT_TAP_COMMAND = L"";
#endif
std::wstring MIDDLE_RIGHT_RECT_HOLD_COMMAND = L"";

std::vector<MainWidget*> windows;

//std::vector<float> embedding_weights;
//std::vector<float> linear_weights;

std::wstring strip_uri(std::wstring pdf_file_name) {

    if (pdf_file_name.size() > 1) {
        if ((pdf_file_name[0] == '"') && (pdf_file_name[pdf_file_name.size() - 1] == '"')) {
            pdf_file_name = pdf_file_name.substr(1, pdf_file_name.size() - 2);
        }
        // support URIs like this: file:///home/user/file.pdf
        if (QString::fromStdWString(pdf_file_name).startsWith("file://")) {
            return pdf_file_name.substr(7, pdf_file_name.size() - 7);
        }
    }
    return pdf_file_name;
}

QStringList convert_arguments(QStringList input_args) {
    // convert the relative path of filename (if it exists) to absolute path

    QStringList output_args;

    //the first argument is always path of the executable
    output_args.push_back(input_args.at(0));
    input_args.pop_front();

    if (input_args.size() > 0) {
        QString path = input_args.at(0);

        bool is_path_argument = true;

        if (path.size() > 2 && path.startsWith("--")) {
            is_path_argument = false;
        }

        if (is_path_argument) {
            std::wstring path_wstring = strip_uri(path.toStdWString());
            Path path_object(path_wstring);
            output_args.push_back(QString::fromStdWString(path_object.get_path()));
            input_args.pop_front();
        }
    }
    for (int i = 0; i < input_args.size(); i++) {
        output_args.push_back(input_args.at(i));
    }

    return output_args;
}

#ifdef SIOYEK_ANDROID
void configure_paths_android() {

    char* APPDIR = std::getenv("XDG_CONFIG_HOME");
    Path linux_home_path(QDir::homePath().toStdWString());

    if (!APPDIR) {
        APPDIR = std::getenv("HOME");
    }

    standard_data_path = Path(utf8_decode(APPDIR));
    standard_data_path = standard_data_path.slash(L".local").slash(L"share").slash(L"Sioyek");
    standard_data_path.create_directories();

    database_file_path = standard_data_path.slash(L"test.db");
    last_opened_file_address_path = standard_data_path.slash(L"last_document_path.txt");
    local_database_file_path = standard_data_path.slash(L"local.db");
    global_database_file_path = standard_data_path.slash(L"shared.db");
    android_config_path = standard_data_path.slash(L"saved.config");
    tutorial_path = Path(L":/tutorial.pdf");
    downloaded_papers_path = standard_data_path.slash(L"downloads");
}
#endif

void configure_paths() {
#ifdef SIOYEK_ANDROID
    configure_paths_android();
#else


    Path parent_path(QCoreApplication::applicationDirPath().toStdWString());
    std::string exe_path = utf8_encode(QCoreApplication::applicationFilePath().toStdWString());

    shader_path = parent_path.slash(L"shaders");


#ifdef Q_OS_MACOS
    Path mac_home_path(QDir::homePath().toStdWString());
    Path mac_standard_config_path = mac_home_path.slash(L".config").slash(L"sioyek");
    user_keys_paths.push_back(mac_standard_config_path.slash(L"keys_user.config"));
    user_config_paths.push_back(mac_standard_config_path.slash(L"prefs_user.config"));
#endif

#ifdef Q_OS_LINUX
    QStringList all_config_paths = QStandardPaths::standardLocations(QStandardPaths::AppConfigLocation);
    for (int i = all_config_paths.size() - 1; i >= 0; i--) {
        user_config_paths.push_back(Path(all_config_paths.at(i).toStdWString()).slash(L"prefs_user.config"));
        user_keys_paths.push_back(Path(all_config_paths.at(i).toStdWString()).slash(L"keys_user.config"));
    }

#ifdef LINUX_STANDARD_PATHS
    Path home_path(QDir::homePath().toStdWString());
    standard_data_path = home_path.slash(L".local").slash(L"share").slash(L"sioyek");
    Path standard_config_path = Path(L"/etc/sioyek");
    Path read_only_data_path = Path(L"/usr/share/sioyek");
    standard_data_path.create_directories();

    default_config_path = standard_config_path.slash(L"prefs.config");
    default_keys_path = standard_config_path.slash(L"keys.config");

    database_file_path = standard_data_path.slash(L"test.db");
    local_database_file_path = standard_data_path.slash(L"local.db");
    global_database_file_path = standard_data_path.slash(L"shared.db");
    tutorial_path = read_only_data_path.slash(L"tutorial.pdf");
    last_opened_file_address_path = standard_data_path.slash(L"last_document_path.txt");
    shader_path = read_only_data_path.slash(L"shaders");
#else
    char* APPDIR = std::getenv("XDG_CONFIG_HOME");
    Path linux_home_path(QDir::homePath().toStdWString());

    if (!APPDIR) {
        APPDIR = std::getenv("HOME");
    }

    standard_data_path = Path(utf8_decode(APPDIR));
    standard_data_path = standard_data_path.slash(L".local").slash(L"share").slash(L"Sioyek");
    standard_data_path.create_directories();

    default_config_path = parent_path.slash(L"prefs.config");
    //user_config_path = standard_data_path.slash(L"prefs_user.config");
    user_config_paths.push_back(standard_data_path.slash(L"prefs_user.config"));
    default_keys_path = parent_path.slash(L"keys.config");
    user_keys_paths.push_back(standard_data_path.slash(L"keys_user.config"));
    database_file_path = standard_data_path.slash(L"test.db");
    local_database_file_path = standard_data_path.slash(L"local.db");
    global_database_file_path = standard_data_path.slash(L"shared.db");
    tutorial_path = standard_data_path.slash(L"tutorial.pdf");
    last_opened_file_address_path = standard_data_path.slash(L"last_document_path.txt");

    Path linux_standard_config_path = linux_home_path.slash(L".config").slash(L"sioyek");
    //user_keys_paths.push_back(mac_standard_config_path.slash(L"keys_user.config"));
    //user_config_paths.push_back(mac_standard_config_path.slash(L"prefs_user.config"));
    user_keys_paths.push_back(linux_standard_config_path.slash(L"keys_user.config"));
    user_config_paths.push_back(linux_standard_config_path.slash(L"prefs_user.config"));

    if (!tutorial_path.file_exists()) {
        copy_file(parent_path.slash(L"tutorial.pdf"), tutorial_path);
    }
#endif
#else //windows
#ifdef NDEBUG
    //install_app(exe_path.c_str());
#endif
    standard_data_path = Path(QStandardPaths::standardLocations(QStandardPaths::AppDataLocation).at(0).toStdWString());
    QStringList all_config_paths = QStandardPaths::standardLocations(QStandardPaths::AppConfigLocation);

    standard_data_path.create_directories();

    default_config_path = parent_path.slash(L"prefs.config");
    default_keys_path = parent_path.slash(L"keys.config");
    tutorial_path = parent_path.slash(L"tutorial.pdf");

#ifdef NON_PORTABLE
    user_config_paths.push_back(standard_data_path.slash(L"prefs_user.config"));
    user_keys_paths.push_back(standard_data_path.slash(L"keys_user.config"));
    for (int i = all_config_paths.size() - 1; i > 0; i--) {
        user_config_paths.push_back(Path(all_config_paths.at(i).toStdWString()).slash(L"prefs_user.config"));
        user_keys_paths.push_back(Path(all_config_paths.at(i).toStdWString()).slash(L"keys_user.config"));
    }
    database_file_path = standard_data_path.slash(L"test.db");
    local_database_file_path = standard_data_path.slash(L"local.db");
    global_database_file_path = standard_data_path.slash(L"shared.db");
    last_opened_file_address_path = standard_data_path.slash(L"last_document_path.txt");
#else
    user_config_paths.push_back(parent_path.slash(L"prefs_user.config"));
    user_keys_paths.push_back(parent_path.slash(L"keys_user.config"));
    database_file_path = parent_path.slash(L"test.db");
    local_database_file_path = parent_path.slash(L"local.db");
    global_database_file_path = parent_path.slash(L"shared.db");
    last_opened_file_address_path = parent_path.slash(L"last_document_path.txt");
#endif

#endif
    auto_config_path = standard_data_path.slash(L"auto.config");
    // user_config_paths.insert(user_config_paths.begin(), auto_config_path);
    downloaded_papers_path = standard_data_path.slash(L"downloads");
#endif

}

void verify_config_paths() {
#define CHECK_DIR_EXIST(path) do{ if(!(path).dir_exists() ) std::wcout << L"Error: " << #path << ": " << path << L" doesn't exist!\n"; } while(false)
#define CHECK_FILE_EXIST(path) do{ if(!(path).file_exists() ) std::wcout << L"Error: " << #path << ": " << path << L" doesn't exist!\n"; } while(false)

    LOG(std::wcout << L"default_config_path: " << default_config_path << L"\n");
    CHECK_FILE_EXIST(default_config_path);
    LOG(std::wcout << L"default_keys_path: " << default_keys_path << L"\n");
    CHECK_FILE_EXIST(default_keys_path);
    for (size_t i = 0; i < user_config_paths.size(); i++) {
        LOG(std::wcout << L"user_config_path: [ " << i << " ] " << user_config_paths[i] << L"\n");
    }
    for (size_t i = 0; i < user_keys_paths.size(); i++) {
        LOG(std::wcout << L"user_keys_path: [ " << i << " ] " << user_keys_paths[i] << L"\n");
    }
}

void verify_paths() {
    LOG(std::wcout << L"database_file_path: " << database_file_path << L"\n");
    LOG(std::wcout << L"local_database_file_path: " << local_database_file_path << L"\n");
    LOG(std::wcout << L"global_database_file_path: " << global_database_file_path << L"\n");
    LOG(std::wcout << L"tutorial_path: " << tutorial_path << L"\n");
    LOG(std::wcout << L"last_opened_file_address_path: " << last_opened_file_address_path << L"\n");
    LOG(std::wcout << L"shader_path: " << shader_path << L"\n");
    CHECK_DIR_EXIST(shader_path);
}
#undef CHECK_DIR_EXIST
#undef CHECK_FILE_EXIST

std::mutex mupdf_mutexes[FZ_LOCK_MAX];

void lock_mutex(void* user, int lock) {
    std::mutex* mut = (std::mutex*)user;
    (mut + lock)->lock();
}

void unlock_mutex(void* user, int lock) {
    std::mutex* mut = (std::mutex*)user;
    (mut + lock)->unlock();
}

void add_paths_to_file_system_watcher(QFileSystemWatcher& watcher, const Path& default_path, const std::vector<Path>& user_paths) {
    if (QFile::exists(QString::fromStdWString(default_path.get_path()))) {
        watcher.addPath(QString::fromStdWString(default_path.get_path()));
    }

    for (auto user_path : user_paths) {
        if (QFile::exists(QString::fromStdWString(user_path.get_path()))) {
            watcher.addPath(QString::fromStdWString(user_path.get_path()));
        }
    }
}

MainWidget* get_window_with_window_id(int window_id) {
    for (auto window : windows) {
        if (window->get_window_id() == window_id) return window;
    }
    return nullptr;
}

MainWidget* get_window_with_opened_file_path(const std::wstring& file_path) {
    if (!QFile::exists(QString::fromStdWString(file_path))) {
        return nullptr;
    }

    if (file_path.size() > 0) {
        for (auto window : windows) {
            //if (window->doc() && window->doc()->get_path() == file_path) {

            if (window->doc()) {

#ifdef Q_OS_WIN
                std::wstring path1 = window->doc()->get_path();
                std::wstring path2 = file_path;
#else
                std::string path1 = utf8_encode(window->doc()->get_path());
                std::string path2 = utf8_encode(file_path);
#endif
                if (std::filesystem::equivalent(path1, path2)) {
                    return window;
                }
            }
        }
    }
    return nullptr;
}

std::vector<std::wstring> get_last_opened_file_name() {
    std::string file_path_;
    std::ifstream last_state_file(last_opened_file_address_path.get_path_utf8());
    std::vector<std::wstring> res;
    while (std::getline(last_state_file, file_path_)) {
        res.push_back(utf8_decode(file_path_));
    }
    last_state_file.close();

    return res;
    //if (file_path_.size() > 0) {
    //    return utf8_decode(file_path_);
    //}
    //else {
    //    return {};
    //}
}

void invalidate_render() {
    for (auto window : windows) {
        window->invalidate_render();
    }
}

MainWidget* handle_args(const QStringList& arguments, QLocalSocket* origin=nullptr) {
    std::optional<int> page = -1;
    std::optional<float> x_loc, y_loc;
    std::optional<float> zoom_level;

    std::vector<std::wstring> aarguments;
    for (int i = 0; i < arguments.size(); i++) {
        aarguments.push_back(arguments.at(i).toStdWString());
    }

    //todo: handle out of bounds error

    QCommandLineParser* parser = get_command_line_parser();

    if (!parser->parse(arguments)) {
        std::wcout << parser->errorText().toStdWString() << L"\n";
        return nullptr;
    }

    std::wstring pdf_file_name = L"";

    if (parser->positionalArguments().size() > 0) {
        pdf_file_name = parser->positionalArguments().at(0).toStdWString();
    }
    else {
        if (windows[0]->doc() == nullptr) {
            // when no file is specified, and no current file is open, use the last opened file or tutorial
            std::vector<std::wstring> last_opened_file_paths = get_last_opened_file_name();
            if (last_opened_file_paths.size() > 0) {
                pdf_file_name = last_opened_file_paths[0];
                windows[0]->open_tabs(last_opened_file_paths);
            }
            else {
                pdf_file_name = tutorial_path.get_path();
            }
        }
    }

    std::optional<std::wstring> latex_file_name = {};
    std::optional<int> latex_line = {};
    std::optional<int> latex_column = {};

    if (parser->isSet("forward-search-file")) {
        latex_file_name = parser->value("forward-search-file").toStdWString();
    }

    if (parser->isSet("forward-search-line")) {
        latex_line = parser->value("forward-search-line").toInt();
    }

    if (parser->isSet("forward-search-column")) {
        latex_column = parser->value("forward-search-column").toInt();
    }

    if (parser->isSet("page")) {

        int page_int = parser->value("page").toInt();
        // 1 is the index for the first page (not 0)
        if (page_int > 0) page_int--;
        page = page_int;
    }

    if (parser->isSet("zoom")) {
        zoom_level = parser->value("zoom").toFloat();
    }

    if (parser->isSet("xloc")) {
        x_loc = parser->value("xloc").toFloat();
    }

    if (parser->isSet("yloc")) {
        y_loc = parser->value("yloc").toFloat();
    }

    pdf_file_name = strip_uri(pdf_file_name);

    if ((pdf_file_name.size() > 0) && (!QFile::exists(QString::fromStdWString(pdf_file_name)))) {
#ifdef SIOYEK_ANDROID
        if (!((pdf_file_name[0] == ':') || (pdf_file_name.substr(0, 2) == L"/:"))) {
            return nullptr;
        }
#else
        return nullptr;
#endif
    }

    MainWidget* target_window = nullptr;
    if (parser->isSet("window-id")) {
        target_window = get_window_with_window_id(parser->value("window-id").toInt());
    }
    else {
        target_window = get_window_with_opened_file_path(pdf_file_name);
    }

    bool should_create_new_window = false;
    if (pdf_file_name.size() > 0) {
        if (parser->isSet("new-window")) {
            should_create_new_window = true;
        }
        if (SHOULD_LAUNCH_NEW_WINDOW && (target_window == nullptr) && (!parser->isSet("reuse-window"))) {
            should_create_new_window = true;
        }
        if (windows[0]->doc() == nullptr) {
            should_create_new_window = false;
        }

    }
    else {
        if (parser->isSet("new-window")) {
            should_create_new_window = true;
        }
    }

    if (should_create_new_window) {
        target_window = new MainWidget(windows[0]);
        target_window->execute_macro_if_enabled(STARTUP_COMMANDS);
        target_window->apply_window_params_for_one_window_mode(true);
        target_window->show();
        windows.push_back(target_window);
    }
    if (target_window == nullptr) {
        target_window = windows[0];
    }

    if (parser->isSet("inverse-search")) {
        if (target_window) {
            target_window->set_inverse_search_command(parser->value("inverse-search").toStdWString());
            target_window->raise();
            //target_window->activateWindow();
        }
    }
    if (parser->isSet("execute-command")) {
        QString command_string = parser->value("execute-command");
        QString command_data = parser->value("execute-command-data");
        if (command_data.size() > 0) {
            command_string += QString::fromStdString("(") + command_data + QString::fromStdString(")");
        }

        if (!parser->isSet("wait-for-response")) {
            origin = nullptr;
        }

        target_window->execute_macro_from_origin(command_string.toStdWString(), origin);
    }

    if (parser->isSet("focus-text")) {
        QString text = parser->value("focus-text");
        int page = parser->value("focus-text-page").toInt();
        target_window->focus_text(page, text.toStdWString());
    }

    // if no file is specified, use the previous file
    if (pdf_file_name == L"" && (windows[0]->doc() != nullptr)) {
        if (target_window->doc()) {
            pdf_file_name = target_window->doc()->get_path();
        }
        else {
            if (windows.size() > 1) {
                if (windows[windows.size() - 2]->doc()) {
                    pdf_file_name = windows[windows.size() - 2]->doc()->get_path();
                }
            }
            else {
                pdf_file_name = tutorial_path.get_path();
            }
        }
    }

    if (page != -1) {
        if (target_window) {
            target_window->push_state();
            target_window->open_document_at_location(pdf_file_name, page.value_or(0), x_loc, y_loc, zoom_level);
        }
    }
    else if (latex_file_name) {
        if (target_window) {
            target_window->do_synctex_forward_search(pdf_file_name, latex_file_name.value(), latex_line.value_or(0), latex_column.value_or(0));
        }
    }
    else {
        bool should_open_document = false;
        if ((target_window->doc() == nullptr) || (target_window->doc()->get_path() != pdf_file_name)) {
            should_open_document = true;
        }
        if (should_open_document) {
            target_window->push_state();
            target_window->open_document(pdf_file_name);
        }
    }

    invalidate_render();

    delete parser;
    return target_window;
}

void focus_on_widget(QWidget* widget) {
    widget->activateWindow();
    widget->setWindowState(widget->windowState() & ~Qt::WindowMinimized | Qt::WindowActive);
}


int main(int argc, char* args[]) {

    SEARCH_URLS['s' - 'a'] = L"https://scholar.google.com/scholar?q=";
    SEARCH_URLS['g' - 'a'] = L"https://www.google.com/search?q=";
#ifdef SIOYEK_ANDROID

    auto r = QtAndroidPrivate::checkPermission("android.permission.WRITE_EXTERNAL_STORAGE").result();
    if (r == QtAndroidPrivate::Denied) {
        r = QtAndroidPrivate::requestPermission("android.permission.WRITE_EXTERNAL_STORAGE").result();

        if (r == QtAndroidPrivate::Denied) {
            qDebug() << "Could not get storage permission\n";
        }
    }

#endif

    if (has_arg(argc, args, "--version")) {
        std::cout << "sioyek " << APPLICATION_VERSION << "\n";
        return 0;
    }
    if (has_arg(argc, args, "--verbose")) {
        VERBOSE = true;
    }

    if (has_arg(argc, args, "--no-auto-config")) {
        NO_AUTO_CONFIG = true;
    }

    int nrows, ncols;

    //load_npy(":/data/embedding.npy", embedding_weights, &nrows, &ncols);
    //load_npy(":/data/linear.npy", linear_weights, &nrows, &ncols);

    QSurfaceFormat format;
#ifdef SIOYEK_ANDROID
    format.setVersion(3, 1);
#else
    format.setVersion(3, 3);
#endif

    //    auto behaviour = format.swapBehavior();
    //    format.setSwapBehavior(QSurfaceFormat::SwapBehavior::SingleBuffer);
    //    format.setSwapInterval(0);
    format.setProfile(QSurfaceFormat::CoreProfile);
    format.setSamples(4);
    QSurfaceFormat::setDefaultFormat(format);

    QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts, true);
    OpenWithApplication app(argc, args);

#ifdef Q_OS_WIN
    // handles dark mode on windows. see: https://github.com/ahrm/sioyek/issues/3
    app.setStyle("fusion");
#endif

    qmlRegisterType<MySortFilterProxyModel>("MySortFilterProxyModel", 1, 0, "MySortFilterProxyModel");
    QCommandLineParser* parser = get_command_line_parser();
    parser->process(app.arguments());

    configure_paths();
    verify_config_paths();


#ifdef SIOYEK_ANDROID
    ConfigManager config_manager(android_config_path, auto_config_path, user_config_paths);
#else
    ConfigManager config_manager(default_config_path, auto_config_path, user_config_paths);
#endif
    CommandManager* command_manager = new CommandManager(&config_manager);

    if (PAPERS_FOLDER_PATH.size() > 0) {
        downloaded_papers_path = PAPERS_FOLDER_PATH;
    }

    if (SHARED_DATABASE_PATH.size() > 0) {
        global_database_file_path = SHARED_DATABASE_PATH;
    }
    char* shared_database_path_arg = get_argv_value(argc, args, "--shared-database-path");
    if (shared_database_path_arg) {
        global_database_file_path = utf8_decode(std::string(shared_database_path_arg));
    }

    verify_paths();

    // should we launche a new instance each time the user opens a PDF or should we reuse the previous instance
    bool use_single_instance = (!SHOULD_LAUNCH_NEW_INSTANCE) && (!SHOULD_LAUNCH_NEW_WINDOW);

    if (should_reuse_instance(argc, args)) {
        use_single_instance = true;
    }
    else if (should_new_instance(argc, args)) {
        use_single_instance = false;
    }

#ifndef SIOYEK_ANDROID
    RunGuard guard("sioyek");
    if (!guard.isPrimary()) {
        QStringList sent_args = convert_arguments(app.arguments());
        bool should_wait = parser->isSet("wait-for-response");
        std::string res = guard.sendMessage(serialize_string_array(sent_args), should_wait);
        std::wcout << utf8_decode(res);
        return 0;
    }
#endif

#ifdef SIOYEK_ANDROID
    if (VOLUME_UP_COMMAND.size() > 0 || VOLUME_DOWN_COMMAND.size() > 0){
        qputenv("QT_ANDROID_VOLUME_KEYS", "1");
    }
#endif


    QCoreApplication::setApplicationName(QString::fromStdWString(APPLICATION_NAME));
    QCoreApplication::setApplicationVersion(QString::fromStdString(APPLICATION_VERSION));

    QStringList positional_args = parser->positionalArguments();
    delete parser;

    DatabaseManager db_manager;
    if (local_database_file_path.file_exists() && global_database_file_path.file_exists()) {
        db_manager.open(local_database_file_path.get_path(), global_database_file_path.get_path());
    }
    else {
        db_manager.open(database_file_path.get_path(), database_file_path.get_path());
    }
    db_manager.ensure_database_compatibility(local_database_file_path.get_path(), global_database_file_path.get_path());
    db_manager.ensure_schema_compatibility();


    fz_locks_context locks;
    locks.user = mupdf_mutexes;
    locks.lock = lock_mutex;
    locks.unlock = unlock_mutex;

    fz_context* mupdf_context = fz_new_context(nullptr, &locks, FZ_STORE_DEFAULT);

    if (!VERBOSE) {
        fz_set_warning_callback(mupdf_context, nullptr, nullptr);
        fz_set_error_callback(mupdf_context, nullptr, nullptr);
    }

    if (!mupdf_context) {
        std::cerr << "could not create mupdf context" << std::endl;
        return -1;
    }
    bool fail = false;
    fz_try(mupdf_context) {
        fz_register_document_handlers(mupdf_context);
    }
    fz_catch(mupdf_context) {
        std::cerr << "could not register document handlers" << std::endl;
        fail = true;
    }

    if (fail) {
        return -1;
    }

    bool quit = false;

    InputHandler input_handler(default_keys_path, user_keys_paths, command_manager);

    std::vector<std::pair<std::wstring, std::wstring>> prev_path_hash_pairs;
    db_manager.get_prev_path_hash_pairs(prev_path_hash_pairs);

    CachedChecksummer checksummer(&prev_path_hash_pairs);

    DocumentManager document_manager(mupdf_context, &db_manager, &checksummer);

    QFileSystemWatcher pref_file_watcher;
    add_paths_to_file_system_watcher(pref_file_watcher, default_config_path, user_config_paths);

    QFileSystemWatcher key_file_watcher;
    add_paths_to_file_system_watcher(key_file_watcher, default_keys_path, user_keys_paths);
    QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGL);


    MainWidget* main_widget = new MainWidget(mupdf_context, &db_manager, &document_manager, &config_manager, command_manager, &input_handler, &checksummer, &quit);
    windows.push_back(main_widget);

#ifndef SIOYEK_ANDROID
    guard.on_delete = std::move([&](QLocalSocket* deleted_socket) {
        main_widget->on_socket_deleted(deleted_socket);
        });
#endif

    if (DEFAULT_DARK_MODE) {
        main_widget->toggle_dark_mode();
    }

    //QString startup_commands_list = QString::fromStdWString(STARTUP_COMMANDS);
    //QStringList startup_commands = startup_commands_list.split(";");
    NewFileChecker new_file_checker(PAPERS_FOLDER_PATH, main_widget);


#ifndef SIOYEK_ANDROID
    if (guard.isPrimary()) {
        QObject::connect(&guard, &RunGuard::messageReceived, [&main_widget](const QByteArray& message, QLocalSocket* socket) {
            QStringList args = deserialize_string_array(message);
            bool nofocus = args.indexOf("--nofocus") != -1;
            MainWidget* target = handle_args(args, socket);
            if (!nofocus) {
                if (target) {
                    //target->activateWindow();
                    focus_on_widget(target);
                }
                else if (windows.size() > 0) {
                    //windows[0]->activateWindow();
                    focus_on_widget(windows[0]);
                }
            }
            });
    }
#endif


    main_widget->topLevelWidget()->resize(500, 500);

    if (START_WITH_HELPER_WINDOW && (HELPER_WINDOW_SIZE[0] > -1)) {
        main_widget->apply_window_params_for_two_window_mode();
    }
    else {
        main_widget->apply_window_params_for_one_window_mode();
    }

    main_widget->show();

    handle_args(app.arguments());
    main_widget->execute_macro_if_enabled(STARTUP_COMMANDS);

    //main_widget->run_multiple_commands(STARTUP_COMMANDS);

    // load input file from `QFileOpenEvent` for macOS drag and drop & "open with"
    QObject::connect(&app, &OpenWithApplication::file_ready, [&main_widget](const QString& file_name) {
        handle_args(QStringList() << QCoreApplication::applicationFilePath() << file_name);
        });

    // live reload the config files, no need to live reload on android because we are not changing config files anyway
#ifndef SIOYEK_ANDROID
    QObject::connect(&pref_file_watcher, &QFileSystemWatcher::fileChanged, [&]() {

        std::vector<std::string> changed_config_file_names;
        config_manager.deserialize(&changed_config_file_names, default_config_path, auto_config_path, user_config_paths);
        input_handler.reload_config_files(default_keys_path, user_keys_paths);

        ConfigFileChangeListener::notify_config_file_changed(&config_manager);
        for (auto window : windows) {
            window->validate_render();
            window->on_configs_changed(&changed_config_file_names);
        }
        add_paths_to_file_system_watcher(pref_file_watcher, default_config_path, user_config_paths);
        });

    QObject::connect(&key_file_watcher, &QFileSystemWatcher::fileChanged, [&]() {
        input_handler.reload_config_files(default_keys_path, user_keys_paths);
        add_paths_to_file_system_watcher(key_file_watcher, default_keys_path, user_keys_paths);
        });
#endif


    if (SHOULD_CHECK_FOR_LATEST_VERSION_ON_STARTUP) {
        check_for_updates(main_widget, APPLICATION_VERSION);
    }

    app.exec();

    quit = true;

    std::vector<MainWidget*> windows_to_delete;
    for (size_t i = 0; i < windows.size(); i++) {
        windows_to_delete.push_back(windows[i]);
    }
    for (size_t i = 0; i < windows_to_delete.size(); i++) {
        delete windows_to_delete[i];
    }

    return 0;
}
