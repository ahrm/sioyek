//todo: cleanup the code
//todo: visibility test is still buggy??
//todo: threading
//todo: add fuzzy search
//todo: handle document memory leak (because documents are not deleted since adding state history)
//todo: tests!
//todo: handle mouse in menues
//todo: sort opened documents by last access

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
#include "config.h"

#define FTS_FUZZY_MATCH_IMPLEMENTATION
#include "fts_fuzzy_match.h"
#undef FTS_FUZZY_MATCH_IMPLEMENTATION


extern float background_color[3] = { 1.0f, 1.0f, 1.0f };
extern float ZOOM_INC_FACTOR = 1.2f;
extern float vertical_move_amount = 1.0f;
extern float horizontal_move_amount = 1.0f;
extern const unsigned int cache_invalid_milies = 1000;
extern const int persist_milies = 1000 * 60;
extern const int page_paddings = 0;
extern const int max_pending_requests = 31;
//extern const char* last_path_file_absolute_location = "C:\\Users\\Lion\\source\\repos\\pdf_viewer\\pdf_viewer\\last_document_path.txt";
string last_path_file_absolute_location;
filesystem::path parent_path;


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
	ConfigManager* config_manager;
	DocumentManager* document_manager;
	DocumentView* main_document_view;
	DocumentView* helper_document_view;

	SDL_Window* main_window;
	SDL_Window* helper_window;
	SDL_GLContext* opengl_context;
	fz_context* mupdf_context;
	sqlite3* database;
	const Command* pending_text_command;

	UIWidget* current_widget;

	//float zoom_level;
	GLuint vertex_array_object;
	GLuint vertex_buffer_object;
	GLuint uv_buffer_object;
	GLuint gl_program;
	GLuint gl_debug_program;
	GLuint gl_unrendered_program;
	GLint highlight_color_uniform_location;

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
	bool is_selecting;
	optional<fz_rect> selected_rect;

	unsigned int last_tick_time;

	bool is_showing_ui;
	bool is_showing_textbar;
	char text_command_buffer[100];
	vector<fz_rect> selected_character_rects;

	int main_window_width, main_window_height;
	int helper_window_width, helper_window_height;

	string selected_text;

	void set_main_document_view_state(DocumentViewState new_view_state) {
		main_document_view = new_view_state.document_view;
		main_document_view->on_view_size_change(main_window_width, main_window_height);
		main_document_view->set_offsets(new_view_state.offset_x, new_view_state.offset_y);
		main_document_view->set_zoom_level(new_view_state.zoom_level);
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
				float link_new_offset_x = main_document_view->get_offset_x();
				float link_new_offset_y = main_document_view->get_offset_y();
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
		PdfRenderer* pdf_renderer,
		ConfigManager* config_manager
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
		current_widget(nullptr),
		pdf_renderer(pdf_renderer),
		config_manager(config_manager),
		is_selecting(false),
		document_manager(new DocumentManager(mupdf_context, database))
	{

		main_document_view = new DocumentView(mupdf_context, database, pdf_renderer, document_manager, config_manager);
		helper_document_view = new DocumentView(mupdf_context, database, pdf_renderer, document_manager, config_manager);

		cached_document_views.push_back(main_document_view);

		gl_program = LoadShaders(parent_path / "shaders\\simple.vertex", parent_path / "shaders\\simple.fragment");
		if (gl_program == 0) {
			cerr << "Error: could not compile shaders" << endl;
		}

		//gl_debug_program = LoadShaders(parent_path / "shaders\\simple.vertex", parent_path / "shaders\\debug.fragment");
		gl_debug_program = LoadShaders(parent_path / "shaders\\simple.vertex", parent_path / "shaders\\highlight.fragment");
		if (gl_debug_program == 0) {
			cerr << "Error: could not compile debug shaders" << endl;
		}

		highlight_color_uniform_location = glGetUniformLocation(gl_debug_program, "highlight_color");
		assert(highlight_color_uniform_location >= 0);

		gl_unrendered_program = LoadShaders(parent_path / "shaders\\simple.vertex", parent_path / "shaders\\unrendered_page.fragment");
		if (gl_unrendered_program == 0) {
			cerr << "Error: could not compile debug shaders" << endl;
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

		if (helper_window) {
			SDL_GetWindowSize(helper_window, &helper_window_width, &helper_window_height);
			helper_document_view->on_view_size_change(helper_window_width, helper_window_height);
		}

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
		}
		if (window_id == SDL_GetWindowID(helper_window)) {
		}
	}

	void handle_command_with_symbol(const Command* command, char symbol) {
		assert(symbol);
		assert(command->requires_symbol);
		if (command->name == "set_mark") {
			assert(main_document_view);
			main_document_view->add_mark(symbol);
			invalidate_render();
		}
		else if (command->name == "goto_mark") {
			assert(main_document_view);
			main_document_view->goto_mark(symbol);
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
		if (command->name == "open_document") {
			//current_document_view->open_document(file_name);
			open_document(file_name);
		}
	}


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
				main_document_view->goto_page(num_repeats);
			}
			else {
				main_document_view->set_offset_y(0.0f);
			}
		}

		if (command->name == "goto_end") {
			main_document_view->goto_end();
		}
		if (command->name == "copy") {
			copy_to_clipboard(selected_text);
		}
		//if (command->name == "search") {
		//	show_searchbar();
		//}
		int rp = max(num_repeats, 1);

		if (command->name == "move_down") {
			main_document_view->move(0.0f, 72.0f * rp * vertical_move_amount);
		}
		else if (command->name == "move_up") {
			main_document_view->move(0.0f, -72.0f * rp * vertical_move_amount);
		}

		else if (command->name == "move_right") {
			main_document_view->move(72.0f * rp * horizontal_move_amount, 0.0f);
		}

		else if (command->name == "move_left") {
			main_document_view->move(-72.0f * rp * horizontal_move_amount, 0.0f);
		}

		else if (command->name == "link") {
			handle_link();
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
			main_document_view->zoom_in();
		}

		else if (command->name == "zoom_out") {
			main_document_view->zoom_out();
		}

		else if (command->name == "next_state") {
			next_state();
		}
		else if (command->name == "prev_state") {
			prev_state();
		}

		else if (command->name == "next_item") {
			main_document_view->goto_search_result(1 + num_repeats);
		}

		else if (command->name == "previous_item") {
			main_document_view->goto_search_result(-1 - num_repeats);
		}
		else if (command->name == "push_state") {
			push_state();
		}
		else if (command->name == "pop_state") {
			prev_state();
		}

		else if (command->name == "next_page") {
			main_document_view->move_pages(1 + num_repeats);
		}
		else if (command->name == "previous_page") {
			main_document_view->move_pages(-1 - num_repeats);
		}
		else if (command->name == "goto_toc") {
			vector<string> flat_toc;
			vector<int> current_document_toc_pages;
			get_flat_toc(main_document_view->get_document()->get_toc(), flat_toc, current_document_toc_pages);
			if (current_document_toc_pages.size() > 0) {
				current_widget = new FilteredSelect<int>(flat_toc, current_document_toc_pages, [&](void* page_pointer) {
					int* page_value = (int*)page_pointer;
					if (page_value) {
						push_state();
						main_document_view->goto_page(*page_value);
					}
					});
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
				current_widget = new FilteredSelect<string>(opened_docs_names, opened_docs_paths, [&](void* string_pointer) {
					string doc_path = *(string*)string_pointer;
					if (doc_path.size() > 0) {
						push_state();
						open_document(doc_path);
					}
					});
				is_showing_ui = true;
			}
		}
		else if (command->name == "goto_bookmark") {
			is_showing_ui = true;
			vector<string> option_names;
			vector<float> option_locations;
			for (int i = 0; i < main_document_view->get_document()->get_bookmarks().size(); i++) {
				option_names.push_back(main_document_view->get_document()->get_bookmarks()[i].description);
				option_locations.push_back(main_document_view->get_document()->get_bookmarks()[i].y_offset);
			}
			current_widget = new FilteredSelect<float>(option_names, option_locations, [&](void* float_pointer) {

				float* offset_value = (float*)float_pointer;
				if (offset_value) {
					push_state();
					main_document_view->set_offset_y(*offset_value);
				}
				});
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
			current_widget = new FilteredSelect<BookState>(descs, book_states, [&](void* book_p) {
				BookState* offset_value = (BookState*)book_p;
				if (offset_value) {
					push_state();
					open_document(offset_value->document_path, 0.0f, offset_value->offset_y);
				}
				});

		}

		else if (command->name == "toggle_fullscreen") {
			toggle_fullscreen();
		}
		else if (command->name == "toggle_one_window") {
			toggle_two_window_mode();
		}

		else if (command->name == "toggle_highlight") {
			main_document_view->toggle_highlight();
			invalidate_render();
		}

		else if (command->name == "search_selected_text_in_google_scholar") {
			ShellExecuteA(0, 0, (*config_manager->get_config<string>("google_scholar_address") + (selected_text)).c_str() , 0, 0, SW_SHOW);
		}
		else if (command->name == "search_selected_text_in_libgen") {
			ShellExecuteA(0, 0, (*config_manager->get_config<string>("libgen_address") + (selected_text)).c_str() , 0, 0, SW_SHOW);
		}
		else if (command->name == "debug") {
			cout << "debug" << endl;
		}

	}


	bool should_render() {
		if (render_is_invalid) {
			return true;
		}
		if (is_selecting) {
			return true;
		}
		if (is_showing_ui) {
			return true;
		}
		if (main_document_view->should_rerender()) {
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
			main_document_view->search_text(text.c_str());
		}

		if (pending_text_command->name == "add_bookmark") {
			main_document_view->add_bookmark(text);
		}
		if (pending_text_command->name == "command") {
			cout << text << endl;
		}
	}





	void on_window_size_changed(Uint32 window_id) {
		if (window_id == SDL_GetWindowID(main_window)) {
			SDL_GetWindowSize(main_window, &main_window_width, &main_window_height);
			main_document_view->on_view_size_change(main_window_width, main_window_height);
		}

		if (helper_window != nullptr && window_id == SDL_GetWindowID(helper_window)) {
			SDL_GetWindowSize(helper_window, &helper_window_width, &helper_window_height);
			helper_document_view->on_view_size_change(helper_window_width, helper_window_height);
		}

		render_is_invalid = true;
	}



	void tick(bool force = false) {
		unsigned int now = SDL_GetTicks();
		if (force || (now - last_tick_time) > persist_milies) {
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
		ss << "Page " << main_document_view->get_current_page_number()+1 << " / " << main_document_view->get_document()->num_pages();
		int num_search_results = main_document_view->get_num_search_results();
		float progress = -1;
		if (num_search_results > 0 || main_document_view->get_is_searching(&progress)) {
			main_document_view->get_is_searching(&progress);

			// show the 0th result if there are no results and the index + 1 otherwise
			int result_index = main_document_view->get_num_search_results() > 0 ? main_document_view->get_current_search_result_index() + 1 : 0;
			ss << " | showing result " << result_index << " / " << num_search_results;
			if (progress > 0) {
				ss << " (" << ((int)(progress * 100)) << "%%" << ")";
			}
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

		main_document_view = new DocumentView(mupdf_context, database, pdf_renderer, document_manager, config_manager);
		main_document_view->open_document(path);
		if (path.size() > 0 && main_document_view->get_document() == nullptr) {
			show_error_message("Could not open file: " + path);
		}
		main_document_view->on_view_size_change(main_window_width, main_window_height);
		if (offset_x) {
			main_document_view->set_offset_x(offset_x.value());
		}
		if (offset_y) {
			main_document_view->set_offset_y(offset_y.value());
		}
		cached_document_views.push_back(main_document_view);

		if (path.size() > 0) {
			ofstream last_path_file(last_path_file_absolute_location);
			last_path_file << path << endl;
			last_path_file.close();
		}
	}
	void toggle_two_window_mode() {
		if (helper_window == nullptr) {
			SDL_Rect first_display_rect;
			SDL_GetDisplayBounds(0, &first_display_rect);
			helper_window = SDL_CreateWindow("Pdf Viewer2", first_display_rect.w, SDL_WINDOWPOS_UNDEFINED, 500, 500, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_MAXIMIZED);
			SDL_SetWindowInputFocus(main_window);
		}
		else{
			SDL_DestroyWindow(helper_window);
			helper_window = nullptr;
		}
	}

	void toggle_fullscreen() {
		bool is_fullscreen = SDL_GetWindowFlags(main_window) & SDL_WINDOW_FULLSCREEN_DESKTOP;
		auto new_flag = is_fullscreen ? 0 : SDL_WINDOW_FULLSCREEN_DESKTOP;

		if (helper_window == nullptr) {
			SDL_SetWindowFullscreen(main_window, new_flag);
		}
		else{
			SDL_SetWindowFullscreen(main_window, new_flag);
			SDL_SetWindowFullscreen(helper_window, new_flag);
		}
	}

	void handle_escape() {
		is_showing_ui = false;
		is_showing_textbar = false;
		ZeroMemory(text_command_buffer, sizeof(text_command_buffer));
		pending_link_source_document_path = "";
		pending_link_source_filled = false;
		pending_text_command = nullptr;
		if (current_widget) {
			delete current_widget;
			current_widget = nullptr;
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
		if (ImGui::GetIO().WantCaptureMouse) {
			return;
		}
		float x_, y_;
		main_document_view->window_to_absolute_document_pos(x, y, &x_, &y_);

		if (down == true) {
			last_mouse_down_x = x_;
			last_mouse_down_y = y_;
			selected_character_rects.clear();
			is_selecting = true;
		}
		else {
			is_selecting = false;
			if ((abs(last_mouse_down_x - x_) + abs(last_mouse_down_y - y_)) > 20) {
				//fz_rect sr;
				//sr.x0 = min(last_mouse_down_x, x_);
				//sr.x1 = max(last_mouse_down_x, x_);

				//sr.y0 = min(last_mouse_down_y, y_);
				//sr.y1 = max(last_mouse_down_y, y_);
				//selected_rect = sr;
				fz_point selection_begin = { last_mouse_down_x, last_mouse_down_y };
				fz_point selection_end = { x_, y_ };

				main_document_view->get_text_selection(selection_begin, selection_end, selected_character_rects, selected_text);
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

	void handle_mouse_move(float x, float y) {
		float x_, y_;
		main_document_view->window_to_absolute_document_pos(x, y, &x_, &y_);

		fz_point selection_begin = { last_mouse_down_x, last_mouse_down_y };
		fz_point selection_end = { x_, y_ };
		vector<fz_rect> selected_characters;
		string text;
		main_document_view->get_text_selection(selection_begin, selection_end, selected_characters, text);
		for (const auto& r : selected_characters) {
			main_document_view->render_highlight_absolute(gl_debug_program, r);
		}
		//main_document_view->render_highlight_absolute(gl_debug_program, sr);
	}

	void render(const Command* pending_command) {
		if (should_render()) {
			int mouse_x, mouse_y;
			SDL_GetMouseState(&mouse_x, &mouse_y);

			cout << "rendering ..." << endl;

			//current_document_view->get_document()->find_closest_link(current_document_view->)
			Link* link = main_document_view->find_closest_link();
			if (link ) {
				if (helper_document_view->get_document() && 
					helper_document_view->get_document()->get_path() == link->document_path) {

					helper_document_view->set_offsets(link->dest_offset_x, link->dest_offset_y);
				}
				else {
					delete helper_document_view;
					helper_document_view = new DocumentView(mupdf_context, database, pdf_renderer, document_manager, config_manager, link->document_path,
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

			main_document_view->render(gl_program, gl_unrendered_program, gl_debug_program, highlight_color_uniform_location);

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
						main_document_view->goto_search_result(0);
						handle_return();
						io.WantCaptureKeyboard = false;
					}
					ImGui::End();
				}
				if (current_widget) {
					if (current_widget->render()) {
						delete current_widget;
						current_widget = nullptr;
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

			glUseProgram(gl_debug_program);
			glUniform3fv(highlight_color_uniform_location, 1, config_manager->get_config<float>("text_highlight_color"));

			for (auto rect : selected_character_rects) {
				main_document_view->render_highlight_absolute(gl_debug_program, rect);
			}
			if (is_selecting) {
				handle_mouse_move(mouse_x, mouse_y);
			}

			//if (selected_rect.has_value()) {
			//	fz_rect sr = selected_rect.value();
			//	main_document_view->render_highlight_absolute(gl_debug_program, sr);
			//}

			SDL_GL_SwapWindow(main_window);

			if (helper_window != nullptr) {
				SDL_GL_MakeCurrent(helper_window, *opengl_context);

				glViewport(0, 0, helper_window_width, helper_window_height);

				glClearColor(1.0f, 0.0f, 0.0f, 1.0f);
				glClear(GL_COLOR_BUFFER_BIT);
				helper_document_view->render(gl_program, gl_unrendered_program, gl_debug_program, highlight_color_uniform_location);
				SDL_GL_SwapWindow(helper_window);
			}

			pdf_renderer->delete_old_pages();
		}
	}
};


int main(int argc, char* args[]) {

	bool quit = false;
		
	char exe_file_name[MAX_PATH];
	GetModuleFileNameA(NULL, exe_file_name, sizeof(exe_file_name));

#ifdef NDEBUG
	install_app(exe_file_name);
#endif

	filesystem::path exe_path = exe_file_name;
	parent_path = exe_path.parent_path();

	//comment this is release mode
#ifdef _DEBUG
	parent_path = "C:\\Users\\Lion\\source\\repos\\pdf_viewer\\pdf_viewer";
#endif

	last_path_file_absolute_location = (parent_path / "last_document_path.txt").string();

	filesystem::path config_path = parent_path / "prefs.config";
	ConfigManager config_manager(config_path.string());
	auto last_config_write_time = std::filesystem::last_write_time(config_path);

	const float* text_h = config_manager.get_config<float>("text_highlight_color");

	sqlite3* db;
	char* error_message = nullptr;
	int rc;

	rc = sqlite3_open((parent_path / "test.db").string().c_str(), &db);
	if (rc) {
		cerr << "could not open database" << sqlite3_errmsg(db) << endl;
	}

	create_tables(db);

	fz_locks_context locks;
	locks.user = mupdf_mutexes;
	locks.lock = lock_mutex;
	locks.unlock = unlock_mutex;

	fz_context* mupdf_context = fz_new_context(nullptr, &locks, FZ_STORE_UNLIMITED);
	//fz_context* mupdf_context = fz_new_context(nullptr, nullptr, FZ_STORE_UNLIMITED);
	if (!mupdf_context) {
		cerr << "could not create mupdf context" << endl;
		return -1;
	}
	bool fail = false;
	fz_try(mupdf_context) {
		fz_register_document_handlers(mupdf_context);
	}
	fz_catch(mupdf_context) {
		cerr << "could not register document handlers" << endl;
		fail = true;
	}

	if (fail) {
		return -1;
	}

	PdfRenderer* pdf_renderer = new PdfRenderer(mupdf_context);
	//global_pdf_renderer = new PdfRenderer();
	//global_pdf_renderer->init();

	thread worker([pdf_renderer, &quit]() {
		pdf_renderer->run(&quit);
		});


	SDL_Window* window = nullptr;
	SDL_Window* window2 = nullptr;

	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		cerr << "could not initialize SDL" << endl;
		return -1;
	}

	CommandManager command_manager;

	bool one_window_mode = false;
	int num_displays = SDL_GetNumVideoDisplays();
	if (num_displays == 1) {
		one_window_mode = true;
	}

	SDL_Rect display_rect;
	SDL_GetDisplayBounds(0, &display_rect);

	if (!one_window_mode) {
		window2 = SDL_CreateWindow("Pdf Viewer2", display_rect.w, SDL_WINDOWPOS_UNDEFINED, 500, 500, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_MAXIMIZED);
	}
	window = SDL_CreateWindow("Pdf Viewer", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 500, 500, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_MAXIMIZED);

	if (window == nullptr) {
		cerr << "could not create SDL window" << endl;
		return -1;
	}
	if (window2 == nullptr && !one_window_mode) {
		cerr << "could not create the second window" << endl;
		return -1;
	}

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 1);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

	SDL_GLContext gl_context = SDL_GL_CreateContext(window);

	if (gl_context == nullptr) {
		cerr << SDL_GetError() << endl;
		cerr << "could not create opengl context" << endl;
		return -1;
	}

	glewExperimental = true;
	GLenum glew_error = glewInit();
	if (glew_error != GLEW_OK) {
		cerr << "could not initialize glew" << endl;
		return -1;
	}

	if (SDL_GL_SetSwapInterval(1) < 0) {
		cerr << "could not enable vsync" << endl;
		return -1;
	}

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	ImGui::StyleColorsDark();
	ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
	ImGui_ImplOpenGL3_Init("#version 400");

	WindowState window_state(window, window2, mupdf_context, &gl_context, db, pdf_renderer, &config_manager);
	if (argc > 1) {
		window_state.open_document(args[1]);
	}

	//char file_path[MAX_PATH] = { 0 };
	string file_path;
	ifstream last_state_file(last_path_file_absolute_location);
	//last_state_file.read(file_path, MAX_PATH);
	getline(last_state_file, file_path);
	//last_state_file >> initial_document;
	last_state_file.close();

	window_state.open_document(file_path);


	pdf_renderer->set_invalidate_pointer(&window_state.render_is_invalid);
	InputHandler input_handler((parent_path / "keys.config").wstring());

	bool is_waiting_for_symbol = false;
	const Command* current_pending_command = nullptr;

	while (!quit) {

		auto current_last_write_time = filesystem::last_write_time(config_path);
		if (current_last_write_time != last_config_write_time) {
			last_config_write_time = current_last_write_time;
			ifstream config_file(config_path);
			config_manager.deserialize(config_file);
			config_file.close();
			window_state.invalidate_render();
		}

		SDL_Event event;

		//while (SDL_PollEvent(&event)) {
		while (SDL_WaitEventTimeout(&event, 2)) {
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
							cerr << "File select failed" << endl;
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

		//SDL_Delay(100);
		window_state.tick();
	}
	window_state.tick(true);

	sqlite3_close(db);
	worker.join();
	return 0;
}
