#pragma once

#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include <mupdf/fitz.h>

#include "book.h"
#include "utf8.h"

using namespace std;

wstring to_lower(const wstring& inp);
void convert_toc_tree(fz_outline* root, vector<TocNode*>& output);
void get_flat_toc(const vector<TocNode*>& roots, vector<wstring>& output, vector<int>& pages);
int mod(int a, int b);
bool intersects(float range1_start, float range1_end, float range2_start, float range2_end);
void parse_uri(string uri, int* page, float* offset_x, float* offset_y);
bool includes_rect(fz_rect includer, fz_rect includee);
char get_symbol(int scancode, bool is_shift_pressed);
//GLuint LoadShaders(filesystem::path vertex_file_path_, filesystem::path fragment_file_path_);

template<typename T>
int argminf(const vector<T> &collection, function<float(T)> f) {

	float min = std::numeric_limits<float>::infinity();
	int min_index = -1;
	for (int i = 0; i < collection.size(); i++) {
		float element_value = f(collection[i]);
		if (element_value < min){
			min = element_value;
			min_index = i;
		}
	}
	return min_index;
}
void rect_to_quad(fz_rect rect, float quad[8]);
bool should_select_char(fz_point selection_begin,fz_point selection_end , fz_rect character_rect);
void copy_to_clipboard(const wstring& text);
fz_rect corners_to_rect(fz_point corner1, fz_point corner2);
void install_app(char* argv0);
int get_f_key(string name);
void show_error_message(wstring error_message);
wstring utf8_decode(string encoded_str);
string utf8_encode(wstring decoded_str);
bool is_rtl(int c);
wstring reverse_wstring(const wstring& inp);
