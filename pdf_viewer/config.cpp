#include "config.h"
#include "utils.h"
#include <cassert>
#include <map>
#include <qdir.h>
//#include <ui.h>

extern float ZOOM_INC_FACTOR;
extern float GAMMA;
extern float VERTICAL_MOVE_AMOUNT;
extern float HORIZONTAL_MOVE_AMOUNT;
extern float MOVE_SCREEN_PERCENTAGE;
extern float BACKGROUND_COLOR[3];
extern float UNSELECTED_SEARCH_HIGHLIGHT_COLOR[3];
extern float DARK_MODE_BACKGROUND_COLOR[3];
extern float CUSTOM_COLOR_MODE_EMPTY_BACKGROUND_COLOR[3];
extern float DARK_MODE_CONTRAST;
extern bool FLAT_TABLE_OF_CONTENTS;
extern bool SMALL_TOC;
extern bool SHOULD_USE_MULTIPLE_MONITORS;
extern bool SORT_BOOKMARKS_BY_LOCATION;
extern bool SHOULD_LOAD_TUTORIAL_WHEN_NO_OTHER_FILE;
extern bool SHOULD_LAUNCH_NEW_INSTANCE;
extern bool SHOULD_LAUNCH_NEW_WINDOW;
extern bool SHOULD_CHECK_FOR_LATEST_VERSION_ON_STARTUP;
extern bool SHOULD_DRAW_UNRENDERED_PAGES;
extern bool HOVER_OVERVIEW;
//extern bool AUTO_EMBED_ANNOTATIONS;
extern bool DEFAULT_DARK_MODE;
extern bool USE_SYSTEM_THEME;
extern bool USE_CUSTOM_COLOR_FOR_DARK_SYSTEM_THEME;
extern float HIGHLIGHT_COLORS[26 * 3];
extern std::wstring SEARCH_URLS[26];
extern std::wstring EXECUTE_COMMANDS[26];
extern std::wstring LIBGEN_ADDRESS;
extern std::wstring GOOGLE_SCHOLAR_ADDRESS;
extern std::wstring INVERSE_SEARCH_COMMAND;
extern std::wstring SHARED_DATABASE_PATH;
extern std::wstring ITEM_LIST_PREFIX;
extern float VISUAL_MARK_NEXT_PAGE_FRACTION;
extern float VISUAL_MARK_NEXT_PAGE_THRESHOLD;
extern std::wstring UI_FONT_FACE_NAME;
extern std::wstring STATUS_FONT_FACE_NAME;
extern std::wstring MIDDLE_CLICK_SEARCH_ENGINE;
extern std::wstring SHIFT_MIDDLE_CLICK_SEARCH_ENGINE;
extern std::wstring STARTUP_COMMANDS;
extern std::wstring SHUTDOWN_COMMANDS;
extern int FONT_SIZE;
extern int RULER_UNDERLINE_PIXEL_WIDTH;
extern float CUSTOM_BACKGROUND_COLOR[3];
extern float CUSTOM_TEXT_COLOR[3];
extern bool RERENDER_OVERVIEW;
extern bool WHEEL_ZOOM_ON_CURSOR;
extern bool RULER_MODE;
extern bool LINEAR_TEXTURE_FILTERING;
extern float DISPLAY_RESOLUTION_SCALE;
extern float STATUS_BAR_COLOR[3];
extern float STATUS_BAR_TEXT_COLOR[3];
extern float UI_BACKGROUND_COLOR[3];
extern float UI_TEXT_COLOR[3];
extern float UI_SELECTED_TEXT_COLOR[3];
extern float UI_SELECTED_BACKGROUND_COLOR[3];
extern int STATUS_BAR_FONT_SIZE;
extern int MAIN_WINDOW_SIZE[2];
extern int HELPER_WINDOW_SIZE[2];
extern int MAIN_WINDOW_MOVE[2];
extern int HELPER_WINDOW_MOVE[2];
extern float TOUCHPAD_SENSITIVITY;
extern float SCROLL_VIEW_SENSITIVITY;
extern float PAGE_SEPARATOR_WIDTH;
extern float PAGE_SEPARATOR_COLOR[3];
extern int SINGLE_MAIN_WINDOW_SIZE[2];
extern int SINGLE_MAIN_WINDOW_MOVE[2];
extern float FIT_TO_PAGE_WIDTH_RATIO;
extern float RULER_PADDING;
extern float RULER_X_PADDING;
extern std::wstring TEXT_HIGHLIGHT_URL;
extern bool TEXT_SUMMARY_HIGHLIGHT_SHOULD_REFINE;
extern bool TEXT_SUMMARY_HIGHLIGHT_SHOULD_FILL;
extern int TEXT_SUMMARY_CONTEXT_SIZE;
extern bool USE_HEURISTIC_IF_TEXT_SUMMARY_NOT_AVAILABLE;
extern std::wstring PAPERS_FOLDER_PATH;
extern bool ENABLE_EXPERIMENTAL_FEATURES;
extern bool CREATE_TABLE_OF_CONTENTS_IF_NOT_EXISTS;
extern int MAX_CREATED_TABLE_OF_CONTENTS_SIZE;
extern bool FORCE_CUSTOM_LINE_ALGORITHM;
extern float OVERVIEW_SIZE[2];
extern float OVERVIEW_OFFSET[2];
extern bool IGNORE_WHITESPACE_IN_PRESENTATION_MODE;
extern bool EXACT_HIGHLIGHT_SELECT;
extern bool SHOW_DOC_PATH;
extern float FASTREAD_OPACITY;
extern bool SHOULD_WARN_ABOUT_USER_KEY_OVERRIDE;
extern bool SINGLE_CLICK_SELECTS_WORDS;
extern float EPUB_LINE_SPACING;
extern float FREETEXT_BOOKMARK_COLOR[3];
extern float FREETEXT_BOOKMARK_FONT_SIZE;
extern bool RENDER_FREETEXT_BORDERS;
extern bool SHOULD_RENDER_PDF_ANNOTATIONS;
extern float STRIKE_LINE_WIDTH;
extern float RULER_COLOR[3];
extern float RULER_MARKER_COLOR[3];
extern bool PAPER_DOWNLOAD_CREATE_PORTAL;
extern bool PAPER_DOWNLOAD_AUTODETECT_PAPER_NAME;
extern bool AUTOMATICALLY_DOWNLOAD_MATCHING_PAPER_NAME;
extern std::wstring BOOK_SCAN_PATH;
extern bool AUTO_RENAME_DOWNLOADED_PAPERS;
extern bool ADJUST_ANNOTATION_COLORS_FOR_DARK_MODE;
extern bool PRESERVE_IMAGE_COLORS;
extern bool INVERTED_PRESERVED_IMAGE_COLORS;
extern std::wstring TABLET_PEN_CLICK_COMMAND;
extern std::wstring TABLET_PEN_DOUBLE_CLICK_COMMAND;
extern std::wstring VOLUME_DOWN_COMMAND;
extern std::wstring VOLUME_UP_COMMAND;
extern int DOCUMENTATION_FONT_SIZE;
extern int NUM_PRERENDERED_NEXT_SLIDES;
extern int NUM_PRERENDERED_PREV_SLIDES;
extern int NUM_CACHED_PAGES;
extern bool INVERT_SELECTED_TEXT;
extern bool IGNORE_SCROLL_EVENTS;
extern bool DONT_FOCUS_IF_SYNCTEX_RECT_IS_VISIBLE;

extern std::vector<AdditionalKeymapData> ADDITIONAL_KEYMAPS;

extern bool REAL_PAGE_SEPARATION;
extern bool NO_AUTO_CONFIG;
extern bool HIDE_OVERLAPPING_LINK_LABELS;
extern bool FILL_TEXTBAR_WITH_SELECTED_TEXT;
extern bool ALIGN_LINK_DEST_TO_TOP;
extern float MENU_SCREEN_WDITH_RATIO;
extern float PAGE_SPACE_X;
extern float PAGE_SPACE_Y;

extern std::wstring RESIZE_COMMAND;
extern std::wstring SHIFT_CLICK_COMMAND;
extern std::wstring CONTROL_CLICK_COMMAND;
extern std::wstring RIGHT_CLICK_COMMAND;
extern std::wstring MIDDLE_CLICK_COMMAND;
extern std::wstring SHIFT_RIGHT_CLICK_COMMAND;
extern std::wstring CONTROL_RIGHT_CLICK_COMMAND;
extern std::wstring ALT_CLICK_COMMAND;
extern std::wstring ALT_RIGHT_CLICK_COMMAND;
extern std::wstring HOLD_MIDDLE_CLICK_COMMAND;

extern bool SHOW_RIGHT_CLICK_CONTEXT_MENU;
extern std::wstring CONTEXT_MENU_ITEMS;
extern std::wstring CONTEXT_MENU_ITEMS_FOR_LINKS;
extern std::wstring CONTEXT_MENU_ITEMS_FOR_SELECTED_TEXT;
extern std::wstring CONTEXT_MENU_ITEMS_FOR_HIGHLIGHTS;
extern std::wstring CONTEXT_MENU_ITEMS_FOR_BOOKMARKS;
extern std::wstring CONTEXT_MENU_ITEMS_FOR_OVERVIEW;
extern bool RIGHT_CLICK_CONTEXT_MENU;
extern bool ALLOW_HORIZONTAL_DRAG_WHEN_DOCUMENT_IS_SMALL;

extern std::wstring BACK_RECT_TAP_COMMAND;
extern std::wstring BACK_RECT_HOLD_COMMAND;
extern std::wstring FORWARD_RECT_TAP_COMMAND;
extern std::wstring FORWARD_RECT_HOLD_COMMAND;
extern std::wstring EDIT_PORTAL_TAP_COMMAND;
extern std::wstring EDIT_PORTAL_HOLD_COMMAND;
extern std::wstring VISUAL_MARK_NEXT_TAP_COMMAND;
extern std::wstring VISUAL_MARK_NEXT_HOLD_COMMAND;
extern std::wstring VISUAL_MARK_PREV_TAP_COMMAND;
extern std::wstring VISUAL_MARK_PREV_HOLD_COMMAND;
extern std::wstring MIDDLE_LEFT_RECT_TAP_COMMAND;
extern std::wstring MIDDLE_LEFT_RECT_HOLD_COMMAND;
extern std::wstring MIDDLE_RIGHT_RECT_TAP_COMMAND;
extern std::wstring MIDDLE_RIGHT_RECT_HOLD_COMMAND;

extern bool ALLOW_MAIN_VIEW_SCROLL_WHILE_IN_OVERVIEW;
extern bool USE_LEGACY_KEYBINDS;
extern bool MULTILINE_MENUS;
extern bool START_WITH_HELPER_WINDOW;
extern std::map<std::wstring, std::wstring> ADDITIONAL_COMMANDS;
extern std::map<std::wstring, std::wstring> ADDITIONAL_MACROS;
extern std::map<std::wstring, JsCommandInfo> ADDITIONAL_JAVASCRIPT_COMMANDS;
extern std::map<std::wstring, JsCommandInfo> ADDITIONAL_ASYNC_JAVASCRIPT_COMMANDS;
extern bool PRERENDER_NEXT_PAGE;
extern bool HIGHLIGHT_MIDDLE_CLICK;
extern float HYPERDRIVE_SPEED_FACTOR;
extern float SMOOTH_SCROLL_SPEED;
extern float SMOOTH_SCROLL_DRAG;
extern bool SUPER_FAST_SEARCH;
extern bool INCREMENTAL_SEARCH;
extern bool SHOW_CLOSEST_BOOKMARK_IN_STATUSBAR;
extern int PRERENDERED_PAGE_COUNT;
extern bool CASE_SENSITIVE_SEARCH;
extern bool SMARTCASE_SEARCH;
extern bool SHOW_DOCUMENT_NAME_IN_STATUSBAR;
extern bool SHOW_CLOSE_PORTAL_IN_STATUSBAR;
extern bool NUMERIC_TAGS;
extern bool SHOULD_HIGHLIGHT_LINKS;
extern bool SHOULD_HIGHLIGHT_UNSELECTED_SEARCH;
extern int KEYBOARD_SELECT_FONT_SIZE;
extern bool FUZZY_SEARCHING;
extern float CUSTOM_COLOR_CONTRAST;
extern bool DEBUG;
extern bool DEBUG_DISPLAY_FREEHAND_POINTS;
extern bool DEBUG_SMOOTH_FREEHAND_DRAWINGS;
extern float HIGHLIGHT_DELETE_THRESHOLD;
extern std::wstring DEFAULT_OPEN_FILE_PATH;
extern std::wstring STATUS_BAR_FORMAT;
extern std::wstring RIGHT_STATUS_BAR_FORMAT;
extern bool INVERTED_HORIZONTAL_SCROLLING;
extern bool TOC_JUMP_ALIGN_TOP;
extern float KEYBOARD_SELECT_BACKGROUND_COLOR[4];
extern float KEYBOARD_SELECT_TEXT_COLOR[4];
extern float KEYBOARD_SELECTED_TAG_TEXT_COLOR[4];
extern float KEYBOARD_SELECTED_TAG_BACKGROUND_COLRO[4];

extern bool AUTOCENTER_VISUAL_SCROLL;
extern bool ALPHABETIC_LINK_TAGS;
extern bool VIMTEX_WSL_FIX;
extern float RULER_AUTO_MOVE_SENSITIVITY;
extern bool SLICED_RENDERING;
extern bool TOUCH_MODE;
extern float TTS_RATE;
extern std::wstring PAPER_SEARCH_URL;
extern std::wstring RULER_DISPLAY_MODE;
extern bool USE_RULER_TO_HIGHLIGHT_SYNCTEX_LINE;
extern bool SHOW_MOST_RECENT_COMMANDS_FIRST;
extern bool USE_KEYBOARD_POINT_SELECTION;
extern std::wstring TAG_FONT_FACE;

extern float EPUB_WIDTH;
extern float EPUB_HEIGHT;
extern float EPUB_FONT_SIZE;
extern std::wstring EPUB_CSS;

extern float SMOOTH_MOVE_MAX_VELOCITY;
//extern float SMOOTH_MOVE_INITIAL_VELOCITY;

extern UIRect PORTRAIT_BACK_UI_RECT;
extern UIRect PORTRAIT_FORWARD_UI_RECT;
extern UIRect LANDSCAPE_BACK_UI_RECT;
extern UIRect LANDSCAPE_FORWARD_UI_RECT;

extern UIRect PORTRAIT_VISUAL_MARK_PREV;
extern UIRect PORTRAIT_VISUAL_MARK_NEXT;
extern UIRect LANDSCAPE_VISUAL_MARK_PREV;
extern UIRect LANDSCAPE_VISUAL_MARK_NEXT;

extern UIRect PORTRAIT_MIDDLE_LEFT_UI_RECT;
extern UIRect PORTRAIT_MIDDLE_RIGHT_UI_RECT;
extern UIRect LANDSCAPE_MIDDLE_LEFT_UI_RECT;
extern UIRect LANDSCAPE_MIDDLE_RIGHT_UI_RECT;

extern float DEFAULT_TEXT_HIGHLIGHT_COLOR[3];
extern float DEFAULT_VERTICAL_LINE_COLOR[4];
extern float DEFAULT_SEARCH_HIGHLIGHT_COLOR[3];
extern float DEFAULT_LINK_HIGHLIGHT_COLOR[3];
extern float DEFAULT_SYNCTEX_HIGHLIGHT_COLOR[3];
extern float HIDE_SYNCTEX_HIGHLIGHT_TIMEOUT;

extern std::wstring PAPER_SEARCH_URL_PATH;
extern std::wstring PAPER_SEARCH_TILE_PATH;
extern std::wstring PAPER_SEARCH_CONTRIB_PATH;

extern int RELOAD_INTERVAL_MILISECONDS;

#ifdef Q_OS_MACOS
extern float MACOS_TITLEBAR_COLOR[3];
extern bool MACOS_HIDE_TITLEBAR;
#endif

#ifdef SIOYEK_ANDROID
extern Path android_config_path;
#endif

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
    add_bool(L"debug", &DEBUG);
    add_bool(L"inverted_horizontal_scrolling", &INVERTED_HORIZONTAL_SCROLLING);
    add_bool(L"toc_jump_align_top", &TOC_JUMP_ALIGN_TOP);
    add_bool(L"autocenter_visual_scroll", &AUTOCENTER_VISUAL_SCROLL);
    add_bool(L"alphabetic_link_tags", &ALPHABETIC_LINK_TAGS);
    add_bool(L"vimtex_wsl_fix", &VIMTEX_WSL_FIX);
    add_bool(L"sliced_rendering", &SLICED_RENDERING);
    add_bool(L"touch_mode", &TOUCH_MODE);
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
    add_macro(L"top_center_tap_command", &EDIT_PORTAL_TAP_COMMAND);
    add_macro(L"top_center_hold_command", &EDIT_PORTAL_HOLD_COMMAND);
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
