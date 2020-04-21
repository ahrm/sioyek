#include "config.h"

void int_serializer(void* int_pointer, stringstream& stream) {
	stream << *(int*)int_pointer;
}

void* int_deserializer(stringstream& stream, void* res_ = nullptr) {
	int* res = (int*)res_;
	if (res == nullptr) {
		res = new int;
	}
	stream >> *res;
	return res;
}

void* vec3_deserializer(stringstream& stream, void* res_ = nullptr) {
	float* res = (float*)res_;
	if (res == nullptr) {
		res = new float[3];
	}

	stream >> *(res + 0) >> *(res + 1) >> *(res + 2);
	return res;
}

void float_serializer(void* float_pointer, stringstream& stream) {
	stream << *(float*)float_pointer;
}

void vec3_serializer(void* vec3_pointer, stringstream& stream) {
	stream << *((float*)(vec3_pointer) + 0);
	stream << *((float*)(vec3_pointer) + 1);
	stream << *((float*)(vec3_pointer) + 2);
}

void* float_deserializer(stringstream& stream, void* res_) {
	float* res = (float*)res_;

	if (res == nullptr) {
		res = new float;
	}

	stream >> *res;
	return res;
}

void* Config::get_value() {
	return value;
}

Config* ConfigManager::get_mut_config_with_name(string config_name) {
	for (auto& it : configs) {
		if (it.name == config_name) return &it;
	}
	return nullptr;
}

ConfigManager::ConfigManager(string path) {
	configs.push_back({ "text_highlight_color", nullptr, vec3_serializer, vec3_deserializer });
	configs.push_back({ "search_highlight_color", nullptr, vec3_serializer, vec3_deserializer });
	configs.push_back({ "link_highlight_color", nullptr, vec3_serializer, vec3_deserializer });
	ifstream infile(path);
	deserialize(infile);
	infile.close();
}

void ConfigManager::serialize(ofstream& file) {
	for (auto it : configs) {
		stringstream ss;
		file << it.name << " ";
		if (it.get_value()) {
			it.serialize(it.get_value(), ss);
		}
		file << ss.str() << endl;
	}
}

void ConfigManager::deserialize(ifstream& file) {
	string line;

	while (getline(file, line)) {
		stringstream ss{ line };
		string conf_name;
		ss >> conf_name;
		Config* conf = get_mut_config_with_name(conf_name);
		if (conf) {
			conf->value = conf->deserialize(ss, conf->value);
		}
	}
}
