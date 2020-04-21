#pragma once

#include <vector>
#include <string>
#include <sstream>
#include <fstream>
#include <iostream>

using namespace std;


void int_serializer(void* int_pointer, stringstream& stream);

void* int_deserializer(stringstream& stream);

void float_serializer(void* float_pointer, stringstream& stream);

void* float_deserializer(stringstream& stream);

struct Config {

	string name;
	void* value;
	void (*serialize) (void*, stringstream&);
	void* (*deserialize) (stringstream&, void* res);

	void* get_value();

};

class ConfigManager {
	vector<Config> configs;

	Config* get_mut_config_with_name(string config_name);

public:

	ConfigManager(string path);
	void serialize(ofstream& file);
	void deserialize(ifstream& file);
	template<typename T>
	T* get_config(string name) {
		return (T*)get_mut_config_with_name(name)->get_value();
	}
};
