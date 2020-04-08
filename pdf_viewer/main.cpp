//todo: cleanup the code considerably!
//todo: do sth with the second window
//todo: toc
//todo: visibility test is still buggy??
//todo: threading
//todo: remove some O(n) things from page height computations

#include "imgui.h"
#include "imgui_impl_sdl.h"
#include "imgui_impl_opengl3.h"

#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <thread>
#include <mutex>

#include <SDL.h>
#include <gl/glew.h>
#include <SDL_opengl.h>
#include <gl/GLU.h>

#include <Windows.h>
#include <mupdf/fitz.h>
#include "sqlite3.h"
#include <filesystem>

#include "input.h"
#include "database.h"
#include "book.h"

const float ZOOM_INC_FACTOR = 1.2f;
const unsigned int cache_invalid_milies = 1000;
const int page_paddings = 0;
const int max_pending_requests = 30;
const char* last_path_file_absolute_location = "C:\\Users\\Lion\\source\\repos\\pdf_viewer\\pdf_viewer\\last_document_path.txt";


using namespace std;

mutex mupdf_mutexes[FZ_LOCK_MAX];

void lock_mutex(void* user, int lock) {
	mutex* mut = (mutex*)user;
	(mut + lock)->lock();
}

void unlock_mutex(void* user, int lock) {
	mutex* mut = (mutex*)user;
	(mut + lock)->unlock();
}

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
		printf("Impossible to open %s. Are you in the right directory ? Don't forget to read the FAQ !\n", fragment_file_path);
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

void window_to_uv_scale(int window_width, int window_height, int document_width, int document_height, float* uv_x, float* uv_y) {
	*uv_x = ((float)(window_width)) / document_width;
	*uv_y = ((float)(window_height)) / document_height;
}

struct SearchResult {
	fz_quad quad;
	int page;
};

struct RenderRequest {
	fz_document* doc;
	int page;
	float zoom_level;
};

struct RenderResponse {
	RenderRequest request;
	unsigned int last_access_time;
	fz_pixmap* pixmap;
	GLuint texture;
};

bool operator==(const RenderRequest& lhs, const RenderRequest& rhs) {
	if (rhs.doc != lhs.doc) {
		return false;
	}
	if (rhs.page != lhs.page) {
		return false;
	}
	if (rhs.zoom_level != lhs.zoom_level) {
		return false;
	}
	return true;
}

class PdfRenderer {
	fz_context* context_to_clone;
	fz_context* mupdf_context;
	vector<fz_pixmap*> pixmaps_to_drop;
	unordered_map<string, fz_document*> opened_documents;
	vector<RenderRequest> pending_requests;
	vector<RenderResponse> cached_responses;
	mutex pending_requests_mutex;
	mutex cached_response_mutex;
	mutex pixmap_drop_mutex;
	bool* invalidate_pointer;

public:

	PdfRenderer(fz_context* context_to_clone) : context_to_clone(context_to_clone) {
		invalidate_pointer = nullptr;
		mupdf_context = nullptr;
	}

	void init_context() {
		if (mupdf_context == nullptr) {
			mupdf_context = fz_clone_context(context_to_clone);
			context_to_clone = nullptr;
		}
	}



	void set_invalidate_pointer(bool* inv_p) {
		invalidate_pointer = inv_p;
	}

	fz_document* get_document_with_path(string path) {
		if (opened_documents.find(path) != opened_documents.end()) {
			return opened_documents.at(path);
		}
		fz_document* ret_val = nullptr;
		fz_try(mupdf_context) {
			ret_val = fz_open_document(mupdf_context, path.c_str());
			opened_documents[path] = ret_val;
		}
		fz_catch(mupdf_context) {
			cout << "Error: could not open document" << endl;
		}

		return ret_val;
	}
	//should only be called from the main thread
	void add_request(string document_path, int page, float zoom_level) {
		fz_document* doc = get_document_with_path(document_path);
		if (doc != nullptr) {
			RenderRequest req;
			req.doc = doc;
			req.page = page;
			req.zoom_level = zoom_level;

			pending_requests_mutex.lock();
			pending_requests.push_back(req);
			if (pending_requests.size() > max_pending_requests) {
				pending_requests.erase(pending_requests.begin());
			}
			pending_requests_mutex.unlock();
		}
		else {
			cout << "Error: could not find documnet" << endl;
		}
	}

	//should only be called from the main thread
	GLuint find_rendered_page(string path, int page, float zoom_level, int* page_width, int* page_height) {
		fz_document* doc = get_document_with_path(path);
		if (doc != nullptr) {
			RenderRequest req;
			req.doc = doc;
			req.page = page;
			req.zoom_level = zoom_level;
			cached_response_mutex.lock();
			GLuint result = 0;
			for (auto& cached_resp : cached_responses) {
				if (cached_resp.request == req) {
					cached_resp.last_access_time = SDL_GetTicks();

					*page_width = cached_resp.pixmap->w;
					*page_height = cached_resp.pixmap->h;

					if (cached_resp.texture != 0) {
						result = cached_resp.texture;
					}
					else {
						glGenTextures(1, &result);
						glBindTexture(GL_TEXTURE_2D, result);

						glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
						glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
						glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
						glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);


						glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
						glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
						glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
						glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);
						glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, cached_resp.pixmap->w, cached_resp.pixmap->h, 0, GL_RGB, GL_UNSIGNED_BYTE, cached_resp.pixmap->samples);
						cout << "texture: " << result << endl;
						cached_resp.texture = result;
					}
					break;
				}
			}
			cached_response_mutex.unlock();
			if (result == 0) {
				add_request(path, page, zoom_level);
				return try_closest_rendered_page(doc, page, zoom_level, page_width, page_height);
			}
			return result;
		}
		return 0;
	}

	GLuint try_closest_rendered_page(fz_document* doc, int page, float zoom_level, int* page_width, int* page_height) {
		cached_response_mutex.lock();

		float min_diff = 10000.0f;
		GLuint best_texture = 0;

		for (const auto& cached_resp : cached_responses) {
			if ((cached_resp.request.doc == doc) && (cached_resp.request.page == page) && (cached_resp.texture != 0)) {
				float diff = cached_resp.request.zoom_level - zoom_level;
				if (diff < min_diff) {
					min_diff = diff;
					best_texture = cached_resp.texture;
					*page_width = static_cast<int>(cached_resp.pixmap->w * zoom_level / cached_resp.request.zoom_level);
					*page_height = static_cast<int>(cached_resp.pixmap->h * zoom_level / cached_resp.request.zoom_level);
				}
			}
		}
		cached_response_mutex.unlock();
		return best_texture;
	}

	//should only be called from main thread
	void delete_old_pages() {
		cached_response_mutex.lock();
		vector<int> indices_to_delete;
		unsigned int now = SDL_GetTicks();

		for (int i = 0; i < cached_responses.size(); i++) {
			if ((now - cached_responses[i].last_access_time) > cache_invalid_milies) {
				cout << "deleting cached texture ... " << endl;
				indices_to_delete.push_back(i);
			}
		}

		for (int j = indices_to_delete.size() - 1; j >= 0; j--) {
			int index_to_delete = indices_to_delete[j];
			RenderResponse resp = cached_responses[index_to_delete];
			//fz_try(mupdf_context) {
			//	fz_drop_pixmap(mupdf_context, resp.pixmap);
			//}
			//fz_catch(mupdf_context) {
			//	cout << "Error: could not free pixmap" << endl;
			//}
			pixmap_drop_mutex.lock();
			pixmaps_to_drop.push_back(resp.pixmap);
			pixmap_drop_mutex.unlock();

			if (resp.texture != 0) {
				glDeleteTextures(1, &resp.texture);
			}

			cached_responses.erase(cached_responses.begin() + index_to_delete);
		}
		cached_response_mutex.unlock();
	}

	void run() {
		init_context();

		while (true) {
			pending_requests_mutex.lock();

			while (pending_requests.size() == 0) {
				pending_requests_mutex.unlock();

				pixmap_drop_mutex.lock();
				for (int i = 0; i < pixmaps_to_drop.size(); i++) {
					fz_try(mupdf_context) {
						fz_drop_pixmap(mupdf_context, pixmaps_to_drop[i]);
					}
					fz_catch(mupdf_context) {
						cout << "Error: could not drop pixmap" << endl;
					}
				}
				pixmaps_to_drop.clear();
				pixmap_drop_mutex.unlock();

				Sleep(100);
				pending_requests_mutex.lock();
			}
			cout << "worker thread running ... pending requests: " << pending_requests.size() << endl;

			RenderRequest req = pending_requests[pending_requests.size() - 1];
			cached_response_mutex.lock();
			bool is_already_rendered = false;
			for (const auto& cached_rep : cached_responses) {
				if (cached_rep.request == req) is_already_rendered = true;
			}
			cached_response_mutex.unlock();
			pending_requests.pop_back();
			pending_requests_mutex.unlock();
			cout << "user: " << mupdf_context->locks.user << endl;

			if (!is_already_rendered) {

				fz_try(mupdf_context) {
					fz_matrix transform_matrix = fz_pre_scale(fz_identity, req.zoom_level, req.zoom_level);
					fz_pixmap* rendered_pixmap = fz_new_pixmap_from_page_number(mupdf_context, req.doc, req.page, transform_matrix, fz_device_rgb(mupdf_context), 0);
					RenderResponse resp;
					resp.request = req;
					resp.last_access_time = SDL_GetTicks();
					resp.pixmap = rendered_pixmap;
					resp.texture = 0;

					cached_response_mutex.lock();
					cached_responses.push_back(resp);
					cached_response_mutex.unlock();
					if (invalidate_pointer != nullptr) {
						//todo: there might be a race condition here
						*invalidate_pointer = true;
					}

				}
				fz_catch(mupdf_context) {
					cout << "Error: could not render page" << endl;
				}
			}

		}
	}

	~PdfRenderer() {
	}

};

PdfRenderer* global_pdf_renderer;




struct TocNode {
	vector<TocNode*> children;
	string title;
	int page;
	float y;
	float x;
};

void convert_toc_tree(fz_outline* root, vector<TocNode*>& output) {

	do {
		if (root == nullptr) {
			break;
		}

		TocNode* current_node = new TocNode;
		current_node->title = root->title;
		current_node->page = root->page;
		current_node->x = root->x;
		current_node->y = root->y;
		convert_toc_tree(root->down, current_node->children);
		output.push_back(current_node);
	} while (root = root->next);

}

void get_flat_toc(const vector<TocNode*>& roots, vector<string>& output, vector<int>& pages) {
	for (const auto& root : roots) {
		output.push_back(root->title);
		pages.push_back(root->page);
		get_flat_toc(root->children, output, pages);
	}
}

class Document {
private:
	vector<Mark> marks;
	vector<BookMark> bookmarks;
	sqlite3* db;
	vector<TocNode*> top_level_toc_nodes;

	int get_mark_index(char symbol) {
		for (int i = 0; i < marks.size(); i++) {
			if (marks[i].symbol == symbol) {
				return i;
			}
		}
		return -1;
	}

public:
	fz_context* context;
	string file_name;
	fz_document* doc;
	vector<fz_rect> page_rects;

	void add_bookmark(string desc, float y_offset) {
		BookMark res;
		res.description = desc;
		res.y_offset = y_offset;
		bookmarks.push_back(res);
		insert_bookmark(db, file_name, desc, y_offset);
	}

	const vector<BookMark>& get_bookmarks() const {
		return bookmarks;
	}

	void add_mark(char symbol, float y_offset) {
		int current_mark_index = get_mark_index(symbol);
		if (current_mark_index == -1) {
			marks.push_back({ y_offset, symbol });
			insert_mark(db, file_name, symbol, y_offset);
		}
		else {
			marks[current_mark_index].y_offset = y_offset;
			update_mark(db, file_name, symbol, y_offset);
		}
	}

	void create_toc_tree(vector<TocNode*>& toc) {
		fz_try(context) {
			fz_outline* outline = fz_load_outline(context, doc);
			convert_toc_tree(outline, toc);

		}
		fz_catch(context) {
			cout << "Error: Could not load outline ... " << endl;
		}
	}

	void load_marks_from_database() {
		marks.clear();
		bookmarks.clear();
		select_mark(db, file_name, marks);
		select_bookmark(db, file_name, bookmarks);
	}

	bool get_mark_location_if_exists(char symbol, float* y_offset) {
		int mark_index = get_mark_index(symbol);
		if (mark_index == -1) {
			return false;
		}
		*y_offset = marks[mark_index].y_offset;
	}

	Document(fz_context* context, string file_name, sqlite3* db) : context(context), file_name(file_name), doc(nullptr), db(db) {
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

	const vector<TocNode*>& get_toc() {
		return top_level_toc_nodes;
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

				load_marks_from_database();
				create_toc_tree(top_level_toc_nodes);

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


int mod(int a, int b)
{
	return (a % b + b) % b;
}

const int max_select_size = 100;

template<typename T>
class FilteredSelect {
private:
	vector<string> options;
	vector<T> values;
	int current_index;
	bool is_done;
	char select_string[max_select_size];

public:
	FilteredSelect(vector<string> options, vector<T> values) : options(options), values(values), current_index(0), is_done(false) {
		ZeroMemory(select_string, sizeof(select_string));
	}

	T* get_value() {
		int index = get_selected_index();
		if (index >= 0 && index < values.size()) {
			return &values[get_selected_index()];
		}
		return nullptr;
	}

	bool is_string_comppatible(string incomplete_string, string option_string) {
		return option_string.find(incomplete_string) < option_string.size();
	}

	int get_selected_index() {
		int index = -1;
		for (int i = 0; i < options.size(); i++) {
			if (is_string_comppatible(select_string, options[i])) {
				index += 1;
			}
			if (index == current_index) {
				return i;
			}
		}
		return index;
	}

	string get_selected_option() {
		return options[get_selected_index()];
	}

	int get_max_index() {
		int max_index = -1;
		for (int i = 0; i < options.size(); i++) {
			if (is_string_comppatible(select_string, options[i])) {
				max_index += 1;
			}
		}
		return max_index;
	}

	void move_item(int offset) {
		int new_index = offset + current_index;
		if (new_index < 0) {
			new_index = 0;
		}
		int max_index = get_max_index();
		if (new_index > max_index) {
			new_index = max_index;
		}
		current_index = new_index;
	}

	void next_item() {
		move_item(1);
	}

	void prev_item() {
		move_item(-1);
	}

	bool render() {
		ImGui::Begin("Select");

		ImGui::SetKeyboardFocusHere();
		if (ImGui::InputText("search string", select_string, sizeof(select_string), ImGuiInputTextFlags_CallbackHistory | ImGuiInputTextFlags_EnterReturnsTrue,
			[](ImGuiInputTextCallbackData* data) {
				FilteredSelect* filtered_select = (FilteredSelect*)data->UserData;

				if (data->EventKey == ImGuiKey_UpArrow) {
					filtered_select->prev_item();
				}
				if (data->EventKey == ImGuiKey_DownArrow) {
					filtered_select->next_item();
				}

				return 0;
			}, this)) {
			is_done = true;
		}
		//ImGui::InputText("search", text_buffer, 100, ImGuiInputTextFlags_CallbackHistory, [&](ImGuiInputTextCallbackData* data) {

		int index = 0;
		for (int i = 0; i < options.size(); i++) {
			//if (options[i].find(select_string) == 0) {
			if (is_string_comppatible(select_string, options[i])) {

				if (current_index == index) {
					ImGui::SetScrollHere();
				}

				ImGui::Selectable(options[i].c_str(), current_index == index);
				index += 1;
			}
		}

		ImGui::End();
		return is_done;
	}


};

bool intersects(float range1_start, float range1_end, float range2_start, float range2_end) {
	if (range2_start > range1_end || range1_start > range2_end) {
		return false;
	}
	return true;
}

class DocumentView {
	fz_context* mupdf_context;
	sqlite3* database;

	Document* current_document;
	string document_path;

	vector<float> page_heights;
	vector<float> page_widths;

	float zoom_level;
	float offset_x;
	float offset_y;

	bool render_is_invalid;

	int view_width;
	int view_height;

	vector<SearchResult> search_results;
	int current_search_result_index;

	inline void set_offsets(float new_offset_x, float new_offset_y) {
		offset_x = new_offset_x;
		offset_y = new_offset_y;
		render_is_invalid = true;
	}
public:
	DocumentView(fz_context* mupdf_context, sqlite3* db) : 
		mupdf_context(mupdf_context),
		database(db)
	{

	}
	
	Document* get_document() {
		return current_document;
	}


	inline void set_offset_x(float new_offset_x) {
		set_offsets(new_offset_x, offset_y);
	}

	inline void set_offset_y(float new_offset_y) {
		set_offsets(offset_x, new_offset_y);
	}

	void add_mark(char symbol) {
		assert(current_document);
		current_document->add_mark(symbol, offset_y);
	}

	void add_bookmark(string desc) {
		assert(current_document);
		current_document->add_bookmark(desc, offset_y);
	}

	void get_relative_rect(int page_width, int page_height, float output_vertices[8], int additional_offset) {
		float x0 = 2 * static_cast<float>(offset_x * zoom_level - page_width / 2) / view_width;
		float x1 = 2 * static_cast<float>(offset_x * zoom_level + page_width / 2) / view_width;
		float y0 = 2 * static_cast<float>(offset_y * zoom_level + additional_offset) / view_height;
		float y1 = 2 * static_cast<float>(offset_y * zoom_level + additional_offset - page_height) / view_height;

		output_vertices[0] = x0;
		output_vertices[1] = y0;

		output_vertices[2] = x1;
		output_vertices[3] = y0;

		output_vertices[4] = x0;
		output_vertices[5] = y1;

		output_vertices[6] = x1;
		output_vertices[7] = y1;
	}

	void on_view_size_change(int new_width, int new_height) {
		view_width = new_width;
		view_height = new_height;
		render_is_invalid = true;
	}

	bool should_rerender() {
		return render_is_invalid;
	}

	fz_rect document_to_window_rect(int page, fz_rect doc_rect) {
		double doc_rect_y_offset = 0.0f;
		for (int i = 0; i < page; i++) {
			doc_rect_y_offset -= page_heights[i];
		}
		doc_rect.y0 = page_heights[page] - doc_rect.y0;
		doc_rect.y1 = page_heights[page] - doc_rect.y1;

		doc_rect.y0 += doc_rect_y_offset;
		doc_rect.y1 += doc_rect_y_offset;

		fz_rect window_rect;
		float window_width_in_doc_space = static_cast<float>(view_width) / zoom_level;
		float window_height_in_doc_space = static_cast<float>(view_height) / zoom_level;

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

	void goto_mark(char symbol) {
		assert(current_document);
		float new_y_offset = 0.0f;
		if (current_document->get_mark_location_if_exists(symbol, &new_y_offset)) {
			set_offset_y(new_y_offset);
		}
	}

	void goto_end() {
		float cum_offset_y = 0.0f;
		for (auto height : page_heights) {
			cum_offset_y += height;
		}

		set_offset_y(cum_offset_y);
	}

	float zoom_in() {
		float old_zoom = zoom_level;
		zoom_level *= ZOOM_INC_FACTOR;
		float new_zoom = zoom_level;
		render_is_invalid = true;
		return zoom_level;
	}

	float zoom_out() {
		float old_zoom = zoom_level;
		zoom_level /= ZOOM_INC_FACTOR;
		float new_zoom = zoom_level;
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

	int get_current_page_number() {
		vector<int> visible_pages;
		int window_height, window_width;
		get_visible_pages(100, visible_pages);
		if (visible_pages.size() > 0) {
			return visible_pages[0];
		}
		return -1;
	}

	int search_text(const char* text) {
		search_results.clear();

		int num_pages = current_document->num_pages();
		int total_results = 0;

		for (int i = 0; i < num_pages; i++) {
			fz_page* page = fz_load_page(mupdf_context, current_document->doc, i);

			const int max_hits_per_page = 20;
			fz_quad hitboxes[max_hits_per_page];
			int num_results = fz_search_page(mupdf_context, page, text, hitboxes, max_hits_per_page);

			for (int j = 0; j < num_results; j++) {
				search_results.push_back(SearchResult{ hitboxes[j], i });
			}

			total_results += num_results;

			fz_drop_page(mupdf_context, page);
		}
		return total_results;
	}

	void goto_search_result(int offset) {
		if (search_results.size() == 0) {
			return;
		}

		int target_index = mod(current_search_result_index + offset, search_results.size());
		current_search_result_index = target_index;

		int target_page = search_results[target_index].page;

		fz_quad quad = search_results[target_index].quad;

		float new_offset_y = quad.ll.y;
		for (int i = 0; i < target_page; i++) {
			new_offset_y += page_heights[i] + page_paddings;
		}

		set_offset_y(new_offset_y);
	}

	void get_visible_pages(int window_height, vector<int>& visible_pages) {
		float window_y_range_begin = offset_y - window_height / (zoom_level);
		float window_y_range_end = offset_y + window_height / (zoom_level);
		window_y_range_begin -= 1;
		window_y_range_end += 1;

		float page_begin = 0.0f;

		for (int i = 0; i < page_heights.size(); i++) {
			float page_end = page_begin + page_heights[i];

			if (intersects(window_y_range_begin, window_y_range_end, page_begin, page_end)) {
				visible_pages.push_back(i);
			}
			page_begin = page_end;
		}
	}

	void move_pages(int num_pages) {
		int current_page = get_current_page_number();
		if (current_page == -1) {
			current_page = 0;
		}
		move_absolute(0, num_pages * (page_heights[current_page] + page_paddings));
	}

	void reset_doc_state() {
		zoom_level = 1.0f;
		set_offsets(0.0f, 0.0f);
	}

	void open_document(string doc_path) {

		string cannonical_path = std::filesystem::canonical(doc_path).string();
		document_path = cannonical_path;
		vector<OpenedBookState> prev_state;

		if (select_opened_book(database, cannonical_path, prev_state)) {
			if (prev_state.size() > 1) {
				cout << "more than one file with one path, this should not happen!" << endl;
			}
		}



		if (current_document != nullptr) {
			delete current_document;
		}

		// this part should be moved to WindowState
		// ------------------------------------------
		ofstream last_path_file(last_path_file_absolute_location);
		cout << "!!! " << last_path_file.is_open() << endl;
		last_path_file << doc_path << endl;
		last_path_file.close();
		// ------------------------------------------

		current_document = new Document(mupdf_context, doc_path, database);
		if (!current_document->open()) {
			delete current_document;
			current_document = nullptr;
		}
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

		if (current_document) {

			int page_count = current_document->num_pages();

			//todo: better (or any!) error handling
			for (int i = 0; i < page_count; i++) {
				fz_page* page = fz_load_page(mupdf_context, current_document->doc, i);
				fz_rect page_rect = fz_bound_page(mupdf_context, page);
				page_heights.push_back(page_rect.y1 - page_rect.y0);
				page_widths.push_back(page_rect.x1 - page_rect.x0);
				fz_drop_page(mupdf_context, page);
			}

		}

		//int window_width, window_height;
		//SDL_GetWindowSize(main_window, &window_width, &window_height);
		//offset_x = window_width / (2*zoom_level);
		//offset_y = window_height / zoom_level;

	}

	void goto_page(int page) {
		int max_page = current_document->num_pages();
		if (page > max_page) {
			page = max_page;
		}
		float new_offset_y = 0.0f;

		for (int i = 0; i < page; i++) {

			new_offset_y += page_heights[i];
		}

		set_offset_y(new_offset_y);
	}

	void render_page(int page_number, GLuint rendered_program, GLuint unrendered_program) {

		int page_width, page_height;
		GLuint texture = global_pdf_renderer->find_rendered_page(document_path, page_number, zoom_level, &page_width, &page_height);
		if (texture == 0) {
			page_width = static_cast<int>(page_widths[page_number] * zoom_level);
			page_height = static_cast<int>(page_heights[page_number] * zoom_level);
		}

		int additional_offset = get_page_additional_offset(page_number);
		float page_vertices[4 * 2];
		get_relative_rect(page_width, page_height, page_vertices, additional_offset);

		if (texture != 0) {
			glUseProgram(rendered_program);
			glEnableVertexAttribArray(0);
			glEnableVertexAttribArray(1);
			//glBindVertexArray(vertex_array_object);
			//glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer_object);
			glBufferData(GL_ARRAY_BUFFER, sizeof(page_vertices), page_vertices, GL_DYNAMIC_DRAW);

			glBindTexture(GL_TEXTURE_2D, texture);

			glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		}
		else {
			glUseProgram(unrendered_program);
			glEnableVertexAttribArray(0);
			glEnableVertexAttribArray(1);
			//glBindVertexArray(vertex_array_object);
			//glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer_object);
			glBufferData(GL_ARRAY_BUFFER, sizeof(page_vertices), page_vertices, GL_DYNAMIC_DRAW);
			glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		}
	}

	void render(GLuint rendered_program, GLuint unrendered_program, GLuint highlight_program) {

		vector<int> visible_pages;
		get_visible_pages(view_height, visible_pages);
		render_is_invalid = false;

		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		for (int j = 0; j < visible_pages.size(); j++) {
			render_page(visible_pages[j], rendered_program, unrendered_program);
		}
		if (search_results.size() > 0) {
			SearchResult current_search_result = search_results[current_search_result_index];
			fz_quad result_quad = current_search_result.quad;
			fz_rect result_rect;
			result_rect.x0 = result_quad.ul.x;
			result_rect.x1 = result_quad.ur.x;

			result_rect.y0 = result_quad.ul.y;
			result_rect.y1 = result_quad.ll.y;

			fz_rect window_rect = document_to_window_rect(current_search_result.page, result_rect);
			float quad_vertex_data[] = {
				window_rect.x0, window_rect.y1,
				window_rect.x1, window_rect.y1,
				window_rect.x0, window_rect.y0,
				window_rect.x1, window_rect.y0
			};

			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			glUseProgram(highlight_program);
			glEnableVertexAttribArray(0);
			glEnableVertexAttribArray(1);
			//glBindVertexArray(vertex_array_object);
			//glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer_object);
			glBufferData(GL_ARRAY_BUFFER, sizeof(quad_vertex_data), quad_vertex_data, GL_DYNAMIC_DRAW);
			glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
			glDisable(GL_BLEND);

		}
	}

	void persist() {
		update_book(database, document_path, zoom_level, offset_x, offset_y);
	}
};

class WindowState {
private:
	DocumentView* main_document_view;
	DocumentView* helper_document_view;
	DocumentView* current_document_view;
	//Document* current_document;
	SDL_Window* main_window;
	SDL_Window* helper_window;
	SDL_GLContext* opengl_context;
	fz_context* mupdf_context;
	sqlite3* database;
	const Command* pending_text_command;
	FilteredSelect<int>* current_toc_select;
	FilteredSelect<float>* current_bookmark_select;

	//float zoom_level;
	GLuint vertex_array_object;
	GLuint vertex_buffer_object;
	GLuint uv_buffer_object;
	GLuint gl_program;
	GLuint gl_debug_program;
	GLuint gl_unrendered_program;
	//vector<float> page_heights;
	//vector<float> page_widths;

	//string current_document_path;

	// offset of the center of screen in the current document space
	//float offset_x;
	//float offset_y;


	//int main_window_prev_width;
	//int main_window_prev_height;

	unsigned int last_tick_time;

	//vector<CachedPage> cached_pages;

	bool is_showing_ui;
	bool is_showing_textbar;
	char text_command_buffer[100];

public:
	bool render_is_invalid;
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
		database(database),
		render_is_invalid(true),
		is_showing_ui(false),
		is_showing_textbar(false),
		pending_text_command(nullptr),
		current_toc_select(nullptr),
		current_bookmark_select(nullptr)
	{

		main_document_view = new DocumentView(mupdf_context, database);
		helper_document_view = new DocumentView(mupdf_context, database);
		current_document_view = main_document_view;

		gl_program = LoadShaders("shaders\\simple.vertex", "shaders\\simple.fragment");
		if (gl_program == 0) {
			cout << "Error: could not compile shaders" << endl;
		}

		gl_debug_program = LoadShaders("shaders\\simple.vertex", "shaders\\debug.fragment");
		if (gl_debug_program == 0) {
			cout << "Error: could not compile debug shaders" << endl;
		}

		gl_unrendered_program = LoadShaders("shaders\\simple.vertex", "shaders\\unrendered_page.fragment");
		if (gl_unrendered_program == 0) {
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


		int main_window_width, main_window_height;
		SDL_GetWindowSize(main_window, &main_window_width, &main_window_height);
		main_document_view->on_view_size_change(main_window_width, main_window_height);

		int helper_window_width, helper_window_height;
		SDL_GetWindowSize(helper_window, &helper_window_width, &helper_window_height);
		helper_document_view->on_view_size_change(helper_window_width, helper_window_height);

		last_tick_time = SDL_GetTicks();
	}

	void on_focus_gained(Uint32 window_id) {
		if (window_id == SDL_GetWindowID(main_window)) {
			current_document_view = main_document_view;
		}
		if (window_id == SDL_GetWindowID(helper_window)) {
			current_document_view = helper_document_view;
		}
	}

	void handle_command_with_symbol(const Command* command, char symbol) {
		assert(symbol);
		assert(command->requires_symbol);
		if (command->name == "set_mark") {
			assert(current_document_view);
			current_document_view->add_mark(symbol);
		}
		else if (command->name == "goto_mark") {
			assert(current_document_view);
			current_document_view->goto_mark(symbol);
		}
	}

	void handle_command_with_file_name(const Command* command, string file_name) {
		assert(command->requires_file_name);
		//cout << "handling " << command->name << " with file " << file_name << endl;
		if (command->name == "open_document") {
			current_document_view->open_document(file_name);
		}
	}


	//void handle_command_with_text(const Command* command, string text) {
	//	assert(command->requires_text);
	//	pending_text_command = command;
	//	is_showing_textbar = true;
	//	is_showing_ui = true;
	//}


	void handle_command(const Command* command, int num_repeats) {
		if (command->requires_text) {
			pending_text_command = command;
			show_textbar();
			return;
		}
		if (command->name == "goto_begining") {
			if (num_repeats) {
				current_document_view->goto_page(num_repeats);
			}
			else {
				current_document_view->set_offset_y(0.0f);
			}
		}

		if (command->name == "goto_end") {
			current_document_view->goto_end();
		}
		//if (command->name == "search") {
		//	show_searchbar();
		//}
		int rp = max(num_repeats, 1);

		if (command->name == "move_down") {
			current_document_view->move(0.0f, 72.0f * rp);
		}
		else if (command->name == "move_up") {
			current_document_view->move(0.0f, -72.0f * rp);
		}

		else if (command->name == "move_right") {
			current_document_view->move(72.0f * rp, 0.0f);
		}

		else if (command->name == "move_left") {
			current_document_view->move(-72.0f * rp, 0.0f);
		}

		else if (command->name == "zoom_in") {
			current_document_view->zoom_in();
		}

		else if (command->name == "zoom_out") {
			current_document_view->zoom_out();
		}

		else if (command->name == "next_item") {
			current_document_view->goto_search_result(1 + num_repeats);
		}

		else if (command->name == "previous_item") {
			current_document_view->goto_search_result(-1 - num_repeats);
		}

		else if (command->name == "open_document") {
			cout << "can not open document right now!" << endl;
		}
		else if (command->name == "next_page") {
			current_document_view->move_pages(1 + num_repeats);
		}
		else if (command->name == "previous_page") {
			current_document_view->move_pages(-1 - num_repeats);
		}
		else if (command->name == "goto_toc") {
			vector<string> flat_toc;
			vector<int> current_document_toc_pages;
			get_flat_toc(current_document_view->get_document()->get_toc(), flat_toc, current_document_toc_pages);
			current_toc_select = new FilteredSelect<int>(flat_toc, current_document_toc_pages);
			is_showing_ui = true;
		}
		else if (command->name == "goto_bookmark") {
			is_showing_ui = true;
			vector<string> option_names;
			vector<float> option_locations;
			for (int i = 0; i < current_document_view->get_document()->get_bookmarks().size(); i++) {
				option_names.push_back(current_document_view->get_document()->get_bookmarks()[i].description);
				option_locations.push_back(current_document_view->get_document()->get_bookmarks()[i].y_offset);
			}
			current_bookmark_select = new FilteredSelect<float>(option_names, option_locations);
			cout << "bookmark" << endl;
		}
		else if (command->name == "debug") {
			cout << "debug" << endl;
		}

	}


	bool should_render() {
		if (render_is_invalid) {
			return true;
		}
		if (is_showing_ui) {
			return true;
		}
		if (current_document_view->should_rerender()) {
			return true;
		}
		return false;
	}

	void handle_return() {
		is_showing_textbar = false;
		is_showing_ui = false;
		render_is_invalid = true;
		//current_search_result_index = 0;
		pending_text_command = nullptr;
	}


	void handle_pending_text_command(string text) {
		if (pending_text_command->name == "search") {
			//search_results.clear();
			//search_text(text.c_str(), search_results);
			current_document_view->search_text(text.c_str());
		}

		if (pending_text_command->name == "add_bookmark") {
			current_document_view->add_bookmark(text);
		}
	}





	void on_window_size_changed(Uint32 window_id) {
		if (window_id == SDL_GetWindowID(main_window)) {
			int new_width, new_height;
			SDL_GetWindowSize(main_window, &new_width, &new_height);
			main_document_view->on_view_size_change(new_width, new_height);
		}

		if (window_id == SDL_GetWindowID(helper_window)) {
			int new_width, new_height;
			SDL_GetWindowSize(helper_window, &new_width, &new_height);
			helper_document_view->on_view_size_change(new_width, new_height);
		}

		//if ((new_width != main_window_prev_width) || (new_height != main_window_prev_height)) {
		//	main_window_prev_width = new_width;
		//	main_window_prev_height = new_height;
		//}
		render_is_invalid = true;
	}



	void tick(bool force = false) {
		unsigned int now = SDL_GetTicks();
		if (force || (now - last_tick_time) > 1000) {
			last_tick_time = now;
			//update_book(database, current_document_path, zoom_level, offset_x, offset_y);
			current_document_view->persist();
		}
	}

	void update_window_title() {
		//if (current_document) {
		//	stringstream title;
		//	title << current_document_path << " ( page " << get_current_page_number() << " / " << current_document->num_pages() << " )";
		//	if (search_results.size() > 0) {
		//		title << " [Showing result " << current_search_result_index + 1 << " / " << search_results.size() << " ]";
		//	}
		//	SDL_SetWindowTitle(main_window, title.str().c_str());
		//}
	}


	void show_textbar() {
		is_showing_ui = true;
		is_showing_textbar = true;
		ZeroMemory(text_command_buffer, sizeof(text_command_buffer));
	}

	void open_document(string path) {
		current_document_view->open_document(path);
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
			glBindVertexArray(vertex_array_object);
			glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer_object);

			main_document_view->render(gl_program, gl_unrendered_program, gl_debug_program);

			{
				ImGui_ImplOpenGL3_NewFrame();
				ImGui_ImplSDL2_NewFrame(main_window);
				ImGui::NewFrame();

				if (is_showing_textbar) {

					ImGui::SetNextWindowPos(ImVec2(0, 0));
					ImGui::SetNextWindowSize(ImVec2(window_width, 60));
					ImGui::Begin(pending_text_command->name.c_str());
					//ImGui::SetKeyboardFocusHere();
					ImGui::SetKeyboardFocusHere();
					if (ImGui::InputText(
						pending_text_command->name.c_str(),
						text_command_buffer,
						sizeof(text_command_buffer),
						ImGuiInputTextFlags_EnterReturnsTrue)
						) {
						//search_results.clear();
						//search_text(search_string, search_results);
						//handle_return();
						handle_pending_text_command(text_command_buffer);
						current_document_view->goto_search_result(0);
						handle_return();
						io.WantCaptureKeyboard = false;
					}
					ImGui::End();
				}
				if (current_toc_select) {
					//vector<string> select_items = { "ali", "alo", "mos", "tafavi" };
					//render_select(select_items, nullptr);
					if (current_toc_select->render()) {
						//cout << "selected option " << current_select->get_selected_option() << endl;
						//int selected_index = current_toc_select->get_selected_index();
						//goto_page(current_document_toc_pages[selected_index]);
						int* page_value = current_toc_select->get_value();
						if (page_value) {
							current_document_view->goto_page(*page_value);
						}
						delete current_toc_select;
						current_toc_select = nullptr;
						is_showing_ui = false;
					}
				}
				if (current_bookmark_select) {
					if (current_bookmark_select->render()) {
						float* offset_value = current_bookmark_select->get_value();
						if (offset_value) {
							current_document_view->set_offset_y(*offset_value);
						}
						delete current_bookmark_select;
						current_bookmark_select = nullptr;
						is_showing_ui = false;
						render_is_invalid = true;
					}
				}

				ImGui::Render();
				ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

			}

			SDL_GL_SwapWindow(main_window);

			SDL_GL_MakeCurrent(helper_window, *opengl_context);

			SDL_GetWindowSize(helper_window, &window_width, &window_height);
			glViewport(0, 0, window_width, window_height);

			glClearColor(1.0f, 0.0f, 0.0f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT);
			helper_document_view->render(gl_program, gl_unrendered_program, gl_debug_program);
			SDL_GL_SwapWindow(helper_window);
			global_pdf_renderer->delete_old_pages();
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

int main(int argc, char* args[]) {



	sqlite3* db;
	char* error_message = nullptr;
	int rc;

	rc = sqlite3_open("test.db", &db);
	if (rc) {
		cout << "could not open database" << sqlite3_errmsg(db) << endl;
	}

	create_opened_books_table(db);
	create_marks_table(db);
	create_bookmarks_table(db);


	fz_locks_context locks;
	locks.user = mupdf_mutexes;
	locks.lock = lock_mutex;
	locks.unlock = unlock_mutex;

	fz_context* mupdf_context = fz_new_context(nullptr, &locks, FZ_STORE_UNLIMITED);
	//fz_context* mupdf_context = fz_new_context(nullptr, nullptr, FZ_STORE_UNLIMITED);
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

	global_pdf_renderer = new PdfRenderer(mupdf_context);
	//global_pdf_renderer = new PdfRenderer();
	//global_pdf_renderer->init();

	thread worker([]() {
		global_pdf_renderer->run();
		});


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

	if (gl_context == nullptr) {
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

	//char file_path[MAX_PATH] = { 0 };
	string file_path;
	ifstream last_state_file(last_path_file_absolute_location);
	//last_state_file.read(file_path, MAX_PATH);
	getline(last_state_file, file_path);
	//last_state_file >> initial_document;
	last_state_file.close();

	window_state.open_document(file_path);

	bool quit = false;

	global_pdf_renderer->set_invalidate_pointer(&window_state.render_is_invalid);
	InputHandler input_handler("keys.config");

	bool is_waiting_for_symbol = false;
	const Command* current_pending_command = nullptr;
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
				window_state.on_window_size_changed(event.window.windowID);
			}
			if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_FOCUS_GAINED) {
				window_state.on_focus_gained(event.window.windowID);
			}
			if (event.type == SDL_KEYDOWN) {
				vector<SDL_Scancode> ignored_codes = {
					SDL_SCANCODE_LSHIFT,
					SDL_SCANCODE_RSHIFT,
					SDL_SCANCODE_LCTRL,
					SDL_SCANCODE_RCTRL
				};
				if (find(ignored_codes.begin(), ignored_codes.end(), event.key.keysym.scancode) != ignored_codes.end()) {
					continue;
				}
				if (is_waiting_for_symbol) {
					char symb = get_symbol(event.key.keysym.scancode, (event.key.keysym.mod & KMOD_SHIFT != 0));
					if (symb) {
						window_state.handle_command_with_symbol(current_pending_command, symb);
						current_pending_command = nullptr;
						is_waiting_for_symbol = false;
					}
					continue;
				}
				int num_repeats = 0;
				const Command* command = input_handler.handle_key(
					SDL_GetKeyFromScancode(event.key.keysym.scancode),
					(event.key.keysym.mod & KMOD_SHIFT) != 0,
					(event.key.keysym.mod & KMOD_CTRL) != 0, &num_repeats);
				if (command) {
					if (command->requires_symbol) {
						is_waiting_for_symbol = true;
						current_pending_command = command;
						continue;
					}
					if (command->requires_file_name) {
						char file_name[MAX_PATH];
						if (select_pdf_file_name(file_name, MAX_PATH)) {
							window_state.handle_command_with_file_name(command, file_name);
						}
						else {
							cout << "File select failed" << endl;
						}
						continue;
					}
					else {
						window_state.handle_command(command, num_repeats);
					}
				}
			}
			if (event.type == SDL_MOUSEWHEEL) {
				int num_repeats = 1;
				const Command* command;
				if (event.wheel.y > 0) {
					command = input_handler.handle_key(SDLK_UP, false, false, &num_repeats);
				}
				if (event.wheel.y < 0) {
					command = input_handler.handle_key(SDLK_DOWN, false, false, &num_repeats);
				}
				
				window_state.handle_command(command, abs(event.wheel.y));
			}

		}
		window_state.render();

		SDL_Delay(16);
		window_state.tick();
	}

	sqlite3_close(db);
	worker.join();
	return 0;
}
