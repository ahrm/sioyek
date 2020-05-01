#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>

#include <qkeyevent.h>
//#include <SDL.h>
#include "input.h"

using namespace std;


CommandManager::CommandManager() {
	commands.push_back({ "goto_begining", false, false , false, true});
	commands.push_back({ "goto_end", false, false , false, true});
	commands.push_back({ "goto_definition", false, false , false, true});
	commands.push_back({ "next_item", false, false , false, true});
	commands.push_back({ "previous_item", false, false , false, true});
	commands.push_back({ "set_mark", false, true , false, false});
	commands.push_back({ "goto_mark", false, true , false, false});
	commands.push_back({ "search", true, false , false, false});
	commands.push_back({ "move_down", false, false , false, false});
	commands.push_back({ "move_up", false, false , false, false});
	commands.push_back({ "move_left", false, false , false, false});
	commands.push_back({ "move_right", false, false , false, false});
	commands.push_back({ "zoom_in", false, false , false, false});
	commands.push_back({ "zoom_out", false, false , false, false});
	commands.push_back({ "next_page", false, false , false, false});
	commands.push_back({ "previous_page", false, false , false, false});
	commands.push_back({ "open_document", false, false , true, true});
	commands.push_back({ "debug", false, false , false, false});
	commands.push_back({ "add_bookmark", true, false , false, false});
	commands.push_back({ "goto_toc", false, false , false, false});
	commands.push_back({ "goto_bookmark", false, false , false, false});
	commands.push_back({ "goto_bookmark_g", false, false , false, false});
	commands.push_back({ "link", false, false , false, false});
	commands.push_back({ "next_state", false, false , false, false});
	commands.push_back({ "prev_state", false, false , false, false});
	commands.push_back({ "pop_state", false, false , false, false});
	commands.push_back({ "test_command", false, false , false, false});
	commands.push_back({ "delete", false, true , false, false});
	commands.push_back({ "goto_link", false, false , false, false});
	commands.push_back({ "edit_link", false, false , false, false});
	commands.push_back({ "open_prev_doc", false, false , false, false});
	commands.push_back({ "copy", false, false , false, false});
	commands.push_back({ "toggle_fullscreen", false, false , false, false});
	commands.push_back({ "toggle_one_window", false, false , false, false});
	commands.push_back({ "toggle_highlight", false, false , false, false});
	commands.push_back({ "command", true, false , false, false});
	commands.push_back({ "search_selected_text_in_google_scholar", false, false , false, false});
	commands.push_back({ "search_selected_text_in_libgen", false, false , false, false});
	commands.push_back({ "screen_down", false, false , false, false});
	commands.push_back({ "screen_up", false, false , false, false});
	commands.push_back({ "next_chapter", false, false , false, true});
	commands.push_back({ "prev_chapter", false, false , false, true});
}
const Command* CommandManager::get_command_with_name(string name) {
	for (const auto &com : commands) {
		if (com.name == name) {
			return &com;
		}
	}
	return nullptr;
}

void print_tree_node(InputParseTreeNode node) {
	if (node.requires_text) {
		cout << "text node" << endl;
		return;
	}
	if (node.requires_symbol) {
		cout << "symbol node" << endl;
		return;
	}

	if (node.control_modifier) {
		cout << "Ctrl+";
	}

	if (node.shift_modifier) {
		cout << "Shift+";
	}
	cout << node.command << endl;
}

InputParseTreeNode parse_token(string token) {
	InputParseTreeNode res;

	if (token == "sym") {
		res.requires_symbol = true;
		return res;
	}
	if (token == "txt") {
		res.requires_text = true;
		return res;
	}

	vector<string> subcommands;

	int current_begin = 0;

	while (token.find("-", current_begin) != -1) {
		int current_end = token.find("-", current_begin);
		subcommands.push_back(token.substr(current_begin, current_end-current_begin));
		current_begin = current_end+1;
	}
	subcommands.push_back(token.substr(current_begin, token.size()));

	for (int i = 0; i < subcommands.size() - 1; i++) {
		if (subcommands[i] == "C") {
			res.control_modifier = true;
		}

		if (subcommands[i] == "S") {
			res.shift_modifier = true;
		}
	}

	string command_string = subcommands[subcommands.size() - 1];
	if (command_string.size() == 1) {
		res.command = subcommands[subcommands.size() - 1][0];
	}
	else {
		if (int f_key = get_f_key(command_string)) {
			res.command = Qt::Key::Key_F1 - 1 + f_key;
		}
		if (command_string == "up") {
			res.command = Qt::Key::Key_Up;
		}

		if (command_string == "backspace") {
			res.command = Qt::Key::Key_Backspace;
		}
		if (command_string == "down") {
			res.command = Qt::Key::Key_Down;
		}

		if (command_string == "tab") {
			res.command = Qt::Key::Key_Tab;
		}

		if (command_string == "left") {
			res.command = Qt::Key::Key_Left;
		}

		if (command_string == "right") {
			res.command = Qt::Key::Key_Right;
		}

		if (command_string == "<up>") {
			res.command = Qt::Key::Key_Up;
		}

		if (command_string == "<down>") {
			res.command = Qt::Key::Key_Down;
		}

		if (command_string == "<left>") {
			res.command = Qt::Key::Key_Left;
		}

		if (command_string == "<right>") {
			res.command = Qt::Key::Key_Right;
		}

		if (command_string == "<tab>") {
			res.command = Qt::Key::Key_Tab;
		}
		if (command_string == "<backspace>") {
			res.command = Qt::Key::Key_Backspace;
		}
	}


	return res;
}

InputParseTreeNode* parse_lines(vector<string> lines, vector<string> command_names) {
	InputParseTreeNode* root = new InputParseTreeNode;
	root->is_root = true;

	for (int j = 0; j < lines.size(); j++) {
		string line = lines[j];

		vector<string> tokens;
		string stack;
		//bool is_stack_mode = false;

		//for (char c : line) {
		//	if (is_stack_mode && (c != '>')) {
		//		stack.push_back(c);
		//	}
		//	else if (is_stack_mode && (c == '>')) {
		//		tokens.push_back(stack);
		//		stack.clear();
		//		is_stack_mode = false;
		//	}
		//	else if (c == '<') {
		//		is_stack_mode = true;
		//	}
		//	else {
		//		tokens.push_back(string(1, c));
		//	}

		//}

		int stack_depth = 0;

		for (char c : line) {
			if (stack_depth && (c != '>') && (c!='<')) {
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
				tokens.push_back(string(1, c));
			}

		}

		InputParseTreeNode* parent_node = root;

		for (int i = 0; i < tokens.size(); i++) {
			InputParseTreeNode node = parse_token(tokens[i]);
			bool existing_node = false;
			for (InputParseTreeNode* child : parent_node->children) {
				if (
					child->command == node.command &&
					child->control_modifier == node.control_modifier &&
					child->requires_symbol == node.requires_symbol &&
					child->requires_text == node.requires_text &&
					child->shift_modifier == node.shift_modifier
					) {
					parent_node = child;
					existing_node = true;
					break;
				}
			}
			if (!existing_node) {
				if ((tokens[i] != "sym") && (tokens[i] != "txt")) {

					if (parent_node->is_final) {
						cout << "adding child command to a final command" << endl;
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

InputParseTreeNode* parse_key_config_file(wstring file_path) {
	ifstream infile(file_path);

	vector<string> command_names;
	vector<string> command_keys;
	while (infile.good()) {
		string command_name;
		string command_key;
		infile >> command_name >> command_key;
		command_names.push_back(command_name);
		command_keys.push_back(command_key);
	}
	return parse_lines(command_keys, command_names);
}


InputHandler::InputHandler(wstring file_path) {
	root = parse_key_config_file(file_path);
	current_node = root;
}

bool is_digit(int key) {
	return key >= Qt::Key::Key_0 && key <= Qt::Key::Key_9;
}

const Command* InputHandler::handle_key(int key, bool shift_pressed, bool control_pressed, int* num_repeats) {
	if (key >= 'A' && key <= 'Z') {
		key = key - 'A' + 'a';
	}

	if (current_node == root && is_digit(key)) {
		number_stack.push_back('0' + key - Qt::Key::Key_0);
		return nullptr;
	}

	for (InputParseTreeNode* child : current_node->children) {
		if (child->command == key && child->shift_modifier == shift_pressed && child->control_modifier == control_pressed){
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
	cout << "invalid command; resetting to root" << endl;
	number_stack.clear();
	current_node = root;
	return nullptr;
}


//int main(int argc, char** argv) {
//	//InputParseTreeNode* root = parse_key_config_file("keys.config");
//
//	////InputParseTreeNode* root =  parse_lines(commands, command_names);
//	//InputParseTreeNode* current_node = root;
//	InputHandler handler("keys.config");
//
//	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
//		cout << "could not initialize SDL" << endl;
//		return -1;
//	}
//
//	SDL_Window* window = SDL_CreateWindow("Input Test", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 500, 500,  SDL_WINDOW_SHOWN );
//
//	bool should_quit = false;
//	while (!should_quit) {
//
//		SDL_Event event;
//		while (SDL_PollEvent(&event)) {
//			if (event.type == SDL_QUIT) {
//				should_quit = true;
//			}
//
//			if (event.type == SDL_KEYDOWN) {
//				vector<SDL_Scancode> igonred_keycodes = {
//					SDL_SCANCODE_LCTRL,
//					SDL_SCANCODE_RCTRL,
//					SDL_SCANCODE_RSHIFT,
//					SDL_SCANCODE_LSHIFT
//				};
//
//				if (find(igonred_keycodes.begin(), igonred_keycodes.end(), event.key.keysym.scancode) != igonred_keycodes.end()) {
//					break;
//				}
//
//				const Command* command = handler.handle_key(SDL_GetKeyFromScancode(event.key.keysym.scancode),
//					(event.key.keysym.mod & KMOD_SHIFT) != 0,
//					(event.key.keysym.mod & KMOD_CTRL) != 0
//					);
//				if (command) {
//					cout << command->name << " " << command->requires_text <<  endl;
//				}
//
//			}
//		}
//	}
//	return 0;
//
//
//	//}
	//InputParseTreeNode node = parse_token("C-S-g");
	//print_tree_node(node);
//}
//InputParseTreeNode* parse_config_file(string config_file_path) {
//
//}

