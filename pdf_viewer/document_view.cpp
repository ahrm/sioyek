#include "document_view.h"

extern float background_color[3];
extern float move_screen_percentage;

DocumentView::DocumentView( fz_context* mupdf_context,
	sqlite3* db,
	DocumentManager* document_manager,
	ConfigManager* config_manager) :
	mupdf_context(mupdf_context),
	database(db),
	document_manager(document_manager),
	config_manager(config_manager)
{

}

DocumentView::DocumentView(fz_context* mupdf_context,
	sqlite3* db,
	DocumentManager* document_manager,
	ConfigManager* config_manager,
	wstring path,
	int view_width,
	int view_height,
	float offset_x,
	float offset_y) :
	DocumentView(mupdf_context,
		db,
		document_manager,
		config_manager)

{
	on_view_size_change(view_width, view_height);
	open_document(path);
	set_offsets(offset_x, offset_y);
}

float DocumentView::get_zoom_level() {
	return zoom_level;
}

DocumentViewState DocumentView::get_state() {
	DocumentViewState res;

	if (current_document) {
		res.document_path = current_document->get_path();
		res.offset_x = get_offset_x();
		res.offset_y = get_offset_y();
		res.zoom_level = get_zoom_level();
	}
	return res;
}

void DocumentView::handle_escape() {
}

void DocumentView::set_offsets(float new_offset_x, float new_offset_y) {
	offset_x = new_offset_x;
	offset_y = new_offset_y;
}

Document* DocumentView::get_document() {
	return current_document;
}

//int DocumentView::get_num_search_results() {
//	search_results_mutex.lock();
//	int num = search_results.size();
//	search_results_mutex.unlock();
//	return num;
//}
//
//int DocumentView::get_current_search_result_index() {
//	return current_search_result_index;
//}

Link* DocumentView::find_closest_link() {
	if (current_document) {
		return current_document->find_closest_link(offset_y);
	}
	return nullptr;
}

void DocumentView::delete_closest_link() {
	if (current_document) {
		current_document->delete_closest_link(offset_y);
	}
}

void DocumentView::delete_closest_bookmark() {
	if (current_document) {
		current_document->delete_closest_bookmark(offset_y);
	}
}

float DocumentView::get_offset_x() {
	return offset_x;
}

float DocumentView::get_offset_y() {
	return offset_y;
}

int DocumentView::get_view_height()
{
	return view_height;
}

int DocumentView::get_view_width()
{
	return view_width;
}

void DocumentView::set_null_document()
{
	current_document = nullptr;
}

void DocumentView::set_offset_x(float new_offset_x) {
	set_offsets(new_offset_x, offset_y);
}

void DocumentView::set_offset_y(float new_offset_y) {
	set_offsets(offset_x, new_offset_y);
}

//void DocumentView::render_highlight_window( GLuint program, fz_rect window_rect) {
//	float quad_vertex_data[] = {
//		window_rect.x0, window_rect.y1,
//		window_rect.x1, window_rect.y1,
//		window_rect.x0, window_rect.y0,
//		window_rect.x1, window_rect.y0
//	};
//	glEnable(GL_BLEND);
//	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
//	glUseProgram(program);
//	glEnableVertexAttribArray(0);
//	glEnableVertexAttribArray(1);
//	glBufferData(GL_ARRAY_BUFFER, sizeof(quad_vertex_data), quad_vertex_data, GL_DYNAMIC_DRAW);
//	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
//	glDisable(GL_BLEND);
//}

//void DocumentView::render_highlight_absolute( GLuint program, fz_rect absolute_document_rect) {
//	fz_rect window_rect = absolute_to_window_rect(absolute_document_rect);
//	render_highlight_window(program, window_rect);
//}
//
//void DocumentView::render_highlight_document( GLuint program, int page, fz_rect doc_rect) {
//	fz_rect window_rect = document_to_window_rect(page, doc_rect);
//	render_highlight_window(program, window_rect);
//}

optional<PdfLink> DocumentView::get_link_in_pos(int view_x, int view_y) {
	if (!current_document) return {};

	float doc_x, doc_y;
	int page;
	window_to_document_pos(view_x, view_y, &doc_x, &doc_y, &page);

	fz_link* links = current_document->get_page_links(page);
	fz_point point = { doc_x, doc_y };
	optional<PdfLink> res = {};

	bool found = false;
	while (links != nullptr) {
		if (fz_is_point_inside_rect(point, links->rect)) {
			res = { links->rect, links->uri };
			return res;
		}
		links = links->next;
	}
	return {};
}
void DocumentView::add_mark(char symbol) {
	//assert(current_document);
	if (current_document) {
		current_document->add_mark(symbol, offset_y);
	}
}
void DocumentView::add_bookmark(wstring desc) {
	//assert(current_document);
	if (current_document) {
		current_document->add_bookmark(desc, offset_y);
	}
}
void DocumentView::on_view_size_change(int new_width, int new_height) {
	view_width = new_width;
	view_height = new_height;
}

void DocumentView::absolute_to_window_pos(float absolute_x, float absolute_y, float* window_x, float* window_y) {
	float half_width = static_cast<float>(view_width) / zoom_level / 2;
	float half_height = static_cast<float>(view_height) / zoom_level / 2;

	*window_x = (absolute_x + offset_x) / half_width;
	*window_y = (-absolute_y + offset_y) / half_height;
}
fz_rect DocumentView::absolute_to_window_rect(fz_rect doc_rect) {
	fz_rect res;
	absolute_to_window_pos(doc_rect.x0, doc_rect.y0, &res.x0, &res.y0);
	absolute_to_window_pos(doc_rect.x1, doc_rect.y1, &res.x1, &res.y1);

	return res;
}
void DocumentView::document_to_window_pos(int page, float doc_x, float doc_y, float* window_x, float* window_y) {

	if (current_document) {
		double doc_rect_y_offset = -current_document->get_accum_page_height(page);

		float half_width = static_cast<float>(view_width) / zoom_level / 2;
		float half_height = static_cast<float>(view_height) / zoom_level / 2;

		*window_y = (doc_rect_y_offset + offset_y - doc_y) / half_height;
		*window_x = (doc_x + offset_x - current_document->get_page_width(page) / 2) / half_width;
	}
}
fz_rect DocumentView::document_to_window_rect(int page, fz_rect doc_rect) {
	fz_rect res;
	document_to_window_pos(page, doc_rect.x0, doc_rect.y0, &res.x0, &res.y0);
	document_to_window_pos(page, doc_rect.x1, doc_rect.y1, &res.x1, &res.y1);

	return res;
}
void DocumentView::window_to_document_pos(float window_x, float window_y, float* doc_x, float* doc_y, int* doc_page) {
	if (current_document) {
		current_document->absolute_to_page_pos(
			(window_x - view_width / 2) / zoom_level - offset_x,
			(window_y - view_height / 2) / zoom_level + offset_y, doc_x, doc_y, doc_page);
	}
}
void DocumentView::window_to_absolute_document_pos(float window_x, float window_y, float* doc_x, float* doc_y) {
	*doc_x = (window_x - view_width / 2) / zoom_level - offset_x;
	*doc_y = (window_y - view_height / 2) / zoom_level + offset_y;
}
void DocumentView::goto_mark(char symbol) {
	if (current_document) {
		float new_y_offset = 0.0f;
		if (current_document->get_mark_location_if_exists(symbol, &new_y_offset)) {
			set_offset_y(new_y_offset);
		}
	}
}
void DocumentView::goto_end() {
	if (current_document) {
		const vector<float>& accum_page_heights = current_document->get_accum_page_heights();
		set_offset_y(accum_page_heights[accum_page_heights.size() - 1]);
	}
}
float DocumentView::set_zoom_level(float zl) {
	zoom_level = zl;
	return zoom_level;
}
float DocumentView::zoom_in() {
	return set_zoom_level(zoom_level * ZOOM_INC_FACTOR);
}
float DocumentView::zoom_out() {
	return set_zoom_level(zoom_level / ZOOM_INC_FACTOR);
}
void DocumentView::move_absolute(float dx, float dy) {
	set_offsets(offset_x + dx, offset_y + dy);
}
void DocumentView::move(float dx, float dy) {
	int abs_dx = (dx / zoom_level);
	int abs_dy = (dy / zoom_level);
	move_absolute(abs_dx, abs_dy);
}
int DocumentView::get_current_page_number() {
	vector<int> visible_pages;
	int window_height, window_width;
	get_visible_pages(100, visible_pages);
	if (visible_pages.size() > 0) {
		return visible_pages[0];
	}
	return -1;
}

//void DocumentView::search_text(const wchar_t* text) {
//	if (!current_document) return;
//
//	search_results_mutex.lock();
//	search_results.clear();
//	current_search_result_index = 0;
//	search_results_mutex.unlock();
//
//	is_searching = true;
//	pdf_renderer->add_request(current_document->get_path(), 
//		get_current_page_number(), text, &search_results, &percent_done, &is_searching, &search_results_mutex);
//}

//void DocumentView::goto_search_result(int offset) {
//	if (!current_document) return;
//
//	search_results_mutex.lock();
//	if (search_results.size() == 0) {
//		search_results_mutex.unlock();
//		return;
//	}
//
//	int target_index = mod(current_search_result_index + offset, search_results.size());
//	current_search_result_index = target_index;
//
//	int target_page = search_results[target_index].page;
//
//	fz_rect rect = search_results[target_index].rect;
//
//	float new_offset_y = rect.y0 + current_document->get_accum_page_height(target_page);
//
//	set_offset_y(new_offset_y);
//	search_results_mutex.unlock();
//}
void DocumentView::get_visible_pages(int window_height, vector<int>& visible_pages) {
	if (!current_document) return;
	float window_y_range_begin = offset_y - window_height / (zoom_level);
	float window_y_range_end = offset_y + window_height / (zoom_level);
	window_y_range_begin -= 1;
	window_y_range_end += 1;

	float page_begin = 0.0f;

	const vector<float>& page_heights = current_document->get_page_heights();
	for (int i = 0; i < page_heights.size(); i++) {
		float page_end = page_begin + page_heights[i];

		if (intersects(window_y_range_begin, window_y_range_end, page_begin, page_end)) {
			visible_pages.push_back(i);
		}
		page_begin = page_end;
	}
}
void DocumentView::move_pages(int num_pages) {
	if (!current_document) return;
	int current_page = get_current_page_number();
	if (current_page == -1) {
		current_page = 0;
	}
	move_absolute(0, num_pages * (current_document->get_page_height(current_page) + page_paddings));
}

void DocumentView::move_screens(int num_screens)
{
	float screen_height_in_doc_space = view_height / zoom_level;
	set_offset_y(get_offset_y() + num_screens * screen_height_in_doc_space * move_screen_percentage);
}

void DocumentView::reset_doc_state() {
	zoom_level = 1.0f;
	set_offsets(0.0f, 0.0f);
}
void DocumentView::open_document(wstring doc_path, bool load_prev_state) {

	error_code error_code;
	filesystem::path cannonical_path_ = std::filesystem::canonical(doc_path, error_code);

	if (error_code) {
		current_document = nullptr;
		return;
	}

	wstring cannonical_path = cannonical_path_.wstring();

	//document_path = cannonical_path;


	//current_document = new Document(mupdf_context, doc_path, database);
	current_document = document_manager->get_document(doc_path);
	//current_document->open();
	if (!current_document->open()) {
		current_document = nullptr;
	}

	reset_doc_state();

	if (load_prev_state) {

		vector<OpenedBookState> prev_state;
		if (select_opened_book(database, cannonical_path, prev_state)) {
			if (prev_state.size() > 1) {
				cerr << "more than one file with one path, this should not happen!" << endl;
			}
		}
		if (prev_state.size() > 0) {
			OpenedBookState previous_state = prev_state[0];
			zoom_level = previous_state.zoom_level;
			offset_x = previous_state.offset_x;
			offset_y = previous_state.offset_y;
			set_offsets(previous_state.offset_x, previous_state.offset_y);
		}
	}
}
float DocumentView::get_page_offset(int page) {

	if (!current_document) return 0.0f;

	int max_page = current_document->num_pages();
	if (page > max_page) {
		page = max_page;
	}
	return current_document->get_accum_page_height(page);
}

void DocumentView::goto_offset_within_page(int page, float offset_x, float offset_y) {
	set_offsets(offset_x, get_page_offset(page - 1) + offset_y);
}

void DocumentView::goto_page(int page) {
	set_offset_y(get_page_offset(page) + view_height_in_document_space()/2);
}

void DocumentView::fit_to_page_width()
{
	int cp = get_current_page_number();
	int page_width = current_document->get_page_width(cp);
	set_offset_x(0);
	set_zoom_level(static_cast<float>(view_width) / page_width);
	//set_zoom_level()
}

//void DocumentView::render_page(int page_number) {
//
//	if (!current_document) return;
//
//	GLuint texture = pdf_renderer->find_rendered_page(current_document->get_path(), page_number, zoom_level, nullptr, nullptr);
//
//	float page_vertices[4 * 2];
//	fz_rect page_rect = { 0, 0, current_document->get_page_width(page_number), current_document->get_page_height(page_number) };
//	fz_rect window_rect = document_to_window_rect(page_number, page_rect);
//	rect_to_quad(window_rect, page_vertices);
//
//	if (texture != 0) {
//
//		glUseProgram(rendered_program);
//		glBindTexture(GL_TEXTURE_2D, texture);
//	}
//	else {
//		glUseProgram(unrendered_program);
//	}
//	glEnableVertexAttribArray(0);
//	glEnableVertexAttribArray(1);
//	glBufferData(GL_ARRAY_BUFFER, sizeof(page_vertices), page_vertices, GL_DYNAMIC_DRAW);
//	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
//}
//
//void DocumentView::render() {
//	if (current_document == nullptr) {
//		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
//		glClear(GL_COLOR_BUFFER_BIT);
//		return;
//	}
//
//	vector<int> visible_pages;
//	get_visible_pages(view_height, visible_pages);
//	render_is_invalid = false;
//
//	glClearColor(background_color[0], background_color[1], background_color[2], 1.0f);
//	glClear(GL_COLOR_BUFFER_BIT);
//
//
//	for (int page : visible_pages) {
//		render_page( page );
//
//		if (should_highlight_links) {
//			glUseProgram(highlight_program);
//			glUniform3fv(highlight_color_uniform_location, 1, config_manager->get_config<float>(L"link_highlight_color"));
//			fz_link* links = current_document->get_page_links(page);
//			while (links != nullptr) {
//				render_highlight_document(highlight_program, page, links->rect);
//				links = links->next;
//			}
//		}
//	}
//
//	search_results_mutex.lock();
//	if (search_results.size() > 0) {
//		SearchResult current_search_result = search_results[current_search_result_index];
//		glUseProgram(highlight_program);
//		glUniform3fv(highlight_color_uniform_location, 1, config_manager->get_config<float>(L"search_highlight_color"));
//		render_highlight_document(highlight_program, current_search_result.page, current_search_result.rect);
//	}
//	search_results_mutex.unlock();
//}
void DocumentView::persist() {
	if (!current_document) return;
	update_book(database, current_document->get_path(), zoom_level, offset_x, offset_y);
}

int DocumentView::get_current_chapter_index()
{
	const vector<int>& chapter_pages = current_document->get_flat_toc_pages();

	if (chapter_pages.size() == 0) {
		return -1;
	}

	int cp = get_current_page_number();

	int current_chapter_index = 0;

	int index = 0;
	for (int p : chapter_pages) {
		if (p <= cp) {
			current_chapter_index = index;
		}
		index++;
	}

	return current_chapter_index;
}

wstring DocumentView::get_current_chapter_name()
{
	const vector<wstring>& chapter_names = current_document->get_flat_toc_names();
	int current_chapter_index = get_current_chapter_index();
	if (current_chapter_index > 0) {
		return chapter_names[current_chapter_index];
	}
	return L"";
}

optional<pair<int, int>> DocumentView::get_current_page_range()
{
	int ci = get_current_chapter_index();
	if (ci < 0) {
		return {};
	}
	const vector<int>& chapter_pages = current_document->get_flat_toc_pages();
	int range_begin = chapter_pages[ci];
	int range_end = current_document->num_pages()-1;

	if (ci < chapter_pages.size() - 1) {
		range_end = chapter_pages[ci + 1];
	}

	return make_pair(range_begin, range_end);
}

void DocumentView::goto_chapter(int diff)
{
	const vector<int>& chapter_pages = current_document->get_flat_toc_pages();
	int curr_page = get_current_page_number();

	int index = 0;

	while (index < chapter_pages.size() && chapter_pages[index] < curr_page) {
		index++;
	}

	int new_index = index + diff;
	if (new_index < 0) {
		goto_page(0);
	}
	else if (new_index >= chapter_pages.size()) {
		goto_end();
	}
	else {
		goto_page(chapter_pages[new_index]);
	}
}

float DocumentView::view_height_in_document_space()
{
	return static_cast<float>(view_height) / zoom_level;
}


void DocumentView::get_text_selection(fz_point selection_begin, fz_point selection_end, vector<fz_rect>& selected_characters, wstring& selected_text) {

	if (!current_document) return;
	int page_begin, page_end;
	fz_rect page_rect;

	selected_characters.clear();
	selected_text.clear();

	fz_rect abs_rect = corners_to_rect(selection_begin, selection_end);

	current_document->absolute_to_page_pos(abs_rect.x0, abs_rect.y0, &page_rect.x0, &page_rect.y0, &page_begin);
	current_document->absolute_to_page_pos(abs_rect.x1, abs_rect.y1, &page_rect.x1, &page_rect.y1, &page_end);

	fz_page* page = nullptr;
	fz_stext_page* stext_page = nullptr;
	selected_text.clear();

	for (int i = page_begin; i <= page_end; i++) {
		fz_try(mupdf_context) {
			page = fz_load_page(mupdf_context, current_document->doc, i);
			stext_page = fz_new_stext_page_from_page(mupdf_context, page, nullptr);
		}
		fz_catch(mupdf_context) {
			cerr << "Error: could not load page for selection" << endl;
		}


		fz_stext_block* current_block = stext_page->first_block;
		while (current_block) {

			if (current_block->type != FZ_STEXT_BLOCK_TEXT) {
				continue;
			}

			fz_stext_line* current_line = current_block->u.t.first_line;
			while (current_line) {
				fz_stext_char* current_char = current_line->first_char;
				bool has_char_in_line = false;
				while (current_char) {
					fz_rect charrect = current_document->page_rect_to_absolute_rect(i, fz_rect_from_quad(current_char->quad));

					if (should_select_char(selection_begin, selection_end, charrect)){
						has_char_in_line = true;
						selected_text.push_back(current_char->c);
						selected_characters.push_back(charrect);
					}
					current_char = current_char->next;
				}
				if (has_char_in_line) {
					selected_text.push_back(' ');
				}
				current_line = current_line->next;
			}

			current_block = current_block->next;
		}

		// there is one extra space character, we append a space for each line
		if (selected_text.size() > 0) {
			selected_text.pop_back();
		}

		fz_drop_stext_page(mupdf_context, stext_page);
		fz_drop_page(mupdf_context, page);
	}

}
