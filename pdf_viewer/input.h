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

class NewCommand {
protected:
	int num_repeats = 1;
public:
	virtual std::optional<Requirement> next_requirement();

	virtual void set_text_requirement(std::wstring value);
	virtual void set_symbol_requirement(char value);
	virtual void set_file_requirement(std::wstring value);
	virtual void set_rect_requirement(fz_rect value);
	virtual void set_num_repeats(int nr);
	virtual std::vector<char> special_symbols();
	virtual void pre_perform(MainWidget* widget);
	virtual bool pushes_state();

	virtual void perform(MainWidget* widget) = 0;
	virtual std::string get_name();
};

class GotoBeginningCommand : public NewCommand {
public:
	void perform(MainWidget* main_widget);
	bool pushes_state();
	std::string get_name();
};

class GotoEndCommand : public NewCommand {
public:
	void perform(MainWidget* main_widget);
	bool pushes_state();
	std::string get_name();
};

class SymbolCommand : public NewCommand {
protected:
	char symbol = 0;
public:
	virtual std::optional<Requirement> next_requirement();
	virtual void set_symbol_requirement(char value);
};

class TextCommand : public NewCommand {
protected:
	std::optional<std::wstring> text = {};
public:
	
	virtual std::string text_requirement_name();
	virtual std::optional<Requirement> next_requirement();
	virtual void set_text_requirement(std::wstring value);
};

class GotoMark : public SymbolCommand {
	void perform(MainWidget* widget);
	std::vector<char> special_symbols();
	bool pushes_state();
	std::string get_name();
};

class SetMark : public SymbolCommand {
	void perform(MainWidget* widget);
	std::string get_name();
};

class NextItemCommand : public NewCommand{
	void perform(MainWidget* widget);
	std::string get_name();
};

class PrevItemCommand : public NewCommand{
	void perform(MainWidget* widget);
	std::string get_name();
};

class SearchCommand : public TextCommand {
	void perform(MainWidget* widget);
	std::string get_name();
	bool pushes_state();
	std::string text_requirement_name();
};

class ChapterSearchCommand : public TextCommand {
	void perform(MainWidget* widget);
	void pre_perform(MainWidget* widget);
	bool pushes_state();
	std::string get_name();
	std::string text_requirement_name();
};

class RegexSearchCommand : public TextCommand {
	void perform(MainWidget* widget);
	std::string get_name();
	bool pushes_state();
	std::string text_requirement_name();
};

class AddBookmarkCommand : public TextCommand {
	void perform(MainWidget* widget);
	std::string get_name();
	std::string text_requirement_name();
};

class GotoBookmarkCommand : public NewCommand {
	void perform(MainWidget* widget);
	bool pushes_state();
	std::string get_name();
};

class GotoBookmarkGlobalCommand : public NewCommand {
	void perform(MainWidget* widget);
	bool pushes_state();
	std::string get_name();
};

class GotoHighlightCommand : public NewCommand {
	void perform(MainWidget* widget);
	bool pushes_state();
	std::string get_name();
};

class GotoHighlightGlobalCommand : public NewCommand {
	void perform(MainWidget* widget);
	bool pushes_state();
	std::string get_name();
};

class GotoTableOfContentsCommand : public NewCommand {
	void perform(MainWidget* widget);
	std::string get_name();
};

class PortalCommand : public NewCommand {
	void perform(MainWidget* widget);
	std::string get_name();
};

class ToggleWindowConfigurationCommand : public NewCommand {
	void perform(MainWidget* widget);
	std::string get_name();
};

class NextStateCommand : public NewCommand {
	void perform(MainWidget* widget);
	std::string get_name();
};

class PrevStateCommand : public NewCommand {
	void perform(MainWidget* widget);
	std::string get_name();
};


class AddHighlightCommand : public SymbolCommand {
	void perform(MainWidget* widget);
	std::string get_name();
};

class CommandCommand : public NewCommand {
	void perform(MainWidget* widget);
	std::string get_name();
};

class OpenDocumentCommand : public NewCommand {

	std::wstring file_name;

	std::optional<Requirement> next_requirement();
	virtual void set_file_requirement(std::wstring value);
	bool pushes_state();
	void perform(MainWidget* widget);
	std::string get_name();
};

class MoveDownCommand : public NewCommand {
	void perform(MainWidget* widget);
	std::string get_name();
};

class MoveUpCommand : public NewCommand {
	void perform(MainWidget* widget);
	std::string get_name();
};

class MoveLeftCommand : public NewCommand {
	void perform(MainWidget* widget);
	std::string get_name();
};

class MoveRightCommand : public NewCommand {
	void perform(MainWidget* widget);
	std::string get_name();
};

class ZoomInCommand : public NewCommand {
	void perform(MainWidget* widget);
	std::string get_name();
};

class FitToPageWidthCommand : public NewCommand {
	void perform(MainWidget* widget);
	std::string get_name();
};

class FitToPageWidthSmartCommand : public NewCommand {
	void perform(MainWidget* widget);
	std::string get_name();
};

class FitToPageHeightCommand : public NewCommand {
	void perform(MainWidget* widget);
	std::string get_name();
};

class FitToPageHeightSmartCommand : public NewCommand {
	void perform(MainWidget* widget);
	std::string get_name();
};

class NextPageCommand : public NewCommand {
	void perform(MainWidget* widget);
	std::string get_name();
};

class PreviousPageCommand : public NewCommand {
	void perform(MainWidget* widget);
	std::string get_name();
};


class ZoomOutCommand : public NewCommand {
	void perform(MainWidget* widget);
	std::string get_name();
};

class GotoDefinitionCommand : public NewCommand {
	void perform(MainWidget* widget);
	bool pushes_state();
	std::string get_name();
};

class OverviewDefinitionCommand : public NewCommand {
	void perform(MainWidget* widget);
	std::string get_name();
};

class PortalToDefinitionCommand : public NewCommand {
	void perform(MainWidget* widget);
	std::string get_name();
};

class MoveVisualMarkDownCommand : public NewCommand {
	void perform(MainWidget* widget);
	std::string get_name();
};

class MoveVisualMarkUpCommand : public NewCommand {
	void perform(MainWidget* widget);
	std::string get_name();
};

class GotoPageWithPageNumberCommand : public TextCommand {
	void perform(MainWidget* widget);
	bool pushes_state();
	std::string get_name();
	std::string text_requirement_name();
};

class DeletePortalCommand : public NewCommand {
	void perform(MainWidget* widget);
	std::string get_name();
};

class DeleteBookmarkCommand : public NewCommand {
	void perform(MainWidget* widget);
	std::string get_name();
};

class DeleteHighlightCommand : public NewCommand {
	void perform(MainWidget* widget);
	std::string get_name();
};

class GotoPortalCommand : public NewCommand {
	void perform(MainWidget* widget);
	bool pushes_state();
	std::string get_name();
};

class EditPortalCommand : public NewCommand {
	void perform(MainWidget* widget);
	bool pushes_state();
	std::string get_name();
};

class OpenPrevDocCommand : public NewCommand {
	void perform(MainWidget* widget);
	bool pushes_state();
	std::string get_name();
};

class OpenDocumentEmbeddedCommand : public NewCommand {
	void perform(MainWidget* widget);
	bool pushes_state();
	std::string get_name();
};

class OpenDocumentEmbeddedFromCurrentPathCommand : public NewCommand {
	void perform(MainWidget* widget);
	bool pushes_state();
	std::string get_name();
};

class CopyCommand : public NewCommand {
	void perform(MainWidget* widget);
	std::string get_name();
};


NewCommand* get_command_with_name(std::string name);

struct Command {
	std::string name;
	bool requires_text;
	bool requires_symbol;
	bool requires_file_name;
	bool pushes_state;
	bool requires_document;
	std::vector<char> special_symbols;
};

class CommandManager {
private:
	std::vector<Command> commands;
public:

	CommandManager(ConfigManager* config_manager);
	const Command* get_command_with_name(std::string name);
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
	std::vector<const Command*> handle_key(QKeyEvent* key_event, bool shift_pressed, bool control_pressed, bool alt_pressed ,int* num_repeats);
	void delete_current_parse_tree(InputParseTreeNode* node_to_delete);

	std::optional<Path> get_or_create_user_keys_path();
	std::vector<Path> get_all_user_keys_paths();
	std::unordered_map<std::string, std::vector<std::string>> get_command_key_mappings() const;

};

