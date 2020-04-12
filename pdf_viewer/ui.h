#pragma once
#include <string>
#include <vector>
#include <Windows.h>
#include "utils.h"
#include "imgui.h"

using namespace std;

const int max_select_size = 100;

template<typename T>
class FilteredSelect {
private:
	vector<string> options;
	vector<T> values;
	int current_index;
	bool is_done;
	char select_string[max_select_size];

public:
	FilteredSelect(vector<string> options, vector<T> values) :
		options(options), values(values), current_index(0), is_done(false) {
		ZeroMemory(select_string, sizeof(select_string));
	}
	T* get_value() {
		int index = get_selected_index();
		if (index >= 0 && index < values.size()) {
			return &values[get_selected_index()];
		}
		return nullptr;
	}
	bool is_string_comppatible(string incomplete_string, string option_string) {
		incomplete_string = to_lower(incomplete_string);
		option_string = to_lower(option_string);
		return option_string.find(incomplete_string) < option_string.size();
	}
	int get_selected_index() {
		int index = -1;
		for (int i = 0; i < options.size(); i++) {
			if (is_string_comppatible(select_string, options[i])) {
				index += 1;
			}
			if (index == current_index) {
				return i;
			}
		}
		return index;
	}
	string get_selected_option() {
		return options[get_selected_index()];
	}
	int get_max_index() {
		int max_index = -1;
		for (int i = 0; i < options.size(); i++) {
			if (is_string_comppatible(select_string, options[i])) {
				max_index += 1;
			}
		}
		return max_index;
	}
	void move_item(int offset) {
		int new_index = offset + current_index;
		if (new_index < 0) {
			new_index = 0;
		}
		int max_index = get_max_index();
		if (new_index > max_index) {
			new_index = max_index;
		}
		current_index = new_index;
	}
	void next_item() {
		move_item(1);
	}
	void prev_item() {
		move_item(-1);
	}
	bool render() {
		ImGui::Begin("Select");

		ImGui::SetKeyboardFocusHere();
		if (ImGui::InputText("search string", select_string, sizeof(select_string), ImGuiInputTextFlags_CallbackHistory | ImGuiInputTextFlags_EnterReturnsTrue,
			[](ImGuiInputTextCallbackData* data) {
				FilteredSelect* filtered_select = (FilteredSelect*)data->UserData;

				if (data->EventKey == ImGuiKey_UpArrow) {
					filtered_select->prev_item();
				}
				if (data->EventKey == ImGuiKey_DownArrow) {
					filtered_select->next_item();
				}

				return 0;
			}, this)) {
			is_done = true;
		}
		//ImGui::InputText("search", text_buffer, 100, ImGuiInputTextFlags_CallbackHistory, [&](ImGuiInputTextCallbackData* data) {

		int index = 0;
		for (int i = 0; i < options.size(); i++) {
			//if (options[i].find(select_string) == 0) {
			if (is_string_comppatible(select_string, options[i])) {

				if (current_index == index) {
					ImGui::SetScrollHere();
				}

				ImGui::Selectable(options[i].c_str(), current_index == index);
				index += 1;
			}
		}

		ImGui::End();
		return is_done;
	}
};

bool select_pdf_file_name(char* out_file_name, int max_length);
