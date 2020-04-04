//todo: cleanup the code
//todo: do sth with the second window
//todo: toc
//todo: visibility test is still buggy??
//todo: threading

#include "imgui.h"
#include "imgui_impl_sdl.h"
#include "imgui_impl_opengl3.h"

#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>

#include <SDL.h>
#include <gl/glew.h>
#include <SDL_opengl.h>
#include <gl/GLU.h>

#include <Windows.h>
#include <mupdf/fitz.h>
#include "sqlite3.h"
#include <filesystem>

const float ZOOM_INC_FACTOR = 1.2f;
const unsigned int cache_invalid_milies = 1000;
const int page_paddings = 0;

const char* create_opened_books_sql = "CREATE TABLE IF NOT EXISTS opened_books ("\
	"id INTEGER PRIMARY KEY AUTOINCREMENT,"\
	"path TEXT UNIQUE,"\
	"zoom_level REAL,"\
	"offset_x REAL,"\
	"offset_y REAL);";

const char* insert_books_sql = ""\
"INSERT INTO opened_books (PATH, zoom_level, offset_x, offset_y) VALUES ";

const char* select_opened_books_sql = ""\
"SELECT zoom_level, offset_x, offset_y FROM opened_books WHERE (path=";

using namespace std;

GLfloat g_quad_vertex[] = {
	-1.0f, -1.0f,
	1.0f, -1.0f,
	-1.0f, 1.0f,
	1.0f, 1.0f
};

GLfloat g_quad_uvs[] = {
	0.0f, 0.0f,
	1.0f, 0.0f,
	0.0f, 1.0f,
	1.0f, 1.0f
};


static int null_callback(void* notused, int argc, char** argv, char** col_name) {
	return 0;
}
GLuint LoadShaders(const char* vertex_file_path, const char* fragment_file_path) {

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
		printf("Impossible to open %s. Are you in the right directory ? Don't forget to read the FAQ !\n", vertex_file_path);
		getchar();
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

void window_to_uv_scale(int window_width, int window_height, int document_width, int document_height, float* uv_x, float* uv_y) {
	*uv_x = ((float)(window_width)) / document_width;
	*uv_y = ((float)(window_height)) / document_height;
}

class Document {
public:
	fz_context* context;
	string file_name;
	fz_document* doc;
	vector<fz_rect> page_rects;

	Document(fz_context* context, string file_name) : context(context), file_name(file_name), doc(nullptr) {
	}

	~Document() {
		if (doc != nullptr) {
			fz_try(context) {
				fz_drop_document(context, doc);
			}
			fz_catch(context) {
				cout << "Error: could not drop documnet" << endl;
			}
		}
	}

	bool open() {
		if (doc == nullptr) {
			fz_try(context) {
				doc = fz_open_document(context, file_name.c_str());
			}
			fz_catch(context) {
				cout << "could not open " << file_name << endl;
			}
			if (doc != nullptr) {
				return true;
			}
			return false;
		}
		else {
			cout << "warning! calling open() on an open document" << endl;
			return false;
		}
	}

	int num_pages() {
		int pages = -1;
		fz_try(context) {
			pages = fz_count_pages(context, doc);
		}
		fz_catch(context) {
			cout << "could not count pages" << endl;
		}
		return pages;
	}

	fz_pixmap* get_page_pixmap(int page, float zoom_level = 1.0f) {
		fz_pixmap* res = nullptr;
		fz_try(context) {
			fz_matrix transform_matrix = fz_pre_scale(fz_identity, zoom_level, zoom_level);
			res = fz_new_pixmap_from_page_number(context, doc, page, transform_matrix, fz_device_rgb(context), 0);
		}
		fz_catch(context) {
			cout << "could not render pixmap for page " << page << endl;
		}
		return res;
	}
};

struct CachedPageData {
	Document* doc;
	int page;
	float zoom_level;
};
struct CachedPage {
	CachedPageData cached_page_data;
	fz_pixmap* cached_page_pixmap;
	unsigned int last_access_time;
	GLuint cached_page_texture;
};

bool operator==(const CachedPageData& lhs, const CachedPageData& rhs) {
	if (lhs.doc != rhs.doc) return false;
	if (lhs.page != rhs.page) return false;
	if (lhs.zoom_level != rhs.zoom_level) return false;
	return true;
}

struct OpenedBookState {
	float zoom_level;
	float offset_x;
	float offset_y;
};

static int opened_book_callback(void* res_vector, int argc, char** argv, char** col_name) {
	vector<OpenedBookState>* res = (vector<OpenedBookState>*) res_vector;

	if (argc != 3) {
		cout << "this should not happen!" << endl;
	}
	float zoom_level = atof(argv[0]);
	float offset_x = atof(argv[1]);
	float offset_y = atof(argv[2]);

	res->push_back(OpenedBookState{ zoom_level, offset_x, offset_y });

	return 0;
}

struct SearchResult {
	fz_quad quad;
	int page;
};

class WindowState {
private:
	Document* current_document;
	SDL_Window* main_window;
	SDL_Window* helper_window;
	SDL_GLContext* opengl_context;
	fz_context* mupdf_context;
	sqlite3* database;

	int current_page;
	float zoom_level;
	GLuint vertex_array_object;
	GLuint vertex_buffer_object;
	GLuint uv_buffer_object;
	GLuint gl_program;
	GLuint gl_debug_program;
	vector<float> page_heights;
	vector<float> page_widths;

	string current_document_path;

	// offset of the center of screen in the current document space
	float offset_x;
	float offset_y;


	int main_window_prev_width;
	int main_window_prev_height;

	unsigned int last_tick_time;

	vector<CachedPage> cached_pages;

	bool render_is_invalid;
	bool is_showing_ui;
	bool is_showing_searchbar;
	vector<SearchResult> search_results;
	int current_search_result_index;
	char search_string[100];

	void delete_old_cached_pages() {
		unsigned int now_milies = SDL_GetTicks();
		vector<int> indices_to_delete;

		for (int i = 0; i < cached_pages.size(); i++) {
			if ((now_milies - cached_pages[i].last_access_time) > cache_invalid_milies) {
				indices_to_delete.push_back(i);
			}
		}

		for (int j = indices_to_delete.size() - 1; j >= 0; j--) {
			CachedPage deleted_cached_page = cached_pages[indices_to_delete[j]];
			glDeleteTextures(1, &deleted_cached_page.cached_page_texture);

			fz_try(mupdf_context) {
				fz_drop_pixmap(mupdf_context, deleted_cached_page.cached_page_pixmap);
			}
			fz_catch(mupdf_context) {
				cout << "could not free cached pixmap" << endl;
			}

			cached_pages.erase(cached_pages.begin() + indices_to_delete[j]);
		}
	}

	CachedPage get_page_rendered(int page, float zoom_level, Document* doc=nullptr) {
		if (doc == nullptr) {
			doc = current_document;
		}
		CachedPageData page_data{ doc, page, zoom_level };
		for (auto& cached_page : cached_pages) {
			if (cached_page.cached_page_data == page_data) {
				cached_page.last_access_time = SDL_GetTicks();
				return cached_page;
			}
		}
		CachedPage cached_page;
		cached_page.cached_page_data = page_data;
		fz_pixmap* pixmap = doc->get_page_pixmap(page, zoom_level);
		GLuint page_texture;
		glGenTextures(1, &page_texture);
		glBindTexture(GL_TEXTURE_2D, page_texture);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);


		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
		glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
		glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, pixmap->w, pixmap->h, 0, GL_RGB, GL_UNSIGNED_BYTE, pixmap->samples);
		cached_page.cached_page_pixmap = pixmap;
		cached_page.cached_page_texture = page_texture;
		cached_page.last_access_time = SDL_GetTicks();
		cached_pages.push_back(cached_page);
		return cached_page;
	}

	CachedPage get_current_rendered_page() {
		cout << cached_pages.size() << endl;
		return get_page_rendered(current_page, zoom_level, current_document);
	}

	inline void set_offsets(float new_offset_x, float new_offset_y) {
		offset_x = new_offset_x;
		offset_y = new_offset_y;
		render_is_invalid = true;
	}

	inline void set_offset_x(float new_offset_x) {
		set_offsets(new_offset_x, offset_y);
	}

	inline void set_offset_y(float new_offset_y) {
		set_offsets(offset_x, new_offset_y);
	}

public:
	WindowState(SDL_Window* main_window,
		SDL_Window* helper_window,
		fz_context* mupdf_context,
		SDL_GLContext* opengl_context,
		sqlite3* database
	) :
		mupdf_context(mupdf_context),
		opengl_context(opengl_context),
		main_window(main_window),
		helper_window(helper_window),
		current_document(nullptr),
		current_page(0),
		zoom_level(1.0f),
		offset_x(0.0f),
		offset_y(0.0f),
		database(database),
		render_is_invalid(true),
		is_showing_ui(false),
		is_showing_searchbar(false),
		current_search_result_index(0)
	{

		gl_program = LoadShaders("shaders\\simple.vertex", "shaders\\simple.fragment");
		if (gl_program == 0) {
			cout << "Error: could not compile shaders" << endl;
		}

		gl_debug_program = LoadShaders("shaders\\simple.vertex", "shaders\\debug.fragment");
		if (gl_program == 0) {
			cout << "Error: could not compile debug shaders" << endl;
		}


		//unsigned char test_texture_data[] = {
		//	255, 0, 0, 0, 255, 0,
		//	0, 0, 255, 255, 0, 0
		//};

		//glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 2, 2, 0, GL_RGB, GL_UNSIGNED_BYTE, test_texture_data);

		glGenVertexArrays(1, &vertex_array_object);
		glBindVertexArray(vertex_array_object);

		glGenBuffers(1, &vertex_buffer_object);
		glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer_object);
		glBufferData(GL_ARRAY_BUFFER, sizeof(g_quad_vertex), g_quad_vertex, GL_DYNAMIC_DRAW);
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);

		glGenBuffers(1, &uv_buffer_object);
		glBindBuffer(GL_ARRAY_BUFFER, uv_buffer_object);
		glBufferData(GL_ARRAY_BUFFER, sizeof(g_quad_uvs), g_quad_uvs, GL_DYNAMIC_DRAW);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0);

		glDisable(GL_CULL_FACE);


		SDL_GetWindowSize(main_window, &main_window_prev_width, &main_window_prev_height);
		last_tick_time = SDL_GetTicks();
	}

	bool should_render() {
		if (render_is_invalid) {
			return true;
		}
		if (is_showing_ui) {
			return true;
		}
		return false;
	}

	void handle_return() {
		is_showing_searchbar = false;
		is_showing_ui = false;
		render_is_invalid = true;
		current_search_result_index = 0;
	}

	float zoom_in() {
		float old_zoom = zoom_level;
		zoom_level *= ZOOM_INC_FACTOR;
		float new_zoom = zoom_level;
		readjust_offsets_after_zoom_change(old_zoom, new_zoom);
		render_is_invalid = true;
		return zoom_level;
	}

	float zoom_out() {
		float old_zoom = zoom_level;
		zoom_level /= ZOOM_INC_FACTOR;
		float new_zoom = zoom_level;
		readjust_offsets_after_zoom_change(old_zoom, new_zoom);
		render_is_invalid = true;
		return zoom_level;
	}

	void move_absolute(float dx, float dy) {
		set_offsets(offset_x + dx, offset_y + dy);
	}

	void move(float dx, float dy) {
		int abs_dx = (dx / zoom_level);
		int abs_dy = (dy / zoom_level);
		move_absolute(abs_dx, abs_dy);
	}

	void move_pages(int num_pages) {
		int current_page = get_current_page_number();
		if (current_page == -1) {
			current_page = 0;
		}

		move_absolute(0, num_pages * (page_heights[current_page] + page_paddings));
	}

	void reset_doc_state() {
		current_page = 0;
		zoom_level = 1.0f;
		set_offsets(0.0f, 0.0f);
	}
	void open_document(string doc_path) {

		string cannonical_path = std::filesystem::canonical(doc_path).string();
		current_document_path = cannonical_path;
		stringstream document_query_ss;
		document_query_ss << "select zoom_level, offset_x, offset_y from opened_books where path='" << cannonical_path << "'";
		char* error_message;
		int rc;

		vector<OpenedBookState> prev_state;
		rc = sqlite3_exec(database, document_query_ss.str().c_str(), opened_book_callback, &prev_state, &error_message);

		if (rc != SQLITE_OK) {
			cout << "there was an error in finding open file state: " << error_message << endl;
			sqlite3_free(error_message);
		}
		if (prev_state.size() > 1) {
			cout << "more than one file with one path, this should not happen!" << endl;
	 	}



		if (current_document != nullptr) {
			delete current_document;
		}

		current_document = new Document(mupdf_context, doc_path);
		current_document->open();
		reset_doc_state();
		if (prev_state.size() > 0) {
			OpenedBookState previous_state = prev_state[0];
			zoom_level = previous_state.zoom_level;
			offset_x = previous_state.offset_x;
			offset_y = previous_state.offset_y;
			set_offsets(previous_state.offset_x, previous_state.offset_y);
		}

		page_heights.clear();
		page_widths.clear();

		int page_count = current_document->num_pages();

		//todo: better (or any!) error handling
		for (int i = 0; i < page_count; i++) {
			fz_page* page = fz_load_page(mupdf_context, current_document->doc, i);
			fz_rect page_rect = fz_bound_page(mupdf_context, page);
			page_heights.push_back(page_rect.y1 - page_rect.y0);
			page_widths.push_back(page_rect.x1 - page_rect.x0);
			fz_drop_page(mupdf_context, page);
		}

		int window_width, window_height;
		SDL_GetWindowSize(main_window, &window_width, &window_height);
		//offset_x = window_width / (2*zoom_level);
		//offset_y = window_height / zoom_level;
		SDL_SetWindowTitle(main_window, current_document_path.c_str());

	}


	//keep the center the same
	void readjust_offsets_after_window_size_change(int old_window_width, int old_window_height, int new_window_width, int new_window_height) {
		//old_offset_x * zoom + old_window_x/2 = new_offset_x * zoom + new_window_x/2
		//new_offset_x * zoom = old_offset_x * zoom + (old_window_x - new_window_x)/2
		//offset_x -= (old_window_width - new_window_width) / (2 * zoom_level);
		//offset_y -= (old_window_height - new_window_height) / (2 * zoom_level);
	}

	int get_current_page_number() {
		vector<int> visible_pages;
		int window_height, window_width;
		get_visible_pages(100, visible_pages);
		if (visible_pages.size() > 0) {
			return visible_pages[0];
		}
		return -1;
	}


	void readjust_offsets_after_zoom_change(float old_zoom, float new_zoom) {
		//old_offset_x * old_zoom + window_x/2 = new_offset_x * new_zoom + window_x/2
		//new_offset_x = (old_offset_x * old_zoom ) / new_zoom;
		//offset_x *= old_zoom / new_zoom;
		//offset_y *= old_zoom / new_zoom;
	}

	void on_main_window_size_changed() {
		int new_width, new_height;
		SDL_GetWindowSize(main_window, &new_width, &new_height);

		if ((new_width != main_window_prev_width) || (new_height != main_window_prev_height)) {
			readjust_offsets_after_window_size_change(main_window_prev_width, main_window_prev_height, new_width, new_height);
			main_window_prev_width = new_width;
			main_window_prev_height = new_height;
		}
		render_is_invalid = true;
	}

	void get_visible_pages(int window_height, vector<int> &visible_pages) {
		//int current_y_offset = offset_y * zoom_level;
		//for (int i = 0; i < page_heights.size(); i++) {
		//	bool is_visible = false;
		//	if ((current_y_offset >= -window_height) && (current_y_offset <= 2*window_height)) {
		//		is_visible = true;
		//	}

		//	current_y_offset -= page_heights[i] * zoom_level;

		//	if ((current_y_offset >= -window_height) && (current_y_offset <= 2*window_height)) {
		//		is_visible = true;
		//	}
		//	if (is_visible) {
		//		visible_pages.push_back(i);
		//	}
		//}
		float window_y_range_begin = offset_y - window_height / (zoom_level);
		float window_y_range_end = offset_y + window_height / (zoom_level);
		window_y_range_begin -= 1;
		window_y_range_end += 1;

		float page_begin = 0.0f;

		for (int i = 0; i < page_heights.size(); i++) {
			float page_end = page_begin + page_heights[i];

			if (intersects(window_y_range_begin, window_y_range_end, page_begin, page_end)){
				visible_pages.push_back(i);
			}
			page_begin = page_end;
		}
	}

	bool intersects(float range1_start, float range1_end, float range2_start, float range2_end) {
		if (range2_start > range1_end || range1_start > range2_end) {
			return false;
		}
		return true;
	}

	void get_relative_rect(SDL_Window* window, int page_width, int page_height, float output_vertices[8], int additional_offset) {
		int window_width, window_height;
		SDL_GetWindowSize(window, &window_width, &window_height);
		float x0 = 2 * static_cast<float>(offset_x * zoom_level - page_width  / 2) / window_width;
		float x1 = 2 * static_cast<float>(offset_x * zoom_level + page_width  /2) / window_width;
		float y0 = 2 * static_cast<float>(offset_y * zoom_level + additional_offset) / window_height ;
		float y1 = 2 * static_cast<float>(offset_y * zoom_level + additional_offset - page_height ) / window_height;

		//y0 = y0 * 2 - 1;
		//y1 = y1 * 2 - 1;
		//x0 = x0 * 2 - 1;
		//x1 = x1 * 2 - 1;

		output_vertices[0] = x0;
		output_vertices[1] = y0;

		output_vertices[2] = x1;
		output_vertices[3] = y0;

		output_vertices[4] = x0;
		output_vertices[5] = y1;

		output_vertices[6] = x1;
		output_vertices[7] = y1;

	}

	fz_rect document_to_window_rect(int window_width, int window_height, int page, fz_rect doc_rect) {
		double doc_rect_y_offset = 0.0f;
		for (int i = 0; i < page; i++) {
			doc_rect_y_offset -= page_heights[i];
		}
		doc_rect.y0 = page_heights[page] - doc_rect.y0;
		doc_rect.y1 = page_heights[page] - doc_rect.y1;

		doc_rect.y0 += doc_rect_y_offset;
		doc_rect.y1 += doc_rect_y_offset;

		fz_rect window_rect;
		float window_width_in_doc_space = static_cast<float>(window_width) / zoom_level;
		float window_height_in_doc_space = static_cast<float>(window_height) / zoom_level;

		window_rect.x0 = offset_x - window_width_in_doc_space / 2;
		window_rect.x1 = offset_x + window_width_in_doc_space / 2;

		window_rect.y0 = offset_y - window_height_in_doc_space / 2;
		window_rect.y1 = offset_y + window_height_in_doc_space / 2;

		fz_rect transformed_doc_rect;
		transformed_doc_rect.x0 = doc_rect.x0 + offset_x - page_widths[page] / 2;
		transformed_doc_rect.x1 = doc_rect.x1 + offset_x - page_widths[page] / 2;

		transformed_doc_rect.y0 = doc_rect.y0 + offset_y - page_heights[page];
		transformed_doc_rect.y1 = doc_rect.y1 + offset_y - page_heights[page];

		transformed_doc_rect.x0 /= (window_rect.x1 - offset_x);
		transformed_doc_rect.x1 /= (window_rect.x1 - offset_x);

		transformed_doc_rect.y0 /= (window_rect.y1 - offset_y);
		transformed_doc_rect.y1 /= (window_rect.y1 - offset_y);

		return transformed_doc_rect;
	}

	float get_page_additional_offset(int page_number) {
		double offset = 0;
		for (int i = 0; i < page_number; i++) {
			float real_page_height = (page_heights[i] * zoom_level);
			offset -= real_page_height;
			offset -= page_paddings;
		}
		return offset;
	}
	void render_page(int page_number) {
		int window_width, window_height;
		SDL_GetWindowSize(main_window, &window_width, &window_height);

		CachedPage current_cached_page = get_page_rendered(page_number, zoom_level);

		int page_width = current_cached_page.cached_page_pixmap->w;
		int page_height = current_cached_page.cached_page_pixmap->h;

		int additional_offset = get_page_additional_offset(page_number);
		float page_vertices[4 * 2];
		get_relative_rect(main_window, page_width, page_height, page_vertices, additional_offset);

		glUseProgram(gl_program);
		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);
		glBindVertexArray(vertex_array_object);
		glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer_object);
		glBufferData(GL_ARRAY_BUFFER, sizeof(page_vertices), page_vertices, GL_DYNAMIC_DRAW);

		glBindTexture(GL_TEXTURE_2D, current_cached_page.cached_page_texture);

		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	}

	void tick(bool force=false) {
		unsigned int now = SDL_GetTicks();
		if (force || (now - last_tick_time) > 1000) {
			last_tick_time = now;
			stringstream update_book_state_sql;
			//update_book_state_sql << "update opened_books set zoom_level=" << zoom_level << " , offset_x="
			//	<< offset_x << ", offset_y=" << offset_y << " where path='" << current_document_path << "';";
			update_book_state_sql << "insert or replace into opened_books(path, zoom_level, offset_x, offset_y) values ('" <<
				current_document_path << "', " << zoom_level << ", " << offset_x << ", " << offset_y << ");";

			char* error_message;
			
			int rc = sqlite3_exec(database, update_book_state_sql.str().c_str(), null_callback, 0, &error_message);

			if (rc != SQLITE_OK) {
				cout << "could not update opened document: " << error_message << endl;
				sqlite3_free(error_message);
			}
		}
	}

	void update_window_title() {
		stringstream title;
		title << current_document_path << " ( page " << get_current_page_number() << " / " << current_document->num_pages() <<  " )";
		SDL_SetWindowTitle(main_window, title.str().c_str());
	}

	int search_text(const char* text, vector<SearchResult> &results) {

		int num_pages = current_document->num_pages();
		int total_results = 0;

		for (int i = 0; i < num_pages; i++) {
			fz_page* page = fz_load_page(mupdf_context, current_document->doc, i);

			const int max_hits_per_page = 20;
			fz_quad hitboxes[max_hits_per_page];
			int num_results = fz_search_page(mupdf_context, page, text, hitboxes, max_hits_per_page);

			for (int j = 0; j < num_results; j++) {
				results.push_back(SearchResult{ hitboxes[j], i });
			}

			total_results += num_results;

			fz_drop_page(mupdf_context, page);
		}
		return total_results;
	}

	void show_searchbar() {
		is_showing_ui = true;
		is_showing_searchbar = true;
		ZeroMemory(search_string, sizeof(search_string));
	}

	void goto_search_result(int offset) {
		if (search_results.size() == 0) {
			return;
		}

		int target_index = (current_search_result_index + offset) % search_results.size();
		current_search_result_index = target_index;
		
		int target_page = search_results[target_index].page;

		fz_quad quad = search_results[target_index].quad;

		float new_offset_y = quad.ll.y;
		for (int i = 0; i < target_page; i++) {
			new_offset_y += page_heights[i] + page_paddings;
		}

		set_offset_y(new_offset_y);

		//float new_offset_y = page_hei
	}

	void render() {
		if (should_render()) {
			render_is_invalid = false;
			cout << "rendering ..." << endl;


			ImGuiIO& io = ImGui::GetIO(); (void)io;

			update_window_title();

			SDL_GL_MakeCurrent(main_window, *opengl_context);
			int window_width, window_height;
			SDL_GetWindowSize(main_window, &window_width, &window_height);
			glViewport(0, 0, window_width, window_height);

			vector<int> visible_pages;
			get_visible_pages(window_height, visible_pages);


			glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT);

			for (int j = 0; j < visible_pages.size(); j++) {
				render_page(visible_pages[j]);
			}

			{
				ImGui_ImplOpenGL3_NewFrame();
				ImGui_ImplSDL2_NewFrame(main_window);
				ImGui::NewFrame();

				if (is_showing_searchbar) {

					ImGui::SetNextWindowPos(ImVec2(0, 0));
					ImGui::SetNextWindowSize(ImVec2(window_width, 60));
					ImGui::Begin("Search");
					//ImGui::SetKeyboardFocusHere();
					ImGui::SetKeyboardFocusHere();
					if (ImGui::InputText("Search Text", search_string, sizeof(search_string), ImGuiInputTextFlags_EnterReturnsTrue)) {
						search_results.clear();
						search_text(search_string, search_results);
						handle_return();
						io.WantCaptureKeyboard = false;
					}
					ImGui::End();
				}

				ImGui::Render();
				ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

			}

			if (search_results.size() > 0) {
				SearchResult current_search_result = search_results[current_search_result_index];
				fz_quad result_quad = current_search_result.quad;
				fz_rect result_rect;
				result_rect.x0 = result_quad.ul.x;
				result_rect.x1 = result_quad.ur.x;

				result_rect.y0 = result_quad.ul.y;
				result_rect.y1 = result_quad.ll.y;

				fz_rect window_rect = document_to_window_rect(window_width, window_height, current_search_result.page, result_rect);
				float quad_vertex_data[] = {
					window_rect.x0, window_rect.y1,
					window_rect.x1, window_rect.y1,
					window_rect.x0, window_rect.y0,
					window_rect.x1, window_rect.y0
				};

				glEnable(GL_BLEND);
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
				glUseProgram(gl_debug_program);
				glEnableVertexAttribArray(0);
				glEnableVertexAttribArray(1);
				glBindVertexArray(vertex_array_object);
				glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer_object);
				glBufferData(GL_ARRAY_BUFFER, sizeof(quad_vertex_data), quad_vertex_data, GL_DYNAMIC_DRAW);
				glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
				glDisable(GL_BLEND);

			}


			SDL_GL_SwapWindow(main_window);

			SDL_GL_MakeCurrent(helper_window, *opengl_context);

			SDL_GetWindowSize(helper_window, &window_width, &window_height);
			glViewport(0, 0, window_width, window_height);

			glClearColor(1.0f, 0.0f, 0.0f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT);
			SDL_GL_SwapWindow(helper_window);
			delete_old_cached_pages();
		}
	}
};


void hanle_keys(unsigned char key, int x, int y) {
	if (key == 'q') {

	}
}

static int callback(void* notused, int argc, char** argv, char** col_name) {
	for (int i = 0; i < argc; i++) {
		cout << col_name[i] << " " << argv[i] << endl;
	}
	return 0;
}


//int main(int argc, char* args[]) {
//	sqlite3* db;
//	char* zErrMsg = nullptr;
//	int rc;
//	rc = sqlite3_open("test.db", &db);
//
//	if (rc) {
//		cout << "could not open database: " << sqlite3_errmsg(db) << endl;
//		return -1;
//	}

	//string create_table_sql = "CREATE TABLE opened_books("\
	//	"ID INT PRIMARY KEY NOT NULL,"\
	//	"PATH TEXT NOT NULL UNIQUE);";

	//rc = sqlite3_exec(db, create_table_sql.c_str(), callback, 0, &zErrMsg);
	//if (rc != SQLITE_OK) {
	//	cout << "could not create table: " << sqlite3_errmsg(db) << endl;
	//}
	//string insert_sql = ""\
	//	"INSERT INTO opened_books (ID, PATH) VALUES (1, 'C:\\somepath\\somefile.pdf');"\
	//	"INSERT INTO opened_books (ID, PATH) VALUES (2, 'C:\\anotherpath\\anotherfile.pdf');";

	//rc = sqlite3_exec(db, insert_sql.c_str(), callback, 0, &zErrMsg);
	//if (rc != SQLITE_OK) {
	//	cout << "could not insert into table: " << zErrMsg << endl;
	//	sqlite3_free(zErrMsg);
	//}


//	sqlite3_close(db);
//
//	return 0;
//}


bool select_pdf_file_name(char* out_file_name, int max_length) {

	OPENFILENAMEA ofn;
	ZeroMemory(out_file_name, max_length);
	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = nullptr;
	ofn.lpstrFilter = "Pdf Files\0*.pdf\0Any File\0*.*\0";
	ofn.lpstrFile = out_file_name;
	ofn.nMaxFile = max_length;
	ofn.lpstrTitle = "Select a document";
	ofn.Flags = OFN_DONTADDTORECENT | OFN_FILEMUSTEXIST;
	
	
	if (GetOpenFileNameA(&ofn)) {
		return true;
	}
	return false;
}

int main(int argc, char* args[]) {

	sqlite3* db;
	char* error_message = nullptr;
	int rc;

	rc = sqlite3_open("test.db", &db);
	if (rc) {
		cout << "could not open database" << sqlite3_errmsg(db) << endl;
	}


	rc = sqlite3_exec(db, create_opened_books_sql, null_callback, 0, &error_message);
	if (rc != SQLITE_OK) {
		cout << "could not create opened books table: " << error_message << endl;
		sqlite3_free(error_message);
	}

	fz_context* mupdf_context = fz_new_context(nullptr, nullptr, FZ_STORE_UNLIMITED);
	if (!mupdf_context) {
		cout << "could not create mupdf context" << endl;
		return -1;
	}
	bool fail = false;
	fz_try(mupdf_context) {
		fz_register_document_handlers(mupdf_context);
	}
	fz_catch(mupdf_context) {
		cout << "could not register document handlers" << endl;
		fail = true;
	}

	if (fail) {
		return -1;
	}


	SDL_Window* window = nullptr;

	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		cout << "could not initialize SDL" << endl;
		return -1;
	}

	window = SDL_CreateWindow("Pdf Viewer", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 500, 500, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
	SDL_Window* window2 = SDL_CreateWindow("Pdf Viewer2", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 500, 500, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);

	if (window == nullptr) {
		cout << "could not create SDL window" << endl;
		return -1;
	}
	if (window2 == nullptr) {
		cout << "could not create the second window" << endl;
		return -1;
	}

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 1);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

	SDL_GLContext gl_context = SDL_GL_CreateContext(window);

	if (gl_context == nullptr){
		cout << SDL_GetError() << endl;
		cout << "could not create opengl context" << endl;
		return -1;
	}

	glewExperimental = true;
	GLenum glew_error = glewInit();
	if (glew_error != GLEW_OK) {
		cout << "could not initialize glew" << endl;
		return -1;
	}

	if (SDL_GL_SetSwapInterval(1) < 0) {
		cout << "could not enable vsync" << endl;
		return -1;
	}

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	ImGui::StyleColorsDark();
	ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
	ImGui_ImplOpenGL3_Init("#version 400");

	WindowState window_state(window, window2, mupdf_context, &gl_context, db);
	window_state.open_document("data\\test.pdf");

	bool quit = false;

	while (!quit) {
		SDL_Event event;

		while (SDL_PollEvent(&event)) {
			ImGui_ImplSDL2_ProcessEvent(&event);

			if ((event.key.type == SDL_KEYDOWN) && io.WantCaptureKeyboard) {
				continue;
			}

			//render_invalidated = true;
			if (event.type == SDL_QUIT) {
				quit = true;
			}
			if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE) {
				quit = true;
			}
			if (event.type == SDL_WINDOWEVENT && ((event.window.event == SDL_WINDOWEVENT_RESIZED)
				|| (event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED))) {
				window_state.on_main_window_size_changed();
			}
			if (event.type == SDL_KEYDOWN) {
				if (event.key.keysym.sym == SDLK_EQUALS) {
					window_state.zoom_in();
				}
				if (event.key.keysym.sym == SDLK_MINUS) {
					window_state.zoom_out();
				}

				if (event.key.keysym.sym == SDLK_LEFT) {
					window_state.move(72, 0);
				}
				if (event.key.keysym.sym == SDLK_RIGHT) {
					window_state.move(-72, 0);
				}

				if (event.key.keysym.sym == SDLK_DOWN) {
					window_state.move(0, 72);
				}
				if (event.key.keysym.sym == SDLK_UP) {
					window_state.move(0, -72);
				}

				if (event.key.keysym.sym == SDLK_COMMA) {
					window_state.move_pages(-1);
				}
				if (event.key.keysym.sym == SDLK_PERIOD) {
					window_state.move_pages(1);
				}

				if (event.key.keysym.sym == SDLK_n) {
					window_state.goto_search_result(1);
				}

				if (event.key.keysym.sym == SDLK_p) {
					window_state.goto_search_result(-1);
				}

				if (event.key.keysym.sym == SDLK_SLASH) {
					//vector<SearchResult> results;
					//window_state.search_text("machine", results);
					//cout << results.size() << endl;
					window_state.show_searchbar();
				}
				
				if (event.key.keysym.sym == SDLK_o) {
					char file_name[MAX_PATH];
					if (select_pdf_file_name(file_name, MAX_PATH)) {
						window_state.open_document(file_name);
					}
				}
				//if (zoom_level_changed) {
				//	fz_drop_pixmap(mupdf_context, pixmap);
				//	pixmap = window_state.get_current_pixmap();

				//	glBindTexture(GL_TEXTURE_2D, pdf_texture);
				//	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, pixmap->w, pixmap->h, 0, GL_RGB, GL_UNSIGNED_BYTE, pixmap->samples);

				//}
			}

		}

		//if (render_invalidated) {
		window_state.render();
		//}

		SDL_Delay(16);
		window_state.tick();
	}

	//glUseProgram(gl_program);
	//glEnableVertexAttribArray(0);

	//glVertexAttrib2fv(0, )
	sqlite3_close(db);



	return 0;
}