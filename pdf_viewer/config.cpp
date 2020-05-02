#include "config.h"

extern float ZOOM_INC_FACTOR;
extern float vertical_move_amount;
extern float horizontal_move_amount;
extern float move_screen_percentage;
extern float background_color[3];

void int_serializer(void* int_pointer, wstringstream& stream) {
	stream << *(int*)int_pointer;
}

void string_serializer(void* string_pointer, wstringstream& stream) {
	stream << *(wstring*)string_pointer;
}

void* int_deserializer(wstringstream& stream, void* res_ = nullptr) {
	int* res = (int*)res_;
	if (res == nullptr) {
		res = new int;
	}
	stream >> *res;
	return res;
}

void* string_deserializer(wstringstream& stream, void* res_ = nullptr) {
	delete res_;
	
	//wstring res(300);
	wstring res;
	getline(stream, res);
	//stream >> res;
	return new wstring(res);
}

void* vec3_deserializer(wstringstream& stream, void* res_ = nullptr) {
	float* res = (float*)res_;
	if (res == nullptr) {
		res = new float[3];
	}

	stream >> *(res + 0) >> *(res + 1) >> *(res + 2);
	return res;
}

void float_serializer(void* float_pointer, wstringstream& stream) {
	stream << *(float*)float_pointer;
}

void vec3_serializer(void* vec3_pointer, wstringstream& stream) {
	stream << *((float*)(vec3_pointer) + 0);
	stream << *((float*)(vec3_pointer) + 1);
	stream << *((float*)(vec3_pointer) + 2);
}

void* float_deserializer(wstringstream& stream, void* res_) {
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

Config* ConfigManager::get_mut_config_with_name(wstring config_name) {
	for (auto& it : configs) {
		if (it.name == config_name) return &it;
	}
	return nullptr;
}

ConfigManager::ConfigManager(wstring path) {
	configs.push_back({ L"text_highlight_color", nullptr, vec3_serializer, vec3_deserializer });
	configs.push_back({ L"search_highlight_color", nullptr, vec3_serializer, vec3_deserializer });
	configs.push_back({ L"link_highlight_color", nullptr, vec3_serializer, vec3_deserializer });
	configs.push_back({ L"background_color", background_color, vec3_serializer, vec3_deserializer });
	configs.push_back({ L"google_scholar_address", nullptr, string_serializer, string_deserializer });
	configs.push_back({ L"libgen_address", nullptr, string_serializer, string_deserializer });
	configs.push_back({ L"zoom_inc_factor", &ZOOM_INC_FACTOR, float_serializer, float_deserializer });
	configs.push_back({ L"vertical_move_amount", &vertical_move_amount, float_serializer, float_deserializer });
	configs.push_back({ L"horizontal_move_amount", &horizontal_move_amount, float_serializer, float_deserializer });
	configs.push_back({ L"move_screen_percentage", &move_screen_percentage, float_serializer, float_deserializer });
	configs.push_back({ L"item_list_stylesheet", nullptr, string_serializer, string_deserializer });
	configs.push_back({ L"item_list_selected_stylesheet", nullptr, string_serializer, string_deserializer });
	wifstream infile(path);
	deserialize(infile);
	infile.close();
}

void ConfigManager::serialize(wofstream& file) {
	for (auto it : configs) {
		wstringstream ss;
		file << it.name << " ";
		if (it.get_value()) {
			it.serialize(it.get_value(), ss);
		}
		file << ss.str() << endl;
	}
}

		//list_view->setStyleSheet("QListView::item::selected::{ background-color: white;color: black; }");
void ConfigManager::deserialize(wifstream& file) {
	wstring line;

	while (getline(file, line)) {
		wstringstream ss{ line };
		wstring conf_name;
		ss >> conf_name;
		Config* conf = get_mut_config_with_name(conf_name);
		if (conf) {
			conf->value = conf->deserialize(ss, conf->value);
		}
	}
}
