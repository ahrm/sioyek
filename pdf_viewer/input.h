#pragma once

#include <vector>
#include <string>
#include <memory>
#include <optional>
#include <unordered_map>
#include <mutex>
#include <qdatetime.h>

#include "path.h"
#include "coordinates.h"
#include "utils.h"

class QLocalSocket;
class MainWidget;
class ConfigManager;

enum RequirementType {
    Text,
    Symbol,
    File,
    Folder,
    Rect,
    Point,
    Generic
};

struct Requirement {
    RequirementType type;
    std::string name;
};

class Command {
private:
    virtual void perform() = 0;
protected:
    int num_repeats = 1;
    MainWidget* widget = nullptr;
    std::optional<std::wstring> result = {};
    std::string command_cname;
public:

    static inline const bool developer_only = false;
    QLocalSocket* result_socket = nullptr;
    std::wstring* result_holder = nullptr;
    bool* is_done = nullptr;

    Command(std::string name, MainWidget* widget);
    virtual std::optional<Requirement> next_requirement(MainWidget* widget);
    virtual std::optional<std::wstring> get_result();

    virtual void set_text_requirement(std::wstring value);
    virtual void set_symbol_requirement(char value);
    virtual void set_file_requirement(std::wstring value);
    virtual void set_rect_requirement(AbsoluteRect value);
    virtual void set_point_requirement(AbsoluteDocumentPos value);
    virtual void set_generic_requirement(QVariant value);
    virtual void handle_generic_requirement();
    virtual void set_num_repeats(int nr);
    virtual std::vector<char> special_symbols();
    virtual void pre_perform();
    virtual bool pushes_state();
    virtual bool requires_document();
    virtual void on_cancel();
    virtual void on_result_computed();
    virtual void set_result_socket(QLocalSocket* result_socket);
    virtual void set_result_mutex(bool* res_mut, std::wstring* result_location);
    virtual std::optional<std::wstring> get_text_suggestion(int index);
    virtual bool is_menu_command();
    virtual void perform_up();
    virtual bool is_holdable();
    virtual void on_key_hold();
    virtual void on_text_change(const QString& new_text);

    void set_next_requirement_with_string(std::wstring str);

    virtual void run();
    virtual std::string get_name();
    virtual std::string get_human_readable_name();
    virtual std::wstring get_text_default_value();
    virtual ~Command();
};


class CommandManager {
private:
    //std::vector<Command> commands;
public:

    std::map < std::string, std::function<std::unique_ptr<Command>(MainWidget*)> > new_commands;
    std::map<std::string, std::string> command_human_readable_names;
    std::map<std::string, QDateTime> command_last_uses;
    std::unordered_map<QString, QString> command_required_prefixes;

    CommandManager(ConfigManager* config_manager);
    std::unique_ptr<Command> get_command_with_name(MainWidget* w, std::string name);
    std::unique_ptr<Command> create_macro_command(MainWidget* w, std::string name, std::wstring macro_string);
    QStringList get_all_command_names();
    void handle_new_javascript_command(std::wstring command_name, JsCommandInfo command_files_pair, bool is_async);
    void update_command_last_use(std::string command_name);

    template<typename T>
    void register_command(std::string alias_name="") {
        bool is_developer_mode = false;

#ifdef SIOYEK_DEVELOPER
        is_developer_mode = true;
#endif

        if (is_developer_mode || !T::developer_only) {
            std::string name = alias_name.size() > 0 ? alias_name : T::cname;
            bool is_alias = alias_name.size() > 0;
            new_commands[name]  = [](MainWidget* widget) {return std::make_unique<T>(widget); };
            command_human_readable_names[name] = is_alias ? "alias for " + T::cname : T::hname;
            command_required_prefixes[QString::fromStdString(name)] = "";
        }
    }
};

struct InputParseTreeNode {

    std::vector<InputParseTreeNode*> children;
    //char command;
    int command;
    //std::vector<std::string> name;
    std::vector<std::string> name_;
    std::optional<std::function<std::unique_ptr<Command>(MainWidget*)>> generator = {};
    bool shift_modifier = false;
    bool control_modifier = false;
    bool alt_modifier = false;
    bool command_modifier = false;
    bool requires_text = false;
    bool requires_symbol = false;
    bool is_root = false;
    bool is_final = false;

    // todo: use a pointer to reduce allocation
    std::wstring defining_file_path;
    int defining_file_line;

    bool is_same(const InputParseTreeNode* other);
    bool matches(int key, bool shift, bool ctrl, bool cmd, bool alt);
};



class InputHandler {
private:
    InputParseTreeNode* root = nullptr;
    InputParseTreeNode* current_node = nullptr;
    CommandManager* command_manager;
    std::string number_stack;
    std::vector<Path> user_key_paths;

    std::string get_key_string_from_tree_node_sequence(const std::vector<InputParseTreeNode*> seq) const;
    std::string get_key_name_from_key_code(int key_code) const;

    void add_command_key_mappings(InputParseTreeNode* root, std::unordered_map<std::string, std::vector<std::string>>& map, std::vector<InputParseTreeNode*> prefix) const;
public:
    //char create_link_sumbol = 0;
    //char create_bookmark_symbol = 0;

    InputHandler(const Path& default_path, const std::vector<Path>& user_paths, CommandManager* cm);
    void reload_config_files(const Path& default_path, const std::vector<Path>& user_path);
    //std::vector<std::unique_ptr<Command>> handle_key(QKeyEvent* key_event, bool shift_pressed, bool control_pressed, bool alt_pressed ,int* num_repeats);
    int get_event_key(QKeyEvent* key_event, bool* shift_pressed, bool* control_pressed, bool* is_command_pressed, bool* alt_pressed);
    std::unique_ptr<Command> handle_key(MainWidget* w, QKeyEvent* key_event, bool shift_pressed, bool control_pressed, bool is_command_pressed, bool alt_pressed, int* num_repeats);
    std::unique_ptr<Command> get_menu_command(MainWidget* w, QKeyEvent* key_event, bool shift_pressed, bool control_pressed, bool command_pressed, bool alt_pressed);
    void delete_current_parse_tree(InputParseTreeNode* node_to_delete);

    std::optional<Path> get_or_create_user_keys_path();
    std::vector<Path> get_all_user_keys_paths();
    std::unordered_map<std::string, std::vector<std::string>> get_command_key_mappings() const;

};

bool is_macro_command_enabled(Command* command);

class KeyboardSelectPointCommand : public Command {
protected:
    std::optional<std::wstring> text = {};
    bool already_pre_performed = false;
    std::unique_ptr<Command> origin;
    bool requires_rect = false;
public:

    KeyboardSelectPointCommand(MainWidget* w, std::unique_ptr<Command> original_command);

    bool is_done();

    virtual std::optional<Requirement> next_requirement(MainWidget* widget);

    virtual void perform();
    virtual void on_cancel() override;

    void pre_perform();

    virtual std::string get_name();


    virtual void set_symbol_requirement(char value);
};

struct MenuItems;

using RecursiveItem = std::variant<std::unique_ptr<Command>, std::unique_ptr<MenuItems>>;

struct MenuItems {
    std::wstring name;
    std::vector<RecursiveItem> items;
};

std::unique_ptr<MenuItems> parse_menu_string(MainWidget* widget, QString name, QString menu_string);
