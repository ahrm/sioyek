#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>

#include <qkeyevent.h>
//#include <SDL.h>
#include "input.h"



CommandManager::CommandManager() {
	commands.push_back({ "goto_begining",		false,	false,	false,	true});
	commands.push_back({ "goto_end",			false,	false,	false,	true});
	commands.push_back({ "goto_definition",		false,	false,	false,	true});
	commands.push_back({ "next_item",			false,	false,	false,	true});
	commands.push_back({ "previous_item", false, false , false, true});
	commands.push_back({ "set_mark", false, true , false, false});
	commands.push_back({ "goto_mark", false, true , false, false});
	commands.push_back({ "goto_page_with_page_number", true, false , false, false});
	commands.push_back({ "search", true, false , false, false});
	commands.push_back({ "ranged_search", true, false , false, false});
	commands.push_back({ "chapter_search", true, false , false, false});
	commands.push_back({ "move_down", false, false , false, false});
	commands.push_back({ "move_up", false, false , false, false});
	commands.push_back({ "move_left", false, false , false, false});
	commands.push_back({ "move_right", false, false , false, false});
	commands.push_back({ "zoom_in", false, false , false, false});
	commands.push_back({ "zoom_out", false, false , false, false});
	commands.push_back({ "fit_to_page_width", false, false , false, false});
	commands.push_back({ "fit_to_page_width_smart", false, false , false, false});
	commands.push_back({ "next_page", false, false , false, false});
	commands.push_back({ "previous_page", false, false , false, false});
	commands.push_back({ "open_document", false, false , true, true});
	commands.push_back({ "debug", false, false , false, false});
	commands.push_back({ "add_bookmark", true, false , false, false});
	commands.push_back({ "add_highlight", false, true , false, false});
	commands.push_back({ "goto_toc", false, false , false, false});
	commands.push_back({ "goto_highlight", false, false , false, false});
	commands.push_back({ "goto_bookmark", false, false , false, false});
	commands.push_back({ "goto_bookmark_g", false, false , false, false});
	commands.push_back({ "goto_highlight_g", false, false , false, false});
	commands.push_back({ "goto_highlight_ranged", false, false , false, false});
	commands.push_back({ "link", false, false , false, false});
	commands.push_back({ "next_state", false, false , false, false});
	commands.push_back({ "prev_state", false, false , false, false});
	commands.push_back({ "pop_state", false, false , false, false});
	commands.push_back({ "test_command", false, false , false, false});
	commands.push_back({ "delete_link", false, false , false, false});
	commands.push_back({ "delete_bookmark", false, false , false, false});
	commands.push_back({ "delete_highlight", false, false , false, false});
	//commands.push_back({ "delete", false, true , false, false});
	commands.push_back({ "goto_link", false, false , false, false});
	commands.push_back({ "edit_link", false, false , false, false});
	commands.push_back({ "open_prev_doc", false, false , false, false});
	commands.push_back({ "open_document_embedded", false, false , false, false});
	commands.push_back({ "open_document_embedded_from_current_path", false, false , false, false});
	commands.push_back({ "copy", false, false , false, false});
	commands.push_back({ "toggle_fullscreen", false, false , false, false});
	commands.push_back({ "toggle_one_window", false, false , false, false});
	commands.push_back({ "toggle_highlight", false, false , false, false});
	commands.push_back({ "toggle_synctex", false, false , false, false});
	//commands.push_back({ "command", true, false , false, false});
	commands.push_back({ "command", false, false , false, false});
	//commands.push_back({ "search_selected_text_in_google_scholar", false, false , false, false});
	//commands.push_back({ "search_selected_text_in_libgen", false, false , false, false});
	commands.push_back({ "external_search", false, true , false, false});
	commands.push_back({ "open_selected_url", false, false , false, false});
	commands.push_back({ "screen_down", false, false , false, false});
	commands.push_back({ "screen_up", false, false , false, false});
	commands.push_back({ "next_chapter", false, false , false, true});
	commands.push_back({ "prev_chapter", false, false , false, true});
	commands.push_back({ "toggle_dark_mode", false, false , false, false});
	commands.push_back({ "toggle_presentation_mode", false, false , false, false});
	commands.push_back({ "toggle_mouse_drag_mode", false, false , false, false});
	commands.push_back({ "quit", false, false , false, false});
	commands.push_back({ "q", false, false , false, false});
	commands.push_back({ "open_link", true, false , false, false});
	commands.push_back({ "keyboard_select", true, false , false, false});
	commands.push_back({ "keyboard_smart_jump", true, false , false, false});
	commands.push_back({ "keyboard_overview", true, false , false, false});
	commands.push_back({ "keys", false, false , false, false});
	commands.push_back({ "keys_user", false, false , false, false});
	commands.push_back({ "prefs", false, false , false, false});
	commands.push_back({ "prefs_user", false, false , false, false});
	commands.push_back({ "import", false, false , false, false});
	commands.push_back({ "export", false, false , false, false});
	commands.push_back({ "move_visual_mark_down", false, false , false, false});
	commands.push_back({ "move_visual_mark_up", false, false , false, false});
	commands.push_back({ "set_page_offset", true, false , false, false});
	commands.push_back({ "toggle_visual_scroll", false, false , false, false});
	commands.push_back({ "toggle_horizontal_scroll_lock", false, false , false, false});
	commands.push_back({ "toggle_custom_color", false, false , false, false});
	commands.push_back({ "execute", true, false , false, false});
	commands.push_back({ "execute_predefined_command", false, true, false, false});
	commands.push_back({ "embed_annotations", false, false, false, false});
	commands.push_back({ "copy_window_size_config", false, false, false, false});
	commands.push_back({ "toggle_select_highlight", false, false, false, false});
	commands.push_back({ "set_select_highlight_type", false, true, false, false});
	commands.push_back({ "open_last_document", false, false, false, false});
	commands.push_back({ "toggle_window_configuration", false, false, false, false});
	commands.push_back({ "prefs_user_all", false, false, false, false});
	commands.push_back({ "keys_user_all", false, false, false, false});
	commands.push_back({ "fit_to_page_width_ratio", false, false, false, false});
    commands.push_back({ "smart_jump_under_cursor", false, false, false, false});
    commands.push_back({ "overview_under_cursor", false, false, false, false});
    commands.push_back({ "close_overview", false, false, false, false});
    commands.push_back({ "visual_mark_under_cursor", false, false, false, false});
    commands.push_back({ "close_visual_mark", false, false, false, false});
    commands.push_back({ "zoom_in_cursor", false, false, false, false});
    commands.push_back({ "zoom_out_cursor", false, false, false, false});
    commands.push_back({ "goto_left", false, false, false, false});
    commands.push_back({ "goto_left_smart", false, false, false, false});
    commands.push_back({ "goto_right", false, false, false, false});
    commands.push_back({ "goto_right_smart", false, false, false, false});
    commands.push_back({ "rotate_clockwise", false, false, false, false});
    commands.push_back({ "rotate_counterclockwise", false, false, false, false});
    commands.push_back({ "goto_next_highlight", false, false, false, false});
    commands.push_back({ "goto_prev_highlight", false, false, false, false});
    commands.push_back({ "goto_next_highlight_of_type", false, false, false, false});
    commands.push_back({ "goto_prev_highlight_of_type", false, false, false, false});
    commands.push_back({ "add_highlight_with_current_type", false, false, false, false});
}

const Command* CommandManager::get_command_with_name(std::string name) {
	for (const auto &com : commands) {
		if (com.name == name) {
			return &com;
		}
	}
	return nullptr;
}

QStringList CommandManager::get_all_command_names() {
	QStringList res;
	for (const auto &com : commands) {
		res.push_back(QString::fromStdString(com.name));
	}
	return res;
}

void print_tree_node(InputParseTreeNode node) {
	if (node.requires_text) {
		std::wcout << "text node" << std::endl;
		return;
	}
	if (node.requires_symbol) {
		std::wcout << "symbol node" << std::endl;
		return;
	}

	if (node.control_modifier) {
		std::wcout << "Ctrl+";
	}

	if (node.shift_modifier) {
		std::wcout << "Shift+";
	}

	if (node.alt_modifier) {
		std::wcout << "Alt+";
	}
	std::wcout << node.command << std::endl;
}

InputParseTreeNode parse_token(std::string token) {
	InputParseTreeNode res;

	if (token == "sym") {
		res.requires_symbol = true;
		return res;
	}
	if (token == "txt") {
		res.requires_text = true;
		return res;
	}

	std::vector<std::string> subcommands;
	split_key_string(token, "-", subcommands);

	for (int i = 0; i < subcommands.size() - 1; i++) {
		if (subcommands[i] == "C") {
			res.control_modifier = true;
		}

		if (subcommands[i] == "S") {
			res.shift_modifier = true;
		}

		if (subcommands[i] == "A") {
			res.alt_modifier = true;
		}
	}

	std::string command_string = subcommands[subcommands.size() - 1];
	if (command_string.size() == 1) {
		res.command = subcommands[subcommands.size() - 1][0];
	}
	else {

		if (int f_key = get_f_key(command_string)) {
			res.command = Qt::Key::Key_F1 - 1 + f_key;
		}
		else {

			std::map<std::string, Qt::Key> keymap_temp = {
				{"up", Qt::Key::Key_Up},
				{"down", Qt::Key::Key_Down},
				{"left", Qt::Key::Key_Left},
				{"right", Qt::Key::Key_Right},
				{"backspace", Qt::Key::Key_Backspace},
				{"space", Qt::Key::Key_Space},
				{"pageup", Qt::Key::Key_PageUp},
				{"pagedown", Qt::Key::Key_PageDown},
				{"home", Qt::Key::Key_Home},
				{"end", Qt::Key::Key_End},
				{"pagedown", Qt::Key::Key_End},
				{"tab", Qt::Key::Key_Tab},
			};
			std::map<std::string, Qt::Key> keymap;

			for (auto item : keymap_temp) {
				keymap[item.first] = item.second;
				keymap["<" + item.first + ">"] = item.second;
			}

			res.command = keymap[command_string];
		}

	}

	return res;
}
void get_tokens(std::string line, std::vector<std::string>& tokens) {
	std::string stack;

	int stack_depth = 0;

	for (char c : line) {
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
			tokens.push_back(std::string(1, c));
		}

	}
}

InputParseTreeNode* parse_lines(InputParseTreeNode* root,
	std::vector<std::string> lines,
	std::vector<std::string> command_names) {

	for (int j = 0; j < lines.size(); j++) {
		std::string line = lines[j];

		// for example convert "<a-<space>> to ["a", "space"]
		std::vector<std::string> tokens;
		get_tokens(line, tokens);

		InputParseTreeNode* parent_node = root;

		for (int i = 0; i < tokens.size(); i++) {
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
				if ((tokens[i] != "sym") && (tokens[i] != "txt")) {

					if (parent_node->is_final) {
						std::wcout << "adding child command to a final command" << std::endl;
					}

					parent_node->children.push_back(new InputParseTreeNode(node));
					parent_node = parent_node->children[parent_node->children.size() - 1];
				}
				else {
					if (tokens[i] == "sym") {
						parent_node->requires_symbol = true;
						parent_node->is_final = true;
					}

					if (tokens[i] == "txt") {
						parent_node->requires_text = true;
						parent_node->is_final = true;
					}
				}
			}

			if (i == (tokens.size() - 1)) {
				parent_node->is_final = true;
				parent_node->name = command_names[j];
			}

		}
	}

	return root;
}

InputParseTreeNode* parse_lines(std::vector<std::string> lines, std::vector<std::string> command_names) {
	// parse key configs into a trie where leaves are annotated with the name of the command

	InputParseTreeNode* root = new InputParseTreeNode;
	root->is_root = true;

	parse_lines(root, lines, command_names);

	return root;

}

InputParseTreeNode* parse_key_config_files(const Path& default_path,
	const std::vector<Path>& user_paths) {

	//std::wstring default_path_wstring = default_path.get_path();
	std::wifstream default_infile = open_wifstream(default_path.get_path());


	std::vector<std::string> command_names;
	std::vector<std::string> command_keys;

	while (default_infile.good()) {
		std::wstring line;
		std::getline(default_infile, line);
		if (line.size() == 0 || line[0] == '#') {
			continue;
		}
		std::wstringstream ss(line);
		std::wstring command_name;
		std::wstring command_key;
		ss >> command_name >> command_key;
		command_names.push_back(utf8_encode(command_name));
		command_keys.push_back(utf8_encode(command_key));
	}

	default_infile.close();


	for (int i = 0; i < user_paths.size(); i++) {

		if (user_paths[i].file_exists()) {
			std::wifstream user_infile = open_wifstream(user_paths[i].get_path());
			while (user_infile.good()) {
				std::wstring line;
				std::getline(user_infile, line);
				if (line.size() == 0 || line[0] == '#') {
					continue;
				}
				std::wstringstream ss(line);
				std::wstring command_name;
				std::wstring command_key;
				ss >> command_name >> command_key;
				command_names.push_back(utf8_encode(command_name));
				command_keys.push_back(utf8_encode(command_key));
			}
			user_infile.close();
		}
	}

	return parse_lines(command_keys, command_names);
}


InputHandler::InputHandler(const Path& default_path, const std::vector<Path>& user_paths) {
	user_key_paths = user_paths;
	reload_config_files(default_path, user_paths);
}

void InputHandler::reload_config_files(const Path& default_config_path, const std::vector<Path>& user_config_paths)
{
	delete_current_parse_tree(root);
	root = parse_key_config_files(default_config_path, user_config_paths);
	current_node = root;
}


bool is_digit(int key) {
	return key >= Qt::Key::Key_0 && key <= Qt::Key::Key_9;
}

const Command* InputHandler::handle_key(int key, bool shift_pressed, bool control_pressed, bool alt_pressed, int* num_repeats) {
	if (key >= 'A' && key <= 'Z') {
		key = key - 'A' + 'a';
	}

	if (current_node == root && is_digit(key)) {
		if (!(key == '0' && (number_stack.size() == 0))) {
			number_stack.push_back('0' + key - Qt::Key::Key_0);
			return nullptr;
		}
	}

	for (InputParseTreeNode* child : current_node->children) {
		//if (child->command == key && child->shift_modifier == shift_pressed && child->control_modifier == control_pressed){
		if (child->matches(key, shift_pressed, control_pressed, alt_pressed)){
			if (child->is_final == true) {
				current_node = root;
				//cout << child->name << endl;
				*num_repeats = 0;
				if (number_stack.size() > 0) {
					*num_repeats = atoi(number_stack.c_str());
					number_stack.clear();
				}

				return command_manager.get_command_with_name(child->name);
			}
			else{
				current_node = child;
				return nullptr;
			}
		}
	}
	std::wcout << "invalid command (key:" << (char)key << "); resetting to root" << std::endl;
	number_stack.clear();
	current_node = root;
	return nullptr;
}

void InputHandler::delete_current_parse_tree(InputParseTreeNode* node_to_delete)
{
	bool is_root = false;

	if (node_to_delete != nullptr) {
		is_root = node_to_delete->is_root;

		for (int i = 0; i < node_to_delete->children.size(); i++) {
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

std::unordered_map<std::string, std::vector<std::string>> InputHandler::get_command_key_mappings() const{
	std::unordered_map<std::string, std::vector<std::string>> res;
	std::vector<InputParseTreeNode*> prefix;
	add_command_key_mappings(root, res, prefix);
	return res;
}

void InputHandler::add_command_key_mappings(InputParseTreeNode* thisroot,
	std::unordered_map<std::string, std::vector<std::string>>& map,
	std::vector<InputParseTreeNode*> prefix) const {

	if (thisroot->is_final) {
		map[thisroot->name].push_back(get_key_string_from_tree_node_sequence(prefix));
	}
	else{
		for (int i = 0; i < thisroot->children.size(); i++) {
			prefix.push_back(thisroot->children[i]);
			add_command_key_mappings(thisroot->children[i], map, prefix);
			prefix.pop_back();
		}

	}
}

std::string InputHandler::get_key_name_from_key_code(int key_code) const{
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
		{Qt::Key::Key_End, "pagedown"},
		{Qt::Key::Key_Tab, "tab"},
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

std::string InputHandler::get_key_string_from_tree_node_sequence(const std::vector<InputParseTreeNode*> seq) const{
	std::string res;
	for (int i = 0; i < seq.size(); i++) {
		if (seq[i]->alt_modifier || seq[i]->shift_modifier || seq[i]->control_modifier ) {
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
		if (seq[i]->alt_modifier || seq[i]->shift_modifier || seq[i]->control_modifier ) {
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

