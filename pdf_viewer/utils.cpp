#include <Windows.h>

#include <cassert>
#include "utils.h"


std::wstring to_lower(const std::wstring& inp) {
	std::wstring res;
	for (char c : inp) {
		res.push_back(::tolower(c));
	}
	return res;
}

void convert_toc_tree(fz_outline* root, std::vector<TocNode*>& output, fz_context* context, fz_document* doc) {
	// convert an fz_outline structure to a tree of TocNodes

	do {
		if (root == nullptr) {
			break;
		}

		TocNode* current_node = new TocNode;
		current_node->title = utf8_decode(root->title);
		//current_node->page = root->page;
		current_node->x = root->x;
		current_node->y = root->y;
		if (root->page == -1) {
			float xp, yp;
			fz_location loc = fz_resolve_link(context, doc, root->uri, &xp, &yp);
			int chapter_page = 0;
			current_node->page = chapter_page + loc.page;
		}
		else {
			current_node->page = root->page;
		}
		convert_toc_tree(root->down, current_node->children, context, doc);


		//float xp, yp;
		//fz_layout_document(context, doc, 600, 800, 20);
		//fz_location loc = fz_resolve_link(context, doc, root->uri, &xp, &yp);

		output.push_back(current_node);
	} while (root = root->next);
}

void get_flat_toc(const std::vector<TocNode*>& roots, std::vector<std::wstring>& output, std::vector<int>& pages) {
	// Enumerate ToC nodes in the DFS order

	for (const auto& root : roots) {
		output.push_back(root->title);
		pages.push_back(root->page);
		get_flat_toc(root->children, output, pages);
	}
}

TocNode* get_toc_node_from_indices_helper(const std::vector<TocNode*>& roots, const std::vector<int>& indices, int pointer) {
	if (pointer < 0) {
		// should not happen
		assert(false);
	}
	if (pointer == 0) {
		return roots[indices[pointer]];
	}

	return get_toc_node_from_indices_helper(roots[indices[pointer]]->children, indices, pointer - 1);
}

TocNode* get_toc_node_from_indices(const std::vector<TocNode*>& roots, const std::vector<int>& indices) {
	return get_toc_node_from_indices_helper(roots, indices, indices.size() - 1);
}


QStandardItem* get_item_tree_from_toc_helper(const std::vector<TocNode*>& children, QStandardItem* parent) {

	for (const auto* child : children) {
		QStandardItem* child_item = new QStandardItem(QString::fromStdWString(child->title));
		child_item->setData(child->page);
		parent->appendRow(get_item_tree_from_toc_helper(child->children, child_item));
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

bool intersects(float range1_start, float range1_end, float range2_start, float range2_end) {
	if (range2_start > range1_end || range1_start > range2_end) {
		return false;
	}
	return true;
}

void parse_uri(std::string uri, int* page, float* offset_x, float* offset_y) {
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

void copy_to_clipboard(const std::wstring& text) {
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

int get_f_key(std::string name) {
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
	std::stringstream ss(name);
	ss >> num;
	return  num;
}

void show_error_message(std::wstring error_message) {
	MessageBoxW(nullptr, error_message.c_str(), L"Error", MB_OK);
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

void get_stext_block_string(fz_stext_block* block, std::wstring& res) {
	assert(block->type == FZ_STEXT_BLOCK_TEXT);

	LL_ITER(line, block->u.t.first_line) {
		LL_ITER(c, line->first_char) {
			res.push_back(c->c);
		}
	}
}

bool does_stext_block_starts_with_string(fz_stext_block* block, const std::wstring& str) {
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

bool is_consequtive(fz_rect rect1, fz_rect rect2) {
	float xdist = abs(rect1.x1 - rect2.x0);
	float ydist = abs(rect1.y0 - rect2.y0);

	if (xdist < 1.0f && ydist < 1.0f) {
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
void simplify_selected_character_rects(std::vector<fz_rect> selected_character_rects, std::vector<fz_rect>& resulting_rects) {
	
	if (selected_character_rects.size() == 0) {
		return;
	}

	std::vector<fz_rect> line_rects;

	fz_rect last_rect = selected_character_rects[0];
	line_rects.push_back(selected_character_rects[0]);

	for (int i = 1; i < selected_character_rects.size(); i++) {
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

}

void string_split(std::string haystack, std::string needle, std::vector<std::string> &res) {

	int loc = -1;
	int needle_size = needle.size();
	while ((loc = haystack.find(needle)) != -1) {
		int skiplen = loc + needle_size;
		if (loc != 0) {
			std::string part = haystack.substr(0, loc);
			res.push_back(part);
		}
		haystack = haystack.substr(skiplen, haystack.size() - skiplen);
	}
	if (haystack.size() > 0) {
		res.push_back(haystack);
	}
}

//void pdf_sandwich_maker(fz_context* context, std::wstring original_file_name, std::wstring sandwich_file_name) {
//
//	const char* utf8_encoded_output_name = utf8_encode(sandwich_file_name).c_str();
//
//	tesseract::TessBaseAPI* api = new tesseract::TessBaseAPI();
//	api->Init(nullptr, "eng");
//	tesseract::TessPDFRenderer* renderer = new tesseract::TessPDFRenderer(utf8_encoded_output_name, api->GetDatapath(), false);
//
//	fz_document* doc = fz_open_document(context, utf8_encode(original_file_name).c_str());
//
//	int num_pages = fz_count_pages(context, doc);
//
//	for (int i = 0; i < num_pages; i++) {
//		fz_page* page = fz_load_page(context, doc, i);
//		fz_pixmap* pixmap = fz_new_pixmap_from_page(context, page, fz_identity, fz_device_rgb(context), 0);
//
//		unsigned int width = pixmap->w;
//		unsigned int height = pixmap->h;
//
//		//Pix* pix;
//		//pix->data = pixmap->samples;
//		//pix->w = pixmap->w;
//		//pix->h = pixmap->h;
//
//		//api->SetImage()
//		//api->ProcessPage()
//
//		//api->SetImage(pixmap->samples, pixmap->w, pixmap->h, 3, pixmap->stride);
//		//api->Recognize(nullptr);
//		bool success = api->ProcessPages("data\\image.png", nullptr, 1000, renderer);
//
//		//std::cout << std::string(api->GetUTF8Text()) << "\n";
//
//		fz_drop_pixmap(context, pixmap);
//		fz_drop_page(context, page);
//
//	}
//	fz_drop_document(context, doc);
//	api->End();
//}
