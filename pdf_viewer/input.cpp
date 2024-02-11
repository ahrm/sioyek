#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>

#include <qjsondocument.h>
#include <qkeyevent.h>
#include <qstring.h>
#include <qstringlist.h>
#include <qlocalsocket.h>
#include <qfileinfo.h>
#include <qclipboard.h>

#include "utils.h"
#include "input.h"
#include "main_widget.h"
#include "ui.h"
#include "document.h"
#include "document_view.h"

extern bool SHOULD_WARN_ABOUT_USER_KEY_OVERRIDE;
extern bool USE_LEGACY_KEYBINDS;
extern std::map<std::wstring, std::wstring> ADDITIONAL_COMMANDS;
extern std::map<std::wstring, JsCommandInfo> ADDITIONAL_JAVASCRIPT_COMMANDS;
extern std::map<std::wstring, JsCommandInfo> ADDITIONAL_ASYNC_JAVASCRIPT_COMMANDS;
extern std::map<std::wstring, std::wstring> ADDITIONAL_MACROS;
extern std::wstring SEARCH_URLS[26];
extern bool ALPHABETIC_LINK_TAGS;
extern std::vector<AdditionalKeymapData> ADDITIONAL_KEYMAPS;

extern float EPUB_WIDTH;
extern float EPUB_HEIGHT;

extern Path default_config_path;
extern Path default_keys_path;
extern std::vector<Path> user_config_paths;
extern std::vector<Path> user_keys_paths;
extern bool TOUCH_MODE;
extern bool VERBOSE;
extern float FREETEXT_BOOKMARK_FONT_SIZE;
extern bool FUZZY_SEARCHING;
extern bool TOC_JUMP_ALIGN_TOP;
extern bool FILL_TEXTBAR_WITH_SELECTED_TEXT;
extern bool SHOW_MOST_RECENT_COMMANDS_FIRST;
extern bool INCREMENTAL_SEARCH;

bool is_command_string_modal(const std::wstring& command_name) {
    return std::find(command_name.begin(), command_name.end(), '[') != command_name.end();
}

std::vector<std::string> parse_command_name(const QString& command_names) {
    QStringList parts = command_names.split(';');
    std::vector<std::string> res;
    for (int i = 0; i < parts.size(); i++) {
        res.push_back(parts.at(i).toStdString());
    }
    return res;
}

struct CommandInvocation {
    QString command_name;
    QStringList command_args;

    QString command_string() {
        if (command_name.size() > 0) {
            if (command_name[0] != '[') return command_name;
            int index = command_name.indexOf(']');
            return command_name.mid(index + 1);
        }
        return "";
    }

    QString mode_string() {
        if (command_name.size() > 0) {
            if (command_name[0] == '[') {
                int index = command_name.indexOf(']');
                return command_name.mid(1, index - 1);
            }
            else {
                return "";
            }
        }
        return "";
    }
};

Command::Command(std::string name, MainWidget* widget_) : command_cname(name), widget(widget_) {

}

void Command::on_text_change(const QString& new_text) {

}

bool Command::is_holdable() {
    return false;
}

void Command::on_key_hold() {

}

bool Command::is_menu_command() {
    // returns true if the command is a macro designed to run only on menus (m)
    // we then ignore keys in menus if they are bound to such commands so they
    // can be later handled by our own InputHandler
    return false;
}

Command::~Command() {

}

std::optional<std::wstring> Command::get_text_suggestion(int index) {
    return {};
}

void Command::perform_up() {
}

void Command::set_result_socket(QLocalSocket* socket) {
    result_socket = socket;
}

void Command::set_result_mutex(bool* id, std::wstring* result_location) {
    is_done = id;
    result_holder = result_location;
}

void Command::set_generic_requirement(QVariant value) {

}

void Command::handle_generic_requirement() {

}

AbsoluteRect get_rect_from_string(std::wstring str) {
    QStringList parts = QString::fromStdWString(str).split(' ');

    AbsoluteRect result;
    result.x0 = parts[0].toFloat();
    result.y0 = parts[1].toFloat();
    result.x1 = parts[2].toFloat();
    result.y1 = parts[3].toFloat();
    return result;
}

AbsoluteDocumentPos get_point_from_string(std::wstring str) {
    QStringList parts = QString::fromStdWString(str).split(' ');

    AbsoluteDocumentPos result;
    result.x = parts[0].toFloat();
    result.y = parts[1].toFloat();
    return result;
}

void Command::set_next_requirement_with_string(std::wstring str) {
    std::optional<Requirement> maybe_req = next_requirement(widget);
    if (maybe_req) {
        Requirement req = maybe_req.value();
        if (req.type == RequirementType::Text) {
            set_text_requirement(str);
        }
        else if (req.type == RequirementType::Symbol) {
            set_symbol_requirement(str[0]);
        }
        else if (req.type == RequirementType::File || req.type == RequirementType::Folder) {
            set_file_requirement(str);
        }
        else if (req.type == RequirementType::Rect) {
            set_rect_requirement(get_rect_from_string(str));
        }
        else if (req.type == RequirementType::Point) {
            set_point_requirement(get_point_from_string(str));
        }
        else if (req.type == RequirementType::Generic) {
            set_generic_requirement(QString::fromStdWString(str));
        }
    }
}

class GenericPathCommand : public Command {
public:
    std::optional<std::wstring> selected_path = {};
    GenericPathCommand(std::string name, MainWidget* w) : Command(name, w) {};

    std::optional<Requirement> next_requirement(MainWidget* widget) override {
        if (selected_path) {
            return {};
        }
        else {
            return Requirement{ RequirementType::Generic, "File path" };
        }
    }

    void set_generic_requirement(QVariant value) {
        selected_path = value.toString().toStdWString();
    }

};

class GenericGotoLocationCommand : public Command {
public:
    GenericGotoLocationCommand(std::string name, MainWidget* w) : Command(name, w) {};

    std::optional<float> target_location = {};

    std::optional<Requirement> next_requirement(MainWidget* widget) {
        if (target_location.has_value()) {
            return {};
        }
        else {
            return Requirement{ RequirementType::Generic, "Target Location" };
        }
    }

    void set_generic_requirement(QVariant value) {
        target_location = value.toFloat();
    }

    void perform() {
        widget->main_document_view->set_offset_y(target_location.value());
        widget->validate_render();
    }

    bool pushes_state() { return true; }

};

class GenericPathAndLocationCommadn : public Command {
public:

    std::optional<QVariant> target_location;
    bool is_hash = false;
    GenericPathAndLocationCommadn(std::string name, MainWidget* w, bool is_hash_ = false) : Command(name, w) { is_hash = is_hash_; };

    std::optional<Requirement> next_requirement(MainWidget* widget) {
        if (target_location) {
            return {};
        }
        else {
            return Requirement{ RequirementType::Generic, "Location" };
        }
    }

    void set_generic_requirement(QVariant value) {
        target_location = value;
    }

    void perform() {
        QList<QVariant> values = target_location.value().toList();
        std::wstring file_name = values[0].toString().toStdWString();

        if (values.size() == 1) {
            if (is_hash) {
                widget->open_document_with_hash(utf8_encode(file_name));
                widget->validate_render();
            }
            else {
                widget->open_document(file_name);
                widget->validate_render();
            }
        }
        else if (values.size() == 2) {
            float y_offset = values[1].toFloat();
            widget->open_document(file_name, 0.0f, y_offset);
            widget->validate_render();
        }
        else {
            float x_offset = values[1].toFloat();
            float y_offset = values[2].toFloat();

            widget->open_document(file_name, x_offset, y_offset);
            widget->validate_render();
        }
    }

    bool pushes_state() {
        return true;
    }
};

class SymbolCommand : public Command {
protected:
    char symbol = 0;
public:
    SymbolCommand(std::string name, MainWidget* w) : Command(name, w) {}
    virtual std::optional<Requirement> next_requirement(MainWidget* widget) {
        if (symbol == 0) {
            return Requirement{ RequirementType::Symbol, "symbol" };
        }
        else {
            return {};
        }
    }

    virtual void set_symbol_requirement(char value) {
        this->symbol = value;
    }
};

class TextCommand : public Command {
protected:
    std::optional<std::wstring> text = {};
public:

    TextCommand(std::string name, MainWidget* w) : Command(name, w) {}

    virtual std::string text_requirement_name() {
        return "text";
    }

    virtual std::optional<Requirement> next_requirement(MainWidget* widget) {
        if (text.has_value()) {
            return {};
        }
        else {
            return Requirement{ RequirementType::Text, text_requirement_name() };
        }
    }

    virtual void set_text_requirement(std::wstring value) {
        this->text = value;
    }
};

class GotoMark : public SymbolCommand {
public:
    static inline const std::string cname = "goto_mark";
    static inline const std::string hname = "Go to marked location";

    GotoMark(MainWidget* w) : SymbolCommand(cname, w) {}

    bool pushes_state() {
        return true;
    }

    std::string get_name() {
        return cname;
    }

    std::vector<char> special_symbols() {
        std::vector<char> res = { '`', '\'', '/' };
        return res;
    }

    void perform() {
        assert(this->symbol != 0);
        widget->goto_mark(this->symbol);
    }

    bool requires_document() { return false; }
};

class SetMark : public SymbolCommand {
public:
    static inline const std::string cname = "set_mark";
    static inline const std::string hname = "Set mark in current location";

    SetMark(MainWidget* w) : SymbolCommand(cname, w) {}

    std::string get_name() {
        return cname;
    }


    void perform() {
        assert(this->symbol != 0);
        widget->set_mark_in_current_location(this->symbol);
    }
};

class ToggleDrawingMask : public SymbolCommand {
public:
    static inline const std::string cname = "toggle_drawing_mask";
    static inline const std::string hname = "Toggle drawing type visibility";

    ToggleDrawingMask(MainWidget* w) : SymbolCommand(cname, w) {}

    std::string get_name() {
        return cname;
    }

    void perform() {
        widget->handle_toggle_drawing_mask(this->symbol);
    }
};

class GotoLoadedDocumentCommand : public GenericPathCommand {
public:
    static inline const std::string cname = "goto_tab";
    static inline const std::string hname = "Open tab";
    GotoLoadedDocumentCommand(MainWidget* w) : GenericPathCommand(cname, w) {}

    void handle_generic_requirement() {
        widget->handle_goto_loaded_document();
    }

    void perform() {
        widget->handle_goto_tab(selected_path.value());
    }

    bool requires_document() {
        return false;
    }

    std::string get_name() {
        return cname;
    }

};

class NextItemCommand : public Command {
public:
    static inline const std::string cname = "next_item";
    static inline const std::string hname = "Go to next search result";
    NextItemCommand(MainWidget* w) : Command(cname, w) {}

    void perform() {
        if (num_repeats == 0) num_repeats++;
        widget->goto_search_result(num_repeats);
    }

    std::string get_name() {
        return cname;
    }

};

class PrevItemCommand : public Command {
public:
    static inline const std::string cname = "previous_item";
    static inline const std::string hname = "Go to previous search result";
    PrevItemCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        if (num_repeats == 0) num_repeats++;
        widget->goto_search_result(-num_repeats);
    }

};

class ToggleTextMarkCommand : public Command {
public:
    static inline const std::string cname = "toggle_text_mark";
    static inline const std::string hname = "Move text cursor to other end of selection";
    ToggleTextMarkCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        //if (num_repeats == 0) num_repeats++;
        widget->handle_toggle_text_mark();
        //widget->invalidate_render();
    }

};

class MoveTextMarkForwardCommand : public Command {
public:
    static inline const std::string cname = "move_text_mark_forward";
    static inline const std::string hname = "Move text cursor forward";
    MoveTextMarkForwardCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        //if (num_repeats == 0) num_repeats++;
        widget->handle_move_text_mark_forward(false);
        //widget->invalidate_render();
    }
};

class MoveTextMarkDownCommand : public Command {
public:
    static inline const std::string cname = "move_text_mark_down";
    static inline const std::string hname = "Move text cursor down";
    MoveTextMarkDownCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        widget->handle_move_text_mark_down();
    }
};

class MoveTextMarkUpCommand : public Command {
public:
    static inline const std::string cname = "move_text_mark_up";
    static inline const std::string hname = "Move text cursor up";
    MoveTextMarkUpCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        widget->handle_move_text_mark_up();
    }
};

class MoveTextMarkForwardWordCommand : public Command {
public:
    static inline const std::string cname = "move_text_mark_forward_word";
    static inline const std::string hname = "Move text cursor forward to the next word";
    MoveTextMarkForwardWordCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        widget->handle_move_text_mark_forward(true);
    }
};

class MoveTextMarkBackwardCommand : public Command {
public:
    static inline const std::string cname = "move_text_mark_backward";
    static inline const std::string hname = "Move text cursor backward";
    MoveTextMarkBackwardCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        widget->handle_move_text_mark_backward(false);
    }
};

class MoveTextMarkBackwardWordCommand : public Command {
public:
    static inline const std::string cname = "move_text_mark_backward_word";
    static inline const std::string hname = "Move text cursor backward to the previous word";
    MoveTextMarkBackwardWordCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        widget->handle_move_text_mark_backward(true);
    }
};

class StartReadingCommand : public Command {
public:
    static inline const std::string cname = "start_reading";
    static inline const std::string hname = "Read using text to speech";
    StartReadingCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        widget->handle_start_reading();
    }
};

class StopReadingCommand : public Command {
public:
    static inline const std::string cname = "stop_reading";
    static inline const std::string hname = "Stop reading";
    StopReadingCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        widget->handle_stop_reading();
    }
};

class SearchCommand : public TextCommand {
public:
    static inline const std::string cname = "search";
    static inline const std::string hname = "Search the PDF document";
    SearchCommand(MainWidget* w) : TextCommand(cname, w) {
    };

    void on_text_change(const QString& new_text) override {
        if (INCREMENTAL_SEARCH && widget->doc()->is_super_fast_index_ready()) {
            widget->perform_search(new_text.toStdWString(), false, true);
        }
    }

    void pre_perform() {
        if (INCREMENTAL_SEARCH) {
            widget->main_document_view->add_mark('/');
        }
    }

    virtual void on_cancel() override{
        if (INCREMENTAL_SEARCH) {
            widget->goto_mark('/');
        }
    }

    void perform() {
        widget->perform_search(this->text.value(), false, INCREMENTAL_SEARCH);
        if (TOUCH_MODE) {
            widget->show_search_buttons();
        }
    }

    bool pushes_state() {
        return true;
    }

    std::optional<std::wstring> get_text_suggestion(int index) {
        return widget->get_search_suggestion_with_index(index);
    }

    std::wstring get_text_default_value() {
        if (FILL_TEXTBAR_WITH_SELECTED_TEXT) {
            return widget->get_selected_text();
        }
        return L"";
    }

    std::string text_requirement_name() {
        return "Search Term";
    }

};

class DownloadClipboardUrlCommand : public Command {
public:
    static inline const std::string cname = "download_clipboard_url";
    static inline const std::string hname = "";

    DownloadClipboardUrlCommand(MainWidget* w) : Command(cname, w) {
    };

    void perform() {
        auto clipboard = QGuiApplication::clipboard();
        std::wstring url = clipboard->text().toStdWString();
        widget->download_paper_with_url(url, false, PaperDownloadFinishedAction::OpenInNewWindow);
    }

    std::string text_requirement_name() {
        return "Paper Url";
    }

};
class DownloadPaperWithUrlCommand : public TextCommand {
public:
    static inline const std::string cname = "download_paper_with_url";
    static inline const std::string hname = "";

    DownloadPaperWithUrlCommand(MainWidget* w) : TextCommand(cname, w) {
    };

    void perform() {
        widget->download_paper_with_url(text.value(), false, PaperDownloadFinishedAction::OpenInNewWindow);
    }

    std::string text_requirement_name() {
        return "Paper Url";
    }

};

class ControlMenuCommand : public TextCommand {
public:
    static inline const std::string cname = "control_menu";
    static inline const std::string hname = "";
    ControlMenuCommand(MainWidget* w) : TextCommand(cname, w) {
    };


    void perform() {
        QString res = widget->handle_action_in_menu(text.value());
        result = res.toStdWString();
    }

    std::string text_requirement_name() {
        return "Action";
    }

};

class ExecuteMacroCommand : public TextCommand {
public:
    static inline const std::string cname = "execute_macro";
    static inline const std::string hname = "";
    ExecuteMacroCommand(MainWidget* w) : TextCommand(cname, w) {
    };

    void perform() {
        widget->execute_macro_if_enabled(text.value());
    }

    std::string text_requirement_name() {
        return "Macro";
    }

};

class SetViewStateCommand : public TextCommand {
public:
    static inline const std::string cname = "set_view_state";
    static inline const std::string hname = "";
    SetViewStateCommand(MainWidget* w) : TextCommand(cname, w) {
    };

    void perform() {
        QStringList parts = QString::fromStdWString(text.value()).split(' ');
        if (parts.size() == 4) {
            float offset_x = parts[0].toFloat();
            float offset_y = parts[1].toFloat();
            float zoom_level = parts[2].toFloat();
            bool pushes_state = parts[3].toInt();

            if (pushes_state == 1) {
                widget->push_state();
            }

            widget->main_document_view->set_offsets(offset_x, offset_y);
            widget->main_document_view->set_zoom_level(zoom_level, true);
        }
    }


    std::string text_requirement_name() {
        return "State String";
    }

};

class TestCommand : public Command {
private:
    std::optional<std::wstring> text1 = {};
    std::optional<std::wstring> text2 = {};
public:
    static inline const std::string cname = "test_command";
    static inline const std::string hname = "";
    TestCommand(MainWidget* w) : Command(cname, w) {};

    void set_text_requirement(std::wstring value) {
        if (!text1.has_value()) {
            text1 = value;
        }
        else {
            text2 = value;
        }
    }

    std::optional<Requirement> next_requirement(MainWidget* widget) {
        if (!text1.has_value()) {
            return Requirement{ RequirementType::Text, "text1" };
        }
        if (!text2.has_value()) {
            return Requirement{ RequirementType::Text, "text2" };
        }
        return {};
    }

    void perform() {
        result = text1.value() + text2.value();
        //widget->set_status_message(result.value());
        show_error_message(result.value());
    }

};

class GetConfigNoDialogCommand : public Command {
    std::optional<std::wstring> command_name = {};
public:
    static inline const std::string cname = "get_config_no_dialog";
    static inline const std::string hname = "";
    GetConfigNoDialogCommand(MainWidget* w) : Command(cname, w) {};

    std::optional<Requirement> next_requirement(MainWidget* widget) {
        if (!command_name.has_value()) {
            return Requirement{ RequirementType::Text, "Prompt Title" };
        }
        return {};
    }

    void set_text_requirement(std::wstring value) {
        command_name = value;
    }

    void perform() {
        //widget->config_manager->get_config
        QStringList config_name_list = QString::fromStdWString(command_name.value()).split(',');

        auto configs = widget->config_manager->get_configs_ptr();
        std::wstringstream output;
        for (auto confname : config_name_list) {

            for (int i = 0; i < configs->size(); i++) {
                if ((*configs)[i].name == confname.toStdWString()) {
                    output << (*configs)[i].get_type_string();
                    output << L" ";
                    (*configs)[i].serialize((*configs)[i].value, output);
                    if (i < configs->size() - 1) {
                        output.put(L'\n');
                    }
                }
            }
        }

        result = output.str();
    }

};

class ShowTextPromptCommand : public Command {

    std::optional<std::wstring> prompt_title = {};
    std::optional<std::wstring> default_value = {};
public:
    static inline const std::string cname = "show_text_prompt";
    static inline const std::string hname = "";
    ShowTextPromptCommand(MainWidget* w) : Command(cname, w) {};

    std::optional<Requirement> next_requirement(MainWidget* widget) {
        if (!prompt_title.has_value()) {
            return Requirement { RequirementType::Text, "Prompt Title" };
        }
        if (!result.has_value()) {
            return Requirement{ RequirementType::Text, utf8_encode(prompt_title.value())};
        }
        return {};
    }

    void set_text_requirement(std::wstring value) {
        if (!prompt_title.has_value()) {
            QString value_qstring = QString::fromStdWString(value);
            int index = value_qstring.indexOf('|');
            if (index != -1) {

                //QStringList parts = value_qstring.split("|");
                prompt_title = value_qstring.left(index).toStdWString();
                default_value = value_qstring.right(value_qstring.size() - prompt_title->size() - 1).toStdWString();
                //default_value = parts.at(1).toStdWString();
            }
            else {
                prompt_title = value;
            }
        }
        else {
            result = value;
        }
    }

    void pre_perform() {
        if (default_value) {
            widget->text_command_line_edit->setText(
                QString::fromStdWString(default_value.value())
            );
        }
    }

    void perform() {
    }

};

class GetStateJsonCommand : public Command {

public:
    static inline const std::string cname = "get_state_json";
    static inline const std::string hname = "";
    GetStateJsonCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        QJsonDocument doc(widget->get_all_json_states());
        std::wstring json_str = utf8_decode(doc.toJson(QJsonDocument::Compact).toStdString());
        result = json_str;
    }
};

class GetPaperNameCommand : public Command {

public:
    static inline const std::string cname = "get_paper_name";
    static inline const std::string hname = "";
    GetPaperNameCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        result = widget->doc()->detect_paper_name();
    }
};

class GetOverviewPaperName : public Command {

public:
    static inline const std::string cname = "get_overview_paper_name";
    static inline const std::string hname = "";
    GetOverviewPaperName(MainWidget* w) : Command(cname, w) {};

    void perform() {
        std::optional<std::wstring> paper_name = widget->get_overview_paper_name();
        result = paper_name.value_or(L"");
    }
};

class ToggleRectHintsCommand : public Command {

public:
    static inline const std::string cname = "toggle_rect_hints";
    static inline const std::string hname = "";
    ToggleRectHintsCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        widget->toggle_rect_hints();
    }

};

class GetAnnotationsJsonCommand : public Command {

public:
    static inline const std::string cname = "get_annotations_json";
    static inline const std::string hname = "";
    GetAnnotationsJsonCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        QJsonDocument doc(widget->get_json_annotations());
        std::wstring json_str = utf8_decode(doc.toJson(QJsonDocument::Compact).toStdString());
        result = json_str;
    }

};

class ShowOptionsCommand : public Command {

private:
    std::vector<std::wstring> options;
    std::optional<std::wstring> selected_option;

public:
    static inline const std::string cname = "show_custom_options";
    static inline const std::string hname = "";
    ShowOptionsCommand(MainWidget* w) : Command(cname, w) {};

    std::optional<Requirement> next_requirement(MainWidget* widget) {
        if (options.size() == 0) {
            return Requirement { RequirementType::Text, "options" };
        }
        if (!selected_option.has_value()) {
            return Requirement { RequirementType::Generic, "selected" };
        }
        return {};
    }

    void set_generic_requirement(QVariant value) {
        selected_option = value.toString().toStdWString();
        result = selected_option.value();
    }

    void handle_generic_requirement() {
        widget->show_custom_option_list(options);
    }

    void set_text_requirement(std::wstring value) {
        QStringList options_ = QString::fromStdWString(value).split("|");
        for (auto option : options_) {
            options.push_back(option.toStdWString());
        }
    }

    void perform() {
    }

};

//class ShowOptionsCommand : public TextCommand {
//public:
//    SearchCommand(MainWidget* w) : TextCommand(w) {};
//
//    void perform() {
//        widget->perform_search(this->text.value(), false);
//        if (TOUCH_MODE) {
//            widget->show_search_buttons();
//        }
//    }
//
//    std::string get_name() {
//        return "search";
//    }
//
//    bool pushes_state() {
//        return true;
//    }
//
//    std::string text_requirement_name() {
//        return "Search Term";
//    }
//
//};

class GetConfigCommand : public TextCommand {
public:
    static inline const std::string cname = "get_config_value";
    static inline const std::string hname = "";
    GetConfigCommand(MainWidget* w) : TextCommand(cname, w) {};

    void perform() {
        auto configs = widget->config_manager->get_configs_ptr();
        for (int i = 0; i < configs->size(); i++) {
            if ((*configs)[i].name == text.value()) {
                std::wstringstream ssr;
                (*configs)[i].serialize((*configs)[i].value, ssr);
                show_error_message(ssr.str());
            }
        }
    }

    std::string text_requirement_name() {
        return "Config Name";
    }

};

class DownloadPaperWithNameCommand : public TextCommand {
public:
    static inline const std::string cname = "download_paper_with_name";
    static inline const std::string hname = "";
    DownloadPaperWithNameCommand(MainWidget* w) : TextCommand(cname, w) {};

    void perform() {
        widget->download_paper_with_name(text.value(), PaperDownloadFinishedAction::OpenInNewWindow);
    }

    std::string text_requirement_name() {
        return "Paper Name";
    }

};

class AddAnnotationToSelectedHighlightCommand : public TextCommand {
public:
    static inline const std::string cname = "add_annot_to_selected_highlight";
    static inline const std::string hname = "Add annotation to selected highlight";
    AddAnnotationToSelectedHighlightCommand(MainWidget* w) : TextCommand(cname, w) {};

    void perform() {
        widget->add_text_annotation_to_selected_highlight(this->text.value());
    }

    void pre_perform() {
        if (widget->selected_highlight_index >= 0){
            widget->text_command_line_edit->setText(
                    QString::fromStdWString(widget->doc()->get_highlights()[widget->selected_highlight_index].text_annot)
                    );
        }
    }


    std::string text_requirement_name() {
        return "Comment";
    }
};

class RenameCommand : public TextCommand {
public:
    static inline const std::string cname = "rename";
    static inline const std::string hname = "";
    RenameCommand(MainWidget* w) : TextCommand(cname, w) {};

    void perform() {
        //widget->add_text_annotation_to_selected_highlight(this->text.value());
        widget->handle_rename(text.value());
    }

    void pre_perform() {
        if (!widget->doc()) return;

        QString file_name = get_file_name_from_paper_name(QString::fromStdWString(widget->doc()->detect_paper_name()));
        widget->text_command_line_edit->setText(
            file_name
        );
        //QString paper_name = QString::fromStdWString(widget->doc()->detect_paper_name());

    }

    std::string text_requirement_name() {
        return "New Name";
    }
};

class SetFreehandThickness : public TextCommand {
public:
    static inline const std::string cname = "set_freehand_thickness";
    static inline const std::string hname = "Set thickness of freehand drawings";
    SetFreehandThickness(MainWidget* w) : TextCommand(cname, w) {};

    void perform() {
        float thickness = QString::fromStdWString(this->text.value()).toFloat();
        widget->set_freehand_thickness(thickness);
        //widget->perform_search(this->text.value(), false);
        //if (TOUCH_MODE) {
        //	widget->show_search_buttons();
        //}
    }


    std::string text_requirement_name() {
        return "Thickness";
    }
};

class GotoPageWithLabel : public TextCommand {
public:
    static inline const std::string cname = "goto_page_with_label";
    static inline const std::string hname = "Go to page with label";
    GotoPageWithLabel(MainWidget* w) : TextCommand(cname, w) {};

    void perform() {
        widget->goto_page_with_label(text.value());
    }

    bool pushes_state() {
        return true;
    }

    std::string text_requirement_name() {
        return "Page Label";
    }
};

class ChapterSearchCommand : public TextCommand {
public:
    static inline const std::string cname = "chapter_search";
    static inline const std::string hname = "Search current chapter";
    ChapterSearchCommand(MainWidget* w) : TextCommand(cname, w) {};

    void perform() {
        widget->perform_search(this->text.value(), false);
    }

    void pre_perform() {
        std::optional<std::pair<int, int>> chapter_range = widget->main_document_view->get_current_page_range();
        if (chapter_range) {
            std::stringstream search_range_string;
            search_range_string << "<" << chapter_range.value().first << "," << chapter_range.value().second << ">";
            widget->text_command_line_edit->setText(search_range_string.str().c_str() + widget->text_command_line_edit->text());
        }

    }

    bool pushes_state() {
        return true;
    }

    std::string text_requirement_name() {
        return "Search Term";
    }
};

class RegexSearchCommand : public TextCommand {
public:
    static inline const std::string cname = "regex_search";
    static inline const std::string hname = "Search using regular expression";
    RegexSearchCommand(MainWidget* w) : TextCommand(cname, w) {};

    void perform() {
        widget->perform_search(this->text.value(), true);
    }

    bool pushes_state() {
        return true;
    }

    std::string text_requirement_name() {
        return "regex";
    }
};

class AddBookmarkCommand : public TextCommand {
public:
    static inline const std::string cname = "add_bookmark";
    static inline const std::string hname = "Add an invisible bookmark in the current location";
    AddBookmarkCommand(MainWidget* w) : TextCommand(cname, w) {}

    void perform() {
        std::string uuid = widget->main_document_view->add_bookmark(text.value());
        result = utf8_decode(uuid);
    }


    std::string text_requirement_name() {
        return "Bookmark Text";
    }
};

class AddBookmarkMarkedCommand : public Command {

public:
    static inline const std::string cname = "add_marked_bookmark";
    static inline const std::string hname = "Add a bookmark in the selected location";

    std::optional<std::wstring> text_;
    std::optional<AbsoluteDocumentPos> point_;

    AddBookmarkMarkedCommand(MainWidget* w) : Command(cname, w) {};

    std::optional<Requirement> next_requirement(MainWidget* widget) {

        if (!point_.has_value()) {
            Requirement req = { RequirementType::Point, "Bookmark Location" };
            return req;
        }
        if (!text_.has_value()) {
            Requirement req = { RequirementType::Text, "Bookmark Text" };
            return req;
        }
        return {};
    }

    void set_text_requirement(std::wstring value) {
        text_ = value;
    }


    void set_point_requirement(AbsoluteDocumentPos value) {
        point_ = value;
    }


    void perform() {
        std::string uuid = widget->doc()->add_marked_bookmark(text_.value(), point_.value());
        widget->invalidate_render();
        result = utf8_decode(uuid);
    }
};


class CreateVisiblePortalCommand : public Command {

public:
    static inline const std::string cname = "create_visible_portal";
    static inline const std::string hname = "";

    std::optional<AbsoluteDocumentPos> point_;

    CreateVisiblePortalCommand(MainWidget* w) : Command(cname, w) {};

    std::optional<Requirement> next_requirement(MainWidget* widget) {

        if (!point_.has_value()) {
            Requirement req = { RequirementType::Point, "Location" };
            return req;
        }
        return {};
    }

    void set_point_requirement(AbsoluteDocumentPos value) {
        point_ = value;
    }

    void perform() {
        widget->start_creating_rect_portal(point_.value());
    }

};

class CopyDrawingsFromScratchpadCommand : public Command {

public:
    static inline const std::string cname = "copy_drawings_from_scratchpad";
    static inline const std::string hname = "";

    std::optional<AbsoluteRect> rect_;

    CopyDrawingsFromScratchpadCommand(MainWidget* w) : Command(cname, w) {};

    std::optional<Requirement> next_requirement(MainWidget* widget) {

        if (!rect_.has_value()) {
            Requirement req = { RequirementType::Rect, "Screenshot rect" };
            return req;
        }
        return {};
    }

    void set_rect_requirement(AbsoluteRect value) {
        rect_ = value;
    }

    void perform() {
        AbsoluteDocumentPos scratchpad_pos = AbsoluteDocumentPos{ widget->scratchpad->get_offset_x(), widget->scratchpad->get_offset_y() };
        AbsoluteDocumentPos main_window_pos = widget->main_document_view->get_offsets();
        AbsoluteDocumentPos click_pos = rect_->center().to_window(widget->scratchpad).to_absolute(widget->main_document_view);

        //float diff_x = main_window_pos.x - scratchpad_pos.x;
        //float diff_y = main_window_pos.y - scratchpad_pos.y;

        auto indices = widget->scratchpad->get_intersecting_objects(rect_.value());
        std::vector<FreehandDrawing> drawings;
        std::vector<PixmapDrawing> pixmaps;

        widget->scratchpad->get_selected_objects_with_indices(indices, drawings, pixmaps);

        for (auto& drawing : drawings) {
            for (int i = 0; i < drawing.points.size(); i++) {
                drawing.points[i].pos = drawing.points[i].pos.to_window(widget->scratchpad).to_absolute(widget->main_document_view);
            }
        }

        FreehandDrawingMoveData md;
        md.initial_drawings = drawings;
        md.initial_pixmaps = {};
        md.initial_mouse_position = click_pos;
        widget->freehand_drawing_move_data = md;
        widget->toggle_scratchpad_mode();
        widget->set_rect_select_mode(false);
        widget->invalidate_render();
    }

};

class CopyScreenshotToClipboard : public Command {

public:
    static inline const std::string cname = "copy_screenshot_to_clipboard";
    static inline const std::string hname = "";

    std::optional<AbsoluteRect> rect_;

    CopyScreenshotToClipboard(MainWidget* w) : Command(cname, w) {};

    std::optional<Requirement> next_requirement(MainWidget* widget) {

        if (!rect_.has_value()) {
            Requirement req = { RequirementType::Rect, "Screenshot rect" };
            return req;
        }
        return {};
    }

    void set_rect_requirement(AbsoluteRect value) {
        rect_ = value;
    }

    void perform() {
        widget->clear_selected_rect();

        WindowRect window_rect = rect_->to_window(widget->main_document_view);
        window_rect.y0 += 1;
        QRect window_qrect = QRect(window_rect.x0, window_rect.y0, window_rect.width(), window_rect.height());

        float ratio = QGuiApplication::primaryScreen()->devicePixelRatio();
        QPixmap pixmap(static_cast<int>(window_qrect.width() * ratio), static_cast<int>(window_qrect.height() * ratio));
        pixmap.setDevicePixelRatio(ratio);

        //widget->render(&pixmap, QPoint(), QRegion(widget->rect()));
        widget->render(&pixmap, QPoint(), QRegion(window_qrect));
        QApplication::clipboard()->setPixmap(pixmap);

        widget->set_rect_select_mode(false);
        widget->invalidate_render();
    }

};

class CopyScreenshotToScratchpad : public Command {

public:
    static inline const std::string cname = "copy_screenshot_to_scratchpad";
    static inline const std::string hname = "";

    std::optional<AbsoluteRect> rect_;

    CopyScreenshotToScratchpad(MainWidget* w) : Command(cname, w) {};

    std::optional<Requirement> next_requirement(MainWidget* widget) {

        if (!rect_.has_value()) {
            Requirement req = { RequirementType::Rect, "Screenshot rect" };
            return req;
        }
        return {};
    }

    void set_rect_requirement(AbsoluteRect value) {
        rect_ = value;
    }

    void perform() {
        widget->clear_selected_rect();

        WindowRect window_rect = rect_->to_window(widget->main_document_view);
        window_rect.y0 += 1;
        QRect window_qrect = QRect(window_rect.x0, window_rect.y0, window_rect.width(), window_rect.height());

        float ratio = QGuiApplication::primaryScreen()->devicePixelRatio();
        QPixmap pixmap(static_cast<int>(window_qrect.width() * ratio), static_cast<int>(window_qrect.height() * ratio));
        pixmap.setDevicePixelRatio(ratio);

        //widget->render(&pixmap, QPoint(), QRegion(widget->rect()));
        widget->render(&pixmap, QPoint(), QRegion(window_qrect));
        widget->toggle_scratchpad_mode();
        widget->add_pixmap_to_scratchpad(pixmap);
        widget->set_rect_select_mode(false);
        widget->invalidate_render();
    }
};

class AddBookmarkFreetextCommand : public Command {

public:
    static inline const std::string cname = "add_freetext_bookmark";
    static inline const std::string hname = "Add a text bookmark in the selected rectangle";

    std::optional<std::wstring> text_;
    std::optional<AbsoluteRect> rect_;
    int pending_index = -1;

    AddBookmarkFreetextCommand(MainWidget* w) : Command(cname, w) {};

    void on_text_change(const QString& new_text) override {
        widget->doc()->get_bookmarks()[widget->selected_bookmark_index].description = new_text.toStdWString();
    }

    std::optional<Requirement> next_requirement(MainWidget* widget) {

        if (!rect_.has_value()) {
            Requirement req = { RequirementType::Rect, "Bookmark Location" };
            return req;
        }
        if (!text_.has_value()) {
            Requirement req = { RequirementType::Text, "Bookmark Text" };
            return req;
        }
        return {};
    }

    void set_text_requirement(std::wstring value) {
        text_ = value;
    }

    void set_rect_requirement(AbsoluteRect value) {
        rect_ = value;
        pending_index = widget->doc()->add_incomplete_freetext_bookmark(value);
        widget->set_selected_bookmark_index(pending_index);

        widget->clear_selected_rect();
        widget->validate_render();
    }


    void on_cancel() {

        if (pending_index != -1) {
            widget->doc()->undo_pending_bookmark(pending_index);
        }
    }

    void perform() {
        //widget->doc()->add_freetext_bookmark(text_.value(), rect_.value());
        if (text_.value().size() > 0) {
            std::string uuid = widget->doc()->add_pending_freetext_bookmark(pending_index, text_.value());
            result = utf8_decode(uuid);
        }
        else {
            widget->doc()->undo_pending_bookmark(pending_index);
            result = L"";
        }

        widget->clear_selected_rect();
        widget->invalidate_render();
    }
};

class GotoBookmarkCommand : public GenericGotoLocationCommand {
public:
    static inline const std::string cname = "goto_bookmark";
    static inline const std::string hname = "Open the bookmark list of current document";
    GotoBookmarkCommand(MainWidget* w) : GenericGotoLocationCommand(cname, w) {};

    void handle_generic_requirement() {
        widget->handle_goto_bookmark();
    }
};

class GotoPortalListCommand : public GenericGotoLocationCommand {
public:
    static inline const std::string cname = "goto_portal_list";
    static inline const std::string hname = "";
    GotoPortalListCommand(MainWidget* w) : GenericGotoLocationCommand(cname, w) {};

    void handle_generic_requirement() {
        widget->handle_goto_portal_list();
    }
};

class GotoBookmarkGlobalCommand : public GenericPathAndLocationCommadn {
public:
    static inline const std::string cname = "goto_bookmark_g";
    static inline const std::string hname = "Open the bookmark list of all documents";

    GotoBookmarkGlobalCommand(MainWidget* w) : GenericPathAndLocationCommadn(cname, w) {};

    void handle_generic_requirement() {
        widget->handle_goto_bookmark_global();
    }
    bool requires_document() { return false; }
};

class IncreaseFreetextBookmarkFontSizeCommand : public Command {
public:
    static inline const std::string cname = "increase_freetext_font_size";
    static inline const std::string hname = "Increase freetext bookmark font size";
    IncreaseFreetextBookmarkFontSizeCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        FREETEXT_BOOKMARK_FONT_SIZE *= 1.1f;
        if (FREETEXT_BOOKMARK_FONT_SIZE > 100) {
            FREETEXT_BOOKMARK_FONT_SIZE = 100;
        }
        widget->update_selected_bookmark_font_size();

    }
};

class DecreaseFreetextBookmarkFontSizeCommand : public Command {
public:
    static inline const std::string cname = "decrease_freetext_font_size";
    static inline const std::string hname = "Decrease freetext bookmark font size";
    DecreaseFreetextBookmarkFontSizeCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        FREETEXT_BOOKMARK_FONT_SIZE /= 1.1f;
        if (FREETEXT_BOOKMARK_FONT_SIZE < 1) {
            FREETEXT_BOOKMARK_FONT_SIZE = 1;
        }
        widget->update_selected_bookmark_font_size();
    }
};


class GotoHighlightCommand : public GenericGotoLocationCommand {
public:
    static inline const std::string cname = "goto_highlight";
    static inline const std::string hname = "Open the highlight list of the current document";
    GotoHighlightCommand(MainWidget* w) : GenericGotoLocationCommand(cname, w) {};

    void handle_generic_requirement() {
        widget->handle_goto_highlight();
    }
};


class GotoHighlightGlobalCommand : public GenericPathAndLocationCommadn {
public:
    static inline const std::string cname = "goto_highlight_g";
    static inline const std::string hname = "Open the highlight list of the all documents";

    GotoHighlightGlobalCommand(MainWidget* w) : GenericPathAndLocationCommadn(cname, w) {};

    void handle_generic_requirement() {
        widget->handle_goto_highlight_global();
    }
    bool requires_document() { return false; }
};

class GotoTableOfContentsCommand : public GenericPathCommand {
public:
    static inline const std::string cname = "goto_toc";
    static inline const std::string hname = "Open table of contents";
    GotoTableOfContentsCommand(MainWidget* w) : GenericPathCommand(cname, w) {};

    std::optional<QVariant>  target_location = {};

    std::optional<Requirement> next_requirement(MainWidget* widget) {
        if (target_location) {
            return {};
        }
        else {
            return Requirement{ RequirementType::Generic, "Location" };
        }
    }

    void set_generic_requirement(QVariant value) {
        target_location = value;
    }


    void handle_generic_requirement() {
        //widget->handle_goto_loaded_document();
        widget->handle_goto_toc();
    }


    void perform() {
        QVariant val = target_location.value();

        if (val.canConvert<int>()) {
            int page = val.toInt();

            widget->main_document_view->goto_page(page);

            if (TOC_JUMP_ALIGN_TOP) {
                widget->main_document_view->scroll_mid_to_top();
            }
        }
        else {
            QList<QVariant> location = val.toList();
            int page = location[0].toInt();
            float x_offset = location[1].toFloat();
            float y_offset = location[2].toFloat();

            if (std::isnan(y_offset)) {
                widget->main_document_view->goto_page(page);
            }
            else {
                widget->main_document_view->goto_offset_within_page(page,  y_offset);
            }
            if (TOC_JUMP_ALIGN_TOP) {
                widget->main_document_view->scroll_mid_to_top();
            }

        }
    }

    bool pushes_state() { return true; }
};

class PortalCommand : public Command {
public:
    static inline const std::string cname = "portal";
    static inline const std::string hname = "Start creating a portal";
    PortalCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        widget->handle_portal();
    }
};

class ToggleWindowConfigurationCommand : public Command {
public:
    static inline const std::string cname = "toggle_window_configuration";
    static inline const std::string hname = "Toggle between one window and two window configuration";
    ToggleWindowConfigurationCommand(MainWidget* w) : Command(cname, w) {};
    void perform() {
        widget->toggle_window_configuration();
    }

    bool requires_document() { return false; }
};

class NextStateCommand : public Command {
public:
    static inline const std::string cname = "next_state";
    static inline const std::string hname = "Go forward in history";
    NextStateCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        widget->next_state();
    }

    bool requires_document() { return false; }
};

class PrevStateCommand : public Command {
public:
    static inline const std::string cname = "prev_state";
    static inline const std::string hname = "Go backward in history";
    PrevStateCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        widget->prev_state();
    }

    bool requires_document() { return false; }
};

class AddHighlightCommand : public SymbolCommand {
public:
    static inline const std::string cname = "add_highlight";
    static inline const std::string hname = "Highlight selected text";
    AddHighlightCommand(MainWidget* w) : SymbolCommand(cname, w) {};

    void perform() {
        result = widget->handle_add_highlight(symbol);
    }

    std::vector<char> special_symbols() {
        std::vector<char> res = { '_', };
        return res;
    }
};

class CommandPaletteCommand : public Command {
public:
    static inline const std::string cname = "command_palette";
    static inline const std::string hname = "";
    CommandPaletteCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        widget->show_command_palette();
    }

    bool requires_document() { return false; }
};

class CommandCommand : public Command {
public:
    static inline const std::string cname = "command";
    static inline const std::string hname = "";
    CommandCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        QStringList command_names = widget->command_manager->get_all_command_names();
        if (!TOUCH_MODE) {

            widget->set_current_widget(new CommandSelector(
                FUZZY_SEARCHING,
                &widget->on_command_done,
                widget,
                command_names,
                widget->command_manager->command_required_prefixes,
                widget->input_handler->get_command_key_mappings())
            );
        }
        else {

            TouchCommandSelector* tcs = new TouchCommandSelector(FUZZY_SEARCHING, command_names, widget);
            widget->set_current_widget(tcs);
        }

        widget->show_current_widget();

    }

    bool requires_document() { return false; }
};

class ScreenshotCommand : public Command {
public:
    static inline const std::string cname = "screenshot";
    static inline const std::string hname = "";
    ScreenshotCommand(MainWidget* w) : Command(cname, w) {};

    std::wstring file_name;

    std::optional<Requirement> next_requirement(MainWidget* widget) {
        if (file_name.size() == 0) {
            return Requirement{ RequirementType::Text, "File Path" };
        }
        else {
            return {};
        }
    }

    void set_text_requirement(std::wstring value) {
        file_name = value;
    }

    void perform() {
        widget->screenshot(file_name);
    }
    bool requires_document() { return false; }
};

class FramebufferScreenshotCommand : public Command {
public:
    static inline const std::string cname = "framebuffer_screenshot";
    static inline const std::string hname = "";
    FramebufferScreenshotCommand(MainWidget* w) : Command(cname, w) {};

    std::wstring file_name;

    std::optional<Requirement> next_requirement(MainWidget* widget) {
        if (file_name.size() == 0) {
            return Requirement{ RequirementType::Text, "File Path" };
        }
        else {
            return {};
        }
    }

    void set_text_requirement(std::wstring value) {
        file_name = value;
    }

    void perform() {
        widget->framebuffer_screenshot(file_name);
    }

    bool requires_document() { return false; }
};

class ExportDefaultConfigFile : public Command {
public:
    static inline const std::string cname = "export_default_config_file";
    static inline const std::string hname = "";
    ExportDefaultConfigFile(MainWidget* w) : Command(cname, w) {};

    std::wstring file_name;

    std::optional<Requirement> next_requirement(MainWidget* widget) {
        if (file_name.size() == 0) {
            return Requirement{ RequirementType::File, "File Path" };
        }
        else {
            return {};
        }
    }

    void set_file_requirement(std::wstring value) {
        file_name = value;
    }

    void perform() {
        widget->export_default_config_file(file_name);
    }

    bool requires_document() { return false; }
};

class ExportCommandNamesCommand : public Command {
public:
    static inline const std::string cname = "export_command_names";
    static inline const std::string hname = "";
    ExportCommandNamesCommand(MainWidget* w) : Command(cname, w) {};

    std::wstring file_name;

    std::optional<Requirement> next_requirement(MainWidget* widget) {
        if (file_name.size() == 0) {
            return Requirement{ RequirementType::File, "File Path" };
        }
        else {
            return {};
        }
    }

    void set_file_requirement(std::wstring value) {
        file_name = value;
    }

    void perform() {
        //widget->framebuffer_screenshot(file_name);
        widget->export_command_names(file_name);
    }

    bool requires_document() { return false; }
};



class ExportConfigNamesCommand : public Command {
public:
    static inline const std::string cname = "export_config_names";
    static inline const std::string hname = "";
    ExportConfigNamesCommand(MainWidget* w) : Command(cname, w) {};

    std::wstring file_name;

    std::optional<Requirement> next_requirement(MainWidget* widget) {
        if (file_name.size() == 0) {
            return Requirement{ RequirementType::File, "File Path" };
        }
        else {
            return {};
        }
    }

    void set_file_requirement(std::wstring value) {
        file_name = value;
    }

    void perform() {
        //widget->framebuffer_screenshot(file_name);
        widget->export_config_names(file_name);
    }

    bool requires_document() { return false; }
};

class GenericWaitCommand : public Command {
public:
    GenericWaitCommand(std::string name, MainWidget* w) : Command(name, w) {};
    bool finished = false;
    QTimer* timer = nullptr;
    QMetaObject::Connection timer_connection;

    std::optional<Requirement> next_requirement(MainWidget* widget) {
        if (!finished) {
            return Requirement{ RequirementType::Generic, "dummy" };
        }
        else {
            return {};
        }
    }

    virtual ~GenericWaitCommand() {

        if (timer) {
            timer->stop();
            timer->deleteLater();
        }
    }

    void set_generic_requirement(QVariant value)
    {
        finished = true;
    }

    virtual bool is_ready() = 0;

    void handle_generic_requirement() override{
        if (finished) return;

        timer = new QTimer();
        timer->setInterval(100);
        timer_connection = QObject::connect(timer, &QTimer::timeout, [this]() {
            if (is_ready()) {
                widget->advance_waiting_command(get_name());
            }
            });
        timer->start();

    }

    void perform() {
    }

};

class WaitCommand : public GenericWaitCommand {
    std::optional<int> duration = {};
    QDateTime start_time;
public:
    static inline const std::string cname = "wait";
    static inline const std::string hname = "";
    WaitCommand(MainWidget* w) : GenericWaitCommand(cname, w) {
        start_time = QDateTime::currentDateTime();
    };

    std::optional<Requirement> next_requirement(MainWidget* widget) {
        if (duration.has_value()) {
            return GenericWaitCommand::next_requirement(widget);
        }
        else {
            return Requirement{ RequirementType::Text, "Duration" };
        }
    }


    void set_text_requirement(std::wstring text) {
        duration = QString::fromStdWString(text).toInt();
    }

    bool is_ready() override {
        return start_time.msecsTo(QDateTime::currentDateTime()) > duration.value();
    }
};

class WaitForIndexingToFinishCommand : public GenericWaitCommand {
public:
    static inline const std::string cname = "wait_for_indexing_to_finish";
    static inline const std::string hname = "";
    WaitForIndexingToFinishCommand(MainWidget* w) : GenericWaitCommand(cname, w) {};

    void set_generic_requirement(QVariant value)
    {
        finished = true;
    }

    bool is_ready() override {
        return widget->is_index_ready();
    }
};

class WaitForRendersToFinishCommand : public GenericWaitCommand {
public:
    static inline const std::string cname = "wait_for_renders_to_finish";
    static inline const std::string hname = "";
    WaitForRendersToFinishCommand(MainWidget* w) : GenericWaitCommand(cname, w) {};

    bool is_ready() override {
        return widget->is_render_ready();
    }
};

class WaitForSearchToFinishCommand : public GenericWaitCommand {
public:
    static inline const std::string cname = "wait_for_search_to_finish";
    static inline const std::string hname = "";
    WaitForSearchToFinishCommand(MainWidget* w) : GenericWaitCommand(cname, w) {};

    bool is_ready() override {
        return widget->is_search_ready();
    }
};

class OpenDocumentCommand : public Command {
public:
    static inline const std::string cname = "open_document";
    static inline const std::string hname = "open_document";
    OpenDocumentCommand(MainWidget* w) : Command(cname, w) {};

    std::wstring file_name;

    bool pushes_state() {
        return true;
    }

    std::optional<Requirement> next_requirement(MainWidget* widget) {
        if (file_name.size() == 0) {
            return Requirement{ RequirementType::File, "File" };
        }
        else {
            return {};
        }
    }

    void set_file_requirement(std::wstring value) {
        file_name = value;
    }

    void perform() {
        widget->open_document(file_name);
    }

    bool requires_document() { return false; }
};


class MoveSmoothCommand : public Command {
    bool was_held = false;
public:
    MoveSmoothCommand(std::string name, MainWidget* w) : Command(name, w) {};

    virtual bool is_down() = 0;

    void perform() {
        widget->handle_move_smooth_press(is_down());
    }

    void perform_up() {
        if (was_held) widget->velocity_y = 0;
    }

    bool is_holdable() {
        return true;
    }

    void on_key_hold() {
        was_held = true;
        widget->handle_move_smooth_hold(is_down());
        widget->validate_render();
    }
};

class MoveUpSmoothCommand : public MoveSmoothCommand {
public:
    static inline const std::string cname = "move_up_smooth";
    static inline const std::string hname = "";
    MoveUpSmoothCommand(MainWidget* w) : MoveSmoothCommand(cname, w) {};

    bool is_down() {
        return false;
    }
};

class MoveDownSmoothCommand : public MoveSmoothCommand {
public:
    static inline const std::string cname = "move_down_smooth";
    static inline const std::string hname = "";
    MoveDownSmoothCommand(MainWidget* w) : MoveSmoothCommand(cname, w) {};

    bool is_down() {
        return true;
    }
};

class ToggleTwoPageModeCommand : public Command {
public:
    static inline const std::string cname = "toggle_two_page_mode";
    static inline const std::string hname = "";
    ToggleTwoPageModeCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        widget->handle_toggle_two_page_mode();
    }
};

class FitEpubToWindowCommand : public Command {
public:
    static inline const std::string cname = "fit_epub_to_window";
    static inline const std::string hname = "";
    FitEpubToWindowCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {

        EPUB_WIDTH = widget->width() / widget->dv()->get_zoom_level();
        EPUB_HEIGHT = widget->height() / widget->dv()->get_zoom_level();
        widget->on_config_changed("epub_width");
    }
};

class MoveDownCommand : public Command {
public:
    static inline const std::string cname = "move_down";
    static inline const std::string hname = "Move down";
    MoveDownCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        int rp = num_repeats == 0 ? 1 : num_repeats;
        widget->handle_vertical_move(rp);
    }
};

class MoveUpCommand : public Command {
public:
    static inline const std::string cname = "move_up";
    static inline const std::string hname = "Move up";
    MoveUpCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        int rp = num_repeats == 0 ? 1 : num_repeats;
        widget->handle_vertical_move(-rp);
    }
};

class MoveLeftInOverviewCommand : public Command {
public:
    static inline const std::string cname = "move_left_in_overview";
    static inline const std::string hname = "";
    MoveLeftInOverviewCommand(MainWidget* w) : Command(cname, w) {};
    void perform() {
        widget->scroll_overview(0, 1);
    }
};

class MoveRightInOverviewCommand : public Command {
public:
    static inline const std::string cname = "move_right_in_overview";
    static inline const std::string hname = "";
    MoveRightInOverviewCommand(MainWidget* w) : Command(cname, w) {};
    void perform() {
        widget->scroll_overview(0, -1);
    }
};

class MoveLeftCommand : public Command {
public:
    static inline const std::string cname = "move_left";
    static inline const std::string hname = "Move left";
    MoveLeftCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        int rp = num_repeats == 0 ? 1 : num_repeats;
        widget->handle_horizontal_move(-rp);
    }
};

class MoveRightCommand : public Command {
public:
    static inline const std::string cname = "move_right";
    static inline const std::string hname = "Move right";
    MoveRightCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        int rp = num_repeats == 0 ? 1 : num_repeats;
        widget->handle_horizontal_move(rp);
    }
};

class JavascriptCommand : public Command {
public:
    std::string command_name;
    std::wstring code;
    std::optional<std::wstring> entry_point = {};
    bool is_async;

    JavascriptCommand(std::string command_name, std::wstring code_, std::optional<std::wstring> entry_point_, bool is_async_, MainWidget* w) :  Command(command_name, w), command_name(command_name) {
        code = code_;
        entry_point = entry_point_;
        is_async = is_async_;
    };

    void perform() {
        widget->run_javascript_command(code, entry_point, is_async);
    }

    std::string get_name() {
        return command_name;
    }

};

class SaveScratchpadCommand : public Command {
public:
    static inline const std::string cname = "save_scratchpad";
    static inline const std::string hname = "";
    SaveScratchpadCommand(MainWidget* w) : Command(cname, w) {};
    void perform() {
        widget->save_scratchpad();
    }
};

class LoadScratchpadCommand : public Command {
public:
    static inline const std::string cname = "load_scratchpad";
    static inline const std::string hname = "";
    LoadScratchpadCommand(MainWidget* w) : Command(cname, w) {};
    void perform() {
        widget->load_scratchpad();
    }
};

class ClearScratchpadCommand : public Command {
public:
    static inline const std::string cname = "clear_scratchpad";
    static inline const std::string hname = "";
    ClearScratchpadCommand(MainWidget* w) : Command(cname, w) {};
    void perform() {
        widget->clear_scratchpad();
    }
};

class ZoomInCommand : public Command {
public:
    static inline const std::string cname = "zoom_in";
    static inline const std::string hname = "Zoom in";
    ZoomInCommand(MainWidget* w) : Command(cname, w) {};
    void perform() {
        widget->main_document_view->zoom_in();
        widget->last_smart_fit_page = {};
    }
};

class ZoomOutOverviewCommand : public Command {
public:
    static inline const std::string cname = "zoom_out_overview";
    static inline const std::string hname = "";
    ZoomOutOverviewCommand(MainWidget* w) : Command(cname, w) {};
    void perform() {
        widget->zoom_out_overview();
    }
};

class ZoomInOverviewCommand : public Command {
public:
    static inline const std::string cname = "zoom_in_overview";
    static inline const std::string hname = "";
    ZoomInOverviewCommand(MainWidget* w) : Command(cname, w) {};
    void perform() {
        widget->zoom_in_overview();
    }
};

class FitToPageWidthCommand : public Command {
public:
    static inline const std::string cname = "fit_to_page_width";
    static inline const std::string hname = "Fit the page to screen width";
    FitToPageWidthCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        widget->handle_fit_to_page_width(false);
    }
};

class FitToPageSmartCommand : public Command {
public:
    static inline const std::string cname = "fit_to_page_smart";
    static inline const std::string hname = "";
    FitToPageSmartCommand(MainWidget* w) : Command(cname, w) {};
    void perform() {
        widget->main_document_view->fit_to_page_height_and_width_smart();
    }

};

class FitToPageWidthSmartCommand : public Command {
public:
    static inline const std::string cname = "fit_to_page_width_smart";
    static inline const std::string hname = "Fit the page to screen width, ignoring white page margins";
    FitToPageWidthSmartCommand(MainWidget* w) : Command(cname, w) {};
    void perform() {
        widget->handle_fit_to_page_width(true);
    }
};

class FitToPageHeightCommand : public Command {
public:
    static inline const std::string cname = "fit_to_page_height";
    static inline const std::string hname = "Fit the page to screen height";
    FitToPageHeightCommand(MainWidget* w) : Command(cname, w) {};
    void perform() {
        widget->main_document_view->fit_to_page_height();
        widget->last_smart_fit_page = {};
    }
};

class FitToPageHeightSmartCommand : public Command {
public:
    static inline const std::string cname = "fit_to_page_height_smart";
    static inline const std::string hname = "Fit the page to screen height, ignoring white page margins";
    FitToPageHeightSmartCommand(MainWidget* w) : Command(cname, w) {};
    void perform() {
        widget->main_document_view->fit_to_page_height(true);
    }
};

class NextPageCommand : public Command {
public:
    static inline const std::string cname = "next_page";
    static inline const std::string hname = "Go to next page";
    NextPageCommand(MainWidget* w) : Command(cname, w) {};
    void perform() {
        widget->main_document_view->move_pages(std::max(1, num_repeats));
    }
};

class PreviousPageCommand : public Command {
public:
    static inline const std::string cname = "previous_page";
    static inline const std::string hname = "Go to previous page";
    PreviousPageCommand(MainWidget* w) : Command(cname, w) {};
    void perform() {
        widget->main_document_view->move_pages(std::min(-1, -num_repeats));
    }
};

class ZoomOutCommand : public Command {
public:
    static inline const std::string cname = "zoom_out";
    static inline const std::string hname = "Zoom out";
    ZoomOutCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        widget->main_document_view->zoom_out();
        widget->last_smart_fit_page = {};
    }
};

class GotoDefinitionCommand : public Command {
public:
    static inline const std::string cname = "goto_definition";
    static inline const std::string hname = "Go to the reference in current highlighted line";
    GotoDefinitionCommand(MainWidget* w) : Command(cname, w) {};
    void perform() {
        if (widget->main_document_view->goto_definition()) {
            widget->main_document_view->exit_ruler_mode();
        }
    }

    bool pushes_state() {
        return true;
    }

};

class OverviewDefinitionCommand : public Command {
public:
    static inline const std::string cname = "overview_definition";
    static inline const std::string hname = "Open an overview to the reference in current highlighted line";
    OverviewDefinitionCommand(MainWidget* w) : Command(cname, w) {};
    void perform() {
        widget->overview_to_definition();
    }
};

class PortalToDefinitionCommand : public Command {
public:
    static inline const std::string cname = "portal_to_definition";
    static inline const std::string hname = "Create a portal to the definition in current highlighted line";
    PortalToDefinitionCommand(MainWidget* w) : Command(cname, w) {};
    void perform() {
        widget->portal_to_definition();
    }

};

class MoveVisualMarkDownCommand : public Command {
public:
    static inline const std::string cname = "move_visual_mark_down";
    static inline const std::string hname = "Move current highlighted line down";
    MoveVisualMarkDownCommand(MainWidget* w) : Command(cname, w) {};
    void perform() {
        int rp = num_repeats == 0 ? 1 : num_repeats;
        widget->move_visual_mark_command(rp);
    }
};

class MoveVisualMarkUpCommand : public Command {
public:
    static inline const std::string cname = "move_visual_mark_up";
    static inline const std::string hname = "Move current highlighted line up";
    MoveVisualMarkUpCommand(MainWidget* w) : Command(cname, w) {};
    void perform() {
        int rp = num_repeats == 0 ? 1 : num_repeats;
        widget->move_visual_mark_command(-rp);
    }
};

class MoveVisualMarkNextCommand : public Command {
public:
    static inline const std::string cname = "move_visual_mark_next";
    static inline const std::string hname = "Move the current highlighted line to the next unread text";
    MoveVisualMarkNextCommand(MainWidget* w) : Command(cname, w) {};
    void perform() {
        widget->move_visual_mark_next();
    }
};

class MoveVisualMarkPrevCommand : public Command {
public:
    static inline const std::string cname = "move_visual_mark_prev";
    static inline const std::string hname = "Move the current highlighted line to the previous";
    MoveVisualMarkPrevCommand(MainWidget* w) : Command(cname, w) {};
    void perform() {
        widget->move_visual_mark_prev();
    }

};


class GotoPageWithPageNumberCommand : public TextCommand {
public:
    static inline const std::string cname = "goto_page_with_page_number";
    static inline const std::string hname = "Go to page with page number";
    GotoPageWithPageNumberCommand(MainWidget* w) : TextCommand(cname, w) {};

    void perform() {
        std::wstring text_ = text.value();
        if (is_string_numeric(text_.c_str()) && text_.size() < 6) { // make sure the page number is valid
            int dest = std::stoi(text_.c_str()) - 1;
            widget->main_document_view->goto_page(dest + widget->main_document_view->get_page_offset());
        }
    }

    std::string text_requirement_name() {
        return "Page Number";
    }

    bool pushes_state() {
        return true;
    }
};

class EditSelectedBookmarkCommand : public TextCommand {
public:
    static inline const std::string cname = "edit_selected_bookmark";
    static inline const std::string hname = "Edit selected bookmark";
    std::wstring initial_text;
    float initial_font_size;
    int index = -1;

    EditSelectedBookmarkCommand(MainWidget* w) : TextCommand(cname, w) {};

    void on_text_change(const QString& new_text) override {
        widget->doc()->get_bookmarks()[widget->selected_bookmark_index].description = new_text.toStdWString();
    }

    void pre_perform() {

        if (widget->selected_bookmark_index > -1) {
            initial_text = widget->doc()->get_bookmarks()[widget->selected_bookmark_index].description;
            initial_font_size = widget->doc()->get_bookmarks()[widget->selected_bookmark_index].font_size;
            index = widget->selected_bookmark_index;

            if (TOUCH_MODE) {
                if (widget->current_widget_stack.size() > 0) {
                    TouchTextEdit* stack_top = dynamic_cast<TouchTextEdit*>(widget->current_widget_stack.back());
                    if (stack_top) {
                        stack_top->set_text(widget->doc()->get_bookmarks()[widget->selected_bookmark_index].description);
                    }
                }
            }
            else {
                widget->text_command_line_edit->setText(
                    QString::fromStdWString(widget->doc()->get_bookmarks()[widget->selected_bookmark_index].description)
                );
            }
        }
    }

    void on_cancel() {
        if (index > -1) {
            widget->doc()->get_bookmarks()[index].description = initial_text;
            widget->doc()->get_bookmarks()[index].font_size = initial_font_size;
        }
    }

    std::optional<Requirement> next_requirement(MainWidget* widget) {
        if (widget->selected_bookmark_index == -1) return {};
        return TextCommand::next_requirement(widget);
    }

    void perform() {
        if (widget->selected_bookmark_index != -1) {
            std::wstring text_ = text.value();
            widget->change_selected_bookmark_text(text_);
            widget->invalidate_render();
        }
        else {
            show_error_message(L"No bookmark is selected");
        }
    }

    std::string text_requirement_name() {
        return "Bookmark Text";
    }

};

class EditSelectedHighlightCommand : public TextCommand {
public:
    static inline const std::string cname = "edit_selected_highlight";
    static inline const std::string hname = "Edit the text comment of current selected highlight";
    int index = -1;

    EditSelectedHighlightCommand(MainWidget* w) : TextCommand(cname, w) {};

    void pre_perform() {
        index = widget->selected_highlight_index;

        if (index != -1) {
            widget->set_text_prompt_text(
                QString::fromStdWString(widget->doc()->get_highlights()[index].text_annot));
        }
        //widget->text_command_line_edit->setText(
        //    QString::fromStdWString(widget->doc()->get_highlights()[widget->selected_highlight_index].text_annot)
        //);
    }

    void perform() {
        if (index != -1) {
            std::wstring text_ = text.value();
            widget->change_selected_highlight_text_annot(text_);
        }
    }

    std::string text_requirement_name() {
        return "Highlight Annotation";
    }

};

class DeletePortalCommand : public Command {
public:
    static inline const std::string cname = "delete_portal";
    static inline const std::string hname = "Delete the closest portal";
    DeletePortalCommand(MainWidget* w) : Command(cname, w) {};
    void perform() {
        widget->main_document_view->delete_closest_portal();
        widget->validate_render();
    }
};

class DeleteBookmarkCommand : public Command {
public:
    static inline const std::string cname = "delete_bookmark";
    static inline const std::string hname = "Delete the closest bookmark";
    DeleteBookmarkCommand(MainWidget* w) : Command(cname, w) {};
    void perform() {
        widget->main_document_view->delete_closest_bookmark();
        widget->validate_render();
    }
};

class GenericVisibleSelectCommand : public Command {
protected:
    std::vector<int> visible_item_indices;
    std::string tag;
    int n_required_tags = 0;
    bool already_pre_performed = false;

public:
    GenericVisibleSelectCommand(std::string name, MainWidget* w) : Command(name, w) {};

    virtual std::vector<int> get_visible_item_indices() = 0;
    virtual void handle_indices_pre_perform() = 0;

    void pre_perform() override {
        if (!already_pre_performed) {
            if (widget->selected_highlight_index == -1) {
                visible_item_indices = get_visible_item_indices();
                n_required_tags = get_num_tag_digits(visible_item_indices.size());

                handle_indices_pre_perform();
                widget->invalidate_render();
                //widget->handle_highlight_tags_pre_perform(visible_item_indices);
            }
            already_pre_performed = true;
        }
    }

    std::optional<Requirement> next_requirement(MainWidget* widget) {
        // since pre_perform must be executed in order to determine the requirements, we manually run it here
        pre_perform();

        if (tag.size() < n_required_tags) {
            return Requirement{ RequirementType::Symbol, "tag" };
        }
        else {
            return {};
        }
    }


    virtual void set_symbol_requirement(char value) {
        tag.push_back(value);
    }

    virtual void perform_with_selected_index(std::optional<int> index) = 0;

    void perform() {
        bool should_clear_labels = false;
        if (tag.size() > 0) {
            int index = get_index_from_tag(tag);
            perform_with_selected_index(index);
            //if (index < visible_item_indices.size()) {
            //    widget->set_selected_highlight_index(visible_highlight_indices[index]);
            //}
            should_clear_labels = true;
        }
        else {
            perform_with_selected_index({});
        }

        if (should_clear_labels) {
            widget->clear_keyboard_select_highlights();
        }
    }
};


class GenericHighlightCommand : public GenericVisibleSelectCommand {

public:
    GenericHighlightCommand(std::string name, MainWidget* w) : GenericVisibleSelectCommand(name, w) {};

    std::vector<int> get_visible_item_indices() override {
        return widget->main_document_view->get_visible_highlight_indices();
    }

    void handle_indices_pre_perform() override {
        widget->handle_highlight_tags_pre_perform(visible_item_indices);

    }

    virtual void perform_with_highlight_selected() = 0;

    void perform_with_selected_index(std::optional<int> index) override {
        if (index) {
            if (index < visible_item_indices.size()) {
                widget->set_selected_highlight_index(visible_item_indices[index.value()]);
            }
        }

        perform_with_highlight_selected();
    }

};

class GenericVisibleBookmarkCommand : public GenericVisibleSelectCommand {

public:
    GenericVisibleBookmarkCommand(std::string name, MainWidget* w) : GenericVisibleSelectCommand(name, w) {};

    std::vector<int> get_visible_item_indices() override {
        return widget->main_document_view->get_visible_bookmark_indices();
    }


    void handle_indices_pre_perform() override {
        widget->handle_visible_bookmark_tags_pre_perform(visible_item_indices);
    }

    virtual void perform_with_bookmark_selected() = 0;

    void perform_with_selected_index(std::optional<int> index) override {
        if (index) {
            if (index < visible_item_indices.size()) {
                widget->set_selected_bookmark_index(visible_item_indices[index.value()]);
            }
        }

        perform_with_bookmark_selected();
    }

};

class DeleteVisibleBookmarkCommand : public GenericVisibleBookmarkCommand {

public:
    static inline const std::string cname = "delete_visible_bookmark";
    static inline const std::string hname = "";
    DeleteVisibleBookmarkCommand(MainWidget* w) : GenericVisibleBookmarkCommand(cname, w) {};

    void perform_with_bookmark_selected() override {
        widget->handle_delete_selected_bookmark();
    }
};

class EditVisibleBookmarkCommand : public GenericVisibleBookmarkCommand {

public:
    static inline const std::string cname = "edit_visible_bookmark";
    static inline const std::string hname = "";
    EditVisibleBookmarkCommand(MainWidget* w) : GenericVisibleBookmarkCommand(cname, w) {};

    void perform_with_bookmark_selected() override {
        widget->execute_macro_if_enabled(L"edit_selected_bookmark");
    }
};

class DeleteHighlightCommand : public GenericHighlightCommand {

public:
    static inline const std::string cname = "delete_highlight";
    static inline const std::string hname = "Delete the selected highlight";
    DeleteHighlightCommand(MainWidget* w) : GenericHighlightCommand(cname, w) {};

    void perform_with_highlight_selected() override {
        widget->handle_delete_selected_highlight();
    }
};


class ChangeHighlightTypeCommand : public GenericHighlightCommand {

public:
    static inline const std::string cname = "change_highlight";
    static inline const std::string hname = "";
    ChangeHighlightTypeCommand(MainWidget* w) : GenericHighlightCommand(cname, w) {};

    void perform_with_highlight_selected() {
        widget->execute_macro_if_enabled(L"add_highlight");
    }

};

class AddAnnotationToHighlightCommand : public GenericHighlightCommand {

public:
    static inline const std::string cname = "add_annot_to_highlight";
    static inline const std::string hname = "";
    AddAnnotationToHighlightCommand(MainWidget* w) : GenericHighlightCommand(cname, w) {};

    void perform_with_highlight_selected() {
        widget->execute_macro_if_enabled(L"add_annot_to_selected_highlight");
    }

};

class GotoPortalCommand : public Command {
public:
    static inline const std::string cname = "goto_portal";
    static inline const std::string hname = "Goto closest portal destination";
    GotoPortalCommand(MainWidget* w) : Command(cname, w) {};
    void perform() {
        std::optional<Portal> link = widget->main_document_view->find_closest_portal();
        if (link) {
            widget->open_document(link->dst);
        }
    }

    bool pushes_state() {
        return true;
    }
};

class EditPortalCommand : public Command {
public:
    static inline const std::string cname = "edit_portal";
    static inline const std::string hname = "Edit portal";
    EditPortalCommand(MainWidget* w) : Command(cname, w) {};
    void perform() {
        std::optional<Portal> link = widget->main_document_view->find_closest_portal();
        if (link) {
            widget->portal_to_edit = link;
            widget->open_document(link->dst);
        }
    }

    bool pushes_state() {
        return true;
    }
};

class OpenPrevDocCommand : public GenericPathAndLocationCommadn {
public:
    static inline const std::string cname = "open_prev_doc";
    static inline const std::string hname = "Open the list of previously opened documents";
    OpenPrevDocCommand(MainWidget* w) : GenericPathAndLocationCommadn(cname, w, true) {};

    void handle_generic_requirement() {
        widget->handle_open_prev_doc();
    }

    bool requires_document() { return false; }
};

class OpenAllDocsCommand : public GenericPathAndLocationCommadn {
public:
    static inline const std::string cname = "open_all_docs";
    static inline const std::string hname = "";
    OpenAllDocsCommand(MainWidget* w) : GenericPathAndLocationCommadn(cname, w, true) {};

    void handle_generic_requirement() {
        widget->handle_open_all_docs();
    }

    bool requires_document() { return false; }
};


class OpenDocumentEmbeddedCommand : public GenericPathCommand {
public:
    static inline const std::string cname = "open_document_embedded";
    static inline const std::string hname = "Open an embedded file explorer";
    OpenDocumentEmbeddedCommand(MainWidget* w) : GenericPathCommand(cname, w) {};

    void handle_generic_requirement() {

        widget->set_current_widget(new FileSelector(
            FUZZY_SEARCHING,
            [widget = widget, this](std::wstring doc_path) {
                set_generic_requirement(QString::fromStdWString(doc_path));
                widget->advance_command(std::move(widget->pending_command_instance));
            }, widget, ""));
        widget->show_current_widget();
    }


    void perform() {
        widget->validate_render();
        widget->open_document(selected_path.value());
    }

    bool pushes_state() {
        return true;
    }

    bool requires_document() { return false; }
};

class OpenDocumentEmbeddedFromCurrentPathCommand : public GenericPathCommand {
public:
    static inline const std::string cname = "open_document_embedded_from_current_path";
    static inline const std::string hname = "Open an embedded file explorer, starting in the directory of current document";
    OpenDocumentEmbeddedFromCurrentPathCommand(MainWidget* w) : GenericPathCommand(cname, w) {};

    void handle_generic_requirement() {
        std::wstring last_file_name = widget->get_current_file_name().value_or(L"");

        widget->set_current_widget(new FileSelector(
            FUZZY_SEARCHING,
            [widget = widget, this](std::wstring doc_path) {
                set_generic_requirement(QString::fromStdWString(doc_path));
                widget->advance_command(std::move(widget->pending_command_instance));
            }, widget, QString::fromStdWString(last_file_name)));
        widget->show_current_widget();

    }

    void perform() {
        widget->validate_render();
        widget->open_document(selected_path.value());
    }

    bool pushes_state() {
        return true;
    }

    bool requires_document() { return false; }
};

class CopyCommand : public Command {
public:
    static inline const std::string cname = "copy";
    static inline const std::string hname = "Copy";
    CopyCommand(MainWidget* w) : Command(cname, w) {};
    void perform() {
        copy_to_clipboard(widget->get_selected_text());
    }

};

class GotoBeginningCommand : public Command {
public:
    static inline const std::string cname = "goto_beginning";
    static inline const std::string hname = "Go to the beginning of the document";
    GotoBeginningCommand(MainWidget* w) : Command(cname, w) {};
public:
    void perform() {
        if (num_repeats) {
            widget->main_document_view->goto_page(num_repeats - 1 + widget->main_document_view->get_page_offset());
        }
        else {
            widget->main_document_view->set_offset_y(0.0f);
        }
    }

    bool pushes_state() {
        return true;
    }

};

class GotoEndCommand : public Command {
public:
    static inline const std::string cname = "goto_end";
    static inline const std::string hname = "Go to the end of the document";
    GotoEndCommand(MainWidget* w) : Command(cname, w) {};
public:
    void perform() {
        widget->main_document_view->goto_end();
    }

    bool pushes_state() {
        return true;
    }
};

class OverviewRulerPortalCommand : public Command {
public:
    static inline const std::string cname = "overview_to_ruler_portal";
    static inline const std::string hname = "";
    OverviewRulerPortalCommand(MainWidget* w) : Command(cname, w) {};
public:
    void perform() {
        widget->handle_overview_to_ruler_portal();
    }
};

class GotoRulerPortalCommand : public Command {
public:
    static inline const std::string cname = "goto_ruler_portal";
    static inline const std::string hname = "";
    GotoRulerPortalCommand(MainWidget* w) : Command(cname, w) {};
    std::optional<char> mark = {};

public:

    void perform() {

        std::string mark_str = "";
        if (mark) {
            mark_str = std::string(1, mark.value());
        }

        widget->handle_goto_ruler_portal(mark_str);
        widget->set_should_highlight_words(false);
    }

    void on_cancel() override {
        widget->set_should_highlight_words(false);
    }

    void pre_perform() override {
        if (widget->get_ruler_portals().size() > 1) {
            widget->highlight_ruler_portals();
        }
    }

    void set_symbol_requirement(char value) override {
        mark = value;
    }

    virtual std::optional<Requirement> next_requirement(MainWidget* widget) {
        if (mark) return {};

        if (widget->get_ruler_portals().size() > 1) {
            return Requirement{ RequirementType::Symbol, "Mark" };
        }

        return {};
    }

    bool pushes_state() override {
        return true;
    }

};

class PrintNonDefaultConfigs : public Command {
public:
    static inline const std::string cname = "print_non_default_configs";
    static inline const std::string hname = "";
    PrintNonDefaultConfigs(MainWidget* w) : Command(cname, w) {};
    void perform() {
        widget->print_non_default_configs();
    }
    bool requires_document() { return false; }
};

class PrintUndocumentedCommandsCommand : public Command {
public:
    static inline const std::string cname = "print_undocumented_commands";
    static inline const std::string hname = "";
    PrintUndocumentedCommandsCommand(MainWidget* w) : Command(cname, w) {};
    void perform() {
        widget->print_undocumented_commands();
    }

    bool requires_document() { return false; }
};

class PrintUndocumentedConfigsCommand : public Command {
public:
    static inline const std::string cname = "print_undocumented_configs";
    static inline const std::string hname = "";
    PrintUndocumentedConfigsCommand(MainWidget* w) : Command(cname, w) {};
    void perform() {
        widget->print_undocumented_configs();
    }

    bool requires_document() { return false; }
};

class ToggleFullscreenCommand : public Command {
public:
    static inline const std::string cname = "toggle_fullscreen";
    static inline const std::string hname = "Toggle fullscreen mode";
    ToggleFullscreenCommand(MainWidget* w) : Command(cname, w) {};
    void perform() {
        widget->toggle_fullscreen();
    }

    bool requires_document() { return false; }
};

class MaximizeCommand : public Command {
public:
    static inline const std::string cname = "maximize";
    static inline const std::string hname = "";
    MaximizeCommand(MainWidget* w) : Command(cname, w) {};
    void perform() {
        widget->maximize_window();
    }

    bool requires_document() { return false; }
};

class ToggleOneWindowCommand : public Command {
public:
    static inline const std::string cname = "toggle_one_window";
    static inline const std::string hname = "Open/close helper window";
    ToggleOneWindowCommand(MainWidget* w) : Command(cname, w) {};
    void perform() {
        widget->toggle_two_window_mode();
    }

    bool requires_document() { return false; }
};

class ToggleHighlightCommand : public Command {
public:
    static inline const std::string cname = "toggle_highlight";
    static inline const std::string hname = "Toggle whether PDF links are highlighted";
    ToggleHighlightCommand(MainWidget* w) : Command(cname, w) {};
    void perform() {
        widget->toggle_highlight_links();
    }

    bool requires_document() { return false; }
};

class ToggleSynctexCommand : public Command {
public:
    static inline const std::string cname = "toggle_synctex";
    static inline const std::string hname = "Toggle synctex mode";
    ToggleSynctexCommand(MainWidget* w) : Command(cname, w) {};
    void perform() {
        widget->toggle_synctex_mode();
    }

    bool requires_document() { return false; }
};

class TurnOnSynctexCommand : public Command {
public:
    static inline const std::string cname = "turn_on_synctex";
    static inline const std::string hname = "Turn synxtex mode on";
    TurnOnSynctexCommand(MainWidget* w) : Command(cname, w) {};
    void perform() {
        widget->set_synctex_mode(true);
    }

    bool requires_document() { return false; }
};

class ToggleShowLastCommand : public Command {
public:
    static inline const std::string cname = "toggle_show_last_command";
    static inline const std::string hname = "Toggle whether the last command is shown in statusbar";
    ToggleShowLastCommand(MainWidget* w) : Command(cname, w) {};
    void perform() {
        widget->should_show_last_command = !widget->should_show_last_command;
    }
};


class ForwardSearchCommand : public Command {
public:
    static inline const std::string cname = "synctex_forward_search";
    static inline const std::string hname = "";
    ForwardSearchCommand(MainWidget* w) : Command(cname, w) {};

    std::optional<std::wstring> file_path = {};
    std::optional<int> line = {};
    std::optional<int> column = {};

    std::optional<Requirement> next_requirement(MainWidget* widget) {

        if (!file_path.has_value()) {
            return Requirement { RequirementType::File, "File Path" };
        }
        if (!line.has_value()) {
            return Requirement { RequirementType::Text, "Line number" };
        }
        return {};
    }

    void set_file_requirement(std::wstring value) {
        file_path = value;
    }

    void set_text_requirement(std::wstring text) {
        QStringList parts = QString::fromStdWString(text).split(" ");
        if (parts.size() == 1) {
            line = parts[0].toInt();
        }
        else {
            line = parts[0].toInt();
            column = parts[1].toInt();
        }

    }

    void perform() {
        widget->do_synctex_forward_search(widget->doc()->get_path(), file_path.value(), line.value(), column.value_or(0));
    }

};

class ExternalSearchCommand : public SymbolCommand {
public:
    static inline const std::string cname = "external_search";
    static inline const std::string hname = "Search using external search engines";
    ExternalSearchCommand(MainWidget* w) : SymbolCommand(cname, w) {};
    void perform() {
        std::wstring selected_text = widget->get_selected_text();

        if ((symbol >= 'a') && (symbol <= 'z')) {
            if (SEARCH_URLS[symbol - 'a'].size() > 0) {
                if (selected_text.size() > 0) {
                    search_custom_engine(selected_text, SEARCH_URLS[symbol - 'a']);
                }
                else {
                    search_custom_engine(widget->doc()->detect_paper_name(), SEARCH_URLS[symbol - 'a']);
                }
            }
            else {
                std::wcerr << L"No search engine defined for symbol " << symbol << std::endl;
            }
        }
    }

};

class OpenSelectedUrlCommand : public Command {
public:
    static inline const std::string cname = "open_selected_url";
    static inline const std::string hname = "";
    OpenSelectedUrlCommand(MainWidget* w) : Command(cname, w) {};
    void perform() {
        open_web_url((widget->get_selected_text()).c_str());
    }
};

class ScreenDownCommand : public Command {
public:
    static inline const std::string cname = "screen_down";
    static inline const std::string hname = "Move screen down";
    ScreenDownCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        int rp = num_repeats == 0 ? 1 : num_repeats;
        widget->handle_move_screen(rp);
    }

};

class ScreenUpCommand : public Command {
public:
    static inline const std::string cname = "screen_up";
    static inline const std::string hname = "Move screen up";
    ScreenUpCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        int rp = num_repeats == 0 ? 1 : num_repeats;
        widget->handle_move_screen(-rp);
    }

};

class NextChapterCommand : public Command {
public:
    static inline const std::string cname = "next_chapter";
    static inline const std::string hname = "Go to next chapter";
    NextChapterCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        int rp = num_repeats == 0 ? 1 : num_repeats;
        widget->main_document_view->goto_chapter(rp);
    }

};

class PrevChapterCommand : public Command {
public:
    static inline const std::string cname = "prev_chapter";
    static inline const std::string hname = "Go to previous chapter";
    PrevChapterCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        int rp = num_repeats == 0 ? 1 : num_repeats;
        widget->main_document_view->goto_chapter(-rp);
    }

};

class ShowContextMenuCommand : public Command {
public:
    static inline const std::string cname = "show_context_menu";
    static inline const std::string hname = "";
    ShowContextMenuCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        widget->show_context_menu();
    }

    bool requires_document() { return false; }
};

class ToggleDarkModeCommand : public Command {
public:
    static inline const std::string cname = "toggle_dark_mode";
    static inline const std::string hname = "Toggle dark mode";

    ToggleDarkModeCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        widget->toggle_dark_mode();
    }


    bool requires_document() { return false; }
};

class ToggleCustomColorMode : public Command {
public:
    static inline const std::string cname = "toggle_custom_color";
    static inline const std::string hname = "Toggle custom color mode";
    ToggleCustomColorMode(MainWidget* w) : Command(cname, w) {};

    void perform() {
        widget->toggle_custom_color_mode();
    }

    bool requires_document() { return false; }
};

class TogglePresentationModeCommand : public Command {
public:
    static inline const std::string cname = "toggle_presentation_mode";
    static inline const std::string hname = "Toggle presentation mode";
    TogglePresentationModeCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        widget->toggle_presentation_mode();
    }

    bool requires_document() { return false; }
};

class TurnOnPresentationModeCommand : public Command {
public:
    static inline const std::string cname = "turn_on_presentation_mode";
    static inline const std::string hname = "Turn on presentation mode";
    TurnOnPresentationModeCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        widget->set_presentation_mode(true);
    }

    bool requires_document() { return false; }
};

class ToggleMouseDragMode : public Command {
public:
    static inline const std::string cname = "toggle_mouse_drag_mode";
    static inline const std::string hname = "Toggle mouse drag mode";
    ToggleMouseDragMode(MainWidget* w) : Command(cname, w) {};

    void perform() {
        widget->toggle_mouse_drag_mode();
    }

    bool requires_document() { return false; }
};

class ToggleFreehandDrawingMode : public Command {
public:
    static inline const std::string cname = "toggle_freehand_drawing_mode";
    static inline const std::string hname = "Toggle freehand drawing mode";
    ToggleFreehandDrawingMode(MainWidget* w) : Command(cname, w) {};

    void perform() {
        widget->toggle_freehand_drawing_mode();
    }

    bool requires_document() { return false; }
};

class TogglePenDrawingMode : public Command {
public:
    static inline const std::string cname = "toggle_pen_drawing_mode";
    static inline const std::string hname = "Toggle pen drawing mode";
    TogglePenDrawingMode(MainWidget* w) : Command(cname, w) {};

    void perform() {
        widget->toggle_pen_drawing_mode();
    }

    bool requires_document() { return false; }
};

class ToggleScratchpadMode : public Command {
public:
    static inline const std::string cname = "toggle_scratchpad_mode";
    static inline const std::string hname = "";
    ToggleScratchpadMode(MainWidget* w) : Command(cname, w) {};

    void perform() {
        widget->toggle_scratchpad_mode();
    }

    bool requires_document() { return false; }
};

class CloseWindowCommand : public Command {
public:
    static inline const std::string cname = "close_window";
    static inline const std::string hname = "Close window";
    CloseWindowCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        widget->close();
    }

    bool requires_document() { return false; }
};

class NewWindowCommand : public Command {
public:
    static inline const std::string cname = "new_window";
    static inline const std::string hname = "Open a new window";
    NewWindowCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        int new_id = widget->handle_new_window()->window_id;
        result = QString::number(new_id).toStdWString();
    }

    bool requires_document() { return false; }
};

class QuitCommand : public Command {
public:
    static inline const std::string cname = "quit";
    static inline const std::string hname = "Quit";
    QuitCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        widget->handle_close_event();
        QApplication::quit();
    }


    bool requires_document() { return false; }
};

class EscapeCommand : public Command {
public:
    static inline const std::string cname = "escape";
    static inline const std::string hname = "Escape";
    EscapeCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        widget->handle_escape();
    }

    bool requires_document() { return false; }
};

class TogglePDFAnnotationsCommand : public Command {
public:
    static inline const std::string cname = "toggle_pdf_annotations";
    static inline const std::string hname = "Toggle whether PDF annotations should be rendered";
    TogglePDFAnnotationsCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        widget->toggle_pdf_annotations();
    }

    bool requires_document() { return true; }
};

class OpenLinkCommand : public Command {
public:
    static inline const std::string cname = "open_link";
    static inline const std::string hname = "Go to PDF links using keyboard";
    OpenLinkCommand(MainWidget* w) : Command(cname, w) {};
protected:
    std::optional<std::wstring> text = {};
    bool already_pre_performed = false;
public:

    virtual std::string text_requirement_name() {
        return "Label";
    }

    bool is_done() {
        if ((!ALPHABETIC_LINK_TAGS && text.has_value())) return true;
        if (ALPHABETIC_LINK_TAGS && text.has_value() && (text.value().size() == get_num_tag_digits(widget->num_visible_links()))) return true;
        return false;
    }

    virtual std::optional<Requirement> next_requirement(MainWidget* widget) {
        bool done = is_done();

        if (done) {
            return {};
        }
        else {
            if (ALPHABETIC_LINK_TAGS) {
                return Requirement{ RequirementType::Symbol, "Label" };
            }
            else if ((widget->num_visible_links() < 10) && (!ALPHABETIC_LINK_TAGS)) {
                return Requirement{ RequirementType::Symbol, "Label" };
            }
            else {
                return Requirement{ RequirementType::Text, text_requirement_name() };
            }
        }
    }

    virtual void perform() {
        widget->handle_open_link(text.value());
    }

    void pre_perform() {
        if (already_pre_performed) return;

        widget->clear_tag_prefix();
        widget->set_highlight_links(true, true);
        widget->invalidate_render();
        already_pre_performed = true;
    }

    virtual void set_text_requirement(std::wstring value) {
        this->text = value;
    }

    virtual void set_symbol_requirement(char value) {
        if (text.has_value()) {
            text.value().push_back(value);
        }
        else {
            std::wstring val;
            val.push_back(value);
            this->text = val;
        }

        if (!is_done()) {
            widget->set_tag_prefix(text.value());
        }
    }
};


KeyboardSelectPointCommand::KeyboardSelectPointCommand(MainWidget* w, std::unique_ptr<Command> original_command) : Command("keyboard_point_select", w) {
    origin = std::move(original_command);
    std::optional<Requirement> next_requirement = origin->next_requirement(widget);
    if (next_requirement && next_requirement->type == Rect) {
        requires_rect = true;
    }
    //if(origin->next_requirement(widget) && origin->next_requirement(widget)-> )
};

void KeyboardSelectPointCommand::on_cancel() {
    origin->on_cancel();
    widget->set_highlighted_tags({});

}

bool KeyboardSelectPointCommand::is_done() {
    if (!text.has_value()) return false;

    if (requires_rect) {
        return text.value().size() == 4;
    }
    else {
        return text.value().size() == 2;
    }
}

std::optional<Requirement> KeyboardSelectPointCommand::next_requirement(MainWidget* widget) {
    bool done = is_done();

    if (done) {
        return {};
    }
    else {
        if (requires_rect) {
            if (text.has_value() && text.value().size() >= 2) {
                return Requirement{ RequirementType::Symbol, "bottom right location" };
            }
            else {
                return Requirement{ RequirementType::Symbol, "top left location" };
            }
        }
        else {
            return Requirement{ RequirementType::Symbol, "point location" };
        }
    }
}

void KeyboardSelectPointCommand::perform() {

    widget->clear_tag_prefix();
    widget->set_highlighted_tags({});
    result = text.value();

    if (requires_rect) {

        std::string tag1 = utf8_encode(result.value().substr(0, 2));
        std::string tag2 = utf8_encode(result.value().substr(2, 2));
        int index1 = get_index_from_tag(tag1);
        int index2 = get_index_from_tag(tag2);
        AbsoluteDocumentPos pos1 = widget->get_index_document_pos(index1).to_absolute(widget->doc());
        AbsoluteDocumentPos pos2 = widget->get_index_document_pos(index2).to_absolute(widget->doc());
        AbsoluteRect rect(pos1, pos2);
        origin->set_rect_requirement(rect);
        widget->set_rect_select_mode(false);
        //origin->set_point_requirement(abspos);
    }
    else {

        int index = get_index_from_tag(utf8_encode(result.value()));
        AbsoluteDocumentPos abspos = widget->get_index_document_pos(index).to_absolute(widget->doc());
        origin->set_point_requirement(abspos);
    }

    widget->clear_keyboard_select_highlights();
    widget->advance_command(std::move(origin));
}

void KeyboardSelectPointCommand::pre_perform() {
    if (already_pre_performed) return;

    widget->clear_tag_prefix();
    widget->highlight_window_points();
    widget->invalidate_render();
    already_pre_performed = true;
}

std::string KeyboardSelectPointCommand::get_name() {
    return "keyboard_point_select";
}
 

void KeyboardSelectPointCommand::set_symbol_requirement(char value) {
    if (text.has_value()) {
        text.value().push_back(value);
    }
    else {
        std::wstring val;
        val.push_back(value);
        this->text = val;
    }

    if (!is_done()) {
        if (requires_rect && (text.value().size() >= 2)) {
            widget->set_tag_prefix(text.value().substr(2));
        }
        else {
            widget->set_tag_prefix(text.value());
        }
    }
    // update selected tags
    if (text.has_value()) {
        if (text->size() >= 2) {
            std::string tag = utf8_encode(text.value().substr(0, 2));
            widget->set_highlighted_tags({ tag });
        }
    }
}

class OverviewLinkCommand : public OpenLinkCommand {
public:
    static inline const std::string cname = "overview_link";
    static inline const std::string hname = "Overview to PDF links using keyboard";
    OverviewLinkCommand(MainWidget* w) : OpenLinkCommand(w) {};

    void perform() {
        widget->handle_overview_link(text.value());
    }

    std::string get_name() {
        return cname;
    }
};

class PortalToLinkCommand : public OpenLinkCommand {
public:
    static inline const std::string cname = "portal_to_link";
    static inline const std::string hname = "Create a portal to PDF links using keyboard";
    PortalToLinkCommand(MainWidget* w) : OpenLinkCommand(w) {};

    void perform() {
        widget->handle_portal_to_link(text.value());
    }

    std::string get_name() {
        return cname;
    }
};

class CopyLinkCommand : public TextCommand {
public:
    static inline const std::string cname = "copy_link";
    static inline const std::string hname = "Copy URL of PDF links using keyboard";
    CopyLinkCommand(MainWidget* w) : TextCommand(cname, w) {};

    void perform() {
        widget->handle_open_link(text.value(), true);
    }

    void pre_perform() {
        widget->set_highlight_links(true, true);
        widget->invalidate_render();

    }


    std::string text_requirement_name() {
        return "Label";
    }
};

class KeyboardSelectCommand : public TextCommand {
public:
    static inline const std::string cname = "keyboard_select";
    static inline const std::string hname = "Select text using keyboard";
    KeyboardSelectCommand(MainWidget* w) : TextCommand(cname, w) {};

    void on_text_change(const QString& new_text) override {
        std::vector<std::string> selected_tags;
        QStringList tags = new_text.split(" ");
        for (auto& tag : tags) {
            selected_tags.push_back(tag.toStdString());
        }
        widget->set_highlighted_tags(selected_tags);
    }

    void on_cancel() override {
        widget->set_highlighted_tags({});

    }

    void perform() {
        widget->handle_keyboard_select(text.value());
        widget->set_highlighted_tags({});
    }

    void pre_perform() {
        widget->highlight_words();

    }

    std::string text_requirement_name() {
        return "Labels";
    }
};

class KeyboardOverviewCommand : public TextCommand {
public:
    static inline const std::string cname = "keyboard_overview";
    static inline const std::string hname = "Open an overview using keyboard";
    KeyboardOverviewCommand(MainWidget* w) : TextCommand(cname, w) {};

    void perform() {
        std::optional<fz_irect> rect_ = widget->get_tag_window_rect(utf8_encode(text.value()));
        if (rect_) {
            fz_irect rect = rect_.value();
            widget->overview_under_pos({ (rect.x0 + rect.x1) / 2, (rect.y0 + rect.y1) / 2 });
            widget->set_should_highlight_words(false);
        }
    }

    void pre_perform() {
        widget->highlight_words();

    }

    std::string text_requirement_name() {
        return "Label";
    }
};

class KeyboardSmartjumpCommand : public TextCommand {
public:
    static inline const std::string cname = "keyboard_smart_jump";
    static inline const std::string hname = "Smart jump using keyboard";
    KeyboardSmartjumpCommand(MainWidget* w) : TextCommand(cname, w) {};

    void perform() {
        std::optional<fz_irect> rect_ = widget->get_tag_window_rect(utf8_encode(text.value()));
        if (rect_) {
            fz_irect rect = rect_.value();
            widget->smart_jump_under_pos({ (rect.x0 + rect.x1) / 2, (rect.y0 + rect.y1) / 2 });
            widget->set_should_highlight_words(false);
        }
    }

    void pre_perform() {
        widget->highlight_words();
    }

    bool pushes_state() {
        return true;
    }

    std::string text_requirement_name() {
        return "Label";
    }
};

class KeysCommand : public Command {
public:
    static inline const std::string cname = "keys";
    static inline const std::string hname = "Open the default keys config file";
    KeysCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        open_file(default_keys_path.get_path(), true);
    }

    bool requires_document() { return false; }
};

class KeysUserCommand : public Command {
public:
    static inline const std::string cname = "keys_user";
    static inline const std::string hname = "Open the default keys_user config file";
    KeysUserCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        std::optional<Path> key_file_path = widget->input_handler->get_or_create_user_keys_path();
        if (key_file_path) {
            open_file(key_file_path.value().get_path(), true);
        }
    }

    bool requires_document() { return false; }
};

class KeysUserAllCommand : public Command {
public:
    static inline const std::string cname = "keys_user_all";
    static inline const std::string hname = "List all user keys config files";
    KeysUserAllCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        widget->handle_keys_user_all();
    }

    bool requires_document() { return false; }
};

class PrefsCommand : public Command {
public:
    static inline const std::string cname = "prefs";
    static inline const std::string hname = "Open the default prefs config file";
    PrefsCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        open_file(default_config_path.get_path(), true);
    }

    bool requires_document() { return false; }
};

class PrefsUserCommand : public Command {
public:
    static inline const std::string cname = "prefs_user";
    static inline const std::string hname = "Open the default prefs_user config file";
    PrefsUserCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        std::optional<Path> pref_file_path = widget->config_manager->get_or_create_user_config_file();
        if (pref_file_path) {
            open_file(pref_file_path.value().get_path(), true);
        }
    }

    bool requires_document() { return false; }
};

class PrefsUserAllCommand : public Command {
public:
    static inline const std::string cname = "prefs_user_all";
    static inline const std::string hname = "List all user keys config files";
    PrefsUserAllCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        widget->handle_prefs_user_all();
    }

    bool requires_document() { return false; }
};

class FitToPageWidthRatioCommand : public Command {
public:
    static inline const std::string cname = "fit_to_page_width_ratio";
    static inline const std::string hname = "Fit page to a percentage of window width";
    FitToPageWidthRatioCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        widget->main_document_view->fit_to_page_width(false, true);
        widget->last_smart_fit_page = {};
    }

};

class SmartJumpUnderCursorCommand : public Command {
public:
    static inline const std::string cname = "smart_jump_under_cursor";
    static inline const std::string hname = "Perform a smart jump to the reference under cursor";
    SmartJumpUnderCursorCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        QPoint mouse_pos = widget->mapFromGlobal(widget->cursor_pos());
        widget->smart_jump_under_pos({ mouse_pos.x(), mouse_pos.y() });
    }
};

class DownloadPaperUnderCursorCommand : public Command {
public:
    static inline const std::string cname = "download_paper_under_cursor";
    static inline const std::string hname = "Try to download the paper name under cursor";
    DownloadPaperUnderCursorCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        widget->download_paper_under_cursor();
    }

};


class OverviewUnderCursorCommand : public Command {
public:
    static inline const std::string cname = "overview_under_cursor";
    static inline const std::string hname = "Open an overview to the reference under cursor";
    OverviewUnderCursorCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        QPoint mouse_pos = widget->mapFromGlobal(widget->cursor_pos());
        widget->overview_under_pos({ mouse_pos.x(), mouse_pos.y() });
    }

};

class SynctexUnderCursorCommand : public Command {
public:
    static inline const std::string cname = "synctex_under_cursor";
    static inline const std::string hname = "Perform a synctex search to the tex file location corresponding to cursor location";
    SynctexUnderCursorCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        QPoint mouse_pos = widget->mapFromGlobal(widget->cursor_pos());
        widget->synctex_under_pos({ mouse_pos.x(), mouse_pos.y() });
    }

};

class SynctexUnderRulerCommand : public Command {
public:
    static inline const std::string cname = "synctex_under_ruler";
    static inline const std::string hname = "";
    SynctexUnderRulerCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        result = widget->handle_synctex_to_ruler();
    }

};

class VisualMarkUnderCursorCommand : public Command {
public:
    static inline const std::string cname = "visual_mark_under_cursor";
    static inline const std::string hname = "Highlight the line under cursor";
    VisualMarkUnderCursorCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        QPoint mouse_pos = widget->mapFromGlobal(widget->cursor_pos());
        widget->visual_mark_under_pos({ mouse_pos.x(), mouse_pos.y() });
    }

};

class CloseOverviewCommand : public Command {
public:
    static inline const std::string cname = "close_overview";
    static inline const std::string hname = "Close overview window";
    CloseOverviewCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        widget->set_overview_page({});
    }

};

class CloseVisualMarkCommand : public Command {
public:
    static inline const std::string cname = "close_visual_mark";
    static inline const std::string hname = "Stop ruler mode";
    CloseVisualMarkCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        widget->main_document_view->exit_ruler_mode();
    }

};

class ZoomInCursorCommand : public Command {
public:
    static inline const std::string cname = "zoom_in_cursor";
    static inline const std::string hname = "Zoom in centered on mouse cursor";
    ZoomInCursorCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        QPoint mouse_pos = widget->mapFromGlobal(widget->cursor_pos());
        widget->main_document_view->zoom_in_cursor({ mouse_pos.x(), mouse_pos.y() });
        widget->last_smart_fit_page = {};
    }

};

class ZoomOutCursorCommand : public Command {
public:
    static inline const std::string cname = "zoom_out_cursor";
    static inline const std::string hname = "Zoom out centered on mouse cursor";
    ZoomOutCursorCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        QPoint mouse_pos = widget->mapFromGlobal(widget->cursor_pos());
        widget->main_document_view->zoom_out_cursor({ mouse_pos.x(), mouse_pos.y() });
        widget->last_smart_fit_page = {};
    }

};

class GotoLeftCommand : public Command {
public:
    static inline const std::string cname = "goto_left";
    static inline const std::string hname = "Go to far left side of the page";
    GotoLeftCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        widget->main_document_view->goto_left();
    }

};

class GotoLeftSmartCommand : public Command {
public:
    static inline const std::string cname = "goto_left_smart";
    static inline const std::string hname = "Go to far left side of the page, ignoring white page margins";
    GotoLeftSmartCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        widget->main_document_view->goto_left_smart();
    }

};

class GotoRightCommand : public Command {
public:
    static inline const std::string cname = "goto_right";
    static inline const std::string hname = "Go to far right side of the page";
    GotoRightCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        widget->main_document_view->goto_right();
    }

};

class GotoRightSmartCommand : public Command {
public:
    static inline const std::string cname = "goto_right_smart";
    static inline const std::string hname = "Go to far right side of the page, ignoring white page margins";
    GotoRightSmartCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        widget->main_document_view->goto_right_smart();
    }

};

class RotateClockwiseCommand : public Command {
public:
    static inline const std::string cname = "rotate_clockwise";
    static inline const std::string hname = "Rotate clockwise";
    RotateClockwiseCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        widget->main_document_view->rotate();
        widget->rotate_clockwise();
    }

};

class RotateCounterClockwiseCommand : public Command {
public:
    static inline const std::string cname = "rotate_counterclockwise";
    static inline const std::string hname = "Rotate counter clockwise";
    RotateCounterClockwiseCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        widget->main_document_view->rotate();
        widget->rotate_counterclockwise();
    }

};

class GotoNextHighlightCommand : public Command {
public:
    static inline const std::string cname = "goto_next_highlight";
    static inline const std::string hname = "Go to the next highlight";
    GotoNextHighlightCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        auto next_highlight = widget->main_document_view->get_document()->get_next_highlight(widget->main_document_view->get_offset_y());
        if (next_highlight.has_value()) {
            widget->long_jump_to_destination(next_highlight.value().selection_begin.y);
        }
    }
};

class GotoPrevHighlightCommand : public Command {
public:
    static inline const std::string cname = "goto_prev_highlight";
    static inline const std::string hname = "Go to the previous highlight";
    GotoPrevHighlightCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {

        auto prev_highlight = widget->main_document_view->get_document()->get_prev_highlight(widget->main_document_view->get_offset_y());
        if (prev_highlight.has_value()) {
            widget->long_jump_to_destination(prev_highlight.value().selection_begin.y);
        }
    }
};

class GotoNextHighlightOfTypeCommand : public Command {
public:
    static inline const std::string cname = "goto_next_highlight_of_type";
    static inline const std::string hname = "Go to the next highlight with the current highlight type";
    GotoNextHighlightOfTypeCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        auto next_highlight = widget->main_document_view->get_document()->get_next_highlight(widget->main_document_view->get_offset_y(), widget->select_highlight_type);
        if (next_highlight.has_value()) {
            widget->long_jump_to_destination(next_highlight.value().selection_begin.y);
        }
    }

};

class GotoPrevHighlightOfTypeCommand : public Command {
public:
    static inline const std::string cname = "goto_prev_highlight_of_type";
    static inline const std::string hname = "Go to the previous highlight with the current highlight type";
    GotoPrevHighlightOfTypeCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        auto prev_highlight = widget->main_document_view->get_document()->get_prev_highlight(widget->main_document_view->get_offset_y(), widget->select_highlight_type);
        if (prev_highlight.has_value()) {
            widget->long_jump_to_destination(prev_highlight.value().selection_begin.y);
        }
    }
};

class SetSelectHighlightTypeCommand : public SymbolCommand {
public:
    static inline const std::string cname = "set_select_highlight_type";
    static inline const std::string hname = "Set the selected highlight type";
    SetSelectHighlightTypeCommand(MainWidget* w) : SymbolCommand(cname, w) {};
    void perform() {
        widget->select_highlight_type = symbol;
    }

    bool requires_document() { return false; }
};

class SetFreehandType : public SymbolCommand {
public:
    static inline const std::string cname = "set_freehand_type";
    static inline const std::string hname = "Set the freehand drawing color type";
    SetFreehandType(MainWidget* w) : SymbolCommand(cname, w) {};
    void perform() {
        widget->current_freehand_type = symbol;
    }

    bool requires_document() { return false; }
};

class AddHighlightWithCurrentTypeCommand : public Command {
public:
    static inline const std::string cname = "add_highlight_with_current_type";
    static inline const std::string hname = "Highlight selected text with current selected highlight type";
    AddHighlightWithCurrentTypeCommand(MainWidget* w) : Command(cname, w) {};
    void perform() {
        if (widget->main_document_view->selected_character_rects.size() > 0) {
            widget->main_document_view->add_highlight(widget->selection_begin, widget->selection_end, widget->select_highlight_type);
            widget->clear_selected_text();
        }
    }

};

class UndoDrawingCommand : public Command {
public:
    static inline const std::string cname = "undo_drawing";
    static inline const std::string hname = "Undo freehand drawing";
    UndoDrawingCommand(MainWidget* w) : Command(cname, w) {};
    void perform() {
        widget->handle_undo_drawing();
    }

    bool requires_document() { return true; }
};


class EnterPasswordCommand : public TextCommand {
public:
    static inline const std::string cname = "enter_password";
    static inline const std::string hname = "Enter password";
    EnterPasswordCommand(MainWidget* w) : TextCommand(cname, w) {};
    void perform() {
        std::string password = utf8_encode(text.value());
        widget->add_password(widget->main_document_view->get_document()->get_path(), password);
    }

    std::string text_requirement_name() {
        return "Password";
    }
};

class ToggleFastreadCommand : public Command {
public:
    static inline const std::string cname = "toggle_fastread";
    static inline const std::string hname = "Go to top of current page";
    ToggleFastreadCommand(MainWidget* w) : Command(cname, w) {};
    void perform() {
        widget->toggle_fastread();
    }
};

class GotoTopOfPageCommand : public Command {
public:
    static inline const std::string cname = "goto_top_of_page";
    static inline const std::string hname = "Go to top of current page";
    GotoTopOfPageCommand(MainWidget* w) : Command(cname, w) {};
    void perform() {
        widget->main_document_view->goto_top_of_page();
    }

};

class GotoBottomOfPageCommand : public Command {
public:
    static inline const std::string cname = "goto_bottom_of_page";
    static inline const std::string hname = "Go to bottom of current page";
    GotoBottomOfPageCommand(MainWidget* w) : Command(cname, w) {};
    void perform() {
        widget->main_document_view->goto_bottom_of_page();
    }

};

class ReloadCommand : public Command {
public:
    static inline const std::string cname = "reload";
    static inline const std::string hname = "Reload document";
    ReloadCommand(MainWidget* w) : Command(cname, w) {};
    void perform() {
        widget->reload();
    }

};

class ReloadNoFlickerCommand : public Command {
public:
    static inline const std::string cname = "reload_no_flicker";
    static inline const std::string hname = "Reload document with no screen flickering";
    ReloadNoFlickerCommand(MainWidget* w) : Command(cname, w) {};
    void perform() {
        widget->reload(false);
    }

};

class ReloadConfigCommand : public Command {
public:
    static inline const std::string cname = "reload_config";
    static inline const std::string hname = "Reload configs";
    ReloadConfigCommand(MainWidget* w) : Command(cname, w) {};
    void perform() {
        widget->on_config_file_changed(widget->config_manager);
    }

    bool requires_document() { return false; }
};

class TurnOnAllDrawings : public Command {
public:
    static inline const std::string cname = "turn_on_all_drawings";
    static inline const std::string hname = "Make all freehand drawings visible";
    TurnOnAllDrawings(MainWidget* w) : Command(cname, w) {};
    void perform() {
        widget->hande_turn_on_all_drawings();
    }

    bool requires_document() { return false; }
};

class TurnOffAllDrawings : public Command {
public:
    static inline const std::string cname = "turn_off_all_drawings";
    static inline const std::string hname = "Make all freehand drawings invisible";
    TurnOffAllDrawings(MainWidget* w) : Command(cname, w) {};
    void perform() {
        widget->hande_turn_off_all_drawings();
    }

    bool requires_document() { return false; }
};

class SetStatusStringCommand : public TextCommand {
public:
    static inline const std::string cname = "set_status_string";
    static inline const std::string hname = "Set custom message to be shown in statusbar";
    SetStatusStringCommand(MainWidget* w) : TextCommand(cname, w) {};

    void perform() {
        widget->set_status_message(text.value());
    }

    std::string text_requirement_name() {
        return "Status String";
    }


    bool requires_document() { return false; }
};

class ClearStatusStringCommand : public Command {
public:
    static inline const std::string cname = "clear_status_string";
    static inline const std::string hname = "Clear custom statusbar message";
    ClearStatusStringCommand(MainWidget* w) : Command(cname, w) {};
    void perform() {
        widget->set_status_message(L"");
    }

    bool requires_document() { return false; }
};

class ToggleTittlebarCommand : public Command {
public:
    static inline const std::string cname = "toggle_titlebar";
    static inline const std::string hname = "Toggle window titlebar";
    ToggleTittlebarCommand(MainWidget* w) : Command(cname, w) {};
    void perform() {
        widget->toggle_titlebar();
    }

    bool requires_document() { return false; }
};

class NextPreviewCommand : public Command {
public:
    static inline const std::string cname = "next_overview";
    static inline const std::string hname = "Go to the next candidate in overview window";
    NextPreviewCommand(MainWidget* w) : Command(cname, w) {};
    void perform() {
        if (widget->smart_view_candidates.size() > 0) {
            //widget->index_into_candidates = (widget->index_into_candidates + 1) % widget->smart_view_candidates.size();
            //widget->set_overview_position(widget->smart_view_candidates[widget->index_into_candidates].page, widget->smart_view_candidates[widget->index_into_candidates].y);
            widget->goto_ith_next_overview(1);
        }
    }

};

class PreviousPreviewCommand : public Command {
public:
    static inline const std::string cname = "previous_overview";
    static inline const std::string hname = "Go to the previous candidate in overview window";
    PreviousPreviewCommand(MainWidget* w) : Command(cname, w) {};
    void perform() {
        if (widget->smart_view_candidates.size() > 0) {
            //widget->index_into_candidates = mod(widget->index_into_candidates - 1, widget->smart_view_candidates.size());
            //widget->set_overview_position(widget->smart_view_candidates[widget->index_into_candidates].page, widget->smart_view_candidates[widget->index_into_candidates].y);
            widget->goto_ith_next_overview(-1);
        }
    }
};

class GotoOverviewCommand : public Command {
public:
    static inline const std::string cname = "goto_overview";
    static inline const std::string hname = "Go to the current overview location";
    GotoOverviewCommand(MainWidget* w) : Command(cname, w) {};
    void perform() {
        widget->goto_overview();
    }

};

class PortalToOverviewCommand : public Command {
public:
    static inline const std::string cname = "portal_to_overview";
    static inline const std::string hname = "Create a portal to the current overview location";
    PortalToOverviewCommand(MainWidget* w) : Command(cname, w) {};
    void perform() {
        widget->handle_portal_to_overview();
    }

};

class GotoSelectedTextCommand : public Command {
public:
    static inline const std::string cname = "goto_selected_text";
    static inline const std::string hname = "Go to the location of current selected text";
    GotoSelectedTextCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        widget->long_jump_to_destination(widget->selection_begin.y);
    }

};

class SetWindowRectCommand : public TextCommand {
public:
    static inline const std::string cname = "set_window_rect";
    static inline const std::string hname = "Move and resize the window to the given coordinates (in pixels).";
    SetWindowRectCommand(MainWidget* w) : TextCommand(cname, w) {};

    void perform() {
        QStringList parts = QString::fromStdWString(text.value()).split(' ');
        int x0 = parts[0].toInt();
        int y0 = parts[1].toInt();
        int w = parts[2].toInt();
        int h = parts[3].toInt();
        widget->resize(w, h);
        widget->move(x0, y0);
    }

};

class FocusTextCommand : public TextCommand {
public:
    static inline const std::string cname = "focus_text";
    static inline const std::string hname = "Focus on the given text";
    FocusTextCommand(MainWidget* w) : TextCommand(cname, w) {};

    void perform() {
        std::wstring text_ = text.value();
        widget->handle_focus_text(text_);
    }

    std::string text_requirement_name() {
        return "Text to focus";
    }
};

class DownloadOverviewPaperCommand : public TextCommand {
public:
    static inline const std::string cname = "download_overview_paper";
    static inline const std::string hname = "";
    std::optional<AbsoluteRect> source_rect = {};
    std::wstring src_doc_path;

    DownloadOverviewPaperCommand(MainWidget* w) : TextCommand(cname, w) {};

    void perform() {

        std::wstring text_ = text.value();

        widget->download_paper_with_name(text_, widget->get_default_paper_download_finish_action());

        if (source_rect) {
            widget->download_and_portal(text_, source_rect->center());
        }


    }

    void pre_perform() {
        std::optional<std::wstring> paper_name = widget->get_overview_paper_name();
        src_doc_path = widget->doc()->get_path();

        if (paper_name) {
            source_rect = widget->get_overview_source_rect();

            if (TOUCH_MODE) {
                TouchTextEdit* paper_name_editor = dynamic_cast<TouchTextEdit*>(widget->current_widget_stack.back());
                if (paper_name_editor) {
                    paper_name_editor->set_text(paper_name.value());
                    widget->close_overview();
                }
                //widget->close_overview();
            }
            else {
                widget->text_command_line_edit->setText(
                    QString::fromStdWString(paper_name.value())
                );
                widget->close_overview();
            }
        }
    }

    std::string text_requirement_name() {
        return "Paper Name";
    }
};

class GotoWindowCommand : public Command {
public:
    static inline const std::string cname = "goto_window";
    static inline const std::string hname = "Open a list of all sioyek windows";
    GotoWindowCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        widget->handle_goto_window();
    }

    bool requires_document() { return false; }
};

class ToggleSmoothScrollModeCommand : public Command {
public:
    static inline const std::string cname = "toggle_smooth_scroll_mode";
    static inline const std::string hname = "Toggle smooth scroll mode";
    ToggleSmoothScrollModeCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        widget->handle_toggle_smooth_scroll_mode();
    }

    bool requires_document() { return false; }
};

class ToggleScrollbarCommand : public Command {
public:
    static inline const std::string cname = "toggle_scrollbar";
    static inline const std::string hname = "Toggle scrollbar";
    ToggleScrollbarCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        widget->toggle_scrollbar();
    }

    bool requires_document() { return false; }

};

class OverviewToPortalCommand : public Command {
public:
    static inline const std::string cname = "overview_to_portal";
    static inline const std::string hname = "Open an overview to the closest portal";
    OverviewToPortalCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        widget->handle_overview_to_portal();
    }

};

class ScanNewFilesFromScanDirCommand : public Command {
public:
    static inline const std::string cname = "scan_new_files";
    static inline const std::string hname = "";
    ScanNewFilesFromScanDirCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        widget->scan_new_files_from_scan_directory();
    }

};

class DebugCommand : public Command {
public:
    inline static const std::string cname = "debug";
    inline static const std::string hname = "[internal]";
    static inline const bool developer_only = true;

    DebugCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        widget->handle_debug_command();
    }

};

class ShowTouchMainMenu : public Command {
public:
    static inline const std::string cname = "show_touch_main_menu";
    static inline const std::string hname = "";
    ShowTouchMainMenu(MainWidget* w) : Command(cname, w) {};

    void perform() {
        widget->show_touch_main_menu();
    }

};

class ShowTouchDrawingMenu : public Command {
public:
    static inline const std::string cname = "show_touch_draw_controls";
    static inline const std::string hname = "";
    ShowTouchDrawingMenu(MainWidget* w) : Command(cname, w) {};

    void perform() {
        widget->show_draw_controls();
    }

};

class ShowTouchSettingsMenu : public Command {
public:
    static inline const std::string cname = "show_touch_settings_menu";
    static inline const std::string hname = "";
    ShowTouchSettingsMenu(MainWidget* w) : Command(cname, w) {};

    void perform() {
        widget->show_touch_settings_menu();
    }

};


class ExportPythonApiCommand : public Command {
public:
    static inline const std::string cname = "export_python_api";
    static inline const std::string hname = "";
    ExportPythonApiCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        widget->export_python_api();
    }

};

class SelectCurrentSearchMatchCommand : public Command {
public:
    static inline const std::string cname = "select_current_search_match";
    static inline const std::string hname = "";
    SelectCurrentSearchMatchCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        widget->handle_select_current_search_match();
    }

};

class SelectRectCommand : public Command {
public:
    static inline const std::string cname = "select_rect";
    static inline const std::string hname = "Select a rectangle to be used in other commands";
    SelectRectCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        widget->set_rect_select_mode(true);
    }

};

class ToggleTypingModeCommand : public Command {
public:
    static inline const std::string cname = "toggle_typing_mode";
    static inline const std::string hname = "Toggle typing minigame";
    ToggleTypingModeCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        widget->handle_toggle_typing_mode();
    }

};

class DonateCommand : public Command {
public:
    static inline const std::string cname = "donate";
    static inline const std::string hname = "Donate to support sioyek's development";
    DonateCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        open_web_url(L"https://www.buymeacoffee.com/ahrm");
    }

    bool requires_document() { return false; }
};

class OverviewNextItemCommand : public Command {
public:
    static inline const std::string cname = "overview_next_item";
    static inline const std::string hname = "Open an overview to the next search result";
    OverviewNextItemCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        if (num_repeats == 0) num_repeats++;
        widget->goto_search_result(num_repeats, true);
    }

};

class OverviewPrevItemCommand : public Command {
public:
    static inline const std::string cname = "overview_prev_item";
    static inline const std::string hname = "Open an overview to the previous search result";
    OverviewPrevItemCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        if (num_repeats == 0) num_repeats++;
        widget->goto_search_result(-num_repeats, true);
    }

};

class DeleteHighlightUnderCursorCommand : public Command {
public:
    static inline const std::string cname = "delete_highlight_under_cursor";
    static inline const std::string hname = "Delete highlight under mouse cursor";
    DeleteHighlightUnderCursorCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        widget->handle_delete_highlight_under_cursor();
    }

};

class NoopCommand : public Command {
public:
    static inline const std::string cname = "noop";
    static inline const std::string hname = "Do nothing";

    NoopCommand(MainWidget* widget_) : Command(cname, widget_) {}

    void perform() {
    }

    bool requires_document() { return false; }
};

class ImportCommand : public Command {
public:
    static inline const std::string cname = "import";
    static inline const std::string hname = "Import annotation data from a json file";
    ImportCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        std::wstring import_file_name = select_json_file_name();
        widget->import_json(import_file_name);
    }

    bool requires_document() { return false; }
};

class ExportCommand : public Command {
public:
    static inline const std::string cname = "export";
    static inline const std::string hname = "Export annotation data to a json file";
    ExportCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        std::wstring export_file_name = select_new_json_file_name();
        widget->export_json(export_file_name);
    }

    bool requires_document() { return false; }
};

class WriteAnnotationsFileCommand : public Command {
public:
    static inline const std::string cname = "write_annotations_file";
    static inline const std::string hname = "";
    WriteAnnotationsFileCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        widget->doc()->persist_annotations(true);
    }

};

class LoadAnnotationsFileCommand : public Command {
public:
    static inline const std::string cname = "load_annotations_file";
    static inline const std::string hname = "";
    LoadAnnotationsFileCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        widget->doc()->load_annotations();
    }

};

class LoadAnnotationsFileSyncDeletedCommand : public Command {
public:
    static inline const std::string cname = "import_annotations_file_sync_deleted";
    static inline const std::string hname = "";
    LoadAnnotationsFileSyncDeletedCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        widget->doc()->load_annotations(true);
    }

};

class EnterVisualMarkModeCommand : public Command {
public:
    static inline const std::string cname = "enter_visual_mark_mode";
    static inline const std::string hname = "Enter ruler mode using keyboard";
    EnterVisualMarkModeCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        widget->visual_mark_under_pos({ widget->width() / 2, widget->height() / 2 });
    }

};

class SetPageOffsetCommand : public TextCommand {
public:
    static inline const std::string cname = "set_page_offset";
    static inline const std::string hname = "Toggle visual scroll mode";
    SetPageOffsetCommand(MainWidget* w) : TextCommand(cname, w) {};

    void perform() {
        if (is_string_numeric(text.value().c_str()) && text.value().size() < 6) { // make sure the page number is valid
            widget->main_document_view->set_page_offset(std::stoi(text.value().c_str()));
        }
    }
};

class ToggleVisualScrollCommand : public Command {
public:
    static inline const std::string cname = "toggle_visual_scroll";
    static inline const std::string hname = "Toggle visual scroll mode";
    ToggleVisualScrollCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        widget->toggle_visual_scroll_mode();
    }
};


class ToggleHorizontalLockCommand : public Command {
public:
    static inline const std::string cname = "toggle_horizontal_scroll_lock";
    static inline const std::string hname = "Toggle horizontal lock";
    ToggleHorizontalLockCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        widget->horizontal_scroll_locked = !widget->horizontal_scroll_locked;
    }

};

class ExecuteCommand : public TextCommand {
public:
    static inline const std::string cname = "execute";
    static inline const std::string hname = "Execute shell command";
    ExecuteCommand(MainWidget* w) : TextCommand(cname, w) {};

    void perform() {
        widget->execute_command(text.value());
    }

    bool requires_document() { return false; }
};

class ImportAnnotationsCommand : public Command {
public:
    static inline const std::string cname = "import_annotations";
    static inline const std::string hname = "Import PDF annotations into sioyek";
    ImportAnnotationsCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        widget->doc()->import_annotations();
    }

};

class EmbedAnnotationsCommand : public Command {
public:
    static inline const std::string cname = "embed_annotations";
    static inline const std::string hname = "";
    EmbedAnnotationsCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        std::wstring embedded_pdf_file_name = select_new_pdf_file_name();
        if (embedded_pdf_file_name.size() > 0) {
            widget->main_document_view->get_document()->embed_annotations(embedded_pdf_file_name);
        }
    }
};

class CopyWindowSizeConfigCommand : public Command {
public:
    static inline const std::string cname = "copy_window_size_config";
    static inline const std::string hname = "Copy current window size configuration";
    CopyWindowSizeConfigCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        copy_to_clipboard(widget->get_window_configuration_string());
    }

    bool requires_document() { return false; }
};

class ToggleSelectHighlightCommand : public Command {
public:
    static inline const std::string cname = "toggle_select_highlight";
    static inline const std::string hname = "Toggle select highlight mode";
    ToggleSelectHighlightCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        widget->is_select_highlight_mode = !widget->is_select_highlight_mode;
    }

};

class OpenLastDocumentCommand : public Command {
public:
    static inline const std::string cname = "open_last_document";
    static inline const std::string hname = "Switch to previous opened document";
    OpenLastDocumentCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        auto last_opened_file = widget->get_last_opened_file_checksum();
        if (last_opened_file) {
            widget->open_document_with_hash(last_opened_file.value());
        }
    }


    bool requires_document() { return false; }
};

class AddMarkedDataCommand : public Command {
public:
    static inline const std::string cname = "add_marked_data";
    static inline const std::string hname = "";
    AddMarkedDataCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        widget->handle_add_marked_data();
    }

    bool requires_document() { return true; }
};

class UndoMarkedDataCommand : public Command {
public:
    static inline const std::string cname = "undo_marked_data";
    static inline const std::string hname = "[internal]";
    UndoMarkedDataCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        widget->handle_undo_marked_data();
    }

    bool requires_document() { return true; }
};

class GotoRandomPageCommand : public Command {
public:
    static inline const std::string cname = "goto_random_page";
    static inline const std::string hname = "[internal]";
    GotoRandomPageCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        widget->handle_goto_random_page();
    }

    bool requires_document() { return true; }
};

class RemoveMarkedDataCommand : public Command {
public:
    static inline const std::string cname = "remove_marked_data";
    static inline const std::string hname = "[internal]";
    RemoveMarkedDataCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        widget->handle_remove_marked_data();
    }

    bool requires_document() { return true; }
};

class ExportMarkedDataCommand : public Command {
public:
    static inline const std::string cname = "export_marked_data";
    static inline const std::string hname = "[internal]";
    ExportMarkedDataCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        widget->handle_export_marked_data();
    }

    bool requires_document() { return true; }
};

class ToggleStatusbarCommand : public Command {
public:
    static inline const std::string cname = "toggle_statusbar";
    static inline const std::string hname = "Toggle statusbar";
    ToggleStatusbarCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        widget->toggle_statusbar();
    }

    bool requires_document() { return false; }
};

class LazyCommand : public Command {
private:
    CommandManager* command_manager;
    std::string command_name;
    //std::wstring command_params;
    std::vector<std::wstring> command_params;
    std::unique_ptr<Command> actual_command = nullptr;
    NoopCommand noop;

    //void parse_command_text(std::wstring command_text) {
    //    int index = command_text.find(L"(");
    //    if (index != -1) {
    //        command_name = utf8_encode(command_text.substr(0, index));
    //        command_params = command_text.substr(index + 1, command_text.size() - index - 2);
    //    }
    //    else {
    //        command_name = utf8_encode(command_text);
    //    }
    //}

    std::optional<std::wstring> get_result() override{
        if (actual_command) {
            return actual_command->get_result();
        }
        return {};
    }

    void set_result_socket(QLocalSocket* socket) {
        result_socket = socket;
        if (actual_command) {
            actual_command->set_result_socket(socket);
        }
    }

    void set_result_mutex(bool* p_is_done, std::wstring* result_location) {
        result_holder = result_location;
        is_done = p_is_done;
        if (actual_command) {
            actual_command->set_result_mutex(p_is_done, result_location);
        }
    }

    Command* get_command() {
        if (actual_command) {
            return actual_command.get();
        }
        else {
            actual_command = std::move(command_manager->get_command_with_name(widget, command_name));
            if (!actual_command) return &noop;

            for (auto arg : command_params) {
                actual_command->set_next_requirement_with_string(arg);
            }

            if (actual_command && (result_socket != nullptr)) {
                actual_command->set_result_socket(result_socket);
            }

            if (actual_command && (is_done != nullptr)) {
                actual_command->set_result_mutex(is_done, result_holder);
            }

            return actual_command.get();
        }
    }


public:
    LazyCommand(std::string name, MainWidget* widget_, CommandManager* manager, CommandInvocation invocation) : Command(name, widget_), noop(widget_) {
        command_manager = manager;
        command_name = invocation.command_name.toStdString();
        for (auto arg : invocation.command_args) {
            command_params.push_back(arg.toStdWString());
        }

        //parse_command_text(command_text);
    }

    void set_text_requirement(std::wstring value) { get_command()->set_text_requirement(value); }
    void set_symbol_requirement(char value) { get_command()->set_symbol_requirement(value); }
    void set_file_requirement(std::wstring value) { get_command()->set_file_requirement(value); }
    void set_rect_requirement(AbsoluteRect value) { get_command()->set_rect_requirement(value); }
    void set_generic_requirement(QVariant value) { get_command()->set_generic_requirement(value); }
    void handle_generic_requirement() { get_command()->handle_generic_requirement(); }
    void set_point_requirement(AbsoluteDocumentPos value) { get_command()->set_point_requirement(value); }
    void set_num_repeats(int nr) { get_command()->set_num_repeats(nr); }
    std::vector<char> special_symbols() { return get_command()->special_symbols(); }
    void pre_perform() { get_command()->pre_perform(); }
    bool pushes_state() { return get_command()->pushes_state(); }
    bool requires_document() { return get_command()->requires_document(); }
    std::optional<Requirement> next_requirement(MainWidget* widget) {
        return get_command()->next_requirement(widget);
    }

    virtual void perform() {
        auto com = get_command();
        if (com) {
            com->run();
        }
    }

    std::string get_name() {
        auto com = get_command();
        if (com) {
            return com->get_name();
        }
        else {
            return "";
        }
    }


};


class ClearCurrentPageDrawingsCommand : public Command {
public:
    static inline const std::string cname = "clear_current_page_drawings";
    static inline const std::string hname = "";
    ClearCurrentPageDrawingsCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        widget->clear_current_page_drawings();
    }


    bool requires_document() { return false; }
};

class ClearCurrentDocumentDrawingsCommand : public Command {
public:
    static inline const std::string cname = "clear_current_document_drawings";
    static inline const std::string hname = "";
    ClearCurrentDocumentDrawingsCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        widget->clear_current_document_drawings();
    }


    bool requires_document() { return false; }
};

class DeleteFreehandDrawingsCommand : public Command {
public:
    static inline const std::string cname = "delete_freehand_drawings";
    static inline const std::string hname = "Add a text bookmark in the selected rectangle";
    DeleteFreehandDrawingsCommand(MainWidget* w) : Command(cname, w) {};

    std::optional<AbsoluteRect> rect_;
    DrawingMode original_drawing_mode = DrawingMode::None;


    void pre_perform() {
        original_drawing_mode = widget->freehand_drawing_mode;
        widget->freehand_drawing_mode = DrawingMode::None;
    }

    std::optional<Requirement> next_requirement(MainWidget* widget) {

        if (!rect_.has_value()) {
            Requirement req = { RequirementType::Rect, "Command Rect" };
            return req;
        }
        return {};
    }

    void set_rect_requirement(AbsoluteRect rect) {
        if (rect.x0 > rect.x1) {
            std::swap(rect.x0, rect.x1);
        }
        if (rect.y0 > rect.y1) {
            std::swap(rect.y0, rect.y1);
        }

        rect_ = rect;
    }

    void perform() {
        widget->delete_freehand_drawings(rect_.value());
        widget->freehand_drawing_mode = original_drawing_mode;
    }
};

class SelectFreehandDrawingsCommand : public Command {
public:
    static inline const std::string cname = "select_freehand_drawings";
    static inline const std::string hname = "Select freehand drawings";
    SelectFreehandDrawingsCommand(MainWidget* w) : Command(cname, w) {};

    std::optional<AbsoluteRect> rect_;
    DrawingMode original_drawing_mode = DrawingMode::None;


    void pre_perform() {
        original_drawing_mode = widget->freehand_drawing_mode;
        widget->freehand_drawing_mode = DrawingMode::None;
    }

    std::optional<Requirement> next_requirement(MainWidget* widget) {

        if (!rect_.has_value()) {
            Requirement req = { RequirementType::Rect, "Command Rect" };
            return req;
        }
        return {};
    }

    void set_rect_requirement(AbsoluteRect rect) {
        if (rect.x0 > rect.x1) {
            std::swap(rect.x0, rect.x1);
        }
        if (rect.y0 > rect.y1) {
            std::swap(rect.y0, rect.y1);
        }

        rect_ = rect;
    }

    void perform() {
        widget->select_freehand_drawings(rect_.value());
        widget->freehand_drawing_mode = original_drawing_mode;
    }
};

class CustomCommand : public Command {

    std::wstring raw_command;
    std::string name;
    std::optional<AbsoluteRect> command_rect;
    std::optional<AbsoluteDocumentPos> command_point;
    std::optional<std::wstring> command_text;

public:

    CustomCommand(MainWidget* widget_, std::string name_, std::wstring command_) : Command(name_, widget_) {
        raw_command = command_;
        name = name_;
    }

    std::optional<Requirement> next_requirement(MainWidget* widget) {
        if (command_requires_rect(raw_command) && (!command_rect.has_value())) {
            Requirement req = { RequirementType::Rect, "Command Rect" };
            return req;
        }
        if (command_requires_text(raw_command) && (!command_text.has_value())) {
            Requirement req = { RequirementType::Text, "Command Text" };
            return req;
        }
        return {};
    }

    void set_rect_requirement(AbsoluteRect rect) {
        command_rect = rect;
    }

    void set_point_requirement(AbsoluteDocumentPos point) {
        command_point = point;
    }

    void set_text_requirement(std::wstring txt) {
        command_text = txt;
    }

    void perform() {
        widget->execute_command(raw_command, command_text.value_or(L""));
    }

    std::string get_name() {
        return name;
    }


};


class ToggleConfigCommand : public Command {
	std::string config_name;
public:
	ToggleConfigCommand(MainWidget* widget, std::string config_name_) : Command("toggleconfig_" + config_name_, widget) {
		config_name = config_name_;
	}

	void perform() {
        //widget->config_manager->deserialize_config(config_name, text.value());
        auto conf = widget->config_manager->get_mut_config_with_name(utf8_decode(config_name));
        *(bool*)conf->value = !*(bool*)conf->value;
        widget->on_config_changed(config_name);
	}

	bool requires_document() { return false; }
};

class ConfigCommand : public Command {
    std::string config_name;
    std::optional<std::wstring> text = {};
    ConfigManager* config_manager;
public:
    ConfigCommand(MainWidget* widget_, std::string config_name_, ConfigManager* config_manager_) : Command("setconfig_" + config_name_, widget_), config_manager(config_manager_) {
        config_name = config_name_;
    }

    void set_text_requirement(std::wstring value) {
        text = value;
    }

    void set_file_requirement(std::wstring value) {
        text = value;
    }

    std::wstring get_text_default_value() {
        Config* config = config_manager->get_mut_config_with_name(utf8_decode(config_name));
        std::wstringstream config_stream;
        config->serialize(config->value, config_stream);
        std::wstring default_value = config_stream.str();

        return default_value;
    }

    std::optional<Requirement> next_requirement(MainWidget* widget) {
        if (TOUCH_MODE) {
            Config* config = config_manager->get_mut_config_with_name(utf8_decode(config_name));
            if ((!text.has_value()) && config->config_type == ConfigType::String) {
                Requirement res;
                res.type = RequirementType::Text;
                res.name = "Config Value";
                return res;
            }
            else if ((!text.has_value()) && config->config_type == ConfigType::FilePath) {
                Requirement res;
                res.type = RequirementType::File;
                res.name = "Config Value";
                return res;
            }
            else if ((!text.has_value()) && config->config_type == ConfigType::FolderPath) {
                Requirement res;
                res.type = RequirementType::Folder;
                res.name = "Config Value";
                return res;
            }
            return {};
        }
        else {
            if (text.has_value()) {
                return {};
            }
            else {

                Requirement res;
                res.type = RequirementType::Text;
                res.name = "Config Value";
                return res;
            }
        }
    }

    void perform() {

        if (TOUCH_MODE) {
            Config* config = widget->config_manager->get_mut_config_with_name(utf8_decode(config_name));


            if (config->config_type == ConfigType::String || config->config_type == ConfigType::FilePath || config->config_type == ConfigType::FolderPath) {
                if (widget->config_manager->deserialize_config(config_name, text.value())) {
                    widget->on_config_changed(config_name);
                }
            }
            if (config->config_type == ConfigType::Macro) {
                widget->push_current_widget(new MacroConfigUI(config_name, widget, (std::wstring*)config->value, *(std::wstring*)config->value));
                widget->show_current_widget();
            }
            if (config->config_type == ConfigType::Color3) {
                widget->push_current_widget(new Color3ConfigUI(config_name, widget, (float*)config->value));
                widget->show_current_widget();
            }

            if (config->config_type == ConfigType::Color4) {
                widget->push_current_widget(new Color4ConfigUI(config_name, widget, (float*)config->value));
                widget->show_current_widget();
            }
            if (config->config_type == ConfigType::Bool) {
                widget->push_current_widget(new BoolConfigUI(config_name, widget, (bool*)config->value, QString::fromStdWString(config->name)));
                widget->show_current_widget();
            }
            if (config->config_type == ConfigType::EnableRectangle) {
                widget->push_current_widget(new RectangleConfigUI(config_name, widget, (UIRect*)config->value));
                widget->show_current_widget();
            }
            if (config->config_type == ConfigType::Float) {
                FloatExtras extras = std::get<FloatExtras>(config->extras);
                widget->push_current_widget(new FloatConfigUI(config_name, widget, (float*)config->value, extras.min_val, extras.max_val));
                widget->show_current_widget();

            }
            if (config->config_type == ConfigType::Int) {
                IntExtras extras = std::get<IntExtras>(config->extras);
                widget->push_current_widget(new IntConfigUI(config_name, widget, (int*)config->value, extras.min_val, extras.max_val));
                widget->show_current_widget();
            }

            //        config->serialize
        }
        else {


            if (text.value().size() > 1) {
                if (text.value().substr(0, 2) == L"+=" || text.value().substr(0, 2) == L"-=") {
                    std::wstring config_name_encoded = utf8_decode(config_name);
                    Config* config_mut = widget->config_manager->get_mut_config_with_name(config_name_encoded);
                    ConfigType config_type = config_mut->config_type;

                    if (config_type == ConfigType::Int) {
                        int mult = text.value()[0] == '+' ? 1 : -1;
                        int* config_ptr = (int*)config_mut->value;
                        int prev_value = *config_ptr;
                        int new_value = QString::fromStdWString(text.value()).right(text.value().size() - 2).toInt();
                        *config_ptr += mult * new_value;
                        widget->on_config_changed(config_name);
                        return;
                    }

                    if (config_type == ConfigType::Float) {
                        float mult = text.value()[0] == '+' ? 1 : -1;
                        float* config_ptr = (float*)config_mut->value;
                        float prev_value = *config_ptr;
                        float new_value = QString::fromStdWString(text.value()).right(text.value().size() - 2).toFloat();
                        *config_ptr += mult * new_value;
                        widget->on_config_changed(config_name);
                        return;
                    }
                }
            }
            if (widget->config_manager->deserialize_config(config_name, text.value())) {
                widget->on_config_changed(config_name);
            }
        }
    }

    bool requires_document() { return false; }
};



struct ParseState {
    QString str;
    int index;

    QChar ch() {
        return str[index];
    }

    void skip_whitespace() {
        while ((index < str.size()) && ch() == ' ') {
            index++;
        }
    }

    bool expect(char c) {
        if (index < str.size() && str[index] == c) {
            index++;
            return true;
        }
        else {
            qDebug() << "Parse error: expected " << c << " but got " << str[index];
            return false;
        }
    }

    std::optional<CommandInvocation> get_next_invocation() {
        skip_whitespace();
        QString next_command_name = get_next_command_name();
        skip_whitespace();
        if (next_command_name.size()) {
            QStringList next_command_args = get_command_args();
            return CommandInvocation{ next_command_name, next_command_args };
        }
        else {
            return {};
        }
    }

    std::vector<CommandInvocation> parse() {
        std::vector<CommandInvocation> res;

        while (true) {
            std::optional<CommandInvocation> next_invocation = get_next_invocation();
            if (!next_invocation.has_value()) break;

            skip_whitespace();

            if (index < str.size() && ch() == ';') expect(';');

            if (next_invocation) {
                res.push_back(next_invocation.value());
            }
            
        }
        return res;
    }

    QString get_argument() {
        bool is_prev_char_backslash = false;
        QString res;

        while (index < str.size()) {
            if (!is_prev_char_backslash) {
                if (ch() == '\\') {
                    is_prev_char_backslash = true;
                    index++;
                }
                else if (ch() == '\'') {
                    return res;
                }
                else {
                    res.append(ch());
                    index++;
                }
            }
            else {
                if (ch() == '\\') {
                    res.append('\\');
                }
                else if (ch() == '\'') {
                    res.append('\'');
                }
                is_prev_char_backslash = false;
                index++;
            }
        }
        return res;
    }

    bool is_next_non_whitespace_character_a_single_quote() {
        int i = index;

        while (i < str.size() && i == ' ') {
            i++;
        }

        if (i < str.size()) {
            if (str[i] == '\'') return true;
        }

        return false;
    }

    QStringList get_command_args() {
        if (index < str.size()) {
            if (ch() == ';') {
                return QStringList();
            }

            QStringList res;
            expect('(');
            if (is_next_non_whitespace_character_a_single_quote()) {
                while (true) {
                    skip_whitespace();
                    expect('\'');
                    res.push_back(get_argument());
                    expect('\'');
                    skip_whitespace();
                    if (index < str.size()) {
                        if (ch() == ')') {
                            index++;
                            break;
                        }
                        if (ch() == ',') {
                            index++;
                            continue;
                        }
                    }
                    else {
                        break;
                    }
                }
                return res;
            }
            else {
                QString arg;
                while (index < str.size() && ch() != ')') {
                    arg.push_back(str[index]);
                    index++;
                }
                res.push_back(arg);
                expect(')');
                return res;
            }
        }
        else {
            return QStringList();
        }

    }

    bool can_current_char_be_command_name() {
        return str[index].isDigit() || str[index].isLetter() || (str[index] == '_') || (str[index] == '[') || (str[index] == ']');
    }

    QString get_next_command_name() {
        skip_whitespace();
        QString res;
        while (index < str.size() && (can_current_char_be_command_name())) {
            res.push_back(ch());
            index++;
        }
        return res;
    }


};


class MacroCommand : public Command {
    std::vector<std::unique_ptr<Command>> commands;
    std::vector<std::string> modes;
    //std::wstring commands;
    std::string name;
    std::wstring raw_commands;
    CommandManager* command_manager;
    bool is_modal = false;
    std::vector<bool> performed;
    std::vector<bool> pre_performed;

public:

    std::unique_ptr<Command> get_subcommand(CommandInvocation invocation) {
        std::string subcommand_name = invocation.command_string().toStdString();
        auto subcommand = widget->command_manager->get_command_with_name(widget, subcommand_name);
        if (subcommand) {
            for (auto arg : invocation.command_args) {
                subcommand->set_next_requirement_with_string(arg.toStdWString());
            }
            return std::move(subcommand);
        }
        else {
            return std::move(std::make_unique<LazyCommand>(subcommand->get_name(), widget, widget->command_manager, invocation));
        }
    }

    void set_result_socket(QLocalSocket* rsocket) {
        result_socket = rsocket;
        for (auto& subcommand : commands) {
            subcommand->set_result_socket(result_socket);
        }
    }

    void set_result_mutex(bool* id, std::wstring* result_location) {
        is_done = id;
        result_holder = result_location;
        for (auto& subcommand : commands) {
            subcommand->set_result_mutex(id, result_location);
        }
    }

    MacroCommand(MainWidget* widget_, CommandManager* manager, std::string name_, std::wstring commands_) : Command(name_, widget_) {
        //commands = std::move(commands_);
        command_manager = manager;
        name = name_;
        raw_commands = commands_;


        QString str = QString::fromStdWString(commands_);
        ParseState parser;
        parser.str = str;
        parser.index = 0;
        std::vector<CommandInvocation> command_invocations = parser.parse();
        for (auto command_invocation : command_invocations) {
            if (command_invocation.command_name.size() > 0) {
                if (command_invocation.command_name[0] == '[') {
                    is_modal = true;
                }

                if (is_modal) {
                    modes.push_back(command_invocation.mode_string().toStdString());
                }

                commands.push_back(get_subcommand(command_invocation));
                performed.push_back(false);
                pre_performed.push_back(false);
            }
        }

    }


    std::optional<std::wstring> get_result() override{
        if (commands.size() > 0) {
            return commands.back()->get_result();
        }
        return {};
    }

    void set_text_requirement(std::wstring value) {
        if (is_modal) {
            int current_mode_index = get_current_mode_index();
            if (current_mode_index >= 0) {
                commands[current_mode_index]->set_text_requirement(value);
            }
        }
        else {
            for (int i = 0; i < commands.size(); i++) {
                std::optional<Requirement> req = commands[i]->next_requirement(widget);
                if (req) {
                    if (req.value().type == RequirementType::Text) {
                        commands[i]->set_text_requirement(value);
                    }
                    return;
                }
            }
        }
    }

    bool is_menu_command() {
        if (is_modal) {
            bool res = false;
            for (std::string mode : modes) {
                if (mode == "m") {
                    res = true;
                }
            }
            return res;
        }
        return false;
    }

    void set_generic_requirement(QVariant value) {
        if (is_modal) {
            int current_mode_index = get_current_mode_index();
            if (current_mode_index >= 0) {
                commands[current_mode_index]->set_generic_requirement(value);
            }
        }
        else {
            for (int i = 0; i < commands.size(); i++) {
                std::optional<Requirement> req = commands[i]->next_requirement(widget);
                if (req) {
                    if (req.value().type == RequirementType::Generic) {
                        commands[i]->set_generic_requirement(value);
                    }
                    return;
                }
            }
        }
    }

    void handle_generic_requirement() {
        if (is_modal) {
            int current_mode_index = get_current_mode_index();
            if (current_mode_index >= 0) {
                commands[current_mode_index]->handle_generic_requirement();
            }
        }
        else {
            for (int i = 0; i < commands.size(); i++) {
                std::optional<Requirement> req = commands[i]->next_requirement(widget);
                if (req) {
                    if (req.value().type == RequirementType::Generic) {
                        commands[i]->handle_generic_requirement();
                    }
                    return;
                }
            }
        }
    }

    void set_symbol_requirement(char value) {
        if (is_modal) {
            int current_mode_index = get_current_mode_index();
            if (current_mode_index >= 0) {
                commands[current_mode_index]->set_symbol_requirement(value);
            }
        }
        else {

            for (int i = 0; i < commands.size(); i++) {
                std::optional<Requirement> req = commands[i]->next_requirement(widget);
                if (req) {
                    if (req.value().type == RequirementType::Symbol) {
                        commands[i]->set_symbol_requirement(value);
                    }
                    return;
                }
            }
        }
    }

    bool requires_document() {
        if (is_modal) {
            int current_mode_index = get_current_mode_index();
            if (current_mode_index >= 0) {
                return commands[current_mode_index]->requires_document();
            }
        }
        else {

            for (int i = 0; i < commands.size(); i++) {
                if (commands[i]->requires_document()) {
                    return true;
                }
            }
            return false;
        }
    }

    void set_file_requirement(std::wstring value) {
        if (is_modal) {
            int current_mode_index = get_current_mode_index();
            if (current_mode_index >= 0) {
                commands[current_mode_index]->set_file_requirement(value);
            }
        }
        else {
            for (int i = 0; i < commands.size(); i++) {
                std::optional<Requirement> req = commands[i]->next_requirement(widget);
                if (req) {
                    if (req->type == RequirementType::File || req->type == RequirementType::Folder) {
                        commands[i]->set_file_requirement(value);
                    }
                    return;
                }
            }
        }
    }

    void set_rect_requirement(AbsoluteRect value) {
        if (is_modal) {
            int current_mode_index = get_current_mode_index();
            if (current_mode_index >= 0) {
                commands[current_mode_index]->set_rect_requirement(value);
            }
        }
        else {

            for (int i = 0; i < commands.size(); i++) {
                std::optional<Requirement> req = commands[i]->next_requirement(widget);
                if (req) {
                    if (req.value().type == RequirementType::Rect) {
                        commands[i]->set_rect_requirement(value);
                    }
                    return;
                }
            }
        }
    }


    void pre_perform() {
        if (is_modal) {
            int current_mode_index = get_current_mode_index();
            if (current_mode_index >= 0) {
                commands[current_mode_index]->pre_perform();
            }
        }
        else {
            for (int i = 0; i < performed.size(); i++) {
                if (performed[i] == false) {
                    commands[i]->pre_perform();
                    return;
                }
            }
        }
    }

    std::optional<Requirement> next_requirement(MainWidget* widget) {
        if (is_modal && (modes.size() != commands.size())) {
            std::wcerr << L"Invalid modal command : " << raw_commands;
            return {};
        }

        if (is_modal) {
            int current_mode_index = get_current_mode_index();
            if (current_mode_index >= 0) {
                return commands[current_mode_index]->next_requirement(widget);
            }
            return {};
        }
        else {
            for (int i = 0; i < commands.size(); i++) {
                if (commands[i]->next_requirement(widget)) {
                    return commands[i]->next_requirement(widget);
                }
                else {
                    if (!performed[i]) {
                        perform_subcommand(i);
                    }
                }
            }
            return {};
        }
    }

    void perform_subcommand(int index) {
        if (!performed[index]) {
            if (commands[index]->pushes_state()) {
                widget->push_state();
            }
            commands[index]->run();
            performed[index] = true;
        }
    }


    bool is_enabled() {
        if (!is_modal) {
            return commands.size() > 0;
        }
        else {
            std::string mode_string = widget->get_current_mode_string();
            for (int i = 0; i < modes.size(); i++) {
                if (mode_matches(mode_string, modes[i])) {
                    return true;
                }
            }
            return false;
        }

    }

    void perform() {
        if (!is_modal) {
            for (int i = 0; i < commands.size(); i++) {

                if (!performed[i]) {
                    perform_subcommand(i);
                }
            }
        }
        else {
            if (modes.size() != commands.size()) {
                qDebug() << "Invalid modal command : " << QString::fromStdWString(raw_commands);
                return;
            }
            std::string mode_string = widget->get_current_mode_string();

            for (int i = 0; i < commands.size(); i++) {
                if (mode_matches(mode_string, modes[i])) {
                    widget->handle_command_types(std::move(commands[i]), 1);
                    //commands[i]->run(widget);
                    return;
                }
            }
        }
    }

    int get_current_mode_index() {
        if (is_modal) {
            std::string mode_str = widget->get_current_mode_string();
            for (int i = 0; i < commands.size(); i++) {
                if (mode_matches(mode_str, modes[i])) {
                    return i;
                }
            }
            return -1;

        }

        return -1;
    }

    bool mode_matches(std::string current_mode, std::string command_mode) {
        for (auto c : command_mode) {
            if (current_mode.find(c) == std::string::npos) {
                return false;
            }
        }
        return true;
    }

    std::string get_name() {
        if (name.size() > 0 || commands.size() == 0) {
            return name;
        }
        else {
            if (is_modal) {
                std::string current_mode_string = widget->get_current_mode_string();
                for (int i = 0; i < modes.size(); i++) {
                    if (mode_matches(current_mode_string, modes[i])) {
                        return commands[i]->get_name();
                    }
                }
            }
            else {

                std::string res;
                for (auto& command : commands) {
                    res += command->get_name();
                }
                return res;
            }
            //return "[macro]" + commands[0]->get_name();
        }
    }

};

class HoldableCommand : public Command {
    std::unique_ptr<Command> down_command = {};
    std::unique_ptr<Command> up_command = {};
    std::unique_ptr<Command> hold_command = {};
    std::string name;
    CommandManager* command_manager;

public:
    HoldableCommand(MainWidget* widget_, CommandManager* manager, std::string name_, std::wstring commands_) : Command(name_, widget_) {
        command_manager = manager;
        name = name_;
        QString str = QString::fromStdWString(commands_);
        QStringList parts = str.split('|');
        if (parts.size() == 2) {
            down_command = std::make_unique<MacroCommand>(widget, manager, name_ + "{down}", parts[0].toStdWString());
            up_command = std::make_unique<MacroCommand>(widget, manager, name_ + "{up}", parts[1].toStdWString());
        }
        else {
            down_command = std::make_unique<MacroCommand>(widget, manager, name_ + "{down}", parts[0].toStdWString());
            hold_command = std::make_unique<MacroCommand>(widget, manager, name_ + "{hold}", parts[1].toStdWString());
            up_command = std::make_unique<MacroCommand>(widget, manager, name_ + "{up}", parts[2].toStdWString());
        }
    }

    std::optional<Requirement> next_requirement(MainWidget* widget) {
        // holdable commands can't have requirements
        return {};
    }

    bool requires_document() {
        return true;
    }

    void perform() {
        down_command->run();
    }

    void perform_up() {
        up_command->run();
    }

    void on_key_hold() {
        if (hold_command) {
            hold_command->run();
        }
    }


    bool is_holdable() {
        return true;
    }


};


CommandManager::CommandManager(ConfigManager* config_manager) {

    register_command<GotoBeginningCommand>();
    register_command<GotoEndCommand>();
    register_command<GotoDefinitionCommand>();
    register_command<OverviewDefinitionCommand>();
    register_command<PortalToDefinitionCommand>();
    register_command<GotoLoadedDocumentCommand>();
    register_command<NextItemCommand>();
    register_command<PrevItemCommand>();
    register_command<ToggleTextMarkCommand>();
    register_command<MoveTextMarkForwardCommand>();
    register_command<MoveTextMarkBackwardCommand>();
    register_command<MoveTextMarkForwardWordCommand>();
    register_command<MoveTextMarkBackwardWordCommand>();
    register_command<MoveTextMarkDownCommand>();
    register_command<MoveTextMarkUpCommand>();
    register_command<SetMark>();
    register_command<ToggleDrawingMask>();
    register_command<TurnOnAllDrawings>();
    register_command<TurnOffAllDrawings>();
    register_command<GotoMark>();
    register_command<GotoPageWithPageNumberCommand>();
    register_command<EditSelectedBookmarkCommand>();
    register_command<EditSelectedHighlightCommand>();
    register_command<SearchCommand>();
    register_command<DownloadPaperWithUrlCommand>();
    register_command<DownloadClipboardUrlCommand>();
    register_command<ExecuteMacroCommand>();
    register_command<ControlMenuCommand>();
    register_command<SetViewStateCommand>();
    register_command<GetConfigCommand>();
    register_command<GetConfigNoDialogCommand>();
    register_command<ShowOptionsCommand>();
    register_command<ShowTextPromptCommand>();
    register_command<GetStateJsonCommand>();
    register_command<GetPaperNameCommand>();
    register_command<GetOverviewPaperName>();
    register_command<GetAnnotationsJsonCommand>();
    register_command<ToggleRectHintsCommand>();
    register_command<AddAnnotationToSelectedHighlightCommand>();
    register_command<AddAnnotationToHighlightCommand>();
    register_command<ChangeHighlightTypeCommand>();
    register_command<RenameCommand>();
    register_command<SetFreehandThickness>();
    register_command<GotoPageWithLabel>();
    register_command<RegexSearchCommand>();
    register_command<ChapterSearchCommand>();
    register_command<ToggleTwoPageModeCommand>();
    register_command<FitEpubToWindowCommand>();
    register_command<MoveDownCommand>();
    register_command<MoveUpCommand>();
    register_command<MoveLeftCommand>();
    register_command<MoveRightCommand>();
    register_command<MoveDownSmoothCommand>();
    register_command<MoveUpSmoothCommand>();
    register_command<MoveLeftInOverviewCommand>();
    register_command<MoveRightInOverviewCommand>();
    register_command<SaveScratchpadCommand>();
    register_command<LoadScratchpadCommand>();
    register_command<ClearScratchpadCommand>();
    register_command<ZoomInCommand>();
    register_command<ZoomOutCommand>();
    register_command<ZoomInOverviewCommand>();
    register_command<ZoomOutOverviewCommand>();
    register_command<FitToPageWidthCommand>();
    register_command<FitToPageHeightCommand>();
    register_command<FitToPageSmartCommand>();
    register_command<FitToPageHeightSmartCommand>();
    register_command<FitToPageWidthSmartCommand>();
    register_command<NextPageCommand>();
    register_command<PreviousPageCommand>();
    register_command<OpenDocumentCommand>();
    register_command<ScreenshotCommand>();
    register_command<FramebufferScreenshotCommand>();
    register_command<WaitCommand>();
    register_command<WaitForRendersToFinishCommand>();
    register_command<WaitForSearchToFinishCommand>();
    register_command<WaitForIndexingToFinishCommand>();
    register_command<AddBookmarkCommand>();
    register_command<AddBookmarkMarkedCommand>();
    register_command<AddBookmarkFreetextCommand>();
    register_command<CopyDrawingsFromScratchpadCommand>();
    register_command<CopyScreenshotToScratchpad>();
    register_command<CopyScreenshotToClipboard>();
    register_command<AddHighlightCommand>();
    register_command<GotoTableOfContentsCommand>();
    register_command<GotoHighlightCommand>();
    register_command<IncreaseFreetextBookmarkFontSizeCommand>();
    register_command<DecreaseFreetextBookmarkFontSizeCommand>();
    register_command<GotoPortalListCommand>();
    register_command<GotoBookmarkCommand>();
    register_command<GotoBookmarkGlobalCommand>();
    register_command<GotoHighlightGlobalCommand>();
    register_command<PortalCommand>();
    register_command<PortalCommand>();
    register_command<CreateVisiblePortalCommand>();
    register_command<NextStateCommand>();
    register_command<PrevStateCommand>();
    register_command<NextStateCommand>();
    register_command<PrevStateCommand>();
    register_command<DeletePortalCommand>();
    register_command<DeletePortalCommand>();
    register_command<DeleteBookmarkCommand>();
    register_command<DeleteHighlightCommand>();
    register_command<DeleteVisibleBookmarkCommand>();
    register_command<EditVisibleBookmarkCommand>();
    register_command<GotoPortalCommand>();
    register_command<GotoPortalCommand>();
    register_command<EditPortalCommand>();
    register_command<EditPortalCommand>();
    register_command<OpenPrevDocCommand>();
    register_command<OpenAllDocsCommand>();
    register_command<OpenDocumentEmbeddedCommand>();
    register_command<OpenDocumentEmbeddedFromCurrentPathCommand>();
    register_command<CopyCommand>();
    register_command<ToggleFullscreenCommand>();
    register_command<MaximizeCommand>();
    register_command<ToggleOneWindowCommand>();
    register_command<ToggleHighlightCommand>();
    register_command<ToggleSynctexCommand>();
    register_command<TurnOnSynctexCommand>();
    register_command<ToggleShowLastCommand>();
    register_command<ForwardSearchCommand>();
    register_command<CommandCommand>();
    register_command<CommandPaletteCommand>();
    register_command<ExternalSearchCommand>();
    register_command<OpenSelectedUrlCommand>();
    register_command<ScreenDownCommand>();
    register_command<ScreenUpCommand>();
    register_command<NextChapterCommand>();
    register_command<PrevChapterCommand>();
    register_command<ShowContextMenuCommand>();
    register_command<ToggleDarkModeCommand>();
    register_command<TogglePresentationModeCommand>();
    register_command<TurnOnPresentationModeCommand>();
    register_command<ToggleMouseDragMode>();
    register_command<ToggleFreehandDrawingMode>();
    register_command<TogglePenDrawingMode>();
    register_command<ToggleScratchpadMode>();
    register_command<CloseWindowCommand>();
    register_command<QuitCommand>();
    register_command<EscapeCommand>();
    register_command<TogglePDFAnnotationsCommand>();
    register_command<CloseWindowCommand>();
    register_command<OpenLinkCommand>();
    register_command<OverviewLinkCommand>();
    register_command<PortalToLinkCommand>();
    register_command<CopyLinkCommand>();
    register_command<KeyboardSelectCommand>();
    register_command<KeyboardSmartjumpCommand>();
    register_command<KeyboardOverviewCommand>();
    register_command<KeysCommand>();
    register_command<KeysUserCommand>();
    register_command<PrefsCommand>();
    register_command<PrefsUserCommand>();
    register_command<MoveVisualMarkDownCommand>();
    register_command<MoveVisualMarkUpCommand>();
    register_command<MoveVisualMarkNextCommand>();
    register_command<MoveVisualMarkPrevCommand>();
    register_command<MoveVisualMarkDownCommand>();
    register_command<MoveVisualMarkUpCommand>();
    register_command<MoveVisualMarkNextCommand>();
    register_command<MoveVisualMarkPrevCommand>();
    register_command<ToggleCustomColorMode>();
    register_command<SetSelectHighlightTypeCommand>();
    register_command<SetFreehandType>();
    register_command<ToggleWindowConfigurationCommand>();
    register_command<PrefsUserAllCommand>();
    register_command<KeysUserAllCommand>();
    register_command<FitToPageWidthRatioCommand>();
    register_command<SmartJumpUnderCursorCommand>();
    register_command<DownloadPaperUnderCursorCommand>();
    register_command<DownloadPaperWithNameCommand>();
    register_command<OverviewUnderCursorCommand>();
    register_command<CloseOverviewCommand>();
    register_command<VisualMarkUnderCursorCommand>();
    register_command<CloseVisualMarkCommand>();
    register_command<ZoomInCursorCommand>();
    register_command<ZoomOutCursorCommand>();
    register_command<GotoLeftCommand>();
    register_command<GotoLeftSmartCommand>();
    register_command<GotoRightCommand>();
    register_command<GotoRightSmartCommand>();
    register_command<RotateClockwiseCommand>();
    register_command<RotateCounterClockwiseCommand>();
    register_command<GotoNextHighlightCommand>();
    register_command<GotoPrevHighlightCommand>();
    register_command<GotoNextHighlightOfTypeCommand>();
    register_command<GotoPrevHighlightOfTypeCommand>();
    register_command<AddHighlightWithCurrentTypeCommand>();
    register_command<UndoDrawingCommand>();
    register_command<EnterPasswordCommand>();
    register_command<ToggleFastreadCommand>();
    register_command<GotoTopOfPageCommand>();
    register_command<GotoBottomOfPageCommand>();
    register_command<NewWindowCommand>();
    register_command<ReloadCommand>();
    register_command<ReloadNoFlickerCommand>();
    register_command<ReloadConfigCommand>();
    register_command<SynctexUnderCursorCommand>();
    register_command<SynctexUnderRulerCommand>();
    register_command<SetStatusStringCommand>();
    register_command<ClearStatusStringCommand>();
    register_command<ToggleTittlebarCommand>();
    register_command<NextPreviewCommand>();
    register_command<PreviousPreviewCommand>();
    register_command<GotoOverviewCommand>();
    register_command<PortalToOverviewCommand>();
    register_command<GotoSelectedTextCommand>();
    register_command<FocusTextCommand>();
    register_command<DownloadOverviewPaperCommand>();
    register_command<GotoWindowCommand>();
    register_command<ToggleSmoothScrollModeCommand>();
    register_command<GotoBeginningCommand>();
    register_command<ToggleScrollbarCommand>();
    register_command<OverviewToPortalCommand>();
    register_command<OverviewRulerPortalCommand>();
    register_command<GotoRulerPortalCommand>();
    register_command<SelectRectCommand>();
    register_command<ToggleTypingModeCommand>();
    register_command<DonateCommand>();
    register_command<OverviewNextItemCommand>();
    register_command<OverviewPrevItemCommand>();
    register_command<DeleteHighlightUnderCursorCommand>();
    register_command<NoopCommand>();
    register_command<ImportCommand>();
    register_command<ExportCommand>();
    register_command<WriteAnnotationsFileCommand>();
    register_command<LoadAnnotationsFileCommand>();
    register_command<LoadAnnotationsFileSyncDeletedCommand>();
    register_command<EnterVisualMarkModeCommand>();
    register_command<SetPageOffsetCommand>();
    register_command<ToggleVisualScrollCommand>();
    register_command<ToggleHorizontalLockCommand>();
    register_command<ExecuteCommand>();
    register_command<EmbedAnnotationsCommand>();
    register_command<ImportAnnotationsCommand>();
    register_command<CopyWindowSizeConfigCommand>();
    register_command<ToggleSelectHighlightCommand>();
    register_command<OpenLastDocumentCommand>();
    register_command<ToggleStatusbarCommand>();
    register_command<StartReadingCommand>();
    register_command<StopReadingCommand>();
    register_command<ScanNewFilesFromScanDirCommand>();
    register_command<AddMarkedDataCommand>();
    register_command<RemoveMarkedDataCommand>();
    register_command<ExportMarkedDataCommand>();
    register_command<UndoMarkedDataCommand>();
    register_command<GotoRandomPageCommand>();
    register_command<ClearCurrentPageDrawingsCommand>();
    register_command<ClearCurrentDocumentDrawingsCommand>();
    register_command<DeleteFreehandDrawingsCommand>();
    register_command<SelectFreehandDrawingsCommand>();
    register_command<SelectCurrentSearchMatchCommand>();
    register_command<ShowTouchMainMenu>();
    register_command<ShowTouchSettingsMenu>();
    register_command<ShowTouchDrawingMenu>();
    register_command<DebugCommand>();
    register_command<ExportPythonApiCommand>();
    register_command<ExportDefaultConfigFile>();
    register_command<ExportCommandNamesCommand>();
    register_command<ExportConfigNamesCommand>();
    register_command<TestCommand>();
    register_command<PrintUndocumentedCommandsCommand>();
    register_command<PrintUndocumentedConfigsCommand>();
    register_command<PrintNonDefaultConfigs>();
    register_command<SetWindowRectCommand>();

    for (auto [command_name_, command_value] : ADDITIONAL_COMMANDS) {
        std::string command_name = utf8_encode(command_name_);
        std::wstring local_command_value = command_value;
        new_commands[command_name] = [command_name, local_command_value, this](MainWidget* w) {return  std::make_unique<CustomCommand>(w, command_name, local_command_value); };
        command_required_prefixes[QString::fromStdString(command_name)] = "_";
    }


    for (auto [command_name_, js_command_info] : ADDITIONAL_JAVASCRIPT_COMMANDS) {
        handle_new_javascript_command(command_name_, js_command_info, false);
        command_required_prefixes[QString::fromStdWString(command_name_)] = "_";
    }

    for (auto [command_name_, js_command_info] : ADDITIONAL_ASYNC_JAVASCRIPT_COMMANDS) {
        handle_new_javascript_command(command_name_, js_command_info, true);
        command_required_prefixes[QString::fromStdWString(command_name_)] = "_";
    }

    for (auto [command_name_, macro_value] : ADDITIONAL_MACROS) {
        std::string command_name = utf8_encode(command_name_);
        std::wstring local_macro_value = macro_value;
        new_commands[command_name] = [command_name, local_macro_value, this](MainWidget* w) {return std::make_unique<MacroCommand>(w, this, command_name, local_macro_value); };
        command_required_prefixes[QString::fromStdString(command_name)] = "_";
    }

    std::vector<Config> configs = config_manager->get_configs();

    for (auto conf : configs) {

        std::string confname = utf8_encode(conf.name);
        std::string config_set_command_name = "setconfig_" + confname;
        //commands.push_back({ config_set_command_name, true, false , false, false, true, {} });
        new_commands[config_set_command_name] = [confname, config_manager](MainWidget* w) {return std::make_unique<ConfigCommand>(w, confname, config_manager); };
        command_required_prefixes[QString::fromStdString(config_set_command_name)] = "setconfig_";

        if (conf.config_type == ConfigType::Bool) {
            std::string config_toggle_command_name = "toggleconfig_" + confname;
            new_commands[config_toggle_command_name] = [confname, config_manager](MainWidget* w) {return std::make_unique<ToggleConfigCommand>(w, confname); };
            command_required_prefixes[QString::fromStdString(config_toggle_command_name)] = "toggleconfig_";
        }

    }

    QDateTime current_time = QDateTime::currentDateTime();
    QDateTime older_time = current_time.addSecs(-1);
    for (auto [command_name, _] : new_commands) {
        if (command_name.size() > 0 && command_name[0] == '_') {
            command_last_uses[command_name] = older_time;
        }
        else {
            command_last_uses[command_name] = current_time;
        }
    }

}

void CommandManager::update_command_last_use(std::string command_name) {
    command_last_uses[command_name] = QDateTime::currentDateTime();
}

void CommandManager::handle_new_javascript_command(std::wstring command_name_, JsCommandInfo info, bool is_async) {
        std::string command_name = utf8_encode(command_name_);
        auto [command_parent_file_path, command_file_path, entry_point] = info;

        QDir parent_dir = QFileInfo(QString::fromStdWString(command_parent_file_path)).dir();
        QFileInfo javascript_file_info(QString::fromStdWString(command_file_path));
        QString absolute_file_path;

        if (javascript_file_info.isRelative()){
            absolute_file_path = parent_dir.absoluteFilePath(javascript_file_info.filePath());
        }
        else{
            absolute_file_path = javascript_file_info.absoluteFilePath();
        }
        QFile code_file(absolute_file_path);
        if (code_file.open(QIODevice::ReadOnly)) {
            QTextStream in(&code_file);
            QString code = in.readAll();
            new_commands[command_name] = [command_name, code, entry_point, is_async, this](MainWidget* w) {
                return std::make_unique<JavascriptCommand>(command_name, code.toStdWString(), entry_point, is_async, w);
                };
        }
}

std::unique_ptr<Command> CommandManager::get_command_with_name(MainWidget* w, std::string name) {

    if (new_commands.find(name) != new_commands.end()) {
        return new_commands[name](w);
    }
    return nullptr;
}

QStringList CommandManager::get_all_command_names() {
    std::vector<std::pair<QDateTime, QString>> pairs;
    QStringList res;

    if (SHOW_MOST_RECENT_COMMANDS_FIRST) {
        for (const auto& com : new_commands) {
            pairs.push_back(std::make_pair(command_last_uses[com.first], QString::fromStdString(com.first)));
        }

        std::sort(pairs.begin(), pairs.end(), [](const auto& a, const auto& b) {
            return a.first > b.first;
            });

        for (auto [_, com] : pairs) {
            res.push_back(com);
        }
    }
    else {
        for (const auto& com : new_commands) {
            res.push_back(QString::fromStdString(com.first));
        }
    }


    return res;
}

void print_tree_node(InputParseTreeNode node) {
    if (node.requires_text) {
        std::wcerr << "text node" << std::endl;
        return;
    }
    if (node.requires_symbol) {
        std::wcerr << "symbol node" << std::endl;
        return;
    }

    if (node.control_modifier) {
        std::wcerr << "Ctrl+";
    }

    if (node.shift_modifier) {
        std::wcerr << "Shift+";
    }

    if (node.alt_modifier) {
        std::wcerr << "Alt+";
    }
    std::wcerr << node.command << std::endl;
}

InputParseTreeNode parse_token(std::wstring token) {
    InputParseTreeNode res;

    if (token == L"sym") {
        res.requires_symbol = true;
        return res;
    }
    if (token == L"txt") {
        res.requires_text = true;
        return res;
    }

    std::vector<std::wstring> subcommands;
    split_key_string(token, L"-", subcommands);

    for (size_t i = 0; i < subcommands.size() - 1; i++) {
        if (subcommands[i] == L"C") {
            res.control_modifier = true;
        }

        if (subcommands[i] == L"S") {
            res.shift_modifier = true;
        }

        if (subcommands[i] == L"A") {
            res.alt_modifier = true;
        }
    }

    std::wstring command_string = subcommands[subcommands.size() - 1];
    if (command_string.size() == 1) {
        res.command = subcommands[subcommands.size() - 1][0];
    }
    else {

        if (int f_key = get_f_key(command_string)) {
            res.command = Qt::Key::Key_F1 - 1 + f_key;
        }
        else {

            std::map<std::wstring, Qt::Key> keymap_temp = {
                {L"up", Qt::Key::Key_Up},
                {L"down", Qt::Key::Key_Down},
                {L"left", Qt::Key::Key_Left},
                {L"right", Qt::Key::Key_Right},
                {L"backspace", Qt::Key::Key_Backspace},
                {L"space", Qt::Key::Key_Space},
                {L"pageup", Qt::Key::Key_PageUp},
                {L"pagedown", Qt::Key::Key_PageDown},
                {L"home", Qt::Key::Key_Home},
                {L"end", Qt::Key::Key_End},
                {L"pagedown", Qt::Key::Key_End},
                {L"tab", Qt::Key::Key_Tab},
                {L"return", Qt::Key::Key_Return},
            };
            std::map<std::wstring, Qt::Key> keymap;

            for (auto item : keymap_temp) {
                keymap[item.first] = item.second;
                keymap[L"<" + item.first + L">"] = item.second;
            }

            res.command = keymap[command_string];
        }

    }

    return res;
}
void get_tokens(std::wstring line, std::vector<std::wstring>& tokens) {
    std::wstring stack;

    int stack_depth = 0;

    for (wchar_t c : line) {
        if (stack_depth && (c != '>') && (c != '<')) {
            stack.push_back(c);
        }
        else if ((c == '>')) {
            stack_depth--;
            if (stack_depth == 0) {
                tokens.push_back(stack);
                stack.clear();
            }
            else {
                stack.push_back(c);
            }
        }
        else if (c == '<') {
            if (stack_depth) {
                stack.push_back(c);
            }
            stack_depth++;
        }
        else {
            tokens.push_back(std::wstring(1, c));
        }

    }
}

bool is_command_incomplete_macro(const std::vector<std::string>& commands){

    for (auto com : commands){
        if (com.find("[") == -1){
            return false;
        }
        if (com.find("[]") != -1){
            return false;
        }
    }
    return true;
}

InputParseTreeNode* parse_lines(
    InputParseTreeNode* root,
    CommandManager* command_manager,
    const std::vector<std::wstring>& lines,
    const std::vector<std::wstring>& command_strings,
    const std::vector<std::wstring>& command_file_names,
    const std::vector<int>& command_line_numbers
) {

    for (size_t j = 0; j < lines.size(); j++) {
        std::wstring line = lines[j];

        // for example convert "<a-<space>> to ["a", "space"]
        std::vector<std::wstring> tokens;
        get_tokens(line, tokens);

        InputParseTreeNode* parent_node = root;

        for (size_t i = 0; i < tokens.size(); i++) {
            InputParseTreeNode node = parse_token(tokens[i]);
            bool existing_node = false;
            for (InputParseTreeNode* child : parent_node->children) {
                if (child->is_same(&node)) {
                    parent_node = child;
                    existing_node = true;
                    break;
                }
            }
            if (!existing_node) {
                if ((tokens[i] != L"sym") && (tokens[i] != L"txt")) {
                    if (parent_node->is_final) {
                        LOG(std::wcerr
                            << L"Warning: key defined in " << command_file_names[j]
                            << L":" << command_line_numbers[j]
                            << L" for " << command_strings[j]
                            << L" is unreachable, shadowed by final key sequence defined in "
                            << parent_node->defining_file_path
                            << L":" << parent_node->defining_file_line << L"\n");
                    }
                    auto new_node = new InputParseTreeNode(node);
                    new_node->defining_file_line = command_line_numbers[j];
                    new_node->defining_file_path = command_file_names[j];
                    parent_node->children.push_back(new_node);
                    parent_node = parent_node->children[parent_node->children.size() - 1];
                }
                else {
                    if (tokens[i] == L"sym") {
                        parent_node->requires_symbol = true;
                        parent_node->is_final = true;
                    }

                    if (tokens[i] == L"txt") {
                        parent_node->requires_text = true;
                        parent_node->is_final = true;
                    }
                }
            }
            else if (((size_t)i == (tokens.size() - 1)) &&
                (SHOULD_WARN_ABOUT_USER_KEY_OVERRIDE ||
                    (command_file_names[j].compare(parent_node->defining_file_path)) == 0)) {
                if ((parent_node->name_.size() == 0) || parent_node->name_[0].compare(utf8_encode(command_strings[j])) != 0) {
                    if (!is_command_string_modal(command_strings[j])) {

                        std::wcerr << L"Warning: key defined in " << parent_node->defining_file_path
                            << L":" << parent_node->defining_file_line
                            << L" overwritten by " << command_file_names[j]
                            << L":" << command_line_numbers[j];
                        if (parent_node->name_.size() > 0) {
                            std::wcerr << L". Overriding command: " << line
                                << L": replacing " << utf8_decode(parent_node->name_[0])
                                << L" with " << command_strings[j];
                        }
                        std::wcerr << L"\n";
                    }
                }
            }
            if ((size_t)i == (tokens.size() - 1)) {
                parent_node->is_final = true;

                QString command_name_qstr = QString::fromStdWString(command_strings[j]);
                std::vector<std::string> command_names = parse_command_name(command_name_qstr);
                std::vector<std::string> previous_names = std::move(parent_node->name_);
                parent_node->name_ = {};
                parent_node->defining_file_line = command_line_numbers[j];
                parent_node->defining_file_path = command_file_names[j];
                for (size_t k = 0; k < command_names.size(); k++) {
                    parent_node->name_.push_back(command_names[k]);
                }
                if (command_name_qstr.startsWith("{holdable}")) {
                    if (command_name_qstr.indexOf("|") == -1) {
                        qDebug() << "Error in " << command_file_names[j] << ":" << command_line_numbers[j] << ": holdable command " << command_name_qstr << " does not contain a | character";
                    }
                    else {
                        std::wstring actual_command = command_name_qstr.mid(10).toStdWString();
                        parent_node->generator = [command_manager, actual_command](MainWidget* w) {return std::make_unique<HoldableCommand>(
                            w, command_manager, "", actual_command); };
                    }
                }
                else if (command_names.size() == 1 && (command_names[0].find("[") == -1) && (command_names[0].find("(") == -1)) {
                    if (command_manager->new_commands.find(command_names[0]) != command_manager->new_commands.end()) {
                        parent_node->generator = command_manager->new_commands[command_names[0]];
                    }
                    else {
                        std::wcerr << L"Warning: command " << utf8_decode(command_names[0]) << L" used in " << parent_node->defining_file_path
                            << L":" << parent_node->defining_file_line << L" not found.\n";
                    }
                }
                else {
                    QStringList command_parts;
                    for (int k = 0; k < command_names.size(); k++) {
                        command_parts.append(QString::fromStdString(command_names[k]));
                    }

                    // is the command incomplete and should be appended to previous command instead of replacing it?
                    if (is_command_incomplete_macro(command_names)) {
                        for (int k = 0; k < previous_names.size(); k++) {
                            command_parts.append(QString::fromStdString(previous_names[k]));
                            parent_node->name_.push_back(previous_names[k]);
                        }
                    }

                    std::wstring joined_command = command_parts.join(";").toStdWString();
                    parent_node->generator = [joined_command, command_manager](MainWidget* w) {return std::make_unique<MacroCommand>(w, command_manager, "", joined_command); };
                }
                //if (command_names[j].size())
            }
            else {
                if (SHOULD_WARN_ABOUT_USER_KEY_OVERRIDE && parent_node->is_final && (parent_node->name_.size() > 0)) {
                    std::wcerr << L"Warning: unmapping " << utf8_decode(parent_node->name_[0]) << L" because of " << command_strings[j] << L" which uses " << line << L"\n";
                }
                parent_node->is_final = false;
            }

        }
    }

    return root;
}

InputParseTreeNode* parse_lines(
    CommandManager* command_manager,
    const std::vector<std::wstring>& lines,
    const std::vector<std::wstring>& command_names,
    const std::vector<std::wstring>& command_file_names,
    const std::vector<int>& command_line_numbers
) {
    // parse key configs into a trie where leaves are annotated with the name of the command

    InputParseTreeNode* root = new InputParseTreeNode;
    root->is_root = true;

    parse_lines(root, command_manager, lines, command_names, command_file_names, command_line_numbers);

    return root;

}


void get_keys_file_lines(const Path& file_path,
    std::vector<std::wstring>& command_strings,
    std::vector<std::wstring>& command_keys,
    std::vector<std::wstring>& command_files,
    std::vector<int>& command_line_numbers) {

    std::ifstream infile = std::ifstream(utf8_encode(file_path.get_path()));

    int line_number = 0;
    std::wstring default_path_name = file_path.get_path();
    while (infile.good()) {
        line_number++;
        std::string line_;
        std::wstring line;
        std::getline(infile, line_);
        line = utf8_decode(line_);

        if (line.size() == 0 || line[0] == '#') {
            continue;
        }

        QString line_string = QString::fromStdWString(line).trimmed();
        int last_space_index = line_string.lastIndexOf(' ');

        if (last_space_index >= 0){
            std::wstring command_name = line_string.left(last_space_index).trimmed().toStdWString();
            std::wstring command_key = line_string.right(line_string.size() - last_space_index - 1).trimmed().toStdWString();
            
            command_strings.push_back(command_name);
            command_keys.push_back(command_key);
            command_files.push_back(default_path_name);
            command_line_numbers.push_back(line_number);
        }
    }

    infile.close();
}

InputParseTreeNode* parse_key_config_files(CommandManager* command_manager, const Path& default_path,
    const std::vector<Path>& user_paths) {

    std::wifstream default_infile = open_wifstream(default_path.get_path());

    std::vector<std::wstring> command_strings;
    std::vector<std::wstring> command_keys;
    std::vector<std::wstring> command_files;
    std::vector<int> command_line_numbers;

    get_keys_file_lines(default_path, command_strings, command_keys, command_files, command_line_numbers);
    for (auto upath : user_paths) {
        get_keys_file_lines(upath, command_strings, command_keys, command_files, command_line_numbers);
    }

    for (auto additional_keymap : ADDITIONAL_KEYMAPS) {
        QString keymap_string = QString::fromStdWString(additional_keymap.keymap_string);
        int last_space_index = keymap_string.lastIndexOf(' ');
        std::wstring command_name = keymap_string.left(last_space_index).toStdWString();
        std::wstring mapping = keymap_string.right(keymap_string.size() - last_space_index - 1).toStdWString();
        command_strings.push_back(command_name);
        command_keys.push_back(mapping);
        command_files.push_back(additional_keymap.file_name);
        command_line_numbers.push_back(additional_keymap.line_number);
    }

    return parse_lines(command_manager, command_keys, command_strings, command_files, command_line_numbers);
}


InputHandler::InputHandler(const Path& default_path, const std::vector<Path>& user_paths, CommandManager* cm) {
    command_manager = cm;
    user_key_paths = user_paths;
    reload_config_files(default_path, user_paths);
}

void InputHandler::reload_config_files(const Path& default_config_path, const std::vector<Path>& user_config_paths)
{
    delete_current_parse_tree(root);

    root = parse_key_config_files(command_manager, default_config_path, user_config_paths);
    current_node = root;
}


bool is_digit(int key) {
    return key >= Qt::Key::Key_0 && key <= Qt::Key::Key_9;
}

std::unique_ptr<Command> InputHandler::get_menu_command(MainWidget* w, QKeyEvent* key_event, bool shift_pressed, bool control_pressed, bool alt_pressed) {
    // get the command for keyevent while we are in a menu. In menus we don't
    // support key melodies so we just check the children of the root if they match
    int key = get_event_key(key_event, &shift_pressed, &control_pressed, &alt_pressed);
    for (auto child : root->children) {
        if (child->is_final && child->matches(key, shift_pressed, control_pressed, alt_pressed)){
            if (child->generator.has_value()) {
                return child->generator.value()(w);
            }
        }
    }

    return {};
}

int InputHandler::get_event_key(QKeyEvent* key_event, bool* shift_pressed, bool* control_pressed, bool* alt_pressed) {
    int key = 0;
    if (!USE_LEGACY_KEYBINDS) {
        std::vector<QString> special_texts = { "\b", "\t", " ", "\r", "\n" };
        if (((key_event->key() >= 'A') && (key_event->key() <= 'Z')) || ((key_event->text().size() > 0) &&
            (std::find(special_texts.begin(), special_texts.end(), key_event->text()) == special_texts.end()))) {
            if (!(*control_pressed) && !(*alt_pressed)) {
                // shift is already handled in the returned text
                *shift_pressed = false;
                std::wstring text = key_event->text().toStdWString();
                key = key_event->text().toStdWString()[0];
            }
            else {
                auto text = key_event->text();
                key = key_event->key();

                if ((key >= 'A' && key <= 'Z') && (!*shift_pressed)) {
                    if (!*shift_pressed) {
                        key = key - 'A' + 'a';
                    }
                }
            }
        }
        else {
            key = key_event->key();

            if (key == Qt::Key::Key_Backtab) {
                key = Qt::Key::Key_Tab;
            }
        }
    }
    else {
        key = key_event->key();
        if (key >= 'A' && key <= 'Z') {
            key = key - 'A' + 'a';
        }

    }
    return key;
}

std::unique_ptr<Command> InputHandler::handle_key(MainWidget* w, QKeyEvent* key_event, bool shift_pressed, bool control_pressed, bool alt_pressed, int* num_repeats) {

    int key = get_event_key(key_event, &shift_pressed, &control_pressed, &alt_pressed);
    if (current_node == root && is_digit(key)) {
        if (!(key == '0' && (number_stack.size() == 0)) && (!control_pressed) && (!shift_pressed) && (!alt_pressed)) {
            number_stack.push_back('0' + key - Qt::Key::Key_0);
            return nullptr;
        }
    }

    for (InputParseTreeNode* child : current_node->children) {
        //if (child->command == key && child->shift_modifier == shift_pressed && child->control_modifier == control_pressed){
        if (child->matches(key, shift_pressed, control_pressed, alt_pressed)) {
            if (child->is_final == true) {
                current_node = root;
                //cout << child->name << endl;
                *num_repeats = 0;
                if (number_stack.size() > 0) {
                    *num_repeats = atoi(number_stack.c_str());
                    number_stack.clear();
                }

                //return command_manager.get_command_with_name(child->name);
                if (child->generator.has_value()) {
                    return (child->generator.value())(w);
                }
                return nullptr;
                //for (size_t i = 0; i < child->name.size(); i++) {
                //	res.push_back(command_manager->get_command_with_name(child->name[i]));
                //}
                //return res;
            }
            else {
                current_node = child;
                return nullptr;
            }
        }
    }
    LOG(std::wcerr << "Warning: invalid command (key:" << (char)key << "); resetting to root" << std::endl);
    number_stack.clear();
    current_node = root;
    return nullptr;
}

void InputHandler::delete_current_parse_tree(InputParseTreeNode* node_to_delete)
{
    bool is_root = false;

    if (node_to_delete != nullptr) {
        is_root = node_to_delete->is_root;

        for (size_t i = 0; i < node_to_delete->children.size(); i++) {
            delete_current_parse_tree(node_to_delete->children[i]);
        }
        delete node_to_delete;
    }

    if (is_root) {
        root = nullptr;
    }
}

bool InputParseTreeNode::is_same(const InputParseTreeNode* other) {
    return (command == other->command) &&
        (shift_modifier == other->shift_modifier) &&
        (control_modifier == other->control_modifier) &&
        (alt_modifier == other->alt_modifier) &&
        (requires_symbol == other->requires_symbol) &&
        (requires_text == other->requires_text);
}

bool InputParseTreeNode::matches(int key, bool shift, bool ctrl, bool alt)
{
    return (key == this->command) && (shift == this->shift_modifier) && (ctrl == this->control_modifier) && (alt == this->alt_modifier);
}

std::optional<Path> InputHandler::get_or_create_user_keys_path() {
    if (user_key_paths.size() == 0) {
        return {};
    }

    for (int i = user_key_paths.size() - 1; i >= 0; i--) {
        if (user_key_paths[i].file_exists()) {
            return user_key_paths[i];
        }
    }
    user_key_paths.back().file_parent().create_directories();
    create_file_if_not_exists(user_key_paths.back().get_path());
    return user_key_paths.back();
}

std::unordered_map<std::string, std::vector<std::string>> InputHandler::get_command_key_mappings() const {
    std::unordered_map<std::string, std::vector<std::string>> res;
    std::vector<InputParseTreeNode*> prefix;
    add_command_key_mappings(root, res, prefix);
    return res;
}

void InputHandler::add_command_key_mappings(InputParseTreeNode* thisroot,
    std::unordered_map<std::string, std::vector<std::string>>& map,
    std::vector<InputParseTreeNode*> prefix) const {

    if (thisroot->is_final) {
        if (thisroot->name_.size() == 1) {
            map[thisroot->name_[0]].push_back(get_key_string_from_tree_node_sequence(prefix));
        }
        else if (thisroot->name_.size() > 1) {
            for (const auto& name : thisroot->name_) {
                map[name].push_back("{" + get_key_string_from_tree_node_sequence(prefix) + "}");
            }
        }
    }
    else {
        for (size_t i = 0; i < thisroot->children.size(); i++) {
            prefix.push_back(thisroot->children[i]);
            add_command_key_mappings(thisroot->children[i], map, prefix);
            prefix.pop_back();
        }

    }
}

std::string InputHandler::get_key_name_from_key_code(int key_code) const {
    std::string result;
    std::map<int, std::string> keymap = {
        {Qt::Key::Key_Up, "up"},
        {Qt::Key::Key_Down, "down"},
        {Qt::Key::Key_Left, "left"},
        {Qt::Key::Key_Right, "right"},
        {Qt::Key::Key_Backspace, "backspace"},
        {Qt::Key::Key_Space, "space"},
        {Qt::Key::Key_PageUp, "pageup"},
        {Qt::Key::Key_PageDown, "pagedown"},
        {Qt::Key::Key_Home, "home"},
        {Qt::Key::Key_End, "end"},
        {Qt::Key::Key_Tab, "tab"},
        {Qt::Key::Key_Backtab, "tab"},
    };

    //if (((key_code <= 'z') && (key_code >= 'a')) || ((key_code <= 'Z') && (key_code >= 'A'))) {
    if (key_code < 127) {
        result.push_back(key_code);
        return result;
    }
    else if (keymap.find(key_code) != keymap.end()) {
        return "<" + keymap[key_code] + ">";
    }
    else if ((key_code >= Qt::Key::Key_F1) && (key_code <= Qt::Key::Key_F35)) {
        int f_number = 1 + (key_code - Qt::Key::Key_F1);
        return "<f" + QString::number(f_number).toStdString() + ">";
    }
    else {
        return "UNK";
    }
}

std::string InputHandler::get_key_string_from_tree_node_sequence(const std::vector<InputParseTreeNode*> seq) const {
    std::string res;
    for (size_t i = 0; i < seq.size(); i++) {
        if (seq[i]->alt_modifier || seq[i]->shift_modifier || seq[i]->control_modifier) {
            res += "<";
        }
        std::string current_key_command_name = get_key_name_from_key_code(seq[i]->command);

        if (seq[i]->alt_modifier) {
            res += "A-";
        }
        if (seq[i]->control_modifier) {
            res += "C-";
        }
        if (seq[i]->shift_modifier) {
            res += "S-";
        }
        res += current_key_command_name;
        if (seq[i]->alt_modifier || seq[i]->shift_modifier || seq[i]->control_modifier) {
            res += ">";
        }
    }
    return res;
}
std::vector<Path> InputHandler::get_all_user_keys_paths() {
    std::vector<Path> res;

    for (int i = user_key_paths.size() - 1; i >= 0; i--) {
        if (user_key_paths[i].file_exists()) {
            res.push_back(user_key_paths[i]);
        }
    }

    return res;
}

std::optional<Requirement> Command::next_requirement(MainWidget* widget) {
    return {};
}

std::optional<std::wstring> Command::get_result() {
    return result;
}

void Command::set_text_requirement(std::wstring value) {}
void Command::set_symbol_requirement(char value) {}
void Command::set_file_requirement(std::wstring value) {}
void Command::set_rect_requirement(AbsoluteRect value) {}
void Command::set_point_requirement(AbsoluteDocumentPos value) {}

std::wstring Command::get_text_default_value() {
    return L"";
}

bool Command::pushes_state() {
    return false;
}

bool Command::requires_document() {
    return true;
}

void Command::set_num_repeats(int nr) {
    num_repeats = nr;
}

void Command::pre_perform() {

}

void Command::on_cancel() {

}

void Command::run() {
    if (this->requires_document() && !(widget->main_document_view_has_document())) {
        return;
    }
    widget->add_command_being_performed(this);
    perform();
    widget->validate_render();
    on_result_computed();
    widget->remove_command_being_performed(this);
}

std::vector<char> Command::special_symbols() {
    std::vector<char> res;
    return res;
}


std::string Command::get_name() {
    return command_cname;
}


bool is_macro_command_enabled(Command* command) {
    MacroCommand* macro_command = dynamic_cast<MacroCommand*>(command);
    if (macro_command) {
        return macro_command->is_enabled();
    }

    return false;
}

std::unique_ptr<Command> CommandManager::create_macro_command(MainWidget* w, std::string name, std::wstring macro_string) {
    return std::make_unique<MacroCommand>(w, this, name, macro_string);
}

void Command::on_result_computed() {

    if (dynamic_cast<MacroCommand*>(this)) {
        return;
    }
    if (dynamic_cast<LazyCommand*>(this)) {
        return;
    }
    if (result_socket && result_socket->isOpen()){
        if (!result.has_value()) {
            result_socket->write("<NULL>");
            return;
        }
        std::string result_str = utf8_encode(result.value());
        if (result_str.size() > 0) {
            result_socket->write(result_str.c_str());
        }
        else {
            result_socket->write("<NULL>");
        }
    }
    if (is_done) {
        if (result) {
            *result_holder = result.value();
        }
        else {
            *result_holder = L"";
        }

        *is_done = true;
    }

}
