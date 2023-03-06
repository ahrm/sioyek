//#include <Windows.h>
#include <cwctype>

#include <cmath>
#include <cassert>
#include "utils.h"
#include <optional>
#include <cstring>
#include <sstream>
#include <string>
#include <qclipboard.h>
#include <qguiapplication.h>
#include <qprocess.h>
#include <qdesktopservices.h>
#include <qurl.h>
#include <qmessagebox.h>
#include <qbytearray.h>
#include <qdatastream.h>
#include <qstringlist.h>
#include <qcommandlineparser.h>
#include <qdir.h>
#include <qurl.h>
#include <qnetworkaccessmanager.h>
#include <qnetworkrequest.h>
#include <qnetworkreply.h>
#include <qscreen.h>

#include <mupdf/pdf.h>

extern std::wstring LIBGEN_ADDRESS;
extern std::wstring GOOGLE_SCHOLAR_ADDRESS;
extern std::ofstream LOG_FILE;
extern int STATUS_BAR_FONT_SIZE;
extern float STATUS_BAR_COLOR[3];
extern float STATUS_BAR_TEXT_COLOR[3];
extern float UI_SELECTED_TEXT_COLOR[3];
extern float UI_SELECTED_BACKGROUND_COLOR[3];
extern bool NUMERIC_TAGS;

#ifdef Q_OS_WIN
#include <windows.h>
#endif


std::wstring to_lower(const std::wstring& inp) {
	std::wstring res;
	for (char c : inp) {
		res.push_back(::tolower(c));
	}
	return res;
}

void get_flat_toc(const std::vector<TocNode*>& roots, std::vector<std::wstring>& output, std::vector<int>& pages) {
	// Enumerate ToC nodes in DFS order

	for (const auto& root : roots) {
		output.push_back(root->title);
		pages.push_back(root->page);
		get_flat_toc(root->children, output, pages);
	}
}

TocNode* get_toc_node_from_indices_helper(const std::vector<TocNode*>& roots, const std::vector<int>& indices, int pointer) {
	assert(pointer >= 0);

	if (pointer == 0) {
		return roots[indices[pointer]];
	}

	return get_toc_node_from_indices_helper(roots[indices[pointer]]->children, indices, pointer - 1);
}

TocNode* get_toc_node_from_indices(const std::vector<TocNode*>& roots, const std::vector<int>& indices) {
	// Get a table of contents item by recursively indexing using `indices`
	return get_toc_node_from_indices_helper(roots, indices, indices.size() - 1);
}


QStandardItem* get_item_tree_from_toc_helper(const std::vector<TocNode*>& children, QStandardItem* parent) {

	for (const auto* child : children) {
		QStandardItem* child_item = new QStandardItem(QString::fromStdWString(child->title));
		QStandardItem* page_item = new QStandardItem("[ " + QString::number(child->page) + " ]");
		child_item->setData(child->page);
		page_item->setTextAlignment(Qt::AlignVCenter | Qt::AlignRight);

		get_item_tree_from_toc_helper(child->children, child_item);
		parent->appendRow(QList<QStandardItem*>() << child_item << page_item);
	}
	return parent;
}


QStandardItemModel* get_model_from_toc(const std::vector<TocNode*>& roots) {

	QStandardItemModel* model = new QStandardItemModel();
	get_item_tree_from_toc_helper(roots, model->invisibleRootItem());
	return model;
}


int mod(int a, int b)
{
	// compute a mod b handling negative numbers "correctly"
	return (a % b + b) % b;
}

bool range_intersects(float range1_start, float range1_end, float range2_start, float range2_end) {
	if (range2_start > range1_end || range1_start > range2_end) {
		return false;
	}
	return true;
}

bool rects_intersect(fz_rect rect1, fz_rect rect2) {
	return range_intersects(rect1.x0, rect1.x1, rect2.x0, rect2.x1) && range_intersects(rect1.y0, rect1.y1, rect2.y0, rect2.y1);
}

ParsedUri parse_uri(fz_context* mupdf_context, std::string uri) {
	fz_link_dest dest = pdf_parse_link_uri(mupdf_context, uri.c_str());
	return { dest.loc.page + 1, dest.x, dest.y };
}

char get_symbol(int key, bool is_shift_pressed, const std::vector<char>& special_symbols) {

	if (key >= 'A' && key <= 'Z') {
		if (is_shift_pressed) {
			return key;
		}
		else {
			return key + 'a' - 'A';
		}
	}

	if (key >= '0' && key <= '9') {
		return key;
	}

	if (std::find(special_symbols.begin(), special_symbols.end(), key) != special_symbols.end()) {
		return key;
	}

	return 0;
}

void rect_to_quad(fz_rect rect, float quad[8]) {
	quad[0] = rect.x0;
	quad[1] = rect.y0;
	quad[2] = rect.x1;
	quad[3] = rect.y0;
	quad[4] = rect.x0;
	quad[5] = rect.y1;
	quad[6] = rect.x1;
	quad[7] = rect.y1;
}

void copy_to_clipboard(const std::wstring& text, bool selection) {
	auto clipboard = QGuiApplication::clipboard();
	auto qtext = QString::fromStdWString(text);
	if (!selection) {
		clipboard->setText(qtext);
	}
	else {
		clipboard->setText(qtext, QClipboard::Selection);
	}
}

#define OPEN_KEY(parent, name, ptr) \
	RegCreateKeyExA(parent, name, 0, 0, 0, KEY_WRITE, 0, &ptr, 0)

#define SET_KEY(parent, name, value) \
	RegSetValueExA(parent, name, 0, REG_SZ, (const BYTE *)(value), (DWORD)strlen(value) + 1)

void install_app(const char *argv0)
{
#ifdef Q_OS_WIN
	char buf[512];
	HKEY software, classes, testpdf, dotpdf;
	HKEY shell, open, command, supported_types;
	HKEY pdf_progids;

	OPEN_KEY(HKEY_CURRENT_USER, "Software", software);
	OPEN_KEY(software, "Classes", classes);
	OPEN_KEY(classes, ".pdf", dotpdf);
	OPEN_KEY(dotpdf, "OpenWithProgids", pdf_progids);
	OPEN_KEY(classes, "Sioyek", testpdf);
	OPEN_KEY(testpdf, "SupportedTypes", supported_types);
	OPEN_KEY(testpdf, "shell", shell);
	OPEN_KEY(shell, "open", open);
	OPEN_KEY(open, "command", command);

	sprintf(buf, "\"%s\" \"%%1\"", argv0);

	SET_KEY(open, "FriendlyAppName", "Sioyek");
	SET_KEY(command, "", buf);
	SET_KEY(supported_types, ".pdf", "");
	SET_KEY(pdf_progids, "sioyek", "");

	RegCloseKey(dotpdf);
	RegCloseKey(testpdf);
	RegCloseKey(classes);
	RegCloseKey(software);
#endif
}

int get_f_key(std::wstring name) {
	if (name[0] == '<') {
		name = name.substr(1, name.size() - 2);
	}
	if (name[0] != 'f') {
		return 0;
	}
	name = name.substr(1, name.size() - 1);
	if (!isdigit(name[0])) {
		return 0;
	}

	int num;
	std::wstringstream ss(name);
	ss >> num;
	return  num;
}

void show_error_message(const std::wstring& error_message) {
	QMessageBox msgBox;
	msgBox.setText(QString::fromStdWString(error_message));
	msgBox.setStandardButtons(QMessageBox::Ok);
	msgBox.setDefaultButton(QMessageBox::Ok);
	msgBox.exec();
}

std::wstring utf8_decode(const std::string& encoded_str) {
	std::wstring res;
	utf8::utf8to32(encoded_str.begin(), encoded_str.end(), std::back_inserter(res));
	return res;
}

std::string utf8_encode(const std::wstring& decoded_str) {
	std::string res;
	utf8::utf32to8(decoded_str.begin(), decoded_str.end(), std::back_inserter(res));
	return res;
}

bool is_rtl(int c){
  if (
    (c==0x05BE)||(c==0x05C0)||(c==0x05C3)||(c==0x05C6)||
    ((c>=0x05D0)&&(c<=0x05F4))||
    (c==0x0608)||(c==0x060B)||(c==0x060D)||
    ((c>=0x061B)&&(c<=0x064A))||
    ((c>=0x066D)&&(c<=0x066F))||
    ((c>=0x0671)&&(c<=0x06D5))||
    ((c>=0x06E5)&&(c<=0x06E6))||
    ((c>=0x06EE)&&(c<=0x06EF))||
    ((c>=0x06FA)&&(c<=0x0710))||
    ((c>=0x0712)&&(c<=0x072F))||
    ((c>=0x074D)&&(c<=0x07A5))||
    ((c>=0x07B1)&&(c<=0x07EA))||
    ((c>=0x07F4)&&(c<=0x07F5))||
    ((c>=0x07FA)&&(c<=0x0815))||
    (c==0x081A)||(c==0x0824)||(c==0x0828)||
    ((c>=0x0830)&&(c<=0x0858))||
    ((c>=0x085E)&&(c<=0x08AC))||
    (c==0x200F)||(c==0xFB1D)||
    ((c>=0xFB1F)&&(c<=0xFB28))||
    ((c>=0xFB2A)&&(c<=0xFD3D))||
    ((c>=0xFD50)&&(c<=0xFDFC))||
    ((c>=0xFE70)&&(c<=0xFEFC))||
    ((c>=0x10800)&&(c<=0x1091B))||
    ((c>=0x10920)&&(c<=0x10A00))||
    ((c>=0x10A10)&&(c<=0x10A33))||
    ((c>=0x10A40)&&(c<=0x10B35))||
    ((c>=0x10B40)&&(c<=0x10C48))||
    ((c>=0x1EE00)&&(c<=0x1EEBB))
  ) return true;
  return false;
}

std::wstring reverse_wstring(const std::wstring& inp) {
	std::wstring res;
	for (int i = inp.size() - 1; i >= 0; i--) {
		res.push_back(inp[i]);
	}
	return res;
}

bool parse_search_command(const std::wstring& search_command, int* out_begin, int* out_end, std::wstring* search_text) {
	std::wstringstream ss(search_command);
	if (search_command[0] == '<') {
		wchar_t dummy;
		ss >> dummy;
		ss >> *out_begin;
		ss >> dummy;
		ss >> *out_end;
		ss >> dummy;
		std::getline(ss, *search_text);
		return true;
	}
	else {
		*search_text = ss.str();
		return false;
	}
}

float dist_squared(fz_point p1, fz_point p2) {
	return (p1.x - p2.x) * (p1.x - p2.x) + 100 * (p1.y - p2.y) * (p1.y - p2.y);
}

bool is_stext_line_rtl(fz_stext_line* line) {

	float rtl_count = 0.0f;
	float total_count = 0.0f;
	LL_ITER(ch, line->first_char) {
		if (is_rtl(ch->c)) {
			rtl_count += 1.0f;
		}
		total_count += 1.0f;
	}
	return ((rtl_count / total_count) > 0.5f);
}
bool is_stext_page_rtl(fz_stext_page* stext_page) {

	float rtl_count = 0.0f;
	float total_count = 0.0f;

	LL_ITER(block, stext_page->first_block) {
		if (block->type == FZ_STEXT_BLOCK_TEXT) {
			LL_ITER(line, block->u.t.first_line) {
				LL_ITER(ch, line->first_char) {
					if (is_rtl(ch->c)) {
						rtl_count += 1.0f;
					}
					total_count += 1.0f;
				}
			}
		}
	}
	return ((rtl_count / total_count) > 0.5f);
}

std::vector<fz_stext_char*> reorder_stext_line(fz_stext_line* line) {

	std::vector<fz_stext_char*> reordered_chars;

	bool rtl = is_stext_line_rtl(line);

	LL_ITER(ch, line->first_char) {
		reordered_chars.push_back(ch);
	}

	if (rtl) {
		std::sort(reordered_chars.begin(), reordered_chars.end(), [](fz_stext_char* lhs, fz_stext_char* rhs) {
			return lhs->quad.ll.x > rhs->quad.ll.x;
			});
	}
	else {
		std::sort(reordered_chars.begin(), reordered_chars.end(), [](fz_stext_char* lhs, fz_stext_char* rhs) {
			return lhs->quad.ll.x < rhs->quad.ll.x;
			});
	}
	return reordered_chars;
}

void get_flat_chars_from_block(fz_stext_block* block, std::vector<fz_stext_char*>& flat_chars) {
	if (block->type == FZ_STEXT_BLOCK_TEXT) {
		LL_ITER(line, block->u.t.first_line) {
			std::vector<fz_stext_char*> reordered_chars = reorder_stext_line(line);
			for (auto ch : reordered_chars) {
				flat_chars.push_back(ch);
			}
		}
	}
}

void get_flat_chars_from_stext_page(fz_stext_page* stext_page, std::vector<fz_stext_char*>& flat_chars) {

	LL_ITER(block, stext_page->first_block) {
		get_flat_chars_from_block(block, flat_chars);
	}
}

bool is_delimeter(int c) {
	std::vector<char> delimeters = {' ', '\n', ';', ','};
	return std::find(delimeters.begin(), delimeters.end(), c) != delimeters.end();
}

float get_character_height(fz_stext_char* c) {
	return std::abs(c->quad.ul.y - c->quad.ll.y);
}

float get_character_width(fz_stext_char* c) {
	return std::abs(c->quad.ul.x - c->quad.ur.x);
}

bool is_start_of_new_line(fz_stext_char* prev_char, fz_stext_char* current_char) {
	float height = get_character_height(current_char);
	float threshold = height / 2;
	if (std::abs(prev_char->quad.ll.y - current_char->quad.ll.y) > threshold) {
		return true;
	}
	return false;
}
bool is_start_of_new_word(fz_stext_char* prev_char, fz_stext_char* current_char) {
	if (is_delimeter(prev_char->c)) {
		return true;
	}

	return is_start_of_new_line(prev_char, current_char);
}

fz_rect create_word_rect(const std::vector<fz_rect>& chars) {
	fz_rect res;
	res.x0 = res.x1 = res.y0 = res.y1 = 0;
	if (chars.size() == 0) return res;
	res = chars[0];

	for (size_t i = 1; i < chars.size(); i++) {
		if (res.x0 > chars[i].x0) res.x0 = chars[i].x0;
		if (res.x1 < chars[i].x1) res.x1 = chars[i].x1;
		if (res.y0 > chars[i].y0) res.y0 = chars[i].y0;
		if (res.y1 < chars[i].y1) res.y1 = chars[i].y1;
	}

	return res;
}

std::vector<fz_rect> create_word_rects_multiline(const std::vector<fz_rect>& chars) {
	std::vector<fz_rect> res;
	std::vector<fz_rect> current_line_chars;

	if (chars.size() == 0) return res;
	current_line_chars.push_back(chars[0]);

	for (size_t i = 1; i < chars.size(); i++) {
		if (chars[i].x0 < chars[i - 1].x0) { // a new line has begun
			res.push_back(create_word_rect(current_line_chars));
			current_line_chars.clear();
			current_line_chars.push_back(chars[i]);
		}
		else {
			current_line_chars.push_back(chars[i]);
		}
	}
	if (current_line_chars.size() > 0) {
		res.push_back(create_word_rect(current_line_chars));
	}
	return res;
}

fz_rect create_word_rect(const std::vector<fz_stext_char*>& chars) {
	fz_rect res;
	res.x0 = res.x1 = res.y0 = res.y1 = 0;
	if (chars.size() == 0) return res;
	res = fz_rect_from_quad(chars[0]->quad);

	for (size_t i = 1; i < chars.size(); i++) {
		fz_rect current_char_rect = fz_rect_from_quad(chars[i]->quad);
		if (res.x0 > current_char_rect.x0) res.x0 = current_char_rect.x0;
		if (res.x1 < current_char_rect.x1) res.x1 = current_char_rect.x1;
		if (res.y0 > current_char_rect.y0) res.y0 = current_char_rect.y0;
		if (res.y1 < current_char_rect.y1) res.y1 = current_char_rect.y1;
	}

	return res;
}

void get_flat_words_from_flat_chars(const std::vector<fz_stext_char*>& flat_chars, std::vector<fz_rect>& flat_word_rects,  std::vector<std::vector<fz_rect>>* out_char_rects) {

	if (flat_chars.size() == 0) return;

	std::vector<std::wstring> res;
	std::vector<fz_stext_char*> pending_word;
	pending_word.push_back(flat_chars[0]);

	for (size_t i = 1; i < flat_chars.size(); i++) {
		if (is_start_of_new_word(flat_chars[i - 1], flat_chars[i])) {
			flat_word_rects.push_back(create_word_rect(pending_word));
			if (out_char_rects != nullptr) {
				std::vector<fz_rect> chars;
				for (auto c : pending_word) {
					chars.push_back(fz_rect_from_quad(c->quad));
				}
				out_char_rects->push_back(chars);
			}
			if (is_start_of_new_line(flat_chars[i - 1], flat_chars[i])) {
				fz_rect new_rect = fz_rect_from_quad(flat_chars[i - 1]->quad);

				new_rect.x0 = (new_rect.x0 + 2 * new_rect.x1) / 3;
				flat_word_rects.push_back(new_rect);
				if (out_char_rects != nullptr) {
					out_char_rects->push_back({ new_rect });
				}
			}
			pending_word.clear();
			pending_word.push_back(flat_chars[i]);
		}
		else {
			pending_word.push_back(flat_chars[i]);
		}
	}
}

void get_word_rect_list_from_flat_chars(const std::vector<fz_stext_char*>& flat_chars,
	std::vector<std::wstring>& words,
	std::vector<std::vector<fz_rect>>& flat_word_rects) {

	if (flat_chars.size() == 0) return;

	std::vector<fz_stext_char*> pending_word;
	pending_word.push_back(flat_chars[0]);

	auto get_rects = [&]() {
		std::vector<fz_rect> res;
		for (auto chr : pending_word) {
			res.push_back(fz_rect_from_quad(chr->quad));
		}
		return res;
	};

	auto get_word = [&]() {
		std::wstring res;
		for (auto chr : pending_word) {
			res.push_back(chr->c);
		}
		return res;
	};

	for (size_t i = 1; i < flat_chars.size(); i++) {
		if (is_start_of_new_word(flat_chars[i - 1], flat_chars[i])) {
			flat_word_rects.push_back(get_rects());
			words.push_back(get_word());

			pending_word.clear();
			pending_word.push_back(flat_chars[i]);
		}
		else {
			pending_word.push_back(flat_chars[i]);
		}
	}
}

int get_num_tag_digits(int n) {
	int res = 1;
	while (n > 26) {
		n = n / 26;
		res++;
	}
	return res;
}

std::string get_aplph_tag(int n, int max_n) {
		int n_digits = get_num_tag_digits(max_n);
		std::string tag;
		for (int i = 0; i < n_digits; i++) {
			tag.push_back('a' + (n % 26));
			n = n / 26;
		}
		return tag;
}

std::vector<std::string> get_tags(int n) {
	std::vector<std::string> res;
	int n_digits = get_num_tag_digits(n);
	for (int i = 0; i < n; i++) {
		int current_n = i;
		std::string tag;
		if (!NUMERIC_TAGS) {
			for (int i = 0; i < n_digits; i++) {
				tag.push_back('a' + (current_n % 26));
				current_n = current_n / 26;
			}
		}
		else{
			tag = std::to_string(i);
		}
		res.push_back(tag);
	}
	return res;
}

int get_index_from_tag(const std::string& tag) {
	int res = 0;
	int mult = 1;

	if (!NUMERIC_TAGS) {
		for (size_t i = 0; i < tag.size(); i++) {
			res += (tag[i] - 'a') * mult;
			mult = mult * 26;
		}
	}
	else {
		res = std::stoi(tag);
	}
	return res;
}

fz_stext_char* find_closest_char_to_document_point(const std::vector<fz_stext_char*> flat_chars, fz_point document_point, int* location_index) {
	float min_distance = std::numeric_limits<float>::infinity();
	fz_stext_char* res = nullptr;

	int index = 0;
	for (auto current_char : flat_chars) {

		fz_point quad_center = current_char->origin;
		float distance = dist_squared(document_point, quad_center);
		if (distance < min_distance) {
			min_distance = distance;
			res = current_char;
			*location_index = index;
		}
		index++;
	}

	return res;
}

bool is_line_separator(fz_stext_char* last_char, fz_stext_char* current_char) {
	if (last_char == nullptr) {
		return false;
	}
	float dist = abs(last_char->quad.ll.y - current_char->quad.ll.y);
	if (dist > 1.0f) {
		return true;
	}
	return false;
}

bool is_separator(fz_stext_char* last_char, fz_stext_char* current_char) {
	if (last_char == nullptr) {
		return false;
	}

	if (current_char->c == ' ') {
		return true;
	}
	float dist = abs(last_char->quad.ll.y - current_char->quad.ll.y);
	if (dist > 1.0f) {
		return true;
	}
	return false;
}

std::wstring get_string_from_stext_line(fz_stext_line* line) {

	std::wstring res;
	LL_ITER(ch, line->first_char) {
		res.push_back(ch->c);
	}
	return res;
}

bool is_consequtive(fz_rect rect1, fz_rect rect2) {

	float xdist = abs(rect1.x1 - rect2.x1);
	float ydist1 = abs(rect1.y0 - rect2.y0);
	float ydist2 = abs(rect1.y1 - rect2.y1);
	float ydist = std::min(ydist1, ydist2);

	float rect1_width = rect1.x1 - rect1.x0;
	float rect2_width = rect2.x1 - rect2.x0;
	float average_width = (rect1_width + rect2_width) / 2.0f;

	float rect1_height = rect1.y1 - rect1.y0;
	float rect2_height = rect2.y1 - rect2.y0;
	float average_height = (rect1_height + rect2_height) / 2.0f;

	if (xdist < 3*average_width && ydist < 2*average_height) {
		return true;
	}

	return false;

}
fz_rect bound_rects(const std::vector<fz_rect>& rects) {
	// find the bounding box of some rects

	fz_rect res = rects[0];

	float average_y0 = 0.0f;
	float average_y1 = 0.0f;

	for (auto rect : rects) {
		if (res.x1 < rect.x1) {
			res.x1 = rect.x1;
		}
		if (res.x0 > rect.x0) {
			res.x0 = rect.x0;
		}

		average_y0 += rect.y0;
		average_y1 += rect.y1;
	}

	average_y0 /= rects.size();
	average_y1 /= rects.size();

	res.y0 = average_y0;
	res.y1 = average_y1;

	return res;

}
void merge_selected_character_rects(const std::vector<fz_rect>& selected_character_rects, std::vector<fz_rect>& resulting_rects) {
	/*
		This function merges the bounding boxes of all selected characters into large line chunks.
	*/

	if (selected_character_rects.size() == 0) {
		return;
	}

	std::vector<fz_rect> line_rects;

	fz_rect last_rect = selected_character_rects[0];
	line_rects.push_back(selected_character_rects[0]);

	for (size_t i = 1; i < selected_character_rects.size(); i++) {
		if (is_consequtive(last_rect, selected_character_rects[i])) {
			last_rect = selected_character_rects[i];
			line_rects.push_back(selected_character_rects[i]);
		}
		else {
			fz_rect bounding_rect = bound_rects(line_rects);
			resulting_rects.push_back(bounding_rect);
			line_rects.clear();
			last_rect = selected_character_rects[i];
			line_rects.push_back(selected_character_rects[i]);
		}
	}

	if (line_rects.size() > 0) {
		fz_rect bounding_rect = bound_rects(line_rects);
		resulting_rects.push_back(bounding_rect);
	}

	// avoid overlapping rects
	for (size_t i = 0; i < resulting_rects.size() - 1; i++) {
		// we don't need to do this across columns of document
		float height = std::abs(resulting_rects[i].y1 - resulting_rects[i].y0);
		if (std::abs(resulting_rects[i + 1].y0 - resulting_rects[i].y0) < (0.5 * height)) {
			continue;
		}
		if ((resulting_rects[i + 1].x0 < resulting_rects[i].x1) ) {
			resulting_rects[i + 1].y0 = resulting_rects[i].y1;
		}
	}

}

int next_path_separator_position(const std::wstring& path) {
	wchar_t sep1 = '/';
	wchar_t sep2 = '\\';
	int index1 = path.find(sep1);
	int index2 = path.find(sep2);
	if (index2 == -1) {
		return index1;
	}

	if (index1 == -1) {
		return index2;
	}
	return std::min(index1, index2);
}

void split_path(std::wstring path, std::vector<std::wstring> &res) {

	size_t loc = -1;
        // overflows
	while ((loc = next_path_separator_position(path)) != (size_t) -1) {

		int skiplen = loc + 1;
		if (loc != 0) {
			std::wstring part = path.substr(0, loc);
			res.push_back(part);
		}
		path = path.substr(skiplen, path.size() - skiplen);
	}
	if (path.size() > 0) {
		res.push_back(path);
	}
}


std::vector<std::wstring> split_whitespace(std::wstring const& input) {
	std::wistringstream buffer(input);
	std::vector<std::wstring> ret((std::istream_iterator<std::wstring, wchar_t>(buffer)),
		std::istream_iterator<std::wstring, wchar_t>());
	return ret;
}

void split_key_string(std::wstring haystack, const std::wstring& needle, std::vector<std::wstring> &res) {
	//todo: we can significantly reduce string allocations in this function if it turns out to be a
	//performance bottleneck.

	if (haystack == needle){
		res.push_back(L"-");
		return;
	}

	size_t loc = -1;
	size_t needle_size = needle.size();
	while ((loc = haystack.find(needle)) != (size_t) -1) {


		int skiplen = loc + needle_size;
		if (loc != 0) {
			std::wstring part = haystack.substr(0, loc);
			res.push_back(part);
		}
		if ((loc < (haystack.size()-1)) &&  (haystack.substr(needle.size(), needle.size()) == needle)) {
			// if needle is repeated, one of them is added as a token for example
			// <C-->
			// means [C, -]
			res.push_back(needle);
		}
		haystack = haystack.substr(skiplen, haystack.size() - skiplen);
	}
	if (haystack.size() > 0) {
		res.push_back(haystack);
	}
}


void run_command(std::wstring command, QStringList parameters, bool wait){


#ifdef Q_OS_WIN
	std::wstring parameters_string = parameters.join(" ").toStdWString();
	SHELLEXECUTEINFO ShExecInfo = { 0 };
	ShExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
	if (wait) {
		ShExecInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
	}
	else {
		ShExecInfo.fMask = SEE_MASK_ASYNCOK;
	}

	ShExecInfo.hwnd = NULL;
	ShExecInfo.lpVerb = NULL;
	ShExecInfo.lpFile = command.c_str();
	ShExecInfo.lpParameters = NULL;
	ShExecInfo.lpDirectory = NULL;
	ShExecInfo.nShow = SW_HIDE;
	ShExecInfo.hInstApp = NULL;
	ShExecInfo.lpParameters = parameters_string.c_str();

	ShellExecuteExW(&ShExecInfo);
	if (wait) {
		WaitForSingleObject(ShExecInfo.hProcess, INFINITE);
		CloseHandle(ShExecInfo.hProcess);
	}

#else
	QProcess* process = new QProcess;
	QString qcommand = QString::fromStdWString(command);
	QStringList qparameters;

	QObject::connect(process, &QProcess::errorOccurred, [process](QProcess::ProcessError error) {
		auto msg = process->errorString().toStdWString();
		show_error_message(msg);
	});

	QObject::connect(process, qOverload<int, QProcess::ExitStatus >(&QProcess::finished), [process](int exit_code, QProcess::ExitStatus stat) {
		process->deleteLater();
	});

	for (int i = 0; i < parameters.size(); i++) {
		qparameters.append(parameters[i]);
	}
	//qparameters.append(QString::fromStdWString(parameters));

	if (wait) {
		process->start(qcommand, qparameters);
		process->waitForFinished();
	}
	else {
		process->startDetached(qcommand, qparameters);
	}
#endif

}


void open_file_url(const QString& url_string) {
	QDesktopServices::openUrl(QUrl::fromLocalFile(url_string));
}

void open_file_url(const std::wstring &url_string) {
	QString qurl_string = QString::fromStdWString(url_string);
	open_file_url(qurl_string);
}

void open_web_url(const QString& url_string) {
	QDesktopServices::openUrl(QUrl(url_string));
}

void open_web_url(const std::wstring &url_string) {
	QString qurl_string = QString::fromStdWString(url_string);
	open_web_url(qurl_string);
}


void search_custom_engine(const std::wstring& search_string, const std::wstring& custom_engine_url) {

	if (search_string.size() > 0) {
		QString qurl_string = QString::fromStdWString(custom_engine_url + search_string);
		open_web_url(qurl_string);
	}
}

void search_google_scholar(const std::wstring& search_string) {
	search_custom_engine(search_string, GOOGLE_SCHOLAR_ADDRESS);
}

void search_libgen(const std::wstring& search_string) {
	search_custom_engine(search_string, LIBGEN_ADDRESS);
}



//void open_url(const std::wstring& url_string) {
//
//	if (url_string.size() > 0) {
//		QString qurl_string = QString::fromStdWString(url_string);
//		open_url(qurl_string);
//	}
//}
//
void create_file_if_not_exists(const std::wstring& path) {
	std::string path_utf8 = utf8_encode(path);
	if (!QFile::exists(QString::fromStdWString(path))) {
		std::ofstream outfile(path_utf8);
		outfile << "";
		outfile.close();
	}
}


void open_file(const std::wstring& path) {
	std::wstring canon_path = get_canonical_path(path);
	open_file_url(canon_path);

}

void get_text_from_flat_chars(const std::vector<fz_stext_char*>& flat_chars, std::wstring& string_res, std::vector<int>& indices) {

	string_res.clear();
	indices.clear();

	for (size_t i = 0; i < flat_chars.size(); i++) {
		fz_stext_char* ch = flat_chars[i];

		if (ch->next == nullptr) { // add a space after the last character in a line, igonre hyphenated characters
			if (ch->c != '-') {
				string_res.push_back(ch->c);
				indices.push_back(i);

				string_res.push_back(' ');
				indices.push_back(-1);
			}
			continue;
		}
		string_res.push_back(ch->c);
		indices.push_back(i);
	}
}


std::wstring find_first_regex_match(const std::wstring& haystack, const std::wstring& regex_string) {
	std::wregex regex(regex_string);
	std::wsmatch match;
	if (std::regex_search(haystack, match, regex)) {
		return match.str();
	}
	return L"";
}

std::vector<std::wstring> find_all_regex_matches(const std::wstring& haystack, const std::wstring& regex_string) {

	std::wregex regex(regex_string);
	std::wsmatch match;
	std::vector<std::wstring> res;
	if (std::regex_search(haystack, match, regex)) {
		for (size_t i = 0; i < match.size(); i++) {
			res.push_back(match[i].str());
		}
	}
	return res;

}

void find_regex_matches_in_stext_page(const std::vector<fz_stext_char*>& flat_chars,
	const std::wregex &regex,
	std::vector<std::pair<int, int>> &match_ranges, std::vector<std::wstring> &match_texts){

	std::wstring page_string;
	std::vector<int> indices;

	get_text_from_flat_chars(flat_chars, page_string, indices);

	std::wsmatch match;

	int offset = 0;
	while (std::regex_search(page_string, match, regex)) {
		int start_index = offset + match.position();
		int end_index = start_index + match.length()-1;
		match_ranges.push_back(std::make_pair(indices[start_index], indices[end_index]));
		match_texts.push_back(match.str());

		int old_length = page_string.size();
		page_string = match.suffix();
		int new_length = page_string.size();

		offset += (old_length - new_length);
	}
}

bool are_stext_chars_far_enough_for_equation(fz_stext_char* first, fz_stext_char* second) {
	float second_width = second->quad.lr.x - second->quad.ll.x;

	if (second_width < 0) {
		return false;
	}

	return (second->origin.x - first->origin.x) > (5 * second_width);
}

bool is_whitespace(int chr) {
	if ((chr == ' ') || (chr == '\n') || (chr == '\t')) {
		return true;
	}
	return false;
}

std::wstring strip_string(std::wstring& input_string) {

	std::wstring result;
	int start_index = 0;
	int end_index = input_string.size() - 1;
	if (input_string.size() == 0) {
		return L"";
	}

	while ( is_whitespace(input_string[start_index])) {
		start_index++;
		if ((size_t) start_index == input_string.size()) {
			return L"";
		}
	}

	while (is_whitespace(input_string[end_index])) {
		end_index--;
	}
	return input_string.substr(start_index, end_index - start_index + 1);

}

void index_generic(const std::vector<fz_stext_char*>& flat_chars, int page_number, std::vector<IndexedData>& indices) {

	std::wstring page_string;
	std::vector<std::optional<fz_rect>> page_rects;

	for (auto ch : flat_chars) {
		page_string.push_back(ch->c);
		page_rects.push_back(fz_rect_from_quad(ch->quad));
		if (ch->next == nullptr) {
			page_string.push_back('\n');
			page_rects.push_back({});
		}
	}

	std::wregex index_dst_regex(L"(^|\n)[A-Z][a-zA-Z]{2,}[ \t]+[0-9]+(\.[0-9]+)*");
	//std::wregex index_dst_regex(L"(^|\n)[A-Z][a-zA-Z]{2,}[ \t]+[0-9]+(\-[0-9]+)*");
	//std::wregex index_src_regex(L"[a-zA-Z]{3,}[ \t]+[0-9]+(\.[0-9]+)*");
	std::wsmatch match;


	int offset = 0;
	while (std::regex_search(page_string, match, index_dst_regex)) {

		IndexedData new_data;
		new_data.page = page_number;
		std::wstring match_string = match.str();
		new_data.text = strip_string(match_string);
		new_data.y_offset = 0.0f;

		int match_start_index = match.position();
		int match_size = match_string.size();
		for (int i = 0; i < match_size; i++) {
			int index = offset +  match_start_index + i;
			if (page_rects[index]) {
				new_data.y_offset = page_rects[index].value().y0;
				break;
			}
		}
		offset += match_start_index + match_size;
		page_string = match.suffix();

		indices.push_back(new_data);
	}
}

void index_equations(const std::vector<fz_stext_char*> &flat_chars, int page_number, std::map<std::wstring, std::vector<IndexedData>>& indices) {
	std::wregex regex(L"\\([0-9]+(\\.[0-9]+)*\\)");
	std::vector<std::pair<int, int>> match_ranges;
	std::vector<std::wstring> match_texts;

	find_regex_matches_in_stext_page(flat_chars, regex, match_ranges, match_texts);

	for (size_t i = 0; i < match_ranges.size(); i++) {
		auto [start_index, end_index] = match_ranges[i];
		if (start_index == -1 || end_index == -1) {
			break;
		}


		// we expect the equation reference to be sufficiently separated from the rest of the text
		if (((start_index > 0) && are_stext_chars_far_enough_for_equation(flat_chars[start_index-1], flat_chars[start_index]))) {

			std::wstring match_text = match_texts[i].substr(1, match_texts[i].size() - 2);
			IndexedData indexed_equation;
			indexed_equation.page = page_number;
			indexed_equation.text = match_text;
			indexed_equation.y_offset = flat_chars[start_index]->quad.ll.y;
			if (indices.find(match_text) == indices.end()) {
				indices[match_text] = std::vector<IndexedData>();
				indices[match_text].push_back(indexed_equation);
			}
			else {
				indices[match_text].push_back(indexed_equation);
			}
		}
	}

}

void index_references(fz_stext_page* page, int page_number, std::map<std::wstring, IndexedData>& indices) {

	char start_char = '[';
	char end_char = ']';
	char delim_char = ',';

	bool started = false;
	std::vector<IndexedData> temp_indices;
	std::wstring current_text = L"";

	LL_ITER(block, page->first_block) {
		if (block->type != FZ_STEXT_BLOCK_TEXT) continue;

		LL_ITER(line, block->u.t.first_line) {
			int chars_in_line = 0;
			LL_ITER(ch, line->first_char) {
				chars_in_line++;
				if (ch->c == ' ') {
					continue;
				}
				//if (ch->c == '.') {
				//	started = false;
				//	temp_indices.clear();
				//	current_text.clear();
				//}

				// references are usually at the beginning of the line, we consider the possibility
				// of up to 3 extra characters before the reference (e.g. numbers, extra spaces, etc.)
				if ((ch->c == start_char) && (chars_in_line < 4)) {
					temp_indices.clear();
					current_text.clear();
					started = true;
					continue;
				}
				if (ch->c == end_char) {
					started = false;

					IndexedData index_data;
					index_data.page = page_number;
					index_data.y_offset = ch->quad.ll.y;
					index_data.text = current_text;

					temp_indices.push_back(index_data);
					//indices[text] = index_data;

					for (auto index : temp_indices) {
						indices[index.text] = index;
					}
					current_text.clear();
					temp_indices.clear();
					continue;
				}
				if (started && (ch->c == delim_char)) {
					IndexedData index_data;
					index_data.page = page_number;
					index_data.y_offset = ch->quad.ll.y;
					index_data.text = current_text;
					current_text.clear();
					temp_indices.push_back(index_data);
					continue;
				}
				if (started) {
					current_text.push_back(ch->c);
				}
			}

		}
	}
}

void sleep_ms(unsigned int ms) {
#ifdef Q_OS_WIN
	Sleep(ms);
#else
	struct timespec ts = { ms / 1000, (ms % 1000) * 1000 * 1000 };
	nanosleep(&ts, NULL);
#endif
}

void get_pixmap_pixel(fz_pixmap* pixmap, int x, int y, unsigned char* r, unsigned char* g, unsigned char* b){
	if (
		(x < 0) ||
		(y < 0) ||
		(x >= pixmap->w) ||
		(y >= pixmap->h)
		) {
		*r = 0;
		*g = 0;
		*b = 0;
		return;
	}

	(*r) = pixmap->samples[y * pixmap->w * 3 + x * 3 + 0];
	(*g) = pixmap->samples[y * pixmap->w * 3 + x * 3 + 1];
	(*b) = pixmap->samples[y * pixmap->w * 3 + x * 3 + 2];
}

int find_max_horizontal_line_length_at_pos(fz_pixmap* pixmap, int pos_x, int pos_y) {
	int min_x = pos_x;
	int max_x = pos_x;

	while (true) {
		unsigned char r, g, b;
		get_pixmap_pixel(pixmap, min_x, pos_y, &r, &g, &b);
		if ((r != 255) || (g != 255) || (b != 255)) {
			break;
		}
		else {
			min_x--;
		}
	}
	while (true) {
		unsigned char r, g, b;
		get_pixmap_pixel(pixmap, max_x, pos_y, &r, &g, &b);
		if ((r != 255) || (g != 255) || (b != 255)) {
			break;
		}
		else {
			max_x++;
		}
	}
	return max_x - min_x;
}

bool largest_contigous_ones(std::vector<int>& arr, int* start_index, int* end_index) {
	arr.push_back(0);

	bool has_at_least_one_one = false;

	for (auto x : arr) {
		if (x == 1) {
			has_at_least_one_one = true;
		}
	}
	if (!has_at_least_one_one) {
		return false;
	}


	int max_count = 0;
	int max_start_index = -1;
	int max_end_index = -1;

	int count = 0;

	for (size_t i = 0; i < arr.size(); i++) {
		if (arr[i] == 1) {
			count++;
		}
		else {
			if (count > max_count) {
				max_count = count;
				max_end_index = i - 1;
				max_start_index = i - count;
			}
			count = 0;
		}
	}

	*start_index = max_start_index;
	*end_index = max_end_index;
	return true;
}

std::vector<unsigned int> get_max_width_histogram_from_pixmap(fz_pixmap* pixmap) {
	std::vector<unsigned int> res;

	for (int j = 0; j < pixmap->h; j++){
		unsigned int x_value = 0;
		for (int i = 0; i < pixmap->w; i++) {
			unsigned char r, g, b;
			get_pixmap_pixel(pixmap, i, j, &r, &g, &b);
			float lightness = (static_cast<float>(r) + static_cast<float>(g) + static_cast<float>(b)) / 3.0f;

			if (lightness > 150) {
				x_value += 1;
			}
		}
		res.push_back(x_value);
	}

	return res;
}

template<typename T>
float average_value(std::vector<T> values) {
	T sum = 0;
	for (auto x : values) {
		sum += x;
	}
	return static_cast<float>(sum) / values.size();
}

template<typename T>
float standard_deviation(std::vector<T> values, float average_value) {
	T sum = 0;
	for (auto x : values) {
		sum += (x - average_value) * (x-average_value);
	}
	return std::sqrt(static_cast<float>(sum) / values.size());
}

//std::vector<unsigned int> get_line_ends_from_histogram(std::vector<unsigned int> histogram) {
void get_line_begins_and_ends_from_histogram(std::vector<unsigned int> histogram, std::vector<unsigned int>& res_begins, std::vector<unsigned int>& res) {

	float mean_width = average_value(histogram);
	float std = standard_deviation(histogram, mean_width);

	std::vector<float> normalized_histogram;

	if (std < 0.00001f) {
		return;
	}

	for (auto x : histogram) {
		normalized_histogram.push_back((x - mean_width) / std);
	}

	size_t i = 0;

	while (i < histogram.size()) {

		while ((i < histogram.size()) && (normalized_histogram[i] > 0.2f)) i++;
		res_begins.push_back(i);
		while ((i < histogram.size()) && (normalized_histogram[i] <= 0.21f)) i++;
		if (i == histogram.size()) break;
		res.push_back(i);
	}

	while (res_begins.size() > res.size()) {
		res_begins.pop_back();
	}

	float additional_distance = 0.0f;

	if (res.size() > 5) {
		std::vector<float> line_distances;

		for (size_t i = 0; i < res.size() - 1; i++) {
			line_distances.push_back(res[i + 1] - res[i]);
		}
		std::nth_element(line_distances.begin(), line_distances.begin() + line_distances.size() / 2, line_distances.end());
		additional_distance = line_distances[line_distances.size() / 2];

		for (size_t i = 0; i < res.size(); i++) {
			res[i] += static_cast<unsigned int>(additional_distance / 5.0f);
			res_begins[i] -= static_cast<unsigned int>(additional_distance / 5.0f);
		}

	}

}

int find_best_vertical_line_location(fz_pixmap* pixmap, int doc_x, int doc_y) {


	int search_height = 5;

	float min_candid_y = doc_y;
	float max_candid_y = doc_y + search_height;

	std::vector<int> max_possible_widths;

	for (int candid_y = min_candid_y; candid_y <= max_candid_y; candid_y++) {
		int current_width = find_max_horizontal_line_length_at_pos(pixmap, doc_x , candid_y);
		max_possible_widths.push_back(current_width);
	}

	int max_width_value = -1;

	for (auto w : max_possible_widths) {
		if (w > max_width_value) {
			max_width_value = w;
		}
	}
	std::vector<int> is_max_list;

	for (auto x : max_possible_widths) {
		if (x == max_width_value) {
			is_max_list.push_back(1);
		}
		else{
			is_max_list.push_back(0);
		}
	}

	int start_index, end_index;
	largest_contigous_ones(is_max_list, &start_index, &end_index);

	//return doc_y + (start_index + end_index) / 2;
	return doc_y + start_index ;
}

bool is_string_numeric_(const std::wstring& str) {
	if (str.size() == 0) {
		return false;
	}
	for (auto ch : str) {
		if (!std::isdigit(ch)) {
			return false;
		}
	}
	return true;
}

bool is_string_numeric(const std::wstring& str) {
	if (str.size() == 0) {
		return false;
	}

	if (str[0] == '-') {
		return is_string_numeric_(str.substr(1, str.length() - 1));
	}
	else {
		return is_string_numeric_(str);
	}
}


bool is_string_numeric_float(const std::wstring& str) {
	if (str.size() == 0) {
		return false;
	}
	int dot_count = 0;

	for (size_t i = 0; i < str.size(); i++) {
		if (i == 0) {
			if (str[i] == '-') continue;
		}
		else {
			if (str[i] == '.') {
				dot_count++;
				if (dot_count >= 2) return false;
			}
			else if (!std::isdigit(str[i])) {
				return false;
			}
		}

	}
	return true;
}

QByteArray serialize_string_array(const QStringList& string_list) {
	QByteArray result;
	QDataStream stream(&result, QIODevice::WriteOnly);
	stream << static_cast<int>(string_list.size());
	for (int i = 0; i < string_list.size(); i++) {
		stream << string_list.at(i);
	}
	return result;
}


QStringList deserialize_string_array(const QByteArray &byte_array) {
	QStringList result;
	QDataStream stream(byte_array);

	int size;
	stream >> size;

	for (int i = 0; i < size; i++) {
		QString string;
		stream >> string;
		result.append(string);
	}

	return result;
}




char* get_argv_value(int argc, char** argv, std::string key) {
	for (int i = 0; i < argc-1; i++) {
		if (key == argv[i]) {
			return argv[i + 1];
		}
	}
	return nullptr;
}

bool has_arg(int argc, char** argv, std::string key) {
	for (int i = 0; i < argc; i++) {
		if (key == argv[i]) {
			return true;
		}
	}
	return false;
}
bool should_reuse_instance(int argc, char** argv) {
	for (int i = 0; i < argc; i++) {
		if (std::strcmp(argv[i], "--reuse-instance") == 0) return true;

	}
	return false;
}

bool should_new_instance(int argc, char** argv) {
	for (int i = 0; i < argc; i++) {
		if (std::strcmp(argv[i], "--new-instance") == 0) return true;

	}
	return false;
}

QCommandLineParser* get_command_line_parser() {

	QCommandLineParser* parser = new QCommandLineParser();

	parser->setApplicationDescription("Sioyek is a PDF reader designed for reading research papers and technical books.");
	//parser->addVersionOption();

	//QCommandLineOption reuse_instance_option("reuse-instance");
	//reuse_instance_option.setDescription("When opening a new file, reuse the previous instance of sioyek instead of opening a new window.");
	//parser->addOption(reuse_instance_option);

	//QCommandLineOption new_instance_option("new-instance");
	//new_instance_option.setDescription("When opening a new file, create a new instance of sioyek.");
	//parser->addOption(new_instance_option);

	QCommandLineOption new_window_option("new-window");
	new_window_option.setDescription("Open the file in a new window but within the same sioyek instance (reuses the previous window if a sioyek window with the same file already exists).");
	parser->addOption(new_window_option);

	QCommandLineOption reuse_window_option("reuse-window");
	reuse_window_option.setDescription("Force sioyek to reuse the current window even when should_launch_new_window is set.");
	parser->addOption(reuse_window_option);

	QCommandLineOption nofocus_option("nofocus");
	nofocus_option.setDescription("Do not bring the sioyek instance to foreground.");
	parser->addOption(nofocus_option);

	QCommandLineOption version_option("version");
	version_option.setDescription("Print sioyek version.");
	parser->addOption(version_option);


	QCommandLineOption page_option("page", "Which page to open.", "page");
	parser->addOption(page_option);

	QCommandLineOption focus_option("focus-text", "Set a visual mark on line including <text>.", "focus-text");
	parser->addOption(focus_option);

	QCommandLineOption focus_page_option("focus-text-page", "Specifies the page which is used for focus-text", "focus-text-page");
	parser->addOption(focus_page_option);


	QCommandLineOption inverse_search_option("inverse-search", "The command to execute when performing inverse search.\
 In <command>, %1 is filled with the file name and %2 is filled with the line number.", "command");
	parser->addOption(inverse_search_option);

	QCommandLineOption command_option("execute-command", "The command to execute on running instance of sioyek", "execute-command");
	parser->addOption(command_option);

	QCommandLineOption command_data_option("execute-command-data", "Optional data for execute-command command", "execute-command-data");
	parser->addOption(command_data_option);

	QCommandLineOption forward_search_file_option("forward-search-file", "Perform forward search on file <file>. You must also include --forward-search-line to specify the line", "file");
	parser->addOption(forward_search_file_option);

	QCommandLineOption forward_search_line_option("forward-search-line", "Perform forward search on line <line>. You must also include --forward-search-file to specify the file", "line");
	parser->addOption(forward_search_line_option);

	QCommandLineOption forward_search_column_option("forward-search-column", "Perform forward search on column <column>. You must also include --forward-search-file to specify the file", "column");
	parser->addOption(forward_search_column_option);

	QCommandLineOption zoom_level_option("zoom", "Set zoom level to <zoom>.", "zoom");
	parser->addOption(zoom_level_option);

	QCommandLineOption xloc_option("xloc", "Set x position within page to <xloc>.", "xloc");
	parser->addOption(xloc_option);

	QCommandLineOption yloc_option("yloc", "Set y position within page to <yloc>.", "yloc");
	parser->addOption(yloc_option);

	QCommandLineOption shared_database_path_option("shared-database-path", "Specify which file to use for shared data (bookmarks, highlights, etc.)", "path");
	parser->addOption(shared_database_path_option);

    parser->addHelpOption();

	return parser;
}


std::wstring concatenate_path(const std::wstring& prefix, const std::wstring& suffix) {
	std::wstring result = prefix;
#ifdef Q_OS_WIN
	wchar_t separator = '\\';
#else
	wchar_t separator = '/';
#endif
	if (prefix == L"") {
		return suffix;
	}

	if (result[result.size() - 1] != separator) {
		result.push_back(separator);
	}
	result.append(suffix);
	return result;
}

std::wstring get_canonical_path(const std::wstring& path) {
	QDir dir(QString::fromStdWString(path));
	//return std::move(dir.canonicalPath().toStdWString());
	return std::move(dir.absolutePath().toStdWString());

}

std::wstring add_redundant_dot_to_path(const std::wstring& path) {
	std::vector<std::wstring> parts;
	split_path(path, parts);

	std::wstring last = parts[parts.size() - 1];
	parts.erase(parts.begin() + parts.size() - 1);
	parts.push_back(L".");
	parts.push_back(last);

	std::wstring res = L"";
	if (path[0] == '/') {
		res = L"/";
	}

	for (size_t i = 0; i < parts.size(); i++) {
		res.append(parts[i]);
		if (i < parts.size() - 1) {
			res.append(L"/");
		}
	}
	return res;
}

float manhattan_distance(float x1, float y1, float x2, float y2) {
	return abs(x1 - x2) + abs(y1 - y2);
}

float manhattan_distance(fvec2 v1, fvec2 v2) {
	return manhattan_distance(v1.x(), v1.y(), v2.x(), v2.y());
}

QWidget* get_top_level_widget(QWidget* widget) {
	while (widget->parent() != nullptr) {
		widget = widget->parentWidget();
	}
	return widget;
}

float type_name_similarity_score(std::wstring name1, std::wstring name2) {
	name1 = to_lower(name1);
	name2 = to_lower(name2);
	size_t common_prefix_index = 0;

	while (name1[common_prefix_index] == name2[common_prefix_index]){
		common_prefix_index++;
		if ((common_prefix_index == name1.size()) || (common_prefix_index == name2.size())) {
			return common_prefix_index;
		}
	}
	return common_prefix_index;
}

void check_for_updates(QWidget* parent, std::string current_version) {

	QString url = "https://github.com/ahrm/sioyek/releases/latest";
	QNetworkAccessManager* manager = new QNetworkAccessManager;

	QObject::connect(manager, &QNetworkAccessManager::finished, [=](QNetworkReply *reply) {
		std::string response_text = reply->readAll().toStdString();
		int first_index = response_text.find("\"");
		int last_index = response_text.rfind("\"");
		std::string url_string = response_text.substr(first_index + 1, last_index - first_index - 1);

		std::vector<std::wstring> parts;
		split_path(utf8_decode(url_string), parts);
		if (parts.size() > 0) {
			std::string version_string = utf8_encode(parts.back().substr(1, parts.back().size() - 1));

			if (version_string != current_version) {
				int ret = QMessageBox::information(parent, "Update", QString::fromStdString("Do you want to update from " + current_version + " to " + version_string + "?"),
					QMessageBox::Ok | QMessageBox::Cancel,
					QMessageBox::Cancel);
				if (ret == QMessageBox::Ok) {
					open_web_url(url);
				}
			}

		}
		});
	manager->get(QNetworkRequest(QUrl(url)));
}

QString expand_home_dir(QString path) {
	if (path.size() > 0) {
		if (path.at(0) == '~') {
			return QDir::homePath() + QDir::separator() + path.remove(0, 2);
		}
	}
	return path;
}

void split_root_file(QString path, QString& out_root, QString& out_partial) {

	QChar sep = QDir::separator();
	if (path.indexOf(sep) == -1) {
		sep = '/';
	}

	QStringList parts = path.split(sep);

	if (path.size() > 0) {
		if (path.at(path.size()-1) == sep) {
			out_root = parts.join(sep);
		}
		else {
			if ((parts.size() == 2) && (path.at(0) == '/')) {
				out_root = "/";
				out_partial = parts.at(1);
			}
			else {
				out_partial = parts.at(parts.size()-1);
				parts.pop_back();
				out_root = parts.join(QDir::separator());
			}
		}
	}
	else {
		out_partial = "";
		out_root = "";
	}
}


std::wstring lowercase(const std::wstring& input) {

	std::wstring res;
	for (int i = 0; i < input.size(); i++) {
		if ((input[i] >= 'A') && (input[i] <= 'Z')) {
			res.push_back(input[i] + 'a' - 'A');
		}
		else {
			res.push_back(input[i]);
		}
	}
	return res;
}

int hex2int(int hex) {
	if (hex >= '0' && hex <= '9') {
		return hex - '0';
	}
	else {
		return (hex - 'a') + 10;
	}
}
float get_color_component_from_hex(std::wstring hexcolor) {
	hexcolor = lowercase(hexcolor);

	if (hexcolor.size() < 2) {
		return 0;
	}
	return static_cast<float>(hex2int(hexcolor[0]) * 16 + hex2int(hexcolor[1]) ) / 255.0f;
}

QString get_color_hexadecimal(float color) {
	QString hex_map[] = { "0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "a", "b", "c", "d", "e", "f" };
	int val = static_cast<int>(color * 255);
	int high = val / 16;
	int low = val % 16;
	return QString("%1%2").arg(hex_map[high], hex_map[low]);

}

QString get_color_qml_string(float r, float g, float b) {
	QString res =  QString("#%1%2%3").arg(get_color_hexadecimal(r), get_color_hexadecimal(g), get_color_hexadecimal(b));
	return res;
}

void hexademical_to_normalized_color(std::wstring color_string, float* color, int n_components) {
	if (color_string[0] == '#') {
		color_string = color_string.substr(1, color_string.size() - 1);
	}

	for (int i = 0; i < n_components; i++) {
		*(color + i) = get_color_component_from_hex(color_string.substr(i * 2, 2));
	}
}

void copy_file(std::wstring src_path, std::wstring dst_path) {
	std::ifstream  src(utf8_encode(src_path), std::ios::binary);
	std::ofstream  dst(utf8_encode(dst_path), std::ios::binary);

	dst << src.rdbuf();
}

fz_quad quad_from_rect(fz_rect r)
{
	fz_quad q;
	q.ul = fz_make_point(r.x0, r.y0);
	q.ur = fz_make_point(r.x1, r.y0);
	q.ll = fz_make_point(r.x0, r.y1);
	q.lr = fz_make_point(r.x1, r.y1);
	return q;
}

std::vector<fz_quad> quads_from_rects(const std::vector<fz_rect>& rects) {
	std::vector<fz_quad> res;
	for (auto rect : rects) {
		res.push_back(quad_from_rect(rect));
	}
	return res;
}

std::wifstream open_wifstream(const std::wstring& file_name) {

#ifdef Q_OS_WIN
	return std::move(std::wifstream(file_name));
#else
	std::string encoded_file_name = utf8_encode(file_name);
	return std::move(std::wifstream(encoded_file_name.c_str()));
#endif
}

std::wstring truncate_string(const std::wstring& inp, int size) {
        if (inp.size() <= (size_t) size) {
		return inp;
	}
        else {
		return inp.substr(0, size - 3) + L"...";
	}

}

std::wstring get_page_formatted_string(int page) {
	std::wstringstream ss;
	ss << L"[ " << page << L" ]";
	return ss.str();
}

bool is_string_titlish(const std::wstring& str) {
	if (str.size() <= 5 || str.size() >= 60) {
		return false;
	}
	std::wregex regex(L"([0-9IVXC]+\\.)+([0-9IVXC]+)*");
	std::wsmatch match;

	std::regex_search(str, match, regex);
	int pos = match.position();
	int size = match.length();
	return (size > 0) && (pos == 0);
}

bool is_title_parent_of(const std::wstring& parent_title, const std::wstring& child_title, bool* are_same) {
	int count = std::min(parent_title.size(), child_title.size());

	*are_same = false;

	for (int i = 0; i < count; i++) {
		if (parent_title.at(i) == ' ') {
			if (child_title.at(i) == ' ') {
				*are_same = true;
				return false;
			}
			else {
				return true;
			}
		}
		if (child_title.at(i) != parent_title.at(i)) {
			return false;
		}
	}

	return true;
}

struct Range {
	float begin;
	float end;

	float size() {
		return end - begin;
	}
};

Range merge_range(Range range1, Range range2) {
	Range res;
	res.begin = std::min(range1.begin, range2.begin);
	res.end = std::max(range1.end, range2.end);
	return res;
}

float line_num_penalty(int num) {
	return 1.0f;
	//if (num == 1) {
	//	return 1.0f;
	//}
	//return 1.0f + static_cast<float>(num) / 5.0f;
}

float height_increase_penalty(float ratio) {
	return  50 * ratio;
}

float width_increase_bonus(float ratio) {
	return -50 * ratio;
}

int find_best_merge_index_for_line_index(const std::vector<fz_stext_line*>& lines,
	const std::vector<fz_rect>& line_rects,
	const std::vector<int> char_counts,
	int index) {

	return index;
	int max_merged_lines = 40;
	//Range current_range = { lines[index]->bbox.y0, lines[index]->bbox.y1 };
	//Range current_range_x = { lines[index]->bbox.x0, lines[index]->bbox.x1 };
	Range current_range = { line_rects[index].y0, line_rects[index].y1 };
	Range current_range_x = { line_rects[index].x0, line_rects[index].x1 };
	float maximum_height = current_range.size();
	float maximum_width = current_range_x.size();
	float min_cost = current_range.size() * line_num_penalty(1) / current_range_x.size();
	int min_index = index;

	for (size_t j = index + 1; (j < lines.size()) && ((j - index) < (size_t) max_merged_lines); j++) {
		float line_height = line_rects[j].y1 - line_rects[j].y0;
		float line_width = line_rects[j].x1 - line_rects[j].x0;
		if (line_height > maximum_height) {
			maximum_height = line_height;
		}
		if (line_width > maximum_width) {
			maximum_width = line_width;
		}
		if (char_counts[j] > 10) {
			current_range = merge_range(current_range, { line_rects[j].y0, line_rects[j].y1 });
		}

		current_range_x = merge_range(current_range, { line_rects[j].x0, line_rects[j].x1 });

		float cost = current_range.size() / (j - index + 1) +
			line_num_penalty(j - index + 1) / current_range_x.size() +
			height_increase_penalty(current_range.size() / maximum_height) +
			width_increase_bonus(current_range_x.size() / maximum_width);
		if (cost < min_cost) {
			min_cost = cost;
			min_index = j;
		}
	}
	return min_index;
}


fz_rect get_line_rect(fz_stext_line* line) {
	fz_rect res;
	res.x0 = res.x1 = res.y0 = res.y1 = 0;

	if (line == nullptr || line->first_char == nullptr) {
		return res;
	}

	res = fz_rect_from_quad(line->first_char->quad);

	std::vector<float> char_x_begins;
	std::vector<float> char_x_ends;
	std::vector<float> char_y_begins;
	std::vector<float> char_y_ends;

	int num_chars = 0;
	LL_ITER(chr, line->first_char) {
		fz_rect char_rect = fz_union_rect(res, fz_rect_from_quad(chr->quad));
		char_x_ends.push_back(char_rect.x1);
		char_y_begins.push_back(char_rect.y0);
		char_y_ends.push_back(char_rect.y1);

		if (char_rect.x0 > 0) {
			char_x_begins.push_back(char_rect.x0);
		}
		num_chars++;
	}


	int percentile_index = static_cast<int>(0.5f * num_chars);
	int first_percentile_index = percentile_index;
	int last_percentile_index = num_chars - percentile_index;

	if (last_percentile_index >= num_chars) {
		last_percentile_index = num_chars - 1;
	}

	if (char_x_begins.size() > 0) {
		res.x0 = *std::min_element(char_x_begins.begin(), char_x_begins.end());
		res.x1 = *std::max_element(char_x_ends.begin(), char_x_ends.end());
	}
	if (char_y_begins.size() > 0) {
		std::nth_element(char_y_begins.begin(), char_y_begins.begin() + first_percentile_index, char_y_begins.end());
		std::nth_element(char_y_ends.begin(), char_y_ends.begin() + last_percentile_index, char_y_ends.end());
		res.y0 = *(char_y_begins.begin() + first_percentile_index);
		res.y1 = *(char_y_ends.begin() + last_percentile_index);
	}

	return res;
}

int line_num_chars(fz_stext_line* line) {
	int res = 0;
	LL_ITER(chr, line->first_char) {
		res++;
	}
	return res;
}


void merge_lines(const std::vector<fz_stext_line*>& lines_, std::vector<fz_rect>& out_rects, std::vector<std::wstring>& out_texts) {

	std::vector<fz_stext_line*> lines = lines_;

	std::vector<fz_rect> temp_rects;
	std::vector<std::wstring> temp_texts;

	std::vector<fz_rect> custom_line_rects;
	std::vector<int> char_counts;

	std::vector<int> indices_to_delete;
	for (size_t i = 0; i < lines.size(); i++) {
		if (line_num_chars(lines[i]) < 5) {
			indices_to_delete.push_back(i);
		}
	}

	for (int i = indices_to_delete.size() - 1; i >= 0; i--) {
		lines.erase(lines.begin() + indices_to_delete[i]);
	}

	for (auto line : lines) {
		custom_line_rects.push_back(get_line_rect(line));
		char_counts.push_back(line_num_chars(line));
	}

	for (size_t i = 0; i < lines.size(); i++) {
		fz_rect rect = custom_line_rects[i];
		int best_index = find_best_merge_index_for_line_index(lines, custom_line_rects, char_counts, i);
		std::wstring text = get_string_from_stext_line(lines[i]);
		for (int j = i+1; j <= best_index; j++) {
			rect = fz_union_rect(rect, lines[j]->bbox);
			text = text + get_string_from_stext_line(lines[j]);
		}
		temp_rects.push_back(rect);
		temp_texts.push_back(text);
		i = best_index;
	}
	for (size_t i = 0; i < temp_rects.size(); i++) {
		if (i > 0 && out_rects.size() > 0) {
			fz_rect prev_rect = out_rects[out_rects.size() - 1];
			fz_rect current_rect = temp_rects[i];
			if ((std::abs(prev_rect.y0 - current_rect.y0) < 1.0f) || (std::abs(prev_rect.y1 - current_rect.y1) < 1.0f)) {
				out_rects[out_rects.size() - 1].x0 = std::min(prev_rect.x0, current_rect.x0);
				out_rects[out_rects.size() - 1].x1 = std::max(prev_rect.x1, current_rect.x1);

				out_rects[out_rects.size() - 1].y0 = std::min(prev_rect.y0, current_rect.y0);
				out_rects[out_rects.size() - 1].y1 = std::max(prev_rect.y1, current_rect.y1);
				out_texts[out_texts.size() - 1] = out_texts[out_texts.size() - 1] + temp_texts[i];
				continue;
			}
		}
		out_rects.push_back(temp_rects[i]);
		out_texts.push_back(temp_texts[i]);
	}
}

float get_max_display_scaling() {
	float scale = 1.0f;
	auto screens = QGuiApplication::screens();
	for (int i = 0; i < screens.size(); i++) {
		float display_scale = screens.at(i)->devicePixelRatio();
		if (display_scale > scale) {
			scale = display_scale;
		}
	}
	return scale;
}

int lcs(const char* X, const char* Y, int m, int n)
{
	//int L[m + 1][n + 1];
	std::vector<std::vector<int>> L;
	for (int i = 0; i < m + 1; i++) {
		L.push_back(std::vector<int>(n + 1));
	}

	int i, j;

	/* Following steps build L[m+1][n+1] in bottom up fashion. Note
	  that L[i][j] contains length of LCS of X[0..i-1] and Y[0..j-1] */
	for (i = 0; i <= m; i++) {
		for (j = 0; j <= n; j++) {
			if (i == 0 || j == 0)
				L[i][j] = 0;

			else if (X[i - 1] == Y[j - 1])
				L[i][j] = L[i - 1][j - 1] + 1;

			else
				L[i][j] = std::max(L[i - 1][j], L[i][j - 1]);
		}
	}

	/* L[m][n] contains length of LCS for X[0..n-1] and Y[0..m-1] */
	return L[m][n];
}

bool command_requires_text(const std::wstring& command) {
	if ((command.find(L"%5") != -1) || (command.find(L"command_text") != -1)) {
		return true;
	}
	return false;
}

bool command_requires_rect(const std::wstring& command) {
	if (command.find(L"%{selected_rect}") != -1) {
		return true;
	}
	return false;
}

void parse_command_string(std::wstring command_string, std::string& command_name, std::wstring& command_data) {
	int lindex = command_string.find(L"(");
	int rindex = command_string.rfind(L")");
	if (lindex < rindex) {
		command_name = utf8_encode(command_string.substr(0, lindex));
		command_data = command_string.substr(lindex + 1, rindex - lindex - 1);
	}
	else {
		command_data = L"";
		command_name = utf8_encode(command_string);
	}
}

void parse_color(std::wstring color_string, float* out_color, int n_components) {
	if (color_string.size() > 0) {
		if (color_string[0] == '#') {
			hexademical_to_normalized_color(color_string, out_color, n_components);
		}
		else {
			std::wstringstream ss(color_string);

			for (int i = 0; i < n_components; i++) {
				ss >> *(out_color + i);
			}
		}
	}
}

int get_status_bar_height() {
    if (STATUS_BAR_FONT_SIZE > 0) {
        return STATUS_BAR_FONT_SIZE + 5;
    }
    else {
        return 20;
    }
}

void flat_char_prism(const std::vector<fz_stext_char*> chars, int page, std::wstring& output_text, std::vector<int>& pages, std::vector<fz_rect>& rects) {
	fz_stext_char* last_char = nullptr;

	for (int j = 0; j < chars.size(); j++) {
		if (is_line_separator(last_char, chars[j])) {
			if (last_char->c == '-') {
				pages.pop_back();
				rects.pop_back();
				output_text.pop_back();
			}
			else {
				pages.push_back(page);
				rects.push_back(fz_rect_from_quad(chars[j]->quad));
				output_text.push_back(' ');
			}
		}
		pages.push_back(page);
		rects.push_back(fz_rect_from_quad(chars[j]->quad));
		output_text.push_back(chars[j]->c);
		last_char = chars[j];
	}
}

QString get_status_stylesheet(bool nofont) {
    if ((!nofont) && (STATUS_BAR_FONT_SIZE > -1)) {
        QString	font_size_stylesheet = QString("font-size: %1px").arg(STATUS_BAR_FONT_SIZE);
        return QString("background-color: %1; color: %2; border: 0; %3;").arg(
            get_color_qml_string(STATUS_BAR_COLOR[0], STATUS_BAR_COLOR[1], STATUS_BAR_COLOR[2]),
            get_color_qml_string(STATUS_BAR_TEXT_COLOR[0], STATUS_BAR_TEXT_COLOR[1], STATUS_BAR_TEXT_COLOR[2]),
            font_size_stylesheet
        );
    }
    else{
        return QString("background-color: %1; color: %2; border: 0;").arg(
            get_color_qml_string(STATUS_BAR_COLOR[0], STATUS_BAR_COLOR[1], STATUS_BAR_COLOR[2]),
            get_color_qml_string(STATUS_BAR_TEXT_COLOR[0], STATUS_BAR_TEXT_COLOR[1], STATUS_BAR_TEXT_COLOR[2])
        );
    }
}

QString get_selected_stylesheet(bool nofont) {
    if ((!nofont) && STATUS_BAR_FONT_SIZE > -1) {
        QString	font_size_stylesheet = QString("font-size: %1px").arg(STATUS_BAR_FONT_SIZE);
        return QString("background-color: %1; color: %2; border: 0; %3;").arg(
            get_color_qml_string(UI_SELECTED_BACKGROUND_COLOR[0], UI_SELECTED_BACKGROUND_COLOR[1], UI_SELECTED_BACKGROUND_COLOR[2]),
            get_color_qml_string(UI_SELECTED_TEXT_COLOR[0], UI_SELECTED_TEXT_COLOR[1], UI_SELECTED_TEXT_COLOR[2]),
            font_size_stylesheet
        );
    }
    else{
        return QString("background-color: %1; color: %2; border: 0;").arg(
            get_color_qml_string(UI_SELECTED_BACKGROUND_COLOR[0], UI_SELECTED_BACKGROUND_COLOR[1], UI_SELECTED_BACKGROUND_COLOR[2]),
            get_color_qml_string(UI_SELECTED_TEXT_COLOR[0], UI_SELECTED_TEXT_COLOR[1], UI_SELECTED_TEXT_COLOR[2])
        );
    }
}

void convert_color4(float* in_color, int* out_color) {
	out_color[0] = (int)(in_color[0] * 255);
	out_color[1] = (int)(in_color[1] * 255);
	out_color[2] = (int)(in_color[2] * 255);
	out_color[3] = (int)(in_color[3] * 255);
}
