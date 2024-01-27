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
extern std::map<std::wstring, std::pair<std::wstring, std::wstring>> ADDITIONAL_JAVASCRIPT_COMMANDS;
extern std::map<std::wstring, std::pair<std::wstring, std::wstring>> ADDITIONAL_ASYNC_JAVASCRIPT_COMMANDS;
extern std::map<std::wstring, std::wstring> ADDITIONAL_MACROS;
extern std::wstring SEARCH_URLS[26];
extern bool ALPHABETIC_LINK_TAGS;
extern std::vector<AdditionalKeymapData> ADDITIONAL_KEYMAPS;

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

Command::Command(MainWidget* widget_) : widget(widget_) {

}

Command::~Command() {

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
    GenericPathCommand(MainWidget* w) : Command(w) {};

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
    GenericGotoLocationCommand(MainWidget* w) : Command(w) {};

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
    GenericPathAndLocationCommadn(MainWidget* w, bool is_hash_ = false) : Command(w) { is_hash = is_hash_; };

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
    SymbolCommand(MainWidget* w) : Command(w) {}
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

    TextCommand(MainWidget* w) : Command(w) {}

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

    GotoMark(MainWidget* w) : SymbolCommand(w) {}

    bool pushes_state() {
        return true;
    }

    std::string get_name() {
        return "goto_mark";
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
    SetMark(MainWidget* w) : SymbolCommand(w) {}

    std::string get_name() {
        return "set_mark";
    }


    void perform() {
        assert(this->symbol != 0);
        widget->set_mark_in_current_location(this->symbol);
    }
};

class ToggleDrawingMask : public SymbolCommand {
public:
    ToggleDrawingMask(MainWidget* w) : SymbolCommand(w) {}

    std::string get_name() {
        return "toggle_drawing_mask";
    }

    void perform() {
        widget->handle_toggle_drawing_mask(this->symbol);
    }
};

class GotoLoadedDocumentCommand : public GenericPathCommand {
public:
    GotoLoadedDocumentCommand(MainWidget* w) : GenericPathCommand(w) {}

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
        return "goto_tab";
    }

};

class NextItemCommand : public Command {
public:
    NextItemCommand(MainWidget* w) : Command(w) {}

    void perform() {
        if (num_repeats == 0) num_repeats++;
        widget->goto_search_result(num_repeats);
    }

    std::string get_name() {
        return "next_item";
    }

};

class PrevItemCommand : public Command {
public:
    PrevItemCommand(MainWidget* w) : Command(w) {};

    void perform() {
        if (num_repeats == 0) num_repeats++;
        widget->goto_search_result(-num_repeats);
    }

    std::string get_name() {
        return "previous_item";
    }

};

class ToggleTextMarkCommand : public Command {
public:
    ToggleTextMarkCommand(MainWidget* w) : Command(w) {};

    void perform() {
        //if (num_repeats == 0) num_repeats++;
        widget->handle_toggle_text_mark();
        //widget->invalidate_render();
    }

    std::string get_name() {
        return "toggle_text_mark";
    }

};

class MoveTextMarkForwardCommand : public Command {
public:
    MoveTextMarkForwardCommand(MainWidget* w) : Command(w) {};

    void perform() {
        //if (num_repeats == 0) num_repeats++;
        widget->handle_move_text_mark_forward(false);
        //widget->invalidate_render();
    }

    std::string get_name() {
        return "move_text_mark_forward";
    }

};

class MoveTextMarkDownCommand : public Command {
public:
    MoveTextMarkDownCommand(MainWidget* w) : Command(w) {};

    void perform() {
        widget->handle_move_text_mark_down();
    }

    std::string get_name() {
        return "move_text_mark_down";
    }

};

class MoveTextMarkUpCommand : public Command {
public:
    MoveTextMarkUpCommand(MainWidget* w) : Command(w) {};

    void perform() {
        widget->handle_move_text_mark_up();
    }

    std::string get_name() {
        return "move_text_mark_up";
    }

};

class MoveTextMarkForwardWordCommand : public Command {
public:
    MoveTextMarkForwardWordCommand(MainWidget* w) : Command(w) {};

    void perform() {
        widget->handle_move_text_mark_forward(true);
    }

    std::string get_name() {
        return "move_text_mark_forward_word";
    }

};

class MoveTextMarkBackwardCommand : public Command {
public:
    MoveTextMarkBackwardCommand(MainWidget* w) : Command(w) {};

    void perform() {
        widget->handle_move_text_mark_backward(false);
    }

    std::string get_name() {
        return "move_text_mark_backward";
    }

};

class MoveTextMarkBackwardWordCommand : public Command {
public:
    MoveTextMarkBackwardWordCommand(MainWidget* w) : Command(w) {};

    void perform() {
        widget->handle_move_text_mark_backward(true);
    }

    std::string get_name() {
        return "move_text_mark_backward_word";
    }

};

class StartReadingCommand : public Command {
public:
    StartReadingCommand(MainWidget* w) : Command(w) {};

    void perform() {
        widget->handle_start_reading();
    }

    std::string get_name() {
        return "start_reading";
    }

};

class StopReadingCommand : public Command {
public:
    StopReadingCommand(MainWidget* w) : Command(w) {};

    void perform() {
        widget->handle_stop_reading();
    }

    std::string get_name() {
        return "stop_reading";
    }

};

class SearchCommand : public TextCommand {
public:
    SearchCommand(MainWidget* w) : TextCommand(w) {
    };

    void perform() {
        widget->perform_search(this->text.value(), false);
        if (TOUCH_MODE) {
            widget->show_search_buttons();
        }
    }

    std::string get_name() {
        return "search";
    }

    bool pushes_state() {
        return true;
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

    DownloadClipboardUrlCommand(MainWidget* w) : Command(w) {
    };

    void perform() {
        auto clipboard = QGuiApplication::clipboard();
        std::wstring url = clipboard->text().toStdWString();
        widget->download_paper_with_url(url, false, PaperDownloadFinishedAction::OpenInNewWindow);
    }

    std::string get_name() {
        return "download_clipboard_url";
    }

    std::string text_requirement_name() {
        return "Paper Url";
    }

};
class DownloadPaperWithUrlCommand : public TextCommand {
public:

    DownloadPaperWithUrlCommand(MainWidget* w) : TextCommand(w) {
    };

    void perform() {
        widget->download_paper_with_url(text.value(), false, PaperDownloadFinishedAction::OpenInNewWindow);
    }

    std::string get_name() {
        return "download_paper_with_url";
    }

    std::string text_requirement_name() {
        return "Paper Url";
    }

};

class ControlMenuCommand : public TextCommand {
public:
    ControlMenuCommand(MainWidget* w) : TextCommand(w) {
    };


    void perform() {
        QString res = widget->handle_action_in_menu(text.value());
        result = res.toStdWString();
    }

    std::string get_name() {
        return "control_menu";
    }

    std::string text_requirement_name() {
        return "Action";
    }

};

class ExecuteMacroCommand : public TextCommand {
public:
    ExecuteMacroCommand(MainWidget* w) : TextCommand(w) {
    };

    void perform() {
        widget->execute_macro_if_enabled(text.value());
    }

    std::string get_name() {
        return "execute_macro";
    }

    std::string text_requirement_name() {
        return "Macro";
    }

};

class SetViewStateCommand : public TextCommand {
public:
    SetViewStateCommand(MainWidget* w) : TextCommand(w) {
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

    std::string get_name() {
        return "set_view_state";
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
    TestCommand(MainWidget* w) : Command(w) {};

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

    std::string get_name() {
        return "test_command";
    }

};

class GetConfigNoDialogCommand : public Command {
    std::optional<std::wstring> command_name = {};
public:
    GetConfigNoDialogCommand(MainWidget* w) : Command(w) {};

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

    std::string get_name() {
        return "get_config_no_dialog";
    }

};

class ShowTextPromptCommand : public Command {

    std::optional<std::wstring> prompt_title = {};
    std::optional<std::wstring> default_value = {};
public:
    ShowTextPromptCommand(MainWidget* w) : Command(w) {};

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

    std::string get_name() {
        return "show_text_prompt";
    }

};

class GetStateJsonCommand : public Command {

public:
    GetStateJsonCommand(MainWidget* w) : Command(w) {};

    void perform() {
        QJsonDocument doc(widget->get_all_json_states());
        std::wstring json_str = utf8_decode(doc.toJson(QJsonDocument::Compact).toStdString());
        result = json_str;
    }

    std::string get_name() {
        return "get_state_json";
    }

};

class GetPaperNameCommand : public Command {

public:
    GetPaperNameCommand(MainWidget* w) : Command(w) {};

    void perform() {
        result = widget->doc()->detect_paper_name();
    }

    std::string get_name() {
        return "get_paper_name";
    }

};

class GetOverviewPaperName : public Command {

public:
    GetOverviewPaperName(MainWidget* w) : Command(w) {};

    void perform() {
        std::optional<std::wstring> paper_name = widget->get_overview_paper_name();
        result = paper_name.value_or(L"");
    }

    std::string get_name() {
        return "get_overview_paper_name";
    }

};

class ToggleRectHintsCommand : public Command {

public:
    ToggleRectHintsCommand(MainWidget* w) : Command(w) {};

    void perform() {
        widget->toggle_rect_hints();
    }

    std::string get_name() {
        return "toggle_rect_hints";
    }

};

class GetAnnotationsJsonCommand : public Command {

public:
    GetAnnotationsJsonCommand(MainWidget* w) : Command(w) {};

    void perform() {
        QJsonDocument doc(widget->get_json_annotations());
        std::wstring json_str = utf8_decode(doc.toJson(QJsonDocument::Compact).toStdString());
        result = json_str;
    }

    std::string get_name() {
        return "get_annotations_json";
    }

};

class ShowOptionsCommand : public Command {

private:
    std::vector<std::wstring> options;
    std::optional<std::wstring> selected_option;

public:
    ShowOptionsCommand(MainWidget* w) : Command(w) {};

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

    std::string get_name() {
        return "show_custom_options";
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
    GetConfigCommand(MainWidget* w) : TextCommand(w) {};

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

    std::string get_name() {
        return "get_config_value";
    }

    std::string text_requirement_name() {
        return "Config Name";
    }

};

class DownloadPaperWithNameCommand : public TextCommand {
public:
    DownloadPaperWithNameCommand(MainWidget* w) : TextCommand(w) {};

    void perform() {
        widget->download_paper_with_name(text.value(), PaperDownloadFinishedAction::OpenInNewWindow);
    }

    std::string get_name() {
        return "download_paper_with_name";
    }

    std::string text_requirement_name() {
        return "Paper Name";
    }

};

class AddAnnotationToSelectedHighlightCommand : public TextCommand {
public:
    AddAnnotationToSelectedHighlightCommand(MainWidget* w) : TextCommand(w) {};

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

    std::string get_name() {
        return "add_annot_to_selected_highlight";
    }


    std::string text_requirement_name() {
        return "Comment";
    }
};

class RenameCommand : public TextCommand {
public:
    RenameCommand(MainWidget* w) : TextCommand(w) {};

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

    std::string get_name() {
        return "rename";
    }


    std::string text_requirement_name() {
        return "New Name";
    }
};

class SetFreehandThickness : public TextCommand {
public:
    SetFreehandThickness(MainWidget* w) : TextCommand(w) {};

    void perform() {
        float thickness = QString::fromStdWString(this->text.value()).toFloat();
        widget->set_freehand_thickness(thickness);
        //widget->perform_search(this->text.value(), false);
        //if (TOUCH_MODE) {
        //	widget->show_search_buttons();
        //}
    }

    std::string get_name() {
        return "set_freehand_thickness";
    }



    std::string text_requirement_name() {
        return "Thickness";
    }
};

class GotoPageWithLabel : public TextCommand {
public:
    GotoPageWithLabel(MainWidget* w) : TextCommand(w) {};

    void perform() {
        widget->goto_page_with_label(text.value());
    }

    std::string get_name() {
        return "goto_page_with_label";
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
    ChapterSearchCommand(MainWidget* w) : TextCommand(w) {};

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

    std::string get_name() {
        return "chapter_search";
    }


    std::string text_requirement_name() {
        return "Search Term";
    }
};

class RegexSearchCommand : public TextCommand {
public:
    RegexSearchCommand(MainWidget* w) : TextCommand(w) {};

    void perform() {
        widget->perform_search(this->text.value(), true);
    }

    std::string get_name() {
        return "regex_search";
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
    AddBookmarkCommand(MainWidget* w) : TextCommand(w) {}

    void perform() {
        std::string uuid = widget->main_document_view->add_bookmark(text.value());
        result = utf8_decode(uuid);
    }

    std::string get_name() {
        return "add_bookmark";
    }


    std::string text_requirement_name() {
        return "Bookmark Text";
    }
};

class AddBookmarkMarkedCommand : public Command {

public:

    std::optional<std::wstring> text_;
    std::optional<AbsoluteDocumentPos> point_;

    AddBookmarkMarkedCommand(MainWidget* w) : Command(w) {};

    std::optional<Requirement> next_requirement(MainWidget* widget) {

        if (!text_.has_value()) {
            Requirement req = { RequirementType::Text, "Bookmark Text" };
            return req;
        }
        if (!point_.has_value()) {
            Requirement req = { RequirementType::Point, "Bookmark Location" };
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

    std::string get_name() {
        return "add_marked_bookmark";
    }
};


class CreateVisiblePortalCommand : public Command {

public:

    std::optional<AbsoluteDocumentPos> point_;

    CreateVisiblePortalCommand(MainWidget* w) : Command(w) {};

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

    std::string get_name() {
        return "create_visible_portal";
    }

};

class CopyDrawingsFromScratchpadCommand : public Command {

public:

    std::optional<AbsoluteRect> rect_;

    CopyDrawingsFromScratchpadCommand(MainWidget* w) : Command(w) {};

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

    std::string get_name() {
        return "copy_drawings_from_scratchpad";
    }

};

class CopyScreenshotToScratchpad : public Command {

public:

    std::optional<AbsoluteRect> rect_;

    CopyScreenshotToScratchpad(MainWidget* w) : Command(w) {};

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

    std::string get_name() {
        return "copy_screenshot_to_scratchpad";
    }

};
class AddBookmarkFreetextCommand : public Command {

public:

    std::optional<std::wstring> text_;
    std::optional<AbsoluteRect> rect_;
    int pending_index = -1;

    AddBookmarkFreetextCommand(MainWidget* w) : Command(w) {};

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
        widget->selected_bookmark_index = pending_index;
    }


    void on_cancel() {

        if (pending_index != -1) {
            widget->doc()->undo_pending_bookmark(pending_index);
        }
    }

    void perform() {
        //widget->doc()->add_freetext_bookmark(text_.value(), rect_.value());
        std::string uuid = widget->doc()->add_pending_freetext_bookmark(pending_index, text_.value());
        widget->clear_selected_rect();
        widget->invalidate_render();
        result = utf8_decode(uuid);
    }

    std::string get_name() {
        return "add_freetext_bookmark";
    }

};

class GotoBookmarkCommand : public GenericGotoLocationCommand {
public:
    GotoBookmarkCommand(MainWidget* w) : GenericGotoLocationCommand(w) {};

    void handle_generic_requirement() {
        widget->handle_goto_bookmark();
    }

    std::string get_name() {
        return "goto_bookmark";
    }

};

class GotoPortalListCommand : public GenericGotoLocationCommand {
public:
    GotoPortalListCommand(MainWidget* w) : GenericGotoLocationCommand(w) {};

    void handle_generic_requirement() {
        widget->handle_goto_portal_list();
    }

    std::string get_name() {
        return "goto_portal_list";
    }

};

class GotoBookmarkGlobalCommand : public GenericPathAndLocationCommadn {
public:

    GotoBookmarkGlobalCommand(MainWidget* w) : GenericPathAndLocationCommadn(w) {};

    void handle_generic_requirement() {
        widget->handle_goto_bookmark_global();
    }


    std::string get_name() {
        return "goto_bookmark_g";
    }


    bool requires_document() { return false; }
};

class IncreaseFreetextBookmarkFontSizeCommand : public Command {
public:
    IncreaseFreetextBookmarkFontSizeCommand(MainWidget* w) : Command(w) {};

    void perform() {
        FREETEXT_BOOKMARK_FONT_SIZE *= 1.1f;
        if (FREETEXT_BOOKMARK_FONT_SIZE > 100) {
            FREETEXT_BOOKMARK_FONT_SIZE = 100;
        }
        widget->update_selected_bookmark_font_size();

    }

    std::string get_name() {
        return "increase_freetext_font_size";
    }

};

class DecreaseFreetextBookmarkFontSizeCommand : public Command {
public:
    DecreaseFreetextBookmarkFontSizeCommand(MainWidget* w) : Command(w) {};

    void perform() {
        FREETEXT_BOOKMARK_FONT_SIZE /= 1.1f;
        if (FREETEXT_BOOKMARK_FONT_SIZE < 1) {
            FREETEXT_BOOKMARK_FONT_SIZE = 1;
        }
        widget->update_selected_bookmark_font_size();
    }

    std::string get_name() {
        return "decrease_freetext_font_size";
    }

};


class GotoHighlightCommand : public GenericGotoLocationCommand {
public:
    GotoHighlightCommand(MainWidget* w) : GenericGotoLocationCommand(w) {};

    void handle_generic_requirement() {
        widget->handle_goto_highlight();
    }

    std::string get_name() {
        return "goto_highlight";
    }

};


class GotoHighlightGlobalCommand : public GenericPathAndLocationCommadn {
public:

    GotoHighlightGlobalCommand(MainWidget* w) : GenericPathAndLocationCommadn(w) {};

    void handle_generic_requirement() {
        widget->handle_goto_highlight_global();
    }

    std::string get_name() {
        return "goto_highlight_g";
    }

    bool requires_document() { return false; }
};

class GotoTableOfContentsCommand : public GenericPathCommand {
public:
    GotoTableOfContentsCommand(MainWidget* w) : GenericPathCommand(w) {};

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

    std::string get_name() {
        return "goto_toc";
    }

};

class PortalCommand : public Command {
public:
    PortalCommand(MainWidget* w) : Command(w) {};

    void perform() {
        widget->handle_portal();
    }

    std::string get_name() {
        return "portal";
    }

};

class ToggleWindowConfigurationCommand : public Command {
public:
    ToggleWindowConfigurationCommand(MainWidget* w) : Command(w) {};
    void perform() {
        widget->toggle_window_configuration();
    }

    std::string get_name() {
        return "toggle_window_configuration";
    }


    bool requires_document() { return false; }
};

class NextStateCommand : public Command {
public:
    NextStateCommand(MainWidget* w) : Command(w) {};

    void perform() {
        widget->next_state();
    }

    std::string get_name() {
        return "next_state";
    }


    bool requires_document() { return false; }
};

class PrevStateCommand : public Command {
public:
    PrevStateCommand(MainWidget* w) : Command(w) {};

    void perform() {
        widget->prev_state();
    }

    std::string get_name() {
        return "prev_state";
    }


    bool requires_document() { return false; }
};

class AddHighlightCommand : public SymbolCommand {
public:
    AddHighlightCommand(MainWidget* w) : SymbolCommand(w) {};

    void perform() {
        result = widget->handle_add_highlight(symbol);
    }

    std::vector<char> special_symbols() {
        std::vector<char> res = { '_', };
        return res;
    }

    std::string get_name() {
        return "add_highlight";
    }

};

class CommandPaletteCommand : public Command {
public:
    CommandPaletteCommand(MainWidget* w) : Command(w) {};

    void perform() {
        widget->show_command_palette();
    }

    std::string get_name() {
        return "command_palette";
    }


    bool requires_document() { return false; }
};

class CommandCommand : public Command {
public:
    CommandCommand(MainWidget* w) : Command(w) {};

    void perform() {
        QStringList command_names = widget->command_manager->get_all_command_names();
        if (!TOUCH_MODE) {

            widget->set_current_widget(new CommandSelector(
                FUZZY_SEARCHING, &widget->on_command_done, widget, command_names, widget->input_handler->get_command_key_mappings()));
        }
        else {

            TouchCommandSelector* tcs = new TouchCommandSelector(FUZZY_SEARCHING, command_names, widget);
            widget->set_current_widget(tcs);
        }

        widget->show_current_widget();

    }

    std::string get_name() {
        return "command";
    }


    bool requires_document() { return false; }
};

class ScreenshotCommand : public Command {
public:
    ScreenshotCommand(MainWidget* w) : Command(w) {};

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

    std::string get_name() {
        return "screenshot";
    }

    bool requires_document() { return false; }
};

class FramebufferScreenshotCommand : public Command {
public:
    FramebufferScreenshotCommand(MainWidget* w) : Command(w) {};

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

    std::string get_name() {
        return "framebuffer_screenshot";
    }

    bool requires_document() { return false; }
};

class ExportDefaultConfigFile : public Command {
public:
    ExportDefaultConfigFile(MainWidget* w) : Command(w) {};

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

    std::string get_name() {
        return "export_default_config_file";
    }

    bool requires_document() { return false; }
};
class ExportCommandNamesCommand : public Command {
public:
    ExportCommandNamesCommand(MainWidget* w) : Command(w) {};

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

    std::string get_name() {
        return "export_command_names";
    }

    bool requires_document() { return false; }
};



class ExportConfigNamesCommand : public Command {
public:
    ExportConfigNamesCommand(MainWidget* w) : Command(w) {};

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

    std::string get_name() {
        return "export_config_names";
    }

    bool requires_document() { return false; }
};

class GenericWaitCommand : public Command {
public:
    GenericWaitCommand(MainWidget* w) : Command(w) {};
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

class WaitForIndexingToFinishCommand : public GenericWaitCommand {
public:
    WaitForIndexingToFinishCommand(MainWidget* w) : GenericWaitCommand(w) {};

    void set_generic_requirement(QVariant value)
    {
        finished = true;
    }

    bool is_ready() override {
        return widget->is_index_ready();
    }

    std::string get_name() {
        return "wait_for_indexing_to_finish";
    }

};

class WaitForRendersToFinishCommand : public GenericWaitCommand {
public:
    WaitForRendersToFinishCommand(MainWidget* w) : GenericWaitCommand(w) {};

    bool is_ready() override {
        return widget->is_render_ready();
    }

    std::string get_name() {
        return "wait_for_renders_to_finish";
    }

};

class WaitForSearchToFinishCommand : public GenericWaitCommand {
public:
    WaitForSearchToFinishCommand(MainWidget* w) : GenericWaitCommand(w) {};

    bool is_ready() override {
        return widget->is_search_ready();
    }

    std::string get_name() {
        return "wait_for_search_to_finish";
    }

};

class OpenDocumentCommand : public Command {
public:
    OpenDocumentCommand(MainWidget* w) : Command(w) {};

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

    std::string get_name() {
        return "open_document";
    }


    bool requires_document() { return false; }
};

class MoveDownCommand : public Command {
public:
    MoveDownCommand(MainWidget* w) : Command(w) {};

    void perform() {
        int rp = num_repeats == 0 ? 1 : num_repeats;
        widget->handle_vertical_move(rp);
    }

    std::string get_name() {
        return "move_down";
    }


};

class MoveUpCommand : public Command {
public:
    MoveUpCommand(MainWidget* w) : Command(w) {};

    void perform() {
        int rp = num_repeats == 0 ? 1 : num_repeats;
        widget->handle_vertical_move(-rp);
    }

    std::string get_name() {

        return "move_up";
    }

};

class MoveLeftInOverviewCommand : public Command {
public:
    MoveLeftInOverviewCommand(MainWidget* w) : Command(w) {};
    void perform() {
        widget->scroll_overview(0, 1);
    }
    std::string get_name() {

        return "move_left_in_overview";
    }

};

class MoveRightInOverviewCommand : public Command {
public:
    MoveRightInOverviewCommand(MainWidget* w) : Command(w) {};
    void perform() {
        widget->scroll_overview(0, -1);
    }
    std::string get_name() {

        return "move_right_in_overview";
    }

};

class MoveLeftCommand : public Command {
public:
    MoveLeftCommand(MainWidget* w) : Command(w) {};
    void perform() {
        int rp = num_repeats == 0 ? 1 : num_repeats;
        widget->handle_horizontal_move(-rp);
    }
    std::string get_name() {

        return "move_left";
    }

};

class MoveRightCommand : public Command {
public:
    MoveRightCommand(MainWidget* w) : Command(w) {};
    void perform() {
        int rp = num_repeats == 0 ? 1 : num_repeats;
        widget->handle_horizontal_move(rp);
    }
    std::string get_name() {

        return "move_right";
    }

};

class JavascriptCommand : public Command {
public:
    std::string command_name;
    std::wstring code;
    bool is_async;

    JavascriptCommand(std::string command_name, std::wstring code_, bool is_async_, MainWidget* w) :  Command(w), command_name(command_name) {
        code = code_;
        is_async = is_async_;
    };

    void perform() {
        widget->run_javascript_command(code, is_async);
    }

    std::string get_name() {
        return command_name;
    }

};

class SaveScratchpadCommand : public Command {
public:
    SaveScratchpadCommand(MainWidget* w) : Command(w) {};
    void perform() {
        widget->save_scratchpad();
    }

    std::string get_name() {
        return "save_scratchpad";
    }

};

class LoadScratchpadCommand : public Command {
public:
    LoadScratchpadCommand(MainWidget* w) : Command(w) {};
    void perform() {
        widget->load_scratchpad();
    }

    std::string get_name() {
        return "load_scratchpad";
    }

};

class ClearScratchpadCommand : public Command {
public:
    ClearScratchpadCommand(MainWidget* w) : Command(w) {};
    void perform() {
        widget->clear_scratchpad();
    }

    std::string get_name() {
        return "clear_scratchpad";
    }

};

class ZoomInCommand : public Command {
public:
    ZoomInCommand(MainWidget* w) : Command(w) {};
    void perform() {
        widget->main_document_view->zoom_in();
        widget->last_smart_fit_page = {};
    }

    std::string get_name() {
        return "zoom_in";
    }

};

class ZoomOutOverviewCommand : public Command {
public:
    ZoomOutOverviewCommand(MainWidget* w) : Command(w) {};
    void perform() {
        widget->zoom_out_overview();
    }

    std::string get_name() {
        return "zoom_out_overview";
    }

};

class ZoomInOverviewCommand : public Command {
public:
    ZoomInOverviewCommand(MainWidget* w) : Command(w) {};
    void perform() {
        widget->zoom_in_overview();
    }

    std::string get_name() {
        return "zoom_in_overview";
    }

};

class FitToPageWidthCommand : public Command {
public:
    FitToPageWidthCommand(MainWidget* w) : Command(w) {};

    void perform() {
        widget->handle_fit_to_page_width(false);
    }

    std::string get_name() {
        return "fit_to_page_width";
    }

};

class FitToPageSmartCommand : public Command {
public:
    FitToPageSmartCommand(MainWidget* w) : Command(w) {};
    void perform() {
        widget->main_document_view->fit_to_page_height_and_width_smart();
    }

    std::string get_name() {
        return "fit_to_page_smart";
    }

};

class FitToPageWidthSmartCommand : public Command {
public:
    FitToPageWidthSmartCommand(MainWidget* w) : Command(w) {};
    void perform() {
        widget->handle_fit_to_page_width(true);
    }

    std::string get_name() {
        return "fit_to_page_width_smart";
    }

};

class FitToPageHeightCommand : public Command {
public:
    FitToPageHeightCommand(MainWidget* w) : Command(w) {};
    void perform() {
        widget->main_document_view->fit_to_page_height();
        widget->last_smart_fit_page = {};
    }

    std::string get_name() {
        return "fit_to_page_height";
    }

};

class FitToPageHeightSmartCommand : public Command {
public:
    FitToPageHeightSmartCommand(MainWidget* w) : Command(w) {};
    void perform() {
        widget->main_document_view->fit_to_page_height(true);
    }

    std::string get_name() {
        return "fit_to_page_height_smart";
    }

};

class NextPageCommand : public Command {
public:
    NextPageCommand(MainWidget* w) : Command(w) {};
    void perform() {
        widget->main_document_view->move_pages(std::max(1, num_repeats));
    }
    std::string get_name() {
        return "next_page";
    }

};

class PreviousPageCommand : public Command {
public:
    PreviousPageCommand(MainWidget* w) : Command(w) {};
    void perform() {
        widget->main_document_view->move_pages(std::min(-1, -num_repeats));
    }

    std::string get_name() {
        return "previous_page";
    }

};

class ZoomOutCommand : public Command {
public:
    ZoomOutCommand(MainWidget* w) : Command(w) {};

    void perform() {
        widget->main_document_view->zoom_out();
        widget->last_smart_fit_page = {};
    }

    std::string get_name() {
        return "zoom_out";
    }

};

class GotoDefinitionCommand : public Command {
public:
    GotoDefinitionCommand(MainWidget* w) : Command(w) {};
    void perform() {
        if (widget->main_document_view->goto_definition()) {
            widget->main_document_view->exit_ruler_mode();
        }
    }

    std::string get_name() {
        return "goto_definition";
    }


    bool pushes_state() {
        return true;
    }

};

class OverviewDefinitionCommand : public Command {
public:
    OverviewDefinitionCommand(MainWidget* w) : Command(w) {};
    void perform() {
        widget->overview_to_definition();
    }

    std::string get_name() {
        return "overview_definition";
    }

};

class PortalToDefinitionCommand : public Command {
public:
    PortalToDefinitionCommand(MainWidget* w) : Command(w) {};
    void perform() {
        widget->portal_to_definition();
    }

    std::string get_name() {
        return "portak_to_definition";
    }

};

class MoveVisualMarkDownCommand : public Command {
public:
    MoveVisualMarkDownCommand(MainWidget* w) : Command(w) {};
    void perform() {
        int rp = num_repeats == 0 ? 1 : num_repeats;
        widget->move_visual_mark_command(rp);
    }

    std::string get_name() {
        return "move_visual_mark_down";
    }

};

class MoveVisualMarkUpCommand : public Command {
public:
    MoveVisualMarkUpCommand(MainWidget* w) : Command(w) {};
    void perform() {
        int rp = num_repeats == 0 ? 1 : num_repeats;
        widget->move_visual_mark_command(-rp);
    }

    std::string get_name() {
        return "move_visual_mark_up";
    }

};

class MoveVisualMarkNextCommand : public Command {
public:
    MoveVisualMarkNextCommand(MainWidget* w) : Command(w) {};
    void perform() {
        widget->move_visual_mark_next();
    }

    std::string get_name() {
        return "move_visual_mark_next";
    }

};

class MoveVisualMarkPrevCommand : public Command {
public:
    MoveVisualMarkPrevCommand(MainWidget* w) : Command(w) {};
    void perform() {
        widget->move_visual_mark_prev();
    }

    std::string get_name() {
        return "move_visual_mark_prev";
    }

};


class GotoPageWithPageNumberCommand : public TextCommand {
public:
    GotoPageWithPageNumberCommand(MainWidget* w) : TextCommand(w) {};

    void perform() {
        std::wstring text_ = text.value();
        if (is_string_numeric(text_.c_str()) && text_.size() < 6) { // make sure the page number is valid
            int dest = std::stoi(text_.c_str()) - 1;
            widget->main_document_view->goto_page(dest + widget->main_document_view->get_page_offset());
        }
    }

    std::string get_name() {
        return "goto_page_with_page_number";
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
    std::wstring initial_text;
    float initial_font_size;
    int index = -1;

    EditSelectedBookmarkCommand(MainWidget* w) : TextCommand(w) {};

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
        else {
            show_error_message(L"No bookmark is selected");
        }
    }

    void on_cancel() {
        if (index > -1) {
            widget->doc()->get_bookmarks()[index].description = initial_text;
            widget->doc()->get_bookmarks()[index].font_size = initial_font_size;
        }
    }

    void perform() {
        std::wstring text_ = text.value();
        widget->change_selected_bookmark_text(text_);
        widget->invalidate_render();
    }

    std::string get_name() {
        return "edit_selected_bookmark";
    }


    std::string text_requirement_name() {
        return "Bookmark Text";
    }

};

class EditSelectedHighlightCommand : public TextCommand {
public:
    int index = -1;

    EditSelectedHighlightCommand(MainWidget* w) : TextCommand(w) {};

    void pre_perform() {
        index = widget->selected_bookmark_index;

        widget->set_text_prompt_text(
            QString::fromStdWString(widget->doc()->get_highlights()[widget->selected_highlight_index].text_annot));
        //widget->text_command_line_edit->setText(
        //    QString::fromStdWString(widget->doc()->get_highlights()[widget->selected_highlight_index].text_annot)
        //);
    }

    void perform() {
        std::wstring text_ = text.value();
        widget->change_selected_highlight_text_annot(text_);
    }

    std::string get_name() {
        return "edit_selected_highlight";
    }


    std::string text_requirement_name() {
        return "Highlight Annotation";
    }

};

class DeletePortalCommand : public Command {
public:
    DeletePortalCommand(MainWidget* w) : Command(w) {};
    void perform() {
        widget->main_document_view->delete_closest_portal();
        widget->validate_render();
    }

    std::string get_name() {
        return "delete_portal";
    }

};

class DeleteBookmarkCommand : public Command {
public:
    DeleteBookmarkCommand(MainWidget* w) : Command(w) {};
    void perform() {
        widget->main_document_view->delete_closest_bookmark();
        widget->validate_render();
    }

    std::string get_name() {
        return "delete_bookmark";
    }

};

class GenericHighlightCommand : public Command {
    std::vector<int> visible_highlight_indices;
    std::string tag;
    int n_required_tags = 0;
    bool already_pre_performed = false;

public:
    GenericHighlightCommand(MainWidget* w) : Command(w) {};

    void pre_perform() override {
        if (!already_pre_performed) {
            if (widget->selected_highlight_index == -1) {
                visible_highlight_indices = widget->main_document_view->get_visible_highlight_indices();
                n_required_tags = get_num_tag_digits(visible_highlight_indices.size());

                widget->handle_highlight_tags_pre_perform(visible_highlight_indices);
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

    virtual void perform_with_highlight_selected() = 0;
    void perform() {
        bool should_clear_labels = false;
        if (tag.size() > 0) {
            int index = get_index_from_tag(tag);
            if (index < visible_highlight_indices.size()) {
                widget->set_selected_highlight_index(visible_highlight_indices[index]);
            }
            should_clear_labels = true;
        }

        perform_with_highlight_selected();
        //widget->handle_delete_selected_highlight();

        if (should_clear_labels) {
            widget->clear_keyboard_select_highlights();
        }
    }
};

class DeleteHighlightCommand : public GenericHighlightCommand {

public:
    DeleteHighlightCommand(MainWidget* w) : GenericHighlightCommand(w) {};

    void perform_with_highlight_selected() {

        widget->handle_delete_selected_highlight();
    }


    std::string get_name() {
        return "delete_highlight";
    }

};

class ChangeHighlightTypeCommand : public GenericHighlightCommand {

public:
    ChangeHighlightTypeCommand(MainWidget* w) : GenericHighlightCommand(w) {};

    void perform_with_highlight_selected() {
        widget->execute_macro_if_enabled(L"add_highlight");
    }


    std::string get_name() {
        return "change_highlight";
    }

};

class AddAnnotationToHighlightCommand : public GenericHighlightCommand {

public:
    AddAnnotationToHighlightCommand(MainWidget* w) : GenericHighlightCommand(w) {};

    void perform_with_highlight_selected() {
        widget->execute_macro_if_enabled(L"add_annot_to_selected_highlight");
    }


    std::string get_name() {
        return "add_annot_to_highlight";
    }

};

class GotoPortalCommand : public Command {
public:
    GotoPortalCommand(MainWidget* w) : Command(w) {};
    void perform() {
        std::optional<Portal> link = widget->main_document_view->find_closest_portal();
        if (link) {
            widget->open_document(link->dst);
        }
    }

    bool pushes_state() {
        return true;
    }

    std::string get_name() {
        return "goto_link";
    }

};

class EditPortalCommand : public Command {
public:
    EditPortalCommand(MainWidget* w) : Command(w) {};
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

    std::string get_name() {
        return "edit_portal";
    }

};

class OpenPrevDocCommand : public GenericPathAndLocationCommadn {
public:
    OpenPrevDocCommand(MainWidget* w) : GenericPathAndLocationCommadn(w, true) {};

    void handle_generic_requirement() {
        widget->handle_open_prev_doc();
    }

    std::string get_name() {
        return "open_prev_doc";
    }


    bool requires_document() { return false; }
};

class OpenAllDocsCommand : public GenericPathAndLocationCommadn {
public:
    OpenAllDocsCommand(MainWidget* w) : GenericPathAndLocationCommadn(w, true) {};

    void handle_generic_requirement() {
        widget->handle_open_all_docs();
    }

    std::string get_name() {
        return "open_all_docs";
    }


    bool requires_document() { return false; }
};


class OpenDocumentEmbeddedCommand : public GenericPathCommand {
public:
    OpenDocumentEmbeddedCommand(MainWidget* w) : GenericPathCommand(w) {};

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

    std::string get_name() {
        return "open_document_embedded";
    }


    bool requires_document() { return false; }
};

class OpenDocumentEmbeddedFromCurrentPathCommand : public GenericPathCommand {
public:
    OpenDocumentEmbeddedFromCurrentPathCommand(MainWidget* w) : GenericPathCommand(w) {};

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

    std::string get_name() {
        return "open_document_embedded_from_current_path";
    }


    bool requires_document() { return false; }
};

class CopyCommand : public Command {
public:
    CopyCommand(MainWidget* w) : Command(w) {};
    void perform() {
        copy_to_clipboard(widget->get_selected_text());
    }

    std::string get_name() {
        return "copy";
    }

};

class GotoBeginningCommand : public Command {
public:
    GotoBeginningCommand(MainWidget* w) : Command(w) {};
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

    std::string get_name() {
        return "goto_beginning";
    }

};

class GotoEndCommand : public Command {
public:
    GotoEndCommand(MainWidget* w) : Command(w) {};
public:
    void perform() {
        widget->main_document_view->goto_end();
    }

    bool pushes_state() {
        return true;
    }

    std::string get_name() {
        return "goto_end";
    }

};

class OverviewRulerPortalCommand : public Command {
public:
    OverviewRulerPortalCommand(MainWidget* w) : Command(w) {};
public:
    void perform() {
        widget->handle_overview_to_ruler_portal();
    }

    std::string get_name() {
        return "overview_to_ruler_portal";
    }

};

class GotoRulerPortalCommand : public Command {
public:
    GotoRulerPortalCommand(MainWidget* w) : Command(w) {};
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

    std::string get_name() {
        return "goto_ruler_portal";
    }

};

class PrintNonDefaultConfigs : public Command {
public:
    PrintNonDefaultConfigs(MainWidget* w) : Command(w) {};
    void perform() {
        widget->print_non_default_configs();
    }
    std::string get_name() {
        return "print_non_default_configs";
    }


    bool requires_document() { return false; }
};

class PrintUndocumentedCommandsCommand : public Command {
public:
    PrintUndocumentedCommandsCommand(MainWidget* w) : Command(w) {};
    void perform() {
        widget->print_undocumented_commands();
    }
    std::string get_name() {
        return "print_undocumented_commands";
    }


    bool requires_document() { return false; }
};

class PrintUndocumentedConfigsCommand : public Command {
public:
    PrintUndocumentedConfigsCommand(MainWidget* w) : Command(w) {};
    void perform() {
        widget->print_undocumented_configs();
    }
    std::string get_name() {
        return "print_undocumented_configs";
    }


    bool requires_document() { return false; }
};

class ToggleFullscreenCommand : public Command {
public:
    ToggleFullscreenCommand(MainWidget* w) : Command(w) {};
    void perform() {
        widget->toggle_fullscreen();
    }
    std::string get_name() {
        return "toggle_fullscreen";
    }


    bool requires_document() { return false; }
};

class MaximizeCommand : public Command {
public:
    MaximizeCommand(MainWidget* w) : Command(w) {};
    void perform() {
        widget->maximize_window();
    }

    std::string get_name() {
        return "maximize";
    }


    bool requires_document() { return false; }
};

class ToggleOneWindowCommand : public Command {
public:
    ToggleOneWindowCommand(MainWidget* w) : Command(w) {};
    void perform() {
        widget->toggle_two_window_mode();
    }
    std::string get_name() {
        return "toggle_one_window";
    }


    bool requires_document() { return false; }
};

class ToggleHighlightCommand : public Command {
public:
    ToggleHighlightCommand(MainWidget* w) : Command(w) {};
    void perform() {
        widget->toggle_highlight_links();
    }
    std::string get_name() {
        return "toggle_highlight";
    }


    bool requires_document() { return false; }
};

class ToggleSynctexCommand : public Command {
public:
    ToggleSynctexCommand(MainWidget* w) : Command(w) {};
    void perform() {
        widget->toggle_synctex_mode();
    }
    std::string get_name() {
        return "toggle_synctex";
    }


    bool requires_document() { return false; }
};

class TurnOnSynctexCommand : public Command {
public:
    TurnOnSynctexCommand(MainWidget* w) : Command(w) {};
    void perform() {
        widget->set_synctex_mode(true);
    }
    std::string get_name() {
        return "turn_on_synctex";
    }


    bool requires_document() { return false; }
};

class ToggleShowLastCommand : public Command {
public:
    ToggleShowLastCommand(MainWidget* w) : Command(w) {};
    void perform() {
        widget->should_show_last_command = !widget->should_show_last_command;
    }
    std::string get_name() {
        return "toggle_show_last_command";
    }

};


class ForwardSearchCommand : public Command {
public:
    ForwardSearchCommand(MainWidget* w) : Command(w) {};

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

    std::string get_name() {
        return "synctex_forward_search";
    }

};

class ExternalSearchCommand : public SymbolCommand {
public:
    ExternalSearchCommand(MainWidget* w) : SymbolCommand(w) {};
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

    std::string get_name() {
        return "external_search";
    }

};

class OpenSelectedUrlCommand : public Command {
public:
    OpenSelectedUrlCommand(MainWidget* w) : Command(w) {};
    void perform() {
        open_web_url((widget->get_selected_text()).c_str());
    }
    std::string get_name() {
        return "open_selected_url";
    }

};

class ScreenDownCommand : public Command {
public:
    ScreenDownCommand(MainWidget* w) : Command(w) {};

    void perform() {
        int rp = num_repeats == 0 ? 1 : num_repeats;
        widget->handle_move_screen(rp);
    }

    std::string get_name() {
        return "screen_down";
    }

};

class ScreenUpCommand : public Command {
public:
    ScreenUpCommand(MainWidget* w) : Command(w) {};

    void perform() {
        int rp = num_repeats == 0 ? 1 : num_repeats;
        widget->handle_move_screen(-rp);
    }

    std::string get_name() {
        return "screen_up";
    }

};

class NextChapterCommand : public Command {
public:
    NextChapterCommand(MainWidget* w) : Command(w) {};

    void perform() {
        int rp = num_repeats == 0 ? 1 : num_repeats;
        widget->main_document_view->goto_chapter(rp);
    }

    std::string get_name() {
        return "next_chapter";
    }

};

class PrevChapterCommand : public Command {
public:
    PrevChapterCommand(MainWidget* w) : Command(w) {};

    void perform() {
        int rp = num_repeats == 0 ? 1 : num_repeats;
        widget->main_document_view->goto_chapter(-rp);
    }

    std::string get_name() {
        return "prev_chapter";
    }

};

class ShowContextMenuCommand : public Command {
public:
    ShowContextMenuCommand(MainWidget* w) : Command(w) {};

    void perform() {
        widget->show_context_menu();
    }

    std::string get_name() {
        return "show_context_menu";
    }


    bool requires_document() { return false; }
};

class ToggleDarkModeCommand : public Command {
public:
    ToggleDarkModeCommand(MainWidget* w) : Command(w) {};

    void perform() {
        widget->toggle_dark_mode();
    }

    std::string get_name() {
        return "toggle_dark_mode";
    }


    bool requires_document() { return false; }
};

class ToggleCustomColorMode : public Command {
public:
    ToggleCustomColorMode(MainWidget* w) : Command(w) {};

    void perform() {
        widget->toggle_custom_color_mode();
    }

    std::string get_name() {
        return "toggle_custom_color";
    }


    bool requires_document() { return false; }
};

class TogglePresentationModeCommand : public Command {
public:
    TogglePresentationModeCommand(MainWidget* w) : Command(w) {};

    void perform() {
        widget->toggle_presentation_mode();
    }

    std::string get_name() {
        return "toggle_presentation_mode";
    }


    bool requires_document() { return false; }
};

class TurnOnPresentationModeCommand : public Command {
public:
    TurnOnPresentationModeCommand(MainWidget* w) : Command(w) {};

    void perform() {
        widget->set_presentation_mode(true);
    }

    std::string get_name() {
        return "turn_on_presentation_mode";
    }


    bool requires_document() { return false; }
};

class ToggleMouseDragMode : public Command {
public:
    ToggleMouseDragMode(MainWidget* w) : Command(w) {};

    void perform() {
        widget->toggle_mouse_drag_mode();
    }

    std::string get_name() {
        return "toggle_mouse_drag_mode";
    }


    bool requires_document() { return false; }
};

class ToggleFreehandDrawingMode : public Command {
public:
    ToggleFreehandDrawingMode(MainWidget* w) : Command(w) {};

    void perform() {
        widget->toggle_freehand_drawing_mode();
    }

    std::string get_name() {
        return "toggle_freehand_drawing_mode";
    }


    bool requires_document() { return false; }
};

class TogglePenDrawingMode : public Command {
public:
    TogglePenDrawingMode(MainWidget* w) : Command(w) {};

    void perform() {
        widget->toggle_pen_drawing_mode();
    }

    std::string get_name() {
        return "toggle_pen_drawing_mode";
    }


    bool requires_document() { return false; }
};

class ToggleScratchpadMode : public Command {
public:
    ToggleScratchpadMode(MainWidget* w) : Command(w) {};

    void perform() {
        widget->toggle_scratchpad_mode();
    }

    std::string get_name() {
        return "toggle_scratchpad_mode";
    }


    bool requires_document() { return false; }
};

class CloseWindowCommand : public Command {
public:
    CloseWindowCommand(MainWidget* w) : Command(w) {};

    void perform() {
        widget->close();
    }

    std::string get_name() {
        return "close_window";
    }


    bool requires_document() { return false; }
};

class NewWindowCommand : public Command {
public:
    NewWindowCommand(MainWidget* w) : Command(w) {};

    void perform() {
        widget->handle_new_window();
    }

    std::string get_name() {
        return "new_window";
    }


    bool requires_document() { return false; }
};

class QuitCommand : public Command {
public:
    QuitCommand(MainWidget* w) : Command(w) {};

    void perform() {
        widget->handle_close_event();
        QApplication::quit();
    }

    std::string get_name() {
        return "quit";
    }


    bool requires_document() { return false; }
};

class EscapeCommand : public Command {
public:
    EscapeCommand(MainWidget* w) : Command(w) {};

    void perform() {
        widget->handle_escape();
    }

    std::string get_name() {
        return "escape";
    }


    bool requires_document() { return false; }
};

class TogglePDFAnnotationsCommand : public Command {
public:
    TogglePDFAnnotationsCommand(MainWidget* w) : Command(w) {};

    void perform() {
        widget->toggle_pdf_annotations();
    }

    std::string get_name() {
        return "toggle_pdf_annotations";
    }


    bool requires_document() { return true; }
};

class OpenLinkCommand : public Command {
public:
    OpenLinkCommand(MainWidget* w) : Command(w) {};
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

    virtual std::string get_name() {
        return "open_link";
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

class OverviewLinkCommand : public OpenLinkCommand {
public:
    OverviewLinkCommand(MainWidget* w) : OpenLinkCommand(w) {};

    void perform() {
        widget->handle_overview_link(text.value());
    }

    std::string get_name() {
        return "overview_link";
    }


};

class PortalToLinkCommand : public OpenLinkCommand {
public:
    PortalToLinkCommand(MainWidget* w) : OpenLinkCommand(w) {};

    void perform() {
        widget->handle_portal_to_link(text.value());
    }

    std::string get_name() {
        return "portal_to_link";
    }


};

class CopyLinkCommand : public TextCommand {
public:
    CopyLinkCommand(MainWidget* w) : TextCommand(w) {};

    void perform() {
        widget->handle_open_link(text.value(), true);
    }

    void pre_perform() {
        widget->set_highlight_links(true, true);
        widget->invalidate_render();

    }

    std::string get_name() {
        return "copy_link";
    }


    std::string text_requirement_name() {
        return "Label";
    }
};

class KeyboardSelectCommand : public TextCommand {
public:
    KeyboardSelectCommand(MainWidget* w) : TextCommand(w) {};

    void perform() {
        widget->handle_keyboard_select(text.value());
    }

    void pre_perform() {
        widget->highlight_words();

    }

    std::string get_name() {
        return "keyboard_select";
    }


    std::string text_requirement_name() {
        return "Labels";
    }
};

class KeyboardOverviewCommand : public TextCommand {
public:
    KeyboardOverviewCommand(MainWidget* w) : TextCommand(w) {};

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

    std::string get_name() {
        return "keyboard_overview";
    }


    std::string text_requirement_name() {
        return "Label";
    }
};

class KeyboardSmartjumpCommand : public TextCommand {
public:
    KeyboardSmartjumpCommand(MainWidget* w) : TextCommand(w) {};

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

    std::string get_name() {
        return "keyboard_smart_jump";
    }


    std::string text_requirement_name() {
        return "Label";
    }
};

class KeysCommand : public Command {
public:
    KeysCommand(MainWidget* w) : Command(w) {};

    void perform() {
        open_file(default_keys_path.get_path(), true);
    }

    std::string get_name() {
        return "keys";
    }


    bool requires_document() { return false; }
};

class KeysUserCommand : public Command {
public:
    KeysUserCommand(MainWidget* w) : Command(w) {};

    void perform() {
        std::optional<Path> key_file_path = widget->input_handler->get_or_create_user_keys_path();
        if (key_file_path) {
            open_file(key_file_path.value().get_path(), true);
        }
    }

    std::string get_name() {
        return "keys_user";
    }


    bool requires_document() { return false; }
};

class KeysUserAllCommand : public Command {
public:
    KeysUserAllCommand(MainWidget* w) : Command(w) {};

    void perform() {
        widget->handle_keys_user_all();
    }

    std::string get_name() {
        return "keys_user_all";
    }


    bool requires_document() { return false; }
};

class PrefsCommand : public Command {
public:
    PrefsCommand(MainWidget* w) : Command(w) {};

    void perform() {
        open_file(default_config_path.get_path(), true);
    }

    std::string get_name() {
        return "prefs";
    }


    bool requires_document() { return false; }
};

class PrefsUserCommand : public Command {
public:
    PrefsUserCommand(MainWidget* w) : Command(w) {};

    void perform() {
        std::optional<Path> pref_file_path = widget->config_manager->get_or_create_user_config_file();
        if (pref_file_path) {
            open_file(pref_file_path.value().get_path(), true);
        }
    }

    std::string get_name() {
        return "prefs_user";
    }


    bool requires_document() { return false; }
};

class PrefsUserAllCommand : public Command {
public:
    PrefsUserAllCommand(MainWidget* w) : Command(w) {};

    void perform() {
        widget->handle_prefs_user_all();
    }

    std::string get_name() {
        return "prefs_user_all";
    }


    bool requires_document() { return false; }
};

class FitToPageWidthRatioCommand : public Command {
public:
    FitToPageWidthRatioCommand(MainWidget* w) : Command(w) {};

    void perform() {
        widget->main_document_view->fit_to_page_width(false, true);
        widget->last_smart_fit_page = {};
    }

    std::string get_name() {
        return "fit_to_page_width_ratio";
    }

};

class SmartJumpUnderCursorCommand : public Command {
public:
    SmartJumpUnderCursorCommand(MainWidget* w) : Command(w) {};

    void perform() {
        QPoint mouse_pos = widget->mapFromGlobal(widget->cursor_pos());
        widget->smart_jump_under_pos({ mouse_pos.x(), mouse_pos.y() });
    }

    std::string get_name() {
        return "smart_jump_under_cursor";
    }

};

class DownloadPaperUnderCursorCommand : public Command {
public:
    DownloadPaperUnderCursorCommand(MainWidget* w) : Command(w) {};

    void perform() {
        widget->download_paper_under_cursor();
    }

    std::string get_name() {
        return "download_paper_under_cursor";
    }

};


class OverviewUnderCursorCommand : public Command {
public:
    OverviewUnderCursorCommand(MainWidget* w) : Command(w) {};

    void perform() {
        QPoint mouse_pos = widget->mapFromGlobal(widget->cursor_pos());
        widget->overview_under_pos({ mouse_pos.x(), mouse_pos.y() });
    }

    std::string get_name() {
        return "overview_under_cursor";
    }

};

class SynctexUnderCursorCommand : public Command {
public:
    SynctexUnderCursorCommand(MainWidget* w) : Command(w) {};

    void perform() {
        QPoint mouse_pos = widget->mapFromGlobal(widget->cursor_pos());
        widget->synctex_under_pos({ mouse_pos.x(), mouse_pos.y() });
    }

    std::string get_name() {
        return "synctex_under_cursor";
    }

};

class SynctexUnderRulerCommand : public Command {
public:
    SynctexUnderRulerCommand(MainWidget* w) : Command(w) {};

    void perform() {
        result = widget->handle_synctex_to_ruler();
    }

    std::string get_name() {
        return "synctex_under_ruler";
    }

};

class VisualMarkUnderCursorCommand : public Command {
public:
    VisualMarkUnderCursorCommand(MainWidget* w) : Command(w) {};

    void perform() {
        QPoint mouse_pos = widget->mapFromGlobal(widget->cursor_pos());
        widget->visual_mark_under_pos({ mouse_pos.x(), mouse_pos.y() });
    }

    std::string get_name() {
        return "visual_mark_under_cursor";
    }

};

class CloseOverviewCommand : public Command {
public:
    CloseOverviewCommand(MainWidget* w) : Command(w) {};

    void perform() {
        widget->set_overview_page({});
    }

    std::string get_name() {
        return "close_overview";
    }

};

class CloseVisualMarkCommand : public Command {
public:
    CloseVisualMarkCommand(MainWidget* w) : Command(w) {};

    void perform() {
        widget->main_document_view->exit_ruler_mode();
    }

    std::string get_name() {
        return "close_visual_mark";
    }

};

class ZoomInCursorCommand : public Command {
public:
    ZoomInCursorCommand(MainWidget* w) : Command(w) {};

    void perform() {
        QPoint mouse_pos = widget->mapFromGlobal(widget->cursor_pos());
        widget->main_document_view->zoom_in_cursor({ mouse_pos.x(), mouse_pos.y() });
        widget->last_smart_fit_page = {};
    }

    std::string get_name() {
        return "zoom_in_cursor";
    }

};

class ZoomOutCursorCommand : public Command {
public:
    ZoomOutCursorCommand(MainWidget* w) : Command(w) {};

    void perform() {
        QPoint mouse_pos = widget->mapFromGlobal(widget->cursor_pos());
        widget->main_document_view->zoom_out_cursor({ mouse_pos.x(), mouse_pos.y() });
        widget->last_smart_fit_page = {};
    }

    std::string get_name() {
        return "zoom_out_cursor";
    }

};

class GotoLeftCommand : public Command {
public:
    GotoLeftCommand(MainWidget* w) : Command(w) {};

    void perform() {
        widget->main_document_view->goto_left();
    }

    std::string get_name() {
        return "goto_left";
    }

};

class GotoLeftSmartCommand : public Command {
public:
    GotoLeftSmartCommand(MainWidget* w) : Command(w) {};

    void perform() {
        widget->main_document_view->goto_left_smart();
    }

    std::string get_name() {
        return "goto_left_smart";
    }

};

class GotoRightCommand : public Command {
public:
    GotoRightCommand(MainWidget* w) : Command(w) {};

    void perform() {
        widget->main_document_view->goto_right();
    }

    std::string get_name() {
        return "goto_right";
    }

};

class GotoRightSmartCommand : public Command {
public:
    GotoRightSmartCommand(MainWidget* w) : Command(w) {};

    void perform() {
        widget->main_document_view->goto_right_smart();
    }

    std::string get_name() {
        return "goto_right_smart";
    }

};

class RotateClockwiseCommand : public Command {
public:
    RotateClockwiseCommand(MainWidget* w) : Command(w) {};

    void perform() {
        widget->main_document_view->rotate();
        widget->rotate_clockwise();
    }

    std::string get_name() {
        return "rotate_clockwise";
    }

};

class RotateCounterClockwiseCommand : public Command {
public:
    RotateCounterClockwiseCommand(MainWidget* w) : Command(w) {};

    void perform() {
        widget->main_document_view->rotate();
        widget->rotate_counterclockwise();
    }

    std::string get_name() {
        return "rotate_counterclockwise";
    }

};

class GotoNextHighlightCommand : public Command {
public:
    GotoNextHighlightCommand(MainWidget* w) : Command(w) {};

    void perform() {
        auto next_highlight = widget->main_document_view->get_document()->get_next_highlight(widget->main_document_view->get_offset_y());
        if (next_highlight.has_value()) {
            widget->long_jump_to_destination(next_highlight.value().selection_begin.y);
        }
    }

    std::string get_name() {
        return "goto_next_highlight";
    }

};

class GotoPrevHighlightCommand : public Command {
public:
    GotoPrevHighlightCommand(MainWidget* w) : Command(w) {};

    void perform() {

        auto prev_highlight = widget->main_document_view->get_document()->get_prev_highlight(widget->main_document_view->get_offset_y());
        if (prev_highlight.has_value()) {
            widget->long_jump_to_destination(prev_highlight.value().selection_begin.y);
        }
    }

    std::string get_name() {
        return "goto_prev_highlight";
    }

};

class GotoNextHighlightOfTypeCommand : public Command {
public:
    GotoNextHighlightOfTypeCommand(MainWidget* w) : Command(w) {};

    void perform() {
        auto next_highlight = widget->main_document_view->get_document()->get_next_highlight(widget->main_document_view->get_offset_y(), widget->select_highlight_type);
        if (next_highlight.has_value()) {
            widget->long_jump_to_destination(next_highlight.value().selection_begin.y);
        }
    }

    std::string get_name() {
        return "goto_next_highlight_of_type";
    }

};

class GotoPrevHighlightOfTypeCommand : public Command {
public:
    GotoPrevHighlightOfTypeCommand(MainWidget* w) : Command(w) {};

    void perform() {
        auto prev_highlight = widget->main_document_view->get_document()->get_prev_highlight(widget->main_document_view->get_offset_y(), widget->select_highlight_type);
        if (prev_highlight.has_value()) {
            widget->long_jump_to_destination(prev_highlight.value().selection_begin.y);
        }
    }

    std::string get_name() {
        return "goto_prev_highlight_of_type";
    }

};

class SetSelectHighlightTypeCommand : public SymbolCommand {
public:
    SetSelectHighlightTypeCommand(MainWidget* w) : SymbolCommand(w) {};
    void perform() {
        widget->select_highlight_type = symbol;
    }

    std::string get_name() {
        return "set_select_highlight_type";
    }


    bool requires_document() { return false; }
};

class SetFreehandType : public SymbolCommand {
public:
    SetFreehandType(MainWidget* w) : SymbolCommand(w) {};
    void perform() {
        widget->current_freehand_type = symbol;
    }

    std::string get_name() {
        return "set_freehand_type";
    }


    bool requires_document() { return false; }
};

class AddHighlightWithCurrentTypeCommand : public Command {
public:
    AddHighlightWithCurrentTypeCommand(MainWidget* w) : Command(w) {};
    void perform() {
        if (widget->main_document_view->selected_character_rects.size() > 0) {
            widget->main_document_view->add_highlight(widget->selection_begin, widget->selection_end, widget->select_highlight_type);
            widget->clear_selected_text();
        }
    }

    std::string get_name() {
        return "add_highlight_with_current_type";
    }

};

class UndoDrawingCommand : public Command {
public:
    UndoDrawingCommand(MainWidget* w) : Command(w) {};
    void perform() {
        widget->handle_undo_drawing();
    }

    std::string get_name() {
        return "undo_drawing";
    }


    bool requires_document() { return true; }
};


class EnterPasswordCommand : public TextCommand {
public:
    EnterPasswordCommand(MainWidget* w) : TextCommand(w) {};
    void perform() {
        std::string password = utf8_encode(text.value());
        widget->add_password(widget->main_document_view->get_document()->get_path(), password);
    }

    std::string get_name() {
        return "enter_password";
    }


    std::string text_requirement_name() {
        return "Password";
    }
};

class ToggleFastreadCommand : public Command {
public:
    ToggleFastreadCommand(MainWidget* w) : Command(w) {};
    void perform() {
        widget->toggle_fastread();
    }


    std::string get_name() {
        return "toggle_fastread";
    }
};

class GotoTopOfPageCommand : public Command {
public:
    GotoTopOfPageCommand(MainWidget* w) : Command(w) {};
    void perform() {
        widget->main_document_view->goto_top_of_page();
    }

    std::string get_name() {
        return "goto_top_of_page";
    }

};

class GotoBottomOfPageCommand : public Command {
public:
    GotoBottomOfPageCommand(MainWidget* w) : Command(w) {};
    void perform() {
        widget->main_document_view->goto_bottom_of_page();
    }

    std::string get_name() {
        return "goto_bottom_of_page";
    }

};

class ReloadCommand : public Command {
public:
    ReloadCommand(MainWidget* w) : Command(w) {};
    void perform() {
        widget->reload();
    }

    std::string get_name() {
        return "relaod";
    }

};

class ReloadNoFlickerCommand : public Command {
public:
    ReloadNoFlickerCommand(MainWidget* w) : Command(w) {};
    void perform() {
        widget->reload(false);
    }

    std::string get_name() {
        return "relaod_no_flicker";
    }

};

class ReloadConfigCommand : public Command {
public:
    ReloadConfigCommand(MainWidget* w) : Command(w) {};
    void perform() {
        widget->on_config_file_changed(widget->config_manager);
    }

    std::string get_name() {
        return "relaod_config";
    }


    bool requires_document() { return false; }
};

class TurnOnAllDrawings : public Command {
public:
    TurnOnAllDrawings(MainWidget* w) : Command(w) {};
    void perform() {
        widget->hande_turn_on_all_drawings();
    }

    std::string get_name() {
        return "turn_on_all_drawings";
    }


    bool requires_document() { return false; }
};

class TurnOffAllDrawings : public Command {
public:
    TurnOffAllDrawings(MainWidget* w) : Command(w) {};
    void perform() {
        widget->hande_turn_off_all_drawings();
    }

    std::string get_name() {
        return "turn_off_all_drawings";
    }


    bool requires_document() { return false; }
};

class SetStatusStringCommand : public TextCommand {
public:
    SetStatusStringCommand(MainWidget* w) : TextCommand(w) {};

    void perform() {
        widget->set_status_message(text.value());
    }

    std::string get_name() {
        return "set_status_string";
    }

    std::string text_requirement_name() {
        return "Status String";
    }


    bool requires_document() { return false; }
};

class ClearStatusStringCommand : public Command {
public:
    ClearStatusStringCommand(MainWidget* w) : Command(w) {};
    void perform() {
        widget->set_status_message(L"");
    }

    std::string get_name() {
        return "clear_status_string";
    }


    bool requires_document() { return false; }
};

class ToggleTittlebarCommand : public Command {
public:
    ToggleTittlebarCommand(MainWidget* w) : Command(w) {};
    void perform() {
        widget->toggle_titlebar();
    }

    std::string get_name() {
        return "toggle_titlebar";
    }


    bool requires_document() { return false; }
};

class NextPreviewCommand : public Command {
public:
    NextPreviewCommand(MainWidget* w) : Command(w) {};
    void perform() {
        if (widget->smart_view_candidates.size() > 0) {
            //widget->index_into_candidates = (widget->index_into_candidates + 1) % widget->smart_view_candidates.size();
            //widget->set_overview_position(widget->smart_view_candidates[widget->index_into_candidates].page, widget->smart_view_candidates[widget->index_into_candidates].y);
            widget->goto_ith_next_overview(1);
        }
    }

    std::string get_name() {
        return "next_preview";
    }

};

class PreviousPreviewCommand : public Command {
public:
    PreviousPreviewCommand(MainWidget* w) : Command(w) {};
    void perform() {
        if (widget->smart_view_candidates.size() > 0) {
            //widget->index_into_candidates = mod(widget->index_into_candidates - 1, widget->smart_view_candidates.size());
            //widget->set_overview_position(widget->smart_view_candidates[widget->index_into_candidates].page, widget->smart_view_candidates[widget->index_into_candidates].y);
            widget->goto_ith_next_overview(-1);
        }
    }

    std::string get_name() {
        return "previous_preview";
    }

};

class GotoOverviewCommand : public Command {
public:
    GotoOverviewCommand(MainWidget* w) : Command(w) {};
    void perform() {
        widget->goto_overview();
    }

    std::string get_name() {
        return "goto_overview";
    }

};

class PortalToOverviewCommand : public Command {
public:
    PortalToOverviewCommand(MainWidget* w) : Command(w) {};
    void perform() {
        widget->handle_portal_to_overview();
    }

    std::string get_name() {
        return "portal_to_overview";
    }

};

class GotoSelectedTextCommand : public Command {
public:
    GotoSelectedTextCommand(MainWidget* w) : Command(w) {};

    void perform() {
        widget->long_jump_to_destination(widget->selection_begin.y);
    }

    std::string get_name() {
        return "goto_selected_text";
    }

};

class FocusTextCommand : public TextCommand {
public:
    FocusTextCommand(MainWidget* w) : TextCommand(w) {};

    void perform() {
        std::wstring text_ = text.value();
        widget->handle_focus_text(text_);
    }

    std::string get_name() {
        return "focus_text";
    }


    std::string text_requirement_name() {
        return "Text to focus";
    }
};

class DownloadOverviewPaperCommand : public TextCommand {
public:
    std::optional<AbsoluteRect> source_rect = {};
    std::wstring src_doc_path;

    DownloadOverviewPaperCommand(MainWidget* w) : TextCommand(w) {};

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

    std::string get_name() {
        return "download_overview_paper";
    }


    std::string text_requirement_name() {
        return "Paper Name";
    }
};

class GotoWindowCommand : public Command {
public:
    GotoWindowCommand(MainWidget* w) : Command(w) {};

    void perform() {
        widget->handle_goto_window();
    }

    std::string get_name() {
        return "goto_window";
    }


    bool requires_document() { return false; }
};

class ToggleSmoothScrollModeCommand : public Command {
public:
    ToggleSmoothScrollModeCommand(MainWidget* w) : Command(w) {};

    void perform() {
        widget->handle_toggle_smooth_scroll_mode();
    }

    std::string get_name() {
        return "toggle_smooth_scroll_mode";
    }


    bool requires_document() { return false; }
};

class ToggleScrollbarCommand : public Command {
public:
    ToggleScrollbarCommand(MainWidget* w) : Command(w) {};

    void perform() {
        widget->toggle_scrollbar();
    }

    bool requires_document() { return false; }

    std::string get_name() {
        return "toggle_scrollbar";
    }

};

class OverviewToPortalCommand : public Command {
public:
    OverviewToPortalCommand(MainWidget* w) : Command(w) {};

    void perform() {
        widget->handle_overview_to_portal();
    }

    std::string get_name() {
        return "overview_to_portal";
    }

};

class ScanNewFilesFromScanDirCommand : public Command {
public:
    ScanNewFilesFromScanDirCommand(MainWidget* w) : Command(w) {};

    void perform() {
        widget->scan_new_files_from_scan_directory();
    }

    std::string get_name() {
        return "scan_new_files";
    }

};

class DebugCommand : public Command {
public:
    DebugCommand(MainWidget* w) : Command(w) {};

    void perform() {
        widget->handle_debug_command();
    }

    std::string get_name() {
        return "debug";
    }

};

class ShowTouchMainMenu : public Command {
public:
    ShowTouchMainMenu(MainWidget* w) : Command(w) {};

    void perform() {
        widget->show_touch_main_menu();
    }

    std::string get_name() {
        return "show_touch_main_menu";
    }

};

class ShowTouchDrawingMenu : public Command {
public:
    ShowTouchDrawingMenu(MainWidget* w) : Command(w) {};

    void perform() {
        widget->show_draw_controls();
    }

    std::string get_name() {
        return "show_touch_draw_controls";
    }

};

class ShowTouchSettingsMenu : public Command {
public:
    ShowTouchSettingsMenu(MainWidget* w) : Command(w) {};

    void perform() {
        widget->show_touch_settings_menu();
    }

    std::string get_name() {
        return "show_touch_settings_menu";
    }

};


class ExportPythonApiCommand : public Command {
public:
    ExportPythonApiCommand(MainWidget* w) : Command(w) {};

    void perform() {
        widget->export_python_api();
    }

    std::string get_name() {
        return "export_python_api";
    }

};

class SelectCurrentSearchMatchCommand : public Command {
public:
    SelectCurrentSearchMatchCommand(MainWidget* w) : Command(w) {};

    void perform() {
        widget->handle_select_current_search_match();
    }

    std::string get_name() {
        return "select_current_search_match";
    }

};

//class StopSearchCommand : public Command {
//public:
//    StopSearchCommand(MainWidget* w) : Command(w) {};
//
//    void perform() {
//        widget->handle_stop_search();
//    }
//
//    std::string get_name() {
//        return "stop_search";
//    }
//
//};

class SelectRectCommand : public Command {
public:
    SelectRectCommand(MainWidget* w) : Command(w) {};

    void perform() {
        widget->set_rect_select_mode(true);
    }

    std::string get_name() {
        return "select_rect";
    }

};

class ToggleTypingModeCommand : public Command {
public:
    ToggleTypingModeCommand(MainWidget* w) : Command(w) {};

    void perform() {
        widget->handle_toggle_typing_mode();
    }

    std::string get_name() {
        return "toggle_typing_mode";
    }

};

class DonateCommand : public Command {
public:
    DonateCommand(MainWidget* w) : Command(w) {};

    void perform() {
        open_web_url(L"https://www.buymeacoffee.com/ahrm");
    }

    std::string get_name() {
        return "donate";
    }


    bool requires_document() { return false; }
};

class OverviewNextItemCommand : public Command {
public:
    OverviewNextItemCommand(MainWidget* w) : Command(w) {};

    void perform() {
        if (num_repeats == 0) num_repeats++;
        widget->goto_search_result(num_repeats, true);
    }

    std::string get_name() {
        return "overview_next_item";
    }

};

class OverviewPrevItemCommand : public Command {
public:
    OverviewPrevItemCommand(MainWidget* w) : Command(w) {};

    void perform() {
        if (num_repeats == 0) num_repeats++;
        widget->goto_search_result(-num_repeats, true);
    }

    std::string get_name() {
        return "overview_prev_item";
    }

};

class DeleteHighlightUnderCursorCommand : public Command {
public:
    DeleteHighlightUnderCursorCommand(MainWidget* w) : Command(w) {};

    void perform() {
        widget->handle_delete_highlight_under_cursor();
    }

    std::string get_name() {
        return "delete_highlight_under_cursor";
    }

};

class NoopCommand : public Command {
public:

    NoopCommand(MainWidget* widget_) : Command(widget_) {}

    void perform() {
    }

    std::string get_name() {
        return "noop";
    }


    bool requires_document() { return false; }
};

class ImportCommand : public Command {
public:
    ImportCommand(MainWidget* w) : Command(w) {};

    void perform() {
        std::wstring import_file_name = select_json_file_name();
        widget->import_json(import_file_name);
    }

    std::string get_name() {
        return "import";
    }


    bool requires_document() { return false; }
};

class ExportCommand : public Command {
public:
    ExportCommand(MainWidget* w) : Command(w) {};

    void perform() {
        std::wstring export_file_name = select_new_json_file_name();
        widget->export_json(export_file_name);
    }

    std::string get_name() {
        return "export";
    }


    bool requires_document() { return false; }
};

class WriteAnnotationsFileCommand : public Command {
public:
    WriteAnnotationsFileCommand(MainWidget* w) : Command(w) {};

    void perform() {
        widget->doc()->persist_annotations(true);
    }

    std::string get_name() {
        return "write_annotations_file";
    }

};

class LoadAnnotationsFileCommand : public Command {
public:
    LoadAnnotationsFileCommand(MainWidget* w) : Command(w) {};

    void perform() {
        widget->doc()->load_annotations();
    }

    std::string get_name() {
        return "load_annotations_file";
    }

};

class LoadAnnotationsFileSyncDeletedCommand : public Command {
public:
    LoadAnnotationsFileSyncDeletedCommand(MainWidget* w) : Command(w) {};

    void perform() {
        widget->doc()->load_annotations(true);
    }

    std::string get_name() {
        return "import_annotations_file_sync_deleted";
    }

};

class EnterVisualMarkModeCommand : public Command {
public:
    EnterVisualMarkModeCommand(MainWidget* w) : Command(w) {};

    void perform() {
        widget->visual_mark_under_pos({ widget->width() / 2, widget->height() / 2 });
    }

    std::string get_name() {
        return "enter_visual_mark_mode";
    }

};

class SetPageOffsetCommand : public TextCommand {
public:
    SetPageOffsetCommand(MainWidget* w) : TextCommand(w) {};

    void perform() {
        if (is_string_numeric(text.value().c_str()) && text.value().size() < 6) { // make sure the page number is valid
            widget->main_document_view->set_page_offset(std::stoi(text.value().c_str()));
        }
    }

    std::string get_name() {
        return "set_page_offset";
    }

};

class ToggleVisualScrollCommand : public Command {
public:
    ToggleVisualScrollCommand(MainWidget* w) : Command(w) {};

    void perform() {
        widget->toggle_visual_scroll_mode();
    }

    std::string get_name() {
        return "toggle_visual_scroll";
    }

};

class ToggleHorizontalLockCommand : public Command {
public:
    ToggleHorizontalLockCommand(MainWidget* w) : Command(w) {};

    void perform() {
        widget->horizontal_scroll_locked = !widget->horizontal_scroll_locked;
    }

    std::string get_name() {
        return "toggle_horizontal_scroll_lock";
    }

};

class ExecuteCommand : public TextCommand {
public:
    ExecuteCommand(MainWidget* w) : TextCommand(w) {};

    void perform() {
        widget->execute_command(text.value());
    }

    std::string get_name() {
        return "execute";
    }


    bool requires_document() { return false; }
};

class ImportAnnotationsCommand : public Command {
public:
    ImportAnnotationsCommand(MainWidget* w) : Command(w) {};

    void perform() {
        widget->doc()->import_annotations();
    }

    std::string get_name() {
        return "import_annotations";
    }

};

class EmbedAnnotationsCommand : public Command {
public:
    EmbedAnnotationsCommand(MainWidget* w) : Command(w) {};

    void perform() {
        std::wstring embedded_pdf_file_name = select_new_pdf_file_name();
        if (embedded_pdf_file_name.size() > 0) {
            widget->main_document_view->get_document()->embed_annotations(embedded_pdf_file_name);
        }
    }

    std::string get_name() {
        return "embed_annotations";
    }

};

class CopyWindowSizeConfigCommand : public Command {
public:
    CopyWindowSizeConfigCommand(MainWidget* w) : Command(w) {};

    void perform() {
        copy_to_clipboard(widget->get_window_configuration_string());
    }

    std::string get_name() {
        return "copy_window_size_config";
    }


    bool requires_document() { return false; }
};

class ToggleSelectHighlightCommand : public Command {
public:
    ToggleSelectHighlightCommand(MainWidget* w) : Command(w) {};

    void perform() {
        widget->is_select_highlight_mode = !widget->is_select_highlight_mode;
    }

    std::string get_name() {
        return "toggle_select_highlight";
    }

};

class OpenLastDocumentCommand : public Command {
public:
    OpenLastDocumentCommand(MainWidget* w) : Command(w) {};

    void perform() {
        auto last_opened_file = widget->get_last_opened_file_checksum();
        if (last_opened_file) {
            widget->open_document_with_hash(last_opened_file.value());
        }
    }

    std::string get_name() {
        return "open_last_document";
    }


    bool requires_document() { return false; }
};

class AddMarkedDataCommand : public Command {
public:
    AddMarkedDataCommand(MainWidget* w) : Command(w) {};

    void perform() {
        widget->handle_add_marked_data();
    }

    std::string get_name() {
        return "add_marked_data";
    }


    bool requires_document() { return true; }
};

class UndoMarkedDataCommand : public Command {
public:
    UndoMarkedDataCommand(MainWidget* w) : Command(w) {};

    void perform() {
        widget->handle_undo_marked_data();
    }

    std::string get_name() {
        return "undo_marked_data";
    }


    bool requires_document() { return true; }
};

class GotoRandomPageCommand : public Command {
public:
    GotoRandomPageCommand(MainWidget* w) : Command(w) {};

    void perform() {
        widget->handle_goto_random_page();
    }

    std::string get_name() {
        return "goto_random_page";
    }


    bool requires_document() { return true; }
};

class RemoveMarkedDataCommand : public Command {
public:
    RemoveMarkedDataCommand(MainWidget* w) : Command(w) {};

    void perform() {
        widget->handle_remove_marked_data();
    }

    std::string get_name() {
        return "remove_marked_data";
    }


    bool requires_document() { return true; }
};

class ExportMarkedDataCommand : public Command {
public:
    ExportMarkedDataCommand(MainWidget* w) : Command(w) {};

    void perform() {
        widget->handle_export_marked_data();
    }

    std::string get_name() {
        return "export_marked_data";
    }


    bool requires_document() { return true; }
};

class ToggleStatusbarCommand : public Command {
public:
    ToggleStatusbarCommand(MainWidget* w) : Command(w) {};

    void perform() {
        widget->toggle_statusbar();
    }

    std::string get_name() {
        return "toggle_statusbar";
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
    LazyCommand(MainWidget* widget_, CommandManager* manager, CommandInvocation invocation) : Command(widget_), noop(widget_) {
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
    ClearCurrentPageDrawingsCommand(MainWidget* w) : Command(w) {};

    void perform() {
        widget->clear_current_page_drawings();
    }

    std::string get_name() {
        return "clear_current_page_drawings";
    }


    bool requires_document() { return false; }
};

class ClearCurrentDocumentDrawingsCommand : public Command {
public:
    ClearCurrentDocumentDrawingsCommand(MainWidget* w) : Command(w) {};

    void perform() {
        widget->clear_current_document_drawings();
    }

    std::string get_name() {
        return "clear_current_document_drawings";
    }


    bool requires_document() { return false; }
};

class DeleteFreehandDrawingsCommand : public Command {
public:
    DeleteFreehandDrawingsCommand(MainWidget* w) : Command(w) {};

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

    std::string get_name() {
        return "delete_freehand_drawings";
    }

};

class SelectFreehandDrawingsCommand : public Command {
public:
    SelectFreehandDrawingsCommand(MainWidget* w) : Command(w) {};

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

    std::string get_name() {
        return "select_freehand_drawings";
    }

};

class CustomCommand : public Command {

    std::wstring raw_command;
    std::string name;
    std::optional<AbsoluteRect> command_rect;
    std::optional<AbsoluteDocumentPos> command_point;
    std::optional<std::wstring> command_text;

public:

    CustomCommand(MainWidget* widget_, std::string name_, std::wstring command_) : Command(widget_) {
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
	ToggleConfigCommand(MainWidget* widget, std::string config_name_) : Command(widget){
		config_name = config_name_;
	}

	void perform() {
        //widget->config_manager->deserialize_config(config_name, text.value());
        auto conf = widget->config_manager->get_mut_config_with_name(utf8_decode(config_name));
        *(bool*)conf->value = !*(bool*)conf->value;
	}

	std::string get_name() {
		return "toggleconfig_" + config_name;
	}
	
	bool requires_document() { return false; }
};

class ConfigCommand : public Command {
    std::string config_name;
    std::optional<std::wstring> text = {};
    ConfigManager* config_manager;
public:
    ConfigCommand(MainWidget* widget_, std::string config_name_, ConfigManager* config_manager_) : Command(widget_), config_manager(config_manager_) {
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

            if (widget->config_manager->deserialize_config(config_name, text.value())) {
                widget->on_config_changed(config_name);
            }
        }
    }

    std::string get_name() {
        return "setconfig_" + config_name;
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
            return std::move(std::make_unique<LazyCommand>(widget, widget->command_manager, invocation));
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

    MacroCommand(MainWidget* widget_, CommandManager* manager, std::string name_, std::wstring commands_) : Command(widget_) {
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
            }
            return {};
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
            for (std::unique_ptr<Command>& subcommand : commands) {

                if (subcommand->pushes_state()) {
                    widget->push_state();
                }

                subcommand->run();
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
            std::string res;
            for (auto& command : commands) {
                res += command->get_name();
            }
            return "{macro}" + res;
            //return "[macro]" + commands[0]->get_name();
        }
    }

};

CommandManager::CommandManager(ConfigManager* config_manager) {

    new_commands["goto_beginning"] = [](MainWidget* widget) {return std::make_unique< GotoBeginningCommand>(widget); };
    new_commands["goto_end"] = [](MainWidget* widget) {return std::make_unique< GotoEndCommand>(widget); };
    new_commands["goto_definition"] = [](MainWidget* widget) {return std::make_unique< GotoDefinitionCommand>(widget); };
    new_commands["overview_definition"] = [](MainWidget* widget) {return std::make_unique< OverviewDefinitionCommand>(widget); };
    new_commands["portal_to_definition"] = [](MainWidget* widget) {return std::make_unique< PortalToDefinitionCommand>(widget); };
    new_commands["goto_tab"] = [](MainWidget* widget) {return std::make_unique< GotoLoadedDocumentCommand>(widget); };
    new_commands["next_item"] = [](MainWidget* widget) {return std::make_unique< NextItemCommand>(widget); };
    new_commands["previous_item"] = [](MainWidget* widget) {return std::make_unique< PrevItemCommand>(widget); };
    new_commands["toggle_text_mark"] = [](MainWidget* widget) {return std::make_unique< ToggleTextMarkCommand>(widget); };
    new_commands["move_text_mark_forward"] = [](MainWidget* widget) {return std::make_unique< MoveTextMarkForwardCommand>(widget); };
    new_commands["move_text_mark_backward"] = [](MainWidget* widget) {return std::make_unique< MoveTextMarkBackwardCommand>(widget); };
    new_commands["move_text_mark_forward_word"] = [](MainWidget* widget) {return std::make_unique< MoveTextMarkForwardWordCommand>(widget); };
    new_commands["move_text_mark_backward_word"] = [](MainWidget* widget) {return std::make_unique< MoveTextMarkBackwardWordCommand>(widget); };
    new_commands["move_text_mark_down"] = [](MainWidget* widget) {return std::make_unique< MoveTextMarkDownCommand>(widget); };
    new_commands["move_text_mark_up"] = [](MainWidget* widget) {return std::make_unique< MoveTextMarkUpCommand>(widget); };
    new_commands["set_mark"] = [](MainWidget* widget) {return std::make_unique< SetMark>(widget); };
    new_commands["toggle_drawing_mask"] = [](MainWidget* widget) {return std::make_unique< ToggleDrawingMask>(widget); };
    new_commands["turn_on_all_drawings"] = [](MainWidget* widget) {return std::make_unique< TurnOnAllDrawings>(widget); };
    new_commands["turn_off_all_drawings"] = [](MainWidget* widget) {return std::make_unique< TurnOffAllDrawings>(widget); };
    new_commands["goto_mark"] = [](MainWidget* widget) {return std::make_unique< GotoMark>(widget); };
    new_commands["goto_page_with_page_number"] = [](MainWidget* widget) {return std::make_unique< GotoPageWithPageNumberCommand>(widget); };
    new_commands["edit_selected_bookmark"] = [](MainWidget* widget) {return std::make_unique< EditSelectedBookmarkCommand>(widget); };
    new_commands["edit_selected_highlight"] = [](MainWidget* widget) {return std::make_unique< EditSelectedHighlightCommand>(widget); };
    new_commands["search"] = [](MainWidget* widget) {return std::make_unique< SearchCommand>(widget); };
    new_commands["download_paper_with_url"] = [](MainWidget* widget) {return std::make_unique< DownloadPaperWithUrlCommand>(widget); };
    new_commands["download_clipboard_url"] = [](MainWidget* widget) {return std::make_unique< DownloadClipboardUrlCommand>(widget); };
    new_commands["execute_macro"] = [](MainWidget* widget) {return std::make_unique< ExecuteMacroCommand>(widget); };
    new_commands["control_menu"] = [](MainWidget* widget) {return std::make_unique< ControlMenuCommand>(widget); };
    new_commands["set_view_state"] = [](MainWidget* widget) {return std::make_unique< SetViewStateCommand>(widget); };
    new_commands["get_config_value"] = [](MainWidget* widget) {return std::make_unique< GetConfigCommand>(widget); };
    new_commands["get_config_no_dialog"] = [](MainWidget* widget) {return std::make_unique< GetConfigNoDialogCommand>(widget); };
    new_commands["show_custom_options"] = [](MainWidget* widget) {return std::make_unique< ShowOptionsCommand>(widget); };
    new_commands["show_text_prompt"] = [](MainWidget* widget) {return std::make_unique< ShowTextPromptCommand>(widget); };
    new_commands["get_state_json"] = [](MainWidget* widget) {return std::make_unique< GetStateJsonCommand>(widget); };
    new_commands["get_paper_name"] = [](MainWidget* widget) {return std::make_unique< GetPaperNameCommand>(widget); };
    new_commands["get_overview_paper_name"] = [](MainWidget* widget) {return std::make_unique< GetOverviewPaperName>(widget); };
    new_commands["get_annotations_json"] = [](MainWidget* widget) {return std::make_unique< GetAnnotationsJsonCommand>(widget); };
    new_commands["toggle_rect_hints"] = [](MainWidget* widget) {return std::make_unique< ToggleRectHintsCommand>(widget); };
    new_commands["add_annot_to_selected_highlight"] = [](MainWidget* widget) {return std::make_unique< AddAnnotationToSelectedHighlightCommand>(widget); };
    new_commands["add_annot_to_highlight"] = [](MainWidget* widget) {return std::make_unique< AddAnnotationToHighlightCommand>(widget); };
    new_commands["rename"] = [](MainWidget* widget) {return std::make_unique< RenameCommand>(widget); };
    new_commands["set_freehand_thickness"] = [](MainWidget* widget) {return std::make_unique< SetFreehandThickness>(widget); };
    new_commands["goto_page_with_label"] = [](MainWidget* widget) {return std::make_unique< GotoPageWithLabel>(widget); };
    new_commands["regex_search"] = [](MainWidget* widget) {return std::make_unique< RegexSearchCommand>(widget); };
    new_commands["chapter_search"] = [](MainWidget* widget) {return std::make_unique< ChapterSearchCommand>(widget); };
    new_commands["move_down"] = [](MainWidget* widget) {return std::make_unique< MoveDownCommand>(widget); };
    new_commands["move_up"] = [](MainWidget* widget) {return std::make_unique< MoveUpCommand>(widget); };
    new_commands["move_left"] = [](MainWidget* widget) {return std::make_unique< MoveLeftCommand>(widget); };
    new_commands["move_right"] = [](MainWidget* widget) {return std::make_unique< MoveRightCommand>(widget); };
    new_commands["move_left_in_overview"] = [](MainWidget* widget) {return std::make_unique< MoveLeftInOverviewCommand>(widget); };
    new_commands["move_right_in_overview"] = [](MainWidget* widget) {return std::make_unique< MoveRightInOverviewCommand>(widget); };
    new_commands["save_scratchpad"] = [](MainWidget* widget) {return std::make_unique< SaveScratchpadCommand>(widget); };
    new_commands["load_scratchpad"] = [](MainWidget* widget) {return std::make_unique< LoadScratchpadCommand>(widget); };
    new_commands["clear_scratchpad"] = [](MainWidget* widget) {return std::make_unique< ClearScratchpadCommand>(widget); };
    new_commands["zoom_in"] = [](MainWidget* widget) {return std::make_unique< ZoomInCommand>(widget); };
    new_commands["zoom_out"] = [](MainWidget* widget) {return std::make_unique< ZoomOutCommand>(widget); };
    new_commands["zoom_in_overview"] = [](MainWidget* widget) {return std::make_unique< ZoomInOverviewCommand>(widget); };
    new_commands["zoom_out_overview"] = [](MainWidget* widget) {return std::make_unique< ZoomOutOverviewCommand>(widget); };
    new_commands["fit_to_page_width"] = [](MainWidget* widget) {return std::make_unique< FitToPageWidthCommand>(widget); };
    new_commands["fit_to_page_height"] = [](MainWidget* widget) {return std::make_unique< FitToPageHeightCommand>(widget); };
    new_commands["fit_to_page_smart"] = [](MainWidget* widget) {return std::make_unique< FitToPageSmartCommand>(widget); };
    new_commands["fit_to_page_height_smart"] = [](MainWidget* widget) {return std::make_unique< FitToPageHeightSmartCommand>(widget); };
    new_commands["fit_to_page_width_smart"] = [](MainWidget* widget) {return std::make_unique< FitToPageWidthSmartCommand>(widget); };
    new_commands["next_page"] = [](MainWidget* widget) {return std::make_unique< NextPageCommand>(widget); };
    new_commands["previous_page"] = [](MainWidget* widget) {return std::make_unique< PreviousPageCommand>(widget); };
    new_commands["open_document"] = [](MainWidget* widget) {return std::make_unique< OpenDocumentCommand>(widget); };
    new_commands["screenshot"] = [](MainWidget* widget) {return std::make_unique< ScreenshotCommand>(widget); };
    new_commands["framebuffer_screenshot"] = [](MainWidget* widget) {return std::make_unique< FramebufferScreenshotCommand>(widget); };
    new_commands["wait_for_renders_to_finish"] = [](MainWidget* widget) {return std::make_unique< WaitForRendersToFinishCommand>(widget); };
    new_commands["wait_for_search_to_finish"] = [](MainWidget* widget) {return std::make_unique< WaitForSearchToFinishCommand>(widget); };
    new_commands["wait_for_indexing_to_finish"] = [](MainWidget* widget) {return std::make_unique< WaitForIndexingToFinishCommand>(widget); };
    new_commands["add_bookmark"] = [](MainWidget* widget) {return std::make_unique< AddBookmarkCommand>(widget); };
    new_commands["add_marked_bookmark"] = [](MainWidget* widget) {return std::make_unique< AddBookmarkMarkedCommand>(widget); };
    new_commands["add_freetext_bookmark"] = [](MainWidget* widget) {return std::make_unique< AddBookmarkFreetextCommand>(widget); };
    new_commands["copy_drawings_from_scratchpad"] = [](MainWidget* widget) {return std::make_unique< CopyDrawingsFromScratchpadCommand>(widget); };
    new_commands["copy_screenshot_to_scratchpad"] = [](MainWidget* widget) {return std::make_unique< CopyScreenshotToScratchpad>(widget); };
    new_commands["add_highlight"] = [](MainWidget* widget) {return std::make_unique< AddHighlightCommand>(widget); };
    new_commands["goto_toc"] = [](MainWidget* widget) {return std::make_unique< GotoTableOfContentsCommand>(widget); };
    new_commands["goto_highlight"] = [](MainWidget* widget) {return std::make_unique< GotoHighlightCommand>(widget); };
    new_commands["increase_freetext_font_size"] = [](MainWidget* widget) {return std::make_unique< IncreaseFreetextBookmarkFontSizeCommand>(widget); };
    new_commands["decrease_freetext_font_size"] = [](MainWidget* widget) {return std::make_unique< DecreaseFreetextBookmarkFontSizeCommand>(widget); };
    new_commands["goto_portal_list"] = [](MainWidget* widget) {return std::make_unique< GotoPortalListCommand>(widget); };
    new_commands["goto_bookmark"] = [](MainWidget* widget) {return std::make_unique< GotoBookmarkCommand>(widget); };
    new_commands["goto_bookmark_g"] = [](MainWidget* widget) {return std::make_unique< GotoBookmarkGlobalCommand>(widget); };
    new_commands["goto_highlight_g"] = [](MainWidget* widget) {return std::make_unique< GotoHighlightGlobalCommand>(widget); };
    new_commands["link"] = [](MainWidget* widget) {return std::make_unique< PortalCommand>(widget); };
    new_commands["portal"] = [](MainWidget* widget) {return std::make_unique< PortalCommand>(widget); };
    new_commands["create_visible_portal"] = [](MainWidget* widget) {return std::make_unique< CreateVisiblePortalCommand>(widget); };
    new_commands["next_state"] = [](MainWidget* widget) {return std::make_unique< NextStateCommand>(widget); };
    new_commands["prev_state"] = [](MainWidget* widget) {return std::make_unique< PrevStateCommand>(widget); };
    new_commands["history_forward"] = [](MainWidget* widget) {return std::make_unique< NextStateCommand>(widget); };
    new_commands["history_back"] = [](MainWidget* widget) {return std::make_unique< PrevStateCommand>(widget); };
    new_commands["delete_link"] = [](MainWidget* widget) {return std::make_unique< DeletePortalCommand>(widget); };
    new_commands["delete_portal"] = [](MainWidget* widget) {return std::make_unique< DeletePortalCommand>(widget); };
    new_commands["delete_bookmark"] = [](MainWidget* widget) {return std::make_unique< DeleteBookmarkCommand>(widget); };
    new_commands["delete_highlight"] = [](MainWidget* widget) {return std::make_unique< DeleteHighlightCommand>(widget); };
    new_commands["change_highlight"] = [](MainWidget* widget) {return std::make_unique< ChangeHighlightTypeCommand>(widget); };
    new_commands["goto_link"] = [](MainWidget* widget) {return std::make_unique< GotoPortalCommand>(widget); };
    new_commands["goto_portal"] = [](MainWidget* widget) {return std::make_unique< GotoPortalCommand>(widget); };
    new_commands["edit_link"] = [](MainWidget* widget) {return std::make_unique< EditPortalCommand>(widget); };
    new_commands["edit_portal"] = [](MainWidget* widget) {return std::make_unique< EditPortalCommand>(widget); };
    new_commands["open_prev_doc"] = [](MainWidget* widget) {return std::make_unique< OpenPrevDocCommand>(widget); };
    new_commands["open_all_docs"] = [](MainWidget* widget) {return std::make_unique< OpenAllDocsCommand>(widget); };
    new_commands["open_document_embedded"] = [](MainWidget* widget) {return std::make_unique< OpenDocumentEmbeddedCommand>(widget); };
    new_commands["open_document_embedded_from_current_path"] = [](MainWidget* widget) {return std::make_unique< OpenDocumentEmbeddedFromCurrentPathCommand>(widget); };
    new_commands["copy"] = [](MainWidget* widget) {return std::make_unique< CopyCommand>(widget); };
    new_commands["toggle_fullscreen"] = [](MainWidget* widget) {return std::make_unique< ToggleFullscreenCommand>(widget); };
    new_commands["maximize"] = [](MainWidget* widget) {return std::make_unique< MaximizeCommand>(widget); };
    new_commands["toggle_one_window"] = [](MainWidget* widget) {return std::make_unique< ToggleOneWindowCommand>(widget); };
    new_commands["toggle_highlight"] = [](MainWidget* widget) {return std::make_unique< ToggleHighlightCommand>(widget); };
    new_commands["toggle_synctex"] = [](MainWidget* widget) {return std::make_unique< ToggleSynctexCommand>(widget); };
    new_commands["turn_on_synctex"] = [](MainWidget* widget) {return std::make_unique< TurnOnSynctexCommand>(widget); };
    new_commands["toggle_show_last_command"] = [](MainWidget* widget) {return std::make_unique< ToggleShowLastCommand>(widget); };
    new_commands["synctex_forward_search"] = [](MainWidget* widget) {return std::make_unique< ForwardSearchCommand>(widget); };
    new_commands["command"] = [](MainWidget* widget) {return std::make_unique< CommandCommand>(widget); };
    new_commands["command_palette"] = [](MainWidget* widget) {return std::make_unique< CommandPaletteCommand>(widget); };
    new_commands["external_search"] = [](MainWidget* widget) {return std::make_unique< ExternalSearchCommand>(widget); };
    new_commands["open_selected_url"] = [](MainWidget* widget) {return std::make_unique< OpenSelectedUrlCommand>(widget); };
    new_commands["screen_down"] = [](MainWidget* widget) {return std::make_unique< ScreenDownCommand>(widget); };
    new_commands["screen_up"] = [](MainWidget* widget) {return std::make_unique< ScreenUpCommand>(widget); };
    new_commands["next_chapter"] = [](MainWidget* widget) {return std::make_unique< NextChapterCommand>(widget); };
    new_commands["prev_chapter"] = [](MainWidget* widget) {return std::make_unique< PrevChapterCommand>(widget); };
    new_commands["show_context_menu"] = [](MainWidget* widget) {return std::make_unique< ShowContextMenuCommand>(widget); };
    new_commands["toggle_dark_mode"] = [](MainWidget* widget) {return std::make_unique< ToggleDarkModeCommand>(widget); };
    new_commands["toggle_presentation_mode"] = [](MainWidget* widget) {return std::make_unique< TogglePresentationModeCommand>(widget); };
    new_commands["turn_on_presentation_mode"] = [](MainWidget* widget) {return std::make_unique< TurnOnPresentationModeCommand>(widget); };
    new_commands["toggle_mouse_drag_mode"] = [](MainWidget* widget) {return std::make_unique< ToggleMouseDragMode>(widget); };
    new_commands["toggle_freehand_drawing_mode"] = [](MainWidget* widget) {return std::make_unique< ToggleFreehandDrawingMode>(widget); };
    new_commands["toggle_pen_drawing_mode"] = [](MainWidget* widget) {return std::make_unique< TogglePenDrawingMode>(widget); };
    new_commands["toggle_scratchpad_mode"] = [](MainWidget* widget) {return std::make_unique< ToggleScratchpadMode>(widget); };
    new_commands["close_window"] = [](MainWidget* widget) {return std::make_unique< CloseWindowCommand>(widget); };
    new_commands["quit"] = [](MainWidget* widget) {return std::make_unique< QuitCommand>(widget); };
    new_commands["escape"] = [](MainWidget* widget) {return std::make_unique< EscapeCommand>(widget); };
    new_commands["toggle_pdf_annotations"] = [](MainWidget* widget) {return std::make_unique< TogglePDFAnnotationsCommand>(widget); };
    new_commands["q"] = [](MainWidget* widget) {return std::make_unique< CloseWindowCommand>(widget); };
    new_commands["open_link"] = [](MainWidget* widget) {return std::make_unique< OpenLinkCommand>(widget); };
    new_commands["overview_link"] = [](MainWidget* widget) {return std::make_unique< OverviewLinkCommand>(widget); };
    new_commands["portal_to_link"] = [](MainWidget* widget) {return std::make_unique< PortalToLinkCommand>(widget); };
    new_commands["copy_link"] = [](MainWidget* widget) {return std::make_unique< CopyLinkCommand>(widget); };
    new_commands["keyboard_select"] = [](MainWidget* widget) {return std::make_unique< KeyboardSelectCommand>(widget); };
    new_commands["keyboard_smart_jump"] = [](MainWidget* widget) {return std::make_unique< KeyboardSmartjumpCommand>(widget); };
    new_commands["keyboard_overview"] = [](MainWidget* widget) {return std::make_unique< KeyboardOverviewCommand>(widget); };
    new_commands["keys"] = [](MainWidget* widget) {return std::make_unique< KeysCommand>(widget); };
    new_commands["keys_user"] = [](MainWidget* widget) {return std::make_unique< KeysUserCommand>(widget); };
    new_commands["prefs"] = [](MainWidget* widget) {return std::make_unique< PrefsCommand>(widget); };
    new_commands["prefs_user"] = [](MainWidget* widget) {return std::make_unique< PrefsUserCommand>(widget); };
    new_commands["move_visual_mark_down"] = [](MainWidget* widget) {return std::make_unique< MoveVisualMarkDownCommand>(widget); };
    new_commands["move_visual_mark_up"] = [](MainWidget* widget) {return std::make_unique< MoveVisualMarkUpCommand>(widget); };
    new_commands["move_visual_mark_next"] = [](MainWidget* widget) {return std::make_unique< MoveVisualMarkNextCommand>(widget); };
    new_commands["move_visual_mark_prev"] = [](MainWidget* widget) {return std::make_unique< MoveVisualMarkPrevCommand>(widget); };
    new_commands["move_ruler_down"] = [](MainWidget* widget) {return std::make_unique< MoveVisualMarkDownCommand>(widget); };
    new_commands["move_ruler_up"] = [](MainWidget* widget) {return std::make_unique< MoveVisualMarkUpCommand>(widget); };
    new_commands["move_ruler_next"] = [](MainWidget* widget) {return std::make_unique< MoveVisualMarkNextCommand>(widget); };
    new_commands["move_ruler_prev"] = [](MainWidget* widget) {return std::make_unique< MoveVisualMarkPrevCommand>(widget); };
    new_commands["toggle_custom_color"] = [](MainWidget* widget) {return std::make_unique< ToggleCustomColorMode>(widget); };
    new_commands["set_select_highlight_type"] = [](MainWidget* widget) {return std::make_unique< SetSelectHighlightTypeCommand>(widget); };
    new_commands["set_freehand_type"] = [](MainWidget* widget) {return std::make_unique< SetFreehandType>(widget); };
    new_commands["toggle_window_configuration"] = [](MainWidget* widget) {return std::make_unique< ToggleWindowConfigurationCommand>(widget); };
    new_commands["prefs_user_all"] = [](MainWidget* widget) {return std::make_unique< PrefsUserAllCommand>(widget); };
    new_commands["keys_user_all"] = [](MainWidget* widget) {return std::make_unique< KeysUserAllCommand>(widget); };
    new_commands["fit_to_page_width_ratio"] = [](MainWidget* widget) {return std::make_unique< FitToPageWidthRatioCommand>(widget); };
    new_commands["smart_jump_under_cursor"] = [](MainWidget* widget) {return std::make_unique< SmartJumpUnderCursorCommand>(widget); };
    new_commands["download_paper_under_cursor"] = [](MainWidget* widget) {return std::make_unique< DownloadPaperUnderCursorCommand>(widget); };
    new_commands["download_paper_with_name"] = [](MainWidget* widget) {return std::make_unique< DownloadPaperWithNameCommand>(widget); };
    new_commands["overview_under_cursor"] = [](MainWidget* widget) {return std::make_unique< OverviewUnderCursorCommand>(widget); };
    new_commands["close_overview"] = [](MainWidget* widget) {return std::make_unique< CloseOverviewCommand>(widget); };
    new_commands["visual_mark_under_cursor"] = [](MainWidget* widget) {return std::make_unique< VisualMarkUnderCursorCommand>(widget); };
    new_commands["close_visual_mark"] = [](MainWidget* widget) {return std::make_unique< CloseVisualMarkCommand>(widget); };
    new_commands["zoom_in_cursor"] = [](MainWidget* widget) {return std::make_unique< ZoomInCursorCommand>(widget); };
    new_commands["zoom_out_cursor"] = [](MainWidget* widget) {return std::make_unique< ZoomOutCursorCommand>(widget); };
    new_commands["goto_left"] = [](MainWidget* widget) {return std::make_unique< GotoLeftCommand>(widget); };
    new_commands["goto_left_smart"] = [](MainWidget* widget) {return std::make_unique< GotoLeftSmartCommand>(widget); };
    new_commands["goto_right"] = [](MainWidget* widget) {return std::make_unique< GotoRightCommand>(widget); };
    new_commands["goto_right_smart"] = [](MainWidget* widget) {return std::make_unique< GotoRightSmartCommand>(widget); };
    new_commands["rotate_clockwise"] = [](MainWidget* widget) {return std::make_unique< RotateClockwiseCommand>(widget); };
    new_commands["rotate_counterclockwise"] = [](MainWidget* widget) {return std::make_unique< RotateCounterClockwiseCommand>(widget); };
    new_commands["goto_next_highlight"] = [](MainWidget* widget) {return std::make_unique< GotoNextHighlightCommand>(widget); };
    new_commands["goto_prev_highlight"] = [](MainWidget* widget) {return std::make_unique< GotoPrevHighlightCommand>(widget); };
    new_commands["goto_next_highlight_of_type"] = [](MainWidget* widget) {return std::make_unique< GotoNextHighlightOfTypeCommand>(widget); };
    new_commands["goto_prev_highlight_of_type"] = [](MainWidget* widget) {return std::make_unique< GotoPrevHighlightOfTypeCommand>(widget); };
    new_commands["add_highlight_with_current_type"] = [](MainWidget* widget) {return std::make_unique< AddHighlightWithCurrentTypeCommand>(widget); };
    new_commands["undo_drawing"] = [](MainWidget* widget) {return std::make_unique< UndoDrawingCommand>(widget); };
    new_commands["enter_password"] = [](MainWidget* widget) {return std::make_unique< EnterPasswordCommand>(widget); };
    new_commands["toggle_fastread"] = [](MainWidget* widget) {return std::make_unique< ToggleFastreadCommand>(widget); };
    new_commands["goto_top_of_page"] = [](MainWidget* widget) {return std::make_unique< GotoTopOfPageCommand>(widget); };
    new_commands["goto_bottom_of_page"] = [](MainWidget* widget) {return std::make_unique< GotoBottomOfPageCommand>(widget); };
    new_commands["new_window"] = [](MainWidget* widget) {return std::make_unique< NewWindowCommand>(widget); };
    new_commands["reload"] = [](MainWidget* widget) {return std::make_unique< ReloadCommand>(widget); };
    new_commands["reload_no_flicker"] = [](MainWidget* widget) {return std::make_unique< ReloadNoFlickerCommand>(widget); };
    new_commands["reload_config"] = [](MainWidget* widget) {return std::make_unique< ReloadConfigCommand>(widget); };
    new_commands["synctex_under_cursor"] = [](MainWidget* widget) {return std::make_unique< SynctexUnderCursorCommand>(widget); };
    new_commands["synctex_under_ruler"] = [](MainWidget* widget) {return std::make_unique< SynctexUnderRulerCommand>(widget); };
    new_commands["set_status_string"] = [](MainWidget* widget) {return std::make_unique< SetStatusStringCommand>(widget); };
    new_commands["clear_status_string"] = [](MainWidget* widget) {return std::make_unique< ClearStatusStringCommand>(widget); };
    new_commands["toggle_titlebar"] = [](MainWidget* widget) {return std::make_unique< ToggleTittlebarCommand>(widget); };
    new_commands["next_overview"] = [](MainWidget* widget) {return std::make_unique< NextPreviewCommand>(widget); };
    new_commands["previous_overview"] = [](MainWidget* widget) {return std::make_unique< PreviousPreviewCommand>(widget); };
    new_commands["goto_overview"] = [](MainWidget* widget) {return std::make_unique< GotoOverviewCommand>(widget); };
    new_commands["portal_to_overview"] = [](MainWidget* widget) {return std::make_unique< PortalToOverviewCommand>(widget); };
    new_commands["goto_selected_text"] = [](MainWidget* widget) {return std::make_unique< GotoSelectedTextCommand>(widget); };
    new_commands["focus_text"] = [](MainWidget* widget) {return std::make_unique< FocusTextCommand>(widget); };
    new_commands["download_overview_paper"] = [](MainWidget* widget) {return std::make_unique< DownloadOverviewPaperCommand>(widget); };
    new_commands["goto_window"] = [](MainWidget* widget) {return std::make_unique< GotoWindowCommand>(widget); };
    new_commands["toggle_smooth_scroll_mode"] = [](MainWidget* widget) {return std::make_unique< ToggleSmoothScrollModeCommand>(widget); };
    new_commands["goto_begining"] = [](MainWidget* widget) {return std::make_unique< GotoBeginningCommand>(widget); };
    new_commands["toggle_scrollbar"] = [](MainWidget* widget) {return std::make_unique< ToggleScrollbarCommand>(widget); };
    new_commands["overview_to_portal"] = [](MainWidget* widget) {return std::make_unique< OverviewToPortalCommand>(widget); };
    new_commands["overview_to_ruler_portal"] = [](MainWidget* widget) {return std::make_unique< OverviewRulerPortalCommand>(widget); };
    new_commands["goto_ruler_portal"] = [](MainWidget* widget) {return std::make_unique< GotoRulerPortalCommand>(widget); };
    new_commands["select_rect"] = [](MainWidget* widget) {return std::make_unique< SelectRectCommand>(widget); };
    new_commands["toggle_typing_mode"] = [](MainWidget* widget) {return std::make_unique< ToggleTypingModeCommand>(widget); };
    new_commands["donate"] = [](MainWidget* widget) {return std::make_unique< DonateCommand>(widget); };
    new_commands["overview_next_item"] = [](MainWidget* widget) {return std::make_unique< OverviewNextItemCommand>(widget); };
    new_commands["overview_prev_item"] = [](MainWidget* widget) {return std::make_unique< OverviewPrevItemCommand>(widget); };
    new_commands["delete_highlight_under_cursor"] = [](MainWidget* widget) {return std::make_unique< DeleteHighlightUnderCursorCommand>(widget); };
    new_commands["noop"] = [](MainWidget* widget) {return std::make_unique< NoopCommand>(widget); };
    new_commands["import"] = [](MainWidget* widget) {return std::make_unique< ImportCommand>(widget); };
    new_commands["export"] = [](MainWidget* widget) {return std::make_unique< ExportCommand>(widget); };
    new_commands["write_annotations_file"] = [](MainWidget* widget) {return std::make_unique< WriteAnnotationsFileCommand>(widget); };
    new_commands["load_annotations_file"] = [](MainWidget* widget) {return std::make_unique< LoadAnnotationsFileCommand>(widget); };
    new_commands["load_annotations_file_sync_deleted"] = [](MainWidget* widget) {return std::make_unique< LoadAnnotationsFileSyncDeletedCommand>(widget); };
    new_commands["enter_visual_mark_mode"] = [](MainWidget* widget) {return std::make_unique< EnterVisualMarkModeCommand>(widget); };
    new_commands["set_page_offset"] = [](MainWidget* widget) {return std::make_unique< SetPageOffsetCommand>(widget); };
    new_commands["toggle_visual_scroll"] = [](MainWidget* widget) {return std::make_unique< ToggleVisualScrollCommand>(widget); };
    new_commands["toggle_horizontal_scroll_lock"] = [](MainWidget* widget) {return std::make_unique< ToggleHorizontalLockCommand>(widget); };
    new_commands["execute"] = [](MainWidget* widget) {return std::make_unique< ExecuteCommand>(widget); };
    new_commands["embed_annotations"] = [](MainWidget* widget) {return std::make_unique< EmbedAnnotationsCommand>(widget); };
    new_commands["import_annotations"] = [](MainWidget* widget) {return std::make_unique< ImportAnnotationsCommand>(widget); };
    new_commands["copy_window_size_config"] = [](MainWidget* widget) {return std::make_unique< CopyWindowSizeConfigCommand>(widget); };
    new_commands["toggle_select_highlight"] = [](MainWidget* widget) {return std::make_unique< ToggleSelectHighlightCommand>(widget); };
    new_commands["open_last_document"] = [](MainWidget* widget) {return std::make_unique< OpenLastDocumentCommand>(widget); };
    new_commands["toggle_statusbar"] = [](MainWidget* widget) {return std::make_unique< ToggleStatusbarCommand>(widget); };
    new_commands["start_reading"] = [](MainWidget* widget) {return std::make_unique< StartReadingCommand>(widget); };
    new_commands["stop_reading"] = [](MainWidget* widget) {return std::make_unique< StopReadingCommand>(widget); };
    new_commands["scan_new_files"] = [](MainWidget* widget) {return std::make_unique< ScanNewFilesFromScanDirCommand>(widget); };
    new_commands["add_marked_data"] = [](MainWidget* widget) {return std::make_unique< AddMarkedDataCommand>(widget); };
    new_commands["remove_marked_data"] = [](MainWidget* widget) {return std::make_unique< RemoveMarkedDataCommand>(widget); };
    new_commands["export_marked_data"] = [](MainWidget* widget) {return std::make_unique< ExportMarkedDataCommand>(widget); };
    new_commands["undo_marked_data"] = [](MainWidget* widget) {return std::make_unique< UndoMarkedDataCommand>(widget); };
    new_commands["goto_random_page"] = [](MainWidget* widget) {return std::make_unique< GotoRandomPageCommand>(widget); };
    new_commands["clear_current_page_drawings"] = [](MainWidget* widget) {return std::make_unique< ClearCurrentPageDrawingsCommand>(widget); };
    new_commands["clear_current_document_drawings"] = [](MainWidget* widget) {return std::make_unique< ClearCurrentDocumentDrawingsCommand>(widget); };
    new_commands["delete_freehand_drawings"] = [](MainWidget* widget) {return std::make_unique< DeleteFreehandDrawingsCommand>(widget); };
    new_commands["select_freehand_drawings"] = [](MainWidget* widget) {return std::make_unique< SelectFreehandDrawingsCommand>(widget); };
    new_commands["select_current_search_match"] = [](MainWidget* widget) {return std::make_unique< SelectCurrentSearchMatchCommand>(widget); };
    new_commands["show_touch_main_menu"] = [](MainWidget* widget) {return std::make_unique< ShowTouchMainMenu>(widget); };
    new_commands["show_touch_settings_menu"] = [](MainWidget* widget) {return std::make_unique< ShowTouchSettingsMenu>(widget); };
    new_commands["show_touch_draw_controls"] = [](MainWidget* widget) {return std::make_unique< ShowTouchDrawingMenu>(widget); };
    //new_commands["stop_search"] = [](MainWidget* widget) {return std::make_unique< StopSearchCommand>(widget); };

//#ifdef _DEBUG
    new_commands["debug"] = [](MainWidget* widget) {return std::make_unique< DebugCommand>(widget); };
    new_commands["export_python_api"] = [](MainWidget* widget) {return std::make_unique< ExportPythonApiCommand>(widget); };
    new_commands["export_default_config_file"] = [](MainWidget* widget) {return std::make_unique< ExportDefaultConfigFile>(widget); };
    new_commands["export_command_names"] = [](MainWidget* widget) {return std::make_unique< ExportCommandNamesCommand>(widget); };
    new_commands["export_config_names"] = [](MainWidget* widget) {return std::make_unique< ExportConfigNamesCommand>(widget); };
    new_commands["test_command"] = [](MainWidget* widget) {return std::make_unique< TestCommand>(widget); };
    new_commands["print_undocumented_commands"] = [](MainWidget* widget) {return std::make_unique< PrintUndocumentedCommandsCommand>(widget); };
    new_commands["print_undocumented_configs"] = [](MainWidget* widget) {return std::make_unique< PrintUndocumentedConfigsCommand>(widget); };
    new_commands["print_non_default_configs"] = [](MainWidget* widget) {return std::make_unique< PrintNonDefaultConfigs>(widget); };
//#endif

    command_human_readable_names["goto_beginning"] = "Go to the beginning of the document";
    command_human_readable_names["goto_end"] = "Go to the end of the document";
    command_human_readable_names["goto_definition"] = "Go to the reference in current highlighted line";
    command_human_readable_names["overview_definition"] = "Open an overview to the reference in current highlighted line";
    command_human_readable_names["portal_to_definition"] = "Create a portal to the definition in current highlighted line";
    command_human_readable_names["goto_tab"] = "Open tab";
    command_human_readable_names["next_item"] = "Go to next search result";
    command_human_readable_names["previous_item"] = "Go to previous search result";
    command_human_readable_names["toggle_text_mark"] = "Move text cursor to other end of selection";
    command_human_readable_names["move_text_mark_forward"] = "Move text cursor forward";
    command_human_readable_names["move_text_mark_backward"] = "Move text cursor backward";
    command_human_readable_names["move_text_mark_forward_word"] = "Move text cursor forward to the next word";
    command_human_readable_names["move_text_mark_backward_word"] = "Move text cursor backward to the previous word";
    command_human_readable_names["move_text_mark_down"] = "Move text cursor down";
    command_human_readable_names["move_text_mark_up"] = "Move text cursor up";
    command_human_readable_names["set_mark"] = "Set mark in current location";
    command_human_readable_names["toggle_drawing_mask"] = "Toggle drawing type visibility";
    command_human_readable_names["turn_on_all_drawings"] = "Make all freehand drawings visible";
    command_human_readable_names["turn_off_all_drawings"] = "Make all freehand drawings invisible";
    command_human_readable_names["goto_mark"] = "Go to marked location";
    command_human_readable_names["goto_page_with_page_number"] = "Go to page with page number";
    command_human_readable_names["edit_selected_bookmark"] = "Edit selected bookmark";
    command_human_readable_names["edit_selected_highlight"] = "Edit the text comment of current selected highlight";
    command_human_readable_names["search"] = "Search";
    command_human_readable_names["add_annot_to_selected_highlight"] = "Add annotation to selected highlight";
    command_human_readable_names["set_freehand_thickness"] = "Set thickness of freehand drawings";
    command_human_readable_names["goto_page_with_label"] = "Go to page with label";
    command_human_readable_names["regex_search"] = "Search using regular expression";
    command_human_readable_names["chapter_search"] = "Search current chapter";
    command_human_readable_names["move_down"] = "Move down";
    command_human_readable_names["move_up"] = "Move up";
    command_human_readable_names["move_left"] = "Move left";
    command_human_readable_names["move_right"] = "Move right";
    command_human_readable_names["zoom_in"] = "Zoom in";
    command_human_readable_names["zoom_out"] = "Zoom out";
    command_human_readable_names["fit_to_page_width"] = "Fit the page to screen width";
    command_human_readable_names["fit_to_page_height"] = "Fit the page to screen height";
    command_human_readable_names["fit_to_page_height_smart"] = "Fit the page to screen height, ignoring white page margins";
    command_human_readable_names["fit_to_page_width_smart"] = "Fit the page to screen width, ignoring white page margins";
    command_human_readable_names["next_page"] = "Go to next page";
    command_human_readable_names["previous_page"] = "Go to previous page";
    command_human_readable_names["open_document"] = "Open documents using native file explorer";
    command_human_readable_names["add_bookmark"] = "Add an invisible bookmark in the current location";
    command_human_readable_names["add_marked_bookmark"] = "Add a bookmark in the selected location";
    command_human_readable_names["add_freetext_bookmark"] = "Add a text bookmark in the selected rectangle";
    command_human_readable_names["add_highlight"] = "Highlight selected text";
    command_human_readable_names["goto_toc"] = "Open table of contents";
    command_human_readable_names["goto_highlight"] = "Open the highlight list of the current document";
    command_human_readable_names["increase_freetext_font_size"] = "Increase freetext bookmark font size";
    command_human_readable_names["decrease_freetext_font_size"] = "Decrease freetext bookmark font size";
    command_human_readable_names["goto_bookmark"] = "Open the bookmark list of current document";
    command_human_readable_names["goto_bookmark_g"] = "Open the bookmark list of all documents";
    command_human_readable_names["goto_highlight_g"] = "Open the highlight list of the all documents";
    command_human_readable_names["link"] = "Go to closest portal destination";
    command_human_readable_names["portal"] = "Start creating a portal";
    command_human_readable_names["next_state"] = "Go forward in history";
    command_human_readable_names["prev_state"] = "Go backward in history";
    command_human_readable_names["delete_link"] = "Alias for delete_portal";
    command_human_readable_names["delete_portal"] = "Delete the closest portal";
    command_human_readable_names["delete_bookmark"] = "Delete the closest bookmark";
    command_human_readable_names["delete_highlight"] = "Delete the selected highlight";
    command_human_readable_names["goto_link"] = "Alias for goto_portal";
    command_human_readable_names["goto_portal"] = "Goto closest portal destination";
    command_human_readable_names["edit_link"] = "Alias for edit_portal";
    command_human_readable_names["edit_portal"] = "Edit portal";
    command_human_readable_names["open_prev_doc"] = "Open the list of previously opened documents";
    command_human_readable_names["open_document_embedded"] = "Open an embedded file explorer";
    command_human_readable_names["open_document_embedded_from_current_path"] = "Open an embedded file explorer, starting in the directory of current document";
    command_human_readable_names["copy"] = "Copy";
    command_human_readable_names["toggle_fullscreen"] = "Toggle fullscreen mode";
    command_human_readable_names["toggle_one_window"] = "Open/close helper window";
    command_human_readable_names["toggle_highlight"] = "Toggle whether PDF links are highlighted";
    command_human_readable_names["toggle_synctex"] = "Toggle synctex mode";
    command_human_readable_names["turn_on_synctex"] = "Turn synxtex mode on";
    command_human_readable_names["toggle_show_last_command"] = "Toggle whether the last command is shown in statusbar";
    command_human_readable_names["command"] = "Open a list of all sioyek commands";
    command_human_readable_names["external_search"] = "Search using external search engines";
    command_human_readable_names["open_selected_url"] = "Open selected URL in a browser";
    command_human_readable_names["screen_down"] = "Move screen down";
    command_human_readable_names["screen_up"] = "Move screen up";
    command_human_readable_names["next_chapter"] = "Go to next chapter";
    command_human_readable_names["prev_chapter"] = "Go to previous chapter";
    command_human_readable_names["toggle_dark_mode"] = "Toggle dark mode";
    command_human_readable_names["toggle_presentation_mode"] = "Toggle presentation mode";
    command_human_readable_names["turn_on_presentation_mode"] = "Turn on presentation mode";
    command_human_readable_names["toggle_mouse_drag_mode"] = "Toggle mouse drag mode";
    command_human_readable_names["toggle_freehand_drawing_mode"] = "Toggle freehand drawing mode";
    command_human_readable_names["toggle_pen_drawing_mode"] = "Toggle pen drawing mode";
    command_human_readable_names["close_window"] = "Close window";
    command_human_readable_names["quit"] = "Quit";
    command_human_readable_names["escape"] = "Escape";
    command_human_readable_names["toggle_pdf_annotations"] = "Toggle whether PDF annotations should be rendered";
    command_human_readable_names["q"] = "";
    command_human_readable_names["open_link"] = "Go to PDF links using keyboard";
    command_human_readable_names["overview_link"] = "Overview to PDF links using keyboard";
    command_human_readable_names["portal_to_link"] = "Create a portal to PDF links using keyboard";
    command_human_readable_names["copy_link"] = "Copy URL of PDF links using keyboard";
    command_human_readable_names["keyboard_select"] = "Select text using keyboard";
    command_human_readable_names["keyboard_smart_jump"] = "Smart jump using keyboard";
    command_human_readable_names["keyboard_overview"] = "Open an overview using keyboard";
    command_human_readable_names["keys"] = "Open the default keys config file";
    command_human_readable_names["keys_user"] = "Open the default keys_user config file";
    command_human_readable_names["prefs"] = "Open the default prefs config file";
    command_human_readable_names["prefs_user"] = "Open the default prefs_user config file";
    command_human_readable_names["move_visual_mark_down"] = "Move current highlighted line down";
    command_human_readable_names["move_visual_mark_up"] = "Move current highlighted line up";
    command_human_readable_names["move_visual_mark_next"] = "Move the current highlighted line to the next unread text";
    command_human_readable_names["move_visual_mark_prev"] = "Move the current highlighted line to the previous";
    command_human_readable_names["toggle_custom_color"] = "Toggle custom color mode";
    command_human_readable_names["set_select_highlight_type"] = "Set the selected highlight type";
    command_human_readable_names["set_freehand_type"] = "Set the freehand drawing color type";
    command_human_readable_names["toggle_window_configuration"] = "Toggle between one window and two window configuration";
    command_human_readable_names["prefs_user_all"] = "List all user keys config files";
    command_human_readable_names["keys_user_all"] = "List all user keys config files";
    command_human_readable_names["fit_to_page_width_ratio"] = "Fit page to a percentage of window width";
    command_human_readable_names["smart_jump_under_cursor"] = "Perform a smart jump to the reference under cursor";
    command_human_readable_names["download_paper_under_cursor"] = "Try to download the paper name under cursor";
    command_human_readable_names["overview_under_cursor"] = "Open an overview to the reference under cursor";
    command_human_readable_names["close_overview"] = "Close overview window";
    command_human_readable_names["visual_mark_under_cursor"] = "Highlight the line under cursor";
    command_human_readable_names["close_visual_mark"] = "Stop ruler mode";
    command_human_readable_names["zoom_in_cursor"] = "Zoom in centered on mouse cursor";
    command_human_readable_names["zoom_out_cursor"] = "Zoom out centered on mouse cursor";
    command_human_readable_names["goto_left"] = "Go to far left side of the page";
    command_human_readable_names["goto_left_smart"] = "Go to far left side of the page, ignoring white page margins";
    command_human_readable_names["goto_right"] = "Go to far right side of the page";
    command_human_readable_names["goto_right_smart"] = "Go to far right side of the page, ignoring white page margins";
    command_human_readable_names["rotate_clockwise"] = "Rotate clockwise";
    command_human_readable_names["rotate_counterclockwise"] = "Rotate counter clockwise";
    command_human_readable_names["goto_next_highlight"] = "Go to the next highlight";
    command_human_readable_names["goto_prev_highlight"] = "Go to the previous highlight";
    command_human_readable_names["goto_next_highlight_of_type"] = "Go to the next highlight with the current highlight type";
    command_human_readable_names["goto_prev_highlight_of_type"] = "Go to the previous highlight with the current highlight type";
    command_human_readable_names["add_highlight_with_current_type"] = "Highlight selected text with current selected highlight type";
    command_human_readable_names["undo_drawing"] = "Undo freehand drawing";
    command_human_readable_names["enter_password"] = "Enter password";
    command_human_readable_names["toggle_fastread"] = "Go to top of current page";
    command_human_readable_names["goto_top_of_page"] = "Go to top of current page";
    command_human_readable_names["goto_bottom_of_page"] = "Go to bottom of current page";
    command_human_readable_names["new_window"] = "Open a new window";
    command_human_readable_names["reload"] = "Reload document";
    command_human_readable_names["reload_no_flicker"] = "Reload document with no screen flickering";
    command_human_readable_names["reload_config"] = "Reload configs";
    command_human_readable_names["synctex_under_cursor"] = "Perform a synctex search to the tex file location corresponding to cursor location";
    command_human_readable_names["set_status_string"] = "Set custom message to be shown in statusbar";
    command_human_readable_names["clear_status_string"] = "Clear custom statusbar message";
    command_human_readable_names["toggle_titlebar"] = "Toggle window titlebar";
    command_human_readable_names["next_overview"] = "Go to the next candidate in overview window";
    command_human_readable_names["previous_overview"] = "Go to the previous candidate in overview window";
    command_human_readable_names["goto_overview"] = "Go to the current overview location";
    command_human_readable_names["portal_to_overview"] = "Create a portal to the current overview location";
    command_human_readable_names["goto_selected_text"] = "Go to the location of current selected text";
    command_human_readable_names["focus_text"] = "Focus on the given text";
    command_human_readable_names["goto_window"] = "Open a list of all sioyek windows";
    command_human_readable_names["toggle_smooth_scroll_mode"] = "Toggle smooth scroll mode";
    command_human_readable_names["goto_begining"] = "";
    command_human_readable_names["toggle_scrollbar"] = "Toggle scrollbar";
    command_human_readable_names["overview_to_portal"] = "Open an overview to the closest portal";
    command_human_readable_names["select_rect"] = "Select a rectangle to be used in other commands";
    command_human_readable_names["toggle_typing_mode"] = "Toggle typing minigame";
    command_human_readable_names["donate"] = "Donate to support sioyek's development";
    command_human_readable_names["overview_next_item"] = "Open an overview to the next search result";
    command_human_readable_names["overview_prev_item"] = "Open an overview to the previous search result";
    command_human_readable_names["delete_highlight_under_cursor"] = "Delete highlight under mouse cursor";
    command_human_readable_names["noop"] = "Do nothing";
    command_human_readable_names["import"] = "Import annotation data from a json file";
    command_human_readable_names["export"] = "Export annotation data to a json file";
    command_human_readable_names["enter_visual_mark_mode"] = "Enter ruler mode using keyboard";
    command_human_readable_names["set_page_offset"] = "Toggle visual scroll mode";
    command_human_readable_names["toggle_visual_scroll"] = "Toggle visual scroll mode";
    command_human_readable_names["toggle_horizontal_scroll_lock"] = "Toggle horizontal lock";
    command_human_readable_names["execute"] = "Execute shell command";
    command_human_readable_names["embed_annotations"] = "Embed sioyek annotations into native PDF annotations";
    command_human_readable_names["import_annotations"] = "Import PDF annotations into sioyek";
    command_human_readable_names["copy_window_size_config"] = "Copy current window size configuration";
    command_human_readable_names["toggle_select_highlight"] = "Toggle select highlight mode";
    command_human_readable_names["open_last_document"] = "Switch to previous opened document";
    command_human_readable_names["toggle_statusbar"] = "Toggle statusbar";
    command_human_readable_names["start_reading"] = "Read using text to speech";
    command_human_readable_names["stop_reading"] = "Stop reading";
    command_human_readable_names["debug"] = "debug";
    command_human_readable_names["add_marked_data"] = "[internal]";
    command_human_readable_names["remove_marked_data"] = "[internal]";
    command_human_readable_names["export_marked_data"] = "[internal]";
    command_human_readable_names["undo_marked_data"] = "[internal]";
    command_human_readable_names["goto_random_page"] = "[internal]";
    command_human_readable_names["delete_freehand_drawings"] = "Add a text bookmark in the selected rectangle";
    command_human_readable_names["select_freehand_drawings"] = "Select freehand drawings";

    for (auto [command_name_, command_value] : ADDITIONAL_COMMANDS) {
        std::string command_name = utf8_encode(command_name_);
        std::wstring local_command_value = command_value;
        new_commands[command_name] = [command_name, local_command_value, this](MainWidget* w) {return  std::make_unique<CustomCommand>(w, command_name, local_command_value); };
    }


    for (auto [command_name_, command_files_pair] : ADDITIONAL_JAVASCRIPT_COMMANDS) {
        handle_new_javascript_command(command_name_, command_files_pair, false);
    }

    for (auto [command_name_, command_files_pair] : ADDITIONAL_ASYNC_JAVASCRIPT_COMMANDS) {
        handle_new_javascript_command(command_name_, command_files_pair, true);
    }

    for (auto [command_name_, macro_value] : ADDITIONAL_MACROS) {
        std::string command_name = utf8_encode(command_name_);
        std::wstring local_macro_value = macro_value;
        new_commands[command_name] = [command_name, local_macro_value, this](MainWidget* w) {return std::make_unique<MacroCommand>(w, this, command_name, local_macro_value); };
    }

    std::vector<Config> configs = config_manager->get_configs();

    for (auto conf : configs) {

        std::string confname = utf8_encode(conf.name);
        std::string config_set_command_name = "setconfig_" + confname;
        //commands.push_back({ config_set_command_name, true, false , false, false, true, {} });
        new_commands[config_set_command_name] = [confname, config_manager](MainWidget* w) {return std::make_unique<ConfigCommand>(w, confname, config_manager); };

        if (conf.config_type == ConfigType::Bool) {
            std::string config_toggle_command_name = "toggleconfig_" + confname;
            new_commands[config_toggle_command_name] = [confname, config_manager](MainWidget* w) {return std::make_unique<ToggleConfigCommand>(w, confname); };
        }

    }

    QDateTime current_time = QDateTime::currentDateTime();
    for (auto [command_name, _] : new_commands) {
        command_last_uses[command_name] = current_time;
    }

}

void CommandManager::update_command_last_use(std::string command_name) {
    command_last_uses[command_name] = QDateTime::currentDateTime();
}

void CommandManager::handle_new_javascript_command(std::wstring command_name_, std::pair<std::wstring, std::wstring> command_files_pair, bool is_async) {
        std::string command_name = utf8_encode(command_name_);
        auto [command_parent_file_path, command_file_path] = command_files_pair;

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
            new_commands[command_name] = [command_name, code, is_async, this](MainWidget* w) {return std::make_unique<JavascriptCommand>(command_name, code.toStdWString(), is_async, w); };
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

        std::sort(pairs.begin(), pairs.end(), [](const auto& a, const auto& b) {return a.first > b.first; });

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
    const std::vector<std::vector<std::string>>& command_names,
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
                        std::wcerr
                            << L"Warning: key defined in " << command_file_names[j]
                            << L":" << command_line_numbers[j]
                            << L" for " << utf8_decode(command_names[j][0])
                            << L" is unreachable, shadowed by final key sequence defined in "
                            << parent_node->defining_file_path
                            << L":" << parent_node->defining_file_line << L"\n";
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
                if ((parent_node->name_.size() == 0) || parent_node->name_[0].compare(command_names[j][0]) != 0) {

                    LOG(std::wcerr << L"Warning: key defined in " << parent_node->defining_file_path
                        << L":" << parent_node->defining_file_line
                        << L" overwritten by " << command_file_names[j]
                        << L":" << command_line_numbers[j]);
                    if (parent_node->name_.size() > 0) {
                        LOG(std::wcerr << L". Overriding command: " << line
                            << L": replacing " << utf8_decode(parent_node->name_[0])
                            << L" with " << utf8_decode(command_names[j][0]));
                    }
                    LOG(std::wcerr << L"\n");
                }
            }
            if ((size_t)i == (tokens.size() - 1)) {
                parent_node->is_final = true;
                std::vector<std::string> previous_names = std::move(parent_node->name_);
                parent_node->name_ = {};
                parent_node->defining_file_line = command_line_numbers[j];
                parent_node->defining_file_path = command_file_names[j];
                for (size_t k = 0; k < command_names[j].size(); k++) {
                    parent_node->name_.push_back(command_names[j][k]);
                }
                if (command_names[j].size() == 1 && (command_names[j][0].find("[") == -1) && (command_names[j][0].find("(") == -1)) {
                    if (command_manager->new_commands.find(command_names[j][0]) != command_manager->new_commands.end()) {
                        parent_node->generator = command_manager->new_commands[command_names[j][0]];
                    }
                    else {
                        std::wcerr << L"Warning: command " << utf8_decode(command_names[j][0]) << L" used in " << parent_node->defining_file_path
                            << L":" << parent_node->defining_file_line << L" not found.\n";
                    }
                }
                else {
                    QStringList command_parts;
                    for (int k = 0; k < command_names[j].size(); k++) {
                        command_parts.append(QString::fromStdString(command_names[j][k]));
                    }

                    // is the command incomplete and should be appended to previous command instead of replacing it?
                    if (is_command_incomplete_macro(command_names[j])) {
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
                if (parent_node->is_final && (parent_node->name_.size() > 0)) {
                    std::wcerr << L"Warning: unmapping " << utf8_decode(parent_node->name_[0]) << L" because of " << utf8_decode(command_names[j][0]) << L" which uses " << line << L"\n";
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
    const std::vector<std::vector<std::string>>& command_names,
    const std::vector<std::wstring>& command_file_names,
    const std::vector<int>& command_line_numbers
) {
    // parse key configs into a trie where leaves are annotated with the name of the command

    InputParseTreeNode* root = new InputParseTreeNode;
    root->is_root = true;

    parse_lines(root, command_manager, lines, command_names, command_file_names, command_line_numbers);

    return root;

}

std::vector<std::string> parse_command_name(const std::wstring& command_names) {
    QStringList parts = QString::fromStdWString(command_names).split(';');
    std::vector<std::string> res;
    for (int i = 0; i < parts.size(); i++) {
        res.push_back(parts.at(i).toStdString());
    }
    return res;
}

void get_keys_file_lines(const Path& file_path,
    std::vector<std::vector<std::string>>& command_names,
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

        QString line_string = QString::fromStdWString(line);
        int last_space_index = line_string.lastIndexOf(' ');

        if (last_space_index >= 0){
            std::wstring command_name = line_string.left(last_space_index).trimmed().toStdWString();
            std::wstring command_key = line_string.right(line_string.size() - last_space_index - 1).trimmed().toStdWString();
            
            command_names.push_back(parse_command_name(command_name));
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

    std::vector<std::vector<std::string>> command_names;
    std::vector<std::wstring> command_keys;
    std::vector<std::wstring> command_files;
    std::vector<int> command_line_numbers;

    get_keys_file_lines(default_path, command_names, command_keys, command_files, command_line_numbers);
    for (auto upath : user_paths) {
        get_keys_file_lines(upath, command_names, command_keys, command_files, command_line_numbers);
    }

    for (auto additional_keymap : ADDITIONAL_KEYMAPS) {
        QString keymap_string = QString::fromStdWString(additional_keymap.keymap_string);
        int last_space_index = keymap_string.lastIndexOf(' ');
        std::wstring command_name = keymap_string.left(last_space_index).toStdWString();
        std::wstring mapping = keymap_string.right(keymap_string.size() - last_space_index - 1).toStdWString();
        command_names.push_back(parse_command_name(command_name));
        command_keys.push_back(mapping);
        command_files.push_back(additional_keymap.file_name);
        command_line_numbers.push_back(additional_keymap.line_number);
    }

    return parse_lines(command_manager, command_keys, command_names, command_files, command_line_numbers);
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

std::unique_ptr<Command> InputHandler::handle_key(MainWidget* w, QKeyEvent* key_event, bool shift_pressed, bool control_pressed, bool alt_pressed, int* num_repeats) {
    int key = 0;
    if (!USE_LEGACY_KEYBINDS) {
        std::vector<QString> special_texts = { "\b", "\t", " ", "\r", "\n" };
        if (((key_event->key() >= 'A') && (key_event->key() <= 'Z')) || ((key_event->text().size() > 0) &&
            (std::find(special_texts.begin(), special_texts.end(), key_event->text()) == special_texts.end()))) {
            if (!control_pressed && !alt_pressed) {
                // shift is already handled in the returned text
                shift_pressed = false;
                std::wstring text = key_event->text().toStdWString();
                key = key_event->text().toStdWString()[0];
            }
            else {
                auto text = key_event->text();
                key = key_event->key();

                if ((key >= 'A' && key <= 'Z') && (!shift_pressed)) {
                    if (!shift_pressed) {
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
    std::wcerr << "Warning: invalid command (key:" << (char)key << "); resetting to root" << std::endl;
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
    on_result_computed();
    widget->remove_command_being_performed(this);
}

std::vector<char> Command::special_symbols() {
    std::vector<char> res;
    return res;
}


std::string Command::get_name() {
    return "";
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
