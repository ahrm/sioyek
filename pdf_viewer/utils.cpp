#include <Windows.h>

#include <cassert>
#include "utils.h"


wstring to_lower(const wstring& inp) {
	wstring res;
	for (char c : inp) {
		res.push_back(::tolower(c));
	}
	return res;
}

void convert_toc_tree(fz_outline* root, vector<TocNode*>& output) {
	// convert an fz_outline structure to a tree of TocNodes

	do {
		if (root == nullptr) {
			break;
		}

		TocNode* current_node = new TocNode;
		current_node->title = utf8_decode(root->title);
		current_node->page = root->page;
		current_node->x = root->x;
		current_node->y = root->y;
		convert_toc_tree(root->down, current_node->children);
		output.push_back(current_node);
	} while (root = root->next);
}

void get_flat_toc(const vector<TocNode*>& roots, vector<wstring>& output, vector<int>& pages) {
	// Enumerate ToC nodes in the DFS order

	for (const auto& root : roots) {
		output.push_back(root->title);
		pages.push_back(root->page);
		get_flat_toc(root->children, output, pages);
	}
}

TocNode* get_toc_node_from_indices_helper(const vector<TocNode*>& roots, const vector<int>& indices, int pointer) {
	if (pointer < 0) {
		// should not happen
		assert(false);
	}
	if (pointer == 0) {
		return roots[indices[pointer]];
	}

	return get_toc_node_from_indices_helper(roots[indices[pointer]]->children, indices, pointer - 1);
}

TocNode* get_toc_node_from_indices(const vector<TocNode*>& roots, const vector<int>& indices) {
	return get_toc_node_from_indices_helper(roots, indices, indices.size() - 1);
}


QStandardItem* get_item_tree_from_toc_helper(const vector<TocNode*>& children, QStandardItem* parent) {

	for (const auto* child : children) {
		QStandardItem* child_item = new QStandardItem(QString::fromStdWString(child->title));
		child_item->setData(child->page);
		parent->appendRow(get_item_tree_from_toc_helper(child->children, child_item));
	}
	return parent;
}


QStandardItemModel* get_model_from_toc(const vector<TocNode*>& roots) {

	QStandardItemModel* model = new QStandardItemModel();
	get_item_tree_from_toc_helper(roots, model->invisibleRootItem());
	return model;
}


int mod(int a, int b)
{
	// compute a mod b handling negative numbers "correctly"
	return (a % b + b) % b;
}

bool intersects(float range1_start, float range1_end, float range2_start, float range2_end) {
	if (range2_start > range1_end || range1_start > range2_end) {
		return false;
	}
	return true;
}

void parse_uri(string uri, int* page, float* offset_x, float* offset_y) {
	int comma_index = -1;

	uri = uri.substr(1, uri.size() - 1);
	comma_index = uri.find(",");
	*page = atoi(uri.substr(0, comma_index ).c_str());

	uri = uri.substr(comma_index+1, uri.size() - comma_index-1);
	comma_index = uri.find(",");
	*offset_x = atof(uri.substr(0, comma_index ).c_str());

	uri = uri.substr(comma_index+1, uri.size() - comma_index-1);
	*offset_y = atof(uri.c_str());
}

bool includes_rect(fz_rect includer, fz_rect includee) {
	fz_rect intersection = fz_intersect_rect(includer, includee);
	if (intersection.x0 == includee.x0 && intersection.x1 == includee.x1 &&
		intersection.y0 == includee.y0 && intersection.y1 == includee.y1) {
		return true;
	}
	return false;
}

char get_symbol(int key, bool is_shift_pressed) {
	//char key = SDL_GetKeyFromScancode(scancode);
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

fz_rect corners_to_rect(fz_point corner1, fz_point corner2) {
	fz_rect res;
	res.x0 = min(corner1.x, corner2.x);
	res.x1 = max(corner1.x, corner2.x);

	res.y0 = min(corner1.y, corner2.y);
	res.y1 = max(corner1.y, corner2.y);
	return res;
}

void copy_to_clipboard(const wstring& text) {
	if (text.size() > 0) {
		const size_t len = text.size() + 1;
		const size_t size = len * sizeof(text[0]);
		HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, size);
		memcpy(GlobalLock(hMem), text.c_str(), size);
		GlobalUnlock(hMem);
		OpenClipboard(0);
		EmptyClipboard();
		SetClipboardData(CF_UNICODETEXT, hMem);
		CloseClipboard();
	}
}

#define OPEN_KEY(parent, name, ptr) \
	RegCreateKeyExA(parent, name, 0, 0, 0, KEY_WRITE, 0, &ptr, 0)

#define SET_KEY(parent, name, value) \
	RegSetValueExA(parent, name, 0, REG_SZ, (const BYTE *)(value), (DWORD)strlen(value) + 1)

void install_app(char *argv0)
{
	char buf[512];
	HKEY software, classes, testpdf, dotpdf;
	HKEY shell, open, command, supported_types;
	HKEY pdf_progids;

	OPEN_KEY(HKEY_CURRENT_USER, "Software", software);
	OPEN_KEY(software, "Classes", classes);
	OPEN_KEY(classes, ".pdf", dotpdf);
	OPEN_KEY(dotpdf, "OpenWithProgids", pdf_progids);
	OPEN_KEY(classes, "TestPdf", testpdf);
	OPEN_KEY(testpdf, "SupportedTypes", supported_types);
	OPEN_KEY(testpdf, "shell", shell);
	OPEN_KEY(shell, "open", open);
	OPEN_KEY(open, "command", command);

	sprintf(buf, "\"%s\" \"%%1\"", argv0);

	SET_KEY(open, "FriendlyAppName", "TestPdf");
	SET_KEY(command, "", buf);
	SET_KEY(supported_types, ".pdf", "");
	SET_KEY(pdf_progids, "TestPdf", "");

	RegCloseKey(dotpdf);
	RegCloseKey(testpdf);
	RegCloseKey(classes);
	RegCloseKey(software);
}

int get_f_key(string name) {
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
	stringstream ss(name);
	ss >> num;
	return  num;
}

void show_error_message(wstring error_message) {
	MessageBoxW(nullptr, error_message.c_str(), L"Error", MB_OK);
}

wstring utf8_decode(const string& encoded_str) {
	wstring res;
	utf8::utf8to32(encoded_str.begin(), encoded_str.end(), std::back_inserter(res));
	return res;
}

string utf8_encode(const wstring& decoded_str) {
	string res;
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

wstring reverse_wstring(const wstring& inp) {
	wstring res;
	for (int i = inp.size() - 1; i >= 0; i--) {
		res.push_back(inp[i]);
	}
	return res;
}

bool parse_search_command(const wstring& search_command, int* out_begin, int* out_end, wstring* search_text) {
	wstringstream ss(search_command);
	if (search_command[0] == '<') {
		wchar_t dummy;
		ss >> dummy;
		ss >> *out_begin;
		ss >> dummy;
		ss >> *out_end;
		ss >> dummy;
		getline(ss, *search_text);
		return true;
	}
	else {
		*search_text = ss.str();
		return false;
	}
}

float dist_squared(fz_point p1, fz_point p2) {
	return (p1.x - p2.x) * (p1.x - p2.x) + (p1.y - p2.y) * (p1.y - p2.y);
}

fz_stext_char_s* find_closest_char_to_document_point(fz_stext_page* stext_page, fz_point document_point, int* location_index) {
	float min_distance = std::numeric_limits<float>::infinity();
	fz_stext_char_s* res = nullptr;

	int index = 0;
	LL_ITER(current_block, stext_page->first_block) {
		if (current_block->type == FZ_STEXT_BLOCK_TEXT) {
			LL_ITER(current_line, current_block->u.t.first_line) {
				LL_ITER(current_char, current_line->first_char) {
					//fz_point quad_center = find_quad_center(current_char->quad); // todo: use .origin instead
					fz_point quad_center = current_char->origin;
					float distance = dist_squared(document_point, quad_center);
					if (distance < min_distance) {
						min_distance = distance;
						res = current_char;
						*location_index = index;
					}
					index++;
				}
			}
		}
	}

	return res;
}

void get_stext_block_string(fz_stext_block* block, wstring& res) {
	assert(block->type == FZ_STEXT_BLOCK_TEXT);

	LL_ITER(line, block->u.t.first_line) {
		LL_ITER(c, line->first_char) {
			res.push_back(c->c);
		}
	}
}

bool does_stext_block_starts_with_string(fz_stext_block* block, const wstring& str) {
	assert(block->type == FZ_STEXT_BLOCK_TEXT);

	if (block->u.t.first_line) {
		int index = 0;
		LL_ITER(ch, block->u.t.first_line->first_char) {
			if (ch->c != str[index]) {
				return false;
			}
			index++;
			if (index == str.size()) {
				return true;
			}
		}
	}
	return false;
}
