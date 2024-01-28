#pragma once

#ifndef MAIN_WIDGET_DEFINE
#define MAIN_WIDGET_DEFINE

#include <string>
#include <memory>
#include <vector>
#include <optional>
#include <deque>
#include <mutex>

#include <qnetworkaccessmanager.h>
#include <qquickwidget.h>
#include <qjsondocument.h>

#include "book.h"
#include "input.h"
#include "path.h"

extern float VERTICAL_MOVE_AMOUNT;
extern float HORIZONTAL_MOVE_AMOUNT;

class SelectionIndicator;
class QLocalSocket;
class QLineEdit;
class QTextEdit;
class QTimer;
class QDragEvent;
class QDropEvent;
class QScrollBar;
class QTextToSpeech;
class QStringListModel;
class QLabel;
class TouchTextSelectionButtons;
class DrawControlsUI;
class SearchButtons;
class HighlightButtons;

struct fz_context;
struct fz_stext_char;

class DocumentView;
class ScratchPad;
class Document;
class InputHandler;
class ConfigManager;
class PdfRenderer;
class CachedChecksummer;
class CommandManager;
class Command;
class PdfViewOpenGLWidget;
class DatabaseManager;
class DocumentManager;

enum class DrawingMode {
    Drawing,
    PenDrawing,
    None
};


struct TextUnderPointerInfo{
    ReferenceType reference_type;
    int page;
    float offset;
    AbsoluteRect source_rect;
    std::wstring source_text;
    std::vector<DocumentRect> overview_highlight_rects;
};

struct BookmarkMoveData {
    int index;
    AbsoluteDocumentPos initial_begin_position;
    AbsoluteDocumentPos initial_end_position;
    AbsoluteDocumentPos initial_mouse_position;
};


struct PortalMoveData {
    int index;
    AbsoluteDocumentPos initial_position;
    AbsoluteDocumentPos initial_mouse_position;
    bool is_pending = false;
};


struct SelectedDrawings {
    int page;
    AbsoluteRect selection_absrect;
    std::vector<SelectedObjectIndex> selected_indices;
};

struct PendingDownloadPortal {
    Portal pending_portal;
    std::wstring paper_name;
    std::wstring source_document_path;
    // the pending portal is marked for deletion
    bool marked = false;
};

struct FreehandDrawingMoveData {
    std::vector<FreehandDrawing> initial_drawings;
    std::vector<PixmapDrawing> initial_pixmaps;
    AbsoluteDocumentPos initial_mouse_position;
};

enum class PaperDownloadFinishedAction {
    None,
    OpenInSameWindow,
    OpenInNewWindow,
    Portal
};

// if we inherit from QWidget there are problems on high refresh rate smartphone displays
class MainWidget : public QQuickWidget {
    Q_OBJECT
public:
    fz_context* mupdf_context = nullptr;
    DatabaseManager* db_manager = nullptr;
    DocumentManager* document_manager = nullptr;
    CommandManager* command_manager = nullptr;
    ConfigManager* config_manager = nullptr;
    QNetworkAccessManager network_manager;
    PdfRenderer* pdf_renderer = nullptr;
    InputHandler* input_handler = nullptr;
    CachedChecksummer* checksummer = nullptr;
    int window_id;

    QTextToSpeech* tts = nullptr;
    // is the TTS engine currently reading text?
    bool is_reading = false;
    bool word_by_word_reading = false;
    bool tts_has_pause_resume_capability = false;
    bool tts_is_about_to_finish = false;
    std::wstring tts_text = L"";
    std::vector<PagelessDocumentRect> tts_corresponding_line_rects;
    std::vector<PagelessDocumentRect> tts_corresponding_char_rects;
    std::optional<PagelessDocumentRect> last_focused_rect = {};

    PdfViewOpenGLWidget* opengl_widget = nullptr;
    PdfViewOpenGLWidget* helper_opengl_widget_ = nullptr;
    QScrollBar* scroll_bar = nullptr;

    QJsonDocument commands_doc_json_document;
    QJsonDocument config_doc_json_document;

    // Some commands can not be executed immediately (e.g. because they require a text or symbol
    // input to be completed) this is where they are stored until they can be executed.
    std::unique_ptr<Command> pending_command_instance = nullptr;
    std::vector<Command*> commands_being_performed;

    DocumentView* main_document_view = nullptr;
    ScratchPad* scratchpad = nullptr;
    DocumentView* helper_document_view_ = nullptr;

    // A stack of widgets to be displayed (e.g. the bookmark menu or the touch main menu).
    // only the top widget is visible. When a widget is popped, the previous widget in the stack
    // will be shown
    std::vector<QWidget*> current_widget_stack;

    // code to execute when a command is pressed (for example when we type `goto_beginning` in the command)
    // menu, we will call on_command_done("goto_beginning"). The reason that this is a closure instead of just a
    // method is historical. I am too lazy to change it.
    std::function<void(std::string, std::string)> on_command_done = nullptr;

    // List of previous locations in the current session. Note that we keep the history even across files
    // hence why `DocumentViewState` has a `document_path` member
    std::vector<DocumentViewState> history;
    // the index in the `history` array that we will jump to when `prev_state` is called.
    int current_history_index = -1;

    // custom message to be displayed in sioyek's statusbar
    std::wstring custom_status_message = L"";

    // A flag which indicates whether the application should quit. We use this to inform other threads
    // (e.g. the PDF rendering thread) that they should exit.
    bool* should_quit = nullptr;

    // last position when mouse was clicked in absolute document space
    AbsoluteDocumentPos last_mouse_down;
    // The document offset (offset_x and offset_y) when mouse was last pressed
    // we use this to update the offset when dragging the mouse in some modes
    // for example in touch mode or when dragging while holding middle mouse button
    AbsoluteDocumentPos last_mouse_down_document_offset;

    // last window position when mouse was clicked, we use this in mouse drag mode
    WindowPos last_mouse_down_window_pos;

    // begin/end position of the current text selection
    AbsoluteDocumentPos selection_begin;
    AbsoluteDocumentPos selection_end;

    // when moving the text selection using keyboard, `selection_begin` and `selection_end`
    // might be out of sync with `selected_text_`. `selected_text_is_dirty` is true when this
    // is the case, which means that we need to update `selected_text_` before using it.
    bool selected_text_is_dirty = false;

    // selected text (using mouse cursor or other methods) which is used e.g. for copying or highlighting
    std::wstring selected_text;

    // whether we are in rect/point select mode (some commands require a rectangle to be executed
    // for example `delete_freehand_drawings`)
    bool rect_select_mode = false;
    bool point_select_mode = false;

    // begin/end of current selected rectangle
    std::optional<AbsoluteDocumentPos> rect_select_begin = {};
    std::optional<AbsoluteDocumentPos> rect_select_end = {};

    std::optional<BookmarkMoveData> bookmark_move_data = {};
    std::optional<PortalMoveData> portal_move_data = {};

    // when set, mouse wheel moves the ruler
    bool visual_scroll_mode = false;
    bool debug_mode = false;

    bool horizontal_scroll_locked = false;

    // is the user currently selecing text? (happens when we left click and move the cursor)
    bool is_selecting = false;
    // is the user in word select mode? (happens when we double left click and move the cursor)
    bool is_word_selecting = false;

    // in select highlight mode, we immediately highlight the text when it is selected
    // with highlight type of `select_highlight_type` 
    bool is_select_highlight_mode = false;
    char select_highlight_type = 'a';

    // color type to use when freehand drawing
    char current_freehand_type = 'r';
    // line thickness of freehand drawings
    float freehand_thickness = 1.0f;

    // in smooth scroll mode we scroll the document smoothly instead of jumping to the target
    // `smooth_scroll_speed` is used to keep track of our speed in this mode
    bool smooth_scroll_mode = false;
    float smooth_scroll_speed = 0.0f;

    // the timer which periodically checks if the UI/rendering needs updating. Normally the timer value is
    // set to be INTERVAL_TIME (which is 200ms at the time of writing this comment), however, it is set to a much
    // lower value when in smooth scroll mode or when user flicks a document in touch mode
    QTimer* validation_interval_timer = nullptr;

    // the portal to be edited. This is usually set by `edit_portal` command which jumps to the portal
    // when we go back to the original location by jumping back in history, the portal will be edited
    // to be the new document view state
    std::optional<Portal> portal_to_edit = {};

    // the index of highlight in doc()->get_highlights() that is selected. This is used to
    // delete/edit highlights e.g. by selecting a highlight by clicking on it and then executing `delete_highlight`
    int selected_highlight_index = -1;
    int selected_bookmark_index = -1;
    int selected_portal_index = -1;

    std::optional<SelectedDrawings> selected_freehand_drawings = {};
    std::optional<FreehandDrawingMoveData> freehand_drawing_move_data = {};

    // An incomplete portal that is being created. The source of the portal is filled
    // but the destination still needs to be set.
    std::optional<std::pair<std::optional<std::wstring>, Portal>> pending_portal;

    // the current freehand drawing mode. None means we are not drawing anything
    // Drawing means we use the mouse to draw a freehand diagram
    // and PenDrawing means we assume the user is using a pen so we treat mouse inputs
    // normally and only use pen inputs for freehand drawing
    DrawingMode freehand_drawing_mode = DrawingMode::None;

    // In mouse drag mode, we use the mouse to drag the document around
    bool mouse_drag_mode = false;

    // In synctex mode right clicking on a document will jump to the corresponding latex file
    // (assuming `inverse_search_command` is properly configured)
    bool synctex_mode = false;

    // are we currently dragging the document
    bool is_dragging = false;
    // are we performing pinch to zoom gesture
    bool is_pinching = false;

    // are we currently freehand drawing on the document
    bool is_drawing = false;

    // should we show the status label?
    // If touch mode is enabled, we don't show the status label at all, unless there is another window
    // (e.g. the main menu or search buttons) is visible. That is because the screen space is already
    // very limited in touch devices and we don't want to waste it using a statusbar unless it is absolutely required
    bool should_show_status_label_ = true;
    bool should_show_status_label();

    // the location of current character in sioyek's typing minigame
    std::optional<CharacterAddress> typing_location;

    int main_window_width = 0;
    int main_window_height = 0;

    QMap<QString, QVariant> js_variables;

    QWidget* text_command_line_edit_container = nullptr;
    QLabel* text_command_line_edit_label = nullptr;
    QLineEdit* text_command_line_edit = nullptr;
    QLabel* status_label = nullptr;
    int text_suggestion_index = 0;

    std::deque<std::wstring> search_terms;

    // determines if the widget render is invalid and needs to be updated
    // when `validation_interval_timer` fired, we check if this is true
    // and only then we re-render the widget
    // note that this is different from the document being invalid (an example of document
    // being invalid could be a latex document that is changed since being loaded). This just means that
    // the current drawing of MainWidget is not correct (for example due to moving vertically)
    bool is_render_invalidated = false;

    // determines if the UI is invalid and needs to be updated
    // this can be the case for example when updating the search progress
    // note that when render is invalid we automatically validate the UI anyway
    bool is_ui_invalidated = false;

    // this is a niche option to show the last executed command in the status bar
    // (I used it when recording sioyek's video tutorial to show the command being executed)
    bool should_show_last_command = false;

    // last page to be fit when we are in smart fit mode
    // this value not being `{}` indicates that we are in smart fit mode
    // which means that every time page is changed, we execute `fit_to_page_width_smart`
    std::optional<int> last_smart_fit_page = {};

    // the command used to perform synctex inverse searches
    std::wstring inverse_search_command;

    // data used to resize/move the overview. which is a small window which shows the target
    // destination of references/links
    std::optional<OverviewMoveData> overview_move_data = {};
    std::optional<OverviewTouchMoveData> overview_touch_move_data = {};
    std::optional<OverviewResizeData> overview_resize_data = {};

    std::vector<PendingDownloadPortal> pending_download_portals;

    // A list of candiadates to be shown in the overview window. We use simple heuristics to determine the
    // target of references, while this works most of the time, it is not perfect. So we keep a list of candidates
    // which the user can naviagte through using `next_preview` and `previous_preview` commands which move
    // `index_into_candidates` pointer to the next/previous candidate
    std::vector<SmartViewCandidate> smart_view_candidates;
    int index_into_candidates = 0;

    // when selecting text, we update the rendering faster, this timer is used 
    // so that we don't update the rendering too fast
    QTime last_text_select_time = QTime::currentTime();

    // last time we updated `smooth_scroll_speed` 
    QTime last_speed_update_time = QTime::currentTime();

    std::vector<QJSEngine*> available_engines;
    std::mutex available_engine_mutex;
    int num_js_engines = 0;

    bool main_document_view_has_document();
    std::optional<std::string> get_last_opened_file_checksum();


    int get_current_colorscheme_index();
    void set_dark_mode();
    void set_light_mode();
    void set_custom_color_mode();
    void toggle_statusbar();
    void toggle_titlebar();

    void add_text_annotation_to_selected_highlight(const std::wstring& annot_text);

    // search the `paper_name` in one of the configurable when middle-click or shift+middle-clicking on paper's name
    void handle_search_paper_name(std::wstring paper_name, bool is_shift_pressed);

    void persist(bool persist_drawings = false);
    bool is_pending_link_source_filled();
    std::wstring get_status_string();
    void handle_escape();
    bool is_waiting_for_symbol();
    void key_event(bool released, QKeyEvent* kevent);
    void handle_left_click(WindowPos click_pos, bool down, bool is_shift_pressed, bool is_control_pressed, bool is_alt_pressed);
    void handle_right_click(WindowPos click_pos, bool down, bool is_shift_pressed, bool is_control_pressed, bool is_alt_pressed);
    void on_config_changed(std::string config_name);
    void on_configs_changed(std::vector<std::string>* config_names);

    void next_state();
    void prev_state();
    void update_current_history_index();

    void set_main_document_view_state(DocumentViewState new_view_state);
    void handle_click(WindowPos pos);

    void update_selected_bookmark_font_size();
    //bool eventFilter(QObject* obj, QEvent* event) override;
    void set_command_textbox_text(const std::wstring& txt);
    void change_selected_highlight_type(char new_type);
    void change_selected_bookmark_text(const std::wstring& new_text);
    void change_selected_highlight_text_annot(const std::wstring& new_text);
    char get_current_selected_highlight_type();
    void show_textbar(const std::wstring& command_name, const std::wstring& initial_value = L"");
    void show_mark_selector();
    void toggle_two_window_mode();
    void toggle_window_configuration();
    void handle_portal();
    void start_creating_rect_portal(AbsoluteDocumentPos location);
    void add_portal(std::wstring source_path, Portal new_link);
    void toggle_fullscreen();
    void toggle_presentation_mode();
    void set_presentation_mode(bool mode);
    void set_synctex_mode(bool mode);
    void toggle_synctex_mode();
    void complete_pending_link(const PortalViewState& destination_view_state);
    void long_jump_to_destination(DocumentPos pos);
    void long_jump_to_destination(int page, float offset_y);
    void long_jump_to_destination(float abs_offset_y);
    void execute_command(std::wstring command, std::wstring text = L"", bool wait = false);
    //QString get_status_stylesheet();
    void smart_jump_under_pos(WindowPos pos);
    bool overview_under_pos(WindowPos pos);
    void visual_mark_under_pos(WindowPos pos);
    bool is_network_manager_running(bool* is_downloading = nullptr);
    void show_download_paper_menu(
        const std::vector<std::wstring>& paper_names,
        const std::vector<std::wstring>& download_urls,
        std::wstring paper_name,
        PaperDownloadFinishedAction action);
    QNetworkReply* download_paper_with_url(std::wstring paper_url, bool use_archive_url, PaperDownloadFinishedAction action);

    QRect get_main_window_rect();
    QRect get_helper_window_rect();

    void show_password_prompt_if_required();
    void handle_link_click(const PdfLink& link);

    void on_next_text_suggestion();
    void on_prev_text_suggestion();
    void set_current_text_suggestion();

    std::wstring get_window_configuration_string();
    std::wstring get_serialized_configuration_string();
    void save_auto_config();

    void handle_close_event();
    void return_to_last_visual_mark();
    bool is_visual_mark_mode();
    void reload(bool flush = true);

    void reset_highlight_links();
    void set_rect_select_mode(bool mode);
    void set_point_select_mode(bool mode);
    void clear_selected_rect();
    void clear_selected_text();
    void toggle_pdf_annotations();

    void expand_selection_vertical(bool begin, bool below);

    std::optional<AbsoluteRect> get_selected_rect_absolute();
    std::optional<DocumentRect> get_selected_rect_document();
    Document* doc();

    MainWidget(
        fz_context* mupdf_context,
        DatabaseManager* db_manager,
        DocumentManager* document_manager,
        ConfigManager* config_manager,
        CommandManager* command_manager,
        InputHandler* input_handler,
        CachedChecksummer* checksummer,
        bool* should_quit_ptr,
        QWidget* parent = nullptr
    );
    MainWidget(MainWidget* other);

    ~MainWidget();

    //void handle_command(NewCommand* command, int num_repeats);
    bool handle_command_types(std::unique_ptr<Command> command, int num_repeats, std::wstring* result=nullptr);
    void handle_pending_text_command(std::wstring text);

    void invalidate_render();
    void invalidate_ui();
    void open_document(const Path& path, std::optional<float> offset_x = {}, std::optional<float> offset_y = {}, std::optional<float> zoom_level = {});
    void open_document_with_hash(const std::string& hash, std::optional<float> offset_x = {}, std::optional<float> offset_y = {}, std::optional<float> zoom_level = {});
    void open_document_at_location(const Path& path, int page, std::optional<float> x_loc, std::optional<float> y_loc, std::optional<float> zoom_level);
    void open_document(const DocumentViewState& state);
    void open_document(const PortalViewState& checksum);
    void validate_render();
    void validate_ui();
    void zoom(WindowPos pos, float zoom_factor, bool zoom_in);
    bool move_document(float dx, float dy, bool force = false);
    void move_document_screens(int num_screens);
    void focus_text(int page, const std::wstring& text);
    int get_page_intersecting_rect_index(DocumentRect rect);
    std::optional<AbsoluteRect> get_page_intersecting_rect(DocumentRect rect);
    void focus_rect(DocumentRect rect);
    std::optional<float> move_visual_mark_next_get_offset();

    void move_visual_mark_next();
    void move_visual_mark_prev();
    AbsoluteRect move_visual_mark(int offset);
    void on_config_file_changed(ConfigManager* new_config);
    void toggle_mouse_drag_mode();
    void toggle_freehand_drawing_mode();
    void exit_freehand_drawing_mode();
    void toggle_pen_drawing_mode();
    void set_pen_drawing_mode(bool enabled);
    void set_hand_drawing_mode(bool enabled);
    void handle_drawing_ui_visibilty();

    void toggle_dark_mode();
    void toggle_custom_color_mode();
    void do_synctex_forward_search(const Path& pdf_file_path, const Path& latex_file_path, int line, int column);
    //void handle_args(const QStringList &arguments);
    void update_link_with_opened_book_state(Portal lnk, const OpenedBookState& new_state);
    void update_closest_link_with_opened_book_state(const OpenedBookState& new_state);
    void set_current_widget(QWidget* new_widget);
    void push_current_widget(QWidget* new_widget);
    void pop_current_widget(bool canceled=false);
    void show_current_widget();
    bool focus_on_visual_mark_pos(bool moving_down);
    void toggle_visual_scroll_mode();
    void set_overview_link(PdfLink link);
    void set_overview_position(int page, float offset);
    ReferenceType find_location_of_selected_text(int* out_page, float* out_offset, AbsoluteRect* out_rect, std::wstring* out_source_text, std::vector<DocumentRect>* out_highlight_rects=nullptr);
    TextUnderPointerInfo find_location_of_text_under_pointer(DocumentPos docpos, bool update_candidates = false);
    std::optional<std::wstring> get_current_file_name();
    CommandManager* get_command_manager();

    void move_vertical(float amount);
    bool move_horizontal(float amount, bool force = false);
    void get_window_params_for_one_window_mode(int* main_window_size, int* main_window_move);
    void get_window_params_for_two_window_mode(int* main_window_size, int* main_window_move, int* helper_window_size, int* helper_window_move);
    void apply_window_params_for_one_window_mode(bool force_resize = false);
    void apply_window_params_for_two_window_mode();
    bool helper_window_overlaps_main_window();
    void highlight_words();
    void highlight_ruler_portals();

    std::vector<PagelessDocumentRect> get_flat_words(std::vector<std::vector<PagelessDocumentRect>>* flat_word_chars = nullptr);

    // get rects using tags (tags are strings shown when executing `keyboard_*` commands)
    std::optional<PagelessDocumentRect> get_tag_rect(std::string tag, std::vector<PagelessDocumentRect>* word_chars = nullptr);
    std::optional<WindowRect> get_tag_window_rect(std::string tag, std::vector<WindowRect>* char_rects = nullptr);

    bool is_rotated();
    void on_new_paper_added(const std::wstring& file_path);
    void scroll_overview(int vertical_amount, int horizontal_amount=0);
    int get_current_page_number() const;
    std::wstring get_current_page_label();
    void goto_page_with_page_number(int page_number);
    void goto_page_with_label(std::wstring label);
    void set_inverse_search_command(const std::wstring& new_command);
    int get_current_monitor_width(); int get_current_monitor_height();
    std::wstring synctex_under_pos(WindowPos position);
    std::optional<std::wstring> get_paper_name_under_cursor(bool use_last_hold_point = false);
    fz_stext_char* get_closest_character_to_cusrsor(QPoint pos);
    void set_status_message(std::wstring new_status_string);
    void remove_self_from_windows();
    //void handle_additional_command(std::wstring command_name, bool wait=false);
    std::optional<DocumentPos> get_overview_position();
    void handle_keyboard_select(const std::wstring& text);
    //void run_multiple_commands(const std::wstring& commands);
    void push_state(bool update = true);
    void toggle_scrollbar();
    void update_scrollbar();
    void handle_portal_overview_update();
    void goto_overview();
    bool is_rect_visible(DocumentRect rect);
    void set_mark_in_current_location(char symbol);
    void goto_mark(char symbol);
    void advance_command(std::unique_ptr<Command> command, std::wstring* result=nullptr);
    void add_search_term(const std::wstring& term);
    void perform_search(std::wstring text, bool is_regex = false, bool is_incremental=false);
    void overview_to_definition();
    void portal_to_definition();
    void move_visual_mark_command(int amount);
    void handle_goto_loaded_document();

    void handle_vertical_move(int amount);
    void handle_horizontal_move(int amount);
    void handle_goto_portal_list();
    void handle_goto_bookmark();
    void handle_goto_bookmark_global();
    std::wstring handle_add_highlight(char symbol);
    void handle_goto_highlight();
    void handle_goto_highlight_global();
    void handle_goto_toc();
    void handle_open_prev_doc();
    void handle_open_all_docs();
    void handle_move_screen(int amount);
    MainWidget* handle_new_window();
    void handle_open_link(const std::wstring& text, bool copy = false);
    void handle_overview_link(const std::wstring& text);
    void handle_portal_to_link(const std::wstring& text);
    void handle_keys_user_all();
    void handle_prefs_user_all();
    void handle_portal_to_overview();
    void handle_focus_text(const std::wstring& text);
    void handle_goto_window();
    void handle_toggle_smooth_scroll_mode();
    void handle_overview_to_portal();
    void handle_toggle_typing_mode();
    void handle_delete_all_highlights();
    void handle_delete_highlight_under_cursor();
    void handle_delete_selected_highlight();
    void handle_start_reading();
    void handle_stop_reading();
    void handle_play();
    void handle_undo_drawing();
    void handle_pause();
    void read_current_line();
    void download_paper_under_cursor(bool use_last_touch_pos = false);
    std::optional<std::wstring> get_direct_paper_name_under_pos(DocumentPos docpos);
    std::optional<std::wstring> get_paper_name_under_pos(DocumentPos docpos, bool clean=false);
    QNetworkReply* download_paper_with_name(const std::wstring& name, PaperDownloadFinishedAction action);
    bool is_pos_inside_selected_text(DocumentPos docpos);
    void handle_debug_command();
    void handle_add_marked_data();
    void handle_undo_marked_data();
    void handle_remove_marked_data();
    void handle_export_marked_data();
    void handle_goto_random_page();
    void hande_turn_on_all_drawings();
    void hande_turn_off_all_drawings();
    void handle_toggle_drawing_mask(char symbol);
    void show_command_palette();

    DocumentPos get_document_pos_under_window_pos(WindowPos window_pos);
    AbsoluteDocumentPos get_absolute_document_pos_under_window_pos(WindowPos window_pos);

    std::string get_current_mode_string();

    void show_audio_buttons();
    void set_freehand_thickness(float val);

    // Text selection indicators in touch mode
    SelectionIndicator* selection_begin_indicator = nullptr;
    SelectionIndicator* selection_end_indicator = nullptr;

    // When in touch mode, sometimes we use the last touch hold point for some commands
    // for example, if select text button is pressed, we select the text under the last touch hold point
    QPoint last_hold_point;
    QPoint last_press_point;
    qint64 last_press_msecs = 0;
    QTime last_quick_tap_time;
    QTime last_middle_down_time;
    QPoint last_quick_tap_position;
    std::optional<QPoint> context_menu_right_click_pos = {};

    // whether mouse is pressed, `is_pressed` is true, we add mouse positions to `position_buffer`
    bool is_pressed = false;
    // list of mouse positions used to calculate the velocity when flicking in touch mode
    std::deque<std::pair<QTime, QPoint>> position_buffer;
    float velocity_x = 0;
    float velocity_y = 0;

    // indicates if mouse was in next/prev ruler rect in touch mode
    // if this is the case, we use mouse movement to perform next/prev ruler command
    // after a certain threshold, so the user doesn't have to click on the ruler rect 
    bool was_last_mouse_down_in_ruler_next_rect = false;
    bool was_last_mouse_down_in_ruler_prev_rect = false;
    // this is used so we can keep track of mouse movement after press and holding on ruler rect
    WindowPos ruler_moving_last_window_pos;
    int ruler_moving_distance_traveled = 0;


    void update_highlight_buttons_position();
    void handle_mobile_selection();
    void update_mobile_selection();
    bool handle_quick_tap(WindowPos click_pos);
    bool handle_double_tap(QPoint pos);
    void android_handle_visual_mode();
    void show_highlight_buttons();
    void clear_highlight_buttons();
    void show_search_buttons();
    void clear_search_buttons();
    void clear_selection_indicators();
    bool is_moving();
    void update_position_buffer();
    bool is_flicking(QPointF* out_velocity);
    void handle_touch_highlight();
    void restore_default_config();
    bool is_in_back_rect(WindowPos pos);
    bool is_in_middle_left_rect(WindowPos pos);
    bool is_in_middle_right_rect(WindowPos pos);
    bool is_in_forward_rect(WindowPos pos);
    bool is_in_edit_portal_rect(WindowPos pos);
    bool is_in_visual_mark_next_rect(WindowPos pos);
    bool is_in_visual_mark_prev_rect(WindowPos pos);
    void handle_drawing_move(QPoint pos, float pressure);
    void start_drawing();
    void finish_drawing(QPoint pos);
    void handle_pen_drawing_event(QTabletEvent* te);
    void select_freehand_drawings(AbsoluteRect rect);
    void delete_freehand_drawings(AbsoluteRect rect);
    void handle_move_text_mark_forward(bool word);
    void handle_move_text_mark_backward(bool word);
    void handle_move_text_mark_down();
    void handle_move_text_mark_up();
    void handle_toggle_text_mark();

    const std::wstring& get_selected_text();
    void move_selection_end(bool expand, bool word);
    void move_selection_begin(bool expand, bool word);
    void shrink_selection_end();
    void shrink_selection_begin();

    void persist_config();

    void synchronize_pending_link();
    void refresh_all_windows();
    std::optional<PdfLink> get_selected_link(const std::wstring& text);

    int num_visible_links();
#ifdef SIOYEK_ANDROID
    //    void onApplicationStateChanged(Qt::ApplicationState applicationState);
    bool pending_intents_checked = false;
#endif

protected:
    TouchTextSelectionButtons* text_selection_buttons_ = nullptr;
    DrawControlsUI* draw_controls_ = nullptr;
    SearchButtons* search_buttons_ = nullptr;
    HighlightButtons* highlight_buttons_ = nullptr;
    bool middle_click_hold_command_already_executed = false;

    TouchTextSelectionButtons* get_text_selection_buttons();
    DrawControlsUI* get_draw_controls();
    SearchButtons* get_search_buttons();
    HighlightButtons* get_highlight_buttons();



    void focusInEvent(QFocusEvent* ev);
    void resizeEvent(QResizeEvent* resize_event) override;
    void changeEvent(QEvent* event) override;
    void mouseMoveEvent(QMouseEvent* mouse_event) override;

    // we already handle drag and drop on macos elsewhere
#ifndef Q_OS_MACOS
    void dragEnterEvent(QDragEnterEvent* e) override;
    void dragMoveEvent(QDragMoveEvent* e) override;
    void dropEvent(QDropEvent* event) override;
#endif

    void closeEvent(QCloseEvent* close_event) override;
    void keyPressEvent(QKeyEvent* kevent) override;
    void keyReleaseEvent(QKeyEvent* kevent) override;
    void mouseReleaseEvent(QMouseEvent* mevent) override;
    void mousePressEvent(QMouseEvent* mevent) override;
    void mouseDoubleClickEvent(QMouseEvent* mevent) override;
    void wheelEvent(QWheelEvent* wevent) override;
    bool event(QEvent* event);

public:

    void handle_rename(std::wstring new_name);
    bool execute_macro_if_enabled(std::wstring macro_command_string, QLocalSocket* result_socket=nullptr);
    bool execute_macro_from_origin(std::wstring macro_command_string, QLocalSocket* origin);
    bool ensure_internet_permission();
    void handle_command_text_change(const QString& new_text);
    QTextToSpeech* get_tts();
    void handle_bookmark_move_finish();
    void handle_bookmark_move();
    void handle_portal_move();
    void handle_portal_move_finish();
    bool is_middle_click_being_used();
    void begin_bookmark_move(int index, AbsoluteDocumentPos begin_cursor_pos);
    void begin_portal_move(int index, AbsoluteDocumentPos begin_cursor_pos, bool is_pending);
    bool should_drag();
    void handle_freehand_drawing_move_finish();
    void move_selected_drawings(AbsoluteDocumentPos new_pos, std::vector<FreehandDrawing>& moved_drawings, std::vector<PixmapDrawing>& moved_pixmaps);
    bool goto_ith_next_overview(int i);
    void on_overview_source_updated();
    std::optional<std::wstring> get_overview_paper_name();
    std::optional<AbsoluteRect> get_overview_source_rect();

    void open_document(const std::wstring& doc_path, bool* invalid_flag, bool load_prev_state = true, std::optional<OpenedBookState> prev_state = {}, bool foce_load_dimensions = false);
    void finish_pending_download_portal(std::wstring download_paper_name, std::wstring downloaded_file_path);

    std::optional<Portal> get_portal_under_absolute_pos(AbsoluteDocumentPos abspos, int* index=nullptr);
    std::optional<Portal> get_portal_under_window_pos(WindowPos pos, int* index=nullptr);
    std::optional<Portal> get_target_portal(bool limit);

    AbsoluteDocumentPos get_cursor_abspos();
    void update_opengl_pending_download_portals();
    void cleanup_expired_pending_portals();
    int get_pending_portal_index_at_pos(AbsoluteDocumentPos abspos);
    void update_pending_portal_indices_after_removed_indices(std::vector<int>& removed_indices);
    void close_overview();
    std::vector<Portal> get_ruler_portals();
    void handle_overview_to_ruler_portal();
    void handle_goto_ruler_portal(std::string tag="");
    void show_touch_buttons(
        std::vector<std::wstring> buttons,
        std::vector<std::wstring> tips,
        std::function<void(int, std::wstring)> on_select, bool top=true);
    bool is_pos_inside_selected_text(AbsoluteDocumentPos pos);
    bool is_pos_inside_selected_text(WindowPos pos);
    void create_pending_download_portal(AbsoluteDocumentPos source_position, std::wstring paper_name);
    void download_and_portal(std::wstring unclean_paper_name, AbsoluteDocumentPos source_pos);
    void download_selected_text();
    void smart_jump_to_selected_text();
    void show_text_prompt(std::wstring initial_value, std::function<void(std::wstring)> on_select);
    void set_overview_page(std::optional<OverviewState> overview);
    std::vector<std::wstring> get_new_files_from_scan_directory();
    void scan_new_files_from_scan_directory();
    void export_python_api();
    QJSEngine* take_js_engine();
    void release_js_engine(QJSEngine* engine);

    QJSValue export_javascript_api(QJSEngine& engine, bool is_async);
    void show_custom_option_list(std::vector<std::wstring> option_list);
    void on_socket_deleted(QLocalSocket* deleted_socket);
    Q_INVOKABLE QJsonObject get_json_state();
    QJsonObject get_json_annotations();
    QJsonArray get_all_json_states();
    void screenshot(std::wstring file_path);
    void framebuffer_screenshot(std::wstring file_path);
    void export_command_names(std::wstring file_path);
    void export_config_names(std::wstring file_path);
    void export_default_config_file(std::wstring file_path);
    void print_undocumented_commands();
    void print_undocumented_configs();
    void print_non_default_configs();
    //void advance_wait_for_render_if_ready();
    bool is_render_ready();
    bool is_search_ready();
    bool is_index_ready();
    void advance_waiting_command(std::string waiting_command_name);
    std::string get_user_agent_string();
    void handle_select_current_search_match();
    void handle_stop_search();
    int get_window_id();
    void add_command_being_performed(Command* new_command);
    void remove_command_being_performed(Command* new_command);
    void load_command_docs();
    QString get_command_documentation(QString command_name);
    void show_command_documentation(QString command_name);

    QString handle_action_in_menu(std::wstring action);
    std::wstring handle_synctex_to_ruler();
    void focus_on_line_with_index(int page, int index);
    void show_touch_main_menu();
    void show_touch_settings_menu();
    void free_document(Document* doc);
    bool is_helper_visible();
    std::wstring get_current_tabs_file_names();
    void open_tabs(const std::vector<std::wstring>& tabs);
    void handle_goto_tab(const std::wstring& path);
    void get_document_views_referencing_doc(std::wstring doc_path, std::vector<DocumentView*>& document_views, std::vector<MainWidget*>& corresponding_widgets, std::vector<bool>& is_helper);
    void restore_document_view_states(const std::vector<DocumentView*>& document_views, const std::vector<DocumentViewState>& states);
    void document_views_open_path(const std::vector<DocumentView*>& document_views, const std::vector<MainWidget*>& main_widgets, const std::vector<bool> is_helpers, std::wstring new_path);
    void update_renamed_document_in_history(std::wstring old_path, std::wstring new_path);
    void maximize_window();
    void toggle_rect_hints();
    void run_command_with_name(std::string command_name, bool should_pop_current_widget=false);
    QStringListModel* get_new_command_list_model();
    void add_password(std::wstring path, std::string password);
    void handle_fit_to_page_width(bool smart);
    int current_document_page_count();
    void goto_search_result(int nth_next_result, bool overview=false);
    void set_should_highlight_words(bool should_highlight_words);
    void toggle_highlight_links();
    void set_highlight_links(bool should_highlight, bool should_show_numbers);
    void rotate_clockwise();
    void rotate_counterclockwise();
    void toggle_fastread();
    void export_json(std::wstring json_file_path);
    void import_json(std::wstring json_file_path);
    bool does_current_widget_consume_quicktap_event();
    bool is_moving_annotations();
    PdfViewOpenGLWidget* helper_opengl_widget();
    DocumentView* helper_document_view();
    void initialize_helper();
    void hide_command_line_edit();
    void deselect_document_indices();
    void zoom_in_overview();
    void zoom_out_overview();
    Q_INVOKABLE QString run_macro_on_main_thread(QString macro_string, bool wait_for_result=true);
    Q_INVOKABLE QString perform_network_request(QString url);
    Q_INVOKABLE QString read_text_file(QString path);
    Q_INVOKABLE void execute_macro_and_return_result(QString macro_string, bool* is_done, std::wstring* result);
    Q_INVOKABLE QString execute_macro_sync(QString macro);
    Q_INVOKABLE void set_variable(QString name, QVariant var);
    Q_INVOKABLE QVariant get_variable(QString name);
    void run_javascript_command(std::wstring javascript_code, bool is_async);
    void set_text_prompt_text(QString text);
    AbsoluteDocumentPos get_window_abspos(WindowPos window_pos);
    DocumentView* dv();
    bool should_draw(bool originated_from_pen);
    void handle_freehand_drawing_selection_click(AbsoluteDocumentPos click_pos);
    bool is_scratchpad_mode();
    void toggle_scratchpad_mode();
    void add_pixmap_to_scratchpad(QPixmap pixmap);
    void save_scratchpad();
    void load_scratchpad();
    void clear_scratchpad();
    char get_current_freehand_type();
    void show_draw_controls();
    PaperDownloadFinishedAction get_default_paper_download_finish_action();
    QString get_paper_download_finish_action_string(PaperDownloadFinishedAction action);
    PaperDownloadFinishedAction get_paper_download_action_from_string(QString str);
    void set_tag_prefix(std::wstring prefix);
    void clear_tag_prefix();
    void show_context_menu();
    QPoint cursor_pos();
    void clear_current_page_drawings();
    void clear_current_document_drawings();
    void set_selected_highlight_index(int index);
    void handle_highlight_tags_pre_perform(const std::vector<int>& visible_highlight_indices);
    void clear_keyboard_select_highlights();
    void handle_goto_link_with_page_and_offset(int page, float y_offset);
    std::optional<std::wstring> get_search_suggestion_with_index(int index);
};

#endif
