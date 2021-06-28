#pragma once

#include <vector>
#include <string>
#include <sstream>
#include <fstream>
#include <iostream>
#include <filesystem>


void int_serializer(void* int_pointer, std::wstringstream& stream);

void* int_deserializer(std::wstringstream& stream);

void float_serializer(void* float_pointer, std::wstringstream& stream);

void* float_deserializer(std::wstringstream& stream);

struct Config {

	std::wstring name;
	void* value = nullptr;
	void (*serialize) (void*, std::wstringstream&) = nullptr;
	void* (*deserialize) (std::wstringstream&, void* res) = nullptr;

	void* get_value();

};

class ConfigManager {
	std::vector<Config> configs;

	Config* get_mut_config_with_name(std::wstring config_name);
	float default_text_highlight_color[3];
	float default_vertical_line_color[4];
	float default_search_highlight_color[3];
	float default_link_highlight_color[3];

public:

	ConfigManager(std::filesystem::path path);
	void serialize(std::wofstream& file);
	void deserialize(std::wifstream& file);
	template<typename T>
	const T* get_config(std::wstring name) {

		void* raw_pointer = get_mut_config_with_name(name)->get_value();
		// todo: Provide a default value for all configs, so that all the nullchecks here and in the
		// places where `get_config` is called can be removed.
		if (raw_pointer == nullptr) return nullptr;
		return (T*)raw_pointer;
	}
};
