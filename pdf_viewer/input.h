#pragma once

#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <memory>
#include <optional>
#include <unordered_map>

#include "utils.h"
#include "path.h"
#include "config.h"

class MainWidget;

enum RequirementType {
	Text,
	Symbol,
	File,
	Rect
};

struct Requirement {
	RequirementType type;
	std::string name;
};

class Command {
private:
	virtual void perform(MainWidget* widget) = 0;
protected:
	int num_repeats = 1;
	MainWidget* widget = nullptr;
public:
	virtual std::optional<Requirement> next_requirement(MainWidget* widget);

	virtual void set_text_requirement(std::wstring value);
	virtual void set_symbol_requirement(char value);
	virtual void set_file_requirement(std::wstring value);
	virtual void set_rect_requirement(fz_rect value);
	virtual void set_num_repeats(int nr);
	virtual std::vector<char> special_symbols();
	virtual void pre_perform(MainWidget* widget);
	virtual bool pushes_state();
	virtual bool requires_document();

	virtual void run(MainWidget* widget);
	virtual std::string get_name();
};


class CommandManager {
private:
	//std::vector<Command> commands;
	std::map < std::string, std::function<std::unique_ptr<Command>()> > new_commands;
public:

	CommandManager(ConfigManager* config_manager);
	std::unique_ptr<Command> get_command_with_name(std::string name);
	std::unique_ptr<Command> create_macro_command(std::string name, std::wstring macro_string);
	QStringList get_all_command_names();
};

struct InputParseTreeNode {

	std::vector<InputParseTreeNode*> children;
	//char command;
	int command;
	std::vector<std::string> name;
	bool shift_modifier = false;
	bool control_modifier = false;
	bool alt_modifier = false;
	bool requires_text = false;
	bool requires_symbol = false;
	bool is_root = false;
	bool is_final = false;

	// todo: use a pointer to reduce allocation
	std::wstring defining_file_path;
	int defining_file_line;

	bool is_same(const InputParseTreeNode* other);
	bool matches(int key, bool shift, bool ctrl, bool alt);
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
	std::vector<std::unique_ptr<Command>> handle_key(QKeyEvent* key_event, bool shift_pressed, bool control_pressed, bool alt_pressed ,int* num_repeats);
	void delete_current_parse_tree(InputParseTreeNode* node_to_delete);

	std::optional<Path> get_or_create_user_keys_path();
	std::vector<Path> get_all_user_keys_paths();
	std::unordered_map<std::string, std::vector<std::string>> get_command_key_mappings() const;

};

