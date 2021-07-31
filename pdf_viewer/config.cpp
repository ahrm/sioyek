#include "config.h"
#include <cassert>

extern float ZOOM_INC_FACTOR;
extern float VERTICAL_LINE_WIDTH;
extern float VERTICAL_LINE_FREQ;
extern float VERTICAL_MOVE_AMOUNT;
extern float HORIZONTAL_MOVE_AMOUNT;
extern float MOVE_SCREEN_PERCENTAGE;
extern float BACKGROUND_COLOR[3];
extern float DARK_MODE_BACKGROUND_COLOR[3];
extern float DARK_MODE_CONTRAST;
extern bool FLAT_TABLE_OF_CONTENTS;
extern bool SHOULD_USE_MULTIPLE_MONITORS;
extern bool SHOULD_LOAD_TUTORIAL_WHEN_NO_OTHER_FILE;
extern bool SHOULD_LAUNCH_NEW_INSTANCE;
extern std::wstring LIBGEN_ADDRESS;
extern std::wstring GOOGLE_SCHOLAR_ADDRESS;
extern std::wstring INVERSE_SEARCH_COMMAND;

template<typename T>
void* generic_deserializer(std::wstringstream& stream, void* res_) {
	T* res = static_cast<T*>(res_);
	stream >> *res;
	return res;
}

void int_serializer(void* int_pointer, std::wstringstream& stream) {
	stream << *(int*)int_pointer;
}

void bool_serializer(void* bool_pointer, std::wstringstream& stream) {
	stream << *(bool*)bool_pointer;
}

void string_serializer(void* string_pointer, std::wstringstream& stream) {
	stream << *(std::wstring*)string_pointer;
}

void* string_deserializer(std::wstringstream& stream, void* res_) {
	assert(res_ != nullptr);
	//delete res_;
	
	std::wstring* res = static_cast<std::wstring*>(res_);
	res->clear();
	std::getline(stream, *res);
	while (iswspace((*res)[0])) {
		res->erase(res->begin());

	}
	return res;
}

template<int N>
void vec_n_serializer(void* vec_n_pointer, std::wstringstream& stream) {
	for (int i = 0; i < N; i++) {
		stream << *(((float*)vec_n_pointer) + i);
	}
}

template<int N>
void* vec_n_deserializer(std::wstringstream& stream, void* res_) {
	assert(res_ != nullptr);
	float* res = (float*)res_;
	if (res == nullptr) {
		res = new float[N];
	}

	for (int i = 0; i < N; i++) {
		stream >> *(res + i);
	}

	return res;
}

void float_serializer(void* float_pointer, std::wstringstream& stream) {
	stream << *(float*)float_pointer;
}


void* Config::get_value() {
	return value;
}

Config* ConfigManager::get_mut_config_with_name(std::wstring config_name) {
	for (auto& it : configs) {
		if (it.name == config_name) return &it;
	}
	return nullptr;
}

ConfigManager::ConfigManager(const std::filesystem::path& default_path, const std::filesystem::path& user_path) {

	auto vec3_serializer = vec_n_serializer<3>;
	auto vec4_serializer = vec_n_serializer<4>;
	auto vec3_deserializer = vec_n_deserializer<3>;
	auto vec4_deserializer = vec_n_deserializer<4>;
	auto float_deserializer = generic_deserializer<float>;
	auto bool_deserializer = generic_deserializer<bool>;

	configs.push_back({ L"text_highlight_color", DEFAULT_TEXT_HIGHLIGHT_COLOR, vec3_serializer, vec3_deserializer });
	configs.push_back({ L"vertical_line_color", DEFAULT_VERTICAL_LINE_COLOR, vec4_serializer, vec4_deserializer });
	configs.push_back({ L"vertical_line_width", &VERTICAL_LINE_WIDTH, float_serializer, float_deserializer });
	configs.push_back({ L"vertical_line_freq", &VERTICAL_LINE_FREQ, float_serializer, float_deserializer });
	configs.push_back({ L"search_highlight_color", DEFAULT_SEARCH_HIGHLIGHT_COLOR, vec3_serializer, vec3_deserializer });
	configs.push_back({ L"link_highlight_color", DEFAULT_LINK_HIGHLIGHT_COLOR, vec3_serializer, vec3_deserializer });
	configs.push_back({ L"synctex_highlight_color", DEFAULT_SYNCTEX_HIGHLIGHT_COLOR, vec3_serializer, vec3_deserializer });
	configs.push_back({ L"background_color", BACKGROUND_COLOR, vec3_serializer, vec3_deserializer });
	configs.push_back({ L"dark_mode_background_color", DARK_MODE_BACKGROUND_COLOR, vec3_serializer, vec3_deserializer });
	configs.push_back({ L"dark_mode_contrast", &DARK_MODE_CONTRAST, float_serializer, float_deserializer });
	configs.push_back({ L"google_scholar_address", &GOOGLE_SCHOLAR_ADDRESS, string_serializer, string_deserializer });
	configs.push_back({ L"inverse_search_command", &INVERSE_SEARCH_COMMAND, string_serializer, string_deserializer });
	configs.push_back({ L"libgen_address", &LIBGEN_ADDRESS, string_serializer, string_deserializer });
	configs.push_back({ L"zoom_inc_factor", &ZOOM_INC_FACTOR, float_serializer, float_deserializer });
	configs.push_back({ L"vertical_move_amount", &VERTICAL_MOVE_AMOUNT, float_serializer, float_deserializer });
	configs.push_back({ L"horizontal_move_amount", &HORIZONTAL_MOVE_AMOUNT, float_serializer, float_deserializer });
	configs.push_back({ L"move_screen_percentage", &MOVE_SCREEN_PERCENTAGE, float_serializer, float_deserializer });
	configs.push_back({ L"flat_toc", &FLAT_TABLE_OF_CONTENTS, bool_serializer, bool_deserializer });
	configs.push_back({ L"should_use_multiple_monitors", &SHOULD_USE_MULTIPLE_MONITORS, bool_serializer, bool_deserializer });
	configs.push_back({ L"should_load_tutorial_when_no_other_file", &SHOULD_LOAD_TUTORIAL_WHEN_NO_OTHER_FILE, bool_serializer, bool_deserializer });
	configs.push_back({ L"should_launch_new_instance", &SHOULD_LAUNCH_NEW_INSTANCE, bool_serializer, bool_deserializer });
//extern bool should_load_tutorial_when_no_other_file = false;

	std::wifstream default_infile(default_path);
	std::wifstream user_infile(user_path);
	deserialize(default_infile, user_infile);
	default_infile.close();
	user_infile.close();
}

//void ConfigManager::serialize(std::wofstream& file) {
//	for (auto it : configs) {
//		std::wstringstream ss;
//		file << it.name << " ";
//		if (it.get_value()) {
//			it.serialize(it.get_value(), ss);
//		}
//		file << ss.str() << std::endl;
//	}
//}

void ConfigManager::deserialize(std::wifstream& default_file, std::wifstream& user_file) {
	std::wstring line;

	while (std::getline(default_file, line)) {

		if (line.size() == 0 || line[0] == '#') {
			continue;
		}

		std::wstringstream ss{ line };
		std::wstring conf_name;
		ss >> conf_name;
		Config* conf = get_mut_config_with_name(conf_name);
		if (conf) {
			conf->value = conf->deserialize(ss, conf->value);
		}
	}

	while (std::getline(user_file, line)) {

		if (line.size() == 0 || line[0] == '#') {
			continue;
		}

		std::wstringstream ss{ line };
		std::wstring conf_name;
		ss >> conf_name;
		Config* conf = get_mut_config_with_name(conf_name);
		if (conf) {
			conf->value = conf->deserialize(ss, conf->value);
		}
	}
}
