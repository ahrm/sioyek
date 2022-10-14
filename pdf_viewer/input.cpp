#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>

#include <qkeyevent.h>
#include <qstring.h>
#include <qstringlist.h>
#include "input.h"

extern bool SHOULD_WARN_ABOUT_USER_KEY_OVERRIDE;
extern bool USE_LEGACY_KEYBINDS;
extern std::map<std::wstring, std::wstring> ADDITIONAL_COMMANDS;
extern std::map<std::wstring, std::wstring> ADDITIONAL_MACROS;

CommandManager::CommandManager(ConfigManager* config_manager) {
	commands.push_back({ "goto_beginning",				false,	false,	false,	true,	true,	{}});
	commands.push_back({ "goto_end",					false,	false,	false,	true,	true,	{}});
	commands.push_back({ "goto_definition",				false,	false,	false,	true,	true,	{}});
	commands.push_back({ "overview_definition",			false,	false,	false,	false,	true,	{}});
	commands.push_back({ "portal_to_definition",		false,	false,	false,	false,	true,	{}});
	commands.push_back({ "next_item",					false,	false,	false,	false,	true,	{}});
	commands.push_back({ "previous_item",				false,	false,	false,	false,	true,	{}});
	commands.push_back({ "set_mark",					false,	true,	false,	false,	true,	{}});
	commands.push_back({ "goto_mark",					false,	true,	false,	true,	true,	{'`', '\'', '/'}});
	commands.push_back({ "goto_page_with_page_number",	true,	false,	false,	true,	true,	{}});
	commands.push_back({ "search",						true,	false,	false,	true,	true,	{}});
	commands.push_back({ "regex_search",				true,	false,	false,	true,	true,	{}});
	commands.push_back({ "ranged_search",				true,	false,	false,	false,	true,	{}});
	commands.push_back({ "chapter_search",				true,	false,	false,	false,	true,	{}});
	commands.push_back({ "move_down",					false,	false,	false,	false,	true,	{}});
	commands.push_back({ "move_up",						false,	false,	false,	false,	true,	{}});
	commands.push_back({ "move_left",					false,	false,	false,	false,	true,	{}});
	commands.push_back({ "move_right",					false,	false,	false,	false,	true,	{}});
	commands.push_back({ "zoom_in",						false,	false,	false,	false,	true,	{}});
	commands.push_back({ "zoom_out",					false,	false,	false,	false,	true,	{}});
	commands.push_back({ "fit_to_page_width",			false,	false,	false,	false,	true,	{}});
	commands.push_back({ "fit_to_page_height",			false,	false,	false,	false,	true,	{}});
	commands.push_back({ "fit_to_page_height_smart",	false,	false,	false,	false,	true,	{}});
	commands.push_back({ "fit_to_page_width_smart",		false,	false,	false,	false,	true,	{}});
	commands.push_back({ "next_page",					false,	false,	false,	false,	true,	{}});
	commands.push_back({ "previous_page",				false,	false,	false,	false,	true,	{}});
	commands.push_back({ "open_document",				false,	false,	true,	true,	false,	{}});
	commands.push_back({ "debug",						false,	false,	false,	false,	false,	{}});
	commands.push_back({ "add_bookmark",				true,	false,	false,	false,	true,	{}});
	commands.push_back({ "add_highlight",				false,	true,	false,	false,	true,	{}});
	commands.push_back({ "goto_toc",					false,	false,	false,	false,	true,	{}});
	commands.push_back({ "goto_highlight",				false,	false,	false,	true,	true,	{}});
	commands.push_back({ "goto_bookmark",				false,	false,	false,	true,	true,	{}});
	commands.push_back({ "goto_bookmark_g",				false,	false,	false,	true,	false,	{}});
	commands.push_back({ "goto_highlight_g",			false,	false,	false,	true,	false,	{}});
	commands.push_back({ "goto_highlight_ranged",		false,	false,	false,	true,	true,	{}});
	commands.push_back({ "link",						false,	false,	false,	false,	true,	{}});
	commands.push_back({ "portal",						false,	false,	false,	false,	true,	{}});
	commands.push_back({ "next_state",					false,	false,	false,	false,	false,	{}});
	commands.push_back({ "prev_state",					false,	false,	false,	false,	false,	{}});
	commands.push_back({ "pop_state",					false,	false,	false,	false,	true,	{}});
	commands.push_back({ "test_command",				false,	false,	false,	false,	false,	{}});
	commands.push_back({ "delete_link",					false,	false,	false,	false,	true,	{}});
	commands.push_back({ "delete_portal",				false,	false,	false,	false,	true,	{}});
	commands.push_back({ "delete_bookmark",				false,	false,	false,	false,	true,	{}});
	commands.push_back({ "delete_highlight",			false,	false,	false,	false,	true,	{}});
	commands.push_back({ "goto_link",					false,	false,	false,	true,	true,	{}});
	commands.push_back({ "goto_portal",					false,	false,	false,	true,	true,	{}});
	commands.push_back({ "edit_link",					false,	false,	false,	true,	true,	{}});
	commands.push_back({ "edit_portal",					false,	false,	false,	true,	true,	{}});
	commands.push_back({ "open_prev_doc",				false,	false,	false,	true,	false,	{}});
	commands.push_back({ "open_document_embedded",		false,	false,	false,	true,	false,	{}});
	commands.push_back(
		{ "open_document_embedded_from_current_path",	false,	false,	false,	true,	false,	{}});
	commands.push_back({ "copy",						false,	false,	false,	false,	true,	{}});
	commands.push_back({ "toggle_fullscreen",			false,	false,	false,	false,	false,	{}});
	commands.push_back({ "toggle_one_window",			false,	false,	false,	false,	false,	{}});
	commands.push_back({ "toggle_highlight",			false,	false,	false,	false,	false,	{}});
	commands.push_back({ "toggle_synctex",				false,	false,	false,	false,	false,	{}});
	commands.push_back({ "toggle_show_last_command",	false,	false,	false,	false,	false,	{}});
	//commands.push_back({ "command", true, false , false, false, {}});
	commands.push_back({ "command",						false,	false,	false,	false,	false,	{}});
	//commands.push_back({ "search_selected_text_in_google_scholar", false, false , false, false, {}});
	//commands.push_back({ "search_selected_text_in_libgen", false, false , false, false, {}});
	commands.push_back({ "external_search",				false,	true,	false,	false,	true,	{}});
	commands.push_back({ "open_selected_url",			false,	false,	false,	false,	true,	{}});
	commands.push_back({ "screen_down",					false,	false,	false,	false,	true,	{}});
	commands.push_back({ "screen_up",					false,	false,	false,	false,	true,	{}});
	commands.push_back({ "next_chapter",				false,	false,	false,	false,	true,	{}});
	commands.push_back({ "prev_chapter",				false,	false,	false,	false,	true,	{}});
	commands.push_back({ "toggle_dark_mode",			false,	false,	false,	false,	false,	{}});
	commands.push_back({ "toggle_presentation_mode",	false,	false,	false,	false,	true,	{}});
	commands.push_back({ "toggle_mouse_drag_mode",		false,	false,	false,	false,	false,	{}});
	commands.push_back({ "close_window",				false,	false,	false,	false,	false,	{}});
	commands.push_back({ "quit",						false,	false,	false,	false,  false,	{}});
	commands.push_back({ "q",							false,	false,	false,	false,	false,	{}});
	commands.push_back({ "open_link",					true,	false,	false,	false,	true,	{}});
	commands.push_back({ "keyboard_select",				true,	false,	false,	false,	true,	{}});
	commands.push_back({ "keyboard_smart_jump",			true,	false,	false,	false,	true,	{}});
	commands.push_back({ "keyboard_overview",			true,	false,	false,	false,	true,	{}});
	commands.push_back({ "keys",						false,	false,	false,	false,	false,	{}});
	commands.push_back({ "keys_user",					false,	false,	false,	false,	false,	{}});
	commands.push_back({ "prefs",						false,	false,	false,	false,	false,	{}});
	commands.push_back({ "prefs_user",					false,	false,	false,	false,	false,	{}});
	commands.push_back({ "import",						false,	false,	false,	false,	false,	{}});
	commands.push_back({ "export",						false,	false,	false,	false,	false,	{}});
	commands.push_back({ "enter_visual_mark_mode",		false,	false,	false,	false,	true,	{}});
	commands.push_back({ "move_visual_mark_down",		false,	false,	false,	false,	true,	{}});
	commands.push_back({ "move_visual_mark_up",			false,	false,	false,	false,	true,	{}});
	commands.push_back({ "set_page_offset",				true,	false,	false,	false,	true,	{}});
	commands.push_back({ "toggle_visual_scroll",		false,	false,	false,	false,	true,	{}});
	commands.push_back(
		{ "toggle_horizontal_scroll_lock",				false,	false,	false,	false,	true,	{}});
	commands.push_back({ "toggle_custom_color",			false,	false,	false,	false,	false,	{}});
	commands.push_back({ "execute",						true,	false,	false,	false,	true,	{}});
	commands.push_back({ "execute_predefined_command",	false,	true,	false,	false,	true,	{}});
	commands.push_back({ "embed_annotations",			false,	false,	false,	false,	true,	{}});
	commands.push_back({ "copy_window_size_config",		false,	false,	false,	false,	false,	{}});
	commands.push_back({ "toggle_select_highlight",		false,	false,	false,	false,	false,	{}});
	commands.push_back({ "set_select_highlight_type",	false,	true,	false,	false,	false,	{}});
	commands.push_back({ "open_last_document",			false,	false,	false,	false,	false,	{}});
	commands.push_back({ "toggle_window_configuration", false,	false,	false,	false,	false,	{}});
	commands.push_back({ "prefs_user_all",				false,	false,	false,	false,	false,	{}});
	commands.push_back({ "keys_user_all",				false,	false,	false,	false,	false,	{}});
	commands.push_back({ "fit_to_page_width_ratio",		false,	false,	false,	false,	true,	{}});
    commands.push_back({ "smart_jump_under_cursor",		false,	false,	false,	false,	true,	{}});
    commands.push_back({ "overview_under_cursor",		false,	false,	false,	false,	true,	{}});
    commands.push_back({ "close_overview",				false,	false,	false,	false,	false,	{}});
    commands.push_back({ "visual_mark_under_cursor",	false,	false,	false,	false,	true,	{}});
    commands.push_back({ "close_visual_mark",			false,	false,	false,	false,	false,	{}});
    commands.push_back({ "zoom_in_cursor",				false,	false,	false,	false,	true,	{}});
    commands.push_back({ "zoom_out_cursor",				false,	false,	false,	false,	true,	{}});
    commands.push_back({ "goto_left",					false,	false,	false,	false,	true,	{}});
    commands.push_back({ "goto_left_smart",				false,	false,	false,	false,	true,	{}});
    commands.push_back({ "goto_right",					false,	false,	false,	false,	true,	{}});
    commands.push_back({ "goto_right_smart",			false,	false,	false,	false,	true,	{}});
    commands.push_back({ "rotate_clockwise",			false,	false,	false,	false,	true,	{}});
    commands.push_back({ "rotate_counterclockwise",		false,	false,	false,	false,	true,	{}});
    commands.push_back({ "goto_next_highlight",			false,	false,	false,	false,	true,	{}});
    commands.push_back({ "goto_prev_highlight",			false,	false,	false,	false,	true,	{}});
    commands.push_back({ "goto_next_highlight_of_type",	false,	false,	false,	false,	true,	{}});
    commands.push_back({ "goto_prev_highlight_of_type", false,	false,	false,	false,	true,	{}});
    commands.push_back(
		{ "add_highlight_with_current_type",			false,	false,	false,	false,	true,	{}});
	commands.push_back({ "enter_password",				true,	false,	false,	false,	true,	{}});
	commands.push_back({ "toggle_fastread",				false,	false,	false,	false,	true,	{}});
	commands.push_back({ "goto_top_of_page",			false,	false,	false,	false,	true,	{}});
	commands.push_back({ "goto_bottom_of_page",			false,	false,	false,	false,	true,	{}});
	commands.push_back({ "new_window",					false,	false,	false,	false,	false,	{}});
	commands.push_back({ "toggle_statusbar",			false,	false,	false,	false,	false,	{}});
	commands.push_back({ "reload",						false,	false,	false,	false,	true,	{}});
	commands.push_back({ "reload_config",				false,	false,	false,	false,	false,	{}});
	commands.push_back({ "synctex_under_cursor",		false,	false,	false,	false,	true,	{}});
	commands.push_back({ "set_status_string",			true,	false,	false,	false,	false,	{}});
	commands.push_back({ "clear_status_string",			false,	false,	false,	false,	false,	{}});
	commands.push_back({ "toggle_titlebar",				false,	false,	false,	false,	false,	{}});
	commands.push_back({ "next_preview",				false,	false,	false,	false,	true,	{}});
	commands.push_back({ "previous_preview",			false,	false,	false,	false,	true,	{}});
	commands.push_back({ "goto_overview",				false,	false,	false,	false,	true,	{}});
	commands.push_back({ "portal_to_overview",			false,	false,	false,	false,	true,	{}});
	commands.push_back({ "goto_selected_text",			false,	false,	false,	false,	true,	{}});
	commands.push_back({ "focus_text",					true,	false,	false,	false,	true,	{}});
	commands.push_back({ "goto_window",					false,	false,	false,	false,	false,	{}});
	commands.push_back({ "set_custom_text_color",		true,	false,	false,	false,	false,	{}});
	commands.push_back({ "set_custom_background_color",	true,	false,	false,	false,	false,	{}});
	commands.push_back({ "toggle_hyperdrive_mode",		false,	false,	false,	false,	false,	{}});
	commands.push_back({ "toggle_smooth_scroll_mode",	false,	false,	false,	false,	false,	{}});
	commands.push_back({ "goto_begining",				false,	false,	false,	true,	true,	{}});
	commands.push_back({ "toggle_scrollbar",			false,	false,	false,	false,	false,	{}});
	commands.push_back({ "overview_to_portal",			false,	false,	false,	false,	false,	{}});
	commands.push_back({ "source_config",				false,	false,	true,	false ,	false,	{} });

	for (auto [command_name, _] : ADDITIONAL_COMMANDS) {
		commands.push_back({ utf8_encode(command_name) , false, false, false, false, true});
	}

	for (auto [command_name, _] : ADDITIONAL_MACROS) {
		commands.push_back({ utf8_encode(command_name) , false, false, false, false, true});
	}

	for (char c = 'a'; c <= 'z'; c++) {
		commands.push_back({ "execute_command_"  +  std::string(1, c), false, false , false, false, true, {}});
	}

	std::vector<Config> configs = config_manager->get_configs();

	for (auto conf : configs) {
		std::string config_set_command_name = "setconfig_" + utf8_encode(conf.name);
		commands.push_back({ config_set_command_name, true, false , false, false, true, {} });

	}

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

InputParseTreeNode* parse_lines(
	InputParseTreeNode* root,
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
                        std::wcout
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
				if ((parent_node->name.size() == 0) || parent_node->name[0].compare(command_names[j][0]) != 0) {

					std::wcout << L"Warning: key defined in " << parent_node->defining_file_path
						<< L":" << parent_node->defining_file_line
						<< L" overwritten by " << command_file_names[j]
						<< L":" << command_line_numbers[j];
					if (parent_node->name.size() > 0) {
						std::wcout << L". Overriding command: " << line 
							<< L": replacing " << utf8_decode(parent_node->name[0])
							<< L" with " << utf8_decode(command_names[j][0]);
					}
					std::wcout << L"\n";
				}
			}
			if ((size_t) i == (tokens.size() - 1)) {
				parent_node->is_final = true;
				parent_node->name.clear();
                parent_node->defining_file_line = command_line_numbers[j];
                parent_node->defining_file_path = command_file_names[j];
				for (size_t k = 0; k < command_names[j].size(); k++) {
					parent_node->name.push_back(command_names[j][k]);
				}
			}

		}
	}

	return root;
}

InputParseTreeNode* parse_lines(
	const std::vector<std::wstring>& lines,
	const std::vector<std::vector<std::string>>& command_names,
	const std::vector<std::wstring>& command_file_names,
	const std::vector<int>& command_line_numbers
	) {
	// parse key configs into a trie where leaves are annotated with the name of the command

	InputParseTreeNode* root = new InputParseTreeNode;
	root->is_root = true;

	parse_lines(root, lines, command_names, command_file_names, command_line_numbers);

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
		std::wstringstream ss(line);
		std::wstring command_name;
		std::wstring command_key;
		ss >> command_name >> command_key;
		//command_names.push_back(utf8_encode(command_name));
		command_names.push_back(parse_command_name(command_name));
		command_keys.push_back(command_key);
		command_files.push_back(default_path_name);
		command_line_numbers.push_back(line_number);
	}

	infile.close();
}

InputParseTreeNode* parse_key_config_files(const Path& default_path,
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

	return parse_lines(command_keys, command_names, command_files, command_line_numbers);
}


InputHandler::InputHandler(const Path& default_path, const std::vector<Path>& user_paths, CommandManager* cm) {
	command_manager = cm;
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

std::vector<const Command*> InputHandler::handle_key(QKeyEvent* key_event, bool shift_pressed, bool control_pressed, bool alt_pressed, int* num_repeats) {
	std::vector<const Command*> res;

	int key = 0;
	if (!USE_LEGACY_KEYBINDS){
		if ((key_event->text().size() > 0) && (key_event->text() != "\b") && (key_event->text() != "\t") && (key_event->text() != " ")) {
			if (!control_pressed && !alt_pressed) {
				// shift is already handled in the returned text
				shift_pressed = false;
				std::wstring text = key_event->text().toStdWString();
				key = key_event->text().toStdWString()[0];
			}
			else {
				key = key_event->key();
				if (key >= 'A' && key <= 'Z') {
					key = key - 'A' + 'a';
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
			return {};
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

				//return command_manager.get_command_with_name(child->name);
				for (size_t i = 0; i < child->name.size(); i++) {
					res.push_back(command_manager->get_command_with_name(child->name[i]));
				}
				return res;
			}
			else{
				current_node = child;
				return {};
			}
		}
	}
	std::wcout << "Warning: invalid command (key:" << (char)key << "); resetting to root" << std::endl;
	number_stack.clear();
	current_node = root;
	return {};
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
		if (thisroot->name.size() == 1) {
			map[thisroot->name[0]].push_back(get_key_string_from_tree_node_sequence(prefix));
		}
		else if (thisroot->name.size() > 1) {
			for (const auto& name : thisroot->name) {
				map[name].push_back("{" + get_key_string_from_tree_node_sequence(prefix) + "}");
			}
		}
	}
	else{
		for (size_t i = 0; i < thisroot->children.size(); i++) {
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

std::string InputHandler::get_key_string_from_tree_node_sequence(const std::vector<InputParseTreeNode*> seq) const{
	std::string res;
	for (size_t i = 0; i < seq.size(); i++) {
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

