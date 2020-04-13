//todo: cleanup the code
//todo: visibility test is still buggy??
//todo: threading
//todo: remove some O(n) things from page height computations
//todo: add fuzzy search
//todo: improve speed and code of document change (cache documents?)
//todo: copy
//todo: handle document memory leak (because documents are not deleted since adding state history)
//todo: tests!
//todo: handle mouse in menues
//todo: stop creating DocumentViews!
//todo: bug: last documnet path is not updated

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
#include <optional>
#include <utility>

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
#include "utils.h"
#include "ui.h"
#include "pdf_renderer.h"
#include "document.h"
#include "document_view.h"

#define FTS_FUZZY_MATCH_IMPLEMENTATION
#include "fts_fuzzy_match.h"
#undef FTS_FUZZY_MATCH_IMPLEMENTATION


extern const float ZOOM_INC_FACTOR = 1.2f;
extern const unsigned int cache_invalid_milies = 1000;
extern const int page_paddings = 0;
extern const int max_pending_requests = 31;
extern const char* last_path_file_absolute_location = "C:\\Users\\Lion\\source\\repos\\pdf_viewer\\pdf_viewer\\last_document_path.txt";


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


class WindowState {
private:
	PdfRenderer* pdf_renderer;
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
	FilteredSelect<string>* current_opened_doc_select;
	FilteredSelect<float>* current_bookmark_select;
	FilteredSelect<BookState>* current_global_bookmark_select;

	//float zoom_level;
	GLuint vertex_array_object;
	GLuint vertex_buffer_object;
	GLuint uv_buffer_object;
	GLuint gl_program;
	GLuint gl_debug_program;
	GLuint gl_unrendered_program;

	string pending_link_source_document_path;
	Link pending_link;
	bool pending_link_source_filled;

	int current_history_index;
	vector<DocumentViewState> history;
	vector<DocumentView*> cached_document_views;

	Link* link_to_edit;

	// last position when mouse was clicked in absolute document space
	float last_mouse_down_x;
	float last_mouse_down_y;
	optional<fz_rect> selected_rect;

	unsigned int last_tick_time;

	bool is_showing_ui;
	bool is_showing_textbar;
	char text_command_buffer[100];
	vector<fz_rect> selected_character_rects;

	int main_window_width, main_window_height;
	int helper_window_width, helper_window_height;

	void set_main_document_view_state(DocumentViewState new_view_state) {
		main_document_view = new_view_state.document_view;
		main_document_view->on_view_size_change(main_window_width, main_window_height);
		main_document_view->set_offsets(new_view_state.offset_x, new_view_state.offset_y);
		main_document_view->set_zoom_level(new_view_state.zoom_level);
		current_document_view = main_document_view;
	}

	void prev_state() {
		/*
		Goto previous history
		In order to edit a link, we set the link to edit and jump to the link location, when going back, we
		update the link with the current location of document, therefore, we must check to see if a link
		is being edited and if so, we should update its destination position
		*/

		if (current_history_index > 0) {
			if (current_history_index == history.size()) {
				push_state();
				current_history_index = history.size()-1;
			}
			current_history_index--;

			if (link_to_edit) {
				float link_new_offset_x = current_document_view->get_offset_x();
				float link_new_offset_y = current_document_view->get_offset_y();
				link_to_edit->dest_offset_x = link_new_offset_x;
				link_to_edit->dest_offset_y = link_new_offset_y;
				update_link(database, history[current_history_index].document_view->get_document()->get_path(),
					link_new_offset_x, link_new_offset_y, link_to_edit->src_offset_y);
				link_to_edit = nullptr;
			}

			set_main_document_view_state(history[current_history_index]);
		}
	}

	void next_state() {
		if (current_history_index+1 < history.size()) {
			current_history_index++;
			set_main_document_view_state(history[current_history_index]);
		}
	}

	void push_state() {
		DocumentViewState dvs = main_document_view->get_state();

		// do not add the same place to history multiple times
		if (history.size() > 0) {
			DocumentViewState last_history = history[history.size() - 1];
			if (last_history.document_view == main_document_view && last_history.offset_x == dvs.offset_x && last_history.offset_y == dvs.offset_y) {
				return;
			}
		}

		if (current_history_index == history.size()) {
			history.push_back(dvs);
			current_history_index = history.size();
		}
		else {
			//todo: should probably do some reference counting garbage collection here

			history.erase(history.begin() + current_history_index, history.end());
			history.push_back(dvs);
			current_history_index = history.size();
		}
	}



public:
	bool render_is_invalid;
	WindowState(SDL_Window* main_window,
		SDL_Window* helper_window,
		fz_context* mupdf_context,
		SDL_GLContext* opengl_context,
		sqlite3* database,
		PdfRenderer* pdf_renderer
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
		current_bookmark_select(nullptr),
		pdf_renderer(pdf_renderer)
	{

		main_document_view = new DocumentView(mupdf_context, database, pdf_renderer);
		helper_document_view = new DocumentView(mupdf_context, database, pdf_renderer);

		cached_document_views.push_back(main_document_view);

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

		SDL_GetWindowSize(main_window, &main_window_width, &main_window_height);
		main_document_view->on_view_size_change(main_window_width, main_window_height);

		SDL_GetWindowSize(helper_window, &helper_window_width, &helper_window_height);
		helper_document_view->on_view_size_change(helper_window_width, helper_window_height);

		last_tick_time = SDL_GetTicks();
	}

	void handle_link() {
		if (pending_link_source_filled) {
			pending_link.dest_offset_x = main_document_view->get_offset_x();
			pending_link.dest_offset_y = main_document_view->get_offset_y();
			pending_link.document_path = main_document_view->get_document()->get_path();
			if (pending_link_source_document_path == main_document_view->get_document()->get_path()) {
				main_document_view->get_document()->add_link(pending_link);
			}
			else {
				for (int i = 0; i < cached_document_views.size(); i++) {
					if (cached_document_views[i]->get_document()) {
						if (cached_document_views[i]->get_document()->get_path() == pending_link_source_document_path) {
							cached_document_views[i]->get_document()->add_link(pending_link, false);
						}
					}
				}
				insert_link(database,
					pending_link_source_document_path,
					pending_link.document_path,
					pending_link.dest_offset_x,
					pending_link.dest_offset_y,
					pending_link.src_offset_y);
			}
			render_is_invalid = true;
			pending_link_source_filled = false;
		}
		else {
			pending_link.src_offset_y = main_document_view->get_offset_y();
			pending_link_source_document_path = main_document_view->get_document()->get_path();
			pending_link_source_filled = true;
		}
		invalidate_render();
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
			invalidate_render();
		}
		else if (command->name == "goto_mark") {
			assert(current_document_view);
			current_document_view->goto_mark(symbol);
		}
		else if (command->name == "delete") {
			if (symbol == 'y') {
				main_document_view->delete_closest_link();
				invalidate_render();
			}
			else if (symbol == 'b') {
				main_document_view->delete_closest_bookmark();
				invalidate_render();
			}
		}
	}

	void handle_command_with_file_name(const Command* command, string file_name) {
		assert(command->requires_file_name);
		//cout << "handling " << command->name << " with file " << file_name << endl;
		if (command->name == "open_document") {
			//current_document_view->open_document(file_name);
			open_document(file_name);
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
		if (command->pushes_state) {
			push_state();
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
		else if (command->name == "link") {
			handle_link();
		}

		else if (command->name == "move_left") {
			current_document_view->move(-72.0f * rp, 0.0f);
		}

		else if (command->name == "goto_link") {
			Link* link = main_document_view->find_closest_link();
			if (link) {

				//todo: add a feature where we can tap tab button to switch between main view and helper view
				push_state();
				open_document(link->document_path, link->dest_offset_x, link->dest_offset_y);
				invalidate_render();
			}
		}
		else if (command->name == "edit_link") {
			Link* link = main_document_view->find_closest_link();
			if (link) {
				push_state();
				link_to_edit = link;
				open_document(link->document_path, link->dest_offset_x, link->dest_offset_y);
				invalidate_render();
			}
		}

		else if (command->name == "zoom_in") {
			current_document_view->zoom_in();
		}

		else if (command->name == "zoom_out") {
			current_document_view->zoom_out();
		}

		else if (command->name == "next_state") {
			next_state();
		}
		else if (command->name == "prev_state") {
			prev_state();
		}

		else if (command->name == "next_item") {
			current_document_view->goto_search_result(1 + num_repeats);
		}

		else if (command->name == "previous_item") {
			current_document_view->goto_search_result(-1 - num_repeats);
		}
		else if (command->name == "push_state") {
			push_state();
		}
		else if (command->name == "pop_state") {
			prev_state();
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
			if (current_document_toc_pages.size() > 0) {
				current_toc_select = new FilteredSelect<int>(flat_toc, current_document_toc_pages, [](void* a) {});
				is_showing_ui = true;
			}
		}
		else if (command->name == "open_prev_doc") {
			vector<string> opened_docs_paths;
			vector<string> opened_docs_names;
			select_prev_docs(database, opened_docs_paths);

			for (const auto& p : opened_docs_paths) {
				opened_docs_names.push_back(std::filesystem::path(p).filename().string());
			}

			if (opened_docs_paths.size() > 0) {
				current_opened_doc_select = new FilteredSelect<string>(opened_docs_names, opened_docs_paths, [](void* a) {});
				is_showing_ui = true;
			}
		}
		else if (command->name == "goto_bookmark") {
			is_showing_ui = true;
			vector<string> option_names;
			vector<float> option_locations;
			for (int i = 0; i < current_document_view->get_document()->get_bookmarks().size(); i++) {
				option_names.push_back(current_document_view->get_document()->get_bookmarks()[i].description);
				option_locations.push_back(current_document_view->get_document()->get_bookmarks()[i].y_offset);
			}
			current_bookmark_select = new FilteredSelect<float>(option_names, option_locations, [](void* a) {});
		}
		else if (command->name == "goto_bookmark_g") {
			is_showing_ui = true;
			vector<pair<string, BookMark>> global_bookmarks;
			global_select_bookmark(database, global_bookmarks);
			vector<string> descs;
			vector<BookState> book_states;

			for (const auto& desc_bm_pair : global_bookmarks) {
				string path = desc_bm_pair.first;
				BookMark bm = desc_bm_pair.second;
				descs.push_back(bm.description);
				book_states.push_back({ path, bm.y_offset });
			}
			current_global_bookmark_select = new FilteredSelect<BookState>(descs, book_states, [](void* a) {});

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
			SDL_GetWindowSize(main_window, &main_window_width, &main_window_height);
			main_document_view->on_view_size_change(main_window_width, main_window_height);
		}

		if (window_id == SDL_GetWindowID(helper_window)) {
			SDL_GetWindowSize(helper_window, &helper_window_width, &helper_window_height);
			helper_document_view->on_view_size_change(helper_window_width, helper_window_height);
		}

		render_is_invalid = true;
	}



	void tick(bool force = false) {
		unsigned int now = SDL_GetTicks();
		if (force || (now - last_tick_time) > 1000) {
			last_tick_time = now;
			//update_book(database, current_document_path, zoom_level, offset_x, offset_y);
			main_document_view->persist();
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

	string get_status_string(const Command* pending_command) {
		stringstream ss;
		if (main_document_view->get_document() == nullptr) return "";
		ss << "Page " << main_document_view->get_current_page_number() << " / " << main_document_view->get_document()->num_pages();
		int num_search_results = main_document_view->get_num_search_results();
		if (num_search_results > 0) {
			ss << " | showing result " << main_document_view->get_current_search_result_index() + 1 << " / " << num_search_results;
		}
		if (pending_link_source_filled) {
			ss << " | linking ...";
		}
		if (link_to_edit) {
			ss << " | editing link ...";
		}
		if (pending_command && pending_command->requires_symbol) {
			ss << " | " << pending_command->name <<" waiting for symbol";
		}
		return ss.str();
	}

	void invalidate_render() {
		render_is_invalid = true;
	}


	void show_textbar() {
		is_showing_ui = true;
		is_showing_textbar = true;
		ZeroMemory(text_command_buffer, sizeof(text_command_buffer));
	}

	void open_document(string path, optional<float> offset_x = {}, optional<float> offset_y = {}) {

		main_document_view = new DocumentView(mupdf_context, database, pdf_renderer);
		main_document_view->open_document(path);
		main_document_view->on_view_size_change(main_window_width, main_window_height);
		if (offset_x) {
			main_document_view->set_offset_x(offset_x.value());
		}
		if (offset_y) {
			main_document_view->set_offset_y(offset_y.value());
		}
		current_document_view = main_document_view;
		cached_document_views.push_back(main_document_view);

		if (path.size() > 0) {
			ofstream last_path_file(last_path_file_absolute_location);
			last_path_file << path << endl;
			last_path_file.close();
		}
	}

	void handle_escape() {
		is_showing_ui = false;
		is_showing_textbar = false;
		ZeroMemory(text_command_buffer, sizeof(text_command_buffer));
		pending_link_source_document_path = "";
		pending_link_source_filled = false;
		pending_text_command = nullptr;
		if (current_toc_select) {
			delete current_toc_select;
			current_toc_select = nullptr;
		}
		if (current_opened_doc_select) {
			delete current_opened_doc_select;
			current_opened_doc_select = nullptr;
		}
		if (current_bookmark_select) {
			delete current_bookmark_select;
			current_bookmark_select = nullptr;
		}
		if (current_global_bookmark_select) {
			delete current_global_bookmark_select;
			current_global_bookmark_select = nullptr;
		}
		if (main_document_view) {
			main_document_view->handle_escape();
		}

		if (helper_document_view) {
			helper_document_view->handle_escape();
		}
		invalidate_render();
	}

	void handle_click(int pos_x, int pos_y) {
		auto link_ = main_document_view->get_link_in_pos(pos_x, pos_y);
		if (link_.has_value()) {
			PdfLink link = link_.value();
			int page;
			float offset_x, offset_y;
			parse_uri(link.uri, &page, &offset_x, &offset_y);

			if (!pending_link_source_filled) {
				push_state();
				main_document_view->goto_offset_within_page(page, offset_x, offset_y);
			}
			else {
				// if we press the link button and then click on a pdf link, we automatically link to the
				// link's destination

				pending_link.dest_offset_x = offset_x;
				pending_link.dest_offset_y = main_document_view->get_page_offset(page-1) + offset_y;
				pending_link.document_path = main_document_view->get_document()->get_path();
				main_document_view->get_document()->add_link(pending_link);
				render_is_invalid = true;
				pending_link_source_filled = false;
			}
		}
	}

	void handle_left_click(float x, float y, bool down) {
		float x_, y_;
		main_document_view->window_to_absolute_document_pos(x, y, &x_, &y_);

		if (down == true) {
			last_mouse_down_x = x_;
			last_mouse_down_y = y_;
		}
		else {
			if ((abs(last_mouse_down_x - x_) + abs(last_mouse_down_y - y_)) > 20) {
				fz_rect sr;
				sr.x0 = min(last_mouse_down_x, x_);
				sr.x1 = max(last_mouse_down_x, x_);

				sr.y0 = min(last_mouse_down_y, y_);
				sr.y1 = max(last_mouse_down_y, y_);
				selected_rect = sr;

				main_document_view->get_text_selection(sr, selected_character_rects);
				invalidate_render();
			}
			else {
				selected_rect = {};
				handle_click(x, y);
				selected_character_rects.clear();
				invalidate_render();
			}
		}
	}

	void render(const Command* pending_command) {
		if (should_render()) {
			cout << "rendering ..." << endl;

			//current_document_view->get_document()->find_closest_link(current_document_view->)
			Link* link = main_document_view->find_closest_link();
			if (link) {
				if (helper_document_view->get_document() && 
					helper_document_view->get_document()->get_path() == link->document_path) {
					helper_document_view->set_offsets(link->dest_offset_x, link->dest_offset_y);
				}
				else {
					delete helper_document_view;
					helper_document_view = new DocumentView(mupdf_context, database, pdf_renderer, link->document_path,
						helper_window_width, helper_window_height, link->dest_offset_x, link->dest_offset_y);
				}
			}
			render_is_invalid = false;

			ImGuiIO& io = ImGui::GetIO(); (void)io;

			update_window_title();

			SDL_GL_MakeCurrent(main_window, *opengl_context);

			glViewport(0, 0, main_window_width, main_window_height);
			glBindVertexArray(vertex_array_object);
			glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer_object);

			main_document_view->render(gl_program, gl_unrendered_program, gl_debug_program);

			{
				ImGui_ImplOpenGL3_NewFrame();
				ImGui_ImplSDL2_NewFrame(main_window);
				ImGui::NewFrame();

				if (is_showing_textbar) {

					ImGui::SetNextWindowPos(ImVec2(0, 0));
					ImGui::SetNextWindowSize(ImVec2(main_window_width, 60));
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
							push_state();
							current_document_view->goto_page(*page_value);
						}
						delete current_toc_select;
						current_toc_select = nullptr;
						is_showing_ui = false;
					}
				}
				if (current_opened_doc_select) {
					if (current_opened_doc_select->render()) {
						string doc_path = *current_opened_doc_select->get_value();
						delete current_opened_doc_select;
						current_opened_doc_select = nullptr;
						is_showing_ui = false;

						if (doc_path.size() > 0) {
							push_state();
							open_document(doc_path);
						}
					}
				}
				if (current_bookmark_select) {
					if (current_bookmark_select->render()) {
						float* offset_value = current_bookmark_select->get_value();
						if (offset_value) {
							push_state();
							current_document_view->set_offset_y(*offset_value);
						}
						delete current_bookmark_select;
						current_bookmark_select = nullptr;
						is_showing_ui = false;
						render_is_invalid = true;
					}
				}
				if (current_global_bookmark_select) {
					if (current_global_bookmark_select->render()) {
						BookState* offset_value = current_global_bookmark_select->get_value();
						if (offset_value) {
							push_state();
							main_document_view = new DocumentView(mupdf_context, database, pdf_renderer);
							main_document_view->open_document(offset_value->document_path);
							main_document_view->set_offset_y(offset_value->offset_y);
							int window_width, window_height;
							SDL_GetWindowSize(main_window, &window_width, &window_height);
							main_document_view->on_view_size_change(window_width, window_height);
							cached_document_views.push_back(main_document_view);

						}
						delete current_global_bookmark_select;
						current_global_bookmark_select = nullptr;
						is_showing_ui = false;
						render_is_invalid = true;
					}
				}

				ImGui::GetStyle().WindowRounding = 0.0f;
				ImGui::GetStyle().WindowBorderSize = 0.0f;
				ImGui::SetNextWindowSize(ImVec2(main_window_width, 20));
				ImGui::SetNextWindowPos(ImVec2(0, main_window_height - 25));
				ImGui::Begin("some test", 0, ImGuiWindowFlags_NoDecoration);
				ImGui::Text(get_status_string(pending_command).c_str());
				ImGui::End();

				ImGui::Render();
				ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

			}
			for (auto rect : selected_character_rects) {
				main_document_view->render_highlight_absolute(gl_debug_program, rect);
			}

			if (selected_rect.has_value()) {
				fz_rect sr = selected_rect.value();
				main_document_view->render_highlight_absolute(gl_debug_program, sr);
			}

			SDL_GL_SwapWindow(main_window);

			SDL_GL_MakeCurrent(helper_window, *opengl_context);

			glViewport(0, 0, helper_window_width, helper_window_height);

			glClearColor(1.0f, 0.0f, 0.0f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT);
			helper_document_view->render(gl_program, gl_unrendered_program, gl_debug_program);
			SDL_GL_SwapWindow(helper_window);
			pdf_renderer->delete_old_pages();
		}
	}
};


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
	create_links_table(db);


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

	PdfRenderer* pdf_renderer = new PdfRenderer(mupdf_context);
	//global_pdf_renderer = new PdfRenderer();
	//global_pdf_renderer->init();

	thread worker([pdf_renderer]() {
		pdf_renderer->run();
		});


	SDL_Window* window = nullptr;

	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		cout << "could not initialize SDL" << endl;
		return -1;
	}

	CommandManager command_manager;

	SDL_Rect display_rect;
	SDL_GetDisplayBounds(0, &display_rect);

	SDL_Window* window2 = SDL_CreateWindow("Pdf Viewer2", display_rect.w, SDL_WINDOWPOS_UNDEFINED, 500, 500, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_MAXIMIZED);
	window = SDL_CreateWindow("Pdf Viewer", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 500, 500, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_MAXIMIZED);

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

	WindowState window_state(window, window2, mupdf_context, &gl_context, db, pdf_renderer);

	//char file_path[MAX_PATH] = { 0 };
	string file_path;
	ifstream last_state_file(last_path_file_absolute_location);
	//last_state_file.read(file_path, MAX_PATH);
	getline(last_state_file, file_path);
	//last_state_file >> initial_document;
	last_state_file.close();

	window_state.open_document(file_path);

	bool quit = false;

	pdf_renderer->set_invalidate_pointer(&window_state.render_is_invalid);
	InputHandler input_handler("keys.config");

	bool is_waiting_for_symbol = false;
	const Command* current_pending_command = nullptr;
	while (!quit) {
		SDL_Event event;

		while (SDL_PollEvent(&event)) {
			ImGui_ImplSDL2_ProcessEvent(&event);

			// retarded hack to deal with imgui one frame lag
			if ((event.key.type == SDL_KEYDOWN || event.key.type == SDL_KEYUP) && event.key.keysym.scancode == SDL_SCANCODE_ESCAPE) {
				current_pending_command = nullptr;
				is_waiting_for_symbol = false;
				window_state.handle_escape();
			}

			else if ((event.key.type == SDL_KEYDOWN) && io.WantCaptureKeyboard) {
				continue;
			}
			if (event.type == SDL_MOUSEBUTTONDOWN) {
				if (event.button.button == SDL_BUTTON_LEFT) {
					//window_state.handle_click(event.button.x, event.button.y);
					window_state.handle_left_click(event.button.x, event.button.y, true);
				}

				if (event.button.button == SDL_BUTTON_X1) {
					window_state.handle_command(command_manager.get_command_with_name("next_state"), 0);
				}

				if (event.button.button == SDL_BUTTON_X2) {
					window_state.handle_command(command_manager.get_command_with_name("prev_state"), 0);
				}

			}

			if (event.type == SDL_MOUSEBUTTONUP) {
				if (event.button.button == SDL_BUTTON_LEFT) {
					//window_state.handle_click(event.button.x, event.button.y);
					window_state.handle_left_click(event.button.x, event.button.y, false);
				}

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
						window_state.invalidate_render();
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
		window_state.render(current_pending_command);

		SDL_Delay(16);
		window_state.tick();
	}

	sqlite3_close(db);
	worker.join();
	return 0;
}
