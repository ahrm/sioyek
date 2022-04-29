#include "config.h"
#include "utils.h"
#include <cassert>

extern float ZOOM_INC_FACTOR;
extern float VERTICAL_MOVE_AMOUNT;
extern float HORIZONTAL_MOVE_AMOUNT;
extern float MOVE_SCREEN_PERCENTAGE;
extern float BACKGROUND_COLOR[3];
extern float DARK_MODE_BACKGROUND_COLOR[3];
extern float DARK_MODE_CONTRAST;
extern bool FLAT_TABLE_OF_CONTENTS;
extern bool SMALL_TOC;
extern bool SHOULD_USE_MULTIPLE_MONITORS;
extern bool SORT_BOOKMARKS_BY_LOCATION;
extern bool SHOULD_LOAD_TUTORIAL_WHEN_NO_OTHER_FILE;
extern bool SHOULD_LAUNCH_NEW_INSTANCE;
extern bool SHOULD_CHECK_FOR_LATEST_VERSION_ON_STARTUP;
extern bool SHOULD_DRAW_UNRENDERED_PAGES;
extern bool HOVER_OVERVIEW;
//extern bool AUTO_EMBED_ANNOTATIONS;
extern bool DEFAULT_DARK_MODE;
extern float HIGHLIGHT_COLORS[26 * 3];
extern std::wstring SEARCH_URLS[26];
extern std::wstring EXECUTE_COMMANDS[26];
extern std::wstring LIBGEN_ADDRESS;
extern std::wstring GOOGLE_SCHOLAR_ADDRESS;
extern std::wstring INVERSE_SEARCH_COMMAND;
extern std::wstring SHARED_DATABASE_PATH;
extern std::wstring ITEM_LIST_PREFIX;
extern float VISUAL_MARK_NEXT_PAGE_FRACTION;
extern float VISUAL_MARK_NEXT_PAGE_THRESHOLD;
extern std::wstring UI_FONT_FACE_NAME;
extern std::wstring MIDDLE_CLICK_SEARCH_ENGINE;
extern std::wstring SHIFT_MIDDLE_CLICK_SEARCH_ENGINE;
extern std::wstring STARTUP_COMMANDS;
extern int FONT_SIZE;
extern float CUSTOM_BACKGROUND_COLOR[3];
extern float CUSTOM_TEXT_COLOR[3];
extern bool RERENDER_OVERVIEW;
extern bool WHEEL_ZOOM_ON_CURSOR;
extern bool RULER_MODE;
extern bool LINEAR_TEXTURE_FILTERING;
extern float DISPLAY_RESOLUTION_SCALE;
extern float STATUS_BAR_COLOR[3];
extern float STATUS_BAR_TEXT_COLOR[3];
extern int STATUS_BAR_FONT_SIZE;
extern int MAIN_WINDOW_SIZE[2];
extern int HELPER_WINDOW_SIZE[2];
extern int MAIN_WINDOW_MOVE[2];
extern int HELPER_WINDOW_MOVE[2];
extern float TOUCHPAD_SENSITIVITY;
extern float PAGE_SEPARATOR_WIDTH;
extern float PAGE_SEPARATOR_COLOR[3];
extern int SINGLE_MAIN_WINDOW_SIZE[2];
extern int SINGLE_MAIN_WINDOW_MOVE[2];
extern float FIT_TO_PAGE_WIDTH_RATIO;
extern float RULER_PADDING;
extern std::wstring TEXT_HIGHLIGHT_URL;
extern bool TEXT_SUMMARY_HIGHLIGHT_SHOULD_REFINE;
extern bool TEXT_SUMMARY_HIGHLIGHT_SHOULD_FILL;
extern int TEXT_SUMMARY_CONTEXT_SIZE;
extern bool USE_HEURISTIC_IF_TEXT_SUMMARY_NOT_AVAILABLE;
extern std::wstring PAPERS_FOLDER_PATH;
extern bool ENABLE_EXPERIMENTAL_FEATURES;
extern bool CREATE_TABLE_OF_CONTENTS_IF_NOT_EXISTS;
extern int MAX_CREATED_TABLE_OF_CONTENTS_SIZE;
extern bool FORCE_CUSTOM_LINE_ALGORITHM;

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

template<int N, typename T>
void vec_n_serializer(void* vec_n_pointer, std::wstringstream& stream) {
	for (int i = 0; i < N; i++) {
		stream << *(((T*)vec_n_pointer) + i);
	}
}

template<int N, typename T>
void* vec_n_deserializer(std::wstringstream& stream, void* res_) {
	assert(res_ != nullptr);
	T* res = (T*)res_;
	if (res == nullptr) {
		res = new T[N];
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

bool ensure_between_0_and_1(const QStringList& parts) {
	bool ok = false;
	for (int i = 0; i < parts.size(); i++) {
		int val = parts.at(i).toFloat(&ok);
		if (!ok) {
			std::wcout << L"Error: invalid value for color component: " << parts.at(i).toStdWString() << "\n";
			return false;
		}
		if (val < 0 || val > 1) {
			std::wcout << L"Error: invalid value for color component (must be between 0 and 1): " << parts.at(i).toStdWString() << "\n";
			return false;
		}
	}
	return true;
}

bool color_3_validator(const std::wstring& str) {
	QString qstr = QString::fromStdWString(str);
	auto parts = qstr.split(' ', Qt::SplitBehaviorFlags::SkipEmptyParts);
	if (parts.size() != 3) {
		std::wcout << L"Error: required 3 values for color, but got " << parts.size() << "\n";
		return false;
	}
	if (!ensure_between_0_and_1(parts)) {
		return false;
	}
	return true;
}

bool color_4_validator(const std::wstring& str) {
	QString qstr = QString::fromStdWString(str);
	auto parts = qstr.split(' ', Qt::SplitBehaviorFlags::SkipEmptyParts);
	if (parts.size() != 4) {
		std::wcout << L"Error: required 4 values for color, but got " << parts.size() << "\n";
		return false;
	}
	if (!ensure_between_0_and_1(parts)) {
		return false;
	}
	return true;
}

bool bool_validator(const std::wstring& str) {
	QString qstr = QString::fromStdWString(str);
	auto parts = qstr.split(' ', Qt::SplitBehaviorFlags::SkipEmptyParts);
	int is_correct = true;
	std::wstring msg = L"Bool values should be either 0 or 1, but got ";
	if (parts.size() != 1) {
		std::wcout << msg << str << L"\n";
		return false;
	}
	if (parts.at(0).trimmed().toStdWString() == L"0" || parts.at(0).trimmed().toStdWString() == L"1") {
		return true;
	}
	std::wcout << msg << str << L"\n";
	return false;
}

ConfigManager::ConfigManager(const Path& default_path, const Path& auto_path ,const std::vector<Path>& user_paths) {

	user_config_paths = user_paths;
	auto ivec2_serializer = vec_n_serializer<2, int>;
	auto ivec2_deserializer = vec_n_deserializer<2, int>;
	auto vec3_serializer = vec_n_serializer<3, float>;
	auto vec4_serializer = vec_n_serializer<4, float>;
	auto vec3_deserializer = vec_n_deserializer<3, float>;
	auto vec4_deserializer = vec_n_deserializer<4, float>;
	auto float_deserializer = generic_deserializer<float>;
	auto int_deserializer = generic_deserializer<int>;
	auto bool_deserializer = generic_deserializer<bool>;

	configs.push_back({ L"text_highlight_color", DEFAULT_TEXT_HIGHLIGHT_COLOR, vec3_serializer, vec3_deserializer, color_3_validator });
	configs.push_back({ L"vertical_line_color", DEFAULT_VERTICAL_LINE_COLOR, vec4_serializer, vec4_deserializer, color_4_validator });
	configs.push_back({ L"search_highlight_color", DEFAULT_SEARCH_HIGHLIGHT_COLOR, vec3_serializer, vec3_deserializer, color_3_validator });
	configs.push_back({ L"link_highlight_color", DEFAULT_LINK_HIGHLIGHT_COLOR, vec3_serializer, vec3_deserializer, color_3_validator });
	configs.push_back({ L"synctex_highlight_color", DEFAULT_SYNCTEX_HIGHLIGHT_COLOR, vec3_serializer, vec3_deserializer, color_3_validator });
	configs.push_back({ L"background_color", BACKGROUND_COLOR, vec3_serializer, vec3_deserializer, color_3_validator });
	configs.push_back({ L"dark_mode_background_color", DARK_MODE_BACKGROUND_COLOR, vec3_serializer, vec3_deserializer, color_3_validator });
	configs.push_back({ L"dark_mode_contrast", &DARK_MODE_CONTRAST, float_serializer, float_deserializer, nullptr });
	configs.push_back({ L"default_dark_mode", &DEFAULT_DARK_MODE, bool_serializer, bool_deserializer, bool_validator });
	configs.push_back({ L"google_scholar_address", &GOOGLE_SCHOLAR_ADDRESS, string_serializer, string_deserializer, nullptr });
	configs.push_back({ L"item_list_prefix", &ITEM_LIST_PREFIX, string_serializer, string_deserializer, nullptr });
	configs.push_back({ L"inverse_search_command", &INVERSE_SEARCH_COMMAND, string_serializer, string_deserializer, nullptr });
	configs.push_back({ L"libgen_address", &LIBGEN_ADDRESS, string_serializer, string_deserializer, nullptr });
	configs.push_back({ L"zoom_inc_factor", &ZOOM_INC_FACTOR, float_serializer, float_deserializer, nullptr });
	configs.push_back({ L"vertical_move_amount", &VERTICAL_MOVE_AMOUNT, float_serializer, float_deserializer, nullptr });
	configs.push_back({ L"horizontal_move_amount", &HORIZONTAL_MOVE_AMOUNT, float_serializer, float_deserializer, nullptr });
	configs.push_back({ L"move_screen_percentage", &MOVE_SCREEN_PERCENTAGE, float_serializer, float_deserializer, nullptr });
	configs.push_back({ L"move_screen_ratio", &MOVE_SCREEN_PERCENTAGE, float_serializer, float_deserializer, nullptr });
	configs.push_back({ L"flat_toc", &FLAT_TABLE_OF_CONTENTS, bool_serializer, bool_deserializer, bool_validator });
	configs.push_back({ L"should_use_multiple_monitors", &SHOULD_USE_MULTIPLE_MONITORS, bool_serializer, bool_deserializer, bool_validator });
	configs.push_back({ L"should_load_tutorial_when_no_other_file", &SHOULD_LOAD_TUTORIAL_WHEN_NO_OTHER_FILE, bool_serializer, bool_deserializer, bool_validator });
	configs.push_back({ L"should_launch_new_instance", &SHOULD_LAUNCH_NEW_INSTANCE, bool_serializer, bool_deserializer, bool_validator });
	configs.push_back({ L"should_draw_unrendered_pages", &SHOULD_DRAW_UNRENDERED_PAGES, bool_serializer, bool_deserializer, bool_validator });
	configs.push_back({ L"check_for_updates_on_startup", &SHOULD_CHECK_FOR_LATEST_VERSION_ON_STARTUP, bool_serializer, bool_deserializer, bool_validator });
	configs.push_back({ L"sort_bookmarks_by_location", &SORT_BOOKMARKS_BY_LOCATION, bool_serializer, bool_deserializer, bool_validator });
	configs.push_back({ L"shared_database_path", &SHARED_DATABASE_PATH, string_serializer, string_deserializer, nullptr });
	configs.push_back({ L"hover_overview", &HOVER_OVERVIEW, bool_serializer, bool_deserializer, bool_validator });
	configs.push_back({ L"visual_mark_next_page_fraction", &VISUAL_MARK_NEXT_PAGE_FRACTION, float_serializer, float_deserializer, nullptr });
	configs.push_back({ L"visual_mark_next_page_threshold", &VISUAL_MARK_NEXT_PAGE_THRESHOLD, float_serializer, float_deserializer, nullptr });
	configs.push_back({ L"ui_font", &UI_FONT_FACE_NAME, string_serializer, string_deserializer, nullptr });
	configs.push_back({ L"middle_click_search_engine", &MIDDLE_CLICK_SEARCH_ENGINE, string_serializer, string_deserializer, nullptr });
	configs.push_back({ L"shift_middle_click_search_engine", &SHIFT_MIDDLE_CLICK_SEARCH_ENGINE, string_serializer, string_deserializer, nullptr });
	configs.push_back({ L"startup_commands", &STARTUP_COMMANDS, string_serializer, string_deserializer, nullptr });
	configs.push_back({ L"font_size", &FONT_SIZE, int_serializer, int_deserializer, nullptr });
	configs.push_back({ L"custom_background_color", CUSTOM_BACKGROUND_COLOR, vec3_serializer, vec3_deserializer, color_3_validator });
	configs.push_back({ L"custom_text_color", CUSTOM_TEXT_COLOR, vec3_serializer, vec3_deserializer, color_3_validator });
	configs.push_back({ L"rerender_overview", &RERENDER_OVERVIEW, bool_serializer, bool_deserializer, bool_validator });
	configs.push_back({ L"wheel_zoom_on_cursor", &WHEEL_ZOOM_ON_CURSOR, bool_serializer, bool_deserializer, bool_validator });
	configs.push_back({ L"linear_filter", &LINEAR_TEXTURE_FILTERING, bool_serializer, bool_deserializer, bool_validator });
	configs.push_back({ L"display_resolution_scale", &DISPLAY_RESOLUTION_SCALE, float_serializer, float_deserializer, nullptr });
	configs.push_back({ L"status_bar_color", STATUS_BAR_COLOR, vec3_serializer, vec3_deserializer, nullptr });
	configs.push_back({ L"status_bar_text_color", STATUS_BAR_TEXT_COLOR, vec3_serializer, vec3_deserializer, nullptr });
	configs.push_back({ L"main_window_size", &MAIN_WINDOW_SIZE, ivec2_serializer, ivec2_deserializer, nullptr });
	configs.push_back({ L"helper_window_size", &HELPER_WINDOW_SIZE, ivec2_serializer, ivec2_deserializer, nullptr });
	configs.push_back({ L"main_window_move", &MAIN_WINDOW_MOVE, ivec2_serializer, ivec2_deserializer, nullptr });
	configs.push_back({ L"helper_window_move", &HELPER_WINDOW_MOVE, ivec2_serializer, ivec2_deserializer, nullptr });
	configs.push_back({ L"touchpad_sensitivity", &TOUCHPAD_SENSITIVITY, float_serializer, float_deserializer, nullptr });
	configs.push_back({ L"page_separator_width", &PAGE_SEPARATOR_WIDTH, float_serializer, float_deserializer, nullptr });
	configs.push_back({ L"page_separator_color", PAGE_SEPARATOR_COLOR, vec3_serializer, vec3_deserializer, nullptr });
	configs.push_back({ L"single_main_window_size", &SINGLE_MAIN_WINDOW_SIZE, ivec2_serializer, ivec2_deserializer, nullptr });
	configs.push_back({ L"single_main_window_move", &SINGLE_MAIN_WINDOW_MOVE, ivec2_serializer, ivec2_deserializer, nullptr });
	configs.push_back({ L"fit_to_page_width_ratio", &FIT_TO_PAGE_WIDTH_RATIO, float_serializer, float_deserializer, nullptr });
	configs.push_back({ L"collapsed_toc", &SMALL_TOC, bool_serializer, bool_deserializer, bool_validator });
	configs.push_back({ L"ruler_mode", &RULER_MODE, bool_serializer, bool_deserializer, bool_validator });
	configs.push_back({ L"ruler_padding", &RULER_PADDING, float_serializer, float_deserializer, nullptr });
	configs.push_back({ L"text_summary_url", &TEXT_HIGHLIGHT_URL, string_serializer, string_deserializer, nullptr });
	configs.push_back({ L"text_summary_should_refine", &TEXT_SUMMARY_HIGHLIGHT_SHOULD_REFINE, bool_serializer, bool_deserializer, bool_validator });
	configs.push_back({ L"text_summary_should_fill", &TEXT_SUMMARY_HIGHLIGHT_SHOULD_FILL, bool_serializer, bool_deserializer, bool_validator });
	configs.push_back({ L"text_summary_context_size", &TEXT_SUMMARY_CONTEXT_SIZE, int_serializer, int_deserializer, nullptr });
	configs.push_back({ L"use_heuristic_if_text_summary_not_available", &USE_HEURISTIC_IF_TEXT_SUMMARY_NOT_AVAILABLE, bool_serializer, bool_deserializer, bool_validator });
	configs.push_back({ L"papers_folder_path", &PAPERS_FOLDER_PATH, string_serializer, string_deserializer, nullptr });
	configs.push_back({ L"enable_experimental_features", &ENABLE_EXPERIMENTAL_FEATURES, bool_serializer, bool_deserializer, bool_validator });
	configs.push_back({ L"create_table_of_contents_if_not_exists", &CREATE_TABLE_OF_CONTENTS_IF_NOT_EXISTS, bool_serializer, bool_deserializer, bool_validator });
	configs.push_back({ L"max_created_toc_size", &MAX_CREATED_TABLE_OF_CONTENTS_SIZE, int_serializer, int_deserializer, nullptr });
	configs.push_back({ L"force_custom_line_algorithm", &FORCE_CUSTOM_LINE_ALGORITHM, bool_serializer, bool_deserializer, bool_validator });

	std::wstring highlight_config_string = L"highlight_color_a";
	std::wstring search_url_config_string = L"search_url_a";
	std::wstring execute_command_config_string = L"execute_command_a";

	for (char letter = 'a'; letter <= 'z'; letter++) {
		highlight_config_string[highlight_config_string.size() - 1] = letter;
		search_url_config_string[search_url_config_string.size() - 1] = letter;
		execute_command_config_string[execute_command_config_string.size() - 1] = letter;

		configs.push_back({ highlight_config_string, &HIGHLIGHT_COLORS[(letter - 'a') * 3], vec3_serializer, vec3_deserializer });
		configs.push_back({ search_url_config_string, &SEARCH_URLS[letter - 'a'], string_serializer, string_deserializer });
		configs.push_back({ execute_command_config_string, &EXECUTE_COMMANDS[letter - 'a'], string_serializer, string_deserializer });
	}

	deserialize(default_path, auto_path, user_paths);
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

void ConfigManager::deserialize_file(const Path& file_path) {

	std::wstring line;
	std::wifstream default_file = open_wifstream(file_path.get_path());
	int line_number = 0;
	while (std::getline(default_file, line)) {
		line_number++;

		if (line.size() == 0 || line[0] == '#') {
			continue;
		}

		std::wstringstream ss{ line };
		std::wstring conf_name;
		ss >> conf_name;
		Config* conf = get_mut_config_with_name(conf_name);
		if (conf == nullptr) {
			std::wcout << L"Error: " << conf_name << L" is not a valid configuration name\n";
			continue;
		}

		std::wstring config_value;
		std::getline(ss, config_value);

		std::wstringstream config_value_stream(config_value);

		if ((conf != nullptr) && (conf->validator != nullptr)) {
			if (!conf->validator(config_value)) {
				std::wcout << L"Error in config file " << file_path.get_path() << L" at line " << line_number << L" : " << line << L"\n";
				continue;
			}
		}

		if (conf) {
			auto deserialization_result = conf->deserialize(config_value_stream, conf->value);
			if (deserialization_result != nullptr) {
				conf->value = deserialization_result;
			}
			else {
				std::wcout << L"Error in config file " << file_path.get_path() << L" at line " << line_number << L" : " << line << L"\n";
			}
		}

	}
	default_file.close();
}

void ConfigManager::deserialize(const Path& default_file_path, const Path& auto_path ,const std::vector<Path>& user_file_paths) {

	deserialize_file(default_file_path);
	deserialize_file(auto_path);

	for (const auto& user_file_path : user_file_paths) {
		deserialize_file(user_file_path);
	}
}

std::optional<Path> ConfigManager::get_or_create_user_config_file() {
	if (user_config_paths.size() == 0) {
		return {};
	}

	for (int i = user_config_paths.size() - 1; i >= 0; i--) {
		if (user_config_paths[i].file_exists() ) {
			return user_config_paths[i];
		}
	}
	user_config_paths.back().file_parent().create_directories();
	create_file_if_not_exists(user_config_paths.back().get_path());
	return user_config_paths.back();
}

std::vector<Path> ConfigManager::get_all_user_config_files(){
	std::vector<Path> res;
	for (int i = user_config_paths.size() - 1; i >= 0; i--) {
		if (user_config_paths[i].file_exists()) {
			res.push_back(user_config_paths[i]);
		}
	}
	return  res;
}
