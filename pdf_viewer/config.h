#pragma once

#include <vector>
#include <string>
#include <sstream>
#include <fstream>
#include <iostream>

using namespace std;


void int_serializer(void* int_pointer, wstringstream& stream);

void* int_deserializer(wstringstream& stream);

void float_serializer(void* float_pointer, wstringstream& stream);

void* float_deserializer(wstringstream& stream);

struct Config {

	wstring name;
	void* value;
	void (*serialize) (void*, wstringstream&);
	void* (*deserialize) (wstringstream&, void* res);

	void* get_value();

};

class ConfigManager {
	vector<Config> configs;

	Config* get_mut_config_with_name(wstring config_name);

public:

	ConfigManager(wstring path);
	void serialize(wofstream& file);
	void deserialize(wifstream& file);
	template<typename T>
	const T* get_config(wstring name) {
		return (T*)get_mut_config_with_name(name)->get_value();
	}
};
