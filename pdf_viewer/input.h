#pragma once

#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>

#include <SDL.h>

#include "utils.h"
using namespace std;

struct Command {
	string name;
	bool requires_text;
	bool requires_symbol;
	bool requires_file_name;
	bool pushes_state;
};


class CommandManager {
private:
	vector<Command> commands;
public:

	CommandManager();
	const Command* get_command_with_name(string name);
};

struct InputParseTreeNode {

	vector<InputParseTreeNode*> children;
	//char command;
	SDL_Keycode command;
	string name = "";
	bool shift_modifier = false;
	bool control_modifier = false;
	bool requires_text = false;
	bool requires_symbol = false;
	bool is_root = false;
	bool is_final = false;
};



class InputHandler {
private:
	InputParseTreeNode* root;
	InputParseTreeNode* current_node;
	CommandManager command_manager;
	string number_stack;

public:
	InputHandler(wstring file_path);
	const Command* handle_key(SDL_Keycode key, bool shift_pressed, bool control_pressed, int* num_repeats);
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
