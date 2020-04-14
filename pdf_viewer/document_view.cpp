#include "document_view.h"

DocumentView::DocumentView(fz_context* mupdf_context, sqlite3* db, PdfRenderer* pdf_renderer, DocumentManager* document_manager) :
	mupdf_context(mupdf_context),
	database(db),
	pdf_renderer(pdf_renderer),
	document_manager(document_manager)
{

}

DocumentView::DocumentView(fz_context* mupdf_context, sqlite3* db, PdfRenderer* pdf_renderer, DocumentManager* document_manager, string path, int view_width, int view_height, float offset_x, float offset_y) :
	DocumentView(mupdf_context, db, pdf_renderer, document_manager)
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
	res.document_view = this;
	res.offset_x = get_offset_x();
	res.offset_y = get_offset_y();
	res.zoom_level = get_zoom_level();
	return res;
}

void DocumentView::handle_escape() {
	search_results.clear();
	current_search_result_index = 0;
}

void DocumentView::set_offsets(float new_offset_x, float new_offset_y) {
	offset_x = new_offset_x;
	offset_y = new_offset_y;
	render_is_invalid = true;
}

Document* DocumentView::get_document() {
	return current_document;
}

int DocumentView::get_num_search_results() {
	return search_results.size();
}

int DocumentView::get_current_search_result_index() {
	return current_search_result_index;
}

Link* DocumentView::find_closest_link() {
	if (current_document) {
		return current_document->find_closest_link(offset_y);
	}
	return nullptr;
}

void DocumentView::delete_closest_link() {
	current_document->delete_closest_link(offset_y);
}

void DocumentView::delete_closest_bookmark() {
	current_document->delete_closest_bookmark(offset_y);
}

float DocumentView::get_offset_x() {
	return offset_x;
}

float DocumentView::get_offset_y() {
	return offset_y;
}

void DocumentView::set_offset_x(float new_offset_x) {
	set_offsets(new_offset_x, offset_y);
}

void DocumentView::set_offset_y(float new_offset_y) {
	set_offsets(offset_x, new_offset_y);
}

void DocumentView::render_highlight_window(GLuint program, fz_rect window_rect) {
	float quad_vertex_data[] = {
		window_rect.x0, window_rect.y1,
		window_rect.x1, window_rect.y1,
		window_rect.x0, window_rect.y0,
		window_rect.x1, window_rect.y0
	};
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glUseProgram(program);
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glBufferData(GL_ARRAY_BUFFER, sizeof(quad_vertex_data), quad_vertex_data, GL_DYNAMIC_DRAW);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glDisable(GL_BLEND);
}

void DocumentView::render_highlight_absolute(GLuint program, fz_rect absolute_document_rect) {
	fz_rect window_rect = absolute_to_window_rect(absolute_document_rect);
	render_highlight_window(program, window_rect);
}

void DocumentView::render_highlight_document(GLuint program, int page, fz_rect doc_rect) {
	fz_rect window_rect = document_to_window_rect(page, doc_rect);
	render_highlight_window(program, window_rect);
}

optional<PdfLink> DocumentView::get_link_in_pos(int view_x, int view_y) {
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
	assert(current_document);
	current_document->add_mark(symbol, offset_y);
}
void DocumentView::add_bookmark(string desc) {
	assert(current_document);
	current_document->add_bookmark(desc, offset_y);
}
void DocumentView::on_view_size_change(int new_width, int new_height) {
	view_width = new_width;
	view_height = new_height;
	render_is_invalid = true;
}
bool DocumentView::should_rerender() {
	return render_is_invalid;
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

	double doc_rect_y_offset = -current_document->get_accum_page_height(page);

	float half_width = static_cast<float>(view_width) / zoom_level / 2;
	float half_height = static_cast<float>(view_height) / zoom_level / 2;

	*window_y = (doc_rect_y_offset + offset_y - doc_y) / half_height;
	*window_x = (doc_x + offset_x - current_document->get_page_width(page) / 2) / half_width;
}
fz_rect DocumentView::document_to_window_rect(int page, fz_rect doc_rect) {
	fz_rect res;
	document_to_window_pos(page, doc_rect.x0, doc_rect.y0, &res.x0, &res.y0);
	document_to_window_pos(page, doc_rect.x1, doc_rect.y1, &res.x1, &res.y1);

	return res;
}
void DocumentView::window_to_document_pos(float window_x, float window_y, float* doc_x, float* doc_y, int* doc_page) {
	current_document->absolute_to_page_pos(
		(window_x - view_width / 2) / zoom_level + offset_x,
		(window_y - view_height / 2) / zoom_level + offset_y, doc_x, doc_y, doc_page);
}
void DocumentView::window_to_absolute_document_pos(float window_x, float window_y, float* doc_x, float* doc_y) {
	*doc_x = (window_x - view_width / 2) / zoom_level + offset_x;
	*doc_y = (window_y - view_height / 2) / zoom_level + offset_y;
}
void DocumentView::goto_mark(char symbol) {
	assert(current_document);
	float new_y_offset = 0.0f;
	if (current_document->get_mark_location_if_exists(symbol, &new_y_offset)) {
		set_offset_y(new_y_offset);
	}
}
void DocumentView::goto_end() {
	const vector<float>& accum_page_heights = current_document->get_accum_page_heights();
	set_offset_y(accum_page_heights[accum_page_heights.size() - 1]);
}
float DocumentView::set_zoom_level(float zl) {
	zoom_level = zl;
	render_is_invalid = true;
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
int DocumentView::search_text(const char* text) {
	search_results.clear();

	int num_pages = current_document->num_pages();
	int total_results = 0;

	for (int i = 0; i < num_pages; i++) {
		fz_page* page = fz_load_page(mupdf_context, current_document->doc, i);

		const int max_hits_per_page = 20;
		fz_quad hitboxes[max_hits_per_page];
		int num_results = fz_search_page(mupdf_context, page, text, hitboxes, max_hits_per_page);

		for (int j = 0; j < num_results; j++) {
			search_results.push_back(SearchResult{ fz_rect_from_quad(hitboxes[j]), i });
		}

		total_results += num_results;

		fz_drop_page(mupdf_context, page);
	}
	return total_results;
}
void DocumentView::goto_search_result(int offset) {
	if (search_results.size() == 0) {
		return;
	}

	int target_index = mod(current_search_result_index + offset, search_results.size());
	current_search_result_index = target_index;

	int target_page = search_results[target_index].page;

	fz_rect rect = search_results[target_index].rect;

	float new_offset_y = rect.y0 + current_document->get_accum_page_height(target_page);

	set_offset_y(new_offset_y);
}
void DocumentView::get_visible_pages(int window_height, vector<int>& visible_pages) {
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
	int current_page = get_current_page_number();
	if (current_page == -1) {
		current_page = 0;
	}
	move_absolute(0, num_pages * (current_document->get_page_height(current_page) + page_paddings));
}
void DocumentView::reset_doc_state() {
	zoom_level = 1.0f;
	set_offsets(0.0f, 0.0f);
}
void DocumentView::open_document(string doc_path) {

	string cannonical_path = std::filesystem::canonical(doc_path).string();
	//document_path = cannonical_path;
	vector<OpenedBookState> prev_state;

	if (select_opened_book(database, cannonical_path, prev_state)) {
		if (prev_state.size() > 1) {
			cout << "more than one file with one path, this should not happen!" << endl;
		}
	}

	//current_document = new Document(mupdf_context, doc_path, database);
	current_document = document_manager->get_document(doc_path);
	current_document->open();

	reset_doc_state();
	if (prev_state.size() > 0) {
		OpenedBookState previous_state = prev_state[0];
		zoom_level = previous_state.zoom_level;
		offset_x = previous_state.offset_x;
		offset_y = previous_state.offset_y;
		set_offsets(previous_state.offset_x, previous_state.offset_y);
	}
}
float DocumentView::get_page_offset(int page) {

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
	set_offset_y(get_page_offset(page));
}
void DocumentView::render_page(int page_number, GLuint rendered_program, GLuint unrendered_program) {

	GLuint texture = pdf_renderer->find_rendered_page(current_document->get_path(), page_number, zoom_level, nullptr, nullptr);

	float page_vertices[4 * 2];
	fz_rect page_rect = { 0, 0, current_document->get_page_width(page_number), current_document->get_page_height(page_number) };
	fz_rect window_rect = document_to_window_rect(page_number, page_rect);
	rect_to_quad(window_rect, page_vertices);

	if (texture != 0) {
		glUseProgram(rendered_program);
		glBindTexture(GL_TEXTURE_2D, texture);
	}
	else {
		glUseProgram(unrendered_program);
	}
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glBufferData(GL_ARRAY_BUFFER, sizeof(page_vertices), page_vertices, GL_DYNAMIC_DRAW);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}
void DocumentView::render(GLuint rendered_program, GLuint unrendered_program, GLuint highlight_program) {

	vector<int> visible_pages;
	get_visible_pages(view_height, visible_pages);
	render_is_invalid = false;

	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);


	for (int page : visible_pages) {
		render_page(page, rendered_program, unrendered_program);
		fz_link* links = current_document->get_page_links(page);
		while (links != nullptr) {
			render_highlight_document(highlight_program, page, links->rect);
			links = links->next;
		}
	}

	if (search_results.size() > 0) {
		SearchResult current_search_result = search_results[current_search_result_index];
		render_highlight_document(highlight_program, current_search_result.page, current_search_result.rect);
	}
}
void DocumentView::persist() {
	update_book(database, current_document->get_path(), zoom_level, offset_x, offset_y);
}
void DocumentView::get_text_selection(fz_rect abs_docspace_rect, vector<fz_rect>& selected_characters) {

	int page_begin, page_end;
	fz_rect page_rect;

	current_document->absolute_to_page_pos(abs_docspace_rect.x0, abs_docspace_rect.y0, &page_rect.x0, &page_rect.y0, &page_begin);
	current_document->absolute_to_page_pos(abs_docspace_rect.x1, abs_docspace_rect.y1, &page_rect.x1, &page_rect.y1, &page_end);

	fz_page* page = nullptr;
	fz_stext_page* stext_page = nullptr;

	string selected_text;
	for (int i = page_begin; i <= page_end; i++) {
		fz_try(mupdf_context) {
			page = fz_load_page(mupdf_context, current_document->doc, i);
			stext_page = fz_new_stext_page_from_page(mupdf_context, page, nullptr);
		}
		fz_catch(mupdf_context) {
			cout << "Error: could not load page for selection" << endl;
		}


		fz_stext_block* current_block = stext_page->first_block;
		while (current_block) {

			if (current_block->type != FZ_STEXT_BLOCK_TEXT) {
				continue;
			}

			fz_stext_line* current_line = current_block->u.t.first_line;
			while (current_line) {
				fz_stext_char* current_char = current_line->first_char;
				while (current_char) {
					fz_rect charrect = current_document->page_rect_to_absolute_rect(i, fz_rect_from_quad(current_char->quad));
					//if (includes_rect(document_space_rect, charrect)) {
					if ((abs_docspace_rect.y0 <= charrect.y0 && abs_docspace_rect.y1 >= charrect.y1) ||
						(abs_docspace_rect.y1 <= charrect.y1 && abs_docspace_rect.y1 >= charrect.y0 && abs_docspace_rect.x1 >= charrect.x1) ||
						(abs_docspace_rect.y0 <= charrect.y1 && abs_docspace_rect.y0 >= charrect.y0 && abs_docspace_rect.x0 <= charrect.x0)
						) {
						selected_text.push_back(current_char->c);
						selected_characters.push_back(charrect);
					}
					current_char = current_char->next;
				}
				//some_text.push_back('\n');
				current_line = current_line->next;
			}

			current_block = current_block->next;
		}

		fz_drop_stext_page(mupdf_context, stext_page);
		fz_drop_page(mupdf_context, page);
	}

	cout << "selecting pages " << selected_text << page_begin << " " << page_end << endl;
}
