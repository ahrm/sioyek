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
	commands.push_back({ "copy", false, false , false, false});
	commands.push_back({ "toggle_fullscreen", false, false , false, false});
	commands.push_back({ "toggle_one_window", false, false , false, false});
	commands.push_back({ "toggle_highlight", false, false , false, false});
	commands.push_back({ "toggle_synctex", false, false , false, false});
	commands.push_back({ "command", true, false , false, false});
	commands.push_back({ "search_selected_text_in_google_scholar", false, false , false, false});
	commands.push_back({ "open_selected_url", false, false , false, false});
	commands.push_back({ "search_selected_text_in_libgen", false, false , false, false});
	commands.push_back({ "screen_down", false, false , false, false});
	commands.push_back({ "screen_up", false, false , false, false});
	commands.push_back({ "next_chapter", false, false , false, true});
	commands.push_back({ "prev_chapter", false, false , false, true});
	commands.push_back({ "toggle_dark_mode", false, false , false, false});
	commands.push_back({ "toggle_presentation_mode", false, false , false, false});
	commands.push_back({ "toggle_mouse_drag_mode", false, false , false, false});
	commands.push_back({ "quit", false, false , false, false});
	commands.push_back({ "open_link", true, false , false, false});
}

const Command* CommandManager::get_command_with_name(std::string name) {
	for (const auto &com : commands) {
		if (com.name == name) {
			return &com;
		}
	}
	return nullptr;
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

	std::ifstream default_infile(default_path.get_path_utf8());


	std::vector<std::string> command_names;
	std::vector<std::string> command_keys;

	while (default_infile.good()) {
		std::string line;
		std::getline(default_infile, line);
		if (line.size() == 0 || line[0] == '#') {
			continue;
		}
		std::stringstream ss(line);
		std::string command_name;
		std::string command_key;
		ss >> command_name >> command_key;
		command_names.push_back(command_name);
		command_keys.push_back(command_key);
	}

	default_infile.close();


	for (int i = 0; i < user_paths.size(); i++) {

		if (user_paths[i].file_exists()) {
			std::ifstream user_infile(user_paths[i].get_path_utf8());
			while (user_infile.good()) {
				std::string line;
				std::getline(user_infile, line);
				if (line.size() == 0 || line[0] == '#') {
					continue;
				}
				std::stringstream ss(line);
				std::string command_name;
				std::string command_key;
				ss >> command_name >> command_key;
				command_names.push_back(command_name);
				command_keys.push_back(command_key);
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
		number_stack.push_back('0' + key - Qt::Key::Key_0);
		return nullptr;
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
