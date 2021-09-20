#include "config.h"
#include "utils.h"
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
extern bool SHOULD_CHECK_FOR_LATEST_VERSION_ON_STARTUP;
extern bool SHOULD_DRAW_UNRENDERED_PAGES;
extern bool HOVER_OVERVIEW;
extern bool DEFAULT_DARK_MODE;
extern float HIGHLIGHT_COLORS[26 * 3];
extern std::wstring LIBGEN_ADDRESS;
extern std::wstring GOOGLE_SCHOLAR_ADDRESS;
extern std::wstring INVERSE_SEARCH_COMMAND;
extern std::wstring SHARED_DATABASE_PATH;

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

ConfigManager::ConfigManager(const Path& default_path, const std::vector<Path>& user_paths) {

	user_config_paths = user_paths;
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
	configs.push_back({ L"default_dark_mode", &DEFAULT_DARK_MODE, bool_serializer, bool_deserializer });
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
	configs.push_back({ L"should_draw_unrendered_pages", &SHOULD_DRAW_UNRENDERED_PAGES, bool_serializer, bool_deserializer });
	configs.push_back({ L"check_for_updates_on_startup", &SHOULD_CHECK_FOR_LATEST_VERSION_ON_STARTUP, bool_serializer, bool_deserializer });
	configs.push_back({ L"shared_database_path", &SHARED_DATABASE_PATH, string_serializer, string_deserializer });
	configs.push_back({ L"hover_overview", &HOVER_OVERVIEW, bool_serializer, bool_deserializer });

	std::wstring highlight_config_string = L"highlight_color_a";
	for (char highlight_type = 'a'; highlight_type <= 'z'; highlight_type++) {
		highlight_config_string[highlight_config_string.size() - 1] = highlight_type;
		configs.push_back({ highlight_config_string, &HIGHLIGHT_COLORS[(highlight_type - 'a') * 3], vec3_serializer, vec3_deserializer });
	}

	deserialize(default_path, user_paths);
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

void ConfigManager::deserialize(const Path& default_file_path, const std::vector<Path>& user_file_paths) {
	//std::string default_path_utf8 = default_path.get_path_utf8();
	//std::string user_path_utf8 = utf8_encode(user_path);

	//std::wifstream default_infile(default_path_utf8);
	//std::wifstream user_infile(user_path_utf8);


	std::wstring line;
	std::wifstream default_file(default_file_path.get_path_utf8());
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
	default_file.close();

	for (int i = 0; i < user_file_paths.size(); i++) {

		if (user_file_paths[i].file_exists()) {
			std::wifstream user_file(user_file_paths[i].get_path_utf8());
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
			user_file.close();

		}

	}
}

std::optional<Path> ConfigManager::get_or_create_user_config_file() {
	if (user_config_paths.size() == 0) {
		return {};
	}

	for (int i = user_config_paths.size() - 1; i >= 0; i--) {
		if (user_config_paths[i].file_exists()) {
			return user_config_paths[i];
		}
	}
	user_config_paths.back().file_parent().create_directories();
	create_file_if_not_exists(user_config_paths.back().get_path());
	return user_config_paths.back();
}
