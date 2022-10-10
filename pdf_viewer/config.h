#pragma once

#include <vector>
#include <string>
#include <sstream>
#include <fstream>
#include <iostream>
#include <optional>
#include "path.h"


struct Config {

	std::wstring name;
	void* value = nullptr;
	void (*serialize) (void*, std::wstringstream&) = nullptr;
	void* (*deserialize) (std::wstringstream&, void* res) = nullptr;
	bool (*validator) (const std::wstring& value);

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

	std::vector<Path> user_config_paths;

public:

	ConfigManager(const Path& default_path, const Path& auto_path ,const std::vector<Path>& user_paths);
	//void serialize(std::wofstream& file);
	void deserialize(const Path& default_file_path, const Path& auto_path, const std::vector<Path>& user_file_paths);
	void deserialize_file(const Path& file_path, bool warn_if_not_exists=false);
	template<typename T>
	const T* get_config(std::wstring name) {

		void* raw_pointer = get_mut_config_with_name(name)->get_value();

		// todo: Provide a default value for all configs, so that all the nullchecks here and in the
		// places where `get_config` is called can be removed.
		if (raw_pointer == nullptr) return nullptr;
		return (T*)raw_pointer;
	}
	std::optional<Path> get_or_create_user_config_file();
	std::vector<Path> get_all_user_config_files();
	std::vector<Config> get_configs();
	void deserialize_config(std::string config_name, std::wstring config_value);
};
