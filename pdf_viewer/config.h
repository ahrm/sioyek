#pragma once

#include <vector>
#include <string>
#include <sstream>
#include <fstream>
#include <iostream>
#include <filesystem>


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
	float DEFAULT_TEXT_HIGHLIGHT_COLOR[3];
	float DEFAULT_VERTICAL_LINE_COLOR[4];
	float DEFAULT_SEARCH_HIGHLIGHT_COLOR[3];
	float DEFAULT_LINK_HIGHLIGHT_COLOR[3];
	float DEFAULT_SYNCTEX_HIGHLIGHT_COLOR[3];

public:

	ConfigManager(const std::filesystem::path& default_path, const std::filesystem::path& user_path);
	//void serialize(std::wofstream& file);
	void deserialize(std::wifstream& default_file, std::wifstream& user_file);
	template<typename T>
	const T* get_config(std::wstring name) {

		void* raw_pointer = get_mut_config_with_name(name)->get_value();

		// todo: Provide a default value for all configs, so that all the nullchecks here and in the
		// places where `get_config` is called can be removed.
		if (raw_pointer == nullptr) return nullptr;
		return (T*)raw_pointer;
	}
};
