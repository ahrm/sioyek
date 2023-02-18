#pragma once

#include <fstream>
#include <functional>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <regex>
#include <optional>
#include <memory>
#include <qcommandlineparser.h>

#include <qstandarditemmodel.h>
#include <qpoint.h>

#include <mupdf/fitz.h>

#include "book.h"
#include "utf8.h"
#include "coordinates.h"

#define LL_ITER(name, start) for(auto name=start;(name);name=name->next)

struct ParsedUri {
	int page;
	float x;
	float y;
};

struct CharacterAddress {
	int page;
	fz_stext_block* block;
	fz_stext_line* line;
	fz_stext_char* character;
	Document* doc;

	CharacterAddress* previous_character = nullptr;

	bool advance(char c);
	bool backspace();
	bool next_char();
	bool next_line();
	bool next_block();
	bool next_page();

	float focus_offset();

};

std::wstring to_lower(const std::wstring& inp);
bool is_separator(fz_stext_char* last_char, fz_stext_char* current_char);
void get_flat_toc(const std::vector<TocNode*>& roots, std::vector<std::wstring>& output, std::vector<int>& pages);
int mod(int a, int b);
bool range_intersects(float range1_start, float range1_end, float range2_start, float range2_end);
bool rects_intersect(fz_rect rect1, fz_rect rect2);
ParsedUri parse_uri(fz_context* mupdf_context, std::string uri);
char get_symbol(int key, bool is_shift_pressed, const std::vector<char>&special_symbols);

template<typename T>
int argminf(const std::vector<T> &collection, std::function<float(T)> f) {

	float min = std::numeric_limits<float>::infinity();
	int min_index = -1;
	for (size_t i = 0; i < collection.size(); i++) {
		float element_value = f(collection[i]);
		if (element_value < min){
			min = element_value;
			min_index = i;
		}
	}
	return min_index;
}
void rect_to_quad(fz_rect rect, float quad[8]);
void copy_to_clipboard(const std::wstring& text, bool selection=false);
void install_app(const char* argv0);
int get_f_key(std::wstring name);
void show_error_message(const std::wstring& error_message);
std::wstring utf8_decode(const std::string& encoded_str);
std::string utf8_encode(const std::wstring& decoded_str);
// is the character a right to left character
bool is_rtl(int c);
std::wstring reverse_wstring(const std::wstring& inp);
bool parse_search_command(const std::wstring& search_command, int* out_begin, int* out_end, std::wstring* search_text);
QStandardItemModel* get_model_from_toc(const std::vector<TocNode*>& roots);

// given a tree of toc nodes and an array of indices, returns the node whose ith parent is indexed by the ith element
// of the indices array. That is:
// root[indices[0]][indices[1]] ... [indices[indices.size()-1]]
TocNode* get_toc_node_from_indices(const std::vector<TocNode*>& roots, const std::vector<int>& indices);

fz_stext_char* find_closest_char_to_document_point(const std::vector<fz_stext_char*> flat_chars, fz_point document_point, int* location_index);
void merge_selected_character_rects(const std::vector<fz_rect>& selected_character_rects, std::vector<fz_rect>& resulting_rects);
void split_key_string(std::wstring haystack, const std::wstring& needle, std::vector<std::wstring>& res);
void run_command(std::wstring command, QStringList parameters, bool wait=true);

std::wstring get_string_from_stext_line(fz_stext_line* line);
void sleep_ms(unsigned int ms);
//void open_url(const std::string& url_string);
//void open_url(const std::wstring& url_string);

void open_file_url(const std::wstring& file_url);
void open_web_url(const std::wstring& web_url);

void open_file(const std::wstring& path);
void search_custom_engine(const std::wstring& search_string, const std::wstring& custom_engine_url);
void search_google_scholar(const std::wstring& search_string);
void search_libgen(const std::wstring& search_string);
void index_references(fz_stext_page* page, int page_number, std::map<std::wstring, IndexedData>& indices);
void get_flat_chars_from_stext_page(fz_stext_page* stext_page, std::vector<fz_stext_char*>& flat_chars);
int find_best_vertical_line_location(fz_pixmap* pixmap, int relative_click_x, int relative_click_y);
//void get_flat_chars_from_stext_page_with_space(fz_stext_page* stext_page, std::vector<fz_stext_char*>& flat_chars, fz_stext_char* space);
void index_equations(const std::vector<fz_stext_char*>& flat_chars, int page_number, std::map<std::wstring, std::vector<IndexedData>>& indices);
void find_regex_matches_in_stext_page(const std::vector<fz_stext_char*>& flat_chars,
	const std::wregex& regex,
	std::vector<std::pair<int, int>>& match_ranges, std::vector<std::wstring>& match_texts);
bool is_string_numeric(const std::wstring& str);
bool is_string_numeric_float(const std::wstring& str);
void create_file_if_not_exists(const std::wstring& path);
QByteArray serialize_string_array(const QStringList& string_list);
QStringList deserialize_string_array(const QByteArray& byte_array);
//Path add_redundant_dot_to_path(const Path& sane_path);
bool should_reuse_instance(int argc, char** argv);
bool should_new_instance(int argc, char** argv);
QCommandLineParser* get_command_line_parser();
std::wstring concatenate_path(const std::wstring& prefix, const std::wstring& suffix);
std::wstring get_canonical_path(const std::wstring& path);
void split_path(std::wstring path, std::vector<std::wstring>& res);
//std::wstring canonicalize_path(const std::wstring& path);
std::wstring add_redundant_dot_to_path(const std::wstring& path);
float manhattan_distance(float x1, float y1, float x2, float y2);
float manhattan_distance(fvec2 v1, fvec2 v2);
QWidget* get_top_level_widget(QWidget* widget);
std::wstring strip_string(std::wstring& input_string);
//void index_generic(const std::vector<fz_stext_char*>& flat_chars, int page_number, std::vector<IndexedData>& indices);
void index_generic(const std::vector<fz_stext_char*>& flat_chars, int page_number, std::vector<IndexedData>& indices);
std::vector<std::wstring> split_whitespace(std::wstring const& input);
float type_name_similarity_score(std::wstring name1, std::wstring name2);
bool is_stext_page_rtl(fz_stext_page* stext_page);
void check_for_updates(QWidget* parent, std::string current_version);
char* get_argv_value(int argc, char** argv, std::string key);
void split_root_file(QString path, QString& out_root, QString& out_partial);
QString expand_home_dir(QString path);
std::vector<unsigned int> get_max_width_histogram_from_pixmap(fz_pixmap* pixmap);
//std::vector<unsigned int> get_line_ends_from_histogram(std::vector<unsigned int> histogram);
void get_line_begins_and_ends_from_histogram(std::vector<unsigned int> histogram, std::vector<unsigned int>& begins, std::vector<unsigned int>& ends);

template<typename T>
int find_nth_larger_element_in_sorted_list(std::vector<T> sorted_list, T value, int n) {
	int i = 0;
	while (i < sorted_list.size() && (value >= sorted_list[i])) i++;
	if ((i < sorted_list.size()) && (sorted_list[i] == value)) i--;
	if ((i + n - 1) < sorted_list.size()) {
		return i + n - 1;
	}
	else {
		return -1;
	}

}

QString get_color_qml_string(float r, float g, float b);
void copy_file(std::wstring src_path, std::wstring dst_path);
fz_quad quad_from_rect(fz_rect r);
std::vector<fz_quad> quads_from_rects(const std::vector<fz_rect>& rects);
std::wifstream open_wifstream(const std::wstring& file_name);
void get_flat_words_from_flat_chars(const std::vector<fz_stext_char*>& flat_chars, std::vector<fz_rect>& flat_word_rects, std::vector<std::vector<fz_rect>>* out_char_rects = nullptr);
void get_word_rect_list_from_flat_chars(const std::vector<fz_stext_char*>& flat_chars,
	std::vector<std::wstring>& words,
	std::vector<std::vector<fz_rect>>& flat_word_rects);

std::vector<std::string> get_tags(int n);
int get_index_from_tag(const std::string& tag);
std::wstring truncate_string(const std::wstring& inp, int size);
std::wstring get_page_formatted_string(int page);
fz_rect create_word_rect(const std::vector<fz_rect>& chars);
std::vector<fz_rect> create_word_rects_multiline(const std::vector<fz_rect>& chars);
void get_flat_chars_from_block(fz_stext_block* block, std::vector<fz_stext_char*>& flat_chars);
void get_text_from_flat_chars(const std::vector<fz_stext_char*>& flat_chars, std::wstring& string_res, std::vector<int>& indices);
bool is_string_titlish(const std::wstring& str);
bool is_title_parent_of(const std::wstring& parent_title, const std::wstring& child_title, bool* are_same);
std::wstring find_first_regex_match(const std::wstring& haystack, const std::wstring& regex_string);
void merge_lines(const std::vector<fz_stext_line*>& lines, std::vector<fz_rect>& out_rects, std::vector<std::wstring>& out_texts);
float get_max_display_scaling();
int lcs(const char* X, const char* Y, int m, int n);
bool has_arg(int argc, char** argv, std::string key);
std::vector<std::wstring> find_all_regex_matches(const std::wstring& haystack, const std::wstring& regex_string);
bool command_requires_text(const std::wstring& command);
bool command_requires_rect(const std::wstring& command);
void hexademical_to_normalized_color(std::wstring color_string, float* color, int n_components);
void parse_command_string(std::wstring command_string, std::string& command_name, std::wstring& command_data);
void parse_color(std::wstring color_string, float* out_color, int n_components);
int get_status_bar_height();
void flat_char_prism(const std::vector<fz_stext_char*> chars, int page, std::wstring& output_text, std::vector<int>& pages, std::vector<fz_rect>& rects);
QString get_status_stylesheet(bool nofont=false);
QString get_selected_stylesheet(bool nofont=false);

template<int d1, int d2, int d3>
void matmul(float m1[], float m2[], float result[]) {
	for (int i = 0; i < d1; i++) {
		for (int j = 0; j < d3; j++) {
			result[i * d3 + j] = 0;
			for (int k = 0; k < d2; k++) {
				result[i * d3 + j] += m1[i * d2 + k] * m2[k * d3 + j];
			}
		}
	}
}

void convert_color4(float* in_color, int* out_color);
std::string get_aplph_tag(int n, int max_n);
