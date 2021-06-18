#pragma once

#include <vector>
#include <string>
#include <sstream>
#include <fstream>
#include <iostream>


void int_serializer(void* int_pointer, std::wstringstream& stream);

void* int_deserializer(std::wstringstream& stream);

void float_serializer(void* float_pointer, std::wstringstream& stream);

void* float_deserializer(std::wstringstream& stream);

struct Config {

	std::wstring name;
	void* value;
	void (*serialize) (void*, std::wstringstream&);
	void* (*deserialize) (std::wstringstream&, void* res);

	void* get_value();

};

class ConfigManager {
	std::vector<Config> configs;

	Config* get_mut_config_with_name(std::wstring config_name);

public:

	ConfigManager(std::wstring path);
	void serialize(std::wofstream& file);
	void deserialize(std::wifstream& file);
	template<typename T>
	const T* get_config(std::wstring name) {
		return (T*)get_mut_config_with_name(name)->get_value();
	}
};
