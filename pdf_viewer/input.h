#pragma once

#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <memory>

//#include <SDL.h>

#include "utils.h"

struct Command {
	std::string name;
	bool requires_text;
	bool requires_symbol;
	bool requires_file_name;
	bool pushes_state;
};


class CommandManager {
private:
	std::vector<Command> commands;
public:

	CommandManager();
	const Command* get_command_with_name(std::string name);
};

struct InputParseTreeNode {

	std::vector<InputParseTreeNode*> children;
	//char command;
	int command;
	std::string name = "";
	bool shift_modifier = false;
	bool control_modifier = false;
	bool requires_text = false;
	bool requires_symbol = false;
	bool is_root = false;
	bool is_final = false;

	bool is_same(const InputParseTreeNode* other);
	bool matches(int key, bool shift, bool ctrl);
};



class InputHandler {
private:
	InputParseTreeNode* root = nullptr;
	InputParseTreeNode* current_node;
	CommandManager command_manager;
	std::string number_stack;

public:
	InputHandler(const std::wstring& file_path);
	void reload_config_file(const std::wstring& file_path);
	const Command* handle_key(int key, bool shift_pressed, bool control_pressed, int* num_repeats);
	void delete_current_parse_tree(InputParseTreeNode* node_to_delete);

};

//int main(int argc, char** argv) {
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
//	//InputParseTreeNode node = parse_token("C-S-g");
//	//print_tree_node(node);
//}
////InputParseTreeNode* parse_config_file(string config_file_path) {
////
////}
//
