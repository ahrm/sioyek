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

char get_symbol(SDL_Scancode scancode, bool is_shift_pressed) {
	char key = SDL_GetKeyFromScancode(scancode);
	if (key >= 'a' && key <= 'z') {
		if (is_shift_pressed) {
			return key + 'A' - 'a';
		}
		else {
			return key;
		}
	}
	return 0;
}

GLuint LoadShaders(filesystem::path vertex_file_path_, filesystem::path fragment_file_path_) {

	const wchar_t* vertex_file_path = vertex_file_path_.c_str();
	const wchar_t* fragment_file_path = fragment_file_path_.c_str();
	// Create the shaders
	GLuint VertexShaderID = glCreateShader(GL_VERTEX_SHADER);
	GLuint FragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);

	// Read the Vertex Shader code from the file
	std::string VertexShaderCode;
	std::ifstream VertexShaderStream(vertex_file_path, std::ios::in);
	if (VertexShaderStream.is_open()) {
		std::stringstream sstr;
		sstr << VertexShaderStream.rdbuf();
		VertexShaderCode = sstr.str();
		VertexShaderStream.close();
	}
	else {
		wprintf(L"Impossible to open %s. Are you in the right directory ? Don't forget to read the FAQ !\n", vertex_file_path);
		return 0;
	}

	// Read the Fragment Shader code from the file
	std::string FragmentShaderCode;
	std::ifstream FragmentShaderStream(fragment_file_path, std::ios::in);
	if (FragmentShaderStream.is_open()) {
		std::stringstream sstr;
		sstr << FragmentShaderStream.rdbuf();
		FragmentShaderCode = sstr.str();
		FragmentShaderStream.close();
	}
	else {
		wprintf(L"Impossible to open %s. Are you in the right directory ? Don't forget to read the FAQ !\n", fragment_file_path);
		return 0;
	}

	GLint Result = GL_FALSE;
	int InfoLogLength;

	// Compile Vertex Shader
	printf("Compiling shader : %s\n", vertex_file_path);
	char const* VertexSourcePointer = VertexShaderCode.c_str();
	glShaderSource(VertexShaderID, 1, &VertexSourcePointer, NULL);
	glCompileShader(VertexShaderID);

	// Check Vertex Shader
	glGetShaderiv(VertexShaderID, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(VertexShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	if (InfoLogLength > 0) {
		std::vector<char> VertexShaderErrorMessage(InfoLogLength + 1);
		glGetShaderInfoLog(VertexShaderID, InfoLogLength, NULL, &VertexShaderErrorMessage[0]);
		printf("%s\n", &VertexShaderErrorMessage[0]);
	}

	// Compile Fragment Shader
	printf("Compiling shader : %s\n", fragment_file_path);
	char const* FragmentSourcePointer = FragmentShaderCode.c_str();
	glShaderSource(FragmentShaderID, 1, &FragmentSourcePointer, NULL);
	glCompileShader(FragmentShaderID);

	// Check Fragment Shader
	glGetShaderiv(FragmentShaderID, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(FragmentShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	if (InfoLogLength > 0) {
		std::vector<char> FragmentShaderErrorMessage(InfoLogLength + 1);
		glGetShaderInfoLog(FragmentShaderID, InfoLogLength, NULL, &FragmentShaderErrorMessage[0]);
		printf("%s\n", &FragmentShaderErrorMessage[0]);
	}

	// Link the program
	printf("Linking program\n");
	GLuint ProgramID = glCreateProgram();
	glAttachShader(ProgramID, VertexShaderID);
	glAttachShader(ProgramID, FragmentShaderID);
	glLinkProgram(ProgramID);

	// Check the program
	glGetProgramiv(ProgramID, GL_LINK_STATUS, &Result);
	glGetProgramiv(ProgramID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	if (InfoLogLength > 0) {
		std::vector<char> ProgramErrorMessage(InfoLogLength + 1);
		glGetProgramInfoLog(ProgramID, InfoLogLength, NULL, &ProgramErrorMessage[0]);
		printf("%s\n", &ProgramErrorMessage[0]);
	}

	glDetachShader(ProgramID, VertexShaderID);
	glDetachShader(ProgramID, FragmentShaderID);

	glDeleteShader(VertexShaderID);
	glDeleteShader(FragmentShaderID);

	return ProgramID;
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

bool should_select_char(fz_point selection_begin, fz_point selection_end, fz_rect character_rect) {
	fz_rect selection_rect = corners_to_rect(selection_begin, selection_end);
	fz_point top_point = selection_begin.y > selection_end.y ? selection_end : selection_begin;
	fz_point bottom_point = selection_begin.y > selection_end.y ? selection_begin : selection_end;

	// if character is included in the selection y range, then it is selected
	if (selection_rect.y1 >= character_rect.y1 && selection_rect.y0 <= character_rect.y0) {
		return true;
	}

	else if (character_rect.y1 >= selection_rect.y1 && character_rect.y0 <= selection_rect.y0) {
		if (selection_rect.x1 >= character_rect.x0 && selection_rect.x0 <= character_rect.x1) {
			return true;
		}
	}
	else if (character_rect.y1 >= selection_rect.y1 && character_rect.y0 <= selection_rect.y1) {
		if (character_rect.x0 <= bottom_point.x) {
			return true;
		}
	}
	else if (character_rect.y1 >= selection_rect.y0 && character_rect.y0 <= selection_rect.y0) {
		if (character_rect.x1 >= top_point.x) {
			return true;
		}
	}
	return false;
}

//todo: see if it still works after wstring
//todo: free the memory!
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

wstring utf8_decode(string encoded_str) {
	wstring res;
	utf8::utf8to32(encoded_str.begin(), encoded_str.end(), std::back_inserter(res));
	return res;
}

string utf8_encode(wstring decoded_str) {
	string res;
	utf8::utf32to8(decoded_str.begin(), decoded_str.end(), std::back_inserter(res));
	return res;
}

